/* SPDX-License-Identifier: MPL-2.0 */
static char *note_text(char *out, StaffNote note, int show_octave)
{
    int letter = positive_mod(note.position, 7);
    out = spell_text(out, letter, note.accidental);
    if (show_octave)
        out = append_number(out, 4 + (note.position - letter) / 7);
    return out;
}

static char *key_text(char *out, int fifths, int minor)
{
    return append_text(append_text(out, key_name(fifths + 7 + (minor ? 15 : 0))), minor ? " minor" : " major");
}

static void draw_ring(int x, int y, int slashed, uint16_t color)
{
    cv_rect(x + 2, y, 3, 1, color);
    cv_rect(x + 2, y + 6, 3, 1, color);
    cv_rect(x, y + 2, 1, 3, color);
    cv_rect(x + 6, y + 2, 1, 3, color);
    cv_rect(x + 1, y + 1, 1, 1, color);
    cv_rect(x + 5, y + 1, 1, 1, color);
    cv_rect(x + 1, y + 5, 1, 1, color);
    cv_rect(x + 5, y + 5, 1, 1, color);
    if (slashed)
        cv_line(x - 1, y + 7, x + 7, y - 1, color);
}

/* A numeral as a harmony book sets it: the quality mark raised, the figured bass stacked. */
static int draw_roman(int x, int y, const RomanLabel *label, uint16_t color)
{
    char figure[2] = {0, 0};
    x = cv_text(x, y, &FONT_M, label->numeral, color);
    if (label->mark == MARK_AUG) {
        x = cv_text(x + 1, y + 1, &FONT_B, "+", color);
    } else if (label->mark) {
        draw_ring(x + 2, y + 5, label->mark == MARK_HALF_DIM, color);
        x += 11;
    }
    if (label->suffix[0] == 's')
        x = cv_text(x + 2, y + 7, &FONT_B, label->suffix, color);
    else if (label->suffix[0])
        x = cv_text(x + 1, y + 2, &FONT_T, label->suffix, color);
    for (int i = 0; i < 2; ++i) {
        if (label->figure[i]) {
            figure[0] = label->figure[i];
            cv_text(x + 1, y + 2 + 11 * i, &FONT_T, figure, color);
        }
    }
    return x + (label->figure[0] ? 8 : 0);
}

static RomanLabel progression_label(int step)
{
    const Progression *progression = &progressions[library.quality];
    return roman_label(library.fifths, library.minor, progression->degree[step], 0, progression->type[step], 0);
}

static const char *tritone_or(const Shape *shape)
{
    return one_tritone() && shape == &intervals[INTERVAL_A4] ? "TT" : shape->symbol;
}

/* What the question is, in words: the prompt of a name question, the answer of the others. */
static void library_target_title(char *out)
{
    int kind = library_kind();
    char *p = out;
    if (kind == ITEM_SIGNATURE) {
        key_text(out, library.fifths, library.minor);
    } else if (kind == ITEM_NOTE) {
        note_text(out, library.target[0], written_notes());
    } else if (kind == ITEM_MELODY) {
        key_text(append_text(out, "Melody in "), library.fifths, 0);
    } else if (kind == ITEM_DEGREE) {
        append_number(append_text(out, "Degree "), library.degree + 1);
    } else if (kind == ITEM_ROMAN) {
        RomanLabel label = roman_label(library.fifths, library.minor, library.degree, 0, library.type, 0);
        roman_text(out, &label);
    } else if (kind == ITEM_PROGRESSION) {
        for (int step = 0; step < library.steps; ++step) {
            RomanLabel label = progression_label(step);
            p = roman_text(step ? append_text(p, " ") : p, &label);
        }
    } else {
        const Shape *shape = library_shape(kind, library.quality);
        p = append_text(out, root_label(library.root, shape));
        if (kind == ITEM_INTERVAL)
            p = append_text(p, " + ");
        p = append_text(p, tritone_or(shape));
        if (kind == ITEM_INVERSION)
            note_text(append_text(p, "/"), library.target[0], 0);
    }
}

static const char *library_prompt_title(void)
{
    static const char *const name_it[] = {"Name the note", "Name the key", "Name the interval", "Name the scale", "Name the chord"};
    int kind = library_kind();
    if (library_prompt() == PROMPT_SOUND)
        return "Listen";
    if (library_prompt() == PROMPT_KEYS)
        return kind == ITEM_NOTE ? "Name the lit key" : "Name the lit keys";
    if (library_answer() == ANSWER_PLAY)
        return "Play what is written";
    if (kind == ITEM_SIGNATURE)
        return library.minor ? "Name the minor key" : "Name the major key";
    return name_it[kind];
}

