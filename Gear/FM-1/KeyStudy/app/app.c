/* SPDX-License-Identifier: MPL-2.0 */
#include "fastmath.h"
#include "theory.h"
#include "library_theory.h"
#include "preferences.h"

enum { FIRST_KEY = 53, LAST_KEY = FIRST_KEY + NKEYS - 1, CHORD_SETTLE_MS = 90 };
enum { PLAY_FROM_NAME, IDENTIFY_BY_EAR };
enum { READY, WRONG, CORRECT, SKIPPED };
enum { DEMO_OFF, DEMO_BLOCK, DEMO_STEPS };

typedef struct {
    uint32_t word[4];
} NoteSet;

typedef struct {
    int root;
    int quality;
} Question;

static struct {
    int practice;
    int topic, learn_topic, practice_topic, practice_started, practice_refresh;
    Question lesson[TOPIC_COUNT];
    Question question;
    int direction;
    int mixed_chords;
    uint32_t mixed_roots, mixed_qualities;
    int choice;
    int collect;
    int hinted;
    int missed;
    int result;
    int completed;
    int correct;
    int first_try;
    int position;
    int starting_note;
    int wait_release;
    int entry_changed;
    int dirty;
    uint32_t collected;
    uint32_t checked;
    uint32_t stable_since;
    uint32_t rng;
    uint32_t panel;
    uint32_t buttons;
    uint32_t shared_usb_link;
    uint32_t held_pitches;
    uint8_t pitch_count[12];
    uint16_t midi[NPORTS][128];
    NoteSet held;
    NoteSet demo_notes;
    int demo;
    uint32_t demo_at;
    char notice[50];
    uint32_t notice_at;
} study;

#include "library_state.h"

static struct { int open, page; } guide;
static NoteSet sound_notes;

static int keys_any(const NoteSet *notes)
{
    return (notes->word[0] | notes->word[1] | notes->word[2] | notes->word[3]) != 0;
}

static int key_contains(const NoteSet *notes, int note)
{
    return (notes->word[note / 32] >> (note % 32)) & 1u;
}

static void key_add(NoteSet *notes, int note)
{
    notes->word[note / 32] |= 1u << (note % 32);
}

static int is_chord(void) { return study.topic == TRIADS || study.topic == SEVENTHS; }
static int ear_mode(void) { return study.practice && study.direction == IDENTIFY_BY_EAR; }
static int round_done(void)
{
    return study.practice && study.completed >= prefs.length && study.result == READY;
}
static int question_open(void)
{
    return study.practice && !ear_mode() && !round_done()
           && study.result != CORRECT && study.result != SKIPPED && !study.wait_release;
}

static Question current(void)
{
    return study.practice ? study.question : study.lesson[study.topic];
}

static const Shape *practice_shape(int quality)
{
    if (study.practice && study.mixed_chords)
        return chord_shape(quality);
    return topics[study.topic].shapes + quality;
}

static uint32_t practice_filter(void)
{
    return study.mixed_chords ? prefs.chords : topic_filter(study.topic);
}

static const Shape *current_shape(void)
{
    return practice_shape(current().quality);
}

static int wrap_index(int index, int count)
{
    index %= count;
    return index < 0 ? index + count : index;
}

static char *append_text(char *out, const char *value)
{
    while (*value)
        *out++ = *value++;
    *out = 0;
    return out;
}

static char *append_number(char *out, int n)
{
    if (n >= 10)
        *out++ = (char)('0' + n / 10);
    *out++ = (char)('0' + n % 10);
    *out = 0;
    return out;
}

static char *append_total(char *out, int value)
{
    return append_number(append_text(append_number(out, value), "/"), prefs.length);
}

static void question_title(char out[24])
{
    Question q = current();
    const Shape *shape = current_shape();
    char *p = append_text(out, root_name[q.root]);
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
    return FIRST_KEY + wrap_index(current().root - FIRST_KEY, 12);
}

static void stop_demo(void)
{
    study.demo = DEMO_OFF;
    study.demo_notes = (NoteSet){{0}};
}

