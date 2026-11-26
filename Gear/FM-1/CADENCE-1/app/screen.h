/* SPDX-License-Identifier: MPL-2.0 */
#define STUDY_BG RGB(14, 21, 27)
#define STUDY_CARD RGB(24, 34, 41)
#define STUDY_LINE RGB(50, 69, 77)
#define STUDY_INK RGB(238, 247, 241)
#define STUDY_MUTED RGB(157, 179, 182)
#define STUDY_MINT RGB(125, 225, 194)
#define STUDY_GOLD RGB(255, 202, 113)
#define STUDY_RED RGB(255, 151, 134)

enum { TOP_HEIGHT = 88, LOWER_HEIGHT = 240 - TOP_HEIGHT };

#include "staff.h"

/* Labels and hints are set in capitals; names and answers keep their case (m2 is not M2). */
static void caps_copy(char out[48], const char *text)
{
    int i = 0;
    for (; text[i] && i < 47; ++i)
        out[i] = text[i] >= 'a' && text[i] <= 'z' ? (char)(text[i] - 32) : text[i];
    out[i] = 0;
}

static int32_t cv_caps(int32_t x, int32_t y, const felucca_font_t *font, const char *text, uint16_t color)
{
    char upper[48];
    caps_copy(upper, text);
    return cv_text(x, y, font, upper, color);
}

static void text_right(int x, int y, const felucca_font_t *font, const char *value, uint16_t color)
{
    char upper[48];
    caps_copy(upper, value);
    cv_text(x - text_w(font, upper), y, font, upper, color);
}

static void tab(int x, const char *label, const char *button, int selected)
{
    cv_rect(x, 7, 106, 21, selected ? STUDY_MINT : STUDY_CARD);
    cv_text(x + 7, 10, &FONT_XS, label, selected ? STUDY_BG : STUDY_MUTED);
    cv_text(x + 98 - text_w(&FONT_T, button), 12, &FONT_T, button, selected ? STUDY_BG : STUDY_MUTED);
}

/* The top of every Learn and Practice page: the two tabs, a heading, and the card's frame. */
static void draw_header(const char *heading, const char *eyebrow, uint16_t accent)
{
    cv_begin(240, TOP_HEIGHT, STUDY_BG);
    tab(11, "LEARN", "HOME", !study.practice);
    tab(123, "PRACTICE", "SEQ", study.practice);
    cv_text(12, 30, &FONT_B, heading, STUDY_INK);
    text_right(228, 32, &FONT_T, eyebrow, STUDY_MUTED);
    cv_rect(11, 48, 218, 40, STUDY_CARD);
    cv_rect(11, 48, 2, 40, accent);
}

/* The card's second line: a long rule steps down to the small face rather than run off the card. */
static void draw_card_detail(const char *detail, uint16_t color)
{
    cv_text(21, 71, text_w(&FONT_XS, detail) > 206 ? &FONT_T : &FONT_XS, detail, color);
}

static void draw_card_text(const char *title, const char *detail, uint16_t detail_color, int room)
{
    const felucca_font_t *face = text_w(&FONT_M, title) > room ? &FONT_B : &FONT_M;
    cv_text(21, face == &FONT_M ? 48 : 54, face, title, STUDY_INK);
    draw_card_detail(detail, detail_color);
}

static uint32_t learn_guides(void)
{
    const Shape *shape = current_shape();
    uint32_t keys = 0;
    for (int i = 0; i < shape->count; ++i)
        keys |= panel_bit(voiced_root() + shape->semitones[i]);
    return keys;
}

static uint32_t demo_lights(void)
{
    uint32_t lights = 0;
    for (int k = 0; k < NKEYS; ++k)
        if (key_contains(&study.demo_notes, base_key() + k))
            lights |= 1u << k;
    return lights;
}

static void draw_key(int x, int y, int w, int h, int k, int black, const uint32_t marks[4])
{
    int note = base_key() + k;
    uint32_t bit = 1u << k;
    uint16_t color = black ? STUDY_BG : RGB(193, 207, 205);
    if (marks[0] & bit)
        color = RGB(72, 147, 130);
    if (marks[2] & bit)
        color = STUDY_GOLD;
    if (key_contains(&study.held, note) || (marks[1] & bit))
        color = STUDY_MINT;
    if (marks[3] & bit)
        color = RGB(226, 84, 70);

    cv_rect(x, y, w, h, black ? STUDY_LINE : STUDY_BG);
    cv_rect(x + 1, y + 1, w - 2, h - 2, color);
    if (!black && note % 12 == 0) {
        char name[4] = "C";
        append_number(name + 1, note / 12 - 1);
        cv_text(x + 2, y + h - 13, &FONT_T, name, STUDY_BG);
    }
}

/* The panel's 27 keys. Each mask is a bit per key: the answer's keys, keys sounding, keys marked
 * as a starting point or gathered so far, and keys held in error. */
