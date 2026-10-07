/* SPDX-License-Identifier: GPL-3.0-only
 * FM1 Quest: the PARTY screen (replaces studio_tracks_draw when FELUCCA_QUEST is 1). README of the design:
 * tracks are heroes, lit steps are attacks, BPM sets the scroll (bpm * 0.22 px/s), the key / scale sets the biome,
 * the sends set the weather, level is HP. Integer maths only (no floats), all drawing through the cv_* canvas.
 *
 * Cosmetic state only (README): the scroll offset and the idle beat clock. The attack phases are derived from the
 * transport clock (clk_beat / clk_pos) and the patterns, nothing is stored.
 *
 * Needs build/gen/felucca_qfont.h (tools/gen_quest_font.py) and felucca_qsprites.h (tools/gen_quest_sprites.py). */
#if FELUCCA_QUEST
static uint32_t str_hash(uint32_t h, const char *s);   /* ui_draw.c */
#include "felucca_qfont.h"
#include "felucca_qsprites.h"

/* ---- the TAVERN palette (README) ---- */
static const uint16_t QPAL[16] = {
    RGB(0x1a, 0x10, 0x16), RGB(0x2b, 0x23, 0x38), RGB(0x5a, 0x30, 0x50), RGB(0x2f, 0x4a, 0x3a),
    RGB(0x6a, 0x40, 0x30), RGB(0x4a, 0x3c, 0x48), RGB(0xa8, 0x9a, 0xa0), RGB(0xf2, 0xe2, 0xc4),
    RGB(0xc8, 0x44, 0x3c), RGB(0xe0, 0x8a, 0x3c), RGB(0xf0, 0xc8, 0x60), RGB(0x6a, 0x9a, 0x4a),
    RGB(0x4a, 0x78, 0xb0), RGB(0x8a, 0x6a, 0x9a), RGB(0xe0, 0x90, 0x90), RGB(0xf0, 0xb8, 0x90)};
#define Q_BG RGB(0x1e, 0x14, 0x18)
#define Q_TEXT RGB(0xf2, 0xe2, 0xc4)
#define Q_DIM RGB(0xa8, 0x90, 0x80)
#define Q_LINE RGB(0x5a, 0x3a, 0x28)
#define Q_OFF RGB(0x2e, 0x22, 0x26)
#define Q_OFFHI RGB(0x45, 0x34, 0x3a)
#define Q_HI RGB(0xf0, 0xc8, 0x60)
#define Q_REC RGB(0xe0, 0x50, 0x3c)
static const uint16_t Q_TRK[4] = {RGB(0x5a, 0x8a, 0xd0), RGB(0x7a, 0xb0, 0x4e), RGB(0xf0, 0xc8, 0x60), RGB(0xe0, 0x8a, 0x3a)};
static const uint8_t Q_HERO[4] = {QS_MAGE_IDLE_0, QS_ARCHER_IDLE_0, QS_CLERIC_IDLE_0, QS_WARRIOR_IDLE_0};   /* track -> hero (README) */
static const char *const Q_HERO_NAME[4] = {"mage", "archer", "cleric", "warrior"};

/* sin(2 pi k / 64) * 127 */
static const int8_t Q_SIN[64] = {0, 12, 25, 37, 49, 60, 71, 81, 90, 98, 106, 112, 117, 122, 125, 126, 127, 126, 125, 122, 117, 112, 106,
                                 98, 90, 81, 71, 60, 49, 37, 25, 12, 0, -12, -25, -37, -49, -60, -71, -81, -90, -98, -106, -112, -117,
                                 -122, -125, -126, -127, -126, -125, -122, -117, -112, -106, -98, -90, -81, -71, -60, -49, -37, -25, -12};
static int32_t qsin(int32_t a) { return Q_SIN[a & 63]; }
static int32_t qcos(int32_t a) { return Q_SIN[(a + 16) & 63]; }
static uint32_t qhash(uint32_t n)       /* the scene's deterministic noise (integer) */
{
    n = n * 2654435761u + 0x9E3779B9u;
    n ^= n >> 15;
    n *= 2246822519u;
    n ^= n >> 13;
    return n;
}
#define QNONE ((int32_t)0x80000000)

/* ---- cosmetic state (README: scroll offset, monster stream offset, attack phases) ---- */
static struct {
    uint32_t sp_q8;            /* scroll, 1/256 px; the monster stream rides on it */
    uint32_t idle_q8;          /* a beat clock for the idle animation (the transport is stopped) */
    uint32_t last_ms, scene_ms, bar_sig, rows_sig[4], knob_sig;
} qs;

