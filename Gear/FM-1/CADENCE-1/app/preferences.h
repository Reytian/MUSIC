/* SPDX-License-Identifier: MPL-2.0 */
enum { CLEF_TREBLE = 1, CLEF_BASS };
enum { HEAR_ASCENDING, HEAR_DESCENDING, HEAR_TOGETHER };
enum { TONE_GRAND_PIANO, TONE_PURE };
/* What a level governs: one ladder per kind of material. */
enum { LV_NOTES, LV_INTERVALS, LV_CHORDS, LV_SCALES, LV_KEYS, LV_KINDS };
/* The settings page's rows. The five level rows show under Levels, the seven after them under Custom. */
enum { SET_CLEF, SET_LENGTH, SET_TIMER, SET_ADVANCE, SET_REVEAL, SET_CONTENT,
       SET_LEVEL, SET_INTERVALS = SET_LEVEL + LV_KINDS, SET_CHORDS, SET_SCALES, SET_NOTES,
       SET_INVERSIONS, SET_SIGNATURES, SET_LEDGER, SET_ORDER, SET_TONE, SET_COUNT };
enum { PREFS_VERSION = 2, MIN_ROUND = 5, MAX_ROUND = 30, TIMER_CHOICES = 4, REVEAL_CHOICES = 3 };
#define INTERVAL_MASK ((1u << (sizeof intervals / sizeof intervals[0])) - 1u)
#define CHORD_MASK ((1u << ((sizeof triads / sizeof triads[0]) + (sizeof sevenths / sizeof sevenths[0]))) - 1u)
#define SCALE_MASK ((1u << (sizeof scales / sizeof scales[0])) - 1u)
#define NATURAL_MASK 0xAB5u

typedef struct {
    uint8_t version, clef, length, naturals, order, tone;
    uint16_t intervals, chords, scales;
    uint8_t guided;               /* Levels choose the material; 0 leaves it to the filters above. */
    uint8_t advance;              /* Move on by itself after a correct answer. */
    uint8_t reveal;               /* Show the answer after two misses (0), three (1), or never (2). */
    uint8_t timer;                /* A round by the clock: minutes, 0 for a round by count. */
    uint8_t inversions, signatures, ledger, spare;
} StudyPreferences;

static const StudyPreferences default_preferences = {
    .version = PREFS_VERSION, .clef = CLEF_TREBLE, .length = 10,
    .intervals = INTERVAL_MASK, .chords = CHORD_MASK, .scales = SCALE_MASK,
    .guided = 1, .advance = 1, .ledger = 1,
};
static StudyPreferences prefs;

static int mask_count(uint32_t mask)
{
    int count = 0;
    while (mask) {
        mask &= mask - 1u;
        ++count;
    }
    return count;
}

static int mask_choice(uint32_t mask, uint32_t index)
{
    if (!mask)
        return 0;
    index %= (uint32_t)mask_count(mask);
    for (int i = 0; i < 32; ++i) {
        if ((mask & (1u << i)) && index-- == 0)
            return i;
    }
    return 0;
}

static int mask_step(uint32_t mask, int current, int delta)
{
    int rank = current > 0 ? mask_count(mask & ((1u << current) - 1u)) : 0;
    return mask_choice(mask, (uint32_t)positive_mod(rank + delta, mask_count(mask)));
}

static int reveal_after(void)
{
    return prefs.reveal == 2 ? 0 : 2 + prefs.reveal;
}

static void load_preferences(void)
{
    StudyPreferences saved = default_preferences;
    int size = plat_store_load(OBJ_PREFS, &saved, sizeof saved);
    prefs = default_preferences;
    if (size == 12 && saved.version == 1) {
        /* 0.4 kept twelve intervals and no levels. A set of filters nobody had touched moves to
         * the levels; a chosen set stays as it was, the tritone now both A4 and d5. */
        uint16_t old = saved.intervals;
        saved.guided = old == 0xFFFu && saved.chords == CHORD_MASK && saved.scales == SCALE_MASK && !saved.naturals;
        saved.intervals = (uint16_t)((old & 0x3Fu) | ((old & 0x20u) << 1) | ((old & 0xFC0u) << 1));
        saved.version = PREFS_VERSION;
        size = sizeof saved;
    }
    if (size == (int)sizeof saved && saved.version == PREFS_VERSION && saved.clef <= CLEF_BASS &&
        saved.naturals <= 1 && saved.order <= HEAR_TOGETHER && saved.tone <= TONE_PURE &&
        saved.length >= MIN_ROUND && saved.length <= MAX_ROUND &&
        saved.intervals && !(saved.intervals & ~INTERVAL_MASK) &&
        saved.chords && !(saved.chords & ~CHORD_MASK) &&
        saved.scales && !(saved.scales & ~SCALE_MASK) &&
        saved.guided <= 1 && saved.advance <= 1 && saved.reveal < REVEAL_CHOICES && saved.timer < TIMER_CHOICES &&
        saved.inversions <= 1 && saved.signatures <= 1 && saved.ledger <= 1) {
        prefs = saved;
        /* Older settings used zero for the grand staff. */
        if (!prefs.clef)
            prefs.clef = default_preferences.clef;
    }
}
