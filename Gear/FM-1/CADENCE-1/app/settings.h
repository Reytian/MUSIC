/* SPDX-License-Identifier: MPL-2.0 */
static int settings_is_level(int item)
{
    return item >= SET_LEVEL && item < SET_LEVEL + LV_KINDS;
}

/* The level rows show while levels choose the material, the filter rows while Custom does. */
static int settings_row_shown(int item)
{
    if (settings_is_level(item))
        return settings.draft.guided;
    if (item >= SET_INTERVALS && item <= SET_LEDGER)
        return !settings.draft.guided;
    return 1;
}

static uint16_t *settings_mask(void)
{
    switch (settings.item) {
    case SET_INTERVALS: return &settings.draft.intervals;
    case SET_CHORDS: return &settings.draft.chords;
    case SET_SCALES: return &settings.draft.scales;
    default: return 0;
    }
}

static int settings_options(void)
{
    switch (settings.item) {
    case SET_INTERVALS: return topics[INTERVALS].count;
    case SET_CHORDS: return chord_quality_count();
    case SET_SCALES: return topics[SCALES].count;
    default: return 0;
    }
}

static const Shape *settings_option_shape(void)
{
    switch (settings.item) {
    case SET_INTERVALS: return &intervals[settings.option];
    case SET_CHORDS: return chord_shape(settings.option);
    default: return &scales[settings.option];
    }
}

static void settings_change(int delta, uint32_t press)
{
    StudyPreferences *draft = &settings.draft;
    uint16_t *mask = settings_mask();
    int flip = delta & 1;
    if (mask) {
        settings.option = wrap_index(settings.option + delta, settings_options());
        if (press & ((1u << B_SEL) | (1u << B_EDIT))) {
            uint16_t next = (uint16_t)(*mask ^ (1u << settings.option));
            if (next)
                *mask = next;
            else
                settings.failed = 2;
        }
    } else if (settings_is_level(settings.item)) {
        int kind = settings.item - SET_LEVEL;
        settings.level[kind] = (uint8_t)clamp(settings.level[kind] + delta, 1, level_top[kind]);
    } else {
        switch (settings.item) {
        case SET_CLEF:
            if (flip)
                draft->clef = draft->clef == CLEF_TREBLE ? CLEF_BASS : CLEF_TREBLE;
            break;
        case SET_LENGTH: draft->length = (uint8_t)clamp(draft->length + delta, MIN_ROUND, MAX_ROUND); break;
        case SET_TIMER: draft->timer = (uint8_t)wrap_index(draft->timer + delta, TIMER_CHOICES); break;
        case SET_ADVANCE: draft->advance ^= (uint8_t)flip; break;
        case SET_REVEAL: draft->reveal = (uint8_t)wrap_index(draft->reveal + delta, REVEAL_CHOICES); break;
        case SET_CONTENT: draft->guided ^= (uint8_t)flip; break;
        case SET_NOTES: draft->naturals ^= (uint8_t)flip; break;
        case SET_INVERSIONS: draft->inversions ^= (uint8_t)flip; break;
        case SET_SIGNATURES: draft->signatures ^= (uint8_t)flip; break;
        case SET_LEDGER: draft->ledger ^= (uint8_t)flip; break;
        case SET_ORDER: draft->order = (uint8_t)wrap_index(draft->order + delta, 3); break;
        default: draft->tone ^= (uint8_t)flip; break;
        }
    }
}

static int settings_controls(uint32_t press, uint32_t now, int select, int preset)
{
    if (press & (1u << B_ENV)) {
        settings.open ^= 1;
        if (settings.open) {
            settings.draft = prefs;
            for (int i = 0; i < LV_KINDS; ++i)
                settings.level[i] = progress.level[i];
            settings.item = settings.option = settings.failed = 0;
        }
        demo_clear();
        library.wait_release = 1;
        study.dirty = 1;
        return 1;
    }
    if (!settings.open)
        return 0;
    if (press & (1u << B_SAVE)) {
        if (plat_store_save(OBJ_PREFS, &settings.draft, sizeof settings.draft) != 0) {
            settings.failed = 1;
        } else {
            StudyPreferences draft = settings.draft, old = prefs;
            int changed = draft.clef != old.clef || draft.length != old.length || draft.timer != old.timer ||
                          draft.guided != old.guided || draft.naturals != old.naturals || draft.order != old.order ||
                          draft.intervals != old.intervals || draft.chords != old.chords || draft.scales != old.scales ||
                          draft.inversions != old.inversions || draft.signatures != old.signatures ||
                          draft.ledger != old.ledger;
            for (int i = 0; i < LV_KINDS; ++i) {
                if (progress.level[i] != settings.level[i]) {
                    progress.level[i] = settings.level[i];
                    progress_dirty = changed = 1;
                }
            }
            if (draft.clef != old.clef)          /* the keys change pitch with the clef */
                release_panel(now);
            prefs = draft;
            settings.open = 0;
            library.wait_release = 1;
            save_progress(now);
            if (changed) {
                int page = library.page;
                if (page != LIB_CLOSED) {
                    library_start(now);
                    library.page = page;
                }
                calc_refresh();
            }
        }
        study.dirty = 1;
        return 1;
    }
    if (select || preset || press) {
        settings.failed = 0;
        for (int step = select < 0 ? -1 : 1; select; select -= step) {
            do {
                settings.item = wrap_index(settings.item + step, SET_COUNT);
            } while (!settings_row_shown(settings.item));
            settings.option = 0;
        }
        settings_change(preset, press);
        study.dirty = 1;
    }
    return 1;
}
