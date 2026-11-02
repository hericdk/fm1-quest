/* SPDX-License-Identifier: GPL-3.0-only */
/* SLOOP 2.5 (experimental): the engines' shared per-part arena (engines.c eng_arena_of) and the two new
 * engines (PHYS, NOISE). Three checks:
 *  1. no stale state: an engine that takes the arena over from another one (GRAIN, FM6, PHYS in both slot
 *     layouts) sounds bit for bit as on a fresh FM-1 (the same phrase, the same random seed);
 *  2. ownership: no engine ever touches the arena of a part that does not play it (eng_arena_bad);
 *  3. a long random session on the three parts: engine switches through the UI's path (eng_req and the
 *     fade), PHYS MODEL flips under sounding notes, chords, note-offs, presets. The mix stays bounded, no
 *     voice hangs, the end is silent. Built with -fsanitize=address,undefined by the runner too.
 *   cc -O1 -DARENA_STATS -Ibuild/gen -Ifirmware/src tests/arena_test.c -lm */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <sys/wait.h>

static int fails;
static void check(int ok, const char *what)
{
    printf("%-100s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok)
        fails++;
}

static uint64_t fnv(uint64_t h, const int32_t *x, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        h ^= (uint32_t)x[i];
        h *= 0x100000001B3ull;
    }
    return h;
}

typedef struct { uint8_t e, pi; int8_t model; const char *name; } cfg_t;   /* model >= 0: PHYS MODEL forced */
static const cfg_t CFG[] = {
    {ENGI_GRAIN, 0, -1, "GRAIN LOFI CLOUD"}, {ENGI_GRAIN, 4, -1, "GRAIN SHIMMER"},
    {ENGI_FM6, 0, -1, "FM6 TINE EP"}, {ENGI_FM6, 2, -1, "FM6 ROUND BASS"},
    {ENGI_PHYS, 0, 0, "PHYS MODAL"}, {ENGI_PHYS, 2, 1, "PHYS STRING"}, {ENGI_PHYS, 5, 2, "PHYS MEMB"},
    {ENGI_PHYS, 7, 3, "PHYS SYMP"}, {ENGI_PHYS, 8, 3, "PHYS SYMP HARP"},
};
#define NCFG (sizeof CFG / sizeof CFG[0])

static void load(track_t *t, const cfg_t *c)
{
    host_preset(t, c->e, c->pi);
    if (c->model >= 0)
        t->p[P_E0] = c->model;
}

/* the phrase on part 0, rendered dry (track_render: no sends, nothing of the other parts) */
static uint64_t phrase(track_t *t)
{
    static const uint8_t N[4] = {48, 55, 60, 64};
    int32_t out[CTL];
    uint64_t h = 0xCBF29CE484222325ull;
    uint32_t b, k;
    for (b = 0; b < 3000u; b++) {                       /* ~2.2 s */
        if (b == 10u)
            for (k = 0; k < 3u; k++)
                trk_note_on(t, N[k], 100);
        if (b == 600u)
            for (k = 0; k < 3u; k++)
                trk_note_off(t, N[k]);
        if (b == 900u)
            trk_note_on(t, N[3], 90);
        if (b == 1500u)
            trk_note_off(t, N[3]);
        for (k = 0; k < CTL; k++)
            out[k] = 0;
        track_render(t, out, CTL);
        h = fnv(h, out, CTL);
    }
    return h;
}

/* run f(a, b) in a child (fresh state, as regress.c does); its 64-bit answer through a pipe */
static uint64_t in_child(uint64_t (*f)(uint32_t, uint32_t), uint32_t a, uint32_t b)
{
    int fd[2];
    uint64_t r = 0;
    pid_t pid;
    if (pipe(fd))
        return 0;
    pid = fork();
    if (pid == 0) {
        r = f(a, b);
        if (write(fd[1], &r, sizeof r) != sizeof r)
            _exit(2);
        _exit(0);
    }
    close(fd[1]);
    if (read(fd[0], &r, sizeof r) != sizeof r)
        r = 0;
    close(fd[0]);
    waitpid(pid, 0, 0);
    return r;
}

static uint64_t fresh(uint32_t ci, uint32_t unused)
{
    static track_t boot;
    track_t *t = &trk[0];
    int32_t out[CTL];
    uint32_t b;
    (void)unused;
    host_tracks_init();
    boot = *t;
    load(t, &CFG[ci]);
    for (b = 0; b < 8000u; b++) {                       /* idle as long as after() lets E idle (GRAIN builds
                                                         * its index meanwhile) */
        engine_block(t);
        track_render(t, out, CTL);
    }
    *t = boot;
    load(t, &CFG[ci]);
    rng_state = 0x5EED5EEDu;
    return phrase(t);
}

