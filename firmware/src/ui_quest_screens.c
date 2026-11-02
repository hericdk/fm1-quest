/* SPDX-License-Identifier: GPL-3.0-only
 * FM1 Quest, part 3: the screens. Each one replaces a SLOOP screen and reads the same state (song, trk, ui, seq):
 *   PARTY     studio_tracks_draw       BOSS    REC with a playing loop / a free take     DUEL     a soloed track
 *   REC READY / COUNT-IN  rec_screen_draw        DUNGEON / KIT  drum_screen_draw (GRID / KIT)
 *   SPELLS / BANISH / BIOME / CAMP  layer_screen_draw (FX / EDIT / SCL / GLO layers); other layers: a plain pad screen
 *   WEATHER   the FX page (DST CHO DLY REV)       EQUIP   the EDIT 1 / EDIT 2 pages        INN   the menu
 * Layout: a 19 px bar (title in Jacquarda Bastarda, Title Case), a scene (84 px; menu 60, duel 110), the HUD.
 * Redrawing is lazy like the firmware's: an element is drawn again when its signature changes; the scene is paced by a
 * token bucket of LCD SPI bytes (12 MHz bus): ~16 fps while the transport plays, a slow ambient tick otherwise, and
 * a due frame that does not fit the budget is dropped, never queued. */
enum { QSN_NONE, QSN_PARTY, QSN_BOSS, QSN_DUEL, QSN_REC, QSN_COUNT, QSN_DUNGEON, QSN_KIT, QSN_SPELLS, QSN_BANISH, QSN_BIOME, QSN_CAMP, QSN_PADS,
       QSN_WEATHER, QSN_EQUIP, QSN_INN, QSN_IMP, QSN_PTY, QSN_STAT, QSN_ABOUT, QSN_PAGE0, QSN_VIZ = QSN_PAGE0 + 16, QSN_GAME, QSN_TOWN, QSN_SONG, QSN_HOLD, QSN_CAVE };
#define Q_SPI_BPMS 1500                     /* LCD SPI at 12 MHz: bytes per millisecond */
static struct { int32_t tokens; uint32_t tok_ms; uint8_t fresh; } qb;
static void quest_town_draw(void);                 /* ui_quest_town.c */
static void quest_flight_draw(void);               /* ui_quest_flight.c */
static void q_rec_screen(void);
static void q_boss_first(void), q_scene_boss2(void), q_scene_camp2(void), q_rhythm_rows(int32_t y0, uint32_t slot, uint32_t thin);     /* ui_quest_boss.c */

static UI_SMALL void q_enter(uint8_t id)
{
    uint32_t i;
    if (qs.screen == id) return;
    qs.screen = id;
    lcd_fill(0, 0, 240, 240, Q_BG);
    for (i = 0; i < 24u; i++) qs.sig[i] = 0;
    qb.fresh = 1;
    qs.scene_ms = 0;
    if (id != QSN_DUEL) qs.combo = 0;
}
#define Q_FULL (ui.force || qb.fresh)
static UI_SMALL int q_changed(uint32_t slot, uint32_t sig)       /* the element's signature: draw it again when it moved */
{
    if (!Q_FULL && qs.sig[slot] == sig) return 0;
    qs.sig[slot] = sig;
    return 1;
}
static UI_SMALL int q_scene_budget(int32_t cost)                 /* the scene, paced: 1 = draw it now */
{
    uint32_t now = fm1_ms, dt = qb.tok_ms ? now - qb.tok_ms : 0u, share = 55u, period = 62u;                         /* the same pace playing or not */
    if (dt > 250u) dt = 250u;
    qb.tok_ms = now;
    qb.tokens += (int32_t)(dt * Q_SPI_BPMS / 100u * share);
    if (qb.tokens > 375000) qb.tokens = 375000;
    if (Q_FULL) { qb.tokens -= cost; if (qb.tokens < -375000) qb.tokens = -375000; qs.scene_ms = now; return 1; }
    if (now - qs.scene_ms < period) return 0;
    qs.scene_ms = now;
    if (qb.tokens < cost) return 0;                     /* dropped: not enough SPI budget this frame */
    qb.tokens -= cost;
    return 1;
}
static UI_SMALL void q_clocks_and_overlay_done(void) { qb.fresh = 0; }

/* ---- the bar ---- */
static UI_SMALL void q_bar_end(void)
{
    if (ui.msg_t) {                                     /* a message takes the right half of the bar (the firmware's head does the same) */
        cv_rect(100, 0, 140, 18, Q_BG);
        q_text(104, 4, ui.msg, Q_HI);
    }
    cv_rect(0, 18, 240, 1, Q_LINE);
    cv_blit(0, 0);
}
static UI_SMALL uint32_t q_msg_sig(void) { return ui.msg_t ? str_hash(7u, ui.msg) : 0u; }
static UI_SMALL void q_bar_right_play(void)                       /* play triangle + bpm on the right */
{
    char b[8];
    fmt_int(b, song.g[G_BPM]);
    q_text_r(234, 4, b, ui.bpm_t ? Q_HI : Q_TEXT);
    if (song.playing) q_tri(234 - text_w(&FONT_P, b) - 10, 6, Q_TEXT);
}
static UI_SMALL void q_bar_bpm_pos(int rec_pips)                  /* the status bar of PARTY / BOSS: bpm, play, bar.beat, the four beats */
{
    char b[16];
    uint32_t playing = song.playing, beat = clk_beat, k;
    fmt_int(b, song.g[G_BPM]);
    k = (uint32_t)cv_text(6, 4, &FONT_P, b, ui.bpm_t ? Q_HI : Q_TEXT);
    q_text((int32_t)k + 3, 4, "bpm", Q_DIM);
    if (playing) {
        q_tri(66, 6, Q_TEXT);
        fmt_int(b, (int32_t)(beat / 4u + 1u));
        k = str_len(b);
        b[k] = '.', b[k + 1] = (char)('1' + beat % 4u), b[k + 2] = 0;
        q_text(75, 4, b, Q_TEXT);
    } else {
        cv_rect(66, 6, 6, 6, Q_DIM);
        q_text(75, 4, "--", Q_DIM);
    }
    for (k = 0; k < 4u; k++)
        cv_rect(98 + (int32_t)k * 4, 7, 3, 3, playing && (rec_pips ? beat % 4u >= k : beat % 4u == k) ? (rec_pips ? Q_REC : Q_TEXT) : Q_OFF);
}

/* ---- the four dials of the HUD (KNOB 1..4): a knob, a label, a value; an empty label = an empty column ---- */
static UI_SMALL void q_dials(const char *const lab[4], const char *const val[4], const int32_t ratio[4], uint32_t slot, uint32_t sigbase)
{
    uint32_t k, sig = sigbase, pi, np = 0, cur = 0;
    if (!ui.home && ui.menu == 0u)                       /* how many pages this button holds, and which one is open */
        for (pi = 0; pi < NPAGES; pi++)
            if (PAGES[pi].fam == cur_page()->fam) { if (pi == ui.page) cur = np; np++; }
    if (np > 1u) sig = sig * 31u + np * 7u + cur * 131u + 5u;
    for (k = 0; k < 4u; k++) sig = studio_hash(sig * 7u + (uint32_t)ratio[k] + (ui.hot_t && ui.hot_col == k) * 5003u, lab[k]), sig = studio_hash(sig, val[k]);
    if (!q_changed(slot, sig)) return;
    cv_begin(240, 46, Q_BG);
    for (k = 0; k < 4u; k++) {
        int32_t cx = 30 + 60 * (int32_t)k;
        if (!lab[k][0]) continue;
        q_knob(cx, 10, ratio[k], Q_TRK[k]);
        q_text_c(cx, 17, lab[k], Q_DIM);
        q_text_c(cx, 31, val[k], ui.hot_t && ui.hot_col == k ? Q_HI : Q_TEXT);
    }
    if (np > 1u)                                         /* press the button again: the next page (a dot each, the open one lit) */
        for (k = 0; k < np && k < 8u; k++)
            cv_rect(238 - (int32_t)(np - k) * 6, 0, 4, 4, k == cur ? Q_TEXT : Q_OFFHI);
    cv_blit(0, 194);
}

/* ---- a tile of a layer screen as the firmware computed it (colours are its TE_* constants): looked up by value ---- */
typedef struct { uint16_t bg, fg, top; } q_look_t;
static UI_SMALL q_look_t q_tile_look(const tile_t *t)
{
    q_look_t o;
    uint32_t k;
    o.bg = Q_PAD, o.fg = Q_DIM, o.top = Q_PADC;
    if (t->bg == C_BLACK) o.bg = Q_BG;
    else if (t->bg == C_WHITE) o.bg = Q_HI;
    else if (t->bg == TE_RED) o.bg = Q_REC;
    else if (t->bg == TE_G2) o.bg = Q_OFF;
    else for (k = 0; k < 4u; k++) {
        if (t->bg == TE_COL[k]) o.bg = Q_TRK[k];
        else if (t->bg == TE_DIM[k]) o.bg = Q_TRK_DIM[k];
        else if (t->bg == TE_MID[k]) o.bg = Q_TRK[k];
    }
    o.fg = t->fg == C_WHITE ? Q_TEXT : t->fg == C_BLACK ? Q_HITXT : t->fg == TE_RED ? Q_REC : t->fg == TE_G4 ? Q_PDIM : t->fg == TE_G2 ? Q_OFF : Q_DIM;
    if (t->top) {
        o.top = t->top == C_WHITE ? Q_TEXT : t->top == TE_RED ? Q_REC : t->top == TE_DRUM ? Q_TRK[3] : Q_PADC;
        for (k = 0; k < 4u; k++) if (t->top == TE_COL[k]) o.top = Q_TRK[k]; else if (t->top == TE_DIM[k]) o.top = Q_TRK[k];
    }
    return o;
}
static UI_SMALL void q_pad(int32_t x, int32_t y, int32_t w, int32_t h, const char *lab, uint16_t bg, uint16_t fg, uint16_t bd, uint16_t top, int has_top)
{
    int32_t tw;
    cv_rect(x, y, w, h, bd);
    cv_rect(x + 1, y + 1, w - 2, h - 2, bg);
    if (has_top) cv_rect(x, y, w, 2, top);
    tw = text_w(&FONT_P, lab);
    if (tw > w - 2) {                                       /* too long for the pad: cut it */
        char b[12];
        str_cpy(b, lab, sizeof b);
        while (b[0] && text_w(&FONT_P, b) > w - 3) b[str_len(b) - 1u] = 0;
        cv_text(x + (w - text_w(&FONT_P, b)) / 2, y + h / 2 - 7, &FONT_P, b, fg);
    } else cv_text(x + (w - tw) / 2, y + h / 2 - 7, &FONT_P, lab, fg);
}
static UI_SMALL uint32_t q_tiles_sig(const tile_t *tl, uint32_t n)
{
    uint32_t i, sig = 7u;
    for (i = 0; i < n; i++) sig = studio_hash(sig * 31u + tl[i].bg * 3u + tl[i].fg * 5u + tl[i].top * 7u + tl[i].marks, tl[i].lab);
    return sig;
}