static const char playing_text[] = "Playing...";
static int drawn_playing;            /* the last card drawn said playing_text (for the tests) */

static const char *library_instruction(void)
{
    int kind = library_kind(), mode = play_mode(), sound = library_prompt() == PROMPT_SOUND;
    if (sound && (demo_playing() || library.autoplay_at))
        return playing_text;
    if (library_answer() == ANSWER_WRITE)
        return kind == ITEM_SIGNATURE ? "Set the sharps or flats" : "Write it on the staff";
    if (library_answer() == ANSWER_CHOICE)
        return sound ? "Choose what you heard" : "Choose, then press EDIT";
    if (kind == ITEM_PROGRESSION)
        return "Hold each chord, lifting between";
    if (kind == ITEM_INVERSION)
        return "Hold it with that note lowest";
    if (mode == PLAY_PITCH_SET && library.collect)
        return "Tap notes to add or remove";
    if (sound && kind == ITEM_NOTE)
        return "C sounded first: play the other";
    if (sound)
        return mode == PLAY_EXACT ? "Play it back from the lit key" : "Hold it, starting on the lit key";
    if (mode == PLAY_PITCH_SET)
        return "Hold all the chord tones";
    if (mode == PLAY_EXACT_SET)
        return "Hold the written keys";
    return library.count > 1 ? "Play the notes going up" : "Play it on the keys";
}

static uint32_t library_all_keys(void)
{
    uint32_t keys = 0;
    for (int i = 0; i < library.count; ++i)
        keys |= panel_bit(library.target[i].midi);
    return keys;
}

static uint32_t library_guide_keys(void)
{
    if (library.over || library_kind() == ITEM_PROGRESSION)
        return 0;
    return library_revealed() || library_prompt() == PROMPT_KEYS ? library_all_keys() : 0;
}

/* Keys marked without giving the answer away: where to start, a chord's root, notes tapped in. */
static uint32_t library_anchor_keys(void)
{
    int kind = library_kind();
    uint32_t keys = 0;
    if (library.over || library_revealed())
        return 0;
    if (library.given >= 0)
        keys |= panel_bit(library.given);
    if (library_prompt() == PROMPT_KEYS && (kind == ITEM_SCALE || kind == ITEM_CHORD))
        keys |= panel_bit(library.target[(library.count - library.inversion) % library.count].midi);
    if (library.collect && may_collect()) {
        for (int k = 0; k < NKEYS; ++k)
            if (library.collected & pitch_bit(base_key() + k))
                keys |= 1u << k;
    }
    return keys;
}

static uint32_t library_wrong_keys(void)
{
    int mode = play_mode();
    uint32_t keys = 0, want = step_pitches(library.step);
    if (library.over || library.result != WRONG || library_answer() != ANSWER_PLAY)
        return 0;
    if (mode == PLAY_ANY_OCTAVE || mode == PLAY_EXACT)
        return library.wrong_note >= 0 && key_contains(&study.held, library.wrong_note) ? panel_bit(library.wrong_note) : 0;
    for (int k = 0; k < NKEYS; ++k) {
        int note = base_key() + k;
        if (!key_contains(&study.held, note))
            continue;
        if (mode == PLAY_PITCH_SET ? !(want & pitch_bit(note)) : !(library_all_keys() & (1u << k)))
            keys |= 1u << k;
    }
    return keys;
}

static uint32_t library_sound_lights(void)
{
    return library_prompt() == PROMPT_SOUND && !library_revealed() ? 0 : demo_lights();
}

