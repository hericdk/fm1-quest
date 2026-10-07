/* SPDX-License-Identifier: GPL-3.0-only
 * FM1 Quest: the RPG sidescroll interface (design: README of design_handoff_fm01_quest).
 * Tracks are heroes, lit steps are attacks, BPM sets the scroll (bpm * 0.22 px/s), the key / scale sets the
 * biome, the sends set the weather, a recording is a boss fight, mute rests at the campfire, solo is a duel.
 * Every musical function stays where SLOOP has it; this layer only draws. Integer maths, the cv_* canvas.
 *
 * Cosmetic state only: the scroll offset, an idle beat clock and the attack phases, which are derived from the
 * transport clock (clk_beat / clk_pos) and the patterns, never stored.
 *
 * Part 1 (this file): helpers, sprites, the world and its scenes. Part 2 (ui_quest_scenes.c): boss, duel, equip.
 * Part 3 (ui_quest_screens.c): the screens. */
#include "felucca_qfont.h"
#include "felucca_qsprites.h"
static uint32_t str_hash(uint32_t h, const char *s);   /* ui_draw.c */

/* ---- the TAVERN palette (README) ---- */
static const uint16_t QPAL[16] = {
    RGB(0x1a, 0x10, 0x16), RGB(0x2b, 0x23, 0x38), RGB(0x5a, 0x30, 0x50), RGB(0x2f, 0x4a, 0x3a),
    RGB(0x6a, 0x40, 0x30), RGB(0x4a, 0x3c, 0x48), RGB(0xa8, 0x9a, 0xa0), RGB(0xf2, 0xe2, 0xc4),
    RGB(0xc8, 0x44, 0x3c), RGB(0xe0, 0x8a, 0x3c), RGB(0xf0, 0xc8, 0x60), RGB(0x6a, 0x9a, 0x4a),
    RGB(0x4a, 0x78, 0xb0), RGB(0x8a, 0x6a, 0x9a), RGB(0xe0, 0x90, 0x90), RGB(0xf0, 0xb8, 0x90)};
#define QP(i) QPAL[(i) & 15u]
#define Q_BG RGB(0x1e, 0x14, 0x18)
#define Q_TEXT RGB(0xf2, 0xe2, 0xc4)
#define Q_DIM RGB(0xa8, 0x90, 0x80)
#define Q_PDIM RGB(0xc8, 0xb0, 0x90)
#define Q_LINE RGB(0x5a, 0x3a, 0x28)
#define Q_OFF RGB(0x2e, 0x22, 0x26)
#define Q_OFFHI RGB(0x45, 0x34, 0x3a)
#define Q_PAD RGB(0x24, 0x18, 0x1c)
#define Q_PADC RGB(0x3a, 0x4a, 0x78)
#define Q_WOOD RGB(0x3a, 0x26, 0x20)
#define Q_GOLD RGB(0xc8, 0x96, 0x3c)
#define Q_HI RGB(0xf0, 0xc8, 0x60)
#define Q_HITXT RGB(0x1e, 0x14, 0x18)
#define Q_REC RGB(0xe0, 0x50, 0x3c)
static const uint16_t Q_TRK[4] = {RGB(0x5a, 0x8a, 0xd0), RGB(0x7a, 0xb0, 0x4e), RGB(0xf0, 0xc8, 0x60), RGB(0xe0, 0x8a, 0x3a)};
static const uint16_t Q_TRK_DIM[4] = {RGB(0x2a, 0x3a, 0x63), RGB(0x3a, 0x5a, 0x28), RGB(0x7a, 0x64, 0x20), RGB(0x7a, 0x4a, 0x20)};
static const uint8_t Q_HERO[4] = {QS_MAGE_IDLE_0, QS_ARCHER_IDLE_0, QS_CLERIC_IDLE_0, QS_WARRIOR_IDLE_0};   /* track -> hero (README) */
static const uint8_t Q_ROT[4] = {QS_MAGE_ROT, QS_ARCHER_ROT, QS_CLERIC_ROT, QS_WARRIOR_ROT};
static const uint8_t Q_SEAT[4] = {QS_MAGE_SEAT, QS_ARCHER_SEAT, QS_CLERIC_SEAT, QS_WARRIOR_SEAT};
static const char *const Q_HERO_NAME[4] = {"mage", "archer", "cleric", "warrior"};
static const int32_t Q_PX[4] = {62, 36, 12, 88};         /* hero slots by track: mage, archer, cleric, warrior (README) */

/* sin(2 pi k / 64) * 127 */
static const int8_t Q_SIN[64] = {0, 12, 25, 37, 49, 60, 71, 81, 90, 98, 106, 112, 117, 122, 125, 126, 127, 126, 125, 122, 117, 112, 106,
                                 98, 90, 81, 71, 60, 49, 37, 25, 12, 0, -12, -25, -37, -49, -60, -71, -81, -90, -98, -106, -112, -117,
                                 -122, -125, -126, -127, -126, -125, -122, -117, -112, -106, -98, -90, -81, -71, -60, -49, -37, -25, -12};
static int32_t qsin(int32_t a) { return Q_SIN[a & 63]; }
static int32_t qcos(int32_t a) { return Q_SIN[(a + 16) & 63]; }
static uint32_t qhash(uint32_t n)       /* the scene's deterministic noise */
{
    n = n * 2654435761u + 0x9E3779B9u;
    n ^= n >> 15;
    n *= 2246822519u;
    n ^= n >> 13;
    return n;
}
static int32_t q_isqrt(int32_t v)
{
    int32_t r = 0, b = 1 << 28;
    if (v <= 0) return 0;
    while (b > v) b >>= 2;
    while (b) {
        if (v >= r + b) { v -= r + b; r = (r >> 1) + b; }
        else r >>= 1;
        b >>= 2;
    }
    return r;
}
static int32_t q_atan(int32_t y, int32_t x)              /* degrees 0..359 */
{
    int32_t ax = x < 0 ? -x : x, ay = y < 0 ? -y : y, a;
    if (ax == 0 && ay == 0) return 0;
    a = ax >= ay ? ay * 45 / ax : 90 - ax * 45 / ay;
    if (x < 0) a = 180 - a;
    if (y < 0) a = 360 - a;
    return a % 360;
}
static int32_t q_min(int32_t a, int32_t b) { return a < b ? a : b; }
static int32_t q_max(int32_t a, int32_t b) { return a > b ? a : b; }
#define QNONE ((int32_t)0x80000000)

/* ---- cosmetic state (README: the scroll offset, the monster stream offset, the attack phases) ---- */
static struct {
    uint32_t sp_q8;            /* scroll, 1/256 px; the monster stream rides on it */
    uint32_t idle_q8;          /* a beat clock for the idle animation (the transport is stopped) */
    uint32_t last_ms, scene_ms;
    uint8_t screen;            /* which Quest screen is drawn (a change repaints everything) */
    uint32_t sig[24];          /* per-element redraw signatures */
    uint32_t combo, combo_step;
    uint32_t victory_ms;
} qs;

