/* SPDX-License-Identifier: MPL-2.0 */
static const char *const settings_labels[SET_COUNT] = {
    "Staff clef", "Questions per round", "Timed round", "Next question", "Reveal the answer", "Material",
    "Note level", "Interval level", "Chord level", "Scale level", "Key level",
    "Intervals", "Chord types", "Scale types", "Note pool", "Chord inversions", "Key signatures", "Ledger lines",
    "Ear interval order", "Tone"
};

static void settings_value(int item, char out[24])
{
    static const char *const orders[] = {"Ascending", "Descending", "Together"};
    static const char *const reveals[] = {"After 2 misses", "After 3 misses", "Never"};
    const StudyPreferences *draft = &settings.draft;
    int on = 0;
    if (settings_is_level(item)) {
        int kind = item - SET_LEVEL;
        append_number(append_text(append_number(out, settings.level[kind]), " of "), level_top[kind]);
        return;
    }
    switch (item) {
    case SET_CLEF: append_text(out, draft->clef == CLEF_BASS ? "Bass" : "Treble"); return;
    case SET_LENGTH: append_number(out, draft->length); return;
    case SET_TIMER:
        if (draft->timer)
            append_text(append_number(out, draft->timer), " min");
        else
            append_text(out, "Off");
        return;
    case SET_ADVANCE: append_text(out, draft->advance ? "Automatic" : "Press EDIT"); return;
    case SET_REVEAL: append_text(out, reveals[draft->reveal]); return;
    case SET_CONTENT: append_text(out, draft->guided ? "Levels" : "Custom"); return;
    case SET_INTERVALS:
        append_number(append_text(append_number(out, mask_count(draft->intervals)), "/"), topics[INTERVALS].count);
        return;
    case SET_CHORDS:
        append_number(append_text(append_number(out, mask_count(draft->chords)), "/"), chord_quality_count());
        return;
    case SET_SCALES:
        append_number(append_text(append_number(out, mask_count(draft->scales)), "/"), topics[SCALES].count);
        return;
    case SET_NOTES: append_text(out, draft->naturals ? "Naturals only" : "Chromatic"); return;
    case SET_ORDER: append_text(out, orders[draft->order]); return;
    case SET_TONE: append_text(out, draft->tone == TONE_GRAND_PIANO ? "Grand Piano" : "Pure Tone"); return;
    case SET_INVERSIONS: on = draft->inversions; break;
    case SET_SIGNATURES: on = draft->signatures; break;
    default: on = draft->ledger; break;
    }
    append_text(out, on ? "On" : "Off");
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

    /* The rows on show, and the eight of them around the selection. */
    uint8_t rows[SET_COUNT];
    int count = 0, selected = 0;
    for (int i = 0; i < SET_COUNT; ++i) {
        if (settings_row_shown(i)) {
            if (i == settings.item)
                selected = count;
            rows[count++] = (uint8_t)i;
        }
    }
    int first = clamp(selected - 3, 0, count > 8 ? count - 8 : 0);

    cv_begin(240, 45, STUDY_BG);
    library_title_bar("Settings", "KeyStudy");
    char place[12];
    append_number(append_text(append_number(place, selected + 1), " of "), count);
    cv_caps(12, 32, &FONT_T, "Changes apply when saved", STUDY_MUTED);
    text_right(228, 32, &FONT_T, place, STUDY_MUTED);
    cv_blit(0, 0);

    /* The lower band is split to keep the shared canvas within its limits. */
    cv_begin(240, 112, STUDY_BG);
    for (int row = 0; row < 8 && first + row < count; ++row) {
        int item = rows[first + row], y = 2 + row * 13;
        int chosen = settings.item == item;
        char value[24];
        settings_value(item, value);
        cv_rect(11, y, 218, 12, chosen ? STUDY_MINT : STUDY_CARD);
        cv_caps(17, y, &FONT_T, settings_labels[item], chosen ? STUDY_BG : STUDY_INK);
        text_right(222, y, &FONT_T, value, chosen ? STUDY_BG : STUDY_MUTED);
    }
    cv_blit(0, 45);

    cv_begin(240, 83, STUDY_BG);
    uint16_t *mask = settings_mask();
    if (settings.failed) {
        cv_text(12, 5, &FONT_XS, settings.failed == 1 ? "Save failed. Try again." : "Keep at least one per group.", STUDY_RED);
    } else if (mask) {
        cv_rect(11, 0, 218, 30, STUDY_CARD);
        cv_text(17, 6, &FONT_XS, settings_option_shape()->name, STUDY_INK);
        int included = *mask & (1u << settings.option);
        text_right(221, 8, &FONT_T, included ? "Included" : "Off", included ? STUDY_MINT : STUDY_MUTED);
    } else if (settings_is_level(settings.item)) {
        int kind = settings.item - SET_LEVEL;
        cv_rect(11, 0, 218, 30, STUDY_CARD);
        cv_text(17, 6, &FONT_XS, level_text[kind][settings.level[kind] - 1], STUDY_INK);
    } else {
        static const char *const hints[] = {
            "Bass reads an octave lower.", "A round ends after this many.", "Or play against the clock.",
            "After a correct answer.", "So a miss turns into a lesson.", "Levels grow as you pass rounds.",
        };
        const char *hint = settings.item == SET_TONE ? "Play keys to preview." : "Turn PRESET to change.";
        if (settings.item <= SET_CONTENT)
            hint = hints[settings.item];
        cv_text(12, 5, &FONT_XS, hint, STUDY_MUTED);
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
    else if (!strcmp(what, "pref_timer")) *got = prefs.timer;
    else if (!strcmp(what, "pref_advance")) *got = prefs.advance;
    else if (!strcmp(what, "pref_reveal")) *got = prefs.reveal;
    else if (!strcmp(what, "pref_guided")) *got = prefs.guided;
    else if (!strcmp(what, "pref_intervals")) *got = mask_count(prefs.intervals);
    else if (!strcmp(what, "pref_chords")) *got = mask_count(prefs.chords);
    else if (!strcmp(what, "pref_scales")) *got = mask_count(prefs.scales);
    else if (!strcmp(what, "pref_naturals")) *got = prefs.naturals;
    else if (!strcmp(what, "pref_inversions")) *got = prefs.inversions;
    else if (!strcmp(what, "pref_signatures")) *got = prefs.signatures;
    else if (!strcmp(what, "pref_ledger")) *got = prefs.ledger;
    else if (!strcmp(what, "pref_order")) *got = prefs.order;
    else if (!strcmp(what, "pref_tone")) *got = prefs.tone;
    else if (!strcmp(what, "pref_error")) *got = settings.failed;
    else return 0;
    return 1;
}
#endif
