/* SPDX-License-Identifier: MPL-2.0 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../app/theory.h"
#include "../app/library_theory.h"

static void expect_tones(int root, const Shape *shape, const char *want)
{
    char got[48], *p = got;
    for (int i = 0; i < shape->count; ++i) {
        if (i)
            *p++ = ' ';
        spell_tone(p, root, shape, i);
        p += strlen(p);
    }
    if (strcmp(got, want)) {
        printf("spelled \"%s\", want \"%s\"\n", got, want);
        assert(0);
    }
}

static void expect_roman(int fifths, int minor, int degree, int alteration, int type, int inversion,
                         const char *want_label, const char *want_notes)
{
    char label[16], notes[32], *p = notes;
    StaffNote chord[4];
    RomanLabel roman = roman_label(fifths, minor, degree, alteration, type, inversion);
    roman_text(label, &roman);
    roman_notes(chord, fifths, minor, degree, alteration, type, inversion);
    for (int i = 0; i < roman_count(type); ++i) {
        if (i)
            *p++ = ' ';
        p = spell_text(p, positive_mod(chord[i].position, 7), chord[i].accidental);
    }
    if (strcmp(label, want_label) || strcmp(notes, want_notes)) {
        printf("roman \"%s\" = \"%s\", want \"%s\" = \"%s\"\n", label, notes, want_label, want_notes);
        assert(0);
    }
}

int main(void)
{
    int total = 0, doubles = 0;
    for (int topic = 0; topic < TOPIC_COUNT; ++topic) {
        for (int type = 0; type < topics[topic].count; ++type) {
            const Shape *shape = &topics[topic].shapes[type];
            for (int root = 0; root < 12; ++root) {
                const int root_midi = root < 5 ? 60 + root : 48 + root;
                const int first = diatonic_from_c4(root_midi, tone_spelling(root, shape, 0));
                int worst = 0;
                for (int i = 0; i < shape->count; ++i) {
                    const ToneSpelling spelling = tone_spelling(root, shape, i);
                    const int midi = root_midi + shape->semitones[i];
                    const int natural = midi - spelling.accidental;
                    assert(spelling.accidental >= -2 && spelling.accidental <= 2);
                    assert(natural % 12 == natural_pitch[spelling.letter]);
                    assert(diatonic_from_c4(midi, spelling) == first + shape->degrees[i]);
                    assert(shape_pitches(root, shape) & pitch_bit(midi));
                    assert(midi >= 53 && midi <= 79);
                    if (spelling.accidental < -worst || spelling.accidental > worst)
                        worst = spelling.accidental < 0 ? -spelling.accidental : spelling.accidental;
                    ++total;
                }
                /* A double accidental stays only where the other name is no simpler. */
                if (worst == 2) {
                    int other = root_is_flat(root, shape) ? sharp_letter[root] : flat_letter[root];
                    assert(spelling_weight(other, root, shape) >= 64 || shape == &scales[2] || shape == &scales[5]);
                    ++doubles;
                }
            }
        }
    }

    const int bm7_positions[] = {-1, 1, 3, 5};
    const int bm7_accidentals[] = {0, 0, 1, 0};
    for (int i = 0; i < 4; ++i) {
        const ToneSpelling spelling = tone_spelling(11, &sevenths[1], i);
        assert(spelling.accidental == bm7_accidentals[i]);
        assert(diatonic_from_c4(59 + sevenths[1].semitones[i], spelling) == bm7_positions[i]);
    }

    /* Each chord and scale names its black-key root from the side that reads more simply. */
    expect_tones(0, &sevenths[4], "C Eb Gb Bbb");
    expect_tones(1, &triads[0], "Db F Ab");
    expect_tones(1, &triads[1], "C# E G#");
    expect_tones(1, &triads[3], "Db F A");
    expect_tones(8, &triads[1], "G# B D#");
    expect_tones(8, &sevenths[4], "G# B D F");
    expect_tones(6, &triads[0], "F# A# C#");
    expect_tones(3, &triads[1], "Eb Gb Bb");
    expect_tones(1, &scales[0], "Db Eb F Gb Ab Bb C Db");
    expect_tones(6, &scales[0], "F# G# A# B C# D# E# F#");
    expect_tones(8, &scales[1], "G# A# B C# D# E F# G#");
    expect_tones(8, &scales[2], "G# A# B C# D# E F## G#");
    expect_tones(8, &scales[0], "Ab Bb C Db Eb F G Ab");
    assert(strcmp(root_label(1, &scales[0]), "Db") == 0);
    assert(strcmp(root_label(10, &triads[1]), "Bb") == 0);
    assert(strcmp(root_label(4, &scales[10]), "E") == 0);

    /* The tritone is two intervals on the staff: a fourth and a fifth. */
    expect_tones(0, &intervals[INTERVAL_A4], "C F#");
    expect_tones(0, &intervals[INTERVAL_D5], "C Gb");
    expect_tones(1, &intervals[INTERVAL_A4], "Db G");
    expect_tones(1, &intervals[INTERVAL_D5], "C# G");
    assert(intervals[INTERVAL_A4].degrees[1] == 3 && intervals[INTERVAL_D5].degrees[1] == 4);

    assert(diatonic_from_c4(60, (ToneSpelling){6, 1}) == -1);
    assert(diatonic_from_c4(59, (ToneSpelling){0, -1}) == 0);
    assert(canonical_note(61).position == 0 && canonical_note(61).accidental == 1);
    assert(shape_root(61, &triads[0]).position == 1 && shape_root(61, &triads[0]).accidental == -1);

    assert(strcmp(major_keys[0], "Cb") == 0 && strcmp(major_keys[14], "C#") == 0);
    assert(strcmp(minor_keys[0], "Ab") == 0 && strcmp(minor_keys[14], "A#") == 0);
    assert(strcmp(key_name(7), "C") == 0 && strcmp(key_name(22), "A") == 0 && key_minor(22));
    assert(key_fifths(5) == -2 && key_fifths(24) == 2);
    assert(chord_quality_count() == 12);
    assert(written_accidental(written_note(3, 0), 1) == ACCIDENTAL_NATURAL);
    assert(written_accidental(written_note(3, 1), 1) == 0);
    assert(written_accidental(written_note(6, -1), -1) == 0);
    for (int fifths = -7; fifths <= 7; ++fifths) {
        int distinct = 0;
        for (int letter = 0; letter < 7; ++letter)
            distinct += signature_alteration(fifths, letter);
        assert(distinct == fifths);
        for (int minor = 0; minor < 2; ++minor) {
            for (int degree = 0; degree < 7; ++degree) {
                for (int type = 0; type < chord_quality_count() + 2; ++type) {
                    int count = roman_count(type);
                    for (int inversion = 0; inversion < count; ++inversion) {
                        for (int alteration = -1; alteration <= 1; ++alteration) {
                            StaffNote notes[4];
                            roman_notes(notes, fifths, minor, degree, alteration, type, inversion);
                            for (int j = 0; j < count; ++j) {
                                assert(notes[j].accidental >= -4 && notes[j].accidental <= 4);
                                assert(j == 0 || notes[j].midi > notes[j - 1].midi);
                                assert(written_note(notes[j].position, notes[j].accidental).midi == notes[j].midi);
                            }
                        }
                    }
                }
            }
        }
    }

    /* Numerals carry their quality and figured bass; an altered degree is a borrowed chord. */
    expect_roman(0, 0, 0, 0, 0, 0, "I", "C E G");
    expect_roman(0, 0, 1, 0, 0, 1, "ii6", "F A D");
    expect_roman(0, 0, 4, 0, 0, 2, "V6/4", "D G B");
    expect_roman(0, 0, 6, 0, 0, 0, "viidim", "B D F");
    expect_roman(0, 0, 4, 0, 1, 1, "V6/5", "B D F G");
    expect_roman(0, 0, 6, 0, 1, 2, "viihdim4/3", "F A B D");
    expect_roman(0, 0, 3, 0, 1, 0, "IVM7", "F A C E");
    expect_roman(0, 0, 4, 0, 1, 3, "V4/2", "F G B D");
    expect_roman(0, 0, 1, -1, 0, 0, "bII", "Db F Ab");
    expect_roman(0, 0, 2, -1, 0, 0, "bIII", "Eb G Bb");
    expect_roman(0, 0, 6, -1, 0, 0, "bVII", "Bb D F");
    expect_roman(0, 0, 6, -1, 1, 0, "bVII7", "Bb D F Ab");
    expect_roman(0, 0, 3, 1, 0, 0, "#ivdim", "F# A C");
    expect_roman(0, 1, 4, 0, 0, 0, "v", "E G B");
    expect_roman(0, 1, 4, 0, 2, 0, "V", "E G# B");
    expect_roman(2, 0, 3, 0, 0, 0, "IV", "G B D");
    expect_roman(-3, 0, 1, 0, 1, 0, "ii7", "F Ab C Eb");
    for (int i = 0; i < PROGRESSION_COUNT; ++i) {
        assert(progressions[i].count >= 3 && progressions[i].count <= 4);
        for (int j = 0; j < progressions[i].count; ++j)
            assert(progressions[i].degree[j] < 7 && roman_count(progressions[i].type[j]) <= 4);
    }

    StaffNote triad[3] = {written_note(0, 0), written_note(2, 0), written_note(4, 0)};
    invert_notes(triad, 3, 1);
    assert(triad[0].midi == 64 && triad[1].midi == 67 && triad[2].midi == 72 && triad[2].position == 7);
    invert_notes(triad, 3, 1);
    assert(triad[0].midi == 67 && triad[2].midi == 76 && triad[2].position == 9);

    StaffNote major_third = shape_note(canonical_note(61), &intervals[3], 1);
    assert(major_third.midi == 65 && major_third.accidental == 1 && major_third.position == 2);
    const uint8_t row[12] = {7, 0, 11, 1, 6, 2, 4, 3, 5, 8, 10, 9};
    for (int y = 0; y < 12; ++y) {
        assert(row_matrix_pitch(row, 0, y) == row[y]);
        assert(row_matrix_pitch(row, y, y) == row[0]);
    }
    assert(row_matrix_pitch(row, 1, 2) == 6);

    printf("Validated %d written pitches; %d shapes keep a double accidental.\n", total, doubles);
    return 0;
}
