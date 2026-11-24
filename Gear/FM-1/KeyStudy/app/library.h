/* SPDX-License-Identifier: MPL-2.0 */
static int library_kind(void)
{
    return library_groups[library.group].kinds[library.item];
}

static const char *library_item_name(void)
{
    static const char *const names[] = {
        "Note", "Key Signature", "Interval", "Scale", "Chord",
        "Reverse Note", "Play the Note", "Accidental", "Roman Analysis", "12-Tone Matrix"
    };
    if (library.group == LIB_EAR && library_kind() == ITEM_NOTE)
        return "Name the Note";
    return names[library_kind()];
}

static const char *library_key_name(int fifths)
{
    return (library.minor ? minor_keys : major_keys)[fifths + 7];
}

static const Shape *library_shape(int kind, int quality)
{
    if (kind == ITEM_CHORD)
        return chord_shape(quality);
    return kind == ITEM_INTERVAL ? &intervals[quality] : &scales[quality];
}

static int library_quality_count(int kind)
{
    if (kind == ITEM_CHORD)
        return chord_quality_count();
    return topics[kind == ITEM_INTERVAL ? INTERVALS : SCALES].count;
}

static uint32_t library_filter(int kind)
{
    if (kind == ITEM_INTERVAL)
        return prefs.intervals;
    if (kind == ITEM_CHORD)
        return prefs.chords;
    if (kind == ITEM_SCALE)
        return prefs.scales;
    if (!prefs.naturals)
        return (1u << 21) - 1u;
    uint32_t naturals = 0;
    for (int i = 0; i < 7; ++i)
        naturals |= 1u << (3 * i + 1);
    return naturals;
}

static StaffNote library_root_note(int root)
{
    return canonical_note(FIRST_KEY + wrap_index(root - FIRST_KEY, 12));
}

static StaffNote library_note_option(int index)
{
    return written_note(index / 3, index % 3 - 1);
}

static StaffNote library_blank_note(void)
{
    return written_note(prefs.clef == CLEF_BASS ? -4 : 0, 0);
}

static int library_is_direct(void)
{
    int kind = library_kind();
    return kind == ITEM_REVERSE || kind == ITEM_EAR_KEYS;
}

static int library_revealed(void)
{
    return library.show_answer || library.result == CORRECT || library.result == SKIPPED;
}

static int library_finished(void)
{
    return library.seen >= prefs.length && library.result == READY;
}

static int library_quiz_open(void)
{
    return library.group != LIB_CALC && library.result != CORRECT &&
           library.result != SKIPPED && !library_finished();
}

static void library_silence(void)
{
    library.demo_count = 0;
    study.demo_notes = (NoteSet){{0}};
}

static void library_play(uint32_t now)
{
    int kind = library_kind();
    int index = 0;
    int ear_interval = kind == ITEM_INTERVAL && library.group == LIB_EAR;
    library.demo_block = kind == ITEM_CHORD || kind == ITEM_ROMAN ||
                         (ear_interval && prefs.order == HEAR_TOGETHER);
    if (kind == ITEM_MATRIX) {
        for (int i = 0; i < library.row_count; ++i)
            library.demo_notes[index++] = (uint8_t)(60 + library.row[i]);
    } else if (kind != ITEM_SIGNATURE) {
        if (library.group == LIB_EAR && (kind == ITEM_NOTE || kind == ITEM_EAR_KEYS))
            library.demo_notes[index++] = 60;
        for (int i = 0; i < library.count; ++i) {
            int note = ear_interval && prefs.order == HEAR_DESCENDING ? library.count - 1 - i : i;
            library.demo_notes[index++] = (uint8_t)library.target[note].midi;
        }
    }
    library.demo_count = index;
    library.demo_at = now;
    study.dirty = 1;
}