/* ---- PARTY ---- */
static UI_SMALL void q_portrait(int32_t x, int32_t y, uint32_t t, uint16_t border, int dead)
{
    cv_rect(x, y, 17, 17, border);
    cv_rect(x + 1, y + 1, 15, 15, Q_OFFHI);
    {   /* the bust of the figure: head, shoulders, the tip of the weapon */
        uint32_t cls = t == 2u && q_var[2] ? 4u : t == 3u ? (q_var[3] ? 5u : 3u) : t;
        uint16_t lt = QF_LT[cls], dk = QF_DK[cls];
        cv_rect(x + 1 + 4, y + 1 + 2, 7, 7, lt), cv_rect(x + 1 + 4, y + 1 + 2, 7, 2, dk), cv_rect(x + 1 + 3, y + 1 + 3, 1, 4, dk), cv_rect(x + 1 + 9, y + 1 + 5, 1, 1, dk);
        cv_rect(x + 1 + 1, y + 1 + 10, 13, 5, lt), cv_rect(x + 1 + 1, y + 1 + 10, 2, 5, dk);
        switch (cls) {
        case 0: cv_rect(x + 1 + 12, y + 1, 2, 2, RGB(0xf2, 0xf2, 0xf8)); break;                              /* the staff's orb */
        case 1: cv_rect(x + 1 + 12, y + 1 + 2, 1, 8, RGB(0x8a, 0x5a, 0x2a)); break;                          /* the bow */
        case 2: cv_rect(x + 1 + 11, y + 1, 3, 3, RGB(0xc8, 0xc8, 0xd2)); break;                              /* the mace */
        case 4: cv_rect(x + 1 + 8, y + 1 + 11, 6, 3, RGB(0x2a, 0x1e, 0x3a)), cv_rect(x + 1 + 9, y + 1 + 12, 4, 1, RGB(0xf2, 0xf2, 0xf8)); break;   /* the FM-1 */
        case 5: cv_rect(x + 1 + 11, y + 1, 3, 4, RGB(0xc8, 0xc8, 0xd2)); break;                              /* the axe */
        default: cv_rect(x + 1 + 12, y + 1, 1, 8, RGB(0xc8, 0xc8, 0xd2)); break;                             /* the sword */
        }
    }
    if (dead) {                                                              /* a resting / silent hero: dimmed */
        int32_t gy, gx;
        for (gy = 1; gy < 16; gy++)
            for (gx = 1 + (gy & 1); gx < 16; gx += 2) cv_pset(x + gx, y + gy, Q_BG);
    }
}
static const uint16_t Q_SELBG[4] = {RGB(0x16, 0x1e, 0x36), RGB(0x1c, 0x2c, 0x14), RGB(0x3a, 0x30, 0x10), RGB(0x3a, 0x24, 0x10)};   /* the active hero's row */
#define Q_SPCOL RGB(0x4a, 0xa8, 0xe8)
static UI_SMALL void q_row(uint32_t i, int32_t y)
{
    track_t *t = &trk[i];
    char nm[16], b[12];
    uint32_t selected = song.sel == i, len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), pos = t->seq_idx % len;
    uint32_t level = i == TRK_DRUM ? (uint32_t)song.g[G_DRLVL] : (uint32_t)t->p[P_LEVEL];
    uint32_t silent = trk_silent(t) || !level, rec = (song.rec >> i) & 1u, solo = (song.solo >> i) & 1u, bank = song.playing ? pos / 16u : 0u, h, j;
    uint32_t fx = fx_active(t), sp = q_sp[i], beam = fxb_t0 && fxb_trk == i && fm1_ms - fxb_t0 < 900u ? 1u + (fm1_ms - fxb_t0) / 60u : 0u;
    int32_t x;
    const char *tag = rec ? "rec" : silent ? "rest" : q_var[i] ? Q_ALT_NAME[i] : Q_HERO_NAME[i];
    if (i == TRK_DRUM) str_cpy(nm, DRUM_KIT_NAMES[drum_kit()], sizeof nm);
    else trk_short_name(i, nm);
    nm[11] = 0;
    h = studio_hash(selected + level * 7u + silent * 997u + rec * 1999u + solo * 4999u + len * 37u + song.playing * 7u + pos * 71u + bank * 13u + fx * 3571u + sp / 4u * 131u + beam * 9973u, nm);
    for (j = 0; j < len; j++) h = h * 31u + (uint32_t)trk_step_on(t, j);
    if (!q_changed(4u + i, h)) return;
    cv_begin(240, 20, Q_BG);
    if (selected) cv_rect(0, 0, 240, 20, Q_SELBG[i]), cv_rect(0, 0, 3, 20, Q_TRK[i]);      /* the active hero: a tinted row and a bar of its colour */
    q_portrait(6, 1, i, selected ? Q_TEXT : Q_TRK[i], (int)silent);
    b[0] = (char)('1' + i), b[1] = 0;
    q_text(26, 0, b, Q_DIM);
    x = cv_text(42, 0, &FONT_P, nm, selected ? Q_TEXT : Q_DIM);
    if (x + 6 + text_w(&FONT_P, tag) < (fx ? 148 : 170)) q_text(x + 6, 0, tag, rec ? Q_REC : Q_TRK[i]);
    for (j = 0; j < 16u; j++) {                          /* the 16 steps in view (the firmware's own folding rules) */
        uint32_t p = bank * 16u + j, on = 0;
        uint16_t c;
        if (len <= 16u) { if (p < len) on = trk_step_on(t, p) ? 2u : 1u; }
        else if (song.playing) { if (p < len) on = trk_step_on(t, p) ? 2u : 1u; }
        else { uint32_t a = j * len / 16u, z = (j + 1u) * len / 16u, k; on = 1; for (k = a; k < z; k++) if (trk_step_on(t, k)) on = 2; }
        c = !on ? RGB(0x0e, 0x0a, 0x0c) : on == 2u ? (silent ? Q_DIM : Q_TRK[i]) : (j % 4u == 0 ? Q_OFFHI : Q_OFF);
        if (song.playing && p == pos) c = Q_TEXT;
        cv_rect(42 + (int32_t)j * 4, 14, 3, 4, c);
    }
    fmt_int(b, (int32_t)(level * 100u / 127u));
    {
        char hp[16];
        int32_t w;
        str_cpy(hp, "hp ", sizeof hp);
        str_cpy(hp + 3, b, sizeof hp - 3);
        w = text_w(&FONT_P, hp);
        q_text_r(218, 0, hp, selected ? RGB(0xff, 0xff, 0xff) : Q_DIM);                  /* the active hero's HP is white */
        if (fx) q_text(218 - w - 20, 0, "fx", Q_HI);                                      /* a running effect: the fx icon beside the HP */
    }
    cv_rect(182, 14, 36, 4, Q_OFF);
    cv_rect(182, 14, (int32_t)(level * 36u / 127u), 4, Q_TRK[i]);
    cv_rect(226, 1, 6, 18, Q_OFF);                       /* SP: a vertical bar that falls; the cleric heals it */
    cv_rect(227, 2 + (int32_t)(16u - sp * 16u / 100u), 4, (int32_t)(sp * 16u / 100u), sp < 25u ? Q_REC : Q_SPCOL);
    if (beam) {                                          /* the cleric's white beam over this hero */
        int32_t bw = 2 + (int32_t)(9u - q_min(9, (int32_t)beam)) ;
        cv_rect(6 + 8 - bw / 2, 0, bw, 20, RGB(0xff, 0xff, 0xff));
    }
    cv_blit(0, y);
}
static UI_SMALL void q_party_dials(void)      /* swing, level, steps, pan of the selected track (the firmware's own dials) */
{
    track_t *t = TSEL;
    static char v[4][8];
    static const char *const lab[4] = {"swing", "level", "steps", "pan"};
    const char *val[4] = {v[0], v[1], v[2], v[3]};
    int32_t ratio[4];
    uint32_t lvl = is_drum(t) ? (uint32_t)song.g[G_DRLVL] : (uint32_t)t->p[P_LEVEL];
    swing_str(v[0], song.g[G_SWING]);
    fmt_int(v[1], t->p[P_MUTE] ? 0 : (int32_t)lvl * 100 / 127);
    fmt_int(v[2], t->p[P_SLEN]);
    fmt_int(v[3], t->p[P_PAN]);
    ratio[0] = song.g[G_SWING] * 10;
    ratio[1] = t->p[P_MUTE] ? 0 : (int32_t)lvl * 1000 / 127;
    ratio[2] = (t->p[P_SLEN] - 1) * 1000 / 63;
    ratio[3] = (t->p[P_PAN] + 64) * 1000 / 127;
    q_dials(lab, val, ratio, 12, song.sel * 17u);
}
static UI_SMALL void q_party_screen(void)
{
    uint32_t i;
    char b[8];
    (void)b;
    q_enter(QSN_PARTY);
    if (qb.fresh) q_idle_seed = fm1_ms | 1u;                 /* a new draw of the camp's activities on every entry */
    if (q_changed(0, studio_hash((uint32_t)song.g[G_BPM] * 7u + song.playing * 3u + (song.playing ? (clk_beat % 4u) * 131u + (clk_beat / 4u) * 7919u : 0u) +
                                 (song.rec != 0) * 1999u + q_msg_sig() + (ui.bpm_t != 0) * 31u, "party"))) {
        cv_begin(240, 19, Q_BG);
        q_bar_bpm_pos(0);
        q_title_r(236, "Party", Q_HI);
        q_homedots(150, 4, 2);
        q_bar_end();
    }
    for (i = 0; i < NTRK; i++) q_row((i + 3u) % NTRK, 108 + (int32_t)i * 20);          /* knight, elf (mage), archer, cleric */
    q_party_dials();
    if (q_scene_budget(240 * 84 * 2)) {
        q_scene_world(QSC_PARTY);
        if (q_run_t0 && fm1_ms - q_run_t0 < 1800u && song.playing) {                  /* "-> Play!": the arrow and the word, plain white */
            int32_t rx = 58, ry = 26;
            cv_rect(rx, ry + 9, 22, 3, RGB(0xff, 0xff, 0xff)), cv_rect(rx + 18, ry + 5, 3, 11, RGB(0xff, 0xff, 0xff)), cv_rect(rx + 21, ry + 8, 2, 5, RGB(0xff, 0xff, 0xff));
            cv_text(rx + 28, ry, &FONT_QT, "Play!", RGB(0xff, 0xff, 0xff));
        }
        if (fxb_t0 && fm1_ms - fxb_t0 < 900u) {          /* the cleric's white light falls on the hero */
            int32_t bw = 2 + (int32_t)(10u * (900u - (fm1_ms - fxb_t0)) / 900u);
            cv_rect(Q_PX[fxb_trk & 3u] + 8 - bw / 2, 0, bw, 84, RGB(0xff, 0xff, 0xff));
        }
        q_scene_flush();
    }
}

/* ---- BOSS: REC armed on a playing loop (README: recording a loop is a fight) or a free take ---- */
static UI_SMALL void q_title_outline(int32_t x, const char *s)
{
    cv_text(x - 1, -1 + 3, &FONT_QT, s, Q_BG), cv_text(x + 1, -1 + 3, &FONT_QT, s, Q_BG), cv_text(x, -2 + 3, &FONT_QT, s, Q_BG), cv_text(x, 3, &FONT_QT, s, Q_BG);
    cv_text(x, 2, &FONT_QT, s, Q_TEXT);
}
static UI_SMALL void q_boss_screen(void)
{
    q_boss_first();                                          /* the first time ever: the mage's ult is half charged, the rest start at zero */
    q_enter(QSN_BOSS);
    if (q_changed(0, studio_hash((uint32_t)song.g[G_BPM] * 7u + (clk_beat % 4u) * 131u + (clk_beat / 4u) * 7919u + q_msg_sig() + (song.rec != 0u) + 3u, "boss"))) {
        cv_begin(240, 19, Q_BG);
        q_bar_bpm_pos(1);
        q_title_r(236, (song.rec || ft_on) ? "Rec" : "Boss", (song.rec || ft_on) ? Q_REC : Q_HI);
        q_homedots(150, 4, 3);
        q_bar_end();
    }
    q_rhythm_rows(176, 1, 1);                                /* the rhythm bars, thin: the boss and his castle get the room */
    q_party_dials();
    if (q_scene_budget(240 * 156 * 2)) q_scene_boss2();
}

