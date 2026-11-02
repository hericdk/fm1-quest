/* SPDX-License-Identifier: GPL-3.0-only
 * FM1 Quest 0.3, part 6: the FLIGHT, a home (HOME tapped again on PARTY cycles village -> flight -> party -> boss). A real (small) 3D: the boss stands in the middle
 * of an arena of blocks, the party's craft flies round him on an arc of up to 180 degrees, and the camera follows over its shoulder, always looking at the boss:
 * a perspective floor, buildings drawn as boxes, the boss, the hexagon shields, everything projected with a pinhole (F3) and drawn far to near, in a few colours.
 * The music drives it all:
 *  - every note a track plays (a key or the loop) steers the craft (the note's place in its octave picks the side, the pitch the height) and fires a homing laser
 *    of the track's colour; the lasers leave in the craft's own direction, so when it flies fast they curve out wide and come back to the boss;
 *  - on the beats of the loop the boss strikes: orange homing missiles, stones, trees and pieces of buildings thrown at the craft, a laser beam (it warns first);
 *  - hold a key and a honeycomb shield of hexagons rises in front of the craft; what hits the craft unshielded throws it far away (the camera lags behind, so you see it go).
 * The boss is one of the monsters we have (slime, wyvern, goblins, the Bone King) as a flat figure facing us, scaled by the distance. The craft is a cube for now (the 2D figure will take its place). The old dungeon's monsters stay as sprites: the bat and the skeleton (and the Bone King). */
