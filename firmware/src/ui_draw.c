/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca UI drawing: status bar (top), columns + gauges, graphs, focus readout,
 * footer (steps + engine / preset / page). */
static void quest_importer_draw(void);
static void quest_party_draw(void);
static void quest_about_draw(void);
static void quest_page_draw(void);
static void quest_viz_draw(void);
static void quest_games_draw(void);
static int q_page_scene(void);
static void quest_status_draw(void);
/* FM1 Quest (ui_quest.c, included after ui_menu.c) */
static void quest_led_draw(void), quest_rhythm_draw(void), quest_filter_draw(void);
static uint32_t gm_phase(void);                 /* ui_menu.c: the phase of the games (cover, songs, play, result) */
static void quest_tracks_draw(void), quest_rec_draw(void), quest_drum_draw(void), quest_menu_draw(void), quest_equip_draw(void), quest_weather_draw(void), quest_leave(void);
static uint32_t str_hash(uint32_t h, const char *s);

/* --------------------------------------------------------- drawing --- */
UI_SMALL static int is_eng_name(const char *s)                 /* one of the ENGINES[]->name strings */
{
    uint32_t e;
    for (e = 0; e < NENGINES; e++)
        if (s == ENGINES[e]->name)
            return 1;
    return 0;
}

/* at most 5 characters, and no wider than maxw */
UI_SMALL static void fit(char *d, const char *src, const felucca_font_t *f, int32_t maxw)
{
    str_cpy(d, src, is_eng_name(src) ? 8 : 6);           /* engine names are kept whole */
    while (d[0] && text_w(f, d) > maxw)
        d[str_len(d) - 1u] = 0;
}

UI_SMALL static int32_t batt_level(void)                         /* ADC ch3 thresholds */
{
    return song.batt_raw >= 591 ? 3 : song.batt_raw >= 561 ? 2 : song.batt_raw >= 531 ? 1 : 0;
}
/* charging (a USB host powers us; there is no charger status line): bars fill 1, 2, 3
 * every 0.6 s, so the header is redrawn twice a second at most; else the level */
UI_SMALL static int32_t batt_shown(void)
{
    if (usb.config && !usb.suspended)
        return 1 + (int32_t)((fm1_ms / 600u) % 3u);
    return batt_level();
}




/* one column: [icon] LABEL / value unit / gauge, redrawn only when it changed.
 * ratio: 0..1000 for the gauge, -1 = no gauge. icon: ICON_* (icons.c), ICON_AUTO = by label */
#define LABEL_X (FELUCCA_ICONS ? ICON_CELL + ICON_GAP : 0)
static void q_knob(int32_t cx, int32_t cy, int32_t ratio, uint16_t col);   











static UI_SMALL uint32_t steps_hash(const track_t *t)
{
    uint32_t h = 2166136261u, i;
    for (i = 0; i < NSTEP; i++) {
        const step_t *st = &t->step[i];
        h = (h ^ (st->note[0] + st->n * 128u + st->time * 1024u + st->flags * 4096u + st->note[1] * 65536u)) *
            16777619u;
    }
    return h;
}

UI_SMALL static uint32_t str_hash(uint32_t h, const char *s)
{
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h;
}








/* TRACKS page: four channel strips (mixer style), one under each column: number +
 * REC / ARM / MUTE, the sound's short name, then a level fader with the output meter
 * (note activity), the pattern over its LEN (time running down; bar width = notes in
 * the step) and the play head. The selected track is drawn bright. Every part is
 * its own small canvas with its own signature: while the transport runs only the
 * head markers and the meters move. (No ZOOM focus here.) */
#define TS_HEAD_Y (Y_GRAPH + 2)
#define TS_NAME_Y (Y_GRAPH + 20)
#define TS_Y (Y_GRAPH + 38)
#define TS_H 84                                      /* body: rows Y_GRAPH + 38 .. + 121 */
static struct {
    uint32_t head[NTRK], name[NTRK], fader[NTRK], ov[NTRK], mark[NTRK];
    uint8_t meter[NTRK];
} ts;