/* ---- sprites ---- */
static void q_spr(uint32_t id, int32_t x, int32_t y)
{
    const qspr_t *s = &QSPR[id];
    const uint8_t *d = QSPR_DATA + s->off;
    int32_t gx, gy;
    for (gy = 0; gy < s->h; gy++)
        for (gx = 0; gx < s->w; gx++) {
            uint8_t v = d[gy * s->w + gx];
            if (v != 255u)
                cv_pset(x + s->ox + gx, y + s->oy + gy, QPAL[v & 15u]);
        }
}
static void q_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t dy;
    for (dy = -r; dy <= r; dy++) {
        int32_t w = 0;
        while ((w + 1) * (w + 1) + dy * dy <= r * r)
            w++;
        cv_rect(cx - w, cy + dy, 2 * w + 1, 1, c);
    }
}
#define QP(i) QPAL[(i) & 15u]

/* ---- attack phases (README): p in beats, Q8; anticipation p in [-0.5, 0), strike [0, 0.25), follow-through to 0.6 ---- */
static int32_t q_aph(const track_t *t, int32_t sf_q8)
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

/* ---- the world ---- */
#define QW 240
#define QH 84
#define QG (QH - 14)

static uint32_t q_biome(void)       /* README: major forest, minor cave, dorian coast, phrygian ruins; any other scale: forest */
{
    switch (trk[0].p[P_SCALE]) {
    case 2: return 1;               /* MIN: cave */
    case 3: return 2;               /* DOR: coast */
    case 8: return 3;               /* PHRY: ruins */
    default: return 0;
    }
}