/* ---- drawing helpers ---- */
static void q_spr(uint32_t id, int32_t x, int32_t y, int32_t s, int flip, int solid)   /* s = scale, solid = palette index or -1 */
{
    const qspr_t *sp = &QSPR[id];
    const uint8_t *d = QSPR_DATA + sp->off;
    int32_t gx, gy, bx = flip ? 16 - sp->ox - sp->w : sp->ox;
    for (gy = 0; gy < sp->h; gy++)
        for (gx = 0; gx < sp->w; gx++) {
            uint8_t v = d[gy * sp->w + gx];
            int32_t px = x + s * (bx + (flip ? sp->w - 1 - gx : gx)), py = y + s * (sp->oy + gy);
            if (v == 255u)
                continue;
            if (s == 1) cv_pset(px, py, QP(solid >= 0 ? (uint32_t)solid : v));
            else cv_rect(px, py, s, s, QP(solid >= 0 ? (uint32_t)solid : v));
        }
}
static void q_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t dy;
    for (dy = -r; dy <= r; dy++) {
        int32_t w = q_isqrt(r * r - dy * dy);
        cv_rect(cx - w, cy + dy, 2 * w + 1, 1, c);
    }
}
static void q_rp(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t i) { cv_rect(x, y, w < 1 ? 1 : w, h < 1 ? 1 : h, QP(i)); }
static void q_tri(int32_t x, int32_t y, uint16_t c)
{
    cv_rect(x, y + 2, 1, 2, c), cv_rect(x + 1, y + 1, 1, 4, c), cv_rect(x + 2, y, 2, 6, c);
    cv_rect(x + 4, y + 1, 1, 4, c), cv_rect(x + 5, y + 2, 1, 2, c);
}
static int32_t q_text(int32_t x, int32_t y, const char *s, uint16_t c) { return cv_text(x, y, &FONT_Q, s, c); }
static void q_text_r(int32_t xr, int32_t y, const char *s, uint16_t c) { cv_text(xr - text_w(&FONT_Q, s), y, &FONT_Q, s, c); }
static void q_text_c(int32_t cx, int32_t y, const char *s, uint16_t c) { cv_text(cx - text_w(&FONT_Q, s) / 2, y, &FONT_Q, s, c); }
static void q_title(int32_t x, const char *s, uint16_t c) { cv_text(x, -1, &FONT_QT, s, c); }
static void q_title_r(int32_t xr, const char *s, uint16_t c) { cv_text(xr - text_w(&FONT_QT, s), -1, &FONT_QT, s, c); }
static void q_outline_text(int32_t x, int32_t y, const char *s, uint16_t c)       /* cream on a dark outline (README) */
{
    cv_text(x - 1, y, &FONT_Q, s, Q_BG), cv_text(x + 1, y, &FONT_Q, s, Q_BG), cv_text(x, y - 1, &FONT_Q, s, Q_BG), cv_text(x, y + 1, &FONT_Q, s, Q_BG);
    cv_text(x, y, &FONT_Q, s, c);
}
static void q_panel(int32_t x, int32_t y, int32_t w, int32_t h)     /* wood panel, gold border, inner shadows (README) */
{
    cv_rect(x, y, w, h, Q_GOLD);
    cv_rect(x + 1, y + 1, w - 2, h - 2, Q_BG);
    cv_rect(x + 2, y + 2, w - 4, h - 4, RGB(0x6a, 0x40, 0x30));
    cv_rect(x + 3, y + 3, w - 6, h - 6, Q_WOOD);
}

/* ---- the knob of the design: 20 x 20, ring -135..+135 deg in the knob's colour, cap, 5 px pointer, no rim ---- */
static uint8_t q_ring[20 * 20];                          /* 255 = not ring, else (angle from -135 deg) / 2 */
static uint8_t q_ring_ready;
static void q_knob(int32_t cx, int32_t cy, int32_t ratio, uint16_t col)
{
    int32_t x, y, a1;
    if (ratio < 0) ratio = 0;
    if (ratio > 1000) ratio = 1000;
    if (!q_ring_ready) {
        for (y = 0; y < 20; y++)
            for (x = 0; x < 20; x++) {
                int32_t X = 2 * x - 19, Y = 2 * y - 19, r2 = X * X + Y * Y, deg = q_atan(X, -Y), from = deg > 180 ? deg - 360 : deg;
                q_ring[y * 20 + x] = (r2 >= 153 && r2 < 296 && from >= -135 && from <= 135) ? (uint8_t)((from + 135) / 2) : 255;
            }
        q_ring_ready = 1;
    }
    a1 = ratio * 270 / 1000;
    for (y = 0; y < 20; y++)
        for (x = 0; x < 20; x++) {
            int32_t X = 2 * x - 19, Y = 2 * y - 19, r2 = X * X + Y * Y;
            uint8_t g = q_ring[y * 20 + x];
            if (g != 255)
                cv_pset(cx - 10 + x, cy - 10 + y, (int32_t)g * 2 <= a1 ? col : Q_OFF);
            else if (r2 < 108)
                cv_pset(cx - 10 + x, cy - 10 + y, r2 >= 71 ? RGB(0x8a, 0x5a, 0x38) : RGB(0x6a, 0x40, 0x30));
        }
    {
        int32_t a = (a1 - 135) * 64 / 360, s;
        for (s = 0; s <= 4; s++)
            cv_pset(cx + (s * qsin(a)) / 127, cy - (s * qcos(a)) / 127, Q_TEXT);
    }
}

/* ---- attack phases (README): p in beats, Q8; anticipation p in [-0.5, 0), strike [0, 0.25), follow-through to 0.6 ---- */
static int32_t q_aph_pat(const track_t *t, int32_t sf_q8)
{
    int32_t len = (int32_t)clamp(t->p[P_SLEN], 1, NSTEP), s = sf_q8 >> 8, k;
    for (k = 1; k <= 2; k++) {
        int32_t n = s + k;
        if (trk_step_on(t, (uint32_t)(((n % len) + len) % len)) && n * 256 - sf_q8 < 2 * 256)
            return (sf_q8 - n * 256) / 4;
    }
    for (k = 0; k < 6; k++) {
        int32_t h = s - k;
        if (trk_step_on(t, (uint32_t)(((h % len) + len) % len))) {
            int32_t p = (sf_q8 - h * 256) / 4;
            return p < 307 ? p : QNONE;                   /* 1.2 beats */
        }
    }
    return QNONE;
}
static int q_in(int32_t p, int32_t a, int32_t b) { return p != QNONE && p >= a * 256 / 100 && p < b * 256 / 100; }
#define QP100(x) ((x) * 256 / 100)

/* ---- the clock the scenes share ---- */
static int32_t q_beat_q8(void)       /* beats, Q8: the transport's, or the idle clock while stopped */
{
    return song.playing ? (int32_t)(clk_beat * 256u + clk_pos / (BEAT_U >> 8)) : (int32_t)qs.idle_q8;
}
static void q_tick_clocks(void)
{
    uint32_t now = fm1_ms, dt = qs.last_ms ? now - qs.last_ms : 0u;
    qs.last_ms = now;
    if (dt > 250u) dt = 250u;
    if (song.playing)
        qs.sp_q8 += ((uint32_t)song.g[G_BPM] * dt * 231u) >> 12;                        /* bpm * 0.22 px/s in 1/256 px */
    qs.idle_q8 += (uint32_t)song.g[G_BPM] * dt * 256u / 60000u;
}
static void q_hop(int32_t bq, int32_t amp, int32_t *lift, int32_t *st)   /* st: 0 air (stretched), 1 rest, 2 landing (squashed) */
{
    int32_t f = bq & 255;
    *lift = 0, *st = 1;
    if (f < 141) { *lift = (amp * qsin(f * 32 / 141)) / 127; *st = 0; }
    else if (f < 184) *st = 2;
}

