/* SPDX-License-Identifier: MPL-2.0 */
#pragma once

typedef struct {
    int midi;
    int position;                 /* Diatonic steps from written middle C. */
    int accidental;
} StaffNote;

static const char *const major_keys[] = {
    "Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#"
};
static const char *const minor_keys[] = {
    "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#", "G#", "D#", "A#"
};

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

static StaffNote canonical_note(int midi)
{
    ToneSpelling spelling = tone_spelling(midi % 12, single_note, 0);
    return (StaffNote){midi, diatonic_from_c4(midi, spelling), spelling.accidental};
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

static StaffNote key_degree(int fifths, int minor, int degree, int alteration)
{
    int letter = positive_mod(4 * fifths + (minor ? 5 : 0) + degree, 7);
    int accidental = signature_alteration(fifths, letter) + alteration;
    StaffNote note = written_note(letter, accidental);
    if (note.midi > 64)
        note = written_note(letter - 7, accidental);
    return note;
}

static void roman_notes(StaffNote out[4], int fifths, int minor, int degree, int alteration,
                        int type, int inversion, int count)
{
    StaffNote root = key_degree(fifths, minor, degree, alteration);
    const Shape *shape = type >= 2 ? chord_shape(type - 2) : 0;
    for (int i = 0; i < count; ++i) {
        int member = (inversion + i) % count;
        if (shape) {
            out[i] = shape_note(root, shape, member);
        } else {
            int position = root.position + 2 * member;
            int accidental = signature_alteration(fifths, positive_mod(position, 7)) + alteration;
            out[i] = written_note(position, accidental);
        }
        if (inversion + i >= count) {
            out[i].midi += 12;
            out[i].position += 7;
        }
    }
}

static int row_matrix_pitch(const uint8_t row[12], int y, int x)
{
    return positive_mod(row[x] + row[0] - row[y], 12);
}