UI_SMALL static int32_t meter_px(int32_t a)                   /* |sample| (Q15) -> px: 6 dB = TS_H / 10 */
{
    int32_t lg = 0, v;
    if (a < 64)
        return 0;
    while ((a >> lg) > 1)
        lg++;
    v = lg * 8 + (((a << 3) >> lg) & 7);             /* 8 log2(a): 48 (-54 dB) .. 128 (+6 dB) */
    return clamp((v - 48) * (TS_H - 2) / 80, 0, TS_H - 2);
}

UI_SMALL static uint32_t trk_level(uint32_t c)                /* LEVEL 0..127 (the drum track: GLO > DRUMS LEVEL) */
{
    return (uint32_t)(c == TRK_DRUM ? song.g[G_DRLVL] : trk[c].p[P_LEVEL]) & 127u;
}

UI_SMALL static void trk_short_name(uint32_t c, char *b)      /* the track's sound, b holds 13 */
{
    const track_t *t = &trk[c];
    const engine_t *e = ENGINES[t->eng_req % NENGINES];
    if (c == TRK_DRUM)
        str_cpy(b, "DRUM", 13);
    else if (user_of(t) < UP_SLOTS)
        up_name(user_of(t), b);
    else if (e->npresets)
        str_cpy(b, e->presets[t->preset % e->npresets].name, 13);
    else
        str_cpy(b, e->name, 13);
}













/* the UI's timers, once a frame whatever the screen (a message, the BPM highlight, an armed GO, the
 * white value of the knob just turned) */
static void quest_wave_draw(void);
static void quest_unlock_draw(void), quest_ult_art_draw(void), quest_stock_draw(void), quest_song_draw(void), quest_hold_draw(void);
UI_SMALL static void ui_timers(void)
{
    quest_unlock_draw();
    quest_ult_art_draw();
    if (ui.msg_t)
        ui.msg_t--;
    if (ui.bpm_t)
        ui.bpm_t--;
    if (ui.arm_t && !--ui.arm_t)
        ui.arm = 0;
    if (ui.hot_t)
        ui.hot_t--;
}

