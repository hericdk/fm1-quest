/* SPDX-License-Identifier: GPL-3.0-only */
/* SLOOP 2.4: the drum track's KIT USR1..USR3, a user sample slot as a drum kit (the web editor's DRUM KIT
 * writes a zone per lane, lo = hi = root = the lane's GM note, drums.c LANE_NOTE). Through hostsim.c, the
 * slot in a RAM image of the flash (SMP_USER_XIP) read by smp_user_scan as at boot:
 *   a lane with a sound plays it (its zone, at its own pitch); a lane without one, and an empty slot, are silent
 *   a GM note plays its lane's sound (LANE_OF_GM: 35 -> KICK 2, 40 -> SNARE 2...), the click's wood block no lane
 *   the closed hat chokes the open one, as in the other kits
 *   the sequencer's drum steps play the kit; the built-in kits are as before */
#include <stdint.h>
static uint32_t host_slots[4u * 0x14000u / 4u];          /* USR1..4 (a RAM image: the flash has USR4 elsewhere) */
#define SMP_USER_XIP(k) ((const uint8_t *)host_slots + (k) * SMP_USER_SIZE)
#define main hostsim_main
#include "hostsim.c"
#undef main
#include "../firmware/hal/fm1_flash.h"                /* (the flash map: FL_STORE_OK; nothing of it runs here) */

static int fails;
static void check(int ok, const char *what) { printf("userkit: %-70s %s\n", what, ok ? "ok" : "FAIL"); fails += !ok; }

/* slot k: a zone per (note, samples) pair; ADPCM nibbles alternating up / down (a square-ish wave) */
static void slot_build(uint32_t k, const uint8_t *notes, const uint32_t *lens, uint32_t nz)
{
    uint8_t *img = (uint8_t *)host_slots + k * SMP_USER_SIZE, *d = img + SMP_USER_DATA;
    smp_user_hdr_t *h = (smp_user_hdr_t *)img;
    uint32_t i, off = 0, j;
    memset(img, 0xFF, SMP_USER_SIZE);
    memset(h, 0, sizeof *h);
    h->magic = SMP_USER_MAGIC;
    h->version = 1;
    h->nz = (uint8_t)nz;
    memcpy(h->name, "KIT", 4);
    for (i = 0; i < nz; i++) {
        smp_zone_t *z = &h->zone[i];
        uint32_t bytes = (lens[i] + 1u) / 2u;
        for (j = 0; j < bytes; j++)
            d[off + j] = (j / 8u) & 1u ? 0xFFu : 0x77u;  /* up a while, then down (big steps) */
        z->off = off; z->n = lens[i]; z->ls = 0; z->le = lens[i] - 1u;
        z->rate = 0x8000u;                               /* 22050 / 44100 */
        z->root16 = (int16_t)(notes[i] * 16);
        z->pred = 0; z->idx = 0; z->lo = notes[i]; z->hi = notes[i]; z->looped = 0;
        off += bytes;
    }
    h->data_len = off;
    smp_user_scan(k);
}

static int32_t run_peak(uint32_t blocks)
{
    static int32_t o[CTL * 2];
    int32_t pk = 0;
    uint32_t b, i;
    for (b = 0; b < blocks; b++) {
        mix_block(o, CTL);
        for (i = 0; i < CTL * 2u; i++) { int32_t a = o[i] < 0 ? -o[i] : o[i]; if (a > pk) pk = a; }
    }
    return pk;
}
static int drum_voice_on(int32_t zid)
{
    uint32_t i;
    for (i = 0; i < NDRUM; i++)
        if (drums.v[i].active && !drums.synth[i] && drums.v[i].s[4] == zid)
            return 1;
    return 0;
}
static void quiet(void) { uint32_t i; for (i = 0; i < NDRUM; i++) drums.v[i].active = 0; drums.tail = 0; run_peak(3u * FS / CTL); }

