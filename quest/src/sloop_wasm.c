/* SPDX-License-Identifier: GPL-3.0-only
 *
 * SLOOP web emulator: the real SLOOP/FELUCCA firmware sources compiled to
 * wasm32, with a browser HAL in place of the FM-1 hardware (as tests/hostsim.c
 * does for the DSP alone, but with the whole UI: screen, layers, LEDs).
 *
 * The firmware is one compilation unit (firmware/src/felucca.c); this file
 * plays that role for the web: the HAL below replaces the fm1_*.h headers,
 * then the same sources are included in the same order. No firmware source is
 * modified.
 *
 *   audio   emu_render(n): mix_block into a float buffer (an AudioWorklet)
 *   screen  an ST7789 panel model behind lcd.c: 240x240 RGB565 framebuffer
 *   input   emu_input(notes, buttons) / emu_enc(e, steps) from JS events
 *   boot    emu_init (splash) then emu_start, emu_frame() ~60/s (ui loop)
 */
#include <stdint.h>

#define WASM_EXPORT(n) __attribute__((export_name(n), used))

/* ------------------------------------------------------------- time HAL --- */
#define FM1_TICKS_PER_US 24u
static uint64_t emu_samples;                   /* total frames rendered */
static uint32_t fm1_ticks(void) { return (uint32_t)(emu_samples * 544u); } /* ~24 MHz */
static void fm1_delay_ms(uint32_t ms) { (void)ms; }   /* never busy-wait in a worklet */
static void fm1_wdt_feed(void) {}

/* -------------------------------------------------------------- irq HAL --- */
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t f) { (void)f; }
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}

/* ------------------------------------------------------------ input HAL --- */
/* as hal/fm1_input.h, without the matrix scan: JS writes the debounced state */
#define FM1_NCOL 11u
#define FM1_NKEY 41u              /* ids: 0..13 buttons, 14..40 note keys */
#define FM1_NENC 7u
enum { FM1_BTN_OCT_DOWN = 0, FM1_BTN_OCT_UP = 1 };

