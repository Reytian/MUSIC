/* SPDX-License-Identifier: MPL-2.0 */
static char *library_pitch_text(char *out, StaffNote note, int show_octave)
{
    int letter = positive_mod(note.position, 7);
    *out++ = "CDEFGAB"[letter];
    int accidental = note.accidental;
    while (accidental < 0) {
        *out++ = 'b';
        ++accidental;
    }
    while (accidental > 0) {
        *out++ = '#';
        --accidental;
    }
    *out = 0;
    if (show_octave)
        out = append_number(out, 4 + (note.position - letter) / 7);
    return out;
}

static void library_key_text(char *out, int fifths)
{
    append_text(append_text(out, library_key_name(fifths)), library.minor ? " minor" : " major");
}

static void library_target_title(char *out)
{
    int kind = library_kind();
    if (kind == ITEM_SIGNATURE) {
        library_key_text(out, library.fifths);
    } else if (kind == ITEM_NOTE || library_is_direct() || kind == ITEM_ACCIDENTAL) {
        library_pitch_text(out, library.target[0], 1);
    } else if (kind == ITEM_ROMAN) {
        static const char *const upper[] = {"I", "II", "III", "IV", "V", "VI", "VII"};
        static const char *const lower[] = {"i", "ii", "iii", "iv", "v", "vi", "vii"};
        StaffNote base[4];
        roman_notes(base, library.fifths, library.minor, library.degree, library.alteration,
                    library.quality, 0, library.count);
        int third = base[1].midi - base[0].midi;
        int fifth = base[2].midi - base[0].midi;
        char *end = append_text(out, library.alteration < 0 ? "b" : (library.alteration > 0 ? "#" : ""));
        end = append_text(end, third == 3 ? lower[library.degree] : upper[library.degree]);
        if (fifth == 6)
            end = append_text(end, library.count == 4 && base[3].midi - base[0].midi == 10 ? "hdim" : "dim");
        else if (fifth == 8)
            end = append_text(end, "aug");
        else if (third == 2 || third == 5)
            end = append_text(end, third == 2 ? "sus2" : "sus4");
        if (library.count == 4) {
            int seventh = base[3].midi - base[0].midi;
            end = append_text(end, seventh == 11 ? "maj7" : "7");
        }
        if (library.inversion)
            append_number(append_text(end, " / "), library.inversion);
    } else if (kind == ITEM_MATRIX) {
        append_text(out, "12-Tone Matrix");
    } else {
        const Shape *shape = library_shape(kind, library.quality);
        char *end = library_pitch_text(out, library.target[0], 0);
        if (kind == ITEM_INTERVAL)
            end = append_text(end, " + ");
        append_text(end, shape->symbol);
    }
}

static void library_choice_text(char *out)
{
    int kind = library_kind();
    if (kind == ITEM_NOTE) {
        library_pitch_text(out, library_note_option(library.choice), 0);
    } else if (kind == ITEM_SIGNATURE) {
        library_key_text(out, library.choice);
    } else if (kind == ITEM_CHORD && library.group != LIB_EAR) {
        const Shape *shape = chord_shape(library.choice);
        char *end = append_text(out, root_name[library.typed_position]);
        append_text(end, shape->symbol);
    } else {
        append_text(out, library_shape(kind, library.choice)->name);
    }
}

static uint32_t library_panel_notes(void)
{
    uint32_t mask = 0;
    for (int i = 0; i < library.count; ++i) {
        int key = library.target[i].midi - FIRST_KEY;
        if ((unsigned)key < NKEYS)
            mask |= 1u << key;
    }
    return mask;
}

static uint32_t library_sound_lights(void)
{
    uint32_t keys = 0;
    if (library.group != LIB_EAR || library_revealed()) {
        for (int i = 0; i < NKEYS; ++i)
            if (key_contains(&study.demo_notes, FIRST_KEY + i))
                keys |= 1u << i;
    }
    return keys;
}

static void library_title_bar(const char *left, const char *right)
{
    cv_rect(11, 8, 218, 22, STUDY_CARD);
    cv_rect(11, 8, 2, 22, STUDY_MINT);
    cv_text(19, 10, &FONT_B, left, STUDY_INK);
    text_right(222, 13, &FONT_T, right, STUDY_MUTED);
}

