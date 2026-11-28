/* SPDX-License-Identifier: MPL-2.0 */
static const LibraryGroupInfo *library_group(void)
{
    return &library_groups[library.group];
}

static int library_kind(void) { return library_group()->kinds[library.item]; }
static int library_prompt(void) { return library_group()->prompt; }
static int library_answer(void) { return library_group()->answer; }

static int library_activity(void)
{
    int index = library.item;
    for (int group = 0; group < library.group; ++group)
        index += library_groups[group].count;
    return index;
}

/* Where an activity's best score is kept, and how SEQ finds it again after power off. */
static int activity_slot(int group, int item)
{
    int index = item;
    if (group == LIB_SIGHT && library_groups[group].kinds[item] == ITEM_MELODY)
        return SLOT_STAFF_MELODY;
    for (int before = 0; before < group; ++before)
        index += library_groups[before].count;
    return group > LIB_SIGHT ? index - 1 : index;
}

static int library_slot(void)
{
    return activity_slot(library.group, library.item);
}

static int kind_level(int kind)
{
    switch (kind) {
    case ITEM_INTERVAL: return LV_INTERVALS;
    case ITEM_SCALE: return LV_SCALES;
    case ITEM_CHORD:
    case ITEM_INVERSION: return LV_CHORDS;
    case ITEM_NOTE:
    case ITEM_MELODY: return LV_NOTES;
    default: return LV_KEYS;
    }
}

static const Shape *library_shape(int kind, int quality)
{
    if (kind == ITEM_INTERVAL)
        return &intervals[quality];
    return kind == ITEM_SCALE ? &scales[quality] : chord_shape(quality);
}

static int library_quality_count(int kind)
{
    if (kind == ITEM_INTERVAL || kind == ITEM_SCALE)
        return topics[kind == ITEM_INTERVAL ? INTERVALS : SCALES].count;
    return chord_quality_count();
}

/* Keys and sound cannot tell an augmented fourth from a diminished fifth: there it is one tritone. */
static int one_tritone(void)
{
    return library_kind() == ITEM_INTERVAL && library_prompt() != PROMPT_NAME && library_prompt() != PROMPT_STAFF;
}

static uint32_t library_filter(int kind)
{
    uint32_t mask = kind == ITEM_INTERVAL ? content_intervals() :
                    (kind == ITEM_SCALE ? content_scales() : content_chords());
    if (one_tritone()) {
        if (mask & (1u << INTERVAL_D5))
            mask |= 1u << INTERVAL_A4;
        mask &= ~(1u << INTERVAL_D5);
    }
    /* Naming one thing out of one is no question: the first scale levels add the natural minor. */
    if (kind == ITEM_SCALE && library_answer() == ANSWER_CHOICE && mask == 1u)
        mask = 3u;
    return mask;
}

static int answer_is_chord(void)
{
    int kind = library_kind();
    return kind == ITEM_CHORD || kind == ITEM_INVERSION || kind == ITEM_ROMAN || kind == ITEM_PROGRESSION;
}

static int heard_together(void)
{
    return answer_is_chord() || (library_kind() == ITEM_INTERVAL && library_prompt() == PROMPT_SOUND &&
                                 prefs.order == HEAR_TOGETHER);
}

static int heard_descending(void)
{
    return library_kind() == ITEM_INTERVAL && library_prompt() == PROMPT_SOUND && prefs.order == HEAR_DESCENDING;
}

static int play_mode(void)
{
    if (answer_is_chord())
        return library_prompt() == PROMPT_STAFF ? PLAY_EXACT_SET : PLAY_PITCH_SET;
    if (heard_together())
        return PLAY_EXACT_SET;
    return library_prompt() == PROMPT_NAME ? PLAY_ANY_OCTAVE : PLAY_EXACT;
}

static int holds_keys(void)
{
    int mode = play_mode();
    return library_answer() == ANSWER_PLAY && (mode == PLAY_PITCH_SET || mode == PLAY_EXACT_SET);
}

/* Tap mode gathers a chord a note at a time. A written chord and an inversion need the real keys. */
static int may_collect(void)
{
    return library_answer() == ANSWER_PLAY && play_mode() == PLAY_PITCH_SET && library_kind() != ITEM_INVERSION;
}

static int library_concluded(void)
{
    return library.result == CORRECT || library.result == SKIPPED || library.result == MISSED;
}

static int library_revealed(void)
{
    return library.show_answer || library_concluded();
}

/* The first hint on a heard question shows it the other way round: its keys when the answer is
 * a name, its name when the answer is the keys. */
static int clue_shows_keys(void)
{
    return library.clue && !library_revealed() && library_prompt() == PROMPT_SOUND &&
           library_answer() == ANSWER_CHOICE;
}

static int clue_shows_name(void)
{
    return library.clue && library_prompt() == PROMPT_SOUND && library_answer() == ANSWER_PLAY;
}

static int library_quiz_open(void)
{
    return !library.over && !library_concluded();
}

static int library_active(void)
{
    return study.practice && library.page == LIB_TASK && !settings.open;
}

static int library_answering(void)
{
    return library_active() && library_quiz_open();
}

static int library_round_length(void)
{
    return library.review ? library.queue_count : prefs.length;
}

static uint32_t library_time_limit(void)
{
    return library.review ? 0 : prefs.timer * 60000u;
}

/* An answer this quick came from knowing it, not from working it out. */
static uint32_t fluent_ms(void)
{
    int answer = library_answer();
    if (answer == ANSWER_WRITE)
        return 4000u * (uint32_t)(library.count ? library.count : 2);
    if (answer == ANSWER_CHOICE)
        return library_kind() == ITEM_CHORD && library_prompt() != PROMPT_SOUND ? 7000u : 5000u;
    return 2000u + 1000u * (uint32_t)library.count;
}