static void q_sky(uint32_t biome, int32_t sp)
{
    static const uint8_t SKY[4][4] = {{1, 2, 8, 9}, {0, 0, 0, 0}, {12, 12, 13, 14}, {1, 2, 5, 13}};
    int32_t i, x, y;
    if (biome == 1) {                                  /* cave: dark walls, stalactites, floor */
        cv_rect(0, 0, QW, QH, QP(0));
        for (x = 0; x < QW; x++) {
            int32_t wx = (x + sp * 9 / 10) / 3, h = 4 + (int32_t)(qhash((uint32_t)wx) % 5u) + (qhash((uint32_t)wx + 3u) % 100u > 85u ? 8 : 0);
            cv_rect(x, 0, 1, h, QP(5));
        }
        for (i = -1; i < 7; i++) {                      /* pillars with torches */
            int32_t ps = sp * 35 / 100, k = ps / 44 + i, px = k * 44 - ps;
            cv_rect(px, 0, 14, QG, QP(1));
            cv_rect(px, 0, 2, QG, QP(5));
            if ((k & 1) == 0) {
                cv_rect(px + 6, QG - 30, 2, 5, QP(4));
                cv_rect(px + 6, QG - 33, 2, 3, QP(9));
                cv_rect(px + 6 + ((qs.idle_q8 >> 7) & 1), QG - 35, 1, 2, QP(10));
            }
        }
        cv_rect(0, QG, QW, QH - QG, QP(5));
        cv_rect(0, QG, QW, 1, QP(6));
        return;
    }
    for (i = 0; i < 4; i++) {                           /* the sky: four bands, dithered edges */
        int32_t y0 = i * QG / 4, y1 = (i + 1) * QG / 4;
        cv_rect(0, y0, QW, y1 - y0, QP(SKY[biome][i]));
        if (i && SKY[biome][i] != SKY[biome][i - 1])
            for (x = 0; x < QW; x += 2) {
                cv_pset(x, y0, QP(SKY[biome][i - 1]));
                if ((x & 3) == 0)
                    cv_pset(x + 1, y0 + 1, QP(SKY[biome][i - 1]));
            }
    }
    if (biome == 3)                                     /* ruins: stars */
        for (i = 0; i < 30; i++)
            cv_pset((int32_t)(qhash((uint32_t)i) % QW), (int32_t)(qhash((uint32_t)i + 50u) % (QG - 30u)), QP(qhash((uint32_t)i + 9u) % 10u > 6u ? 6 : 7));
    else {                                              /* the sunset disc and its bands */
        q_disc(172, QG - 28, 15, QP(10));
        for (i = 0; i < 4; i++)
            cv_rect(150, QG - 25 + i * 4, 46, i + 1, QP(9));
    }
    for (x = 0; x < QW; x++) {                          /* two ranges of mountains */
        int32_t wx = x + sp * 12 / 100, h = 24 + (8 * qsin(wx * 29 / 64)) / 127 + (5 * qsin(wx * 83 / 64 + 20)) / 127;
        cv_rect(x, QG - h, 1, h, QP(biome == 3 ? 5 : 2));
        wx = x + sp * 30 / 100;
        h = 11 + (5 * qsin(wx * 18 / 64 + 10)) / 127 + (3 * qsin(wx * 48 / 64)) / 127;
        cv_rect(x, QG - h, 1, h, QP(1));
    }
    if (biome == 2) {                                   /* coast: the sea */
        cv_rect(0, QG - 9, QW, 9, QP(12));
        for (x = 0; x < QW; x++)
            if (((x + (int32_t)(qs.idle_q8 >> 6) + sp / 2) / 5) & 1)
                cv_pset(x, QG - 9 + (int32_t)(qhash((uint32_t)((x + sp / 2) / 7)) % 8u), QP(7));
        cv_rect(0, QG - 9, QW, 1, QP(6));
    } else if (biome == 3) {                            /* ruins: broken pillars */
        for (i = -1; i < 8; i++) {
            int32_t ts = sp * 6 / 10, k = ts / 36 + i, px = k * 36 + (int32_t)(qhash((uint32_t)k) % 14u) - ts, hh = 12 + (int32_t)(qhash((uint32_t)k + 4u) % 20u);
            cv_rect(px - 1, QG - hh - 1, 9, hh + 1, QP(0));
            cv_rect(px, QG - hh, 7, hh, QP(6));
            cv_rect(px, QG - hh, 2, hh, QP(7));
            cv_rect(px + 5, QG - hh, 2, hh, QP(5));
            if (qhash((uint32_t)k + 8u) % 2u)
                cv_rect(px - 2, QG - hh - 3, 11, 3, QP(0)), cv_rect(px - 1, QG - hh - 2, 9, 1, QP(7));
        }
    } else {                                            /* forest: pines */
        int32_t ts = sp * 6 / 10;
        for (i = -1; i < 11; i++) {
            int32_t k = ts / 24 + i, px = k * 24 + (int32_t)(qhash((uint32_t)k) % 12u) - ts, th = 10 + (int32_t)(qhash((uint32_t)k + 9u) % 12u), j;
            cv_rect(px, QG - 3, 1, 3, QP(4));
            for (j = 0; j < th; j++) {
                int32_t w = (j % 5) * 7 / 10 + j * 22 / 100;
                cv_rect(px - w, QG - 3 - th + j, w * 2 + 1, 1, QP(3));
                cv_pset(px + w, QG - 3 - th + j, QP(11));
            }
        }
    }
    cv_rect(0, QG, QW, QH - QG, QP(biome == 2 ? 15 : biome == 3 ? 5 : 4));       /* the ground */
    cv_rect(0, QG, QW, 2, QP(biome == 2 ? 10 : biome == 3 ? 6 : 11));
    if (biome == 0)
        cv_rect(0, QG + 2, QW, 1, QP(3));
    for (x = 0; x < QW; x += 6) {
        int32_t k = (x + sp) / 6;
        uint32_t v = qhash((uint32_t)k * 3u) % 100u;
        if (v > 55 && biome != 2)
            cv_pset(x - sp % 6, QG - 1, QP(biome == 3 ? 6 : 11));
        if (v > 30 && v < 50)
            cv_rect(x - sp % 6 + 2, QG + 5 + (int32_t)(qhash((uint32_t)k + 2u) % 6u), 2, 1, QP(biome == 3 ? 4 : 0));
    }
}

typedef struct { int32_t k, mx, kind, lift, state; } q_mon_t;

