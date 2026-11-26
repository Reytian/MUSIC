/* SPDX-License-Identifier: MPL-2.0 */
/* The practice library: every activity is a kind of material, shown one way and answered another. */
enum LibraryPage { LIB_CLOSED, LIB_MENU, LIB_TASK };
enum LibraryGroup { LIB_NAME, LIB_SIGHT, LIB_BYEAR, LIB_INKEY, LIB_STAFF, LIB_WRITE, LIB_KEYS, LIB_EAR, LIB_GROUPS };
enum LibraryKind { ITEM_NOTE, ITEM_SIGNATURE, ITEM_INTERVAL, ITEM_SCALE, ITEM_CHORD,
                   ITEM_INVERSION, ITEM_MELODY, ITEM_DEGREE, ITEM_ROMAN, ITEM_PROGRESSION, ITEM_KINDS };
enum { PROMPT_NAME, PROMPT_STAFF, PROMPT_KEYS, PROMPT_SOUND };
enum { ANSWER_PLAY, ANSWER_CHOICE, ANSWER_WRITE };
/* How played keys are matched: a pattern from any octave, the exact keys in order, the pitch
 * classes of a chord in any voicing, or exactly the written keys held together. */
enum { PLAY_ANY_OCTAVE, PLAY_EXACT, PLAY_PITCH_SET, PLAY_EXACT_SET };
enum { TARGET_MAX = 16, BAG_WORDS = 8, AGAIN_MAX = 4, AGAIN_GAP = 3,
       ADVANCE_MS = 900, AUTOPLAY_MS = 280, WRONG_FLASH_MS = 600 };
/* After a reveal, and when something new is shown, the answer stays up until the player has
 * played it once (or pressed EDIT): waiting for that, then done. */
enum { COPY_NONE, COPY_WAIT, COPY_DONE };
/* Saved scores and the resumed activity go by slot. 0.5.0 had 34 activities; one added since
 * takes a slot after them, so the others keep theirs. */
enum { SLOT_STAFF_MELODY = 34 };

typedef struct {
    const char *name, *short_name, *about;
    uint8_t prompt, answer, count;
    uint8_t kinds[5];
} LibraryGroupInfo;

static const LibraryGroupInfo library_groups[LIB_GROUPS] = {
    {"Play from Name", "Play Name", "Read a name, play it on the keys",
     PROMPT_NAME, ANSWER_PLAY, 5, {ITEM_NOTE, ITEM_INTERVAL, ITEM_SCALE, ITEM_CHORD, ITEM_INVERSION}},
    {"Play from Staff", "Play Staff", "Read the staff, play what is written",
     PROMPT_STAFF, ANSWER_PLAY, 5, {ITEM_NOTE, ITEM_INTERVAL, ITEM_SCALE, ITEM_CHORD, ITEM_MELODY}},
    {"Play by Ear", "Play Ear", "Listen, then play it back",
     PROMPT_SOUND, ANSWER_PLAY, 4, {ITEM_NOTE, ITEM_INTERVAL, ITEM_CHORD, ITEM_MELODY}},
    {"Play in Key", "Play Key", "Find degrees and chords inside a key",
     PROMPT_NAME, ANSWER_PLAY, 3, {ITEM_DEGREE, ITEM_ROMAN, ITEM_PROGRESSION}},
    {"Staff Identification", "Read Staff", "Read the staff, choose its name",
     PROMPT_STAFF, ANSWER_CHOICE, 5, {ITEM_NOTE, ITEM_SIGNATURE, ITEM_INTERVAL, ITEM_SCALE, ITEM_CHORD}},
    {"Staff Construction", "Write Staff", "Read a name, write it on the staff",
     PROMPT_NAME, ANSWER_WRITE, 5, {ITEM_NOTE, ITEM_SIGNATURE, ITEM_INTERVAL, ITEM_SCALE, ITEM_CHORD}},
    {"Keyboard Identification", "Keyboard", "See the lit keys, choose their name",
     PROMPT_KEYS, ANSWER_CHOICE, 4, {ITEM_NOTE, ITEM_INTERVAL, ITEM_SCALE, ITEM_CHORD}},
    {"Ear Training", "Ear Training", "Listen, choose its name",
     PROMPT_SOUND, ANSWER_CHOICE, 4, {ITEM_NOTE, ITEM_INTERVAL, ITEM_SCALE, ITEM_CHORD}},
};
static const char *const kind_names[ITEM_KINDS] = {
    "Note", "Key Signature", "Interval", "Scale", "Chord",
    "Inversion", "Melody", "Scale Degree", "Chord in Key", "Progression"
};

static struct {
    int page, group, item;                 /* the activity under way */
    int menu_group, menu_item;             /* the menu's cursor */

    /* The round. */
    int over, seen, solved, first_try, review;
    int missed_count, queue_count, queue_at;
    uint16_t missed[MAX_ROUND], queue[MAX_ROUND];
    uint32_t bag[BAG_WORDS];
    int last_item, asked;                  /* questions posed since the activity opened */
    int again_count, again_due[AGAIN_MAX]; /* missed items waiting to be asked once more */
    uint16_t again[AGAIN_MAX];
    uint32_t clock_ms, clock_at, time_total;
    int timed_answers, level_offer, new_best;

    /* The question. */
    int item_id, stat, result, misses, hinted, show_answer, wait_release;
    int clue;                              /* FX once: the rule, or the question the other way round */
    int copy, teaching;                    /* COPY_*; teaching: a first meeting, shown before it is asked */
    int reasked;                           /* this question came back from the again queue */
    const char *remark;
    int root, quality, fifths, minor, inversion, degree, type, flat;
    int count, steps, given;
    StaffNote target[TARGET_MAX];
    uint8_t column[TARGET_MAX];            /* which chord of a progression a note belongs to */

    /* The answer in progress. */
    int choice, choice_root, choice_accidental;
    uint32_t tried;
    int cursor, filled;
    uint32_t wrong_slots;
    StaffNote written[8];
    int position, starting_note, wrong_note, step;
    int collect, entry_changed;
    uint32_t collected, checked;
    NoteSet checked_keys;
    uint32_t stable_since, shown_at, advance_at, wrong_at, autoplay_at;
} library;

static void library_key(int midi, uint32_t now);
