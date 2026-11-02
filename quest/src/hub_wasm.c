/* The 3D-fight hub build: the same web HAL, plus two hooks so the hub page can jump to a home and a boss level. */
#include "fm1quest_wasm.c"
WASM_EXPORT("hub_home") void hub_home(uint32_t h) { q_town = (uint8_t)(h & 3u); ui.force = 1; }
#ifdef HUB_FLIGHT3D
WASM_EXPORT("hub_dbg") int32_t hub_dbg(uint32_t k) { return k == 0 ? sp_x : k == 1 ? sp_y : k == 2 ? sp_z : k == 3 ? fl.near : k == 4 ? fl.a : k == 5 ? (int32_t)fl.calm_ms : fl.dphi; }
WASM_EXPORT("hub_level") void hub_level(uint32_t l) { fl.lvl = (int32_t)l; fl.hp = 1000 + 40 * (int32_t)l; fl.php = 5; }
#endif
WASM_EXPORT("hub_menu") void hub_menu(uint32_t m) { ui.menu = (uint8_t)m; ui.force = 1; }
WASM_EXPORT("hub_stage") void hub_stage(uint32_t f, uint32_t l, uint32_t t, uint32_t s) { st_form = (uint8_t)f, st_lead = (uint8_t)l, st_turn = (uint8_t)t, st_style = (uint8_t)s; ui.force = 1; }
WASM_EXPORT("hub_solo") uint32_t hub_solo(void) { return song.solo; }
WASM_EXPORT("hub_arp") uint32_t hub_arp(int32_t set) { if (set >= 0) TSEL->p[P_AMODE] = (int16_t)set; return (uint32_t)TSEL->p[P_AMODE]; }
WASM_EXPORT("hub_par") int32_t hub_par(uint32_t id, int32_t v) { if (v > -1000) TSEL->p[id] = (int16_t)v; return TSEL->p[id]; }
WASM_EXPORT("hub_pamode") uint32_t hub_pamode(void) { return P_AMODE; }
WASM_EXPORT("hub_paorder") uint32_t hub_paorder(void) { return P_AORDER; }
WASM_EXPORT("hub_pahold") uint32_t hub_pahold(void) { return P_AHOLD; }
WASM_EXPORT("hub_scan") void hub_scan(uint32_t k) { smp_user_scan(k); }
WASM_EXPORT("hub_wave") void hub_wave(uint32_t in, uint32_t out)
{
    uint32_t i;
    select_engine(4u);
    TSEL->p[P_E0] = (int16_t)SMP_NSETS, TSEL->p[P_E5] = (int16_t)in, TSEL->p[P_E7] = (int16_t)out;
    for (i = 0; i < NPAGES; i++) if (PAGES[i].graph == GR_WAVE) { ui.page = (uint8_t)i; break; }
    ui.home = 0, ui.force = 1;
}
