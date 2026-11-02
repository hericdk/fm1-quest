/* SPDX-License-Identifier: GPL-3.0-only
 * FM1 Quest, part 5: the VILLAGE, the second home (HOME tapped again on PARTY). The lab's "street" scene (quest/lab/lab-opening.js): a long street of
 * houses, a smithy, a tower with an arch and banners, lamps and torches that catch fire as the party walks by them, people at their doors, horses,
 * and a day that passes: dawn with the rising sun, afternoon, the golden hour, the night with stars and a crescent moon; a clock in the corner.
 * The houses are not hand-written here: tools/gen_town.mjs runs the lab's own station functions against a recording canvas and stores what they
 * draw (ui_quest_town_data.h, one LZ4 block per station). The scenery goes through the day's tint colour by colour (the lab lays a translucent
 * wash over the picture: the same result for flat shapes), lights and glows are blended after it, the people on top.
 * Painted with ps_paint (ui_quest_pages.c) in the scene rows 19..193, paced by the SPI token bucket of the screens (q_scene_budget's own rules).
 * The street walks at bpm * 0.25 px/s while the loop plays and 20 px/s while it rests; the day lasts 96 s. */
#include "ui_quest_town_data.h"
#define TW_PER 1600
#define TW_G 29                                                                    /* the quest board's middle, in the street (world x): the guild is by it */
#define TW_DAY_MS 96000u
#define TW_Y0 (PS_Y0 - 22)                         /* the picture is moved up 22 rows (some sky is cut): the bottom of the screen carries the party's strip */
#define TW_GY 140                                  /* the street line of the 175-row scene (the lab: 80 % of the picture) */
#define TW_NCOL ((int32_t)(sizeof TD_COL / sizeof TD_COL[0]))
static struct {
    uint32_t cam_q8, last_ms, now;
    int32_t cam, by, nt, gold, go, na, sx, sy, sr, glow, flare, hour, pr;
    uint16_t sk[5], sun, tc[sizeof TD_COL / sizeof TD_COL[0]];
    uint8_t warm[3], cur;
} tw = {.cur = 255};
static uint8_t tw_buf[TD_BUF];

static UI_NOINLINE int32_t tw_wrap(int32_t x, int32_t per, int32_t lo) { return ((x % per) + per + lo) % per - lo; }
static UI_NOINLINE uint16_t tw_tint(uint16_t c)                       /* the hour's wash: orange at the golden hour, deep blue at night */
{
    if (tw.go) c = ps_mix(c, RGB(255, 150, 40), (uint32_t)tw.go);
    if (tw.na) c = ps_mix(c, RGB(8, 12, 52), (uint32_t)tw.na);
    if (q_filter == 0u)                                                                    /* the default: as drawn by day, melting into the violet night */
        return tw.nt > 0 ? ps_mix(c, q_flt_c(c, 8), (uint32_t)tw.nt * 256u / 500u) : c;
    return q_flt_c(c, q_filter);
}
static UI_NOINLINE uint16_t tw_tint_spr(uint16_t c)                   /* the people take the hour at 55 % / 16 % (the lab composes them after the wash) */
{
    if (tw.nt) c = ps_mix(c, RGB(12, 14, 56), (uint32_t)tw.nt * 141u / 1000u);
    if (tw.gold) c = ps_mix(c, RGB(255, 150, 50), (uint32_t)tw.gold * 41u / 1000u);
    return c;
}
static UI_NOINLINE void tw_blend(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c, uint32_t a)   /* a translucent rectangle, a = 0..256 */
{
    int32_t x1 = x + w + cv_ox, y1 = y + h + cv_oy, i;
    x += cv_ox, y += cv_oy;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > (int32_t)cv_w) x1 = (int32_t)cv_w;
    if (y1 > (int32_t)cv_h) y1 = (int32_t)cv_h;
    for (; y < y1; y++)
        for (i = x; i < x1; i++) {                       /* pixel art: no smooth wash, the translucency is four steps and the in-between is a 2 x 2 block ordered dither */
            static const uint8_t BY[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
            uint16_t *p = &cv_px[(uint32_t)y * cv_w + (uint32_t)i];
            uint32_t ax = (uint32_t)(i - cv_ox) >> 1, ay = (uint32_t)(y - cv_oy) >> 1, lv = a >> 6, rem = a & 63u;
            if (rem > (uint32_t)BY[(ay & 3u) * 4u + (ax & 3u)] * 4u + 2u) lv++;
            if (lv) *p = swap16(ps_mix(swap16(*p), c, lv > 4u ? 256u : lv * 64u));
        }
}
static UI_NOINLINE void tw_glow(int32_t cx, int32_t cy, int32_t r, uint32_t a1000)         /* the lab's lampGlow: a warm translucent disc */
{
    int32_t dy;
    if (cx + r + cv_ox < 0 || cx - r + cv_ox >= (int32_t)cv_w || cy + r + cv_oy < 0 || cy - r + cv_oy >= (int32_t)cv_h) return;
    for (dy = -r; dy <= r; dy++) { int32_t w = q_isqrt(r * r - dy * dy); tw_blend(cx - w, cy + dy, 2 * w + 1, 1, RGB(255, 170, 60), a1000 * 256u / 1000u); }
}
static UI_NOINLINE int32_t tw_v(const uint8_t **p)                                      /* zig-zag LEB128 */
{
    uint32_t z = 0, s = 0, b;
    do { b = *(*p)++; z |= (b & 127u) << s; s += 7u; } while (b & 128u);
    return (z & 1u) ? -(int32_t)((z + 1u) >> 1) : (int32_t)(z >> 1);
}
static UI_NOINLINE void tw_poly(const int32_t *pt, int32_t n, int32_t ox, int32_t oy, uint16_t c, uint16_t d, int32_t step)   /* points in 1/4 px; roof tiles when step */
{
    int32_t y, y0 = 99999, y1 = -99999, i, ya;
    for (i = 0; i < n; i++) { y0 = q_min(y0, pt[i * 2 + 1]); y1 = q_max(y1, pt[i * 2 + 1]); }
    y0 >>= 2, y1 = (y1 + 3) >> 2, ya = y0;
    for (y = y0; y <= y1; y++) {
        int32_t xs[8], m = 0, a, b, yy = y * 4 + 2, sy = oy + y + cv_oy;
        if (sy < 0 || sy >= (int32_t)cv_h) continue;
        for (i = 0; i < n && m < 8; i++) {
            int32_t ax = pt[i * 2], ay = pt[i * 2 + 1], bx = pt[((i + 1) % n) * 2], by = pt[((i + 1) % n) * 2 + 1];
            if ((ay <= yy && by > yy) || (by <= yy && ay > yy)) xs[m++] = ax + (bx - ax) * (yy - ay) / (by - ay);
        }
        for (a = 1; a < m; a++) { int32_t v = xs[a]; for (b = a - 1; b >= 0 && xs[b] > v; b--) xs[b + 1] = xs[b]; xs[b + 1] = v; }
        for (a = 0; a + 1 < m; a += 2) {
            int32_t xa = (xs[a] + 2) >> 2, xb = (xs[a + 1] + 2) >> 2;
            if (xb <= xa) continue;
            if (step) {
                int32_t r = (y - ya) % step;
                cv_rect(ox + xa, oy + y, xb - xa, 1, r == 0 ? d : c);
                if (r == step - 1) for (b = xa + (((y - ya) / step) & 1) * 2; b < xb; b += 4) cv_pset(ox + b, oy + y, d);
            } else cv_rect(ox + xa, oy + y, xb - xa, 1, c);
        }
    }
}
static UI_NOINLINE void tw_smoke(int32_t x, int32_t y)
{
    int32_t k;
    for (k = 0; k < 5; k++) {
        int32_t u = (int32_t)((tw.now / 2u + (uint32_t)k * 200u) % 1000u);
        p_ell(x + p_sinr(u * 5 + k * 1000) * 3 / 127 + u * 6 / 1000, y - u * 22 / 1000, 1 + u * 25 / 10000, 1 + u * 25 / 10000, tw_tint(RGB(0x8a, 0x8a, 0x9a)));
    }
}
static UI_NOINLINE void tw_station(uint32_t st, int32_t ox, int32_t oy)                  /* a station's recorded drawing, at (ox, oy = the street line) */
{
    const uint8_t *p;
    if (tw.cur != st) { lz4_unpack(TD_DATA + TD_ZO[st], TD_DATA + TD_ZO[st + 1u], tw_buf); tw.cur = (uint8_t)st; }
    for (p = tw_buf; *p != 255u;) {
        uint32_t k = *p++, col = *p++;
        uint16_t c = tw.tc[col % (uint32_t)TW_NCOL];
        int32_t a, b, e, d, n;
        if (k == 3 || k == 4) {
            int32_t pt[16], step = 0, dc = 0, i;
            n = tw_v(&p);
            if (k == 4) step = tw_v(&p), dc = tw_v(&p);
            for (i = 0; i < n * 2 && i < 16; i++) pt[i] = tw_v(&p);
            tw_poly(pt, n, ox, oy, c, tw.tc[dc % TW_NCOL], step);
            continue;
        }
        a = tw_v(&p), b = tw_v(&p);
        if (k == 1) { cv_pset(ox + a, oy + b, c); continue; }
        if (k == 9) { tw_smoke(ox + a, oy + b); continue; }
        e = tw_v(&p);
        if (k == 7) { p_ell(ox + a, oy + b, e, e, c); continue; }
        d = tw_v(&p);
        switch (k) {
        case 0: p_R(ox + a, oy + b, e, d, c); break;
        case 2: cv_line(ox + a, oy + b, ox + e, oy + d, c); break;
        case 5: for (n = 0; n * 5 < d; n++) {                                           /* stone courses: a joint a row, staggered uprights */
                    int32_t kk;
                    p_R(ox + a, oy + b + n * 5, e, 1, c);
                    for (kk = (n & 1) * 4; kk < e; kk += 9) p_R(ox + a + kk, oy + b + n * 5, 1, q_min(5, d - n * 5), c);
                } break;
        case 6: p_ell(ox + a, oy + b, e, d, c); break;
        case 8: tw_blend(ox + a, oy + b, e, d, c, 71); break;
        default: for (n = -d; n <= d; n++) { int32_t w = e * q_isqrt((d * d - n * n) * 1024) / (32 * (d ? d : 1)); tw_blend(ox + a - w, oy + b + n, 2 * w + 1, 1, c, 71); }
        }
    }
}
/* ---- people: the four classes of the party recoloured as villagers (their skin, the cream and the outline stay) ---- */
static UI_NOINLINE void tw_spr(uint32_t id, int32_t cx, int32_t feet, int32_t s, int flip, uint16_t base)       /* s 0: half size (the children) */
{
    if (id <= QS_WARRIOR_ROT) {                                                        /* the figures: the party in its colours, villagers in theirs with empty hands */
        uint32_t slot, pose, fr, cls;
        qf_ident(id, &slot, &pose, &fr);
        cls = base ? 6u : slot == 2u && q_var[2] ? 4u : slot == 3u ? (q_var[3] ? 5u : 3u) : slot;
        q_fig(cls, pose, fr, cx, feet, s < 1 ? 1 : s, flip, -1, tw_tint_spr(base ? p_shade(base, 1100) : QF_LT[cls]), tw_tint_spr(base ? p_shade(base, 640) : QF_DK[cls]), s == 0);
        return;
    }
    const qspr_t *sp = &QSPR[id];
    const uint8_t *d = pk_spr(id), *mk = d + ((uint32_t)sp->w * sp->h + 1u) / 2u;
    int32_t gx, gy, kid = s == 0, sc = kid ? 1 : s, st = kid ? 2 : 1;
    int32_t x = cx - sc * (sp->ox + sp->w / 2) / st, y = feet - sc * (sp->oy + sp->h) / st, bx = flip ? 16 - sp->ox - sp->w : sp->ox;
    for (gy = 0; gy < sp->h; gy += st)
        for (gx = 0; gx < sp->w; gx += st) {
            uint32_t n = (uint32_t)(gy * sp->w + gx), v = (d[n >> 1] >> ((n & 1u) ? 0 : 4)) & 15u;
            uint16_t c = QP(v);
            if (!((mk[n >> 3] >> (7u - (n & 7u))) & 1u)) continue;
            if (base && v != 0 && v != 7 && v != 14 && v != 15) c = p_shade(base, 300u + (((c >> 11) & 31u) * 3u + ((c >> 5) & 63u) * 3u / 2u + (c & 31u)) * 8u);
            cv_rect(x + sc * (bx + (flip ? sp->w - 1 - gx : gx)) / st, y + sc * (sp->oy + gy) / st, sc, sc, tw_tint_spr(c));
        }
}
static const struct { int16_t wx; uint8_t a, c, s; int8_t d; } TW_CIT[20] = {
    {56, 0, 0, 40, 1}, {74, 0, 1, 42, 1}, {300, 0, 2, 34, -1}, {410, 0, 3, 36, -1}, {424, 0, 3, 24, -1}, {440, 0, 3, 24, -1}, {448, 0, 4, 22, -1}, {556, 0, 5, 40, 1},
    {596, 0, 6, 40, -1}, {1300, 0, 5, 36, -1}, {704, 2, 7, 36, -1}, {960, 0, 6, 36, 1}, {1100, 0, 9, 36, -1}, {782, 0, 8, 42, -1}, {884, 0, 6, 44, 1}, {930, 0, 5, 40, -1},
    {1040, 0, 9, 36, 1}, {1180, 0, 5, 38, -1}, {1240, 0, 0, 36, -1}, {1380, 0, 6, 40, 1}};
