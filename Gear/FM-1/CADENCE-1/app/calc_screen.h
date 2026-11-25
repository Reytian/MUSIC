/* SPDX-License-Identifier: MPL-2.0 */
static RomanLabel calc_roman(void)
{
    return roman_label(calc.fifths, calc.minor, calc.degree, calc.alteration, calc.quality, calc.inversion);
}

static void calc_title(char *out)
{
    if (calc.item == CALC_ACCIDENTAL) {
        note_text(out, calc.target[0], 1);
    } else if (calc.item == CALC_ROMAN) {
        RomanLabel label = calc_roman();
        roman_text(out, &label);
    } else if (calc.item == CALC_MATRIX) {
        append_text(out, calc_names[CALC_MATRIX]);
    } else {
        const Shape *shape = calc_shape();
        char *end = append_text(out, root_label(calc.root, shape));
        if (calc.item == CALC_INTERVAL)
            end = append_text(end, " + ");
        append_text(end, shape->symbol);
    }
}

static void calc_context(char out[48])
{
    if (calc.item == CALC_SCALE) {
        append_text(out, calc_shape()->name);
        return;
    }
    char *end = key_text(out, calc.fifths, calc.minor);
    if (calc.item == CALC_ROMAN) {
        const char *type = calc.quality == 0 ? "triad" : (calc.quality == 1 ? "seventh" : chord_shape(calc.quality - 2)->name);
        append_text(append_text(end, "  /  "), type);
    }
}

static void calc_menu(void)
{
    cv_begin(240, 91, STUDY_BG);
    library_title_bar("Learn Tools", "CADENCE-1");
    cv_rect(11, 41, 81, 20, STUDY_MINT);
    cv_text(16, 43, &FONT_XS, "Calculators", STUDY_BG);
    for (int band = 0; band < 2; ++band) {
        int offset = band ? 91 : 0, height = band ? 149 : 91;
        if (band) {
            cv_blit(0, 0);
            cv_begin(240, 149, STUDY_BG);
        }
        for (int i = 0; i < CALC_COUNT; ++i) {
            int y = 41 + 25 * i - offset, active = calc.item == i;
            if (y < 0 || y + 20 > height)
                continue;
            cv_rect(101, y, 128, 20, STUDY_CARD);
            if (active)
                cv_rect(101, y, 2, 20, STUDY_MINT);
            cv_text(109, y + 2, &FONT_XS, calc_names[i], active ? STUDY_INK : STUDY_MUTED);
        }
    }
    cv_rect(11, 111, 218, 1, STUDY_LINE);
    control_label(11, 115, "HOME", "Learn");
    control_label(118, 115, "PRESET", "Tool");
    control_label(11, 127, "EDIT", "Open");
    control_label(118, 127, "GLO", "Close");
    control_label(11, 137, "ENV", "Settings");
    cv_blit(0, 91);
}

static void calc_matrix_top(void)
{
    cv_begin(240, TOP_HEIGHT, STUDY_BG);
    library_title_bar(calc_names[CALC_MATRIX], "GLO Back");
    if (calc.row_count == 12) {
        cv_text(12, 38, &FONT_XS, "P / I / R / RI", STUDY_INK);
        cv_text(12, 58, &FONT_T, "C = 0   T = 10   E = 11", STUDY_MUTED);
        control_label(142, 39, "PLAY", "Hear row");
        control_label(142, 52, "REC", "Undo");
        control_label(142, 65, "FX", "Clear");
    } else {
        cv_text(12, 36, &FONT_B, "Enter a tone row", STUDY_INK);
        cv_text(12, 57, &FONT_XS, "Play each pitch class once", STUDY_MUTED);
    }
    cv_blit(0, 0);
}

static void calc_matrix_bottom(void)
{
    static const char *const pitch[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "T", "E"};
    cv_begin(240, LOWER_HEIGHT, STUDY_BG);
    if (calc.row_count == 12) {
        for (int y = 0; y < 12; ++y) {
            int first = row_matrix_pitch(calc.row, y, 0);
            char label[5];
            append_text(append_text(label, "P"), pitch[first]);
            cv_text(10, 13 + y * 10, &FONT_T, label, STUDY_MINT);
            append_text(append_text(label, "R"), pitch[first]);
            cv_text(212, 13 + y * 10, &FONT_T, label, STUDY_MUTED);
            for (int x = 0; x < 12; ++x) {
                int item = row_matrix_pitch(calc.row, y, x);
                cv_rect(28 + x * 15, 13 + y * 10, 14, 10, x == y ? STUDY_LINE : STUDY_CARD);
                cv_text(33 + x * 15, 12 + y * 10, &FONT_T, pitch[item], STUDY_INK);
            }
        }
        for (int x = 0; x < 12; ++x) {
            int top = row_matrix_pitch(calc.row, 0, x);
            char label[6];
            append_text(append_text(label, "I"), pitch[top]);
            cv_text(29 + x * 15, 0, &FONT_T, label, STUDY_MINT);
            append_text(append_text(label, "RI"), pitch[top]);
            cv_text(28 + x * 15, 135, &FONT_T, label, STUDY_MUTED);
        }
    } else {
        for (int i = 0; i < 12; ++i) {
            int x = 12 + (i % 6) * 36, y = 8 + (i / 6) * 28;
            cv_rect(x, y, 31, 24, STUDY_CARD);
            cv_text(x + 5, y + 3, &FONT_XS, i < calc.row_count ? root_name[calc.row[i]] : ".", STUDY_MINT);
        }
        draw_keyboard(75, 40, 0, demo_lights(), 0, 0);
        control_label(11, 128, "REC", "Undo");
        control_label(84, 128, "PLAY", "Hear row");
        control_label(174, 128, "FX", "Clear");
    }
    cv_blit(0, TOP_HEIGHT);
}

