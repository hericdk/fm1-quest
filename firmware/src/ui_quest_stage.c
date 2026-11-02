/* ---------------------------------------------------------------- STAGE (the PARTY effect, from winampFM): who is on stage. KNOB 1 FORM: SOLO one hero,
 * DUEL two, TRIAD three, PARTY all; KNOB 2 LEAD: the first of them; KNOB 3 TURN: every 1 / 2 / 4 / 8 bars the stage turns (OFF: it stays); KNOB 4 STYLE:
 * KEEP (the lead stays, the others take turns) or YOYO (the stage walks on and comes back). It works through the solo mask: the heroes off stage
 * rest (a fade, nothing is erased). FORM PARTY with TURN OFF touches nothing. The Party menu (ui.menu 4) takes the four knobs. */
static uint8_t st_form = 3, st_lead, st_turn, st_style;       /* (TURN starts at OFF) */
static uint32_t st_p0, st_hint;                               /* the turns count from PLAY; the last knob turned (for the screen) */
static uint8_t st_mask = 0xFF;                                /* the stage last set (0xFF: not ours) */
static const char *const ST_FORM[4] = {"solo", "duel", "triad", "party"};
static const char *const ST_TURN[5] = {"off", "1 bar", "2 bars", "4 bars", "8 bars"};
static const char *const ST_STYLE[2] = {"keep", "yoyo"};
static UI_SMALL uint32_t st_bar(void)                      /* bars since PLAY, a little early (a note on the beat finds its hero on stage) */
{
    uint32_t la = 2205u * (uint32_t)song.g[G_BPM], pos = clk_pos + la, beat = clk_beat + pos / BEAT_U;
    return beat / 4u;
}
static UI_SMALL uint32_t st_stage(void)                    /* the heroes on stage now: a mask of 4 */
{
    static const uint8_t BARS[5] = {0, 1, 2, 4, 8}, YO[6] = {0, 1, 2, 3, 2, 1};
    uint32_t n = st_form + 1u, b = BARS[st_turn % 5u], st = 0, j, m = 0, p = song.playing ? st_bar() : 0u, el = p > st_p0 ? p - st_p0 : 0u;
    if (b) st = el / b;
    if (st_style) {
        uint32_t o = b ? YO[st % 6u] : 0u;
        for (j = 0; j < n; j++) m |= 1u << ((st_lead + o + j) & 3u);
        return m;
    }
    m = 1u << (st_lead & 3u);
    for (j = 0; j + 1u < n; j++) m |= 1u << ((st_lead + 1u + (st + j) % 3u) & 3u);
    return m;
}
static UI_SMALL void stage_tick(void)                      /* ui_draw, every frame: the stage turns on the bar */
{
    uint32_t m;
    if (!song.playing) st_p0 = 0;                             /* (a new PLAY counts from its first bar) */
    if (sx_on) return;                                        /* (the sound preview uses the solo: leave it) */
    m = st_stage();
    if (m == 15u) m = 0u;                                     /* everyone: no solo */
    if (m == st_mask) return;
    if (st_mask == 0xFFu && !m) return;                       /* PARTY and nothing of ours to undo */
    song.solo = (uint8_t)m;
    st_mask = m ? (uint8_t)m : 0xFFu;
    ui.force = 1;
}
static UI_SMALL void stage_knobs(void)                     /* the Party menu: a step per detent */
{
    static const uint8_t TOP[4] = {3, 3, 4, 1};
    uint8_t *pv[4] = {&st_form, &st_lead, &st_turn, &st_style};
    uint32_t k;
    int32_t s;
    for (k = 0; k < 4u; k++) {
        if ((s = panel_enc(EN_K1 + k)) == 0) continue;
        *pv[k] = (uint8_t)clamp((int32_t)*pv[k] + (s > 0 ? 1 : -1), 0, TOP[k]);
        st_hint = fm1_ms ? fm1_ms : 1u;
        ui.force = 1;
    }
}
static UI_SMALL uint32_t stage_sig(void) { return 1u + st_form * 3u + st_lead * 17u + st_turn * 53u + st_style * 211u + (uint32_t)(song.solo & 15u) * 997u; }