static void library_audio(uint32_t now)
{
    NoteSet next = {{0}};
    if (library.demo_count) {
        uint32_t elapsed = now - library.demo_at;
        uint32_t step = elapsed / 320u;
        if (library.demo_block && elapsed < 900) {
            for (int i = 0; i < library.demo_count; ++i)
                key_add(&next, library.demo_notes[i]);
        } else if (!library.demo_block && step < (uint32_t)library.demo_count) {
            if (elapsed % 320u < 225)
                key_add(&next, library.demo_notes[step]);
        } else {
            library.demo_count = 0;
        }
    }
    for (int i = 0; i < 4; ++i)
        if (next.word[i] != study.demo_notes.word[i])
            study.dirty = 1;
    study.demo_notes = next;
}

static void library_calculate(void)
{
    int kind = library_kind();
    if (kind == ITEM_MATRIX) {
        library.count = 0;
    } else if (kind == ITEM_ACCIDENTAL) {
        library.count = 1;
        library.target[0] = written_note(library.typed_position, library.typed_accidental);
    } else if (kind == ITEM_ROMAN) {
        library.count = library.quality < 2 ? 3 + library.quality : chord_shape(library.quality - 2)->count;
        library.inversion = clamp(library.inversion, 0, library.count - 1);
        roman_notes(library.target, library.fifths, library.minor, library.degree,
                    library.alteration, library.quality, library.inversion, library.count);
        if (library.target[library.count - 1].midi > LAST_KEY ||
            library.target[library.count - 1].position > 12) {
            for (int i = 0; i < library.count; ++i) {
                library.target[i].midi -= 12;
                library.target[i].position -= 7;
            }
        }
    } else {
        const Shape *shape = library_shape(kind, library.quality);
        StaffNote root = library_root_note(library.root);
        library.count = shape->count;
        for (int i = 0; i < shape->count; ++i)
            library.target[i] = shape_note(root, shape, i);
    }
}

static void library_question(void)
{
    int kind = library_kind();
    uint32_t random = next_random();
    library.result = READY;
    library.missed = 0;
    library.hinted = 0;
    library.show_answer = 0;
    library.wait_release = 1;
    library.count = 0;
    library.cursor = 0;
    library.typed_position = 0;
    library.filled = 0;
    library.choice = kind == ITEM_NOTE ? 1 :
        (kind == ITEM_INTERVAL || kind == ITEM_CHORD || kind == ITEM_SCALE ? mask_choice(library_filter(kind), 0) : 0);
    library.root = mask_choice(quiz_roots(kind == ITEM_NOTE || library_is_direct()), random);
    library_silence();
    study.dirty = 1;
    if (library_finished())
        return;

    if (kind == ITEM_SIGNATURE) {
        library.fifths = (int)(random % 15u) - 7;
        library.minor = (int)((random >> 12) & 1u);
    } else if (kind == ITEM_NOTE || library_is_direct()) {
        library.count = 1;
        int written_prompt = library.group == LIB_STAFF || library.group == LIB_WRITE;
        int first = prefs.clef == CLEF_BASS ? FIRST_KEY : 60;
        if (written_prompt) {
            StaffNote note = library_note_option(mask_choice(library_filter(ITEM_NOTE), random));
            int midi = first + wrap_index(note.midi - first, 12);
            int octaves = (midi - note.midi) / 12;
            library.target[0] = written_note(note.position + 7 * octaves, note.accidental);
        } else if (kind == ITEM_REVERSE) {
            library.target[0] = canonical_note(first + wrap_index(library.root - first, 12));
        } else {
            library.target[0] = canonical_note(60 + library.root);
        }
    } else {
        library.quality = mask_choice(library_filter(kind), random >> 9);
        const Shape *shape = library_shape(kind, library.quality);
        StaffNote root = library_root_note(library.root);
        library.count = shape->count;
        for (int i = 0; i < shape->count; ++i)
            library.target[i] = shape_note(root, shape, i);
    }
    if (library.group == LIB_WRITE) {
        for (int i = 0; i < library.count; ++i)
            library.written[i] = library_blank_note();
        if (library.count > 1) {
            library.written[0] = library.target[0];
            library.filled = 1;
            library.cursor = 1;
        }
    }
}