static uint32_t calc_keys(void)
{
    uint32_t keys = 0;
    for (int i = 0; i < calc.count; ++i)
        keys |= panel_bit(calc.target[i].midi);
    return keys;
}

static void calc_task_top(void)
{
    char title[48], detail[48];
    draw_header(calc_names[calc.item], "GLO Tools", STUDY_MINT);
    calc_context(detail);
    if (calc.item == CALC_ROMAN) {
        RomanLabel label = calc_roman();
        draw_roman(21, 48, &label, STUDY_INK);
        cv_text(21, 71, &FONT_XS, detail, STUDY_MUTED);
    } else {
        calc_title(title);
        draw_card_text(title, detail, STUDY_MUTED, 202);
    }
    cv_blit(0, 0);
}

static void calc_task_bottom(void)
{
    char line[48] = "", *p = line;
    cv_begin(240, LOWER_HEIGHT, STUDY_BG);
    draw_notation(&(Score){.notes = calc.target, .count = calc.count,
                           .stacked = calc.item == CALC_CHORD || calc.item == CALC_ROMAN,
                           .filled = (1u << calc.count) - 1u, .fifths = calc.item == CALC_SCALE ? 0 : calc.fifths,
                           .cursor = -1, .live = 1});
    if (calc.item == CALC_ACCIDENTAL) {
        int sign = written_accidental(calc.target[0], calc.fifths);
        static const char *const names[] = {"Triple flat", "Double flat", "Flat", "No sign needed",
                                            "Sharp", "Double sharp", "Triple sharp", "Natural sign"};
        append_text(line, names[sign == ACCIDENTAL_NATURAL ? 7 : clamp(sign, -3, 3) + 3]);
    } else {
        for (int i = 0; i < calc.count; ++i)
            p = note_text(i ? append_text(p, " ") : p, calc.target[i], 0);
    }
    draw_score_readout(line, STUDY_MINT);
    draw_keyboard(SCORE_KEYS_Y, 117 - SCORE_KEYS_Y, calc_keys(), demo_lights(), 0, 0);
    cv_rect(11, 118, 218, 1, STUDY_LINE);
    if (calc.item == CALC_ACCIDENTAL) {
        control_label(11, 120, "SELECT", "Note");
        control_label(91, 120, "ALGO", "Key");
        control_label(162, 120, "PRESET", "Sign");
    } else if (calc.item == CALC_ROMAN) {
        control_label(11, 120, "SELECT", "Key");
        control_label(91, 120, "ALGO", "Degree");
        control_label(162, 120, "PRESET", "Type");
        control_label(11, 131, "K1", "Inversion");
        control_label(91, 131, "K2", "Alter");
        control_label(162, 131, "REC", "Maj/min");
    } else {
        if (calc.item != CALC_SCALE)
            control_label(11, 120, "SELECT", "Key");
        control_label(91, 120, "ALGO", "Type");
        control_label(162, 120, "PRESET", "Root");
    }
    control_label(11, 141, "PLAY", "Hear");
    control_label(91, 141, "OCT", "Octave");
    draw_notice();
    cv_blit(0, TOP_HEIGHT);
}

static void calc_frame(uint32_t now)
{
    if (ann_on() && ann_where()) {
        char speech[120], title[48];
        char *p = append_text(append_text(speech, "Calculators. "), calc_names[calc.item]);
        if (calc.page == LIB_TASK) {
            calc_title(title);
            append_text(append_text(p, ". "), title);
        }
        ann_say(speech, 0, ANN_NOW);
    }
    ann_tick(now);
    if (!study.dirty)
        return;
    study.dirty = 0;
    if (calc.page == LIB_MENU) {
        plat_lights((1u << B_HOME) | (1u << B_GLO), study.panel, 1u << B_EDIT, 0, 2);
        calc_menu();
        return;
    }
    plat_lights((1u << B_HOME) | (1u << B_GLO) | (demo_playing() ? LED_PLAY_GREEN : 0),
                study.panel | demo_lights(), 1u << B_PLAY, calc_keys(), 2);
    if (calc.item == CALC_MATRIX) {
        calc_matrix_top();
        calc_matrix_bottom();
    } else {
        calc_task_top();
        calc_task_bottom();
    }
}

#ifdef PLAT_HOST
static int calc_expect(const char *what, int *got)
{
    if (!strcmp(what, "calc_page")) *got = calc.page;
    else if (!strcmp(what, "calc_item")) *got = calc.item;
    else if (!strcmp(what, "calc_count")) *got = calc.count;
    else if (!strcmp(what, "calc_first_note")) *got = calc.target[0].midi;
    else if (!strcmp(what, "calc_last_note")) *got = calc.target[calc.count - 1].midi;
    else if (!strcmp(what, "calc_note_acc")) *got = calc.target[0].accidental;
    else if (!strcmp(what, "calc_row_count")) *got = calc.row_count;
    else if (!strcmp(what, "calc_matrix_sample")) *got = row_matrix_pitch(calc.row, 5, 7);
    else return 0;
    return 1;
}
#endif