static uint8_t q_town;                  /* ui_quest.c: which home */
static uint8_t vis_on, vis_shown_last, vis_name_t;   /* (SLOOP 2.6 visualiser: not in the Quest) */
static int vis_shown(void) { return 0; }
static void vis_draw(void) { }
static void stage_tick(void);                           /* ui_quest_stage.c: who is on stage */
static int cd_on(void);                                 /* ui_quest_cd.c: the sound changed, the CDs */
static void n_cd(void);
UI_SMALL static void ui_draw(void)
{
    ui.frame++;
    {   /* a screen change starts a transition: the homes iris in out of black, the menu dithers in, the other pages pixelate */
        static uint32_t last_key = 0xFFFFFFFFu;
        uint32_t key = ui.menu == 7 ? 2007u + gm_phase() * 100u : ui.menu ? 2000u + ui.menu : (!ui.home && cur_page()->scope == SC_TRK) ? 1000u + q_town : 3000u + ui.page;
        if (ui.layer == LY_PLAY && !ui.hold_kind && key != last_key) {
            uint32_t was_home = last_key >= 1000u && last_key < 2000u, is_home = key >= 1000u && key < 2000u, is_menu = key >= 2000u && key < 3000u, was_menu = last_key >= 2000u && last_key < 3000u;
            uint32_t in_game = key >= 2007u && key < 2400u && (key - 2007u) % 100u == 0u && last_key >= 2007u && last_key < 2400u && (last_key - 2007u) % 100u == 0u;
            if (in_game) tr_start(1, 380);                                               /* the games: cover, songs, play, result slide in */
            else if (key == 2009u) tr_start(4, 560);                                     /* the sound select: the tape */
            else if (last_key == 0xFFFFFFFFu || is_menu || was_menu) tr_start(3, 420);          /* power-on and everything of the menu: the halftone fills in */
            else if (is_home && was_home) tr_start(3, 420);                               /* one home to another: the halftone fills in */
            else tr_start(2, 360);                                                       /* the other pages: pixelate */
            last_key = key;
        }
        if (tr_kind) {                                  /* every part of the screen goes through the mask; at the end one clean draw */
            if (fm1_ms - tr_t0 >= tr_dur) tr_kind = 0;
            ui.force = 1;
        }
    }
    pads_tick();
    stage_tick();
    if (rec_go) {                                       /* the take started: say so */
        rec_go = 0;
        ui_message("RECORDING");
    }
    if (ft_bars) {                                      /* a free take closed: the loop it made */
        char m[24];
        uint32_t n = ft_bars;
        ft_bars = 0;
        if (n == 0xFFu) {
            ui_message("TAKE DROPPED");
        } else {
            str_cpy(m, n == 1u ? "LOOP 1 BAR " : n == 2u ? "LOOP 2 BARS " : "LOOP 4 BARS ", sizeof m);
            fmt_int(m + str_len(m), song.g[G_BPM]);
            str_cpy(m + str_len(m), " BPM", sizeof m - str_len(m));
            ui_message(m);
        }
    }
    if (er_flash) {                                     /* EDIT + key took something out */
        er_flash = 0;
        if (!ui.msg_t)
            ui_message("ERASED");
    }
    if (!ui.menu && (ui.layer != LY_PLAY || ui.hold_kind)) {   /* a layer held / a hold to confirm */
        if (ui.hold_kind) {
            layer_shown = 1;
            quest_hold_draw();
        }
        else
            layer_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (layer_shown) {                                  /* back to the page */
        layer_shown = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if ((rec_wait || ft_on) && !ui.menu && !on_song_page()) {   /* armed / free take */
        quest_rec_draw();                               /* FM1 Quest: REC READY / COUNT-IN / the boss */
        ui_timers();
        ui.force = 0;
        return;
    }
    if (rec_shown) {                                    /* back to the page */
        rec_shown = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if (cd_on() && !ui.menu && !ui.hold_kind && ui.layer == LY_PLAY && !rec_wait && !ft_on) {
        n_cd();                                         /* the sound changed: the CDs over everything, then back */
        ui_timers();
        ui.force = 0;
        return;
    }
#if FELUCCA_ARRANGER
    if (!ui.menu && on_song_page()) {
        quest_song_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
#endif
    if (ui.menu) {
        if (ui.menu == 1) quest_menu_draw();            /* FM1 Quest: the inn */
        else if (ui.menu == 3) quest_importer_draw();   /* the IMPORTER list */
        else if (ui.menu == 2) quest_about_draw();      /* the memory and the credits */
        else if (ui.menu == 7) quest_games_draw();      /* FM Hero / Piano Hero */
        else if (ui.menu == 8) quest_led_draw();        /* KEYBOARD LED */
        else if (ui.menu == 10) quest_filter_draw();    /* FILTER */
        else if (ui.menu == 9) quest_rhythm_draw();     /* RHYTHMS */
        else if (ui.menu == 6) quest_viz_draw();        /* the visualizer */
        else if (ui.menu == 4) quest_party_draw();      /* the four heroes */
        else if (ui.menu == 5) quest_status_draw();     /* the selected hero's sheet */
        else quest_leave();

        ui_timers();
        ui.force = 0;
        return;
    }
    if (vis_shown()) {                                  /* the visualiser over the TRACKS screen */
        vis_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (vis_shown_last) {                               /* back from it */
        vis_shown_last = 0;
        vis_on = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if (!ui.home && cur_page()->scope == SC_TRK) {
        quest_tracks_draw();                            /* FM1 Quest: PARTY / BOSS / DUEL */
        ui_timers();
        ui.force = 0;
        return;
    }
    if (on_drum_page()) {
        quest_drum_draw();                              /* FM1 Quest: DUNGEON / KIT */
        ui_timers();
        ui.force = 0;
        return;
    }
    if (q_page_scene() >= 0) {                         /* FM1 Quest: the scene pages (ENV, LFO, SLICER, DLY, REV, SCL) */
        quest_page_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (!ui.home && !is_drum(TSEL) && cur_page()->graph == GR_WAVE) {            /* the mini sample editor (EDIT > WAVE, SAMPLE engine) */
        quest_wave_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (!ui.home && !is_drum(TSEL) && cur_page()->scope == SC_ENGINE) {          /* FM1 Quest: EQUIP (EDIT 1 / 2) */
        quest_equip_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    cursor_fix();
    felucca_dbg.stage = 3;
    quest_stock_draw();                                 /* FM1 Quest: every other page (the old stock layout is gone) */
    ui_timers();
    ui.force = 0;
}
