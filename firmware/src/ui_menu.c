/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* SLOOP menu (HOME held): IMPORTER (the user samples and songs: see, erase), LOWCUT, ZOOM, LIGHTS, KEYS, NOTES, USB AUDIO, HARDWARE CALIBRATION, ABOUT. */
/* ------------------------------------------------------------ menu --- */
enum { MI_VIZ, MI_PARTY, MI_GAMES, MI_LED, MI_FILTER, MI_IMPORT, MI_STATUS, MI_LOWCUT, MI_USB, MI_BOOT, MI_ULT, MI_DENS, MI_DMG, MI_BSPD, MI_ABOUT, MI_COUNT };     /* the visualizers, the rhythms and the keyboard LEDs come first */
static const char *const MI_NAME[MI_COUNT] = {"VISUALIZER", "PARTY", "GAMES", "KEYBOARD LED", "FILTER", "IMPORTER", "STATUS", "LOWCUT", "USB AUDIO", "BOOT SOUND", "ULT ANIMATION", "MAP ENEMIES", "DAMAGE NUMBERS", "BOSS SPEED", "ABOUT"};
/* SOUNDS: PRESETS browses every sound of the selected hero (the synth bank, the drum kits), OCT+ plays / pauses it with a short phrase, the other
 * tracks are soloed out while it plays; SEL keeps it on the hero's track, leaving puts back the sound, the solo and the transport */
static uint16_t sx_i, sx_n, sx_com, sx_np, sx_perc;                              /* the highlighted one, the count, the one the track has */
static uint8_t qf_sel;                                          /* FILTER: the highlighted one (previewed, not kept until confirmed) */
static uint8_t sx_on, sx_open, sx_resume, sx_eng, sx_pre, sx_usr, sx_note, sx_solo;
static uint32_t sx_t0, sx_st, sx_pend;                           /* sx_pend: when the knob last moved (the sound starts when it rests) */
static int32_t sx_cur, sx_tgt;                                     /* the tape strip, 1/256 of a tape: where it is and where it goes */
static int16_t sx_p[P_COUNT];
static uint8_t sx_hn[8], sx_ho[8];                              /* the notes sounding and the step they end on */
static uint32_t led_demo_t0;                                  /* KEYBOARD LED: a turn of a knob plays the effect once (ui_input.c ui_leds) */
#define LED_NFX 6u
#define MI_DY 18                                   /* rows between two menu lines */

static void stage_knobs(void);                                    /* ui_quest_stage.c: the Party menu's knobs */
static UI_SMALL void enc_drop(void)                             /* knob turns nobody takes */
{
    uint32_t k;
    for (k = 0; k < NE; k++)
        panel_enc(k);
}


/* ---- IMPORTER (menu -> IMPORTER): 3 sample slots, 6 Piano Hero songs, 6 FM Hero songs, loaded from the web editor.
 * PRESETS moves, REC held 1.3 s erases the one selected (the ring fills; let go before and nothing happens), OCT- back. */
