/* SPDX-License-Identifier: GPL-3.0-only
 * FM1 Quest: the boss as a fight against the loop. Every note the four heroes play is a hit: it takes HP off the boss, charges that hero's ult and
 * floats a number out of him. A loop with nothing in it does nothing; a dense one hurts. The boss never dies fast (HP is a few dozen bars of a
 * busy loop), comes back stronger (+15 % a level, capped) and heals while it is quiet. Drawn from shapes only (ellipses, polygons, rectangles).
 * Included by ui_quest.c after the village (it uses the scene toolkit of ui_quest_pages.c). */

static uint32_t boss_hp = 14000, boss_lv = 1, boss_hit_ms, boss_die_ms, boss_heal_ms, boss_in_t0;      /* boss_in_t0: when this boss arrived */
static uint8_t q_ult[4];                         /* each hero's ult charge, 0..100 */
enum { FX_SPARK, FX_BALL, FX_BOLT, FX_HEAL, FX_DOME, FX_ARROW, FX_SLASH, FX_JUMP, FX_FLARE };
static uint8_t qh_fx[4];                         /* the attack each hero's last hit used */
static uint32_t q_buff_t0;                       /* the cleric's dome: attack and defence up, damage doubled for 5 s */
static uint32_t q_ult_ms[4];                     /* when an ult went off */
static uint8_t q_ult_hero = 255;                 /* the hero whose ult is playing (255: none), when it started, the earliest the next may start, a hero's lockout after its own */
static uint32_t q_ult_t0, q_ult_gate, q_ult_cd[4], q_ult_full[4];
static uint8_t q_ult_dealt, q_ult_shown;        /* q_ult_shown: bit per hero, the full-screen picture already played this loop */
static uint8_t q_ult_art_open;
static uint8_t q_ult_art = 255;                 /* the hero whose picture is on screen (255: none) and since when */
static uint32_t q_ult_art_t0;
#define QULT_MS 4000u                              /* every ult lasts this long, all its frames (it was 10 s) */
static uint32_t q_def_t0;                        /* when an ult left the boss with his guard down (8 s of more damage) */
static struct { uint32_t t0; int16_t x; uint16_t v; uint8_t hero; } q_dn[6];

