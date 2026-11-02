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
#include "felucca_qsprites.h"
#include "felucca_qrec.h"
#include "felucca_qgames.h"
#include "felucca_qult.h"
#include "felucca_qgob.h"
#include "felucca_qmaga.h"
static uint32_t str_hash(uint32_t h, const char *s);   /* ui_draw.c */

/* ---- the TAVERN palette (README) ---- */
static const uint16_t QPAL_BASE[16] = {
    RGB(0x1a, 0x10, 0x16), RGB(0x2b, 0x23, 0x38), RGB(0x5a, 0x30, 0x50), RGB(0x2f, 0x4a, 0x3a),
    RGB(0x6a, 0x40, 0x30), RGB(0x4a, 0x3c, 0x48), RGB(0xa8, 0x9a, 0xa0), RGB(0xf2, 0xe2, 0xc4),
    RGB(0xc8, 0x44, 0x3c), RGB(0xe0, 0x8a, 0x3c), RGB(0xf0, 0xc8, 0x60), RGB(0x6a, 0x9a, 0x4a),
    RGB(0x4a, 0x78, 0xb0), RGB(0x8a, 0x6a, 0x9a), RGB(0xe0, 0x90, 0x90), RGB(0xf0, 0xb8, 0x90)};
#define CD_T 64
static uint8_t cd_tex[CD_T][CD_T] __attribute__((section(".pool")));   /* 4 KB of the pool, shared: the CDs (ui_quest_cd.c: HOME, no menu) and the games' cover decode (ui_quest_pages.c: the games menu) are never up together */
static uint16_t QPAL[16];                                  /* what the scenes paint with: the TAVERN palette through the colour filter (q_filter_set) */
#define QP(i) QPAL[(i) & 15u]

/* ---- the colour filter (menu FILTER): every scene colour goes through a few tones of one hue (the lab's "mapa de poucas cores"); warm bright pixels become the orange light, so
 * what is lit, hit or played still stands out. The UI and the heroes are drawn with their own colours: not filtered. 0 is the default: no filter, except the village at night (violet). ---- */
typedef struct { uint8_t n, ng, nl, vig; uint16_t t[5], g[2]; } q_flt_t;       /* tones (dark -> light), pale greens, no light accent, vignette */
static const q_flt_t Q_FLT[12] = {
    {0},                                                                                                                       /* 0: the default (7 and 8 by the hour) */
    {0},                                                                                                                       /* 1 TAVERN 16: as drawn */
    {4, 0, 1, 0, {RGB(0x0f, 0x38, 0x0f), RGB(0x30, 0x62, 0x30), RGB(0x8b, 0xac, 0x0f), RGB(0x9b, 0xbc, 0x0f)}},               /* 2 Game Boy */
    {2, 0, 1, 0, {RGB(0x10, 0x10, 0x10), RGB(0xf4, 0xf4, 0xf4)}},                                                             /* 3 1 bit */
    {0, 0, 0, 36},                                                                                                             /* 4 VHS: colours as drawn, a heavy vignette */
    {5, 0, 1, 0, {RGB(0x00, 0x00, 0x00), RGB(0x30, 0x30, 0x30), RGB(0x70, 0x70, 0x70), RGB(0xb8, 0xb8, 0xb8), RGB(0xf0, 0xf0, 0xf0)}},   /* 5 Noir */
    {5, 0, 1, 0, {RGB(0x1c, 0x12, 0x0c), RGB(0x4a, 0x33, 0x20), RGB(0x7a, 0x5a, 0x38), RGB(0xb0, 0x8a, 0x5a), RGB(0xe8, 0xd0, 0xa0)}},   /* 6 Sepia */
    {4, 0, 0, 0, {RGB(0x0f, 0x12, 0x14), RGB(0x14, 0x70, 0x7a), RGB(0x27, 0xc3, 0xcc), RGB(0xf4, 0xff, 0xff)}},               /* 7 Ruins */
    {4, 2, 0, 0, {RGB(0x1c, 0x10, 0x33), RGB(0x3a, 0x2a, 0x6a), RGB(0x5a, 0x4a, 0x98), RGB(0x8a, 0x7a, 0xc8)}, {RGB(0x3f, 0x8a, 0x6a), RGB(0x9a, 0xd8, 0xa0)}},   /* 8 Violet night */
    {4, 0, 0, 0, {RGB(0x3e, 0x14, 0x1e), RGB(0x2e, 0x44, 0x10), RGB(0x5e, 0x82, 0x18), RGB(0xa8, 0xd0, 0x3a)}},               /* 9 Forest */
    {5, 0, 0, 0, {RGB(0x00, 0x00, 0x00), RGB(0x3a, 0x2a, 0x1a), RGB(0x6a, 0x52, 0x38), RGB(0x94, 0x8c, 0x78), RGB(0xc8, 0xc0, 0xa8)}},   /* 10 Castle */
    {5, 0, 0, 0, {RGB(0x08, 0x08, 0x0e), RGB(0x1a, 0x1e, 0x28), RGB(0x2c, 0x30, 0x3a), RGB(0x46, 0x48, 0x50), RGB(0x6a, 0x6a, 0x74)}},   /* 11 Night city */
};
static const char *const Q_FLT_NAME[12] = {"Default", "TAVERN 16", "Game Boy", "1 bit", "VHS", "Noir", "Sepia", "Ruins", "Violet night", "Forest", "Castle", "Night city"};
static const uint16_t Q_LIGHT[3] = {RGB(0xc8, 0x56, 0x1a), RGB(0xf0, 0x8a, 0x30), RGB(0xff, 0xc4, 0x60)};
static UI_NOINLINE uint16_t q_flt_c(uint16_t c, uint32_t fi)
{
    const q_flt_t *f = &Q_FLT[fi % 12u];
    int32_t r = (int32_t)((c >> 11) & 31u) << 3, g = (int32_t)((c >> 5) & 63u) << 2, b = (int32_t)(c & 31u) << 3, mx, mn, l, lum;
    if (!f->n) return c;
    mx = r > g ? (r > b ? r : b) : (g > b ? g : b), mn = r < g ? (r < b ? r : b) : (g < b ? g : b), l = (mx + mn) >> 1;
    if (mx - mn > 50 && mx > 220 && !f->nl) {                                           /* coloured and bright: a light, or the pale green of the leaves */
        if (f->ng && g > r && g > b) return f->g[l > 170];
        if (r > b && g >= b) return Q_LIGHT[l > 190 ? 2 : l > 140 ? 1 : 0];
    }
    lum = (r * 77 + g * 150 + b * 29) >> 8;
    return f->t[(lum * f->n) >> 8];
}
static UI_NOINLINE void q_filter_set(uint32_t fi)                                       /* the scenes' palette for a filter (0: the default, ruins) */
{
    uint32_t i;
    for (i = 0; i < 16u; i++) QPAL[i] = q_flt_c(QPAL_BASE[i], fi);                        /* (0 and 1: as drawn) */
}

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
static uint16_t Q_TRK[4] = {RGB(0xf0, 0xc8, 0x60), RGB(0x7a, 0xb0, 0x4e), RGB(0xe0, 0x8a, 0x3a), RGB(0x5a, 0x8a, 0xd0)};      /* mage yellow, archer green, cleric orange, knight blue */
static uint16_t Q_TRK_DIM[4] = {RGB(0x7a, 0x64, 0x20), RGB(0x3a, 0x5a, 0x28), RGB(0x7a, 0x4a, 0x20), RGB(0x2a, 0x3a, 0x63)};
static const uint8_t Q_HERO[4] = {QS_MAGE_IDLE_0, QS_ARCHER_IDLE_0, QS_CLERIC_IDLE_0, QS_WARRIOR_IDLE_0};   /* track -> hero (README) */
static const uint8_t Q_ROT[4] = {QS_MAGE_ROT, QS_ARCHER_ROT, QS_CLERIC_ROT, QS_WARRIOR_ROT};
static const uint8_t Q_SEAT[4] = {QS_MAGE_SEAT, QS_ARCHER_SEAT, QS_CLERIC_SEAT, QS_WARRIOR_SEAT};
static const char *const Q_HERO_NAME[4] = {"mage", "archer", "cleric", "knight"};
static const int32_t Q_PX[4] = {62, 36, 12, 88};         /* hero slots by track: mage, archer, cleric, warrior (README) */