static void library_item_title(int item, char *out)
{
    int kind = library_kind();
    char *p = out;
    if (kind == ITEM_NOTE && written_notes()) {
        int lo, hi, signs = library_prompt() == PROMPT_STAFF && content_signatures();
        note_range(&lo, &hi);
        note_text(out, written_note(lo + item / 3, signs ? 0 : item % 3 - 1), 1);
    } else if (kind == ITEM_NOTE) {
        append_text(out, root_name[item]);
    } else if (kind == ITEM_SIGNATURE) {
        append_text(append_text(out, key_name(item)), key_minor(item) ? " min" : " maj");
    } else if (kind == ITEM_MELODY) {
        append_text(out, "Melody");
    } else if (kind == ITEM_DEGREE || kind == ITEM_ROMAN) {
        int minor = key_minor(item / 7);
        if (kind == ITEM_DEGREE) {
            p = append_number(out, item % 7 + 1);
        } else {
            RomanLabel label = roman_label(key_fifths(item / 7), minor, item % 7, 0, minor && item % 7 == 4 ? 2 : 0, 0);
            p = roman_text(out, &label);
        }
        append_text(append_text(append_text(p, " in "), key_name(item / 7)), minor ? "m" : "");
    } else if (kind == ITEM_PROGRESSION) {
        p = append_text(out, key_name(item / PROGRESSION_COUNT));
        append_text(append_text(p, key_minor(item / PROGRESSION_COUNT) ? "m" : ""), " changes");
    } else {
        const Shape *shape = library_shape(kind, item / 12);
        p = append_text(out, root_label(item % 12, shape));
        if (kind == ITEM_INTERVAL)
            p = append_text(p, "+");
        append_text(p, tritone_or(shape));
    }
}

static void library_title_bar(const char *left, const char *right)
{
    cv_rect(11, 8, 218, 22, STUDY_CARD);
    cv_rect(11, 8, 2, 22, STUDY_MINT);
    cv_text(19, 10, &FONT_B, left, STUDY_INK);
    text_right(222, 13, &FONT_T, right, STUDY_MUTED);
}

/* Eight groups down the left, the chosen group's activities down the right. */
static void library_menu_rows(int offset, int height)
{
    const LibraryGroupInfo *group = &library_groups[library.menu_group];
    for (int i = 0; i < LIB_GROUPS; ++i) {
        int y = 34 + 19 * i - offset, active = library.menu_group == i;
        if (y < 0 || y + 17 > height)
            continue;
        cv_rect(11, y, 86, 17, active ? STUDY_MINT : STUDY_CARD);
        cv_text(16, y + 1, &FONT_XS, library_groups[i].short_name, active ? STUDY_BG : STUDY_MUTED);
    }
    for (int i = 0; i < group->count; ++i) {
        int y = 34 + 19 * i - offset, active = library.menu_item == i;
        int running = library.menu_group == library.group && library.item == i;
        if (y < 0 || y + 17 > height)
            continue;
        cv_rect(103, y, 126, 17, STUDY_CARD);
        if (active)
            cv_rect(103, y, 2, 17, STUDY_MINT);
        cv_text(111, y + 1, &FONT_XS, kind_names[group->kinds[i]], active ? STUDY_INK : STUDY_MUTED);
        if (running)
            text_right(224, y + 3, &FONT_T, "Open", STUDY_MINT);
    }
}

static void library_draw_menu(void)
{
    const LibraryGroupInfo *group = &library_groups[library.menu_group];
    int level = kind_level(group->kinds[library.menu_item]);
    char line[48], *p;
    cv_begin(240, 110, STUDY_BG);
    library_title_bar("Practice Library", "CADENCE-1");
    library_menu_rows(0, 110);
    cv_blit(0, 0);

    cv_begin(240, 130, STUDY_BG);
    library_menu_rows(110, 130);
    cv_text(12, 78, &FONT_XS, group->about, STUDY_INK);
    p = append_text(line, level_names[level]);
    if (prefs.guided) {
        p = append_number(append_text(p, " level "), progress.level[level]);
        append_number(append_text(p, " of "), level_top[level]);
    } else {
        append_text(p, ": custom");
    }
    cv_caps(12, 93, &FONT_T, line, STUDY_MUTED);
    cv_rect(11, 106, 218, 1, STUDY_LINE);
    control_label(11, 108, "SELECT", "Group");
    control_label(118, 108, "PRESET", "Activity");
    control_label(11, 118, "EDIT", "Open");
    control_label(118, 118, "GLO", "Back");
    cv_blit(0, 110);
}

static void library_counter(char out[12])
{
    uint32_t limit = library_time_limit();
    if (limit) {
        uint32_t left = (limit - (library.clock_ms < limit ? library.clock_ms : limit) + 999u) / 1000u;
        char *p = append_text(append_number(out, (int)(left / 60u)), left % 60u < 10u ? ":0" : ":");
        append_number(p, (int)(left % 60u));
    } else {
        char *p = append_text(append_number(out, library.seen + library_quiz_open()), "/");
        append_number(p, library_round_length());
    }
}