static int written_notes(void)
{
    return library_kind() == ITEM_NOTE && (library_prompt() == PROMPT_STAFF || library_answer() == ANSWER_WRITE);
}

static int white_position(int midi, int step)
{
    while (!((NATURAL_MASK >> (midi % 12)) & 1u))
        midi += step;
    return canonical_note(midi).position;
}

/* The staff positions a note question draws from: the staff itself, with up to three ledger lines
 * either side once those are in play. A note to be played also has to be on the keys. */
static void note_range(int *low, int *high)
{
    int bottom = staff_bottom(), margin = content_ledger() ? 6 : 0;
    int lo = bottom - margin, hi = bottom + 8 + margin;
    if (library_answer() == ANSWER_PLAY) {
        int reach_lo = white_position(base_key(), 1), reach_hi = white_position(base_key() + NKEYS - 1, -1);
        if (lo < reach_lo)
            lo = reach_lo;
        if (hi > reach_hi)
            hi = reach_hi;
        if (hi - lo < 4) {
            lo = reach_lo > bottom - 6 ? reach_lo : bottom - 6;
            hi = reach_hi < bottom + 14 ? reach_hi : bottom + 14;
        }
        if (hi - lo < 4) {
            lo = reach_lo;
            hi = reach_hi;
        }
    }
    *low = lo;
    *high = hi;
}

static int item_total(void)
{
    int kind = library_kind(), lo, hi;
    switch (kind) {
    case ITEM_NOTE:
        if (!written_notes())
            return 12;
        note_range(&lo, &hi);
        return 3 * (hi - lo + 1);
    case ITEM_SIGNATURE: return KEY_COUNT;
    case ITEM_INTERVAL:
    case ITEM_SCALE:
    case ITEM_CHORD:
    case ITEM_INVERSION: return 12 * library_quality_count(kind);
    case ITEM_DEGREE:
    case ITEM_ROMAN: return 7 * KEY_COUNT;
    case ITEM_PROGRESSION: return PROGRESSION_COUNT * KEY_COUNT;
    default: return 1;
    }
}

static int item_allowed(int item)
{
    int kind = library_kind(), depth = content_key_depth();
    switch (kind) {
    case ITEM_NOTE:
        if (written_notes())
            return item % 3 == 1 || content_roots(LV_NOTES) == 0xFFFu;
        return (content_roots(LV_NOTES) >> item) & 1u;
    case ITEM_SIGNATURE: return content_key(item);
    case ITEM_INTERVAL:
    case ITEM_SCALE:
    case ITEM_CHORD:
    case ITEM_INVERSION:
        return ((library_filter(kind) >> (item / 12)) & 1u) && ((content_roots(kind_level(kind)) >> (item % 12)) & 1u);
    case ITEM_DEGREE: return content_key(item / 7);
    case ITEM_ROMAN: {
        int degree = item % 7;
        return content_key(item / 7) && (depth > 1 || degree == 0 || degree == 3 || degree == 4);
    }
    case ITEM_PROGRESSION: {
        int key = item / PROGRESSION_COUNT, which = item % PROGRESSION_COUNT;
        int known = depth == 1 ? 1 : (depth == 2 ? 5 : PROGRESSION_COUNT);
        return content_key(key) && progressions[which].minor == key_minor(key) && which < known;
    }
    default: return 1;
    }
}

static int item_stat(int item)
{
    int kind = library_kind(), sound = library_prompt() == PROMPT_SOUND, lo, hi;
    switch (kind) {
    case ITEM_NOTE:
        if (!written_notes())
            return (sound ? S_EAR_PITCH : S_PITCH) + item;
        note_range(&lo, &hi);
        lo += item / 3 - (staff_bottom() - 6);
        return lo < 0 || lo >= 24 ? -1 : (prefs.clef == CLEF_BASS ? S_BASS : S_TREBLE) + lo;
    case ITEM_SIGNATURE: return S_SIGNATURE + item;
    case ITEM_INTERVAL: return (sound ? S_EAR_INTERVAL : S_INTERVAL) + item / 12;
    case ITEM_SCALE: return sound ? S_EAR_SCALE + item / 12 : S_SCALE + item;
    case ITEM_CHORD:
    case ITEM_INVERSION: return sound ? S_EAR_CHORD + item / 12 : S_CHORD + item;
    case ITEM_DEGREE:
    case ITEM_ROMAN: return S_KEY + item;
    default: return -1;
    }
}

static int item_open(int item)
{
    return item_allowed(item) && !((library.bag[item >> 5] >> (item & 31)) & 1u) && item != library.last_item;
}

static int open_weight(int total)
{
    int sum = 0;
    for (int i = 0; i < total; ++i)
        if (item_open(i))
            sum += progress_weight(item_stat(i));
    return sum;
}

/* Draw the next item. Nothing comes back until the rest have had a turn; what the player keeps
 * missing is drawn sooner, and what is known cold sits out every other time round. */
static int library_pick(uint32_t random)
{
    int total = item_total(), sum = open_weight(total);
    if (!sum) {
        for (int i = 0; i < BAG_WORDS; ++i)
            library.bag[i] = 0;
        for (int i = 0; i < total; ++i) {
            int stat = item_stat(i);
            if (stat >= 0 && progress.box[stat] == BOX_TOP && (next_random() & 1u))
                library.bag[i >> 5] |= 1u << (i & 31);
        }
        sum = open_weight(total);
    }
    if (!sum) {
        for (int i = 0; i < BAG_WORDS; ++i)
            library.bag[i] = 0;
        sum = open_weight(total);
    }
    if (sum) {
        int pick = (int)(random % (uint32_t)sum);
        for (int i = 0; i < total; ++i) {
            if (item_open(i) && (pick -= progress_weight(item_stat(i))) < 0)
                return i;
        }
    }
    for (int i = 0; i < total; ++i)          /* a pool of one */
        if (item_allowed(i))
            return i;
    return 0;
}