static UI_SMALL void q_rec_picture(void)                 /* the CRT campfire (assets/quest/rec-crt.png), 120 wide drawn doubled, in the filter's colours: a range-coded stream decoded as it is drawn */
{
    static uint16_t pr[QREC_N * QREC_N * QREC_N];
    static uint8_t row[2][QREC_W];
    qrc_t r;
    uint32_t x, y, i;
    r.p = QREC_Z, r.range = 0xFFFFFFFFu, r.code = 0;
    for (i = 0; i < 5u; i++) r.code = (r.code << 8) | *r.p++;
    for (i = 0; i < QREC_N * QREC_N * QREC_N; i++) pr[i] = 1024;
    for (i = 0; i < QREC_W; i++) row[0][i] = 0;
    const uint32_t on = song.rec || ft_on, now = fm1_ms, ph = now / 110u, breath = (now / 380u) & 3u, fi = (now / 140u) & 3u;
    static uint8_t fire[QREC_FW * QREC_FH], fire_id = 255;
    if (fi && QREC_NF && fire_id != fi) {                              /* the flame's own frames (the picture is the fourth): decoded when they change */
        qrc_t fr;
        static uint16_t fp[QREC_N * QREC_N * QREC_N];
        fire_id = (uint8_t)fi;
        fr.p = QREC_FZ + QREC_FO[(fi - 1u) % QREC_NF], fr.range = 0xFFFFFFFFu, fr.code = 0;
        for (i = 0; i < 5u; i++) fr.code = (fr.code << 8) | *fr.p++;
        for (i = 0; i < QREC_N * QREC_N * QREC_N; i++) fp[i] = 1024;
        for (y = 0; y < QREC_FH; y++)
            for (x = 0; x < QREC_FW; x++) {
                uint32_t l = x ? fire[y * QREC_FW + x - 1u] : 0u, c = (l * QREC_N + (y ? fire[(y - 1u) * QREC_FW + x] : 0u)) * QREC_N, v = QREC_N - 1u, k;
                for (k = 0; k + 1u < QREC_N; k++)
                    if (qrc_bit(&fr, &fp[c + k])) { v = k; break; }
                fire[y * QREC_FW + x] = (uint8_t)v;
            }
    }
    cv_begin(QW, (uint32_t)QREC_H * 2u, QP(0));
    for (y = 0; y < (uint32_t)QREC_H; y++) {
        uint8_t *cur = row[y & 1u], *up = row[(y & 1u) ^ 1u];
        for (x = 0; x < (uint32_t)QREC_W; x++) {
            uint32_t l = x ? cur[x - 1u] : 0u, c = (l * QREC_N + (y ? up[x] : 0u)) * QREC_N, v = QREC_N - 1u, k, pi;
            for (k = 0; k + 1u < QREC_N; k++)
                if (qrc_bit(&r, &pr[c + k])) { v = k; break; }
            cur[x] = (uint8_t)v;
            pi = QREC_PAL[v];
            if (fi && QREC_NF && x >= QREC_FX && x < QREC_FX + QREC_FW && y >= QREC_FY && y < QREC_FY + QREC_FH) pi = QREC_PAL[fire[(y - QREC_FY) * QREC_FW + x - QREC_FX]];
            if (pi == 9u || pi == 10u) {                                  /* always (armed or recording): the fire breathes - its orange and yellow shimmer, and swell and fade */
                uint32_t h = (x * 7u + y * 13u + ph * 5u) % 11u;
                if (h < 2u) pi = pi == 9u ? 10u : 9u;
                else if (pi == 10u && breath == 2u && h < 5u) pi = 7u;
                else if (pi == 10u && breath == 0u && h < 4u) pi = 9u;
            }
            cv_rect((int32_t)x * 2, (int32_t)y * 2, 2, 2, QP(pi));
        }
    }
    {
        uint32_t i;
        const uint16_t red = Q_REC, hot = RGB(0xff, 0x78, 0x58);
        const uint32_t closed = (now % 3300u) < 170u || ((now + 1700u) % 5400u) < 120u;     /* two red eyes in the dark, blinking now and then */
        static const int8_t EW[5] = {3, 5, 6, 5, 3};
        for (i = 0; i < (on ? 2u : 0u); i++) {                              /* (the eyes only while it records) */
            int32_t cx = i ? 136 : 108, cy = 12, j;
            if (closed) { cv_rect(cx - 6, cy, 12, 1, red); continue; }
            for (j = 0; j < 5; j++) cv_rect(cx - EW[j] + (i ? 0 : 0), cy - 2 + j, EW[j] * 2, 1, j == 2 ? hot : red);
            cv_rect(cx - 2, cy - 1, 4, 3, hot);
            cv_rect(cx + (i ? -7 : 5), cy - 1 - (i ? 0 : 0), 2, 1, red);
        }
        for (i = 0; i < 9u; i++) {                                          /* sparks rise from the fire, drifting */
            uint32_t a = (now / 22u + i * 41u) % 190u;
            int32_t sx = 116 + (int32_t)(i * 7u % 17u) - 8 + qsin((int32_t)((a / 3u + i * 9u) & 63u)) * 6 / 127, sy = 78 - (int32_t)a * 3 / 5;
            cv_rect(sx, sy, 2, 2, a < 70u ? RGB(0xf0, 0xc8, 0x60) : a < 130u ? RGB(0xe0, 0x8a, 0x3c) : RGB(0xc8, 0x44, 0x3c));
        }
    }
}
static void q_rec_list(uint32_t rt, int32_t y0, uint32_t slot);
static UI_SMALL void q_rec_screen(void)                 /* a loop is being recorded: the camp picture, the one list of the four heroes (the one that records outlined), the dials */
{
    q_enter(QSN_REC);
    if (q_changed(0, studio_hash((uint32_t)song.g[G_BPM] * 7u + (clk_beat % 4u) * 131u + (clk_beat / 4u) * 7919u + q_msg_sig() + (song.rec != 0u), "campfire"))) {
        cv_begin(240, 19, Q_BG);
        q_bar_bpm_pos(1);
        q_bar_end();                                         /* (no "Rec" word: the eyes in the dark of the picture say it) */
    }
    q_rec_list(song.sel, 148, 2);
    q_party_dials();
    if (q_scene_budget(240 * 124 * 2)) {
        if (QREC_H) q_rec_picture();
        else q_scene_camp2();
        q_scene_flush();
    }
}

/* ---- the tracks screen of SLOOP is PARTY, BOSS or DUEL ---- */
static UI_SMALL void quest_tracks_draw(void)
{
    q_tick_clocks();
    if (ft_on || (song.rec && song.playing)) q_rec_screen();                          /* recording: the camp */
    else if (q_town == 2u) q_boss_screen();                                           /* the third home: the boss */
    else if (q_town == 3u) quest_flight_draw();         /* the flight: the second home (HOME again: village -> flight -> party -> boss) */
    else if (q_town == 0u) quest_town_draw();           /* the first home: the village */
    else q_party_screen();
    qb.fresh = 0;
}

/* ---- REC READY / COUNT-IN (the firmware's rec_screen_draw; a free take is the boss) ---- */
static UI_SMALL void q_rec_list(uint32_t rt, int32_t y0, uint32_t slot)       /* the one list of the heroes: 16 cells each, what is saved in them, the selected one outlined, the playhead lit */
{
    uint32_t i, j, sig = rt * 3u + rec_wait + drum_kit() * 977u + (uint32_t)song.playing * 5u;
    for (i = 0; i < NTRK; i++) {
        sig = sig * 31u + (uint32_t)trk[i].p[P_SLEN] + (song.playing ? (uint32_t)trk[i].seq_idx * 7u : 0u);
        for (j = 0; j < NSTEP; j++) sig = sig * 3u + (uint32_t)trk_step_on(&trk[i], j);
    }
    if (!q_changed(slot, sig)) return;
    cv_begin(240, 46, Q_BG);
    for (i = 0; i < NTRK; i++) {
        const track_t *t = &trk[i];
        uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), armed = i == rt, head = song.playing ? (t->seq_idx % len) * 16u / len : 99u;
        int32_t y = (int32_t)i * 11;
        if (armed) cv_rect(6, y, 228, 11, Q_REC), cv_rect(7, y + 1, 226, 9, Q_BG);
        cv_rect(10, y + 2, 6, 6, Q_TRK[i]);
        for (j = 0; j < 16u; j++) {
            uint32_t a = j * len / 16u, z = (j + 1u) * len / 16u, k, on = 0;
            if (z == a) z = a + 1u;
            for (k = a; k < z && k < NSTEP; k++) if (trk_step_on(t, k)) on = 1;
            cv_rect(24 + (int32_t)j * 13, y + 3, 11, 4, j == head ? (on ? Q_TEXT : Q_OFFHI) : on ? (armed ? Q_TRK[i] : Q_TRK_DIM[i]) : Q_OFF);
        }
    }
    cv_blit(0, (uint32_t)y0);
}
static UI_SMALL void quest_rec_draw(void)
{
    uint32_t count = ci_on, take = ft_on, empty = project_empty(), free_ = empty && !rec_tempo, rt = song.sel, i;
    q_tick_clocks();
    if (take) { q_rec_screen(); qb.fresh = 0; return; }
    q_enter(count ? QSN_COUNT : QSN_REC);
    if (q_changed(0, studio_hash(count * 3u + ci_beat * 7u + (uint32_t)song.g[G_BPM] + q_msg_sig(), "rec"))) {
        cv_begin(240, 19, Q_BG);
        {
            char b[8];
            fmt_int(b, song.g[G_BPM]);
            i = (uint32_t)cv_text(6, 4, &FONT_P, b, Q_TEXT);
            q_text((int32_t)i + 3, 4, "bpm", Q_DIM);
        }
        cv_rect(66, 6, 6, 6, Q_DIM), q_text(75, 4, "loop", Q_DIM);
        q_text_r(234, 4, count ? "count-in" : "rec ready", Q_REC);
        q_bar_end();
    }
    q_rec_list(rt, 148, 2);
    {   /* the dials: how it records (the firmware's own values) */
        static char v[3][10];
        static const char *const LAB_E[4] = {"mode", "length", "start", ""}, *const LAB_F[4] = {"mode", "", "", ""}, *const LAB_N[4] = {"", "length", "start", ""}, *const LAB_0[4] = {"", "", "", ""};
        const char *val[4] = {v[0], v[1], v[2], ""};
        const char *const *lab = count ? LAB_0 : !empty ? LAB_N : free_ ? LAB_F : LAB_E;
        int32_t ratio[4];
        uint32_t len = (uint32_t)clamp(TSEL->p[P_SLEN], 1, 64), k;
        str_cpy(v[0], rec_tempo ? "tempo" : "free", sizeof v[0]);
        if (len % 16u == 0u) { fmt_int(v[1], (int32_t)(len / 16u)); str_cpy(v[1] + str_len(v[1]), len == 16u ? " bar" : " bars", 6); }
        else { fmt_int(v[1], (int32_t)len); str_cpy(v[1] + str_len(v[1]), " st", 4); }
        str_cpy(v[2], rec_count ? "count" : "note", sizeof v[2]);
        ratio[0] = rec_tempo ? 1000 : 0, ratio[1] = (int32_t)(len - 1u) * 1000 / 63, ratio[2] = rec_count ? 1000 : 0, ratio[3] = 0;
        for (k = 0; k < 4u; k++) if (!lab[k][0]) val[k] = "";
        q_dials(lab, val, ratio, 12, rt * 7u + count * 3u + (uint32_t)rec_tempo * 11u);
    }
    if (q_scene_budget(240 * 124 * 2)) {
        static const char *const L1[3] = {"play freely: warm up", "play a note: record", "press play: record"};
        uint32_t m = free_ ? 0u : rec_count ? 2u : 1u;
        if (QREC_H) q_rec_picture();
        else q_scene_camp2();
        cv_rect(0, 106, 240, 18, Q_BG), cv_rect(0, 106, 240, 1, Q_REC);                                 /* the message, a strip over the foot of the picture */
        if (count) q_text(8, 108, "Recording in one bar!", Q_REC), q_text_r(232, 108, "rec: cancel", Q_PDIM);
        else q_text(8, 108, "The fire is lit!", Q_REC), q_text_r(232, 108, L1[m], Q_TEXT);
        if (count) {                                                          /* the big number (the firmware's seven-segment digit), outlined */
            uint32_t n = 4u - (ci_beat > 3u ? 3u : ci_beat);
            te_digit(52, 18, 30, 54, 6, n, Q_BG);
            te_digit(50, 14, 30, 54, 6, n, Q_TEXT);
        }
        q_scene_flush();
    }
    qb.fresh = 0;
}

