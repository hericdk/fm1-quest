/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Small-canvas renderer (no full framebuffer). Draw text/lines into
 * an off-screen strip, then blit it in one DMA transfer. Pixels are stored
 * byte-swapped (the panel takes RGB565 big-endian). */
/* UI_SMALL: the screens' and the buttons' functions (panel.c, ui*.c, icons.c; never in the audio
 * path) are compiled for size (clang minsize, as -Oz). 2.7: the drums' SOUND page and the favourites
 * made the image overflow the flash (XIP); this gives it back with nothing taken out. */
#ifndef UI_SMALL
#if defined(__clang__)
#define UI_SMALL __attribute__((minsize))
#else
#define UI_SMALL
#endif
#endif
typedef struct {               /* proportional, see tools/gen_font.py */
    uint8_t h;
    uint8_t pad;               /* bitmap starts this many pixels left of the pen */
    uint8_t first, last;
    const uint8_t *adv;        /* advance per glyph */
    const uint8_t *bw;         /* bitmap width per glyph (starts FONT_PAD left of the pen) */
    const uint16_t *off;       /* byte offset of each glyph */
    const uint8_t *data;       /* the glyph bitmaps, or (gc != 0) the LZ4 chunks of tools/pack_gfx.py */
    const uint8_t *gc;         /* packed: the chunk of each glyph (off[] is then inside the chunk) */
    const uint16_t *zo;        /* packed: where each chunk starts in data (+ the end) */
} felucca_font_t;
#include "felucca_qfont.h"      /* FM1 Quest: FONT_Q (Pixelify Sans 8 px), FONT_QT (Jacquarda Bastarda 9, 16 px), FONT_P (Pixelify Sans 14 px) */
#define FONT_S FONT_P           /* the stock pages: Pixelify Sans 14 px (was Terminus 16 px) */
#define FONT_L FONT_P           /* (was Terminus 32 px: values and titles of the stock screens) */

#define CV_MAX (240u * 124u)      /* the graph strip is 240 x 124 */
static uint16_t cv_px[CV_MAX] __attribute__((section(".pool")));
static uint32_t cv_w, cv_h;
static int32_t cv_oy;            /* y offset for graph drawing */
static int32_t cv_ox;            /* x offset: a scene redraws only a window of the screen (ui_quest_pages.c) */

#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
static uint16_t pal_bg;                  /* SLOOP 2.7: the background, black unless a theme says (palette_set) */
#define C_BLACK pal_bg                   /* the background (and dark text on a light tile) */
static uint16_t pal_hi = 0xFFFFu;        /* the accent: white unless a theme says (palette_set) */
#define C_WHITE pal_hi               /* accent only: what is being touched / where we are */
/* The screen is five steps of one colour, darkest to brightest, plus white.
 * Palettes are picked in the HOME-hold menu (COLOR). */
typedef struct {
    const char *name;
    uint16_t c[5];
} palette_t;
/* FM1 Quest: one palette, TAVERN (README): rules, dim, labels, secondary text, values. The list keeps its five slots so that settings
 * saved by SLOOP (palette index 0..4) stay valid; every slot is the same palette, and the COLOR menu row no longer changes it. */
#define QTAV {RGB(0x5a, 0x3a, 0x28), RGB(0x7a, 0x60, 0x58), RGB(0xa8, 0x90, 0x80), RGB(0xc8, 0xb0, 0x90), RGB(0xf0, 0xc8, 0x60)}
static const palette_t PALETTES[] = {
    {"TAVERN", QTAV}, {"TAVERN", QTAV}, {"TAVERN", QTAV}, {"TAVERN", QTAV}, {"TAVERN", QTAV},
};
#define NPALETTES (sizeof(PALETTES) / sizeof(PALETTES[0]))
static uint16_t pal[5];
#define C_LINE pal[0]                /* 1 rules, separators */
#define C_DIM pal[1]                 /* 2 inactive, empty steps, units */
#define C_GRAY pal[2]                /* 3 labels */
#define C_AMB pal[3]                 /* 4 secondary text */
#define C_HI pal[4]                  /* 5 values, curves */

