/* SPDX-License-Identifier: MPL-2.0 */
#pragma once

typedef struct {
    int midi;
    int position;                 /* Diatonic steps from written middle C. */
    int accidental;
} StaffNote;

/* A key is its signature plus 15 for minor: 0 is C-flat major, 7 C major, 22 A minor. */
enum { KEY_COUNT = 30 };
static const char *const major_keys[] = {
    "Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#"
};
static const char *const minor_keys[] = {
    "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#", "G#", "D#", "A#"
};

static int key_fifths(int key) { return key % 15 - 7; }
static int key_minor(int key) { return key >= 15; }
static const char *key_name(int key) { return (key_minor(key) ? minor_keys : major_keys)[key % 15]; }

static char *append_text(char *out, const char *value)
{
    while (*value)
        *out++ = *value++;
    *out = 0;
    return out;
}

static int positive_mod(int value, int count)
{
    int remainder = value % count;
    return remainder < 0 ? remainder + count : remainder;
}

static StaffNote written_note(int position, int accidental)
{
    int letter = positive_mod(position, 7);
    int midi = 60 + 12 * ((position - letter) / 7) + natural_pitch[letter] + accidental;
    return (StaffNote){midi, position, accidental};
}

static StaffNote spelled_note(int midi, ToneSpelling spelling)
{
    return (StaffNote){midi, diatonic_from_c4(midi, spelling), spelling.accidental};
}

static StaffNote canonical_note(int midi)
{
    return spelled_note(midi, bare_spelling(midi % 12));
}

/* The root as this chord or scale names it: D-flat for a major triad, C-sharp for a minor one. */
static StaffNote shape_root(int midi, const Shape *shape)
{
    return spelled_note(midi, tone_spelling(midi % 12, shape, 0));
}

static StaffNote shape_note(StaffNote root, const Shape *shape, int index)
{
    StaffNote note = written_note(root.position + shape->degrees[index], 0);
    int midi = root.midi + shape->semitones[index];
    note.accidental = midi - note.midi;
    note.midi = midi;
    return note;
}

static int signature_alteration(int fifths, int letter)
{
    static const uint8_t sharps[] = {3, 0, 4, 1, 5, 2, 6}; /* F C G D A E B */
    for (int i = 0; i < fifths; ++i)
        if (sharps[i] == letter)
            return 1;
    for (int i = 0; i < -fifths; ++i)
        if (sharps[6 - i] == letter)
            return -1;
    return 0;
}

enum { ACCIDENTAL_NATURAL = 99 };

static int written_accidental(StaffNote note, int fifths)
{
    if (note.accidental == signature_alteration(fifths, positive_mod(note.position, 7)))
        return 0;
    return note.accidental ? note.accidental : ACCIDENTAL_NATURAL;
}

static const Shape *chord_shape(int quality)
{
    int triad_count = topics[TRIADS].count;
    return quality < triad_count ? &triads[quality] : &sevenths[quality - triad_count];
}

static int chord_quality_count(void)
{
    return topics[TRIADS].count + topics[SEVENTHS].count;
}

/* The lowest notes of a close-position chord move up an octave, one per inversion. */
static void invert_notes(StaffNote *notes, int count, int inversion)
{
    for (; inversion > 0; --inversion) {
        StaffNote low = notes[0];
        low.midi += 12;
        low.position += 7;
        for (int i = 1; i < count; ++i)
            notes[i - 1] = notes[i];
        notes[count - 1] = low;
    }
}

static StaffNote key_degree(int fifths, int minor, int degree, int alteration)
{
    int letter = positive_mod(4 * fifths + (minor ? 5 : 0) + degree, 7);
    int accidental = signature_alteration(fifths, letter) + alteration;
    StaffNote note = written_note(letter, accidental);
    if (note.midi > 64)
        note = written_note(letter - 7, accidental);
    return note;
}

/* A Roman numeral's chord type: 0 a triad and 1 a seventh from the key's own notes, 2 and up a
 * named chord (chord_shape) on that degree. */
static int roman_count(int type)
{
    return type < 2 ? 3 + type : chord_shape(type - 2)->count;
}

/* An altered degree is a borrowed chord, not the key's chord shifted: a lowered root carries a
 * major chord (bII, bIII, bVI, bVII), a raised one a diminished chord (#iv dim). */
static const Shape *roman_shape(int minor, int degree, int alteration, int type)
{
    if (type >= 2)
        return chord_shape(type - 2);
    if (!alteration)
        return 0;
    if (alteration > 0)
        return type ? &sevenths[4] : &triads[2];
    if (type)
        return &sevenths[!minor && degree == 6 ? 2 : 0];
    return &triads[0];
}