static const char *library_menu_item(int index)
{
    if (library.group == LIB_CALC) {
        static const char *const items[] = {"Accidental", "Interval", "Scale", "Chord", "Roman Analysis", "12-Tone Matrix"};
        return items[index];
    }
    if (library.group == LIB_EAR) {
        static const char *const items[] = {"Play the Note", "Name the Note", "Interval", "Scale", "Chord"};
        return items[index];
    }
    if (library.group == LIB_KEYS) {
        static const char *const items[] = {"Note", "Reverse Note", "Interval", "Scale", "Chord"};
        return items[index];
    }
    static const char *const items[] = {"Note", "Key Signature", "Interval", "Scale", "Chord"};
    return items[index];
}

static void library_menu_rows(int offset, int height)
{
    int groups = study.practice ? LIB_CALC : 1;
    for (int row = 0; row < groups; ++row) {
        int i = study.practice ? row : LIB_CALC;
        int y = 41 + 25 * row - offset;
        if (y < 0 || y + 20 > height)
            continue;
        int active = library.group == i;
        cv_rect(11, y, 81, 20, active ? STUDY_MINT : STUDY_CARD);
        cv_text(16, y + 2, &FONT_XS, library_groups[i].short_name, active ? STUDY_BG : STUDY_MUTED);
    }
    for (int i = 0; i < library_groups[library.group].count; ++i) {
        int y = 41 + 25 * i - offset;
        if (y < 0 || y + 20 > height)
            continue;
        int active = library.item == i;
        cv_rect(101, y, 128, 20, STUDY_CARD);
        if (active)
            cv_rect(101, y, 2, 20, STUDY_MINT);
        cv_text(109, y + 2, &FONT_XS, library_menu_item(i), active ? STUDY_INK : STUDY_MUTED);
    }
}

static void library_draw_menu(void)
{
    cv_begin(240, 91, STUDY_BG);
    library_title_bar(study.practice ? "Practice Library" : "Learn Tools", "KEYSTUDY");
    library_menu_rows(0, 91);
    cv_blit(0, 0);

    cv_begin(240, 149, STUDY_BG);
    library_menu_rows(91, 149);
    cv_rect(11, 111, 218, 1, STUDY_LINE);
    control_label(11, 115, study.practice ? "SELECT" : "HOME", study.practice ? "Group" : "Learn");
    control_label(118, 115, "PRESET", "Activity");
    control_label(11, 127, "EDIT", "Open");
    control_label(118, 127, "GLO", "Close");
    control_label(11, 137, "ENV", "Settings");
    cv_blit(0, 91);
}

static const char *library_feedback(void)
{
    if (library.result == CORRECT)
        return "Correct";
    if (library.result == SKIPPED)
        return "Skipped";
    if (library.result == WRONG)
        return "Try again";
    if (library.show_answer)
        return "Answer";
    if (library.group == LIB_CALC)
        return "Result";
    return "Ready";
}

static void library_context(char out[48])
{
    int kind = library_kind();
    if (kind == ITEM_ROMAN || kind == ITEM_ACCIDENTAL || kind == ITEM_INTERVAL || kind == ITEM_CHORD) {
        library_key_text(out, library.fifths);
        if (kind == ITEM_ROMAN) {
            const char *type = library.quality == 0 ? "Diatonic triad" : (library.quality == 1 ?
                               "Diatonic seventh" : chord_shape(library.quality - 2)->name);
            char *end = out;
            while (*end)
                ++end;
            append_text(append_text(end, "  /  "), type);
        }
    } else if (kind == ITEM_SCALE) {
        append_text(out, library_shape(kind, library.quality)->name);
    } else {
        out[0] = 0;
    }
}

