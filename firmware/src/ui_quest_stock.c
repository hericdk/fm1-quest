/* SPDX-License-Identifier: GPL-3.0-only
 * FM1 Quest, part 5: the pages that used to keep the firmware's stock layout (head, four columns, graph, foot), now drawn by our own code, and the SONG chain and
 * the HOLD ring with them. One scene a family of pages above the four dials:
 *   Ledger   PRESETS / USER / PROJECT   the list on a parchment, the chosen line lit by a torch
 *   Board    ARP, ENV DEST, SCL 2, TOOLS, SYSTEM   a row a knob: a gem, its name and a bar with its range
 *   Beats    GLOBAL   eight torches on a beam, the odd ones pushed by the swing, the beat flashing
 *   Window   MASTER   the DJ filter curve in a tower window, the dust as sparks, the duck as a pumping bar
 *   Seats    DRUMS (GLO) / VOICE   sixteen pads or the voices as seats that light
 *   Scroll   STEP / PATTERN   the steps on a scroll: the bank as a little piano roll, or the whole pattern as bars
 * The knobs' legend is always at the bottom (q_dials). Nothing here reads a button: the keys and knobs are the firmware's, unchanged. */
static const char *sk_lab[4], *sk_val[4];
static int32_t sk_ratio[4];

static UI_SMALL int sk_scene(void)                                     /* 0 ledger, 1 board, 2 beats, 3 window, 4 seats, 5 scroll, 6 "the drum track has none" */
{
    const page_t *pg = cur_page();
    if (is_drum(TSEL) && !page_for_drum(pg)) return 6;
    if (pg->graph == GR_BROWSE || pg->graph == GR_USER || pg->graph == GR_SLOTS) return 0;
    if (pg->graph == GR_STEPS || pg->scope == SC_STEP) return 5;
    switch (pg->id[0]) {
    case G_BPM: return 2;
    case G_DUST: return 3;
    case G_DRCH: case P_VOICE: case P_ALLOC: return 4;
    default: return 1;
    }
}
static UI_SMALL const char *sk_name(void)                              /* the name of the place */
{
    const page_t *pg = cur_page();
    if (pg->graph == GR_BROWSE) return "Armory";
    if (pg->graph == GR_USER) return "My Gear";
    if (pg->graph == GR_SLOTS) return "Chronicles";
    if (pg->scope == SC_STEP) return "Spell Book";
    if (pg->graph == GR_STEPS) return "Scroll";
    switch (pg->id[0]) {
    case G_BPM: return "Drum Hall";
    case G_DUST: return "Mage Tower";
    case G_DRCH: return "Drum Rack";
    case P_VOICE: case P_ALLOC: return "Roll Call";
    case P_AMODE: case P_ASWING: return "Minstrel";
    case P_ED_FLT: return "Smithy";
    case P_TRANS: return "Scale 2";
    case G_MIDI: return "Watchtower";
    default: return "Workshop";
    }
}
static UI_SMALL void sk_wall(void)                                     /* a dark brick wall, the room of every page here */
{
    int32_t r, k;
    p_R(0, PS_Y0, 240, 176, QP(0));
    for (r = 0; r < 13; r++) for (k = 0; k < 9; k++) p_R(k * 28 + (r & 1) * 14 - 8, PS_Y0 + 1 + r * 14, 26, 12, QP(1));
}
static UI_SMALL void sk_torch(int32_t x, int32_t y, int32_t big, uint32_t t)   /* a wall torch: handle, cup, flicker */
{
    int32_t fx = p_sin(t + (uint32_t)x * 40u, 130) * 2 / 127, h = 7 + big * 4 + p_sin(t + (uint32_t)x * 17u, 90) * 2 / 127;
    p_R(x - 1, y, 3, 8, QP(4)), p_R(x - 3, y - 2, 7, 3, QP(5));
    p_tri(x - 3, y - 2, x + 3, y - 2, x + fx, y - 2 - h, QP(8));
    p_tri(x - 2, y - 2, x + 2, y - 2, x + fx / 2, y - 2 - h * 7 / 10, QP(9));
    p_tri(x - 1, y - 2, x + 1, y - 2, x, y - 2 - h * 4 / 10, QP(10));
}
static UI_SMALL void sk_row(int32_t y, int32_t sel, uint16_t c)       /* a list line: the chosen one lit with a torch mark */
{
    if (sel) p_R(14, y - 2, 212, 17, QP(4)), p_R(14, y - 2, 212, 1, QP(10)), p_R(16, y + 2, 4, 8, QP(9)), p_R(17, y, 2, 3, QP(10));
    (void)c;
}
static UI_SMALL void sk_ledger(const ps_c *c)
{
    const page_t *pg = cur_page();
    int32_t i;
    (void)c;
    sk_wall();
    q_panel(8, 24, 224, 166);
    if (pg->graph == GR_BROWSE) {
        uint32_t total, cur = preset_pos(&total);
        for (i = -3; i <= 3 && total; i++) {
            uint32_t n = (cur + total * 4u + (uint32_t)i) % total, e, k;
            char tag[8], nm[16];
            int32_t y = 36 + (i + 3) * 21;
            e = preset_at(n, &k);
            if (e == NENGINES) up_slot_label(tag, k), up_name(k, nm);
            else str_cpy(tag, preset_kind(n), sizeof tag), str_cpy(nm, ENGINES[e]->presets[k].name, sizeof nm);
            te_lower(nm, nm, sizeof nm);
            sk_row(y, i == 0, 0);
            q_text(24, y, tag, i == 0 ? Q_HITXT : Q_DIM), q_text(70, y, nm, i == 0 ? Q_HITXT : Q_TEXT);
        }
    } else if (pg->graph == GR_USER) {
        int32_t first = clamp((int32_t)ui.uslot - 3, 0, UP_SLOTS - 7);
        for (i = 0; i < 7; i++) {
            uint32_t k = (uint32_t)(first + i);
            int32_t y = 36 + i * 21, sel = k == ui.uslot, used = up_used(k);
            char tag[8], nm[16];
            up_slot_label(tag, k);
            if (used) up_name(k, nm), te_lower(nm, nm, sizeof nm); else str_cpy(nm, "empty", sizeof nm);
            sk_row(y, sel, 0);
            q_text(24, y, tag, sel ? Q_HITXT : Q_DIM), q_text(70, y, nm, sel ? Q_HITXT : used ? Q_TEXT : Q_PDIM);
        }
    } else {                                                                                           /* PROJECT: four chronicles, used or empty */
        for (i = 0; i < 4; i++) {
            int32_t y = 40 + i * 36, sel = i + 1 == song.g[G_SLOT], used = project_used((uint32_t)i);
            char b[16];
            p_R(14, y - 4, 212, 30, sel ? Q_HI : Q_PAD), p_R(15, y - 3, 210, 28, sel ? Q_PADC : Q_BG);
            b[0] = 'c', str_cpy(b + 1, "hronicle ", 12), b[10] = (char)('1' + i), b[11] = 0;
            q_text(24, y + 2, b, sel ? Q_TEXT : Q_DIM);
            if (used) { int32_t k; for (k = 0; k < 4; k++) p_R(24 + k * 14, y + 14, 10, 6, Q_TRK[k]); }
            q_text_r(216, y + 2, used ? "used" : "empty", used ? Q_HI : Q_PDIM);
        }
    }
}
static UI_SMALL void sk_board(const ps_c *c)
{
    int32_t i;
    sk_wall();
    for (i = 0; i < 4; i++) {
        int32_t y = 30 + i * 40, k;
        if (!sk_lab[i][0]) continue;
        p_R(10, y - 4, 220, 34, Q_PAD), p_R(11, y - 3, 218, 32, Q_BG);
        p_ell(26, y + 11, 9, 9, QP(0)), p_ell(26, y + 11, 7, 7, Q_TRK[i]), p_ell(24, y + 8, 2, 2, QP(7));          /* the gem */
        q_text(42, y, sk_lab[i], Q_TEXT);
        q_text_r(222, y, sk_val[i], Q_HI);
        p_R(42, y + 15, 176, 8, QP(0));                                                                          /* the range, with a notch every quarter */
        p_R(43, y + 16, 174 * q_max(0, q_min(1000, sk_ratio[i])) / 1000, 6, Q_TRK[i]);
        for (k = 1; k < 4; k++) p_R(42 + k * 44, y + 13, 1, 12, QP(5));
    }
    (void)c;
}
static UI_SMALL void sk_beats(const ps_c *c)                                                              /* GLOBAL: the swung eighths */
{
    int32_t sw = song.g[G_SWING] > 50 ? song.g[G_SWING] - 50 : 0, k, cur = song.playing ? (int32_t)(fm1_ms * (uint32_t)song.g[G_BPM] / 30000u % 8u) : -1;
    char b[12];
    sk_wall();
    p_R(0, 104, 240, 5, QP(4)), p_R(0, 104, 240, 1, QP(10));                                                    /* the beam */
    for (k = 0; k < 8; k++) {
        int32_t x = 20 + k * 28 + ((k & 1) ? sw * 28 / 50 : 0);
        sk_torch(x, 100, k == cur, c->t);
        p_R(x - 2, 112, 5, (k & 1) ? 8 : 12, k == cur ? QP(7) : (k & 1) ? Q_TRK[song.sel & 3u] : QP(5));       /* its mark below: shorter when it is a swung one */
    }
    fmt_int(b, song.g[G_BPM]);
    q_panel(80, 28, 80, 40);
    q_text_c(120, 34, b, Q_HI), q_text_c(120, 50, "bpm", Q_DIM);
}
static UI_SMALL void sk_window(const ps_c *c)                                                             /* MASTER: the DJ filter, the dust, the duck */
{
    int32_t n = song.g[G_FILT], x0 = n < 0 ? 239 + n * 239 / 64 : n * 239 / 63, x, py = 0, k, d = song.g[G_DUCK] * 60 / 127;
    sk_wall();
    p_R(14, 26, 212, 150, QP(0)), p_R(16, 28, 208, 146, QP(1)), p_R(16, 28, 208, 4, QP(5));                     /* the window */
    for (x = 0; x < 208; x++) {
        int32_t v = 100, y;
        int32_t xx = x * 239 / 207;
        if (n < 0 && xx > x0) v = 100 - (xx - x0) * 3 / 2;
        if (n > 0 && xx < x0) v = 100 - (x0 - xx) * 3 / 2;
        if (v < 4) v = 4;
        y = 168 - v;
        if (x) cv_line(16 + x - 1, py, 16 + x, y, QP(10));
        py = y;
    }
    for (k = 0; k < song.g[G_DUST] * 26 / 127; k++)                                                              /* the dust: crackles */
        cv_pset(18 + (int32_t)((((uint32_t)k + 1u) * 2654435761u + (c->t / 120u) * 40503u) >> 8) % 204, 36 + (int32_t)((((uint32_t)k + 5u) * 2246822519u + (c->t / 120u) * 3266489917u) >> 9) % 70, QP(7));
    p_R(228, 28 + 146 - d - 4, 8, d + 4, QP(8)), p_R(228, 28, 8, 146, QP(5));                                  /* the duck: a bar that is pushed down */
    p_R(229, 29 + 146 - 2 - d, 6, d, QP(9));
}
static UI_SMALL void sk_seats(const ps_c *c)                                                               /* DRUMS (the pads) or the voices */
{
    int32_t k;
    sk_wall();
    if (cur_page()->id[0] == G_DRCH) {
        for (k = 0; k < DRUM_LANES; k++) {
            int32_t x = 20 + (k % 8) * 26, y = 50 + (k / 8) * 40, on = pad_lit[k] != 0;
            p_R(x, y, 22, 30, on ? QP(7) : QP(0)), p_R(x + 1, y + 1, 20, 28, on ? Q_HI : Q_PAD);
            if (on) p_R(x + 6, y - 6, 10, 4, QP(10));
        }
    } else {
        for (k = 0; k < NVOICE; k++) {
            int32_t x = 24 + (k % 4) * 50, y = 56 + (k / 4) * 54, on = TSEL->v[k].active != 0;
            p_ell(x + 14, y + 24, 14, 5, QP(0)), p_R(x + 2, y + 6, 24, 18, on ? Q_TRK[song.sel & 3u] : Q_PAD), p_R(x + 2, y + 2, 24, 5, on ? QP(7) : Q_PADC);
            p_ell(x + 14, y - 4, 7, 7, on ? QP(15) : QP(5));
        }
    }
    (void)c;
}
static UI_SMALL void sk_scroll(const ps_c *c)                                                              /* STEP: the bank as a piano roll; PATTERN: all the steps */
{
    const track_t *t = TSEL;
    uint32_t i, j, len = (uint32_t)t->p[P_SLEN];
    const uint16_t col = Q_TRK[song.sel & 3u];
    (void)c;
    sk_wall();
    q_panel(6, 26, 228, 160);
    if (cur_page()->scope == SC_STEP) {
        uint32_t base = ui.bank * 16u;
        int32_t lo = 127, hi = 0, prev_y = -1;
        for (i = 0; i < len; i++) for (j = 0; j < t->step[i].n; j++) { if (t->step[i].note[j] < lo) lo = t->step[i].note[j]; if (t->step[i].note[j] > hi) hi = t->step[i].note[j]; }
        if (lo > hi) lo = 48, hi = 72;
        if (hi - lo < 12) hi = lo + 12;
        for (i = 0; i < 16u; i++) {
            uint32_t si = base + i;
            const step_t *st = &t->step[si];
            int32_t x = 14 + (int32_t)i * 13;
            if (si >= len) break;
            p_R(x, 34, 11, 108, i % 4u == 0u ? Q_PADC : Q_PAD);
            if (si == ui.cursor) p_R(x - 1, 33, 13, 110, Q_HI), p_R(x, 34, 11, 108, Q_BG);
            if (song.playing && si == t->seq_idx) p_R(x, 146, 11, 3, QP(7));
            if (st->time == ST_TIE && prev_y >= 0) { p_R(x, prev_y, 11, 3, Q_DIM); continue; }
            if (!step_on(st)) { prev_y = -1; continue; }
            for (j = 0; j < st->n; j++) {
                int32_t y = 134 - (st->note[j] - lo) * 92 / (hi - lo);
                p_R(x, y, 11, 4, (st->flags & SF_ACCENT) ? QP(7) : col);
                if (j == 0) prev_y = y;
            }
        }
    } else {
        for (i = 0; i < NSTEP; i++) {
            int32_t x = 18 + (int32_t)(i % 16u) * 13 + (int32_t)(i % 16u) / 4 * 3, y = 38 + (int32_t)(i / 16u) * 36;
            const step_t *st = &t->step[i];
            if (i >= len) { p_R(x, y + 18, 9, 2, Q_PAD); continue; }
            if (step_on(st)) p_R(x, y, 9, 18, col);
            else if (st->time == ST_TIE) p_R(x, y + 8, 9, 10, Q_DIM);
            else p_R(x, y + 16, 9, 2, Q_OFFHI);
            if (st->flags & SF_ACCENT) p_R(x, y - 4, 9, 2, QP(7));
            if (song.playing && i == t->seq_idx) p_R(x - 1, y + 21, 11, 4, QP(7));
        }
    }
}
static UI_SMALL void sk_nodrum(const ps_c *c)
{
    (void)c;
    sk_wall();
    q_panel(30, 56, 180, 70);
    q_text_c(120, 70, "the drum track has no", Q_TEXT), q_text_c(120, 84, "sound of its own", Q_TEXT);
    q_text_c(120, 104, "seq  tracks  glo  drums", Q_DIM);
}
static UI_SMALL uint32_t sk_dirty(const ps_c *c, ps_r *o)                                                   /* what moves: the torches, the dust, the playhead, the pads */
{
    (void)c;
    o[0] = (ps_r){0, PS_Y0, 239, 193, 0};
    return 1;
}
static UI_SMALL void q_stock_cols(const char *lab[4], const char *val[4], int32_t ratio[4], char (*vb)[16], char (*lb)[10])
{
    const page_t *pg = cur_page();
    uint32_t c;
    if (pg->graph == GR_BROWSE || pg->graph == GR_USER || pg->scope == SC_STEP) {
        for (c = 0; c < 4u; c++) lab[c] = lb[c], val[c] = vb[c], ratio[c] = 0, lb[c][0] = 0, vb[c][0] = 0;
        if (pg->graph == GR_BROWSE) {
            uint32_t total, cur = preset_pos(&total);
            str_cpy(lb[0], "no.", 10), fmt_int(vb[0], (int32_t)cur + 1), str_cpy(vb[0] + str_len(vb[0]), "/", 2), fmt_int(vb[0] + str_len(vb[0]), (int32_t)total);
            str_cpy(lb[1], "engine", 10), str_cpy(vb[1], ENGINES[TSEL->eng_req % NENGINES]->name, 16), te_lower(vb[1], vb[1], 16);
        } else if (pg->graph == GR_USER) {
            int used = up_used(ui.uslot);
            str_cpy(lb[0], "slot", 10), up_slot_label(vb[0], ui.uslot), ratio[0] = (int32_t)ui.uslot * 1000 / (int32_t)(UP_SLOTS - 1u);
            str_cpy(lb[1], "load", 10), str_cpy(vb[1], used ? "go" : "--", 16);
            str_cpy(lb[2], "erase", 10), str_cpy(vb[2], used ? "go" : "--", 16);
            str_cpy(lb[3], "save", 10), str_cpy(vb[3], "go", 16);
        } else {
            static const char *const TIME_N[3] = {"note", "tie", "rest"}, *const FLAG_N[4] = {"-", "acc", "sld", "a+s"};
            const step_t *st = &TSEL->step[ui.cursor];
            str_cpy(lb[0], "step", 10), fmt_int(vb[0], (int32_t)ui.cursor + 1), str_cpy(vb[0] + str_len(vb[0]), "/", 2), fmt_int(vb[0] + str_len(vb[0]), TSEL->p[P_SLEN]);
            str_cpy(lb[1], "note", 10);
            if (st->n) note_name(vb[1], st->note[0]); else str_cpy(vb[1], "--", 16);
            str_cpy(lb[2], "time", 10), str_cpy(vb[2], TIME_N[st->time % 3u], 16);
            str_cpy(lb[3], "flag", 10), str_cpy(vb[3], FLAG_N[(st->flags & SF_ACCENT ? 1u : 0u) | (st->flags & SF_SLIDE ? 2u : 0u)], 16);
        }
        return;
    }
    q_cols(lab, val, ratio, vb, lb);
    for (c = 0; c < 4u; c++) {
        if (pg->scope != SC_GLOBAL) break;
        if (pg->id[c] == G_MIDI) { str_cpy(lb[c], "usb", 10), str_cpy(vb[c], !usb.up ? "off" : usb.config ? "midi" : usb.setups ? "enum" : usb.sof_seen ? "bus" : "wait", 16); }
        else if (pg->id[c] == G_INFO) { fmt_int(vb[c], (int32_t)(song.cpu_q8 * 100u / 256u)); str_cpy(vb[c] + str_len(vb[c]), "%", 2); }
    }
}
static UI_SMALL void quest_stock_draw(void)
{
    static void (*const FN[7])(const ps_c *) = {sk_ledger, sk_board, sk_beats, sk_window, sk_seats, sk_scroll, sk_nodrum};
    static ps_r prev;
    static uint32_t last_ms, last_sig, last_sc = 99, frame;
    char lb[4][10] = {{0}}, vb[4][16] = {{0}}, pn[16], b[40];
    const char *lab[4], *val[4];
    int32_t ratio[4], k4[4], sc = sk_scene(), i;
    uint32_t sig, pi, np = 0, k = 0;
    const page_t *pg = cur_page();
    ps_c c;
    q_tick_clocks();
    q_enter((uint8_t)(QSN_PAGE0 + 8 + (uint32_t)sc));
    q_stock_cols(lab, val, ratio, vb, lb);
    for (i = 0; i < 4; i++) k4[i] = ratio[i] < 0 ? 0 : ratio[i], sk_lab[i] = lab[i], sk_val[i] = val[i], sk_ratio[i] = k4[i];
    c.k = k4, c.t = fm1_ms, c.a = c.b = c.c = c.d = 0;
    sig = (uint32_t)sc * 31u + q_msg_sig() + ui.page * 977u + ui.cursor * 13u + ui.bank * 7u + ui.uslot * 17u + (uint32_t)song.g[G_SLOT] * 5u + up_gen * 7919u + TSEL->preset * 3u + song.sel * 11u;
    for (i = 0; i < 4; i++) sig = studio_hash(sig * 7u + (uint32_t)ratio[i], vb[i]);
    if (sc == 5) sig = sig * 31u + steps_hash(TSEL) + (song.playing ? TSEL->seq_idx : 0xFFFFu) * 3u;
    for (pi = 0; pi < NPAGES; pi++) if (PAGES[pi].fam == pg->fam) { np++; if (pi == ui.page) k = np; }
    if (q_changed(0, studio_hash((uint32_t)song.g[G_BPM] * 7u + song.playing * 3u + q_msg_sig() + k + song.sel * 101u + TSEL->preset * 5u, sk_name()))) {
        cv_begin(240, 19, Q_BG);
        q_title(6, sk_name(), Q_HI);
        trk_short_name(song.sel, pn);
        te_lower(pn, pn, sizeof pn);
        b[0] = (char)('1' + song.sel), b[1] = ' ', b[2] = 0, str_cpy(b + 2, pn, 18);
        q_text(6 + text_w(&FONT_QT, sk_name()) + 8, 4, b, Q_DIM);
        q_bar_right_play();
        q_bar_end();
    }
    q_dials(lab, val, ratio, 12, sig);
    if (last_sc != (uint32_t)sc || last_sig != sig || Q_FULL) {
        last_sc = (uint32_t)sc, last_sig = sig, last_ms = fm1_ms;
        ps_paint(FN[sc], &c, 0, PS_Y0, 239, PS_Y1 - 1);
        prev.x1 = -1;
    } else if (sc >= 2 && sc <= 5 && fm1_ms - last_ms >= 40u) {                                                  /* the moving ones, ~25 frames a second */
        ps_r r;
        last_ms = fm1_ms, frame++;
        sk_dirty(&c, &r);
        ps_paint(FN[sc], &c, r.x0, r.y0, r.x1, r.y1);
    }
    (void)frame;
    qb.fresh = 0;
}

