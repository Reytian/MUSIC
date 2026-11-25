/* SPDX-License-Identifier: MPL-2.0 */
/* What the player has learned, kept across power cycles: a level for each kind of material, a
 * best score for each activity, and a box for each thing that can be asked. A box is a Leitner
 * box, 0 for just missed up to BOX_TOP for known cold; questions are drawn more often from the
 * low boxes. */
enum { S_TREBLE = 0,                         /* a staff position, three ledger lines either way */
       S_BASS = S_TREBLE + 24,
       S_PITCH = S_BASS + 24,                /* a pitch class named, lit or played */
       S_EAR_PITCH = S_PITCH + 12,
       S_INTERVAL = S_EAR_PITCH + 12,
       S_EAR_INTERVAL = S_INTERVAL + 13,
       S_CHORD = S_EAR_INTERVAL + 13,        /* type x root */
       S_EAR_CHORD = S_CHORD + 144,
       S_SCALE = S_EAR_CHORD + 12,           /* type x root */
       S_EAR_SCALE = S_SCALE + 132,
       S_SIGNATURE = S_EAR_SCALE + 11,
       S_KEY = S_SIGNATURE + KEY_COUNT,      /* key x degree */
       S_COUNT = S_KEY + 7 * KEY_COUNT };
enum { PROGRESS_VERSION = 1, BOX_NEW = 0xFF, BOX_TOP = 5, ACTIVITY_MAX = 40 };
enum { SOLVED_FLUENT, SOLVED_SLOW, SOLVED_HELPED, SOLVED_NOT };

typedef struct {
    uint8_t version;
    uint8_t level[LV_KINDS];
    uint8_t best[ACTIVITY_MAX];              /* first-try answers a minute in a timed round */
    uint8_t box[S_COUNT];
} Progress;

static Progress progress;
static int progress_dirty, progress_failed;
static uint32_t progress_tried_at;

static struct {
    StudyPreferences draft;
    uint8_t level[LV_KINDS];
    int open, item, option, failed;
} settings;

static const uint8_t level_top[LV_KINDS] = {4, 5, 7, 6, 4};
static const char *const level_names[LV_KINDS] = {"Notes", "Intervals", "Chords", "Scales", "Keys"};
static const char *const level_text[LV_KINDS][7] = {
    {"Naturals inside the staff", "+ ledger lines", "+ sharps and flats", "+ key signatures"},
    {"M2 M3 P4 P5 P8", "+ m2 m3", "+ m6 M6", "+ m7 M7", "+ A4 d5"},
    {"Major, minor on white keys", "All twelve roots", "+ dim, aug", "+ sus2, sus4",
     "+ maj7, m7, 7", "+ m7b5, dim7, mM7", "+ inversions"},
    {"Major from white keys", "Major from all roots", "+ natural minor", "+ harmonic, melodic",
     "+ pentatonics", "+ modes"},
    {"C, G and F major", "Up to 3 sharps or flats", "+ minor keys, up to 5", "All 15 signatures"},
};

static void reset_progress(void)
{
    progress.version = PROGRESS_VERSION;
    for (int i = 0; i < LV_KINDS; ++i)
        progress.level[i] = 1;
    for (int i = 0; i < ACTIVITY_MAX; ++i)
        progress.best[i] = 0;
    for (int i = 0; i < S_COUNT; ++i)
        progress.box[i] = BOX_NEW;
}

static void load_progress(void)
{
    int valid = plat_store_load(OBJ_PROGRESS, &progress, sizeof progress) == (int)sizeof progress &&
                progress.version == PROGRESS_VERSION;
    for (int i = 0; valid && i < LV_KINDS; ++i)
        valid = progress.level[i] >= 1 && progress.level[i] <= level_top[i];
    if (!valid)
        reset_progress();
    for (int i = 0; i < S_COUNT; ++i)
        if (progress.box[i] > BOX_TOP)
            progress.box[i] = BOX_NEW;
    progress_dirty = progress_failed = 0;
}