#ifndef FELUCCA_VIZ_FULL
#define FELUCCA_VIZ_FULL 0
#endif
#define PV_N 5u             /* visualizer pages (ui_quest_pages.c) */
static void gm_input(uint32_t ok, uint32_t back, int32_t s);   /* (ui_quest_pages.c) */
static void gm_tempo(int32_t s);                               /* (ui_quest_pages.c) */
static void gm_rewind(int32_t s);                              /* (ui_quest_pages.c) */
static uint32_t pv_pg;                                  /* VISUALIZER: the page 0..5 (ui_quest_pages.c) */
static uint8_t about_pg;                                /* ABOUT: 0 = the memory, 1 = the credits (PRESETS flips) */
extern uint32_t _data_start[], _data_end[], _bss_start[], _bss_end[], _pool_start[], _pool_end[], _rt_start[], _rt_end[];
/* GAMES (menu -> GAMES): ph 0 = FM Hero / Piano Hero, 1 = the song (the 6 songs sent from the web editor), 2 = playing, 3 = the result (ui_quest_pages.c) */
static struct { uint8_t kind, ph, sel; uint32_t t0, last_ms, score, combo, best, hit, miss, good, first, n, prev_keys, endms; uint8_t done[120]; int32_t pos; uint32_t adv, oct_t0, wrong; uint8_t snd[27], help; } gm;
static uint32_t gm_phase(void) { return gm.ph; }
#define GM_TAPES (SNG_PER + 1u)                         /* the songs and, last, the empty tape: add a file */
static int32_t gm_tgt, gm_cur;                          /* the tapes strip, 1/256 of a tape: where it goes and where it is */
static int16_t gm_bpmx[2 * SNG_PER];                    /* the tempo picked for each song (0 = the song's own) */
#define IMP_N (SMP_USER_SLOTS + SNG_N)
#define IMP_HOLD_MS 1300u
static uint8_t imp_sel, imp_done;
static uint32_t imp_t0;                                 /* REC pressed since (ms), 0 = not */
static uint8_t flash_ok;                                /* (felucca.c) */
static UI_SMALL int ed_smp_erase(uint32_t k, uint32_t all);      /* (editor.c) */
static int ed_sng_erase(uint32_t sid);
static UI_SMALL uint32_t imp_prog(void)                          /* 0..100 */
{
    uint32_t d = imp_t0 ? fm1_ms - imp_t0 : 0u;
    return imp_done ? 100u : d >= IMP_HOLD_MS ? 100u : d * 100u / IMP_HOLD_MS;
}
static UI_SMALL int imp_used(uint32_t i)                         /* 1 = something is stored in the item */
{
    return i < SMP_USER_SLOTS ? usr_nz[i] != 0u : sng_ok[i - SMP_USER_SLOTS] != 0u;
}
static UI_SMALL void imp_erase(uint32_t i)
{
    char b[8] = "USR1";
    if (!flash_ok || !imp_used(i)) {
        ui_message("NOTHING THERE");
        return;
    }
    if (i < SMP_USER_SLOTS) {
        b[3] = (char)('1' + i);
        ed_smp_erase(i, 1u);
    } else {
        uint32_t s = i - SMP_USER_SLOTS;
        b[0] = s < SNG_PER ? 'P' : 'H', b[1] = (char)('1' + s % SNG_PER), b[2] = 0;
        ed_sng_erase(s);
        sng_scan(s);
    }
    ui_say("ERASED ", b);
}
static UI_SMALL void importer_input(void)
{
    int32_t s = panel_enc(EN_PRESET);
    if (s)
        imp_sel = (uint8_t)((imp_sel + (s > 0 ? 1u : IMP_N - 1u)) % IMP_N);
    if (fm1_in.buttons & (1u << panel.btn[B_REC])) {
        if (!imp_t0)
            imp_t0 = fm1_ms | 1u, imp_done = 0;
        if (!imp_done && fm1_ms - imp_t0 >= IMP_HOLD_MS) {
            imp_done = 1;
            imp_erase(imp_sel);
        }
    } else {
        imp_t0 = 0;
        imp_done = 0;
    }
}

static UI_SMALL void sx_leave(void);
static UI_SMALL void sx_name(char *d, uint32_t i, uint32_t max);       /* ui_quest_screens.c */
static UI_SMALL void menu_close(void)
{
    if (ui.menu == 9) sx_leave();
    if (song.playing || transport_req)
        settings_later = 1;                            /* (a flash write stops the audio: once stopped) */
    else
        settings_save();                               /* palette / panel table, if changed */
    ui.menu = 0;
    ui.force = 1;
    go_home();
}