/* the party and the monster stream */
static void q_scene_draw(void)
{
    static const int32_t PX[4] = {62, 36, 12, 88};       /* by track: mage, archer, cleric, warrior (README positions) */
    uint32_t biome = q_biome(), playing = song.playing, i, nm = 0;
    int32_t sp = (int32_t)(qs.sp_q8 >> 8), beat_q8, sf_q8, ph[4], tx = 0, ty = 0, have_t = 0;
    q_mon_t mon[10];
    int32_t per = 62, die = 124, k0, wx, wy = 0, wframe = -1, rain = 0, mist = 0, dst = 0, echo = 0, fxmax = 0, buff = 0;
    cv_begin(QW, QH, QP(0));
    q_sky(biome, sp);
    if (playing) {
        beat_q8 = (int32_t)(clk_beat * 256u + clk_pos / (BEAT_U >> 8));
    } else {
        beat_q8 = (int32_t)qs.idle_q8;
    }
    sf_q8 = beat_q8 * 4;
    for (i = 0; i < NTRK; i++) {                        /* the weather: the strongest send of the party (README: FX = weather) */
        int32_t v;
        v = trk[i].p[P_REV] > rain ? trk[i].p[P_REV] : rain, rain = v;
        v = trk[i].p[P_CHOR] > mist ? trk[i].p[P_CHOR] : mist, mist = v;
        v = trk[i].p[P_DLY] > echo ? trk[i].p[P_DLY] : echo, echo = v;
        v = trk[i].p[P_DIST] > dst ? trk[i].p[P_DIST] : dst, dst = v;
    }
    fxmax = rain > mist ? rain : mist;
    if (echo > fxmax) fxmax = echo;
    if (dst > fxmax) fxmax = dst;
    buff = fxmax > 10;
    /* the monsters: slime, slime, bat, 62 px apart; each dies at x = 124 (README) */
    k0 = (sp - 300) / per - 2;
    for (i = 0; i < 9u && nm < 10u; i++) {
        int32_t k = k0 + (int32_t)i, mx = k * per - sp + 300, kind = (((k % 3) + 3) % 3) == 2, bq, f, lift = 0, st = 1;
        if (mx > QW + 20 || mx < die - 20)
            continue;
        bq = beat_q8 + k * 95;                           /* hop(beat + k * 0.37) */
        f = bq & 255;
        if (f < 141) {
            lift = (4 * qsin(f * 32 / 141)) / 127;
            st = 0;
        } else if (f < 184) {
            st = 2;
        }
        mon[nm].k = k, mon[nm].mx = mx, mon[nm].kind = kind, mon[nm].lift = lift, mon[nm].state = st, nm++;
    }
    {   /* nobody attacks without a living monster in view; all aim at the nearest (README) */
        int32_t best = 0x7fffffff;
        for (i = 0; i < nm; i++)
            if (mon[i].mx >= die && mon[i].mx < QW - 16 && mon[i].mx < best) {
                best = mon[i].mx;
                have_t = 1;
                tx = mon[i].mx + 12;
                ty = mon[i].kind ? QG - 40 + 6 : QG - 8 - mon[i].lift;
            }
    }
    for (i = 0; i < 4; i++)
        ph[i] = (playing && have_t && !trk_silent(&trk[i])) ? q_aph(&trk[i], sf_q8) : QNONE;
    if (have_t && tx - 12 - (PX[3] + 16) > 44)           /* the warrior only goes out for a monster within 44 px */
        ph[3] = QNONE;
    for (i = 0; i < nm; i++) {                           /* draw the monsters */
        q_mon_t *m = &mon[i];
        if (m->mx >= die) {
            int32_t hit = have_t && m->mx + 12 == tx && (q_in(ph[3], 0, 25) || q_in(ph[0], 62, 76) || q_in(ph[1], 38, 52));
            int32_t hpv, top;
            if (m->kind) {
                int32_t by = QG - 40 + (int32_t)(qsin((int32_t)(qs.idle_q8 >> 4) + m->k * 7) * 2 / 127);
                q_spr((((beat_q8 >> 7) + m->k) & 1) ? QS_BAT_1 : QS_BAT_0, m->mx, by);
                top = by - 5;
            } else {
                uint32_t id = m->state == 0 ? QS_SLIME_AIR_0 : m->state == 1 ? QS_SLIME_REST_0 : QS_SLIME_LAND_0;
                q_spr(id + (uint32_t)hit, m->mx + 11, QG - m->lift);
                top = QG - 22 - m->lift;
            }
            hpv = (m->mx - die) * 100 / (QW - 30 - die);       /* the bar shrinks with proximity, not with damage (README) */
            hpv = hpv < 8 ? 8 : hpv > 100 ? 100 : hpv;
            cv_rect(m->mx, top, 24, 3, QP(0));
            cv_rect(m->mx + 1, top + 1, 22 * hpv / 100 > 0 ? 22 * hpv / 100 : 1, 1, QP(8));
        } else {                                          /* a poof */
            int32_t age = die - m->mx, a;
            for (a = 0; a < 10; a++)
                cv_rect(m->mx + 12 + (age * 11 / 10) * qcos(a * 6) / 127, (m->kind ? QG - 34 : QG - 12) + (age * 11 / 10) * qsin(a * 6) / 127, age < 9 ? 2 : 1, age < 9 ? 2 : 1, QP(age < 10 ? 7 : 6));
        }
    }
    /* the heroes: cleric, archer, mage, warrior, left to right */
    wx = PX[3];
    if (have_t && ph[3] != QNONE) {                      /* warrior: runs to the monster, strikes up close, jumps back */
        int32_t appr = tx - 12 - 34, p = ph[3];
        appr = appr < PX[3] ? PX[3] : appr > PX[3] + 72 ? PX[3] + 72 : appr;
        if (p < 0) {
            int32_t q = 256 + p * 2;                     /* 1 + p / 0.5 */
            wx = PX[3] + (appr - PX[3]) * (q < 0 ? 0 : q > 256 ? 256 : q) / 256;
            wframe = (int32_t)((fm1_ms / 70u) & 1u);
        } else if (p < 77) {
            wx = appr;
        } else if (p < 192) {
            int32_t q = (p - 77) * 256 / 115;            /* back to the slot in a 9 px arc */
            wx = appr + (PX[3] - appr) * q / 256;
            wy = -(9 * qsin(q / 8) / 127);
        }
    }
    for (i = 0; i < 4; i++) {
        static const uint8_t ORDER[4] = {2, 1, 0, 3};
        uint32_t t = ORDER[i], pose = Q_HERO[t], fr = playing ? ((uint32_t)(beat_q8 >> 7) + t) & 1u : 0u;
        int32_t x = PX[t], y = QG - 24, p = ph[t];
        if (t == 3) {
            x = wx;
            y += wy;
            if (wframe >= 0) fr = (uint32_t)wframe;
            pose = (p == QNONE || p >= 154) ? QS_WARRIOR_IDLE_0 : p < 0 ? QS_WARRIOR_ANTIC_0 : p < 64 ? QS_WARRIOR_STRIKE_0 : QS_WARRIOR_FOLLOW_0;
        } else if (t == 0) {
            pose = (p != QNONE && p > -102 && p < 154) ? QS_MAGE_CAST_0 : QS_MAGE_IDLE_0;
            y += -4 + (int32_t)(qsin((int32_t)(qs.idle_q8 >> 3)) * 3 / 254);
            fr = 0;
        } else if (t == 1) {
            pose = (p != QNONE && p < 0) ? QS_ARCHER_DRAW_0 : QS_ARCHER_IDLE_0;
        } else {
            pose = (p != QNONE && p > -77 && p < 256) ? QS_CLERIC_CAST_0 : QS_CLERIC_IDLE_0;
        }
        q_spr(pose + fr, x, y);
        if (t == 3 && wframe >= 0)                        /* speed lines */
            for (fr = 0; fr < 5; fr++)
                cv_rect(x - 4 - (int32_t)(qhash(fr) % 16u), y + 6 + (int32_t)fr * 3, 6 + (int32_t)(qhash(fr + 2u) % 8u), 1, QP(12));
        {   /* the attack effects */
            int32_t hx = x + 8;
            if (t == 3 && p != QNONE && p >= 0 && p < 154) {            /* crescent smear + contact star */
                int32_t prog = p < 64 ? p * 100 / 64 : 100, th = p < 64 ? 4 : 4 - (p - 64) * 4 / 90, a;
                for (a = 0; a < prog * 11 / 100; a++) {
                    int32_t ang = -14 + a * 2, rx = 8 * qcos(ang) / 127, ry = 8 * qsin(ang) / 127;
                    cv_rect(x + 10 + rx, y + 12 + ry, 1, th > 0 ? th : 1, QP(a & 1 ? 7 : 12));
                }
                if (p < 38 && have_t) {
                    cv_rect(tx - 10 - 5, ty, 11, 1, QP(7));
                    cv_rect(tx - 10, ty - 5, 1, 11, QP(7));
                }
            }
            if (t == 0 && p != QNONE && p >= 0 && p < 230 && have_t) {  /* mage: a fireball summoned above her, then it falls to the target */
                int32_t sx = x + 18 - 6, sy = y - 20 + 10 - 26 + 18;
                sy = y - 10;
                if (p < 90) {
                    int32_t r = 1 + p * 3 / 90, a;
                    for (a = 0; a < 10; a++)
                        cv_pset(sx + (r + 4) * qcos(a * 6 + p / 4) / 127, sy + (r + 4) * qsin(a * 6 + p / 4) * 7 / 1270, QP(a & 1 ? 10 : 9));
                    cv_rect(sx - r, sy - r, 2 * r + 1, 2 * r + 1, QP(0));
                    cv_rect(sx - r + 1, sy - r + 1, 2 * r - 1 > 0 ? 2 * r - 1 : 1, 2 * r - 1 > 0 ? 2 * r - 1 : 1, QP(9));
                    cv_pset(sx, sy, QP(7));
                } else if (p < 159) {
                    int32_t q = (p - 90) * 256 / 69, e = q * q / 256, mx2 = sx + (tx - sx) * e / 256, my2 = sy + (ty - sy) * e / 256, a;
                    for (a = 1; a <= 9; a++)
                        cv_rect(mx2 - a * 2, my2 - a * 2 + a / 2, 3 - a / 4 > 0 ? 3 - a / 4 : 1, 3 - a / 4 > 0 ? 3 - a / 4 : 1, QP(a < 3 ? 10 : a < 6 ? 9 : 8));
                    cv_rect(mx2 - 3, my2 - 3, 7, 7, QP(0));
                    cv_rect(mx2 - 2, my2 - 2, 5, 5, QP(9));
                    cv_rect(mx2 - 1, my2 - 1, 3, 3, QP(10));
                } else {
                    int32_t q = (p - 159) * 100 / 70, a;
                    for (a = 0; a < 10; a++)
                        cv_rect(tx + (2 + q * 12 / 100) * qcos(a * 6) / 127, ty + (2 + q * 12 / 100) * qsin(a * 6) / 127, q < 50 ? 2 : 1, q < 50 ? 2 : 1, QP(a & 3 ? 9 : 10));
                }
            }
            if (t == 1 && p != QNONE && p >= 0 && p < 128 && have_t) {  /* archer: the arrow flies on a parabola (peak 22 px) */
                int32_t ax0 = x + 19, ay0 = y + 12;
                if (p < 102) {
                    int32_t q = p * 256 / 102, x1 = ax0 + (tx - ax0) * q / 256, y1 = ay0 + (ty - ay0) * q / 256 - 22 * qsin(q / 8) / 127, a;
                    for (a = 0; a < 7; a++)
                        cv_pset(x1 - a, y1, QP(7));
                    cv_pset(x1 + 1, y1, QP(7));
                } else {
                    cv_rect(tx - 1, ty - 1, 3, 3, QP(7));
                }
            }
            if (t == 2 && buff && p != QNONE && p >= 0 && p < 256) {    /* cleric: aura rings and up-arrows on every hero */
                uint32_t col = rain >= mist && rain >= echo && rain >= dst ? 11 : mist >= echo && mist >= dst ? 12 : dst >= echo ? 8 : 9, h2;
                for (h2 = 0; h2 < 4; h2++) {
                    int32_t cx = PX[h2] + 8, r = 6 + p * 8 / 256, a, ay = QG - 30 - p * 8 / 256;
                    for (a = 0; a < 21; a++)
                        if ((a + p / 25) % 2 == 0)
                            cv_pset(cx + r * qcos(a * 3) / 127, QG + r * qsin(a * 3) * 3 / 1270, QP(col));
                    cv_rect(cx, ay, 1, 4, QP(col));
                    cv_rect(cx - 1, ay + 1, 3, 1, QP(col));
                }
            }
            (void)hx;
        }
    }
    for (i = 0; i < 4; i++) {                            /* HP bars under the heroes (the track's level) */
        static const int32_t HX[4] = {62, 36, 12, 88};
        int32_t lvl = i == TRK_DRUM ? song.g[G_DRLVL] : trk[i].p[P_LEVEL];
        cv_rect(HX[i] + 2, QG + 4, 13, 3, QP(0));
        cv_rect(HX[i] + 3, QG + 5, 11 * lvl / 127, 1, QP(11));
    }
    if (mist > 10) {                                     /* mist: a dithered band */
        int32_t x, y;
        for (y = QG - 24; y < QG - 6; y++)
            for (x = (y & 1); x < QW; x += 2)
                if ((qhash((uint32_t)((x + (int32_t)(qs.idle_q8 >> 5)) / 5) * 13u + (uint32_t)y) % 100u) < (uint32_t)(mist * 60 / 127))
                    cv_pset(x, y, QP(6));
    }
    if (rain > 10) {                                     /* rain */
        int32_t n = 70 * rain / 127, d;
        for (d = 0; d < n; d++) {
            int32_t y = (int32_t)((qhash((uint32_t)d + 300u) % QH + (qs.idle_q8 >> 1) * 3u / 2u) % QH), x = (int32_t)(((qhash((uint32_t)d) % QW) + QW * 4 - y * 3 / 10) % QW);
            cv_rect(x, y, 1, 3, QP(12));
        }
    }
    cv_blit(0, 19);
}

