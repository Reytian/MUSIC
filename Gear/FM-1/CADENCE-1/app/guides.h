/* SPDX-License-Identifier: MPL-2.0 */
typedef struct {
    const char *title;
    const char *lines[7];
    const char *example;
    uint8_t notes[12];
    uint8_t count, chord;         /* chord: how many of the notes sound at once */
} QuickGuide;

static const QuickGuide quick_guides[] = {
    {"Read the Staff", {
        "Treble lines: E G B D F",
        "Every Green Bus Drives Fast",
        "Treble spaces: F A C E",
        "Bass lines: G B D F A",
        "Good Bikes Don't Fall Apart",
        "Bass spaces: A C E G",
        "All Cows Eat Grass",
    }, "C4 through G4", {60, 62, 64, 65, 67}, 5, 1},
    {"Find an Interval", {
        "Count both letters first.",
        "C D E is a third.",
        "Then count half steps.",
        "C to E: 4, a major third.",
        "C to Eb: 3, a minor third.",
        "Six half steps: C to F# is an A4,",
        "C to Gb a d5. The letters decide.",
    }, "C to E, major third", {60, 64}, 2, 1},
    {"Build Triads", {
        "Stack root, third and fifth.",
        "Half steps from the root:",
        "Major 0-4-7    Minor 0-3-7",
        "Dim 0-3-6     Aug 0-4-8",
        "Sus2 replaces the third with 2.",
        "Sus4 replaces the third with 4.",
        "C major: C E G.",
    }, "C major", {60, 64, 67}, 3, 3},
    {"Build Seventh Chords", {
        "Add a seventh to the triad.",
        "maj7: 0 4 7 11    7: 0 4 7 10",
        "m7:   0 3 7 10    mM7: 0 3 7 11",
        "m7b5: 0 3 6 10  dim7: 0 3 6 9",
        "Numbers are half steps from root.",
        "Bm7 = B minor triad plus A.",
        "Play B D F# A.",
    }, "Bm7", {59, 62, 66, 69}, 4, 4},
    {"Chord Inversions", {
        "The lowest pitch sets the inversion.",
        "C E G: root position.",
        "E G C: first inversion (C/E).",
        "G C E: second inversion (C/G).",
        "Roman numerals write I6 and I6/4.",
        "Sevenths: 7, 6/5, 4/3, 4/2.",
        "The figures count up from the bass.",
    }, "C, C/E, C/G", {60, 64, 67, 64, 67, 72, 67, 72, 76}, 9, 3},
    {"Build Scales", {
        "W = whole step. H = half step.",
        "Major: W W H W W W H",
        "Natural minor: W H W W H W W",
        "Harmonic minor: raise degree 7.",
        "Melodic minor up: raise 6 and 7.",
        "C major and A minor share notes.",
        "Play from the tonic to its octave.",
    }, "C major scale", {60, 62, 64, 65, 67, 69, 71, 72}, 8, 1},
    {"Read Key Signatures", {
        "Sharps: F C G D A E B",
        "Flats: B E A D G C F",
        "Last sharp + half step = major key.",
        "Flat keys: use the next-to-last flat.",
        "One flat = F major.",
        "Relative minor: down 3 half steps.",
        "One sharp (F#) = G major.",
    }, "G major scale", {67, 69, 71, 72, 74, 76, 78, 79}, 8, 1},
    {"Chords in a Key", {
        "Stack thirds on each scale degree.",
        "Major: I ii iii IV V vi vii dim",
        "Minor: i ii dim III iv v VI VII",
        "A raised 7th makes V major in minor.",
        "I, IV and V are the primary chords.",
        "ii V I is the classic cadence.",
        "In C major that is Dm, G, C.",
    }, "ii V I in C major", {62, 65, 69, 55, 59, 62, 60, 64, 67}, 9, 3},
};
#define QUICK_GUIDE_COUNT ((int)(sizeof quick_guides / sizeof quick_guides[0]))

static int guide_controls(uint32_t press, uint32_t now, int select)
{
    if (study.practice || calc.page != LIB_CLOSED)
        return 0;
    if (press & (1u << B_SEL)) {
        guide.open ^= 1;
        demo_clear();
        study.dirty = 1;
        return 1;
    }
    if (!guide.open)
        return 0;
    if (press & (1u << B_GLO)) {
        guide.open = 0;
        demo_clear();
        return 0;
    }
    if (select) {
        guide.page = wrap_index(guide.page + select, QUICK_GUIDE_COUNT);
        demo_clear();
        study.dirty = 1;
    }
    if (press & (1u << B_PLAY)) {
        const QuickGuide *lesson = &quick_guides[guide.page];
        demo_clear();
        for (int i = 0; i < lesson->count; ++i)
            demo_add(lesson->notes[i], i / lesson->chord);
        demo_start(now, lesson->chord > 1 ? DEMO_BLOCK : DEMO_NOTES);
    }
    return 1;
}