/* ---- DUNGEON / KIT (the drum track's two pages) ---- */
static UI_SMALL void quest_drum_draw(void)
{
    uint32_t i, j, len = (uint32_t)clamp(TDRUM->p[P_SLEN], 1, 64), kit = drum_kit(), bank, lit = 0, sig;
    char b[16];
    q_tick_clocks();
    if (drum_cursor >= len) drum_cursor = (uint8_t)(len - 1u);
    bank = drum_cursor / 16u;
    q_enter(drum_page ? QSN_KIT : QSN_DUNGEON);
    for (i = 0; i < DRUM_LANES; i++) if (pad_lit[i]) lit |= 1u << i;
    if (q_changed(0, studio_hash(kit * 131u + drum_page * 7u + (uint32_t)song.g[G_BPM] + q_msg_sig(), DRUM_KIT_NAMES[kit]))) {    /* the bar: the kit's number in an orange box, its name, its style, grid / kit */
        char st[16];
        cv_begin(240, 19, Q_BG);
        fmt_int(b, (int32_t)kit + 1);
        cv_rect(6, 3, text_w(&FONT_P, b) + 8, 13, Q_TRK[3]);
        q_text(10, 3, b, Q_HITXT);
        { char nm[16]; str_cpy(nm, DRUM_KIT_NAMES[kit], sizeof nm); for (j = 1; nm[j]; j++) if (nm[j] >= 'A' && nm[j] <= 'Z') nm[j] = (char)(nm[j] + 32); i = (uint32_t)cv_text(10 + text_w(&FONT_P, b) + 8, -1, &FONT_QT, nm, Q_HI); }
        te_lower(st, DRUM_KIT_STYLES[kit], sizeof st);
        st[10] = 0;
        q_text((int32_t)i + 6, 4, st, Q_DIM);
        q_text_r(212, 4, "grid", drum_page ? Q_DIM : Q_TEXT);
        q_text_r(234, 4, "kit", drum_page ? Q_TEXT : Q_DIM);
        q_bar_end();
    }
    sig = drum_page + drum_lane * 7u + drum_cursor * 101u + song.playing * 71u + len * 3u + lit * 977u;
    if (song.playing) sig = sig * 31u + TDRUM->seq_idx;
    for (i = 0; i < len; i++) {
        const dstep_t *s = &TDRUM->dstep[i];
        sig = sig * 31u + dstep_mask(s) + s->lvl[0] + s->lvl[1] * 7u + s->lvl[2] * 49u + s->lvl[3] * 343u;
        sig = sig * 31u + s->rat[0] + s->rat[1] * 7u + s->rat[2] * 49u + s->rat[3] * 343u;
    }
    if (q_changed(1, sig)) {
        cv_begin(240, 84, Q_BG);
        if (!drum_page) {                                    /* GRID: 8 of the 16 lanes (the half the cursor lane is in) x 16 steps of the bank */
            uint32_t base = drum_lane >= 8u ? 8u : 0u, r;
            for (r = 0; r < 8u; r++) {
                uint32_t lane = base + r;
                int32_t y = (int32_t)r * 10 + 2;
                cv_rect(6, y, 4, 8, pad_lit[lane] ? Q_TEXT : lane == drum_lane ? Q_TRK[3] : Q_OFF);
                for (j = 0; j < 16u; j++) {
                    uint32_t p = bank * 16u + j;
                    const dstep_t *s = &TDRUM->dstep[p < NSTEP ? p : 0];
                    uint16_t c;
                    int32_t x = 14 + (int32_t)j * 220 / 16;
                    if (p >= len) continue;
                    if (dstep_has(s, lane)) { uint32_t lv = dstep_lvl(s, lane); c = lv == LV_GHOST ? Q_TRK_DIM[3] : lv == LV_SOFT ? RGB(0xb4, 0x6c, 0x2c) : lv == LV_HARD ? Q_TEXT : Q_TRK[3]; }
                    else c = (song.playing && p == TDRUM->seq_idx) ? Q_OFFHI : (j % 4u == 0 ? Q_OFFHI : Q_OFF);
                    cv_rect(x, y, 12, 8, c);
                    if (dstep_has(s, lane) && dstep_rat(s, lane)) { uint32_t rr; for (rr = 0; rr < dstep_rat(s, lane); rr++) cv_rect(x + 1 + (int32_t)rr * 3, y + 3, 2, 1, Q_BG); }
                    if (p == drum_cursor && lane == drum_lane) { cv_rect(x - 1, y - 1, 14, 1, Q_REC); cv_rect(x - 1, y + 8, 14, 1, Q_REC); cv_rect(x - 1, y, 1, 8, Q_REC); cv_rect(x + 12, y, 1, 8, Q_REC); }
                }
            }
        } else {                                             /* KIT: 16 pads, lit on each hit */
            for (i = 0; i < DRUM_LANES; i++) {
                int32_t x = 6 + (int32_t)(i % 4u) * 58, y = (int32_t)(i / 4u) * 19;
                q_pad(x, y, 56, 17, lane_label(i), pad_lit[i] ? Q_TRK[3] : Q_PAD, pad_lit[i] ? Q_HITXT : i == drum_lane ? Q_TEXT : Q_DIM, Q_PADC, 0, 0);
            }
        }
        cv_blit(0, 108);
    }
    {   /* the firmware's own dials: sound step hit level / kit level reverb pan */
        static char v[4][12];
        const char *val[4] = {v[0], v[1], v[2], v[3]};
        static const char *const LG[4] = {"sound", "step", "hit", "level"}, *const LK[4] = {"kit", "level", "reverb", "pan"};
        int32_t ratio[4];
        const dstep_t *s = &TDRUM->dstep[drum_cursor];
        if (!drum_page) {
            str_cpy(v[0], lane_label(drum_lane), 8); fmt_int(v[1], drum_cursor + 1);
            str_cpy(v[2], dstep_has(s, drum_lane) ? "on" : "--", 4); str_cpy(v[3], dstep_has(s, drum_lane) ? LV_NAME[dstep_lvl(s, drum_lane)] : "--", 8);
            ratio[0] = (int32_t)drum_lane * 1000 / (DRUM_LANES - 1); ratio[1] = (int32_t)drum_cursor * 1000 / (int32_t)(len > 1u ? len - 1u : 1u);
            ratio[2] = v[2][0] == 'o' ? 1000 : 0; ratio[3] = dstep_has(s, drum_lane) ? (int32_t)((dstep_lvl(s, drum_lane) + 1u) % 4u) * 333 : 0;
            q_dials(LG, val, ratio, 12, 1u);
        } else {
            fmt_int(v[0], (int32_t)kit + 1); fmt_int(v[1], song.g[G_DRLVL] * 100 / 127); fmt_int(v[2], song.g[G_DRREV] * 100 / 127); fmt_int(v[3], TDRUM->p[P_PAN]);
            ratio[0] = (int32_t)kit * 1000 / (int32_t)(DRUM_KITS - 1u); ratio[1] = song.g[G_DRLVL] * 1000 / 127; ratio[2] = song.g[G_DRREV] * 1000 / 127; ratio[3] = (TDRUM->p[P_PAN] + 64) * 1000 / 127;
            q_dials(LK, val, ratio, 12, 2u);
        }
    }
    if (q_scene_budget(240 * 84 * 2)) { if (drum_page) q_scene_kit(); else q_scene_world(QSC_STEPS); q_scene_flush(); }
    qb.fresh = 0;
}

/* ---- the layers: SPELLS (FX), BANISH (EDIT), BIOME (SCL), CAMP (GLO); the rest get a plain pad screen ---- */
static UI_SMALL void quest_layer_draw(uint32_t layer, const char *sub, const tile_t *tl, const char *const lab[4], const char *const val[4], const int32_t ratio[4])
{
    static const char *const NAME[8] = {"", "Spells", "Banish", "Roll", "Steps", "Biome", "Camp", "Song"};
    uint32_t id = layer == LY_FX ? QSN_SPELLS : layer == LY_ERASE ? QSN_BANISH : layer == LY_SCALE ? QSN_BIOME : layer == LY_MIX ? QSN_CAMP : QSN_PADS, i, pads_y, ph, pitch;
    uint16_t tcol = layer == LY_ERASE ? Q_REC : Q_HI;
    int has_scene = id != QSN_PADS;
    q_tick_clocks();
    q_enter((uint8_t)(id == QSN_PADS ? QSN_PADS + (layer & 7u) * 0 : id));
    if (q_changed(0, studio_hash((uint32_t)song.g[G_BPM] * 7u + song.playing * 3u + layer + (ly_lock != LY_PLAY) * 977u + q_msg_sig(), sub))) {
        cv_begin(240, 19, Q_BG);
        q_title(6, NAME[layer & 7u], tcol);
        q_text(6 + text_w(&FONT_QT, NAME[layer & 7u]) + 8, 4, layer == LY_MIX ? "rest  tap" : sub, Q_DIM);
        if (ly_lock != LY_PLAY) q_text_r(190, 4, "LOCK", Q_TEXT);
        q_bar_right_play();
        q_bar_end();
    }
    pads_y = has_scene ? 108u : 26u, ph = has_scene ? 17u : 27u, pitch = has_scene ? 19u : 30u;
    if (layer == LY_SCALE) ph = 14u, pitch = 16u;
    if (q_changed(1, q_tiles_sig(tl, 16u) + layer * 13u)) {
        cv_begin(240, 4u * pitch, Q_BG);
        if (layer == LY_MIX) {                                           /* camp: rest 1-4, duel 1-4, then tap tempo and the bpm (the unused row is skipped) */
            static const uint8_t ORDER[8] = {0, 1, 2, 3, 12, 13, 14, 15};
            for (i = 0; i < 8u; i++) {
                const tile_t *t = &tl[ORDER[i]];
                q_look_t lk = q_tile_look(t);
                char nm[8];
                str_cpy(nm, t->lab, sizeof nm);
                if (i < 4u) {                                            /* the firmware lights a mute tile while the track is HEARD; the skin lights "rest n" while the hero RESTS */
                    int muted = t->bg == TE_G2;
                    str_cpy(nm, "rest 1", sizeof nm), nm[5] = (char)('1' + i);
                    q_pad(6 + (int32_t)(i % 4u) * 58, (int32_t)(i / 4u) * (int32_t)pitch, 56, (int32_t)ph, nm, muted ? Q_TRK[i] : Q_PAD, muted ? Q_HITXT : Q_DIM, Q_PADC, 0, 0);
                    continue;
                }
                q_pad(6 + (int32_t)(i % 4u) * 58, (int32_t)(i / 4u) * (int32_t)pitch, 56, (int32_t)ph, nm, lk.bg, lk.fg, Q_PADC, lk.top, t->top != 0);
            }
        } else {
            for (i = 0; i < 16u; i++) {
                q_look_t lk = q_tile_look(&tl[i]);
                int32_t x = 6 + (int32_t)(i % 4u) * 58, y = (int32_t)(i / 4u) * (int32_t)pitch;
                if (tl[i].bg == C_BLACK && !tl[i].lab[0]) continue;
                q_pad(x, y, 56, (int32_t)ph, tl[i].lab, lk.bg, lk.fg, tl[i].bg == TE_RED ? Q_REC : Q_PADC, lk.top, tl[i].top != 0);
                if (tl[i].marks) { uint32_t m; for (m = 0; m < tl[i].marks; m++) cv_rect(x + 24 + (int32_t)m * 5, y + (int32_t)ph - 5, 3, 3, lk.fg); }
            }
        }
        cv_blit(0, pads_y);
    }
    if (layer == LY_SCALE && q_changed(2, (uint32_t)trk[0].p[P_SCALE] * 7u + 1u)) {            /* the biome legend */
        static const struct { uint8_t sc; const char *k, *v; } LG[4] = {{1, "maj", "forest"}, {2, "min", "cave"}, {3, "dor", "coast"}, {8, "phr", "ruins"}};
        cv_begin(240, 16, Q_BG);
        for (i = 0; i < 4u; i++) {
            int32_t x = 6 + (int32_t)i * 58, w;
            int on = trk[0].p[P_SCALE] == LG[i].sc;
            if (on) { cv_rect(x, 0, 56, 16, Q_TRK[1]); cv_rect(x + 1, 1, 54, 14, Q_BG); }
            w = text_w(&FONT_P, LG[i].v);
            q_text(x + (56 - w) / 2, 0, LG[i].v, on ? Q_TRK[1] : Q_DIM);
        }
        cv_blit(0, 174);
    }
    if (layer == LY_ROLL) {                                               /* the arpeggio's last notes, as a strip: higher notes higher */
        static uint8_t hs[20], last;
        uint32_t nt = TSEL->arp_note, sg = 5u + song.sel;
        if (nt && nt != last) { uint32_t k; for (k = 0; k < 19u; k++) hs[k] = hs[k + 1u]; hs[19] = (uint8_t)nt; }
        last = (uint8_t)nt;
        for (i = 0; i < 20u; i++) sg = sg * 31u + hs[i];
        if (q_changed(2, sg)) {
            cv_begin(240, 44, Q_BG);
            for (i = 0; i < 20u; i++) {
                int32_t v = hs[i], y = 38 - (q_max(36, q_min(96, v)) - 36) * 34 / 60;
                if (!v) cv_rect(10 + (int32_t)i * 11, 40, 8, 2, Q_OFF);
                else cv_rect(10 + (int32_t)i * 11, y, 8, 5, i == 19u ? Q_TEXT : Q_TRK[song.sel & 3u]);
            }
            cv_blit(0, 150);
        }
    }
    q_dials(lab, val, ratio, 12, layer * 7919u);
    if (has_scene && q_scene_budget(240 * 84 * 2)) {
        if (id == QSN_SPELLS) q_scene_world(QSC_PUNCH);
        else if (id == QSN_BANISH) q_scene_erase();
        else if (id == QSN_BIOME) q_scene_world(QSC_KEY);
        else q_scene_camp();
        q_scene_flush();
    }
    qb.fresh = 0;
}

