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
       QSN_WEATHER, QSN_EQUIP, QSN_INN };
#define Q_SPI_BPMS 1500                     /* LCD SPI at 12 MHz: bytes per millisecond */
static struct { int32_t tokens; uint32_t tok_ms; uint8_t fresh; } qb;

static void q_enter(uint8_t id)
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
static int q_changed(uint32_t slot, uint32_t sig)       /* the element's signature: draw it again when it moved */
{
    if (!Q_FULL && qs.sig[slot] == sig) return 0;
    qs.sig[slot] = sig;
    return 1;
}
static int q_scene_budget(int32_t cost)                 /* the scene, paced: 1 = draw it now */
{
    uint32_t now = fm1_ms, dt = qb.tok_ms ? now - qb.tok_ms : 0u, share = song.playing ? 55u : 12u, period = song.playing ? 62u : 250u;
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
static void q_clocks_and_overlay_done(void) { qb.fresh = 0; }

/* ---- the bar ---- */
static void q_bar_end(void)
{
    if (ui.msg_t) {                                     /* a message takes the right half of the bar (the firmware's head does the same) */
        cv_rect(100, 0, 140, 18, Q_BG);
        q_text(104, 4, ui.msg, Q_HI);
    }
    cv_rect(0, 18, 240, 1, Q_LINE);
    cv_blit(0, 0);
}
static uint32_t q_msg_sig(void) { return ui.msg_t ? str_hash(7u, ui.msg) : 0u; }
static void q_bar_right_play(void)                       /* play triangle + bpm on the right */
{
    char b[8];
    fmt_int(b, song.g[G_BPM]);
    q_text_r(234, 4, b, ui.bpm_t ? Q_HI : Q_TEXT);
    if (song.playing) q_tri(234 - text_w(&FONT_Q, b) - 10, 6, Q_TEXT);
}
static void q_bar_bpm_pos(int rec_pips)                  /* the status bar of PARTY / BOSS: bpm, play, bar.beat, the four beats */
{
    char b[16];
    uint32_t playing = song.playing, beat = clk_beat, k;
    fmt_int(b, song.g[G_BPM]);
    k = (uint32_t)cv_text(6, 4, &FONT_Q, b, ui.bpm_t ? Q_HI : Q_TEXT);
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
static void q_dials(const char *const lab[4], const char *const val[4], const int32_t ratio[4], uint32_t slot, uint32_t sigbase)
{
    uint32_t k, sig = sigbase;
    for (k = 0; k < 4u; k++) sig = studio_hash(sig * 7u + (uint32_t)ratio[k] + (ui.hot_t && ui.hot_col == k) * 5003u, lab[k]), sig = studio_hash(sig, val[k]);
    if (!q_changed(slot, sig)) return;
    cv_begin(240, 44, Q_BG);
    for (k = 0; k < 4u; k++) {
        int32_t cx = 30 + 60 * (int32_t)k;
        if (!lab[k][0]) continue;
        q_knob(cx, 10, ratio[k], Q_TRK[k]);
        q_text_c(cx, 21, lab[k], Q_DIM);
        q_text_c(cx, 32, val[k], ui.hot_t && ui.hot_col == k ? Q_HI : Q_TEXT);
    }
    cv_blit(0, 194);
}

/* ---- a tile of a layer screen as the firmware computed it (colours are its TE_* constants): looked up by value ---- */
typedef struct { uint16_t bg, fg, top; } q_look_t;
static q_look_t q_tile_look(const tile_t *t)
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
static void q_pad(int32_t x, int32_t y, int32_t w, int32_t h, const char *lab, uint16_t bg, uint16_t fg, uint16_t bd, uint16_t top, int has_top)
{
    int32_t tw;
    cv_rect(x, y, w, h, bd);
    cv_rect(x + 1, y + 1, w - 2, h - 2, bg);
    if (has_top) cv_rect(x, y, w, 2, top);
    tw = text_w(&FONT_Q, lab);
    if (tw > w - 2) {                                       /* too long for the pad: cut it */
        char b[12];
        str_cpy(b, lab, sizeof b);
        while (b[0] && text_w(&FONT_Q, b) > w - 3) b[str_len(b) - 1u] = 0;
        cv_text(x + (w - text_w(&FONT_Q, b)) / 2, y + (h - 11) / 2 + 1, &FONT_Q, b, fg);
    } else cv_text(x + (w - tw) / 2, y + (h - 11) / 2 + 1, &FONT_Q, lab, fg);
}
static uint32_t q_tiles_sig(const tile_t *tl, uint32_t n)
{
    uint32_t i, sig = 7u;
    for (i = 0; i < n; i++) sig = studio_hash(sig * 31u + tl[i].bg * 3u + tl[i].fg * 5u + tl[i].top * 7u + tl[i].marks, tl[i].lab);
    return sig;
}

/* ---- PARTY ---- */
static void q_portrait(int32_t x, int32_t y, uint32_t t, uint16_t border, int dead)
{
    cv_rect(x, y, 17, 17, border);
    cv_rect(x + 1, y + 1, 15, 15, Q_OFFHI);
    q_spr(Q_HERO[t], x + 1 - 1, y + 1 + (t == 0 ? 6 : 3), 1, 0, -1);        /* the design: figure at (-1, 3), the mage at (-1, 6) */
    if (dead) {                                                              /* a resting / silent hero: dimmed */
        int32_t gy, gx;
        for (gy = 1; gy < 16; gy++)
            for (gx = 1 + (gy & 1); gx < 16; gx += 2) cv_pset(x + gx, y + gy, Q_BG);
    }
}
static void q_row(uint32_t i, int32_t y)
{
    track_t *t = &trk[i];
    char nm[16], b[12];
    uint32_t selected = song.sel == i, len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), pos = t->seq_idx % len;
    uint32_t level = i == TRK_DRUM ? (uint32_t)song.g[G_DRLVL] : (uint32_t)t->p[P_LEVEL];
    uint32_t silent = trk_silent(t) || !level, rec = (song.rec >> i) & 1u, solo = (song.solo >> i) & 1u, bank = song.playing ? pos / 16u : 0u, h, j;
    int32_t x;
    const char *tag = rec ? "rec" : solo ? "duel" : silent ? "rest" : Q_HERO_NAME[i];
    if (i == TRK_DRUM) str_cpy(nm, DRUM_KIT_NAMES[drum_kit()], sizeof nm);
    else trk_short_name(i, nm);
    nm[11] = 0;
    h = studio_hash(selected + level * 7u + silent * 997u + rec * 1999u + solo * 4999u + len * 37u + song.playing * 7u + pos * 71u + bank * 13u, nm);
    for (j = 0; j < len; j++) h = h * 31u + (uint32_t)trk_step_on(t, j);
    if (!q_changed(4u + i, h)) return;
    cv_begin(240, 18, Q_BG);
    q_portrait(6, 0, i, selected ? Q_HI : Q_TRK[i], (int)silent);
    b[0] = (char)('1' + i), b[1] = 0;
    q_text(28, -1, b, Q_DIM);
    x = cv_text(36, -1, &FONT_Q, nm, selected ? Q_TEXT : Q_DIM);
    q_text(x + 6, -1, tag, rec ? Q_REC : Q_TRK[i]);
    for (j = 0; j < 16u; j++) {                          /* the 16 steps in view (the firmware's own folding rules) */
        uint32_t p = bank * 16u + j, on = 0;
        uint16_t c;
        if (len <= 16u) { if (p < len) on = trk_step_on(t, p) ? 2u : 1u; }
        else if (song.playing) { if (p < len) on = trk_step_on(t, p) ? 2u : 1u; }
        else { uint32_t a = j * len / 16u, z = (j + 1u) * len / 16u, k; on = 1; for (k = a; k < z; k++) if (trk_step_on(t, k)) on = 2; }
        c = !on ? RGB(0x0e, 0x0a, 0x0c) : on == 2u ? (silent ? Q_DIM : Q_TRK[i]) : (j % 4u == 0 ? Q_OFFHI : Q_OFF);
        if (song.playing && p == pos) c = Q_TEXT;
        cv_rect(36 + (int32_t)j * 4, 12, 3, 4, c);
    }
    fmt_int(b, (int32_t)(level * 100u / 127u));
    {
        char hp[16];
        str_cpy(hp, "hp ", sizeof hp);
        str_cpy(hp + 3, b, sizeof hp - 3);
        q_text_r(234, -1, hp, Q_DIM);
    }
    cv_rect(198, 12, 36, 4, Q_OFF);
    cv_rect(198, 12, (int32_t)(level * 36u / 127u), 4, Q_TRK[i]);
    cv_blit(0, y);
}
static void q_party_dials(void)      /* swing, level, steps, pan of the selected track (the firmware's own dials) */
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
static void q_party_screen(void)
{
    uint32_t i;
    char b[8];
    (void)b;
    q_enter(QSN_PARTY);
    if (q_changed(0, studio_hash((uint32_t)song.g[G_BPM] * 7u + song.playing * 3u + (song.playing ? (clk_beat % 4u) * 131u + (clk_beat / 4u) * 7919u : 0u) +
                                 (song.rec != 0) * 1999u + q_msg_sig() + (ui.bpm_t != 0) * 31u, "party"))) {
        cv_begin(240, 19, Q_BG);
        q_bar_bpm_pos(0);
        q_title_r(236, "Party", Q_HI);
        q_bar_end();
    }
    for (i = 0; i < NTRK; i++) q_row(i, 108 + (int32_t)i * 18);
    q_party_dials();
    if (q_scene_budget(240 * 84 * 2)) { q_scene_world(QSC_PARTY); q_scene_flush(); }
}

/* ---- BOSS: REC armed on a playing loop (README: recording a loop is a fight) or a free take ---- */
static void q_title_outline(int32_t x, const char *s)
{
    cv_text(x - 1, -1 + 3, &FONT_QT, s, Q_BG), cv_text(x + 1, -1 + 3, &FONT_QT, s, Q_BG), cv_text(x, -2 + 3, &FONT_QT, s, Q_BG), cv_text(x, 3, &FONT_QT, s, Q_BG);
    cv_text(x, 2, &FONT_QT, s, Q_TEXT);
}
static void q_boss_screen(void)
{
    static const char *const CM[4] = {"FIGHT", "SPELL", "ITEM", "RUN"}, *const CS[4] = {"play", "punch", "kit", "stop"};
    uint32_t i, rt = 0, take = ft_on, bars = 0, bpm = 0;
    int32_t hp100;
    q_enter(QSN_BOSS);
    if (take) { bars = ft_fit(ft_t, &bpm); rt = ft_trk % NTRK; }
    else for (i = 0; i < NTRK; i++) if ((song.rec >> i) & 1u) { rt = i; break; }
    if (take) { uint32_t secs_q8 = ft_t * CTL * 256u / FS, tot = bars ? bars * 4u * 60u * 256u / (bpm ? bpm : 1u) : 0u; hp100 = tot ? (int32_t)q_max(0, 100 - (int32_t)(secs_q8 * 100u / tot)) : 100; }
    else { uint32_t len = (uint32_t)clamp(trk[rt].p[P_SLEN], 1, NSTEP); hp100 = 100 - (int32_t)((trk[rt].seq_idx % len) * 100u / len); }
    if (q_changed(0, studio_hash((uint32_t)song.g[G_BPM] * 7u + (clk_beat % 4u) * 131u + (clk_beat / 4u) * 7919u + q_msg_sig(), "boss"))) {
        cv_begin(240, 19, Q_BG);
        q_bar_bpm_pos(1);
        q_text_r(234, 4, "rec", Q_REC);
        q_bar_end();
    }
    if (q_changed(1, (uint32_t)(hp100 * 7 + rt * 977u))) {                         /* the party's HP list and the commands */
        cv_begin(240, 52, Q_BG);
        q_panel(6, 0, 100, 52);
        for (i = 0; i < 4u; i++) {
            int32_t y = 4 + (int32_t)i * 11;
            if (i == 0) cv_rect(8, y, 96, 11, Q_HI);
            q_text(12, y - 1, CM[i], i == 0 ? Q_HITXT : Q_TEXT);
            q_text_r(100, y - 1, CS[i], i == 0 ? Q_HITXT : Q_PDIM);
        }
        for (i = 0; i < 4u; i++) {
            int32_t y = 2 + (int32_t)i * 12;
            char nm[16];
            uint32_t lvl = i == TRK_DRUM ? (uint32_t)song.g[G_DRLVL] : (uint32_t)trk[i].p[P_LEVEL];
            if (i == TRK_DRUM) str_cpy(nm, DRUM_KIT_NAMES[drum_kit()], sizeof nm); else trk_short_name(i, nm);
            nm[9] = 0;
            q_text(116, y - 1, nm, i == rt ? Q_TEXT : Q_DIM);
            cv_rect(190, y + 3, 44, 4, Q_OFF), cv_rect(190, y + 3, (int32_t)(lvl * 44u / 127u), 4, Q_TRK[i]);
        }
        cv_blit(0, 108);
    }
    q_party_dials();
    if (q_scene_budget(240 * 84 * 2)) {
        char b[16];
        int32_t nx;
        q_scene_boss();
        nx = text_w(&FONT_QT, "King Slime") + 5;
        q_title_outline(5, "King Slime");
        (void)b;
        cv_rect(5 + nx, 4, 234 - 5 - nx, 8, Q_TEXT), cv_rect(6 + nx, 5, 232 - 5 - nx, 6, Q_BG);
        cv_rect(6 + nx, 5, (232 - 5 - nx) * hp100 / 100, 6, Q_REC);
        q_scene_flush();
    }
}

/* ---- DUEL: a soloed track (README: solo = a 1-on-1 duel) ---- */
static void q_duel_screen(void)
{
    uint32_t i = 0, k, len, pos, pg, pages, j;
    char b[16], v[4][12];
    const char *lab[4], *val[4] = {v[0], v[1], v[2], v[3]};
    int32_t ratio[4];
    q_enter(QSN_DUEL);
    for (k = 0; k < NTRK; k++) if ((song.solo >> k) & 1u) { i = k; break; }
    len = (uint32_t)clamp(trk[i].p[P_SLEN], 1, NSTEP), pos = trk[i].seq_idx % len, pages = (len + 15u) / 16u;
    pg = song.playing ? pos / 16u : ui.step_page;
    if (pg >= pages) pg = 0;
    if (song.playing && trk[i].seq_idx != qs.combo_step) {                     /* combo: the lit steps that passed */
        qs.combo_step = trk[i].seq_idx;
        if (trk_step_on(&trk[i], trk[i].seq_idx % len)) qs.combo = qs.combo < 99u ? qs.combo + 1u : 99u;
    }
    if (q_changed(0, studio_hash((uint32_t)song.g[G_BPM] * 7u + song.playing * 3u + i + q_msg_sig(), "duel"))) {
        cv_begin(240, 19, Q_BG);
        q_title(6, "Duel", Q_HI);
        str_cpy(b, "solo ", sizeof b), b[5] = (char)('1' + i), b[6] = 0;
        q_text(6 + text_w(&FONT_QT, "Duel") + 8, 4, b, Q_DIM);
        q_bar_right_play();
        q_bar_end();
    }
    if (q_changed(1, studio_hash(i * 977u + pg * 31u + len * 7u + pos * 13u * song.playing + qs.combo * 3571u, "dl"))) {          /* the header and the 16 steps of the page */
        char nm[16];
        cv_begin(240, 36, Q_BG);
        b[0] = (char)('1' + i), b[1] = 0;
        q_text(6, 2, b, Q_DIM);
        if (i == TRK_DRUM) str_cpy(nm, DRUM_KIT_NAMES[drum_kit()], sizeof nm); else trk_short_name(i, nm);
        j = (uint32_t)cv_text(16, 2, &FONT_Q, nm, Q_TEXT);
        q_text((int32_t)j + 6, 2, Q_HERO_NAME[i], Q_TRK[i]);
        fmt_int(b, (int32_t)pg + 1);
        str_cpy(b + str_len(b), "/", 2), fmt_int(b + str_len(b), (int32_t)pages);
        { char pgs[24]; str_cpy(pgs, "page ", sizeof pgs); str_cpy(pgs + 5, b, 16); q_text_r(234, 2, pgs, Q_DIM); }
        for (j = 0; j < 16u; j++) {
            uint32_t p = pg * 16u + j;
            uint16_t c = p >= len ? Q_BG : (song.playing && p == pos) ? Q_TEXT : trk_step_on(&trk[i], p) ? Q_TRK[3] : (j % 4u == 0 ? Q_OFFHI : Q_OFF);
            cv_rect(6 + (int32_t)j * 14, 16, 13, 11, c);
        }
        cv_blit(0, 134);
    }
    lab[0] = i == TRK_DRUM ? "sound" : "note", lab[1] = "div", lab[2] = "swing", lab[3] = "steps";
    if (i == TRK_DRUM) str_cpy(v[0], LANE_SHORT[pen_lane], 8); else note_name(v[0], pen_note[0]);
    str_cpy(v[1], N_DIV[trk[i].p[P_SDIV] % 6], 8);
    swing_str(v[2], trk[i].p[P_SSWING]);
    fmt_int(v[3], trk[i].p[P_SLEN]);
    ratio[0] = i == TRK_DRUM ? (int32_t)pen_lane * 1000 / 15 : (int32_t)pen_note[0] * 1000 / 127, ratio[1] = trk[i].p[P_SDIV] * 200, ratio[2] = trk[i].p[P_SSWING] * 10, ratio[3] = (trk[i].p[P_SLEN] - 1) * 1000 / 63;
    q_dials(lab, val, ratio, 12, i * 13u + 5u);
    if (q_scene_budget(240 * 110 * 2)) {
        char cb[8];
        q_scene_duel(i);
        q_outline_text(6, 3, "combo", Q_TEXT);
        cb[0] = 'x', fmt_int(cb + 1, (int32_t)qs.combo);
        cv_text(6 - 1, 14, &FONT_QT, cb, Q_BG), cv_text(6 + 1, 14, &FONT_QT, cb, Q_BG), cv_text(6, 13, &FONT_QT, cb, Q_BG), cv_text(6, 15, &FONT_QT, cb, Q_BG);
        cv_text(6, 14, &FONT_QT, cb, Q_HI);
        cv_blit(0, 19);
    }
}

/* ---- the tracks screen of SLOOP is PARTY, BOSS or DUEL ---- */
static void quest_tracks_draw(void)
{
    q_tick_clocks();
    if (ft_on || (song.rec && song.playing)) q_boss_screen();
    else if (song.solo) q_duel_screen();
    else q_party_screen();
    qb.fresh = 0;
}

/* ---- REC READY / COUNT-IN (the firmware's rec_screen_draw; a free take is the boss) ---- */
static void q_rec_list(uint32_t rt, int32_t y0, uint32_t slot)
{
    uint32_t i, j, sig = rt * 3u + rec_wait + drum_kit() * 977u;
    for (i = 0; i < NTRK; i++) { sig = sig * 31u + (uint32_t)trk[i].p[P_SLEN]; for (j = 0; j < NSTEP; j++) sig = sig * 3u + (uint32_t)trk_step_on(&trk[i], j); }
    if (!q_changed(slot, sig)) return;
    cv_begin(240, 46, Q_BG);
    for (i = 0; i < NTRK; i++) {
        const track_t *t = &trk[i];
        uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), armed = i == rt;
        int32_t y = (int32_t)i * 11;
        char nm[16];
        if (armed) cv_rect(6, y, 228, 11, Q_REC), cv_rect(7, y + 1, 226, 9, Q_BG);
        cv_rect(10, y + 2, 6, 6, Q_TRK[i]);
        if (i == TRK_DRUM) str_cpy(nm, DRUM_KIT_NAMES[drum_kit()], sizeof nm); else trk_short_name(i, nm);
        nm[10] = 0;
        q_text(22, y - 1, nm, armed ? Q_TEXT : Q_DIM);
        for (j = 0; j < 16u; j++) {
            uint32_t a = j * len / 16u, z = (j + 1u) * len / 16u, k, on = 0;
            if (z == a) z = a + 1u;
            for (k = a; k < z && k < NSTEP; k++) if (trk_step_on(t, k)) on = 1;
            cv_rect(102 + (int32_t)j * 8, y + 3, 6, 4, on ? (armed ? Q_TRK[i] : Q_TRK_DIM[i]) : Q_OFF);
        }
    }
    cv_blit(0, (uint32_t)y0);
}
static void quest_rec_draw(void)
{
    uint32_t count = ci_on, take = ft_on, empty = project_empty(), free_ = empty && !rec_tempo, rt = song.sel, i;
    q_tick_clocks();
    if (take) { q_boss_screen(); qb.fresh = 0; return; }
    q_enter(count ? QSN_COUNT : QSN_REC);
    if (q_changed(0, studio_hash(count * 3u + ci_beat * 7u + (uint32_t)song.g[G_BPM] + q_msg_sig(), "rec"))) {
        cv_begin(240, 19, Q_BG);
        {
            char b[8];
            fmt_int(b, song.g[G_BPM]);
            i = (uint32_t)cv_text(6, 4, &FONT_Q, b, Q_TEXT);
            q_text((int32_t)i + 3, 4, "bpm", Q_DIM);
        }
        cv_rect(66, 6, 6, 6, Q_DIM), q_text(75, 4, "loop", Q_DIM);
        q_text_r(234, 4, count ? "count-in" : "rec ready", Q_REC);
        q_bar_end();
    }
    {   /* the dialog */
        uint32_t m = free_ ? 0u : rec_count ? 2u : 1u;
        if (q_changed(1, count * 5u + m * 17u + ci_beat * 3u)) {
            static const char *const L1[3] = {"play freely: warm up", "play a note: fight", "press play: fight"};
            static const char *const L2[3] = {"then rec on the 1", "it starts the loop", "4 clicks, then rec"};
            cv_begin(240, 36, Q_BG);
            q_panel(6, 0, 228, 34);
            if (count) {
                q_text(12, 4, "Battle in one bar!", Q_REC);
                q_text(12, 15, "4 clicks", Q_TEXT), q_text_r(228, 15, "rec: cancel", Q_PDIM);
            } else {
                q_text(12, 3, "A wild LOOP appears!", Q_REC);
                q_text(12, 14, L1[m], Q_TEXT), q_text_r(228, 14, "rec: flee", Q_PDIM);
                q_text(12, 23, L2[m], Q_PDIM);
            }
            cv_blit(0, 108);
        }
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
    if (q_scene_budget(240 * 84 * 2)) {
        q_scene_rec((int)count);
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
static void quest_drum_draw(void)
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
        cv_rect(6, 3, text_w(&FONT_Q, b) + 8, 13, Q_TRK[3]);
        q_text(10, 3, b, Q_HITXT);
        { char nm[16]; str_cpy(nm, DRUM_KIT_NAMES[kit], sizeof nm); for (j = 1; nm[j]; j++) if (nm[j] >= 'A' && nm[j] <= 'Z') nm[j] = (char)(nm[j] + 32); i = (uint32_t)cv_text(10 + text_w(&FONT_Q, b) + 8, -1, &FONT_QT, nm, Q_HI); }
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
                q_pad(x, y, 56, 17, LANE_SHORT[i], pad_lit[i] ? Q_TRK[3] : Q_PAD, pad_lit[i] ? Q_HITXT : i == drum_lane ? Q_TEXT : Q_DIM, Q_PADC, 0, 0);
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
            str_cpy(v[0], LANE_SHORT[drum_lane], 8); fmt_int(v[1], drum_cursor + 1);
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
static void quest_layer_draw(uint32_t layer, const char *sub, const tile_t *tl, const char *const lab[4], const char *const val[4], const int32_t ratio[4])
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
        q_text(6 + text_w(&FONT_QT, NAME[layer & 7u]) + 8, 4, layer == LY_MIX ? "rest  duel  tap" : sub, Q_DIM);
        if (ly_lock != LY_PLAY) q_text_r(190, 4, "LOCK", Q_TEXT);
        q_bar_right_play();
        q_bar_end();
    }
    pads_y = has_scene ? 108u : 26u, ph = has_scene ? 17u : 27u, pitch = has_scene ? 19u : 30u;
    if (layer == LY_SCALE) ph = 15u, pitch = 17u;
    if (q_changed(1, q_tiles_sig(tl, 16u) + layer * 13u)) {
        cv_begin(240, 4u * pitch, Q_BG);
        if (layer == LY_MIX) {                                           /* camp: rest 1-4, duel 1-4, then tap tempo and the bpm (the unused row is skipped) */
            static const uint8_t ORDER[12] = {0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15};
            for (i = 0; i < 12u; i++) {
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
                if (i < 8u) { str_cpy(nm, "duel 1", sizeof nm), nm[5] = (char)('1' + (i - 4u)); }
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
        cv_begin(240, 13, Q_BG);
        for (i = 0; i < 4u; i++) {
            int32_t x = 6 + (int32_t)i * 58, w;
            int on = trk[0].p[P_SCALE] == LG[i].sc;
            if (on) { cv_rect(x, 0, 56, 13, Q_TRK[1]); cv_rect(x + 1, 1, 54, 11, Q_BG); }
            w = text_w(&FONT_Q, LG[i].k) + 4 + text_w(&FONT_Q, LG[i].v);
            q_text(x + (56 - w) / 2, 0, LG[i].k, on ? Q_TRK[1] : Q_DIM);
            q_text(x + (56 - w) / 2 + text_w(&FONT_Q, LG[i].k) + 4, 0, LG[i].v, Q_DIM);
        }
        cv_blit(0, 178);
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
static void quest_menu_draw(void)
{
    uint32_t i, sig = ui.menu_sel * 131u + settings.palette * 1009u + settings.lowcut * 7919u + settings.zoom * 104729u + lights_lvl * 1299709u + lights_keys * 15485863u + lights_notes * 32452843u + usb_full * 49979687u;
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
        for (i = 0; i < MI_COUNT; i++) {
            int32_t y = 5 + (int32_t)i * 11;
            int sel = i == ui.menu_sel;
            const char *val = "";
            if (i == MI_LOWCUT) val = settings.lowcut ? "ON" : "OFF"; else if (i == MI_ZOOM) val = settings.zoom ? "ON" : "OFF"; else if (i == MI_NOTES) val = lights_notes ? "ON" : "OFF";
            else if (i == MI_LIGHTS) val = LIGHTS_NAME[lights_lvl % LIGHTS_N]; else if (i == MI_USB) val = usb_full ? "FULL" : "MASTER";
            else if (i == MI_KEYS) val = KEYS_NAME[lights_keys % KEYS_N]; else if (i == MI_COLOR) val = PALETTES[settings.palette].name;
            if (sel) q_tri(12, y + 3, Q_TEXT);
            q_text(22, y - 1, MI_NAME[i], sel ? Q_TEXT : Q_DIM);
            q_text(128, y - 1, val, sel ? Q_TEXT : Q_PDIM);
        }
        cv_blit(0, 82);
        cv_begin(240, 26, Q_BG);
        q_text(6, 0, "presets: move", Q_DIM), q_text(120, 0, "knob 1: set", Q_DIM);
        q_text(6, 11, "oct+: ok", Q_DIM), q_text(120, 11, "oct-: back", Q_DIM);
        cv_blit(0, 208);
    }
    if (q_scene_budget(240 * 60 * 2)) { q_scene_menu(); q_scene_flush(); }
    qb.fresh = 0;
}

/* ---- the two edit pages: EQUIP (EDIT 1 / 2: the engine's parameters, presets as items) and WEATHER (the FX page) ---- */
static void q_icon(const char *kind, int32_t x, int32_t y, uint16_t c)       /* 12 x 12 waveform icon (saw sine square tri noise) */
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
static void q_cols(const char *lab[4], const char *val[4], int32_t ratio[4], char (*vb)[16], char (*lb)[10])
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
static void quest_equip_draw(void)
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
            while (nm[0] && text_w(&FONT_Q, nm) > 30) nm[str_len(nm) - 1u] = 0;
            cv_rect(x, y, 32, 26, Q_PADC);
            cv_rect(x + 1, y + 1, 30, 24, on ? Q_HI : Q_PAD);
            q_icon(ICON[kind & 7u], x + 10, y + 1, on ? Q_HITXT : Q_TRK[i % 4u]);
            q_text_c(x + 16, y + 13, nm, on ? Q_HITXT : Q_TEXT);
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
            te_lower(line, e->name, 12);
            str_cpy(line + str_len(line), " / ", 4);
            te_lower(line + str_len(line), preset_kind(cur), 8);
            q_text(104, 24, line, Q_PDIM);
            q_text(104, 37, "presets: browse", Q_TEXT);
        }
        cv_blit(0, 110);
    }
    if (q_changed(2, studio_hash(sel * 13u + (uint32_t)trk[sel].p[P_LEVEL], "equip-hero"))) {                        /* the hero panel (static part) */
        static const char *const HN[4] = {"MAGE", "ARCHER", "CLERIC", "WARRIOR"};
        char lv[16];
        cv_begin(86, 140, Q_BG);
        q_panel(0, 0, 86, 140);
        for (i = 0; i < 4u; i++) q_portrait(4 + (int32_t)i * 20, 4, i, i == sel ? Q_HI : Q_PADC, 0);
        q_text(6, 124, HN[sel], Q_TEXT);
        fmt_int(lv, (int32_t)((sel == TRK_DRUM ? song.g[G_DRLVL] : trk[sel].p[P_LEVEL]) * 100 / 127));
        { char t2[24]; str_cpy(t2, "lv ", sizeof t2); str_cpy(t2 + 3, lv, 12); q_text_r(80, 124, t2, Q_PDIM); }
        cv_blit(6, 24);
    }
    if (q_scene_budget(72 * 96 * 2)) q_scene_equip(13, 46, sel);          /* the hero on the pedestal: ~16 fps while playing, slow otherwise */
    q_dials(lab, val, ratio, 12, 5u);
    qb.fresh = 0;
}
static void quest_weather_draw(void)
{
    static const char *const W[4] = {"storm", "mist", "echo", "rain"}, *const BF[4] = {"+atk", "+def", "+spd", "regen"};
    char lb[4][10] = {{0}}, vb[4][16] = {{0}}, nm[16], b[24];
    const char *lab[4], *val[4];
    int32_t ratio[4];
    uint32_t i, sel = song.sel, sig, np = 0, k = 0, pi;
    q_tick_clocks();
    q_enter(QSN_WEATHER);
    q_cols(lab, val, ratio, vb, lb);
    for (i = 0; i < 4u; i++) { char *p; for (p = lb[i]; *p; p++) if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 32); }          /* README: DST CHO DLY REV */
    sig = sel * 17u + q_msg_sig() + (uint32_t)song.g[G_BPM];
    for (i = 0; i < 4u; i++) sig = studio_hash(sig * 7u + (uint32_t)ratio[i], vb[i]);
    if (q_changed(0, studio_hash((uint32_t)song.g[G_BPM] * 7u + song.playing * 3u + q_msg_sig(), "weather"))) {
        cv_begin(240, 19, Q_BG);
        q_title(6, "Weather", Q_HI);
        q_text(6 + text_w(&FONT_QT, "Weather") + 8, 4, "buffs", Q_DIM);
        q_bar_right_play();
        q_bar_end();
    }
    for (pi = 0; pi < NPAGES; pi++) if (PAGES[pi].fam == cur_page()->fam) { np++; if (pi == ui.page) k = np; }
    if (q_changed(1, sig)) {
        cv_begin(240, 116, Q_BG);
        for (i = 0; i < 4u; i++) {
            int32_t cx = 30 + 60 * (int32_t)i, n = (ratio[i] < 0 ? 0 : ratio[i]) / 100, j;
            if (i) cv_rect(60 * (int32_t)i, 0, 1, 118, Q_LINE);
            q_text_c(cx, 0, lab[i], Q_TRK[i]);
            q_text_c(cx, 11, W[i], Q_DIM);
            q_knob(cx, 33, ratio[i], Q_TRK[i]);
            q_text_c(cx, 48, val[i], Q_TEXT);
            q_text_c(cx, 59, BF[i], Q_TRK[i]);
            for (j = 0; j < 10; j++) cv_rect(cx - 10, 72 + j * 3, 20, 2, j >= 10 - n ? Q_TRK[i] : Q_OFF);
        }
        cv_blit(0, 104);
    }
    if (q_changed(2, sel * 13u + (uint32_t)trk[sel].p[P_LEVEL] + k)) {            /* the footer: the sound, its hero, the page */
        cv_begin(240, 18, Q_BG);
        cv_rect(0, 0, 240, 1, Q_LINE);
        trk_short_name(sel, nm);
        { uint32_t e = (uint32_t)q_text(6, 3, nm, Q_TEXT); q_text((int32_t)e + 6, 3, Q_HERO_NAME[sel], Q_TRK[sel]); }
        str_cpy(b, "fx ", sizeof b), fmt_int(b + 3, (int32_t)k), str_cpy(b + str_len(b), "/", 2), fmt_int(b + str_len(b), (int32_t)np);
        q_text_r(234, 3, b, Q_DIM);
        cv_blit(0, 222);
    }
    if (q_scene_budget(240 * 84 * 2)) { q_scene_world(QSC_FX); q_scene_flush(); }
    qb.fresh = 0;
}

/* a stock screen takes over (a page without a Quest screen, the song screen, a hold, ABOUT): wipe ours */
static void quest_leave(void)
{
    if (qs.screen) {
        qs.screen = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
}