static uint32_t major_keys_in_play(void)
{
    uint32_t keys = 0;
    for (int key = 0; key < 15; ++key)
        if (content_key(key))
            keys |= 1u << key;
    return keys;
}

static void build_note(int item, uint32_t random)
{
    int base = base_key(), prompt = library_prompt();
    library.count = 1;
    if (written_notes()) {
        int lo, hi, plain = 0;
        note_range(&lo, &hi);
        int position = lo + item / 3, accidental = item % 3 - 1;
        if (prompt == PROMPT_STAFF && content_signatures()) {
            /* The signature sets the plain note. A written sign then changes it: a natural where
             * the signature alters the letter, a sharp or a flat where it does not. */
            library.fifths = mask_choice(major_keys_in_play(), random >> 8) - 7;
            plain = signature_alteration(library.fifths, positive_mod(position, 7));
            if (!accidental)
                accidental = plain;
            else if (plain)
                accidental = 0;
        }
        library.target[0] = written_note(position, accidental);
        if (library_answer() == ANSWER_PLAY && !panel_bit(library.target[0].midi)) {
            /* The panel's end keys are white: a sign can push the note off them. Drop the written
             * sign first; if the signature itself does it, ask the note without a signature. */
            library.target[0] = written_note(position, plain);
            if (!panel_bit(library.target[0].midi)) {
                library.fifths = 0;
                library.target[0] = written_note(position, 0);
            }
        }
    } else {
        int midi = base + positive_mod(item - base, 12);
        const uint8_t *letters = sharp_letter;
        library.root = item;
        library.flat = (FLAT_BY_DEFAULT >> item) & 1;
        if (prompt == PROMPT_NAME)               /* a black key goes by either name */
            library.flat = (random >> 4) & 1;
        if (library.flat)
            letters = flat_letter;
        if (prompt == PROMPT_SOUND) {
            library.given = base + positive_mod(-base, 12);
            midi = library.given + item;
        } else if (prompt == PROMPT_KEYS) {
            midi += 12 * (int)(random % (uint32_t)((base + NKEYS - 1 - midi) / 12 + 1));
        }
        library.target[0] = spelled_note(midi, (ToneSpelling){letters[item], letter_accidental(letters[item], item)});
    }
}

static void build_shape(int item, uint32_t random)
{
    int kind = library_kind(), prompt = library_prompt(), base = base_key();
    library.quality = item / 12;
    library.root = item % 12;
    const Shape *shape = library_shape(kind, library.quality);
    StaffNote root = shape_root(base + positive_mod(library.root - base, 12), shape);
    library.count = shape->count;
    for (int i = 0; i < shape->count; ++i)
        library.target[i] = shape_note(root, shape, i);
    /* A suspended chord turned over is the other suspended chord (Gsus4 over C is Csus2), so
     * where the chord has to be recognised those two stay in root position. */
    int turns = kind == ITEM_CHORD && prompt != PROMPT_NAME && content_inversions() &&
                shape != &triads[4] && shape != &triads[5];
    if (kind == ITEM_INVERSION)
        library.inversion = 1 + (int)(random % (uint32_t)(shape->count - 1));
    else if (turns)
        library.inversion = (int)(random % shape->count);
    if (library.inversion) {
        invert_notes(library.target, library.count, library.inversion);
        fit_notes(library.target, library.count);
    }
    if (prompt == PROMPT_SOUND && library_answer() == ANSWER_PLAY) {
        int first = heard_descending() ? library.count - 1 : 0;
        if (kind == ITEM_CHORD)
            first = (library.count - library.inversion) % library.count;
        library.given = library.target[first].midi;
    }
}

static void build_in_key(int item, uint32_t random)
{
    int kind = library_kind();
    int key = kind == ITEM_PROGRESSION ? item / PROGRESSION_COUNT : item / 7;
    library.fifths = key_fifths(key);
    library.minor = key_minor(key);
    library.degree = item % 7;
    if (kind == ITEM_DEGREE) {
        library.count = 1;
        library.target[0] = key_degree(library.fifths, library.minor, library.degree, 0);
        fit_notes(library.target, 1);
    } else if (kind == ITEM_ROMAN) {
        library.type = content_key_depth() >= 3 && (random & 1u);
        if (library.minor && library.degree == 4)       /* the raised seventh degree makes V major */
            library.type = library.type ? 10 : 2;
        library.count = roman_count(library.type);
        roman_notes(library.target, library.fifths, library.minor, library.degree, 0, library.type, 0);
        fit_notes(library.target, library.count);
    } else {
        const Progression *progression = &progressions[item % PROGRESSION_COUNT];
        library.quality = item % PROGRESSION_COUNT;
        library.steps = progression->count;
        for (int step = 0; step < progression->count; ++step) {
            StaffNote chord[4];
            int count = roman_count(progression->type[step]);
            roman_notes(chord, library.fifths, library.minor, progression->degree[step], 0, progression->type[step], 0);
            fit_notes(chord, count);
            for (int i = 0; i < count; ++i) {
                library.target[library.count] = chord[i];
                library.column[library.count++] = (uint8_t)step;
            }
        }
    }
}

/* The note a staff position holds under the question's key signature. */
static StaffNote key_note(int position)
{
    return written_note(position, signature_alteration(library.fifths, positive_mod(position, 7)));
}

