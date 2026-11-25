/* SPDX-License-Identifier: MPL-2.0 */
#pragma once

enum Topic { NOTES, INTERVALS, TRIADS, SEVENTHS, SCALES, TOPIC_COUNT };
enum { INTERVAL_A4 = 5, INTERVAL_D5 = 6 };

typedef struct {
    const char *name;
    const char *symbol;
    const char *formula;
    uint8_t count;
    uint8_t semitones[8];
    uint8_t degrees[8];
} Shape;

typedef struct {
    const char *label;
    const Shape *shapes;
    uint8_t count;
} TopicInfo;

/* A bare note takes the commoner name; a chord or scale picks its own side (root_is_flat). */
static const char *const root_name[12] = {
    "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"
};
static const char *const sharp_name[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
static const char *const flat_name[12] = {
    "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
};
static const uint8_t sharp_letter[12] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};
static const uint8_t flat_letter[12] = {0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6};
static const uint8_t natural_pitch[] = {0, 2, 4, 5, 7, 9, 11};
enum { FLAT_BY_DEFAULT = (1 << 3) | (1 << 8) | (1 << 10) };

static const Shape single_note[] = {
    {"Note", "", "1", 1, {0}, {0}},
};
static const Shape intervals[] = {
    {"Minor 2nd",      "m2", "1 b2", 2, {0, 1},  {0, 1}},
    {"Major 2nd",      "M2", "1 2",  2, {0, 2},  {0, 1}},
    {"Minor 3rd",      "m3", "1 b3", 2, {0, 3},  {0, 2}},
    {"Major 3rd",      "M3", "1 3",  2, {0, 4},  {0, 2}},
    {"Perfect 4th",    "P4", "1 4",  2, {0, 5},  {0, 3}},
    {"Augmented 4th",  "A4", "1 #4", 2, {0, 6},  {0, 3}},
    {"Diminished 5th", "d5", "1 b5", 2, {0, 6},  {0, 4}},
    {"Perfect 5th",    "P5", "1 5",  2, {0, 7},  {0, 4}},
    {"Minor 6th",      "m6", "1 b6", 2, {0, 8},  {0, 5}},
    {"Major 6th",      "M6", "1 6",  2, {0, 9},  {0, 5}},
    {"Minor 7th",      "m7", "1 b7", 2, {0, 10}, {0, 6}},
    {"Major 7th",      "M7", "1 7",  2, {0, 11}, {0, 6}},
    {"Octave",         "P8", "1 8",  2, {0, 12}, {0, 7}},
};
static const Shape triads[] = {
    {"Major",      "",     "1 3 5",   3, {0, 4, 7}, {0, 2, 4}},
    {"Minor",      "m",    "1 b3 5",  3, {0, 3, 7}, {0, 2, 4}},
    {"Diminished", "dim",  "1 b3 b5", 3, {0, 3, 6}, {0, 2, 4}},
    {"Augmented",  "aug",  "1 3 #5",  3, {0, 4, 8}, {0, 2, 4}},
    {"Suspended 2","sus2", "1 2 5",   3, {0, 2, 7}, {0, 1, 4}},
    {"Suspended 4","sus4", "1 4 5",   3, {0, 5, 7}, {0, 3, 4}},
};
static const Shape sevenths[] = {
    {"Major seventh",   "maj7", "1 3 5 7",    4, {0, 4, 7, 11}, {0, 2, 4, 6}},
    {"Minor seventh",   "m7",   "1 b3 5 b7",  4, {0, 3, 7, 10}, {0, 2, 4, 6}},
    {"Dominant seventh","7",    "1 3 5 b7",   4, {0, 4, 7, 10}, {0, 2, 4, 6}},
    {"Half diminished", "m7b5", "1 b3 b5 b7", 4, {0, 3, 6, 10}, {0, 2, 4, 6}},
    {"Diminished 7th",  "dim7", "1 b3 b5 bb7",4, {0, 3, 6, 9},  {0, 2, 4, 6}},
    {"Minor-major 7th", "mM7",  "1 b3 5 7",   4, {0, 3, 7, 11}, {0, 2, 4, 6}},
};
static const Shape scales[] = {
    {"Major",            " major",    "1 2 3 4 5 6 7 8",    8, {0, 2, 4, 5, 7, 9, 11, 12}, {0, 1, 2, 3, 4, 5, 6, 7}},
    {"Natural minor",    " nat min",  "1 2 b3 4 5 b6 b7 8", 8, {0, 2, 3, 5, 7, 8, 10, 12}, {0, 1, 2, 3, 4, 5, 6, 7}},
    {"Harmonic minor",   " harm min", "1 2 b3 4 5 b6 7 8",  8, {0, 2, 3, 5, 7, 8, 11, 12}, {0, 1, 2, 3, 4, 5, 6, 7}},
    {"Major pentatonic", " maj pent", "1 2 3 5 6 8",        6, {0, 2, 4, 7, 9, 12},       {0, 1, 2, 4, 5, 7}},
    {"Minor pentatonic", " min pent", "1 b3 4 5 b7 8",      6, {0, 3, 5, 7, 10, 12},      {0, 2, 3, 4, 6, 7}},
    {"Melodic minor",    " mel min",  "1 2 b3 4 5 6 7 8",  8, {0, 2, 3, 5, 7, 9, 11, 12}, {0, 1, 2, 3, 4, 5, 6, 7}},
    {"Dorian",           " dorian",   "1 2 b3 4 5 6 b7 8", 8, {0, 2, 3, 5, 7, 9, 10, 12}, {0, 1, 2, 3, 4, 5, 6, 7}},
    {"Phrygian",         " phrygian", "1 b2 b3 4 5 b6 b7 8",8,{0, 1, 3, 5, 7, 8, 10, 12}, {0, 1, 2, 3, 4, 5, 6, 7}},
    {"Lydian",           " lydian",   "1 2 3 #4 5 6 7 8",  8, {0, 2, 4, 6, 7, 9, 11, 12}, {0, 1, 2, 3, 4, 5, 6, 7}},
    {"Mixolydian",       " mixolydian","1 2 3 4 5 6 b7 8", 8, {0, 2, 4, 5, 7, 9, 10, 12}, {0, 1, 2, 3, 4, 5, 6, 7}},
    {"Locrian",          " locrian",  "1 b2 b3 4 b5 b6 b7 8",8,{0, 1, 3, 5, 6, 8, 10, 12}, {0, 1, 2, 3, 4, 5, 6, 7}},
};
#define TOPIC(label, array) {label, array, sizeof array / sizeof array[0]}
static const TopicInfo topics[TOPIC_COUNT] = {
    TOPIC("Notes", single_note), TOPIC("Intervals", intervals), TOPIC("Triads", triads),
    TOPIC("Sevenths", sevenths), TOPIC("Scales", scales),
};
#undef TOPIC

