/* SPDX-License-Identifier: MPL-2.0 */
/* The Learn tab's tools: turn the encoders and read the result on the staff and the keys. */
enum { CALC_ACCIDENTAL, CALC_INTERVAL, CALC_SCALE, CALC_CHORD, CALC_ROMAN, CALC_MATRIX, CALC_COUNT };
static const char *const calc_names[CALC_COUNT] = {
    "Accidental", "Interval", "Scale", "Chord", "Roman Analysis", "12-Tone Matrix"
};

static struct {
    int page, item;
    int root, quality, fifths, minor, degree, inversion, alteration, position, accidental;
    int count;
    StaffNote target[8];
    uint8_t row[12];
    int row_count, row_mask;
} calc;

static const Shape *calc_shape(void)
{
    if (calc.item == CALC_INTERVAL)
        return &intervals[calc.quality];
    return calc.item == CALC_SCALE ? &scales[calc.quality] : chord_shape(calc.quality);
}

static int calc_quality_count(void)
{
    if (calc.item == CALC_CHORD)
        return chord_quality_count();
    return topics[calc.item == CALC_INTERVAL ? INTERVALS : SCALES].count;
}

static void calc_refresh(void)
{
    if (calc.page != LIB_TASK)
        return;
    if (calc.item == CALC_MATRIX) {
        calc.count = 0;
    } else if (calc.item == CALC_ACCIDENTAL) {
        calc.position = clamp(calc.position, staff_bottom() - 8, staff_bottom() + 9);
        calc.count = 1;
        calc.target[0] = written_note(calc.position, calc.accidental);
    } else if (calc.item == CALC_ROMAN) {
        calc.count = roman_count(calc.quality);
        calc.inversion = clamp(calc.inversion, 0, calc.count - 1);
        roman_notes(calc.target, calc.fifths, calc.minor, calc.degree, calc.alteration, calc.quality, calc.inversion);
        fit_notes(calc.target, calc.count);
    } else {
        const Shape *shape = calc_shape();
        StaffNote root = shape_root(base_key() + wrap_index(calc.root - base_key(), 12), shape);
        calc.count = shape->count;
        for (int i = 0; i < shape->count; ++i)
            calc.target[i] = shape_note(root, shape, i);
    }
    study.dirty = 1;
}

static void calc_play(uint32_t now)
{
    int together = calc.item == CALC_CHORD || calc.item == CALC_ROMAN;
    demo_clear();
    if (calc.item == CALC_MATRIX) {
        int c = base_key() + wrap_index(-base_key(), 12);
        for (int i = 0; i < calc.row_count; ++i)
            demo_add(c + calc.row[i], i);
    } else {
        for (int i = 0; i < calc.count; ++i)
            demo_add(calc.target[i].midi, together ? 0 : i);
    }
    demo_start(now, together ? DEMO_BLOCK : DEMO_NOTES);
}

static void calc_key(int midi)
{
    int pitch = midi % 12;
    if (calc.page == LIB_TASK && calc.item == CALC_MATRIX && calc.row_count < 12 && !(calc.row_mask & (1 << pitch))) {
        calc.row[calc.row_count++] = (uint8_t)pitch;
        calc.row_mask |= 1 << pitch;
        demo_clear();
    }
}

static int calc_controls(uint32_t press, uint32_t now, int select, int algo, int preset, int k1, int k2)
{
    if (study.practice)
        return 0;
    if (press & (1u << B_GLO)) {
        calc.page = calc.page == LIB_MENU ? LIB_CLOSED : LIB_MENU;
        demo_clear();
        study.dirty = 1;
        return 1;
    }
    if (calc.page == LIB_CLOSED)
        return 0;
    if (calc.page == LIB_MENU) {
        if (select || preset) {
            calc.item = wrap_index(calc.item + select + preset, CALC_COUNT);
            study.dirty = 1;
        }
        if (press & ((1u << B_REC) | (1u << B_EDIT) | (1u << B_SEL))) {
            calc.page = LIB_TASK;
            calc.root = calc.quality = calc.fifths = calc.minor = 0;
            calc.degree = calc.inversion = calc.alteration = calc.accidental = 0;
            calc.position = staff_bottom() + 4;
            calc_refresh();
        }
        return 1;
    }

    if (press & (1u << B_PLAY)) {
        calc_play(now);
        return 1;
    }
    if (!(press & ((1u << B_REC) | (1u << B_FX))) && !select && !algo && !preset && !k1 && !k2)
        return 1;
    if (calc.item == CALC_MATRIX) {
        if ((press & (1u << B_REC)) && calc.row_count)
            calc.row_mask &= ~(1 << calc.row[--calc.row_count]);
        if (press & (1u << B_FX))
            calc.row_count = calc.row_mask = 0;
    } else if (calc.item == CALC_ACCIDENTAL) {
        calc.position += select;
        calc.fifths = wrap_index(calc.fifths + algo + 7, 15) - 7;
        calc.accidental = clamp(calc.accidental + preset, -2, 2);
    } else if (calc.item == CALC_ROMAN) {
        calc.fifths = wrap_index(calc.fifths + select + 7, 15) - 7;
        calc.degree = wrap_index(calc.degree + algo, 7);
        calc.quality = wrap_index(calc.quality + preset, chord_quality_count() + 2);
        calc.inversion = clamp(calc.inversion + k1, 0, 3);
        calc.alteration = clamp(calc.alteration + k2, -1, 1);
        if (press & (1u << B_REC))
            calc.minor ^= 1;
    } else {
        calc.root = wrap_index(calc.root + preset, 12);
        calc.quality = wrap_index(calc.quality + algo, calc_quality_count());
        if (calc.item != CALC_SCALE)
            calc.fifths = wrap_index(calc.fifths + select + 7, 15) - 7;
    }
    demo_clear();
    calc_refresh();
    study.dirty = 1;
    return 1;
}
