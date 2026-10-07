/* SPDX-License-Identifier: GPL-3.0-only
 * FM1 Quest, part 2: the boss fight, the duel, the banish bomb and the equip pedestal (README: BOSS, DUEL, BANISH, EQUIP).
 * Included by ui_quest.c. Beats and phases are Q8 (256 = one beat / one unit). */
#define QPC(x) ((x) * 256 / 100)

/* hero pose by attack phase p (Q8 beats), a hero of the party drawn at (x, y): the same rules as the world scene */
static uint32_t q_pose(uint32_t t, int32_t p)
{
    switch (t) {
    case 0: return (p != QNONE && p > -QPC(40) && p < QPC(60)) ? QS_MAGE_CAST_0 : QS_MAGE_IDLE_0;
    case 1: return (p != QNONE && p < 0) ? QS_ARCHER_DRAW_0 : QS_ARCHER_IDLE_0;
    case 2: return (p != QNONE && p > -QPC(30) && p < 256) ? QS_CLERIC_CAST_0 : QS_CLERIC_IDLE_0;
    default: return (p == QNONE || p >= QPC(60)) ? QS_WARRIOR_IDLE_0 : p < 0 ? QS_WARRIOR_ANTIC_0 : p < QPC(25) ? QS_WARRIOR_STRIKE_0 : QS_WARRIOR_FOLLOW_0;
    }
}