/* D plays (the arena full of its state), the part switches to E the UI's way, then the track is put back as
 * it was at power-on (only the arena and the engines' own globals keep what D left) and E plays the phrase */
static uint64_t after(uint32_t di, uint32_t ci)
{
    static track_t boot;
    track_t *t = &trk[0];
    int32_t out[CTL];
    uint32_t b, k, guard;
    host_tracks_init();
    boot = *t;
    load(t, &CFG[di]);
    for (k = 0; k < 4u; k++)
        trk_note_on(t, (uint32_t)(43 + 5 * k), 110);
    for (b = 0; b < 1500u; b++) {
        engine_block(t);
        track_render(t, out, CTL);
    }
    host_preset_req(t, CFG[ci].e, CFG[ci].pi);          /* the switch with notes sounding: the fade */
    if (CFG[ci].model >= 0)
        t->p[P_E0] = CFG[ci].model;
    for (guard = 0; guard < 20000u && (t->engine != CFG[ci].e || t->xf_on); guard++) {
        engine_block(t);
        track_render(t, out, CTL);
    }
    for (k = 0; k < 4u; k++)
        trk_note_off(t, (uint32_t)(43 + 5 * k));
    for (b = 0; b < 8000u; b++) {
        engine_block(t);
        track_render(t, out, CTL);
    }
    *t = boot;
    load(t, &CFG[ci]);
    rng_state = 0x5EED5EEDu;
    return phrase(t) ^ (eng_arena_bad ? 1u : 0u);
}

/* ------------------------------------------------------------ fuzz --- */
static uint32_t fz = 0xA5A5F00Du;
static uint32_t frnd(uint32_t n)
{
    fz ^= fz << 13;
    fz ^= fz >> 17;
    fz ^= fz << 5;
    return fz % n;
}

