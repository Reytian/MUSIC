/* SPDX-License-Identifier: MPL-2.0 */
#include "fastmath.h"
#include "theory.h"
#include "library_theory.h"
#include "preferences.h"
#include "progress.h"

enum { PANEL_LOW = 53, CHORD_SETTLE_MS = 90 };
enum { READY, WRONG, CORRECT, SKIPPED, MISSED };
enum { DEMO_NOTES, DEMO_BLOCK, DEMO_CHORDS, DEMO_MELODY };
enum { DEMO_MAX = 40 };

typedef struct {
    uint32_t word[4];
} NoteSet;

typedef struct {
    int root;
    int quality;
} Question;

static struct {
    int practice;                 /* The PRACTICE tab is showing; the library holds its state. */
    int topic;
    Question lesson[TOPIC_COUNT];
    int octave;                   /* OCT- and OCT+, in octaves from the clef's own register. */
    int dirty;
    uint32_t rng;
    uint32_t panel;
    uint32_t buttons;
    uint32_t shared_usb_link;
    uint32_t held_pitches;
    uint32_t sound_at;
    uint8_t pitch_count[12];
    uint16_t midi[NPORTS][128];
    NoteSet held;
    NoteSet demo_notes;
    char notice[50];
    uint32_t notice_at;
} study;

/* Notes played for the player: steps in order, the notes of one step together. */
static struct {
    uint8_t note[DEMO_MAX], step[DEMO_MAX];
    int count, steps;
    uint32_t step_ms, gate_ms, at;
} demo;

static struct { int open, page; } guide;
static NoteSet sound_notes;

static int keys_any(const NoteSet *notes)
{
    return (notes->word[0] | notes->word[1] | notes->word[2] | notes->word[3]) != 0;
}

static int key_contains(const NoteSet *notes, int note)
{
    return (unsigned)note < 128u && ((notes->word[note / 32] >> (note % 32)) & 1u);
}

static void key_add(NoteSet *notes, int note)
{
    if ((unsigned)note < 128u)
        notes->word[note / 32] |= 1u << (note % 32);
}

static int same_notes(const NoteSet *a, const NoteSet *b)
{
    return a->word[0] == b->word[0] && a->word[1] == b->word[1] && a->word[2] == b->word[2] && a->word[3] == b->word[3];
}

static int pitch_total(uint32_t pitches)
{
    int count = 0;
    while (pitches) {
        pitches &= pitches - 1u;
        ++count;
    }
    return count;
}

static int note_total(const NoteSet *notes)
{
    return pitch_total(notes->word[0]) + pitch_total(notes->word[1]) + pitch_total(notes->word[2]) + pitch_total(notes->word[3]);
}

static int lowest_note(const NoteSet *notes)
{
    for (int note = 0; note < 128; ++note)
        if (key_contains(notes, note))
            return note;
    return -1;
}

/* The panel's lowest key. Bass clef reads an octave lower, so its notes sit on the staff. */
static int base_key(void)
{
    return PANEL_LOW + 12 * (study.octave - (prefs.clef == CLEF_BASS));
}

static int panel_key(int midi)
{
    return midi - base_key();
}

static uint32_t panel_bit(int midi)
{
    int key = panel_key(midi);
    return (unsigned)key < NKEYS ? 1u << key : 0;
}

static int wrap_index(int index, int count)
{
    index %= count;
    return index < 0 ? index + count : index;
}

/* The staff's bottom line, in steps from written middle C. */
static int staff_bottom(void)
{
    return prefs.clef == CLEF_BASS ? -10 : 2;
}

/* Move a chord or a scale by octaves until its lowest note is in the panel's first octave: every
 * close-position shape then fits the keys. */
static void fit_notes(StaffNote *notes, int count)
{
    int lowest = notes[0].midi, distance, octaves;
    for (int i = 1; i < count; ++i)
        if (notes[i].midi < lowest)
            lowest = notes[i].midi;
    distance = base_key() - lowest;
    octaves = distance >= 0 ? (distance + 11) / 12 : -(-distance / 12);
    for (int i = 0; i < count; ++i) {
        notes[i].midi += 12 * octaves;
        notes[i].position += 7 * octaves;
    }
}

static char *append_number(char *out, int n)
{
    char digits[6];
    int count = 0;
    if (n < 0)
        n = 0;
    do {
        digits[count++] = (char)('0' + n % 10);
        n /= 10;
    } while (n && count < 6);
    while (count)
        *out++ = digits[--count];
    *out = 0;
    return out;
}

static uint32_t next_random(void)
{
    uint32_t x = study.rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    study.rng = x;
    return x;
}