/* ---- a slime (README): translucent dome, darker lower band, white highlight, oval eyes; squash and stretch ---- */
static void q_slime(int32_t cx, int32_t gy, int32_t w, int32_t h, uint32_t M, uint32_t D, int32_t sq_q8, int32_t lift, int flash, int solid,
                    int shadow, int crown, uint32_t dseed, int32_t dis_q8)
{
    int32_t W2 = q_max(4, w * (256 + sq_q8) / 256), H2 = q_max(3, h * (256 - sq_q8) / 256), top = gy - H2 - lift, yy, x;
    uint32_t Mc = flash ? 8u : M, Dc = flash ? 2u : D;
#define SK(px, py) (dis_q8 > 0 && (int32_t)(qhash(((uint32_t)(px) - (uint32_t)cx + 200u) * 13u + ((uint32_t)(py) - (uint32_t)top + 200u) * 7u + dseed) % 256u) < dis_q8)
#define SP(px, py, i) do { if (!SK(px, py)) cv_pset(px, py, QP(solid >= 0 ? (uint32_t)solid : (uint32_t)(i))); } while (0)
    if (shadow && solid < 0) {
        int32_t sw = W2 * 42 / 100 * (256 - q_min(154, lift * 256 / 30)) / 256;
        for (x = -sw; x <= sw; x++)
            cv_pset(cx + x, gy, QP((x < 0 ? -x : x) < sw * 7 / 10 ? 0 : ((x + gy) & 1 ? 0 : 5)));
    }
    for (yy = -1; yy < H2; yy++) {
        int32_t tt = yy < 0 ? 0 : (2 * yy + 1) * 128 / H2, hw, y = top + yy, inv;           /* tt = (yy + 0.5) / H2, Q8 */
        if (yy < 0) {
            int32_t tw = W2 * 16 / 100;
            for (x = -tw; x <= tw; x++) SP(cx + x, y, 0);
            continue;
        }
        inv = 256 - q_min(256, tt * 100 / 70);                                               /* 1 - min(1, tt / 0.7) */
        hw = (W2 * q_isqrt(65536 - inv * inv) + 255) / 512;                                  /* (W2 / 2) * sqrt(1 - inv^2), rounded */
        if (tt > 220) hw = hw * (256 - (tt - 220) * 9 / 10) / 256;
        for (x = -hw - 1; x <= hw + 1; x++) {
            int edge = x < -hw || x > hw || yy == H2 - 1;
            SP(cx + x, y, edge ? 0u : (tt > 179 ? Dc : Mc));
        }
    }
    {   /* highlight (top right) and the eyes */
        int32_t hx = cx + W2 * 22 / 100, hy = top + H2 * 26 / 100, rx = q_max(1, W2 * 8 / 100), ry = q_max(1, H2 * 13 / 100), ex = q_max(2, W2 * 14 / 100), ey = top + H2 * 46 / 100, eh = q_max(2, H2 * 20 / 100), px, py, y;
        for (py = -ry; py <= ry; py++)
            for (px = -rx; px <= rx; px++)
                if (px * px * 100 / (rx * rx) + py * py * 100 / (ry * ry) <= 100)
                    SP(hx + px, hy + py, flash ? 14u : 7u);
        for (y = 0; y < eh; y++) {
            SP(cx - ex - 1, ey + y, 0); SP(cx - ex, ey + y, 0); SP(cx + ex - 1, ey + y, 0); SP(cx + ex, ey + y, 0);
        }
    }
    if (crown) {
        int32_t cxl = cx - 4;
        q_rp(cxl, top - 5, 9, 4, 0), q_rp(cxl + 1, top - 4, 7, 2, 10), q_rp(cxl + 1, top - 6, 1, 2, 10);
        q_rp(cx, top - 7, 1, 3, 10), q_rp(cx + 3, top - 6, 1, 2, 10), q_rp(cx, top - 3, 1, 1, 8);
    }
#undef SP
#undef SK
}

/* ---- effects (README): meteor, summon, burst, arrows, crescent smear, spark ---- */
static void q_meteor(int32_t x, int32_t y, int32_t sc10)
{
    int32_t k;
    for (k = 1; k <= 9; k++) {
        int32_t tx = x - k * 16 * sc10 / 100, ty = y - k * 14 * sc10 / 100, sz = q_max(1, (12 - k) * sc10 / 40);
        q_rp(tx, ty, sz, sz, k < 3 ? 10 : k < 6 ? 9 : 8);
    }
    q_rp(x - sc10 * 2 / 10, y - sc10 * 2 / 10, sc10 * 5 / 10, sc10 * 5 / 10, 0);
    q_rp(x - sc10 * 15 / 100, y - sc10 * 15 / 100, sc10 * 4 / 10, sc10 * 4 / 10, 9);
    q_rp(x - sc10 / 10, y - sc10 / 10, sc10 * 3 / 10, sc10 * 3 / 10, 10);
    q_rp(x, y, 1, 1, 7);
}
static void q_summon(int32_t x, int32_t y, int32_t k_q8, int32_t sc10)          /* k: 0..1 (Q8) */
{
    int32_t r = q_max(1, k_q8 * 3 * sc10 / 2560), q;
    for (q = 0; q < 10; q++) {
        int32_t a = q * 10 + k_q8 / 4, rr = r + 4 + qsin(k_q8 / 3 + q * 9) * 3 / 254;
        cv_pset(x + rr * qcos(a) / 127, y + rr * qsin(a) * 7 / 1270, QP(q & 1 ? 10 : 9));
    }
    if (k_q8 < 90) q_rp(x, y - 3, 1, 7, 7), q_rp(x - 3, y, 7, 1, 7);
    q_rp(x - r - 1, y - r - 1, 2 * r + 3, 2 * r + 3, 0), q_rp(x - r, y - r, 2 * r + 1, 2 * r + 1, 9);
    q_rp(x - r + 1, y - r + 1, 2 * r - 1, 2 * r - 1, 10), q_rp(x, y, 1, 1, 7);
}
static void q_burst(int32_t x, int32_t y, int32_t q_q8, const uint8_t *cols, int32_t nc)
{
    int32_t k;
    for (k = 0; k < 10; k++) {
        int32_t a = k * 10 + q_q8 * 10 / 256, r = 2 + q_q8 * 12 / 256, sz = q_q8 < 128 ? 2 : 1;
        q_rp(x + r * qcos(a) / 127, y + r * qsin(a) / 127, sz, sz, cols[k % nc]);
    }
}
static void q_arrow(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t col, int32_t trail)
{
    int32_t dx = x1 - x0, dy = y1 - y0, l = q_isqrt(dx * dx + dy * dy), k, ux, uy;
    if (l == 0) l = 1;
    ux = dx * 256 / l, uy = dy * 256 / l;
    for (k = 0; k < 7; k++) q_rp(x1 - ux * k / 256, y1 - uy * k / 256, 1, 1, col);
    q_rp(x1 + ux / 256, y1 + uy / 256, 1, 1, 7);
    if (trail >= 0)
        for (k = 0; k < 10; k += 2) q_rp(x1 - ux * (9 + k * 2) / 256 + qsin(k * 4) * 3 / 254, y1 - uy * (9 + k * 2) / 256 + qcos(k * 4) * 3 / 254, 1, 1, (uint32_t)trail);
}
static void q_arc_arrow(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t q_q8, int32_t hgt, uint32_t col, int32_t trail)
{
    int32_t qa = q_max(0, q_q8 - 20), ax = x0 + (x1 - x0) * qa / 256, ay = y0 + (y1 - y0) * qa / 256 - hgt * qsin(qa >> 3) / 127;
    int32_t bx = x0 + (x1 - x0) * q_q8 / 256, by = y0 + (y1 - y0) * q_q8 / 256 - hgt * qsin(q_q8 >> 3) / 127, k;
    q_arrow(ax, ay, bx, by, col, -1);
    if (trail >= 0)
        for (k = 1; k < 6; k += 2) {
            int32_t qq = q_max(0, q_q8 - 20 - k * 10);
            q_rp(x0 + (x1 - x0) * qq / 256, y0 + (y1 - y0) * qq / 256 - hgt * qsin(qq >> 3) / 127, 1, 1, (uint32_t)trail);
        }
}
static void q_swoosh(int32_t cx, int32_t cy, int32_t r, int32_t th, int32_t prog_q8, uint32_t col, uint32_t col2, int flip)   /* crescent smear */
{
    int32_t a0 = -115, span = 175, a1 = a0 + span * q_max(13, q_min(256, prog_q8)) / 256, rr = r + th + 1, x, y;
    for (y = -rr; y <= rr; y++)
        for (x = -rr; x <= rr; x++) {
            int32_t d = q_isqrt(x * x + y * y), a, u, w;
            if (d < r) continue;
            a = q_atan(y, flip ? -x : x);
            if (a > 180) a -= 360;
            if (a < a0 || a > a1) continue;
            u = (a - a0) * 64 / (a1 - a0 + 1);
            w = th * qsin(u / 2) / 127;
            if (d <= r + w) cv_pset(cx + x, cy + y, QP(d > r + w / 2 ? col : col2));
        }
}