int main(void)
{
    static const uint8_t notes[4] = {36, 38, 42, 46};     /* KICK, SNARE, HAT, OPEN HAT */
    static const uint32_t lens[4] = {8000, 3000, 2000, 16000};
    int32_t pk;
    if ((uintptr_t)host_slots < (uintptr_t)SMP_DATA || (uintptr_t)host_slots - (uintptr_t)SMP_DATA > 0xF0000000u) {
        printf("userkit: the slot image must lie above SMP_DATA within 4 GiB (32-bit offsets)\n");
        return 1;
    }
    host_tracks_init();
    song.g[G_BPM] = 120;
    song.g[G_DRREV] = 0;                                 /* (no reverb tail between the checks) */
    slot_build(0, notes, lens, 4);
    check(usr_nz[0] == 4u && !usr_nz[1], "USR1 read by smp_user_scan: 4 zones; USR2 empty");
    check(DRUM_SYN == DRUM_USR + 5u && DRUM_KITS == DRUM_SYN + 4u && !strcmp(DRUM_KIT_NAMES[DRUM_USR], "USR1") && !strcmp(DRUM_KIT_NAMES[DRUM_USR + 3u], "USR4") &&
          !strcmp(DRUM_KIT_NAMES[DRUM_PAIR], "USR3+4") && DRUM_PAIR == DRUM_SYN - 1u && !strcmp(DRUM_KIT_NAMES[DRUM_SAMPLED], "808"),
          "KIT: USR1..USR4, USR3+4 after the synthesised kits (the old kit numbers kept)");
    check(SMP_USER_OFF(0u) == 0xA0000u && SMP_USER_OFF(2u) == 0xC8000u && SMP_USER_OFF(3u) == 0xE7000u &&
          FL_STORE_OK(0xE7000u, 0x14000u) && !FL_STORE_OK(0xE7000u, 0x14001u) && !FL_STORE_OK(0xFB000u, 0x1000u) &&
          SMP_USER_OFF(3u) >= FL_FM6_HI && SMP_USER_OFF(3u) + SMP_USER_SIZE <= FL_GLOB_LO && !strcmp(SMP_ALL_NAMES[SMP_NSETS + 3u], "USR4"),
          "USR4: 0xE7000..0xFAFFF, after the FM6 bank, before the settings; the store may write it, not past it");

    TDRUM->p[P_E0] = (int16_t)DRUM_USR;                  /* USR1 */
    quiet();
    drum_on(36, 110);
    check(drum_voice_on(DZ_USR + 0), "USR1, KICK: its zone (slot 0, zone 0)");
    {
        uint32_t i;
        for (i = 0; i < NDRUM; i++)
            if (drums.v[i].active && drums.v[i].s[4] == DZ_USR + 0)
                break;
        check(i < NDRUM && drums.v[i].s[5] == (int32_t)((pow2_q16(0) >> 8) * (0x8000u >> 8)), "... at its own pitch (root = the lane's note)");
    }
    pk = run_peak(40);
    check(pk > 2000, "... and it sounds");
    quiet();
    drum_on(38, 110);
    check(drum_voice_on(DZ_USR + 1), "SNARE: zone 1");
    quiet();
    drum_on(40, 110);                                    /* GM electric snare -> SNARE 2 lane: no sound in the kit */
    check(!drum_voice_on(DZ_USR + 1) && run_peak(40) < 64, "a lane without a sound (SNARE 2): silent");
    quiet();
    drum_on(35, 110);                                    /* GM 35 -> KICK 2: none */
    check(run_peak(40) < 64, "GM 35 (KICK 2 lane): silent, not the kick");
    quiet();
    drum_on(76, 110);
    check(run_peak(40) < 64, "the click's wood block (76): not a lane");
    quiet();
    drum_on(46, 110);
    check(drum_voice_on(DZ_USR + 3), "OPEN HAT: zone 3");
    run_peak(4);
    drum_on(42, 110);
    check(!drum_voice_on(DZ_USR + 3) && drum_voice_on(DZ_USR + 2), "the closed hat chokes the open one");
    quiet();
    drum_on(44, 110);                                    /* GM pedal hat -> PEDAL lane: none, and no choke */
    check(run_peak(20) < 64, "PEDAL lane without a sound: silent");

    TDRUM->p[P_E0] = (int16_t)(DRUM_USR + 1u);           /* USR2: empty */
    quiet();
    drum_on(36, 110);
    check(run_peak(40) < 64, "USR2 (an empty slot): silent");

    {   /* USR3+4: one kit over two slots, the editor's split: KICK and HAT in USR3, SNARE and OPEN HAT in USR4 */
        static const uint8_t n3[2] = {36, 42}, n4[2] = {38, 46};
        static const uint32_t l3[2] = {30000, 4000}, l4[2] = {6000, 60000};
        slot_build(2, n3, l3, 2);
        slot_build(3, n4, l4, 2);
        check(usr_nz[2] == 2u && usr_nz[3] == 2u, "USR3 and USR4 read: 2 zones each (USR4 from its own place)");
        TDRUM->p[P_E0] = (int16_t)DRUM_PAIR;
        quiet(); drum_on(36, 110);
        check(drum_voice_on(DZ_USR + 2 * 16 + 0), "USR3+4: KICK from USR3");
        quiet(); drum_on(38, 110);
        check(drum_voice_on(DZ_USR + 3 * 16 + 0) && run_peak(40) > 2000, "USR3+4: SNARE from USR4, and it sounds");
        quiet(); drum_on(46, 110); run_peak(4); drum_on(42, 110);
        check(!drum_voice_on(DZ_USR + 3 * 16 + 1) && drum_voice_on(DZ_USR + 2 * 16 + 1), "USR3+4: the closed hat (USR3) chokes the open one (USR4)");
        quiet(); drum_on(49, 110);
        check(run_peak(40) < 64, "USR3+4: a lane in neither slot (CRASH): silent");
        TDRUM->p[P_E0] = (int16_t)(DRUM_USR + 3u);       /* USR4 alone: its two sounds */
        quiet(); drum_on(36, 110);
        check(run_peak(40) < 64, "USR4 alone: no KICK there");
        quiet(); drum_on(38, 110);
        check(drum_voice_on(DZ_USR + 3 * 16 + 0), "USR4 alone: its SNARE");
        quiet();
    }
    {   /* the sequencer: KICK on 1, SNARE on 5 */
        uint32_t j;
        TDRUM->p[P_E0] = (int16_t)DRUM_USR;
        quiet();
        for (j = 0; j < NSTEP; j++) memset(&TDRUM->dstep[j], 0, sizeof(dstep_t));
        dstep_set(&TDRUM->dstep[0], 0, LV_NORM, 0);
        dstep_set(&TDRUM->dstep[4], 2, LV_NORM, 0);
        TDRUM->p[P_SLEN] = 16;
        TDRUM->seq_active = 1;
        transport_req = 1;
        pk = run_peak(FS / CTL);
        transport_req = 2;
        run_peak(10);
        check(pk > 2000, "the drum steps play the user kit");
    }
    TDRUM->p[P_E0] = (int16_t)DRUM_SAMPLED;              /* 808: as before */
    quiet();
    drum_on(36, 110);
    {
        uint32_t i, syn = 0;
        for (i = 0; i < NDRUM; i++) syn += drums.v[i].active && drums.synth[i];
        check(syn == 1u, "808 (a synthesised kit) as before");
    }
    TDRUM->p[P_E0] = 0;                                  /* ACOUSTIC: the GM sample set */
    quiet();
    drum_on(38, 110);
    {
        uint32_t i, gm = 0;
        for (i = 0; i < NDRUM; i++) gm += drums.v[i].active && !drums.synth[i] && drums.v[i].s[4] < DZ_USR;
        check(gm == 1u && run_peak(40) > 2000, "ACOUSTIC (the GM sample set) as before");
    }
    {   /* 2.4.1: the click (GLO > GLOBAL > CLICK = ON) is its own voice: heard with a user kit (it has no wood
         * block: in 2.4 it was silent), with the drum track muted, at the level of GLO > DRUMS > LVL */
        uint32_t j, k;
        int32_t pk[4];
        for (k = 0; k < 4u; k++) {
            host_tracks_init();
            song.g[G_BPM] = 120; song.g[G_CLOCK] = 2; song.g[G_DRLVL] = k == 3u ? 0 : 100;
            for (j = 0; j < NSTEP; j++) memset(&TDRUM->dstep[j], 0, sizeof(dstep_t));
            TDRUM->p[P_E0] = (int16_t)(k == 0u ? DRUM_PAIR : k == 1u ? DRUM_USR + 1u : DRUM_SAMPLED);
            TDRUM->p[P_MUTE] = (int16_t)(k == 2u);
            transport_req = 1;
            pk[k] = run_peak(2u * FS / CTL);
            transport_req = 2;
            run_peak(4);
        }
        check(pk[0] > 8000 && pk[1] > 8000, "the click with KIT USR3+4 and an empty USR2: heard");
        check(pk[2] > 8000, "the click with the drum track muted: heard");
        check(pk[3] < 64, "the click at GLO > DRUMS > LVL 0: silent (its level)");
        song.g[G_CLOCK] = 0;
    }
    printf("userkit: %s\n", fails ? "FAILED" : "KIT USR1..USR4, USR3+4 PASS");
    return fails != 0;
}
