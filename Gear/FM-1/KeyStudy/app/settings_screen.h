/* SPDX-License-Identifier: MPL-2.0 */
static const char *const settings_labels[] = {
    "Staff clef", "Questions per round", "Intervals", "Chord types", "Scale types", "Note pool", "Ear interval order", "Tone"
};
static const char *const settings_orders[] = {"Ascending", "Descending", "Together"};

static void settings_value(int item, char out[24])
{
    const StudyPreferences *draft = &settings.draft;
    if (item == FILTER_CLEF) {
        append_text(out, draft->clef == CLEF_BASS ? "Bass only" : "Treble only");
    } else if (item == FILTER_LENGTH) {
        append_number(out, draft->length);
    } else if (item == FILTER_INTERVAL || item == FILTER_CHORD || item == FILTER_SCALE) {
        int count = item == FILTER_INTERVAL ? mask_count(draft->intervals) :
                    (item == FILTER_CHORD ? mask_count(draft->chords) : mask_count(draft->scales));
        int total = item == FILTER_INTERVAL ? topics[INTERVALS].count :
                    (item == FILTER_CHORD ? chord_quality_count() : topics[SCALES].count);
        append_number(append_text(append_number(out, count), "/"), total);
    } else if (item == FILTER_NOTES) {
        append_text(out, draft->naturals ? "Naturals only" : "Chromatic");
    } else if (item == FILTER_ORDER) {
        append_text(out, settings_orders[draft->order]);
    } else {
        append_text(out, draft->tone == TONE_GRAND_PIANO ? "Grand Piano" : "Pure Tone");
    }
}

static void settings_frame(uint32_t now)
{
    if (ann_on() && ann_where()) {
        char value[24], message[85];
        settings_value(settings.item, value);
        char *p = append_text(message, "Settings. ");
        p = append_text(p, settings_labels[settings.item]);
        append_text(append_text(p, ". "), value);
        ann_say(message, 0, ANN_NOW);
    }
    ann_tick(now);
    if (!study.dirty)
        return;
    study.dirty = 0;
    plat_lights(1u << B_ENV, study.panel, 1u << B_SAVE, 0, 2);

    cv_begin(240, 45, STUDY_BG);
    library_title_bar("Settings", "KEYSTUDY");
    cv_text(12, 32, &FONT_T, "Changes apply when saved", STUDY_MUTED);
    cv_blit(0, 0);

    /* The lower band is split to keep the shared canvas within its limits. */
    cv_begin(240, 112, STUDY_BG);
    for (int i = 0; i < FILTER_COUNT; ++i) {
        int y = 2 + i * 13;
        int selected = settings.item == i;
        char value[24];
        settings_value(i, value);
        cv_rect(11, y, 218, 12, selected ? STUDY_MINT : STUDY_CARD);
        cv_text(17, y, &FONT_T, settings_labels[i], selected ? STUDY_BG : STUDY_INK);
        text_right(222, y, &FONT_T, value, selected ? STUDY_BG : STUDY_MUTED);
    }
    cv_blit(0, 45);

    cv_begin(240, 83, STUDY_BG);
    uint16_t *mask = settings_mask();
    if (settings.failed) {
        cv_text(12, 5, &FONT_XS, settings.failed == 1 ? "Save failed. Try again." : "Keep at least one per group.", STUDY_RED);
    } else if (mask) {
        cv_rect(11, 0, 218, 30, STUDY_CARD);
        cv_text(17, 6, &FONT_XS, settings_option_shape()->name, STUDY_INK);
        int selected = *mask & (1u << settings.option);
        text_right(221, 8, &FONT_T, selected ? "INCLUDED" : "OFF", selected ? STUDY_MINT : STUDY_MUTED);
    } else {
        cv_text(12, 5, &FONT_XS, settings.item == FILTER_TONE ? "Play keys to preview" : "Turn PRESET to change", STUDY_MUTED);
    }
    control_label(11, 35, "SELECT", "Setting");
    control_label(118, 35, "PRESET", mask ? "Pick type" : "Value");
    if (mask)
        control_label(11, 49, "SEL", "Toggle");
    control_label(11, 63, "SAVE", "Apply");
    control_label(118, 63, "ENV", "Cancel");
    cv_blit(0, 157);
}

#ifdef PLAT_HOST
static int settings_expect(const char *what, int *got)
{
    if (!strcmp(what, "settings_open")) *got = settings.open;
    else if (!strcmp(what, "settings_item")) *got = settings.item;
    else if (!strcmp(what, "pref_clef")) *got = prefs.clef;
    else if (!strcmp(what, "pref_length")) *got = prefs.length;
    else if (!strcmp(what, "pref_intervals")) *got = mask_count(prefs.intervals);
    else if (!strcmp(what, "pref_chords")) *got = mask_count(prefs.chords);
    else if (!strcmp(what, "pref_scales")) *got = mask_count(prefs.scales);
    else if (!strcmp(what, "pref_naturals")) *got = prefs.naturals;
    else if (!strcmp(what, "pref_order")) *got = prefs.order;
    else if (!strcmp(what, "pref_tone")) *got = prefs.tone;
    else if (!strcmp(what, "pref_error")) *got = settings.failed;
    else return 0;
    return 1;
}
#endif