static const int8_t FM1_KEYMAP[6][FM1_NCOL] = {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { 5, 11,  4, 10,  3,  9,  2,  8, -1, -1, -1},
    {34, 35, 36, 37, 38, 40, 39, 13,  7,  6, 12},
    {23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
    { 0,  1, 15, 14, 17, 16, 19, 18, 20, 21, 22},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
};

static volatile struct {
    uint32_t notes;              /* bit n = note key n (0 = F3 .. 26 = G5) */
    uint32_t buttons;            /* bit i = matrix button i (0..13) */
    uint32_t pressed, released;  /* button edges since the last fm1_input_edges() */
    uint32_t notes_pressed;      /* note-key press edges */
    uint8_t raw[FM1_NCOL];
    uint8_t cnt[FM1_NKEY];
    uint8_t enc_prev[FM1_NENC], enc_last[FM1_NENC];
    uint8_t enc_rest[FM1_NENC];
    uint16_t enc_still[FM1_NENC];
    int8_t enc_sub[FM1_NENC];
    int16_t enc_steps[FM1_NENC];
    uint32_t frames;
} fm1_in;

static uint8_t fm1_led[FM1_NCOL];
static uint8_t fm1_led_dim[FM1_NCOL];
static uint8_t fm1_led_bg[FM1_NCOL];
static volatile uint16_t fm1_led_bg_ns;

static void fm1_input_init(void) {}

static int32_t fm1_enc_take(uint32_t e)
{
    int32_t s = fm1_in.enc_steps[e];
    fm1_in.enc_steps[e] = 0;
    return s;
}

static uint32_t fm1_input_edges(uint32_t *released)
{
    uint32_t p = fm1_in.pressed;
    if (released)
        *released = fm1_in.released;
    fm1_in.pressed = fm1_in.released = 0;
    return p;
}

static uint32_t fm1_input_note_edges(void)
{
    uint32_t p = fm1_in.notes_pressed;
    fm1_in.notes_pressed = 0;
    return p;
}

static void fm1_led_key(uint32_t id, int on)
{
    uint32_t p, r;
    for (p = 0; p < FM1_NCOL; p++)
        for (r = 1; r < 5u; r++)
            if (FM1_KEYMAP[r][p] == (int8_t)id) {
                if (on)
                    fm1_led[p] |= (uint8_t)(1u << r);
                else
                    fm1_led[p] &= (uint8_t)~(1u << r);
            }
}

/* -------------------------------------------------------------- adc HAL --- */
enum { FM1_ADC_BATT = 3, FM1_ADC_MASTER = 4 };
static volatile int32_t emu_adc_v[8] = {0, 0, 0, 800, 1023, 0, 0, 0};
static int32_t fm1_adc_read(uint32_t ch) { return emu_adc_v[ch & 7u]; }
static void fm1_adc_init(void) {}

/* -------------------------------------------------- lcd HAL (ST7789 model) */
/* lcd.c speaks the real panel protocol; this consumes it into a framebuffer.
 * Only CASET (2A), RASET (2B) and RAMWR (2C) matter; init commands are
 * ignored. Pixels arrive RGB565 big-endian (gfx.c pre-swaps). */
static uint16_t emu_fb[240 * 240];
static volatile uint32_t emu_fb_gen;           /* bumped on every pixel write */
static uint32_t fm1_lcd_timeouts;
static uint8_t st_cmd;
static uint32_t st_argn;
static uint8_t st_arg[4];
static uint16_t st_x0, st_x1 = 239, st_y0, st_y1 = 239, st_cx, st_cy;

static void fm1_lcd_hw_init(void) {}
static void fm1_lcd_baud(uint32_t b) { (void)b; }
static void fm1_lcd_wait(void) {}
static void fm1_lcd_deselect(void) {}

static void fm1_lcd_send_cmd(uint8_t c)
{
    st_cmd = c;
    st_argn = 0;
    if (c == 0x2Cu) {                           /* RAMWR resets the cursor */
        st_cx = st_x0;
        st_cy = st_y0;
    }
}

static void fm1_lcd_send_data(const void *p, uint32_t n)
{
    const uint8_t *b = p;
    if (st_cmd == 0x2Au || st_cmd == 0x2Bu) {
        while (n-- && st_argn < 4u)
            st_arg[st_argn++] = *b++;
        if (st_argn == 4u) {
            uint16_t a = (uint16_t)(st_arg[0] << 8 | st_arg[1]);
            uint16_t e = (uint16_t)(st_arg[2] << 8 | st_arg[3]);
            if (st_cmd == 0x2Au) {
                st_x0 = a;
                st_x1 = e;
            } else {
                st_y0 = a;
                st_y1 = e;
            }
        }
        return;
    }
    if (st_cmd != 0x2Cu && st_cmd != 0x3Cu)
        return;
    for (; n >= 2u; n -= 2u, b += 2u) {
        if (st_cx < 240u && st_cy < 240u)
            emu_fb[st_cy * 240u + st_cx] = (uint16_t)(b[0] << 8 | b[1]);
        if (++st_cx > st_x1) {
            st_cx = st_x0;
            if (++st_cy > st_y1)
                st_cy = st_y0;
        }
    }
    emu_fb_gen++;
}

/* ------------------------------------------------------------ audio HAL --- */
/* audio.c's ISR glue compiles against these but is never driven; the worklet
 * pulls mix_block through emu_render instead. */
#define FM1_AUDIO_HALF 0x80u
static void fm1_audio_init(int32_t *b, uint32_t n, void (*i)(void), uint32_t p)
{
    (void)b; (void)n; (void)i; (void)p;
}
static uint8_t fm1_audio_pending(void) { return 0; }
static void fm1_audio_ack_aux(uint8_t p) { (void)p; }
static uint32_t fm1_audio_free_half(void) { return 0; }
static void fm1_audio_ack_half(void) {}
static void fm1_audio_stop(void) {}
void isr_alnk0(void) {}

/* ----------------------------------------------------------- system HAL --- */
static void fm1_reboot(void) {}
static void fm1_enter_uboot(void) {}

/* ------------------------------------------------------ firmware sources --- */
#include "felucca_tables.h"
#include "libc.c"
#include "lcd.c"
#include "gfx.c"
#include "core.h"

/* the flash image (declared before engines.c: eng_sample.c reads the user
 * sample slots through SMP_USER_XIP, which on hardware is the memory-mapped
 * XIP window — here it points into the same RAM image the storage uses) */
#define EMU_FLASH_BASE 0x90000u
#define EMU_FLASH_SIZE 0x70000u
static uint8_t emu_flash[EMU_FLASH_SIZE];
static volatile uint32_t emu_flash_gen;
#define SMP_USER_XIP(k) (emu_flash + (SMP_USER_BASE - EMU_FLASH_BASE) + (k) * SMP_USER_SIZE)

#include "engines.c"
#include "drums.c"
#include "params.c"
#include "voice.c"
#include "slicer.c"
#include "fx.c"
#define FELUCCA_OTA 0
#define FELUCCA_CDC 0
#define FELUCCA_UAC 0
#include "usb.c"
#define FELUCCA_ARRANGER 1
#include "arranger.c"
#include "seq.c"
#include "audio.c"
#include "panel.c"
#include "ui.c"
#include "ui_song.c"
#include "ui_studio.c"
#include "icons.c"
#ifndef FELUCCA_QUEST
#define FELUCCA_QUEST 1          /* the web build always has the skin's C screens */
#endif
#include "ui_quest.c"
#include "ui_draw.c"
#include "ui_layers.c"
#include "ui_menu.c"
#include "ui_input.c"
/* ---- flash: a RAM image of the FM-1's persisted areas ----
 * storage.c's A/B sector scheme runs unchanged on this array; JS loads a
 * saved image into it before boot and writes it back to IndexedDB whenever
 * emu_flash_gen moves, so projects, autosave, settings and user presets
 * survive a reload. Offsets are the real flash map (0x97000..0xFEFFF). */
#define FELUCCA_FLASH 1
static uint8_t flash_ok;
#define FL_FAR(f) f
static uint32_t fl_jedec_ram(void) { return 0x856014u; }   /* the expected part: flash_ok */
static void fl_plain_window_init(void) {}
static int emu_fl_ok(uint32_t off, uint32_t n)
{
    return off >= EMU_FLASH_BASE && n <= EMU_FLASH_SIZE && off - EMU_FLASH_BASE <= EMU_FLASH_SIZE - n;
}
static int st_read(uint32_t off, void *dst, uint32_t n)
{
    if (!emu_fl_ok(off, n))
        return -8;
    memcpy(dst, emu_flash + (off - EMU_FLASH_BASE), n);
    return 0;
}
static int st_erase(uint32_t off)
{
    if (!emu_fl_ok(off, 4096u))
        return -8;
    memset(emu_flash + (off - EMU_FLASH_BASE), 0xFF, 4096u);
    emu_flash_gen++;
    return 0;
}
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    if (!emu_fl_ok(off, n))
        return -8;
    memcpy(emu_flash + (off - EMU_FLASH_BASE), src, n);
    emu_flash_gen++;
    return 0;
}
#include "storage.c"
#include "upreset.c"
#include "project.c"
#include "splash.c"