/* ---- INN: the menu (HOME held). ABOUT keeps the firmware's own page. ---- */
static UI_SMALL void quest_menu_draw(void)
{
    uint32_t i, sig = ui.menu_sel * 131u + settings.palette * 1009u + settings.lowcut * 7919u + usb_full * 49979687u + boot_mute * 104729u + q_ult_simple * 1299709u + settings.zoom * 15485863u + q_ult_fullscr * 15485863u;
    q_tick_clocks();
    q_enter(QSN_INN);
    if (q_changed(0, 1u)) {
        cv_begin(240, 19, Q_BG);
        q_title(6, "Menu", Q_HI);
        q_text(6 + text_w(&FONT_QT, "Menu") + 8, 4, "the party rests", Q_DIM);
        q_bar_end();
    }
    if (q_changed(1, sig)) {
        cv_begin(240, 124, Q_BG);
        q_panel(6, 0, 228, 120);
        {
        uint32_t top = ui.menu_sel > 5u ? ui.menu_sel - 5u : 0u;
        if (top + 7u > MI_COUNT) top = MI_COUNT > 7u ? MI_COUNT - 7u : 0u;
        for (i = top; i < top + 7u && i < MI_COUNT; i++) {
            int32_t y = 4 + (int32_t)(i - top) * 16;
            int sel = i == ui.menu_sel;
            const char *val = "";
            if (i == MI_LOWCUT) val = settings.lowcut ? "ON" : "OFF"; else if (i == MI_USB) val = usb_full ? "FULL" : "MASTER"; else if (i == MI_BOOT) val = boot_mute ? "OFF" : "ON"; else if (i == MI_DENS) val = Q_OPT_DENS() == 0u ? "FEW" : Q_OPT_DENS() == 1u ? "NORMAL" : "MANY"; else if (i == MI_DMG) val = Q_OPT_DMG() ? "ON" : "OFF"; else if (i == MI_BSPD) val = Q_OPT_BSPD() == 0u ? "SLOW" : Q_OPT_BSPD() == 1u ? "NORMAL" : "FAST"; else if (i == MI_ULT) val = q_ult_simple ? "OFF" : "SIDE";
            if (sel) q_tri(12, y + 4, Q_TEXT);
            q_text(22, y, MI_NAME[i], sel ? Q_TEXT : Q_DIM);
            q_text(128, y, val, sel ? Q_TEXT : Q_PDIM);
        }
        }
        cv_blit(0, 82);
        cv_begin(240, 32, Q_BG);
        q_text(6, 0, "presets: move", Q_DIM), q_text(130, 0, "knob 1: set", Q_DIM);
        q_text(6, 16, "oct+: ok", Q_DIM), q_text(130, 16, "oct-: back", Q_DIM);
        cv_blit(0, 208);
    }
    if (q_scene_budget(240 * 60 * 2)) { q_scene_menu(); q_scene_flush(); }
    qb.fresh = 0;
}

/* ---- SOUNDS (menu): every sound of the selected hero (synth bank / drum kits). OCT+ = play / pause (the other tracks are soloed out), SEL keeps it ---- */
static UI_SMALL void sx_name(char *d, uint32_t i, uint32_t max)
{
    const char *n;
    if (is_drum(TSEL)) n = DRUM_KIT_NAMES[i % DRUM_KITS];
    else if (i >= sx_np) n = BR_PAD[((i - sx_np) / 16u) & 1u][(i - sx_np) % 16u];     /* the Brazilian instruments */
    else if (i < NBANK) n = BANK[i].name;
    else n = "USER";
    str_cpy(d, n, max);
}
static UI_SMALL void sx_btn(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t edge, uint16_t fill)   /* a button: bevelled box */
{
    cv_rect(x, y, w, h, fill), cv_rect(x, y, w, 1, edge), cv_rect(x, y + h - 1, w, 1, edge), cv_rect(x, y, 1, h, edge), cv_rect(x + w - 1, y, 1, h, edge);
    cv_rect(x + 1, y + h, w - 1, 1, Q_PDIM);                                      /* its shadow */
}
static void gm_cass(int32_t x, int32_t y, int32_t w, uint16_t col, int empty);                     /* (ui_quest_pages.c) */
static UI_SMALL void quest_rhythm_draw(void)
{
    static const uint16_t KC[8] = {RGB(0xc8, 0x48, 0x3a), RGB(0x5a, 0x8a, 0xd0), RGB(0xe0, 0x80, 0x3a), RGB(0x8a, 0x6a, 0x9a), RGB(0xf0, 0xc8, 0x60), RGB(0x7a, 0xb0, 0x4e),
                                   RGB(0x4a, 0xa8, 0xa8), RGB(0xe0, 0x90, 0x90)};
    static uint32_t roll_ms;
    uint32_t i;
    q_tick_clocks();
    q_enter(QSN_WEATHER);
    if (fm1_ms - roll_ms >= 24u) {                                                    /* the strip eases to its tape: fast start, soft end */
        int32_t dc = sx_tgt - sx_cur;
        roll_ms = fm1_ms;
        if (dc) sx_cur += dc * 2 / 5 ? dc * 2 / 5 : dc > 0 ? 1 : -1;
    }
    if (q_changed(0, 2u + song.sel + (uint32_t)(ui.msg_t != 0) * 5u)) {
        char b[16];
        cv_begin(240, 19, Q_BG);
        q_title(6, "Sounds", Q_HI);
        str_cpy(b, q_var[song.sel] ? Q_ALT_NAME[song.sel] : Q_HERO_NAME[song.sel], sizeof b);
        q_text(236 - text_w(&FONT_P, b), 3, b, Q_TRK[song.sel]);
        q_bar_end();
    }
    if (q_changed(1, 5u + (uint32_t)sx_cur * 3u + fm1_ms / 100u + sx_on * 977u)) {   /* the tapes, horizontally: the middle one big */
        int32_t pass, k, bob = (qsin((int32_t)(fm1_ms / 40u) & 63) * 3) / 127;
        cv_begin(240, 80, Q_BG);
        for (pass = 0; pass < 2; pass++)
            for (k = -3; k <= 3 && sx_n; k++) {
                int32_t off = k * 256 + (sx_tgt - sx_cur), a = off < 0 ? -off : off, t = a < 256 ? a : 256, cx = 120 + off * 86 / 256, w = 70 - 24 * t / 256, y = 8 + 14 * t / 256 - bob * (256 - t) / 256;
                uint32_t n = (sx_i + sx_n * 4u + (uint32_t)(k + 3)) % sx_n;
                uint16_t c = is_drum(TSEL) ? KC[n % 8u] : n >= sx_np ? KC[(n - sx_np) % 8u] : n < NBANK ? KC[BANK[n].kind & 7u] : Q_PDIM;
                if ((a >= 128) != (pass == 0) || cx < -40 || cx > 280) continue;
                if (a >= 128) c = (uint16_t)((c >> 1) & 0x7BEFu);
                gm_cass(cx - w / 2, y, w, c, 0);
                if (a < 64) cv_rect(cx - w / 2, y + w * 3 / 5 + 4, w, 2, sx_on ? Q_TEXT : Q_HI);
            }
        cv_blit(0, 24);
    }
    if (q_changed(2, 9u + sx_i * 31u + sx_com * 7u + sx_on * 977u + song.sel * 131u)) {
        char b[24], nm[16];
        int32_t x;
        cv_begin(240, 80, Q_BG);
        sx_name(nm, sx_i, sizeof nm);
        x = 120 - text_w(&FONT_P, nm) / 2;
        q_text(x < 4 ? 4 : x, 6, nm, Q_TEXT);
        if (sx_i == sx_com) q_text(228, 4, "*", Q_HI);                              /* the sound the track has */
        str_cpy(b, is_drum(TSEL) ? "kit" : sx_i >= sx_np ? "percussion" : sx_i < NBANK ? BANK_KIND[BANK[sx_i].kind] : "user", sizeof b);
        str_cpy(b + str_len(b), "  ", 3), fmt_int(b + str_len(b), (int32_t)sx_i + 1), str_cpy(b + str_len(b), " / ", 4), fmt_int(b + str_len(b), (int32_t)sx_n);
        q_text(120 - text_w(&FONT_P, b) / 2, 28, b, Q_DIM);
        sx_btn(8, 52, 40, 22, Q_HI, Q_WOOD);                                      /* SEL: a button */
        q_text(14, 56, "SEL", Q_TEXT);
        q_text(58, 52, "copy it to your track", Q_TEXT);
        q_text(58, 66, sx_on ? "playing alone" : "turn and rest: it plays", Q_DIM);
        cv_blit(0, 106);
    }
    qb.fresh = 0;
}

/* ---- FILTER (menu): the colour filters as radio buttons; the strip of scene above is the preview (cheap: 60 rows, redrawn when the pick moves), and the ramp of each one beside it ---- */
static UI_SMALL void quest_filter_draw(void)
{
    uint32_t i, top = qf_sel > 4u ? qf_sel - 4u : 0u, sig = 61u + qf_sel * 131u + q_filter * 977u + (uint32_t)(ui.msg_t != 0) * 7u;
    q_tick_clocks();
    q_enter(QSN_INN);
    if (q_changed(0, 3u + (uint32_t)(ui.msg_t != 0))) {
        cv_begin(240, 19, Q_BG);
        q_title(6, "Filter", Q_HI);
        q_text(6 + text_w(&FONT_QT, "Filter") + 8, 4, "preview above", Q_DIM);
        q_bar_end();
    }
    if (q_changed(1, sig)) {
        if (top + 8u > 12u) top = 4u;
        cv_begin(240, 124, Q_BG);
        q_panel(6, 0, 228, 120);
        for (i = top; i < top + 8u && i < 12u; i++) {
            int32_t y = 4 + (int32_t)(i - top) * 14;
            int on = i == qf_sel;
            const q_flt_t *f = &Q_FLT[i];
            uint32_t k, nt = i == 0u ? 4u : f->n;
            if (on) cv_rect(8, y - 2, 224, 14, Q_WOOD);
            q_disc(18, y + 5, 5, on ? Q_HI : Q_OFFHI);                           /* the radio button: filled = the pick */
            if (!on) q_disc(18, y + 5, 3, Q_BG);
            q_text(30, y, Q_FLT_NAME[i], on ? Q_TEXT : Q_DIM);
            if (i == q_filter) q_text(128, y, "(kept)", Q_HI);
            for (k = 0; k < nt; k++) cv_rect(200 + (int32_t)k * 6, y + 1, 5, 8, i == 0u ? (k == 3u ? Q_FLT[8].t[2] : QPAL_BASE[k * 4 + 1]) : f->n ? f->t[k] : QPAL_BASE[2 + k * 4]);   /* its tones */
        }
        cv_blit(0, 82);
        cv_begin(240, 32, Q_BG);
        q_text(6, 0, "presets: look at it", Q_DIM), q_text(130, 0, "oct+ / sel: keep", Q_DIM);
        q_text(6, 16, "oct-: back (undoes the preview)", Q_DIM);
        cv_blit(0, 208);
    }
    if (q_scene_budget(240 * 60 * 2)) { q_scene_menu(); q_scene_flush(); }       /* the preview: the inn scene in the filter */
    qb.fresh = 0;
}

/* ---- KEYBOARD LED (menu): the effect a key press makes on the LEDs, one at a time; the knobs set its speed and its fade ---- */
static UI_SMALL void quest_led_draw(void)
{
    static const char *const FXN[6] = {"Fade", "Waves", "Explosion", "Rain", "Comet", "Fireworks"};          /* by the effect's own number */
    static const char *const FXD[6] = {"the light fades out", "waves go to both sides", "a small burst", "drops fall on the diagonal", "a light shoots right", "a rocket climbs and bursts"};
    uint32_t i, sig = 77u + led_fx * 131u + led_sp * 17u + led_fd * 1009u + led_hold * 4001u;
    q_tick_clocks();
    q_enter(QSN_WEATHER);
    if (q_changed(0, 1u)) {
        cv_begin(240, 19, Q_BG);
        q_title(6, "Keyboard LED", Q_HI);
        q_bar_end();
    }
    if (q_changed(1, sig)) {
        static char v[4][8];
        static const char *const lab[4] = {"speed", "fade", "mode", ""};
        const char *val[4];
        int32_t ratio[4];
        cv_begin(240, 124, Q_BG);
        for (i = 0; i < 6u; i++) {
            int32_t y = 2 + (int32_t)i * 18, on = LED_ORDER[i] == led_fx;
            if (on) cv_rect(6, y - 1, 228, 18, Q_WOOD);
            q_disc(18, y + 8, 5, on ? Q_HI : Q_OFFHI);                 /* the radio button */
            if (!on) q_disc(18, y + 8, 3, Q_BG);
            q_text(32, y, FXN[LED_ORDER[i]], on ? Q_TEXT : Q_DIM);
        }
        cv_blit(0, 22);
        fmt_int(v[0], (int32_t)led_sp), fmt_int(v[1], (int32_t)led_fd);
        ratio[0] = (int32_t)led_sp * 1000 / 15, ratio[1] = (int32_t)led_fd * 1000 / 15, ratio[2] = led_hold ? 1000 : 0, ratio[3] = 0;
        val[0] = v[0], val[1] = v[1], val[2] = led_hold ? "hold" : "once", val[3] = "";
        q_dials(lab, val, ratio, 12, 99u + sig);
        cv_begin(240, 40, Q_BG);
        q_text(6, 0, FXD[led_fx % 6u], Q_HI);
        q_text(6, 18, "presets: effect  knob 3: once/hold", Q_DIM);
        cv_blit(0, 146);
    }
    qb.fresh = 0;
}