/* sin(2 pi k / 64) * 127 */
static const int8_t Q_SIN[64] = {0, 12, 25, 37, 49, 60, 71, 81, 90, 98, 106, 112, 117, 122, 125, 126, 127, 126, 125, 122, 117, 112, 106,
                                 98, 90, 81, 71, 60, 49, 37, 25, 12, 0, -12, -25, -37, -49, -60, -71, -81, -90, -98, -106, -112, -117,
                                 -122, -125, -126, -127, -126, -125, -122, -117, -112, -106, -98, -90, -81, -71, -60, -49, -37, -25, -12};
static UI_NOINLINE int32_t qsin(int32_t a) { return Q_SIN[a & 63]; }
static UI_NOINLINE int32_t qcos(int32_t a) { return Q_SIN[(a + 16) & 63]; }
static UI_NOINLINE uint32_t qhash(uint32_t n)       /* the scene's deterministic noise */
{
    n = n * 2654435761u + 0x9E3779B9u;
    n ^= n >> 15;
    n *= 2246822519u;
    n ^= n >> 13;
    return n;
}
static UI_NOINLINE int32_t q_isqrt(int32_t v)
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
static UI_NOINLINE int32_t q_atan(int32_t y, int32_t x)              /* degrees 0..359 */
{
    int32_t ax = x < 0 ? -x : x, ay = y < 0 ? -y : y, a;
    if (ax == 0 && ay == 0) return 0;
    a = ax >= ay ? ay * 45 / ax : 90 - ax * 45 / ay;
    if (x < 0) a = 180 - a;
    if (y < 0) a = 360 - a;
    return a % 360;
}
static UI_NOINLINE int32_t q_min(int32_t a, int32_t b) { return a < b ? a : b; }
static UI_NOINLINE int32_t q_max(int32_t a, int32_t b) { return a > b ? a : b; }
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
static uint8_t q_town;         /* the homes: 0 the village (the first), 1 the party, 2 the boss (HOME tapped again on the tracks page cycles them) */