static void start_demo(uint32_t now, int broken)
{
    int together = is_chord() || (study.topic == INTERVALS && ear_mode() && prefs.order == HEAR_TOGETHER);
    study.demo = broken || !together ? DEMO_STEPS : DEMO_BLOCK;
    study.demo_at = now;
}

static void update_demo(uint32_t now)
{
    static const uint32_t step_ms = 285;
    NoteSet next = {{0}};
    if (study.demo) {
        const Shape *shape = current_shape();
        uint32_t age = now - study.demo_at;
        int root = voiced_root();
        if (study.demo == DEMO_BLOCK && age < 870) {
            for (int i = 0; i < shape->count; ++i)
                key_add(&next, root + shape->semitones[i]);
        } else if (study.demo == DEMO_STEPS && age / step_ms < shape->count) {
            if (age % step_ms < 220) {
                unsigned step = age / step_ms;
                if (study.topic == INTERVALS && ear_mode() && prefs.order == HEAR_DESCENDING)
                    step = shape->count - 1u - step;
                key_add(&next, root + shape->semitones[step]);
            }
        } else {
            study.demo = DEMO_OFF;
        }
    }
    for (int i = 0; i < 4; ++i) {
        if (next.word[i] != study.demo_notes.word[i])
            study.dirty = 1;
    }
    study.demo_notes = next;
}

static void clear_answer(uint32_t now)
{
    study.position = 0;
    study.collected = 0;
    study.checked = 0;
    study.stable_since = now;
    study.entry_changed = 0;
    study.wait_release = 1;
    if (study.result == WRONG)
        study.result = READY;
}

