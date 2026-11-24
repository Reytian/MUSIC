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

static int answer_visible(void)
{
    return !study.practice || study.hinted || study.result == CORRECT || study.result == SKIPPED;
}

static int score_visible(void)
{
    return answer_visible() && !round_done();
}

static uint32_t key_guides(void)
{
    if (!score_visible())
        return 0;

    const Shape *shape = current_shape();
    int root = voiced_root();
    uint32_t keys = 0;
    for (int i = 0; i < shape->count; ++i)
        keys |= 1u << (root + shape->semitones[i] - FIRST_KEY);
    return keys;
}

static uint32_t demo_lights(void)
{
    uint32_t lights = 0;
    if (!ear_mode() || answer_visible()) {
        for (int k = 0; k < NKEYS; ++k) {
            if (key_contains(&study.demo_notes, FIRST_KEY + k))
                lights |= 1u << k;
        }
    }
    return lights;
}

static void update_lights(void)
{
    uint32_t buttons = 1u << (study.practice ? B_SEQ : B_HOME);
    if (study.demo || (study.practice && study.result == CORRECT))
        buttons |= LED_PLAY_GREEN;
    uint32_t controls = 1u << B_PLAY;
    if (study.practice)
        controls |= (1u << B_EDIT) | (1u << B_FX) | (1u << B_SAVE);
    if (question_open() && is_chord() && study.collect)
        controls |= 1u << B_REC;
    if ((!study.practice && is_chord()) || (study.practice && is_chord() && !ear_mode()))
        controls |= 1u << B_ARP;
    plat_lights(buttons, study.panel | demo_lights(), controls, key_guides(), 2);
}

static void tab(int x, const char *label, const char *button, int selected)
{
    cv_rect(x, 7, 106, 21, selected ? STUDY_MINT : STUDY_CARD);
    cv_text(x + 7, 10, &FONT_XS, label, selected ? STUDY_BG : STUDY_MUTED);
    cv_text(x + 98 - text_w(&FONT_T, button), 12, &FONT_T, button, selected ? STUDY_BG : STUDY_MUTED);
}

static void text_right(int x, int y, const felucca_font_t *font, const char *value, uint16_t color)
{
    cv_text(x - text_w(font, value), y, font, value, color);
}

static void screen_top(void)
{
    const Shape *shape = current_shape();
    char title[24], counter[20];
    const char *eyebrow = study.practice ? (ear_mode() ? "IDENTIFY BY EAR" : "PLAY FROM NAME") : "REFERENCE";
    const char *detail = shape->name;
    uint16_t detail_color = STUDY_MUTED;

    cv_begin(240, TOP_HEIGHT, STUDY_BG);
    tab(11, "LEARN", "HOME", !study.practice);
    tab(123, "PRACTICE", "SEQ", study.practice);
    cv_text(12, 30, &FONT_B, study.practice && study.mixed_chords ? "Chord Practice" : topics[study.topic].label, STUDY_INK);
    text_right(228, 32, &FONT_T, eyebrow, STUDY_MUTED);
    cv_rect(11, 48, 218, 40, STUDY_CARD);
    cv_rect(11, 48, 2, 40, STUDY_MINT);

    question_title(title);
    if (round_done()) {
        append_text(title, "Round complete");
        detail = "First try, without hints";
    } else if (ear_mode() && !answer_visible()) {
        append_text(title, "Listen");
        detail = practice_shape(study.choice)->name;
    } else if (study.practice && !answer_visible()) {
        detail = study.topic == INTERVALS || study.topic == SCALES ? "Play in ascending order" : "Play the notes on the keys";
        if (is_chord())
            detail = study.collect ? "Tap notes to add or remove" : "Hold all chord tones";
    }

    if (study.practice && !round_done()) {
        if (study.result == CORRECT) {
            detail = "Correct";
            detail_color = STUDY_MINT;
        } else if (study.result == WRONG && !ear_mode()) {
            detail = "Try again";
            detail_color = STUDY_RED;
        } else if (study.result == SKIPPED) {
            detail = "Skipped";
            detail_color = STUDY_GOLD;
        }
        int number = study.completed + (study.result != CORRECT && study.result != SKIPPED);
        append_total(counter, number);
        text_right(221, 53, &FONT_T, counter, STUDY_MUTED);
    }

    const felucca_font_t *face = text_w(&FONT_M, title) > (study.practice && !round_done() ? 166 : 202)
                                 ? &FONT_B : &FONT_M;
    cv_text(21, face == &FONT_M ? 48 : 54, face, title, STUDY_INK);
    cv_text(21, 71, &FONT_XS, detail, detail_color);
    cv_blit(0, 0);
}