static UI_NOINLINE uint32_t boss_max(void)
{
    uint32_t lv = boss_lv > 10u ? 10u : boss_lv;
    return 14000u * (100u + 15u * (lv - 1u)) / 100u;                       /* a dense loop takes about a minute to bring him down */
}
/* who is in front of the party: slimes most of the time (green, blue, grey), the Red Wyvern now and then, a goblin (axe, archer) rarely */
static UI_NOINLINE uint32_t boss_kind(void)       /* 0 slime, 1 wyvern, 2 goblin with an axe, 3 goblin archer */
{
    static const uint8_t K[10] = {0, 1, 0, 2, 0, 1, 0, 3, 0, 4};          /* 4: the Bone King, in his own dungeon (0.3) */
    return K[(boss_lv - 1u) % 10u];
}
static UI_NOINLINE uint32_t boss_slime_n(void) { return (boss_lv - 1u) >> 1; }                  /* the slimes in order: the colour is n % 3, the size n % 4 */
static UI_NOINLINE const char *boss_name(void)
{
    static const char *const SL[3] = {"King Slime", "Blue Slime", "Grey Slime"};
    uint32_t k = boss_kind(), n = boss_slime_n();
    if (k == 1u) return "Red Wyvern";
    if (k == 2u) return "Goblin Axe";
    if (k == 3u) return "Goblin Bow";
    if (k == 4u) return "Bone King";
    return SL[n % 3u];
}
/* ---- the player's own hits: keys pressed in a rhythm, a combo that grows (and a bigger hit for each 25) ---- */
static uint32_t cb_n, cb_last, cb_ref, cb_bad, cb_prevkeys, cb_tap_ms[3], cb_tier_ms;
static int16_t cb_tapx[3], cb_tapy[3];
static uint32_t q_unlock_t0;                     /* the first boss down: the banner is up since (ms), 0 = no */
static uint8_t q_unlock_eat;                     /* the HOME that closed it: its tap is eaten */
static UI_NOINLINE void q_boss_hurt(uint32_t dmg)
{
    if (q_def_t0 && fm1_ms - q_def_t0 < 8000u) dmg = dmg * 3u / 2u;                    /* the guard is down after an ult */
    if (boss_hp > dmg) boss_hp -= dmg;
    else boss_hp = 0;
    if (boss_hp == 0 && !boss_die_ms) {
        boss_die_ms = fm1_ms | 1u;
        if (!q_warrior_ok) q_warrior_ok = 1, q_unlock_t0 = fm1_ms | 1u, settings_later = 1;      /* the warrior joins: once, for the first boss */
    }
}
static UI_NOINLINE uint32_t q_pick_fx(uint32_t h)         /* the chance each hero has of each attack, the same whoever stands in the slot */
{
    uint32_t r = qhash(fm1_ms * 7u + h * 977u) % 100u;
    switch (h) {
    case 0: return r < 40u ? FX_SPARK : r < 80u ? FX_BALL : FX_BOLT;                  /* the mage: spark 40, magic ball 40, bolt 20 */
    case 2: return r < 50u ? FX_HEAL : r < 75u ? FX_BALL : r < 85u ? FX_DOME : FX_FLARE;   /* the support: heal 50, ball 25, dome 10, charge and flare 15 */
    case 1: return r < 70u ? FX_ARROW : FX_SPARK;                                     /* the archer: arrow 70, spark 30 */
    default: return r < 90u ? FX_SLASH : FX_JUMP;                                     /* the fighter: always melee: slash 90, the high jump 10 */
    }
}
static UI_NOINLINE uint32_t q_vary(uint32_t dmg)                                      /* no fixed numbers: 3 up or down, never under 1 */
{
    int32_t v = (int32_t)dmg + (int32_t)(qhash(fm1_ms * 13u + dmg * 7u) % 7u) - 3;
    return v < 1 ? 1u : (uint32_t)v;
}
static UI_NOINLINE void q_hit(uint32_t h, uint32_t dmg, uint32_t note)
{
    uint32_t i, mx = boss_max(), fx = q_pick_fx(h);
    qh_ms[h] = fm1_ms | 1u, qh_note[h] = (uint8_t)note, qh_fx[h] = (uint8_t)fx;
    if (fx == FX_DOME) q_buff_t0 = fm1_ms | 1u;
    if (fx == FX_FLARE) q_def_t0 = fm1_ms | 1u;                                        /* the cleric's debuff: the flare takes the boss's defence down for 8 s */
    if (fx == FX_HEAL) for (i = 0; i < 4u; i++) q_sp[i] = (uint8_t)(q_sp[i] + 12u > 100u ? 100u : q_sp[i] + 12u);
    if (q_buff_t0 && fm1_ms - q_buff_t0 < 5000u) dmg *= 2u;                          /* the dome: everybody hits twice as hard for 5 s */
    dmg = q_vary(dmg);
    qh_dmg[h] = (uint8_t)(dmg > 255u ? 255u : dmg);
    boss_hit_ms = fm1_ms;
    for (i = 0; i < 6u; i++)
        if (fm1_ms - q_dn[i].t0 > 700u) { q_dn[i].t0 = fm1_ms, q_dn[i].x = (int16_t)(150 + (int32_t)(qhash(fm1_ms + h) % 60u)), q_dn[i].v = (uint16_t)dmg, q_dn[i].hero = (uint8_t)h; break; }
    if (q_ult[h] < 100u && !(h == 2u && q_var[2]) && (int32_t)(fm1_ms - q_ult_cd[h]) > 6000)            /* the hits charge it (slowly again for 6 s after its own ult) */
        q_ult[h] = (uint8_t)(q_ult[h] + 3u > 100u ? 100u : q_ult[h] + 3u), q_ult_full[h] = q_ult[h] >= 100u ? (fm1_ms | 1u) : 0u;
    (void)mx;
    q_boss_hurt(dmg);
}
static UI_NOINLINE int cb_in_rhythm(uint32_t iv)    /* an interval is "in rhythm" when it matches the one before (x1, half or double), or the loop's beat grid */
{
    uint32_t r;
    if (!cb_ref) return 1;
    r = iv * 100u / cb_ref;
    if ((r >= 75u && r <= 125u) || (r >= 40u && r <= 63u) || (r >= 160u && r <= 250u)) return 1;
    if (song.playing && song.g[G_BPM] > 0) {
        uint32_t beat = 60000u / (uint32_t)song.g[G_BPM], k;
        for (k = 1; k <= 4u; k *= 2u) {
            uint32_t g = beat / k;
            if (iv * 100u / g >= 80u && iv * 100u / g <= 120u) return 1;
        }
        if (iv * 100u / (beat * 2u) >= 80u && iv * 100u / (beat * 2u) <= 120u) return 1;
    }
    return 0;
}
static UI_NOINLINE void cb_tick(void)
{
    uint32_t now = fm1_ms, nk = fm1_in.notes, born = nk & ~cb_prevkeys;
    int on_boss = !ui.menu && !ui.home && cur_page()->scope == SC_TRK && q_town == 2u;
    cb_prevkeys = nk;
    if (cb_n && now - cb_last > 4000u) cb_n = 0, cb_ref = 0, cb_bad = 0;         /* nothing for 4 s: the combo is gone */
    if (!on_boss || !born || boss_die_ms) return;
    {
        uint32_t iv = cb_last ? now - cb_last : 0u, ok = 1u, tier, dmg, i;
        if (!cb_last || iv > 4000u) cb_n = 0, cb_ref = 0, cb_bad = 0, iv = 0;
        else if (iv > 0u) {
            ok = (uint32_t)cb_in_rhythm(iv);
            if (ok) { uint32_t r = cb_ref ? iv * 100u / cb_ref : 100u; if (!cb_ref) cb_ref = iv; else if (r >= 75u && r <= 125u) cb_ref = (cb_ref * 3u + iv) / 4u; cb_bad = 0; }   /* the pace follows a steady hand */
            else if (++cb_bad >= 2u) { cb_ref = iv, cb_bad = 0; }                  /* the pace really changed: this is the new one */
        }
        if (!cb_ref && iv) cb_ref = iv;
        cb_last = now;
        if (!ok) return;                                                          /* a key off the rhythm: no hit of its own, the combo waits */
        cb_n++;
        tier = cb_n / 25u;
        dmg = 30u * (100u + 10u * (tier > 20u ? 20u : tier)) / 100u;                /* 3 x a hero's hit, +10 % for every 25 in the rhythm */
        if (q_buff_t0 && now - q_buff_t0 < 5000u) dmg *= 2u;
        dmg = q_vary(dmg);
        q_boss_hurt(dmg);
        boss_hit_ms = now;
        i = cb_n % 3u;
        cb_tap_ms[i] = now | 1u, cb_tapx[i] = (int16_t)(150 + (int32_t)(qhash(now) % 70u)), cb_tapy[i] = (int16_t)(66 + (int32_t)(qhash(now * 3u) % 40u));
        for (i = 0; i < 6u; i++)
            if (now - q_dn[i].t0 > 700u) { q_dn[i].t0 = now, q_dn[i].x = cb_tapx[cb_n % 3u], q_dn[i].v = (uint16_t)dmg, q_dn[i].hero = (uint8_t)(4u + (cb_n / 25u) % 4u); break; }       /* (the player's: 4 + the combo ring's tier, for the colour) */
        if (cb_n % 25u == 0u) cb_tier_ms = now | 1u;                              /* a full ring: the border changes */
    }
}
static UI_NOINLINE void q_boss_first(void)
{
    if (!q_boss1_seen) q_boss1_seen = 1, q_ult[0] = 50, settings_later = 1;
}
static void q_ult_art_step(void);
static UI_NOINLINE void q_ult_tick(void)         /* one ult at a time: the first hero that is full goes when the gap since the last one has passed (5 to 8 s) */
{
    uint32_t now = fm1_ms, i, best = 4u;
    q_ult_art_step();
    if (q_ult_hero == 255u) {
        if ((int32_t)(now - q_ult_gate) < 0) return;
        for (i = 0; i < 4u; i++)
            if (q_ult[i] >= 100u && (best == 4u || q_ult_full[i] < q_ult_full[best])) best = i;
        if (best < 4u && boss_hp && !boss_die_ms) {
            q_ult_hero = (uint8_t)best, q_ult_t0 = now | 1u, q_ult_dealt = 0;
            if (!q_ult_simple) q_ult_art = (uint8_t)best, q_ult_art_t0 = now | 1u, q_ult_art_open = 0;                  /* every ult: its whole animation, the whole screen */
            q_ult[best] = 0, q_ult_cd[best] = now, q_ult_ms[best] = now | 1u;
        }
    } else {
        uint32_t el;
        if (q_ult_art != 255u) { q_ult_t0 = now | 1u; return; }                                     /* the animation plays first: the hit lands after it */
        el = now - q_ult_t0;
        if (!q_ult_dealt && el >= 1100u) {                                      /* the hit lands */
            uint32_t d = q_vary(boss_max() / 22u), k;
            q_ult_dealt = 1;
            for (k = 0; k < 6u; k++)
                if (now - q_dn[k].t0 > 700u) { q_dn[k].t0 = now, q_dn[k].x = 160, q_dn[k].v = (uint16_t)(d > 65535u ? 65535u : d), q_dn[k].hero = q_ult_hero; break; }
            boss_hit_ms = now;
            q_boss_hurt(d);
            q_def_t0 = now | 1u;                                                 /* and his defence is down for a while */
        }
        if (el >= 2700u) q_ult_hero = 255, q_ult_gate = now + 1500u;   /* the next one: another hero full, 1.5 s later */
    }
}
static UI_NOINLINE void q_hits_tick(void)       /* once a frame: which steps just started, and what they cost the boss */
{
    uint32_t i;
    q_ult_tick();
    cb_tick();
    if (boss_die_ms && fm1_ms - boss_die_ms > 1400u) {       /* defeated: back, stronger */
        boss_die_ms = 0, boss_lv++, boss_hp = boss_max(), boss_in_t0 = fm1_ms;
        q_ult_shown = 0;                           /* a new loop: every hero's picture plays once more (the charge bars stay as they were) */
    }
    for (i = 0; i < 4u; i++) {
        const track_t *t = &trk[i];
        uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), idx = t->seq_idx % len, on = 0, dmg = 0, note = 60, l;
        if (idx == qh_idx[i]) continue;
        qh_idx[i] = (uint16_t)idx;
        if (!song.playing || q_rest(i) || boss_die_ms) continue;
        if (is_drum(t)) {
            uint32_t m = dstep_mask(&t->dstep[idx]);
            for (l = 0; l < DRUM_LANES; l++)
                if ((m >> l) & 1u) on = 1, dmg += l < 2u ? 10u : l == 2u || l == 8u ? 14u : 5u, note = 36u + l;
        } else if (trk_step_on(t, idx)) {
            on = 1, dmg = 6u + (t->step[idx].vel ? t->step[idx].vel : 90u) / 32u, note = t->step[idx].note[0];
        }
        if (on) q_hit(i, dmg, note);
    }
    if (!boss_die_ms && fm1_ms - boss_hit_ms > 1500u && fm1_ms - boss_heal_ms > 500u) {     /* a quiet loop: it heals */
        uint32_t mx = boss_max();
        boss_heal_ms = fm1_ms;
        boss_hp = boss_hp + mx / 300u > mx ? mx : boss_hp + mx / 300u;
    }
}

