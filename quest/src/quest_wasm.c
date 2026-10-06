/* SPDX-License-Identifier: GPL-3.0-only
 *
 * FM1 Quest: the web emulator's firmware build plus a read-only state export for
 * the RPG skin. sloop_wasm.c (sabliran/sloop-web-emu) is included UNCHANGED; the
 * firmware sources it pulls in are unchanged too. This file only appends exports
 * that copy firmware state into a plain int32 array the page can read.
 *
 * Every field is read straight from the firmware's own variables:
 *   song / trk[]            seq/mixer state            (core.h)
 *   ui                      which screen is up         (ui.c)
 *   rec_wait/ft_on/ci_on    REC armed / take / count-in (seq.c)
 *   clk_beat/clk_pos        the transport clock         (seq.c)
 */
#include "sloop_wasm.c"

#define Q_N 512
#define Q_BN 4096
static int32_t quest_s[Q_N];
static uint8_t quest_b[Q_BN];           /* strings and small tables: layout in web/quest.js (QB) */
static char quest_txt[NTRK][2][20];     /* per track: short sound name, engine/kit name */
static char quest_kit[20];

/* screen codes: the same decision order as ui_draw() in ui_draw.c */
enum { QS_PAGE = 0, QS_TRACKS = 1, QS_REC_READY = 2, QS_COUNT_IN = 3, QS_TAKE = 4, QS_DRUM = 5,
       QS_LAYER = 6, QS_MENU = 7, QS_SONG = 8, QS_HOLD = 9 };


/* ---------------------------------------------------------------- byte buffer layout */
#define QB_SUB 0            /* layer sub title, 24 */
#define QB_TILES 32         /* 16 x 16: lab[8], bg, fg, top (u16 LE), marks, pad */
#define QB_DIALS 288        /* 4 x 32: lab[10], val[12], ratio i16 at +22 */
#define QB_COLS 416         /* 4 x 40: label[12], val[12], unit[8], ratio i16 at +32, vc u16 at +34 */
#define QB_FOOT 576         /* ename[12], pn[20], ti[20] */
#define QB_LANES 640        /* 16 x 8 LANE_SHORT */
#define QB_KIT 768          /* kit name[20], kit style[16] at +20 */
#define QB_GRID 832         /* 16 lanes x 16 steps of the bank: bit0 has, bits1-2 level, bits3-4 ratchet */
#define QB_DDIALS 1100      /* the drum screen's 4 dials, as QB_DIALS */
#define QB_MENU 1240        /* 10 rows x 36: name[24], value[12] */
#define QB_BANK 1620        /* 12 x 24: name[13], kind[6], user flag, pad */
#define QB_MISC 1920        /* palette name[12] */
static void qb_str(uint32_t off, const char *s, uint32_t max)
{
    uint32_t i = 0;
    while (s && s[i] && i + 1u < max) { quest_b[off + i] = (uint8_t)s[i]; i++; }
    while (i < max) quest_b[off + i++] = 0;
}
static void qb_u16(uint32_t off, uint32_t v) { quest_b[off] = (uint8_t)v; quest_b[off + 1] = (uint8_t)(v >> 8); }

static void quest_cap_layer(uint32_t layer, const char *sub, uint16_t col, const void *tlv, const char *const *lab, const char *const *val, const int32_t *ratio)
{
    const tile_t *tl = tlv;
    uint32_t i;
    (void)layer; (void)col;
    qb_str(QB_SUB, sub, 24);
    for (i = 0; i < 16u; i++) {
        uint32_t o = QB_TILES + i * 16u;
        qb_str(o, tl[i].lab, 8);
        qb_u16(o + 8, tl[i].bg); qb_u16(o + 10, tl[i].fg); qb_u16(o + 12, tl[i].top);
        quest_b[o + 14] = tl[i].marks;
    }
    for (i = 0; i < 4u; i++) {
        uint32_t o = QB_DIALS + i * 32u;
        qb_str(o, lab[i], 10); qb_str(o + 10, val[i], 12); qb_u16(o + 22, (uint32_t)(ratio[i] & 0xFFFF));
    }
}
static void quest_cap_col(uint32_t c, const char *label, const char *val, const char *unit, int32_t ratio, uint16_t vc)
{
    uint32_t o = QB_COLS + (c & 3u) * 40u;
    qb_str(o, label, 12); qb_str(o + 12, val, 12); qb_str(o + 24, unit, 8);
    qb_u16(o + 32, (uint32_t)(ratio & 0xFFFF)); qb_u16(o + 34, vc);
}
static void quest_cap_foot(const char *ename, const char *pn, const char *ti)
{
    qb_str(QB_FOOT, ename, 12); qb_str(QB_FOOT + 12, pn, 20); qb_str(QB_FOOT + 32, ti, 20);
}