/* ---- BOSS (README): one hero per beat of the bar: warrior dash + impact frame, mage 3 meteors, archer volley, boss slam + barrier ---- */
static void q_boss_draw(int solid)      /* the King Slime and his minion, as the whole (solid -1) or as a silhouette (0) / flash (7) */
{
    int32_t G = 70, bq = q_beat_q8(), bi = (bq >> 8) & 3, bp = bq & 255, kl = 0, ksq = 0, l2, s2;
    if (bi == 3 && bp < QPC(30)) kl = 16 * qsin(bp * 16 / QPC(30)) / 127;
    else if (bi == 3 && bp < QPC(45)) ksq = QPC(30) * (256 - (bp - QPC(30)) * 256 / QPC(15)) / 256;
    else { int32_t st; q_hop(bq, 3, &kl, &st); ksq = st == 0 ? -31 : st == 2 ? 56 : 0; }
    q_slime(176, G, 54, 36, 12, 1, ksq, kl, solid == 7, solid == 0 ? 0 : -1, 1, 1, 0, 0);
    q_hop(bq + 128, 4, &l2, &s2);
    q_slime(224, G, 14, 10, 11, 3, s2 == 0 ? -31 : s2 == 2 ? 56 : 0, l2, solid == 7, solid == 0 ? 0 : -1, 1, 0, 0, 0);
}
static void q_scene_boss(void)
{
    int32_t G = 70, bq = q_beat_q8(), bi = (bq >> 8) & 3, bp = bq & 255, wx = Q_PX[3], wy = 0, front = 172 - 40, bcx = 172, bcy = G - 18, flash = 0;
    int32_t wp = QNONE, pm = QNONE, pa = QNONE, pc = QNONE, k;
    uint32_t sf;
    if ((bi == 0 && bp >= QPC(27) && bp < QPC(36)) || (bi == 2 && bp >= QPC(44) && bp < QPC(50))) flash = 7;
    if (bi == 1) for (k = 0; k < 3; k++) { int32_t v = k == 0 ? 38 : k == 1 ? 56 : 74; if (bp >= QPC(v) && bp < QPC(v + 5)) flash = 7; }
    cv_begin(QW, 84, QP(0));
    q_world(QB_CAVE, 0, 84, 0, 0);
    q_boss_draw(flash ? 7 : -1);
    if (bi == 0) {
        if (bp < QPC(18)) { wx = Q_PX[3] + (front - Q_PX[3]) * bp / QPC(18); wp = -QPC(20); }
        else if (bp < QPC(60)) { wx = front; wp = (bp - QPC(18)) * QPC(60) / QPC(42); }
        else { int32_t q = (bp - QPC(60)) * 256 / QPC(40); wx = front + (Q_PX[3] - front) * q / 256; wy = -(10 * qsin(q >> 3) / 127); }
        if (bp < QPC(22)) for (k = 0; k < 10; k++) q_rp(Q_PX[3] + (int32_t)(qhash((uint32_t)k + 3u) % 100u) * (wx - Q_PX[3]) / 100, G - 26 + (int32_t)(qhash((uint32_t)k) % 24u), 14 + (int32_t)(qhash((uint32_t)k + 5u) % 20u), 1, 12);
        if (bp < QPC(20)) for (k = 1; k <= 2; k++) q_spr(QS_WARRIOR_ANTIC_0, wx - k * 10, G - 24, 1, 0, 13);    /* afterimages */
    }
    if (bi == 1 && bp < QPC(85)) pm = QPC(10);
    if (bi == 2 && bp < QPC(45)) pa = -QPC(20);
    if (bi == 3) pc = q_min(QPC(90), bp);
    q_spr(q_pose(2, pc) + 0, Q_PX[2], G - 24, 1, 0, -1);
    q_spr(q_pose(1, pa), Q_PX[1], G - 24, 1, 0, -1);
    q_spr(q_pose(0, pm), Q_PX[0], G - 24 - 4 + qsin((int32_t)(qs.idle_q8 >> 3)) * 3 / 254, 1, 0, -1);
    sf = (bi == 0 && bp < QPC(18)) ? (uint32_t)((fm1_ms / 70u) & 1u) : 0u;
    {
        uint32_t wpose = (wp == QNONE || wp >= QPC(60)) ? QS_WARRIOR_IDLE_0 : wp < 0 ? QS_WARRIOR_ANTIC_0 : wp < QPC(25) ? QS_WARRIOR_STRIKE_0 : QS_WARRIOR_FOLLOW_0;
        q_spr(wpose + sf, wx, G - 24 + wy, 1, 0, -1);
        if (wp != QNONE && wp >= 0 && wp < QPC(60)) q_swoosh(wx + 10, G - 24 + wy + 12, 10, wp < QPC(25) ? 6 : q_max(1, 6 * (256 - (wp - QPC(25)) * 256 / QPC(35)) / 256), wp < QPC(25) ? wp * 256 / QPC(25) : 256, 12, 7, 0);
    }
    if (bi == 0 && bp >= QPC(27) && bp < QPC(50)) { static const uint8_t C3[3] = {7, 10, 12}; q_burst(bcx - 8, bcy, (bp - QPC(27)) * 256 / QPC(23) * 7 / 10, C3, 3); }
    if (bi == 1) {                                                                                                           /* the mage's 3 meteors */
        int32_t ax = Q_PX[0] + 18, ay = G - 24 - 4 + 10, hx = ax - 6, hy = ay - 24;
        for (k = 0; k < 8; k++) { int32_t a = k * 8 + bp / 6; q_rp(ax + 5 * qcos(a) / 127, ay + 5 * qsin(a) / 127, 1, 1, k & 1 ? 10u : 9u); }
        for (k = 0; k < 3; k++) {
            int32_t st = QPC(k * 18), q0 = (bp - st) * 256 / QPC(20), q = (bp - st - QPC(20)) * 256 / QPC(18), tx = bcx - 8 + k * 8, ty = bcy - 8 + k * 6, sx = hx + (k - 1) * 12;
            if (bp - st >= 0 && q0 < 256) q_summon(sx, hy, q0, 13);
            else if (bp - st - QPC(20) >= 0 && q < 256) { int32_t e = q * q / 256; q_meteor(sx + (tx - sx) * e / 256, hy + (ty - hy) * e / 256, 20); }
            else if (bp - st - QPC(20) >= 0 && q < 384) { static const uint8_t C4[4] = {10, 9, 8, 7}; q_burst(tx, ty, (q - 256) * 14 / 10, C4, 4); }
        }
    }
    if (bi == 2) {                                                                                                           /* the archer's volley of 6 arrows */
        int32_t ax = Q_PX[1] + 19, ay = G - 24 + 12;
        for (k = 0; k < 6; k++) {
            int32_t q = (bp - QPC(12) - k * QPC(6)) * 256 / QPC(32), x1 = bcx - 14 + k * 6, y1 = bcy - 10 + (int32_t)(qhash((uint32_t)k) % 16u);
            if (bp - QPC(12) - k * QPC(6) >= 0 && q < 256) q_arc_arrow(ax, ay, x1, y1, q, 34, 7, 12);
            else if (bp - QPC(12) - k * QPC(6) >= 0 && q < 333) q_rp(x1 - 1, y1 - 1, 3, 3, 7);
        }
        if (bp < QPC(45)) q_rp(ax, ay - 1, 2, 3, 12);
    }
    if (bi == 3) {                                                                                                           /* the shockwave, the cleric's barrier */
        if (bp >= QPC(30) && bp < QPC(85)) {
            int32_t q = (bp - QPC(30)) * 256 / QPC(55), x = bcx - 24 - q * (bcx - 24 - 60) / 256;
            q_rp(x, G - 10, 3, 10, 7), q_rp(x + 3, G - 6, 3, 6, 12);
            for (k = 0; k < 4; k++) q_rp(x + 4 + k * 3, G - 12 - (int32_t)(qhash((uint32_t)k + (qs.idle_q8 >> 4)) % 8u), 1, 1, 4);
        }
        if (bp >= QPC(20)) {
            int32_t pulse = bp >= QPC(70) && bp < QPC(80), a;
            for (a = 32; a <= 64; a++)
                if (pulse || (a & 1) == 0) q_rp(62 + 60 * qcos(a) / 127, G + 34 * qsin(a) / 127, 1, 1, pulse ? 7u : 12u);
        }
    }
    if (bi == 0 && bp >= QPC(20) && bp < QPC(27)) {                                                                          /* the impact frame: cream, two silhouettes */
        q_rp(0, 0, QW, 84, 7);
        q_boss_draw(0);
        q_spr(QS_WARRIOR_STRIKE_0, wx, G - 24 + wy, 1, 0, 0);
        q_swoosh(wx + 13, G - 12, 10, 5, 256, 0, 0, 0);
    }
}