static uint32_t pitch_bit(int note)
{
    return 1u << (note % 12);
}

static uint32_t shape_pitches(int root, const Shape *shape)
{
    uint32_t pitches = 0;
    for (int i = 0; i < shape->count; ++i)
        pitches |= pitch_bit(root + shape->semitones[i]);
    return pitches;
}

typedef struct {
    int letter;
    int accidental;
} ToneSpelling;

static int letter_accidental(int letter, int pitch)
{
    return (pitch - natural_pitch[letter] + 18) % 12 - 6;
}

/* How hard a shape reads from a root letter: its worst accidental, then how many it carries. */
static int spelling_weight(int letter, int root, const Shape *shape)
{
    int worst = 0, total = 0;
    for (int i = 0; i < shape->count; ++i) {
        if (shape->semitones[i] == 12)
            continue;
        int accidental = letter_accidental((letter + shape->degrees[i]) % 7, (root + shape->semitones[i]) % 12);
        if (accidental < 0)
            accidental = -accidental;
        if (accidental > worst)
            worst = accidental;
        total += accidental;
    }
    return 32 * worst + total;
}

/* C# or Db: the side with fewer accidentals, the usual name on a tie. The harmonic and melodic
 * minors follow the natural minor, so one tonic keeps one name across the three. */
static int root_is_flat(int root, const Shape *shape)
{
    int usual_flat = (FLAT_BY_DEFAULT >> root) & 1;
    if (sharp_letter[root] == flat_letter[root])
        return 0;
    if (shape == &scales[2] || shape == &scales[5])
        shape = &scales[1];
    int sharp = spelling_weight(sharp_letter[root], root, shape);
    int flat = spelling_weight(flat_letter[root], root, shape);
    if (sharp == flat)
        return usual_flat;
    return flat < sharp;
}

static const char *root_label(int root, const Shape *shape)
{
    return (root_is_flat(root, shape) ? flat_name : sharp_name)[root];
}

static ToneSpelling tone_spelling(int root, const Shape *shape, int index)
{
    int root_letter = (root_is_flat(root, shape) ? flat_letter : sharp_letter)[root];
    int letter = (root_letter + shape->degrees[index]) % 7;
    return (ToneSpelling){letter, letter_accidental(letter, (root + shape->semitones[index]) % 12)};
}

/* A lone note has no shape to weigh, so it takes the bare name. */
static ToneSpelling bare_spelling(int pitch)
{
    int letter = ((FLAT_BY_DEFAULT >> pitch) & 1 ? flat_letter : sharp_letter)[pitch];
    return (ToneSpelling){letter, letter_accidental(letter, pitch)};
}

static int diatonic_from_c4(int midi, ToneSpelling spelling)
{
    int natural_midi = midi - spelling.accidental;
    return 7 * (natural_midi / 12 - 5) + spelling.letter;
}

/* A letter and up to three accidentals: five bytes with the terminator. */
static char *spell_text(char *out, int letter, int accidental)
{
    *out++ = "CDEFGAB"[letter];
    for (int i = 0; i < 3 && i < accidental; ++i)
        *out++ = '#';
    for (int i = 0; i < 3 && i < -accidental; ++i)
        *out++ = 'b';
    *out = 0;
    return out;
}

static void spell_tone(char out[5], int root, const Shape *shape, int index)
{
    ToneSpelling spelling = tone_spelling(root, shape, index);
    spell_text(out, spelling.letter, spelling.accidental);
}