/* what the firmware's own screens compute from state (no draw call to hook): the drum screen,
 * the menu, the preset window, the free take. Transcribed from ui_studio.c / ui_menu.c / ui.c. */
static void quest_extra(void)
{
    uint32_t i, j, n = 256;
    uint32_t len = (uint32_t)clamp(TDRUM->p[P_SLEN], 1, 64), kit = drum_kit(), bank = (drum_cursor % len) / 16u;
    /* menu */
    quest_s[n++] = settings.lowcut; quest_s[n++] = settings.zoom; quest_s[n++] = lights_lvl; quest_s[n++] = lights_keys;
    quest_s[n++] = lights_notes; quest_s[n++] = usb_full; quest_s[n++] = settings.palette;
    n = 264;                                              /* colour constants (the same macros the firmware draws with) */
    quest_s[n++] = C_WHITE; quest_s[n++] = C_BLACK; quest_s[n++] = TE_G1; quest_s[n++] = TE_G2; quest_s[n++] = TE_G3; quest_s[n++] = TE_G4;
    quest_s[n++] = TE_RED; quest_s[n++] = TE_DRUM;
    for (i = 0; i < 4u; i++) quest_s[n++] = TE_COL[i];
    for (i = 0; i < 4u; i++) quest_s[n++] = TE_DIM[i];
    for (i = 0; i < 4u; i++) quest_s[n++] = TE_MID[i];     /* 264..283 */
    n = 288;
    {   uint32_t total, cur = preset_pos(&total), k = 0, np = 0;
        quest_s[n++] = (int32_t)cur; quest_s[n++] = (int32_t)total;
        for (i = 0; i < NPAGES; i++) if (PAGES[i].fam == cur_page()->fam) { np++; if (i == ui.page) k = np; }
        quest_s[n++] = (int32_t)np; quest_s[n++] = (int32_t)k;        /* 290 pages in the family, 291 this one */
        quest_s[n++] = (int32_t)NBANK;
    }
    n = 296;
    quest_s[n++] = (int32_t)ft_t; { uint32_t b = 0, bp = 0; b = ft_on ? ft_fit(ft_t, &bp) : 0u; quest_s[n++] = (int32_t)b; quest_s[n++] = (int32_t)bp; }
    quest_s[n++] = ft_trk; quest_s[n++] = (int32_t)(CTL); quest_s[n++] = (int32_t)(FS);   /* 296 ft_t, 297 bars, 298 bpm, 299 trk, 300 CTL, 301 FS */
    quest_s[n++] = punch.req; quest_s[n++] = 0;
    {   uint32_t lit = 0; for (i = 0; i < DRUM_LANES; i++) if (pad_lit[i]) lit |= 1u << i; quest_s[n++] = (int32_t)lit; }   /* 304 */
    /* drum screen */
    for (i = 0; i < DRUM_LANES; i++) qb_str(QB_LANES + i * 8u, LANE_SHORT[i], 8);
    qb_str(QB_KIT, DRUM_KIT_NAMES[kit], 20); qb_str(QB_KIT + 20, DRUM_KIT_STYLES[kit], 16);
    for (i = 0; i < DRUM_LANES; i++)
        for (j = 0; j < 16u; j++) {
            uint32_t p = bank * 16u + j; uint8_t v = 0;
            if (p < len) { const dstep_t *s = &TDRUM->dstep[p]; if (dstep_has(s, i)) v = (uint8_t)(1u | dstep_lvl(s, i) << 1 | dstep_rat(s, i) << 3); else v = 0; v |= (uint8_t)(p < len ? 0x80u : 0u); }
            quest_b[QB_GRID + i * 16u + j] = v;
        }
    {   static char v[4][12]; static const char *const LG[4] = {"sound", "step", "hit", "level"}, *const LK[4] = {"kit", "level", "reverb", "pan"};
        int32_t ratio[4]; const dstep_t *s = &TDRUM->dstep[drum_cursor % len]; const char *const *lab = drum_page ? LK : LG;
        if (!drum_page) {
            str_cpy(v[0], LANE_SHORT[drum_lane], 8); fmt_int(v[1], drum_cursor + 1);
            str_cpy(v[2], dstep_has(s, drum_lane) ? "on" : "--", 4); str_cpy(v[3], dstep_has(s, drum_lane) ? LV_NAME[dstep_lvl(s, drum_lane)] : "--", 8);
            ratio[0] = (int32_t)drum_lane * 1000 / (DRUM_LANES - 1); ratio[1] = (int32_t)drum_cursor * 1000 / (int32_t)(len > 1u ? len - 1u : 1u);
            ratio[2] = v[2][0] == 'o' ? 1000 : 0; ratio[3] = dstep_has(s, drum_lane) ? (int32_t)((dstep_lvl(s, drum_lane) + 1u) % 4u) * 333 : 0;
        } else {
            fmt_int(v[0], (int32_t)kit + 1); fmt_int(v[1], song.g[G_DRLVL] * 100 / 127); fmt_int(v[2], song.g[G_DRREV] * 100 / 127); fmt_int(v[3], TDRUM->p[P_PAN]);
            ratio[0] = (int32_t)kit * 1000 / (int32_t)(DRUM_KITS - 1u); ratio[1] = song.g[G_DRLVL] * 1000 / 127; ratio[2] = song.g[G_DRREV] * 1000 / 127; ratio[3] = (TDRUM->p[P_PAN] + 64) * 1000 / 127;
        }
        for (i = 0; i < 4u; i++) { uint32_t o = QB_DDIALS + i * 32u; qb_str(o, lab[i], 10); qb_str(o + 10, v[i], 12); qb_u16(o + 22, (uint32_t)(ratio[i] & 0xFFFF)); }
    }
    /* menu rows */
    for (i = 0; i < MI_COUNT; i++) {
        const char *val = "";
        if (i == MI_LOWCUT) val = settings.lowcut ? "ON" : "OFF"; else if (i == MI_ZOOM) val = settings.zoom ? "ON" : "OFF"; else if (i == MI_NOTES) val = lights_notes ? "ON" : "OFF";
        else if (i == MI_LIGHTS) val = LIGHTS_NAME[lights_lvl % LIGHTS_N]; else if (i == MI_USB) val = usb_full ? "FULL" : "MASTER";
        else if (i == MI_KEYS) val = KEYS_NAME[lights_keys % KEYS_N]; else if (i == MI_COLOR) val = PALETTES[settings.palette].name;
        qb_str(QB_MENU + i * 36u, MI_NAME[i], 24); qb_str(QB_MENU + i * 36u + 24u, val, 12);
    }
    qb_str(QB_MISC, PALETTES[settings.palette].name, 12);
    { char nb[8]; note_name(nb, pen_note[0]); qb_str(QB_MISC + 12, nb, 8); }
    for (i = 0; i < 6u; i++) qb_str(QB_MISC + 40u + i * 8u, N_DIV[i], 8);
    for (i = 0; i < NTRK; i++) { char sb[8]; swing_str(sb, trk[i].p[P_SSWING]); qb_str(QB_MISC + 96u + i * 8u, sb, 8); }
    { char sb[8]; swing_str(sb, song.g[G_SWING]); qb_str(QB_MISC + 128u, sb, 8); }
    quest_s[310] = pen_lane; quest_s[311] = pen_note[0];
    /* the preset list window around the selected track's preset (BANK, ui.c): 12 entries */
    {   uint32_t total, cur = preset_pos(&total), first = cur / 12u * 12u;
        for (i = 0; i < 12u; i++) {
            uint32_t idx = first + i, o = QB_BANK + i * 24u;
            if (idx < NBANK) { qb_str(o, BANK[idx].name, 13); qb_str(o + 13, BANK_KIND[BANK[idx].kind], 6); quest_b[o + 19] = 0; }
            else if (idx < total) { char b[16]; uint32_t k; preset_at(idx, &k); up_name(k, b); qb_str(o, b, 13); qb_str(o + 13, "USER", 6); quest_b[o + 19] = 1; }
            else { qb_str(o, "", 13); qb_str(o + 13, "", 6); }
        }
        quest_s[n++] = (int32_t)first;                      /* 306 */
    }
}