/* ---- the IMPORTER (menu): the 3 sample slots and the 12 songs, loaded from the web editor ---- */
static UI_SMALL void imp_name(char *d, const char *src, uint32_t max)   /* a header's name: not 0-terminated when full */
{
    uint32_t j;
    for (j = 0; j < max && src[j] >= 32 && src[j] < 127; j++)
        d[j] = src[j];
    d[j] = 0;
}
#define IMP_ROWS 11u
static UI_SMALL uint32_t imp_top(void) { return imp_sel < 5u ? 0u : imp_sel + 6u > IMP_N ? IMP_N - IMP_ROWS : imp_sel - 5u; }
static UI_SMALL uint32_t imp_sig(uint32_t i)
{
    uint32_t h = (i == imp_sel ? 3u : 1u) * 977u, j;
    if (i < SMP_USER_SLOTS) {
        const smp_user_hdr_t *u = (const smp_user_hdr_t *)smp_user_xip(i);
        if (usr_nz[i]) {
            h += 5u + u->data_len * 7u + usr_nz[i] * 131u;
            for (j = 0; j < 8u; j++) h = h * 31u + (uint8_t)u->name[j];
        }
    } else if (sng_ok[i - SMP_USER_SLOTS]) {
        const sng_hdr_t *g = sng_hdr(i - SMP_USER_SLOTS);
        h += 9u + g->notes * 11u + g->bpm * 131u;
        for (j = 0; j < 12u; j++) h = h * 31u + (uint8_t)g->name[j];
    }
    return h;
}
static UI_SMALL void quest_importer_draw(void)
{
    uint32_t i, r, kib = 0, nsng = 0, prog = imp_prog(), top = imp_top();
    q_tick_clocks();
    q_enter(QSN_IMP);
    if (q_changed(15, 1u + q_msg_sig())) {
        cv_begin(240, 19, Q_BG);
        q_title(6, "Importer", Q_HI);
        q_text(6 + text_w(&FONT_QT, "Importer") + 8, 4, "samples and songs", Q_DIM);
        q_bar_end();
    }
    for (r = 0; r < IMP_ROWS; r++) {
        i = top + r;
        if (!q_changed(r, imp_sig(i) + top * 7919u)) continue;
        {
            char lab[12], nm[14] = {0}, num[12], val[20] = {0};
            int sel = i == imp_sel, used = imp_used(i);
            uint32_t n;
            if (i < SMP_USER_SLOTS) {
                const smp_user_hdr_t *u = (const smp_user_hdr_t *)smp_user_xip(i);
                str_cpy(lab, "USR1", sizeof lab);
                lab[3] = (char)('1' + i);
                if (used) {
                    imp_name(nm, u->name, 8u);
                    fmt_int(num, (int32_t)usr_nz[i]);
                    str_cpy(val, num, sizeof val), n = str_len(val), str_cpy(val + n, "z ", sizeof val - n), n = str_len(val);
                    fmt_int(num, (int32_t)((u->data_len + 1023u) / 1024u));
                    str_cpy(val + n, num, sizeof val - n), n = str_len(val), str_cpy(val + n, "K", sizeof val - n);
                }
            } else {
                uint32_t sg = i - SMP_USER_SLOTS;
                const sng_hdr_t *g = sng_hdr(sg);
                str_cpy(lab, sg < SNG_PER ? "PIANO 1" : "HERO 1", sizeof lab);
                lab[str_len(lab) - 1u] = (char)('1' + sg % SNG_PER);
                if (used) {
                    imp_name(nm, g->name, 12u);
                    fmt_int(num, (int32_t)g->notes);
                    str_cpy(val, num, sizeof val), n = str_len(val), str_cpy(val + n, "n ", sizeof val - n), n = str_len(val);
                    fmt_int(num, (int32_t)g->bpm);
                    str_cpy(val + n, num, sizeof val - n);
                }
            }
            cv_begin(240, 16, Q_BG);
            if (sel) { cv_rect(4, 0, 232, 16, Q_WOOD); q_tri(8, 4, Q_HI); }
            q_text(18, 0, lab, sel ? Q_TEXT : Q_DIM);
            q_text(80, 0, used ? nm : "-", used ? (sel ? Q_TEXT : Q_PDIM) : Q_DIM);
            if (used) q_text_r(232, 0, val, sel ? Q_HI : Q_PDIM);
            cv_blit(0, 21u + r * 16u);
        }
    }
    for (i = 0; i < SMP_USER_SLOTS; i++)
        if (usr_nz[i]) kib += (((const smp_user_hdr_t *)smp_user_xip(i))->data_len + 1023u) / 1024u;
    for (i = 0; i < SNG_N; i++) nsng += sng_ok[i];
    if (q_changed(16, kib * 64u + nsng * 4099u + prog * 524287u + (imp_t0 != 0u) * 3u)) {
        char a[24], b[8], n2[8];
        uint32_t n;
        cv_begin(240, 32, Q_BG);
        fmt_int(b, (int32_t)kib);
        str_cpy(a, "samples ", sizeof a), n = str_len(a), str_cpy(a + n, b, sizeof a - n), n = str_len(a), str_cpy(a + n, "/192K", sizeof a - n);
        q_text(6, 0, a, Q_DIM);
        fmt_int(b, (int32_t)nsng), fmt_int(n2, (int32_t)SNG_N);
        str_cpy(a, "songs ", sizeof a), n = str_len(a), str_cpy(a + n, b, sizeof a - n), n = str_len(a), str_cpy(a + n, "/", sizeof a - n), n = str_len(a), str_cpy(a + n, n2, sizeof a - n);
        q_text_r(234, 0, a, Q_DIM);
        if (imp_t0 && prog < 100u) {                         /* the ring of the hold: a bar */
            cv_rect(6, 19, 228, 10, Q_GOLD), cv_rect(7, 20, 226, 8, Q_BG), cv_rect(8, 21, (int32_t)(224u * prog / 100u), 6, Q_REC);
        } else {
            q_text(6, 16, "rec held: erase  oct-: back", Q_DIM);
        }
        cv_blit(0, 208);
    }
    qb.fresh = 0;
}

