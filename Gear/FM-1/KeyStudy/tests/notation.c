/* SPDX-License-Identifier: MPL-2.0 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../app/theory.h"
#include "../app/library_theory.h"

int main(void)
{
    int total = 0;
    for (int topic = 0; topic < TOPIC_COUNT; ++topic) {
        for (int type = 0; type < topics[topic].count; ++type) {
            const Shape *shape = &topics[topic].shapes[type];
            for (int root = 0; root < 12; ++root) {
                const int root_midi = root < 5 ? 60 + root : 48 + root;
                const int first = diatonic_from_c4(root_midi, tone_spelling(root, shape, 0));
                for (int i = 0; i < shape->count; ++i) {
                    const ToneSpelling spelling = tone_spelling(root, shape, i);
                    const int midi = root_midi + shape->semitones[i];
                    const int natural = midi - spelling.accidental;
                    assert(spelling.accidental >= -2 && spelling.accidental <= 2);
                    assert(natural % 12 == natural_pitch[spelling.letter]);
                    assert(diatonic_from_c4(midi, spelling) == first + shape->degrees[i]);
                    assert(shape_pitches(root, shape) & pitch_bit(midi));
                    assert(midi >= 53 && midi <= 79);
                    ++total;
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

    char name[4];
    spell_tone(name, 0, &sevenths[4], 3);
    assert(strcmp(name, "Bbb") == 0);
    spell_tone(name, 1, &triads[3], 2);
    assert(strcmp(name, "G##") == 0);
    assert(diatonic_from_c4(60, (ToneSpelling){6, 1}) == -1);
    assert(diatonic_from_c4(59, (ToneSpelling){0, -1}) == 0);

    assert(strcmp(major_keys[0], "Cb") == 0 && strcmp(major_keys[14], "C#") == 0);
    assert(strcmp(minor_keys[0], "Ab") == 0 && strcmp(minor_keys[14], "A#") == 0);
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
                    int count = type < 2 ? 3 + type : chord_shape(type - 2)->count;
                    for (int inversion = 0; inversion < count; ++inversion) {
                        StaffNote notes[4];
                        roman_notes(notes, fifths, minor, degree, 1, type, inversion, count);
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
    StaffNote major_third = shape_note(canonical_note(61), &intervals[3], 1);
    assert(major_third.midi == 65 && major_third.accidental == 1 && major_third.position == 2);
    const uint8_t row[12] = {7, 0, 11, 1, 6, 2, 4, 3, 5, 8, 10, 9};
    for (int y = 0; y < 12; ++y) {
        assert(row_matrix_pitch(row, 0, y) == row[y]);
        assert(row_matrix_pitch(row, y, y) == row[0]);
    }
    assert(row_matrix_pitch(row, 1, 2) == 6);

    printf("Validated %d written pitches, including double accidentals and octave boundaries.\n", total);
    return 0;
}
