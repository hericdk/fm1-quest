/* ---------------------------------------------------------------- CDs: every time the sound of a track changes (PRESETS, the PRESETS page, a drum
 * kit) the screen shows the sounds as CDs in a carousel, the old one rolling out and the new one rolling in, then goes back to what was there.
 * Ported from No Man Sky FM (ui_cd.c), after winampFM's TAPES (wt_make / wt_quad): the same 42 models' colours in the same shuffled order,
 * the same PS1 look (low resolution, affine texture, a dithered shade on the far side, the ones behind see-through). The 64 x 64 texture
 * lives in .pool (RAM is tight under SLOOP 2.6); the screen is black behind the CDs. */
#define CDK C_BLACK
#define CDW C_WHITE
static const char *const CD_ROLE[8] = {"BASS", "KEYS", "ORGAN", "PAD", "LEAD", "PLUCK", "STAB", "FX"};
static int32_t cd_sin(uint32_t a) { return SINE[a & 1023u]; }                  /* Q15, 1024 a turn */
static int32_t cd_cos(uint32_t a) { return SINE[(a + 256u) & 1023u]; }
static uint16_t cd_mix(uint16_t a, uint16_t b, int32_t t)                        /* a to b, t 0..16 */
{
    int32_t r = ((a >> 11) * (16 - t) + (b >> 11) * t) >> 4, g = (((a >> 5) & 63) * (16 - t) + ((b >> 5) & 63) * t) >> 4, bl = ((a & 31) * (16 - t) + (b & 31) * t) >> 4;
    return (uint16_t)(r << 11 | g << 5 | bl);
}
static int32_t cd_big(int32_t x, int32_t y, const char *s, uint32_t n, uint16_t c)             /* the small font at n times */
{
    const felucca_font_t *f = &FONT_S;
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), w = f->bw[gi], bpr = (w + 1u) / 2u, gx, gy;
        const uint8_t *gd = glyph_bits(f, gi);
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t v = gd[gy * bpr + gx / 2u];
                if (((gx & 1u) ? (v & 15u) : (v >> 4)) > 7u)       /* (4-bit glyphs: half or more) */
                    cv_rect(x + ((int32_t)gx - f->pad) * (int32_t)n, y + (int32_t)(gy * n), (int32_t)n, (int32_t)n, c);
            }
        x += f->adv[gi] * (int32_t)n;
    }
    return x;
}
static int32_t cd_bigw(const char *s, uint32_t n) { return text_w(&FONT_S, s) * (int32_t)n; }
static void cd_bigo(int32_t x, int32_t y, const char *s, uint32_t n, uint16_t c)               /* with a black outline */
{
    cd_big(x - 1, y, s, n, CDK), cd_big(x + 1, y, s, n, CDK), cd_big(x, y - 1, s, n, CDK), cd_big(x, y + 1, s, n, CDK), cd_big(x + 1, y + 1, s, n, CDK);
    cd_big(x, y, s, n, c);
}
#define CD_HOLD 1500u                        /* the screen stays this long after the last change */
#define CD_IN 320u                           /* it comes up ... */
#define CD_OUT 300u                          /* ... and goes down */
/* cd_tex[CD_T][CD_T] (ui_quest.c): palette indices, 0 = nothing (outside the disc, the hole) */
static uint16_t cd_pal[16];
static uint32_t cd_t0, cd_open, cd_flip;     /* the last change, when the screen opened, when the middle CD arrived */
static int32_t cd_cur, cd_tgt;               /* the carousel, 1/256 CD, chasing the sound's place */
static uint32_t cd_n;                        /* how many in the list */
static uint8_t cd_drum, cd_ghost;
/* 0 nothing 1 rim 2 rim dark 3 print 4 label 5 label 2 6 small print 7 the name 8..13 accents 14 the clear ring 15 white */
enum { CM_CLASSIC, CM_ART, CM_COMPUTER, CM_AUDITION, CM_SONYB, CM_FUTURE, CM_TOLAKU, CM_DOODLE, CM_PINK, CM_STAR,
       CM_FISHER, CM_POLIMER, CM_AGFASC, CM_TEAC, CM_AGFAFC, CM_AGFALNS, CM_BASFR, CM_BASFO, CM_HAPPY, CM_BARBIE,
       CM_HOLO, CM_FEB, CM_BRITPOP, CM_EMO, CM_HORTEN, CM_EXCL, CM_SKC, CM_OIL, CM_FLORAL, CM_AGFAY, CM_CLASSICBR,
       CM_SCOTCH, CM_SEIKO, CM_TDKD, CM_KITTYP, CM_KITTYB, CM_SPANKP, CM_SPANKG, CM_AGFAC90, CM_APPLEPIE, CM_3M, CM_DEMAG, CM_N };