static void library_task_top(void)
{
    int kind = library_kind(), revealed = library_revealed(), named = library_prompt() == PROMPT_NAME;
    char heading[32], title[56] = "", counter[12];
    const char *detail = library_instruction();
    uint16_t detail_color = STUDY_MUTED, accent = STUDY_MINT;
    drawn_playing = 0;

    append_text(heading, kind_names[kind]);
    if (library_group() == &library_groups[LIB_INKEY] && !library.over)
        key_text(heading, library.fifths, library.minor);
    if (library.over) {
        append_text(title, library.review ? "Review complete" : (library_time_limit() ? "Time is up" : "Round complete"));
        detail = "First try, without hints";
    } else if (library.result == CORRECT) {
        detail = "Correct";
        detail_color = STUDY_MINT;
    } else if (library.result == SKIPPED) {
        detail = "Skipped";
        detail_color = STUDY_GOLD;
    } else if (library.result == MISSED) {
        detail = "Missed: this is the answer";
        detail_color = accent = STUDY_GOLD;
    } else if (library.result == WRONG) {
        detail = library.remark ? library.remark : "Try again";
        detail_color = accent = STUDY_RED;
    } else if (library.show_answer) {
        detail = "Hint";
        detail_color = STUDY_GOLD;
    }

    draw_header(heading, library_group()->name, accent);
    if (!library.over) {
        library_counter(counter);
        text_right(221, 53, &FONT_T, counter, STUDY_MUTED);
        if (named || revealed)
            library_target_title(title);
        else
            append_text(title, library_prompt_title());
    }
    if (!library.over && kind == ITEM_ROMAN) {
        RomanLabel label = roman_label(library.fifths, library.minor, library.degree, 0, library.type, 0);
        draw_roman(21, 48, &label, STUDY_INK);
        cv_text(21, 71, &FONT_XS, detail, detail_color);
    } else if (!library.over && kind == ITEM_PROGRESSION) {
        for (int step = 0, x = 21; step < library.steps; ++step) {
            RomanLabel label = progression_label(step);
            int ahead = step > library.step && !library_concluded();
            x = draw_roman(x, 48, &label, step == library.step && !library_concluded() ? STUDY_MINT :
                           (ahead ? STUDY_MUTED : STUDY_INK)) + 12;
        }
        cv_text(21, 71, &FONT_XS, detail, detail_color);
    } else {
        draw_card_text(title, detail, detail_color, library.over ? 202 : 166);
    }
    drawn_playing = detail == playing_text;
    cv_blit(0, 0);
}

static void draw_cell(int x, int y, int w, int h, const char *label, int selected, int tried)
{
    const felucca_font_t *font = h >= 17 ? &FONT_XS : &FONT_T;
    cv_rect(x, y, w - 1, h - 1, selected ? STUDY_MINT : STUDY_CARD);
    cv_text(x + (w - 1 - text_w(font, label)) / 2, y + (h - 1 - font->h) / 2, font, label,
            selected ? STUDY_BG : (tried ? STUDY_RED : STUDY_INK));
}

/* Every answer on offer at once, the chosen one lit: no turning through a hidden list. */
static void draw_choices(int y, int height)
{
    int kind = library_kind(), count = 0, columns, rows, row_height;
    int values[16];
    char labels[16][12];
    int roots = kind == ITEM_CHORD && library_prompt() != PROMPT_SOUND;

    if (kind == ITEM_NOTE) {
        int chromatic = content_roots(LV_NOTES) == 0xFFFu || library.fifths;
        rows = chromatic ? 3 : 1;
        row_height = height / rows < 22 ? height / rows : 22;
        for (int row = 0; row < rows; ++row) {
            for (int letter = 0; letter < 7; ++letter) {
                int accidental = chromatic ? 1 - row : 0;
                spell_text(labels[0], letter, accidental);
                draw_cell(11 + 31 * letter, y + row * row_height, 31, row_height, labels[0],
                          library.choice == letter && library.choice_accidental == accidental, 0);
            }
        }
        return;
    }
    if (kind == ITEM_SIGNATURE) {
        for (int key = 0; key < 15; ++key) {
            if (content_key(key + (library.minor ? 15 : 0))) {
                values[count] = key - 7;
                append_text(labels[count++], key_name(key + (library.minor ? 15 : 0)));
            }
        }
        columns = 5;
    } else {
        uint32_t filter = library_filter(kind);
        for (int quality = 0; quality < library_quality_count(kind); ++quality) {
            if (!((filter >> quality) & 1u))
                continue;
            const Shape *shape = library_shape(kind, quality);
            const char *label = kind == ITEM_SCALE ? shape->symbol + 1 : tritone_or(shape);
            values[count] = quality;
            append_text(labels[count++], *label ? label : "maj");
        }
        columns = kind == ITEM_INTERVAL ? 5 : (kind == ITEM_SCALE ? 4 : 6);
    }
    if (!count)
        return;
    if (count < columns)
        columns = count;
    rows = (count + columns - 1) / columns + roots;
    row_height = height / rows < 22 ? height / rows : 22;
    for (int i = 0; i < count; ++i) {
        int width = 218 / columns;
        draw_cell(11 + width * (i % columns), y + row_height * (i / columns), width, row_height, labels[i],
                  values[i] == library.choice, kind != ITEM_SIGNATURE && ((library.tried >> values[i]) & 1u));
    }
    if (roots) {
        uint32_t pool = content_roots(LV_CHORDS);
        int total = mask_count(pool), width = 218 / total, shown = 0;
        for (int root = 0; root < 12; ++root) {
            if ((pool >> root) & 1u)
                draw_cell(11 + width * shown++, y + row_height * (rows - 1), width, row_height,
                          root_label(root, chord_shape(library.choice)), root == library.choice_root, 0);
        }
    }
}