/* The next move of a tune, in scale steps: mostly by step, some thirds, and once the player is
 * past the first levels a few leaps of a fourth or fifth. */
static int melody_move(int leaps)
{
    uint32_t dice = next_random(), roll = dice % 100u;
    int size = roll < (leaps ? 50u : 65u) ? 1 : 2;
    if (leaps && roll >= 82u)
        size = roll >= 92u ? 4 : 3;
    return (dice >> 16) & 1u ? size : -size;
}

/* A short tune in a major key that starts on the tonic and stays on the keys. Read from the
 * staff, it keeps to the staff until ledger lines are in play, and to C major until key
 * signatures are; heard, it starts on a lit key. */
static void build_melody(uint32_t random)
{
    int level = guided_level(LV_NOTES), base = base_key(), staff = library_prompt() == PROMPT_STAFF;
    int length = level ? level + 2 : 5, leaps = level != 1 && level != 2;
    int lo = white_position(base, 1), hi = white_position(base + NKEYS - 1, -1);
    if (staff)
        note_range(&lo, &hi);
    library.fifths = staff && !content_signatures() ? 0 : mask_choice(major_keys_in_play(), random) - 7;
    int position = key_degree(library.fifths, 0, 0, 0).position;
    while (position < lo + 3)
        position += 7;
    while (position > hi - 3)
        position -= 7;
    for (int i = 0; i < length; ++i) {
        for (int tries = 0; i && tries < 40; ++tries) {
            int next = position + melody_move(leaps);
            if (next >= lo && next <= hi && panel_bit(key_note(next).midi)) {
                position = next;
                break;
            }
        }
        library.target[i] = key_note(position);
    }
    library.count = length;
    if (!staff)
        library.given = library.target[0].midi;
}

static void library_build(int item, uint32_t random)
{
    int kind = library_kind();
    library.item_id = item;
    library.stat = item_stat(item);
    library.count = 0;
    library.steps = 1;
    library.fifths = library.minor = library.inversion = library.type = library.flat = 0;
    library.given = -1;
    for (int i = 0; i < TARGET_MAX; ++i)
        library.column[i] = 0;
    if (kind == ITEM_NOTE) {
        build_note(item, random);
    } else if (kind == ITEM_SIGNATURE) {
        library.fifths = key_fifths(item);
        library.minor = key_minor(item);
    } else if (kind == ITEM_MELODY) {
        build_melody(random);
    } else if (kind == ITEM_DEGREE || kind == ITEM_ROMAN || kind == ITEM_PROGRESSION) {
        build_in_key(item, random);
    } else {
        build_shape(item, random);
    }
}

static int sequence_note(int index)
{
    return library.target[heard_descending() ? library.count - 1 - index : index].midi;
}

static uint32_t step_pitches(int step)
{
    uint32_t pitches = 0;
    for (int i = 0; i < library.count; ++i)
        if (library.steps == 1 || library.column[i] == step)
            pitches |= pitch_bit(library.target[i].midi);
    return pitches;
}

static StaffNote library_blank_note(void)
{
    return written_note(staff_bottom() + 4, 0);
}

static int demo_run(const StaffNote *notes, int count, int step)
{
    int together = heard_together(), descending = heard_descending();
    for (int i = 0; i < count; ++i)
        demo_add(notes[descending ? count - 1 - i : i].midi, together ? step : step + i);
    return step + (together ? 1 : count);
}

/* The answer just given, built on the question's own root, to set beside the real one. */
static int demo_choice(void)
{
    int kind = library_kind(), base = base_key();
    StaffNote notes[8];
    if (kind == ITEM_NOTE) {
        demo_add(library.given, 0);
        demo_add(library.given + (natural_pitch[library.choice] + library.choice_accidental + 12) % 12, 1);
        return 2;
    }
    const Shape *shape = library_shape(kind, library.choice);
    StaffNote root = shape_root(base + positive_mod(library.root - base, 12), shape);
    for (int i = 0; i < shape->count; ++i)
        notes[i] = shape_note(root, shape, i);
    if (kind == ITEM_CHORD && library.inversion < shape->count) {
        invert_notes(notes, shape->count, library.inversion);
        fit_notes(notes, shape->count);
    }
    return demo_run(notes, shape->count, 0);
}

/* Play the question; with compare, the wrong answer first and the question after a breath. */
static void library_play(uint32_t now, int compare)
{
    int kind = library_kind(), step = 0;
    demo_clear();
    if (kind == ITEM_SIGNATURE)
        return;
    if (compare)
        step = demo_choice() + 1;
    if (kind == ITEM_PROGRESSION) {
        for (int i = 0; i < library.count; ++i)
            demo_add(library.target[i].midi, step + library.column[i]);
    } else {
        if (kind == ITEM_NOTE && library_prompt() == PROMPT_SOUND)
            demo_add(library.given, step++);
        demo_run(library.target, library.count, step);
    }
    demo_start(now, kind == ITEM_PROGRESSION ? DEMO_CHORDS :
               (heard_together() ? DEMO_BLOCK : (kind == ITEM_MELODY ? DEMO_MELODY : DEMO_NOTES)));
}

static void library_finish(void)
{
    int kind = kind_level(library_kind());
    uint32_t limit = library_time_limit();
    library.over = 1;
    library.level_offer = prefs.guided && !library.review && library.seen >= MIN_ROUND &&
                          10 * library.first_try >= 9 * library.seen && progress.level[kind] < level_top[kind];
    if (limit) {
        int rate = (library.first_try + prefs.timer / 2) / prefs.timer, activity = library_slot();
        if (rate > 255)
            rate = 255;
        library.new_best = rate > progress.best[activity];
        if (library.new_best) {
            progress.best[activity] = (uint8_t)rate;
            progress_dirty = 1;
        }
    }
    demo_clear();
    study.dirty = 1;
}