#define C3(r, g, b) RGB(0x##r, 0x##g, 0x##b)
static const struct { uint16_t shell, label, label2, ink, name, a[6]; } CD_M[CM_N] = {   /* (winampFM's WT_M, colour for colour) */
    [CM_CLASSIC] = {C3(e6, dc, b8), C3(f6, f4, ee), C3(40, 46, 4a), C3(60, 60, 60), C3(e8, e8, ec), {C3(60, b8, 40), C3(f8, c0, 30), C3(f0, 80, 28), C3(e0, 40, 38), C3(90, 40, c0), C3(30, 70, d0)}},
    [CM_CLASSICBR] = {C3(e8, e0, a0), C3(f8, f8, f0), C3(10, 60, 30), C3(10, 60, 30), C3(f8, d8, 20), {C3(10, 90, 40), C3(10, 90, 40), C3(f8, d8, 20), C3(f8, d8, 20), C3(20, 50, b0), C3(20, 50, b0)}},
    [CM_ART] = {C3(c8, d0, d8), C3(f8, f8, f8), C3(10, 10, 14), C3(30, a0, 80), C3(20, 80, 60), {C3(40, 50, d8), C3(f8, e0, c8), C3(10, 10, 14), C3(d8, e8, f8), C3(f0, 90, 98), C3(20, 80, 60)}},
    [CM_COMPUTER] = {C3(f4, f2, ea), C3(f4, f2, ea), C3(20, 20, 20), C3(20, 20, 20), C3(20, 20, 20), {C3(60, b8, 40), C3(f8, c8, 30), C3(f0, 80, 28), C3(e0, 40, 38), C3(90, 40, a0), C3(30, 80, d0)}},
    [CM_AUDITION] = {C3(1c, 1c, 1c), C3(f0, f0, e8), C3(d8, 28, 20), C3(30, 30, 30), C3(f8, f8, f8), {C3(20, 40, a0), C3(d0, 30, 30), C3(b0, b0, b0), 0, 0, 0}},
    [CM_SONYB] = {C3(c0, c8, d0), C3(f0, ec, dc), C3(20, 90, 50), C3(30, 30, 30), C3(18, 18, 18), {C3(20, 90, 50), C3(30, 30, 30), 0, 0, 0, 0}},
    [CM_FUTURE] = {C3(b8, 18, 10), C3(f4, f2, ee), C3(b8, 18, 10), C3(20, 20, 20), C3(20, 20, 20), {C3(30, f0, 80), C3(d0, 30, 30), C3(e8, 50, 40), 0, 0, 0}},
    [CM_TOLAKU] = {C3(d8, 20, 20), C3(f4, f4, f0), C3(40, b8, e8), C3(20, 20, 20), C3(d8, 20, 30), {C3(20, 90, e0), C3(f8, f8, f8), C3(d8, 20, 30), 0, 0, 0}},
    [CM_DOODLE] = {C3(2c, 2c, 30), C3(f8, c8, 40), C3(90, c8, e8), C3(30, 30, 30), C3(e0, 48, 30), {C3(f8, f8, f8), C3(90, 70, c8), C3(f0, 70, 30), C3(e0, 48, 30), 0, 0}},
    [CM_PINK] = {C3(f4, c0, c8), C3(f8, f8, f8), C3(60, b8, b8), C3(20, 20, 20), C3(18, 18, 18), {C3(e0, 20, 20), C3(f8, f8, f8), C3(e0, a0, a8), 0, 0, 0}},
    [CM_STAR] = {C3(30, 30, 34), C3(c8, 18, 30), C3(c8, 18, 30), C3(f0, c8, 30), C3(f8, d0, 30), {C3(20, 30, 90), C3(10, 18, 50), C3(f0, c8, 30), C3(40, 60, d0), 0, 0}},
    [CM_FISHER] = {C3(f8, d8, 20), C3(f0, e8, d0), C3(e0, 30, 40), C3(30, 30, 30), C3(30, 28, 20), {C3(30, 70, c0), C3(40, a8, 50), C3(f0, 90, 30), C3(f8, f8, f8), 0, 0}},
    [CM_POLIMER] = {C3(f0, 78, 18), C3(30, b0, d0), C3(30, b0, d0), C3(f8, f8, f8), C3(10, 10, 20), {C3(f8, d0, 30), C3(f0, 80, 30), C3(e0, 40, 60), C3(90, 50, c0), 0, 0}},
    [CM_AGFASC] = {C3(30, 80, d0), C3(50, c8, d8), C3(50, c8, d8), C3(10, 20, 70), C3(10, 20, 70), {C3(10, 20, 70), C3(e0, 30, 30), C3(f0, e0, c0), C3(f8, f8, f8), 0, 0}},
    [CM_TEAC] = {C3(b8, d0, f0), C3(b8, d0, f0), C3(48, 20, 18), C3(30, 30, 40), C3(f8, f8, f8), {C3(48, 20, 18), C3(a0, 70, c0), 0, 0, 0, 0}},
    [CM_AGFAFC] = {C3(e0, 28, 28), C3(f0, f0, ea), C3(e0, 28, 28), C3(20, 20, 20), C3(20, 20, 20), {C3(10, 20, 70), C3(e0, 30, 30), C3(f8, f8, f8), C3(10, 10, 10), 0, 0}},
    [CM_AGFALNS] = {C3(f8, 70, 20), C3(f8, 70, 20), C3(f8, 70, 20), C3(20, 20, 20), C3(20, 20, 20), {C3(10, 20, 70), C3(e0, 30, 30), C3(f8, f8, f8), 0, 0, 0}},
    [CM_BASFR] = {C3(20, 20, 22), C3(f0, 40, 20), C3(10, 10, 10), C3(10, 10, 10), C3(f8, f8, f8), {C3(f0, 40, 20), C3(f8, f8, f8), 0, 0, 0, 0}},
    [CM_BASFO] = {C3(20, 20, 22), C3(f8, f8, f4), C3(f8, a0, 30), C3(20, 20, 20), C3(10, 10, 10), {C3(e0, 30, 30), C3(f8, f8, f8), C3(b0, b0, b8), 0, 0, 0}},
    [CM_HAPPY] = {C3(f0, 50, 18), C3(40, b8, d0), C3(40, b8, d0), C3(10, 10, 20), C3(f8, f8, f8), {C3(70, 30, a0), C3(f8, f8, f8), 0, 0, 0, 0}},
    [CM_BARBIE] = {C3(f0, 40, b0), C3(e0, f0, 40), C3(f0, 70, 80), C3(10, 10, 20), C3(e0, 20, 90), {C3(20, 30, 90), C3(f0, 70, 80), C3(a0, 90, b8), 0, 0, 0}},
    [CM_HOLO] = {C3(b0, 88, e8), C3(d8, e0, f0), C3(d8, e0, f0), C3(20, 20, 20), C3(10, 10, 10), {C3(f0, b0, d0), C3(a0, e0, f0), C3(f8, f0, a0), C3(c0, b0, f0), 0, 0}},
    [CM_FEB] = {C3(f4, b8, c4), C3(f8, d8, dc), C3(c8, 18, 30), C3(f8, d8, dc), C3(c8, 18, 30), {C3(d8, 20, 40), C3(f0, a0, b0), 0, 0, 0, 0}},
    [CM_BRITPOP] = {C3(90, b0, d0), C3(f4, f0, e8), C3(70, a8, d8), C3(20, 20, 20), C3(d0, 40, 40), {C3(d0, 40, 40), C3(20, 20, 20), C3(70, a8, d8), 0, 0, 0}},
    [CM_EMO] = {C3(20, 20, 20), C3(f0, ec, e4), C3(d0, 40, 40), C3(20, 20, 20), C3(20, 20, 20), {C3(d0, 40, 40), C3(f0, ec, e4), C3(20, 20, 20), 0, 0, 0}},
    [CM_HORTEN] = {C3(c8, cc, c4), C3(f0, 50, 90), C3(f0, 50, 90), C3(20, 20, 20), C3(f8, f8, f8), {C3(10, 10, 10), C3(30, 30, 90), 0, 0, 0, 0}},
    [CM_EXCL] = {C3(e8, 28, 20), C3(f8, f4, f0), C3(e8, 28, 20), C3(20, 20, 20), C3(f8, d8, 20), {C3(f8, d8, 20), C3(c0, 30, 30), 0, 0, 0, 0}},
    [CM_SKC] = {C3(d0, d8, e0), C3(d0, d8, e0), C3(20, 20, 28), C3(20, 20, 30), C3(f8, f8, f8), {C3(f0, 30, 70), C3(f8, c0, 30), C3(30, a0, b0), C3(20, 20, 28), C3(40, 60, 90), 0}},
    [CM_OIL] = {C3(d8, d8, e0), C3(f0, b0, c0), C3(f0, b0, c0), C3(30, 30, 30), C3(f8, f8, f8), {C3(f0, a0, 60), C3(a0, d0, f0), C3(c0, 90, d8), C3(f8, f0, 90), C3(60, 50, 70), 0}},
    [CM_FLORAL] = {C3(d8, d8, e0), C3(20, 28, 20), C3(20, 28, 20), C3(f8, f8, f8), C3(f8, f8, f8), {C3(e0, 70, 60), C3(f0, b0, a0), C3(40, 80, 40), C3(f8, f8, f0), 0, 0}},
    [CM_AGFAY] = {C3(f8, c0, 20), C3(f8, c0, 20), C3(f8, c0, 20), C3(20, 20, 20), C3(20, 20, 20), {C3(10, 20, 70), C3(e0, 30, 30), C3(f8, f8, f8), 0, 0, 0}},
    [CM_SCOTCH] = {C3(80, b8, f0), C3(a8, d0, d8), C3(70, 48, 30), C3(10, 10, 20), C3(10, 10, 20), {C3(70, 48, 30), C3(f0, e0, a0), 0, 0, 0, 0}},
    [CM_SEIKO] = {C3(c8, c8, c4), C3(f4, f4, f0), C3(30, a8, e0), C3(20, 20, 20), C3(f8, f8, f8), {C3(30, a8, e0), C3(10, 10, 20), 0, 0, 0, 0}},
    [CM_TDKD] = {C3(50, 48, d8), C3(50, 48, d8), C3(10, 10, 14), C3(f8, f8, f8), C3(e8, d8, 90), {C3(10, 10, 14), C3(70, 90, f0), C3(d8, c0, 60), C3(f8, f8, f8), 0, 0}},
    [CM_KITTYP] = {C3(f8, c8, d4), C3(f8, d8, e0), C3(30, 18, 20), C3(e0, 30, 70), C3(e0, 30, 70), {C3(e0, 30, 70), C3(30, 18, 20), C3(f8, f8, f8), 0, 0, 0}},
    [CM_KITTYB] = {C3(a8, c8, f0), C3(b8, d4, f8), C3(20, 30, 40), C3(30, 50, c0), C3(30, 50, c0), {C3(30, 50, c0), C3(20, 30, 40), C3(f8, f8, f8), 0, 0, 0}},
    [CM_SPANKP] = {C3(f8, b0, c0), C3(f8, b0, c0), C3(20, 18, 20), C3(e0, 50, 70), C3(e0, 50, 70), {C3(60, d0, b0), C3(f0, 60, 80), C3(60, a0, d0), C3(20, 18, 20), 0, 0}},
    [CM_SPANKG] = {C3(60, f0, 60), C3(60, f0, 60), C3(20, 18, 20), C3(e0, 40, 90), C3(e0, 40, 90), {C3(60, d0, b0), C3(f0, 50, 90), C3(60, a0, e0), C3(20, 18, 20), 0, 0}},
    [CM_AGFAC90] = {C3(f8, b0, 20), C3(f8, b0, 20), C3(f8, b0, 20), C3(20, 20, 20), C3(f8, f8, f8), {C3(10, 20, 70), C3(e0, 30, 30), C3(f8, f8, f8), 0, 0, 0}},
    [CM_APPLEPIE] = {C3(22, 22, 22), C3(f8, b8, 10), C3(f8, b8, 10), C3(20, 20, 20), C3(20, 20, 20), {C3(20, 20, 20), C3(a0, a0, a0), C3(f8, f8, f8), 0, 0, 0}},
    [CM_3M] = {C3(f8, 50, 20), C3(e0, 28, 28), C3(f8, f0, e8), C3(f8, f0, e8), C3(f8, f0, e8), {C3(f8, f0, e8), 0, 0, 0, 0, 0}},
    [CM_DEMAG] = {C3(d0, d8, e0), C3(e8, e4, d8), C3(30, 28, 28), C3(30, 28, 28), C3(20, 20, 20), {C3(b0, 78, 40), C3(30, 30, 30), C3(90, 80, c0), C3(c0, 60, 40), C3(c0, a0, 40), C3(e0, 40, 40)}},
};
static const uint8_t CD_ORDER[CM_N] = {      /* every model once, shuffled (winampFM's WT_ORDER): the list takes them in turn */
    CM_TEAC, CM_3M, CM_COMPUTER, CM_AGFAY, CM_TDKD, CM_EXCL, CM_TOLAKU, CM_BASFR, CM_BASFO, CM_BARBIE, CM_HOLO, CM_POLIMER, CM_CLASSIC, CM_SEIKO,
    CM_SKC, CM_SPANKP, CM_AGFAC90, CM_KITTYB, CM_FISHER, CM_DOODLE, CM_SCOTCH, CM_HAPPY, CM_CLASSICBR, CM_BRITPOP, CM_ART, CM_FUTURE, CM_STAR,
    CM_EMO, CM_KITTYP, CM_OIL, CM_AGFASC, CM_AUDITION, CM_SPANKG, CM_DEMAG, CM_FLORAL, CM_FEB, CM_HORTEN, CM_APPLEPIE, CM_PINK, CM_AGFAFC,
    CM_SONYB, CM_AGFALNS};