static void draw_key(int x, int y, int w, int h, int k, int black, uint32_t guides, uint32_t heard, uint32_t anchors)
{
    int note = FIRST_KEY + k;
    int pressed = key_contains(&study.held, note);
    uint32_t bit = 1u << k;
    int collected = library.page == LIB_CLOSED && question_open() && is_chord() && study.collect && (study.collected & pitch_bit(note));
    uint16_t color = black ? STUDY_BG : RGB(193, 207, 205);
    if (guides & bit)
        color = RGB(72, 147, 130);
    if (collected || (anchors & bit))
        color = STUDY_GOLD;
    if (pressed || (heard & bit))
        color = STUDY_MINT;

    cv_rect(x, y, w, h, black ? STUDY_LINE : STUDY_BG);
    cv_rect(x + 1, y + 1, w - 2, h - 2, color);
    if (!black && note % 12 == 0)
        cv_text(x + 3, y + h - 14, &FONT_T, "C", STUDY_BG);
}

static void draw_keyboard(int y, int height, uint32_t guides, uint32_t heard, uint32_t anchors)
{
    static const uint8_t whites[] = PLAT_WHITE_KEYS;
    static const uint8_t blacks[] = PLAT_BLACK_KEYS;
    static const uint8_t after[] = {0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14};
    for (int i = 0; i < 16; ++i)
        draw_key(8 + i * 14, y, 14, height, whites[i], 0, guides, heard, anchors);
    for (int i = 0; i < 11; ++i)
        draw_key(17 + after[i] * 14, y, 10, height * 3 / 5, blacks[i], 1, guides, heard, anchors);
}

static void control_label(int x, int y, const char *control, const char *action)
{
    x = cv_text(x, y, &FONT_T, control, STUDY_MINT);
    cv_text(x + 4, y, &FONT_T, action, STUDY_MUTED);
}

static void draw_score_readout(const char *line)
{
    const felucca_font_t *face = text_w(&FONT_B, line) <= 216 ? &FONT_B : &FONT_XS;
    cv_text(12, SCORE_HEIGHT + 1, face, line, STUDY_MINT);
}

static void summary_line(char out[40])
{
    char *p = append_text(append_total(out, study.first_try), " first try");
    append_text(append_total(append_text(p, "  |  "), study.correct), " solved");
}

static void lower_line(char out[40])
{
    const Shape *shape = current_shape();
    if (round_done()) {
        summary_line(out);
    } else if (answer_visible()) {
        char tones[40];
        tone_list(tones);
        append_text(out, tones);
    } else if (ear_mode()) {
        const char *status = "Turn PRESET to choose";
        if (study.result == WRONG)
            status = "Try again";
        else if (study.demo)
            status = "Playing...";
        append_text(out, status);
    } else if (study.topic == SCALES || study.topic == INTERVALS) {
        char *p = append_text(out, "Notes played  ");
        p = append_text(append_number(p, study.position), " / ");
        append_number(p, shape->count);
    } else if (is_chord()) {
        uint32_t pitches = study.result == WRONG ? study.checked : chord_entry();
        char *p = append_text(out, study.collect ? "Collected  " : "Held  ");
        p = append_text(append_number(p, pitch_total(pitches)), " / ");
        append_number(p, shape->count);
    } else {
        append_text(out, "Find the named note");
    }
}