static void library_reset_answer(void)
{
    int kind = library_kind();
    library.result = READY;
    library.misses = library.hinted = library.show_answer = 0;
    library.clue = library.copy = library.teaching = library.reasked = 0;
    library.wait_release = 1;
    library.remark = 0;
    library.choice = library.choice_root = library.choice_accidental = 0;
    library.tried = 0;
    library.cursor = library.filled = 0;
    library.wrong_slots = 0;
    library.position = library.step = 0;
    library.wrong_note = -1;
    library.collected = library.checked = 0;
    library.checked_keys = (NoteSet){{0}};
    library.entry_changed = 0;
    library.advance_at = library.wrong_at = library.autoplay_at = 0;
    if (kind == ITEM_INTERVAL || kind == ITEM_SCALE || kind == ITEM_CHORD) {
        library.choice = mask_choice(library_filter(kind), 0);
        library.choice_root = mask_choice(content_roots(kind_level(kind)), 0);
    }
}

/* Put one item in front of the player. */
static void library_pose(int item, uint32_t random, uint32_t now)
{
    library_reset_answer();
    demo_clear();
    library.bag[item >> 5] |= 1u << (item & 31);
    library.last_item = item;
    library_build(item, random);
    if (library_answer() == ANSWER_WRITE) {
        for (int i = 0; i < 8; ++i)
            library.written[i] = library_blank_note();
        if (library.count > 1) {
            library.written[0] = library.target[0];
            library.filled = 1;
            library.cursor = 1;
        }
    }
    library.shown_at = now;
    if (library_prompt() == PROMPT_SOUND)
        library.autoplay_at = now + AUTOPLAY_MS;
    study.dirty = 1;
}

/* Ask an item once more a few questions on, while it is still fresh. */
static void library_ask_again(int item)
{
    if (library.review || library.again_count >= AGAIN_MAX)
        return;
    library.again[library.again_count] = (uint16_t)item;
    library.again_due[library.again_count++] = library.asked + AGAIN_GAP;
}

static int teach_off;                     /* test scripts: "teach 0" to test the rest without lessons */

/* An interval, chord or scale never answered on any root (heard as a whole, it has the one). */
static int library_new_shape(void)
{
    int kind = library_kind();
    if (teach_off || (kind != ITEM_INTERVAL && kind != ITEM_SCALE && kind != ITEM_CHORD && kind != ITEM_INVERSION))
        return 0;
    for (int root = 0; root < 12; ++root) {
        int stat = item_stat(12 * library.quality + root);
        if (stat < 0 || progress.box[stat] != BOX_NEW)
            return 0;
    }
    return 1;
}

/* The first meeting with a shape shows it before it is asked: name, rule, staff, keys and sound.
 * Played once (or passed with EDIT), it counts as seen with help and comes back as a question a
 * few turns later. It is not one of the round's questions. */
static void library_teach(void)
{
    library.teaching = 1;
    library.copy = COPY_WAIT;
    library.show_answer = 1;
    if (!library.autoplay_at)
        library.autoplay_at = library.shown_at + AUTOPLAY_MS;
    progress_record(library.stat, SOLVED_HELPED);
    library_ask_again(library.item_id);
}

static void library_question(uint32_t now)
{
    if (library.seen >= library_round_length() && !library_time_limit()) {
        library_finish();
        return;
    }
    int item, again = 0;
    if (library.review) {
        item = library.queue[library.queue_at++];
    } else if (library.again_count && library.again_due[0] <= library.asked) {
        /* An item missed or skipped comes back a few questions later, while it is still fresh. */
        item = library.again[0];
        again = 1;
        --library.again_count;
        for (int i = 0; i < library.again_count; ++i) {
            library.again[i] = library.again[i + 1];
            library.again_due[i] = library.again_due[i + 1];
        }
    } else {
        item = library_pick(next_random());
    }
    ++library.asked;
    library_pose(item, next_random(), now);
    library.reasked = again;
    if (!library.review && !library_time_limit() && library_new_shape())
        library_teach();
}

static void library_round(uint32_t now, int review)
{
    library.review = review && library.missed_count;
    library.queue_count = library.queue_at = 0;
    if (library.review) {
        for (int i = 0; i < library.missed_count; ++i)
            library.queue[i] = library.missed[i];
        library.queue_count = library.missed_count;
    }
    library.over = library.seen = library.solved = library.first_try = library.missed_count = 0;
    library.clock_ms = library.time_total = 0;
    library.clock_at = now;
    library.timed_answers = library.level_offer = library.new_best = 0;
    library_question(now);
}

static void library_start(uint32_t now)
{
    int slot = library_slot() + 1;
    if (prefs.spare != slot) {
        prefs.spare = (uint8_t)slot;
        resume_dirty = 1;
    }
    library.page = LIB_TASK;
    library.collect = 0;
    library.last_item = -1;
    for (int i = 0; i < BAG_WORDS; ++i)
        library.bag[i] = 0;
    library.missed_count = library.again_count = library.asked = 0;
    library_round(now, 0);
}

/* The keys moved an octave, and what can be asked moved with them: a written note's number
 * counts from the lowest one in reach. Lists made before the move would name other notes, so
 * the activity starts over. */
static void library_register(uint32_t now)
{
    int page = library.page;
    if (page != LIB_CLOSED) {
        library_start(now);
        library.page = page;
    }
    study.dirty = 1;
}