/* ---- biomes (README: major forest, minor cave, dorian coast, phrygian ruins; night / storm for camp, menu and weather) ---- */
enum { QB_FOREST, QB_CAVE, QB_COAST, QB_RUINS, QB_NIGHT, QB_STORM };
#define QW 240
static uint32_t q_biome_of_key(void)    /* trk[0] holds the key of the song (every synth part has it): MAJ forest, MIN cave, DOR coast, PHRY ruins */
{
    switch (trk[0].p[P_SCALE]) {
    case 2: return QB_CAVE;
    case 3: return QB_COAST;
    case 8: return QB_RUINS;
    default: return QB_FOREST;
    }
}
static void q_world(uint32_t biome, int32_t sp, int32_t H, int flowers, int birds)
{
    static const uint8_t SKY[6][4] = {{1, 2, 8, 9}, {0, 0, 0, 0}, {12, 12, 13, 14}, {1, 2, 5, 13}, {0, 0, 1, 1}, {5, 5, 13, 6}};
    static const uint8_t MNT[6] = {2, 0, 13, 5, 5, 1}, MNT2[6] = {1, 0, 255, 1, 1, 0};
    int32_t G = H - 14, i, x;
    if (biome == QB_CAVE) {                                 /* cave: dark walls, torches, stalactites, flagstones */
        cv_rect(0, 0, QW, H, QP(0));
        for (i = -1; i < 7; i++) {
            int32_t ps = sp * 35 / 100, k = ps / 44 + i, px = k * 44 - ps;
            q_rp(px, 0, 14, G, 1), q_rp(px, 0, 2, G, 5);
            if ((k & 1) == 0) {
                q_rp(px + 6, G - 30, 2, 5, 4), q_rp(px + 6, G - 33, 2, 3, 9);
                q_rp(px + 6 + ((qs.idle_q8 >> 7) & 1), G - 35, 1, 2, 10);
            }
        }
        for (x = 0; x < QW; x++) {
            int32_t wx = (x + sp * 9 / 10) / 3, h = 4 + (int32_t)(qhash((uint32_t)wx) % 5u) + (qhash((uint32_t)wx + 3u) % 100u > 85u ? 8 : 0);
            q_rp(x, 0, 1, h, 5);
        }
        q_rp(0, G, QW, H - G, 5), q_rp(0, G, QW, 1, 6);
        for (i = 0; i < 3; i++) {
            int32_t y = G + 4 + i * 4, off = (i & 1) * 6, k;
            q_rp(0, y, QW, 1, 0);
            for (k = sp / 12 - 1; k < (sp + QW) / 12 + 1; k++) q_rp(k * 12 + off - sp, y - 3, 1, 3, 0);
        }
        return;
    }
    for (i = 0; i < 4; i++) {                               /* the sky: four bands, dithered edges */
        int32_t y0 = i * G / 4, y1 = (i + 1) * G / 4;
        q_rp(0, y0, QW, y1 - y0, SKY[biome][i]);
        if (i && SKY[biome][i] != SKY[biome][i - 1])
            for (x = 0; x < QW; x += 2) {
                q_rp(x, y0, 1, 1, SKY[biome][i - 1]);
                if ((x & 3) == 0) q_rp(x + 1, y0 + 1, 1, 1, SKY[biome][i - 1]);
            }
    }
    if (biome == QB_RUINS || biome == QB_NIGHT)
        for (i = 0; i < 30; i++)
            q_rp((int32_t)(qhash((uint32_t)i) % QW), (int32_t)(qhash((uint32_t)i + 50u) % (uint32_t)(G - 30)), 1, 1, qhash((uint32_t)i + 9u) % 10u > 6u ? 6u : 7u);
    if (biome == QB_NIGHT) {
        q_disc(198, 14, 7, QP(7)), q_rp(195, 11, 2, 2, 6), q_rp(200, 16, 3, 2, 6);
    } else if (biome == QB_FOREST || biome == QB_COAST) {   /* the sunset disc and its bands */
        q_disc(172, G - 28, 15, QP(10));
        for (i = 0; i < 4; i++) q_rp(150, G - 25 + i * 4, 46, i + 1, 9);
    }
    if (biome == QB_STORM)                                   /* clouds */
        for (i = 0; i < 7; i++) {
            int32_t cx = (((i * 53 - (int32_t)(qs.idle_q8 >> 5) * 6 / 8 - sp / 5) % 300) + 300) % 300 - 30, cy = 5 + (i % 3) * 5;
            q_rp(cx, cy, 30, 5, 6), q_rp(cx + 6, cy - 3, 16, 3, 6), q_rp(cx + 2, cy + 5, 26, 2, 13);
        }
    for (x = 0; x < QW; x++) {                               /* two ranges of mountains */
        int32_t wx = x + sp * 12 / 100, h = 24 + (8 * qsin(wx * 29 / 64)) / 127 + (5 * qsin(wx * 83 / 64 + 20)) / 127;
        q_rp(x, G - h, 1, h, MNT[biome]);
        if (MNT2[biome] != 255) {
            wx = x + sp * 30 / 100;
            h = 11 + (5 * qsin(wx * 18 / 64 + 10)) / 127 + (3 * qsin(wx * 48 / 64)) / 127;
            q_rp(x, G - h, 1, h, MNT2[biome]);
        }
    }
    if (biome == QB_COAST) {                                 /* the sea */
        q_rp(0, G - 9, QW, 9, 12);
        for (x = 0; x < QW; x++)
            if (((x + (int32_t)(qs.idle_q8 >> 6) + sp / 2) / 5) & 1)
                q_rp(x, G - 9 + (int32_t)(qhash((uint32_t)((x + sp / 2) / 7)) % 8u), 1, 1, 7);
        q_rp(0, G - 9, QW, 1, 6);
    } else if (biome == QB_RUINS) {                          /* broken pillars */
        for (i = -1; i < 8; i++) {
            int32_t ts = sp * 6 / 10, k = ts / 36 + i, px = k * 36 + (int32_t)(qhash((uint32_t)k) % 14u) - ts, hh = 12 + (int32_t)(qhash((uint32_t)k + 4u) % 20u), q;
            q_rp(px - 1, G - hh - 1, 9, hh + 1, 0), q_rp(px, G - hh, 7, hh, 6), q_rp(px, G - hh, 2, hh, 7), q_rp(px + 5, G - hh, 2, hh, 5);
            if (qhash((uint32_t)k + 8u) % 2u) q_rp(px - 2, G - hh - 3, 11, 3, 0), q_rp(px - 1, G - hh - 2, 9, 1, 7);
            else q_rp(px + 2, G - hh, 3, 2, 5);
            for (q = 0; q < hh; q += 4) q_rp(px + 1, G - q - 2, 5, 1, 5);
        }
    } else {                                                 /* pines */
        int32_t ts = sp * 6 / 10;
        for (i = -1; i < 11; i++) {
            int32_t k = ts / 24 + i, px = k * 24 + (int32_t)(qhash((uint32_t)k) % 12u) - ts, th = 10 + (int32_t)(qhash((uint32_t)k + 9u) % 12u), j;
            q_rp(px, G - 3, 1, 3, 4);
            for (j = 0; j < th; j++) {
                int32_t w = (j % 5) * 7 / 10 + j * 22 / 100;
                q_rp(px - w, G - 3 - th + j, w * 2 + 1, 1, 3), q_rp(px + w, G - 3 - th + j, 1, 1, 11);
            }
        }
    }
    q_rp(0, G, QW, H - G, biome == QB_COAST ? 15u : biome == QB_RUINS ? 5u : 4u);       /* the ground */
    q_rp(0, G, QW, 2, biome == QB_COAST ? 10u : biome == QB_RUINS ? 6u : 11u);
    if (biome == QB_FOREST || biome == QB_NIGHT || biome == QB_STORM) q_rp(0, G + 2, QW, 1, 3);
    for (x = -6; x < QW; x += 6) {
        int32_t k = (x + sp - ((x + sp) % 6 + 6) % 6) / 6, bx = k * 6 - sp;
        uint32_t v = qhash((uint32_t)k * 3u) % 100u;
        if (v > 55 && biome != QB_COAST) q_rp(bx, G - 1, 1, 1, biome == QB_RUINS ? 6u : 11u);
        if (v > 30 && v < 50) q_rp(bx + 2, G + 5 + (int32_t)(qhash((uint32_t)k + 2u) % (uint32_t)(H - G - 7)), 2, 1, biome == QB_RUINS ? 4u : 0u);
        if (flowers && v > 78) q_rp(bx, G - 2, 1, 2, 3), q_rp(bx, G - 3, 1, 1, qhash((uint32_t)k + 4u) % 2u ? 14u : 10u);
    }
    if (birds)
        for (i = 0; i < 3; i++) {
            int32_t bx = ((i * 90 + (int32_t)(qs.idle_q8 >> 4) * 22 / 16) % 280) - 20, by = 12 + i * 6 + qsin((int32_t)(qs.idle_q8 >> 3) + i * 9) * 2 / 127, fp = (int32_t)(qs.idle_q8 >> 6) & 1;
            q_rp(bx, by + fp, 1, 1, 0), q_rp(bx + 1, by + 1, 1, 1, 0), q_rp(bx + 2, by + fp, 1, 1, 0);
        }
}

