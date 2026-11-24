/* SPDX-License-Identifier: MPL-2.0 */
typedef struct {
    const char *title;
    const char *lines[7];
    const char *example;
    uint8_t notes[8];
    uint8_t count, together;
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
    }, "C4 through G4", {60, 62, 64, 65, 67}, 5, 0},
    {"Find an Interval", {
        "Count both letters first.",
        "C D E is a third.",
        "Then count half steps.",
        "C to E: 4, a major third.",
        "C to Eb: 3, a minor third.",
        "Perfect sizes: 1, 4, 5, 8.",
        "A tritone spans 6 half steps.",
    }, "C to E, major third", {60, 64}, 2, 0},
    {"Build Triads", {
        "Stack root, third and fifth.",
        "Half steps from the root:",
        "Major 0-4-7    Minor 0-3-7",
        "Dim 0-3-6     Aug 0-4-8",
        "Sus2 replaces the third with 2.",
        "Sus4 replaces the third with 4.",
        "C major: C E G.",
    }, "C major", {60, 64, 67}, 3, 1},
    {"Build Seventh Chords", {
        "Add a seventh to the triad.",
        "maj7: 0 4 7 11    7: 0 4 7 10",
        "m7:   0 3 7 10    mM7: 0 3 7 11",
        "m7b5: 0 3 6 10  dim7: 0 3 6 9",
        "Numbers are half steps from root.",
        "Bm7 = B minor triad plus A.",
        "Play B D F# A.",
    }, "Bm7", {59, 62, 66, 69}, 4, 1},
    {"Chord Inversions", {
        "The lowest pitch sets the inversion.",
        "C E G: root position.",
        "E G C: first inversion (C/E).",
        "G C E: second inversion (C/G).",
        "A seventh can also be in the bass.",
        "Keep the same note names.",
        "Move the bottom note up an octave.",
    }, "C/E, first inversion", {64, 67, 72}, 3, 1},
    {"Build Scales", {
        "W = whole step. H = half step.",
        "Major: W W H W W W H",
        "Natural minor: W H W W H W W",
        "Harmonic minor: raise degree 7.",
        "Melodic minor up: raise 6 and 7.",
        "C major and A minor share notes.",
        "Play from the tonic to its octave.",
    }, "C major scale", {60, 62, 64, 65, 67, 69, 71, 72}, 8, 0},
    {"Read Key Signatures", {
        "Sharps: F C G D A E B",
        "Flats: B E A D G C F",
        "Last sharp + half step = major key.",
        "Flat keys: use the next-to-last flat.",
        "One flat = F major.",
        "Relative minor: down 3 half steps.",
        "One sharp (F#) = G major.",
    }, "G major scale", {67, 69, 71, 72, 74, 76, 78, 79}, 8, 0},
};
#define QUICK_GUIDE_COUNT (sizeof quick_guides / sizeof quick_guides[0])

static int guide_controls(uint32_t press, uint32_t now, int select)
{
    if (study.practice || library.page != LIB_CLOSED)
        return 0;
    if (press & (1u << B_SEL)) {
        guide.open ^= 1;
        stop_demo();
        library_silence();
        study.dirty = 1;
        return 1;
    }
    if (!guide.open)
        return 0;
    if (press & (1u << B_GLO)) {
        guide.open = 0;
        library_silence();
        return 0;
    }
    if (select) {
        guide.page = wrap_index(guide.page + select, QUICK_GUIDE_COUNT);
        library_silence();
        study.dirty = 1;
    }
    if (press & (1u << B_PLAY)) {
        const QuickGuide *lesson = &quick_guides[guide.page];
        library.demo_count = lesson->count;
        library.demo_block = lesson->together;
        library.demo_at = now;
        for (int i = 0; i < lesson->count; ++i)
            library.demo_notes[i] = lesson->notes[i];
        study.dirty = 1;
    }
    return 1;
}
