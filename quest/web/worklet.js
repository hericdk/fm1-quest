/* SPDX-License-Identifier: GPL-3.0-only
 * SLOOP web emulator: the AudioWorklet hosts the whole firmware (wasm).
 * process() renders audio with mix_block; every few blocks a UI frame runs
 * and the framebuffer + LED state are posted to the main thread. */
class SloopProcessor extends AudioWorkletProcessor {
    constructor(options) {
        super();
        const mod = new WebAssembly.Module(options.processorOptions.wasmBytes);
        this.e = new WebAssembly.Instance(mod, {}).exports;
        const img = options.processorOptions.flashImage;
        if (img && img.byteLength) {        // restore the saved flash before boot
            const fl = new Uint8Array(this.e.memory.buffer, this.e.emu_flash_ptr(), this.e.emu_flash_size());
            fl.set(new Uint8Array(img, 0, Math.min(img.byteLength, fl.length)));
        }
        this.lastFlashGen = 0;
        this.e.emu_init();
        this.lastFlashGen = this.e.emu_flash_gen();   // boot-time writes need no save
        this.fbPtr = this.e.emu_fb_ptr();
        this.audioPtr = this.e.emu_audio_ptr();
        this.ledPtr = this.e.emu_led_ptr();
        this.ledDimPtr = this.e.emu_led_dim_ptr();
        this.lastGen = -1;
        this.blocks = 0;
        this.running = true;
        this.port.onmessage = (ev) => {
            const m = ev.data;
            if (m.t === 'input') this.e.emu_input(m.notes, m.buttons);
            else if (m.t === 'enc') this.e.emu_enc(m.e, m.steps);
            else if (m.t === 'start') this.e.emu_start();
            else if (m.t === 'adc') this.e.emu_adc_set(m.ch, m.v);
            else if (m.t === 'midi') this.e.emu_midi(m.p);
            else if (m.t === 'stop') this.running = false;
        };
        this.postFrame();           // the splash, before audio starts
    }

    postFrame() {
        const gen = this.e.emu_fb_gen();
        const leds = new Uint8Array(this.e.memory.buffer, this.ledPtr, 11).slice();
        const dims = new Uint8Array(this.e.memory.buffer, this.ledDimPtr, 11).slice();
        if (gen !== this.lastGen) {
            this.lastGen = gen;
            const fb = new Uint16Array(this.e.memory.buffer, this.fbPtr, 240 * 240).slice();
            this.port.postMessage({ t: 'frame', fb, leds, dims }, [fb.buffer]);
        } else {
            this.port.postMessage({ t: 'leds', leds, dims });
        }
    }

    process(inputs, outputs) {
        const out = outputs[0];
        const n = out[0].length;             // 128
        this.e.emu_render(n);
        const a = new Float32Array(this.e.memory.buffer, this.audioPtr, 2 * n);
        const L = out[0], R = out[1] || out[0];
        for (let i = 0; i < n; i++) {
            L[i] = a[2 * i];
            R[i] = a[2 * i + 1];
        }
        if (++this.blocks % 6 === 0) {       // a UI frame every ~17 ms
            this.e.emu_frame();
            this.postFrame();
        }
        if (this.blocks % 256 === 0) {       // ~0.75 s: ship changed flash to be saved
            const gen = this.e.emu_flash_gen();
            if (gen !== this.lastFlashGen) {
                this.lastFlashGen = gen;
                const fl = new Uint8Array(this.e.memory.buffer, this.e.emu_flash_ptr(), this.e.emu_flash_size()).slice();
                this.port.postMessage({ t: 'flash', data: fl.buffer }, [fl.buffer]);
            }
        }
        return this.running;
    }
}
registerProcessor('sloop', SloopProcessor);