/* SLOOP 2.7: your own theme, made in the web editor (its Theme page): the five steps of the screen and the four
 * tracks' colours, with a name. MENU > COLOR shows it after MONO (theme_on: panel.c lights_word bit 30; the
 * palette number stays a built-in one, the one an older SLOOP shows). Kept in the settings object's side record
 * (project.c, storage.c st_side): an older SLOOP never reads it, and forgets it when it saves its settings. */
#define THEME_MAGIC 0x324D4854u                  /* "THM2" (2.7 beta 2: with the accent) */
#define THEME_MAGIC1 0x314D4854u                 /* "THM1": the first beta's, 36 bytes (its CRC where hi is now),
                                                  * taken with a white accent (black on a light background) */
typedef struct {
    uint32_t magic;
    char name[8];                                /* ASCII, 0-padded (not terminated when 8 long) */
    uint16_t c[5];                               /* the screen, faintest .. strongest (as a palette), RGB565 */
    uint16_t trk[4];                             /* the tracks' colours (knobs, tiles, meters), RGB565 */
    uint16_t bg;                                 /* the background, RGB565 (0 black: as before) */
    uint16_t hi;                                 /* the accent (what is touched, the cursor, the play head) */
    uint16_t rsv;                                /* 0 */
    uint32_t crc;                                /* CRC-32 (storage.c st_crc32) of the 36 bytes before it */
} theme_t;
_Static_assert(sizeof(theme_t) == 40u, "theme layout (the editor's Theme page writes these bytes)");
static theme_t theme;                            /* the theme there is (theme_valid: project.c theme_check passed) */
static uint8_t theme_valid, theme_on;
/* the tracks' colours: full, two thirds, one third (the built-in ones as they always were) */
static const uint16_t TE_COL_DEF[4] = {RGB(40, 124, 255), RGB(30, 204, 112), RGB(255, 198, 24), RGB(255, 98, 26)};
static const uint16_t TE_MID_DEF[4] = {RGB(26, 82, 170), RGB(20, 136, 76), RGB(170, 132, 16), RGB(170, 66, 18)};
static const uint16_t TE_DIM_DEF[4] = {RGB(14, 40, 86), RGB(10, 66, 38), RGB(86, 66, 8), RGB(86, 32, 8)};
static uint16_t TE_COL[4] = {RGB(40, 124, 255), RGB(30, 204, 112), RGB(255, 198, 24), RGB(255, 98, 26)};
static uint16_t TE_MID[4] = {RGB(26, 82, 170), RGB(20, 136, 76), RGB(170, 132, 16), RGB(170, 66, 18)};
static uint16_t TE_DIM[4] = {RGB(14, 40, 86), RGB(10, 66, 38), RGB(86, 66, 8), RGB(86, 32, 8)};
static uint16_t rgb565_scale(uint32_t c, uint32_t num, uint32_t den)
{
    uint32_t r = ((c >> 11) & 31u) * num / den, g = ((c >> 5) & 63u) * num / den, b = (c & 31u) * num / den;
    return (uint16_t)(r << 11 | g << 5 | b);
}
static uint32_t st_crc32(const void *p, uint32_t n);   /* storage.c (zlib CRC-32, as the editor's) */
/* a channel of an RGB565 colour on 0..255 (as the editor reads it: x * 8, x * 4, x * 8) */
static int32_t rgb565_ch(uint32_t c, uint32_t k)
{
    return k == 0u ? (int32_t)((c >> 11) & 31u) * 8 : k == 1u ? (int32_t)((c >> 5) & 63u) * 4 : (int32_t)(c & 31u) * 8;
}
/* one colour stands out from the background: one channel 96 or more apart */
static int theme_apart(uint32_t c, uint32_t bg)
{
    uint32_t k;
    for (k = 0; k < 3u; k++) {
        int32_t d = rgb565_ch(c, k) - rgb565_ch(bg, k);
        if (d >= 96 || d <= -96)
            return 1;
    }
    return 0;
}
/* a theme as the editor sends it and flash keeps it: its magic, its CRC, a name of printable ASCII, and the strongest
 * step and the accent standing out from the background (a screen whose values or menu cursor cannot be seen would hide
 * the way back to COLOR). A first-beta THM1 (36 bytes) is turned into a THM2 in *t */