/* ------------------------------------------------------------- web entry --- */
static float emu_audio_f[2u * 512u];
static int32_t emu_mix[2u * CTL];
static uint32_t emu_ms_acc;

/* power-on defaults, as main.c felucca_init() */
static void emu_felucca_init(void)
{
    uint32_t i;
    for (i = 0; i < G_COUNT; i++)
        song.g[i] = GP[i].def;
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        track_defaults(t);
        if (i < NPART) {
            set_engine_of(t, TRK_DEF[i][0]);
            apply_preset_to(t, TRK_DEF[i][1]);
            t->engine = t->eng_req;
        }
        track_defaults_steps(t);
    }
    TDRUM->p[P_E0] = DRUM_DEFAULT_KIT;
    song.sel = 0;
    song.master_q12 = 2048;
    autosave_resume();
    song.g[G_SYNC] = (int16_t)lights_sync;
    layers_init();
    go_home();
    ui.force = 1;
}

WASM_EXPORT("emu_init") void emu_init(void)
{
    persist_boot();
    settings_init();
    lcd_init();
    sloop_splash();
    fm1_input_init();
    fm1_adc_init();
    panel_init();
    emu_felucca_init();
}

WASM_EXPORT("emu_start") void emu_start(void)   /* after the splash has shown */
{
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.force = 1;
}