int main(int argc, char **argv)
{
    uint32_t a, b, same = 0, pairs = 0, secs = argc > 1 ? (uint32_t)atoi(argv[1]) : 300u;
    char what[160];
    uint64_t ref[NCFG];

    /* 1. stale state */
    for (b = 0; b < NCFG; b++)
        ref[b] = in_child(fresh, b, 0);
    for (b = 0; b < NCFG; b++)
        for (a = 0; a < NCFG; a++) {
            uint64_t h;
            if (a == b || (CFG[a].e == CFG[b].e && CFG[a].e != ENGI_PHYS))
                continue;                               /* (the same engine keeps its arena: not a takeover) */
            h = in_child(after, a, b);
            pairs++;
            if (h == ref[b])
                same++;
            else {
                snprintf(what, sizeof what, "arena: %s after %s sounds as on a fresh FM-1", CFG[b].name, CFG[a].name);
                check(0, what);
            }
        }
    snprintf(what, sizeof what, "arena: every engine after every other one (%u pairs, both PHYS layouts) bit-identical to a fresh start", pairs);
    check(same == pairs, what);

    /* 3. the long random session (2. is counted all along) */
    {
        int32_t o[2 * CTL];
        uint32_t blk, nblk = secs * (FS / CTL), i, p, switches = 0, flips = 0, maxabs = 0, hung = 0, tailmax = 0;
        uint8_t held[NPART][128];
        memset(held, 0, sizeof held);
        host_tracks_init();
        for (p = 0; p < NPART; p++)
            host_preset(&trk[p], 0, 0);
        for (blk = 0; blk < nblk; blk++) {
            if (frnd(64) == 0) {                        /* ~ every 46 ms an event */
                uint32_t ev = frnd(100);
                p = frnd(NPART);
                if (ev < 12) {                          /* engine and preset, the UI's way */
                    uint32_t e = frnd(argc > 2 ? (uint32_t)atoi(argv[2]) : NENGINES);
                    host_preset_req(&trk[p], e, frnd(ENGINES[e]->npresets));
                    switches++;
                } else if (ev < 20) {                   /* PHYS: MODEL under the notes (layout flips) */
                    if (trk[p].engine == ENGI_PHYS) {
                        trk[p].p[P_E0] = (int16_t)frnd(4);
                        flips++;
                    }
                } else if (ev < 70) {
                    uint32_t n = 36u + frnd(48);
                    if (!held[p][n]) {
                        trk_note_on(&trk[p], n, 40u + frnd(88));
                        held[p][n] = 1;
                    }
                } else if (ev < 97) {
                    uint32_t n;
                    for (n = 0; n < 128u; n++)
                        if (held[p][n] && frnd(3) == 0) {
                            trk_note_off(&trk[p], n);
                            held[p][n] = 0;
                        }
                } else {
                    uint32_t n;
                    for (n = 0; n < 128u; n++)
                        if (held[p][n]) {
                            trk_note_off(&trk[p], n);
                            held[p][n] = 0;
                        }
                }
            }
            mix_block(o, CTL);
            for (i = 0; i < 2u * CTL; i++) {
                uint32_t v = (uint32_t)(o[i] < 0 ? -o[i] : o[i]);
                if (v > maxabs)
                    maxabs = v;
            }
        }
        for (p = 0; p < NPART; p++)
            for (i = 0; i < 128u; i++)
                if (held[p][i])
                    trk_note_off(&trk[p], i);
        for (blk = 0; blk < 20u * (FS / CTL); blk++) {   /* 20 s: releases, FX tails */
            mix_block(o, CTL);
            if (blk >= 19u * (FS / CTL))
                for (i = 0; i < 2u * CTL; i++) {
                    uint32_t v = (uint32_t)(o[i] < 0 ? -o[i] : o[i]);
                    if (v > tailmax)
                        tailmax = v;
                }
        }
        for (p = 0; p < NPART; p++)
            for (i = 0; i < NVOICE; i++)
                hung += trk[p].v[i].active;
        printf("fuzz: %u s, %u engine switches, %u PHYS model flips, %u arena claims, peak %u\n", secs, switches, flips,
               eng_arena_claims, maxabs);
        snprintf(what, sizeof what, "fuzz: %u s on 3 parts: the mix stays under full scale (peak %u)", secs, maxabs);
        check(maxabs < 32767u, what);
        snprintf(what, sizeof what, "fuzz: every voice free 20 s after the last note-off (%u hanging)", hung);
        check(hung == 0, what);
        snprintf(what, sizeof what, "fuzz: silence at the end (last second peak %u)", tailmax);
        check(tailmax <= 6u, what);
        snprintf(what, sizeof what, "ownership: no engine touched the arena of a part that does not play it (%u)", eng_arena_bad);
        check(eng_arena_bad == 0, what);
    }
    /* 4. PHYS: MODEL flipped every ~50 ms under sounding chords on the three parts (the two slot layouts take
     * turns over the same bytes), against the same chords with MODEL left alone */
    {
        int32_t o[2 * CTL];
        uint32_t pass, blk, i, p, peak[2] = {0, 0}, hung = 0, tail = 0;
        for (pass = 0; pass < 2u; pass++) {
            fz = 0x13579BDFu;
            host_tracks_init();
            for (p = 0; p < NPART; p++) {
                host_preset(&trk[p], ENGI_PHYS, p == 0 ? 2u : p == 1 ? 7u : 0u);
            }
            for (blk = 0; blk < 120u * (FS / CTL); blk++) {
                if (frnd(64) == 0) {
                    p = frnd(NPART);
                    if (frnd(4) == 0) {
                        uint32_t md = frnd(4);
                        if (pass)
                            trk[p].p[P_E0] = (int16_t)md;
                    } else if (frnd(2)) {
                        uint32_t k, r = 40u + frnd(30);
                        for (k = 0; k < 3u; k++)
                            trk_note_on(&trk[p], r + 4u * k, 60u + frnd(68));
                    } else {
                        trk_all_off(&trk[p]);
                    }
                }
                mix_block(o, CTL);
                for (i = 0; i < 2u * CTL; i++) {
                    uint32_t v = (uint32_t)(o[i] < 0 ? -o[i] : o[i]);
                    if (v > peak[pass])
                        peak[pass] = v;
                }
            }
            for (p = 0; p < NPART; p++)
                trk_all_off(&trk[p]);
            for (blk = 0; blk < 20u * (FS / CTL); blk++) {
                mix_block(o, CTL);
                if (pass && blk >= 19u * (FS / CTL))
                    for (i = 0; i < 2u * CTL; i++) {
                        uint32_t v = (uint32_t)(o[i] < 0 ? -o[i] : o[i]);
                        if (v > tail)
                            tail = v;
                    }
            }
            if (pass)
                for (p = 0; p < NPART; p++)
                    for (i = 0; i < NVOICE; i++)
                        hung += trk[p].v[i].active;
        }
        snprintf(what, sizeof what, "PHYS: MODEL flips under chords for 120 s: peak %u (%u without the flips), %u voices hanging, end %u",
                 peak[1], peak[0], hung, tail);
        check(peak[1] < 32767u && peak[1] <= peak[0] + peak[0] / 2u && hung == 0 && tail <= 6u, what);
    }
    printf(fails ? "arena_test: %d FAILED\n" : "arena_test: all checks ok\n", fails);
    return fails != 0;
}
