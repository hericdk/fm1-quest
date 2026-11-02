/* SPDX-License-Identifier: GPL-3.0-only */
/* Compact 240 x 240 song-order editor. All changes and flash saves require
 * stopped transport. The four existing project slots are sections A..D. */
static uint8_t song_cursor, song_store_armed, song_load_armed;
static uint32_t song_store_t;   /* when REC or OCT+ asked for its confirmation (ms) */
UI_SMALL static int on_song_page(void) { return !ui.home && cur_page()->scope == SC_SONG; }
UI_SMALL static void song_sane(void)                         /* a bad chain (blank / damaged settings): defaults */
{
    uint32_t i, ok = arrangement.count >= 1u && arrangement.count <= ARR_STEPS;
    for (i = 0; ok && i < arrangement.count; i++)
        ok = arrangement.entry[i].scene < ARR_SCENES && arrangement.entry[i].bars >= 1u && arrangement.entry[i].bars <= 64u;
    if (!ok)
        arr_defaults(&arrangement);
    if (song_cursor >= arrangement.count)
        song_cursor = (uint8_t)(arrangement.count - 1u);
}



UI_SMALL static void song_screen_input(uint32_t pressed, uint32_t home)
{
    uint32_t k, b;
    int32_t steps;
    song_sane();
    if (home == 1u) { go_home(); return; }       /* BT_TAP */
    for (k = 0; k < NB; k++) {
        if (!((pressed >> panel.btn[k]) & 1u)) continue;
        b = k;
        if ((b == B_PLAY || b == B_REC) && ft_owns_press())
            continue;                            /* (they close a free take: seq.c) */
        if (b == B_PLAY) {
            if (!song.playing && arrangement_enabled && !arr_valid(&arrangement, arrangement_ready())) {
                ui_message("EMPTY SECTION: REC");
            } else transport_req = song.playing ? 2 : 1;
        } else if (b == B_SEQ) {
            open_family(FAM_SEQ);
        } else if (b == B_SAVE || b == B_REC || b == B_OCTDN || b == B_OCTUP) {
            uint32_t scene = arrangement.entry[song_cursor].scene;
            if (song.playing || transport_req) { ui_message("STOP FIRST"); continue; }
            if (b == B_SAVE) {
                arrangement_save();
                song_store_armed = 0;
            } else if (b == B_OCTDN) {
                arrangement_enabled ^= 1u;
                ui_message(arrangement_enabled ? "SONG MODE" : "LOOP MODE");
            } else if (b == B_OCTUP) {                   /* load: a second press within 3 s (unsaved work goes) */
                if (song_load_armed && fm1_ms - song_store_t <= 3000u) {
                    song_load_armed = 0;
                    project_load(scene);
                } else {
                    song_load_armed = 1;
                    song_store_armed = 0;
                    song_store_t = fm1_ms;
                    ui_message("OCT+ AGAIN: LOAD");
                }
            } else if (project_used(scene) &&
                       (!song_store_armed || fm1_ms - song_store_t > 3000u)) {
                song_store_armed = 1;
                song_store_t = fm1_ms;
                ui_message("REC AGAIN: REPLACE");
            } else {
                project_save(scene);
                settings_save();
                song_store_armed = 0;
            }
        }
    }
    for (k = 0; k < 4u; k++) {
        steps = panel_enc(EN_K1 + k);
        if (!steps) continue;
        song_store_armed = 0;
        song_load_armed = 0;
        if (k == 0) {
            song_cursor = (uint8_t)clamp(song_cursor + steps, 0, arrangement.count - 1);
            continue;
        }
        if (song.playing || transport_req) { ui_message("STOP FIRST"); continue; }
        if (k == 1) arrangement.entry[song_cursor].scene = (uint8_t)clamp(arrangement.entry[song_cursor].scene + steps, 0, 3);
        if (k == 2) arrangement.entry[song_cursor].bars = (uint8_t)clamp(arrangement.entry[song_cursor].bars + steps, 1, 64);
        if (k == 3) {
            arrangement.count = (uint8_t)clamp(arrangement.count + steps, 1, ARR_STEPS);
            if (song_cursor >= arrangement.count) song_cursor = arrangement.count - 1;
        }
    }
    /* Unused encoders have no hidden action on this screen. */
    panel_enc(EN_SELECT); panel_enc(EN_ALGO); panel_enc(EN_PRESET);
}
