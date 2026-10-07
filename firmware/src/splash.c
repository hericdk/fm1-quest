/* SPDX-License-Identifier: GPL-3.0-only */
/* SLOOP boot splash: the logo (tools/gen_logo.py -> sloop_logo.h: 16 colours, RLE runs of
 * (run - 1) << 4 | colour), drawn into the canvas in bands of at most 120 rows. */
#include "sloop_logo.h"
static void sloop_logo_draw(uint32_t y0)
{
    uint32_t r0;
    for (r0 = 0; r0 < SLOOP_SPLASH_H; r0 += 120u) {
        uint32_t rows = SLOOP_SPLASH_H - r0 > 120u ? 120u : SLOOP_SPLASH_H - r0;
        uint32_t lo = r0 * SLOOP_SPLASH_W, hi = (r0 + rows) * SLOOP_SPLASH_W, pos = 0, i;
        cv_begin(SLOOP_SPLASH_W, rows, C_BLACK);
        for (i = 0; i < sizeof SLOOP_SPLASH_RLE && pos < hi; i++) {
            uint32_t run = (SLOOP_SPLASH_RLE[i] >> 4) + 1u, c = SLOOP_SPLASH_RLE[i] & 15u, p;
            if (c && pos + run > lo)
                for (p = pos < lo ? lo : pos; p < pos + run && p < hi; p++)
                    cv_px[p - lo] = swap16(SLOOP_SPLASH_PAL[c]);
            pos += run;
        }
        cv_blit((240u - SLOOP_SPLASH_W) / 2u, y0 + r0);
    }
}

/* FM1 Quest title (README: TITLE): the illustration reduced to 240 x 240 in the scene palette, a dark gradient at the top and bottom,
 * the logo (~170 px, a 1-bit mask drawn cream with a dark outline), a "press play" panel and "based on Sloop". Drawn in two bands of
 * 120 rows with cv_oy so that everything is in screen coordinates. */
#include "felucca_qtitle.h"
static uint16_t q_dim565(uint16_t c, uint32_t f)           /* f / 256 of the brightness */
{
    uint32_t r = (c >> 11) * f / 256u, g = ((c >> 5) & 63u) * f / 256u, b = (c & 31u) * f / 256u;
    return (uint16_t)((r << 11) | (g << 5) | b);
}
static void sloop_splash(void)
{
    uint32_t band, x, y, ly, lx;
    int32_t lx0 = (240 - QTITLE_LOGO_W) / 2, ly0 = 7;
    lcd_fill(0, 0, 240, 240, Q_BG);
    for (band = 0; band < 2u; band++) {
        uint32_t y0 = band * 120u;
        cv_begin(240, 120, Q_BG);
        cv_oy = -(int32_t)y0;
        for (y = y0; y < y0 + 120u; y++) {
            uint32_t f = y < 91u ? 115u + (256u - 115u) * y / 91u : y > 149u ? 256u - 192u * (y - 149u) / 91u : 256u;
            for (x = 0; x < 240u; x++) {
                uint32_t i = y * 240u + x, v = (QTITLE_BG[i / 2u] >> ((i & 1u) ? 0 : 4)) & 15u;
                cv_pset((int32_t)x, (int32_t)y, f == 256u ? QPAL[v] : q_dim565(QPAL[v], f));
            }
        }
        for (ly = 0; ly < QTITLE_LOGO_H; ly++)                          /* the outline first, then the cream letters */
            for (lx = 0; lx < QTITLE_LOGO_W; lx++)
                if ((QTITLE_LOGO[ly * QTITLE_LOGO_ROW + lx / 8u] >> (7u - (lx & 7u))) & 1u) {
                    int32_t px = lx0 + (int32_t)lx, py = ly0 + (int32_t)ly, ox, oy;
                    for (oy = -1; oy <= 2; oy++)
                        for (ox = -1; ox <= 1; ox++) cv_pset(px + ox, py + oy, Q_BG);
                }
        for (ly = 0; ly < QTITLE_LOGO_H; ly++)
            for (lx = 0; lx < QTITLE_LOGO_W; lx++)
                if ((QTITLE_LOGO[ly * QTITLE_LOGO_ROW + lx / 8u] >> (7u - (lx & 7u))) & 1u)
                    cv_pset(lx0 + (int32_t)lx, ly0 + (int32_t)ly, Q_TEXT);
        q_panel(72, 192, 96, 22);
        q_tri(82, 200, Q_TEXT);
        q_text(94, 197, "press play", Q_TEXT);
        q_outline_text(120 - text_w(&FONT_Q, "based on Sloop") / 2, 221, "based on Sloop", Q_TEXT);
        cv_oy = 0;
        cv_blit(0, y0);
    }
    lcd_sync();
}