/* ---- PARTY and STATUS (menu): the four heroes = the four tracks, with the author's portraits ---- */
static const char *const QH_CLASS[4] = {"Elf Wiz.", "ARCHER", "CLERIC", "KNIGHT"};
static UI_SMALL const char *q_eng_name(const track_t *t) { return is_drum(t) ? "DRUMS" : ENGINES[t->eng_req % NENGINES]->name; }
static UI_SMALL uint32_t q_hero_pct(const track_t *t, uint32_t k)   /* the sheet: 0 FOR (how hard), 1 AGI (how busy), 2 INT (how wide), 3 SOR (swing), 4 XP (steps filled) */
{
    uint32_t len = trk_len(t), i, on = 0, sum = 0, lo = 127, hi = 0, lanes = 0;
    for (i = 0; i < len && i < NSTEP; i++) {
        if (is_drum(t)) {
            const dstep_t *d = &t->dstep[i];
            uint32_t l, hit = 0;
            for (l = 0; l < DRUM_LANES; l++)
                if (dstep_has(d, l)) hit++, lanes |= 1u << l, sum += ((d->lvl[l >> 2] >> (2u * (l & 3u))) & 3u) == LV_HARD ? 127u : 90u;
            on += hit != 0u;
            if (hit) lo = 0;
        } else if (step_on(&t->step[i])) {
            uint32_t q;
            on++, sum += t->step[i].vel ? t->step[i].vel : 90u;
            for (q = 0; q < t->step[i].n && q < 4u; q++) {
                if (t->step[i].note[q] < lo) lo = t->step[i].note[q];
                if (t->step[i].note[q] > hi) hi = t->step[i].note[q];
            }
        }
    }
    if (k == 0) return on ? clamp((int32_t)(sum / on) * 100 / 127, 0, 100) : 0u;
    if (k == 1 || k == 4) return clamp((int32_t)(on * 100u / len), 0, 100);
    if (k == 2) {
        uint32_t n = 0;
        if (is_drum(t)) { for (i = 0; i < DRUM_LANES; i++) n += (lanes >> i) & 1u; return n * 100u / 8u > 100u ? 100u : n * 100u / 8u; }
        return on && hi >= lo ? clamp((int32_t)(hi - lo) * 100 / 24, 0, 100) : 0u;
    }
    return clamp(t->p[P_SSWING] * 100 / 127, 0, 100);
}
static UI_SMALL void q_hero_card(uint32_t i, int32_t x, int32_t y, int sel)
{
    const track_t *t = &trk[i];
    int silent = trk_silent(t);
    cv_rect(x - 1, y - 1, 54, 110, sel ? Q_HI : Q_LINE);
    cv_rect(x, y, 52, 108, Q_BG);
    cv_rect(x + 1, y + 1, 50, 11, RGB(0x3a, 0x3a, 0x3e));
    q_text_c(x + 26, y + 1, q_var[i] ? Q_ALT_CLASS[i] : QH_CLASS[i], RGB(0xff, 0xff, 0xff));
    cv_rect(x + 4, y + 14, 44, 62, RGB(0x1d, 0x15, 0x20));                              /* the hero, from the waist up: the figure big, its legs cut by the panel under it */
    { uint32_t cls = i == 2u && q_var[2] ? 4u : i == 3u ? (q_var[3] ? 5u : 3u) : i; qf_scale = 900; q_fig(cls, 0, 0, x + 26, y + 112, 1, 0, -1, 0, 0, 0); qf_scale = 256; }
    cv_rect(x + 1, y + 13, 3, 64, Q_BG), cv_rect(x + 48, y + 13, 4, 64, Q_BG), cv_rect(x + 1, y + 77, 50, 31, Q_BG);          /* (the cut) */
    if (i == 2u || i == 3u) {                                                           /* a class with more heroes: the cards pile up behind, and one pip each */
        uint32_t p;
        cv_rect(x + 52, y + 4, 1, 100, Q_LINE), cv_rect(x + 53, y + 8, 1, 92, Q_OFF);
        for (p = 0; p < 2u; p++) {
            int on = p == (uint32_t)(q_var[i] != 0u);
            uint16_t c = on ? Q_HI : (i == 3u && !q_warrior_ok && p == 1u ? RGB(0x5a, 0x40, 0x2a) : Q_OFFHI);
            cv_rect(x + 20 + (int32_t)p * 8, y + 103, 5, 4, c);
        }
    }
    q_text_c(x + 26, y + 79, q_eng_name(t), Q_TRK[i]);
    cv_rect(x + 6, y + 93, 40, 5, Q_OFF);
    cv_rect(x + 6, y + 93, t->p[P_LEVEL] * 40 / 127, 5, silent ? Q_DIM : Q_TRK[i]);
    if (silent) q_text_c(x + 26, y + 98, "rest", Q_DIM);
    if (silent) cv_rect(x + 1, y + 13, 50, 64, RGB(0x1e, 0x14, 0x18)), cv_rect(x + 1, y + 13, 0, 0, 0);
}
static UI_SMALL void quest_party_draw(void)
{
    uint32_t i;
    q_tick_clocks();
    q_enter(QSN_PTY);
    if (q_changed(15, 1u + q_msg_sig())) {
        cv_begin(240, 19, Q_BG);
        q_title(6, "Party", Q_HI);
        q_text(6 + text_w(&FONT_QT, "Party") + 8, 4, "four heroes, four tracks", Q_DIM);
        q_bar_end();
    }
    for (i = 0; i < NTRK; i++) {
        const track_t *t = &trk[i];
        uint32_t sig = i * 7u + (i == song.sel ? 3u : 0u) + (uint32_t)t->p[P_LEVEL] * 31u + (uint32_t)trk_silent(t) * 977u + t->eng_req * 131u;
        uint32_t fl = pm_flip_t0 && i == pm_flip_slot ? fm1_ms - pm_flip_t0 : 9999u;
        if (fl < 380u) sig += 1u + fl / 25u;
        if (!q_changed(i, sig)) continue;
        cv_begin(54, 110, Q_BG);
        if (fl < 190u && i == pm_flip_slot) { uint8_t keep = q_var[i]; q_var[i] = pm_flip_prev; q_hero_card(i, 1, 1, i == song.sel); q_var[i] = keep; }   /* the old face while it turns away */
        else q_hero_card(i, 1, 1, i == song.sel);
        if (fl < 380u && i == pm_flip_slot) {                                            /* the card turns over: squeezed to the middle and open again */
            int32_t half = fl < 190u ? (int32_t)(190u - fl) : (int32_t)(fl - 190u), wnow = 4 + half * 50 / 190, r, dx;
            uint16_t tmp[54];
            for (r = 0; r < 110; r++) {
                uint16_t *row = cv_px + r * 54;
                for (dx = 0; dx < 54; dx++) tmp[dx] = row[dx], row[dx] = Q_BG;
                for (dx = 0; dx < wnow; dx++) row[27 - wnow / 2 + dx] = tmp[dx * 54 / wnow];
            }
        }
        cv_blit(9 + (int32_t)i * 58, 24);
    }
    if (q_changed(8, (uint32_t)song.sel * 17u + 5u + q_var[2] * 31u + q_var[3] * 61u + stage_sig() * 7u)) {
        const track_t *t = &trk[song.sel];
        char b[12];
        cv_begin(240, 66, Q_BG);
        q_text(10, 0, q_var[song.sel] ? Q_ALT_CLASS[song.sel] : QH_CLASS[song.sel], Q_HI);
        q_text(80, 0, q_eng_name(t), Q_TRK[song.sel]);
        fmt_int(b, (int32_t)song.sel + 1);
        q_text(6, 15, "track", Q_DIM), q_text(70, 15, b, Q_TEXT);
        fmt_int(b, (int32_t)q_hero_pct(t, 4));
        q_text(6, 30, "steps", Q_DIM), q_text(70, 30, b, Q_TEXT), q_text(70 + text_w(&FONT_P, b), 30, "% filled", Q_PDIM);
        {   /* the stage (knobs 1-4): who plays; the next turn is on the bar */
            char sb[40];
            str_cpy(sb, ST_FORM[st_form], sizeof sb);
            str_cpy(sb + str_len(sb), " " , sizeof sb - str_len(sb));
            fmt_int(sb + str_len(sb), (int32_t)st_lead + 1);
            str_cpy(sb + str_len(sb), " / ", sizeof sb - str_len(sb));
            str_cpy(sb + str_len(sb), ST_TURN[st_turn], sizeof sb - str_len(sb));
            str_cpy(sb + str_len(sb), " / ", sizeof sb - str_len(sb));
            str_cpy(sb + str_len(sb), ST_STYLE[st_style], sizeof sb - str_len(sb));
            q_text(6, 45, "stage", Q_DIM), q_text(70, 45, sb, st_form == 3u && !st_turn ? Q_PDIM : Q_HI);
        }
        cv_blit(0, 140);
    }
    if (q_changed(16, 1u + song.sel * 7u + q_var[2] * 31u + q_var[3] * 61u)) {
        cv_begin(240, 36, Q_BG);
        q_text(6, 0, "presets: hero   algorithm: class", Q_DIM);
        if (song.sel == 2u) q_text(6, 16, q_var[2] ? "algorithm: back to cleric" : "algorithm: bard", Q_HI);
        else if (song.sel == 3u) q_text(6, 16, !q_warrior_ok ? "warrior: beat the first boss" : q_var[3] ? "algorithm: back to knight" : "algorithm: red warrior", q_warrior_ok ? Q_HI : Q_DIM);
        else q_text(6, 16, "this class has one hero", Q_PDIM);
        cv_blit(0, 204);
    }
    qb.fresh = 0;
}
static UI_SMALL void quest_status_draw(void)
{
    static const char *const L[4] = {"FOR", "AGI", "INT", "SOR"};
    const track_t *t = &trk[song.sel];
    uint32_t i, sig = (uint32_t)song.sel * 13u, v[5];
    for (i = 0; i < 5u; i++) v[i] = q_hero_pct(t, i), sig = sig * 31u + v[i];
    q_tick_clocks();
    q_enter(QSN_STAT);
    if (q_changed(15, 1u + q_msg_sig())) {
        cv_begin(240, 19, Q_BG);
        q_title(6, "Status", Q_HI);
        q_text(6 + text_w(&FONT_QT, "Status") + 8, 4, q_var[song.sel] ? Q_ALT_CLASS[song.sel] : QH_CLASS[song.sel], Q_DIM);
        q_bar_end();
    }
    if (q_changed(0, sig)) {
        cv_begin(240, 124, Q_BG);
        cv_rect(8, 0, 68, 124, Q_LINE), cv_rect(9, 1, 66, 122, RGB(0x1d, 0x15, 0x20));
        { const qspr_t *sp = &QSPR[Q_HERO[song.sel]]; q_spr(Q_HERO[song.sel], 42 - 3 * (sp->ox + sp->w / 2), 112 - 3 * (sp->oy + sp->h), 3, 0, -1); }
        for (i = 0; i < 4u; i++) {
            char b[8];
            int32_t y = 4 + (int32_t)i * 30;
            q_text(88, y, L[i], Q_TEXT);
            fmt_int(b, (int32_t)v[i]);
            q_text_r(232, y, b, Q_HI);
            cv_rect(88, y + 17, 144, 7, Q_OFF);
            cv_rect(88, y + 17, (int32_t)v[i] * 144 / 100, 7, Q_TRK[song.sel]);
        }
        cv_blit(0, 22);
    }
    if (q_changed(1, sig + 1u)) {
        char b[8];
        cv_begin(240, 66, Q_BG);
        q_text(8, 2, q_eng_name(t), Q_TRK[song.sel]);
        fmt_int(b, (int32_t)song.sel + 1);
        q_text(8, 20, "track", Q_DIM), q_text(70, 20, b, Q_TEXT);
        q_text(8, 38, "xp", Q_DIM);
        cv_rect(40, 43, 190, 6, Q_OFF);
        cv_rect(40, 43, (int32_t)v[4] * 190 / 100, 6, Q_GOLD);
        cv_blit(0, 150);
    }
    if (q_changed(16, 1u)) {
        cv_begin(240, 20, Q_BG);
        q_text(6, 0, "algorithm: hero", Q_DIM), q_text_r(234, 0, "oct-: back", Q_DIM);
        cv_blit(0, 218);
    }
    qb.fresh = 0;
}

/* ---- ABOUT: page 0 = how much room is left in the FM-1 (firmware, RAM, samples, songs), page 1 = the credits ---- */
static UI_SMALL void q_kb(char *d, uint32_t bytes)                 /* 560912 -> "547.7K" (KiB, one decimal) */
{
    uint32_t t = (bytes * 10u + 512u) / 1024u;
    fmt_int(d, (int32_t)(t / 10u));
    d += str_len(d);
    d[0] = '.', d[1] = (char)('0' + t % 10u), d[2] = 'K', d[3] = 0;
}
static UI_SMALL void q_mem_row(int32_t y, const char *lab, uint32_t used, uint32_t total, uint16_t col, uint32_t kib)
{
    char a[12], b[12], c[12], line[32];
    uint32_t n;
    if (kib) { fmt_int(a, (int32_t)used), str_cpy(a + str_len(a), "K", 3), fmt_int(b, (int32_t)total), str_cpy(b + str_len(b), "K", 3), fmt_int(c, (int32_t)(total - used)), str_cpy(c + str_len(c), "K", 3); }
    else q_kb(a, used), q_kb(b, total), q_kb(c, total > used ? total - used : 0u);
    q_text(10, y, lab, Q_TEXT);
    str_cpy(line, a, sizeof line), n = str_len(line), str_cpy(line + n, " / ", sizeof line - n), n = str_len(line), str_cpy(line + n, b, sizeof line - n);
    q_text_r(230, y, line, Q_HI);
    cv_rect(10, y + 17, 220, 6, Q_OFF);
    cv_rect(10, y + 17, total ? (int32_t)((uint64_t)used * 220u / total) : 0, 6, col);
    q_text(10, y + 21, "free", Q_DIM);
    q_text_r(230, y + 21, c, Q_PDIM);
}
static UI_SMALL void quest_about_draw(void)
{
    uint32_t fw = (uint32_t)((uintptr_t)_etext - FM1_APP_BASE) + (uint32_t)(_rt_end - _rt_start) * 4u + (uint32_t)(_data_end - _data_start) * 4u;
    uint32_t ram = (uint32_t)(_data_end - _data_start + _bss_end - _bss_start) * 4u, pool = (uint32_t)(_pool_end - _pool_start) * 4u, i, kib = 0, ns = 0;
    for (i = 0; i < SMP_USER_SLOTS; i++)
        if (usr_nz[i]) kib += (((const smp_user_hdr_t *)smp_user_xip(i))->data_len + 1023u) / 1024u;
    for (i = 0; i < SNG_N; i++) ns += sng_ok[i];
    q_tick_clocks();
    q_enter(QSN_ABOUT);
    if (q_changed(15, 1u + about_pg + q_msg_sig())) {
        cv_begin(240, 19, Q_BG);
        q_title(6, "About", Q_HI);
        q_text(6 + text_w(&FONT_QT, "About") + 8, 4, about_pg ? "credits" : "memory", Q_DIM);
        q_text_r(234, 4, FELUCCA_VERSION, Q_TRK[2]);
        q_bar_end();
    }
    if (about_pg == 0u) {
        if (q_changed(0, 11u + fw * 3u + ram * 5u + kib * 7u + ns * 131u)) {
            cv_begin(240, 118, Q_BG);
            q_mem_row(0, "Firmware", fw, 0x8DFBCu, Q_TRK[0], 0);
            q_mem_row(38, "RAM", ram, 98304u, Q_TRK[1], 0);
            q_mem_row(76, "Pool", pool, 0x54000u, Q_TRK[2], 0);
            cv_blit(0, 22);
        }
        if (q_changed(1, 13u + kib * 7u + ns * 131u)) {
            cv_begin(240, 62, Q_BG);
            q_mem_row(0, "Samples", kib, 192u, Q_TRK[3], 1);
            { char b[12], t[24]; uint32_t n; fmt_int(b, (int32_t)ns), str_cpy(t, b, sizeof t), n = str_len(t), str_cpy(t + n, " / 12", sizeof t - n); q_text(10, 40, "Songs", Q_TEXT); q_text_r(230, 40, t, Q_HI); }
            cv_blit(0, 140);
        }
    } else {
        if (q_changed(0, 17u)) {
        cv_begin(240, 118, Q_BG);
        q_text(10, 0, "Based on SLOOP 2.6, Felucca", Q_TEXT);
        q_text(10, 16, "r/MVaveFM1 community", Q_TEXT);
        q_text(10, 34, "fm1.quest", Q_HI);
        q_text(10, 50, "GPL-3.0, no warranty", Q_DIM);
        q_text(10, 66, "github.com/hericdk/fm1-quest", Q_DIM);
        q_text(10, 82, "Fonts: Pixelify, Jacquarda", Q_DIM);
        q_text(10, 98, "Samples: Versilian, Sonic Pi", Q_DIM);
        cv_blit(0, 22);
        cv_begin(240, 62, Q_BG);
        q_text(10, 0, "Phase: CrispyZebra (GPL)", Q_DIM);
        q_text(10, 16, "Voice: Klattsch (MIT)", Q_DIM);
        q_text(10, 32, "Goblin sprites: Zneeke", Q_TEXT);
        cv_blit(0, 140);
        }
    }
    if (q_changed(16, 1u + about_pg)) {
        cv_begin(240, 20, Q_BG);
        q_text(6, 0, about_pg ? "presets: memory" : "presets: credits", Q_DIM), q_text_r(234, 0, "oct-: back", Q_DIM);
        cv_blit(0, 218);
    }
    qb.fresh = 0;
}