static const uint16_t TW_NPC[10] = {RGB(0xd8, 0x60, 0x3a), RGB(0x4a, 0x4a, 0x56), RGB(0x4a, 0x6a, 0x9a), RGB(0x4a, 0x78, 0xb0), RGB(0x8a, 0x6a, 0x3a), RGB(0xc8, 0xa0, 0x50),
                                    RGB(0x8a, 0x8a, 0x96), RGB(0xb8, 0x3a, 0x2a), RGB(0x6a, 0x3a, 0x3a), RGB(0xa8, 0x48, 0x3a)};
static const struct { int16_t wx; uint8_t s; uint16_t c; } TW_COMP[8] = {   /* the companions that fall into step with the party when it reaches them */
    {410, 44, RGB(0xc8, 0xc8, 0xd8)}, {470, 36, RGB(0x8a, 0x8a, 0x96)}, {560, 46, RGB(0x4a, 0x4a, 0x56)}, {640, 44, RGB(0x3f, 0x9a, 0x7a)},
    {770, 32, RGB(0xe0, 0x8a, 0x3a)}, {860, 46, RGB(0xb8, 0x3a, 0x2a)}, {1010, 40, RGB(0x7a, 0xb0, 0x4e)}, {1130, 42, RGB(0xa8, 0x2a, 0x2a)}};
static const int16_t TW_PETX[5] = {262, 905, 1020, 1060, 1260};
static const uint16_t TW_PETC[5] = {RGB(0xc8, 0xa0, 0x50), RGB(0xd8, 0x6a, 0x2a), RGB(0x8a, 0x8a, 0x96), RGB(0xd8, 0x6a, 0x2a), RGB(0xc8, 0xa0, 0x50)};