static void prepare_question(uint32_t now)
{
    study.result = READY;
    study.hinted = 0;
    study.missed = 0;
    study.choice = mask_choice(practice_filter(), 0);
    study.dirty = 1;
    stop_demo();
    clear_answer(now);
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

static int unused_pick(uint32_t mask, uint32_t *used)
{
    uint32_t available = mask & ~*used;
    if (!available) {
        *used = 0;
        available = mask;
    }
    int choice = mask_choice(available, next_random());
    *used |= 1u << choice;
    return choice;
}

static void mixed_question(void)
{
    int root = unused_pick(0xFFFu, &study.mixed_roots);
    int quality = unused_pick(prefs.chords, &study.mixed_qualities);
    study.question = (Question){root, quality};
}

static Question quick_question(uint32_t random)
{
    uint32_t qualities = practice_filter();
    int count = mask_count(qualities);
    int root = mask_choice(quiz_roots(study.topic == NOTES), random / (uint32_t)count);
    return (Question){root, mask_choice(qualities, random)};
}

static void start_round(uint32_t now, int from_lesson)
{
    study.completed = 0;
    study.correct = 0;
    study.first_try = 0;
    if (study.mixed_chords) {
        study.mixed_roots = study.mixed_qualities = 0;
        mixed_question();
    } else {
        uint32_t random = next_random();
        Question lesson = study.lesson[study.topic];
        int permitted = (practice_filter() & (1u << lesson.quality)) &&
                        (quiz_roots(study.topic == NOTES) & (1u << lesson.root));
        study.question = from_lesson && permitted && !ear_mode() ? lesson : quick_question(random);
    }
    prepare_question(now);
}

static void next_question(uint32_t now)
{
    if (study.completed < prefs.length && study.mixed_chords) {
        mixed_question();
    } else if (study.completed < prefs.length) {
        Question next = quick_question(next_random());
        if (next.root == study.question.root && next.quality == study.question.quality)
            next.root = mask_step(quiz_roots(study.topic == NOTES), next.root, 1);
        study.question = next;
    }
    prepare_question(now);
}

static void conclude_answer(int result)
{
    ++study.completed;
    study.result = result;
    if (result == CORRECT) {
        ++study.correct;
        if (!study.missed && !study.hinted)
            ++study.first_try;
    }
    study.dirty = 1;
    stop_demo();
}

static void mark_wrong(void)
{
    study.result = WRONG;
    study.missed = 1;
    study.dirty = 1;
}

static int is_held(int note)
{
    int key = note - FIRST_KEY;
    return ((unsigned)key < NKEYS && (study.panel & (1u << key)))
           || study.midi[PORT_USB][note] || study.midi[PORT_TRS][note];
}

static void answer_note(int note, uint32_t now)
{
    if (!question_open() || study.demo)
        return;

    const Shape *shape = current_shape();
    if (is_chord()) {
        study.result = READY;
        if (study.collect)
            study.collected ^= pitch_bit(note);
        study.stable_since = now;
        study.dirty = 1;
        return;
    }

    if (!study.position) {
        if (note % 12 != study.question.root || note + shape->semitones[shape->count - 1] > 127) {
            mark_wrong();
            return;
        }
        study.starting_note = note;
    } else if (note != study.starting_note + shape->semitones[study.position]) {
        mark_wrong();
        study.position = 0;
        return;
    }
    study.result = READY;
    ++study.position;
    study.dirty = 1;
    if (study.position == shape->count)
        conclude_answer(CORRECT);
}

static void update_held(int note, uint32_t now)
{
    int down = is_held(note);
    if (down == key_contains(&study.held, note))
        return;
    int pitch = note % 12;
    if (down) {
        key_add(&study.held, note);
        if (!study.pitch_count[pitch]++)
            study.held_pitches |= 1u << pitch;
        if (!settings.open) {
            if (library.page != LIB_CLOSED)
                library_key(note);
            else
                answer_note(note, now);
        }
    } else {
        study.held.word[note / 32] &= ~(1u << (note % 32));
        if (!--study.pitch_count[pitch])
            study.held_pitches &= ~(1u << pitch);
    }
    study.stable_since = now;
    study.entry_changed = 1;
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

static void read_keyboard(uint32_t now)
{
    uint32_t keys = plat_keys(), changed = keys ^ study.panel;
    study.panel = keys;
    for (int k = 0; k < NKEYS; ++k) {
        if (changed & (1u << k))
            update_held(FIRST_KEY + k, now);
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
    if (!keys_any(&study.held)) {
        study.wait_release = 0;
        library.wait_release = 0;
    }
}

static uint32_t chord_entry(void)
{
    return study.collect ? study.collected : study.held_pitches;
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

static void check_chord(int submit)
{
    uint32_t entry = chord_entry();
    const Shape *shape = current_shape();
    if (entry == shape_pitches(current().root, shape)) {
        conclude_answer(CORRECT);
    } else if (entry && (submit || pitch_total(entry) >= shape->count)
               && (study.result != WRONG || entry != study.checked)) {
        study.checked = entry;
        mark_wrong();
    }
}

#include "library.h"
#include "settings.h"
#include "guides.h"

static void read_controls(uint32_t now)
{
    uint32_t buttons = plat_buttons();
    uint32_t press = buttons & ~study.buttons;
    study.buttons = buttons;
    int topic = plat_enc(EN_SELECT);
    int algo = plat_enc(EN_ALGO);
    int preset = plat_enc(EN_PRESET);
    if (press & ((1u << B_HOME) | (1u << B_SEQ))) {
        settings.open = 0;
        guide.open = 0;
        library.page = LIB_CLOSED;
        library_silence();
        stop_demo();
        study.entry_changed = 0;
        study.wait_release = 1;
        study.dirty = 1;
        if (press & (1u << B_HOME)) {
            if (study.practice) {
                study.practice_topic = study.topic;
                study.topic = study.learn_topic;
                study.practice = 0;
            }
        } else {
            if (!study.practice) {
                study.learn_topic = study.topic;
                study.topic = study.practice_topic;
                study.practice = 1;
            }
            if (!study.practice_started || study.practice_refresh) {
                if (!study.practice_started || !practice_filter())
                    study.mixed_chords = is_chord();
                if (!study.practice_started)
                    study.direction = PLAY_FROM_NAME;
                study.practice_started = 1;
                study.practice_refresh = 0;
                start_round(now, 0);
            }
        }
        return;
    }
    if (settings_controls(press, now, topic, preset) || guide_controls(press, now, topic) ||
        library_controls(press, now, topic, algo, preset))
        return;
    if (topic) {
        study.mixed_chords = 0;
        if (study.practice) {
            uint32_t topics_available = 0;
            for (int i = 0; i < TOPIC_COUNT; ++i)
                if (topic_filter(i))
                    topics_available |= 1u << i;
            study.topic = mask_step(topics_available, study.topic, topic);
        } else {
            study.topic = wrap_index(study.topic + topic, TOPIC_COUNT);
        }
        if (study.practice && study.topic == NOTES)
            study.direction = PLAY_FROM_NAME;
        if (study.practice)
            start_round(now, 1);
        study.dirty = 1;
        stop_demo();
    }
    if (!study.practice) {
        Question *q = &study.lesson[study.topic];
        if (algo || preset) {
            q->quality = wrap_index(q->quality + algo, topics[study.topic].count);
            q->root = wrap_index(q->root + preset, 12);
            stop_demo();
            study.dirty = 1;
        }
        if (press & ((1u << B_PLAY) | (1u << B_ARP)))
            start_demo(now, (press & (1u << B_ARP)) != 0);
        return;
    }

    if (study.topic != NOTES && algo % 2) {
        study.direction ^= 1;
        start_round(now, 0);
    }
    if (ear_mode() && preset && !round_done() && study.result != CORRECT && study.result != SKIPPED) {
        study.choice = mask_step(practice_filter(), study.choice, preset);
        study.result = READY;
        study.dirty = 1;
    }
    if (press & (1u << B_EDIT)) {
        if (round_done()) {
            start_round(now, 0);
            return;
        }
        if (study.result == CORRECT || study.result == SKIPPED) {
            next_question(now);
            return;
        }
        if (ear_mode()) {
            if (study.choice == study.question.quality)
                conclude_answer(CORRECT);
            else
                mark_wrong();
        } else if (question_open() && is_chord() && !study.demo) {
            check_chord(1);
        }
    }
    if (round_done())
        return;
    if ((press & (1u << B_FX)) && study.result != CORRECT && study.result != SKIPPED) {
        study.hinted = 1;
        study.dirty = 1;
    }
    if ((press & (1u << B_SAVE)) && study.result != CORRECT && study.result != SKIPPED)
        conclude_answer(SKIPPED);
    if (press & (1u << B_PLAY)) {
        if (!ear_mode() && study.result != CORRECT && study.result != SKIPPED) {
            study.hinted = 1;
            clear_answer(now);
        }
        start_demo(now, 0);
    }
    if ((press & (1u << B_ARP)) && is_chord() && !ear_mode()) {
        study.collect ^= 1;
        clear_answer(now);
        study.dirty = 1;
    }
    if ((press & (1u << B_REC)) && question_open() && is_chord() && study.collect) {
        clear_answer(now);
        study.dirty = 1;
    }
}

static void tick_study(void)
{
    uint32_t now = plat_ms();
    read_controls(now);
    read_keyboard(now);
    if (settings.open) {
        study.demo_notes = (NoteSet){{0}};
    } else if (library.page != LIB_CLOSED || guide.open)
        library_audio(now);
    else
        update_demo(now);
    if (!settings.open && library.page == LIB_CLOSED && study.entry_changed && question_open() && is_chord() && !study.demo
        && now - study.stable_since >= CHORD_SETTLE_MS) {
        study.entry_changed = 0;
        check_chord(0);
    }

    NoteSet next = study.held;
    for (int i = 0; i < 4; ++i)
        next.word[i] |= study.demo_notes.word[i];
    for (int i = 0; i < 4; ++i) {
        if (sound_notes.word[i] != next.word[i]) {
            uint32_t lock = plat_lock();
            sound_notes = next;
            plat_unlock(lock);
            break;
        }
    }
}

#include "sound.h"

void app_init(void)
{
    load_preferences();
    guide.open = guide.page = 0;
    study.topic = study.learn_topic = study.practice_topic = SEVENTHS;
    study.lesson[SEVENTHS] = (Question){11, 1};
    study.rng = 0x3BE5F4A1u;
    sound_init();
}

#include "screen.h"