static void library_progress_line(char out[48])
{
    int mode = play_mode();
    char *p = out;
    if (library_kind() == ITEM_PROGRESSION) {
        p = append_number(append_text(p, "Chord "), library.step + 1);
        p = append_text(append_number(append_text(p, " / "), library.steps), "    ");
    }
    if (mode == PLAY_ANY_OCTAVE || mode == PLAY_EXACT) {
        p = append_number(append_text(p, "Played  "), library.position);
        append_number(append_text(p, " / "), library.count);
    } else if (mode == PLAY_EXACT_SET) {
        p = append_number(append_text(p, "Held  "), note_total(&study.held));
        append_number(append_text(p, " / "), library.count);
    } else {
        uint32_t entry = library.collect ? library.collected : study.held_pitches;
        p = append_number(append_text(p, library.collect ? "Tapped  " : "Held  "), pitch_total(entry));
        append_number(append_text(p, " / "), pitch_total(step_pitches(library.step)));
    }
}

/* The chord symbol of one chord in root position, from the notes of a progression. */
static const char *chord_symbol(const StaffNote *notes, int column)
{
    int size = 1;
    while (notes + size < library.target + library.count && library.column[notes + size - library.target] == column)
        ++size;
    for (int quality = 0; quality < chord_quality_count(); ++quality) {
        const Shape *shape = chord_shape(quality);
        int same = shape->count == size;
        for (int i = 1; same && i < size; ++i)
            same = notes[i].midi - notes[0].midi == shape->semitones[i];
        if (same)
            return shape->symbol;
    }
    return "";
}

static void library_answer_names(char out[56])
{
    char *p = out;
    if (library_kind() == ITEM_SIGNATURE) {
        int signs = library.fifths < 0 ? -library.fifths : library.fifths;
        append_text(append_number(out, signs), library.fifths < 0 ? " flats" : (signs ? " sharps" : " sharps or flats"));
        return;
    }
    if (library_kind() == ITEM_PROGRESSION) {      /* four chords of notes will not fit: their names do */
        for (int i = 0; i < library.count; ++i) {
            if (i && library.column[i] == library.column[i - 1])
                continue;
            p = note_text(i ? append_text(p, "   ") : p, library.target[i], 0);
            p = append_text(p, chord_symbol(&library.target[i], library.column[i]));
        }
        return;
    }
    for (int i = 0; i < library.count; ++i)
        p = note_text(i ? append_text(p, " ") : p, library.target[i], 0);
}