static uint16_t tw_pace;                                                          /* the gait speed of the next person (0: its own), 1000 = 1x */
static uint8_t tw_run, tw_fade;                                                            /* 1: the next walker runs (another cycle: leaning, long strides) */
static UI_NOINLINE void tw_person(int32_t i, int32_t x, int32_t feet, int32_t size, int flip, int kind, uint16_t col)   /* kind 0 stands, 1 walks, 2 hammers, 3 sits, 4 talks */
{
    uint32_t cls = (uint32_t)i & 3u, fr = (uint32_t)((tw.now / (kind == 1 ? 180u : 600u) + (uint32_t)i) & 1u), id = Q_HERO[cls] + fr;
    int32_t s = size <= 26 ? 0 : 1;                                                   /* everybody one size, a few children at half of it */
    if (kind == 2) id = QS_WARRIOR_ANTIC_0 + (uint32_t)(((tw.now + (uint32_t)i * 350u) / 350u) & 1u) * 2u;
    if (kind == 3) id = Q_SEAT[cls];
    if (x < -20 || x > 260) return;
    tw_blend(x - (s ? 7 : 4), feet - 1, s ? 14 : 8, 2, RGB(0, 0, 0), 71);
    qf_walk = (uint8_t)(kind == 1 ? 1u + tw_run : 0u);
    qf_speed = tw_pace ? tw_pace : (uint16_t)(800 + ((uint32_t)i * 71u) % 6u * 80u);
    qf_scale = (uint16_t)(size * 256 / 36);                                            /* far rows smaller and thinner, near rows bigger */
    qf_fade = (uint8_t)(size <= 28 ? 2 : tw_fade);                                      /* the farthest row: a dashed stick, cheap and quiet */
    if (kind == 5) q_fig(6, 7, 0, x, feet, 1, flip, -1, tw_tint_spr(p_shade(col, 1100)), tw_tint_spr(p_shade(col, 640)), s == 0);                         /* smoking */
    else if (kind == 4) q_fig(6, 6, (uint32_t)((tw.now / 380u + (uint32_t)i) & 1u), x, feet, s ? 1 : 1, flip, -1, tw_tint_spr(p_shade(col, 1100)), tw_tint_spr(p_shade(col, 640)), s == 0);   /* talking, a hand going */
    else tw_spr(id, x, feet, s ? 1 : 0, flip, col);
    qf_scale = 256, qf_walk = 0, qf_fade = 0, qf_speed = 1000;
}
/* ---- sky actors ---- */
static UI_NOINLINE void tw_sky(void)
{
    int32_t k, r, x, sh2 = TW_GY / 2, i;
    for (k = 0; k < 5; k++) {                                                          /* the lab's g.sky: 5 bands, a 2-row checker at each border */
        int32_t a = k * sh2 / 5, b = (k + 1) * sh2 / 5;
        p_R(0, TW_Y0 + a, 240, b - a, tw_tint(tw.sk[k]));
        if (k) for (r = 0; r < 2; r++) for (x = r; x < 240; x += 2) cv_pset(x, TW_Y0 + a + r, tw_tint(tw.sk[k - 1]));
    }
    p_R(0, TW_Y0 + sh2, 240, TW_GY - sh2, tw_tint(tw.sk[4]));
    if (tw.sr) {                                                                       /* the sun with its halo, rays (afternoon) and lens-flare circles (dawn, sunset) */
        int32_t sx = tw.sx, sy = TW_Y0 + tw.sy, sh = 870 + p_sin(tw.now, 17) * 130 / 127;
        uint16_t warm = RGB(tw.warm[0], tw.warm[1], tw.warm[2]);
        static const int16_t HL[4][2] = {{38, 50}, {28, 80}, {20, 120}, {14, 160}};
        for (i = 0; i < 4; i++) tw_glow(sx, sy, HL[i][0], (uint32_t)HL[i][1] * (uint32_t)tw.glow / 1000u), (void)warm;
        if (tw.flare > 20) {
            static const int16_t FL[6][4] = {{14, 7, 0, 0}, {32, 5, 1, 1}, {52, 12, 1, 2}, {76, 6, 0, 3}, {100, 9, 1, 2}, {126, 5, 0, 4}};
            static const uint16_t FC[5] = {RGB(255, 235, 170), RGB(255, 160, 140), RGB(255, 245, 210), RGB(170, 220, 255), RGB(255, 255, 255)};
            uint32_t fa = (uint32_t)tw.flare * (uint32_t)sh / 1000u;
            tw_blend(sx - 70, sy, 140, 1, warm, fa * 128u / 1000u), tw_blend(sx - 36, sy - 1, 72, 3, warm, fa * 128u / 1000u);
            for (i = 0; i < 6; i++) {
                int32_t fx = sx - FL[i][0] * 88 / 100, fy = sy + FL[i][0] * 48 / 100, rr = FL[i][1], dy;
                for (dy = -rr; dy <= rr; dy++) { int32_t w = q_isqrt(rr * rr - dy * dy); tw_blend(fx - w, fy + dy, 2 * w + 1, 1, FC[FL[i][3]], (FL[i][2] ? 77u : 217u) * fa / 1000u); }
            }
        }
        p_ell(sx, sy, tw.sr, tw.sr, tw.sun), p_ell(sx, sy, q_max(2, tw.sr - 4), q_max(2, tw.sr - 4), RGB(0xff, 0xfe, 0xf0));
    }
    for (i = 0; i < 3; i++) {                                                          /* clouds: slow, white at day, dark at night */
        int32_t cx2 = tw_wrap(i * 96 - tw.cam / 20, 300, 40), cy = TW_Y0 + 8 + i * 12, s = 2 + i % 2;
        uint16_t c = tw.nt > 300 ? RGB(0x2a, 0x2a, 0x50) : ps_mix(tw.sk[(8 + i * 12) * 5 / (TW_GY / 2) > 4 ? 4 : (8 + i * 12) * 5 / (TW_GY / 2)], RGB(255, 255, 255), 153);
        c = tw_tint(c);
        p_R(cx2, cy + 3 * s, 16 * s, 4 * s, c), p_R(cx2 + 3 * s, cy, 8 * s, 6 * s, c), p_R(cx2 + 8 * s, cy + s, 7 * s, 5 * s, c);
    }
    if (tw.nt > 200) {                                                                 /* stars (the dusk to the dawn) and the crescent (the deep night: never with the sun) */
        for (i = 0; i < 46; i++) {
            int32_t x2 = (int32_t)p_hs((uint32_t)i * 3u) * 240 / 1024, y2 = TW_Y0 + (int32_t)p_hs((uint32_t)i * 5u + 2u) * (TW_GY - 60) / 1024;
            int on = p_sinr((int32_t)((tw.now * 2u) % 6283u) + i * 1700) > 70;
            cv_pset(x2, y2, on ? RGB(255, 255, 255) : RGB(0xc8, 0xc8, 0xd8));
            if (i % 9 == 0 && on) cv_pset(x2 - 1, y2, RGB(0xa8, 0xa8, 0xc0)), cv_pset(x2 + 1, y2, RGB(0xa8, 0xa8, 0xc0)), cv_pset(x2, y2 - 1, RGB(0xa8, 0xa8, 0xc0)), cv_pset(x2, y2 + 1, RGB(0xa8, 0xa8, 0xc0));
        }
    }
    if (tw.pr >= 700 && tw.pr < 860) {
        tw_glow(198, TW_Y0 + 24, 23, 100), tw_glow(198, TW_Y0 + 24, 16, 100);
        for (i = -9; i <= 9; i++)
            for (x = -9; x <= 9; x++) {
                int32_t bx = x * 100 - 450, by = i * 100 + 198;
                if (x * x + i * i > 81 || bx * bx + by * by <= 627264) continue;
                cv_pset(198 + x, TW_Y0 + 24 + i, i * 100 > 315 ? RGB(0xe8, 0xe0, 0xb0) : RGB(0xf6, 0xef, 0xc8));
            }
    }
}