/* ---- the scene: the castle behind, the party small on the left, the slime big on the right ---- */
static UI_NOINLINE void qb_castle(void)
{
    const uint16_t sky0 = RGB(0x3a, 0x2e, 0x5a), sky1 = RGB(0x5a, 0x46, 0x78), sky2 = RGB(0x7a, 0x5a, 0x86), st = RGB(0x4a, 0x4a, 0x66), st2 = RGB(0x3a, 0x3a, 0x54), lit = RGB(0xf0, 0xc8, 0x60);
    int32_t k, x;
    p_R(0, 0, 240, 28, sky0), p_R(0, 28, 240, 24, sky1), p_R(0, 52, 240, 32, sky2);
    for (x = 0; x < 240; x += 2) cv_pset(x + (x / 2) % 2, 28, sky0), cv_pset(x + (x / 2) % 2, 52, sky1);          /* dither at the band borders */
    p_R(14, 10, 40, 6, RGB(0x6a, 0x58, 0x8a)), p_R(24, 6, 26, 6, RGB(0x6a, 0x58, 0x8a)), p_R(170, 14, 36, 5, RGB(0x6a, 0x58, 0x8a));   /* clouds */
    p_R(84, 18, 30, 56, st), p_R(58, 34, 20, 40, st), p_R(120, 34, 20, 40, st), p_R(78, 52, 46, 22, st2);         /* the keep, two towers, the wall */
    for (k = 0; k < 4; k++) p_R(84 + k * 8, 14, 5, 5, st), p_R(58 + k * 6 + (k > 2 ? 2 : 0), 30, 4, 5, st), p_R(120 + k * 6, 30, 4, 5, st);
    p_tri(98, 18, 98, 4, 110, 11, RGB(0x5a, 0x3a, 0x6a)), p_tri(64, 34, 64, 24, 72, 29, RGB(0x5a, 0x3a, 0x6a));      /* flags */
    p_R(96, 28, 4, 8, lit), p_R(66, 46, 3, 6, lit), p_R(128, 46, 3, 6, lit);                                  /* lit windows */
    p_R(0, 74, 240, 10, RGB(0x2a, 0x22, 0x3e));                                                               /* the ground */
}
static UI_NOINLINE void qb_slime(int32_t cx, int32_t gy, int32_t sq, uint16_t body, uint16_t hi, int flash, int32_t sc)   /* sc: the size in percent */
{
#define SS(v) ((v) * sc / 100)
    int32_t rx = SS(56) + sq / 2, ry = SS(44) - sq / 2;
    const uint16_t dk = RGB(0x24, 0x18, 0x40), pu = RGB(0x8a, 0x6a, 0xe0);
    p_ell(cx, gy - ry, rx, ry, flash ? RGB(0xd8, 0xf0, 0xd8) : body);
    p_ell(cx - SS(16), gy - ry - SS(14), SS(30), SS(16), flash ? RGB(0xff, 0xff, 0xff) : hi);                   /* the shine */
    p_R(cx - SS(30), gy - ry - SS(8), SS(7), SS(12), dk), p_R(cx + SS(18), gy - ry - SS(8), SS(7), SS(12), dk);
    p_R(cx - SS(28), gy - ry - SS(6), SS(3), SS(5), pu), p_R(cx + SS(20), gy - ry - SS(6), SS(3), SS(5), pu);   /* the eyes */
    p_ell(cx + SS(8), gy - ry + SS(26), SS(34), SS(9), dk), p_R(cx - SS(12), gy - ry + SS(2), SS(30), 2, dk);   /* the mouth, a flat nose line */
    p_ell(cx + SS(36), gy - ry - SS(26), SS(4), SS(4), hi), p_ell(cx - SS(44), gy - ry - SS(4), SS(3), SS(3), hi), p_ell(cx + SS(42), gy - ry + SS(8), SS(4), SS(4), hi);   /* spots */
#undef SS
}
static void q_goblin(int32_t cx, int32_t gy, uint32_t k, uint32_t fr, int flash, int32_t sc);
static UI_NOINLINE void qb_goblin(int32_t cx, int32_t gy, uint32_t kind, uint32_t fr, int flash) { q_goblin(cx, gy, kind ? 0u : 1u, fr, flash, 4); }   /* boss: 0 axe, 1 archer */
static UI_NOINLINE void qb_wyvern(int32_t cx, int32_t gy, int32_t flap, int32_t jaw, int flash, int32_t sc)   /* the Red Wyvern, from the same ellipses and triangles as the slime: facing left; sc in percent (the arrival: small and far, then big) */
{
#define WX(v) (cx + (v) * sc / 100)
#define WY(v) (gy + (v) * sc / 100)
#define WS(v) q_max(1, (v) * sc / 100)
    const uint16_t red = flash ? RGB(0xff, 0xff, 0xff) : RGB(0xc0, 0x38, 0x30), dk = RGB(0x7a, 0x20, 0x28), lt = RGB(0xe8, 0xc0, 0x70), mem = RGB(0x9a, 0x2a, 0x40), ey = RGB(0xff, 0xe0, 0x50);
    int32_t i, wt = -(flap * 22 / 127);                                                 /* the wing tips go up and down */
    p_tri(WX(18), WY(-52), WX(72 + wt / 2), WY(-112 + wt), WX(84), WY(-58), mem);        /* the far wing, then the near one */
    cv_line(WX(18), WY(-52), WX(72 + wt / 2), WY(-112 + wt), dk), cv_line(WX(18), WY(-52), WX(84), WY(-58), dk), cv_line(WX(40), WY(-54), WX(80), WY(-84 + wt / 2), dk);
    for (i = 0; i < 9; i++) p_ell(WX(40 + i * 9), WY(-28 + i * 2 - (i > 5 ? (i - 5) * 3 : 0)), WS(11 - i), WS(9 - i / 2), i & 1 ? red : dk);     /* the tail, curling up */
    p_ell(WX(6), WY(-36), WS(36), WS(25), red);                                         /* the body */
    p_ell(WX(-2), WY(-28), WS(24), WS(14), lt);                                         /* the belly */
    p_ell(WX(22), WY(-10), WS(10), WS(12), dk), p_ell(WX(-14), WY(-9), WS(9), WS(11), dk);   /* the legs */
    p_R(WX(14), WY(-2), WS(14), WS(3), dk), p_R(WX(-22), WY(-1), WS(12), WS(3), dk);    /* the feet */
    p_ell(WX(-26), WY(-52), WS(10), WS(14), red), p_ell(WX(-34), WY(-66), WS(9), WS(12), red);   /* the neck */
    p_ell(WX(-44), WY(-74), WS(15), WS(10), red);                                       /* the head */
    p_ell(WX(-59), WY(-71), WS(10), WS(5), red);                                        /* the snout */
    p_R(WX(-68), WY(-70 + jaw / 3), WS(14), WS(4 + jaw / 4), dk);                       /* the jaw: open when it breathes */
    cv_pset(WX(-66), WY(-72), lt), cv_pset(WX(-62), WY(-72), lt), cv_pset(WX(-58), WY(-72), lt);   /* teeth */
    p_R(WX(-48), WY(-78), WS(4), WS(3), ey), cv_pset(WX(-47), WY(-77), RGB(0x1a, 0x10, 0x16));   /* the eye */
    p_tri(WX(-40), WY(-82), WX(-28), WY(-100), WX(-32), WY(-80), dk), p_tri(WX(-48), WY(-82), WX(-42), WY(-98), WX(-44), WY(-80), dk);   /* the horns */
    for (i = 0; i < 5; i++) p_tri(WX(-20 + i * 11), WY(-58 + i), WX(-15 + i * 11), WY(-68 + i), WX(-10 + i * 11), WY(-58 + i), dk);        /* the spines */
#undef WX
#undef WY
#undef WS
}
#define QB_H 156                                   /* the scene's height: the squares below are thin now, so the boss gets the room */
#define QB_DY (QB_H - 120)                         /* what the 120-row layout is moved down by */
static void qult_band(void);
static UI_NOINLINE void q_boss_pass(int32_t base)               /* one half of the scene (the canvas is 124 rows): base = 0 the top, -78 the bottom */
{
    static const uint8_t ORD[4] = {2, 1, 0, 3};
    static const int16_t HX[4] = {62, 36, 12, 88};
    const uint32_t now = fm1_ms;
    int32_t bt = -1;                                                                                           /* ms into the wyvern's bite, and who it takes */
    uint32_t gf = (now / 180u) % 4u, bvic = 9u, bn = 0;                                                                      /* the goblin's frame: idle */
    int32_t i, bq = q_beat_q8(), hop = (int32_t)(qsin((bq & 255) * 32 / 256 * 2) * 3 / 127), bx = 184, GY = 112, tx, ty = 76, k, jx = 0, jy = 0, jsq = 0;
    {   /* now and then he creeps up on the party, slowly, hopping, and leaps back to his place */
        static uint32_t cyc_t, cyc_last;                                                                   /* his clock stops while an ult's animation is on: nothing happens behind the window */
        uint32_t cyc, dl = now - cyc_last;
        cyc_last = now;
        if (dl < 500u) cyc_t += dl;
        cyc = cyc_t % ((boss_kind() == 1u ? 6000u : 11000u) * (Q_OPT_BSPD() == 0u ? 140u : Q_OPT_BSPD() == 1u ? 100u : 65u) / 100u);   /* (menu BOSS SPEED: all the bosses) */              /* the wyvern comes at the party almost twice as often as the slime */
        if (boss_kind() == 1u && cyc >= 1500u && cyc < 3500u) bt = (int32_t)cyc - 1500, bn = cyc_t / 6000u, bvic = (bn * 5u + 2u) % 4u;
        if (cyc < 1500u) { int32_t u = (int32_t)(cyc * 256u / 1500u); jx = -50 * u / 256; jy = -(int32_t)((cyc % 380u) < 190u ? (cyc % 380u) / 38u : (380u - cyc % 380u) / 38u); gf = 4u + (now / 90u) % 4u; if (boss_kind() >= 2u) jy = 0; }
        else if (cyc < 2300u) { jx = -50; jsq = (cyc / 140u) % 2u ? 4 : 0; gf = 8u + (cyc - 1500u) * 8u / 800u; }
        else if (cyc < 2950u) { int32_t u = (int32_t)((cyc - 2300u) * 256u / 650u); jx = -50 * (256 - u) / 256; jy = -(int32_t)(40 * 4 * u * (256 - u) / 65536); gf = 4u + (now / 90u) % 4u; }
    }
    bx += jx;
    tx = bx - 40;
    uint32_t mx = boss_max(), flash = 0, die = boss_die_ms ? now - boss_die_ms : 0u, buff = q_buff_t0 && now - q_buff_t0 < 5000u;
    for (i = 0; i < 4; i++) if (qh_ms[i] && now - qh_ms[i] < 90u) flash = 1;
    cv_begin(QW, QB_H / 2, QP(0));
    cv_oy = base;
    if (boss_kind() == 4u) qb_cave(), cv_oy = base + QB_H - 84;                                               /* the skeleton boss: the dungeon is his place */
    else {
        p_R(0, 0, 240, 36 + QB_DY, RGB(0x3a, 0x2e, 0x5a));
        q_stars(26, 0, 40 + QB_DY, 4242u);
        cv_oy = base + QB_H - 84;
        qb_castle();
    }
    if (!boss_die_ms || (die / 120u) % 2u == 0u)                                                              /* it blinks out when defeated */
    {
        uint32_t bk = boss_kind(), sn = boss_slime_n();
        int32_t gy0 = 80 + (hop < 0 ? 0 : -hop / 2) + jy;
        if (bk == 0u) {
                        static const uint16_t BODY[3] = {RGB(0x4a, 0x94, 0x6a), RGB(0x3c, 0x6c, 0xc8), RGB(0x80, 0x84, 0x94)}, HI[3] = {RGB(0xa8, 0xdc, 0xa4), RGB(0x9c, 0xc8, 0xf4), RGB(0xc8, 0xcc, 0xd8)};
            qb_slime(bx, gy0, (flash ? -4 : 0) + jsq, BODY[sn % 3u], HI[sn % 3u], 0, 100);   /* (the little ones are in the map, not here) */   /* (no colour change when hurt) */
        } else if (bk == 1u)
        {   /* a new wyvern comes in from far away, small and growing, rises and drops to its size */
            int32_t el = boss_in_t0 ? (int32_t)(now - boss_in_t0) : 9999, wx = bx - 12, wy = 100 + (hop < 0 ? 0 : -hop / 2) + jy, wsc = 100, u;
            if (el < 1500) u = el * 256 / 1500, wsc = 25 + 70 * u / 256, wx = bx - 12 + 150 * (256 - u) / 256, wy = 70 - 8 * u / 256;
            else if (el < 2000) u = (el - 1500) * 256 / 500, wsc = 95 + 5 * u / 256, wy = 62 - 46 * u / 256;
            else if (el < 2400) u = (el - 2000) * 256 / 400, wy = 16 + (100 - 16) * (u * u / 256) / 256;
            qb_wyvern(wx, wy, qsin((int32_t)(now / (el < 1500 ? 50u : 90u)) & 63), (int32_t)(now / 200u) % 3 * 3, 0, wsc);
        }
        else if (bk == 4u)
            dg_skel_s(bx, gy0 - 20, now, flash, 3);
        else
            qb_goblin(bk == 3u ? bx - 24 : bx, gy0 + 4, bk - 2u, boss_die_ms ? 16u + (die * 6u / 1400u > 5u ? 5u : die * 6u / 1400u) : gf, flash);
    }
    if (boss_kind() != 4u) q_pixelize(0, -60, 240, 100, 3);                                                          /* the castle and the boss as pixel art */
    cv_oy = base + QB_DY;                                                                                             /* the party and the effects: the 120-row layout, lower */
    if (buff) {                                                                                                /* the dome over the party: a dithered arc */
        int32_t a;
        for (a = 32; a <= 64; a++)
            if (!(a & 1)) cv_pset(52 + qcos(a) * 62 / 127, GY - 6 + qsin(a) * 44 / 127, RGB(0xa8, 0xd8, 0xff));
    }
    for (i = 0; i < 4; i++) {                                                                                  /* the party, small, on the left: the weapon always in view */
        uint32_t t = ORD[i], age = qh_ms[t] ? now - qh_ms[t] : 9999u, fx = qh_fx[t], pose = 0;
        int32_t x = HX[t] - 4, y = GY - 24, ox = 0, oy = 0;
        if (age < 440u) {
            if (t == 3u) {                                                                                     /* the fighter always closes in and hits: a slash, or the high jump */
                int32_t tgt = bx - 82 - HX[3];
                if (fx == FX_JUMP) {
                    int32_t u = (int32_t)age * 256 / 440;
                    ox = u < 170 ? tgt * u / 170 : tgt * (256 - u) / 86;
                    oy = -(int32_t)(46 * 4 * u * (256 - u) / 65536);
                    pose = u < 90 ? 1u : u < 170 ? 2u : 3u;
                } else {
                    ox = age < 140u ? tgt * (int32_t)age / 140 : age < 260u ? tgt : tgt * (440 - (int32_t)age) / 180;
                    pose = age < 140u ? 1u : age < 260u ? 2u : 3u;
                }
            } else if (age < 280u) pose = (t == 1u && age < 120u) ? 1u : 2u;
        }
        if (bt >= 0 && t == bvic) {                                                                           /* the wyvern's bite: the hero is thrown up, lands somewhere and runs back */
            int32_t tgt = (int32_t)(qhash(bn * 977u + 5u) % 90u) - 45, u;
            if (bt < 600) u = bt * 256 / 600, ox = tgt * u / 256, oy = -(48 * 4 * u * (256 - u) / 65536), pose = 3;
            else if (bt < 900) ox = tgt, pose = 0;
            else u = q_min(256, (bt - 900) * 256 / 900), ox = tgt * (256 - u) / 256, qf_walk = 2, qf_flip = tgt > 0, pose = 0;
        }
        qf_scale = 154;                                                                                       /* 60 %: the boss gets the room */
        if (t == 3u && fx != FX_JUMP && age < 440u && (age < 140u || age >= 260u)) {                          /* the fighter RUNS in and back (the village's run), and strikes in between */
            qf_walk = 2, qf_flip = age >= 260u, pose = 0;
        }
        q_hero_act(t, pose, x + ox, y + oy - (t == 0u ? 4 : 0), 154, 0);
        qf_walk = 0;
        qf_scale = 256;
        if (buff) {                                                                                           /* the attack and defence icons above everybody */
            p_R(x + 5, y - 12, 2, 5, RGB(0xe4, 0xe4, 0xf0)), p_R(x + 4, y - 8, 4, 1, RGB(0xe0, 0xb8, 0x4a));     /* a sword */
            p_R(x + 10, y - 11, 4, 4, RGB(0x6a, 0xb0, 0xe8)), p_R(x + 11, y - 7, 2, 1, RGB(0x6a, 0xb0, 0xe8));   /* a shield */
        }
        if (q_ult_ms[t] && now - q_ult_ms[t] < 500u) {                                                         /* the ult: a column of light on the boss */
            int32_t w = 24 - (int32_t)(now - q_ult_ms[t]) * 24 / 500;
            p_R(bx - w / 2, -QB_DY, w, QB_H, Q_TRK[t]);
        }
        if (age < 440u || (fx == FX_HEAL && age < 1000u) || (fx == FX_FLARE && age < 600u)) {                  /* the attack: small, in pixels */
            int32_t hx = x + 8 + ox, hy = y + 6 + oy, q = (int32_t)age * 256 / 300;
            uint16_t c = Q_TRK[t], w = RGB(0xff, 0xff, 0xff);
            switch (fx) {
            case FX_SPARK: if (age >= 80u && age < 300u) { int32_t r = ((int32_t)age - 80) / 22 + 2; p_R(tx - r, ty, 2 * r + 1, 1, w), p_R(tx, ty - r, 1, 2 * r + 1, w);
                    cv_pset(tx - r, ty - r, c), cv_pset(tx + r, ty - r, c), cv_pset(tx - r, ty + r, c), cv_pset(tx + r, ty + r, c); } break;
            case FX_BALL: if (q < 256) { int32_t bxp = hx + (tx - hx) * q / 256, byp = hy + (ty - hy) * q / 256 - (q * (256 - q)) / 1500; p_ell(bxp, byp, 2, 2, c), p_R(bxp - 1, byp - 1, 2, 2, w), cv_pset(bxp - 5, byp + 1, c); }
                   else if (age < 360u) p_R(tx - 3, ty - 3, 7, 7, c), p_R(tx - 1, ty - 1, 3, 3, w); break;
            case FX_BOLT: if (age < 200u) {                                                                   /* the bolt lands anywhere on the boss */
                    int32_t lx = bx - 36 + (int32_t)(qhash(qh_ms[t]) % 74u), ly = 54 + (int32_t)(qhash(qh_ms[t] * 3u) % 50u), xx = lx + 6, yy = -QB_DY;
                    for (k = 0; k < 5; k++) { int32_t nx = lx + (int32_t)(qhash(now / 40u + (uint32_t)k) % 9u) - 4, ny = -QB_DY + (k + 1) * ((ly + QB_DY) / 5); cv_line(xx, yy, nx, ny, k & 1 ? c : w), cv_line(xx + 1, yy, nx + 1, ny, w); xx = nx, yy = ny; }
                    p_R(lx - 3, ly - 1, 7, 3, w); } break;
            case FX_HEAL: {                                                                                   /* the glow of healing: sparkles appear, rise and fade out one after another */
                    uint32_t n;
                    for (n = 0; n < 12u; n++) {
                        uint32_t hh = qhash(qh_ms[t] * 3u + n * 977u), st = n * 55u;
                        int32_t life = 650, ar;
                        if (age < st || (int32_t)(age - st) >= life) continue;
                        ar = (int32_t)(age - st);
                        if ((int32_t)(hh >> 12) % 100 >= 100 - ar * 100 / life) continue;                            /* the fade: fewer and fewer of its pixels */
                        {
                            int32_t sx = hx - 12 + (int32_t)(hh % 26u), sy = hy + 2 - ar * 26 / life;
                            uint16_t sc = (hh >> 4) % 3u == 0u ? w : (hh >> 4) % 3u == 1u ? RGB(0x9a, 0xf0, 0xa0) : RGB(0xf0, 0xe0, 0x90);
                            p_R(sx - 1, sy, 3, 1, sc), p_R(sx, sy - 1, 1, 3, sc);
                            if (ar < 120) cv_pset(sx, sy, w);
                        }
                    }
                } break;
            case FX_FLARE: {                                                                                  /* charge: motes gather at the staff; then the flash */
                    if (age < 300u) {
                        int32_t R = 24 - (int32_t)age * 24 / 300;
                        for (k = 0; k < 9; k++) cv_pset(hx + qcos(k * 64 / 9 + (int32_t)age / 20) * R / 127, hy - 8 + qsin(k * 64 / 9 + (int32_t)age / 20) * R / 127, k & 1 ? w : c);
                        p_R(hx - 1, hy - 9, 3, 3, w);
                    } else if (age < 560u) {
                        int32_t g = 8 + (int32_t)(age - 300u) / 10, fa = 560 - (int32_t)age;
                        p_R(tx - g, ty, 2 * g + 1, 1, w), p_R(tx, ty - g, 1, 2 * g + 1, w);
                        for (k = 0; k < 8; k++) cv_line(tx, ty, tx + qcos(k * 8) * g * 6 / 5 / 127, ty + qsin(k * 8) * g * 6 / 5 / 127, k & 1 ? c : w);
                        if (fa > 200) p_ell(tx, ty, 6, 6, w);
                    }
                } break;
            case FX_DOME: if (age < 200u) { int32_t r = (int32_t)age / 4 + 8; for (k = 0; k < 24; k++) cv_pset(52 + qcos(k * 64 / 24) * r / 127, GY - 8 + qsin(k * 64 / 24) * r / 254, k & 1 ? w : c); } break;
            case FX_ARROW: if (q < 256) { int32_t axp = hx + (tx - hx) * q / 256, ayp = hy + (ty - hy) * q / 256 - (q * (256 - q)) / 1200; cv_line(axp - 4, ayp + 1, axp + 1, ayp, w), cv_pset(axp + 2, ayp, c); } break;
            case FX_SLASH: if (age >= 140u && age < 300u) { for (k = 0; k < 9; k++) cv_pset(x + ox + 14 + qcos(k * 6 - 24) * 9 / 127, y + oy + 10 + qsin(k * 6 - 24) * 9 / 127, k & 1 ? w : c);
                    p_R(tx - 40, ty - 6, 3, 1, w), p_R(tx - 39, ty - 7, 1, 3, w); } break;
            default: if (age >= 150u && age < 330u) { int32_t r = ((int32_t)age - 150) / 20 + 2; p_R(tx - 36 - r, ty - 14, 2 * r + 1, 1, w), p_R(tx - 36, ty - 14 - r, 1, 2 * r + 1, w), p_R(tx - 36 - r / 2, ty - 14 - r / 2, 1, 1, c), p_R(tx - 36 + r / 2, ty - 14 + r / 2, 1, 1, c); } break;
            }
        }
    }
    for (i = 0; i < 6; i++) {                                                                                  /* damage numbers float up and fade */
        uint32_t age = now - q_dn[i].t0;
        if (!Q_OPT_DMG()) continue;                                                                          /* (menu DAMAGE NUMBERS) */
        char b[8];
        if (!q_dn[i].t0 || age > 700u) continue;
        fmt_int(b, (int32_t)q_dn[i].v);
        {
            static const uint16_t TC[4] = {RGB(0xf2, 0xe2, 0xc4), RGB(0x7a, 0xb0, 0x4e), RGB(0x5a, 0x8a, 0xd0), RGB(0xf0, 0xc8, 0x60)};   /* the combo ring's colours */
            int32_t y = 56 - (int32_t)(age * 30u / 700u);
            if (q_dn[i].hero < 4u) q_text(q_dn[i].x, y, b, Q_TRK[q_dn[i].hero]);
            else { uint16_t c = TC[(q_dn[i].hero - 4u) & 3u]; q_text(q_dn[i].x, y, b, c), q_text(q_dn[i].x + 1, y, b, c); }      /* the player's: bold */
        }
    }
    for (i = 0; i < 3; i++) {                                                                                  /* the player's clicks on the boss: a ring and a cross where it landed */
        uint32_t age = now - cb_tap_ms[i];
        if (!cb_tap_ms[i] || age > 220u) continue;
        { int32_t r = (int32_t)age / 40 + 1, x = cb_tapx[i], y = cb_tapy[i];
          p_R(x - r - 1, y, 2 * r + 3, 1, RGB(0xff, 0xf0, 0xb0)), p_R(x, y - r - 1, 1, 2 * r + 3, RGB(0xff, 0xf0, 0xb0));
          cv_pset(x - r, y - r, Q_HI), cv_pset(x + r, y - r, Q_HI), cv_pset(x - r, y + r, Q_HI), cv_pset(x + r, y + r, Q_HI); }
    }
    cv_oy = base;
    if (cb_n >= 2u) {                                                                                          /* the combo: a ring that fills up to 25 and changes its border every time */
        int32_t cx = 212, cy = 40, a, idle = (int32_t)(now - cb_last), fill = (int32_t)(cb_n % 25u) * 64 / 25, tier = (int32_t)((cb_n / 25u) % 4u);
        char b[8];
        uint16_t col = tier == 0 ? RGB(0xf2, 0xe2, 0xc4) : tier == 1 ? RGB(0x7a, 0xb0, 0x4e) : tier == 2 ? RGB(0x5a, 0x8a, 0xd0) : RGB(0xf0, 0xc8, 0x60);
        if (idle > 2000) fill = fill * (4000 - idle) / 2000;                                                   /* after 2 s the ring starts to go back */
        if (fill < 0) fill = 0;
        p_ell(cx, cy, 17, 17, RGB(0x14, 0x10, 0x20));
        for (a = 0; a < 64; a++) cv_pset(cx + qcos(a - 16) * 16 / 127, cy + qsin(a - 16) * 16 / 127, RGB(0x3a, 0x30, 0x4a));   /* the empty ring */
        for (a = 0; a < fill; a++) {
            int32_t px = cx + qcos(a - 16) * 16 / 127, py = cy + qsin(a - 16) * 16 / 127;
            cv_pset(px, py, col), cv_pset(cx + qcos(a - 16) * 15 / 127, cy + qsin(a - 16) * 15 / 127, col);
        }
        if (tier >= 1) for (a = 0; a < 64; a += 4) cv_pset(cx + qcos(a) * 19 / 127, cy + qsin(a) * 19 / 127, col);                    /* an outer ring of dots */
        if (tier >= 2) for (a = 0; a < 64; a += 16) p_R(cx + qcos(a) * 21 / 127 - 1, cy + qsin(a) * 21 / 127 - 1, 3, 3, col);       /* diamonds at the quarters */
        if (tier == 3) for (a = 0; a < 64; a += 8) cv_pset(cx + qcos(a) * 23 / 127, cy + qsin(a) * 23 / 127, RGB(0xff, 0xff, 0xff));
        if (cb_tier_ms && now - cb_tier_ms < 350u) for (a = 0; a < 64; a += 2) cv_pset(cx + qcos(a) * (18 + (int32_t)(now - cb_tier_ms) / 25) / 127, cy + qsin(a) * (18 + (int32_t)(now - cb_tier_ms) / 25) / 127, RGB(0xff, 0xff, 0xff));
        fmt_int(b, (int32_t)cb_n);
        q_text_c(cx, cy - 8, b, idle > 2000 ? Q_DIM : Q_TEXT);
    }
    cv_oy = base;                                                                                                 /* the avatars (left): a ring that fills with what each hero hits */
    {
        static const uint8_t AO[4] = {3, 0, 1, 2};
        static const uint16_t FACE[4] = {RGB(0xf6, 0xf6, 0xfa), RGB(0x92, 0xc8, 0x68), RGB(0xf0, 0xa0, 0x50), RGB(0x7a, 0xaa, 0xe8)};
        int32_t r;
        for (r = 0; r < 4; r++) {
            uint32_t h = AO[r];
            int32_t cx = 13, cy = 44 + r * 25, a, fill = (int32_t)q_ult[h] * 64 / 100, full = q_ult[h] >= 100u, blink = full && ((now / 140u) & 1u);
            p_ell(cx, cy, 8, 8, RGB(0x14, 0x10, 0x1a));
            p_ell(cx, cy, 6, 6, h == 2u && q_var[2] ? RGB(0xd8, 0xb8, 0xff) : h == 3u && q_var[3] ? RGB(0xe0, 0x60, 0x50) : FACE[h]);
            cv_rect(cx - 3, cy - 1, 2, 2, RGB(0x1a, 0x10, 0x16)), cv_rect(cx + 1, cy - 1, 2, 2, RGB(0x1a, 0x10, 0x16));                 /* eyes */
            for (a = 0; a < 64; a++) {
                uint16_t c = full ? (blink ? RGB(0xff, 0xff, 0xff) : Q_TRK[h]) : a < fill ? Q_TRK[h] : Q_OFF;
                cv_pset(cx + qcos(a - 16) * 11 / 127, cy + qsin(a - 16) * 11 / 127, c);
                cv_pset(cx + qcos(a - 16) * 10 / 127, cy + qsin(a - 16) * 10 / 127, c);
            }
            if (q_ult_hero == h) cv_rect(cx + 14, cy - 3, 5, 6, RGB(0xff, 0xff, 0xff));                                               /* its ult is playing */
        }
    }
    if (q_ult_art != 255u && !q_ult_fullscr) qult_band();                                                           /* the whole animation, in the slanted band, over everything */
    else if (q_ult_hero < 4u) {                                                                                    /* the ult: a flash, rays from the boss, the name (the simple one) */
        uint32_t el = now - q_ult_t0, h = q_ult_hero;
        int32_t k, bxx = 184 + 0, byy = 64, len = (int32_t)(el > 2400u ? 2400u : el) / 12;
        uint16_t c = Q_TRK[h], w = RGB(0xff, 0xff, 0xff);
        static const char *const UN[4] = {"MAGE", "ARCHER", "CLERIC", "KNIGHT"};
        if (el < 160u) for (k = 0; k < 156; k += 2) cv_rect(0, k, 240, 1, w);                                      /* the flash */
        else if (el < 360u) for (k = 0; k < 156; k += 3) cv_rect(0, k, 240, 1, c);
        if (q_ult_simple) {                                                                                        /* the simple ult: the flash, a column of light on the boss, the hit */
            int32_t cw = 26 - (int32_t)(el > 700u ? 700u : el) * 26 / 700;
            if (el < 700u) cv_rect(184 - cw / 2, -base, cw, 156, c);
            if (el >= 1100u && el < 1250u) for (k = 0; k < 156; k += 2) cv_rect(0, k, 240, 1, w);
        } else
        for (k = 0; k < 18; k++) {                                                                                 /* the rays */
            int32_t a = k * 64 / 18 + (int32_t)(el / 40u), r0 = 8 + len / 4, r1 = r0 + 20 + len / 2;
            cv_line(bxx + qcos(a) * r0 / 127, byy + qsin(a) * r0 / 127, bxx + qcos(a) * r1 / 127, byy + qsin(a) * r1 / 127, (k & 1) ? w : c);
        }
        if (!q_ult_simple) {
            cv_oy = base + 36, q_title(76, UN[h], c), cv_oy = base;
            q_text(76 + text_w(&FONT_QT, UN[h]) + 8, 40, "ULT", w);
        }
        if (el >= 1100u && el < 1300u) for (k = 0; k < 156; k += 2) cv_rect(0, k, 240, 1, w);                      /* the hit */
    }
    cv_oy = base;                                                                                                 /* the name, the level under it and the HP bar: the top */
    {
        char b[12];
        int32_t w = 120;
        q_title(6, boss_name(), Q_TEXT);
        str_cpy(b, "Lv ", sizeof b), fmt_int(b + 3, (int32_t)boss_lv);
        q_text(6, 17, b, Q_HI);
        p_R(108, 3, w + 2, 9, RGB(0xf2, 0xe2, 0xc4)), p_R(109, 4, w, 7, RGB(0x1a, 0x10, 0x2a));
        p_R(109, 4, (int32_t)((uint64_t)boss_hp * (uint32_t)w / mx), 7, RGB(0xc8, 0x44, 0x3c));
        if (q_def_t0 && now - q_def_t0 < 8000u) {                                                               /* the guard is down: a cracked shield under the bar, and what is left of it */
            int32_t rem = (int32_t)(8000u - (now - q_def_t0)) * 40 / 8000;
            p_R(108, 15, 11, 12, RGB(0x8a, 0x9a, 0xb8)), p_R(109, 16, 9, 10, RGB(0x3a, 0x4a, 0x70)), p_R(111, 25, 5, 1, RGB(0x3a, 0x4a, 0x70));
            cv_line(110, 16, 115, 25, RGB(0xe0, 0x50, 0x3c)), cv_line(111, 16, 116, 25, RGB(0xe0, 0x50, 0x3c));
            p_R(122, 19, rem, 3, RGB(0xe0, 0x50, 0x3c));
        }
    }
}
static UI_NOINLINE void q_scene_boss2(void)
{
    q_boss_pass(0), cv_blit(0, 19);
    q_boss_pass(-(QB_H / 2)), cv_blit(0, 19 + QB_H / 2);
    cv_oy = 0;
}
/* the rhythm bars under the scene (as the old REC READY list): one row a hero, 16 steps, the playing one lit, a hit flashes */
static UI_NOINLINE void q_rhythm_rows(int32_t y0, uint32_t slot, uint32_t thin)
{
    static const uint8_t ORD[4] = {3, 0, 1, 2};
    uint32_t r, j, sig = 11u + (boss_hp / 64u);
    for (r = 0; r < 4u; r++) {
        const track_t *t = &trk[ORD[r]];
        sig = sig * 31u + (uint32_t)(t->seq_idx % 64u) + (uint32_t)(qh_ms[ORD[r]] && fm1_ms - qh_ms[ORD[r]] < 120u) * 7u;
        for (j = 0; j < 16u; j++) sig = sig * 3u + (uint32_t)trk_step_on(t, j);
    }
    if (!q_changed(slot, sig)) return;
    cv_begin(240, thin ? 16 : 44, Q_BG);                                          /* thin: the boss fight, a third of the height */
    for (r = 0; r < 4u; r++) {
        uint32_t i = ORD[r];
        const track_t *t = &trk[i];
        uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), pos = t->seq_idx % len, silent = q_rest(i), hit = qh_ms[i] && fm1_ms - qh_ms[i] < 120u;
        int32_t y = (int32_t)r * (thin ? 4 : 11), hh = thin ? 3 : 8;
        cv_rect(6, y + (thin ? 0 : 1), 4, hh, Q_TRK[i]);
        for (j = 0; j < 16u; j++) {
            uint32_t p = song.playing ? (pos / 16u) * 16u + j : j, on = 0;
            uint16_t c;
            if (len <= 16u) { if (j < len) on = trk_step_on(t, j) ? 2u : 1u; }
            else if (p < len) on = trk_step_on(t, p) ? 2u : 1u;
            c = !on ? Q_BG : on == 2u ? (silent ? Q_DIM : Q_TRK[i]) : (j % 4u == 0 ? Q_OFFHI : Q_OFF);
            if (song.playing && p == pos) c = on == 2u && hit ? RGB(0xff, 0xff, 0xff) : on == 2u ? Q_TEXT : Q_OFFHI;
            cv_rect(16 + (int32_t)j * 14, y + (thin ? 0 : 1), 11, hh, c);
        }
    }
    cv_blit(0, (uint32_t)y0);
}