static void library_task_top(void)
{
    int kind = library_kind();
    char title[48], detail[48], number[8];

    cv_begin(240, TOP_HEIGHT, STUDY_BG);
    library_title_bar(library_groups[library.group].name, "GLO BACK");
    cv_text(12, 34, &FONT_XS, library_item_name(), STUDY_MUTED);
    if (library.group != LIB_CALC && !library_finished()) {
        int n = library.seen + (library_quiz_open() != 0);
        append_total(number, n);
        text_right(227, 35, &FONT_T, number, STUDY_MUTED);
    }

    if (library_finished()) {
        append_text(title, "Round complete");
        append_text(detail, "Results for this activity");
    } else if (library.group == LIB_CALC || library_revealed() || library.group == LIB_WRITE) {
        library_target_title(title);
        if (library.group == LIB_CALC)
            library_context(detail);
        else if (library.group == LIB_WRITE && !library_revealed() && library.result != WRONG)
            append_text(detail, kind == ITEM_SIGNATURE ? "Set the accidentals on the staff" : "Write the notes on the staff");
        else
            append_text(detail, library_feedback());
    } else {
        const char *prompt = library.group == LIB_EAR ? "Listen and identify" : "Identify the notes";
        if (kind == ITEM_SIGNATURE)
            prompt = library.minor ? "Find the minor key" : "Find the major key";
        else if (library_is_direct())
            prompt = library.group == LIB_EAR ? "Listen, then play" : "Play the written note";
        else if (kind == ITEM_NOTE)
            prompt = library.group == LIB_EAR ? "Name the sound" : "Name the note";
        else if (kind == ITEM_CHORD)
            prompt = library.group == LIB_EAR ? "Name the chord type" : "Name the chord";
        else if (kind == ITEM_SCALE)
            prompt = "Name the scale";
        else if (kind == ITEM_INTERVAL)
            prompt = "Name the interval";
        append_text(title, prompt);
        append_text(detail, library.result == WRONG ? "Try again" : (library.group == LIB_EAR ?
                    (library.demo_count ? "Playing..." : "PLAY to hear the prompt") : ""));
    }
    const felucca_font_t *face = text_w(&FONT_M, title) <= 216 ? &FONT_M : &FONT_B;
    cv_text(12, face == &FONT_M ? 49 : 55, face, title, STUDY_INK);
    cv_text(12, 72, &FONT_T, detail, library.result == CORRECT ? STUDY_MINT : STUDY_MUTED);
    cv_blit(0, 0);
}

static void library_matrix_top(void)
{
    cv_begin(240, TOP_HEIGHT, STUDY_BG);
    library_title_bar("12-Tone Matrix", "GLO BACK");
    if (library.row_count == 12) {
        cv_text(12, 38, &FONT_XS, "P / I / R / RI", STUDY_INK);
        cv_text(12, 58, &FONT_T, "C = 0   T = 10   E = 11", STUDY_MUTED);
        control_label(142, 39, "PLAY", "Hear row");
        control_label(142, 52, "REC", "Undo");
        control_label(142, 65, "FX", "Clear");
    } else {
        cv_text(12, 36, &FONT_B, "Enter a tone row", STUDY_INK);
        cv_text(12, 57, &FONT_T, "Play each pitch class once", STUDY_MUTED);
    }
    cv_blit(0, 0);
}

static void library_matrix_bottom(void)
{
    static const char *const pitch[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "T", "E"};
    cv_begin(240, LOWER_HEIGHT, STUDY_BG);
    if (library.row_count == 12) {
        for (int y = 0; y < 12; ++y) {
            int first = row_matrix_pitch(library.row, y, 0);
            char label[5];
            append_text(append_text(label, "P"), pitch[first]);
            cv_text(10, 13 + y * 10, &FONT_T, label, STUDY_MINT);
            append_text(append_text(label, "R"), pitch[first]);
            cv_text(212, 13 + y * 10, &FONT_T, label, STUDY_MUTED);
            for (int x = 0; x < 12; ++x) {
                int item = row_matrix_pitch(library.row, y, x);
                cv_rect(28 + x * 15, 13 + y * 10, 14, 10, x == y ? STUDY_LINE : STUDY_CARD);
                cv_text(33 + x * 15, 12 + y * 10, &FONT_T, pitch[item], STUDY_INK);
            }
        }
        for (int x = 0; x < 12; ++x) {
            int top = row_matrix_pitch(library.row, 0, x);
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
            cv_text(x + 5, y + 3, &FONT_XS, i < library.row_count ? root_name[library.row[i]] : ".", STUDY_MINT);
        }
        draw_keyboard(75, 40, 0, library_sound_lights(), 0);
        control_label(11, 128, "REC", "Undo");
        control_label(84, 128, "PLAY", "Hear row");
        control_label(174, 128, "FX", "Clear");
    }
    cv_blit(0, TOP_HEIGHT);
}

static void library_note_names(char out[48])
{
    char *p = out;
    for (int i = 0; i < library.count; ++i) {
        if (i)
            p = append_text(p, " ");
        p = library_pitch_text(p, library.target[i], 0);
    }
    *p = 0;
}