static void draw_keyboard(int y, int height, uint32_t guides, uint32_t heard, uint32_t anchors, uint32_t wrong)
{
    static const uint8_t whites[] = PLAT_WHITE_KEYS;
    static const uint8_t blacks[] = PLAT_BLACK_KEYS;
    static const uint8_t after[] = {0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14};
    const uint32_t marks[4] = {guides, heard, anchors, wrong};
    for (int i = 0; i < 16; ++i)
        draw_key(8 + i * 14, y, 14, height, whites[i], 0, marks);
    for (int i = 0; i < 11; ++i)
        draw_key(17 + after[i] * 14, y, 10, height * 3 / 5, blacks[i], 1, marks);
}

static void control_label(int x, int y, const char *control, const char *action)
{
    x = cv_text(x, y, &FONT_T, control, STUDY_MINT);
    cv_caps(x + 4, y, &FONT_T, action, STUDY_MUTED);
}

static void draw_score_readout(const char *line, uint16_t color)
{
    const felucca_font_t *face = text_w(&FONT_B, line) <= 216 ? &FONT_B : &FONT_XS;
    cv_text(12, SCORE_HEIGHT + 1, face, line, color);
}

static void draw_notice(void)
{
    if (study.notice[0]) {
        cv_rect(8, 121, 224, 29, STUDY_CARD);
        cv_text(13, 128, &FONT_XS, study.notice, STUDY_GOLD);
    }
}

static void learn_lights(void)
{
    uint32_t buttons = (1u << B_HOME) | (demo_playing() ? LED_PLAY_GREEN : 0);
    uint32_t controls = (1u << B_PLAY) | (is_chord() ? 1u << B_ARP : 0);
    plat_lights(buttons, study.panel | demo_lights(), controls, learn_guides(), 2);
}

static void learn_top(void)
{
    char title[24];
    draw_header(topics[study.topic].label, "Reference", STUDY_MINT);
    question_title(title);
    draw_card_text(title, current_shape()->name, STUDY_MUTED, 202);
    cv_blit(0, 0);
}

static void learn_bottom(void)
{
    const Shape *shape = current_shape();
    StaffNote notes[8], root = shape_root(voiced_root(), shape);
    char line[40];
    for (int i = 0; i < shape->count; ++i)
        notes[i] = shape_note(root, shape, i);
    cv_begin(240, LOWER_HEIGHT, STUDY_BG);
    draw_notation(&(Score){.notes = notes, .count = shape->count, .stacked = is_chord(),
                           .filled = (1u << shape->count) - 1u, .cursor = -1, .live = 1});
    tone_list(line);
    draw_score_readout(line, STUDY_MINT);
    draw_keyboard(SCORE_KEYS_Y, 117 - SCORE_KEYS_Y, learn_guides(), demo_lights(), 0, 0);
    cv_rect(11, 118, 218, 1, STUDY_LINE);
    control_label(11, 120, "SELECT", "Topic");
    control_label(94, 120, "ALGO", "Type");
    control_label(169, 120, "PRESET", "Root");
    control_label(11, 130, "PLAY", "Hear");
    if (is_chord())
        control_label(94, 130, "ARP", "Roll");
    else
        control_label(94, 130, "OCT", "Octave");
    control_label(169, 130, "ENV", "Settings");
    cv_text(11, 139, &FONT_XS, shape->formula, STUDY_INK);
    control_label(130, 140, "SEL", "Guide");
    control_label(181, 140, "GLO", "Tools");
    draw_notice();
    cv_blit(0, TOP_HEIGHT);
}

#include "library_screen.h"
#include "calc_screen.h"
#include "settings_screen.h"
#include "guide_screen.h"

void app_say(const char *a, const char *b)
{
    char *out = study.notice;
    const char *parts[] = {a, b};
    for (int part = 0; part < 2; ++part) {
        const char *p = parts[part];
        if (!p || !*p)
            continue;
        if (part && out != study.notice)
            *out++ = ' ';
        while (*p && out < study.notice + sizeof study.notice - 2)
            *out++ = *p++;
    }
    *out = 0;
    study.notice_at = plat_ms();
    study.dirty = 1;
    if (ann_on())
        ann_say(study.notice, 0, ANN_NOW);
}

static void announce_learn(uint32_t now)
{
    if (ann_on() && ann_where()) {
        char speech[120], line[40], title[24];
        char *p = append_text(append_text(speech, "LEARN. "), topics[study.topic].label);
        question_title(title);
        p = append_text(append_text(p, ". "), title);
        tone_list(line);
        append_text(append_text(p, ". "), line);
        ann_say(speech, 0, ANN_NOW);
    }
    ann_tick(now);
}

void app_ui_init(void)
{
    stir_random(plat_ms());
    study.dirty = 1;
}

void app_input_only(void) { tick_study(); }

void app_frame(void)
{
    tick_study();
    uint32_t now = plat_ms();
    if (study.notice[0] && now - study.notice_at >= 1500u) {
        study.notice[0] = 0;
        study.dirty = 1;
    }
    if (settings.open) {
        settings_frame(now);
    } else if (study.practice) {
        library_frame(now);
    } else if (guide.open) {
        guide_frame(now);
    } else if (calc.page != LIB_CLOSED) {
        calc_frame(now);
    } else {
        announce_learn(now);
        if (study.dirty) {
            study.dirty = 0;
            learn_lights();
            learn_top();
            learn_bottom();
        }
    }
}