/* ---- DUEL (README): one diagonal camera, ground y = 102 - x * 0.2; the hero against a shadow rival who counters off the beat ---- */
static int32_t q_gy(int32_t x) { return 102 - x / 5; }
static int32_t q_aph_str(const char *pat, int32_t len, int32_t sf_q8)       /* as q_aph_pat for a duel's 0/1 pattern (bytes) */
{
    int32_t s = sf_q8 >> 8, k;
    for (k = 1; k <= 2; k++) { int32_t n = s + k; if (pat[((n % len) + len) % len] && n * 256 - sf_q8 < 512) return (sf_q8 - n * 256) / 4; }
    for (k = 0; k < 6; k++) { int32_t h = s - k; if (pat[((h % len) + len) % len]) { int32_t p = (sf_q8 - h * 256) / 4; return p < 307 ? p : QNONE; } }
    return QNONE;
}
static void q_scene_duel(uint32_t trk_i)
{
    int32_t bq = q_beat_q8(), sf = bq * 4, len = (int32_t)clamp(trk[trk_i].p[P_SLEN], 1, NSTEP), i, x, tms = (int32_t)(fm1_ms & 0xFFFFFF);
    char hp_[NSTEP], rp_[NSTEP];
    int32_t ph, pr, hx = 38, rx = 164, home = 38, rhome = 164, hy, ry;
    int32_t sx = 0, sy = 0;
    for (i = 0; i < len; i++) { hp_[i] = trk_step_on(&trk[trk_i], (uint32_t)i); rp_[i] = (i % 16) == 12; }
    ph = song.playing ? q_aph_str(hp_, len, sf) : QNONE;
    pr = song.playing ? q_aph_str(rp_, len, sf) : QNONE;
    if (q_in(ph, 5, 16) || q_in(pr, 5, 16)) { sx = (int32_t)(qhash((uint32_t)(tms / 33)) % 5u) - 2; sy = (int32_t)(qhash((uint32_t)(tms / 33) + 3u) % 5u) - 2; }
    cv_begin(QW, 110, QP(5));
    {   /* sky, dashes in the air, the floor, specks on it */
        static const uint8_t SK[3] = {1, 2, 13};
        for (i = 0; i < 3; i++) q_rp(0, i * 77 / 3, QW, 77 / 3 + 2, SK[i]);
        for (i = 0; i < 26; i++) {
            int32_t len2 = 20 + (int32_t)(qhash((uint32_t)i + 3u) % 50u), off = 14 + (int32_t)(qhash((uint32_t)i) % 80u), spd = 120 + (int32_t)(qhash((uint32_t)i) % 90u);
            int32_t x0 = (((int32_t)(qhash((uint32_t)i + 9u) % 400u) - tms * spd / 1000) % 400 + 400) % 400 - 80, k;
            for (k = 0; k < len2; k++) if (x0 + k >= 0 && x0 + k < QW) q_rp(x0 + k + sx, q_gy(x0 + k) - off + sy, 1, 1, i % 3 ? 6u : 10u);
        }
        for (x = 0; x < QW; x++) { int32_t y = q_gy(x); q_rp(x, y, 1, 110 - y + 6, 5); q_rp(x, y, 1, 1, 10); }
        for (i = 0; i < 24; i++) {
            int32_t x0 = (((int32_t)(qhash((uint32_t)i) % 300u) - tms * 50 / 1000) % 300 + 300) % 300 - 30, off = 4 + (int32_t)(qhash((uint32_t)i + 1u) % 22u), k;
            for (k = 0; k < 6; k++) if (x0 + k >= 0 && x0 + k < QW) q_rp(x0 + k, q_gy(x0 + k) + off, 1, 1, 6);
        }
    }
    if (ph != QNONE) {
        if (ph < 0) hx = home - 3;
        else if (ph < QPC(15)) hx = home + (rhome - 34 - home) * ph / QPC(15);
        else if (ph < QPC(60)) hx = rhome - 34;
        else if (ph < 256) hx = rhome - 34 + (home - (rhome - 34)) * (ph - QPC(60)) / QPC(40);
    }
    if (q_in(ph, 12, 60)) rx = rhome + 10 * q_min(256, (ph - QPC(12)) * 256 / QPC(10)) / 256;
    if (pr != QNONE) {
        if (pr < 0) rx = rhome + 3;
        else if (pr < QPC(15)) rx = rhome + (home + 34 - rhome) * pr / QPC(15);
        else if (pr < QPC(60)) rx = home + 34;
        else if (pr < 256) rx = home + 34 + (rhome - (home + 34)) * (pr - QPC(60)) / QPC(40);
    }
    if (q_in(pr, 12, 60)) hx = home - 10 * q_min(256, (pr - QPC(12)) * 256 / QPC(10)) / 256;
    hy = q_gy(hx + 16) - 48, ry = q_gy(rx + 16) - 48;
    for (i = 0; i < 12; i++) { q_rp(hx + 4 + i * 2, q_gy(hx + 16) + 1, 1, 1, 0); q_rp(rx + 4 + i * 2, q_gy(rx + 16) + 1, 1, 1, 0); }
    if (q_in(ph, 0, 15)) for (i = 1; i <= 2; i++) q_spr(QS_WARRIOR_STRIKE_0, hx - i * 12, q_gy(hx - i * 12 + 16) - 48, 2, 0, 13);
    if (q_in(pr, 0, 15)) for (i = 1; i <= 2; i++) q_spr(QS_RIVAL_STRIKE, rx + i * 12, q_gy(rx + i * 12 + 16) - 48, 2, 1, 2);
    {
        uint32_t rpose = (pr == QNONE || pr >= QPC(60)) ? QS_RIVAL_IDLE : pr < 0 ? QS_RIVAL_ANTIC : pr < QPC(25) ? QS_RIVAL_STRIKE : QS_RIVAL_FOLLOW;
        q_spr(rpose, rx, ry, 2, 1, q_in(ph, 10, 22) ? 7 : -1);
        q_spr(q_pose(3, ph), hx, hy, 2, 0, q_in(pr, 10, 22) ? 7 : -1);
        if (ph != QNONE && ph >= 0 && ph < QPC(60)) q_swoosh(hx + 20, hy + 24, 16, (ph < QPC(25) ? 4 : 2) * 2, ph < QPC(25) ? ph * 256 / QPC(25) : 256, 7, 12, 0);
        if (pr != QNONE && pr >= 0 && pr < QPC(60)) q_swoosh(rx + 12, ry + 24, 16, (pr < QPC(25) ? 4 : 2) * 2, pr < QPC(25) ? pr * 256 / QPC(25) : 256, 14, 13, 1);
    }
    if (q_in(ph, 10, 35)) { static const uint8_t CA[3] = {7, 10, 12}; q_burst(rx + 10, ry + 22, (ph - QPC(10)) * 256 / QPC(25) * 6 / 10, CA, 3); }
    if (q_in(pr, 10, 35)) { static const uint8_t CB[2] = {7, 14}; q_burst(hx + 22, hy + 22, (pr - QPC(10)) * 256 / QPC(25) * 6 / 10, CB, 2); }
}

