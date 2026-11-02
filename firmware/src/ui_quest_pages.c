/* SPDX-License-Identifier: GPL-3.0-only
 * FM1 Quest, part 4: the scene pages (group 2), ported from the lab's pictures. One full-height scene (rows 19..193) above the four dials, for
 *   ENV -> Forge   LFO -> Sea   LFO DEST -> Alchemy   SLICER -> Towers   DLY -> Forest   REV/CHO -> Cathedral   SCL -> Torch wall
 * A scene is one function that paints everything from its page values and a clock. The LCD keeps what it was sent, so after a first full paint only the
 * rectangles that move (flames, hammer, waves, figures) are painted again: a window of the canvas (cv_ox / cv_oy) at ~30 frames a second. */
static uint32_t pn_t0;                   /* when a key was last pressed (the delay's rings leave on every note) */
#define PS_Y0 19
#define PS_Y1 194
typedef struct { int16_t x0, y0, x1, y1; uint8_t div; } ps_r;
typedef struct { const int32_t *k; int32_t a, b, c, d; uint32_t t; } ps_c;   /* k = the four ratios (0..1000), a..d = discrete values of the page, t = ms */
static UI_SMALL uint32_t p_hs(uint32_t n) { return qhash(n * 7919u + 13u) & 1023u; }       /* 0..1023, a fixed random per n */
static UI_SMALL int32_t p_sinr(int32_t rad_q1000) { int32_t i = (rad_q1000 * 64 / 6283) % 64; return qsin(i < 0 ? i + 64 : i); }
static UI_SMALL int32_t p_cosr(int32_t rad_q1000) { return p_sinr(rad_q1000 + 1571); }
static UI_SMALL int32_t p_sin(uint32_t t, uint32_t rad_x10) { return qsin((int32_t)(((t % 62832u) * rad_x10 / 982u) & 63u)); }   /* sin(t * rad), t in ms, rad / 10 */
static UI_SMALL uint16_t p_shade(uint16_t c, uint32_t f)                                   /* c * f / 1000 */
{
    uint32_t r = ((c >> 11) & 31u) * f / 1000u, g = ((c >> 5) & 63u) * f / 1000u, b = (c & 31u) * f / 1000u;
    return (uint16_t)((r > 31u ? 31u : r) << 11 | (g > 63u ? 63u : g) << 5 | (b > 31u ? 31u : b));
}
static UI_SMALL uint16_t ps_mix(uint16_t a, uint16_t b, uint32_t t)                          /* RGB565 blend, t 0..256 */
{
    uint32_t r = (((a >> 11) & 31u) * (256u - t) + ((b >> 11) & 31u) * t) >> 8, g = (((a >> 5) & 63u) * (256u - t) + ((b >> 5) & 63u) * t) >> 8, bl = ((a & 31u) * (256u - t) + (b & 31u) * t) >> 8;
    return (uint16_t)((r << 11) | (g << 5) | bl);
}
static UI_SMALL void p_R(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c) { cv_rect(x, y, w < 1 ? 1 : w, h < 1 ? 1 : h, c); }
static UI_SMALL void q_pixelize(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t bs)   /* pixel art for a background: blocks of bs x bs take the colour of their corner, the low bit dithers (scene coordinates) */
{
    static const uint8_t BY[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
    int32_t cy, x;
    int32_t cya = y0 + cv_oy < 0 ? 0 : y0 + cv_oy, cyb = y1 + cv_oy > (int32_t)cv_h ? (int32_t)cv_h : y1 + cv_oy;
    for (cy = cya; cy < cyb; cy++) {
        int32_t sy = cy - cv_oy, byi = sy >= 0 ? sy / bs : -((-sy + bs - 1) / bs), sry = byi * bs + cv_oy;
        if (sry < 0) sry = 0;
        for (x = x0; x < x1; x++) {
            int32_t cx = x + cv_ox, bxi = x / bs, srx = bxi * bs + cv_ox, r, g, b;
            uint32_t c, th;
            if (cx < 0 || cx >= (int32_t)cv_w) continue;
            if (srx < 0) srx = 0;
            c = swap16(cv_px[(uint32_t)sry * cv_w + (uint32_t)srx]);
            th = BY[((uint32_t)byi & 3u) * 4u + ((uint32_t)bxi & 3u)];
            r = (int32_t)((c >> 11) & 31u), g = (int32_t)((c >> 5) & 63u), b = (int32_t)(c & 31u);
            if ((r & 1) && r < 31 && th >= 8u) r++;
            if ((g & 1) && g < 63 && th >= 4u && th < 12u) g++;
            if ((b & 1) && b < 31 && th >= 12u) b++;
            cv_px[(uint32_t)cy * cv_w + (uint32_t)cx] = swap16(((uint32_t)(r & ~1) << 11) | ((uint32_t)(g & ~1) << 5) | (uint32_t)(b & ~1));
        }
    }
}
static UI_SMALL void p_ell(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint16_t c)
{
    int32_t dy;
    if (ry < 1) ry = 1;
    for (dy = -ry; dy <= ry; dy++) {
        int32_t w = rx * q_isqrt((ry * ry - dy * dy) * 1024) / (32 * ry);
        cv_rect(cx - w, cy + dy, 2 * w + 1, 1, c);
    }
}
static UI_SMALL void p_ellO(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint16_t c)
{
    int32_t k, n = q_max(8, q_max(rx, ry) * 3 / 2);
    for (k = 0; k < n; k++) cv_pset(cx + qcos(k * 64 / n) * rx / 127, cy + qsin(k * 64 / n) * ry / 127, c);
}
static UI_SMALL void p_poly(const int32_t (*P)[2], int32_t n, uint16_t c)
{
    int32_t y, y0 = 9999, y1 = -9999, i;
    for (i = 0; i < n; i++) { y0 = q_min(y0, P[i][1]); y1 = q_max(y1, P[i][1]); }
    for (y = q_max(y0, 0); y <= q_min(y1, 239); y++) {
        int32_t xs[8], m = 0, a, b;
        for (i = 0; i < n && m < 8; i++) {
            int32_t ax = P[i][0], ay = P[i][1], bx = P[(i + 1) % n][0], by = P[(i + 1) % n][1];
            if ((ay * 2 <= y * 2 + 1 && by * 2 > y * 2 + 1) || (by * 2 <= y * 2 + 1 && ay * 2 > y * 2 + 1)) xs[m++] = ax + (bx - ax) * (y * 2 + 1 - ay * 2) / (2 * (by - ay));
        }
        for (a = 1; a < m; a++) { int32_t v = xs[a]; for (b = a - 1; b >= 0 && xs[b] > v; b--) xs[b + 1] = xs[b]; xs[b + 1] = v; }
        for (a = 0; a + 1 < m; a += 2) if (xs[a + 1] > xs[a]) cv_rect(xs[a], y, xs[a + 1] - xs[a], 1, c);
    }
}
static UI_SMALL void p_tri(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t c)
{
    const int32_t P[3][2] = {{x0, y0}, {x1, y1}, {x2, y2}};
    p_poly(P, 3, c);
}
static UI_SMALL void p_quad(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, uint16_t c)
{
    const int32_t P[4][2] = {{x0, y0}, {x1, y1}, {x2, y2}, {x3, y3}};
    p_poly(P, 4, c);
}
static UI_SMALL void p_sky(int32_t y0, int32_t y1, const uint8_t *cols, int32_t n)           /* bands with a 2-row checker at each border (the lab's g.sky) */
{
    int32_t k, x, r;
    for (k = 0; k < n; k++) {
        int32_t a = y0 + k * (y1 - y0) / n, b = y0 + (k + 1) * (y1 - y0) / n;
        cv_rect(0, a, 240, b - a, QP(cols[k]));
        if (k) for (r = 0; r < 2; r++) for (x = r; x < 240; x += 2) cv_pset(x, a + r, QP(cols[k - 1]));
    }
}
static UI_SMALL void p_hero(uint32_t id, int32_t cx, int32_t feet, int32_t s, int solid)    /* a hero sprite, feet on `feet`, centred on cx */
{
    const qspr_t *sp = &QSPR[id];
    q_spr(id, cx - s * (sp->ox + sp->w / 2), feet - s * (sp->oy + sp->h), s, 0, solid);
}
static UI_SMALL int32_t p_lerp(int32_t a, int32_t b, int32_t u1000) { return a + (b - a) * u1000 / 1000; }
static UI_SMALL int32_t p_ease(int32_t u) { u = u < 0 ? 0 : u > 1000 ? 1000 : u; return u * u / 1000 * (3000 - 2 * u) / 1000; }
static const uint16_t PX_INK = RGB(0x1a, 0x10, 0x16);

/* ======================================================= ENV: the forge ======================================================= */
static struct { uint32_t cyc; int32_t heat, n, bx0, bx1, by0, by1; struct { int16_t x, y, vx, vy, vz; uint32_t t0; } sp[20]; } pf;
static UI_SMALL int32_t pf_xh(uint32_t cyc) { return 90 + (int32_t)p_hs(cyc * 7u) * 62 / 1024; }
static UI_SMALL int32_t pf_ph(uint32_t t) { return (int32_t)(t % 1500u) * 1000 / 1500; }      /* 0..1000 of the strike */
static UI_SMALL void pf_hammer(uint32_t t, int32_t *hx, int32_t *hy, int32_t *px, int32_t *py, int32_t *th)
{
    int32_t ph = pf_ph(t), xh = pf_xh(t / 1500u), L = 78;
    *th = ph < 120 ? p_lerp(-1200, 500, ph * 1000 / 120) : ph < 300 ? p_lerp(500, 150, p_ease((ph - 120) * 1000 / 180)) : p_lerp(150, -1200, p_ease((ph - 300) * 1000 / 700));
    *px = xh + 34, *py = 110 - 61 - 6;
    *hx = *px - L * p_sinr(*th) / 127, *hy = *py + L * p_cosr(*th) / 127;
}
static UI_SMALL void pf_tick(const ps_c *c)
{
    uint32_t cyc = c->t / 1500u;
    int32_t i;
    if (pf.cyc != cyc && pf_ph(c->t) > 120) {
        pf.cyc = cyc, pf.heat = 1000, pf.n = 16;
        for (i = 0; i < 16; i++) {
            pf.sp[i].x = (int16_t)pf_xh(cyc), pf.sp[i].y = 110;
            pf.sp[i].vx = (int16_t)(((int32_t)p_hs(cyc * 31u + (uint32_t)i) - 512) * 90 / 1024), pf.sp[i].vy = (int16_t)(-40 - (int32_t)p_hs(cyc * 17u + (uint32_t)i) * 70 / 1024);
            pf.sp[i].vz = (int16_t)(20 + (int32_t)p_hs((uint32_t)i + cyc) * 60 / 1024), pf.sp[i].t0 = c->t;
        }
    }
    pf.heat = pf.heat * 94 / 100;
    pf.bx0 = pf.bx1 = 0;
}
static UI_SMALL void ps_forge(const ps_c *c)
{
    int32_t k0 = c->k[0], A = 60 + k0 * 3 / 10, D = 60 + c->k[1] * 3 / 10, S = 200 + c->k[2] * 7 / 10, Rl = 100 + c->k[3] * 45 / 100, r, cc, i, xh = pf_xh(c->t / 1500u);
    int32_t fl = 700 + 300 * p_sin(c->t, 130) / 127 * p_sin(c->t, 70) / 127, hx, hy, px, py, th, nx, ny;
    static const uint8_t SKY[4] = {0, 0, 1, 0};
    p_sky(PS_Y0, 197, SKY, 4);
    for (r = 0; r < 8; r++) for (cc = 0; cc < 9; cc++) p_R(cc * 28 + (r & 1) * 14 - 8, 22 + r * 14, 26, 12, QP((r + cc) % 3 == 0 ? 5 : 1));
    p_ell(205, 70, 30, 30, QP(4)), p_ell(205, 72, 22, 22, QP(8)), p_ell(205, 74, 14 * fl / 1000 + 4, 14 * fl / 1000 + 4, QP(9)), p_ell(205, 76, 7 * fl / 1000 + 2, 7 * fl / 1000 + 2, QP(10));
    p_ell(120, 176, 76, 9, QP(0));                                                      /* the anvil */
    p_quad(70, 156, 172, 156, 182, 172, 60, 172, QP(5)), p_quad(88, 136, 154, 136, 160, 156, 82, 156, QP(5)), p_quad(58, 116, 182, 116, 176, 138, 64, 138, QP(6));
    p_tri(58, 116, 30, 124, 64, 138, QP(6)), p_R(58, 115, 124, 1, QP(7)), p_R(32, 124, 26, 1, QP(7));
    for (i = 0; i < 47; i++) {                                                          /* the blade: the ADSR as a silhouette, hot where the hammer struck */
        int32_t u = i * 1000 / 47, v = u < A ? u * 1000 / A : u < A + D ? p_lerp(1000, S, (u - A) * 1000 / D) : u < 1000 - Rl ? S : S * (1000 - (u - (1000 - Rl)) * 1000 / Rl) / 1000;
        int32_t h = q_max(2, v * 27 / 1000), x = 74 + u * 94 / 1000, dx = x > xh ? x - xh : xh - x, heat = (1000 - dx * 1000 / 38) * pf.heat / 1000 * 12 / 10;
        p_R(x, 112 - h, 2, h, QP(heat > 700 ? 10 : heat > 400 ? 9 : heat > 150 ? 8 : (i % 6 < 3 ? 6 : 7))), p_R(x, 112, 2, 2, QP(0));
    }
    p_R(68, 82, 5, 31, QP(10)), p_R(58, 91, 11, 9, QP(4)), p_R(61, 84, 14, 3, QP(6));        /* the hilt: taller, with a crossguard */
    { int32_t y; for (y = -4; y <= 4; y++) { int32_t w = q_isqrt(16 - y * y); p_R(55 - w, 94 + y, 2 * w + 1, 1, QP(10)); } }
    pf_hammer(c->t, &hx, &hy, &px, &py, &th);
    nx = p_cosr(th), ny = p_sinr(th);
    cv_line(px, py, hx, hy, QP(4)), cv_line(px + 1, py, hx + 1, hy, QP(4)), cv_line(px - 1, py, hx - 1, hy, QP(15));
    { int32_t ax = hx - 11 * nx / 127 - 3 * ny / 127, ay = hy - 11 * ny / 127 + 3 * nx / 127, bx = hx + 11 * nx / 127 - 3 * ny / 127, by = hy + 11 * ny / 127 + 3 * nx / 127,
              cx2 = hx + 11 * nx / 127 + 7 * ny / 127, cy2 = hy + 11 * ny / 127 - 7 * nx / 127, dx2 = hx - 11 * nx / 127 + 7 * ny / 127, dy2 = hy - 11 * ny / 127 - 7 * nx / 127;
      p_quad(ax, ay, bx, by, cx2, cy2, dx2, dy2, QP(6));
      cv_line(ax, ay, bx, by, QP(0)), cv_line(bx, by, cx2, cy2, QP(0)), cv_line(cx2, cy2, dx2, dy2, QP(0)), cv_line(dx2, dy2, ax, ay, QP(0)); }
    for (i = 0; i < pf.n; i++) {
        int32_t u = (int32_t)(c->t - pf.sp[i].t0);
        if (u < 0 || u > 900) continue;
        { int32_t x = pf.sp[i].x + pf.sp[i].vx * u / 1000, y = pf.sp[i].y + pf.sp[i].vy * u / 1000 + 160 * u / 1000 * u / 1000, s = 1 + pf.sp[i].vz * u / 1000 / 22;
          p_R(x, y, s, s, QP(u < 250 ? 7 : u < 500 ? 10 : u < 700 ? 9 : 8)); }
    }
    q_panel(8, 156, 224, 34);
    { char b[24]; int32_t v;
      v = 2 + k0 * 400 / 1000; str_cpy(b, "A ", 4), fmt_int(b + 2, v), str_cpy(b + str_len(b), "ms  D ", 8), fmt_int(b + str_len(b), 20 + c->k[1] * 800 / 1000), str_cpy(b + str_len(b), "ms", 4); q_text(18, 158, b, Q_TEXT);
      str_cpy(b, "S ", 4), fmt_int(b + 2, S / 10), str_cpy(b + str_len(b), "%  R ", 8), fmt_int(b + str_len(b), 30 + c->k[3] * 1200 / 1000), str_cpy(b + str_len(b), "ms", 4); q_text(18, 172, b, Q_TEXT);
      q_text(176, 158, "heat", Q_DIM); p_R(176, 178, 46, 4, Q_OFF); p_R(176, 178, 46 * q_min(1000, pf.heat) / 1000, 4, QP(9)); }
}
static UI_SMALL uint32_t ps_forge_dirty(const ps_c *c, ps_r *o)
{
    int32_t hx, hy, px, py, th, i, x0 = 999, y0 = 999, x1 = -1, y1 = -1;
    pf_hammer(c->t, &hx, &hy, &px, &py, &th);
    o[0] = (ps_r){168, 38, 239, 112, 0};                                                /* the furnace */
    o[1] = (ps_r){(int16_t)(q_min(hx, px) - 16), (int16_t)(q_min(hy, py) - 16), (int16_t)(q_max(hx, px) + 16), (int16_t)(q_max(hy, py) + 16), 0};
    for (i = 0; i < pf.n; i++) {
        int32_t u = (int32_t)(c->t - pf.sp[i].t0);
        if (u < 0 || u > 900) continue;
        { int32_t x = pf.sp[i].x + pf.sp[i].vx * u / 1000, y = pf.sp[i].y + pf.sp[i].vy * u / 1000 + 160 * u / 1000 * u / 1000; x0 = q_min(x0, x), x1 = q_max(x1, x), y0 = q_min(y0, y), y1 = q_max(y1, y); }
    }
    o[2] = x1 < 0 ? (ps_r){0, 0, -1, -1, 0} : (ps_r){(int16_t)(x0 - 4), (int16_t)(y0 - 4), (int16_t)(x1 + 6), (int16_t)(y1 + 6), 0};
    o[3] = (ps_r){50, 76, 172, 116, 0};                                                 /* the blade (heat) */
    o[4] = (ps_r){168, 172, 232, 186, 0};                                               /* the heat bar */
    return 5;
}
/* ---- stars of three kinds: a twinkling dot, a small plus, a bright four-pointed one (RPG skies) ---- */
static UI_SMALL void q_stars(int32_t n, int32_t y0, int32_t ymax, uint32_t seed)
{
    int32_t i;
    for (i = 0; i < n; i++) {
        uint32_t h = qhash(seed + (uint32_t)i * 7u);
        int32_t x = (int32_t)(h % 240u), y = y0 + (int32_t)((h >> 8) % (uint32_t)ymax);
        uint32_t kind = (h >> 17) % 10u, ph = (fm1_ms / 350u + (uint32_t)i) % 6u;
        uint16_t c = ph < 4u ? RGB(0xf2, 0xf2, 0xff) : RGB(0x9a, 0x9a, 0xc8);
        if (kind < 6u) cv_pset(x, y, c);                                                         /* a dot that twinkles */
        else if (kind < 9u) { cv_pset(x, y, c), cv_pset(x - 1, y, RGB(0x8a, 0x8a, 0xb8)), cv_pset(x + 1, y, RGB(0x8a, 0x8a, 0xb8)), cv_pset(x, y - 1, RGB(0x8a, 0x8a, 0xb8)), cv_pset(x, y + 1, RGB(0x8a, 0x8a, 0xb8)); }   /* a plus */
        else { p_R(x - 2, y, 5, 1, RGB(0xff, 0xf0, 0xb0)), p_R(x, y - 2, 1, 5, RGB(0xff, 0xf0, 0xb0)); if (ph < 3u) cv_pset(x + 1, y - 1, RGB(0xff, 0xff, 0xff)); }   /* a bright one */
    }
}
static UI_SMALL int p_vis(int32_t x0, int32_t y0, int32_t x1, int32_t y1)                    /* does a rectangle touch the window being painted? */
{
    int32_t wx0 = -cv_ox, wy0 = -cv_oy;
    return x1 >= wx0 && x0 < wx0 + (int32_t)cv_w && y1 >= wy0 && y0 < wy0 + (int32_t)cv_h;
}
static UI_SMALL int32_t p_wv(int32_t s, int32_t p)                                           /* a waveform at p (Q1000 cycles): -1000..1000 */
{
    p %= 1000;
    if (p < 0) p += 1000;
    switch (s) {
    case 1: return p < 500 ? p * 4 - 1000 : 3000 - p * 4;
    case 2: return p * 2 - 1000;
    case 3: return p < 500 ? 1000 : -1000;
    case 4: return ((int32_t)p_hs((uint32_t)(p * 6 / 1000) + 1u) - 512) * 2;
    default: return qsin(p * 64 / 1000) * 1000 / 127;
    }
}
/* ======================================================= LFO: the sea ======================================================= */
static UI_SMALL void ps_sea(const ps_c *c)
{
    static const uint8_t SKY[5] = {0, 1, 1, 2, 13};
    int32_t rate = 200 + c->k[0] * 6, shape = c->a, ph0 = c->k[2], fade = c->k[3], x, i, sx = 120, y0, y1, p0, tilt;
    int32_t amp = fade < 20 ? 1000 : q_min(1000, (int32_t)(c->t % 7000u) * 1000 / (fade * 5 + 200));
    int32_t tp = (int32_t)((c->t % 65536u) * (uint32_t)rate * 3u / 10000u);
    p_sky(PS_Y0, 100, SKY, 5), p_ell(60, 50, 12, 12, QP(7)), p_ell(66, 46, 11, 11, QP(1));
    q_stars(26, 22, 60, 31u);
    for (x = 0; x < 240; x++) {
        int32_t y = 118 + p_wv(shape, x * 2000 / 240 - tp + ph0) * 156 / 10 * amp / 1000000;
        p_R(x, y, 1, 197 - y, QP(12)), p_R(x, y, 1, 2, QP(7));
        if (((x + (int32_t)(c->t / 50u)) & 7) == 0) p_R(x, y + 8, 3, 1, p_shade(QP(12), 800));
    }
    p0 = sx * 2000 / 240 - tp + ph0;
    y0 = 118 + p_wv(shape, p0) * 156 / 10 * amp / 1000000, y1 = 118 + p_wv(shape, p0 + 20) * 156 / 10 * amp / 1000000, tilt = (y1 - y0) / 4;
    p_quad(sx - 18, y0 - 6 + tilt, sx + 18, y0 - 6 - tilt, sx + 12, y0 + 2 - tilt, sx - 12, y0 + 2 + tilt, QP(4)), p_R(sx - 1, y0 - 30, 2, 26, QP(4));
    p_tri(sx + 2, y0 - 29, sx + 16, y0 - 12, sx + 2, y0 - 12, QP(7)), p_tri(sx - 2, y0 - 26, sx - 12, y0 - 12, sx - 2, y0 - 12, QP(15));
}
static UI_SMALL uint32_t ps_sea_dirty(const ps_c *c, ps_r *o) { (void)c; o[0] = (ps_r){0, 82, 239, 150, 0}; return 1; }
/* ======================================================= LFO DEST: the alchemist's brew ======================================================= */
static UI_SMALL void ps_alch(const ps_c *c)
{
    static const uint8_t SKY[5] = {0, 0, 1, 1, 0};
    static const char *const DST[4] = {"pitch", "filter", "shape", "amp"}, *const SH[4] = {"sine", "tri", "saw", "square"};
    int32_t rate = c->a, depth = c->b, shape = c->c & 3, dst = c->d, i, j, x, y, tq = (int32_t)(c->t % 60000u);
    p_sky(PS_Y0, 197, SKY, 5);
    p_R(0, 176, 240, 21, QP(4));
    for (i = 0; i < 6; i++) p_R(i * 40 + 6, 176, 1, 21, QP(0));
    p_R(0, 176, 240, 1, QP(15));
    for (i = 0; i < 5; i++) {
        int32_t fx = 86 + i * 17 + qsin(i * 3 * 64 / 40) / 32, f = 6 + p_sin((uint32_t)tq + (uint32_t)i * 220u, 90) * 3 / 127;
        p_R(fx - 2, 168, 6, 8, QP(4)), p_tri(fx + 1, 150 - f, fx - 3, 168, fx + 5, 168, QP(8)), p_tri(fx + 1, 156 - f / 2, fx - 1, 168, fx + 3, 168, QP(9)), p_tri(fx + 1, 162, fx, 168, fx + 2, 168, QP(10));
    }
    p_R(70, 164, 100, 6, QP(4)), p_ell(120, 130, 70, 44, QP(0)), p_ell(120, 128, 66, 42, QP(5)), p_ell(120, 124, 58, 36, QP(0)), p_R(62, 150, 6, 22, QP(0)), p_R(172, 150, 6, 22, QP(0));
    p_ell(120, 100, 68, 22, QP(0)), p_ell(120, 99, 66, 20, QP(6)), p_ell(120, 100, 58, 16, QP(0)), p_ell(120, 102, 55, 14, QP(3));
    for (j = 0; j < 4; j++) {
        int32_t base = (j * 1000 + 600) * 10 / 42 / 10, w = p_wv(shape, (int32_t)((uint32_t)tq * (uint32_t)rate / 2000u) - j * 220) * depth / 1000 * 22 / 100, rr = q_max(80, q_min(980, base + w));
        p_ellO(120, 102, 54 * rr / 1000, 135 * rr / 10000, QP(j % 2 ? 11 : 7));
    }
    p_ell(120, 102, 7, 2, QP(11));
    for (i = 0; i < 9; i++) {
        int32_t u = (int32_t)((uint32_t)tq * (250u + p_hs((uint32_t)i) * 250u / 1024u) / 1000u + p_hs((uint32_t)i + 5u)) % 1000;
        int32_t bx = 120 + ((int32_t)p_hs((uint32_t)i + 9u) - 512) * 96 / 1024 * (600 + u * 4 / 10) / 1000 + p_sin((uint32_t)tq, 20) * 4 / 127, by = 94 - u * 62 / 1000, s = 2 + u * 4 / 1000;
        if (u < 60) continue;
        p_ellO(bx, by, s, s, QP(7)), cv_pset(bx - s / 2, by - s / 2, QP(7));
    }
    for (i = 0; i < 12; i++) {
        int32_t u = (int32_t)((uint32_t)tq * 400u / 1000u + (uint32_t)i * 1000u / 12u) % 1000;
        x = 120 + qsin((u * 9 / 1000 + i) * 64 / 6) * (12 + u * 30 / 1000) / 127, y = 90 - u * 56 / 1000;
        if (u > 850 && (i & 1)) continue;
        p_R(x, y, 2 + (u > 500), 1 + (u > 500), QP(u > 550 ? 5 : 6));
    }
    q_panel(152, 20, 84, 64), q_text(158, 22, SH[shape], QP(10));
    { char b[16]; str_cpy(b, "-> ", 4), str_cpy(b + 3, DST[dst & 3], 10); q_text(158, 38, b, Q_DIM); }
    for (x = 0; x < 58; x++) { int32_t v = p_wv(shape, x * 2000 / 58 * (500 + rate / 8) / 1000) * depth / 1000; cv_pset(168 + x, 70 - v * 10 / 1000, QP(11)); }
    p_R(168, 70, 58, 1, Q_LINE);
    { int32_t px = (int32_t)((uint32_t)tq * (uint32_t)rate / 2000u % 58u), vv = p_wv(shape, px * 2000 / 58 * (500 + rate / 8) / 1000) * depth / 1000; p_R(168 + px, 70 - vv * 10 / 1000 - 1, 3, 3, QP(10)); }
}
static UI_SMALL uint32_t ps_alch_dirty(const ps_c *c, ps_r *o)
{
    (void)c;
    o[0] = (ps_r){30, 24, 215, 112, 0}; o[1] = (ps_r){80, 140, 175, 172, 0};
    return 2;
}
/* ======================================================= SLICER: houses become towers ======================================================= */
static UI_SMALL int32_t pt_val(const ps_c *c, int32_t i)
{
    int32_t mode = c->a, pat = c->b, v;
    if (mode == 0) return 100;
    v = (int32_t)p_hs((uint32_t)(i * 33 + pat * 71)) < 563 ? 350 + (int32_t)p_hs((uint32_t)(i * 17 + pat * 10)) * 650 / 1024 : 120;
    v = v * (300 + c->k[3] * 11 / 10) / 1000;
    return q_max(80, q_min(1000, v));
}
static int16_t pt_disp[16];                                                        /* the height each tower shows now (it walks to pt_val) */
static uint8_t pt_ok;
static UI_SMALL int pt_step(const ps_c *c)                                     /* the towers that are too tall shrink first, then the new ones rise; a new pattern in the middle just retargets */
{
    int32_t i, down = 0, moving = 0;
    for (i = 0; i < 16; i++) {
        int32_t tg = pt_val(c, i);
        if (!pt_ok) pt_disp[i] = (int16_t)tg;
        down |= pt_disp[i] > tg + 3;
    }
    pt_ok = 1;
    for (i = 0; i < 16; i++) {
        int32_t tg = pt_val(c, i), d = pt_disp[i];
        if (d > tg + 3) d = q_max(tg, d - 45), moving = 1;
        else if (d < tg - 3 && !down) d = q_min(tg, d + 32), moving = 1;
        pt_disp[i] = (int16_t)d;
    }
    return moving;
}
static UI_SMALL int32_t pt_play(const ps_c *c)
{
    static const uint16_t RT[6] = {200, 400, 800, 267, 533, 1067};
    return (int32_t)(c->t * RT[c->d % 6] / 100000u % 16u);
}
static UI_SMALL void ps_towers(const ps_c *c)
{
    static const uint8_t SKY[5] = {0, 0, 1, 1, 2};
    int32_t mode = c->a, i, q, y, play = pt_play(c);
    p_sky(PS_Y0, 130, SKY, 5);
    q_stars(28, 22, 70, 57u);
    p_ell(206, 44, 10, 10, QP(7)), p_ell(211, 41, 9, 9, QP(0));
    p_R(0, 150, 240, 47, QP(3));
    for (y = 152; y < 197; y += 6) p_R(0, y, 240, 1, QP(0));
    for (i = 0; i < 16; i++) {
        int32_t v = pt_disp[i], x = 6 + i * 145 / 10, base = 168 - (i & 1) * 6, hgt = 8 + v * 78 / 1000, lit = i == play, w = 11 + (v > 600 ? 3 : 0), tier = v > 850 ? 3 : v > 550 ? 2 : v > 300 ? 1 : 0;
        if (!p_vis(x - 2, base - hgt - 60, x + w + 10, base + 6)) continue;
        p_ell(x + w / 2, base + 2, w, 3, QP(0)), p_R(x, base - hgt, w, hgt, QP(tier == 3 ? 6 : 5)), p_R(x, base - hgt, w, 2, QP(7)), p_R(x + w - 2, base - hgt, 2, hgt, QP(0));
        if (tier == 0) p_tri(x - 1, base - hgt, x + w / 2, base - hgt - 6, x + w + 1, base - hgt, QP(8));
        else if (tier == 1) p_tri(x - 2, base - hgt, x + w / 2, base - hgt - 9, x + w + 2, base - hgt, QP(8));
        else if (tier == 2) { for (q = 0; q < 3; q++) p_R(x + q * 4, base - hgt - 4, 3, 4, QP(5)); p_R(x + w / 2, base - hgt - 14, 1, 10, QP(7)), p_tri(x + w / 2 + 1, base - hgt - 14, x + w / 2 + 7, base - hgt - 11, x + w / 2 + 1, base - hgt - 8, QP(8)); }
        else { for (q = 0; q < 4; q++) p_R(x + q * 4 - 1, base - hgt - 5, 3, 5, QP(6)); p_tri(x + 2, base - hgt - 5, x + w / 2, base - hgt - 20, x + w - 2, base - hgt - 5, QP(12)), p_R(x + w / 2, base - hgt - 28, 1, 8, QP(7)), p_tri(x + w / 2 + 1, base - hgt - 28, x + w / 2 + 8, base - hgt - 25, x + w / 2 + 1, base - hgt - 22, QP(8)); }
        for (q = 0; q < hgt / 12; q++) {
            int32_t on = mode > 0 && (p_hs((uint32_t)(i * 7 + q)) < 614 || lit) && (mode != 2 || ((c->t / 90u + (uint32_t)q) & 1u));
            p_R(x + 3 + (q & 1) * 3, base - hgt + 8 + q * 11, 3, 4, QP(on ? 10 : 1));
        }
        if (lit && mode > 0) {
            int32_t f = 3 + p_sin(c->t, 200) * 3 / 254, tx = x + w / 2;
            p_R(tx - 1, base - hgt - 36, 2, 5, QP(4)), p_tri(tx, base - hgt - 42 - f, tx - 3, base - hgt - 36, tx + 3, base - hgt - 36, QP(9)), p_tri(tx, base - hgt - 40, tx - 1, base - hgt - 36, tx + 2, base - hgt - 36, QP(10));
        }
    }
}
static UI_SMALL uint32_t ps_towers_dirty(const ps_c *c, ps_r *o)
{
    int32_t i, play = pt_play(c), v = pt_disp[play], x = 6 + play * 145 / 10, hgt = 8 + v * 78 / 1000;
    o[0] = (ps_r){(int16_t)(x - 6), (int16_t)(168 - hgt - 64), (int16_t)(x + 24), 175, 0};
    if (c->a == 2 || pt_ok > 1u) o[1] = (ps_r){0, 60, 239, 175, c->a == 2 ? 3 : 0}; else o[1] = (ps_r){0, 0, -1, -1, 0};
    (void)i;
    return 2;
}
/* ======================================================= DLY: echoes in the forest ======================================================= */
static UI_SMALL void ps_forest(const ps_c *c)
{
    static const uint8_t SKY[5] = {0, 0, 1, 1, 2};
    int32_t n = 1 + (c->k[1] * 4 + 500) / 1000, gap = 30 + c->k[0] * 18 / 1000, ph = (int32_t)(c->t * 45u / 100u % 1000u), i, y, r;
    p_sky(PS_Y0, 120, SKY, 5);
    q_stars(30, 22, 80, 91u);
    p_ell(204, 44, 10, 10, QP(7)), p_ell(209, 41, 9, 9, QP(0));
    for (i = 0; i < 14; i++) { int32_t x = i * 19 + 4, h = 40 + (int32_t)p_hs((uint32_t)i) * 30 / 1024; p_tri(x, 120, x + 9, 120 - h, x + 18, 120, QP(i & 1 ? 3 : 1)); }
    p_R(0, 120, 240, 77, QP(3));
    for (y = 122; y < 197; y += 5) p_R(0, y, 240, 1, QP(0));
    p_quad(0, 197, 100, 120, 140, 120, 240, 197, QP(4));
    for (i = n; i >= 0; i--) {
        int32_t sc = i == 0 ? 3 : i == 1 ? 2 : 1, x = 26 + i * gap * (100 - i * 8) / 100, fy = 168 - i * 9, w = 17 * sc, h2 = 24 * sc;
        int32_t vis = i == 0 ? 1000 : q_max(150, q_min(1000, 350 + c->k[3] * 650 / 1000 - i * 100)), on = i == 0 ? ph < 500 : ((ph - i * 130) % 1000 + 1000) % 1000 < 500;
        int32_t tx = x + w / 2, ty = fy - h2 - 12 - (i ? 0 : 4);
        if (!p_vis(x - 8, ty - 6, x + w + 40, fy + 8)) continue;
        p_ell(x + w / 2, fy + 2, w * 55 / 100, 2 + sc, QP(0));
        p_hero(Q_HERO[1] + (uint32_t)((c->t / 450u) & 1u), tx, fy, sc, i ? (c->k[2] > 500 && i > 2 ? 13 : 6) : -1);
        if (on) {
            const char *lab = i == 0 ? "HEY!" : "hey";
            int32_t ww = text_w(&FONT_P, lab) + 8;
            uint16_t bg = i == 0 ? QP(7) : p_shade(RGB(0xf2, 0xe2, 0xc4), (uint32_t)vis);
            p_R(tx - ww / 2, ty - 3, ww, i == 0 ? 18 : 16, bg), p_tri(tx - 2, ty + 15, tx + 2, ty + 15, tx, ty + 19, bg), q_text(tx - ww / 2 + 4, ty - 2, lab, QP(0));
        }
    }
    if (pn_t0 && c->t - pn_t0 < 1700u)                                                 /* a key was pressed: its ring, then the echoes, one every delay time */
        for (r = 0; r < n + 1; r++) { int32_t a = (int32_t)(c->t - pn_t0) - r * (130 + gap * 6), u; if (a < 0 || a > 900) continue; u = a * 1000 / 900; p_ellO(40 + u * 170 / 1000, 176, 8 + u * 30 / 1000, 3 + u * 8 / 1000, QP(r ? 14 : 7)); }
}
static UI_SMALL uint32_t ps_forest_dirty(const ps_c *c, ps_r *o) { (void)c; o[0] = (ps_r){14, 56, 232, 193, 0}; return 1; }
/* ======================================================= REV/CHO: the cathedral ======================================================= */
static UI_SMALL void ps_cathedral(const ps_c *c)
{
    static const uint8_t GL[6] = {12, 8, 10, 11, 13, 14};
    int32_t nw = 2 + (c->k[0] * 2 + 500) / 1000, damp = c->k[1], nch = 1 + (c->k[3] * 3 + 500) / 1000, dim = 1000 - damp * 450 / 1000, FL = 168, sp = 240 / nw, ww = q_min(46, sp - 22), wt = 26, wh = FL - wt - 12;
    int32_t i, r, cc, x, y, q, p, f;
    p_R(0, 19, 240, 178, RGB(0x1c, 0x1a, 0x24));
    for (r = 0; r < 14; r++) for (cc = 0; cc < 10; cc++) p_R(cc * 26 + (r & 1) * 13 - 10, 20 + r * 12, 25, 11, (r * 5 + cc * 3) % 5 ? RGB(0x24, 0x20, 0x30) : RGB(0x2c, 0x26, 0x38));
    for (i = 0; i <= nw; i++) { x = i * sp; p_R(x - 8, 19, 16, FL - 19, RGB(0x3a, 0x30, 0x38)), p_R(x - 8, 19, 3, FL - 19, RGB(0x4a, 0x40, 0x48)), p_R(x + 5, 19, 3, FL - 19, RGB(0x24, 0x1c, 0x24)), p_R(x - 10, FL - 14, 20, 14, RGB(0x2c, 0x24, 0x2c)), p_R(x - 10, 19, 20, 8, RGB(0x2c, 0x24, 0x2c)); }
    for (i = 0; i < nw; i++) {
        int32_t cx = i * sp + sp / 2, x0 = cx - ww / 2;
        const uint16_t BLK = RGB(0x0c, 0x0a, 0x10);
        p_poly((const int32_t[5][2]){{x0 - 3, wt + wh}, {x0 - 3, wt + 22}, {cx, wt - 6}, {x0 + ww + 3, wt + 22}, {x0 + ww + 3, wt + wh}}, 5, BLK);
        if (p_vis(x0 - 4, wt - 8, x0 + ww + 4, wt + wh + 4))
            for (y = wt; y < wt + wh; y += 5) {
                int32_t arch = y < wt + 24 ? q_isqrt(1000000 - (wt + 24 - y) * 1000 / 24 * ((wt + 24 - y) * 1000 / 24)) : 1000, hw = ww / 2 * arch / 1000;
                for (x = cx - hw; x < cx + hw - 1; x += 5) {
                    int32_t qq = (int32_t)p_hs((uint32_t)((x / 5) * 31 + (y / 5) * 77 + i * 110)), band = y < wt + 26 ? 4 : y > wt + wh - 24 ? 2 : (x - x0) / (ww / 3);
                    p_R(x, y, 4, 4, p_shade(QP(GL[(qq * 3 / 1024 + band) % 6]), (uint32_t)((550 + qq * 350 / 1024) * dim / 1000)));
                }
            }
        p_R(cx - 1, wt + 24, 1, wh - 24, BLK), p_R(x0 + ww * 33 / 100, wt + 24, 1, wh - 24, BLK), p_R(x0 + ww * 66 / 100, wt + 24, 1, wh - 24, BLK), p_R(x0, wt + 62, ww, 1, BLK), p_R(x0, wt + 100, ww, 1, BLK);
        p_ell(cx, wt + 12, 6, 6, BLK), p_ell(cx, wt + 12, 4, 4, p_shade(QP(GL[(i + 2) % 6]), (uint32_t)(800 * dim / 1000))), p_R(x0 - 3, wt + wh, ww + 6, 4, RGB(0x4a, 0x40, 0x48));
        if (nw < 4 && i % 2 == 0) { p_R(cx - ww / 2 - 18, wt + 8, 12, 46, QP(0)), p_R(cx - ww / 2 - 17, wt + 9, 10, 44, QP(8)), p_R(cx - ww / 2 - 14, wt + 24, 4, 12, QP(10)), p_tri(cx - ww / 2 - 17, wt + 54, cx - ww / 2 - 12, wt + 48, cx - ww / 2 - 7, wt + 54, QP(8)); }
    }
    p_R(0, FL, 240, 29, RGB(0x3a, 0x34, 0x40)), p_R(0, FL, 240, 3, RGB(0x6a, 0x64, 0x70));
    for (r = 0; r < 2; r++) for (cc = 0; cc < 12; cc++) p_R(cc * 22 + r * 11 - 4, FL + 6 + r * 11, 21, 10, (r + cc) % 3 ? RGB(0x2c, 0x28, 0x34) : RGB(0x34, 0x2e, 0x3c));
    p_R(40, FL + 4, 170, 5, QP(8)), p_R(40, FL + 4, 170, 1, QP(10)), p_R(40, FL + 8, 170, 1, QP(10));
    for (p = 0; p < 3; p++) { int32_t px = 8 + p * 24; p_R(px, FL - 16, 22, 5, RGB(0x5a, 0x1a, 0x1c)), p_R(px, FL - 11, 22, 11, RGB(0x3a, 0x10, 0x14)), p_R(px - 2, FL - 18, 3, 18, RGB(0x4a, 0x24, 0x18)), p_R(px + 2, FL - 20, 18, 3, RGB(0x6a, 0x3a, 0x20)); }
    for (i = 0; i < nw; i++) {
        int32_t cx = i * sp + sp / 2, col = GL[(i * 2) % 6];
        if (!p_vis(cx - 6, wt + 70, cx + 70, FL)) continue;
        for (y = wt + 70; y < FL; y += 2) { int32_t u = (y - wt - 70) * 1000 / (FL - wt - 70), xl = cx - 6 + u * 40 / 1000, xr = cx + 14 + u * 52 / 1000; for (x = xl; x < xr; x += 2) if (((x + y) & 3) == 0) cv_pset(x, y, p_shade(QP(col), (uint32_t)(900 * dim / 1000))); }
    }
    f = 10 + damp * 40 / 1000;
    for (q = 0; q < f; q++) { int32_t a = (int32_t)((c->t % 100000u) * (40u + p_hs((uint32_t)q) * 50u / 1024u) / 1000000u + p_hs((uint32_t)q + 7u) * 1000u / 1024u) % 1000; cv_pset(((int32_t)p_hs((uint32_t)q * 3u) * 240 / 1024 + p_sin(c->t + (uint32_t)q * 1000u, 10) * 6 / 127 + 240) % 240, 40 + a * 124 / 1000, QP(7)); }
    for (cc = nch; cc >= 0; cc--) {
        int32_t hx = 120 - 17 + (cc ? (cc % 2 ? 1 : -1) * (24 + (cc + 1) / 2 * 18 * (400 + c->k[3]) / 1000) : 0) + (cc ? p_sin(c->t, (uint32_t)(15 + c->k[2] * 6 / 100) * 1u) * (1 + c->k[3] * 2 / 1000) / 127 : 0);
        if (!p_vis(hx - 12, FL - 52, hx + 50, FL + 6)) continue;
        p_hero(Q_HERO[2] + (uint32_t)((c->t / 500u) & 1u), hx + 17, FL + 4, 2, cc ? 13 : -1);
    }
}
static UI_SMALL uint32_t ps_cathedral_dirty(const ps_c *c, ps_r *o) { (void)c; o[0] = (ps_r){40, 112, 200, 176, 0}; o[1] = (ps_r){0, 38, 239, 166, 5}; return 2; }
/* ======================================================= SCL: the torch wall ======================================================= */
static const uint16_t PT_SCL[16] = {0xFFF, 0xAB5, 0x5AD, 0x6B5, 0x6AD, 0x295, 0x215, 0x9AD, 0x5AB, 0xAD5, 0x56B, 0x5AD, 0x4A9, 0xAAA, 0xB6D, 0x6DB};
static UI_SMALL int pt_lit(const ps_c *c, int32_t n, int32_t *black, int32_t *root)
{
    int32_t note = n, rel = (note - c->a + 12) % 12, in = (PT_SCL[c->b & 15] >> rel) & 1, bl = n == 1 || n == 3 || n == 6 || n == 8 || n == 10, qn = c->c;
    *black = bl, *root = note == c->a;
    return qn == 0 ? 1 : qn == 2 ? in && !bl : in;
}
static UI_SMALL void ps_torch(const ps_c *c)
{
    static const char *const NN[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int32_t r, cc, n, i, px, py, ch = c->d;
    p_R(0, 19, 240, 150, RGB(0x1d, 0x15, 0x20));
    for (r = 0; r < 11; r++) for (cc = 0; cc < 9; cc++) { int32_t x = cc * 28 + (r & 1) * 14 - 10, y = 21 + r * 14; p_R(x, y, 26, 12, (r * 3 + cc) % 4 ? QP(5) : RGB(0x3a, 0x30, 0x40)), p_R(x, y, 26, 1, QP(6)); }
    p_R(0, 169, 240, 28, QP(4));
    for (cc = 0; cc < 240; cc += 24) p_R(cc, 169, 1, 28, QP(0));
    for (i = 0; i < 4; i++) { int32_t x = 8 + i * 76; p_R(x, 19, 8, 150, RGB(0x2a, 0x20, 0x30)), p_R(x + 8, 19, 2, 150, QP(0)); }
    for (n = 0; n < 12; n++) {                                                          /* the glow of every lit torch */
        int32_t bl, rt, lit = pt_lit(c, n, &bl, &rt), x = 20 + n * 18, y = bl ? 96 : 128;
        if (!lit || !p_vis(x - 22, y - 24, x + 22, y + 12)) continue;
        for (py = -18; py <= 18; py++) for (px = -22; px <= 22; px++) {
            int32_t d2 = px * px * 1000 / 484 + py * py * 1000 / 324;
            if (d2 < 1000 && ((px + py) & 1) == 0 && (d2 < 300 || ((px >> 1) + (py >> 1)) % 2 == 0)) cv_pset(x + px, y - 6 + py, QP(d2 < 300 ? 10 : 9));
        }
    }
    for (n = 0; n < 12; n++) {
        int32_t bl, rt, lit = pt_lit(c, n, &bl, &rt), x = 20 + n * 18, y = bl ? 96 : 128, fl = 4 + (p_sin(c->t, 120 + (uint32_t)n * 0) * 15 / 127 + p_sin(c->t + (uint32_t)n * 400u, 70) * 10 / 127) / 10 + (n & 1);
        if (!p_vis(x - 8, y - 50, x + 24, y + 26) && !p_vis(x - 12, (bl ? 72 : 152), x + 12, (bl ? 72 : 152) + 12)) continue;
        p_R(x - 2, y + 12, 5, 12, QP(0)), p_R(x - 1, y + 12, 3, 12, QP(4)), p_R(x - 1, y + 12, 1, 12, QP(15)), p_R(x - 6, y + 6, 13, 3, QP(0)), p_R(x - 5, y + 7, 11, 1, QP(6));
        p_quad(x - 6, y + 6, x - 4, y - 1, x + 5, y - 1, x + 7, y + 6, QP(0)), p_quad(x - 5, y + 5, x - 3, y, x + 4, y, x + 6, y + 5, QP(6)), p_R(x - 4, y, 9, 1, QP(7));
        if (lit) { p_tri(x, y - 12 - fl, x - 5, y - 1, x + 5, y - 1, QP(8)), p_tri(x, y - 9 - fl * 4 / 5, x - 3, y - 1, x + 4, y - 1, QP(9)), p_tri(x, y - 6 - fl / 2, x - 2, y - 1, x + 2, y - 1, QP(10)), cv_pset(x, y - 3, QP(7)); }
        else p_R(x - 2, y - 2, 5, 2, QP(0)), cv_pset(x, y - 3, QP(5));
        if (rt) { p_R(x + 10, y - 26, 1, 22, QP(7)), p_tri(x + 11, y - 26, x + 21, y - 22, x + 11, y - 18, QP(8)); }
        q_text_c(x, bl ? 72 : 152, NN[n], lit ? (rt ? Q_HI : Q_TEXT) : Q_DIM);
        if (ch && rt) for (i = 0; i < ch + 2; i++) { int32_t a = (int32_t)(c->t * 500u / 1000u % 1000u) + i * 166; a %= 1000; p_R(x + qsin(a * 7 / 1000 * 64 / 6 + i * 5) * 6 / 127, y - 16 - a * 30 / 1000, 3, 3, QP(6)); }
    }
}
static UI_SMALL uint32_t ps_torch_dirty(const ps_c *c, ps_r *o)
{
    int32_t n, m = 0;
    for (n = 0; n < 12; n++) {
        int32_t bl, rt, lit = pt_lit(c, n, &bl, &rt), x = 20 + n * 18, y = bl ? 96 : 128;
        if (lit) o[m++] = (ps_r){(int16_t)(x - 8), (int16_t)(y - 22), (int16_t)(x + 8), (int16_t)(y + 2), 0};
        if (rt && c->d) o[m++] = (ps_r){(int16_t)(x - 2), (int16_t)(y - 56), (int16_t)(x + 14), (int16_t)(y - 8), 0};
    }
    return (uint32_t)m;
}
/* ======================================================= FX: four stompboxes, each with its own little animation ======================================================= */
static UI_SMALL void ps_pedals(const ps_c *c)
{
    static const uint8_t SKY[3] = {0, 1, 0}, COL[4] = {8, 12, 11, 13};
    static const char *const NM[4] = {"dist", "chorus", "delay", "reverb"};
    int32_t i, j, tq = (int32_t)(c->t % 60000u);
    p_sky(PS_Y0, 197, SKY, 3);
    p_R(0, 150, 240, 47, QP(4)), p_R(0, 150, 240, 2, QP(0));                                        /* the stage boards */
    for (j = 0; j < 8; j++) p_R(j * 30 + 14, 152, 1, 45, QP(0));
    for (i = 0; i < 4; i++) {
        int32_t x = 9 + i * 58, k = c->k[i], a = 24 + k * 48 / 1000, cx = x + 25, cy = 128, ax = x + 25, am = 62, lit = k > 20, w;
        p_R(x, 100, 50, 72, QP(0)), p_R(x + 1, 101, 48, 70, QP(COL[i])), p_R(x + 1, 101, 48, 3, QP(7)), p_R(x + 1, 168, 48, 3, QP(0));     /* the box */
        p_R(x + 8, 106, 34, 14, QP(0)), p_R(x + 10, 108, 30, 10, QP(5));
        p_R(x + 20, 109, 10, 6, lit && ((tq / (i == 2 ? 250 : 400)) & 1 || i != 2) ? QP(10) : QP(4));                                  /* the LED (the delay's blinks) */
        p_ell(cx, cy, 13, 13, QP(0)), p_ell(cx, cy, 11, 11, QP(5));
        cv_line(cx, cy, cx + qcos(a) * 10 / 127, cy + qsin(a) * 10 / 127, QP(7));
        p_ell(cx, 154, 8, 5, QP(0)), p_ell(cx, 153, 6, 3, QP(6));                                                               /* the switch */
        q_text(x + 25 - text_w(&FONT_P, NM[i]) / 2, 143, NM[i], QP(0));
        switch (i) {
        case 0:                                                                                                                 /* a sine, clipped harder and harder */
            for (j = 0; j < 48; j++) {
                int32_t v = p_sin((uint32_t)(j * 130 + tq / 2), 60) * 20 / 127, lim = 20 - k * 17 / 1000;
                if (v > lim) v = lim;
                if (v < -lim) v = -lim;
                p_R(x + 1 + j, am - 14 + v, 1, 2, QP(8));
            }
            break;
        case 1:                                                                                                                 /* two voices drifting apart */
            for (j = 0; j < 48; j++) {
                p_R(x + 1 + j, am - 14 + p_sin((uint32_t)(j * 150 + tq / 2), 60) * 10 / 127, 1, 2, QP(12));
                p_R(x + 1 + j, am - 14 + p_sin((uint32_t)(j * 150 + tq / 2 + k * 3), 60) * 10 / 127, 1, 2, QP(7));
            }
            break;
        case 2:                                                                                                                 /* echoes, fainter and fainter, the more feedback the more of them */
            for (j = 0; j < 5; j++) {
                int32_t u = (j * 10 + tq / 40) % 50, sz = 6 - j;
                if (j * 200 > 300 + k * 7 / 10) break;
                p_R(x + 2 + u, am - 14 - sz / 2, sz, sz, QP(j < 2 ? 10 : 9));
            }
            break;
        default:                                                                                                                 /* rings that spread: the bigger the room the more of them */
            w = 2 + k * 3 / 1000;
            for (j = 0; j < w; j++) {
                int32_t r = 4 + ((tq / 30 + j * 50 / w) % 50) * 22 / 50;
                p_ellO(ax, am - 14, r, r * 6 / 10, QP(r > 18 ? 6 : 13));
            }
            break;
        }
    }
}
static UI_SMALL uint32_t ps_pedals_dirty(const ps_c *c, ps_r *o)
{
    int32_t i;
    for (i = 0; i < 4; i++) o[i] = (ps_r){(int16_t)(9 + i * 58), 22, (int16_t)(9 + i * 58 + 50), 98, 0};
    (void)c;
    return 4;
}
/* ======================================================= the driver: pages -> scenes, first paint, then only what moves ======================================================= */
static UI_SMALL int q_page_scene(void)                                                           /* the scene of the page on screen, or -1 */
{
    const page_t *p = cur_page();
    if (ui.home) return -1;
    switch (p->id[0]) {
    case P_ATK: return is_drum(TSEL) ? -1 : 0;
    case P_LRATE: return is_drum(TSEL) ? -1 : 1;
    case P_LD_PIT: return is_drum(TSEL) ? -1 : 2;
    case P_SLCR: return 3;
    case G_DTIME: return 4;
    case G_RSIZE: return 5;
    case P_ROOT: return 6;
    case P_DIST: return is_drum(TSEL) ? -1 : 7;
    default: return -1;
    }
}
typedef void (*ps_draw)(const ps_c *);
typedef uint32_t (*ps_dirty)(const ps_c *, ps_r *);
static UI_SMALL void ps_paint(ps_draw fn, const ps_c *c, int32_t x0, int32_t y0, int32_t x1, int32_t y1)   /* paint a rectangle of the screen (scene rows only) */
{
    int32_t rows, y, w;
    if (x0 < 0) x0 = 0;
    if (y0 < PS_Y0) y0 = PS_Y0;
    if (x1 > 239) x1 = 239;
    if (y1 > PS_Y1 - 1) y1 = PS_Y1 - 1;
    if (x1 < x0 || y1 < y0) return;
    w = x1 - x0 + 1, rows = (int32_t)(CV_MAX / (uint32_t)w);
    for (y = y0; y <= y1; y += rows) {
        int32_t h = q_min(rows, y1 - y + 1);
        cv_begin((uint32_t)w, (uint32_t)h, Q_BG);
        cv_ox = -x0, cv_oy = -y;
        fn(c);
        cv_ox = 0, cv_oy = 0;
        cv_blit((uint32_t)x0, (uint32_t)y);
    }
}
static UI_SMALL void quest_page_draw(void)
{
    static const ps_draw FN[8] = {ps_forge, ps_sea, ps_alch, ps_towers, ps_forest, ps_cathedral, ps_torch, ps_pedals};
    static const ps_dirty DF[8] = {ps_forge_dirty, ps_sea_dirty, ps_alch_dirty, ps_towers_dirty, ps_forest_dirty, ps_cathedral_dirty, ps_torch_dirty, ps_pedals_dirty};
    static const char *const TT[8] = {"Forge", "LFO", "Alchemy", "FX Slicer", "FX Delay", "FX Reverb", "Scale", "FX Pedals"}, *const SS[8] = {"the amp envelope", "the tide", "the lfo brew", "sleepless city", "the forest", "the cathedral", "the torch wall", "pedals"};
    static ps_r prev[16];
    static uint32_t last_ms, frame, last_sig, last_sc = 99, prev_notes;
    char lb[4][10] = {{0}}, vb[4][16] = {{0}}, pg[16];
    const char *lab[4], *val[4];
    if (fm1_in.notes & ~prev_notes) pn_t0 = fm1_ms | 1u;
    prev_notes = fm1_in.notes;
    int32_t ratio[4], k4[4], sc = q_page_scene(), i, mx = 0;
    uint32_t sig, np = 0, k = 0, pi, n;
    const track_t *t = &trk[song.sel];
    ps_c c;
    ps_r rs[16];
    q_tick_clocks();
    q_enter((uint8_t)(QSN_PAGE0 + (uint32_t)sc));
    q_cols(lab, val, ratio, vb, lb);
    for (i = 0; i < 4; i++) k4[i] = ratio[i] < 0 ? 0 : ratio[i];
    c.k = k4, c.t = fm1_ms;
    switch (sc) {
    case 1: c.a = t->p[P_LWAVE]; break;
    case 2: for (i = 0; i < 4; i++) { int32_t m = k4[i] > 500 ? (k4[i] - 500) * 2 : (500 - k4[i]) * 2; if (m > mx) mx = m, c.d = i; }
            c.a = 200 + (t->p[P_LRATE] < 0 ? 0 : t->p[P_LRATE] > 127 ? 127 : t->p[P_LRATE]) * 8000 / 127, c.b = mx, c.c = t->p[P_LWAVE]; break;
    case 3: c.a = t->p[P_SLCR], c.b = t->p[P_SLPAT] > 0 ? t->p[P_SLPAT] - 1 : 0, c.d = t->p[P_SLRATE]; break;
    case 6: c.a = t->p[P_ROOT] % 12, c.b = t->p[P_SCALE], c.c = t->p[P_QUANT], c.d = t->p[P_CHORD]; break;
    default: break;
    }
    if (sc != 3) pt_ok = 0;
    else {
        static uint32_t pt_ms;
        if (fm1_ms - pt_ms >= 30u) { pt_ms = fm1_ms; pt_ok = (uint8_t)(pt_step(&c) ? 2u : 1u); }
    }
    sig = (uint32_t)sc * 31u + q_msg_sig();
    for (i = 0; i < 4; i++) sig = studio_hash(sig * 7u + (uint32_t)ratio[i], vb[i]);
    sig = sig * 31u + (uint32_t)(c.a + c.b * 3 + c.c * 5 + c.d * 7);
    if (sc == 3) for (i = 0; i < 16; i++) sig = sig * 3u + (uint32_t)pt_disp[i];
    for (pi = 0; pi < NPAGES; pi++) if (PAGES[pi].fam == cur_page()->fam) { np++; if (pi == ui.page) k = np; }
    if (q_changed(0, studio_hash((uint32_t)song.g[G_BPM] * 7u + song.playing * 3u + q_msg_sig() + k, TT[sc]))) {
        cv_begin(240, 19, Q_BG);
        q_title(6, TT[sc], Q_HI);
        q_text(6 + text_w(&FONT_QT, TT[sc]) + 8, 4, SS[sc], Q_DIM);
        str_cpy(pg, cur_page()->title, sizeof pg);
        if (190 - text_w(&FONT_P, pg) > 6 + text_w(&FONT_QT, TT[sc]) + 8 + text_w(&FONT_P, SS[sc]) + 6) q_text_r(190, 4, pg, Q_DIM);   /* (the page name only if it fits) */
        q_bar_right_play();
        q_bar_end();
    }
    q_dials(lab, val, ratio, 12, sig);
    if (sc == 0) pf_tick(&c);
    if (last_sc != (uint32_t)sc || last_sig != sig || Q_FULL) {                           /* first paint, or a knob moved: the whole scene */
        last_sc = (uint32_t)sc, last_sig = sig, last_ms = fm1_ms;
        ps_paint(FN[sc], &c, 0, PS_Y0, 239, PS_Y1 - 1);
        n = DF[sc](&c, rs);
        for (i = 0; i < 16; i++) prev[i] = i < (int32_t)n ? rs[i] : (ps_r){0, 0, -1, -1, 0};
    } else if (fm1_ms - last_ms >= 30u) {                                                 /* ~30 frames a second: only the rectangles that moved */
        last_ms = fm1_ms, frame++;
        n = DF[sc](&c, rs);
        for (i = 0; i < (int32_t)n && i < 16; i++) {
            ps_r u = rs[i];
            if (rs[i].div && frame % rs[i].div) continue;
            if (u.x1 < u.x0) continue;
            if (prev[i].x1 >= prev[i].x0) { u.x0 = (int16_t)q_min(u.x0, prev[i].x0), u.y0 = (int16_t)q_min(u.y0, prev[i].y0), u.x1 = (int16_t)q_max(u.x1, prev[i].x1), u.y1 = (int16_t)q_max(u.y1, prev[i].y1); }
            ps_paint(FN[sc], &c, u.x0, u.y0, u.x1, u.y1);
            prev[i] = rs[i];
        }
    }
    qb.fresh = 0;
}
/* ======================================================= VISUALIZER (menu): six pages driven by the four tracks' real output levels ======================================================= */
static const uint16_t PV_HC[4] = {RGB(0xf0, 0xc8, 0x60), RGB(0x7a, 0xb0, 0x4e), RGB(0xe0, 0x8a, 0x3a), RGB(0x5a, 0x8a, 0xd0)};
static struct { int32_t e[4], v[16], b[40], beat; uint32_t pk[4]; } pv;
static UI_SMALL void pv_tick(uint32_t t)                                                   /* levels 0..1000 from the tracks' peaks (read and cleared, like the TRACKS meters) */
{
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        int32_t pk = i == TRK_DRUM ? drums.peak : trk[i].peak, m;
        if (i == TRK_DRUM) drums.peak = 0; else trk[i].peak = 0;
        m = trk_silent(&trk[i]) ? 0 : meter_px(pk) * 1000 / 82;
        pv.e[i] = q_max(m, pv.e[i] * 86 / 100);
    }
    pv.beat = 1000 - (int32_t)((uint32_t)q_beat_q8() & 255u) * 1000 / 256;
    (void)t;
}
#ifndef FELUCCA_VIZ_FULL
#define FELUCCA_VIZ_FULL 0   /* 1 = the two heaviest pages, Sky (1.4 KB) and Campfire (1.0 KB), are built too: 6 pages instead of 4 */
#endif
static UI_SMALL void pv_sky(const ps_c *c)
{
    static const uint8_t SKY[6] = {0, 0, 1, 1, 2, 13};
    int32_t i, x, y, cx, nb = 12, bass = (1000 - (1000 - c->a)) * 600 / 1000, mr = 14 + bass * 4 / 1000, tq = (int32_t)(c->t % 60000u), bw = 240 / nb;
    p_sky(PS_Y0, 120, SKY, 6);
    for (i = 0; i < 46; i++) if (p_sin((uint32_t)tq * 1u + (uint32_t)i * 300u, 20) > -89) cv_pset((int32_t)p_hs((uint32_t)i) * 240 / 1024, 22 + (int32_t)p_hs((uint32_t)i + 4u) * 70 / 1024, QP(i & 1 ? 7 : 6));
    p_ell(190, 52, mr + 4, mr + 4, QP(1)), p_ell(190, 52, mr, mr, QP(7)), p_ell(195, 49, mr - 3, mr - 3, QP(0)), cv_pset(186, 56, QP(6)), cv_pset(190, 59, QP(6));
    for (cx = 0; cx < 2; cx++) for (x = 0; x < 240; x += 3) {
        int32_t L = cx ? (c->k[2] + c->k[3]) / 2 : (c->k[0] + c->k[1]) / 2, w = 0, top = 34 + cx * 16 - L * 8 / 1000 + (qsin((x * 9 / 10 + cx * 11 + (int32_t)(tq / 160) + L / 30) & 63) * 14 / 127 + qsin((x * 2 + (int32_t)(tq / 70)) & 63) * 6 / 127) * 6 / 10, hgt = 22 + L * 14 / 1000 + qsin((x + cx * 7 + (int32_t)(tq / 250) + L / 40) & 63) * (8 + L / 60) / 127;
        (void)w;
        for (y = 0; y < hgt; y += 3) p_R(x, top + y, 3, 2, QP(y < hgt * 3 / 10 ? 11 : y < hgt * 7 / 10 ? 12 : 13));
    }
    for (i = 0; i < 2; i++) for (x = 0; x < 240; x++) { int32_t amp = (i ? 18 : 22) + c->k[i ? 3 : 0] * 14 / 1000, h = amp + qsin((x * 3 / 5 + i * 20) & 63) * amp / 2 / 127 + qsin((x * 2 + i * 9) & 63) * amp / 5 / 127; p_R(x, (i ? 126 : 124) - h, 1, h + 1, i ? RGB(0x1e, 0x1a, 0x30) : RGB(0x2a, 0x26, 0x40)); }
    for (i = 0; i < nb; i++) pv.v[i] = pv.v[i] * 6 / 10 + (c->k[i % 4] * (950 - i * 40) / 1000 + (int32_t)p_hs((uint32_t)(c->t / 125u) + (uint32_t)i * 37u) / 16 + 40) * 4 / 10;      /* each band follows a hero */
    for (x = 0; x < 240; x++) { int32_t f = x * 100 / bw, ii = q_min(nb - 1, f / 100), j = q_min(nb - 1, ii + 1), h = (pv.v[ii] + (pv.v[j] - pv.v[ii]) * (f - ii * 100) / 100) * 56 / 1000 + 12; p_R(x, 128 - h, 1, h, x & 1 ? RGB(0x16, 0x12, 0x24) : RGB(0x12, 0x0e, 0x1e)), p_R(x, 128 - h, 1, 2, QP(h > 44 ? 7 : 13)); }
    p_R(0, 128, 240, 69, RGB(0x0d, 0x0a, 0x18));
    for (y = 130; y < 197; y += 2) {
        int32_t off = qsin((y * 3 / 10 + (int32_t)(tq / 80)) & 63) * (1 + (y - 128) * 3 / 69) * (100 + (c->k[0] + c->k[1] + c->k[2] + c->k[3]) / 12) / 12700;
        for (x = 0; x < 240; x += 2) { int32_t f = (x + off) * 100 / bw, ii = q_max(0, q_min(nb - 1, f / 100)); if ((y - 128) * 11 / 10 < pv.v[ii] * 56 / 1000 + 12 && ((x + y) & 3) == 0) cv_pset(x, y, RGB(0x2a, 0x26, 0x40)); }
        if (y % 6 == 0) {                                                                    /* the moon's glints: each line rides one hero's loop, left to right, as wide as its level */
            int32_t tr = (y / 6) % 4, len = clamp(trk[tr].p[P_SLEN], 1, NSTEP), pos = song.playing ? (int32_t)(trk[tr].seq_idx % (uint32_t)len) : (int32_t)(c->t / 400u) % len;
            int32_t w = 8 + c->k[tr] * 16 / 1000, gx = 14 + pos * (200 - w) / len + ((int32_t)p_hs((uint32_t)y) - 512) * 10 / 1024;
            p_R(gx, y, w, 1, c->k[tr] > 400 ? QP(7) : QP(13));
        }
    }
    p_R(0, 128, 240, 1, QP(13));
}

static UI_SMALL void pv_tides(const ps_c *c)
{
    static const uint8_t SKY[5] = {0, 0, 1, 1, 2};
    static const char *const N[4] = {"MAGE", "ARCHER", "CLERIC", "WARRIOR"};
    int32_t i, x;
    p_sky(PS_Y0, 197, SKY, 5);
    for (i = 0; i < 4; i++) {
        int32_t y0 = 56 + i * 34, amp = 6 + c->k[i] * 22 / 1000;
        for (x = 0; x < 240; x += 2) {
            int32_t y = y0 + qsin(((x * 45 / 100 * 64 / 100) + (int32_t)(c->t * (14u + (uint32_t)i * 4u) / 1000u) + i * 10) & 63) * amp / 127 + qsin((x * 11 / 100 * 64 / 100 * 10 - (int32_t)(c->t / 20u)) & 63) * amp / 4 / 127;
            p_R(x, y, 2, 2, PV_HC[i]), p_R(x, y + 3, 2, 6, p_shade(PV_HC[i], 400));
        }
        q_text(6, y0 - 22, N[i], PV_HC[i]);
    }
}
static UI_SMALL void pv_ring(const ps_c *c)
{
    int32_t i, a, cx = 120, cy = 108;
    p_R(0, PS_Y0, 240, 178, RGB(0x0c, 0x0a, 0x14));
    for (i = 0; i < 4; i++) {
        int32_t r0 = 18 + i * 14, amp = 3 + c->k[i] * 12 / 1000;
        for (a = 0; a < 120; a++) { int32_t ph = a * 64 / 120, r = r0 + qsin((ph * (3 + i) + (int32_t)(c->t * (10u + (uint32_t)i * 4u) / 1000u * 64u / 10u)) & 63) * amp / 127; p_R(cx + qcos(ph) * r / 127, cy + qsin(ph) * r / 127, 2, 2, PV_HC[i]); }
    }
    p_ell(cx, cy, 6 + c->a * 6 / 1000, 6 + c->a * 6 / 1000, RGB(0xff, 0xf6, 0xd8)), p_ell(cx, cy, 3, 3, QP(10));
}
static UI_SMALL void pv_canyon(const ps_c *c)
{
    int32_t i;
    p_R(0, PS_Y0, 240, 178, RGB(0x0a, 0x08, 0x14)), p_R(0, PS_Y0, 240, 80, RGB(0x1a, 0x12, 0x30));
    for (i = 0; i < 40; i++) {
        int32_t tr = i / 10, tgt = q_min(1000, c->k[tr] * (500 + (int32_t)p_hs((uint32_t)(c->t / 110u) + (uint32_t)i * 23u) / 2) / 1000 + 60);
        pv.b[i] = q_max(tgt, pv.b[i] * 88 / 100);
        { int32_t h = 8 + pv.b[i] * 62 / 1000, x = i * 6; p_R(x, 108 - h, 5, h, PV_HC[tr]), p_R(x, 108 - h, 5, 2, RGB(0xff, 0xf6, 0xd8)), p_R(x, 110, 5, h * 55 / 100, p_shade(PV_HC[tr], 270)); }
    }
    p_R(0, 108, 240, 2, RGB(0x3a, 0x30, 0x50));
}
static UI_SMALL void pv_scope(const ps_c *c)
{
    int32_t x, i, py = 108;
    p_R(0, PS_Y0, 240, 178, RGB(0x06, 0x14, 0x0c));
    for (x = 0; x < 240; x += 24) p_R(x, PS_Y0, 1, 178, RGB(0x0d, 0x2a, 0x18));
    for (i = 30; i < 190; i += 20) p_R(0, i, 240, 1, RGB(0x0d, 0x2a, 0x18));
    p_R(0, 108, 240, 1, RGB(0x1a, 0x4a, 0x2a));
    for (x = 0; x < 240; x++) {
        int32_t y = 0;
        for (i = 0; i < 4; i++) y += qsin(((x * (50 + i * 37) / 1000 * 64 / 6 / 10) + (int32_t)(c->t * (30u + (uint32_t)i * 10u) / 1000u * 64u / 10u)) & 63) * c->k[i] / 1000 * 16 / 127;
        cv_line(x - 1, py, x, 108 + y, RGB(0x6a, 0xff, 0x9a)), py = 108 + y;
    }
    for (i = 0; i < 24; i++) {
        int32_t tr = i / 6, tgt = q_min(1000, c->k[tr] * (400 + (int32_t)p_hs((uint32_t)(c->t / 100u) + (uint32_t)i * 19u) * 6 / 10) / 1000);
        pv.b[i] = q_max(tgt, pv.b[i] * 85 / 100);
        p_R(8 + i * 96 / 10, 192 - pv.b[i] * 40 / 1000, 7, pv.b[i] * 40 / 1000 + 1, RGB(0x2a, 0xb0, 0x5a));
    }
}
#if FELUCCA_VIZ_FULL
static UI_SMALL void pv_fire(const ps_c *c)
{
    static const uint8_t SKY[5] = {0, 0, 1, 1, 2};
    static const uint16_t FL[5] = {RGB(0xa8, 0x32, 0x2a), RGB(0xe0, 0x60, 0x2a), RGB(0xf6, 0xa2, 0x3a), RGB(0xff, 0xd8, 0x78), RGB(0xff, 0xf6, 0xd8)};
    int32_t i, f, act = (c->k[0] + c->k[1] + c->k[2] + c->k[3]) / 4, cx = 120, by = 150, hgt = 16 + act * 36 / 1000 + c->a * 6 / 1000;
    p_sky(PS_Y0, 130, SKY, 5);
    for (i = 0; i < 40; i++) cv_pset((int32_t)p_hs((uint32_t)i) * 240 / 1024, 22 + (int32_t)p_hs((uint32_t)i + 3u) * 80 / 1024, i & 1 ? RGB(0x8a, 0x8a, 0xa8) : RGB(0x5a, 0x5a, 0x78));
    p_R(0, 138, 240, 59, RGB(0x1a, 0x14, 0x10));
    for (i = 140; i < 197; i += 5) p_R(0, i, 240, 1, RGB(0x12, 0x0e, 0x0a));
    { int32_t gr = 70 + act * 20 / 1000, gx, gy; for (gy = -gr; gy <= gr; gy++) for (gx = -gr; gx <= gr; gx++) if (gx * gx + gy * gy < gr * gr && ((gx + gy) & 3) == 0 && (gy & 1) == 0) cv_pset(cx + gx, by - 14 + gy, RGB(0x60, 0x38, 0x1c)); }   /* the glow: a translucent halo (a checker of warm pixels) */
    for (i = 0; i < 3; i++) p_R(cx - 14 + i * 10, by + 2, 18, 4, RGB(0x3a, 0x24, 0x18));
    for (f = 0; f < 5; f++) {
        int32_t w = 14 - f * 24 / 10, hh = hgt * (100 - f * 17) / 100, x0 = cx + qsin((f * 3 + (int32_t)(c->t / 90u)) & 63) / 60;
        p_poly((const int32_t[5][2]){{x0 - w, by}, {x0 - w * 6 / 10, by - hh * 55 / 100}, {x0 + qsin(((int32_t)(c->t / 60u) + f) & 63) * 3 / 127, by - hh}, {x0 + w * 6 / 10, by - hh / 2}, {x0 + w, by}}, 5, FL[f]);
    }
    for (i = 0; i < 8; i++) { int32_t u = (int32_t)((c->t * (600u + (uint32_t)i * 80u) / 1000u + (uint32_t)i * 125u) % 1000u); p_R(cx + qsin(i * 13 + (int32_t)(c->t / 160u)) * 16 / 127 + u * 6 / 1000, by - 20 - u * (50 + act * 40 / 1000) / 1000, 2, 2, u < 600 ? FL[3] : FL[0]); }
    for (i = 0; i < 4; i++) {
        static const int16_t PX[4] = {-62, -26, 26, 62}, PY[4] = {10, 22, 22, 10};
        int32_t x = cx + PX[i], y = by + PY[i];
        p_R(x - 9, y + 20, 18, 3, RGB(0x10, 0x0c, 0x10));
        q_spr(Q_SEAT[i], x - 12, y - 8 - c->k[i] * 2 / 1000, 1, i >= 2, -1);
        p_R(x - 2, y + 24, 5, 5, PV_HC[i]);
    }
}
#endif
/* pages: Tides, Ring, Canyon, Scope (+ Sky first and Campfire last when FELUCCA_VIZ_FULL) */
static const ps_r PV_R[5] = {{0, 19, 239, 196, 0}, {0, 19, 239, 196, 0}, {40, 30, 200, 190, 0}, {0, 28, 239, 160, 0}, {0, 19, 239, 196, 0}};
static const ps_draw PV_FN[5] = {pv_sky, pv_tides, pv_ring, pv_canyon, pv_scope};
static const char *const PV_NM[5] = {"Sky", "Tides", "Ring", "Canyon", "Scope"};
static UI_SMALL uint32_t pv_dirty(uint32_t page, ps_r *o)
{
    o[0] = PV_R[page % PV_N];
    return 1;
}
static UI_SMALL void quest_viz_draw(void)
{
    static uint32_t last_ms, last_pg = 99;
    ps_c c;
    ps_r r;
    char b[8];
    q_tick_clocks();
    q_enter(QSN_VIZ);
    c.k = pv.e, c.a = pv.beat, c.t = fm1_ms, c.b = c.c = c.d = 0;
    if (q_changed(0, 100u + pv_pg + q_msg_sig())) {
        cv_begin(240, 19, Q_BG);
        q_title(6, "Visualizer", Q_HI);
        q_text(6 + text_w(&FONT_QT, "Visualizer") + 8, 4, PV_NM[pv_pg % PV_N], Q_DIM);
        b[0] = (char)('1' + pv_pg % PV_N), b[1] = '/', b[2] = (char)('0' + PV_N), b[3] = 0;
        q_text_r(234, 4, b, Q_TEXT);
        q_bar_end();
    }
    if (q_changed(1, 7u + pv_pg)) {
        cv_begin(240, 44, Q_BG);
        cv_rect(0, 0, 240, 1, Q_LINE);
        q_text(6, 4, "presets: page", Q_DIM), q_text_r(234, 4, "oct-: back", Q_DIM);
        q_text(6, 22, "levels move the picture", Q_DIM);
        cv_blit(0, 194);
    }
    if (last_pg != pv_pg || Q_FULL) {
        last_pg = pv_pg;
        pv_tick(fm1_ms);
        ps_paint(PV_FN[pv_pg % PV_N], &c, 0, PS_Y0, 239, PS_Y1 - 1);
    } else if (fm1_ms - last_ms >= 30u) {
        last_ms = fm1_ms;
        pv_tick(fm1_ms);
        pv_dirty(pv_pg, &r);
        ps_paint(PV_FN[pv_pg % PV_N], &c, r.x0, r.y0, r.x1, r.y1);
    }
    qb.fresh = 0;
}
/* ======================================================= GAMES: FM Hero (5 lanes) and Piano Hero (the 27 keys), the song chosen from the 12 sent from the web editor ======================================================= */
/* The chart is the song as stored (notes of 4 bytes: u16 start in 1/48 beat, u8 pitch, u8 length). A note falls for 1.5 s and is hit by pressing its key within 170 ms;
 * perfect <= 70 ms. Nothing here plays the song: the keys you press sound as always. */
#define GM_FALL 1500
#define GM_LEAD 2000
static const uint8_t *gm_notes(void) { return (const uint8_t *)sng_hdr((uint32_t)gm.kind * SNG_PER + gm.sel) + SNG_DATA; }
static const sng_hdr_t *gm_song(void) { return sng_hdr((uint32_t)gm.kind * SNG_PER + gm.sel); }
static uint32_t gm_bpm(void)                                                           /* the tempo it plays at: the one picked, or the song's own */
{
    uint32_t x = (uint32_t)gm.kind * SNG_PER + gm.sel;
    return x < 2u * SNG_PER && gm_bpmx[x] ? (uint32_t)gm_bpmx[x] : gm_song()->bpm ? gm_song()->bpm : 120u;
}
static void gm_tempo(int32_t s)                                                        /* SELECT on the tapes: +- 1 bpm for the tape in the middle */
{
    uint32_t x = (uint32_t)gm.kind * SNG_PER + gm.sel;
    if (!s || gm.ph != 1 || gm.sel >= SNG_PER || !sng_ok[x]) return;
    gm_bpmx[x] = (int16_t)clamp((int32_t)gm_bpm() + s, 40, 240);
    ui.force = 1;
}
static uint32_t gm_time(uint32_t i)                                                    /* ms from the start of the song */
{
    const uint8_t *n = gm_notes() + i * 4u;
    uint32_t t48 = n[0] | (uint32_t)n[1] << 8, bpm = gm_bpm();
    return t48 * (60000u / bpm) / 48u;
}
static uint32_t gm_note(uint32_t i) { return gm_notes()[i * 4u + 2u]; }
static uint32_t gm_lane(uint32_t i)
{
    uint32_t p = gm_note(i);
    if (gm.kind == 1) return p % 4u;                                                    /* FM Hero: four lanes, the same every four keys */
    {   /* Piano: the key (of the 27) with that note's name nearest to it: an octave out of reach is played in the nearest one */
        int32_t k = (int32_t)p - 53, best = 0, d = 99, c;
        for (c = 0; c < 27; c++) {
            int32_t dd = c > k ? c - k : k - c;
            if ((53u + (uint32_t)c) % 12u != p % 12u) continue;
            if (dd < d) d = dd, best = c;
        }
        return (uint32_t)best;
    }
}
static int gm_match(uint32_t i, uint32_t k)                                            /* does key k play note i: the name (any octave) in Piano, the lane in FM Hero */
{
    return gm.kind ? (k % 4u) == gm_note(i) % 4u : (53u + k) % 12u == gm_note(i) % 12u;
}
static int32_t gm_now(void) { return gm.pos; }
static void gm_start(void)
{
    uint32_t i;
    for (i = 0; i < sizeof gm.done; i++) gm.done[i] = 0;
    for (i = 0; i < 27u; i++) gm.snd[i] = 0;
    gm.pos = -GM_LEAD, gm.adv = 0, gm.oct_t0 = 0, gm.help = 0, gm.wrong = 0;
    gm.t0 = fm1_ms, gm.score = gm.combo = gm.best = gm.hit = gm.miss = gm.good = gm.first = 0;
    gm.n = gm_song()->notes, gm.prev_keys = fm1_in.notes, gm.endms = gm_time(gm.n - 1u) + 1500u, gm.ph = 2;
}
static uint32_t gm_next(void)                                                          /* the first note not played yet */
{
    uint32_t i;
    for (i = gm.first; i < gm.n; i++)
        if (!((gm.done[i >> 3] >> (i & 7u)) & 1u)) return i;
    return 0xFFFFu;
}
static void gm_stop_sound(void)
{
    uint32_t k;
    for (k = 0; k < 27u; k++) if (gm.snd[k]) trk_note_off(TSEL, gm.snd[k]), gm.snd[k] = 0;
}
static void gm_tick(void)                                                              /* the clock (it waits in Piano, it speeds up in FM Hero), the presses, the misses */
{
    uint32_t keys = fm1_in.notes, edges = keys & ~gm.prev_keys, rel = gm.prev_keys & ~keys, k, i, j, now = fm1_ms, dtm = gm.adv ? now - gm.adv : 0u;
    int32_t win = gm.kind ? 260 : 220, rate = gm.kind ? 150 + (int32_t)(gm.hit * 3u > 106u ? 106u : gm.hit * 3u) : 256;
    gm.adv = now, gm.prev_keys = keys;
    if (dtm > 100u) dtm = 100u;
    for (k = 0; k < 27u; k++)
        if (((rel >> k) & 1u) && gm.snd[k]) trk_note_off(TSEL, gm.snd[k]), gm.snd[k] = 0;
    j = gm_next();
    if (gm.kind == 0 && j != 0xFFFFu) {                                                 /* Piano: the screen slows to the line and waits for the right key */
        int32_t till = (int32_t)gm_time(j) - gm.pos;
        if (till < 600) rate = rate * (till < 0 ? 0 : till) / 600;
    }
    gm.pos += (int32_t)dtm * rate / 256;
    if (gm.kind == 0 && j != 0xFFFFu && gm.pos > (int32_t)gm_time(j)) gm.pos = (int32_t)gm_time(j);
    for (k = 0; k < 27u; k++) {
        int32_t bd = win + 1, bi = -1;
        if (!((edges >> k) & 1u)) continue;
        for (i = gm.first; i < gm.n; i++) {
            int32_t dt = (int32_t)gm_time(i) - gm.pos;
            if (dt > win) break;
            if ((gm.done[i >> 3] >> (i & 7u)) & 1u || !gm_match(i, k)) continue;
            if ((dt < 0 ? -dt : dt) < bd) bd = dt < 0 ? -dt : dt, bi = (int32_t)i;
        }
        if (bi >= 0) {
            gm.done[(uint32_t)bi >> 3] |= (uint8_t)(1u << ((uint32_t)bi & 7u));
            gm.combo++, gm.hit++, gm.good += bd > 80;
            gm.score += (bd <= 80 ? 100u : 50u) * (1u + (gm.combo > 30u ? 3u : gm.combo / 10u));
            if (gm.combo > gm.best) gm.best = gm.combo;
            gm.snd[k] = (uint8_t)gm_note((uint32_t)bi);                                 /* the sound is the note of the song, in its own octave */
            trk_note_on(TSEL, gm.snd[k], 100);
        } else gm.wrong++, gm.combo = 0;                                                /* a wrong key: nothing sounds */
    }
    while (gm.first < gm.n && ((gm.done[gm.first >> 3] >> (gm.first & 7u)) & 1u)) gm.first++;
    if (gm.kind) while (gm.first < gm.n && (int32_t)gm_time(gm.first) < gm.pos - win) {   /* FM Hero: too late is a miss, the song goes on */
        gm.miss++, gm.combo = 0, gm.first++;
        while (gm.first < gm.n && ((gm.done[gm.first >> 3] >> (gm.first & 7u)) & 1u)) gm.first++;
    }
    if (fm1_in.buttons & (1u << panel.btn[B_OCTDN])) {                                  /* hold OCT-: back to the list of songs */
        if (!gm.oct_t0) gm.oct_t0 = now | 1u;
        else if ((int32_t)(now - gm.oct_t0) > 600) { gm_stop_sound(); gm.ph = 1, gm.oct_t0 = 0, ui.force = 1; return; }
    } else gm.oct_t0 = 0;
    if (gm.first >= gm.n && gm.pos > (int32_t)gm.endms) { gm_stop_sound(); gm.ph = 3; }
}
static void gm_rewind(int32_t s)                                                       /* KNOB 1 to the left: back in the song, as far as it is turned; never before 0 */
{
    int32_t np;
    uint32_t i;
    if (gm.ph != 2 || s >= 0 || gm.pos <= 0) return;
    np = gm.pos + s * 450;
    if (np < 0) np = 0;
    gm.pos = np;
    gm.first = 0;
    for (i = 0; i < gm.n; i++)                                                         /* what was played from here on has to be played again */
        if ((int32_t)gm_time(i) >= np - 300) {
            if ((gm.done[i >> 3] >> (i & 7u)) & 1u) { gm.done[i >> 3] &= (uint8_t)~(1u << (i & 7u)); if (gm.hit) gm.hit--; }
        } else if (!gm.first || i >= gm.first) gm.first = i + 1u;
    gm.combo = 0;
    gm_stop_sound();
    tr_start(4, 450);                                                                  /* the tape */
    ui.force = 1;
}
static uint32_t gm_hint_mask(void)                                                     /* the keys of the note to play, lit */
{
    uint32_t i = gm_next(), k, m = 0;
    if (i == 0xFFFFu || (int32_t)gm_time(i) - gm.pos > 800) return 0;
    if (gm.kind == 0) return 1u << gm_lane(i);
    for (k = 0; k < 27u; k++) if (k % 4u == gm_lane(i)) m |= 1u << k;
    return m;
}
static uint32_t gm_guide_mask(void)                                                    /* dim: the first key of every four (FM Hero); the same note's other octaves (Piano) */
{
    uint32_t i = gm_next(), k, m = 0;
    if (gm.kind) { for (k = 0; k < 27u; k += 4u) m |= 1u << k; return m; }
    if (i == 0xFFFFu || (int32_t)gm_time(i) - gm.pos > 800) return 0;
    for (k = 0; k < 27u; k++) if (gm_match(i, k)) m |= 1u << k;
    return m;
}
static void gm_input(uint32_t ok, uint32_t back, int32_t s)
{
    if (s && gm.ph == 0) gm.kind ^= 1u;
    if (s && gm.ph == 1) {                                                             /* the strip rolls one tape: the target moves, the picture eases to it */
        gm_tgt += s > 0 ? 256 : -256;
        gm.sel = (uint8_t)((((gm_tgt >> 8) % (int32_t)GM_TAPES) + (int32_t)GM_TAPES) % (int32_t)GM_TAPES);
        gm.help = 0;
    }
    if (back) {
        if (gm.ph == 0) ui.menu = 1;
        else if (gm.ph == 2) { /* a tap does nothing in a game: hold OCT- */ }
        else gm.ph = gm.ph == 3 ? 1 : 0;
        gm.help = 0;
        ui.force = 1;
    } else if (ok) {
        if (gm.ph == 0) gm.ph = 1, gm.sel = 0, gm_tgt = gm_cur = 0;
        else if (gm.ph == 1 && gm.sel >= SNG_PER) gm.help = 1;                         /* the empty tape: how to put a song on it */
        else if ((gm.ph == 1 || gm.ph == 3) && sng_ok[(uint32_t)gm.kind * SNG_PER + gm.sel]) gm_start();
        ui.force = 1;
    }
}
static void gm_cass(int32_t x, int32_t y, int32_t w, uint16_t col, int empty)          /* a cassette: body, label, the window with its two reels */
{
    int32_t h = w * 3 / 5, lw = w * 3 / 4, lx = x + (w - lw) / 2, ly = y + h / 8, lh = h * 2 / 5, wy = y + h * 11 / 20, wh = h * 3 / 10, r = wh / 2 > 3 ? wh / 2 : 3;
    uint16_t cream = RGB(0xd8, 0xcf, 0xae), ink = RGB(0x14, 0x10, 0x1a);
    cv_rect(x, y, w, h, ink);
    cv_rect(x + 1, y + 1, w - 2, h - 2, col);
    cv_rect(lx, ly, lw, lh, cream);
    if (empty) cv_rect(lx + lw / 2 - 1, ly + 3, 3, lh - 6, ink), cv_rect(lx + lw / 2 - lh / 2 + 3, ly + lh / 2 - 1, lh - 6, 3, ink);   /* a plus: add a file */
    else cv_rect(lx + 4, ly + lh / 3, lw - 8, 1, ink), cv_rect(lx + 4, ly + lh * 2 / 3, lw * 2 / 3, 1, ink);
    cv_rect(x + w / 4, wy, w / 2, wh, ink);
    q_disc(x + w / 4 + r, wy + wh / 2, r - 1, cream), q_disc(x + w * 3 / 4 - r, wy + wh / 2, r - 1, cream);
    cv_rect(x + w / 4 + r - 1, wy + wh / 2 - 1, 2, 2, ink), cv_rect(x + w * 3 / 4 - r - 1, wy + wh / 2 - 1, 2, 2, ink);
}
static void gm_strip(void)                                                             /* the floating tapes: the middle one big, the others smaller, rolling */
{
    static const uint16_t TC[SNG_PER] = {RGB(0x5a, 0x8a, 0xd0), RGB(0x7a, 0xb0, 0x4e), RGB(0xf0, 0xc8, 0x60), RGB(0xe0, 0x80, 0x3a), RGB(0xc8, 0x48, 0x3a), RGB(0x9a, 0x6a, 0xc8)};
    int32_t pass, i, bob = (qsin((int32_t)(fm1_ms / 40u) & 63) * 3) / 127;
    cv_begin(240, 80, Q_BG);
    for (pass = 0; pass < 2; pass++)                                                   /* the far ones first */
        for (i = 0; i < (int32_t)GM_TAPES; i++) {
            int32_t off = (((i * 256 - gm_cur) % (int32_t)(GM_TAPES * 256)) + (int32_t)(GM_TAPES * 256) + (int32_t)(GM_TAPES * 128)) % (int32_t)(GM_TAPES * 256) - (int32_t)(GM_TAPES * 128);
            int32_t a = off < 0 ? -off : off, t = a < 256 ? a : 256, cx = 120 + off * 86 / 256, w = 70 - 24 * t / 256, y = 8 + 14 * t / 256 - bob * (256 - t) / 256;
            uint16_t c = i < (int32_t)SNG_PER ? TC[i] : Q_PDIM;
            if ((a >= 128) != (pass == 0) || cx < -40 || cx > 280) continue;
            if (a >= 128) c = (uint16_t)((c >> 1) & 0x7BEFu);                          /* the ones to the side are dimmer */
            gm_cass(cx - w / 2, y, w, c, i >= (int32_t)SNG_PER);
            if (a < 64) cv_rect(cx - w / 2, y + w * 3 / 5 + 4, w, 2, Q_HI);           /* the one picked */
        }
    cv_blit(0, 26);
}
static void gm_field(const ps_c *c)                                                    /* the falling notes, the hit line, the keys held */
{
    int32_t now = gm.pos, w = gm.kind == 0 ? 8 : 36, gap = gm.kind == 0 ? 0 : 8, x0 = gm.kind == 0 ? 12 : 36, ln = gm.kind == 0 ? 27 : 4, HY = 168, i, l;
    (void)c;
    p_R(0, PS_Y0, 240, 175, RGB(0x12, 0x0c, 0x18));
    for (l = 0; l < ln; l++) {
        int32_t bl = gm.kind == 0 && ((53 + l) % 12 == 1 || (53 + l) % 12 == 3 || (53 + l) % 12 == 6 || (53 + l) % 12 == 8 || (53 + l) % 12 == 10), held = 0, k;
        p_R(x0 + l * (w + gap), PS_Y0, w, 175, bl ? RGB(0x1c, 0x14, 0x22) : RGB(0x20, 0x18, 0x28));
        if (gm.kind == 0) held = (int32_t)((fm1_in.notes >> l) & 1u);
        else for (k = l; k < 27; k += 4) held |= (int32_t)((fm1_in.notes >> k) & 1u);
        if (held) p_R(x0 + l * (w + gap), HY + 4, w, 14, PV_HC[l & 3]);
    }
    p_R(0, HY, 240, 3, QP(10));
    for (i = (int32_t)gm.first; i < (int32_t)gm.n; i++) {
        int32_t dt = (int32_t)gm_time((uint32_t)i) - now, y;
        if (dt > GM_FALL) break;
        if ((gm.done[(uint32_t)i >> 3] >> ((uint32_t)i & 7u)) & 1u) continue;
        y = HY - dt * (HY - PS_Y0) / GM_FALL;
        l = (int32_t)gm_lane((uint32_t)i);
        p_R(x0 + l * (w + gap), y - 5, w, 6, PV_HC[l & 3]), p_R(x0 + l * (w + gap), y - 5, w, 1, RGB(0xff, 0xf6, 0xd8));
    }
}
static const char *gm_rank(void)
{
    uint32_t pct = gm.n ? gm.hit * 100u / gm.n : 0u;
    return pct >= 95u ? "S" : pct >= 85u ? "A" : pct >= 70u ? "B" : pct >= 50u ? "C" : "D";
}
static UI_SMALL void quest_games_draw(void)
{
    static const char *const KN[2] = {"Piano", "FM Hero"};
    char b[28];
    uint32_t i;
    ps_c c;
    static uint8_t last_ph = 9;
    q_tick_clocks();
    q_enter(QSN_GAME);
    if (last_ph != gm.ph) {                                                             /* a new phase: a clean screen, everything drawn again */
        last_ph = gm.ph;
        lcd_fill(0, 19, 240, 221, Q_BG);
        for (i = 0; i < 24u; i++) qs.sig[i] = 0;
        qb.fresh = 1;
        gm.last_ms = 0;
    }
    if (gm.ph != 0 && q_changed(15, 1u + gm.ph * 7u + gm.kind * 31u + gm.sel * 101u + q_msg_sig())) {
        cv_begin(240, 19, Q_BG);
        q_title(6, KN[gm.kind & 1u], Q_HI);
        q_text(6 + text_w(&FONT_QT, KN[gm.kind & 1u]) + 8, 4, gm.ph == 0 ? "choose" : gm.ph == 1 ? "song" : gm.ph == 2 ? "play" : "result", Q_DIM);
        if (gm.ph == 2) q_text_r(236, 4, "knob 1: rewind", Q_DIM);
        q_bar_end();
    }
    if (gm.ph == 0) {                                                                   /* which game: its cover fills the screen, arrows to the other one */
        static uint8_t cov_kind = 255, cov_from = 255;
        static uint32_t cov_t0;
        uint32_t flipping;
        if (cov_kind != gm.kind) {                                                      /* the other game: the cover turns over like a card */
            if (cov_kind != 255u) cov_from = cov_kind, cov_t0 = fm1_ms;
            cov_kind = gm.kind;
        }
        flipping = cov_from != 255u && fm1_ms - cov_t0 < 420u;
        if (cov_from != 255u && !flipping) cov_from = 255, qs.sig[0] = 0;               /* done: the still cover and its arrows */
        if (flipping || q_changed(0, 41u + gm.kind)) {
            uint8_t *const cb = &cd_tex[0][0];                           /* (the pool buffer the CDs use: 3600 of its 4096 B) */
            const uint32_t a = flipping ? (fm1_ms - cov_t0) * 32u / 420u : 0u, face = flipping && a < 16u ? cov_from : gm.kind;
            const int32_t cw = flipping ? (qcos((int32_t)a) < 0 ? -qcos((int32_t)a) : qcos((int32_t)a)) * 240 / 127 : 240, x0 = 120 - cw / 2;
            const uint8_t *z = face ? QCOVER_FM_0 : QCOVER_PIANO_0, *ze = z + (face ? sizeof QCOVER_FM_0 : sizeof QCOVER_PIANO_0);
            uint32_t half, x, y;
            for (half = 0; half < 2u; half++) {
                if (half) z = face ? QCOVER_FM_1 : QCOVER_PIANO_1, ze = z + (face ? sizeof QCOVER_FM_1 : sizeof QCOVER_PIANO_1);
                lz4_unpack(z, ze, cb);
                cv_begin(240, 120, QP(0));
                for (y = 0; y < 120u; y++)
                    for (x = 0; x < 240u; x++) {
                        int32_t sx = (int32_t)x;
                        if (flipping) {
                            if ((int32_t)x < x0 || (int32_t)x >= x0 + cw || !cw) continue;
                            sx = ((int32_t)x - x0) * 240 / cw;
                        }
                        if ((cb[y * 30u + (uint32_t)sx / 8u] >> (7u - ((uint32_t)sx & 7u))) & 1u) cv_pset((int32_t)x, (int32_t)y, QP(7));
                    }
                if (half == 0) cv_blit(0, 0); else {
                    if (!flipping) {
                        int32_t ay = 20, k;                                             /* the arrows: the other game is one turn of PRESETS away, black edged */
                        static const int16_t AR[6][4] = {{6, 0, 3, 11}, {9, 2, 3, 7}, {12, 4, 3, 3}, {231, 0, 3, 11}, {228, 2, 3, 7}, {225, 4, 3, 3}};
                        for (k = 0; k < 6; k++) cv_rect(AR[k][0] - 1, ay + AR[k][1] - 1, AR[k][2] + 2, AR[k][3] + 2, RGB(0, 0, 0));
                        for (k = 0; k < 6; k++) cv_rect(AR[k][0], ay + AR[k][1], AR[k][2], AR[k][3], RGB(0xff, 0xff, 0xff));
                    }
                    cv_blit(0, 120);
                }
            }
        }
    } else if (gm.ph == 1) {                                                            /* which song: the tapes float by, the strip rolls */
        static uint32_t roll_ms;
        uint32_t sid = (uint32_t)(gm.kind & 1u) * SNG_PER + gm.sel, ok = gm.sel < SNG_PER && sng_ok[sid];
        if (fm1_ms - roll_ms >= 24u) {
            int32_t dc = gm_tgt - gm_cur;
            roll_ms = fm1_ms;
            if (dc) gm_cur += dc * 2 / 5 ? dc * 2 / 5 : dc > 0 ? 1 : -1;               /* fast start, soft end */
            else if (gm_cur < 0 || gm_cur >= (int32_t)(GM_TAPES * 256)) gm_tgt = gm_cur = (int32_t)gm.sel * 256;
        }
        if (q_changed(0, 5u + (uint32_t)gm_cur * 3u + fm1_ms / 100u)) gm_strip();
        if (q_changed(1, 7u + gm.sel * 977u + (ok ? gm_bpm() * 31u + sng_hdr(sid)->notes * 13u : 0u) + (uint32_t)(ui.msg_t != 0) * 5u + gm.help * 4001u)) {
            char nm[14];
            int32_t x;
            cv_begin(240, 66, Q_BG);
            if (ok) imp_name(nm, sng_hdr(sid)->name, 12u); else str_cpy(nm, gm.sel >= SNG_PER ? "add file" : "empty", sizeof nm);
            x = 120 - text_w(&FONT_P, nm) / 2;
            q_text(x < 4 ? 4 : x, 6, nm, Q_TEXT);
            if (ok) {
                fmt_int(b, (int32_t)gm_bpm()), str_cpy(b + str_len(b), " bpm   ", 12), fmt_int(b + str_len(b), (int32_t)sng_hdr(sid)->notes), str_cpy(b + str_len(b), " notes", 12);
                q_text(120 - text_w(&FONT_P, b) / 2, 26, b, gm_bpmx[sid] ? Q_HI : Q_DIM);        /* the picked tempo shows lit */
            } else if (gm.help) {                                                       /* how a song gets on a tape: by USB */
                q_text(120 - text_w(&FONT_P, "songs come by USB:") / 2, 24, "songs come by USB:", Q_HI);
                q_text(120 - text_w(&FONT_P, "plug the FM-1, open fm1.quest") / 2, 38, "plug the FM-1, open fm1.quest", Q_TEXT);
                q_text(120 - text_w(&FONT_P, "and send the song to this tape") / 2, 52, "and send the song to this tape", Q_TEXT);
            } else q_text(120 - text_w(&FONT_P, "oct+: how to add a song") / 2, 26, "oct+: how to add a song", Q_DIM);
            if (!gm.help) for (i = 0; i < GM_TAPES; i++) {                                           /* where in the list: the dots */
                int32_t on = i == gm.sel;
                cv_rect(120 - (int32_t)GM_TAPES * 6 + (int32_t)i * 12 + (on ? 0 : 1), 52 + (on ? 0 : 1), on ? 7 : 5, on ? 7 : 5, on ? Q_HI : Q_PDIM);
            }
            cv_blit(0, 110);
        }
        if (q_changed(8, 3u)) { cv_begin(240, 32, Q_BG); q_text(6, 0, "presets: tape   select: tempo", Q_DIM), q_text(6, 16, "oct+ / sel / play: start  oct-: back", Q_DIM); cv_blit(0, 208); }
    } else if (gm.ph == 2) {                                                            /* playing */
        c.k = 0, c.a = c.b = c.c = c.d = 0, c.t = fm1_ms;
        if (fm1_ms - gm.last_ms >= 30u) {
            gm.last_ms = fm1_ms;
            gm_tick();
            ps_paint(gm_field, &c, 0, PS_Y0, 239, 192);
        }
        {   /* the bottom: the score, and the grade, big and bold on the right: D, then C, B, A with every 25 of the combo, S when most of it is perfect */
            static const char *const GL[5] = {"D", "C", "B", "A", "S"};
            static const uint16_t GC[5] = {RGB(0xa8, 0x90, 0x80), RGB(0xe0, 0x8a, 0x3a), RGB(0xf0, 0xc8, 0x60), RGB(0x7a, 0xb0, 0x4e), RGB(0x6a, 0xe0, 0xff)};
            uint32_t gr = gm.best >= 75u ? 3u : gm.best >= 50u ? 2u : gm.best >= 25u ? 1u : 0u, perfect = gm.hit - gm.good;
            if (gr == 3u && gm.hit >= 20u && perfect * 100u / gm.hit >= 85u) gr = 4u;
            if (q_changed(9, gm.score * 3u + gm.combo * 7u + gm.miss + gr * 977u + gm.wrong * 13u)) {
                uint32_t sx, sy;
                int32_t x0 = 176;
                cv_begin(240, 46, Q_BG);
                cv_text(0, -1, &FONT_QT, GL[gr], GC[gr]), cv_text(1, -1, &FONT_QT, GL[gr], GC[gr]);        /* bold: drawn twice, then doubled in size */
                for (sy = 0; sy < 18u; sy++)
                    for (sx = 0; sx < 24u; sx++) {
                        uint16_t px = swap16(cv_px[sy * cv_w + sx]);                    /* (the canvas keeps the colours byte-swapped for the LCD) */
                        if (px != Q_BG) cv_rect(x0 + (int32_t)sx * 2, 2 + (int32_t)sy * 2, 2, 2, px);
                    }
                cv_rect(0, 0, 36, 18, Q_BG);
                fmt_int(b, (int32_t)gm.score), q_text(6, 2, "score", Q_DIM), q_text(56, 2, b, Q_HI);
                fmt_int(b, (int32_t)gm.combo), q_text(6, 16, "combo", Q_DIM), q_text(56, 16, b, Q_TEXT);
                fmt_int(b, (int32_t)gm.miss), q_text(6, 30, "x", Q_REC), q_text(18, 30, b, Q_REC);
                cv_blit(0, 194);
            }
        }
    } else {                                                                            /* the result */
        if (q_changed(0, 500u + gm.score)) {
            cv_begin(240, 124, Q_BG);
            q_title(100, gm_rank(), Q_HI);
            fmt_int(b, (int32_t)gm.score), q_text(14, 34, "score", Q_DIM), q_text(160, 34, b, Q_HI);
            fmt_int(b, (int32_t)gm.hit), str_cpy(b + str_len(b), " / ", 4), fmt_int(b + str_len(b), (int32_t)gm.n), q_text(14, 50, "notes", Q_DIM), q_text(160, 50, b, Q_TEXT);
            fmt_int(b, (int32_t)gm.best), q_text(14, 66, "best combo", Q_DIM), q_text(160, 66, b, Q_TEXT);
            fmt_int(b, (int32_t)gm.good), q_text(14, 82, "good (late/early)", Q_DIM), q_text(160, 82, b, Q_TEXT);
            cv_blit(0, 40);
        }
        if (q_changed(1, 6u)) { cv_begin(240, 32, Q_BG); q_text(6, 0, "oct+: again", Q_DIM), q_text(6, 16, "oct-: songs", Q_DIM); cv_blit(0, 208); }
    }
    qb.fresh = 0;
}