static int theme_check(theme_t *t)
{
    uint32_t k;
    if (t->magic == THEME_MAGIC1) {
        if (((uint32_t)t->hi | (uint32_t)t->rsv << 16) != st_crc32(t, 32u))
            return 0;
        t->hi = theme_apart(0xFFFFu, t->bg) ? 0xFFFFu : 0u;
        t->rsv = 0;
        t->magic = THEME_MAGIC;
        t->crc = st_crc32(t, __builtin_offsetof(theme_t, crc));
    }
    if (t->magic != THEME_MAGIC || t->crc != st_crc32(t, __builtin_offsetof(theme_t, crc)))
        return 0;
    for (k = 0; k < sizeof t->name; k++)
        if (t->name[k] && (t->name[k] < 0x20 || t->name[k] > 0x7E))
            return 0;
    return theme_apart(t->c[4], t->bg) && theme_apart(t->hi, t->bg);
}
/* colour c at a / n over the background (antialiased text and icons) */
static uint16_t rgb565_over(uint32_t c, uint32_t a, uint32_t n)
{
    uint32_t b = pal_bg, r, g, bl;
    r = (((b >> 11) & 31u) * (n - a) + ((c >> 11) & 31u) * a) / n;
    g = (((b >> 5) & 63u) * (n - a) + ((c >> 5) & 63u) * a) / n;
    bl = ((b & 31u) * (n - a) + (c & 31u) * a) / n;
    return (uint16_t)(r << 11 | g << 5 | bl);
}
static int theme_shown(void) { return theme_on && theme_valid; }
/* (2.7) the greys of the TE-style pages (tiles, empty steps, labels, secondary text) are drawn for black: on a theme's
 * background, each the same share of the way from it toward white (a dark one) or black (a light one). On black: as
 * ever. te_gray() does it for any grey */
static const uint16_t TE_G_DEF[4] = {RGB(26, 26, 30), RGB(54, 54, 60), RGB(118, 118, 126), RGB(196, 196, 204)};
static uint16_t te_g[4] = {RGB(26, 26, 30), RGB(54, 54, 60), RGB(118, 118, 126), RGB(196, 196, 204)};
#define TE_G1 te_g[0]                    /* tiles */
#define TE_G2 te_g[1]                    /* empty steps, dial tracks */
#define TE_G3 te_g[2]                    /* labels */
#define TE_G4 te_g[3]                    /* secondary text */
static uint16_t te_gray(uint32_t c)
{
    uint32_t bg = pal_bg, k, out = 0;
    static const uint8_t SH[3] = {11, 5, 0}, MX[3] = {31, 63, 31};
    int light;
    if (!bg)
        return (uint16_t)c;
    light = ((bg >> 11) & 31u) * 2u * 299u + ((bg >> 5) & 63u) * 587u + (bg & 31u) * 2u * 114u > 31500u;
    for (k = 0; k < 3u; k++) {
        int32_t m = MX[k], b = (int32_t)((bg >> SH[k]) & (uint32_t)m), x = (int32_t)((c >> SH[k]) & (uint32_t)m);
        out |= (uint32_t)(b + ((light ? 0 : m) - b) * x / m) << SH[k];
    }
    return (uint16_t)out;
}