/* The round's list of what to go over: each item once, however often it was missed. */
static void note_missed(int item)
{
    for (int i = 0; i < library.missed_count; ++i)
        if (library.missed[i] == item)
            return;
    if (library.missed_count < MAX_ROUND)
        library.missed[library.missed_count++] = (uint16_t)item;
}

static void library_conclude(int result, uint32_t now)
{
    if (library.copy) {                  /* the shown answer, played: nothing more to count */
        library.copy = COPY_DONE;
        library.result = library.teaching ? SKIPPED : MISSED;
        if (prefs.advance)
            library.advance_at = now + ADVANCE_MS;
        study.dirty = 1;
        return;
    }
    int first = result == CORRECT && !library.misses && !library.hinted, outcome = SOLVED_NOT;
    uint32_t took = now - library.shown_at;
    if ((int32_t)took < 0)
        took = 0;
    if (took > 60000u)
        took = 60000u;
    library.result = result;
    ++library.seen;
    if (result == CORRECT) {
        ++library.solved;
        library.time_total += took;
        ++library.timed_answers;
        outcome = first ? (took <= fluent_ms() ? SOLVED_FLUENT : SOLVED_SLOW) : SOLVED_HELPED;
        if (prefs.advance)
            library.advance_at = now + ADVANCE_MS;
    }
    library.first_try += first;
    if (!first) {                        /* missed, skipped or helped: it comes round again soon, once */
        note_missed(library.item_id);
        if (!library.reasked)
            library_ask_again(library.item_id);
    }
    progress_record(library.stat, outcome);
    library.autoplay_at = 0;
    study.dirty = 1;
}

/* A revealed answer to be played stays up and the keys keep listening: playing it once puts it
 * in the hands as well as the eyes. */
static void library_begin_copy(void)
{
    library.copy = COPY_WAIT;
    library.result = READY;
    library.show_answer = 1;
    library.position = library.step = 0;
    library.wrong_note = -1;
    library.collected = library.checked = 0;
    library.checked_keys = (NoteSet){{0}};
    library.wait_release = 1;
}

static void library_wrong(uint32_t now, const char *remark)
{
    int limit = reveal_after();
    library.wrong_at = now;
    library.result = WRONG;
    study.dirty = 1;
    if (library.copy) {                  /* a slip while copying the answer counts for nothing */
        library.remark = 0;
        return;
    }
    ++library.misses;
    library.remark = remark;
    if (limit && library.misses >= limit) {
        library_conclude(MISSED, now);
        library_play(now, 0);
        if (library_answer() == ANSWER_PLAY)
            library_begin_copy();
    }
}

/* How far a written note may go: the panel's reach and at least three ledger lines either way. */
static void write_range(int *low, int *high)
{
    int lo = white_position(base_key(), 1) - 1, hi = white_position(base_key() + NKEYS - 1, -1) + 2;
    *low = lo < staff_bottom() - 7 ? lo : staff_bottom() - 7;
    *high = hi > staff_bottom() + 15 ? hi : staff_bottom() + 15;
}

static void library_set_written(StaffNote note)
{
    library.written[library.cursor] = note;
    library.filled |= 1 << library.cursor;
    library.wrong_slots &= ~(1u << library.cursor);
    if (library.result == WRONG)
        library.result = READY;
    study.dirty = 1;
}

static void library_advance_cursor(int delta)
{
    int first = library.count > 1;
    library.cursor = first + wrap_index(library.cursor - first + delta, library.count - first);
    study.dirty = 1;
}

static void library_submit(uint32_t now)
{
    int kind = library_kind(), prompt = library_prompt(), correct;
    const char *remark = 0;
    if (kind == ITEM_SIGNATURE) {
        correct = library.choice == library.fifths;
    } else if (library_answer() == ANSWER_WRITE) {
        /* Every wrong note on the right key is a spelling slip; on the right line, a sign slip. */
        int keys = 1, lines = 1;
        library.wrong_slots = 0;
        for (int i = 0; i < library.count; ++i) {
            StaffNote want = library.target[i], got = library.written[i];
            int filled = (library.filled >> i) & 1;
            if (!filled || want.position != got.position || want.accidental != got.accidental) {
                library.wrong_slots |= 1u << i;
                keys &= filled && got.midi == want.midi;
                lines &= filled && got.position == want.position;
            }
        }
        correct = !library.wrong_slots;
        if (!correct && keys)
            remark = "Right keys: spell with other letters";
        else if (!correct && lines)
            remark = "Right letters: check the signs";
    } else if (kind == ITEM_NOTE) {
        StaffNote target = library.target[0];
        int letter = positive_mod(target.position, 7) == library.choice;
        int pitch = (natural_pitch[library.choice] + library.choice_accidental + 12) % 12;
        if (prompt == PROMPT_STAFF) {
            correct = letter && target.accidental == library.choice_accidental;
            if (letter && !correct)
                remark = library.fifths ? "Right letter: mind the signature" : "Right letter: check the sign";
            else if (!letter && pitch == target.midi % 12)
                remark = "Same key, other letter: read the staff";
        } else {
            correct = pitch == target.midi % 12;
        }
    } else {
        correct = library.choice == library.quality;
        if (kind == ITEM_CHORD && prompt != PROMPT_SOUND) {
            int root = library.choice_root == library.root;
            if (correct != root)
                remark = correct ? "Right type: check the root" : "Right root: check the type";
            correct = correct && root;
        } else if (!correct) {
            library.tried |= 1u << library.choice;
            /* Seen intervals have a size in letters and one in half steps: say which is off. */
            if (kind == ITEM_INTERVAL && prompt != PROMPT_SOUND) {
                const Shape *got = &intervals[library.choice], *want = &intervals[library.quality];
                if (got->semitones[1] == want->semitones[1])
                    remark = "Same size: count the letters";
                else if (got->degrees[1] == want->degrees[1])
                    remark = "Right number: count the half steps";
            }
        }
    }
    if (correct) {
        library_conclude(CORRECT, now);
    } else {
        library_wrong(now, remark);
        if (library.result == WRONG && prompt == PROMPT_SOUND)
            library_play(now, 1);
    }
}