static UI_NOINLINE void tw_far(int32_t sp100, int32_t by, int32_t camx, uint16_t col, uint16_t fog)      /* a layer of far houses: the lab's farHouses */
{
    int32_t per = 360, off = -((camx * sp100 / 100) % per), k, i;
    for (k = 0; k < 2; k++)
        for (i = 0; i < 9; i++) {
            int32_t w = TD_FW[i], h = TD_FH[i], x = off + k * per + i * 40 - 10;
            uint16_t c = ps_mix(col, fog, (uint32_t)(90 + (i % 3) * 20));
            if (x > 250 || x + w < -4) continue;
            p_R(x, by - h, w, h, tw_tint(c));
            p_tri(x - 2, by - h, x + w / 2, by - h - w * 55 / 100, x + w + 2, by - h, tw_tint(ps_mix(c, RGB(0x1a, 0x3a, 0x40), 64)));
            if (tw.nt > 100 && ((TD_FL[k] >> i) & 1)) p_R(x + 5, by - h + 8, 3, 4, tw_tint(RGB(0xf0, 0xc8, 0x60)));
        }
}
/* ---- the day's birds: one for every beat while it plays, thin black strokes crossing from left to right at the height of the music's pitch ---- */
static UI_NOINLINE void tw_birds(void)
{
    static struct { uint32_t t0; int16_t y; } bd[8];
    static uint32_t seen[4];
    uint32_t i, k;
    if (tw.nt > 120) return;
    for (k = 0; k < 4u; k++)                                                           /* a bird for every note any of the four plays: rests leave the sky empty */
        if (qh_ms[k] && qh_ms[k] != seen[k]) {
            int32_t nt = qh_note[k], y = 64 - (q_max(36, q_min(96, nt)) - 36) * 52 / 60;
            seen[k] = qh_ms[k];
            for (i = 0; i < 8u; i++)
                if (fm1_ms - bd[i].t0 > 6000u) { bd[i].t0 = fm1_ms, bd[i].y = (int16_t)y; break; }
        }
    for (i = 0; i < 8u; i++) {
        uint32_t age = fm1_ms - bd[i].t0;
        int32_t x, y, w;
        if (age > 6000u) continue;
        x = -8 + (int32_t)(age * 52u / 1000u);
        if (x > 248) continue;
        y = TW_Y0 + bd[i].y + (int32_t)p_sinr((int32_t)(age * 3u % 6283u)) * 3 / 127;
        w = (age / 140u) & 1u ? 2 : 0;                                                  /* the wings: two strokes, up and down */
        cv_line(x - 3, y - 1 + w, x, y + 1 - w, RGB(0x10, 0x10, 0x14)), cv_line(x, y + 1 - w, x + 3, y - 1 + w, RGB(0x10, 0x10, 0x14));
    }
}
static UI_NOINLINE void tw_ground(void)
{
    int32_t y = TW_Y0 + TW_GY, r, x, k, bw;
    p_R(0, y, 240, 175 - TW_GY, tw_tint(RGB(0x8a, 0x82, 0x72)));
    for (r = 0; y < TW_Y0 + 175; r++, y += 6 + r * 2) {
        p_R(0, y, 240, 1, tw_tint(RGB(0x5a, 0x54, 0x48)));
        bw = 18 + r * 6;
        for (x = -((tw.cam * (100 + r * 8) / 100) % bw) + (r & 1) * bw / 2; x < 240; x += bw) p_R(x, y, 1, 6 + r * 2, tw_tint(RGB(0x5a, 0x54, 0x48)));
    }
    for (k = 0; k < 14; k++) p_R(tw_wrap(k * 71 - tw.cam, 340, 30), TW_Y0 + TW_GY + 3 + (k % 3) * 9, 5 + k % 4, 1, tw_tint(RGB(0xa8, 0xa0, 0x90)));
}
/* ---- the lights that catch fire as the party walks by (the lab's "luzes acesas") ---- */
static UI_NOINLINE int32_t tw_ign(const td_lt_t *l, int32_t x)
{
    if (l->always || tw.nt > 300) return 1000;
    if (tw.gold > 300 || tw.nt > 50) return q_max(0, q_min(1000, (118 + 12 - x) * 1000 / 18));
    return 0;
}
static UI_NOINLINE void tw_light(const td_lt_t *l, int32_t ox, int32_t oy, uint32_t sid)
{
    int32_t x = ox + l->x, y = oy + l->y, ig, bu, f, up;
    uint32_t t = tw.now;
    if (x < -30 || x > 270) return;
    f = p_sinr((int32_t)((t * 9u) % 6283u) + x * 1000) * 40 / 127;                     /* flicker, +-0.04 */
    switch (l->k) {
    case 3: {                                                                         /* the forge mouth */
        int32_t fl = 920 + p_sinr((int32_t)((t * 11u) % 6283u) + x * 1000) * 80 / 127, q, w = l->w, h = l->h;
        p_R(x - w / 2, y - h * 4 / 10, w, h * 8 / 10, ps_mix(RGB(0xe0, 0x70, 0x1c), RGB(0xff, 0xc4, 0x60), 64));
        p_ell(x, y - h * 4 / 10, w / 2, 9, RGB(0xf0, 0x90, 0x2c)), p_R(x - w / 3, y, w * 66 / 100, h * 35 / 100, RGB(0xff, 0xd8, 0x78)), p_ell(x, y + 6, w / 3, 5, RGB(0xff, 0xf0, 0xb8));
        for (q = 0; q < 5; q++) cv_pset(x - 8 + (int32_t)p_hs((uint32_t)q + t / 250u) * 16 / 1024, y - 8 - (int32_t)((t / 50u + (uint32_t)q * 7u) % 22u), RGB(0xff, 0xd8, 0x78));
        tw_glow(x, y, 46, (uint32_t)(200 * fl / 1000)), tw_glow(x, y, 26, (uint32_t)(220 * fl / 1000));
    } break;
    case 1:                                                                           /* a lamp */
        ig = tw_ign(l, x);
        if (ig <= 0) break;
        if (song.playing && tw.nt > 300) {                                              /* at night each lamp is one hero: it flashes on that hero's notes */
            uint32_t hh = (uint32_t)(sid % 4u), age = fm1_ms - qh_ms[hh];
            ig = ig * (age < 180u && qh_ms[hh] ? 350 + 650 * (int32_t)(180u - age) / 180 : 350) / 1000;
        }
        bu = ig < 950 ? 1000 - (ig > 500 ? ig - 500 : 500 - ig) * 2 : 0;
        p_R(x - 2, y - 2, 5, 5, ig > 350 ? RGB(0xff, 0xd8, 0x78) : RGB(0x8a, 0x70, 0x40));
        tw_glow(x, y, 26, (uint32_t)((200 + f) * ig / 1000)), tw_glow(x, y, 14, (uint32_t)(240 * ig / 1000)), tw_glow(x, y, 7, (uint32_t)(300 * ig / 1000));
        if (bu > 0) tw_glow(x, y, 38, (uint32_t)(220 * bu / 1000)), cv_pset(x - 3, y - 4 - bu * 3 / 1000, RGB(0xff, 0xf0, 0xb0)), cv_pset(x + 3, y - 3 - bu * 4 / 1000, RGB(0xff, 0xd8, 0x78));
        { int32_t dy; for (dy = -7; dy <= 7; dy++) { int32_t w = 30 * q_isqrt((49 - dy * dy) * 1024) / (32 * 7); tw_blend(x - w, oy + 12 + dy - 0, 2 * w + 1, 1, RGB(255, 170, 60), (uint32_t)(26 * ig / 1000)); } }
        break;
    case 2: {                                                                         /* a wall torch */
        int32_t fx = p_sinr((int32_t)((t * 13u) % 6283u) + x * 3000) * 12 / (127 * 10);
        ig = tw_ign(l, x);
        if (ig <= 0) break;
        bu = ig < 950 ? 1000 - (ig > 500 ? ig - 500 : 500 - ig) * 2 : 0;
        up = 350 + 650 * ig / 1000 + bu / 2;
        p_poly((const int32_t[5][2]){{x - 2, y}, {x - 1 + fx, y - 7 * up / 1000}, {x, y - 3 * up / 1000}, {x + 2 + fx, y - 8 * up / 1000}, {x + 3, y}}, 5, RGB(0xff, 0x9a, 0x30));
        p_tri(x - 1, y, x + fx / 2, y - 4 * up / 1000, x + 1, y, RGB(0xff, 0xe0, 0x90));
        tw_glow(x, y - 3, 20, (uint32_t)(200 * ig / 1000 + 150 * bu / 1000)), tw_glow(x, y - 3, 9, (uint32_t)(200 * ig / 1000));
        if (bu > 0) cv_pset(x - 2, y - 9 - bu * 4 / 1000, RGB(0xff, 0xf0, 0xb0)), cv_pset(x + 3, y - 8 - bu * 5 / 1000, RGB(0xff, 0xd8, 0x78));
    } break;
    case 0:                                                                           /* a window */
        if ((tw.nt > 220 || l->always || (tw.gold > 450 && x < 118)) && (l->always || p_hs(sid) > 307u)) {
            p_R(x, y, l->w, l->h, RGB(0xf0, 0xc8, 0x60)), p_R(x + 1, y + 1, q_max(1, l->w - 2), 2, RGB(0xff, 0xf0, 0xb0));
            p_R(x + l->w / 2, y, 1, l->h, RGB(0xb0, 0x8a, 0x3a));
            if (l->h > 8) p_R(x, y + l->h / 2, l->w, 1, RGB(0xb0, 0x8a, 0x3a));
            tw_glow(x + l->w / 2, y + l->h / 2, 12, 100);
        }
        break;
    default: break;
    }
}
static UI_NOINLINE void tw_guild(int32_t x, int32_t by)                                 /* the adventurers' guild, next to the quest board: stone, a red roof, a crest over the door */
{
    const uint16_t stone = tw_tint(RGB(0x7a, 0x84, 0x98)), dk = tw_tint(RGB(0x5a, 0x62, 0x74)), roof = tw_tint(RGB(0x9a, 0x3a, 0x32)), roof2 = tw_tint(RGB(0x7a, 0x2a, 0x26)),
                   wood = tw_tint(RGB(0x6a, 0x42, 0x24)), gold = tw_tint(RGB(0xe0, 0xb8, 0x4a)), red = tw_tint(RGB(0xb8, 0x3a, 0x2a)), white = tw_tint(RGB(0xf0, 0xea, 0xd0));
    int32_t r, cx = x + 62, wl = tw.nt > 100;
    p_R(x + 4, by - 62, 116, 62, stone);
    for (r = 0; r < 7; r++) p_R(x + 4, by - 62 + r * 9, 116, 1, dk);
    p_quad(x - 2, by - 62, x + 126, by - 62, x + 106, by - 94, x + 18, by - 94, roof);                       /* the roof, a hip */
    for (r = 0; r < 4; r++) p_R(x + 2 + r * 4, by - 66 - r * 8, 120 - r * 8, 1, roof2);
    p_R(x + 4, by - 63, 116, 2, roof2);
    p_R(x + 14, by - 46, 14, 18, wl ? tw_tint(RGB(0xf0, 0xc8, 0x60)) : dk), p_R(x + 96, by - 46, 14, 18, wl ? tw_tint(RGB(0xf0, 0xc8, 0x60)) : dk);   /* the windows */
    p_R(x + 20, by - 46, 2, 18, stone), p_R(x + 102, by - 46, 2, 18, stone);
    p_R(cx - 12, by - 32, 24, 32, wood), p_ell(cx, by - 32, 12, 9, wood), p_R(cx, by - 40, 1, 40, dk);            /* the door, arched */
    p_R(cx - 4, by - 18, 2, 3, gold), p_R(cx + 3, by - 18, 2, 3, gold);
    p_R(cx - 13, by - 3, 26, 3, dk);                                                                          /* the step */
    p_R(cx - 12, by - 60, 24, 14, red), p_R(cx - 12, by - 46, 24, 1, gold);                                   /* the crest: a shield in gold and red, a sword on it */
    p_tri(cx - 12, by - 46, cx, by - 33, cx + 12, by - 46, red);
    p_R(cx - 12, by - 60, 24, 1, gold), p_R(cx - 12, by - 60, 1, 14, gold), p_R(cx + 11, by - 60, 1, 14, gold);
    p_R(cx - 1, by - 57, 3, 20, white), p_R(cx - 5, by - 49, 11, 2, gold), p_R(cx - 1, by - 38, 3, 3, gold);
    p_R(cx - 40, by - 56, 7, 18, red), p_tri(cx - 40, by - 38, cx - 36, by - 33, cx - 33, by - 38, red), p_R(cx - 41, by - 57, 9, 1, gold);   /* two banners */
    p_R(cx + 33, by - 56, 7, 18, red), p_tri(cx + 33, by - 38, cx + 37, by - 33, cx + 40, by - 38, red), p_R(cx + 32, by - 57, 9, 1, gold);
}
static UI_NOINLINE void tw_lights(void)
{
    int32_t i, j, x;
    for (i = 0; i < (int32_t)(sizeof TD_WORLD / sizeof TD_WORLD[0]); i++) {
        uint32_t st = (uint32_t)TD_WORLD[i][0];
        x = tw_wrap(TD_WORLD[i][1] - tw.cam, TW_PER, 300);
        if (x > 244 || x + TD_WORLD[i][2] + 40 < -20 || st == TS_STONE) continue;
        for (j = TD_LI[st]; j < TD_LI[st + 1u]; j++) tw_light(&TD_LT[j], x, tw.by, (uint32_t)(st * 16 + (uint32_t)(j - TD_LI[st])));
    }
    for (i = 0; i < (int32_t)(sizeof TD_LAMPS / sizeof TD_LAMPS[0]); i++) {
        x = tw_wrap(TD_LAMPS[i] - tw.cam, TW_PER, 300);
        if (x > -10 && x < 250) for (j = TD_LI[TS_LAMP]; j < TD_LI[TS_LAMP + 1]; j++) tw_light(&TD_LT[j], x, tw.by, (uint32_t)i);
    }
}
/* ---- the people ---- */
static UI_NOINLINE void tw_one(uint32_t type, int32_t i, int32_t m, int32_t by)    /* one person: 0 street, 1 crowd, 2 companion, 3 hero, 4 foreground walker */
{
    if (type == 0) {                                                                   /* the street: at their doors, the smiths hammering */
        int32_t x = tw_wrap(TW_CIT[i].wx - tw.cam, TW_PER, 300);
        tw_person(i, x, by + 10 + (i % 3) * 5, TW_CIT[i].s, TW_CIT[i].d < 0, TW_CIT[i].a ? 2 : 0, TW_NPC[TW_CIT[i].c]);
    } else if (type == 1) {                                                            /* the crowd: three rows, some sit, some walk */
        /* the guild cycle: a while after the party goes by the guild, 0..10 villagers set off in pairs, at random moments, from places ahead of it; they walk (some run) to the next
         * guild and stop around the quest board, mostly in front of it, talking in pairs or smoking; after a while they go in at the guild door. Slot i >= 12: the same slot
         * for the lap before (a walker can take longer than a lap). The party walks on past them. */
        int32_t cam = tw.cam + 80 - TW_G + TW_PER, lap = cam / TW_PER, u = cam % TW_PER, lk = i / 12, k = lap - lk, sl = i % 12, second = sl & 1, row = sl % 3;
        uint32_t hk = qhash((uint32_t)(sl >> 1) * 977u + (uint32_t)k * 131u + 11u), cnt = qhash((uint32_t)k * 7919u + 5u) % 11u;
        static const uint8_t SPD[8] = {24, 26, 28, 30, 32, 36, 44, 58};
        if (!tw.now || qhash((uint32_t)(sl >> 1) * 331u + (uint32_t)k * 17u + 3u) % 6u >= (cnt + 1u) / 2u) return;       /* this pair is not out this lap (pairs: 6 slots of two) */
        {
            int32_t tau_s = 4000 + (int32_t)(hk % 10000u) + second * 500, e = (u + lk * TW_PER) * 50 - tau_s, sx = 130 + (int32_t)((hk >> 14) % 110u), v = SPD[(hk >> 4) & 7u];
            int32_t xs = tau_s / 50 - 80 + sx + second * 14, r = (int32_t)((hk >> 8) % 100u), sp = r < 70 ? (int32_t)((hk >> 16) % 47u) - 23 : r < 85 ? (int32_t)((hk >> 16) % 36u) - 60 : 25 + (int32_t)((hk >> 16) % 36u);
            int32_t xt = TW_PER + sp + second * 14, door = TW_PER + 93, ea, dw = 18000 + (int32_t)((hk >> 20) % 24000u), xe, sgn = door > xt ? 1 : -1, ex, act = (int32_t)((hk >> 26) & 3u), px, flip, kind, fade = 0, stage;
            if (e < 0) return;
            ea = (xt - xs) * 1000 / v, ex = (door - xt) * sgn * 1000 / v;
            if (e < ea) xe = xs + v * e / 1000, stage = 0;
            else if (e < ea + dw) xe = xt, stage = 1;
            else if (e < ea + dw + ex) xe = xt + sgn * v * (e - ea - dw) / 1000, stage = 2;
            else return;
            px = xe - (u + lk * TW_PER) + 80;                                          /* the screen x */
            if (stage == 0 && e < 500) fade = 1;                                       /* they come out of a doorway: the dither fills in */
            if (stage == 2 && e > ea + dw + ex - 500) fade = 1;                       /* and go in at the guild door */
            flip = stage == 0 ? 0 : stage == 2 ? sgn < 0 : second;
            kind = stage == 1 ? (act == 3 ? 5 : act == 2 ? 0 : 4) : 1;
            tw_pace = (uint16_t)(700 + v * 12), tw_run = stage != 1 && v >= 44, tw_fade = (uint8_t)fade;
            tw_person(i + 20, px, by + 4 + (row == 0 ? 0 : row == 1 ? 9 : 20), (hk >> 24) % 9u == 0 ? 24 : 36, flip, kind, TW_NPC[(sl + (int32_t)(hk >> 20)) % 10]);
            tw_pace = 0, tw_run = 0, tw_fade = 0;
        }
    } else if (type == 2) {                                                            /* the companions: waiting in the street until the party reaches them */
        if (!((0x4Au >> i) & 1u)) return;                                              /* only three of the eight keep company: the party is not a crowd */
        int32_t sx = 122 + i * 13, wxs = TW_COMP[i].wx - m, x = q_max(sx, wxs), joined = wxs <= sx, y = by + 14 + (i % 3) * 3, s = TW_COMP[i].s <= 32 ? 0 : 1;
        uint32_t fr = (uint32_t)((tw.now / 180u + (uint32_t)i) & 1u);
        if (joined) {
            x = sx + p_sinr((int32_t)((tw.now * (6u + (uint32_t)i * 2u) / 10u) % 6283u) + i * 2000) * (3 + i) / 254;     /* each drifts at its own pace round its place */
            if (m > TW_PER - 120) x += (m - (TW_PER - 120)) * 2;                       /* (they leave toward the end of the lap, so the street can start over) */
        } else if (x > 270 || x < -30) return;
        if (x > 262) return;
        tw_blend(x - (s ? 7 : 4), y - 1, s ? 14 : 8, 2, RGB(0, 0, 0), 71);
        qf_walk = (uint8_t)joined;
        qf_speed = (uint16_t)(760 + (i * 97) % 6 * 90);                                 /* a pace of its own: 0.76 .. 1.2 x */
        qf_scale = (uint16_t)(TW_COMP[i].s * 256 / 40);
        tw_spr(Q_HERO[i & 3] + (joined ? fr : (uint32_t)((tw.now / 600u) & 1u)), x, y, s ? 1 : 0, 0, TW_COMP[i].c);
        qf_scale = 256, qf_walk = 0, qf_speed = 1000;
        if (i == 6) tw_glow(x + 12, y - 24 + p_sinr((int32_t)((tw.now * 3u) % 6283u)) * 2 / 127, 10, 250), p_ell(x + 12, y - 24, 2, 2, RGB(0xba, 0xff, 0xc0));
    } else if (type == 3) {                                                            /* the four heroes: whoever rests stops on the road; at level again he runs in from the left */
        static uint32_t rest_t0[4], back_t0[4];
        static int32_t rest_x0[4];
        static uint8_t was[4] = {2, 2, 2, 2};
        int32_t xh = 56 + i * 16 + p_sinr((int32_t)((tw.now * 6u / 10u) % 6283u) + i * 1000) * 3 / 254, x = xh, y = by + 12 + (i % 2) * 5, mode = 1;   /* mode 1 walks, 2 runs, 0 stands */
        uint32_t fr = (uint32_t)((tw.now / 180u + (uint32_t)i) & 1u), rest = q_rest((uint32_t)i), now = tw.now;
        if (was[i] == 2u) { was[i] = (uint8_t)rest; rest_t0[i] = now - 12000u, rest_x0[i] = xh; }          /* (the first look: a resting hero is already far behind) */
        if (rest && !was[i]) was[i] = 1, rest_t0[i] = now, rest_x0[i] = xh, back_t0[i] = 0;
        if (!rest && was[i]) was[i] = 0, back_t0[i] = now;
        if (rest) {                                                                    /* he stays where he stopped: the street carries him off to the left */
            x = rest_x0[i] - (int32_t)(20u * (now - rest_t0[i]) / 1000u), mode = 0;
            if (x < -30) return;
        } else if (back_t0[i] && now - back_t0[i] < 5000u) {                           /* back from the left edge: running, slowing to a walk at his place */
            int32_t dur = (xh + 24) * 1000 / 140 + 300, el = (int32_t)(now - back_t0[i]), u = el >= dur ? 256 : el * 256 / dur, e = 256 - (256 - u) * (256 - u) / 256;
            if (el >= dur) back_t0[i] = 0;
            else x = -24 + (xh + 24) * e / 256, mode = u < 170 ? 2 : 1;
        }
        tw_blend(x - 7, y - 1, 14, 2, RGB(0, 0, 0), 71);
        qf_walk = (uint8_t)mode;
        tw_spr(Q_HERO[i] + (mode ? fr : 0u), x, y, 1, 0, 0);
        qf_walk = 0;
        {   /* the little square over him: lit = playing the loop, an outline = has a part, stopped, dim = resting */
            uint32_t hit = qh_ms[i] && now - qh_ms[i] < 140u;
            if (rest) p_R(x - 2, y - 44, 4, 4, RGB(0x4a, 0x3c, 0x48));
            else if (song.playing) p_R(x - 2, y - 44, 4, 4, hit ? RGB(0xff, 0xff, 0xff) : Q_TRK[i]);
            else if (!track_empty(&trk[i])) p_R(x - 2, y - 44, 4, 1, Q_TRK[i]), p_R(x - 2, y - 41, 4, 1, Q_TRK[i]), p_R(x - 2, y - 43, 1, 2, Q_TRK[i]), p_R(x + 1, y - 43, 1, 2, Q_TRK[i]);
        }
    } else {                                                                           /* near the camera: bigger, walking against the flow, passing through */
        static const uint8_t FSP[5] = {22, 34, 50, 72, 100};                             /* walkers close to the camera: a stroll up to a run */
        int32_t x = 300 - (int32_t)((tw.now * FSP[i % 5] / 1000u + (uint32_t)i * 140u) % 380u);
        uint32_t fr = (uint32_t)((tw.now / (200u - FSP[i % 5] / 2u) + (uint32_t)i) & 1u);
        if (x < -30 || x > 270) return;
        tw_blend(x - 12, by + 25, 24, 2, RGB(0, 0, 0), 71);
        qf_walk = FSP[i % 5] >= 60 ? 2u : 1u;                                           /* the fast ones run */
        qf_scale = 400, qf_fade = 0, qf_speed = (uint16_t)(700 + FSP[i % 5] * 7);        /* big and solid: easy to see */                                                    /* close to the camera: bigger, with every other row cut out like a dithered transparency */
        tw_spr(Q_HERO[(i + 1) & 3] + fr, x, by + 27, 1, 1, TW_NPC[(i * 3 + 1) % 10]);
        qf_scale = 256, qf_walk = 0, qf_fade = 0, qf_speed = 1000;
    }
}
static UI_NOINLINE void tw_people(void)                                                /* everybody, drawn from the back to the front by their feet */
{
    struct { int16_t y; uint8_t type, i; } q[64];
    int32_t i, j, n = 0, m = tw.cam % TW_PER, by = tw.by;
    for (i = 0; i < 20; i++) q[n].y = (int16_t)(10 + (i % 3) * 5), q[n].type = 0, q[n++].i = (uint8_t)i;
    for (i = 0; i < 24; i++) { int32_t row = (i % 12) % 3; q[n].y = (int16_t)(4 + (row == 0 ? 0 : row == 1 ? 9 : 20)), q[n].type = 1, q[n++].i = (uint8_t)i; }
    for (i = 0; i < 8; i++) q[n].y = (int16_t)(14 + (i % 3) * 3), q[n].type = 2, q[n++].i = (uint8_t)i;
    for (i = 0; i < 4; i++) q[n].y = (int16_t)(12 + (i % 2) * 5), q[n].type = 3, q[n++].i = (uint8_t)i;
    for (i = 0; i < 5; i++) q[n].y = 27, q[n].type = 4, q[n++].i = (uint8_t)i;
    for (i = 1; i < n; i++) {                                                          /* insertion sort, stable: the same row keeps its order */
        typeof(q[0]) t = q[i];
        for (j = i - 1; j >= 0 && q[j].y > t.y; j--) q[j + 1] = q[j];
        q[j + 1] = t;
    }
    for (i = 0; i < n; i++) tw_one(q[i].type, q[i].i, m, by);
}
static UI_NOINLINE void tw_clock(void)                                                 /* the hour clock: 4 slices, the living one lit, a pulsing hand */
{
    static const struct { int16_t a0, a1; uint16_t c; } SL[4] = {{135, 225, RGB(0xf0, 0x8a, 0x6a)}, {225, 315, RGB(0xf2, 0xd0, 0x70)}, {315, 405, RGB(0xe0, 0x60, 0x3a)}, {45, 135, RGB(0x4a, 0x56, 0xa8)}};
    const uint16_t bg = RGB(0x14, 0x10, 0x1a);
    int32_t cx = 17, cy = PS_Y0 + 17, x, y, am, pul = 1000 + p_sin(tw.now, 40) * 250 / 127;
    for (y = -15; y <= 15; y++)
        for (x = -15; x <= 15; x++) {
            int32_t d2 = x * x + y * y, a, aa, k, tick = ((x == -1 || x == 0) && ((y >= -14 && y <= -11) || (y >= 10 && y <= 13))) || ((y == -1 || y == 0) && ((x >= -14 && x <= -11) || (x >= 10 && x <= 13)));
            uint16_t c = bg;
            if (d2 > 225) continue;
            if (d2 >= 196) c = RGB(0xc8, 0x96, 0x3c);                                  /* the gold ring */
            else if (!tick) {
                a = q_atan(y, x);
                for (k = 0; k < 4; k++) {
                    aa = a < SL[k].a0 ? a + 360 : a;
                    if (aa >= SL[k].a0 + 2 && aa < SL[k].a1 - 2 && d2 <= (k == tw.hour ? 144 : 100)) c = k == tw.hour ? SL[k].c : ps_mix(SL[k].c, bg, 174);
                }
            }
            cv_pset(cx + x, cy + y, c);
        }
    am = (SL[tw.hour].a0 + SL[tw.hour].a1) / 2 * 64 / 360;
    cv_line(cx, cy, cx + qcos(am) * 9 * pul / 127000, cy + qsin(am) * 9 * pul / 127000, RGB(0xff, 0xf6, 0xd8));
    p_ell(cx, cy, 2, 2, RGB(0xff, 0xf6, 0xd8));
}
static UI_NOINLINE void tw_fg(void)                                                    /* the dark foreground: flowers, barrels, wheels, sacks, buckets */
{
    static const int16_t FG[9][2] = {{30, 0}, {170, 1}, {420, 2}, {610, 3}, {820, 4}, {980, 2}, {1190, 1}, {1330, 0}, {1480, 3}};
    const uint16_t dk = RGB(0x12, 0x16, 0x1a), mid = RGB(0x2a, 0x30, 0x36);
    int32_t i, q, b = TW_Y0 + 175;
    for (i = 0; i < 9; i++) {
        int32_t x = tw_wrap(FG[i][0] - tw.cam * 3 / 2, TW_PER * 3 / 2, 200);
        if (x < -60 || x > 270) continue;
        switch (FG[i][1]) {
        case 0: for (q = 0; q < 12; q++) p_R(x + q * 4 - 8, b - 6 - (q * 7 % 9), 3, 8 + (q * 7 % 9), RGB(0x1a, 0x2a, 0x22)), p_R(x + q * 4 - 9, b - 8 - (q * 7 % 9), 5, 3, RGB(0x6a, 0x78, 0xc8)); break;
        case 1: p_R(x, b - 22, 24, 24, dk), p_R(x - 1, b - 14, 26, 2, mid), p_R(x - 1, b - 6, 26, 2, mid); break;
        case 2: p_ell(x + 20, b + 6, 26, 26, dk), p_ell(x + 20, b + 6, 21, 21, RGB(0x1c, 0x22, 0x26)), p_ell(x + 20, b + 6, 5, 5, dk);
                for (q = 0; q < 6; q++) cv_line(x + 20, b + 6, x + 20 + qcos(q * 64 / 6) * 24 / 127, b + 6 + qsin(q * 64 / 6) * 24 / 127, dk);
                break;
        case 3: p_ell(x + 12, b - 4, 16, 14, dk), p_ell(x + 34, b - 2, 12, 10, dk), p_R(x + 10, b - 18, 4, 4, dk); break;
        default: p_R(x, b - 16, 14, 18, dk), p_ell(x + 7, b - 16, 7, 3, mid), p_R(x + 12, b - 24, 2, 10, dk);
        }
    }
}
static UI_NOINLINE void tw_scene(const ps_c *c)
{
    int32_t i, x, by = tw.by;
    (void)c;
    tw_sky();
    tw_far(16, TW_Y0 + TW_GY - 30, tw.cam, RGB(0x3a, 0x6a, 0x62), tw.sk[3]);
    tw_far(40, TW_Y0 + TW_GY - 6, tw.cam + 140, RGB(0x5a, 0x6a, 0x6a), tw.sk[3]);
    tw_birds();
    q_pixelize(0, TW_Y0, 240, TW_Y0 + TW_GY, 3);                                          /* the sky and the far hills as pixel art; the houses keep their thin lines */
    for (i = 0; i < (int32_t)(sizeof TD_WORLD / sizeof TD_WORLD[0]); i++) {
        x = tw_wrap(TD_WORLD[i][1] - tw.cam, TW_PER, 300);
        if (x > 244 || x + TD_WORLD[i][2] + 40 < -20) continue;
        if (TD_WORLD[i][0] == TS_STONE) tw_guild(x, by);
        else tw_station((uint32_t)TD_WORLD[i][0], x, by);
    }
    for (i = 0; i < (int32_t)(sizeof TD_LAMPS / sizeof TD_LAMPS[0]); i++) {
        x = tw_wrap(TD_LAMPS[i] - tw.cam, TW_PER, 300);
        if (x > -10 && x < 250) tw_station(TS_LAMP, x, by);
    }
    tw_ground();
    for (i = 0; i < 5; i++) {                                                          /* cats and dogs */
        int32_t px = tw_wrap(TW_PETX[i] - tw.cam, TW_PER, 300) + p_sinr((int32_t)((tw.now * 6u / 10u) % 6283u) + i * 1000) * 6 / 127, gy = by + 16 + (i & 1) * 4, s = i == 1 ? 14 : 10;
        uint16_t pc = tw_tint(TW_PETC[i]);
        if (px < -10 || px > 250) continue;
        p_ell(px, gy - 3 * s / 10, 4 * s / 10, 24 * s / 100 + 1, pc), p_ell(px + 4 * s / 10, gy - 5 * s / 10, 2 * s / 10, 2 * s / 10, pc);
        cv_pset(px + 4 * s / 10, gy - 8 * s / 10, pc), cv_pset(px + 6 * s / 10, gy - 8 * s / 10, pc);
        p_R(px - 6 * s / 10, gy - 5 * s / 10 - p_sinr((int32_t)((tw.now * 6u) % 6283u) + px * 1000) * 12 / 1270, 2 * s / 10, 1, pc), p_R(px - 3 * s / 10, gy - 1, 1, 2, pc), p_R(px + 2 * s / 10, gy - 1, 1, 2, pc);
    }
    tw_station(TS_HORSE0, tw_wrap(400 - tw.cam, TW_PER, 300), by + 8);
    tw_station(TS_HORSE1, tw_wrap(872 - tw.cam, TW_PER, 300), by + 10);
    if (tw.gold > 50) tw_blend(0, TW_Y0, 240, 37, RGB(255, 200, 90), (uint32_t)tw.gold * 31u / 1000u);
    tw_lights();
    tw_people();
    tw_fg();
    {   /* the vignette: soft at midday, heavier as the afternoon goes (the default), the filter's own otherwise */
        int32_t st = q_filter == 0u ? tw.go * 2 : Q_FLT[q_filter % 12u].vig * 3, k;
        for (k = 0; st > 0 && k < 8; k++) {
            uint32_t al = (uint32_t)(st * (8 - k) / 8);
            tw_blend(k * 4, TW_Y0 + 22, 4, 153, RGB(0, 0, 0), al), tw_blend(236 - k * 4, TW_Y0 + 22, 4, 153, RGB(0, 0, 0), al);
        }
    }
    tw_clock();
}
/* ---- the hour, the camera, the driver ---- */
static UI_NOINLINE void tw_prepare(void)
{
    static const int16_t DK[7][2] = {{0, 0}, {420, 0}, {580, 1}, {720, 2}, {860, 2}, {950, 3}, {1000, 0}};
    int32_t pr = (int32_t)(tw.now % TW_DAY_MS * 1000u / TW_DAY_MS), k = 0, u, n, i;
    uint32_t go, na;
    while (k < 5 && pr > DK[k + 1][0]) k++;
    u = (pr - DK[k][0]) * 256 / (DK[k + 1][0] - DK[k][0]);
    for (i = 0; i < 5; i++) tw.sk[i] = ps_mix(TD_SKY[DK[k][1]][i], TD_SKY[DK[k + 1][1]][i], (uint32_t)u);
    n = pr < 560 ? 0 : pr < 720 ? (pr - 560) * 1000 / 160 : pr < 860 ? 1000 : 1000 - (pr - 860) * 950 / 140;
    tw.nt = n / 2;                                                                     /* 0 .. 0.5, like the lab */
    tw.gold = pr > 300 ? q_max(0, 1000 - (pr > 520 ? pr - 520 : 520 - pr) * 1000 / 140) : 0;
    go = (uint32_t)tw.gold * 56u / 1000u, na = (uint32_t)tw.nt * 205u / 1000u;        /* the wash: 22 % orange, 80 % of the night */
    tw.go = (int32_t)go, tw.na = (int32_t)na, tw.pr = pr;
    tw.hour = pr < 420 ? 1 : pr < 720 ? 2 : pr < 900 ? 3 : 0;
    tw.sr = 0;                                                                         /* the sun: rising at dawn, high in the afternoon, red and sinking at sunset */
    if (pr >= 860 || pr < 720) {
        int32_t e = pr >= 860 ? (pr - 860) * 256 / 140 : 256, ez = 256 - (256 - e) * (256 - e) / 256;
        tw.sx = 176, tw.warm[0] = 255, tw.warm[1] = 190, tw.warm[2] = 130, tw.sun = RGB(0xff, 0xf0, 0xc0);
        if (pr >= 860) tw.sy = 54 + (17 - 54) * ez / 256, tw.sr = 10, tw.glow = 450 + 550 * ez / 256, tw.flare = 1000 * ez / 256;
        else {
            int32_t g = pr < 420 ? 0 : pr < 580 ? (pr - 420) * 256 / 160 : 256;         /* afternoon -> sunset */
            tw.sy = 17 + 11 * g / 256 + (pr > 580 ? (pr - 580) * 92 / 140 : 0), tw.sr = 7 + 5 * g / 256, tw.sx = 172 + 4 * g / 256;
            tw.glow = 700 + 300 * g / 256, tw.flare = 300 + 400 * g / 256;
            tw.warm[0] = 255, tw.warm[1] = (uint8_t)(235 - 75 * g / 256), tw.warm[2] = (uint8_t)(170 - 90 * g / 256);
            tw.sun = ps_mix(RGB(0xff, 0xfb, 0xe0), RGB(0xff, 0xb8, 0x60), (uint32_t)g);
        }
    }
    for (i = 0; i < TW_NCOL; i++) tw.tc[i] = tw_tint(TD_COL[i]);
}
static UI_NOINLINE int tw_budget(int32_t cost)                                          /* like q_scene_budget (ui_quest_screens.c), the village's own pace: a bigger picture */
{
    uint32_t now = fm1_ms, dt = qb.tok_ms ? now - qb.tok_ms : 0u, share = 100u, period = 40u;                     /* the village is the screen: all the SPI time, a frame every 40 ms where it fits */
    if (dt > 250u) dt = 250u;
    qb.tok_ms = now;
    qb.tokens += (int32_t)(dt * Q_SPI_BPMS / 100u * share);
    if (qb.tokens > 375000) qb.tokens = 375000;
    if (Q_FULL) { qb.tokens -= cost; if (qb.tokens < -375000) qb.tokens = -375000; qs.scene_ms = now; return 1; }
    if (now - qs.scene_ms < period || qb.tokens < cost) return 0;
    qs.scene_ms = now;
    qb.tokens -= cost;
    return 1;
}
/* the strip under the street (where the dials were): the selected hero and its sound, then for each hero its whole part: 16 squares a line and the next 16 under them, as
 * far as the part goes; the playing step lit, a hero in rest has no colour at all */