static void palette_set(uint32_t i)              /* built-in palette i, or the theme when it is chosen (2.7) */
{
    uint32_t k;
    int cu = theme_shown();
    for (k = 0; k < 5u; k++)
        pal[k] = cu ? theme.c[k] : PALETTES[0].c[k];          /* FM1 Quest: TAVERN, or the theme (2.7) */
    pal_bg = cu ? theme.bg : 0u;
    pal_hi = cu ? theme.hi : 0xFFFFu;
    for (k = 0; k < 4u; k++)
        te_g[k] = te_gray(TE_G_DEF[k]);
    for (k = 0; k < 4u; k++) {
        TE_COL[k] = cu ? theme.trk[k] : TE_COL_DEF[k];
        TE_MID[k] = cu ? rgb565_scale(theme.trk[k], 2u, 3u) : TE_MID_DEF[k];
        TE_DIM[k] = cu ? rgb565_scale(theme.trk[k], 1u, 3u) : TE_DIM_DEF[k];
    }
}

static inline uint16_t swap16(uint32_t c) { return (uint16_t)(((c >> 8) & 0xFFu) | ((c & 0xFFu) << 8)); }

static void cv_begin(uint32_t w, uint32_t h, uint16_t bg)
{
    uint32_t i, n;
    if (w * h > CV_MAX)
        h = CV_MAX / w;
    lcd_sync();                     /* the last blit may still read cv_px */
    cv_w = w;
    cv_h = h;
    n = w * h;
    for (i = 0; i < n; i++)
        cv_px[i] = swap16(bg);
}

static volatile uint32_t fm1_ms;     /* (core.h) */
/* ---- screen transitions: a mask applied to the canvas on its way to the LCD (the screens redraw whole while one runs) ----
 * 1 slide (the new screen comes in from the right), 2 pixelate (blocks swell and fall back), 3 dither (a halftone fills in) */