static void save_progress(uint32_t now)
{
    if (!progress_dirty)
        return;
    progress_tried_at = now;
    progress_failed = plat_store_save(OBJ_PROGRESS, &progress, sizeof progress) != 0;
    if (!progress_failed)
        progress_dirty = 0;
}

/* A save stops the sound for a moment, so it waits for a quiet one. Away from an open question
 * it then goes at once; during a round it goes once a minute, so switching off costs little. A
 * save that failed is tried again after ten seconds, not on every pass. */
static int progress_save_due(uint32_t now, int answering)
{
    uint32_t since = now - progress_tried_at;
    if (!progress_dirty)
        return 0;
    if (progress_failed)
        return since >= 10000u;
    return !answering || since >= 60000u;
}

static void progress_record(int stat, int outcome)
{
    if (stat < 0)
        return;
    int box = progress.box[stat], fresh = box == BOX_NEW;
    if (outcome == SOLVED_FLUENT)
        box = fresh ? 3 : (box < BOX_TOP ? box + 1 : BOX_TOP);
    else if (outcome == SOLVED_SLOW)
        box = fresh || box < 2 ? 2 : box;
    else if (outcome == SOLVED_HELPED)
        box = fresh || box < 2 ? 1 : box - 1;
    else
        box = 0;
    if (box != progress.box[stat]) {
        progress.box[stat] = (uint8_t)box;
        progress_dirty = 1;
    }
}

static int progress_weight(int stat)
{
    static const uint8_t weight[BOX_TOP + 1] = {10, 7, 5, 3, 2, 1};
    if (stat < 0 || progress.box[stat] == BOX_NEW)
        return 4;
    return weight[progress.box[stat]];
}

/* The material in play: the current level of its kind, or the Custom filters (level 0). */
static int guided_level(int kind)
{
    return prefs.guided ? progress.level[kind] : 0;
}

static uint32_t content_intervals(void)
{
    static const uint16_t by_level[] = {0x109A, 0x109F, 0x139F, 0x1F9F, 0x1FFF};
    int level = guided_level(LV_INTERVALS);
    return level ? by_level[level - 1] : prefs.intervals;
}

static uint32_t content_chords(void)
{
    static const uint16_t by_level[] = {0x003, 0x003, 0x00F, 0x03F, 0x1FF, 0xFFF, 0xFFF};
    int level = guided_level(LV_CHORDS);
    return level ? by_level[level - 1] : prefs.chords;
}

static uint32_t content_scales(void)
{
    static const uint16_t by_level[] = {0x001, 0x001, 0x003, 0x027, 0x03F, 0x7FF};
    int level = guided_level(LV_SCALES);
    return level ? by_level[level - 1] : prefs.scales;
}

/* The pool of notes and roots: white keys alone on the first levels of a kind. */
static uint32_t content_roots(int kind)
{
    static const uint8_t white_until[LV_KINDS] = {2, 2, 1, 1, 0};
    int level = guided_level(kind);
    int white = level ? level <= white_until[kind] : kind == LV_NOTES && prefs.naturals;
    return white ? NATURAL_MASK : 0xFFFu;
}

static int content_inversions(void)
{
    int level = guided_level(LV_CHORDS);
    return level ? level >= 7 : prefs.inversions;
}

static int content_signatures(void)
{
    int level = guided_level(LV_NOTES);
    return level ? level >= 4 : prefs.signatures;
}

static int content_ledger(void)
{
    int level = guided_level(LV_NOTES);
    return level ? level >= 2 : prefs.ledger;
}

/* How far into the keys the in-key work reaches: 1 to 4, Custom at the top. */
static int content_key_depth(void)
{
    int level = guided_level(LV_KEYS);
    return level ? level : 4;
}

static int content_key(int key)
{
    static const uint8_t span[] = {1, 3, 5, 7};
    int depth = content_key_depth(), fifths = key_fifths(key);
    if (key_minor(key) && depth < 3)
        return 0;
    return fifths >= -span[depth - 1] && fifths <= span[depth - 1];
}