static void library_summary(void)
{
    char line[72], *p;
    cv_begin(240, LOWER_HEIGHT, STUDY_BG);
    p = append_number(line, library.first_try);
    append_text(append_number(append_text(p, " / "), library.seen), " first try");
    cv_text(12, 0, &FONT_M, line, STUDY_MINT);

    p = append_text(append_number(line, library.solved), " solved");
    if (library.timed_answers) {
        uint32_t tenths = library.time_total / (100u * (uint32_t)library.timed_answers);
        p = append_number(append_text(p, "    average "), (int)(tenths / 10u));
        append_text(append_number(append_text(p, "."), (int)(tenths % 10u)), " s");
    }
    cv_text(12, 27, &FONT_XS, line, STUDY_INK);
    if (library_time_limit()) {
        p = append_number(line, (library.first_try + prefs.timer / 2) / prefs.timer);
        p = append_text(p, " a minute");
        if (library.new_best)
            append_text(p, "    New best");
        else
            append_number(append_text(p, "    best "), progress.best[library_activity()]);
        cv_text(12, 43, &FONT_XS, line, library.new_best ? STUDY_GOLD : STUDY_MUTED);
    }

    if (library.missed_count) {
        int y = 61, x = 12;
        cv_caps(12, 61, &FONT_T, "Go over", STUDY_MUTED);
        x = 60;
        for (int i = 0; i < library.missed_count && y < 100; ++i) {
            library_item_title(library.missed[i], line);
            if (x + text_w(&FONT_XS, line) > 228) {
                x = 12;
                y += 15;
            }
            if (y < 100)
                x = cv_text(x, y - 2, &FONT_XS, line, STUDY_INK) + 9;
        }
    } else if (library.seen) {
        cv_text(12, 61, &FONT_XS, "Nothing missed.", STUDY_MUTED);
    }
    if (library.level_offer) {
        int level = kind_level(library_kind());
        p = append_number(append_text(append_text(line, level_names[level]), " level "), progress.level[level] + 1);
        append_text(append_text(p, ": "), level_text[level][progress.level[level]]);
        cv_text(12, 102, &FONT_XS, line, STUDY_GOLD);
    }
    cv_rect(11, 118, 218, 1, STUDY_LINE);
    control_label(11, 120, "EDIT", "New round");
    if (library.missed_count)
        control_label(118, 120, "SEL", "Retry missed");
    if (library.level_offer)
        control_label(11, 131, "SAVE", "Level up");
    control_label(118, 131, "GLO", "Library");
    control_label(11, 141, "ENV", "Settings");
    draw_notice();
    cv_blit(0, TOP_HEIGHT);
}

