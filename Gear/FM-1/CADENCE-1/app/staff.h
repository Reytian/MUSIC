/* SPDX-License-Identifier: MPL-2.0 */
typedef struct {
    int8_t offset_y;
    uint8_t width, height;
    const uint8_t *pixels;
} StaffGlyph;

#include "staff_glyphs.h"

#define SCORE_PAPER RGB(235, 241, 229)
#define SCORE_RULE RGB(109, 123, 118)
#define SCORE_INK RGB(22, 36, 40)
#define SCORE_ACTIVE RGB(0, 110, 83)
#define SCORE_WRONG RGB(196, 52, 40)

enum {
    STAFF_SPACE = 6,
    STAFF_SPAN = 4 * STAFF_SPACE,
    STAFF_LEFT = 22,
    STAFF_RIGHT = 222,
    SCORE_HEIGHT = 78,
    SCORE_KEYS_Y = 96,
};

/* What to draw: a run of notes, one chord, or chords side by side. A bit in filled, done or wrong
 * speaks for the note of that index. */
typedef struct {
    const StaffNote *notes;
    const uint8_t *column;        /* the chord each note belongs to, for chords side by side */
    int count, columns;
    int stacked;                  /* whole notes in a stack; else quarter notes in a row */
    uint32_t filled, done, wrong;
    int fifths, cursor, live;
} Score;

typedef struct {
    int top;
    int top_position;
    int bass;
} StaffLayout;

static void staff_symbol(enum StaffSymbol symbol, int x, int y, uint16_t color)
{
    static const uint16_t offset = 0;
    const StaffGlyph *glyph = &staff_glyphs[symbol];
    const felucca_font_t face = {
        .h = glyph->height,
        .first = 'A',
        .last = 'A',
        .adv = &glyph->width,
        .bw = &glyph->width,
        .off = &offset,
        .data = glyph->pixels,
    };
    cv_text(x, y + glyph->offset_y, &face, "A", color);
}

static int note_row(StaffLayout staff, StaffNote note)
{
    return staff.top + (staff.top_position - note.position) * (STAFF_SPACE / 2);
}

static StaffLayout staff_layout(const Score *score)
{
    StaffLayout staff = {.bass = prefs.clef == CLEF_BASS};
    staff.top_position = staff.bass ? -2 : 10;
    int first_row = staff.bass ? 0 : -8;
    int last_row = staff.bass && !score->fifths ? STAFF_SPAN : 34;
    for (int i = 0; i < score->count; ++i) {
        if (!(score->filled & (1u << i)) && i != score->cursor)
            continue;
        int row = note_row(staff, score->notes[i]);
        if (row - 11 < first_row)
            first_row = row - 11;
        if (row + 8 > last_row)
            last_row = row + 8;
    }
    int min_top = 1 - first_row;
    int max_top = SCORE_HEIGHT - 2 - last_row;
    /* Center the staff when possible and leave room for accidentals and ledger lines. */
    staff.top = min_top <= max_top ? clamp(25, min_top, max_top) : (min_top + max_top) / 2;
    return staff;
}

static void draw_staff(StaffLayout staff)
{
    cv_rect(11, 0, 218, SCORE_HEIGHT, SCORE_PAPER);
    for (int i = 0; i < 5; ++i)
        cv_rect(STAFF_LEFT, staff.top + i * STAFF_SPACE, STAFF_RIGHT - STAFF_LEFT + 1, 1, SCORE_RULE);
    cv_rect(STAFF_LEFT, staff.top, 1, STAFF_SPAN + 1, SCORE_INK);
    cv_rect(STAFF_RIGHT, staff.top, 1, STAFF_SPAN + 1, SCORE_RULE);
    staff_symbol(staff.bass ? SYMBOL_BASS : SYMBOL_TREBLE, 26,
                 staff.top + (staff.bass ? STAFF_SPACE : STAFF_SPAN - STAFF_SPACE), SCORE_INK);
}

static void draw_note_head(StaffLayout staff, StaffNote note, int x, int chord, uint16_t ink)
{
    int y = note_row(staff, note);
    int bottom = staff.top + STAFF_SPAN;
    int reach = chord ? 8 : 6;
    for (int ledger = staff.top - STAFF_SPACE; ledger >= y; ledger -= STAFF_SPACE)
        cv_rect(x - reach, ledger, 2 * reach + 1, 1, SCORE_INK);
    for (int ledger = bottom + STAFF_SPACE; ledger <= y; ledger += STAFF_SPACE)
        cv_rect(x - reach, ledger, 2 * reach + 1, 1, SCORE_INK);

    if (chord) {
        cv_rect(x - 4, y - 1, 9, 2, SCORE_PAPER);
        staff_symbol(SYMBOL_WHOLE, x - 5, y, ink);
    } else {
        if (y >= staff.top + STAFF_SPAN / 2)
            cv_rect(x + 3, y - 17, 1, 17, ink);
        else
            cv_rect(x - 4, y, 1, 17, ink);
        staff_symbol(SYMBOL_QUARTER, x - 4, y, ink);
    }
}

