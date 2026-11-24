/* SPDX-License-Identifier: MPL-2.0 */
static uint16_t *settings_mask(void)
{
    switch (settings.item) {
    case FILTER_INTERVAL: return &settings.draft.intervals;
    case FILTER_CHORD: return &settings.draft.chords;
    case FILTER_SCALE: return &settings.draft.scales;
    default: return 0;
    }
}

static int settings_options(void)
{
    switch (settings.item) {
    case FILTER_INTERVAL: return topics[INTERVALS].count;
    case FILTER_CHORD: return chord_quality_count();
    case FILTER_SCALE: return topics[SCALES].count;
    default: return 0;
    }
}

static const Shape *settings_option_shape(void)
{
    switch (settings.item) {
    case FILTER_INTERVAL: return &intervals[settings.option];
    case FILTER_CHORD: return chord_shape(settings.option);
    default: return &scales[settings.option];
    }
}

static void settings_change(int delta, uint32_t press)
{
    uint16_t *mask = settings_mask();
    if (mask) {
        settings.option = wrap_index(settings.option + delta, settings_options());
        if (press & ((1u << B_SEL) | (1u << B_EDIT))) {
            uint16_t next = (uint16_t)(*mask ^ (1u << settings.option));
            if (next) {
                *mask = next;
            } else {
                settings.failed = 2;
            }
        }
    } else if (settings.item == FILTER_CLEF) {
        if (delta % 2)
            settings.draft.clef = settings.draft.clef == CLEF_TREBLE ? CLEF_BASS : CLEF_TREBLE;
    } else if (settings.item == FILTER_LENGTH) {
        settings.draft.length = (uint8_t)clamp(settings.draft.length + delta, MIN_ROUND, MAX_ROUND);
    } else if (settings.item == FILTER_NOTES) {
        settings.draft.naturals ^= (uint8_t)(delta & 1);
    } else if (settings.item == FILTER_ORDER) {
        settings.draft.order = (uint8_t)wrap_index(settings.draft.order + delta, 3);
    } else if (settings.item == FILTER_TONE) {
        settings.draft.tone ^= (uint8_t)(delta & 1);
    }
}

static int settings_controls(uint32_t press, uint32_t now, int select, int preset)
{
    if (press & (1u << B_ENV)) {
        settings.open ^= 1;
        if (settings.open) {
            settings.draft = prefs;
            settings.option = 0;
            settings.failed = 0;
        }
        stop_demo();
        library_silence();
        study.wait_release = library.wait_release = 1;
        study.entry_changed = 0;
        study.dirty = 1;
        return 1;
    }
    if (!settings.open)
        return 0;
    if (press & (1u << B_SAVE)) {
        if (plat_store_save(OBJ_PREFS, &settings.draft, sizeof settings.draft) != 0) {
            settings.failed = 1;
        } else {
            StudyPreferences draft = settings.draft;
            int changed_quiz = draft.clef != prefs.clef || draft.length != prefs.length || draft.naturals != prefs.naturals ||
                               draft.order != prefs.order || draft.intervals != prefs.intervals ||
                               draft.chords != prefs.chords || draft.scales != prefs.scales;
            prefs = draft;
            settings.open = 0;
            if (changed_quiz) {
                study.practice_refresh = study.practice_started;
                if (library.page == LIB_TASK && library.group != LIB_CALC) {
                    library_start();
                } else if (library.page == LIB_CLOSED && study.practice) {
                    if (!topic_filter(study.topic))
                        study.mixed_chords = 1;
                    study.practice_refresh = 0;
                    start_round(now, 0);
                }
            }
        }
        study.dirty = 1;
        return 1;
    }
    if (select || preset || press) {
        settings.failed = 0;
        if (select) {
            settings.item = wrap_index(settings.item + select, FILTER_COUNT);
            settings.option = 0;
        }
        settings_change(preset, press);
        study.dirty = 1;
    }
    return 1;
}