/* ---- SONG: the chain of sections as a chronicle ---- */
static UI_SMALL void quest_song_draw(void)
{
    static uint32_t previous;
    uint32_t i, start, sig;
    char b[30];
    song_sane();
    start = song_cursor > 3u ? song_cursor - 3u : 0u;
    q_tick_clocks();
    q_enter(QSN_SONG);
    sig = song_cursor + 17u * arrangement_enabled + 37u * song.playing + 71u * arrangement_clock.index + 127u * arrangement_clock.bar + 257u * arrangement.count + 509u * (uint32_t)song.g[G_BPM];
    for (i = 0; i < arrangement.count; i++) sig = sig * 31u + arrangement.entry[i].scene + 7u * arrangement.entry[i].bars;
    for (i = 0; i < ARR_SCENES; i++) sig = sig * 3u + (uint32_t)project_used(i);
    sig += q_msg_sig();
    if (!Q_FULL && sig == previous) return;
    previous = sig;
    cv_begin(240, 19, Q_BG);
    q_title(6, "Chronicle", Q_HI);
    q_text(6 + text_w(&FONT_QT, "Chronicle") + 8, 4, arrangement_enabled ? "song mode" : "loop mode", arrangement_enabled ? Q_TRK[3] : Q_DIM);
    q_bar_right_play();
    q_bar_end();
    for (i = 0; i < 5u; i++) {
        uint32_t pos = start + i;
        cv_begin(240, 26, Q_BG);
        if (pos < arrangement.count) {
            const arr_entry_t *e = &arrangement.entry[pos];
            int selected = pos == song_cursor, used = project_used(e->scene);
            uint16_t sc = Q_TRK[e->scene & 3u];
            int32_t w = e->bars * 120 / 64 + 4;
            if (selected) cv_rect(2, 0, 236, 25, Q_HI), cv_rect(3, 1, 234, 23, Q_PADC);
            fmt_int(b, (int32_t)pos + 1);
            q_text(10, 6, b, selected ? Q_TEXT : Q_DIM);
            cv_rect(34, 3, 22, 20, used ? sc : Q_PAD);
            b[0] = (char)('A' + e->scene), b[1] = 0;
            q_text(41, 6, b, used ? Q_HITXT : sc);
            cv_rect(64, 9, w, 8, used ? (selected ? sc : Q_DIM) : Q_PAD);
            fmt_int(b, e->bars);
            str_cpy(b + str_len(b), e->bars == 1 ? " bar" : " bars", 8);
            q_text(64 + w + 6, 6, used ? b : "empty", selected ? Q_TEXT : Q_DIM);
            if (arrangement_clock.running && arrangement_clock.index == pos) {
                cv_rect(64, 19, (int32_t)(arrangement_clock.bar + 1u) * w / (e->bars ? (int32_t)e->bars : 1), 2, Q_TEXT);
                cv_rect(222, 9, 9, 9, Q_TRK[1]);
            }
        }
        cv_blit(0, 22 + 26 * (int32_t)i);
    }
    cv_begin(240, 60, Q_BG);
    if (ui.msg_t) {
        cv_rect(10, 4, 220, 28, Q_HI);
        q_text_c(120, 11, ui.msg, Q_HITXT);
    } else {
        static const char *const L[4] = {"entry", "section", "bars", "length"};
        for (i = 0; i < 4u; i++) {
            cv_rect((int32_t)i * 60 + 4, 6, 52, 3, Q_TRK[i]);
            q_text_c((int32_t)i * 60 + 30, 12, L[i], Q_DIM);
        }
        q_text(6, 34, "rec: store   save: chain", Q_TEXT);
        q_text(6, 46, "oct-: loop/song  oct+ x2: load", Q_DIM);
    }
    cv_blit(0, 160);
    qb.fresh = 0;
}