/* ---- the two edit pages: EQUIP (EDIT 1 / 2: the engine's parameters, presets as items) and WEATHER (the FX page) ---- */
static UI_SMALL void q_icon(const char *kind, int32_t x, int32_t y, uint16_t c)       /* 12 x 12 waveform icon (saw sine square tri noise) */
{
    int32_t px, prev = -1;
    for (px = 1; px <= 10; px++) {
        int32_t yy;
        if (kind[0] == 'a' || kind[0] == 'S') yy = 9 - ((px - 1) % 5) * 3 / 2;                                          /* saw */
        else if (kind[0] == 's' && kind[1] == 'i') yy = 6 - qsin((px - 1) * 64 / 9) * 7 / 254;                       /* sine */
        else if (kind[0] == 's' && kind[1] == 'q') yy = px <= 5 ? 3 : 9;                                              /* square */
        else if (kind[0] == 't') yy = 9 - ((((px - 1) % 6) - 3) < 0 ? 3 - ((px - 1) % 6) : ((px - 1) % 6) - 3) * 2;   /* tri */
        else yy = 2 + (int32_t)(qhash((uint32_t)px * 7u) % 8u);                                                        /* noise */
        cv_rect(x + px, y + yy, 1, 1, c);
        if (prev >= 0) { int32_t a = q_min(prev, yy), b = q_max(prev, yy), k; for (k = a; k <= b; k++) cv_rect(x + px, y + k, 1, 1, c); }
        prev = yy;
    }
}
static UI_SMALL void q_cols(const char *lab[4], const char *val[4], int32_t ratio[4], char (*vb)[16], char (*lb)[10])
{
    uint32_t c;
    for (c = 0; c < 4u; c++) {
        int16_t *vp;
        const param_desc_t *d = page_desc(cur_page(), c, &vp);
        const char *unit;
        char u[12];
        lab[c] = lb[c], val[c] = vb[c], ratio[c] = 0, lb[c][0] = 0, vb[c][0] = 0;
        if (!d || !d->label || d->label[0] == '-') continue;
        param_format(d, *vp, vb[c], &unit);
        if (unit && unit[0]) { str_cpy(u, unit, sizeof u); str_cpy(vb[c] + str_len(vb[c]), u, 8); }
        te_lower(lb[c], d->label, 10);
        ratio[c] = d->fmt == F_ENUM && d->max < 2 ? 0 : RATIO(d, *vp);
    }
}
/* ---- WAVE (EDIT, SAMPLE engine only; the mini sample editor from No Man Sky FM / winampFM): the wave of a ½ sampler slot (USR1-4), IN and OUT as orange lines
 * (what is outside them is a mesh), the loop green, the play head white; KNOBS IN, OUT, LOOP, TUNE ---- */
static UI_SMALL void q_wtime(char *b, uint32_t pos, uint32_t rate)       /* a place in the slot as seconds, 1.25 */
{
    uint32_t hz = (rate * 44100u) >> 16, cs = hz ? pos * 100u / hz : 0;
    fmt_int(b, (int32_t)(cs / 100u));
    b += str_len(b);
    b[0] = '.', b[1] = (char)('0' + cs / 10u % 10u), b[2] = (char)('0' + cs % 10u), b[3] = 0;
}
static UI_SMALL void quest_wave_draw(void)
{
    const uint16_t OR = RGB(0xff, 0x5a, 0x1f);
    uint32_t si = (uint32_t)TSEL->p[P_E0] % SMP_NALL, k = si >= SMP_NSETS ? si - SMP_NSETS : 9u, c, sig, n = 0, rate = 0;
    int32_t in = TSEL->p[P_E5], out = TSEL->p[P_E7], x, ix, ox, head = -1, mx = 1;
    int ok = k < SMP_USER_SLOTS && usr_nz[k] == 1u && usr_grid[k][0];
    char lb[4][10] = {{0}}, vb[4][16] = {{0}}, pn[16], b[24];
    const char *lab[4], *val[4];
    int32_t ratio[4];
    q_tick_clocks();
    q_enter(QSN_EQUIP);
    q_cols(lab, val, ratio, vb, lb);
    if (ok) {
        n = usr_zone[k][0].n, rate = usr_zone[k][0].rate;
        for (c = 0; c < NVOICE; c++)                              /* the play head: a voice on this slot */
            if (TSEL->v[c].active && (uint32_t)TSEL->v[c].s[4] >= 0x8000u && (((uint32_t)TSEL->v[c].s[4] >> 5) & 3u) == k)
                head = (int32_t)(TSEL->v[c].ph[0] * 240u / n);
        { q_wtime(vb[0], ((uint32_t)in * n) >> 7, rate); q_wtime(vb[1], n - (n >> 7) * (uint32_t)out, rate); }
    }
    sig = studio_hash((uint32_t)in * 7u + (uint32_t)out * 131u + (uint32_t)TSEL->p[P_E3] * 977u + (uint32_t)TSEL->p[P_E1] * 31u + si * 13u + (uint32_t)(head + 1) * 3u + song.sel * 7919u + (ui.hot_t ? ui.hot_col + 1u : 0u) * 4001u, ok ? "wave" : "none");
    if (q_changed(20, 1u + song.sel * 7u + q_msg_sig())) {
        cv_begin(240, 19, Q_BG);
        q_title(6, "Wave", Q_HI);
        trk_short_name(song.sel, pn);
        b[0] = (char)('1' + song.sel), b[1] = ' ', b[2] = 0, str_cpy(b + 2, pn, 18);
        q_text(6 + text_w(&FONT_QT, "Wave") + 8, 4, b, Q_TEXT);
        q_bar_end();
    }
    if (q_changed(21, sig)) {
        cv_begin(240, 120, Q_BG);
        if (!ok) {
            const char *t = k < SMP_USER_SLOTS ? "empty slot" : "set: USR1 - USR4";
            q_text_c(120, 52, t, Q_DIM);
        } else {
            ix = in * 240 / 128, ox = 239 - out * 240 / 128;
            for (x = 0; x < 240; x++)
                if (usr_peak[k][x] > mx) mx = usr_peak[k][x];
            for (x = 0; x < 240; x++) {                           /* the wave in the hero's colour; outside IN..OUT a mesh */
                int32_t a = usr_peak[k][x] * 52 / mx, y;
                if (x >= ix && x <= ox) cv_rect(x, 58 - a, 1, 2 * a + 1, Q_TRK[song.sel & 3u]);
                else for (y = 58 - a + ((x ^ a) & 1); y <= 58 + a; y += 2) cv_rect(x, y, 1, 1, Q_OFFHI);
            }
            cv_rect(0, 58, 240, 1, Q_OFFHI);
            if (TSEL->p[P_E3] && (in || out))                     /* LOOP (trimmed): a green bracket under IN..OUT */
                cv_rect(ix, 114, ox - ix + 1, 3, RGB(0x7a, 0xb0, 0x4e)), cv_rect(ix, 108, 2, 9, RGB(0x7a, 0xb0, 0x4e)), cv_rect(ox - 1, 108, 2, 9, RGB(0x7a, 0xb0, 0x4e));
            cv_rect(ix > 0 ? ix - 1 : 0, 0, 2, 120, OR);
            cv_rect(ox, 0, 2, 120, OR);
            if (head >= 0 && head < 240) cv_rect(head, 0, 1, 120, Q_TEXT);
        }
        cv_blit(0, 24);
    }
    q_dials(lab, val, ratio, 22, sig);
    qb.fresh = 0;
}
static UI_SMALL void quest_equip_draw(void)
{
    static const char *const ICON[8] = {"saw", "sine", "square", "sine", "saw", "tri", "square", "noise"};
    uint32_t i, sel = song.sel, total, cur = preset_pos(&total), first = cur / 12u * 12u, sig;
    char lb[4][10] = {{0}}, vb[4][16] = {{0}}, pn[16], b[24];
    const char *lab[4], *val[4];
    int32_t ratio[4];
    q_tick_clocks();
    q_enter(QSN_EQUIP);
    q_cols(lab, val, ratio, vb, lb);
    sig = cur * 131u + sel * 17u + (uint32_t)trk[sel].p[P_LEVEL] + ui.page * 7u + q_msg_sig();
    for (i = 0; i < 4u; i++) sig = studio_hash(sig * 7u + (uint32_t)ratio[i], vb[i]);
    if (q_changed(0, sig)) {
        cv_begin(240, 19, Q_BG);
        q_title(6, "Equip", Q_HI);
        trk_short_name(sel, pn);
        b[0] = (char)('1' + sel), b[1] = ' ', b[2] = 0, str_cpy(b + 2, pn, 18);
        q_text(6 + text_w(&FONT_QT, "Equip") + 8, 4, b, Q_TEXT);
        fmt_int(b, (int32_t)cur + 1), str_cpy(b + str_len(b), "/", 2), fmt_int(b + str_len(b), (int32_t)total);
        { char no[28]; str_cpy(no, "No. ", sizeof no); str_cpy(no + 4, b, 20); q_text_r(234, 4, no, Q_DIM); }
        q_bar_end();
    }
    if (q_changed(1, studio_hash(cur * 131u + sel * 17u + (uint32_t)trk[sel].p[P_LEVEL], "equip-list"))) {          /* the item grid and the info panel */
        const engine_t *e = ENGINES[trk[sel].eng_req % NENGINES];
        cv_begin(240, 84, Q_BG);
        for (i = 0; i < 12u; i++) {
            uint32_t idx = first + i, kind = idx < NBANK ? BANK[idx].kind : 8u;
            int32_t x = 98 + (int32_t)(i % 4u) * 34, y = (int32_t)(i / 4u) * 28;
            int on = idx == cur;
            char nm[16], *sp;
            if (idx >= total) { cv_rect(x, y, 32, 26, Q_PAD); continue; }
            if (idx < NBANK) str_cpy(nm, BANK[idx].name, sizeof nm);
            else { uint32_t k; preset_at(idx, &k); up_name(k, nm); }
            if ((sp = nm) && 1) { uint32_t n; for (n = 0; nm[n]; n++) if (nm[n] == ' ') { nm[n] = 0; break; } }       /* the first word; the full name is in the panel */
            while (nm[0] && text_w(&FONT_P, nm) > 30) nm[str_len(nm) - 1u] = 0;
            cv_rect(x, y, 32, 26, Q_PADC);
            cv_rect(x + 1, y + 1, 30, 24, on ? Q_HI : Q_PAD);
            q_icon(ICON[kind & 7u], x + 10, y, on ? Q_HITXT : Q_TRK[i % 4u]);
            q_text_c(x + 16, y + 10, nm, on ? Q_HITXT : Q_TEXT);
            (void)sp;
        }
        cv_blit(0, 24);
        cv_begin(240, 54, Q_BG);
        q_panel(98, 0, 136, 52);
        {
            char pname[16], line[40];
            if (user_of(&trk[sel]) < UP_SLOTS) up_name(user_of(&trk[sel]), pname);
            else if (e->npresets) str_cpy(pname, e->presets[trk[sel].preset % e->npresets].name, sizeof pname);
            else pname[0] = 0;
            te_lower(pname, pname, sizeof pname);
            if (pname[0] >= 'a' && pname[0] <= 'z') pname[0] = (char)(pname[0] - 32);
            while (pname[0] && text_w(&FONT_QT, pname) > 126) pname[str_len(pname) - 1u] = 0;
            cv_text(104, 2, &FONT_QT, pname, Q_HI);
            te_lower(line, e->name, 8);
            str_cpy(line + str_len(line), " / ", 4);
            te_lower(line + str_len(line), preset_kind(cur), 6);
            q_text(104, 20, line, Q_PDIM);
            q_text(104, 34, "presets: browse", Q_TEXT);
        }
        cv_blit(0, 110);
    }
    if (q_changed(2, studio_hash(sel * 13u + (uint32_t)trk[sel].p[P_LEVEL], "equip-hero"))) {                        /* the hero panel (static part) */
        static const char *const HN[4] = {"MAGE", "ARCHER", "CLERIC", "WARRIOR"};
        char lv[16];
        cv_begin(86, 140, Q_BG);
        q_panel(0, 0, 86, 140);
        for (i = 0; i < 4u; i++) q_portrait(4 + (int32_t)i * 20, 4, i, i == sel ? Q_HI : Q_PADC, 0);
        q_text(6, 122, HN[sel], Q_TEXT);
        fmt_int(lv, (int32_t)((sel == TRK_DRUM ? song.g[G_DRLVL] : trk[sel].p[P_LEVEL]) * 100 / 127));
        q_text_r(80, 122, lv, Q_PDIM);
        cv_blit(6, 24);
    }
    if (q_scene_budget(72 * 96 * 2)) q_scene_equip(13, 46, sel);          /* the hero on the pedestal: ~16 fps while playing, slow otherwise */
    q_dials(lab, val, ratio, 12, 5u);
    qb.fresh = 0;
}
/* a stock screen takes over (a page without a Quest screen, the song screen, a hold, ABOUT): wipe ours */
static UI_SMALL void quest_leave(void)
{
    if (qs.screen) {
        qs.screen = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
}