/* ---- BANISH (README): the warrior throws a bomb in an arc, it explodes and dissolves the slimes, which re-form 8 beats later ---- */
static void q_scene_erase(void)
{
    int32_t G = 70, bq = q_beat_q8(), cyc = bq & 2047, cc = cyc * 100 / 256, k = bq >> 11, m, n;       /* cc: beat of the 8-beat cycle, x100 */
    int32_t wph = QNONE, bx0 = Q_PX[3] + 16, by0 = G - 16, tx = 173, ty = G - 6;
    cv_begin(QW, 84, QP(0));
    q_world(QB_CAVE, 0, 84, 0, 0);
    wph = cc < 45 ? -QPC(20) : cc < 70 ? QPC(10) : QNONE;
    for (m = 0; m < 2; m++) {
        int32_t mx = m ? 194 : 152, lift, st, dph = 0;                                                   /* dph: dissolve, Q8 (1.1 = 281) */
        q_hop(bq + m * 102, 3, &lift, &st);
        if (cc >= 180 && cc < 600) dph = q_min(281, (cc - 180) * 256 / 140);
        else if (cc >= 600) dph = q_max(0, 281 - (cc - 600) * 256 / 160);
        q_slime(mx, G, 22, 16, 11, 3, dph > 0 ? 0 : (st == 0 ? -31 : st == 2 ? 56 : 0), dph > 0 ? 0 : lift, cc >= 150 && cc < 210, -1, dph <= 102, 0, (uint32_t)(k * 31 + m * 5 + 500), dph);
        if (dph > 0 && cc < 600)
            for (n = 0; n < 14; n++)
                if ((int32_t)(qhash((uint32_t)(n + k + m)) % 256u) < dph)
                    q_rp(mx - 11 + (int32_t)(qhash((uint32_t)(n * 3 + m)) % 22u), G - 8 - dph * 24 * (int32_t)(qhash((uint32_t)(n + 7)) % 100u) / 25600 - n % 3, 1, 1, n % 2 ? 8u : 7u);
    }
    q_spr(wph == QNONE ? QS_WARRIOR_IDLE_0 : wph < 0 ? QS_WARRIOR_ANTIC_0 : QS_WARRIOR_STRIKE_0, Q_PX[3], G - 24, 1, 0, -1);
    q_spr(QS_CLERIC_IDLE_0, Q_PX[2], G - 24, 1, 0, -1), q_spr(QS_ARCHER_IDLE_0, Q_PX[1], G - 24, 1, 0, -1), q_spr(QS_MAGE_IDLE_0, Q_PX[0], G - 28, 1, 0, -1);
    if (cc >= 60 && cc < 150) {                                                                          /* the bomb */
        int32_t q = (cc - 60) * 256 / 90, bx = bx0 + (tx - bx0) * q / 256, by = by0 + (ty - by0) * q / 256 - 30 * qsin(q >> 3) / 127, f = (int32_t)((fm1_ms / 50u) & 1u);
        q_rp(bx - 3, by - 3, 7, 7, 0), q_rp(bx - 2, by - 2, 5, 5, 5), q_rp(bx - 1, by - 2, 2, 1, 6), q_rp(bx + 1, by - 5, 1, 2, 4), q_rp(bx + 1 + f, by - 6, 1, 1, f ? 10u : 9u);
    }
    if (cc >= 150 && cc < 280) {                                                                         /* the explosion: a dark disc, smoke, a cross flash */
        int32_t q = (cc - 150) * 100 / 130, r = q < 30 ? q * 24 / 30 : 24 * (100 - (q - 30) * 100 / 70 * 50 / 100) / 100;
        if (q < 65) q_disc(tx, ty - 6, r, QP(0));
        for (n = 0; n < 8; n++) { int32_t a = n * 8 + 4, rr = r + 3 + q * 8 / 100; q_rp(tx + rr * qcos(a) / 127 - 2, ty - 6 + rr * qsin(a) * 8 / 1270 - 2, 5, 5, q < 50 ? 6u : 5u); }
        if (q < 25) { q_rp(tx - 16, ty - 6, 33, 1, 10); q_rp(tx, ty - 22, 1, 33, 10); for (n = -9; n <= 9; n++) q_rp(tx + n, ty - 6 + n, 1, 1, 7), q_rp(tx + n, ty - 6 - n, 1, 1, 7); }
        if (q >= 60) for (n = 0; n < 10; n++) { int32_t a = n * 10, rr = 10 + q * 20 / 100; q_rp(tx + rr * qcos(a) / 127, ty - 6 + rr * qsin(a) * 6 / 762 - q * 6 / 100, 3, 3, 6); }
    }
}