/* ---- drawing helpers ---- */
static uint8_t pk_sbuf[1152];                            /* the sprites' chunk (tools/pack_gfx.py) */
static uint32_t pk_sck = 0xFFFFu;
static UI_NOINLINE const uint8_t *pk_spr(uint32_t id)
{
    uint32_t c = QSPR_CK[id];
    if (pk_sck != c) {
        lz4_unpack(QSPR_DATA + QSPR_ZO[c], QSPR_DATA + QSPR_ZO[c + 1u], pk_sbuf);
        pk_sck = c;
    }
    return pk_sbuf + QSPR[id].off;
}
typedef struct { const uint8_t *p; uint32_t range, code; } qrc_t;
static UI_NOINLINE uint32_t qrc_bit(qrc_t *r, uint16_t *pr)
{
    uint32_t bound = (r->range >> 11) * *pr, b;
    if (r->code < bound) { r->range = bound; *pr = (uint16_t)(*pr + ((2048u - *pr) >> 5)); b = 0; }
    else { r->code -= bound; r->range -= bound; *pr = (uint16_t)(*pr - (*pr >> 5)); b = 1; }
    while (r->range < (1u << 24)) { r->range <<= 8; r->code = (r->code << 8) | *r->p++; }
    return b;
}
static uint8_t gob_px[680];                      /* the goblin frame on screen (kind * 32 + frame, 255 = none) */
static uint8_t gob_id = 255;
static void q_goblin(int32_t cx, int32_t gy, uint32_t k, uint32_t fr, int flash, int32_t sc)   /* a goblin (k 0 the bow, 1 the axe), facing left, sc pixels a dot; (the one in the map, sc 1, and the boss, 4) */
{
    const uint32_t W = QGOB_W[k], H = QGOB_H[k];
    const uint16_t col[4] = {0, RGB(0x12, 0x10, 0x20), RGB(0x70, 0x6c, 0x90), RGB(0xf0, 0xec, 0xe0)};
    uint32_t x, y;
    if (fr >= QGOB_N) fr = QGOB_N - 1u;
    if (gob_id != k * 32u + fr) {
        qrc_t r;
        uint16_t pr[48];
        uint32_t i;
        r.p = QGOB_Z + QGOB_FO[k][fr], r.range = 0xFFFFFFFFu, r.code = 0;
        for (i = 0; i < 5u; i++) r.code = (r.code << 8) | *r.p++;
        for (i = 0; i < 48u; i++) pr[i] = 1024;
        for (y = 0; y < H; y++)
            for (x = 0; x < W; x++) {
                uint32_t l = x ? gob_px[y * W + x - 1u] : 0u, u = y ? gob_px[(y - 1u) * W + x] : 0u, c = (l * 4u + u) * 3u, v = 0;
                if (qrc_bit(&r, &pr[c])) { v = 1; if (qrc_bit(&r, &pr[c + 1u])) { v = 2; if (qrc_bit(&r, &pr[c + 2u])) v = 3; } }
                gob_px[y * W + x] = (uint8_t)v;
            }
        gob_id = (uint8_t)(k * 32u + fr);
    }
    for (y = 0; y < H; y++) {
        x = 0;
        while (x < W) {
            uint32_t v = gob_px[y * W + x], e = x;
            while (e < W && gob_px[y * W + e] == v) e++;
            if (v) cv_rect(cx + ((int32_t)W / 2 - (int32_t)e) * sc, gy - (int32_t)(H - y) * sc, (int32_t)(e - x) * sc, sc, flash ? RGB(0xff, 0xff, 0xff) : col[v]);
            x = e;
        }
    }
}
#include "ui_quest_fig.c"
static UI_NOINLINE void q_spr(uint32_t id, int32_t x, int32_t y, int32_t s, int flip, int solid)   /* s = scale, solid = palette index or -1 */
{
    if (id <= QS_WARRIOR_ROT) { q_hero_fig(id, x, y, s, flip, solid); return; }      /* the heroes are drawn, not baked */
    const qspr_t *sp = &QSPR[id];
    const uint8_t *d = pk_spr(id), *mk = d + ((uint32_t)sp->w * sp->h + 1u) / 2u;
    int32_t gx, gy, bx = flip ? 16 - sp->ox - sp->w : sp->ox;
    uint32_t n = 0;
    for (gy = 0; gy < sp->h; gy++)
        for (gx = 0; gx < sp->w; gx++, n++) {
            uint32_t v = (d[n >> 1] >> ((n & 1u) ? 0 : 4)) & 15u;
            int32_t px = x + s * (bx + (flip ? sp->w - 1 - gx : gx)), py = y + s * (sp->oy + gy);
            if (!((mk[n >> 3] >> (7u - (n & 7u))) & 1u))
                continue;
            if (s == 1) cv_pset(px, py, QP(solid >= 0 ? (uint32_t)solid : v));
            else cv_rect(px, py, s, s, QP(solid >= 0 ? (uint32_t)solid : v));
        }
}
static UI_NOINLINE void q_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t dy;
    for (dy = -r; dy <= r; dy++) {
        int32_t w = q_isqrt(r * r - dy * dy);
        cv_rect(cx - w, cy + dy, 2 * w + 1, 1, c);
    }
}
static UI_NOINLINE void q_rp(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t i) { cv_rect(x, y, w < 1 ? 1 : w, h < 1 ? 1 : h, QP(i)); }
static UI_NOINLINE void q_tri(int32_t x, int32_t y, uint16_t c)
{
    cv_rect(x, y + 2, 1, 2, c), cv_rect(x + 1, y + 1, 1, 4, c), cv_rect(x + 2, y, 2, 6, c);
    cv_rect(x + 4, y + 1, 1, 4, c), cv_rect(x + 5, y + 2, 1, 2, c);
}
static UI_NOINLINE int32_t q_text(int32_t x, int32_t y, const char *s, uint16_t c) { return cv_text(x, y, &FONT_P, s, c); }
static UI_NOINLINE void q_text_r(int32_t xr, int32_t y, const char *s, uint16_t c) { cv_text(xr - text_w(&FONT_P, s), y, &FONT_P, s, c); }
static UI_NOINLINE void q_text_c(int32_t cx, int32_t y, const char *s, uint16_t c) { cv_text(cx - text_w(&FONT_P, s) / 2, y, &FONT_P, s, c); }
static UI_NOINLINE void q_title(int32_t x, const char *s, uint16_t c) { cv_text(x, -1, &FONT_QT, s, c); }
static UI_NOINLINE void q_title_r(int32_t xr, const char *s, uint16_t c) { cv_text(xr - text_w(&FONT_QT, s), -1, &FONT_QT, s, c); }
static UI_NOINLINE void q_outline_text(int32_t x, int32_t y, const char *s, uint16_t c)       /* cream on a dark outline (README) */
{
    cv_text(x - 1, y, &FONT_P, s, Q_BG), cv_text(x + 1, y, &FONT_P, s, Q_BG), cv_text(x, y - 1, &FONT_P, s, Q_BG), cv_text(x, y + 1, &FONT_P, s, Q_BG);
    cv_text(x, y, &FONT_P, s, c);
}
static UI_NOINLINE void q_panel(int32_t x, int32_t y, int32_t w, int32_t h)     /* wood panel, gold border, inner shadows (README) */
{
    cv_rect(x, y, w, h, Q_GOLD);
    cv_rect(x + 1, y + 1, w - 2, h - 2, Q_BG);
    cv_rect(x + 2, y + 2, w - 4, h - 4, RGB(0x6a, 0x40, 0x30));
    cv_rect(x + 3, y + 3, w - 6, h - 6, Q_WOOD);
}