static void screen_bottom(void)
{
    char line[40];
    const Shape *shape = current_shape();
    int show_score = score_visible();
    cv_begin(240, LOWER_HEIGHT, STUDY_BG);
    lower_line(line);
    if (show_score) {
        draw_score();
        draw_score_readout(line);
        draw_keyboard(SCORE_KEYS_Y, 117 - SCORE_KEYS_Y, key_guides(), demo_lights(), 0);
    } else {
        cv_rect(11, 0, 218, 24, STUDY_CARD);
        cv_text(21, 5, &FONT_XS, line, STUDY_MUTED);
        draw_keyboard(52, 65, key_guides(), demo_lights(), 0);
    }
    cv_rect(11, 118, 218, 1, STUDY_LINE);

    control_label(11, 120, "SELECT", "Topic");
    if (!study.practice) {
        control_label(94, 120, "ALGO", "Type");
        control_label(169, 120, "PRESET", "Root");
        control_label(11, 130, "PLAY", "Hear");
        control_label(169, 130, "ENV", "Settings");
        if (is_chord())
            control_label(94, 130, "ARP", "Roll");
        cv_text(11, 139, &FONT_XS, shape->formula, STUDY_INK);
    } else if (round_done()) {
        control_label(94, 120, "EDIT", "New round");
        if (study.topic != NOTES)
            control_label(11, 130, "ALGO", "Mode");
    } else {
        if (study.topic != NOTES)
            control_label(94, 120, "ALGO", "Mode");
        if (ear_mode())
            control_label(169, 120, "PRESET", "Choice");
        else if (is_chord())
            control_label(169, 120, "ARP", study.collect ? "Tap" : "Hold");
        control_label(11, 130, "PLAY", "Hear");
        if (is_chord() || ear_mode() || study.result == CORRECT || study.result == SKIPPED)
            control_label(94, 130, "EDIT", study.result == CORRECT || study.result == SKIPPED ? "Next" : "Check");
        control_label(169, 130, "FX", "Hint");
        control_label(11, 140, "SAVE", "Skip");
        if (!ear_mode() && is_chord() && study.collect)
            control_label(94, 140, "REC", "Clear");
    }
    if (study.practice)
        control_label(169, 140, "GLO", "Library");
    else {
        control_label(130, 140, "SEL", "Guide");
        control_label(181, 140, "GLO", "Tools");
    }
    if (study.notice[0]) {
        cv_rect(8, 121, 224, 29, STUDY_CARD);
        cv_text(13, 128, &FONT_T, study.notice, STUDY_GOLD);
    }
    cv_blit(0, TOP_HEIGHT);
}

#include "library_screen.h"
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

static void announce_study(uint32_t now)
{
    if (ann_on() && ann_where()) {
        char speech[120], line[40], title[24];
        char *p = append_text(speech, study.practice ? "PRACTICE. " : "LEARN. ");
        p = append_text(p, topics[study.topic].label);
        if (!round_done()) {
            p = append_text(p, ". ");
            if (ear_mode() && !answer_visible())
                p = append_text(p, practice_shape(study.choice)->name);
            else {
                question_title(title);
                p = append_text(p, title);
            }
        }
        lower_line(line);
        append_text(append_text(p, ". "), line);
        ann_say(speech, 0, ANN_NOW);
    }
    ann_tick(now);
}

void app_ui_init(void)
{
    uint32_t now = plat_ms();
    study.rng ^= now;
    if (!study.rng)
        study.rng = 1;
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
        return;
    }
    if (guide.open) {
        guide_frame(now);
        return;
    }
    if (library.page != LIB_CLOSED) {
        library_frame(now);
        return;
    }
    announce_study(now);
    if (study.dirty) {
        study.dirty = 0;
        update_lights();
        screen_top();
        screen_bottom();
    }
}

int app_expect(const char *what, int *got)
{
#ifdef PLAT_HOST
    if (!strcmp(what, "practice")) *got = study.practice;
    else if (!strcmp(what, "topic")) *got = study.topic;
    else if (!strcmp(what, "guide_open")) *got = guide.open;
    else if (!strcmp(what, "guide_page")) *got = guide.page;
    else if (!strcmp(what, "mixed_chords")) *got = study.mixed_chords;
    else if (!strcmp(what, "mixed_roots")) *got = pitch_total(study.mixed_roots);
    else if (!strcmp(what, "mixed_qualities")) *got = pitch_total(study.mixed_qualities);
    else if (!strcmp(what, "shape_size")) *got = current_shape()->count;
    else if (!strcmp(what, "root")) *got = current().root;
    else if (!strcmp(what, "quality")) *got = current().quality;
    else if (!strcmp(what, "result")) *got = study.result;
    else if (!strcmp(what, "complete")) *got = study.completed;
    else if (!strcmp(what, "solved")) *got = study.correct;
    else if (!strcmp(what, "first_try")) *got = study.first_try;
    else if (!strcmp(what, "position")) *got = study.position;
    else if (!strcmp(what, "held_pcs")) *got = (int)study.held_pitches;
    else if (!strcmp(what, "choice")) *got = study.choice;
    else if (!strcmp(what, "collect_pcs")) *got = (int)study.collected;
    else if (!strcmp(what, "ear")) *got = ear_mode();
    else if (!strcmp(what, "guides")) *got = (int)key_guides();
    else if (!strcmp(what, "score_visible")) *got = score_visible();
    else if (!strcmp(what, "demo_lights")) *got = (int)demo_lights();
    else if (!strcmp(what, "score_screen")) *got = round_done();
    else if (settings_expect(what, got)) return 1;
    else return library_expect(what, got);
    return 1;
#else
    (void)what;
    (void)got;
    return 0;
#endif
}
