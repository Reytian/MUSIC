/* SPDX-License-Identifier: MPL-2.0 */
enum LibraryPage { LIB_CLOSED, LIB_MENU, LIB_TASK };
enum LibraryGroup { LIB_STAFF, LIB_WRITE, LIB_KEYS, LIB_EAR, LIB_CALC, LIB_GROUPS };
enum LibraryKind { ITEM_NOTE, ITEM_SIGNATURE, ITEM_INTERVAL, ITEM_SCALE, ITEM_CHORD,
                   ITEM_REVERSE, ITEM_EAR_KEYS, ITEM_ACCIDENTAL, ITEM_ROMAN, ITEM_MATRIX };

static const struct {
    const char *name;
    const char *short_name;
    int count;
    int kinds[6];
} library_groups[] = {
    {"Staff Identification", "Read Staff", 5, {ITEM_NOTE, ITEM_SIGNATURE, ITEM_INTERVAL, ITEM_SCALE, ITEM_CHORD}},
    {"Staff Construction", "Write Staff", 5, {ITEM_NOTE, ITEM_SIGNATURE, ITEM_INTERVAL, ITEM_SCALE, ITEM_CHORD}},
    {"Keyboard Identification", "Keyboard", 5, {ITEM_NOTE, ITEM_REVERSE, ITEM_INTERVAL, ITEM_SCALE, ITEM_CHORD}},
    {"Ear Training", "Ear Training", 5, {ITEM_EAR_KEYS, ITEM_NOTE, ITEM_INTERVAL, ITEM_SCALE, ITEM_CHORD}},
    {"Calculators", "Calculators", 6, {ITEM_ACCIDENTAL, ITEM_INTERVAL, ITEM_SCALE, ITEM_CHORD, ITEM_ROMAN, ITEM_MATRIX}},
};

static struct {
    int page, group, item;
    int result, seen, solved, first_try, missed, hinted, show_answer, wait_release;
    int root, quality, fifths, minor, choice;
    int count, cursor, filled;
    int typed_position, typed_accidental, degree, inversion, alteration;
    StaffNote target[8], written[8];
    uint8_t row[12];
    int row_count, row_mask;
    int demo_count, demo_block;
    uint8_t demo_notes[14];
    uint32_t demo_at;
} library;

static void library_key(int midi);