static void draw_accidental(StaffLayout staff, StaffNote note, int accidental, int right, int *columns)
{
    static const enum StaffSymbol symbols[] = {
        SYMBOL_TRIPLE_FLAT, SYMBOL_DOUBLE_FLAT, SYMBOL_FLAT, SYMBOL_NATURAL,
        SYMBOL_SHARP, SYMBOL_DOUBLE_SHARP, SYMBOL_TRIPLE_SHARP,
    };
    if (!accidental)
        return;
    int repeat = accidental != ACCIDENTAL_NATURAL && (accidental < -3 || accidental > 3);
    enum StaffSymbol symbol = accidental == ACCIDENTAL_NATURAL ? SYMBOL_NATURAL :
        symbols[(repeat ? (accidental < 0 ? -2 : 2) : accidental) + 3];
    const StaffGlyph *glyph = &staff_glyphs[symbol];
    int y = note_row(staff, note), column = 0;
    if (columns) {
        int glyph_top = y + glyph->offset_y;
        while (column < 3 && glyph_top + glyph->height >= columns[column])
            ++column;
        columns[column] = glyph_top;
    }
    int x = right - glyph->width - 13 * column;
    if (repeat)
        staff_symbol(symbol, x - glyph->width, y, SCORE_INK);
    staff_symbol(symbol, x, y, SCORE_INK);
}

static void draw_key_signature(StaffLayout staff, int fifths)
{
    static const int8_t treble_sharps[] = {10, 7, 11, 8, 5, 9, 6};
    static const int8_t bass_sharps[] = {-4, -7, -3, -6, -9, -5, -8};
    static const int8_t treble_flats[] = {6, 9, 5, 8, 4, 7, 3};
    static const int8_t bass_flats[] = {-8, -5, -9, -6, -10, -7, -11};  /* F-flat below, as most music sets it */
    const int8_t *sharps = staff.bass ? bass_sharps : treble_sharps;
    const int8_t *flats = staff.bass ? bass_flats : treble_flats;
    const int8_t *positions = fifths > 0 ? sharps : flats;
    enum StaffSymbol symbol = fifths > 0 ? SYMBOL_SHARP : SYMBOL_FLAT;
    int count = fifths >= 0 ? fifths : -fifths;
    for (int i = 0; i < count; ++i)
        staff_symbol(symbol, 48 + 8 * i, note_row(staff, (StaffNote){.position = positions[i]}), SCORE_INK);
}

static void draw_notation(const Score *score)
{
    int signs = score->fifths < 0 ? -score->fifths : score->fifths;
    int left = signs ? 62 + 8 * signs : 61, right = STAFF_RIGHT - 11;
    int slots = score->stacked ? (score->columns > 1 ? score->columns : 1) : score->count;
    int gap = slots > 1 ? clamp((right - left) / (slots - 1), 0, 49) : 0;
    int first_x = (left + right) / 2 - gap * (slots - 1) / 2;
    int columns[4], previous = -20, shift = 0, group = -1;

    StaffLayout staff = staff_layout(score);
    cv_band(0, SCORE_HEIGHT);
    draw_staff(staff);
    draw_key_signature(staff, score->fifths);
    for (int i = 0; i < score->count; ++i) {
        int exists = (score->filled >> i) & 1u;
        if (!exists && i != score->cursor)
            continue;
        StaffNote note = score->notes[i];
        int sounding = score->live && (key_contains(&study.demo_notes, note.midi) || key_contains(&study.held, note.midi));
        uint16_t ink = SCORE_INK;
        if (!exists)
            ink = SCORE_RULE;
        else if ((score->wrong >> i) & 1u)
            ink = SCORE_WRONG;
        else if (i == score->cursor || sounding || ((score->done >> i) & 1u))
            ink = SCORE_ACTIVE;
        int accidental = written_accidental(note, score->fifths);
        if (score->stacked) {
            int slot = score->column && score->columns > 1 ? score->column[i] : 0;
            int x = first_x + slot * gap;
            if (slot != group) {
                group = slot;
                previous = -20;
                shift = 0;
                for (int c = 0; c < 4; ++c)
                    columns[c] = 120;
            }
            /* Offset adjacent heads so seconds stay distinct. */
            shift = note.position == previous + 1 ? 9 - shift : 0;
            draw_note_head(staff, note, x + (i == score->cursor && !exists ? 9 : shift), 1, ink);
            draw_accidental(staff, note, accidental, x - 9, columns);
        } else {
            int x = first_x + i * gap;
            draw_note_head(staff, note, x, 0, ink);
            draw_accidental(staff, note, accidental, x - 7, 0);
        }
        previous = note.position;
    }
    cv_band(0, LOWER_HEIGHT);
}