static void library_task_bottom(void)
{
    int kind = library_kind();
    int calculator = library.group == LIB_CALC;
    int shown = library_revealed() || calculator;
    int write = library.group == LIB_WRITE && !shown;
    int staff = shown || library.group == LIB_STAFF || library.group == LIB_WRITE || kind == ITEM_REVERSE;
    uint32_t guides = 0, root = 0;
    char line[48] = "";
    cv_begin(240, LOWER_HEIGHT, STUDY_BG);

    if (library_finished()) {
        char *p = append_text(append_total(line, library.first_try), " first try     ");
        append_text(append_total(p, library.solved), " solved");
        cv_text(12, 15, &FONT_M, line, STUDY_INK);
        control_label(12, 115, "EDIT", "New round");
        control_label(150, 115, "GLO", "Library");
        cv_blit(0, TOP_HEIGHT);
        return;
    }

    if (staff) {
        int fifths = kind == ITEM_SIGNATURE ? (write ? library.choice : library.fifths) :
                     (calculator && kind != ITEM_SCALE ? library.fifths : 0);
        draw_notation(write ? library.written : library.target, library.count,
                      write ? library.filled : (1 << library.count) - 1,
                      kind == ITEM_CHORD || kind == ITEM_ROMAN, fifths,
                      write && kind != ITEM_SIGNATURE ? library.cursor : -1, shown);
    }
    if (shown || (library.group == LIB_KEYS && kind != ITEM_REVERSE)) {
        guides = library_panel_notes();
        if (!shown && (kind == ITEM_SCALE || kind == ITEM_CHORD)) {
            int key = library.target[0].midi - FIRST_KEY;
            if ((unsigned)key < NKEYS)
                root = 1u << key;
        }
    }
    if (shown) {
        if (kind == ITEM_ACCIDENTAL) {
            int sign = written_accidental(library.target[0], library.fifths);
            static const char *const names[] = {"Triple flat", "Double flat", "Flat", "No sign needed",
                                                "Sharp", "Double sharp", "Triple sharp", "Natural sign"};
            append_text(line, names[sign == ACCIDENTAL_NATURAL ? 7 : clamp(sign, -3, 3) + 3]);
        } else {
            library_note_names(line);
        }
    } else if (write && kind != ITEM_SIGNATURE) {
        char *p = append_text(append_number(append_text(line, "Note "), library.cursor + 1), " / ");
        append_number(p, library.count);
    } else if (write) {
        int fifths = library.choice;
        char *p = append_number(line, fifths < 0 ? -fifths : fifths);
        append_text(p, fifths < 0 ? " flats" : (fifths > 0 ? " sharps" : " accidentals"));
    } else if (!library_is_direct()) {
        char answer[35];
        library_choice_text(answer);
        append_text(append_text(line, "Answer   "), answer);
    } else {
        append_text(line, library.demo_count ? "Playing..." : "Answer on the keys");
    }

    if (staff)
        draw_score_readout(line);
    else
        cv_text(12, 17, &FONT_M, line, STUDY_MINT);
    int keys_y = staff ? SCORE_KEYS_Y : 52;
    draw_keyboard(keys_y, 117 - keys_y, guides, library_sound_lights(), root);
    cv_rect(11, 118, 218, 1, STUDY_LINE);
    if (calculator) {
        if (kind == ITEM_ACCIDENTAL) {
            control_label(11, 120, "SELECT", "Note");
            control_label(91, 120, "ALGO", "Key");
            control_label(162, 120, "PRESET", "Accid.");
        } else if (kind == ITEM_ROMAN) {
            control_label(11, 120, "SELECT", "Key");
            control_label(91, 120, "ALGO", "Degree");
            control_label(162, 120, "PRESET", "Type");
            control_label(11, 131, "K1", "Inversion");
            control_label(91, 131, "K2", "Alter");
            control_label(162, 131, "REC", "Maj/min");
        } else {
            if (kind != ITEM_SCALE)
                control_label(11, 120, "SELECT", "Key");
            control_label(91, 120, "ALGO", "Type");
            control_label(162, 120, "PRESET", "Root");
        }
        control_label(11, 140, "PLAY", "Hear");
    } else {
        int open = library_quiz_open();
        if (open && library.group == LIB_WRITE && kind != ITEM_SIGNATURE) {
            control_label(11, 120, "SELECT", "Slot");
            control_label(91, 120, "ALGO", "Line");
            control_label(162, 120, "PRESET", "Accid.");
            control_label(91, 130, "REC", "Erase");
            control_label(162, 130, "SEL", "Set");
        } else if (open && !library_is_direct()) {
            control_label(11, 120, "PRESET", write ? "Accidentals" : "Answer");
            if (kind == ITEM_CHORD && library.group != LIB_EAR)
                control_label(115, 120, "ALGO", "Root");
        }
        if (kind != ITEM_SIGNATURE && (shown || library.group == LIB_EAR))
            control_label(11, 130, "PLAY", "Hear");
        if (open) {
            control_label(11, 140, "FX", shown ? "Hide" : "Hint");
            control_label(91, 140, "SAVE", "Skip");
        }
        if (!open || !library_is_direct())
            control_label(162, 140, "EDIT", open ? "Check" : "Next");
    }
    cv_blit(0, TOP_HEIGHT);
}