WASM_EXPORT("quest_snap") void quest_snap(void)
{
    uint32_t i, j, n = 0;
    int32_t scr;
    if (!ui.menu && (ui.layer != LY_PLAY || ui.hold_kind))
        scr = ui.hold_kind ? QS_HOLD : QS_LAYER;
    else if ((rec_wait || ft_on) && !ui.menu && !on_song_page())
        scr = ft_on ? QS_TAKE : ci_on ? QS_COUNT_IN : QS_REC_READY;
    else if (!ui.menu && on_song_page())
        scr = QS_SONG;
    else if (ui.menu)
        scr = QS_MENU;
    else if (!ui.home && cur_page()->scope == SC_TRK)
        scr = QS_TRACKS;
    else if (on_drum_page())
        scr = QS_DRUM;
    else
        scr = QS_PAGE;
    quest_s[n++] = 1;                         /* 0  layout version */
    quest_s[n++] = scr;                       /* 1  screen */
    quest_s[n++] = ui.layer;                  /* 2  LY_* */
    quest_s[n++] = ui.hold_kind;              /* 3 */
    quest_s[n++] = ui.menu;                   /* 4  0 off, 1 list, 2 about */
    quest_s[n++] = ui.menu_sel;               /* 5 */
    quest_s[n++] = ui.home;                   /* 6 */
    quest_s[n++] = ui.page;                   /* 7  index into PAGES */
    quest_s[n++] = cur_page()->fam;           /* 8  FAM_* */
    quest_s[n++] = song.playing;              /* 9 */
    quest_s[n++] = song.sel;                  /* 10 */
    quest_s[n++] = song.rec;                  /* 11 bit per track */
    quest_s[n++] = song.solo;                 /* 12 bit per track */
    quest_s[n++] = song.g[G_BPM];             /* 13 */
    quest_s[n++] = song.g[G_SWING];           /* 14 */
    quest_s[n++] = (int32_t)clk_beat;         /* 15 */
    quest_s[n++] = (int32_t)clk_pos;          /* 16 clock units into the beat (BEAT_U per beat) */
    quest_s[n++] = rec_wait;                  /* 17 */
    quest_s[n++] = ft_on;                     /* 18 */
    quest_s[n++] = ci_on;                     /* 19 */
    quest_s[n++] = ci_beat;                   /* 20 */
    quest_s[n++] = rec_count;                 /* 21 */
    quest_s[n++] = rec_tempo;                 /* 22 */
    quest_s[n++] = (int32_t)ui.frame;         /* 23 */
    quest_s[n++] = (int32_t)BEAT_U;           /* 24 */
    quest_s[n++] = drum_page;                 /* 25 0 GRID, 1 KIT */
    quest_s[n++] = drum_cursor;               /* 26 */
    quest_s[n++] = drum_lane;                 /* 27 */
    quest_s[n++] = (int32_t)drum_kit();       /* 28 */
    quest_s[n++] = ui.step_page;              /* 29 */
    quest_s[n++] = song.g[G_DRLVL];           /* 30 */
    quest_s[n++] = project_empty();           /* 31 */
    n = 32;
    for (i = 0; i < NTRK; i++) {              /* 32 + 24 * i */
        track_t *t = &trk[i];
        uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, 64);
        quest_s[n++] = t->eng_req;
        quest_s[n++] = t->preset;
        quest_s[n++] = i == TRK_DRUM ? song.g[G_DRLVL] : t->p[P_LEVEL];
        quest_s[n++] = t->p[P_MUTE];
        quest_s[n++] = t->p[P_PAN];
        quest_s[n++] = (int32_t)len;
        quest_s[n++] = t->seq_idx;
        quest_s[n++] = trk_silent(t);
        quest_s[n++] = t->p[P_ROOT];
        quest_s[n++] = t->p[P_SCALE];
        quest_s[n++] = t->p[P_DIST];
        quest_s[n++] = t->p[P_CHOR];
        quest_s[n++] = t->p[P_DLY];
        quest_s[n++] = t->p[P_REV];
        quest_s[n++] = t->p[P_TRANS];
        quest_s[n++] = (int32_t)(t->seq_abs);
        quest_s[n++] = t->p[P_SDIV];
        quest_s[n++] = t->p[P_SSWING];
        n += 6;                               /* spare */
    }
    n = 160;
    for (i = 0; i < NTRK; i++) {              /* 160 + 2 * i: the 64 steps, bit per step (1 = something plays) */
        uint32_t lo = 0, hi = 0;
        for (j = 0; j < NSTEP; j++)
            if (trk_step_on(&trk[i], j)) {
                if (j < 32u) lo |= 1u << j; else hi |= 1u << (j - 32u);
            }
        quest_s[n++] = (int32_t)lo;
        quest_s[n++] = (int32_t)hi;
    }
    n = 176;
    for (i = 0; i < G_COUNT && n < Q_N; i++)  /* 176 + i: every global parameter */
        quest_s[n++] = song.g[i];
    for (i = 0; i < NTRK; i++) {
        char b[24];
        if (i == TRK_DRUM) {
            str_cpy(quest_txt[i][0], DRUM_KIT_NAMES[drum_kit()], 20);
            str_cpy(quest_txt[i][1], "drums", 20);
        } else {
            trk_short_name(i, b);
            str_cpy(quest_txt[i][0], b, 20);
            str_cpy(quest_txt[i][1], ENGINES[trk[i].eng_req % NENGINES]->name, 20);
        }
    }
    quest_extra();
}
WASM_EXPORT("quest_ptr") uint32_t quest_ptr(void) { return (uint32_t)(uintptr_t)quest_s; }
WASM_EXPORT("quest_b_ptr") uint32_t quest_b_ptr(void) { return (uint32_t)(uintptr_t)quest_b; }
WASM_EXPORT("quest_txt_ptr") uint32_t quest_txt_ptr(void) { return (uint32_t)(uintptr_t)quest_txt; }