/* ---- the recording scene: a camp fire at night, the band sitting round it; the bard holds the FM-1 ---- */
static UI_NOINLINE void q_scene_camp2(void)
{
    const uint32_t now = fm1_ms;
    int32_t i, fl = (int32_t)p_sinr((int32_t)((now * 11u) % 6283u)) * 3 / 127, gy = 72;
    cv_begin(QW, 84, QP(0));
    p_R(0, 0, 240, 30, RGB(0x10, 0x14, 0x2e)), p_R(0, 30, 240, 24, RGB(0x1a, 0x1c, 0x3c)), p_R(0, 54, 240, 30, RGB(0x24, 0x22, 0x44));
    q_stars(30, 0, 46, 777u);                                                              /* stars: dots, pluses, bright ones */
    p_tri(0, 56, 50, 34, 100, 56, RGB(0x1c, 0x1c, 0x34)), p_tri(150, 56, 205, 30, 240, 56, RGB(0x1c, 0x1c, 0x34));   /* far hills */
    p_R(0, gy, 240, 12, RGB(0x1c, 0x24, 0x22));
    p_ell(120, gy - 2, 46, 14, RGB(0x3a, 0x26, 0x22)), p_ell(120, gy - 2, 30, 9, RGB(0x5a, 0x34, 0x20));   /* the glow on the ground */
    p_R(104, gy - 4, 32, 4, RGB(0x4a, 0x2e, 0x1a)), p_R(110, gy - 7, 20, 3, RGB(0x5a, 0x38, 0x20));        /* the logs */
    p_poly((const int32_t[5][2]){{108, gy - 7}, {112 + fl, gy - 24}, {120, gy - 14}, {128 - fl, gy - 26}, {132, gy - 7}}, 5, RGB(0xe0, 0x6a, 0x20));
    p_poly((const int32_t[4][2]){{113, gy - 7}, {120 + fl, gy - 20}, {127, gy - 7}, {120, gy - 10}}, 4, RGB(0xf6, 0xb0, 0x40));
    p_tri(117, gy - 7, 120, gy - 14, 123, gy - 7, RGB(0xff, 0xf0, 0xb0));
    for (i = 0; i < 6; i++) {                                                           /* embers */
        uint32_t a = (now / 40u + (uint32_t)i * 37u) % 120u;
        cv_pset(116 + (int32_t)((uint32_t)i * 5u) % 14 + (int32_t)p_sinr((int32_t)(a * 90u)) * 3 / 127, gy - 12 - (int32_t)a / 3, a < 80u ? RGB(0xff, 0xc8, 0x60) : RGB(0xc8, 0x6a, 0x30));
    }
    {   /* the four round the fire: the mage, the archer, the bard with the FM-1, the knight */
        static const int16_t PX[4] = {62, 88, 156, 182};
        static const uint8_t CL[4] = {0, 1, 4, 3};
        for (i = 0; i < 4; i++) {
            uint32_t slot = i < 2 ? (uint32_t)i : i == 2 ? 2u : 3u;
            uint32_t cls = i == 2 ? 4u : i == 3 ? (q_var[3] ? 5u : 3u) : CL[i];
            int hit = qh_ms[slot] && now - qh_ms[slot] < 180u;
            q_fig(cls, 4, hit ? 1u : 0u, PX[i], gy + 3, 1, i >= 2 ? 1 : 0, -1, 0, 0, 0);
            if (hit) p_R(PX[i] - 2, gy - 34 - (i & 1) * 3, 4, 4, Q_TRK[slot]);          /* a note floats up from whoever plays */
        }
    }
}