/* ---- the fire, the sleeping z's ---- */
static void q_fire(int32_t x, int32_t g)
{
    int32_t f = (int32_t)(qs.idle_q8 >> 5) % 3, dx;
    for (dx = -12; dx <= 12; dx += 2) q_rp(x + dx, g, 1, 1, 9);
    q_rp(x - 6, g - 2, 12, 2, 4), q_rp(x - 3, g - 7, 7, 5, 8), q_rp(x - 2, g - 9 - (f == 1), 5, 6, 9);
    q_rp(x - 1, g - 11 - (f == 2), 3, 5, 10), q_rp(x, g - 12 - f, 1, 2, 7), q_rp(x - 3 + f * 3, g - 15 - f * 2, 1, 1, 9);
}
static void q_zz(int32_t x, int32_t y, int32_t n)
{
    int32_t k;
    for (k = 0; k < 2; k++) {
        int32_t p = (((int32_t)(qs.idle_q8 >> 3) * 7 / 10 + k * 128 + n * 59) & 255), zx = x + p * 8 / 256, zy = y - p * 14 / 256;
        q_rp(zx, zy, 3, 1, 7), q_rp(zx + 1, zy + 1, 1, 1, 7), q_rp(zx, zy + 2, 3, 1, 7);
    }
}

/* ---- the world's weather from the sends (README: DST lightning +atk, CHO mist +def, DLY echo +spd, REV rain regen) ---- */
typedef struct { int32_t rain, mist, echo, dst, any; uint32_t buffcol; } q_fx_t;
static q_fx_t q_fx_now(void)
{
    q_fx_t f = {0, 0, 0, 0, 0, 11};
    uint32_t i;
    for (i = 0; i < NTRK; i++) {
        f.rain = q_max(f.rain, trk[i].p[P_REV]), f.mist = q_max(f.mist, trk[i].p[P_CHOR]);
        f.echo = q_max(f.echo, trk[i].p[P_DLY]), f.dst = q_max(f.dst, trk[i].p[P_DIST]);
    }
    f.any = q_max(q_max(f.rain, f.mist), q_max(f.echo, f.dst)) > 10;
    f.buffcol = f.rain >= f.mist && f.rain >= f.echo && f.rain >= f.dst ? 11u : f.mist >= f.echo && f.mist >= f.dst ? 12u : f.dst >= f.echo ? 8u : 9u;
    return f;
}
static void q_weather_over(const q_fx_t *f, int32_t H, int32_t beat_q8)
{
    int32_t G = H - 14, x, y;
    if (f->dst > 10 && (beat_q8 & 1023) < 46) {              /* lightning (every bar, 0.18 beat) */
        int32_t lx = 40 + (int32_t)(qhash((uint32_t)(beat_q8 >> 10)) % 160u), ly;
        q_rp(0, 0, QW, G - 10, 6);
        for (ly = 0; ly < G - 26; ly += 3) { q_rp(lx, ly, 1, 3, 7); lx += (qhash((uint32_t)(ly + (beat_q8 >> 10))) & 1u ? 1 : -1) * 2; }
    }
    if (f->mist > 10)
        for (y = G - 24; y < G - 6; y++)
            for (x = (y & 1); x < QW; x += 2)
                if ((int32_t)(qhash((uint32_t)((x + (int32_t)(qs.idle_q8 >> 5)) / 5) * 13u + (uint32_t)y) % 100u) < f->mist * 60 / 127)
                    cv_pset(x, y, QP(6));
    if (f->rain > 10) {
        int32_t n = 70 * f->rain / 127, d;
        for (d = 0; d < n; d++) {
            y = (int32_t)((qhash((uint32_t)d + 300u) % (uint32_t)H + (qs.idle_q8 >> 1) * 3u / 2u) % (uint32_t)H);
            x = (int32_t)(((qhash((uint32_t)d) % QW) + QW * 4 - (uint32_t)y * 3 / 10) % QW);
            q_rp(x, y, 1, 3, 12);
            if (y > G - 3) q_rp(x - 1, G - 1, 3, 1, 6);
        }
    }
}