static void library_task_bottom(void)
{
    int kind = library_kind(), answer = library_answer(), prompt = library_prompt();
    int shown = library_revealed(), open = library_quiz_open();
    int write = answer == ANSWER_WRITE && !shown;
    int staff = shown || prompt == PROMPT_STAFF || answer == ANSWER_WRITE;
    int grid = answer == ANSWER_CHOICE && !shown;
    uint32_t guides = library_guide_keys(), anchors = library_anchor_keys(), wrong = library_wrong_keys();
    char line[56] = "";

    cv_begin(240, LOWER_HEIGHT, STUDY_BG);
    if (staff) {
        uint32_t all = library.count >= 32 ? ~0u : (1u << library.count) - 1u, done = 0;
        if (answer == ANSWER_PLAY && !shown && play_mode() == PLAY_EXACT) {
            for (int i = 0; i < library.position; ++i)
                done |= 1u << (heard_descending() ? library.count - 1 - i : i);
        }
        int stacked = heard_together() || (kind == ITEM_CHORD && answer != ANSWER_PLAY) ||
                      (kind == ITEM_INTERVAL && prompt == PROMPT_SOUND && prefs.order == HEAR_TOGETHER);
        draw_notation(&(Score){
            .notes = write ? library.written : library.target,
            .column = library.column,
            .count = library.count,
            .columns = library.steps,
            .stacked = stacked,
            .filled = write ? (uint32_t)library.filled : all,
            .done = done,
            .wrong = write ? library.wrong_slots : 0,
            .fifths = kind == ITEM_SIGNATURE && write ? library.choice : library.fifths,
            .cursor = write && kind != ITEM_SIGNATURE ? library.cursor : -1,
            .live = shown,
        });
    }

    if (shown) {
        library_answer_names(line);
    } else if (write && kind == ITEM_SIGNATURE) {
        int signs = library.choice < 0 ? -library.choice : library.choice;
        append_text(append_number(line, signs), library.choice < 0 ? " flats" : (signs ? " sharps" : " sharps or flats"));
    } else if (write) {
        char *p = append_text(append_number(append_text(line, "Note "), library.cursor + 1), " / ");
        append_number(p, library.count);
    } else if (answer == ANSWER_PLAY) {
        library_progress_line(line);
    }

    if (grid && prompt == PROMPT_STAFF) {
        draw_choices(80, 38);
    } else if (grid && prompt == PROMPT_KEYS) {
        draw_keyboard(2, 40, guides, library_sound_lights(), anchors, wrong);
        draw_choices(48, 70);
    } else if (grid) {
        draw_choices(6, 110);
    } else if (staff) {
        draw_score_readout(line, STUDY_MINT);
        draw_keyboard(SCORE_KEYS_Y, 117 - SCORE_KEYS_Y, guides, library_sound_lights(), anchors, wrong);
    } else {
        cv_rect(11, 0, 218, 24, STUDY_CARD);
        cv_text(21, 5, &FONT_XS, line, STUDY_MUTED);
        draw_keyboard(52, 65, guides, library_sound_lights(), anchors, wrong);
    }

    cv_rect(11, 118, 218, 1, STUDY_LINE);
    if (open && write && kind != ITEM_SIGNATURE) {
        control_label(11, 120, "SELECT", "Slot");
        control_label(91, 120, "ALGO", "Line");
        control_label(162, 120, "PRESET", "Sign");
        control_label(11, 130, "SEL", "Set");
        control_label(91, 130, "REC", "Erase");
    } else if (open && answer == ANSWER_PLAY) {
        control_label(11, 120, "SELECT", "Activity");
        if (may_collect())
            control_label(91, 120, "ARP", library.collect ? "Tap" : "Hold");
        control_label(162, 120, "REC", holds_keys() ? "Clear" : "Restart");
        control_label(11, 130, "PLAY", "Hear");
        control_label(91, 130, "OCT", "Octave");
    } else if (open) {
        control_label(11, 120, "PRESET", kind == ITEM_NOTE ? "Letter" : (write ? "Signs" : "Answer"));
        if (kind == ITEM_NOTE && (content_roots(LV_NOTES) == 0xFFFu || library.fifths))
            control_label(91, 120, "ALGO", "Sign");
        else if (kind == ITEM_CHORD && prompt != PROMPT_SOUND)
            control_label(91, 120, "ALGO", "Root");
        if (answer != ANSWER_WRITE)
            control_label(162, 120, "SELECT", "Activity");
        if (kind != ITEM_SIGNATURE)
            control_label(11, 130, "PLAY", "Hear");
    } else if (kind != ITEM_SIGNATURE) {
        control_label(11, 130, "PLAY", "Hear");
    }
    if (open) {
        control_label(162, 130, "FX", library.show_answer ? "Hide" : "Hint");
        control_label(11, 140, "SAVE", "Skip");
    }
    if (!open || answer != ANSWER_PLAY || holds_keys())
        control_label(91, 140, "EDIT", open ? "Check" : "Next");
    control_label(162, 140, "GLO", "Library");
    draw_notice();
    cv_blit(0, TOP_HEIGHT);
}

static void library_lights(void)
{
    uint32_t buttons = 1u << B_SEQ, controls = (1u << B_PLAY) | (1u << B_EDIT);
    if (library.page == LIB_MENU) {
        plat_lights(buttons | (1u << B_GLO), study.panel, controls, 0, 2);
        return;
    }
    if (!library.over) {
        controls |= (1u << B_FX) | (1u << B_SAVE) | (may_collect() ? 1u << B_ARP : 0);
        if (demo_playing() || library.result == CORRECT)
            buttons |= LED_PLAY_GREEN;
        if (library.wrong_at)                /* PLAY turns red while a miss registers */
            buttons |= 1u << B_PLAY;
    }
    plat_lights(buttons, study.panel | library_sound_lights(), controls,
                library_guide_keys() | library_anchor_keys(), 2);
}

static void library_frame(uint32_t now)
{
    if (ann_on() && ann_where()) {
        char speech[140], title[56];
        char *p = append_text(speech, library_group()->name);
        p = append_text(append_text(p, ". "), kind_names[library_kind()]);
        if (library.page == LIB_TASK && !library.over) {
            if (library_prompt() == PROMPT_NAME || library_revealed())
                library_target_title(title);
            else
                append_text(title, library_prompt_title());
            append_text(append_text(p, ". "), title);
        }
        ann_say(speech, 0, ANN_NOW);
    }
    ann_tick(now);
    if (!study.dirty)
        return;
    study.dirty = 0;
    library_lights();
    if (library.page == LIB_MENU) {
        library_draw_menu();
    } else {
        library_task_top();
        if (library.over)
            library_summary();
        else
            library_task_bottom();
    }
}