/* the print of each model, after its tape's art: 0 rainbow rings (the classic tapes' rainbow), 1 a label band across, 2 sunburst, 3 dots,
 * 4 stripes, 5 the square-and-diamond logo with lines, 6 stars, 7 holographic, 8 checker band, 9 petals */
static const uint8_t CD_ART[CM_N] = {
    [CM_CLASSIC] = 0, [CM_CLASSICBR] = 0, [CM_ART] = 9, [CM_COMPUTER] = 4, [CM_AUDITION] = 1, [CM_SONYB] = 1, [CM_FUTURE] = 2, [CM_TOLAKU] = 8,
    [CM_DOODLE] = 3, [CM_PINK] = 3, [CM_STAR] = 6, [CM_FISHER] = 4, [CM_POLIMER] = 2, [CM_AGFASC] = 5, [CM_TEAC] = 1, [CM_AGFAFC] = 5,
    [CM_AGFALNS] = 5, [CM_BASFR] = 1, [CM_BASFO] = 0, [CM_HAPPY] = 2, [CM_BARBIE] = 3, [CM_HOLO] = 7, [CM_FEB] = 9, [CM_BRITPOP] = 4,
    [CM_EMO] = 8, [CM_HORTEN] = 2, [CM_EXCL] = 6, [CM_SKC] = 4, [CM_OIL] = 7, [CM_FLORAL] = 9, [CM_AGFAY] = 5, [CM_SCOTCH] = 1,
    [CM_SEIKO] = 0, [CM_TDKD] = 7, [CM_KITTYP] = 3, [CM_KITTYB] = 3, [CM_SPANKP] = 6, [CM_SPANKG] = 6, [CM_AGFAC90] = 5, [CM_APPLEPIE] = 8,
    [CM_3M] = 2, [CM_DEMAG] = 0};