/* ---- the unlock banner: slides down from the top, stays 10 s with a countdown, HOME closes it (ui_timers draws it over any screen) ---- */
static UI_NOINLINE void quest_unlock_draw(void)
{
    static uint32_t last_sig;
    uint32_t age, secs, sig;
    int32_t vis, w;
    if (!q_unlock_t0) { if (cv_ban_h) cv_ban_h = 0, ui.force = 1; return; }
    age = (int32_t)(fm1_ms - q_unlock_t0) < 0 ? 0u : fm1_ms - q_unlock_t0;
    if (age >= 10000u) { q_unlock_t0 = 0, cv_ban_h = 0; ui.force = 1; return; }
    vis = age < 400u ? (int32_t)(age * 52u / 400u) : 52;
    secs = 10u - age / 1000u;
    w = (int32_t)(10000u - age) * 224 / 10000;
    sig = (uint32_t)vis * 7919u + (uint32_t)w * 31u + 1u;
    cv_ban_h = (uint32_t)vis;                                                           /* the scenes leave these rows alone while it is shown */
    if (sig == last_sig && !ui.force) return;
    last_sig = sig;
    cv_ban_h = 0;
    cv_begin(240, (uint32_t)vis, Q_BG);
    cv_oy = vis - 52;
    cv_rect(0, 0, 240, 52, RGB(0x1a, 0x10, 0x2a)), cv_rect(0, 0, 240, 1, Q_HI), cv_rect(0, 51, 240, 1, Q_HI);
    q_title(8, "Boss defeated!", Q_HI);
    q_text(8, 20, "WARRIOR unlocked", Q_TEXT);
    q_text(8, 35, "change heroes: menu > party", Q_DIM);
    { char n[8]; fmt_int(n, (int32_t)secs); q_text_r(232, 2, n, Q_HI); }
    q_text_r(232, 20, "home: close", Q_PDIM);
    cv_rect(8, 49, w, 2, Q_HI);                                                         /* the timer: it runs out as the banner goes */
    cv_oy = 0;
    cv_blit(0, 0);
    cv_ban_h = (uint32_t)vis;
}