static void roman_notes(StaffNote out[4], int fifths, int minor, int degree, int alteration,
                        int type, int inversion)
{
    int count = roman_count(type);
    StaffNote root = key_degree(fifths, minor, degree, alteration);
    const Shape *shape = roman_shape(minor, degree, alteration, type);
    for (int i = 0; i < count; ++i) {
        if (shape) {
            out[i] = shape_note(root, shape, i);
        } else {
            int position = root.position + 2 * i;
            out[i] = written_note(position, signature_alteration(fifths, positive_mod(position, 7)));
        }
    }
    invert_notes(out, count, inversion);
}

enum { MARK_NONE, MARK_DIM, MARK_HALF_DIM, MARK_AUG };

typedef struct {
    char numeral[6];              /* "bVII", "iv" */
    char suffix[5];               /* "sus4", or "M" for a major seventh */
    uint8_t mark;
    char figure[2];               /* Figured bass, upper then lower; 0 where there is none. */
} RomanLabel;

static RomanLabel roman_label(int fifths, int minor, int degree, int alteration, int type, int inversion)
{
    static const char *const upper[] = {"I", "II", "III", "IV", "V", "VI", "VII"};
    static const char *const lower[] = {"i", "ii", "iii", "iv", "v", "vi", "vii"};
    static const char figures[2][4][2] = {
        {{0, 0}, {'6', 0}, {'6', '4'}, {0, 0}},
        {{'7', 0}, {'6', '5'}, {'4', '3'}, {'4', '2'}},
    };
    RomanLabel label = {{0}, {0}, MARK_NONE, {0, 0}};
    StaffNote base[4];
    int count = roman_count(type);
    roman_notes(base, fifths, minor, degree, alteration, type, 0);
    int third = base[1].midi - base[0].midi;
    int fifth = base[2].midi - base[0].midi;
    int seventh = count == 4 ? base[3].midi - base[0].midi : 0;

    char *p = label.numeral;
    if (alteration)
        *p++ = alteration < 0 ? 'b' : '#';
    append_text(p, (third == 3 ? lower : upper)[degree]);
    if (fifth == 6)
        label.mark = seventh == 10 ? MARK_HALF_DIM : MARK_DIM;
    else if (fifth == 8)
        label.mark = MARK_AUG;
    if (third == 2 || third == 5)
        append_text(label.suffix, third == 2 ? "sus2" : "sus4");
    else if (seventh == 11)
        append_text(label.suffix, "M");
    if (inversion < 0 || inversion >= count)
        inversion = 0;
    label.figure[0] = figures[count == 4][inversion][0];
    label.figure[1] = figures[count == 4][inversion][1];
    return label;
}

/* The numeral in plain letters, for a list or the screen reader: "V6/5", "viihdim7". */
static char *roman_text(char *out, const RomanLabel *label)
{
    static const char *const marks[] = {"", "dim", "hdim", "aug"};
    out = append_text(append_text(out, label->numeral), marks[label->mark]);
    out = append_text(out, label->suffix);
    if (label->figure[0]) {
        *out++ = label->figure[0];
        if (label->figure[1]) {
            *out++ = '/';
            *out++ = label->figure[1];
        }
    }
    *out = 0;
    return out;
}

/* Chord degrees count from zero; types are roman_count's. V in a minor key is the major chord
 * the raised seventh degree gives it. */
typedef struct {
    uint8_t minor, count;
    uint8_t degree[4];
    uint8_t type[4];
} Progression;

static const Progression progressions[] = {
    {0, 4, {0, 3, 4, 0}, {0, 0, 0, 0}},        /* I IV V I */
    {0, 3, {1, 4, 0},    {0, 0, 0}},           /* ii V I */
    {0, 4, {0, 4, 5, 3}, {0, 0, 0, 0}},        /* I V vi IV */
    {0, 4, {0, 5, 3, 4}, {0, 0, 0, 0}},        /* I vi IV V */
    {0, 4, {0, 5, 1, 4}, {0, 0, 0, 0}},        /* I vi ii V */
    {0, 3, {1, 4, 0},    {1, 1, 1}},           /* ii7 V7 IM7 */
    {1, 4, {0, 3, 4, 0}, {0, 0, 2, 0}},        /* i iv V i */
    {1, 3, {1, 4, 0},    {0, 2, 0}},           /* ii dim V i */
};
enum { PROGRESSION_COUNT = sizeof progressions / sizeof progressions[0] };

static int row_matrix_pitch(const uint8_t row[12], int y, int x)
{
    return positive_mod(row[x] + row[0] - row[y], 12);
}
