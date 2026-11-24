/* SPDX-License-Identifier: MPL-2.0 */
enum { CLEF_TREBLE = 1, CLEF_BASS };
enum { HEAR_ASCENDING, HEAR_DESCENDING, HEAR_TOGETHER };
enum { TONE_GRAND_PIANO, TONE_PURE };
enum { FILTER_CLEF, FILTER_LENGTH, FILTER_INTERVAL, FILTER_CHORD, FILTER_SCALE,
       FILTER_NOTES, FILTER_ORDER, FILTER_TONE, FILTER_COUNT };
enum { PREFS_VERSION = 1, MIN_ROUND = 5, MAX_ROUND = 30 };
#define INTERVAL_MASK ((1u << (sizeof intervals / sizeof intervals[0])) - 1u)
#define CHORD_MASK ((1u << ((sizeof triads / sizeof triads[0]) + (sizeof sevenths / sizeof sevenths[0]))) - 1u)
#define SCALE_MASK ((1u << (sizeof scales / sizeof scales[0])) - 1u)
#define TRIAD_MASK ((1u << (sizeof triads / sizeof triads[0])) - 1u)
#define NATURAL_MASK 0xAB5u

typedef struct {
    uint8_t version, clef, length, naturals, order, tone;
    uint16_t intervals, chords, scales;
} StudyPreferences;

static const StudyPreferences default_preferences = {
    .version = PREFS_VERSION, .clef = CLEF_TREBLE, .length = 10,
    .intervals = INTERVAL_MASK, .chords = CHORD_MASK, .scales = SCALE_MASK,
};
static StudyPreferences prefs;
static struct {
    StudyPreferences draft;
    int open, item, option, failed;
} settings;

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

static uint32_t quiz_roots(int notes)
{
    return notes && prefs.naturals ? NATURAL_MASK : 0xFFFu;
}

static uint32_t topic_filter(int topic)
{
    switch (topic) {
    case INTERVALS: return prefs.intervals;
    case TRIADS: return prefs.chords & TRIAD_MASK;
    case SEVENTHS: return prefs.chords >> topics[TRIADS].count;
    case SCALES: return prefs.scales;
    default: return 1;
    }
}

static void load_preferences(void)
{
    prefs = default_preferences;
    StudyPreferences saved;
    if (plat_store_load(OBJ_PREFS, &saved, sizeof saved) == (int)sizeof saved &&
        saved.version == PREFS_VERSION && saved.clef <= CLEF_BASS && saved.naturals <= 1 &&
        saved.order <= HEAR_TOGETHER && saved.tone <= TONE_PURE && saved.length >= MIN_ROUND && saved.length <= MAX_ROUND &&
        saved.intervals && !(saved.intervals & ~INTERVAL_MASK) &&
        saved.chords && !(saved.chords & ~CHORD_MASK) &&
        saved.scales && !(saved.scales & ~SCALE_MASK)) {
        prefs = saved;
        /* Older settings used zero for the grand staff. */
        if (!prefs.clef)
            prefs.clef = default_preferences.clef;
    }
    settings.open = settings.item = settings.option = settings.failed = 0;
}