/* The moment of every press goes into the generator: no two sessions ask in the same order. */
static void stir_random(uint32_t now)
{
    study.rng ^= now * 2654435761u;
    if (!study.rng)
        study.rng = 0x3BE5F4A1u;
    next_random();
}

static void demo_clear(void)
{
    demo.count = demo.steps = 0;
    study.demo_notes = (NoteSet){{0}};
}

static void demo_add(int note, int step)
{
    if (demo.count < DEMO_MAX && (unsigned)note < 128u) {
        demo.note[demo.count] = (uint8_t)note;
        demo.step[demo.count++] = (uint8_t)step;
        if (step >= demo.steps)
            demo.steps = step + 1;
    }
}

static void demo_start(uint32_t now, int pace)
{
    static const uint16_t step_ms[] = {300, 900, 800, 420}, gate_ms[] = {225, 860, 730, 340};
    demo.step_ms = step_ms[pace];
    demo.gate_ms = gate_ms[pace];
    demo.at = now;
    study.dirty = 1;
}

static int demo_playing(void)
{
    return demo.count != 0;
}

static uint32_t demo_length(void)
{
    return (uint32_t)demo.steps * demo.step_ms;
}

static void demo_tick(uint32_t now)
{
    NoteSet next = {{0}};
    if (demo.count && demo.step_ms) {
        uint32_t age = now - demo.at, step = age / demo.step_ms;
        if (step >= (uint32_t)demo.steps) {
            demo.count = demo.steps = 0;
            study.dirty = 1;
        } else if (age % demo.step_ms < demo.gate_ms) {
            for (int i = 0; i < demo.count; ++i)
                if (demo.step[i] == step)
                    key_add(&next, demo.note[i]);
        }
    }
    if (!same_notes(&next, &study.demo_notes))
        study.dirty = 1;
    study.demo_notes = next;
}

static int is_chord(void) { return study.topic == TRIADS || study.topic == SEVENTHS; }

static Question current(void)
{
    return study.lesson[study.topic];
}

static const Shape *current_shape(void)
{
    return topics[study.topic].shapes + current().quality;
}

static void question_title(char out[24])
{
    const Shape *shape = current_shape();
    char *p = append_text(out, root_label(current().root, shape));
    if (study.topic == INTERVALS)
        p = append_text(p, " + ");
    append_text(p, shape->symbol);
}

static void tone_list(char out[40])
{
    const Shape *shape = current_shape();
    char *p = out;
    for (int i = 0; i < shape->count; ++i) {
        if (i)
            *p++ = ' ';
        spell_tone(p, current().root, shape, i);
        while (*p)
            ++p;
    }
}

static int voiced_root(void)
{
    return base_key() + wrap_index(current().root - base_key(), 12);
}

static void learn_demo(uint32_t now, int broken)
{
    const Shape *shape = current_shape();
    int together = is_chord() && !broken;
    demo_clear();
    for (int i = 0; i < shape->count; ++i)
        demo_add(voiced_root() + shape->semitones[i], together ? 0 : i);
    demo_start(now, together ? DEMO_BLOCK : DEMO_NOTES);
}

#include "library_state.h"
#include "calculators.h"

static int is_held(int note)
{
    return (study.panel & panel_bit(note)) || study.midi[PORT_USB][note] || study.midi[PORT_TRS][note];
}

static void update_held(int note, uint32_t now)
{
    if ((unsigned)note >= 128u)
        return;
    int down = is_held(note), pitch = note % 12;
    if (down == key_contains(&study.held, note))
        return;
    if (down) {
        key_add(&study.held, note);
        if (!study.pitch_count[pitch]++)
            study.held_pitches |= 1u << pitch;
        stir_random(now);
        if (!settings.open) {
            if (study.practice)
                library_key(note, now);
            else
                calc_key(note);
        }
    } else {
        study.held.word[note / 32] &= ~(1u << (note % 32));
        if (!--study.pitch_count[pitch])
            study.held_pitches &= ~(1u << pitch);
    }
    library.stable_since = now;
    library.entry_changed = 1;
    study.dirty = 1;
}

static void clear_midi_source(int port, uint16_t channels, uint32_t now)
{
    for (int i = 0; i < 128; ++i) {
        if (study.midi[port][i] & channels) {
            study.midi[port][i] &= (uint16_t)~channels;
            update_held(i, now);
        }
    }
}

/* The panel's keys change pitch with the register: let go of what they hold first. */
static void release_panel(uint32_t now)
{
    uint32_t keys = study.panel;
    int base = base_key();
    study.panel = 0;
    for (int k = 0; k < NKEYS; ++k)
        if (keys & (1u << k))
            update_held(base + k, now);
}