static uint8_t tr_kind;
static uint32_t tr_t0, tr_dur;
static void tr_start(uint32_t kind, uint32_t dur) { tr_kind = (uint8_t)kind, tr_t0 = fm1_ms, tr_dur = dur; }
static void tr_apply(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t *px)
{
    uint32_t el = fm1_ms - tr_t0, p, ry, cx;
    static const uint8_t BY[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
    if (el >= tr_dur) return;                                          /* (ui_draw ends it and forces one clean draw: ending it here would leave bands half-masked) */
    p = el * 1024u / tr_dur;
    if (tr_kind == 1u) {                                              /* slide: done in cv_blit (the new picture is laid over the old one from the right) */
    } else if (tr_kind == 2u) {                                       /* pixelate: blocks 1 -> 16 -> 1 */
        uint32_t b = 1u + (p < 512u ? p : 1024u - p) * 15u / 512u;
        if (b < 2u) return;
        for (ry = 0; ry < h; ry++) {
            uint32_t sr = ry - (y + ry) % b;
            if ((int32_t)sr < 0) sr = 0;
            for (cx = 0; cx < w; cx++) {
                uint32_t sc = cx - (x + cx) % b;
                if ((int32_t)sc < 0) sc = 0;
                px[ry * w + cx] = px[sr * w + sc];
            }
        }
    } else if (tr_kind == 4u) {                                       /* tape: the rows tear sideways, a tracking band rolls down, it settles */
        uint32_t amp = (1024u - p) * 22u / 1024u, band = (p * 3u * 240u / 1024u) % 260u;
        uint16_t tmp[240];
        for (ry = 0; ry < h; ry++) {
            uint32_t gy = y + ry, hs = (gy * 2654435761u + (fm1_ms / 45u) * 40503u) >> 22;
            int32_t sh = amp ? (int32_t)(hs % (2u * amp + 1u)) - (int32_t)amp : 0, d = (int32_t)gy - (int32_t)band;
            uint16_t *row = px + ry * w;
            if (d >= 0 && d < 14) sh += (int32_t)(1024u - p) * 28 / 1024;
            if (!sh) continue;
            for (cx = 0; cx < w && cx < 240u; cx++) tmp[cx] = row[cx];
            for (cx = 0; cx < w && cx < 240u; cx++) { int32_t sc = (int32_t)cx - sh; row[cx] = sc >= 0 && sc < (int32_t)w ? tmp[sc] : 0; }
            if (d >= 0 && d < 14 && !(gy & 1u)) for (cx = 0; cx < w; cx++) row[cx] = (uint16_t)((row[cx] >> 1) & 0x7BEFu);   /* the band is dimmer */
        }
    } else {                                                          /* dither: a 4 x 4 halftone fills in */
        uint32_t lv = p * 17u / 1024u;
        for (ry = 0; ry < h; ry++)
            for (cx = 0; cx < w; cx++)
                if (BY[((y + ry) & 3u) * 4u + ((x + cx) & 3u)] >= lv) px[ry * w + cx] = 0;
    }
}
static uint32_t cv_ban_h;                          /* a banner holds the top rows of the screen: nothing else paints there (no flicker) */
static void cv_blit(uint32_t x, uint32_t y)
{
    uint32_t skip = y < cv_ban_h ? cv_ban_h - y : 0u;
    if (skip >= cv_h) return;
    if (tr_kind == 1u && x == 0u && cv_w == 240u && fm1_ms - tr_t0 < tr_dur) {   /* slide: the new picture comes in from the right edge over the old one, which stays under it */
        uint32_t el = fm1_ms - tr_t0, p = el * 1024u / tr_dur, sh = (1024u - p) * 240u / 1024u, vw = 240u - sh, ry;
        if (sh && vw) {
            for (ry = skip; ry < cv_h; ry++) { uint32_t cx; uint16_t *src = cv_px + ry * 240u, *dst = cv_px + ry * vw; for (cx = 0; cx < vw; cx++) dst[cx] = src[cx]; }
            for (ry = skip; ry < cv_h; ry++) { uint32_t cx; uint16_t *dst = cv_px + (ry - skip) * vw, *src = cv_px + ry * vw; if (skip) for (cx = 0; cx < vw; cx++) dst[cx] = src[cx]; }
            lcd_blit(sh, y + skip, vw, cv_h - skip, cv_px);
            return;
        }
    }
    if (tr_kind) tr_apply(x, y + skip, cv_w, cv_h - skip, cv_px + skip * cv_w);
    lcd_blit(x, y + skip, cv_w, cv_h - skip, cv_px + skip * cv_w);
}

/* canvas rows r0 .. cv_h-1 only, to screen row y + r0 */
static void cv_blit_from(uint32_t x, uint32_t y, uint32_t r0)
{
    if (y + r0 < cv_ban_h) r0 = cv_ban_h - y;
    if (r0 < cv_h) {
        if (tr_kind) tr_apply(x, y + r0, cv_w, cv_h - r0, cv_px + r0 * cv_w);
        lcd_blit(x, y + r0, cv_w, cv_h - r0, cv_px + r0 * cv_w);
    }
}

static inline void cv_pset(int32_t x, int32_t y, uint16_t c)
{
    y += cv_oy;
    x += cv_ox;
    if ((uint32_t)x < cv_w && (uint32_t)y < cv_h)
        cv_px[(uint32_t)y * cv_w + (uint32_t)x] = swap16(c);
}

static void cv_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    int32_t x1 = x + w + cv_ox, y1 = y + h + cv_oy, i;
    uint16_t sc = swap16(c);
    y += cv_oy;                             /* clipped once, then filled row by row */
    x += cv_ox;
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (x1 > (int32_t)cv_w)
        x1 = (int32_t)cv_w;
    if (y1 > (int32_t)cv_h)
        y1 = (int32_t)cv_h;
    for (; y < y1; y++)
        for (i = x; i < x1; i++)
            cv_px[(uint32_t)y * cv_w + (uint32_t)i] = sc;
}