#ifdef PLAT_HOST
/* What a power cycle forgets: everything but the store. */
void app_host_reboot(void)
{
    memset(&study, 0, sizeof study);
    memset(&demo, 0, sizeof demo);
    memset(&guide, 0, sizeof guide);
    memset(&library, 0, sizeof library);
    memset(&calc, 0, sizeof calc);
    memset(&settings, 0, sizeof settings);
    memset(&sound_notes, 0, sizeof sound_notes);
}

static int host_fails;

int app_host_fails(void)
{
    return host_fails;
}

/* card TEXT / title TEXT: the practice card's second or first line, as last drawn, is TEXT. */
static int host_card_check(const char *cmd, const char *drawn)
{
    const char *want = plat_host_line + strlen(cmd);
    size_t length;
    while (*want == ' ')
        ++want;
    length = strlen(want);
    while (length && (want[length - 1] == '\n' || want[length - 1] == '\r' || want[length - 1] == ' '))
        --length;
    if (strlen(drawn) == length && !strncmp(drawn, want, length)) {
        printf("  ok %s \"%s\"\n", cmd, drawn);
    } else {
        printf("FAIL %s: \"%s\", wanted \"%.*s\"\n", cmd, drawn, (int)length, want);
        ++host_fails;
    }
    return 1;
}

/* Commands for the test scripts.  seed N: start the generator from N.  ask ITEM [N]: pose that
 * item of the open activity, N standing in for the dice. */
int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d)
{
    (void)c;
    (void)d;
    if (!strcmp(cmd, "card"))
        return host_card_check(cmd, drawn_detail);
    if (!strcmp(cmd, "title"))
        return host_card_check(cmd, drawn_title);
    if (!strcmp(cmd, "seed")) {
        study.rng = (uint32_t)strtoul(a, 0, 0);
        return 1;
    }
    if (!strcmp(cmd, "teach")) {             /* teach 0|1: new shapes shown before they are asked */
        teach_off = !atoi(a);
        return 1;
    }
    if (!strcmp(cmd, "activity")) {          /* activity GROUP ITEM: where SEQ opens (at once if open) */
        library.group = atoi(a);
        library.item = atoi(b);
        if (library.page != LIB_CLOSED)
            library_start(plat_ms());
        return 1;
    }
    if (!strcmp(cmd, "level")) {             /* level KIND N: that kind of material at level N */
        progress.level[atoi(a)] = (uint8_t)atoi(b);
        if (library.page != LIB_CLOSED)
            library_start(plat_ms());
        return 1;
    }
    if (!strcmp(cmd, "legacy")) {            /* legacy [INTERVALS]: settings as 0.4 saved them */
        uint16_t marked = a[0] ? (uint16_t)strtoul(a, 0, 0) : 0xFFFu;
        uint8_t old[12] = {1, CLEF_BASS, 12, 0, HEAR_DESCENDING, TONE_PURE,
                           (uint8_t)marked, (uint8_t)(marked >> 8), 0xFF, 0x0F, 0xFF, 0x07};
        plat_store_save(OBJ_PREFS, old, sizeof old);
        return 1;
    }
    if (!strcmp(cmd, "ask")) {
        library_pose(atoi(a), (uint32_t)strtoul(b, 0, 0), plat_ms());
        library.autoplay_at = 0;
        library.wait_release = keys_any(&study.held);
        return 1;
    }
    return 0;
}
#endif

int app_expect(const char *what, int *got)
{
#ifdef PLAT_HOST
    if (!strcmp(what, "practice")) *got = study.practice;
    else if (!strcmp(what, "topic")) *got = study.topic;
    else if (!strcmp(what, "octave")) *got = study.octave;
    else if (!strcmp(what, "base_key")) *got = base_key();
    else if (!strcmp(what, "guide_open")) *got = guide.open;
    else if (!strcmp(what, "guide_page")) *got = guide.page;
    else if (!strcmp(what, "shape_size")) *got = current_shape()->count;
    else if (!strcmp(what, "root")) *got = current().root;
    else if (!strcmp(what, "quality")) *got = current().quality;
    else if (!strcmp(what, "root_flat")) *got = root_is_flat(current().root, current_shape());
    else if (!strcmp(what, "held_pcs")) *got = (int)study.held_pitches;
    else if (!strcmp(what, "lowest_held")) *got = lowest_note(&study.held);
    else if (!strcmp(what, "rng")) *got = (int)(study.rng & 0x7FFFFFFFu);
    else if (!strcmp(what, "guides")) *got = (int)learn_guides();
    else if (!strcmp(what, "demo_lights")) *got = (int)demo_lights();
    else if (!strcmp(what, "demo")) *got = demo.count;
    else if (!strcmp(what, "progress_dirty")) *got = progress_dirty;
    else if (!strncmp(what, "level_", 6)) *got = progress.level[atoi(what + 6)];
    else if (settings_expect(what, got)) return 1;
    else if (calc_expect(what, got)) return 1;
    else return library_expect(what, got);
    return 1;
#else
    (void)what;
    (void)got;
    return 0;
#endif
}