static UI_NOINLINE void tw_party_strip(void)
{
    static const uint8_t ORD[4] = {3, 0, 1, 2};
    uint32_t r, j, sig = 17u + song.sel * 131u + song.playing * 7u;
    const track_t *ts = &trk[song.sel];
    for (r = 0; r < 4u; r++) {
        const track_t *t = &trk[ORD[r]];
        uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP);
        sig = sig * 31u + (uint32_t)(t->seq_idx % 64u) + q_rest(ORD[r]) * 3u + len * 5u + (uint32_t)(qh_ms[ORD[r]] && fm1_ms - qh_ms[ORD[r]] < 120u) * 7u;
        for (j = 0; j < len; j++) sig = sig * 3u + (uint32_t)trk_step_on(t, j);
    }
    sig = sig * 31u + (uint32_t)ts->eng_req * 17u + ts->preset + q_var[2] * 5u + q_var[3] * 9u + (is_drum(ts) ? (uint32_t)drum_kit() * 13u : 0u);
    if (!q_changed(14, sig)) return;
    cv_begin(240, 68, Q_BG);
    {   /* the selected hero and its sound */
        char nm[16];
        const engine_t *e = ENGINES[ts->eng_req % NENGINES];
        int32_t x;
        if (is_drum(ts)) str_cpy(nm, DRUM_KIT_NAMES[drum_kit()], sizeof nm);
        else if (ts->user) str_cpy(nm, "USER", sizeof nm);
        else if (e->npresets) str_cpy(nm, e->presets[ts->preset % e->npresets].name, sizeof nm);
        else str_cpy(nm, e->name, sizeof nm);
        cv_rect(4, 2, 7, 7, Q_TRK[song.sel]);
        x = q_text(16, 0, q_var[song.sel] ? Q_ALT_NAME[song.sel] : Q_HERO_NAME[song.sel], Q_TRK[song.sel]);
        (void)x;
        q_text(16 + text_w(&FONT_P, q_var[song.sel] ? Q_ALT_NAME[song.sel] : Q_HERO_NAME[song.sel]) + 10, 0, nm, Q_TEXT);
    }
    {
        int32_t y = 14;
        for (r = 0; r < 4u; r++) {
            uint32_t i = ORD[r];
            const track_t *t = &trk[i];
            uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), pos = t->seq_idx % len, silent = q_rest(i), hit = qh_ms[i] && fm1_ms - qh_ms[i] < 120u, lines = (len + 15u) / 16u, ln;
            cv_rect(2, y, i == song.sel ? 8 : 3, lines * 4u - 1u > 3u ? (int32_t)(lines * 4u - 1u) : 3, silent ? Q_DIM : Q_TRK[i]);
            for (ln = 0; ln < lines; ln++, y += 4)
                for (j = 0; j < 16u; j++) {
                    uint32_t p = ln * 16u + j, on = p < len ? (trk_step_on(t, p) ? 2u : 1u) : 0u;
                    uint16_t c;
                    if (!on) continue;
                    c = on == 2u ? (silent ? Q_OFFHI : Q_TRK[i]) : Q_OFF;                       /* in rest: nothing coloured */
                    if (song.playing && !silent && p == pos) c = on == 2u && hit ? RGB(0xff, 0xff, 0xff) : on == 2u ? Q_TEXT : Q_OFFHI;
                    cv_rect(14 + (int32_t)j * 14, y, 11, 3, c);
                }
            y += 2;
        }
    }
    cv_blit(0, PS_Y1 - 22);
}
static UI_NOINLINE void quest_town_draw(void)
{
    ps_c c;
    uint32_t now = fm1_ms, dt = tw.last_ms ? now - tw.last_ms : 0u;
    q_tick_clocks();
    q_enter(QSN_TOWN);
    tw.last_ms = now;
    if (dt > 250u) dt = 250u;
    tw.cam_q8 += dt * 20u * 256u / 1000u;                                              /* the street walks at 20 px/s, play or not */
    tw.cam = (int32_t)(tw.cam_q8 >> 8) % 1440000, tw.now = now, tw.by = TW_Y0 + TW_GY - 2;
    if (q_changed(0, studio_hash((uint32_t)song.g[G_BPM] * 7u + song.playing * 3u + (song.playing ? (clk_beat % 4u) * 131u + (clk_beat / 4u) * 7919u : 0u) + q_msg_sig() +
                                 (ui.bpm_t != 0) * 31u, "town"))) {
        cv_begin(240, 19, Q_BG);
        q_bar_bpm_pos(0);
        q_title_r(236, "Village", Q_HI);
        q_homedots(150, 4, 0);
        q_bar_end();
    }
    tw_party_strip();
    if (tw_budget(240 * 153 * 2)) {                                                    /* the whole picture at once: a split by bands tears the houses in two */
        c.k = 0, c.a = c.b = c.c = c.d = 0, c.t = now;
        tw_prepare();
        ps_paint(tw_scene, &c, 0, PS_Y0, 239, PS_Y1 - 23);
    }
    qb.fresh = 0;
}