/* ---- the knob of the design: 20 x 20, ring -135..+135 deg in the knob's colour, cap, 5 px pointer, no rim ---- */
static uint8_t q_ring[20 * 20];                          /* 255 = not ring, else (angle from -135 deg) / 2 */
static uint8_t q_ring_ready;
static UI_NOINLINE void q_knob(int32_t cx, int32_t cy, int32_t ratio, uint16_t col)
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
static UI_NOINLINE int32_t q_aph_pat(const track_t *t, int32_t sf_q8)
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
static UI_NOINLINE int q_in(int32_t p, int32_t a, int32_t b) { return p != QNONE && p >= a * 256 / 100 && p < b * 256 / 100; }
#define QP100(x) ((x) * 256 / 100)

/* ---- the clock the scenes share ---- */
static UI_NOINLINE int32_t q_beat_q8(void)       /* beats, Q8: the transport's, or the idle clock while stopped */
{
    return song.playing ? (int32_t)(clk_beat * 256u + clk_pos / (BEAT_U >> 8)) : (int32_t)qs.idle_q8;
}
static uint32_t q_run_t0;                 /* when PLAY was pressed (the RUN arrow) */
static uint32_t q_idle_seed = 1;          /* chosen each time the fight home is entered: what everybody does when nothing plays */
/* the homes' reference: a little stack of squares in the bar, one for each home (party, village, boss), the open one lit; a button with one screen shows none */
static UI_NOINLINE void q_homedots(int32_t x, uint32_t n, uint32_t cur)
{
    uint32_t k;
    for (k = 0; k < n; k++) cv_rect(x, n > 3u ? 2 + (int32_t)k * 4 : 3 + (int32_t)k * 5, 3, 3, k == cur ? Q_TEXT : Q_OFFHI);
}
static void q_hits_tick(void);            /* ui_quest_boss.c */
/* the notes the four heroes play, as hits (ui_quest_boss.c fills them: the village, the boss and the pads read them) */
static uint32_t qh_ms[4];                       /* when each hero last hit */
static uint8_t qh_note[4], qh_dmg[4];           /* its note and the damage of that hit */
static uint16_t qh_idx[4] = {0xFFFFu, 0xFFFFu, 0xFFFFu, 0xFFFFu};
/* the front line: the warrior holds it; when he rests (level 0) the first hero that is playing (mage, archer, cleric) steps up to his place
 * and he drops back into hers. q_hx are the slots as they walk there. */
static int32_t q_hx[4] = {62, 36, 12, 88};
static UI_NOINLINE uint32_t q_rest(uint32_t i)
{
    int32_t lvl = i == TRK_DRUM ? song.g[G_DRLVL] : trk[i].p[P_LEVEL];
    return trk_silent(&trk[i]) || lvl <= 0;
}
static UI_NOINLINE void q_front_walk(uint32_t dt)
{
    int32_t tgt[4] = {Q_PX[0], Q_PX[1], Q_PX[2], Q_PX[3]};
    uint32_t i, f = 3;
    if (q_rest(3))
        for (i = 0; i < 3u; i++)
            if (!q_rest(i)) { f = i; break; }
    if (f != 3u) tgt[f] = Q_PX[3], tgt[3] = Q_PX[f];
    for (i = 0; i < 4u; i++) {
        int32_t d = tgt[i] - q_hx[i];
        int32_t st = (int32_t)(dt * 150u / 1000u) + 1;          /* ~150 px/s */
        q_hx[i] += d > st ? st : d < -st ? -st : d;
    }
}
static UI_NOINLINE void q_tick_clocks(void)
{
    if (!QPAL[7]) q_filter_set(q_filter);                  /* (the first frame: the palette is filled) */
    uint32_t now = fm1_ms, dt = qs.last_ms ? now - qs.last_ms : 0u;
    qs.last_ms = now;
    if (dt > 250u) dt = 250u;
    if (song.playing)
        qs.sp_q8 += ((uint32_t)song.g[G_BPM] * dt * 231u) >> 12;                        /* bpm * 0.22 px/s in 1/256 px */
    qs.idle_q8 += (uint32_t)song.g[G_BPM] * dt * 256u / 60000u;
    q_front_walk(dt);
    q_hits_tick();
    {   /* PLAY pressed: the "RUN" arrow shows for a moment on the walking screen */
        static uint8_t was;
        if (song.playing && !was) q_run_t0 = now | 1u;
        was = (uint8_t)song.playing;
    }
    {   /* SP: running effects cost it, the cleric heals (every 250 ms while the loop plays) */
        static uint32_t sp_ms;
        if (now - sp_ms >= 250u) {
            uint32_t h, heal = !trk_silent(&trk[2]) ? 6u : 0u;
            sp_ms = now;
            if (song.playing)
                for (h = 0; h < 4u; h++) {
                    int32_t v = (int32_t)q_sp[h] + (int32_t)heal - 3 - (int32_t)fx_active(&trk[h]) * 5;
                    q_sp[h] = (uint8_t)(v < 0 ? 0 : v > 100 ? 100 : v);
                }
        }
    }
}
static UI_NOINLINE void q_hop(int32_t bq, int32_t amp, int32_t *lift, int32_t *st)   /* st: 0 air (stretched), 1 rest, 2 landing (squashed) */
{
    int32_t f = bq & 255;
    *lift = 0, *st = 1;
    if (f < 141) { *lift = (amp * qsin(f * 32 / 141)) / 127; *st = 0; }
    else if (f < 184) *st = 2;
}