/* ---- HUD pieces ---- */
static void q_tri(int32_t x, int32_t y, uint16_t c)
{
    cv_rect(x, y + 2, 1, 2, c), cv_rect(x + 1, y + 1, 1, 4, c), cv_rect(x + 2, y, 1, 6, c);
    cv_rect(x + 3, y, 1, 6, c), cv_rect(x + 4, y + 1, 1, 4, c), cv_rect(x + 5, y + 2, 1, 2, c);
}
static void q_text(int32_t x, int32_t y, const char *s, uint16_t c) { cv_text(x, y, &FONT_Q, s, c); }
static void q_text_r(int32_t xr, int32_t y, const char *s, uint16_t c) { cv_text(xr - text_w(&FONT_Q, s), y, &FONT_Q, s, c); }
static void q_text_c(int32_t cx, int32_t y, const char *s, uint16_t c) { cv_text(cx - text_w(&FONT_Q, s) / 2, y, &FONT_Q, s, c); }

static void q_bar_draw(void)        /* status bar: bpm, transport, bar.beat, the four beats; title "Party" */
{
    char b[16];
    uint32_t playing = song.playing, beat = clk_beat, k;
    uint32_t sig = studio_hash((uint32_t)song.g[G_BPM] * 7u + playing * 3u + (playing ? (beat % 4u) * 131u + (beat / 4u) * 7919u : 0u) +
                               (song.rec != 0) * 1999u + (ui.msg_t ? str_hash(7u, ui.msg) : 0u) + (ui.bpm_t != 0) * 31u, "party");
    if (!ui.force && sig == qs.bar_sig)
        return;
    qs.bar_sig = sig;
    cv_begin(240, 19, Q_BG);
    if (ui.msg_t) {                                      /* a message takes the bar (as in the firmware's head) */
        q_text(6, 4, ui.msg, Q_HI);
    } else {
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
            cv_rect(98 + (int32_t)k * 4, 7, 3, 3, playing && beat % 4u == k ? (song.rec ? Q_REC : Q_TEXT) : Q_OFF);
        if (song.rec)
            q_text(122, 4, "rec", Q_REC);
        cv_text(236 - text_w(&FONT_QT, "Party"), -1, &FONT_QT, "Party", Q_HI);
    }
    cv_rect(0, 18, 240, 1, Q_LINE);
    cv_blit(0, 0);
}

