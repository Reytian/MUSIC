/* SPDX-License-Identifier: MPL-2.0 */
/* Why an answer is what it is. The rule shows with every revealed answer. The first press of FX
 * gives a clue that leaves the answer to the player: the rule of what was named, a way to read
 * what was shown, or a heard question the other way round. */

static char *append_steps(char *out, int count)
{
    return append_text(append_number(out, count), count == 1 ? " half step" : " half steps");
}

static char *append_ordinal(char *out, int n)
{
    return append_text(append_number(out, n), n == 1 ? "st" : (n == 2 ? "nd" : (n == 3 ? "rd" : "th")));
}

/* Where each key sits among the black keys: the way to find it on any keyboard. */
static const char *const key_places[12] = {
    "left of the 2 black keys", "lower of the 2 black keys", "between the 2 black keys",
    "upper of the 2 black keys", "right of the 2 black keys", "left of the 3 black keys",
    "lowest of the 3 black keys", "between black keys 1 and 2 of 3", "middle of the 3 black keys",
    "between black keys 2 and 3 of 3", "highest of the 3 black keys", "right of the 3 black keys",
};

/* The staff's lines and spaces, bottom up, as the rhymes teach them. */
static const char *const staff_letters[2][2] = {
    {"E G B D F", "F A C E"}, {"G B D F A", "A C E G"},
};

static const char *clef_letters(int spaces)
{
    return staff_letters[prefs.clef == CLEF_BASS][spaces];
}

static char *signature_list(char *out, int fifths)
{
    int count = fifths < 0 ? -fifths : fifths;
    if (!count)
        return append_text(out, "No sharps or flats");
    for (int i = 0; i < count; ++i) {
        if (i)
            *out++ = ' ';
        *out++ = "FCGDAEB"[fifths > 0 ? i : 6 - i];
        *out++ = fifths > 0 ? '#' : 'b';
    }
    *out = 0;
    return out;
}

/* A shape's formula, less the octave a scale closes on. */
static char *append_formula(char *out, const Shape *shape)
{
    char *start = out;
    out = append_text(out, shape->formula);
    if (out - start > 2 && out[-1] == '8' && out[-2] == ' ')
        *(out -= 2) = 0;
    return out;
}

/* A scale as whole and half steps: the way to tell one from the keys or the staff. */
static char *append_step_pattern(char *out, const Shape *shape)
{
    out = append_text(out, "Steps:");
    for (int i = 1; i < shape->count; ++i) {
        int size = shape->semitones[i] - shape->semitones[i - 1];
        out = append_text(out, size == 1 ? " H" : (size == 2 ? " W" : " W+H"));
    }
    return out;
}

/* The chord a set of notes spells from its lowest, root position: 0 for none of the twelve. */
static const Shape *matching_chord(const StaffNote *notes, int size)
{
    for (int quality = 0; quality < chord_quality_count(); ++quality) {
        const Shape *shape = chord_shape(quality);
        int same = shape->count == size;
        for (int i = 1; same && i < size; ++i)
            same = notes[i].midi - notes[0].midi == shape->semitones[i];
        if (same)
            return shape;
    }
    return 0;
}

/* A written note's place: its line or space with the rhyme that names it, else its ledger lines. */
static char *note_place(char *out, StaffNote note)
{
    int step = note.position - staff_bottom(), letter = positive_mod(note.position, 7);
    if (step >= 0 && step <= 8) {
        out = append_text(append_text(out, step & 1 ? "Spaces " : "Lines "), clef_letters(step & 1));
        *out++ = ':';
        *out++ = ' ';
        *out++ = "CDEFGAB"[letter];
        return append_number(append_text(out, step & 1 ? " is space " : " is line "), step / 2 + 1);
    }
    *out++ = "CDEFGAB"[letter];
    out = append_text(out, ": ");
    if (step == -1 || step == 9)
        return append_text(out, step < 0 ? "just below the staff" : "just above the staff");
    int lines = step < 0 ? -step / 2 : (step - 8) / 2;
    if (!(step & 1))
        return append_text(append_number(append_text(out, "ledger line "), lines), step < 0 ? " below" : " above");
    return append_number(append_text(out, step < 0 ? "below ledger line " : "above ledger line "), lines);
}

static void explain_note(char *out, int clue)
{
    StaffNote note = library.target[0];
    int prompt = library_prompt();
    char *p = out;
    if (prompt == PROMPT_SOUND) {                 /* heard after C: how far above it */
        p = append_text(note_text(out, note, 0), ": ");
        append_text(append_steps(p, note.midi - library.given), " above C");
    } else if (written_notes() && !clue) {
        p = note_place(out, note);
        if (note.accidental && !written_accidental(note, library.fifths))
            append_text(spell_text(append_text(p, " (key: "), positive_mod(note.position, 7), note.accidental), ")");
    } else if (written_notes() && library.fifths) {
        signature_list(append_text(out, "Key signature: "), library.fifths);
    } else if (written_notes()) {
        p = append_text(append_text(append_text(out, "Lines "), clef_letters(0)), ", spaces ");
        append_text(p, clef_letters(1));
    } else if (clue && prompt == PROMPT_KEYS) {
        append_text(out, "Find C, left of the 2 black keys");
    } else {
        p = note_text(out, note, 0);
        if (note.accidental)                      /* a black key goes by two names */
            p = append_text(append_text(p, " = "), (note.accidental > 0 ? flat_name : sharp_name)[note.midi % 12]);
        append_text(append_text(p, ": "), key_places[note.midi % 12]);
    }
}