static void cv_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
{
    int32_t dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int32_t dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy, guard = 2000;
    while (guard--) {                       /* bounded: a line is never longer than 480 px */
        int32_t e2 = 2 * err;               /* both tests use the same error value */
        cv_pset(x0, y0, c);
        if (x0 == x1 && y0 == y1)
            break;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* glyph index of a character: lower case folds to upper case when the font
 * has none, anything missing (and C1 controls) draws as '?' */
static uint32_t glyph(const felucca_font_t *f, uint32_t ch)
{
    if (ch >= 'a' && ch <= 'z' && f->last < 'a')
        ch -= 32u;
    if (ch < f->first || ch > f->last || (ch >= 127u && ch < 160u))
        ch = '?';
    return ch - f->first;
}

/* packed graphics (tools/pack_gfx.py): LZ4 chunks of whole glyphs / sprites; the chunk of the glyph being drawn stays unpacked in pk_fbuf */
static void lz4_unpack(const uint8_t *s, const uint8_t *e, uint8_t *d)
{
    while (s < e) {
        uint32_t t = *s++, l = t >> 4, m, b;
        if (l == 15u)
            do { b = *s++; l += b; } while (b == 255u);
        while (l--)
            *d++ = *s++;
        if (s >= e)
            break;
        m = s[0] | (uint32_t)s[1] << 8;
        s += 2;
        {
            const uint8_t *r = d - m;
            m = t & 15u;
            if (m == 15u)
                do { b = *s++; m += b; } while (b == 255u);
            for (m += 4u; m--;)
                *d++ = *r++;
        }
    }
}
static uint8_t pk_fbuf[1152];
static const felucca_font_t *pk_fnt;
static uint32_t pk_fck;
static const uint8_t *glyph_bits(const felucca_font_t *f, uint32_t gi)
{
    uint32_t c;
    if (!f->gc)
        return f->data + f->off[gi];
    c = f->gc[gi];
    if (pk_fnt != f || pk_fck != c) {
        lz4_unpack(f->data + f->zo[c], f->data + f->zo[c + 1u], pk_fbuf);
        pk_fnt = f, pk_fck = c;
    }
    return pk_fbuf + f->off[gi];
}

/* text, alpha-blended onto the background with colour c; returns the end x */
static int32_t cv_text(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    uint16_t ramp[16];
    uint32_t a;
    for (a = 0; a < 16u; a++)
        ramp[a] = rgb565_over(c, a, 15u);              /* (on black: c * a / 15, as before) */
    uint32_t bold = f == &FONT_QT;                       /* the title font (Jacquarda) is shown bold: every glyph twice, one pixel apart */
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), gx, gy, w, bpr;
        const uint8_t *gd;
        w = f->bw[gi];
        bpr = (w + 1u) / 2u;
        gd = glyph_bits(f, gi);
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t v = gd[gy * bpr + gx / 2u];
                v = (gx & 1u) ? (v & 15u) : (v >> 4);
                if (v) {
                    cv_pset(x - f->pad + (int32_t)gx, y + (int32_t)gy, ramp[v]);
                    if (bold) cv_pset(x - f->pad + (int32_t)gx + 1, y + (int32_t)gy, ramp[v]);
                }
            }
        x += f->adv[gi] + (int32_t)bold;
    }
    return x;
}

static int32_t text_w(const felucca_font_t *f, const char *s)
{
    int32_t w = 0;
    for (; *s; s++)
        w += f->adv[glyph(f, (uint8_t)*s)] + (f == &FONT_QT);
    return w;
}

/* one-shot: text in a box, cleared to black, blitted */
static void draw_text_box(uint32_t x, uint32_t y, uint32_t w, const felucca_font_t *f, const char *s,
                          uint16_t c, int align)
{
    int32_t tw = text_w(f, s), tx = 0;
    cv_begin(w, f->h, C_BLACK);
    if (align == 1)
        tx = ((int32_t)w - tw) / 2;
    else if (align == 2)
        tx = (int32_t)w - tw;
    cv_text(tx, 0, f, s, c);
    cv_blit(x, y);
    lcd_sync();                     /* one-shots (boot, crash, UBOOT, update) finish here */
}