static UI_SMALL uint32_t sx_brkit(uint32_t which)           /* the index (in the synthesised kits) of BRASIL (0) or BRASIL 2 (1) */
{
    uint32_t i;
    for (i = DRUM_SAMPLED; i < DRUM_KITS; i++)
        if (str_eq(DRUM_KIT_NAMES[i], which ? "BRASIL 2" : "BRASIL")) return i - DRUM_SAMPLED;
    return 0;
}
static UI_SMALL void sx_enter(void)                          /* remember the track's sound as it is */
{
    track_t *t = TSEL;
    uint32_t i;
    for (i = 0; i < P_COUNT; i++) sx_p[i] = t->p[i];
    sx_eng = t->eng_req, sx_pre = t->preset, sx_usr = t->user;
    sx_perc = t->perc;
    if (is_drum(t)) sx_n = DRUM_KITS, sx_np = DRUM_KITS, sx_i = (uint16_t)drum_kit();
    else {                                                        /* the presets, and for the cleric / bard also the Brazilian instruments (the others keep the normal sounds) */
        uint32_t n;
        sx_i = (uint16_t)preset_pos(&n), sx_np = (uint16_t)n, sx_n = (uint16_t)(n + (song.sel == 2u ? 32u : 0u));
        if (song.sel != 2u) t->perc = 0, sx_perc = 0;
        if (t->perc) sx_i = (uint16_t)(n + ((t->perc - 1u) / 16u == sx_brkit(1) ? 16u : 0u) + (t->perc - 1u) % 16u);
    }
    sx_com = sx_i;
    sx_open = 1;
    sx_cur = sx_tgt = 0, sx_pend = 0;
}
static UI_SMALL void sx_stop(void)                           /* silence, the solo and the transport back */
{
    track_t *t = TSEL;
    if (!sx_on) return;
    sx_on = 0;
    { uint32_t k; for (k = 0; k < 8u; k++) sx_hn[k] = 0; }
    trk_all_off(t);
    song.solo = sx_solo;
    if (sx_resume) transport_req = 1, sx_resume = 0;
}
static UI_SMALL void sx_start(void)
{
    sx_solo = song.solo;
    song.solo = (uint8_t)(1u << song.sel);
    sx_resume = (uint8_t)(song.playing && !transport_req);
    if (sx_resume) transport_req = 2;
    sx_on = 1, sx_t0 = fm1_ms, sx_st = 0, sx_note = 0;
}
static UI_SMALL void sx_apply(void)                          /* the highlighted sound, on the track (until leaving) */
{
    track_t *t = TSEL;
    trk_all_off(t);
    if (is_drum(t)) TDRUM->p[P_E0] = (int16_t)sx_i;
    else if (sx_i >= sx_np) {                                      /* a Brazilian instrument: the hero's keys hit it */
        uint32_t k = sx_i - sx_np;
        t->perc = (uint16_t)(1u + sx_brkit(k / 16u) * 16u + k % 16u);
    } else {
        t->perc = 0;
        preset_go(sx_i);
    }
    sx_t0 = fm1_ms, sx_st = 0, sx_note = 0;
}
static UI_SMALL void sx_leave(void)
{
    track_t *t = TSEL;
    uint32_t i;
    sx_stop();
    if (sx_open && sx_i != sx_com) {                              /* heard but not kept: the old sound */
        for (i = 0; i < P_COUNT; i++) t->p[i] = sx_p[i];
        t->eng_req = sx_eng, t->preset = sx_pre, t->user = sx_usr, t->perc = sx_perc;
        sync_reload = 1;
        panic_req |= (uint8_t)(1u << song.sel);
    }
    sx_open = 0;
}
/* what each kind of sound plays (the sounds page's phrases): a bass line, a melody, held chords, comping chords, chord stabs, a beat for the kits; once, ~5 s */
typedef struct { uint8_t s, n, l; } sxe_t;
static const sxe_t SX_BASS[] = {{0, 36, 3}, {4, 36, 1}, {6, 43, 2}, {8, 41, 3}, {12, 39, 2}, {14, 38, 2}, {16, 36, 3}, {20, 36, 1}, {22, 43, 2}, {24, 46, 3}, {28, 43, 4}};
static const sxe_t SX_MEL[] = {{0, 72, 2}, {2, 75, 2}, {4, 77, 3}, {8, 79, 2}, {10, 77, 2}, {12, 75, 4}, {16, 72, 2}, {18, 70, 2}, {20, 72, 3}, {24, 67, 4}, {28, 70, 2}, {30, 72, 2}};
static const sxe_t SX_HELD[] = {{0, 60, 14}, {0, 63, 14}, {0, 67, 14}, {0, 70, 14}, {16, 58, 14}, {16, 62, 14}, {16, 65, 14}, {16, 68, 14}};
static const sxe_t SX_STAB[] = {{0, 60, 2}, {0, 63, 2}, {0, 67, 2}, {0, 70, 2}, {8, 60, 2}, {8, 63, 2}, {8, 67, 2}, {8, 70, 2}, {16, 58, 2}, {16, 62, 2}, {16, 65, 2}, {16, 68, 2}, {24, 58, 2}, {24, 62, 2}, {24, 65, 2}, {24, 68, 2}};
static const uint8_t SX_COMPS[10] = {0, 3, 6, 10, 12, 16, 19, 22, 26, 28};
static const sxe_t SX_DRUM[] = {{0, 36, 1}, {2, 42, 1}, {3, 70, 1}, {4, 38, 1}, {6, 42, 1}, {7, 63, 1}, {8, 36, 1}, {10, 36, 1}, {10, 42, 1}, {11, 56, 1}, {12, 38, 1}, {14, 46, 1}, {15, 70, 1}};
static UI_SMALL uint32_t sx_role(void)                       /* 0 bass, 1 melody, 2 held chords, 3 comping, 4 stabs, 5 beat */
{
    static const uint8_t BY_KIND[8] = {0, 3, 2, 2, 1, 1, 4, 1};   /* BASS KEYS ORGN PAD LEAD PLCK STAB FX */
    if (is_drum(TSEL)) return 5u;
    if (sx_i >= sx_np) return 0u;                               /* a percussion instrument: a groove of hits */
    return sx_i < NBANK ? BY_KIND[BANK[sx_i].kind & 7u] : 1u;
}
static UI_SMALL void sx_hit(track_t *t, uint32_t note, uint32_t step, uint32_t len)
{
    uint32_t k;
    if (is_drum(t)) { drum_on(note, 100); return; }
    trk_note_on(t, note, 100);
    for (k = 0; k < 8u; k++)
        if (!sx_hn[k]) { sx_hn[k] = (uint8_t)note, sx_ho[k] = (uint8_t)(step + len); return; }
}
static UI_SMALL void sx_tick(void)                           /* the phrase of this kind of sound, one time: 150 ms a step */
{
    track_t *t = TSEL;
    uint32_t st, k, role, nev;
    const sxe_t *ev;
    if (!sx_on) return;
    st = (fm1_ms - sx_t0) / 150u;
    role = sx_role();
    ev = role == 0u ? SX_BASS : role == 1u ? SX_MEL : role == 2u ? SX_HELD : role == 4u ? SX_STAB : SX_DRUM;
    nev = role == 0u ? sizeof SX_BASS / sizeof *SX_BASS : role == 1u ? sizeof SX_MEL / sizeof *SX_MEL : role == 2u ? sizeof SX_HELD / sizeof *SX_HELD :
          role == 4u ? sizeof SX_STAB / sizeof *SX_STAB : sizeof SX_DRUM / sizeof *SX_DRUM;
    for (; sx_st <= st && sx_st < 36u; sx_st++) {
        uint32_t s = sx_st;
        for (k = 0; k < 8u; k++)
            if (sx_hn[k] && sx_ho[k] <= s) trk_note_off(t, sx_hn[k]), sx_hn[k] = 0;
        if (role == 3u) {
            for (k = 0; k < 10u; k++)
                if (SX_COMPS[k] == s) sx_hit(t, 60, s, 2), sx_hit(t, 63, s, 2), sx_hit(t, 67, s, 2);
        } else {
            uint32_t sm = role == 5u ? s % 16u : s;
            for (k = 0; k < nev; k++)
                if (ev[k].s == sm) sx_hit(t, ev[k].n, s, ev[k].l);
        }
    }
    if (st >= 36u) sx_stop();                                   /* it plays once: PLAY plays it again */
}