/* the ult's whole animation: 24 x 24 pixels, 3 colours, the video's length (assets/quest/ult, tools/gen_quest_ult.py); decoded frame by frame with a binary range
 * coder whose context is the left pixel, the one above and the same pixel of the frame before. It plays in a slanted band cut across the screen (the default) or
 * on the whole screen (menu ULT ANIMATION); the first time of each hero it plays through, after that any button skips it. ---- */
static qrc_t qult_rc;
static uint16_t qult_pr[128];
#define QULT_MAX 2304u                            /* 48 x 48, one bit a pixel (all the ults are two-colour) */
static uint8_t qult_curb[QULT_MAX / 8u], qult_prevb[QULT_MAX / 8u];
#define QBIT(a, i) (((a)[(i) >> 3] >> ((i) & 7u)) & 1u)
#define QSET(a, i, v) ((v) ? ((a)[(i) >> 3] |= (uint8_t)(1u << ((i) & 7u))) : ((a)[(i) >> 3] &= (uint8_t)~(1u << ((i) & 7u))))
static uint32_t qult_frame;                      /* frames decoded so far */
static uint32_t qult_idx(void);
static UI_NOINLINE void qult_decode(void)         /* the next frame of the stream into qult_curb (W x H, 48 x 48 with 2 colours) */
{
    const uint32_t a = qult_idx(), W = QULT_W[a], H = QULT_H[a];
    uint32_t x, y, i;
    for (i = 0; i < QULT_MAX / 8u; i++) qult_prevb[i] = qult_curb[i];
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++) {
            uint32_t l = x ? QBIT(qult_curb, y * W + x - 1u) : 0u, u = y ? QBIT(qult_curb, (y - 1u) * W + x) : 0u, v;
            uint32_t ul = x && y ? QBIT(qult_curb, (y - 1u) * W + x - 1u) : 0u, ur = y && x + 1u < W ? QBIT(qult_curb, (y - 1u) * W + x + 1u) : 0u, pn = x + 1u < W ? QBIT(qult_prevb, y * W + x + 1u) : 0u;
            v = qrc_bit(&qult_rc, &qult_pr[l | u << 1 | QBIT(qult_prevb, y * W + x) << 2 | ul << 3 | ur << 4 | pn << 5]);
            QSET(qult_curb, y * W + x, v);
        }
    qult_frame++;
}
static uint32_t qult_idx(void) { return q_ult_art == 3u && q_var[3] ? 4u : q_ult_art; }
static uint8_t q_ult_art_new;                    /* a new frame was decoded: the full-screen picture is painted again */
static UI_NOINLINE void q_ult_art_step(void)      /* once a frame: start, advance, skip or finish the animation */
{
    static uint32_t pb;
    uint32_t el, a, seen_bit, i, newly, now = fm1_ms, target;
    int skippable;
    if (q_ult_art == 255u) return;
    a = qult_idx();
    seen_bit = 1u << (q_ult_art == 3u ? 3u : q_ult_art);
    skippable = 0;                                                               /* it can no longer be skipped or cancelled: the whole animation, always */
    el = (int32_t)(now - q_ult_art_t0) < 0 ? 0u : now - q_ult_art_t0;
    newly = (fm1_in.buttons | fm1_in.notes) & ~pb;
    pb = fm1_in.buttons | fm1_in.notes;
    if (!q_ult_art_open) {                                                      /* start the stream of this hero */
        qult_rc.p = QULT_Z + QULT_ZO[a], qult_rc.range = 0xFFFFFFFFu, qult_rc.code = 0;
        for (i = 0; i < 5u; i++) qult_rc.code = (qult_rc.code << 8) | *qult_rc.p++;
        for (i = 0; i < 128u; i++) qult_pr[i] = 1024;
        for (i = 0; i < QULT_MAX / 8u; i++) qult_curb[i] = 0;
        qult_frame = 0, q_ult_art_open = 1, newly = 0;
    }
    target = el * QULT_N[a] / QULT_MS;                                          /* every ult lasts QULT_MS, all its frames */
    if (target >= QULT_N[a] || (skippable && newly)) {                          /* over (the whole of it counts as seen), or skipped */
        if (target >= QULT_N[a] && qs.screen == QSN_BOSS) q_ult_seen |= (uint8_t)seen_bit, settings_later = 1;
        q_ult_art = 255, q_ult_art_open = 0, q_ult_art_new = 0, cv_ban_h = 0, ui.force = 1;
        return;
    }
    while (qult_frame <= target) qult_decode(), q_ult_art_new = 1;
}
static UI_NOINLINE void qult_band(void)           /* drawn over the boss scene (cv_oy set by the caller): a small slanted black box above the heroes' heads, on the left, the animation at twice its size in it, black and white */
{
    const uint16_t *pal = QULT_PAL[qult_idx()];
    uint32_t el = fm1_ms - q_ult_art_t0, total = QULT_MS, k, lum[3], lo = 9999u, hi = 0u, on = 0u;
    int32_t prog = el < 220u ? (int32_t)el * 60 / 220 : total - el < 220u ? (int32_t)(total - el) * 60 / 220 : 60, xs, x, y;
    const int32_t X0 = 4, W = 60, Y0 = 74, H = 56, SL = 8;
    const uint16_t blk = RGB(0x08, 0x06, 0x0c), wht = RGB(0xff, 0xff, 0xff);
    for (k = 0; k < 3u; k++) {
        lum[k] = ((pal[k] >> 11) & 31u) * 2u + ((pal[k] >> 5) & 63u) + (pal[k] & 31u) * 2u;
        if (lum[k] < lo) lo = lum[k];
        if (lum[k] > hi) hi = lum[k];
    }
    for (k = 0; k < 3u; k++) if (lum[k] * 2u >= lo + hi && hi > lo) on |= 1u << k;     /* the lighter colours are white, the others black */
    for (xs = 0; xs < prog && xs < W; xs += 2) {
        int32_t yt = Y0 - xs * SL / W, yb = yt + H;
        cv_rect(X0 + xs, yt, 2, H, blk);
        cv_rect(X0 + xs, yt - 1, 2, 1, wht), cv_rect(X0 + xs, yb, 2, 1, wht);
    }
    if (prog >= W) {
        const int32_t aw = (int32_t)QULT_W[qult_idx()], ah = (int32_t)QULT_H[qult_idx()], dw = 48;      /* the picture is always 48 x 48 on the screen: 24 x 2, 32 x 1.5 */
        int32_t px0 = X0 + (W - dw) / 2, py0 = Y0 - SL / 2 + (H - dw) / 2;
        for (y = 0; y < ah; y++)
            for (x = 0; x < aw; x++) {
                uint32_t v = QBIT(qult_curb, (uint32_t)(y * aw + x));
                int32_t x0 = x * dw / aw, x1 = (x + 1) * dw / aw, y0 = y * dw / ah, y1 = (y + 1) * dw / ah;
                if ((on >> v) & 1u) cv_rect(px0 + x0, py0 + y0, x1 - x0, y1 - y0, wht);
            }
    }
}
static UI_NOINLINE void quest_ult_art_draw(void)  /* (the whole-screen ult is gone: only the small slanted band at the side, qult_band) */
{
}