static void library_start(void)
{
    library.page = LIB_TASK;
    library.seen = library.solved = library.first_try = 0;
    library.result = READY;
    if (library.group == LIB_CALC) {
        library.root = library.degree = library.quality = library.inversion = library.alteration = 0;
        library.fifths = library.minor = library.typed_position = library.typed_accidental = 0;
        library_calculate();
    } else {
        library_question();
    }
    study.dirty = 1;
}

static void library_check(int correct)
{
    library.result = correct ? CORRECT : WRONG;
    if (correct) {
        ++library.seen;
        ++library.solved;
        library.first_try += !library.missed && !library.hinted;
        library_silence();
    } else {
        library.missed = 1;
    }
    study.dirty = 1;
}

static void library_submit(void)
{
    int kind = library_kind();
    if (kind == ITEM_SIGNATURE) {
        library_check(library.choice == library.fifths);
    } else if (library.group == LIB_WRITE) {
        int correct = library.filled == (1 << library.count) - 1;
        for (int i = 0; correct && i < library.count; ++i)
            correct = library.target[i].position == library.written[i].position &&
                      library.target[i].accidental == library.written[i].accidental;
        library_check(correct);
    } else if (kind == ITEM_NOTE) {
        StaffNote guess = library_note_option(library.choice);
        StaffNote target = library.target[0];
        if (library.group == LIB_STAFF)
            library_check(positive_mod(target.position, 7) == guess.position && target.accidental == guess.accidental);
        else
            library_check(guess.midi % 12 == target.midi % 12);
    } else if (!library_is_direct()) {
        int correct = library.choice == library.quality;
        if (kind == ITEM_CHORD && library.group != LIB_EAR)
            correct = correct && library.typed_position == library.root;
        library_check(correct);
    }
}

static void library_set_written(StaffNote note)
{
    library.written[library.cursor] = note;
    library.filled |= 1 << library.cursor;
    library.result = READY;
    study.dirty = 1;
}

static void library_advance_cursor(int delta)
{
    int first = library.count > 1;
    library.cursor = first + wrap_index(library.cursor - first + delta, library.count - first);
    study.dirty = 1;
}

static void library_key(int midi)
{
    if (library.page != LIB_TASK)
        return;
    int kind = library_kind();
    if (library.group == LIB_CALC && kind == ITEM_MATRIX) {
        int pitch = midi % 12;
        if (library.row_count < 12 && !(library.row_mask & (1 << pitch))) {
            library.row[library.row_count++] = (uint8_t)pitch;
            library.row_mask |= 1 << pitch;
            library_silence();
        }
    } else if (library_quiz_open() && !library.wait_release) {
        if (library_is_direct() && !library.demo_count) {
            library.wait_release = 1;
            library_check(midi == library.target[0].midi);
        } else if (library.group == LIB_WRITE && kind != ITEM_SIGNATURE && midi >= FIRST_KEY && midi <= LAST_KEY) {
            library_set_written(canonical_note(midi));
            if (library.cursor < library.count - 1)
                library_advance_cursor(1);
        }
    }
}

static void library_calc_controls(uint32_t press, int select, int algo, int preset, int k1, int k2)
{
    int kind = library_kind();
    if (kind == ITEM_MATRIX) {
        if ((press & (1u << B_REC)) && library.row_count)
            library.row_mask &= ~(1 << library.row[--library.row_count]);
        if (press & (1u << B_FX))
            library.row_count = library.row_mask = 0;
    } else if (kind == ITEM_ACCIDENTAL) {
        library.typed_position = clamp(library.typed_position + select, -6, 11);
        library.fifths = wrap_index(library.fifths + algo + 7, 15) - 7;
        library.typed_accidental = clamp(library.typed_accidental + preset, -2, 2);
    } else if (kind == ITEM_ROMAN) {
        library.fifths = wrap_index(library.fifths + select + 7, 15) - 7;
        library.degree = wrap_index(library.degree + algo, 7);
        library.quality = wrap_index(library.quality + preset, chord_quality_count() + 2);
        library.inversion = clamp(library.inversion + k1, 0, 3);
        library.alteration = clamp(library.alteration + k2, -1, 1);
        if (press & (1u << B_REC))
            library.minor ^= 1;
    } else {
        library.root = wrap_index(library.root + preset, 12);
        library.quality = wrap_index(library.quality + algo, library_quality_count(kind));
        if (kind != ITEM_SCALE)
            library.fifths = wrap_index(library.fifths + select + 7, 15) - 7;
    }
    library_silence();
    library_calculate();
    study.dirty = 1;
}