/* ---- the HOLD ring (REC held: clear; SAVE held: save) ---- */
static UI_SMALL void quest_hold_draw(void)
{
    static uint32_t cache;
    uint32_t span = ui.hold_kind == 1u ? HOLD_CLEAR_MS : HOLD_SAVE_MS, el = fm1_ms - ui.hold_t0;
    int32_t ratio = el >= span ? 1000 : (int32_t)(el * 1000u / span), a, rr, end;
    uint32_t sig = ui.hold_kind * 7u + (uint32_t)ratio / 20u + ui.hold_trk * 131u;
    char b[24];
    q_tick_clocks();
    q_enter(QSN_HOLD);
    if (sig == cache && !Q_FULL) return;
    cache = sig;
    cv_begin(240, 124, Q_BG);
    cv_text(120 - text_w(&FONT_QT, ui.hold_kind == 1u ? "Clear" : "Save") / 2, 4, &FONT_QT, ui.hold_kind == 1u ? "Clear" : "Save", ui.hold_kind == 1u ? Q_REC : Q_HI);
    end = ratio * 64 / 1000;                                                                         /* the ring: 64 steps, filling clockwise from the top */
    for (a = 0; a < 64; a++) {
        uint16_t col = a < end ? (ui.hold_kind == 1u ? Q_REC : Q_HI) : Q_PAD;
        for (rr = 30; rr <= 36; rr++) cv_rect(120 + qcos(a - 16) * rr / 127, 80 + qsin(a - 16) * rr / 127, 2, 2, col);
    }
    cv_blit(0, 20);
    cv_begin(240, 60, Q_BG);
    if (ui.hold_kind == 1u) str_cpy(b, "track 1", sizeof b), b[6] = (char)('1' + ui.hold_trk);
    else str_cpy(b, "project in slot 1", sizeof b), b[16] = (char)('0' + clamp(song.g[G_SLOT], 1, 4));
    q_text_c(120, 4, b, Q_TEXT);
    q_text_c(120, 24, "keep holding", Q_DIM);
    cv_blit(0, 160);
    qb.fresh = 0;
}