static uint32_t cd_model(uint32_t n) { return CD_ORDER[(n + (cd_drum ? 7u : 0u)) % CM_N]; }
static const char *cd_name(uint32_t n, const char **kind)    /* the sound n of the list: its name and its kind */
{
    static char b[13];
    if (cd_drum) {
        *kind = "DRUMS";
        return DRUM_KIT_NAMES[n % DRUM_KITS];
    }
    if (n < NBANK) {
        *kind = CD_ROLE[BANK[n].kind & 7u];
        return BANK[n].name;
    }
    *kind = "USER";
    n -= NBANK;
    if (n < up_count()) {
        up_name(up_nth(n), b);
        return b;
    }
    return "USER";
}
static void cd_px(int32_t x, int32_t y, uint8_t c) { if ((uint32_t)x < CD_T && (uint32_t)y < CD_T && cd_tex[y][x]) cd_tex[y][x] = c; }
static int32_t cd_text(int32_t x, int32_t y, const char *s, uint8_t c)   /* the small font at half size (2 x 2 pixels of a glyph = 1 texel) */
{
    const felucca_font_t *f = &FONT_S;
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), w = f->bw[gi], bpr = (w + 1u) / 2u, gx, gy;
        const uint8_t *gd = glyph_bits(f, gi);
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t v = gd[gy * bpr + gx / 2u];
                if (((gx & 1u) ? (v & 15u) : (v >> 4)) > 7u)
                    cd_px(x + ((int32_t)gx - f->pad) / 2, y + (int32_t)gy / 2, c);
            }
        x += f->adv[gi] / 2;
    }
    return x;
}
static int32_t cd_tw(const char *s) { return text_w(&FONT_S, s) / 2; }
static uint32_t cd_ang(int32_t dx, int32_t dy)          /* 0..255 round from the right, clockwise (an octant atan, good enough for art) */
{
    int32_t ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy, a;
    if (!ax && !ay)
        return 0;
    a = ax >= ay ? ay * 32 / ax : 64 - ax * 32 / ay;     /* 0..64 in the first quadrant */
    if (dx < 0)
        a = 128 - a;
    if (dy < 0)
        a = 256 - a;
    return (uint32_t)a & 255u;
}
/* the CD of sound n into cd_tex / cd_pal */
static void cd_make(uint32_t n)
{
    uint32_t md = cd_model(n), art = CD_ART[md], i;
    int32_t x, y;
    const char *kind, *nm = cd_name(n, &kind);
    char b[16];
    cd_pal[0] = CDK;
    cd_pal[1] = RGB(0xc8, 0xcc, 0xd4), cd_pal[2] = RGB(0x70, 0x74, 0x80);
    cd_pal[3] = CD_M[md].shell, cd_pal[4] = CD_M[md].label, cd_pal[5] = CD_M[md].label2, cd_pal[6] = CD_M[md].ink, cd_pal[7] = CD_M[md].name;
    for (i = 0; i < 6u; i++)
        cd_pal[8 + i] = CD_M[md].a[i] ? CD_M[md].a[i] : CD_M[md].a[i & 1u];
    cd_pal[14] = RGB(0xa8, 0xb0, 0xbc), cd_pal[15] = CDW;
    for (y = 0; y < CD_T; y++)
        for (x = 0; x < CD_T; x++) {
            int32_t dx = 2 * x - 63, dy = 2 * y - 63, d2 = dx * dx + dy * dy, r = q_isqrt(d2) / 2;   /* (r in texels from the middle) */
            uint32_t a = cd_ang(dx, dy), c = 3;
            if (r >= 32 || r < 4) {
                cd_tex[y][x] = 0;                       /* outside, the hole */
                continue;
            }
            if (r >= 31)
                c = 1;                                  /* the silver rim */
            else if (r < 8)
                c = (x + y) & 1 ? 14 : 0;               /* the clear ring round the hole: see-through every other texel */
            else if (r < 10)
                c = 2;                                  /* the stacking ring */
            else if (r < 13)
                c = 4;                                  /* the label round the hub */
            else
                switch (art) {
                case 0: c = r > 27 ? 3 : 8 + (uint32_t)(r - 13) / 3u % 6u; break;                             /* rainbow rings */
                case 1: c = y > 23 && y < 40 ? (y == 24 || y == 39 ? 6 : 4) : y >= 40 && y < 44 ? 8 : 3; break;   /* a label band */
                case 2: c = (a >> 4) & 1u ? 8 + (a >> 5) % 3u : 3; break;                                       /* sunburst */
                case 3: c = ((x % 9 - 4) * (x % 9 - 4) + (y % 9 - 4) * (y % 9 - 4)) < 6 ? 8 + (uint32_t)(x / 9 + y / 9) % 4u : 3; break;   /* dots */
                case 4: c = y % 10 < 3 ? 8 + (uint32_t)(y / 10) % 6u : 3; break;                               /* stripes */
                case 5: c = x > 14 && x < 27 && y > 14 && y < 27 ? (x - 20 + y - 20 < 4 && x - 20 + y - 20 > -4 && x - y < 4 && y - x < 4 ? 9 : 8)
                          : y > 36 && y < 50 && (y & 1) ? 6 : 3; break;                                       /* the square and diamond, lines */
                case 6: { uint32_t h = (uint32_t)(x * 73 + y * 151) % 97u; c = h < 3 ? 15 : h < 6 ? 8 + h % 3u : 3; } break;   /* stars */
                case 7: c = 8 + (a / 11u + (uint32_t)r / 4u) % 4u; break;                                        /* holographic */
                case 8: c = y > 22 && y < 42 ? (((x >> 2) + (y >> 2)) & 1 ? 6 : 4) : 3; break;                  /* a checker band */
                default: c = (((a * 6u) >> 4) & 15u) < (uint32_t)(20 - r / 2 > 0 ? 20 - r / 2 : 0) / 2 ? 8 + (a * 6u >> 8) % 2u : 3; break;   /* petals */
                }
            cd_tex[y][x] = (uint8_t)c;
        }
    str_cpy(b, kind, sizeof b);                         /* the kind on top, the name below (one line, as on the tapes) */
    cd_text(32 - cd_tw(b) / 2, 15, b, 6);
    i = (uint32_t)cd_tw(nm);
    cd_text(32 - (int32_t)(i > 46u ? 46u : i) / 2, 44, nm, 7);
}
/* the disc drawn turning: centre cx, cy; diameter w0; yaw a (1024 a turn), spin s (the print round the hole, 1024 a turn); bs: block size.
 * The texture mapped linearly (the PS1 wobble), the far side darker in a 4 x 4 dither; a light line sweeps the face as it turns */