WASM_EXPORT("emu_frame") void emu_frame(void)   /* the main loop's UI pass */
{
    {   /* master volume as the pot in fm1_main */
        uint32_t k10 = (uint32_t)fm1_adc_read(FM1_ADC_MASTER);
        song.master_q12 = (k10 * k10) >> 8;
    }
    {   /* battery: a steady healthy reading */
        int32_t b = fm1_adc_read(FM1_ADC_BATT);
        song.batt_raw = song.batt_raw ? song.batt_raw + (b - song.batt_raw) / 32 : b;
    }
    ui_input();
    ui_leds();
    ui_draw();
    autosave_tick();
    sections_flush();
}

WASM_EXPORT("emu_render") void emu_render(uint32_t n)
{
    uint32_t k, i;
    if (n > 512u)
        n = 512u;
    for (k = 0; k + CTL <= n; k += CTL) {
        mix_block(emu_mix, CTL);
        for (i = 0; i < CTL; i++) {
            int32_t l = emu_mix[2u * i], r = emu_mix[2u * i + 1u];
            if (i & 1u)
                scope_buf[scope_w++ & (SCOPE_N - 1u)] = (int16_t)(l > 32767 ? 32767 : l < -32768 ? -32768 : l);
            emu_audio_f[2u * (k + i)] = (float)l * (1.0f / 32768.0f);
            emu_audio_f[2u * (k + i) + 1u] = (float)r * (1.0f / 32768.0f);
        }
    }
    emu_samples += n;
    emu_ms_acc += n * 1000u;
    while (emu_ms_acc >= FS) {
        emu_ms_acc -= FS;
        fm1_ms++;
    }
}

WASM_EXPORT("emu_input") void emu_input(uint32_t notes, uint32_t buttons)
{
    uint32_t pn = fm1_in.notes, pb = fm1_in.buttons;
    fm1_in.notes = notes & 0x7FFFFFFu;
    fm1_in.buttons = buttons & 0x3FFFu;
    fm1_in.notes_pressed |= notes & ~pn;
    fm1_in.pressed |= buttons & ~pb;
    fm1_in.released |= pb & ~buttons;
}

WASM_EXPORT("emu_enc") void emu_enc(uint32_t e, int32_t steps)
{
    if (e < FM1_NENC)
        fm1_in.enc_steps[e] = (int16_t)(fm1_in.enc_steps[e] + steps);
}

WASM_EXPORT("emu_adc_set") void emu_adc_set(uint32_t ch, int32_t v)
{
    emu_adc_v[ch & 7u] = v;
}

WASM_EXPORT("emu_midi") void emu_midi(uint32_t pkt)   /* a USB-MIDI event packet */
{
    midi_in_q[mi_w++ % MQ] = pkt;
}

WASM_EXPORT("emu_fb_ptr") uint32_t emu_fb_ptr(void) { return (uint32_t)(uintptr_t)emu_fb; }
WASM_EXPORT("emu_fb_gen") uint32_t emu_fb_gen_get(void) { return emu_fb_gen; }
WASM_EXPORT("emu_audio_ptr") uint32_t emu_audio_ptr(void) { return (uint32_t)(uintptr_t)emu_audio_f; }
WASM_EXPORT("emu_led_ptr") uint32_t emu_led_ptr(void) { return (uint32_t)(uintptr_t)fm1_led; }
WASM_EXPORT("emu_led_dim_ptr") uint32_t emu_led_dim_ptr(void) { return (uint32_t)(uintptr_t)fm1_led_dim; }
WASM_EXPORT("emu_flash_ptr") uint32_t emu_flash_ptr(void) { return (uint32_t)(uintptr_t)emu_flash; }
WASM_EXPORT("emu_flash_size") uint32_t emu_flash_size(void) { return EMU_FLASH_SIZE; }
WASM_EXPORT("emu_flash_gen") uint32_t emu_flash_gen_get(void) { return emu_flash_gen; }