static void library_check_chord(uint32_t now, int submit)
{
    if (play_mode() == PLAY_EXACT_SET) {
        NoteSet want = {{0}};
        for (int i = 0; i < library.count; ++i)
            key_add(&want, library.target[i].midi);
        if (same_notes(&want, &study.held)) {
            library_conclude(CORRECT, now);
        } else if ((submit ? keys_any(&study.held) : note_total(&study.held) >= library.count) &&
                   (library.result != WRONG || !same_notes(&study.held, &library.checked_keys))) {
            library.checked_keys = study.held;
            library_wrong(now, 0);
        }
        return;
    }

    uint32_t want = step_pitches(library.step);
    uint32_t entry = library.collect ? library.collected : study.held_pitches;
    int fresh = library.result != WRONG || entry != library.checked;
    if (entry == want) {
        if (library_kind() == ITEM_INVERSION && lowest_note(&study.held) % 12 != library.target[0].midi % 12) {
            if (fresh) {
                library.checked = entry;
                library_wrong(now, "Right notes: change the lowest");
            }
        } else if (++library.step < library.steps) {
            library.wait_release = 1;
            library.collected = library.checked = 0;
            library.result = READY;
            study.dirty = 1;
        } else {
            library_conclude(CORRECT, now);
        }
    } else if (entry && (submit || pitch_total(entry) >= pitch_total(want)) && fresh) {
        library.checked = entry;
        library_wrong(now, 0);
    }
}

static void library_key(int midi, uint32_t now)
{
    if (!library_active() || !library_quiz_open() || library.wait_release)
        return;
    int answer = library_answer(), mode = play_mode();
    if (library.copy && answer != ANSWER_PLAY)
        return;
    if (answer == ANSWER_WRITE) {
        StaffNote note = canonical_note(midi);
        int lo, hi;
        write_range(&lo, &hi);
        if (library_kind() != ITEM_SIGNATURE && note.position >= lo && note.position <= hi) {
            library_set_written(note);
            if (library.cursor < library.count - 1)
                library_advance_cursor(1);
        }
        return;
    }
    if (answer != ANSWER_PLAY || (library_prompt() == PROMPT_SOUND && demo_playing()))
        return;
    if (mode == PLAY_PITCH_SET || mode == PLAY_EXACT_SET) {
        if (library.collect && mode == PLAY_PITCH_SET)
            library.collected ^= pitch_bit(midi);
        if (library.result == WRONG)
            library.result = READY;
        return;
    }

    int expected = sequence_note(library.position), right;
    if (mode == PLAY_ANY_OCTAVE && !library.position) {
        int span = 0;
        for (int i = 1; i < library.count; ++i)
            if (sequence_note(i) - expected > span)
                span = sequence_note(i) - expected;
        right = midi % 12 == expected % 12;
        if (right && panel_bit(midi) && !panel_bit(midi + span)) {
            app_say("Start an octave lower", "");
            return;
        }
        library.starting_note = midi;
    } else if (mode == PLAY_ANY_OCTAVE) {
        right = midi == library.starting_note + expected - sequence_note(0);
    } else {
        right = midi == expected;
    }
    if (!right) {
        library.wrong_note = midi;
        library_wrong(now, 0);
        return;
    }
    library.wrong_note = -1;
    library.result = READY;
    study.dirty = 1;
    if (++library.position == library.count)
        library_conclude(CORRECT, now);
}

static void library_tick(uint32_t now)
{
    if (!library_active()) {
        library.clock_at = now;
        return;
    }
    uint32_t limit = library_time_limit();
    if (!library.over) {
        uint32_t before = library.clock_ms;
        library.clock_ms += now - library.clock_at;
        if (limit && library.clock_ms / 1000u != before / 1000u)
            study.dirty = 1;
        if (limit && library.clock_ms >= limit)
            library_finish();
    }
    library.clock_at = now;
    if (library.over)
        return;

    if (library.wrong_at && now - library.wrong_at >= WRONG_FLASH_MS) {
        library.wrong_at = 0;
        study.dirty = 1;
    }
    if (library.autoplay_at && (int32_t)(now - library.autoplay_at) >= 0 && !keys_any(&study.held)) {
        library.autoplay_at = 0;
        library_play(now, 0);
        library.shown_at = now + demo_length();
    }
    if (library_quiz_open() && holds_keys() && library.entry_changed && !library.wait_release &&
        !demo_playing() && now - library.stable_since >= CHORD_SETTLE_MS) {
        library.entry_changed = 0;
        library_check_chord(now, 0);
    }
    if (library.advance_at && library_concluded() && (int32_t)(now - library.advance_at) >= 0 &&
        !keys_any(&study.held) && !demo_playing())
        library_question(now);
}