static void q_portrait(int32_t x, int32_t y, uint32_t t, uint16_t border, int dead)
{
    cv_rect(x, y, 17, 17, border);
    cv_rect(x + 1, y + 1, 15, 15, Q_OFFHI);
    q_spr(Q_HERO[t], x + 1 - 1, y + 1 + (t == 0 ? 6 : 3));      /* the design: figure at (-1, 3), the mage at (-1, 6) */
    if (dead)                                                    /* a resting / silent hero: dimmed */
        for (int32_t gy = 1; gy < 16; gy++)
            for (int32_t gx = 1 + (gy & 1); gx < 16; gx += 2)
                cv_pset(x + gx, y + gy, Q_BG);
}

static void q_row_draw(uint32_t i)
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
    if (!ui.force && h == qs.rows_sig[i])
        return;
    qs.rows_sig[i] = h;
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
    fmt_int(b + 0, (int32_t)(level * 100u / 127u));
    {
        char hp[16];
        str_cpy(hp, "hp ", sizeof hp);
        str_cpy(hp + 3, b, sizeof hp - 3);
        q_text_r(234, -1, hp, Q_DIM);
    }
    cv_rect(198, 12, 36, 4, Q_OFF);
    cv_rect(198, 12, (int32_t)(level * 36u / 127u), 4, Q_TRK[i]);
    cv_blit(0, 108 + (int32_t)i * 18);
}