static void read_keyboard(uint32_t now)
{
    uint32_t keys = plat_keys(), changed = keys ^ study.panel;
    int base = base_key();
    study.panel = keys;
    for (int k = 0; k < NKEYS; ++k) {
        if (changed & (1u << k))
            update_held(base + k, now);
    }
    uint32_t link = plat_midi_out_link();
    if (plat_usb_lost() || link != study.shared_usb_link)
        clear_midi_source(PORT_USB, 0xFFFFu, now);
    study.shared_usb_link = link;

    uint32_t packet;
    int port;
    for (int i = 0; i < 64 && plat_midi_in(&packet, &port); ++i) {
        if (port != PORT_USB && port != PORT_TRS)
            continue;
        int command = (packet >> 8) & 0xF0u;
        uint16_t channel = (uint16_t)(1u << ((packet >> 8) & 15u));
        int note = (packet >> 16) & 127u;
        int velocity = (packet >> 24) & 127u;
        if (command == 0x90 && velocity) {
            study.midi[port][note] |= channel;
            update_held(note, now);
        } else if (command == 0x80 || command == 0x90) {
            study.midi[port][note] &= (uint16_t)~channel;
            update_held(note, now);
        } else if (command == 0xB0 && (note == 120 || note == 123)) {
            clear_midi_source(port, channel, now);
        }
    }
    if (!keys_any(&study.held))
        library.wait_release = 0;
}

#include "library.h"
#include "settings.h"
#include "guides.h"

static void read_controls(uint32_t now)
{
    uint32_t buttons = plat_buttons();
    uint32_t press = buttons & ~study.buttons;
    study.buttons = buttons;
    int select = plat_enc(EN_SELECT);
    int algo = plat_enc(EN_ALGO);
    int preset = plat_enc(EN_PRESET);
    int k1 = plat_enc(EN_K1), k2 = plat_enc(EN_K2);
    if (press)
        stir_random(now);

    if (press & ((1u << B_HOME) | (1u << B_SEQ))) {
        /* A note an outside keyboard never released would hold every question up: let go of them. */
        clear_midi_source(PORT_USB, 0xFFFFu, now);
        clear_midi_source(PORT_TRS, 0xFFFFu, now);
        settings.open = 0;
        guide.open = 0;
        calc.page = LIB_CLOSED;
        demo_clear();
        library.wait_release = 1;
        study.practice = (press & (1u << B_SEQ)) != 0;
        if (study.practice && library.page == LIB_CLOSED)
            library_start(now);
        study.dirty = 1;
        return;
    }
    if (settings_controls(press, now, select, preset))
        return;
    if (press & ((1u << B_OCTDN) | (1u << B_OCTUP))) {
        int octave = clamp(study.octave + ((press & (1u << B_OCTUP)) ? 1 : -1), -1, 1);
        if (octave != study.octave) {
            release_panel(now);
            study.octave = octave;
            demo_clear();
            library_register(now);
            calc_refresh();
        }
        app_say("Keys from", octave < 0 ? "one octave down" : octave ? "one octave up" : "the middle");
        return;
    }
    if (library_controls(press, now, select, algo, preset) || guide_controls(press, now, select) ||
        calc_controls(press, now, select, algo, preset, k1, k2))
        return;

    if (select) {
        study.topic = wrap_index(study.topic + select, TOPIC_COUNT);
        demo_clear();
        study.dirty = 1;
    }
    Question *q = &study.lesson[study.topic];
    if (algo || preset) {
        q->quality = wrap_index(q->quality + algo, topics[study.topic].count);
        q->root = wrap_index(q->root + preset, 12);
        demo_clear();
        study.dirty = 1;
    }
    if (press & ((1u << B_PLAY) | (1u << B_ARP)))
        learn_demo(now, (press & (1u << B_ARP)) != 0);
}

static void tick_study(void)
{
    uint32_t now = plat_ms();
    read_controls(now);
    read_keyboard(now);
    if (settings.open)
        demo_clear();
    demo_tick(now);
    library_tick(now);

    NoteSet next = study.held;
    for (int i = 0; i < 4; ++i)
        next.word[i] |= study.demo_notes.word[i];
    if (keys_any(&next))
        study.sound_at = now;
    if (now - study.sound_at > 1500u && progress_save_due(now, library_answering()))
        save_progress(now);
    if (!same_notes(&sound_notes, &next)) {
        uint32_t lock = plat_lock();
        sound_notes = next;
        plat_unlock(lock);
    }
}

#include "sound.h"

void app_init(void)
{
    load_preferences();
    load_progress();
    study.topic = SEVENTHS;
    study.lesson[SEVENTHS] = (Question){11, 1};
    study.rng = 0x3BE5F4A1u;
    library.group = LIB_NAME;
    library.item = 3;                 /* SEQ opens on chords, played from their names */
    library.last_item = -1;
    sound_init();
}

#include "screen.h"