static void library_task_controls(uint32_t press, uint32_t now, int select, int algo, int preset)
{
    int kind = library_kind(), answer = library_answer();
    if (library.over) {
        if (press & ((1u << B_REC) | (1u << B_EDIT))) {
            library_round(now, 0);
        } else if ((press & (1u << B_SEL)) && library.missed_count) {
            library_round(now, 1);
        } else if ((press & (1u << B_SAVE)) && library.level_offer) {
            int level = kind_level(kind);
            char name[24];
            ++progress.level[level];
            progress_dirty = 1;
            save_progress(now);
            library_start(now);
            append_text(append_number(append_text(append_text(name, level_names[level]), " level "),
                                      progress.level[level]), ":");
            app_say(name, level_text[level][progress.level[level] - 1]);
        }
        return;
    }
    if ((press & (1u << B_PLAY)) && kind != ITEM_SIGNATURE) {
        if (library_prompt() != PROMPT_SOUND && !library_revealed())
            library.hinted = 1;
        library_play(now, 0);
    }
    /* REC goes on: it checks, submits and moves to the next question. EDIT clears the keys or erases a
     * written note, and where there is nothing to clear it goes on as REC does. */
    if (library.copy) {
        /* The answer is up to be played once: REC or SKIP moves on, the keys' own controls work. */
        int keys = library.copy == COPY_WAIT && answer == ANSWER_PLAY;
        if (press & ((1u << B_REC) | (1u << B_SAVE) | (keys ? 0 : 1u << B_EDIT))) {
            library_question(now);
            return;
        }
        if (!keys)
            return;
        press &= ~(1u << B_FX);
    } else if (library_concluded()) {
        if (press & ((1u << B_REC) | (1u << B_EDIT)))
            library_question(now);
        return;
    }
    if (select && answer != ANSWER_WRITE) {
        library.item = wrap_index(library.item + select, library_group()->count);
        library_start(now);
        return;
    }

    if (kind == ITEM_SIGNATURE) {
        if (answer == ANSWER_WRITE) {
            library.choice = wrap_index(library.choice + preset + 7, 15) - 7;
        } else if (preset) {
            uint32_t keys = 0;
            for (int key = 0; key < 15; ++key)
                if (content_key(key + (library.minor ? 15 : 0)))
                    keys |= 1u << key;
            library.choice = mask_step(keys, library.choice + 7, preset) - 7;
        }
    } else if (answer == ANSWER_WRITE) {
        if (select)
            library_advance_cursor(select);
        if (algo || preset || (press & (1u << B_SEL))) {
            StaffNote note = library.written[library.cursor];
            int lo, hi;
            write_range(&lo, &hi);
            library_set_written(written_note(clamp(note.position + algo, lo, hi), clamp(note.accidental + preset, -2, 2)));
            if (press & (1u << B_SEL))
                library_advance_cursor(1);
        }
        if (press & (1u << B_EDIT)) {
            library.filled &= ~(1 << library.cursor);
            library.wrong_slots &= ~(1u << library.cursor);
            library.written[library.cursor] = library_blank_note();
        }
    } else if (answer == ANSWER_CHOICE && kind == ITEM_NOTE) {
        library.choice = wrap_index(library.choice + preset, 7);
        if (content_roots(LV_NOTES) == 0xFFFu || library.fifths)
            library.choice_accidental = clamp(library.choice_accidental + algo, -1, 1);
    } else if (answer == ANSWER_CHOICE) {
        if (preset)
            library.choice = mask_step(library_filter(kind), library.choice, preset);
        if (algo && kind == ITEM_CHORD && library_prompt() != PROMPT_SOUND)
            library.choice_root = mask_step(content_roots(LV_CHORDS), library.choice_root, algo);
    }
    if ((select || algo || preset) && library.result == WRONG && answer != ANSWER_PLAY)
        library.result = READY;

    if (press & (1u << B_FX)) {          /* first the rule or a clue, then the answer itself */
        library.hinted = 1;
        if (library.clue)
            library.show_answer ^= 1;
        else
            library.clue = 1;
        if (library.result == WRONG)     /* help asked for replaces the verdict on the card */
            library.result = READY;
    }
    if ((press & (1u << B_ARP)) && may_collect()) {
        library.collect ^= 1;
        library.collected = library.checked = 0;
        library.wait_release = 1;
    }
    if ((press & (1u << B_EDIT)) && answer == ANSWER_PLAY) {
        library.collected = library.checked = 0;
        library.position = 0;
        library.wrong_note = -1;
        if (library.result == WRONG)
            library.result = READY;
    }
    int nothing_to_clear = answer == ANSWER_CHOICE || kind == ITEM_SIGNATURE;
    if (press & (1u << B_SAVE)) {
        library_conclude(SKIPPED, now);
        demo_clear();
    } else if (press & ((1u << B_REC) | (nothing_to_clear ? 1u << B_EDIT : 0))) {
        if (answer != ANSWER_PLAY)
            library_submit(now);
        else if (holds_keys() && !library.wait_release)
            library_check_chord(now, 1);
    }
}

static int library_controls(uint32_t press, uint32_t now, int select, int algo, int preset)
{
    if (!study.practice)
        return 0;
    if (press & (1u << B_GLO)) {
        library.page = library.page == LIB_TASK ? LIB_MENU : LIB_TASK;
        library.menu_group = library.group;
        library.menu_item = library.item;
        library.wait_release = 1;
        demo_clear();
        study.dirty = 1;
        return 1;
    }
    if (library.page == LIB_MENU) {
        if (select) {
            library.menu_group = wrap_index(library.menu_group + select, LIB_GROUPS);
            library.menu_item = 0;
        }
        if (preset)
            library.menu_item = wrap_index(library.menu_item + preset, library_groups[library.menu_group].count);
        if (press & ((1u << B_REC) | (1u << B_EDIT) | (1u << B_SEL))) {
            library.group = library.menu_group;
            library.item = library.menu_item;
            library_start(now);
        }
        if (select || preset)
            study.dirty = 1;
        return 1;
    }
    library_task_controls(press, now, select, algo, preset);
    if (press || select || algo || preset)
        study.dirty = 1;
    return 1;
}