/* ---- a slime (README): translucent dome, darker lower band, white highlight, oval eyes; squash and stretch ---- */
static UI_NOINLINE void q_slime(int32_t cx, int32_t gy, int32_t w, int32_t h, uint32_t M, uint32_t D, int32_t sq_q8, int32_t lift, int flash, int solid,
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
static inline void q_meteor(int32_t x, int32_t y, int32_t sc10) { (void)x; (void)y; (void)sc10; }       /* (gone: the mage's meteors) */
static UI_NOINLINE void q_summon(int32_t x, int32_t y, int32_t k_q8, int32_t sc10)          /* k: 0..1 (Q8) */
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
static inline void q_burst(int32_t x, int32_t y, int32_t q_q8, const uint8_t *cols, int32_t nc) { (void)x; (void)y; (void)q_q8; (void)cols; (void)nc; }   /* (gone: the bursts) */

static inline void q_arc_arrow(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t q_q8, int32_t hgt, uint32_t col, int32_t trail) { (void)x0; (void)y0; (void)x1; (void)y1; (void)q_q8; (void)hgt; }   /* (gone: the archer's arrows) */
static UI_NOINLINE void q_swoosh(int32_t cx, int32_t cy, int32_t r, int32_t th, int32_t prog_q8, uint32_t col, uint32_t col2, int flip)   /* crescent smear */
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
static UI_NOINLINE uint32_t q_biome_of_key(void)    /* trk[0] holds the key of the song (every synth part has it): MAJ forest, MIN cave, DOR coast, PHRY ruins */
{
    switch (trk[0].p[P_SCALE]) {
    case 2: return QB_CAVE;
    case 3: return QB_COAST;
    case 8: return QB_RUINS;
    default: return QB_FOREST;
    }
}
static UI_NOINLINE void q_world(uint32_t biome, int32_t sp, int32_t H, int flowers, int birds)
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
static UI_NOINLINE void q_fire(int32_t x, int32_t g)
{
    int32_t f = (int32_t)(qs.idle_q8 >> 5) % 3, dx;
    for (dx = -12; dx <= 12; dx += 2) q_rp(x + dx, g, 1, 1, 9);
    q_rp(x - 6, g - 2, 12, 2, 4), q_rp(x - 3, g - 7, 7, 5, 8), q_rp(x - 2, g - 9 - (f == 1), 5, 6, 9);
    q_rp(x - 1, g - 11 - (f == 2), 3, 5, 10), q_rp(x, g - 12 - f, 1, 2, 7), q_rp(x - 3 + f * 3, g - 15 - f * 2, 1, 1, 9);
}
static UI_NOINLINE void q_zz(int32_t x, int32_t y, int32_t n)
{
    int32_t k;
    for (k = 0; k < 2; k++) {
        int32_t p = (((int32_t)(qs.idle_q8 >> 3) * 7 / 10 + k * 128 + n * 59) & 255), zx = x + p * 8 / 256, zy = y - p * 14 / 256;
        q_rp(zx, zy, 3, 1, 7), q_rp(zx + 1, zy + 1, 1, 1, 7), q_rp(zx, zy + 2, 3, 1, 7);
    }
}

/* (the sends' weather -- lightning, mist, echo, rain and the buff auras -- is gone for now: it comes back redesigned) */

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

typedef struct { int32_t k, mx, kind, lift, st; } q_mon_t;      /* kind: 0 slime, 1 bat, 2 mini slime, 3 goblin with an axe, 4 goblin archer */
static void q_goblin(int32_t cx, int32_t gy, uint32_t k, uint32_t fr, int flash, int32_t sc);

/* ---- the camp, when nothing plays: everybody has the whole width. Each hero keeps its own rhythm: it walks to a new spot (its own pace), does one thing, then another, and
 * again. PLAY: the monsters come in and everybody runs to its place, slowing to a walk as it gets there. ---- */
static uint32_t q_camp_t0, q_fl_t0;                                  /* q_fl_t0: when the mage last made the field of flowers */
static int32_t q_fl_x;
static int32_t q_camp_from[4], q_run_from[4];
static uint8_t q_camp_was = 2;
static uint32_t q_camp_seed;
static UI_NOINLINE int32_t q_camp_spot(uint32_t t, uint32_t n) { return 6 + (int32_t)(qhash(q_idle_seed * 7u + t * 977u + n * 131u) % 206u); }
static UI_NOINLINE int32_t q_camp_x(uint32_t t, uint32_t el, int *mv, int *dir, uint32_t *act)
{
    uint32_t seg = 9000u + t * 1700u, n = el / seg, in = el % seg, wd;
    int32_t prev = n ? q_camp_spot(t, n - 1u) : q_camp_from[t], cur = q_camp_spot(t, n), d = cur - prev, ad = d < 0 ? -d : d;
    wd = (uint32_t)ad * 1000u / 45u;                                                    /* 45 px/s */
    *dir = d < 0 ? -1 : 1;
    if (in < wd) { *mv = 1, *act = 0; return prev + d * (int32_t)(in * 256u / wd) / 256; }
    *mv = 0;
    *act = qhash(q_idle_seed + t * 131u + n * 17u + (in - wd < (seg - wd) / 2u ? 0u : 1u) * 5u) % 10u;     /* one thing, then another */
    return cur;
}
/* the party walking, the monsters coming, the attacks */
static UI_NOINLINE void q_scene_world(uint32_t key)
{
    const q_scn_t *c = &QSCN[key];
    uint32_t biome = c->biome_mode ? (uint32_t)c->biome_mode - 1u : q_biome_of_key(), playing = song.playing, i, nm = 0;
    int32_t H = c->h, G = H - 14, beat_q8 = q_beat_q8(), sf_q8 = beat_q8 * 4, sp = (int32_t)(qs.sp_q8 >> 8), ph[4], tx = 0, ty = 0, have_t = 0;
    int32_t per = Q_OPT_DENS() == 0u ? 92 : Q_OPT_DENS() == 1u ? 62 : 44, die = 124, wx, wy = 0, wframe = -1, k0, hsave[4], cmv[4] = {0, 0, 0, 0}, cdr[4] = {1, 1, 1, 1}, rflip[4] = {0, 0, 0, 0}, rmode[4] = {0, 0, 0, 0};
    uint32_t cact[4] = {0, 0, 0, 0}, arrived = 1, camp = key == QSC_PARTY && !playing, t0;
    q_mon_t mon[10];
    if (c->loop && punch.req >= 0 && punch.req <= 3)                       /* README: with loop on, the scene rewinds every bar */
        sp = 300 + ((beat_q8 & 1023) * 13200 / 1024) / 10;
    cv_begin(QW, (uint32_t)H, QP(0));
    q_world(biome, sp, H, c->flowers, c->birds);
    for (i = 0; i < 4; i++) ph[i] = QNONE, hsave[i] = q_hx[i];
    if (key == QSC_PARTY) {                                                                       /* where everybody stands: the camp's spots, or the run to the places */
        uint32_t now = fm1_ms;
        if (q_camp_seed != q_idle_seed) { q_camp_seed = q_idle_seed, q_camp_t0 = now, q_camp_was = 0; for (i = 0; i < 4; i++) q_camp_from[i] = hsave[i]; }
        if (!playing && q_camp_was == 1u) { q_camp_t0 = now; for (i = 0; i < 4; i++) q_camp_from[i] = hsave[i]; }          /* stopped: they go out again from their places */
        if (playing && q_camp_was != 1u) {                                                        /* PLAY: from wherever they were */
            for (i = 0; i < 4; i++) { int mv, dr; uint32_t a; q_run_from[i] = q_camp_x(i, now - q_camp_t0, &mv, &dr, &a); }
        }
        q_camp_was = (uint8_t)playing;
        t0 = q_run_t0 ? q_run_t0 : now;
        if (!playing) {
            for (i = 0; i < 4; i++) { int mv, dr; uint32_t a; q_hx[i] = q_camp_x(i, now - q_camp_t0, &mv, &dr, &a), cmv[i] = mv, cdr[i] = dr, cact[i] = a; }
        } else {
            uint32_t el = now - t0;
            for (i = 0; i < 4; i++) {
                int32_t d = hsave[i] - q_run_from[i], ad = d < 0 ? -d : d, dur = ad * 1000 / 140 + 250, u = (int32_t)q_min((int32_t)el, dur) * 256 / dur, e = 256 - (256 - u) * (256 - u) / 256;
                if ((int32_t)el < dur) arrived = 0, rmode[i] = u < 180 ? 2 : 1, rflip[i] = d < 0;
                q_hx[i] = q_run_from[i] + d * e / 256;
            }
        }
    }
    if (c->mon && !camp) {                                                          /* the stream, 62 px apart: many slimes, little ones, bats, a goblin now and then */
        static const uint8_t PAT[28] = {0, 0, 2, 0, 1, 0, 2, 3, 0, 0, 2, 0, 1, 2, 0, 4, 0, 2, 0, 0, 1, 2, 0, 3, 0, 2, 0, 4};   /* 14 slimes, 7 little ones, 3 bats, 4 goblins */
        k0 = (sp - 300) / per - 2;
        for (i = 0; i < 9u && nm < 10u; i++) {
            int32_t k = k0 + (int32_t)i, mx = k * per - sp + 300, kind = PAT[((k % 28) + 28) % 28], lift, st;
            if (mx > QW + 20 || mx < die - 20) continue;
            q_hop(beat_q8 + k * 95, 4, &lift, &st);
            mon[nm].k = k, mon[nm].mx = mx, mon[nm].kind = kind, mon[nm].lift = lift, mon[nm].st = st, nm++;
        }
        {   /* nobody attacks without a living monster in view; all aim at the nearest (README) */
            int32_t best = 0x7fffffff;
            for (i = 0; i < nm; i++)
                if (mon[i].mx >= die && mon[i].mx < QW - 16 && mon[i].mx < best) {
                    best = mon[i].mx, have_t = 1, tx = mon[i].mx + 12;
                    ty = mon[i].kind == 1 ? G - 40 + 6 : G - 8 - mon[i].lift;
                }
        }
    }
    if (c->att && playing && have_t && arrived)
        for (i = 0; i < 4; i++) ph[i] = q_rest(i) ? QNONE : q_aph_pat(&trk[i], sf_q8);
    if (have_t && tx - 12 - (q_hx[3] + 16) > 130) ph[3] = QNONE;           /* the fighter goes out for a monster within 84 px and always hits it close */
    for (i = 0; i < nm; i++) {                                             /* the monsters */
        q_mon_t *m = &mon[i];
        if (m->mx >= die) {
            int32_t hit = have_t && m->mx + 12 == tx && (q_in(ph[3], 0, 25) || q_in(ph[0], 62, 76) || q_in(ph[1], 38, 52)), top;
            if (m->kind == 1) {
                int32_t by = G - 40 + qsin((int32_t)(qs.idle_q8 >> 4) + m->k * 7) * 2 / 127;
                q_spr((((beat_q8 >> 7) + m->k) & 1) ? QS_BAT_1 : QS_BAT_0, m->mx, by, 1, 0, hit ? 7 : -1);
                top = by - 5;
            } else if (m->kind == 2) {
                q_slime(m->mx + 12, G, 14, 11, 11, 3, m->st == 0 ? -31 : m->st == 2 ? 56 : 0, m->lift / 2, hit, -1, 1, 0, 0, 0);          /* a little one */
                top = G - 15 - m->lift / 2;
            } else if (m->kind >= 3) {
                const uint32_t gk = m->kind == 4 ? 0u : 1u;
                q_goblin(m->mx + 12, G, gk, 4u + ((fm1_ms / 110u) + (uint32_t)m->k) % 4u, hit, 1);
                top = G - (gk ? 22 : 19);
            } else {
                q_spr((m->st == 0 ? QS_SLIME_AIR_0 : m->st == 1 ? QS_SLIME_REST_0 : QS_SLIME_LAND_0) + (uint32_t)hit, m->mx + 11, G - m->lift, 1, 0, -1);
                top = G - 22 - m->lift;
            }
            if (hit && Q_OPT_DMG()) {                                      /* every monster shows what it took: a number over its head */
                char b[8];
                b[0] = '-', fmt_int(b + 1, 3 + (int32_t)(qhash((uint32_t)m->k * 7u + (uint32_t)(beat_q8 >> 8)) % 9u));
                cv_text(m->mx + 4, top - (c->hp ? 14 : 10), &FONT_P, b, QP(10));
            }
            if (c->hp) {                                                   /* the bar shrinks with proximity, not with damage (README) */
                int32_t hpv = q_max(8, q_min(100, (m->mx - die) * 100 / (QW - 30 - die)));
                q_rp(m->mx, top, 24, 3, 0), q_rp(m->mx + 1, top + 1, 22 * hpv / 100, 1, 8);
            }
        } else {                                                           /* a poof */
            int32_t age = die - m->mx, a;
            for (a = 0; a < 10; a++) {
                int32_t r = age * 11 / 10;
                q_rp(m->mx + 12 + r * qcos(a * 6) / 127, (m->kind == 1 ? G - 34 : G - 12) + r * qsin(a * 6) / 127, age < 9 ? 2 : 1, age < 9 ? 2 : 1, age < 10 ? 7u : 6u);
            }
        }
    }
    wx = q_hx[3];
    if (have_t && ph[3] != QNONE) {                                        /* warrior: runs to the monster, strikes up close, jumps back (9 px arc) */
        int32_t appr = q_max(q_hx[3], q_min(tx - 12 - 34, q_hx[3] + 72)), p = ph[3];
        if (p < 0) { int32_t q = 256 + p * 2; wx = q_hx[3] + (appr - q_hx[3]) * q_max(0, q_min(256, q)) / 256; wframe = (int32_t)((fm1_ms / 70u) & 1u); }
        else if (p < 77) wx = appr;
        else if (p < 192) { int32_t q = (p - 77) * 256 / 115; wx = appr + (q_hx[3] - appr) * q / 256; wy = -(9 * qsin(q >> 3) / 127); }
    }
    if (have_t && c->att && arrived) {                                      /* he backs off when a monster gets too close */
        int32_t gap = tx - 12 - (wx + 16);
        if (gap < 6 && (ph[3] == QNONE || ph[3] >= QP100(75))) { int32_t b = 3 * qsin((int32_t)(fm1_ms >> 5)) / 127; wx -= q_min(16, 6 - gap); wy = -(b < 0 ? -b : b); }
    }
    {
        static const uint8_t ORDER[4] = {2, 1, 0, 3};
        uint32_t o;
        for (o = 0; o < 4; o++) {
            uint32_t t = ORDER[o], pose = Q_HERO[t], fr = (playing && c->walk) ? ((uint32_t)(beat_q8 >> 7) + t) & 1u : 0u;
            int32_t x = q_hx[t], y = G - 24, p = ph[t];
            if (t == 3) {
                x = wx, y += wy;
                if (wframe >= 0) fr = (uint32_t)wframe;
                pose = (p == QNONE || p >= QP100(60) || p < 0) ? QS_WARRIOR_IDLE_0 : p < QP100(25) ? QS_WARRIOR_STRIKE_0 : QS_WARRIOR_FOLLOW_0;   /* (no dash pose: he runs at the monster with the village's run, then strikes) */
            } else if (t == 0) {
                pose = (p != QNONE && p > -QP100(40) && p < QP100(60)) ? QS_MAGE_CAST_0 : QS_MAGE_IDLE_0;
                y += -4 + qsin((int32_t)(qs.idle_q8 >> 3)) * 3 / 254;
                fr = 0;
            } else if (t == 1) pose = (p != QNONE && p < 0) ? QS_ARCHER_DRAW_0 : QS_ARCHER_IDLE_0;
            else pose = ((p != QNONE && p > -QP100(30) && p < 256)) ? QS_CLERIC_CAST_0 : QS_CLERIC_IDLE_0;
            qf_walk = (uint8_t)((t == 3 && wframe >= 0) ? 2u : (playing && c->walk && t != 3 && p == QNONE) ? 1u : 0u);       /* the fighter runs at his monster */
            if (playing && !arrived && key == QSC_PARTY && rmode[t]) {                         /* PLAY: running to the place, slowing to a walk as it comes */
                qf_walk = (uint8_t)rmode[t], qf_flip = (uint8_t)rflip[t];
                q_hero_act(t, 0, x, y, 290, 0);
            } else if (!playing && key == QSC_PARTY && cmv[t]) {                               /* the camp: on its way to a new spot */
                qf_walk = 1, qf_flip = (uint8_t)(cdr[t] < 0);
                q_hero_act(t, 0, x, y, 290, 0);
            } else if (!playing && p == QNONE && key == QSC_PARTY && t == 3u && cact[3] % 4u == 1u) {   /* the fighter trains: wind-up, strike, follow-through, again */
                uint32_t ph3 = (fm1_ms / 330u) % 4u;
                qf_flip = (uint8_t)(cdr[3] < 0);
                q_hero_act(3u, ph3 == 0u ? 1u : ph3 == 1u ? 2u : ph3 == 2u ? 3u : 0u, x, y, 290, 0);
            } else if (!playing && p == QNONE && key == QSC_PARTY) {                           /* nobody is playing: the camp has a life of its own */
                /* acts 0 stand, 1 sit (6 and 8 too: nobody lies down to sleep any more), 2 and 5 talk, 3 smoke, 4 sit with the FM-1, 7 exercise (the fighter's sit-ups) */
                uint32_t act = cact[t], pz = act == 0u ? 0u : act == 1u ? 4u : act == 2u ? 6u : act == 3u ? 7u : act == 4u ? 4u : act == 5u ? 6u : act == 7u ? (t == 3u ? 8u : 0u) : act == 9u ? (t == 0u ? 2u : 0u) : 1u;
                if (t == 3u) { uint32_t m = cact[3] % 4u; pz = m == 2u ? 8u : 0u; act = m; }   /* the fighter: stand, train (above), sit-ups, sleep */
                qf_flip = (uint8_t)(cdr[t] < 0);
                if (t == 0u && act == 9u && (!q_fl_t0 || fm1_ms - q_fl_t0 > 12500u)) q_fl_t0 = fm1_ms | 1u, q_fl_x = x + 8;   /* the mage calls up a field of flowers */
                q_hero_act(t, pz, x, y + (pz == 10u || pz == 8u ? 6 : 0), 290, act == 4u && t != 3u);
                if (pz == 7u) { uint32_t a2 = (fm1_ms / 80u + t * 5u) % 14u; cv_pset(qf_handx + 2 + (int32_t)(a2 / 5u), qf_handy - 3 - (int32_t)a2, RGB(0xd8, 0xd8, 0xe0)), cv_pset(qf_handx + 3 - (int32_t)(a2 % 3u), qf_handy - 6 - (int32_t)a2, RGB(0x9a, 0x9a, 0xaa)); }
            } else {
                qf_scale = (uint16_t)(key == QSC_PARTY ? 290 : 256);
                q_spr(pose + fr, x, y, 1, 0, -1);
                qf_scale = 256;
            }
            qf_walk = 0;
        }
    }
    if (key == QSC_PARTY && !playing && q_fl_t0 && fm1_ms - q_fl_t0 < 11500u) {   /* the mage's field of flowers: pixels and a few round ones, mostly light blue; 10 s, then it fades out */
        uint32_t el = fm1_ms - q_fl_t0, n, fade = el > 10000u ? 100u - (el - 10000u) * 100u / 1500u : 100u;
        for (n = 0; n < 46u; n++) {
            uint32_t h = qhash(n * 977u + q_fl_t0);
            int32_t fx = q_fl_x + (int32_t)(h % 76u) - 38, fy = G + 1 + (int32_t)((h >> 8) % 10u);
            uint16_t col = (h >> 4) % 10u < 7u ? RGB(0xa8, 0xd8, 0xff) : RGB(0xff, 0xff, 0xff);
            if (el < (h >> 12) % 1600u || (h >> 20) % 100u >= fade) continue;           /* they open one by one, and go one by one */
            if ((h >> 2) % 6u == 0u) cv_rect(fx - 1, fy, 3, 1, col), cv_rect(fx, fy - 1, 1, 3, col), cv_pset(fx, fy, RGB(0xff, 0xff, 0xff));
            else cv_pset(fx, fy, col);
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
            int32_t sx = q_hx[0] + 18 - 6, sy = G - 24 - 4 + 10 - 26;
            if (p < QP100(35)) q_summon(sx, sy + qsin(p / 2), q_min(256, p * 256 / QP100(30)), 10);
            else if (p < QP100(62)) { int32_t q = (p - QP100(35)) * 256 / QP100(27), e = q * q / 256; q_meteor(sx + (tx - sx) * e / 256, sy + (ty - sy) * e / 256, 16); }
            else { static const uint8_t C4[4] = {10, 9, 8, 7}; q_burst(tx, ty, (p - QP100(62)) * 256 / QP100(30), C4, 4); }
        }
        p = ph[1];
        if (p != QNONE && p >= 0 && p < QP100(50)) {                       /* archer: the arrow flies on a parabola (peak 22 px) */
            int32_t ax = q_hx[1] + 19, ay = G - 24 + 12;
            if (p < QP100(40)) q_arc_arrow(ax, ay, tx, ty, p * 256 / QP100(40), 22, 7, 12);
            else { static const uint8_t C2[2] = {7, 12}; q_burst(tx, ty, (p - QP100(40)) * 256 / QP100(15) / 2, C2, 2); }
        }
    }
    if (c->loop && punch.req >= 0 && punch.req <= 3) {                      /* loop: the mage's orbit, a glitch line at every bar */
        int32_t cx = q_hx[0] + 8, cy = G - 14, k;
        for (k = 0; k < 12; k++) {
            int32_t a = (int32_t)(qs.idle_q8 >> 3) + k * 5;
            q_rp(cx + 16 * qcos(a) / 127, cy + 8 * qsin(a) / 127, 1, 1, k & 1 ? 12u : 13u);
        }
        if ((beat_q8 & 1023) < 35)
            for (k = 0; k < 6; k++) q_rp(0, (int32_t)(qhash((uint32_t)(k + (qs.idle_q8 >> 4))) % (uint32_t)H), QW, 1, 13);
    }
    for (i = 0; i < 4; i++) q_hx[i] = hsave[i];                             /* (the slots go on walking where they were) */
}

/* ---- single-purpose scenes ---- */
static UI_NOINLINE void q_scene_kit(void)           /* the chest and its floating items (README: KIT) */
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
static UI_NOINLINE void q_scene_camp(void)          /* the night camp: muted heroes rest at the fire, the others fight a bat (README: CAMP) */
{
    int32_t G = 70, beat_q8 = q_beat_q8(), sf_q8 = beat_q8 * 4, n, bx = 184, by = G - 40 + qsin((int32_t)(qs.idle_q8 >> 3)) * 3 / 127, tx = bx + 12, ty = by + 6, ph[4], fighters = 0;
    static const int32_t SLOTS[4] = {82, 106, 132, 156}, SIT[3] = {20, 2, 62};
    uint32_t t, sitters = 0;
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
}
static UI_NOINLINE void q_scene_menu(void)          /* the inn: the party sleeps around the fire (README: MENU) */
{
    static const int32_t X[4] = {136, 80, 52, 164};                           /* mage, archer, cleric, warrior */
    int32_t G = 46, t;
    cv_begin(QW, 60, QP(0));
    q_world(QB_NIGHT, 0, 60, 0, 0);
    q_fire(120, G);
    for (t = 0; t < 4; t++) { q_spr(Q_ROT[t], X[t], G - 16, 1, 0, -1), q_zz(X[t] + 6, G - 18, t); }
}
static UI_NOINLINE void q_scene_rec(int count)      /* rec ready / count-in: a King Slime blocks the way (README) */
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
static UI_NOINLINE void q_scene_flush(void) { cv_blit(0, 19); }          /* the scenes draw into the canvas; the screen adds its overlay, then flushes */
#include "ui_quest_stage.c"        /* STAGE: who plays (the PARTY effect from winampFM) */
#include "ui_quest_scenes.c"
#include "ui_quest_screens.c"
#include "ui_quest_pages.c"
#include "ui_quest_stock.c"
#include "ui_quest_town.c"
#include "ui_quest_flight.c"
#include "ui_quest_boss.c"
#include "ui_quest_cd.c"           /* the sound changed: the CDs (after No Man Sky FM / winampFM's tapes) */