static void cd_quad(int32_t cx, int32_t cy, int32_t w0, uint32_t a, uint32_t s, int32_t shade, int32_t bs)
{
    static const uint8_t BY[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
    int32_t c = cd_cos(a), sn = cd_sin(a), hw = (w0 / 2) * c >> 15, zl = -(w0 / 2) * sn >> 15, zr = -zl, hl, hr, sx;
    int32_t sc = cd_cos(s), ss = cd_sin(s), flip = hw < 0, T = w0 / 40 + 1, sw = T * (sn < 0 ? -sn : sn) >> 15;
    if (flip)
        hw = -hw, zl = -zl, zr = -zr;
    hl = w0 * 220 / (220 + zl), hr = w0 * 220 / (220 + zr);
    if (sw > 0) {                                       /* the edge: a thin silver side beside the near edge */
        int32_t ex = zl < 0 ? cx - hw - sw : cx + hw, hn = zl < 0 ? hl : hr;
        cv_rect(ex, cy - hn / 3, sw + 1, hn * 2 / 3, cd_pal[2]);
    }
    if (hw < 2)
        return;
    for (sx = cx - hw; sx < cx + hw; sx += bs) {
        int32_t u = (sx - (cx - hw)) * 1024 / (2 * hw), h = hl + (hr - hl) * u / 1024, top = cy - h / 2, sy;
        int32_t dark = shade + (flip ? 2 : 0) + (u < 512 ? (zl > 0 ? zl / 6 : 0) : (zr > 0 ? zr / 6 : 0));
        int32_t fu = ((flip ? 1023 - u : u) - 512) * CD_T / 1024;
        for (sy = top; sy < top + h; sy += bs) {
            int32_t fv = (sy - top) * CD_T / (h ? h : 1) - CD_T / 2, tu = ((fu * sc - fv * ss) >> 15) + CD_T / 2, tv = ((fu * ss + fv * sc) >> 15) + CD_T / 2;
            uint32_t ti;
            uint16_t col;
            if ((uint32_t)tu >= CD_T || (uint32_t)tv >= CD_T || !(ti = cd_tex[tv][tu]))
                continue;
            col = cd_pal[ti];
            if (ti <= 2u)                               /* the silver catches a rainbow as it turns */
                col = cd_mix(col, cd_pal[8 + ((uint32_t)(sy + (int32_t)(a >> 4)) / 6u) % 6u], 6);
            if (((sx + sy - (int32_t)(a >> 2)) & 63) < 4)
                col = cd_mix(col, CDW, 7);              /* the light across the face */
            if (dark > 0 && BY[((sy / bs) & 3) * 4 + ((sx / bs) & 3)] < (uint32_t)(dark > 15 ? 15 : dark))
                col = cd_mix(col, CDK, 7);
            if (cd_ghost) {                             /* the ones behind: every other block, darker */
                if (((sx / bs) + (sy / bs)) & 1)
                    continue;
                col = cd_mix(col, CDK, 8);
            }
            cv_rect(sx, sy, bs, bs, col);
        }
    }
}
/* a change: called with the sound's new place in the list and which way it went */
static void cd_note(uint32_t pos, uint32_t total, int32_t dir)
{
    uint32_t now = fm1_ms ? fm1_ms : 1u;               /* (not fm1_ms | 1: one ms ahead, the age would wrap) */
    if (!total)
        return;
    cd_drum = (uint8_t)is_drum(TSEL);
    if (!cd_t0 || cd_n != total) {                      /* the screen opens: the carousel starts one CD away and rolls in */
        cd_open = now, cd_n = total;
        cd_cur = ((int32_t)pos - (dir < 0 ? -1 : 1)) * 256;
    }
    cd_tgt = (int32_t)pos * 256;
    if (cd_tgt - cd_cur > (int32_t)total * 128)        /* (round the end of the list: the short way) */
        cd_cur += (int32_t)total * 256;
    else if (cd_cur - cd_tgt > (int32_t)total * 128)
        cd_cur -= (int32_t)total * 256;
    cd_t0 = now;
    ui.force = 1;
}
static int cd_on(void)
{
    if (cd_t0 && fm1_ms - cd_t0 >= CD_HOLD + CD_OUT)
        cd_t0 = 0, ui.force = 1;
    return cd_t0 != 0;
}
static void cd_scene(void)                              /* one band of it (cv_oy set by the caller) */
{
    uint32_t now = fm1_ms, ag = now - cd_t0, op = now - cd_open, o;
    int32_t frac = cd_cur - cd_tgt, lift = 0, k;
    const char *kind, *nm;
    if (op < CD_IN)                                     /* up from below, fast then soft (expo-out) */
        k = 1024 - (int32_t)op * 1024 / CD_IN, lift = 150 * k / 1024 * k / 1024 * k / 1024;
    else if (ag > CD_HOLD)                              /* down again, faster and faster */
        k = (int32_t)(ag - CD_HOLD) * 1024 / CD_OUT, lift = 170 * k / 1024 * k / 1024;
    for (o = 0; o < 5u; o++) {                          /* far ones first: -2, 2, -1, 1, 0 */
        static const int8_t ORD[5] = {-2, 2, -1, 1, 0};
        int32_t d = ORD[o], p = d * 256 + frac, ap = p < 0 ? -p : p, x, cy, w0;
        uint32_t idx = (uint32_t)((cd_tgt / 256 + d) % (int32_t)cd_n + (int32_t)cd_n) % cd_n, a, s = 0;
        if (ap > 640)
            continue;
        x = 120 + p * 74 / 256;
        w0 = ap < 256 ? 124 - ap * 54 / 256 : 70 - (ap - 256) * 16 / 256;
        cy = 104 - ap * 10 / 256 + lift + (d ? lift / 3 * (o & 1u ? 1 : 2) : 0);
        if (ap < 40) {                                  /* the middle one: a whole turn as it arrives, then it faces you and spins */
            uint32_t t = now - cd_flip;
            a = t < 520u ? (uint32_t)(1024 - (int32_t)(520u - t) * (int32_t)(520u - t) / 264) & 1023u : 0u;   /* (ease out to front) */
            s = song.playing ? now * 3u / 4u : now / 6u;
        } else {
            a = (uint32_t)((p > 0 ? 1 : -1) * (ap > 256 ? 110 : ap * 110 / 256)) & 1023u;   /* the others turned toward the middle */
        }
        cd_make(idx);
        cd_ghost = (uint8_t)(ap >= 40);
        cd_quad(x, cy, w0, a, s & 1023u, ap / 90, ap < 40 ? 1 : 2);
        cd_ghost = 0;
    }
    nm = cd_name((uint32_t)(cd_tgt / 256) % cd_n, &kind);  /* the name, its kind, where in the list */
    {
        char b[24];
        int32_t y = 182 + lift;
        cd_bigo(120 - cd_bigw(nm, 2) / 2, y, nm, 2, CDW);
        str_cpy(b, kind, 12);
        str_cpy(b + str_len(b), "  ", 24 - str_len(b));
        fmt_int(b + str_len(b), cd_tgt / 256 % (int32_t)cd_n + 1);
        str_cpy(b + str_len(b), "/", 24 - str_len(b));
        fmt_int(b + str_len(b), (int32_t)cd_n);
        cd_bigo(120 - cd_bigw(b, 1) / 2, y + 34, b, 1, TE_COL[song.sel & 3u]);
    }
}
static void n_cd(void)                                  /* the whole screen: the CDs over black */
{
    static uint32_t last;
    uint32_t half;
    int32_t d = cd_tgt - cd_cur;
    if (fm1_ms - last < 45u) {                          /* the whole screen is ~77 ms of SPI: one picture every 45 ms at most, the rest of the loop is left to the sound and the keys */
        ui.force = 1;
        return;
    }
    last = fm1_ms;
    if (d > 128 || d < -128)
        cd_flip = fm1_ms;                               /* (still rolling: the middle one's turn starts when it lands) */
    cd_cur += d / 5 + (d > 0 ? 2 : d < 0 ? -2 : 0);   /* the carousel eases to the CD */
    if ((d > 0) != (cd_tgt - cd_cur > 0))
        cd_cur = cd_tgt;
    for (half = 0; half < 2u; half++) {
        int32_t y0 = (int32_t)(half * 120u);
        cv_begin(240, 120, CDK);
        cv_oy = -y0;
        cd_scene();
        cv_oy = 0;
        cv_blit(0, half * 120u);
    }
    ui.force = 1;
}