/* ---- the world scene (README: PARTY, DUNGEON, SPELLS, BIOME, WEATHER share it) ---- */
typedef struct {
    uint8_t biome_mode;     /* 0 follows the key, else QB_x + 1 */
    uint8_t h, walk, mon, att, hp, loop, flowers, birds, buff, echo, weather;
} q_scn_t;
enum { QSC_PARTY, QSC_STEPS, QSC_PUNCH, QSC_KEY, QSC_FX };
static const q_scn_t QSCN[] = {
    /* party  */ {0, 84, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1},
    /* steps  */ {QB_CAVE + 1, 84, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    /* punch  */ {0, 84, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1},
    /* key    */ {0, 84, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0},
    /* fx     */ {QB_STORM + 1, 84, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1},
};

typedef struct { int32_t k, mx, kind, lift, st; } q_mon_t;

/* the party walking, the monsters coming, the attacks */
static void q_scene_world(uint32_t key)
{
    const q_scn_t *c = &QSCN[key];
    uint32_t biome = c->biome_mode ? (uint32_t)c->biome_mode - 1u : q_biome_of_key(), playing = song.playing, i, nm = 0;
    int32_t H = c->h, G = H - 14, beat_q8 = q_beat_q8(), sf_q8 = beat_q8 * 4, sp = (int32_t)(qs.sp_q8 >> 8), ph[4], tx = 0, ty = 0, have_t = 0;
    int32_t per = 62, die = 124, wx, wy = 0, wframe = -1, k0;
    q_mon_t mon[10];
    q_fx_t fx = q_fx_now();
    if (c->loop && punch.req >= 0 && punch.req <= 3)                       /* README: with loop on, the scene rewinds every bar */
        sp = 300 + ((beat_q8 & 1023) * 13200 / 1024) / 10;
    cv_begin(QW, (uint32_t)H, QP(0));
    q_world(biome, sp, H, c->flowers, c->birds);
    for (i = 0; i < 4; i++) ph[i] = QNONE;
    if (c->mon) {                                                          /* the stream: slime, slime, bat, 62 px apart */
        k0 = (sp - 300) / per - 2;
        for (i = 0; i < 9u && nm < 10u; i++) {
            int32_t k = k0 + (int32_t)i, mx = k * per - sp + 300, kind = (((k % 3) + 3) % 3) == 2, lift, st;
            if (mx > QW + 20 || mx < die - 20) continue;
            q_hop(beat_q8 + k * 95, 4, &lift, &st);
            mon[nm].k = k, mon[nm].mx = mx, mon[nm].kind = kind, mon[nm].lift = lift, mon[nm].st = st, nm++;
        }
        {   /* nobody attacks without a living monster in view; all aim at the nearest (README) */
            int32_t best = 0x7fffffff;
            for (i = 0; i < nm; i++)
                if (mon[i].mx >= die && mon[i].mx < QW - 16 && mon[i].mx < best) {
                    best = mon[i].mx, have_t = 1, tx = mon[i].mx + 12;
                    ty = mon[i].kind ? G - 40 + 6 : G - 8 - mon[i].lift;
                }
        }
    }
    if (c->att && playing && have_t)
        for (i = 0; i < 4; i++) ph[i] = trk_silent(&trk[i]) ? QNONE : q_aph_pat(&trk[i], sf_q8);
    if (have_t && tx - 12 - (Q_PX[3] + 16) > 44) ph[3] = QNONE;           /* the warrior only goes out for a monster within 44 px */
    if (c->echo && fx.echo > 10)                                           /* delay: echo ghosts */
        for (i = 0; i < 4; i++) {
            static const uint8_t ORD[4] = {2, 1, 0, 3};
            uint32_t t = ORD[i];
            q_spr(Q_HERO[t] + (t == 0 ? 0u : ((uint32_t)(beat_q8 >> 7) + t + 1u) % 2u), Q_PX[t] - 8, G - 24 + (t == 0 ? -4 : 0), 1, 0, 13);
        }
    for (i = 0; i < nm; i++) {                                             /* the monsters */
        q_mon_t *m = &mon[i];
        if (m->mx >= die) {
            int32_t hit = have_t && m->mx + 12 == tx && (q_in(ph[3], 0, 25) || q_in(ph[0], 62, 76) || q_in(ph[1], 38, 52)), top;
            if (m->kind) {
                int32_t by = G - 40 + qsin((int32_t)(qs.idle_q8 >> 4) + m->k * 7) * 2 / 127;
                q_spr((((beat_q8 >> 7) + m->k) & 1) ? QS_BAT_1 : QS_BAT_0, m->mx, by, 1, 0, hit ? 7 : -1);
                top = by - 5;
            } else {
                q_spr((m->st == 0 ? QS_SLIME_AIR_0 : m->st == 1 ? QS_SLIME_REST_0 : QS_SLIME_LAND_0) + (uint32_t)hit, m->mx + 11, G - m->lift, 1, 0, -1);
                top = G - 22 - m->lift;
            }
            if (c->hp) {                                                   /* the bar shrinks with proximity, not with damage (README) */
                int32_t hpv = q_max(8, q_min(100, (m->mx - die) * 100 / (QW - 30 - die)));
                q_rp(m->mx, top, 24, 3, 0), q_rp(m->mx + 1, top + 1, 22 * hpv / 100, 1, 8);
            }
        } else {                                                           /* a poof */
            int32_t age = die - m->mx, a;
            for (a = 0; a < 10; a++) {
                int32_t r = age * 11 / 10;
                q_rp(m->mx + 12 + r * qcos(a * 6) / 127, (m->kind ? G - 34 : G - 12) + r * qsin(a * 6) / 127, age < 9 ? 2 : 1, age < 9 ? 2 : 1, age < 10 ? 7u : 6u);
            }
        }
    }
    wx = Q_PX[3];
    if (have_t && ph[3] != QNONE) {                                        /* warrior: runs to the monster, strikes up close, jumps back (9 px arc) */
        int32_t appr = q_max(Q_PX[3], q_min(tx - 12 - 34, Q_PX[3] + 72)), p = ph[3];
        if (p < 0) { int32_t q = 256 + p * 2; wx = Q_PX[3] + (appr - Q_PX[3]) * q_max(0, q_min(256, q)) / 256; wframe = (int32_t)((fm1_ms / 70u) & 1u); }
        else if (p < 77) wx = appr;
        else if (p < 192) { int32_t q = (p - 77) * 256 / 115; wx = appr + (Q_PX[3] - appr) * q / 256; wy = -(9 * qsin(q >> 3) / 127); }
    }
    if (have_t && c->att) {                                                 /* he backs off when a monster gets too close */
        int32_t gap = tx - 12 - (wx + 16);
        if (gap < 6 && (ph[3] == QNONE || ph[3] >= QP100(75))) { int32_t b = 3 * qsin((int32_t)(fm1_ms >> 5)) / 127; wx -= q_min(16, 6 - gap); wy = -(b < 0 ? -b : b); }
    }
    {
        static const uint8_t ORDER[4] = {2, 1, 0, 3};
        uint32_t o;
        for (o = 0; o < 4; o++) {
            uint32_t t = ORDER[o], pose = Q_HERO[t], fr = (playing && c->walk) ? ((uint32_t)(beat_q8 >> 7) + t) & 1u : 0u;
            int32_t x = Q_PX[t], y = G - 24, p = ph[t];
            if (t == 3) {
                x = wx, y += wy;
                if (wframe >= 0) fr = (uint32_t)wframe;
                pose = (p == QNONE || p >= QP100(60)) ? QS_WARRIOR_IDLE_0 : p < 0 ? QS_WARRIOR_ANTIC_0 : p < QP100(25) ? QS_WARRIOR_STRIKE_0 : QS_WARRIOR_FOLLOW_0;
            } else if (t == 0) {
                pose = (p != QNONE && p > -QP100(40) && p < QP100(60)) ? QS_MAGE_CAST_0 : QS_MAGE_IDLE_0;
                y += -4 + qsin((int32_t)(qs.idle_q8 >> 3)) * 3 / 254;
                fr = 0;
            } else if (t == 1) pose = (p != QNONE && p < 0) ? QS_ARCHER_DRAW_0 : QS_ARCHER_IDLE_0;
            else pose = ((p != QNONE && p > -QP100(30) && p < 256) || (c->buff && fx.any && playing)) ? QS_CLERIC_CAST_0 : QS_CLERIC_IDLE_0;
            if (t == 3 && wframe >= 0) {                                    /* speed lines */
                uint32_t sl;
                for (sl = 0; sl < 5; sl++) q_rp(x - 4 - (int32_t)(qhash(sl) % 16u), y + 6 + (int32_t)sl * 3, 6 + (int32_t)(qhash(sl + 2u) % 8u), 1, 12);
            }
            q_spr(pose + fr, x, y, 1, 0, -1);
        }
    }
    if (c->att && have_t) {                                                 /* the attack effects */
        int32_t p;
        p = ph[3];
        if (p != QNONE && p >= 0 && p < QP100(60)) {                       /* crescent smear + contact spark */
            int32_t prog = p < QP100(25) ? p * 256 / QP100(25) : 256, th = p < QP100(25) ? 4 : q_max(1, 4 * (256 - (p - QP100(25)) * 256 / QP100(35)) / 256);
            q_swoosh(wx + 10, G - 24 + wy + 12, 8, th, prog, 12, 7, 0);
            if (p < QP100(15)) {
                int32_t sx = tx - 10, sy = ty, n;
                q_rp(sx - 5, sy, 11, 1, 7), q_rp(sx, sy - 5, 1, 11, 7);
                for (n = -3; n <= 3; n++) q_rp(sx + n, sy + n, 1, 1, 10), q_rp(sx + n, sy - n, 1, 1, 10);
            }
        }
        p = ph[0];
        if (p != QNONE && p >= 0 && p < QP100(90)) {                       /* mage: summons the fireball above her, then it falls onto the target */
            int32_t sx = Q_PX[0] + 18 - 6, sy = G - 24 - 4 + 10 - 26;
            if (p < QP100(35)) q_summon(sx, sy + qsin(p / 2), q_min(256, p * 256 / QP100(30)), 10);
            else if (p < QP100(62)) { int32_t q = (p - QP100(35)) * 256 / QP100(27), e = q * q / 256; q_meteor(sx + (tx - sx) * e / 256, sy + (ty - sy) * e / 256, 16); }
            else { static const uint8_t C4[4] = {10, 9, 8, 7}; q_burst(tx, ty, (p - QP100(62)) * 256 / QP100(30), C4, 4); }
        }
        p = ph[1];
        if (p != QNONE && p >= 0 && p < QP100(50)) {                       /* archer: the arrow flies on a parabola (peak 22 px) */
            int32_t ax = Q_PX[1] + 19, ay = G - 24 + 12;
            if (p < QP100(40)) q_arc_arrow(ax, ay, tx, ty, p * 256 / QP100(40), 22, 7, 12);
            else { static const uint8_t C2[2] = {7, 12}; q_burst(tx, ty, (p - QP100(40)) * 256 / QP100(15) / 2, C2, 2); }
        }
    }
    if (fx.any && (c->buff || key == QSC_FX)) {                             /* cleric: aura rings and up-arrows on every hero, coloured by the strongest buff */
        int32_t p = key == QSC_FX ? ((beat_q8 & 1023) * 1100 / 1024 - 50) * 256 / 1000 : ph[2];
        if (p != QNONE && p >= 0 && p < 256) {
            uint32_t h2;
            for (h2 = 0; h2 < 4; h2++) {
                int32_t cx = Q_PX[h2] + 8, r = 6 + p * 8 / 256, a, ay = G - 30 - p * 8 / 256;
                for (a = 0; a < 21; a++)
                    if ((a + p / 25) % 2 == 0) q_rp(cx + r * qcos(a * 3) / 127, G + r * qsin(a * 3) * 3 / 1270, 1, 1, fx.buffcol);
                q_rp(cx, ay, 1, 4, fx.buffcol), q_rp(cx - 1, ay + 1, 3, 1, fx.buffcol);
            }
        }
    }
    if (c->hp) {                                                            /* HP bars under the heroes: the track's level */
        for (i = 0; i < 4; i++) {
            int32_t lvl = i == TRK_DRUM ? song.g[G_DRLVL] : trk[i].p[P_LEVEL];
            q_rp(Q_PX[i] + 2, G + 4, 13, 3, 0), q_rp(Q_PX[i] + 3, G + 5, 11 * lvl / 127, 1, 11);
        }
    }
    if (c->loop && punch.req >= 0 && punch.req <= 3) {                      /* loop: the mage's orbit, a glitch line at every bar */
        int32_t cx = Q_PX[0] + 8, cy = G - 14, k;
        for (k = 0; k < 12; k++) {
            int32_t a = (int32_t)(qs.idle_q8 >> 3) + k * 5;
            q_rp(cx + 16 * qcos(a) / 127, cy + 8 * qsin(a) / 127, 1, 1, k & 1 ? 12u : 13u);
        }
        if ((beat_q8 & 1023) < 35)
            for (k = 0; k < 6; k++) q_rp(0, (int32_t)(qhash((uint32_t)(k + (qs.idle_q8 >> 4))) % (uint32_t)H), QW, 1, 13);
    }
    if (c->weather) q_weather_over(&fx, H, beat_q8);
}

/* ---- single-purpose scenes ---- */
static void q_scene_kit(void)           /* the chest and its floating items (README: KIT) */
{
    int32_t G = 70, cx = 150, cy = G - 12, k, t = (int32_t)(qs.idle_q8 >> 4);
    static const uint8_t IT[4] = {12, 11, 10, 9};
    cv_begin(QW, 84, QP(0));
    q_world(q_biome_of_key(), 0, 84, 0, 0);
    q_rp(cx, cy, 20, 12, 0), q_rp(cx + 1, cy + 1, 18, 10, 4), q_rp(cx + 1, cy + 5, 18, 1, 10), q_rp(cx + 9, cy + 4, 2, 3, 10);
    q_rp(cx, cy - 6, 20, 5, 0), q_rp(cx + 1, cy - 5, 18, 3, 4);
    for (k = 0; k < 4; k++) {
        int32_t ix = cx + 2 + k * 9 / 2, iy = cy - 16 - qsin(t + k * 8) * 2 / 127 - (k % 2) * 4;
        q_rp(ix - 1, iy - 1, 5, 5, 0), q_rp(ix, iy, 3, 3, IT[k]);
    }
    for (k = 0; k < 4; k++)
        if ((int32_t)(qhash((uint32_t)(k + (qs.idle_q8 >> 6))) & 1u)) q_rp(cx + (int32_t)(qhash((uint32_t)k * 7u) % 20u), cy - 26 - (int32_t)(qhash((uint32_t)k * 3u) % 8u), 1, 1, 7);
}
static void q_scene_camp(void)          /* the night camp: muted heroes rest at the fire, the others fight a bat (README: CAMP) */
{
    int32_t G = 70, beat_q8 = q_beat_q8(), sf_q8 = beat_q8 * 4, n, bx = 184, by = G - 40 + qsin((int32_t)(qs.idle_q8 >> 3)) * 3 / 127, tx = bx + 12, ty = by + 6, ph[4], fighters = 0;
    static const int32_t SLOTS[4] = {82, 106, 132, 156}, SIT[3] = {20, 2, 62};
    uint32_t t, sitters = 0;
    q_fx_t fx = q_fx_now();
    cv_begin(QW, 84, QP(0));
    q_world(QB_NIGHT, 0, 84, 0, 0);
    q_fire(46, G);
    for (t = 0; t < 4; t++) ph[t] = (song.playing && !trk_silent(&trk[t])) ? q_aph_pat(&trk[t], sf_q8) : QNONE;
    for (t = 0; t < 4; t++) if (trk_silent(&trk[t])) {                    /* resting: seated by the fire, z's */
        int32_t x = SIT[sitters < 3u ? sitters : 2u];
        q_spr(Q_SEAT[t], x, G - 24 + 4, 1, 0, -1), q_zz(x + 10, G - 22, (int32_t)sitters);
        sitters++;
    }
    for (t = 0; t < 4; t++) if (!trk_silent(&trk[t])) fighters++;
    if (fighters) q_spr(((beat_q8 >> 7) & 1) ? QS_BAT_1 : QS_BAT_0, bx, by, 1, 0, (q_in(ph[0], 62, 76) || q_in(ph[3], 0, 25)) ? 7 : -1);
    n = 0;
    for (t = 4; t-- > 0;) if (!trk_silent(&trk[t])) {                     /* fighters, from the fire to the right */
        int32_t x = SLOTS[n], y = G - 24, p = ph[t];
        uint32_t pose = Q_HERO[t], fr = song.playing ? ((uint32_t)(beat_q8 >> 7) + (uint32_t)n) & 1u : 0u;
        if (t == 0) { pose = (p != QNONE && p > -QP100(40) && p < QP100(60)) ? QS_MAGE_CAST_0 : QS_MAGE_IDLE_0; y += -4 + qsin((int32_t)(qs.idle_q8 >> 3)) * 3 / 254; fr = 0; }
        else if (t == 1) pose = (p != QNONE && p < 0) ? QS_ARCHER_DRAW_0 : QS_ARCHER_IDLE_0;
        else if (t == 2) pose = (p != QNONE && p > -QP100(30) && p < 256) ? QS_CLERIC_CAST_0 : QS_CLERIC_IDLE_0;
        else pose = (p == QNONE || p >= QP100(60)) ? QS_WARRIOR_IDLE_0 : p < 0 ? QS_WARRIOR_ANTIC_0 : p < QP100(25) ? QS_WARRIOR_STRIKE_0 : QS_WARRIOR_FOLLOW_0;
        q_spr(pose + fr, x, y, 1, 0, -1);
        if (t == 0 && p != QNONE && p >= 0 && p < QP100(90)) {              /* the fireball */
            int32_t sx = x + 12, sy = y - 10;
            if (p < QP100(35)) q_summon(sx, sy, q_min(256, p * 256 / QP100(30)), 10);
            else if (p < QP100(62)) { int32_t q = (p - QP100(35)) * 256 / QP100(27), e = q * q / 256; q_meteor(sx + (tx - sx) * e / 256, sy + (ty - sy) * e / 256, 16); }
            else { static const uint8_t C4[4] = {10, 9, 8, 7}; q_burst(tx, ty, (p - QP100(62)) * 256 / QP100(30), C4, 4); }
        }
        if (t == 1 && p != QNONE && p >= 0 && p < QP100(40)) q_arc_arrow(x + 19, y + 12, tx, ty, p * 256 / QP100(40), 22, 7, 12);
        if (t == 3 && p != QNONE && p >= 0 && p < QP100(60)) q_swoosh(x + 10, y + 12, 8, p < QP100(25) ? 4 : 2, p < QP100(25) ? p * 256 / QP100(25) : 256, 12, 7, 0);
        n++;
    }
    q_weather_over(&fx, 84, beat_q8);
}
static void q_scene_menu(void)          /* the inn: the party sleeps around the fire (README: MENU) */
{
    static const int32_t X[4] = {136, 80, 52, 164};                           /* mage, archer, cleric, warrior */
    int32_t G = 46, t;
    cv_begin(QW, 60, QP(0));
    q_world(QB_NIGHT, 0, 60, 0, 0);
    q_fire(120, G);
    for (t = 0; t < 4; t++) { q_spr(Q_ROT[t], X[t], G - 16, 1, 0, -1), q_zz(X[t] + 6, G - 18, t); }
}
static void q_scene_rec(int count)      /* rec ready / count-in: a King Slime blocks the way (README) */
{
    int32_t beat_q8 = q_beat_q8(), lift, st, sq, G = 70;
    q_hop(beat_q8, count ? 5 : 3, &lift, &st);
    sq = st == 0 ? -31 : st == 2 ? 56 : 0;
    cv_begin(QW, 84, QP(0));
    q_world(q_biome_of_key(), (int32_t)(qs.sp_q8 >> 8), 84, 0, 0);
    q_spr(QS_MAGE_IDLE_0, Q_PX[0], G - 24 - 4, 1, 0, -1), q_spr(QS_ARCHER_IDLE_0, Q_PX[1], G - 24, 1, 0, -1);
    q_spr(QS_CLERIC_IDLE_0, Q_PX[2], G - 24, 1, 0, -1), q_spr(QS_WARRIOR_IDLE_0, Q_PX[3], G - 24, 1, 0, -1);
    q_slime(180, G, 44, 32, 12, 1, sq, lift, 0, -1, 1, 1, 0, 0);
    if (!count && (beat_q8 & 255) < 179) {                                   /* the "!" bubble */
        int32_t x = 202, y = G - 32 - 14 - lift;
        q_rp(x, y, 7, 11, 0), q_rp(x + 1, y + 1, 5, 9, 7), q_rp(x + 3, y + 2, 1, 5, 8), q_rp(x + 3, y + 8, 1, 1, 8);
    }
    if (count && (beat_q8 & 255) < 31) {                                    /* a white frame flash on every click */
        q_rp(0, 0, QW, 2, 7), q_rp(0, 82, QW, 2, 7), q_rp(0, 0, 2, 84, 7), q_rp(238, 0, 2, 84, 7);
    }
}
static void q_scene_flush(void) { cv_blit(0, 19); }          /* the scenes draw into the canvas; the screen adds its overlay, then flushes */
#include "ui_quest_scenes.c"
#include "ui_quest_screens.c"