/* a knob of the design: 20 x 20, track ring (-135..+135 deg) filled in the knob's colour, cap, 5 px pointer, no rim */
static uint8_t q_ring[20 * 20];                          /* 255 = not ring, else the angle of the pixel from -135 deg (0..270, /2) */
static uint8_t q_ring_ready;
static int32_t q_atan(int32_t y, int32_t x)              /* degrees 0..359, integer */
{
    int32_t ax = x < 0 ? -x : x, ay = y < 0 ? -y : y, a;
    if (ax == 0 && ay == 0) return 0;
    a = ax >= ay ? ay * 45 / ax : 90 - ax * 45 / ay;
    if (x < 0) a = 180 - a;
    if (y < 0) a = 360 - a;
    return a % 360;
}
static void q_knob(int32_t cx, int32_t cy, int32_t ratio, uint16_t col)
{
    int32_t x, y, a1;
    if (ratio < 0) ratio = 0;
    if (ratio > 1000) ratio = 1000;
    if (!q_ring_ready) {
        for (y = 0; y < 20; y++)
            for (x = 0; x < 20; x++) {
                int32_t X = 2 * x - 19, Y = 2 * y - 19, r2 = X * X + Y * Y, deg = (q_atan(X, -Y) + 180 + 360) % 360, from;   /* 0 = up, clockwise */
                deg = (q_atan(X, -Y));                   /* angle from up, clockwise: atan2(x, -y) */
                from = deg > 180 ? deg - 360 : deg;      /* -180..180 */
                q_ring[y * 20 + x] = (r2 >= 153 && r2 < 296 && from >= -135 && from <= 135) ? (uint8_t)((from + 135) / 2) : 255;
            }
        q_ring_ready = 1;
    }
    a1 = ratio * 270 / 1000;                             /* 0..270 */
    for (y = 0; y < 20; y++)
        for (x = 0; x < 20; x++) {
            int32_t X = 2 * x - 19, Y = 2 * y - 19, r2 = X * X + Y * Y;
            uint8_t g = q_ring[y * 20 + x];
            if (g != 255)
                cv_pset(cx - 10 + x, cy - 10 + y, (int32_t)g * 2 <= a1 ? col : Q_OFF);
            else if (r2 < 108)
                cv_pset(cx - 10 + x, cy - 10 + y, r2 >= 71 ? RGB(0x8a, 0x5a, 0x38) : RGB(0x6a, 0x40, 0x30));
        }
    {   /* the pointer: 5 px from the centre */
        int32_t a = (a1 - 135) * 64 / 360, s;
        for (s = 0; s <= 4; s++)
            cv_pset(cx - 1 + (s * qsin(a) + 64) / 127 + 1, cy - 1 - (s * qcos(a) + 64) / 127 + 1, Q_TEXT);
    }
}