static uint8_t q_var[4];                                      /* ui_quest_fig.c: the class variants of the support and fighter slots */
static uint32_t pm_flip_t0;                                   /* PARTY: when a card last turned over (ms), its slot and the variant it had */
static uint8_t pm_flip_slot, pm_flip_prev;
static void q_set_var(uint32_t slot, uint32_t v);
/* menu: PRESETS moves, OCT+ confirms, OCT- cancels (ABOUT -> list -> close) */
static UI_SMALL void menu_input(uint32_t pressed)
{
    int32_t s;
    uint32_t ok = (pressed >> panel.btn[B_OCTUP]) & 1u, back = (pressed >> panel.btn[B_OCTDN]) & 1u, sel = (pressed >> panel.btn[B_SCL]) & 1u;
    if (ui.menu == 1 || ui.menu == 7 || ui.menu == 10) ok |= sel;                                    /* the menu list and the games: SEL does what OCT+ does */
    if (ui.menu == 6 && ((pressed >> panel.btn[B_PLAY]) & 1u) && !ft_owns_press()) transport_req = song.playing ? 2 : 1;    /* the visualizer: PLAY plays the loop */
    if (ui.menu == 7 && gm.ph <= 1u) ok |= (pressed >> panel.btn[B_PLAY]) & 1u;     /* (and PLAY picks / starts the tape: its LED is lit) */
    if (ui.menu == 7) {
        gm_tempo(panel_enc(EN_SELECT));                  /* SELECT: the tempo of the tape chosen */
        gm_rewind(panel_enc(EN_K1));                     /* KNOB 1 in a game: back to the start, as far as it is turned (VHS) */
        gm_input(ok, back, panel_enc(EN_PRESET));
        enc_drop();
        return;
    }
    if (back && ui.menu == 10) {                        /* the preview goes: the kept filter again */
        q_filter_set(q_filter);
        ui.menu = 1, ui.force = 1;
        return;
    }
    if (back && ui.menu == 9) {                         /* reached by holding PLAY: back goes home */
        sx_leave();
        ui.menu = 0, ui.force = 1;
        go_home();
        return;
    }
    if (back) {
        if (ui.menu == 9) sx_leave();
        if (ui.menu >= 2)
            ui.menu = 1, ui.force = 1, imp_t0 = 0;
        else
            menu_close();
        return;
    }
    if (ui.menu == 2) {                                /* ABOUT: PRESETS flips between the memory and the credits */
        if (panel_enc(EN_PRESET)) {
            about_pg ^= 1u;
            ui.force = 1;
        }
        enc_drop();
        return;
    }
    if (ui.menu == 3) {                                /* IMPORTER */
        importer_input();
        enc_drop();
        return;
    }
    if (ui.menu == 10) {                               /* FILTER: PRESETS move (the scene strip shows the preview at once), OCT+ / SEL keep it, OCT- puts the old one back */
        int32_t d = panel_enc(EN_PRESET) + panel_enc(EN_SELECT);
        if (d) {
            qf_sel = (uint8_t)((qf_sel + (d > 0 ? 1u : 11u)) % 12u);
            q_filter_set(qf_sel);
            ui.force = 1;
        }
        if (ok) {
            q_filter = qf_sel;
            settings_later = 1;                        /* (saved with the settings) */
            ui_message("FILTER KEPT");
            ui.force = 1;
        }
        enc_drop();
        return;
    }
    if (ui.menu == 8) {                                /* KEYBOARD LED: PRESETS picks the effect (one at a time), KNOB 1 its speed, KNOB 2 its fade; every change plays it on the keys */
        int32_t d = panel_enc(EN_PRESET) + panel_enc(EN_SELECT), k1 = panel_enc(EN_K1), k2 = panel_enc(EN_K2), k3 = panel_enc(EN_K3);
        if (k3) led_hold = (uint8_t)(k3 > 0);                                    /* KNOB 3: once per press / while held */
        if (d) {
            uint32_t pos = 0, q;
            for (q = 0; q < LED_NFX; q++) if (LED_ORDER[q] == led_fx) pos = q;
            led_fx = LED_ORDER[(pos + (d > 0 ? 1u : LED_NFX - 1u)) % LED_NFX];
        }
        if (k1) led_sp = (uint8_t)clamp((int32_t)led_sp + k1, 0, 15);
        if (k2) led_fd = (uint8_t)clamp((int32_t)led_fd + k2, 0, 15);
        if (d || k1 || k2 || k3) led_demo_t0 = fm1_ms | 1u;
        enc_drop();
        return;
    }
    if (ui.menu == 9) {                                /* SOUND SELECT (hold PLAY): PRESETS roll the tapes and, when the knob rests, the sound plays alone; SEL keeps it; ALGORITHM picks the hero */
        int32_t d = panel_enc(EN_PRESET) + panel_enc(EN_SELECT), a = panel_enc(EN_ALGO);
        uint32_t sel = (pressed >> panel.btn[B_SCL]) & 1u, pl = (pressed >> panel.btn[B_PLAY]) & 1u;
        if (pl) {                                      /* PLAY: hear it again */
            if (sx_on) sx_stop();
            sx_pend = 0;
            sx_start();
        }
        if (a) {
            sx_leave();
            song.sel = (uint8_t)((song.sel + (a > 0 ? 1u : NTRK - 1u)) % NTRK);
            sx_enter();
            ui.force = 1;
        }
        if (d && sx_n) {
            sx_i = (uint16_t)((sx_i + (d > 0 ? 1u : sx_n - 1u)) % sx_n);
            sx_tgt += d > 0 ? 256 : -256;
            if (sx_on) sx_stop();                                              /* quiet while it moves */
            sx_apply();
            sx_pend = fm1_ms | 1u;
            ui.force = 1;
        }
        if (sx_pend && !sx_on && (int32_t)(fm1_ms - sx_pend) >= 450) {                   /* the knob rested: now it plays */
            sx_pend = 0;
            sx_start();
            ui.force = 1;
        }
        if (sel) {                                     /* copy: this is the track's sound now, clean: the hero's effects, chord and arpeggio off, as the preview played */
            if (!is_drum(TSEL)) {                      /* the sound's own parameters again, nothing of what was set before: the preset applied whole, the effects of the hero, chord, arpeggio, pan */
                track_t *t = TSEL;
                if (sx_i < sx_np) apply_preset(t->preset);
                fx_reset(song.sel);
                t->p[P_CHORD] = TP[P_CHORD].def;
                t->p[P_AMODE] = TP[P_AMODE].def;
                t->p[P_PAN] = TP[P_PAN].def;
            }
            sx_enter();
            { char nm[16], mm[28]; sx_name(nm, sx_i, sizeof nm); str_cpy(mm, "COPIED: ", sizeof mm); str_cpy(mm + 8, nm, sizeof mm - 8); ui_message(mm); }
            sx_leave();                                 /* the screen closes: home, with the message on the bar */
            ui.menu = 0, ui.force = 1;
            go_home();
            return;
        }
        sx_tick();
        enc_drop();
        return;
    }
    if (ui.menu == 6) {                                /* VISUALIZER: PRESETS flips the page (it stays until OCT-) */
        int32_t d = panel_enc(EN_PRESET);
        if (d) {
            pv_pg = (pv_pg + (d > 0 ? 1u : PV_N - 1u)) % PV_N;
            ui.force = 1;
        }
        enc_drop();
        return;
    }
    if (ui.menu == 4 || ui.menu == 5) {                /* PARTY / STATUS: PRESETS picks the hero (the selected track) */
        int32_t d = panel_enc(EN_ALGO), c = panel_enc(EN_PRESET) + panel_enc(EN_SELECT);
        if (ui.menu == 5) {                              /* STATUS: ALGORITHM picks the hero, OCT+ the next one */
            if (d) song.sel = (uint8_t)((song.sel + (d > 0 ? 1u : NTRK - 1u)) % NTRK);
            if (ok) song.sel = (uint8_t)((song.sel + 1u) % NTRK);
        } else {                                         /* PARTY: PRESETS move over the cards (no wrap), ALGORITHM changes the hero of the class (no loop: it stops at the ends), the card turns; OCT- is back */
            int32_t v = d;
            if (c) { int32_t ns = (int32_t)song.sel + (c > 0 ? 1 : -1); if (ns >= 0 && ns < (int32_t)NTRK) song.sel = (uint8_t)ns, ui.force = 1; }
            if (v && (song.sel == 2u || song.sel == 3u)) {
                uint32_t mx = song.sel == 2u ? 1u : q_warrior_ok ? 1u : 0u, nv = v > 0 ? (q_var[song.sel] < mx ? q_var[song.sel] + 1u : q_var[song.sel]) : (q_var[song.sel] ? q_var[song.sel] - 1u : 0u);
                if (nv != q_var[song.sel]) {
                    pm_flip_prev = q_var[song.sel], pm_flip_slot = song.sel, pm_flip_t0 = fm1_ms | 1u;
                    q_set_var(song.sel, nv);
                    ui.force = 1;
                }
            }
        }
        if (ui.menu == 4) stage_knobs();                 /* KNOBS 1-4: FORM, LEAD, TURN, STYLE (ui_quest_stage.c) */
        enc_drop();
        return;
    }
    if ((s = panel_enc(EN_PRESET)) != 0 && ui.menu == 1)
        ui.menu_sel = (uint8_t)((ui.menu_sel + (s > 0 ? 1u : MI_COUNT - 1u)) % MI_COUNT);
    s = panel_enc(EN_K1);
    /* FM1 Quest: one palette (TAVERN); the COLOR row only shows it */
    if ((s != 0 || ok) && ui.menu == 1 && ui.menu_sel == MI_LOWCUT) {
        /* KNOB 1: right = ON, left = OFF; OCT+ toggles */
        uint32_t *v = &settings.lowcut;
        *v = s > 0 ? 1u : s < 0 ? 0u : !*v;
        fx_lowcut = (uint8_t)(settings.lowcut != 0);
        ok = 0;
    }
    if ((s != 0 || ok) && ui.menu == 1 && ui.menu_sel == MI_USB) {     /* right FULL, left MASTER; OCT+ toggles */
        usb_full = (uint8_t)(s > 0 ? 1u : s < 0 ? 0u : !usb_full);
        ok = 0;
    }
    if ((s != 0 || ok) && ui.menu == 1 && ui.menu_sel == MI_BOOT) {     /* right ON, left OFF; OCT+ toggles: the sound at power-on (the lights stay) */
        boot_mute = (uint8_t)(s > 0 ? 0 : s < 0 ? 1 : !boot_mute);
        ok = 0;
    }
    if ((s != 0 || ok) && ui.menu == 1 && ui.menu_sel == MI_ULT) {      /* right ON, left OFF; OCT+ toggles: the animated ult, or only the simple effect */
        q_ult_simple = (uint8_t)!q_ult_simple;               /* SIDE <-> OFF (the whole-screen ult is gone) */
        ok = 0;
    }
    if ((s != 0 || ok) && ui.menu == 1 && (ui.menu_sel == MI_DENS || ui.menu_sel == MI_BSPD)) {      /* few / normal / many, slow / normal / fast: right more, left less, OCT+ the next */
        uint32_t v = ui.menu_sel == MI_DENS ? Q_OPT_DENS() : Q_OPT_BSPD();
        v = s > 0 ? (v < 2u ? v + 1u : 2u) : s < 0 ? (v ? v - 1u : 0u) : (v + 1u) % 3u;
        if (ui.menu_sel == MI_DENS) Q_OPT_SET(0, 3u, v); else Q_OPT_SET(3, 3u, v);
        ok = 0;
    }
    if ((s != 0 || ok) && ui.menu == 1 && ui.menu_sel == MI_DMG) {                                   /* the damage numbers over the monsters and the boss */
        Q_OPT_SET(2, 1u, s > 0 ? 1u : s < 0 ? 0u : !Q_OPT_DMG());
        ok = 0;
    }
    if (ok && ui.menu == 1) {
        switch (ui.menu_sel) {
        case MI_IMPORT:
            ui.menu = 3;
            imp_sel = 0;
            imp_t0 = 0;
            ui.force = 1;
            break;
        case MI_GAMES:
            ui.menu = 7;
            gm.ph = 0, gm.sel = 0, gm.kind = 0;
            ui.force = 1;
            break;
        case MI_VIZ:
            ui.menu = 6;
            ui.force = 1;
            break;
        case MI_FILTER:                                        /* FILTER: the radio list with a light preview */
            ui.menu = 10;
            qf_sel = q_filter;
            ui.force = 1;
            break;
        case MI_LED:
            ui.menu = 8;
            led_demo_t0 = fm1_ms | 1u;
            ui.force = 1;
            break;
        case MI_PARTY:
        case MI_STATUS:
            ui.menu = ui.menu_sel == MI_PARTY ? 4 : 5;
            ui.force = 1;
            break;
        case MI_ABOUT:
            ui.menu = 2;
            about_pg = 0;
            ui.force = 1;
            break;
        default:
            menu_close();
            break;
        }
    }
    enc_drop();                                        /* swallow the rest while the menu is up */
}

