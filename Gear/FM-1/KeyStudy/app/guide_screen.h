/* SPDX-License-Identifier: MPL-2.0 */
static void guide_frame(uint32_t now)
{
    const QuickGuide *lesson = &quick_guides[guide.page];
    if (ann_on() && ann_where()) {
        char title[106];
        char *end = append_text(append_text(title, "Learn. "), lesson->title);
        append_text(append_text(end, ". "), lesson->lines[0]);
        ann_say(title, 0, ANN_NOW);
    }
    ann_tick(now);
    if (!study.dirty)
        return;
    study.dirty = 0;
    plat_lights((1u << B_HOME) | (library.demo_count ? LED_PLAY_GREEN : 0),
                study.panel | demo_lights(), (1u << B_PLAY) | (1u << B_SEL), 0, 2);

    cv_begin(240, TOP_HEIGHT, STUDY_BG);
    tab(11, "LEARN", "HOME", 1);
    tab(123, "PRACTICE", "SEQ", 0);
    cv_text(12, 33, &FONT_XS, "Quick Reference", STUDY_MUTED);
    char counter[8];
    char *p = append_number(counter, guide.page + 1);
    append_number(append_text(p, "/"), QUICK_GUIDE_COUNT);
    text_right(228, 35, &FONT_T, counter, STUDY_MUTED);
    cv_text(12, 52, &FONT_M, lesson->title, STUDY_INK);
    cv_blit(0, 0);

    cv_begin(240, LOWER_HEIGHT, STUDY_BG);
    cv_rect(11, 0, 218, 108, STUDY_CARD);
    for (int i = 0; i < 7; ++i)
        cv_text(17, 3 + 14 * i, &FONT_XS, lesson->lines[i], i == 0 ? STUDY_MINT : STUDY_INK);
    cv_text(12, 111, &FONT_XS, lesson->example, STUDY_MINT);
    control_label(12, 135, "SELECT", "Topic");
    control_label(92, 135, "PLAY", "Hear");
    control_label(167, 135, "SEL", "Back");
    cv_blit(0, TOP_HEIGHT);
}