/* ---- EQUIP (README): the hero of the selected track on a pedestal with a glow, 3x; particles rise ---- */
static void q_scene_equip(int32_t bx, int32_t by, uint32_t t)           /* a 72 x 96 strip blitted at (bx, by) */
{
    int32_t x, y, dy, k, tms = (int32_t)(fm1_ms & 0xFFFFFF);
    uint32_t pose = Q_HERO[t];
    const qspr_t *sp;
    cv_begin(72, 96, QP(1));
    for (y = 0; y < 96; y++)
        for (x = (y & 1); x < 72; x += 2) {
            int32_t dx = x - 36, ddy = y - 50;
            if (dx * dx * 100 / 900 + ddy * ddy * 100 / 1700 < 100) cv_pset(x, y, QP(2));
        }
    for (dy = -4; dy <= 4; dy++) { int32_t w = 28 * q_isqrt(16 * 256 - dy * dy * 256) / 64; q_rp(36 - w, 86 + dy, 2 * w, 1, dy < -2 ? 10u : 4u); }
    for (k = 0; k < 6; k++) { int32_t p = (tms * 4 / 10 + k * 1000 / 6) % 1000; q_rp(10 + (int32_t)(qhash((uint32_t)k) % 52u), 84 - p * 70 / 1000, 1, 1, 10); }
    if ((tms % 2400) < 800) pose = t == 0 ? QS_MAGE_CAST_0 : t == 1 ? QS_ARCHER_DRAW_0 : t == 2 ? QS_CLERIC_CAST_0 : QS_WARRIOR_STRIKE_0;
    sp = &QSPR[pose];
    q_spr(pose, 36 - 3 * (sp->ox + sp->w / 2), 86 - 3 * (sp->oy + sp->h), 3, 0, -1);
    cv_blit((uint32_t)bx, (uint32_t)by);
}