#ifdef PLAT_HOST
static int library_expect(const char *what, int *got)
{
    if (!strcmp(what, "lib_page")) *got = library.page;
    else if (!strcmp(what, "lib_group")) *got = library.group;
    else if (!strcmp(what, "lib_item")) *got = library.item;
    else if (!strcmp(what, "lib_menu_group")) *got = library.menu_group;
    else if (!strcmp(what, "lib_menu_item")) *got = library.menu_item;
    else if (!strcmp(what, "lib_kind")) *got = library_kind();
    else if (!strcmp(what, "lib_activity")) *got = library_activity();
    else if (!strcmp(what, "lib_count")) *got = library.count;
    else if (!strcmp(what, "lib_steps")) *got = library.steps;
    else if (!strcmp(what, "lib_step")) *got = library.step;
    else if (!strcmp(what, "lib_position")) *got = library.position;
    else if (!strcmp(what, "lib_result")) *got = library.result;
    else if (!strcmp(what, "lib_misses")) *got = library.misses;
    else if (!strcmp(what, "lib_seen")) *got = library.seen;
    else if (!strcmp(what, "lib_solved")) *got = library.solved;
    else if (!strcmp(what, "lib_first_try")) *got = library.first_try;
    else if (!strcmp(what, "lib_missed")) *got = library.missed_count;
    else if (!strcmp(what, "lib_over")) *got = library.over;
    else if (!strcmp(what, "lib_review")) *got = library.review;
    else if (!strcmp(what, "lib_offer")) *got = library.level_offer;
    else if (!strcmp(what, "lib_revealed")) *got = library_revealed();
    else if (!strcmp(what, "lib_lights")) *got = (int)library_sound_lights();
    else if (!strcmp(what, "lib_guides")) *got = (int)library_guide_keys();
    else if (!strcmp(what, "lib_anchors")) *got = (int)library_anchor_keys();
    else if (!strcmp(what, "lib_wrong_keys")) *got = (int)library_wrong_keys();
    else if (!strcmp(what, "lib_filled")) *got = library.filled;
    else if (!strcmp(what, "lib_wrong_slots")) *got = (int)library.wrong_slots;
    else if (!strcmp(what, "lib_fifths")) *got = library.fifths;
    else if (!strcmp(what, "lib_minor")) *got = library.minor;
    else if (!strcmp(what, "lib_choice")) *got = library.choice;
    else if (!strcmp(what, "lib_choice_root")) *got = library.choice_root;
    else if (!strcmp(what, "lib_quality")) *got = library.quality;
    else if (!strcmp(what, "lib_root")) *got = library.root;
    else if (!strcmp(what, "lib_inversion")) *got = library.inversion;
    else if (!strcmp(what, "lib_collect")) *got = library.collect;
    else if (!strcmp(what, "lib_collected")) *got = (int)library.collected;
    else if (!strcmp(what, "lib_tried")) *got = (int)library.tried;
    else if (!strcmp(what, "lib_hinted")) *got = library.hinted;
    else if (!strcmp(what, "lib_again")) *got = library.again_count;
    else if (!strcmp(what, "lib_says_playing")) *got = drawn_playing;
    else if (!strcmp(what, "lib_bag")) {
        *got = 0;
        for (int i = 0; i < BAG_WORDS; ++i)
            *got += pitch_total(library.bag[i]);
    }
    else if (!strcmp(what, "lib_item_id")) *got = library.item_id;
    else if (!strcmp(what, "lib_items")) *got = item_total();
    else if (!strcmp(what, "lib_box")) *got = library.stat < 0 ? -1 : progress.box[library.stat];
    else if (!strcmp(what, "lib_best")) *got = progress.best[library_activity()];
    else if (!strcmp(what, "lib_given")) *got = library.given;
    else if (!strcmp(what, "lib_note_letter")) *got = positive_mod(library.target[0].position, 7);
    else if (!strcmp(what, "lib_note_acc")) *got = library.target[0].accidental;
    else if (!strncmp(what, "lib_note", 8) && what[8] >= '0' && what[8] <= '9') *got = library.target[atoi(what + 8)].midi;
    else if (!strcmp(what, "lib_first_note")) *got = library.target[0].midi;
    else if (!strcmp(what, "lib_last_note")) *got = library.target[library.count - 1].midi;
    else if (!strcmp(what, "lib_clock")) *got = (int)library.clock_ms;
    else if (!strcmp(what, "lib_allowed")) {
        *got = 0;
        for (int i = 0; i < item_total(); ++i)
            *got += item_allowed(i);
    }
    else return 0;
    return 1;
}
#endif