static void library_frame(uint32_t now)
{
    if (ann_on() && ann_where()) {
        char speech[120], title[48];
        char *p = append_text(speech, library_groups[library.group].name);
        p = append_text(append_text(p, ". "), library_item_name());
        if (library.page == LIB_TASK && (library.group == LIB_CALC || library_revealed())) {
            library_target_title(title);
            append_text(append_text(p, ". "), title);
        }
        ann_say(speech, 0, ANN_NOW);
    }
    ann_tick(now);
    if (!study.dirty)
        return;
    study.dirty = 0;

    if (library.page == LIB_MENU) {
        plat_lights(1u << B_GLO, study.panel, 1u << B_EDIT, 0, 2);
        library_draw_menu();
    } else {
        int show_notes = library_revealed() || library.group == LIB_CALC ||
                         (library.group == LIB_KEYS && library_kind() != ITEM_REVERSE);
        uint32_t keys = show_notes ? library_panel_notes() : 0;
        uint32_t buttons = (1u << B_GLO) | (library.demo_count ? LED_PLAY_GREEN : 0);
        plat_lights(buttons, study.panel | library_sound_lights(), (1u << B_PLAY) | (1u << B_EDIT), keys, 2);
        if (library_kind() == ITEM_MATRIX) {
            library_matrix_top();
            library_matrix_bottom();
        } else {
            library_task_top();
            library_task_bottom();
        }
    }
}

#ifdef PLAT_HOST
static int library_expect(const char *what, int *got)
{
    if (!strcmp(what, "lib_page")) *got = library.page;
    else if (!strcmp(what, "lib_group")) *got = library.group;
    else if (!strcmp(what, "lib_item")) *got = library.item;
    else if (!strcmp(what, "lib_kind")) *got = library_kind();
    else if (!strcmp(what, "lib_count")) *got = library.count;
    else if (!strcmp(what, "lib_result")) *got = library.result;
    else if (!strcmp(what, "lib_first_try")) *got = library.first_try;
    else if (!strcmp(what, "lib_revealed")) *got = library_revealed();
    else if (!strcmp(what, "lib_demo")) *got = library.demo_count;
    else if (!strcmp(what, "lib_block_demo")) *got = library.demo_block;
    else if (!strcmp(what, "lib_lights")) *got = (int)library_sound_lights();
    else if (!strcmp(what, "lib_filled")) *got = library.filled;
    else if (!strcmp(what, "lib_fifths")) *got = library.fifths;
    else if (!strcmp(what, "lib_choice")) *got = library.choice;
    else if (!strcmp(what, "lib_quality")) *got = library.quality;
    else if (!strcmp(what, "lib_note_letter")) *got = positive_mod(library.target[0].position, 7);
    else if (!strcmp(what, "lib_note_acc")) *got = library.target[0].accidental;
    else if (!strcmp(what, "lib_first_note")) *got = library.target[0].midi;
    else if (!strcmp(what, "lib_last_note")) *got = library.target[library.count - 1].midi;
    else if (!strcmp(what, "lib_row_count")) *got = library.row_count;
    else if (!strcmp(what, "lib_matrix_sample")) *got = row_matrix_pitch(library.row, 5, 7);
    else return 0;
    return 1;
}
#endif