static void q_knobs_draw(void)      /* swing, level, steps, pan of the selected track (README Party) */
{
    track_t *t = TSEL;
    static char v[4][8];
    static const char *const lab[4] = {"swing", "level", "steps", "pan"};
    int32_t ratio[4];
    uint32_t k, lvl = is_drum(t) ? (uint32_t)song.g[G_DRLVL] : (uint32_t)t->p[P_LEVEL], sig;
    swing_str(v[0], song.g[G_SWING]);
    fmt_int(v[1], t->p[P_MUTE] ? 0 : (int32_t)lvl * 100 / 127);
    fmt_int(v[2], t->p[P_SLEN]);
    fmt_int(v[3], t->p[P_PAN]);
    ratio[0] = song.g[G_SWING] * 10;
    ratio[1] = t->p[P_MUTE] ? 0 : (int32_t)lvl * 1000 / 127;
    ratio[2] = (t->p[P_SLEN] - 1) * 1000 / 63;
    ratio[3] = (t->p[P_PAN] + 64) * 1000 / 127;
    sig = studio_hash((uint32_t)(ratio[0] * 3 + ratio[1] * 5 + ratio[2] * 7 + ratio[3] * 11) + song.sel * 17u + (ui.msg_t != 0) * 1013u, v[0]);
    sig = studio_hash(sig, v[1]), sig = studio_hash(sig, v[2]), sig = studio_hash(sig, v[3]);
    if (!ui.force && sig == qs.knob_sig)
        return;
    qs.knob_sig = sig;
    cv_begin(240, 44, Q_BG);
    for (k = 0; k < 4u; k++) {
        int32_t cx = 30 + 60 * (int32_t)k;
        q_knob(cx, 10, ratio[k], Q_TRK[k]);
        q_text_c(cx, 21, lab[k], Q_DIM);
        q_text_c(cx, 32, v[k], ui.hot_t && ui.hot_col == k ? Q_HI : Q_TEXT);
    }
    cv_blit(0, 194);
}

static void quest_tracks_draw(void)
{
    uint32_t i, now = fm1_ms, dt, period;
    if (ui.force) {
        lcd_fill(0, 0, 240, 240, Q_BG);
        lcd_fill(0, 103, 240, 1, Q_LINE);
    }
    /* cosmetic clocks: the scroll runs only while the transport plays (bpm * 0.22 px/s); the idle beat always */
    dt = qs.last_ms ? now - qs.last_ms : 0u;
    qs.last_ms = now;
    if (dt > 250u) dt = 250u;
    if (song.playing)
        qs.sp_q8 += ((uint32_t)song.g[G_BPM] * dt * 231u) >> 12;                        /* bpm * 0.22 px/s in 1/256 px */
    qs.idle_q8 += (uint32_t)song.g[G_BPM] * dt * 256u / 60000u;
    q_bar_draw();
    for (i = 0; i < NTRK; i++)
        q_row_draw(i);
    q_knobs_draw();
    /* the scene: ~16 fps while the transport plays, a slow ambient tick otherwise (one strip = 240 x 84 x 2 B on the LCD's SPI) */
    period = song.playing ? 62u : 250u;
    if (ui.force || now - qs.scene_ms >= period) {
        qs.scene_ms = now;
        q_scene_draw();
    }
}
#endif