#define FL_Y0 PS_Y0                                        /* the first screen row of the picture */
#define FL_H 153
#define F3 150                                             /* the pinhole: focal length in px */
#define HY (FL_Y0 + 90)                                    /* the horizon row */
#define FL_NL 8
#define FL_NM 4
#define FL_NO 4
#define FL_NS 20
#define FL_NT 4                                            /* the castle towers: the first four buildings */
#define FL_NB 34                                           /* the village round the boss: small houses and castle towers */
#define FL_TURN_MS 5500u                                   /* this long without a note: she turns to the camera */
static struct {
    uint32_t last_ms, now, next_atk, hold_ms, hit_fl, boss_fl, beat_seen, calm_ms, dead_ms, trl_ms;
    int32_t a, a_tgt, alt, alt_tgt, R, hp, php, shield, lvl;                    /* the craft: angle round the boss (mrad), height, radius */
    int32_t cax, cay, caz;                                                      /* the craft in the world (units) */
    int32_t kox, koy, koz, kvx, kvy, kvz, kt;                                   /* knocked away: the offset, its speed, the time left */
    int32_t camphi, camx, camy, camz, fx, fz, rx, rz;                           /* the camera: it lags the craft; its basis, Q7 */
    int32_t twr;                                                           /* the towers' height, % (60..140): the selected track's REVERB send */
    int32_t pvx, pvz, dphi, near;                                               /* dphi: the angle's speed (mrad/s); near: 0..256, she has turned to the camera and it came close */
    uint8_t shield_on, last_trk, atk_n;
    uint32_t hit_ms[4];
} fl = {.twr = 100, .hp = 1000, .php = 5, .shield = 1000, .lvl = 1, .R = 210, .alt = 60, .alt_tgt = 60};
static struct { int32_t x, y, z, vx, vy, vz; int16_t tx[4], ty[4], tz[4]; uint8_t col, life; } fll[FL_NL];     /* the craft's homing lasers (position Q4) */
static struct { int32_t x, y, z, vx, vy, vz, px, py, pz; int16_t life; } fm[FL_NM];                         /* the boss's orange missiles (units, Q4 speed) */
static struct { int32_t sx, sy, sz, tx, ty, tz, t, dur; uint8_t kind, hit; } flo[FL_NO];                    /* thrown things 0 stone, 1 tree, 2 house piece, 3 beam */
static struct { int16_t x, y; int8_t vx, vy; uint8_t t, c; } fls[FL_NS];                                    /* sparks and debris (screen) */
static int16_t trl[12][3];                                                                                  /* the white ribbon: the craft's last places */
static uint32_t boom_ms, boom_next;
static int16_t boom_x, boom_y;
/* where things are on the screen this frame (fl_think fills them, fl_scene draws) */
static int32_t sp_x, sp_y, sp_z, sb_x, sb_y, sb_z;                                                           /* the craft, the boss */
static UI_NOINLINE int32_t fl_mag(int32_t x, int32_t y)                                   /* a cheap length */
{
    int32_t ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
    return ax > ay ? ax + ay * 3 / 8 : ay + ax * 3 / 8;
}
static UI_NOINLINE int32_t fl_mag3(int32_t x, int32_t y, int32_t z) { return fl_mag(fl_mag(x, z), y); }
/* the pinhole: a world point (units) to the screen; 0 when it is behind the camera */
static UI_NOINLINE int fl_proj(int32_t X, int32_t Y, int32_t Z, int32_t *sx, int32_t *sy, int32_t *zc)
{
    int32_t rx = X - fl.camx, rz = Z - fl.camz, z = (rx * fl.fx + rz * fl.fz) / 127, x = (rx * fl.rx + rz * fl.rz) / 127;
    if (z < 12) return 0;
    *zc = z, *sx = 120 + x * F3 / z, *sy = HY - (Y - fl.camy) * F3 / z;
    return 1;
}
static UI_NOINLINE void fl_spark(int32_t x, int32_t y, uint8_t c)
{
    int32_t i, n = 0;
    for (i = 0; i < FL_NS && n < 6; i++)
        if (!fls[i].t) {
            uint32_t h = qhash((uint32_t)(x * 7 + y * 3 + i * 13) + fl.now);
            fls[i].x = (int16_t)x, fls[i].y = (int16_t)y, fls[i].vx = (int8_t)((int32_t)(h % 7u) - 3), fls[i].vy = (int8_t)((int32_t)((h >> 4) % 7u) - 5), fls[i].t = 12, fls[i].c = c, n++;
        }
}
static UI_NOINLINE void fl_boom(int32_t x, int32_t y)                                      /* the big yellow burst with its debris */
{
    int32_t i, n = 0;
    boom_ms = fl.now | 1u, boom_x = (int16_t)x, boom_y = (int16_t)y;
    for (i = 0; i < FL_NS && n < 12; i++)
        if (!fls[i].t) {
            uint32_t h = qhash((uint32_t)(x * 5 + y + i * 17) + fl.now);
            fls[i].x = (int16_t)x, fls[i].y = (int16_t)y, fls[i].vx = (int8_t)((int32_t)(h % 13u) - 6), fls[i].vy = (int8_t)((int32_t)((h >> 5) % 11u) - 8), fls[i].t = 20, fls[i].c = 5, n++;
        }
}
/* ---- the sky: dithered sunset bands down to the horizon, stars. ytop = the screen row of the picture's top, hy = its horizon row ---- */
static UI_NOINLINE void fl_sky(int32_t ytop, int32_t hy, int32_t spin)
{
    static const uint8_t BY[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
    static const uint16_t SKY[5] = {RGB(0x14, 0x0c, 0x2a), RGB(0x2a, 0x18, 0x4a), RGB(0x5a, 0x2a, 0x6a), RGB(0xa8, 0x46, 0x6a), RGB(0xe8, 0x84, 0x58)};
    int32_t x, y;
    for (y = 0; y < (int32_t)cv_h; y++) {
        int32_t sy = y - cv_oy, r = sy - ytop;
        uint16_t *row = &cv_px[(uint32_t)y * cv_w];
        if (r < 0 || sy > hy) continue;
        for (x = 0; x < (int32_t)cv_w; x++) {
            int32_t sx = x - cv_ox, band = r * 64 / (hy - ytop + 1) * 4 / 4, k = band >> 4, f = band & 15;
            uint16_t c;
            if (k > 3) k = 3, f = 15;
            c = f > (int32_t)BY[((uint32_t)sy & 3u) * 4u + ((uint32_t)sx & 3u)] ? SKY[k + 1] : SKY[k];
            if (r < 62 && (qhash((uint32_t)(sx + spin / 9 + 1000) * 131u + (uint32_t)r * 7u) % 211u) == 0u) c = RGB(0xf2, 0xe2, 0xc4);   /* a star */
            row[x] = swap16(c);
        }
    }
}
/* ---- the floor: a perspective grid, cast row by row (the world position of every pixel is walked, not projected), fogged toward the horizon ---- */
static UI_NOINLINE void fl_floor(void)
{
    static const uint8_t BY[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
    int32_t x, y;
    for (y = 0; y < (int32_t)cv_h; y++) {
        int32_t sy = y - cv_oy, dy = sy - HY;
        uint16_t *row = &cv_px[(uint32_t)y * cv_w];
        int32_t zc, ux, dX, dZ, X, Z, fog;
        if (dy <= 0 || sy >= FL_Y0 + FL_H) continue;
        zc = F3 * fl.camy / dy;
        ux = zc * 256 / F3;                                                            /* Q8 units for one pixel across */
        dX = fl.rx * ux / 127, dZ = fl.rz * ux / 127;
        X = (fl.camx + fl.fx * zc / 127) * 256 - 120 * dX, Z = (fl.camz + fl.fz * zc / 127) * 256 - 120 * dZ;
        fog = zc * 16 / 900;
        X += dX * (-cv_ox), Z += dZ * (-cv_ox);
        for (x = 0; x < (int32_t)cv_w; x++, X += dX, Z += dZ) {
            int32_t xu = X >> 8, zu = Z >> 8;
            uint16_t c;
            if ((xu & 63) < 2 || (zu & 63) < 2) c = RGB(0x8a, 0x62, 0xb0);               /* the grid lines */
            else c = (((xu >> 6) ^ (zu >> 6)) & 1) ? RGB(0x38, 0x24, 0x50) : RGB(0x2c, 0x1a, 0x44);
            if (fog > (int32_t)BY[((uint32_t)sy & 3u) * 4u + ((uint32_t)(x - cv_ox) & 3u)]) c = RGB(0xa8, 0x46, 0x6a);   /* the haze at the horizon */
            row[x] = swap16(c);
        }
    }
}
/* ---- the pieces: all in 3 tones, built from rectangles, s = size in 1/256 ---- */
static UI_NOINLINE void fl_stone(int32_t x, int32_t y, int32_t s)
{
    int32_t r = 3 + s * 12 / 256;
    p_ell(x, y, r, r - r / 5, RGB(0x6a, 0x5e, 0x70));
    p_ell(x - r / 4, y - r / 4, r * 6 / 10, r * 5 / 10, RGB(0x9a, 0x8e, 0x9e));
    p_R(x + r / 3, y + r / 4, r / 2, r / 3, RGB(0x44, 0x3a, 0x50));
}
static UI_NOINLINE void fl_tree(int32_t x, int32_t y, int32_t s)
{
    int32_t h = 6 + s * 22 / 256;
    p_R(x - h / 10, y - h / 2, h / 5 + 1, h / 2, RGB(0x6a, 0x42, 0x28));                        /* the trunk */
    p_ell(x, y - h * 6 / 10, h / 3 + 1, h / 3 + 1, RGB(0x2a, 0x6a, 0x3a));
    p_ell(x - h / 8, y - h * 7 / 10, h / 4 + 1, h / 4, RGB(0x4a, 0x9a, 0x4a));
    p_R(x - h / 2 + h / 8, y - h * 6 / 10, h / 6, h / 8 + 1, RGB(0x2a, 0x6a, 0x3a));
}
static UI_NOINLINE void fl_house(int32_t x, int32_t y, int32_t s)                               /* a piece of a house: a wall with a window and a bit of roof */
{
    int32_t w = 8 + s * 22 / 256, h = w * 7 / 10;
    p_R(x - w / 2, y - h / 2, w, h, RGB(0x9a, 0x7a, 0x62));
    p_R(x - w / 2, y - h / 2 - h / 5, w + 2, h / 4 + 1, RGB(0xb0, 0x44, 0x38));                  /* the roof, torn */
    p_R(x - w / 8, y - h / 5, w / 4 + 1, h / 3 + 1, RGB(0xf0, 0xc8, 0x60));                      /* a lit window */
    p_R(x + w / 4, y, w / 4, h / 2, RGB(0x6a, 0x4e, 0x44));
}
static UI_NOINLINE void fl_cube(int32_t cx, int32_t cy, int32_t s, uint16_t col, uint32_t t, int32_t lean)   /* the craft, a cube: edges in the track's colour, faces in grey dither */
{
    static const int8_t V[8][3] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}};
    static const uint8_t E[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    int32_t px[8], py[8], i, ang = (int32_t)(t % 3000u) * 6283 / 3000, ca = p_cosr(ang), sa = p_sinr(ang), cb = p_cosr(500 + lean), sb = p_sinr(500 + lean), r = 4 + s * 7 / 256;
    for (i = 0; i < 8; i++) {
        int32_t x = V[i][0] * ca - V[i][2] * sa, z = V[i][0] * sa + V[i][2] * ca, y = V[i][1] * 127, z2;
        z2 = (y * sb + z * cb) / 127, y = (y * cb - z * sb) / 127;
        px[i] = cx + x * r / 127 + z2 * r / 640, py[i] = cy + y * r / 127;
    }
    for (i = 0; i < 12; i++) cv_line(px[E[i][0]], py[E[i][0]], px[E[i][1]], py[E[i][1]], i < 8 ? col : ps_mix(col, RGB(0x20, 0x18, 0x30), 130));
}
/* the mage inside the cube (tools/gen_quest_maga.py: 4 poses, 8 solid colours of the Quest palette). h = her height in px: nearest-pixel scaled from the 44 rows,
 * feet at the bottom, centred on cx / cy. A glint on the staff's gem while she fires */
static UI_NOINLINE void fl_mage(int32_t cx, int32_t cy, int32_t h, uint32_t pose, int fire)
{
    const qmaga_t *m = &QMAGA[pose % QMG_N];
    const uint8_t *d = &QMAGA_DATA[m->off], *mk = d + ((uint32_t)m->w * m->h + 1u) / 2u;
    int32_t ow = (int32_t)m->w * h / 44, oh = (int32_t)m->h * h / 44, x, y;
    if (ow < 3 || oh < 3) return;
    for (y = 0; y < oh; y++)
        for (x = 0; x < ow; x++) {
            uint32_t n = (uint32_t)(y * m->h / oh) * m->w + (uint32_t)(x * m->w / ow), v = (d[n >> 1] >> ((n & 1u) ? 0 : 4)) & 15u;
            if ((mk[n >> 3] >> (7u - (n & 7u))) & 1u) cv_pset(cx - ow / 2 + x, cy - oh / 2 + y, QP(v));
        }
    if (fire) {
        int32_t gx = cx - ow / 2 + ow * 83 / 100, gy = cy - oh / 2 + oh * 14 / 100, gs = 1 + h / 30;
        p_R(gx - gs, gy - gs, gs * 2 + 1, gs * 2 + 1, RGB(0xff, 0xff, 0xff));
    }
}
/* ---- the monsters of the old dungeon, kept as sprites ---- */
static UI_NOINLINE void dg_bat(int32_t x, int32_t y, uint32_t ph, int flash)
{
    int32_t w = (int32_t)((ph / 110u) & 1u);
    uint16_t c = flash ? RGB(0xff, 0xff, 0xff) : RGB(0x9a, 0x8e, 0xb8);
    p_R(x - 1, y, 3, 2, c);
    p_R(x - 4, y - 1 + w * 2, 3, 1, c), p_R(x + 2, y - 1 + w * 2, 3, 1, c);
    p_R(x - 5, y - 2 + w * 3, 1, 1, c), p_R(x + 5, y - 2 + w * 3, 1, 1, c);
    cv_pset(x - 1, y, RGB(0xff, 0x40, 0x30)), cv_pset(x + 1, y, RGB(0xff, 0x40, 0x30));
}
static UI_NOINLINE void dg_bone(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c, int32_t m)   /* a line, 1 px for the small ones, 2 for the boss */
{
    cv_line(x0, y0, x1, y1, c);
    if (m > 1) cv_line(x0 + 1, y0, x1 + 1, y1, c);
}
static UI_NOINLINE void dg_skel_s(int32_t x, int32_t y, uint32_t ph, int flash, int32_t m)   /* the skeleton: only lines, light grey; m = its size (1 small, 2..5 the boss) */
{
    int32_t sw = p_sinr((int32_t)(ph * 7u % 6283u)) * 5 / 127, i;
    uint16_t c = flash ? RGB(0xff, 0xff, 0xff) : RGB(0xe4, 0xe4, 0xdc), d = RGB(0x9a, 0x9a, 0x94);
    p_ell(x, y - 26 * m, 3 * m, 3 * m, c);                                             /* the skull */
    p_R(x - m, y - 24 * m, 3 * m, m, d), p_R(x - m, y - 27 * m, m, m, RGB(0x10, 0x10, 0x14)), p_R(x + m, y - 27 * m, m, m, RGB(0x10, 0x10, 0x14));
    dg_bone(x, y - 22 * m, x, y - 11 * m, c, m);                                       /* the spine */
    for (i = 0; i < 3; i++) dg_bone(x - 4 * m, y - (20 - i * 3) * m, x + 4 * m, y - (20 - i * 3) * m, d, 1);   /* the ribs */
    dg_bone(x - 2 * m, y - 11 * m, x + 2 * m, y - 11 * m, c, 1);                       /* the pelvis */
    dg_bone(x - 3 * m, y - 20 * m, x - (6 + sw / 2) * m, y - 12 * m, c, m), dg_bone(x + 3 * m, y - 20 * m, x + (6 + sw / 2) * m, y - 12 * m, c, m);   /* the arms */
    dg_bone(x - m, y - 11 * m, x + (-2 + sw) * m, y, c, m), dg_bone(x + m, y - 11 * m, x + (2 - sw) * m, y, c, m);                                   /* the legs */
}
/* the Bone King's own sky (ui_quest_boss.c) */
static UI_NOINLINE void qb_cave(void) { fl_sky(0, 156, 0); }
static UI_NOINLINE int32_t fl_isqrt(int32_t v)
{
    int32_t r = 0, b = 1 << 15;
    if (v <= 0) return 0;
    for (; b; b >>= 1) if ((r + b) * (r + b) <= v) r += b;
    return r;
}
/* ---- the buildings of the arena: a fixed scatter of boxes, from a hash (no table), drawn as their visible vertical faces, column by column ---- */
static UI_NOINLINE void fl_bld_get(int32_t i, int32_t *x, int32_t *z, int32_t *w, int32_t *d, int32_t *h, int32_t *tone)
{   /* the boss stands in a village: many small houses (half-width 5..10, 12..27 high) and four castle towers. The camera only goes round the near half of the boss
     * (z > 0), so the towers stand far behind him (z < -180), apart from each other, one tall and the others lower: never two big ones in front of the camera */
    static const int16_t TW[FL_NT][4] = {{-300, -300, 62, 6}, {-70, -390, 98, 7}, {170, -300, 70, 6}, {350, -410, 58, 6}};   /* x, z, height, half-width */
    uint32_t h1 = qhash((uint32_t)i * 977u + 13u), h2 = qhash((uint32_t)i * 313u + 7u);
    int32_t bx = (int32_t)(h1 % 700u) - 350, bz = (int32_t)((h1 >> 12) % 700u) - 350;
    if (i < FL_NT) {
        *x = TW[i][0], *z = TW[i][1], *h = TW[i][2] * fl.twr / 100, *w = *d = TW[i][3], *tone = 3;
        return;
    }
    if (bx > -110 && bx < 110 && bz > -110 && bz < 110) bx += bx < 0 ? -140 : 140;     /* none on the boss's own ground */
    *x = bx, *z = bz;
    *w = 5 + (int32_t)(h2 % 6u), *d = 5 + (int32_t)((h2 >> 6) % 6u), *h = 12 + (int32_t)((h2 >> 12) % 16u), *tone = (int32_t)((h2 >> 22) % 3u);
}
static UI_NOINLINE void fl_face(int32_t xa, int32_t za, int32_t xb, int32_t zb, int32_t hb, int32_t h, uint16_t col, uint16_t col2)
{
    int32_t ax, ay, az, bx, by, bz, ta, tb, t2, x0, x1, y0b, y1b, y0t, y1t, x;
    if (!fl_proj(xa, hb, za, &ax, &ay, &az) || !fl_proj(xb, hb, zb, &bx, &by, &bz)) return;
    if (!fl_proj(xa, h, za, &ax, &ta, &az) || !fl_proj(xb, h, zb, &bx, &tb, &bz)) return;
    (void)t2;
    if (ax == bx) return;
    if (ax < bx) x0 = ax, x1 = bx, y0b = ay, y1b = by, y0t = ta, y1t = tb;
    else x0 = bx, x1 = ax, y0b = by, y1b = ay, y0t = tb, y1t = ta;
    if (x1 - x0 > 600 || x1 < -cv_ox - 4 || x0 > 244 - cv_ox) return;
    for (x = x0 < 0 ? 0 : x0; x <= x1 && x < 240; x++) {
        int32_t u = (x - x0) * 256 / (x1 - x0 + 1), yb = y0b + (y1b - y0b) * u / 256, yt = y0t + (y1t - y0t) * u / 256;
        cv_rect(x, yt, 1, yb - yt + 1, (x & 3) == 0 ? col2 : col);
    }
    cv_rect(x0 < 0 ? 0 : x0, (y0t + y1t) / 2 - 1, (x1 < 239 ? x1 : 239) - (x0 < 0 ? 0 : x0) + 1, 1, RGB(0x9a, 0x7a, 0xc0));   /* a lit edge on the top */
}
static UI_NOINLINE void fl_wall(int32_t xa, int32_t za, int32_t xb, int32_t zb, int32_t h, uint16_t c1, uint16_t c2, int tower)
{
    int32_t q;
    fl_face(xa, za, xb, zb, 0, h, c1, c2);
    if (tower)                                                                         /* the battlement: five parts of the top, the 1st, 3rd and 5th go up (merlons) */
        for (q = 0; q < 5; q += 2) fl_face(xa + (xb - xa) * q / 5, za + (zb - za) * q / 5, xa + (xb - xa) * (q + 1) / 5, za + (zb - za) * (q + 1) / 5, h, h + 6, c1, c2);
}
static UI_NOINLINE void fl_bld_draw(int32_t i)
{
    static const uint16_t WL[4][3] = {{RGB(0x4a, 0x2e, 0x62), RGB(0x38, 0x22, 0x4c), RGB(0x2a, 0x18, 0x3a)}, {RGB(0x58, 0x38, 0x6e), RGB(0x42, 0x2a, 0x56), RGB(0x32, 0x1e, 0x44)},
                                       {RGB(0x6a, 0x3c, 0x56), RGB(0x50, 0x2c, 0x46), RGB(0x3a, 0x1e, 0x34)}, {RGB(0x7a, 0x6e, 0x86), RGB(0x5e, 0x54, 0x6c), RGB(0x46, 0x3e, 0x54)}};   /* (3: the castle stone) */
    int32_t x, z, w, d, h, t, tw = i < FL_NT;
    fl_bld_get(i, &x, &z, &w, &d, &h, &t);
    if (fl.camz < z - d) fl_wall(x - w, z - d, x + w, z - d, h, WL[t][0], WL[t][1], tw);
    else if (fl.camz > z + d) fl_wall(x + w, z + d, x - w, z + d, h, WL[t][0], WL[t][1], tw);
    if (fl.camx < x - w) fl_wall(x - w, z - d, x - w, z + d, h, WL[t][1], WL[t][2], tw);
    else if (fl.camx > x + w) fl_wall(x + w, z + d, x + w, z - d, h, WL[t][1], WL[t][2], tw);
}
/* ---- the simulation ---- */
static UI_NOINLINE void fl_laser(uint32_t trk)
{
    int32_t i;
    for (i = 0; i < FL_NL; i++)
        if (!fll[i].life) {
            int32_t k;
            uint32_t h = qhash(fl.now + (uint32_t)i * 7u);
            fll[i].x = fl.cax * 16, fll[i].y = fl.cay * 16, fll[i].z = fl.caz * 16;
            fll[i].vx = fl.pvx * 16 * 8 / 10 + ((int32_t)(h % 121u) - 60) * 16, fll[i].vz = fl.pvz * 16 * 8 / 10 + ((int32_t)((h >> 8) % 121u) - 60) * 16;
            fll[i].vy = ((int32_t)((h >> 16) % 81u) - 20) * 16;
            fll[i].col = (uint8_t)trk, fll[i].life = 200;
            for (k = 0; k < 4; k++) fll[i].tx[k] = (int16_t)fl.cax, fll[i].ty[k] = (int16_t)fl.cay, fll[i].tz[k] = (int16_t)fl.caz;
            return;
        }
}
static UI_NOINLINE void fl_throw(uint32_t kind)
{
    int32_t i;
    for (i = 0; i < FL_NO; i++)
        if (!flo[i].dur) {
            flo[i].sx = (int32_t)(qhash(fl.now) % 41u) - 20, flo[i].sy = 80 + (int32_t)(qhash(fl.now * 3u) % 25u), flo[i].sz = (int32_t)(qhash(fl.now * 5u) % 41u) - 20;
            flo[i].tx = fl.cax + (int32_t)(qhash(fl.now * 7u) % 21u) - 10, flo[i].ty = fl.cay + (int32_t)(qhash(fl.now * 11u) % 15u) - 7, flo[i].tz = fl.caz;
            flo[i].t = 0, flo[i].kind = (uint8_t)kind, flo[i].hit = 0;
            flo[i].dur = kind == 3u ? 1500 : 1300 + (int32_t)(qhash(fl.now * 13u) % 400u);
            if (kind == 3u) flo[i].tx = fl.cax, flo[i].ty = fl.cay;
            return;
        }
}
static UI_NOINLINE void fl_missile(int32_t n)                                               /* orange homing missiles out of the boss: they climb, then turn on the craft */
{
    int32_t i, k = 0;
    for (i = 0; i < FL_NM && k < n; i++)
        if (!fm[i].life) {
            uint32_t h = qhash(fl.now * 3u + (uint32_t)i * 101u);
            fm[i].x = (int32_t)(h % 41u) - 20, fm[i].y = 95, fm[i].z = (int32_t)((h >> 6) % 41u) - 20;
            fm[i].px = fm[i].x, fm[i].py = fm[i].y, fm[i].pz = fm[i].z;
            fm[i].vx = ((int32_t)((h >> 12) % 101u) - 50) * 16, fm[i].vy = 110 * 16, fm[i].vz = ((int32_t)((h >> 18) % 101u) - 50) * 16;
            fm[i].life = 3200, k++;
        }
}
static UI_NOINLINE void fl_hurt(int32_t dx, int32_t dz)                                     /* a blow: the craft is thrown far off, spinning */
{
    if (fl.kt > 0) return;
    fl.php = fl.php > 0 ? fl.php - 1 : 0, fl.hit_fl = fl.now | 1u, fl.kt = 800;
    fl.kvx = dx * 7, fl.kvz = dz * 7, fl.kvy = 260;                                      /* away from the blow, and up */
    fl_boom(sp_x, sp_y);
}
static UI_NOINLINE void fl_think(uint32_t dt)
{
    uint32_t now = fl.now, i, vh = vis_hit, any_hit = 0;
    int32_t k, held = fm1_in.notes != 0u, dts = (int32_t)(dt > 100u ? 100u : dt), ox = fl.cax, oz = fl.caz, cs, cc, bx3, by3, bz3;
    vis_hit = 0;
    for (i = 0; i < 4u; i++)
        if ((vh >> i) & 1u) {
            int32_t nt = (int32_t)vis_note[i];
            fl.hit_ms[i] = now | 1u, fl.last_trk = (uint8_t)i, any_hit = 1;
            fl.a_tgt = (((nt * 7) % 12) - 6) * 215 + ((int32_t)i * 2 - 3) * 90;           /* the note's place in the octave (fifths apart: a zigzag) and the track pick the side, the pitch the height */
            fl.alt_tgt = 22 + ((nt < 36 ? 36 : nt > 84 ? 84 : nt) - 36) * 140 / 48;
            fl_laser(i);
            fl.calm_ms = 0;
        }
    {   /* the REVERB send (the FX layer, the hero's sends) grows the castle towers at the back: more echo, taller towers */
        int32_t rv = TSEL->p[P_DIST + 3], tt = 60 + (rv < 0 ? 0 : rv > 127 ? 127 : rv) * 80 / 127;
        fl.twr += (tt - fl.twr) * (dts > 300 ? 300 : dts) / 300;
    }
    if (!any_hit) fl.calm_ms += (uint32_t)dts;
    {   /* nothing played for a while: she looks at the boss, then turns to the camera and it comes in close, the boss behind her */
        int32_t nt = fl.calm_ms > FL_TURN_MS ? 256 : 0;
        fl.near += (nt - fl.near) * (nt ? dts : dts * 4) / 500;
        if (fl.near < 0) fl.near = 0;
        if (fl.near > 256) fl.near = 256;
    }
    if (fl.calm_ms > 1600u) {                                                          /* nothing played for a while: a slow weave, so the craft is never still */
        fl.a_tgt = p_sinr((int32_t)(now % 5200u) * 1208 / 1000) * (1100 - fl.near * 1100 / 256) / 127;      /* (turned to the camera: she hovers, close and centred) */
        fl.alt_tgt = 70 + p_sinr((int32_t)(now % 3100u) * 2027 / 1000) * (45 - fl.near * 30 / 256) / 127;
    }
    {   /* the flight: the angle (up to 180 degrees round the boss) chases its target fast, the height and the radius slower */
        int32_t da = fl.a_tgt - fl.a, st = dts * 3, ao = fl.a;
        fl.a += da > st ? st : da < -st ? -st : da;
        if (fl.a < -1500) fl.a = -1500;
        if (fl.a > 1500) fl.a = 1500;
        fl.dphi = dts ? (fl.a - ao) * 1000 / dts : 0;
        fl.alt += (fl.alt_tgt - fl.alt) * dts / 180;
        fl.R = 210 + p_sinr((int32_t)(now % 6000u) * 1047 / 1000) * 20 / 127;
    }
    cs = p_sinr(fl.a), cc = p_cosr(fl.a);
    if (fl.kt > 0) {                                                                   /* thrown by a blow: tumbling away, then it drifts back */
        fl.kt -= dts;
        fl.kox += fl.kvx * dts / 1000, fl.koy += fl.kvy * dts / 1000, fl.koz += fl.kvz * dts / 1000;
        fl.kvx = fl.kvx * 96 / 100, fl.kvz = fl.kvz * 96 / 100, fl.kvy -= dts * 2;
    } else fl.kox = fl.kox * 90 / 100, fl.koy = fl.koy * 90 / 100, fl.koz = fl.koz * 90 / 100;
    fl.cax = fl.R * cs / 127 + fl.kox, fl.cay = fl.alt + fl.koy, fl.caz = fl.R * cc / 127 + fl.koz;
    if (fl.cay < 6) fl.cay = 6;
    fl.pvx = dts ? (fl.cax - ox) * 1000 / dts : 0, fl.pvz = dts ? (fl.caz - oz) * 1000 / dts : 0;
    {   /* the camera: behind the craft (away from the boss), a little to the side and above; it lags, so a fast craft slides on the screen */
        int32_t ccs, ccc, ctgt;
        fl.camphi += (fl.a - fl.camphi) * dts / 150;
        ccs = p_sinr(fl.camphi), ccc = p_cosr(fl.camphi);
        fl.fx = -ccs, fl.fz = -ccc, fl.rx = ccc, fl.rz = -ccs;                         /* looking at the boss; right = (-fz, fx) */
        { int32_t cd = fl.R + 52 - fl.near * 30 / 256, sd = 18 - fl.near * 8 / 256; fl.camx = cd * ccs / 127 + sd * fl.rx / 127, fl.camz = cd * ccc / 127 + sd * fl.rz / 127; }
        ctgt = fl.alt + 10 - fl.near * 9 / 256;                                  /* (turned to the camera: it sinks a little, she rises in the picture) */
        fl.camy += (ctgt - fl.camy) * dts / 130;
        if (fl.camy < 26) fl.camy = 26;
    }
    if (!fl_proj(fl.cax, fl.cay, fl.caz, &sp_x, &sp_y, &sp_z)) sp_x = 70, sp_y = HY + 24, sp_z = 52;
    if (!fl_proj(0, 0, 0, &sb_x, &sb_y, &sb_z)) sb_x = 120, sb_y = HY + 30, sb_z = 250;
    bx3 = 0, by3 = 75, bz3 = 0;
    /* the shield: a key held for a moment raises it; it drains, and fills again when let go */
    fl.hold_ms = held ? fl.hold_ms + (uint32_t)dts : 0u;
    fl.shield_on = (fl.hold_ms > 220u || fl.near > 140) && fl.shield > 0;                 /* (turned to the camera, the shield is always up: it stands in front of her for free) */
    if (fl.hold_ms > 220u) fl.shield -= dts * 1000 / 3200; else fl.shield += dts * 1000 / 4500;
    fl.shield = fl.shield < 0 ? 0 : fl.shield > 1000 ? 1000 : fl.shield;
    if (now - fl.trl_ms >= 55u) { int32_t q; fl.trl_ms = now; for (q = 11; q > 0; q--) trl[q][0] = trl[q - 1][0], trl[q][1] = trl[q - 1][1], trl[q][2] = trl[q - 1][2]; trl[0][0] = (int16_t)fl.cax, trl[0][1] = (int16_t)fl.cay, trl[0][2] = (int16_t)fl.caz; }
    /* the boss: he strikes on the beats of the loop (every 4th), or every ~2.4 s when nothing loops */
    if (fl.hp > 0) {
        int beat = song.playing && clk_beat != fl.beat_seen;
        if (song.playing) fl.beat_seen = clk_beat;
        if ((beat && (clk_beat % 4u) == 0u) || (!song.playing && now >= fl.next_atk)) {
            uint32_t h = qhash(now * 3u + (uint32_t)fl.lvl), kind = h % 6u;
            if (kind <= 1u) fl_missile(2 + (int32_t)(h >> 9) % 3);                       /* a volley of homing missiles */
            else if (kind == 2u) fl_throw(0);
            else if (kind == 3u) fl_throw(1);
            else if (kind == 4u) fl_throw(2);
            else fl_throw(3);                                                          /* the beam */
            fl.next_atk = now + 2300u;
        }
    }
    for (k = 0; k < FL_NM; k++) {                                                      /* the missiles: they turn on the craft, only so fast: a quick craft slips past */
        int32_t dx, dy, dz, m, sp = 150 * 16;
        if (!fm[k].life) continue;
        fm[k].life -= (int16_t)dts;
        if (fm[k].life <= 0) { fm[k].life = 0; continue; }
        fm[k].px = fm[k].x, fm[k].py = fm[k].y, fm[k].pz = fm[k].z;
        dx = fl.cax - fm[k].x, dy = fl.cay - fm[k].y, dz = fl.caz - fm[k].z, m = fl_mag3(dx, dy, dz) + 1;
        if (now - (uint32_t)0 > 0u) {
            int32_t kk = dts * 9 / 100;
            fm[k].vx += (dx * sp / m - fm[k].vx) * kk / 16, fm[k].vy += (dy * sp / m - fm[k].vy) * kk / 16, fm[k].vz += (dz * sp / m - fm[k].vz) * kk / 16;
        }
        fm[k].x += fm[k].vx * dts / 16000, fm[k].y += fm[k].vy * dts / 16000, fm[k].z += fm[k].vz * dts / 16000;
        if (m < (fl.shield_on ? 34 : 16)) {
            fm[k].life = 0;
            if (fl.shield_on) fl_spark(sp_x + 10, sp_y - 8, 4), fl.shield -= 140;
            else fl_hurt(-(dx > 0 ? 1 : -1) * 30, -(dz > 0 ? 1 : -1) * 30);
        }
    }
    for (k = 0; k < FL_NO; k++) {                                                      /* the thrown things and the beam */
        int32_t u;
        if (!flo[k].dur) continue;
        flo[k].t += dts;
        u = flo[k].t * 1000 / flo[k].dur;
        if (flo[k].kind == 3u) {
            if (u > 400 && u < 760 && !flo[k].hit) {                                   /* the beam hits what is on its line (in the picture: the craft near the line) */
                int32_t ex, ey, ez, bsx, bsy, bsz, dxs, dys, cx, cy, cr, len;
                if (fl_proj(0, 85, 0, &bsx, &bsy, &bsz) && fl_proj(flo[k].tx, flo[k].ty, flo[k].tz, &ex, &ey, &ez)) {
                    dxs = ex - bsx, dys = ey - bsy, cx = sp_x - bsx, cy = sp_y - bsy, cr = cx * dys - cy * dxs, len = fl_mag(dxs, dys) + 1;
                    if ((cr < 0 ? -cr : cr) / len < 12 && fl_mag3(fl.cax - flo[k].tx, fl.cay - flo[k].ty, fl.caz - flo[k].tz) < 40) {
                        flo[k].hit = 1;
                        if (fl.shield_on) fl_spark(sp_x + 10, sp_y - 8, 4), fl.shield -= 120; else fl_hurt(fl.cax > 0 ? 30 : -30, fl.caz > 0 ? 30 : -30);
                    }
                }
            }
        } else if (u >= 1000 && !flo[k].hit) {
            int32_t dx = flo[k].tx - fl.cax, dy = flo[k].ty - fl.cay, dz = flo[k].tz - fl.caz;
            flo[k].hit = 1;
            if (fl_mag3(dx, dy, dz) < 24) {
                if (fl.shield_on) fl_spark(sp_x + 10, sp_y - 8, 4), fl.shield -= 160; else fl_hurt(dx > 0 ? -30 : 30, dz > 0 ? -30 : 30);
            }
        }
        if (flo[k].t > flo[k].dur + 120) flo[k].dur = 0;
    }
    /* the craft's lasers: they steer toward the boss, turning only so fast, so a fast craft sends them wide */
    for (k = 0; k < FL_NL; k++) {
        int32_t dx, dy, dz, m, sp = 420 * 16, tdt = (int32_t)(dt > 60u ? 60u : dt), q;
        if (!fll[k].life) continue;
        fll[k].life--;
        for (q = 3; q > 0; q--) fll[k].tx[q] = fll[k].tx[q - 1], fll[k].ty[q] = fll[k].ty[q - 1], fll[k].tz[q] = fll[k].tz[q - 1];
        fll[k].tx[0] = (int16_t)(fll[k].x >> 4), fll[k].ty[0] = (int16_t)(fll[k].y >> 4), fll[k].tz[0] = (int16_t)(fll[k].z >> 4);
        dx = bx3 * 16 - fll[k].x, dy = by3 * 16 - fll[k].y, dz = bz3 * 16 - fll[k].z, m = fl_mag3(dx, dy, dz) + 1;
        fll[k].vx += (dx * (sp >> 4) / (m >> 4 ? m >> 4 : 1) - fll[k].vx) * 7 / 100 * tdt / 16;
        fll[k].vy += (dy * (sp >> 4) / (m >> 4 ? m >> 4 : 1) - fll[k].vy) * 7 / 100 * tdt / 16;
        fll[k].vz += (dz * (sp >> 4) / (m >> 4 ? m >> 4 : 1) - fll[k].vz) * 7 / 100 * tdt / 16;
        fll[k].x += fll[k].vx * tdt / 1000, fll[k].y += fll[k].vy * tdt / 1000, fll[k].z += fll[k].vz * tdt / 1000;
        if (m < 46 * 16) {                                                             /* it hits the boss */
            int32_t dmg = fl.lvl > 6 ? 4 : 9 - fl.lvl, px, py, pz;
            fll[k].life = 0, fl.hp -= dmg, fl.boss_fl = now | 1u;
            if (fl_proj(fll[k].x >> 4, fll[k].y >> 4, fll[k].z >> 4, &px, &py, &pz)) {
                fl_spark(px, py, fll[k].col);
                if ((qhash(now + (uint32_t)k) & 3u) == 0u) fl_boom(px, py);
            }
            if (fl.hp <= 0 && !fl.dead_ms) fl.dead_ms = now | 1u;
        }
    }
    for (k = 0; k < FL_NS; k++)
        if (fls[k].t) { fls[k].t--, fls[k].x = (int16_t)(fls[k].x + fls[k].vx), fls[k].y = (int16_t)(fls[k].y + fls[k].vy), fls[k].vy = (int8_t)(fls[k].vy + 1); }
    if (fl.dead_ms && now >= boom_next) boom_next = now + 280u, fl_boom(sb_x + (int32_t)(qhash(now) % 61u) - 30, sb_y - 20 - (int32_t)(qhash(now * 3u) % 61u));   /* the boss going down: a burst after burst */
    if (fl.dead_ms && now - fl.dead_ms > 2600u) fl.dead_ms = 0, fl.lvl++, fl.hp = 1000 + 40 * fl.lvl, fl.php = 5;   /* down: the next one, bigger */
    if (fl.php == 0 && now - fl.hit_fl > 900u) fl.php = 5;                              /* (a game to play along with the music: no game over, a fresh start) */
}
/* ---- the picture ---- */
static UI_NOINLINE void fl_hex(int32_t hx, int32_t hy, int32_t r, uint32_t k)
{
    int32_t a;
    if (r < 3) { p_R(hx - 1, hy - 1, 2, 2, RGB(0x70, 0xd8, 0xff)); return; }
    for (a = 0; a < 6; a++) cv_line(hx + p_cosr(a * 1047) * r / 127, hy + p_sinr(a * 1047) * r / 127, hx + p_cosr((a + 1) * 1047) * r / 127, hy + p_sinr((a + 1) * 1047) * r / 127, RGB(0x70, 0xd8, 0xff));
    p_R(hx - 1, hy - 1, 3, 3, ((fl.now / 120u + k) & 1u) ? RGB(0xc8, 0xf0, 0xff) : RGB(0x40, 0x90, 0xc0));
}
static UI_NOINLINE void qb_slime(int32_t cx, int32_t gy, int32_t sq, uint16_t body, uint16_t hi, int flash, int32_t sc);       /* ui_quest_boss.c */
static UI_NOINLINE void qb_wyvern(int32_t cx, int32_t gy, int32_t flap, int32_t jaw, int flash, int32_t sc);
static void q_goblin(int32_t cx, int32_t gy, uint32_t k, uint32_t fr, int flash, int32_t sc);
static UI_NOINLINE void fl_boss_draw(int32_t x, int32_t y, int32_t hpx, int fl_hit)           /* the boss of this round: slime, wyvern, goblin (axe, bow) or the Bone King, fixed, facing us */
{
    static const uint16_t BODY[3] = {RGB(0x4a, 0x94, 0x6a), RGB(0x3c, 0x6c, 0xc8), RGB(0x80, 0x84, 0x94)}, HI[3] = {RGB(0xa8, 0xdc, 0xa4), RGB(0x9c, 0xc8, 0xf4), RGB(0xc8, 0xcc, 0xd8)};
    int32_t kind = (fl.lvl - 1) % 5, pct = hpx * 100 / 80, hop = p_sinr((int32_t)((fl.now % 1500u) * 4188u / 1000u)) * 3 / 127;
    if (pct < 40) pct = 40;
    if (kind == 0) qb_slime(x, y, hop, BODY[(fl.lvl / 5) % 3], HI[(fl.lvl / 5) % 3], fl_hit, pct * 9 / 10);
    else if (kind == 1) qb_wyvern(x + 8 * pct / 100, y + hop / 2, qsin((int32_t)(fl.now / 90u) & 63), (int32_t)(fl.now / 200u) % 3 * 3, fl_hit, pct * 75 / 100);
    else if (kind == 2 || kind == 3) q_goblin(x, y, kind == 3 ? 0u : 1u, (fl.now / 180u) % 4u, fl_hit, q_max(3, q_min(6, pct * 4 / 100)));
    else {
        int32_t m = (hpx + 14) / 29;
        m = m < 2 ? 2 : m > 6 ? 6 : m;
        dg_skel_s(x, y, fl.now, fl_hit, m);
        {   /* the golden ring on his chest */
            int32_t pr = m * 2 + ((fl.now / 130u) & 1u), ey = y - 17 * m;
            p_ell(x, ey, pr, pr, RGB(0xf0, 0xb0, 0x30)), p_ell(x, ey, pr - 2, pr - 2, RGB(0x30, 0x14, 0x18)), p_ell(x, ey, 2, 2, RGB(0xff, 0xe0, 0x90));
        }
    }
}
static UI_NOINLINE void fl_scene(const ps_c *c)
{
    int32_t i, k, kb, ord[FL_NB], key[FL_NB], nb = 0;
    const uint32_t now = fl.now;
    (void)c;
    fl_sky(FL_Y0, HY, fl.camphi);
    fl_floor();
    for (i = 0; i < FL_NB; i++) {                                                      /* the buildings, far to near */
        int32_t x, z, w, d, h, t, zc = ((x = 0), 0);
        fl_bld_get(i, &x, &z, &w, &d, &h, &t);
        zc = ((x - fl.camx) * fl.fx + (z - fl.camz) * fl.fz) / 127;
        if (zc < 30) continue;
        for (k = nb; k > 0 && key[k - 1] < zc; k--) key[k] = key[k - 1], ord[k] = ord[k - 1];
        key[k] = zc, ord[k] = i, nb++;
    }
    for (k = 0; k < nb && key[k] >= sb_z; k++) fl_bld_draw(ord[k]);                      /* those behind the boss */
    {   /* the boss: he blinks white when hit, and goes out in pieces when beaten */
        int32_t hpx = 130 * F3 / (sb_z > 40 ? sb_z : 40), fl_hit = fl.boss_fl && now - fl.boss_fl < 90u;
        if (!fl.dead_ms || ((now - fl.dead_ms) / 110u) % 2u == 0u) fl_boss_draw(sb_x, sb_y, hpx, fl_hit);
    }
    for (; k < nb && key[k] >= sp_z; k++) fl_bld_draw(ord[k]);                          /* those between him and her: behind her */
    kb = k;
    for (k = 0; k < FL_NO; k++) {                                                      /* the beam: a warning, then fire */
        int32_t u, ax, ay, az, bx, by, bz;
        if (!flo[k].dur || flo[k].kind != 3u) continue;
        u = flo[k].t * 1000 / flo[k].dur;
        if (!fl_proj(0, 85, 0, &ax, &ay, &az) || !fl_proj(flo[k].tx, flo[k].ty, flo[k].tz, &bx, &by, &bz)) continue;
        if (u < 400) { if ((now / 90u) & 1u) cv_line(ax, ay, bx, by, RGB(0xe0, 0x30, 0x20)); }
        else if (u < 760) {
            for (i = -2; i <= 2; i++) cv_line(ax + i, ay, bx + i, by, i == 0 ? RGB(0xff, 0xff, 0xff) : (i == 1 || i == -1) ? RGB(0xff, 0xa0, 0x60) : RGB(0xe0, 0x30, 0x20));
            p_ell(ax, ay, 7, 7, RGB(0xff, 0xe0, 0x90));
        }
    }
    for (k = 0; k < FL_NO; k++) {                                                      /* the thrown things, growing as they come */
        int32_t u, X, Y, Z, px, py, pz, s;
        if (!flo[k].dur || flo[k].kind == 3u) continue;
        u = flo[k].t * 256 / flo[k].dur;
        if (u > 256) u = 256;
        X = flo[k].sx + (flo[k].tx - flo[k].sx) * u / 256, Z = flo[k].sz + (flo[k].tz - flo[k].sz) * u / 256, Y = flo[k].sy + (flo[k].ty - flo[k].sy) * u / 256 + (u * (256 - u) / 256) * 70 / 256;
        if (!fl_proj(X, Y, Z, &px, &py, &pz)) continue;
        s = 256 * 16 * F3 / pz / 14;                                                    /* a stone 16 units wide: that many px, as the sprites' size parameter */
        if (s > 300) s = 300;
        if (flo[k].kind == 0u) fl_stone(px, py, s); else if (flo[k].kind == 1u) fl_tree(px, py, s); else fl_house(px, py, s);
    }
    for (k = 0; k < FL_NM; k++) {                                                      /* the orange missiles: a dart and its flame */
        int32_t px, py, pz, qx, qy, qz, sz;
        if (!fm[k].life) continue;
        if (!fl_proj(fm[k].x, fm[k].y, fm[k].z, &px, &py, &pz) || !fl_proj(fm[k].px - fm[k].vx / 40, fm[k].py - fm[k].vy / 40, fm[k].pz - fm[k].vz / 40, &qx, &qy, &qz)) continue;
        sz = 3 + 60 / (pz / 12 + 1);
        cv_line(qx, qy, px, py, RGB(0xff, 0x70, 0x20)), cv_line(qx + 1, qy, px + 1, py, RGB(0xff, 0xb0, 0x40));
        p_R(px - sz / 2, py - sz / 2, sz, sz, RGB(0xff, 0x9a, 0x30)), p_R(px - sz / 4, py - sz / 4, sz / 2 + 1, sz / 2 + 1, RGB(0xff, 0xf0, 0xa0));
    }
    {   /* the craft's ground shadow, its ribbon, the craft, its honeycomb shield */
        int32_t q, sx, sy, sz, qx, qy, qz, rr = 8 * F3 / (sp_z > 20 ? sp_z : 20), sp = fl_mag(fl.pvx, fl.pvz);
        if (fl_proj(fl.cax, 0, fl.caz, &sx, &sy, &sz)) p_ell(sx, sy, rr, rr / 3 + 1, RGB(0x10, 0x08, 0x1c));
        for (q = 0; q < 11; q++) {
            int32_t ax, ay, az, bx, by, bz;
            if (!trl[q][0] && !trl[q][2]) continue;
            if (!fl_proj(trl[q][0], trl[q][1], trl[q][2], &ax, &ay, &az) || !fl_proj(trl[q + 1][0], trl[q + 1][1], trl[q + 1][2], &bx, &by, &bz)) continue;
            cv_line(ax, ay, bx, by, RGB(0xf4, 0xf0, 0xff));
            if (q < 6) cv_line(ax, ay + 1, bx, by + 1, RGB(0xf4, 0xf0, 0xff));
        }
        (void)qx, (void)qy, (void)qz;
        if (sp > 100)                                                                  /* speed lines */
            for (q = 0; q < 9; q++) {
                uint32_t h = qhash((uint32_t)q * 977u + now / 70u);
                int32_t lx = (int32_t)(h % 240u), ly = FL_Y0 + 10 + (int32_t)((h >> 8) % 130u), ln = 6 + sp / 20;
                cv_line(lx, ly, lx + (fl.dphi > 0 ? -ln : ln), ly, RGB(0xc8, 0xc0, 0xe0));
            }
        {
            int32_t blink = fl.hit_fl && now - fl.hit_fl < 800u && ((now / 60u) & 1u), r = rr < 5 ? 5 : rr > 18 ? 18 : rr;
            if (fl.shield_on) {                                                        /* the shield: a honeycomb of hexagons standing in front of the craft; every blow breaks a tile */
                int32_t hx, hy, hz, tn = (fl.shield + 142) / 143, t, hr;
                if (fl_proj(fl.cax + fl.fx * 34 / 127, fl.cay + 2, fl.caz + fl.fz * 34 / 127, &hx, &hy, &hz)) {
                    static const int8_t HO[7][2] = {{0, 0}, {-7, 0}, {7, 0}, {-3, -6}, {4, -6}, {-3, 6}, {4, 6}};
                    hr = 12 * F3 / hz;
                    hr = hr < 7 ? 7 : hr > 13 ? 13 : hr;
                    for (t = 0; t < 7 && t < tn; t++) {
                        int32_t tx = hx + HO[t][0] * hr / 4, ty = hy + HO[t][1] * hr / 4;
                        p_ell(tx, ty, hr - 1, hr - 1, RGB(0x1c, 0x4a, 0x6a));
                        fl_hex(tx, ty, hr, (uint32_t)t);
                    }
                }
            }
            if (!blink) {
                int32_t mh = r * 5 / 2 + fl.near * (88 - r * 5 / 2) / 256;                      /* her height: 12..45 px, 88 when she has turned to the camera */
                uint32_t pose = fl.near > 110 ? QMG_FRONT : fl.dphi > 350 ? QMG_RIGHT : fl.dphi < -350 ? QMG_LEFT : QMG_BACK;
                fl_cube(sp_x, sp_y, (r - 4) * 256 / 7, Q_TRK[fl.last_trk & 3u], now + (fl.kt > 0 ? now * 3u : 0u), fl.dphi / 10);
                fl_mage(sp_x, sp_y, mh, pose, fl.hit_ms[fl.last_trk & 3u] && now - fl.hit_ms[fl.last_trk & 3u] < 160u);
            }
        }
    }
    for (k = kb; k < nb; k++) fl_bld_draw(ord[k]);                                      /* those nearer than her: she hides behind them */
    for (k = 0; k < FL_NL; k++) {                                                      /* the craft's lasers, a trail behind each head */
        int32_t j, px, py, pz, ax, ay, az, bx, by, bz;
        if (!fll[k].life) continue;
        if (!fl_proj(fll[k].x >> 4, fll[k].y >> 4, fll[k].z >> 4, &px, &py, &pz)) continue;
        ax = px, ay = py;
        for (j = 0; j < 4; j++) {
            uint16_t lc = j < 2 ? Q_TRK[fll[k].col & 3u] : ps_mix(Q_TRK[fll[k].col & 3u], RGB(0x20, 0x14, 0x30), 130);
            if (!fl_proj(fll[k].tx[j], fll[k].ty[j], fll[k].tz[j], &bx, &by, &bz)) break;
            cv_line(ax, ay, bx, by, lc), cv_line(ax + 1, ay, bx + 1, by, lc);
            ax = bx, ay = by;
        }
        j = 2 + 90 / (pz / 8 + 1);
        p_R(px - j / 2, py - j / 2, j, j, Q_TRK[fll[k].col & 3u]), p_R(px - j / 4, py - j / 4, j / 2 + 1, j / 2 + 1, RGB(0xff, 0xff, 0xff));
    }
    if (boom_ms && now - boom_ms < 420u) {                                             /* the burst: yellow, white in the heart, orange at the rim */
        int32_t br = 4 + (int32_t)(now - boom_ms) / 14;
        p_ell(boom_x, boom_y, br, br, RGB(0xff, 0xa0, 0x30)), p_ell(boom_x, boom_y, br * 7 / 10, br * 7 / 10, RGB(0xff, 0xd8, 0x60)), p_ell(boom_x, boom_y, br / 3 + 1, br / 3 + 1, RGB(0xff, 0xff, 0xf0));
    }
    for (k = 0; k < FL_NS; k++) {                                                      /* sparks and debris */
        if (!fls[k].t) continue;
        p_R(fls[k].x, fls[k].y, fls[k].c == 5 ? 3 : 2, fls[k].c == 5 ? 3 : 2, fls[k].c < 4 ? Q_TRK[fls[k].c] : fls[k].c == 5 ? ((fls[k].t & 1u) ? RGB(0x50, 0x48, 0x58) : RGB(0xff, 0xd0, 0x50)) : RGB(0x88, 0xe0, 0xff));
    }
    {   /* the HUD: the boss's life across the top, the hearts and the shield below it */
        int32_t hw = fl.hp > 0 ? fl.hp * 180 / (1000 + 40 * fl.lvl) : 0;
        p_R(30, FL_Y0 + 3, 182, 6, RGB(0x10, 0x08, 0x1c)), p_R(31, FL_Y0 + 4, hw > 180 ? 180 : hw, 4, RGB(0xd0, 0x46, 0x3c));
        for (i = 0; i < 5; i++) p_R(6 + i * 8, FL_Y0 + 14, 6, 5, i < fl.php ? RGB(0xe0, 0x50, 0x3c) : RGB(0x3a, 0x24, 0x34));
        p_R(6, FL_Y0 + 22, 42, 3, RGB(0x10, 0x08, 0x1c)), p_R(6, FL_Y0 + 22, fl.shield * 42 / 1000, 3, RGB(0x88, 0xe0, 0xff));
        p_R(sb_x - 4, FL_Y0 + 80, 9, 1, RGB(0xff, 0xe0, 0x90)), p_R(sb_x, FL_Y0 + 76, 1, 9, RGB(0xff, 0xe0, 0x90));   /* the crosshair on the boss */
    }
}
static UI_NOINLINE void quest_flight_draw(void)
{
    ps_c c;
    uint32_t now = fm1_ms, dt = fl.last_ms ? now - fl.last_ms : 0u;
    q_tick_clocks();
    q_enter(QSN_CAVE);
    fl.last_ms = now;
    if (dt > 250u) dt = 250u;
    fl.now = now;
    fl_think(dt);
    if (q_changed(0, studio_hash((uint32_t)song.g[G_BPM] * 7u + song.playing * 3u + (song.playing ? (clk_beat % 4u) * 131u + (clk_beat / 4u) * 7919u : 0u) + q_msg_sig() +
                                 (ui.bpm_t != 0) * 31u, "fly"))) {
        cv_begin(240, 19, Q_BG);
        q_bar_bpm_pos(0);
        q_title_r(236, "Flight", Q_HI);
        q_homedots(150, 4, 1);
        q_bar_end();
    }
    tw_party_strip();
    if (tw_budget(240 * 153 * 2)) {
        c.k = 0, c.a = c.b = c.c = c.d = 0, c.t = now;
        ps_paint(fl_scene, &c, 0, PS_Y0, 239, PS_Y1 - 23);
    }
    qb.fresh = 0;
}