static void explain_signature(char *out, int clue)
{
    int fifths = library.fifths, write = library_answer() == ANSWER_WRITE;
    const char *major = key_name(fifths + 7);
    if (!clue && library.minor)
        signature_list(append_text(append_text(append_text(out, "Like "), major), " major: "), fifths);
    else if (!clue)
        signature_list(out, fifths);
    else if (library.minor && write)
        append_text(append_text(append_text(out, "Same signs as "), major), " major");
    else if (library.minor)
        append_text(out, "Minor: 3 half steps below its major");
    else if (write)
        append_text(out, fifths > 0 ? "Sharps go F C G D A E B" : (fifths ? "Flats go B E A D G C F" : "C major has none"));
    else
        append_text(out, fifths > 0 ? "Last sharp + a half step = the key" :
                         (fifths < -1 ? "Next-to-last flat names the key" : (fifths ? "One flat: F major" : "No signs: C major")));
}

static void explain_interval(char *out, int clue)
{
    const Shape *shape = &intervals[library.quality];
    int prompt = library_prompt(), number = shape->degrees[1] + 1;
    if (clue && prompt == PROMPT_KEYS) {
        append_text(append_steps(out, shape->semitones[1]), " apart");
    } else if (clue && prompt == PROMPT_STAFF) {
        char *p = number == 8 ? append_text(out, "An octave") : append_ordinal(append_text(out, "A "), number);
        append_text(p, ": count the half steps");
    } else if (one_tritone() && shape == &intervals[INTERVAL_A4]) {
        append_steps(append_text(out, "Tritone: "), 6);
    } else {
        char *p = append_steps(append_text(append_text(out, shape->name), ": "), shape->semitones[1]);
        append_text(append_number(append_text(p, ", "), number), " letters");
    }
}

static void explain_chord(char *out, int clue)
{
    const Shape *shape = chord_shape(library.quality);
    int prompt = library_prompt(), inversion = library.inversion;
    char *p;
    if (clue && prompt == PROMPT_KEYS) {
        p = append_text(out, "Half steps from the root:");
        for (int i = 1; i < shape->count; ++i)
            p = append_number(append_text(p, " "), shape->semitones[i]);
    } else if (clue && prompt == PROMPT_STAFF) {
        append_text(append_text(append_text(out, "Root is "), root_label(library.root, shape)), ": now the type");
    } else if (library_kind() == ITEM_INVERSION) {
        p = append_text(append_ordinal(out, inversion), " inversion: the ");
        append_text(append_ordinal(p, shape->degrees[inversion] + 1), " lowest");
    } else {
        p = append_formula(append_text(append_text(out, shape->name), ": "), shape);
        if (inversion)
            append_text(append_ordinal(append_text(p, ", "), inversion), " inversion");
    }
}

static void explain_in_key(char *out, int clue)
{
    int kind = library_kind();
    char *p = out;
    if (kind == ITEM_DEGREE && clue) {
        append_text(note_text(out, key_degree(library.fifths, library.minor, 0, 0), 0), " is 1: count up the scale");
    } else if (kind == ITEM_DEGREE) {
        for (int degree = 0; degree <= library.degree; ++degree)
            p = note_text(degree ? append_text(p, " ") : p, key_degree(library.fifths, library.minor, degree, 0), 0);
        append_number(append_text(p, ": degree "), library.degree + 1);
    } else if (kind == ITEM_ROMAN) {
        StaffNote notes[4];
        int count = roman_count(library.type);
        roman_notes(notes, library.fifths, library.minor, library.degree, 0, library.type, 0);
        const Shape *shape = matching_chord(notes, count);
        p = append_text(out, shape ? shape->name : "Chord");
        if (count == 3)
            p = append_text(p, " triad");
        append_number(append_text(p, " on degree "), library.degree + 1);
    } else {
        const Progression *progression = &progressions[library.quality];
        p = append_text(out, "Roots on degrees");
        for (int step = 0; step < progression->count; ++step)
            p = append_number(append_text(p, " "), progression->degree[step] + 1);
    }
}

/* The rule (clue 0) or the first clue (clue 1) for the question in front of the player. */
static void library_explain(char *out, int clue)
{
    int kind = library_kind(), prompt = library_prompt(), play = library_answer() == ANSWER_PLAY;
    *out = 0;
    if (clue && prompt == PROMPT_SOUND) {
        append_text(out, play ? "Now find it on the keys" :
                         (kind == ITEM_NOTE ? "Count up from C on the lit keys" : "Count the half steps on the lit keys"));
    } else if (clue && prompt == PROMPT_STAFF && play && kind != ITEM_NOTE) {
        char *p;
        if (kind == ITEM_MELODY)
            p = append_text(key_text(out, library.fifths, 0), ": starts on ");
        else
            p = append_text(out, kind == ITEM_CHORD ? "Lowest note " : "Starts on ");
        note_text(p, library.target[0], 1);
    } else if (kind == ITEM_NOTE) {
        explain_note(out, clue);
    } else if (kind == ITEM_SIGNATURE) {
        explain_signature(out, clue);
    } else if (kind == ITEM_INTERVAL) {
        explain_interval(out, clue);
    } else if (kind == ITEM_SCALE) {
        const Shape *shape = &scales[library.quality];
        if (clue && prompt != PROMPT_NAME)
            append_step_pattern(out, shape);
        else
            append_formula(append_text(append_text(out, shape->name), ": "), shape);
    } else if (kind == ITEM_CHORD || kind == ITEM_INVERSION) {
        explain_chord(out, clue);
    } else if (kind == ITEM_MELODY) {
        append_text(key_text(out, library.fifths, 0), ": starts on the tonic");
    } else {
        explain_in_key(out, clue);
    }
}