static void library_quiz_controls(uint32_t press, int select, int algo, int preset)
{
    int kind = library_kind();
    if (press & (1u << B_EDIT)) {
        if (library_finished()) {
            library_start();
            return;
        }
        if (!library_quiz_open()) {
            library_question();
            return;
        }
    }
    if (!library_quiz_open())
        return;

    if (kind == ITEM_SIGNATURE) {
        library.choice = wrap_index(library.choice + preset + 7, 15) - 7;
    } else if (library.group == LIB_WRITE) {
        if (select)
            library_advance_cursor(select);
        if (algo || preset || (press & (1u << B_SEL))) {
            StaffNote note = library.written[library.cursor];
            int position = clamp(note.position + algo, -7, 12);
            int accidental = clamp(note.accidental + preset, -2, 2);
            library_set_written(written_note(position, accidental));
            if (press & (1u << B_SEL))
                library_advance_cursor(1);
        }
        if (press & (1u << B_REC)) {
            library.filled &= ~(1 << library.cursor);
            library.written[library.cursor] = library_blank_note();
            library.result = READY;
        }
    } else if (!library_is_direct()) {
        library.choice = mask_step(library_filter(kind), library.choice, preset);
        if (kind == ITEM_CHORD && library.group != LIB_EAR)
            library.typed_position = wrap_index(library.typed_position + algo, 12);
    }
    if (select || algo || preset)
        library.result = READY;
    if (press & (1u << B_FX)) {
        library.hinted = 1;
        library.show_answer ^= 1;
    }
    if (press & (1u << B_SAVE)) {
        ++library.seen;
        library.result = SKIPPED;
        library_silence();
    } else if (press & (1u << B_EDIT)) {
        library_submit();
    }
    if (press || select || algo || preset)
        study.dirty = 1;
}

static int library_controls(uint32_t press, uint32_t now, int select, int algo, int preset)
{
    if (press & (1u << B_GLO)) {
        if (library.page == LIB_CLOSED) {
            stop_demo();
            study.entry_changed = 0;
            study.wait_release = 1;
            if (!study.practice && library.group != LIB_CALC) {
                library.group = LIB_CALC;
                library.item = 0;
            } else if (study.practice && library.group == LIB_CALC) {
                library.group = LIB_STAFF;
                library.item = 0;
            }
        }
        library.page = library.page == LIB_MENU ? LIB_CLOSED : LIB_MENU;
        library_silence();
        study.dirty = 1;
        return 1;
    }
    if (library.page == LIB_CLOSED)
        return 0;

    if (library.page == LIB_MENU) {
        if (select && study.practice) {
            library.group = wrap_index(library.group + select, LIB_CALC);
            library.item = 0;
        }
        if (preset)
            library.item = wrap_index(library.item + preset, library_groups[library.group].count);
        if (press & ((1u << B_EDIT) | (1u << B_SEL)))
            library_start();
        if (select || preset)
            study.dirty = 1;
        return 1;
    }

    int k1 = plat_enc(EN_K1), k2 = plat_enc(EN_K2);
    uint32_t calculator_buttons = (1u << B_REC) | (1u << B_FX);
    if (library.group == LIB_CALC) {
        if ((press & calculator_buttons) || select || algo || preset || k1 || k2)
            library_calc_controls(press, select, algo, preset, k1, k2);
    } else {
        library_quiz_controls(press, select, algo, preset);
    }
    if ((press & (1u << B_PLAY)) && !library_finished() &&
        (library.group == LIB_EAR || library.group == LIB_CALC || library_revealed()))
        library_play(now);
    return 1;
}
