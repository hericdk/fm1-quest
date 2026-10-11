// FM1 Quest · the 3D faceplate drives the REAL firmware.
// The firmware (quest/web/fm1quest.wasm, built from firmware/src) runs inside an AudioWorklet (worklet.js). This file puts its 240x240 framebuffer on the glass of the
// 3D model, sends it every press / key / knob turn the model receives (mouse, touch, computer keyboard, MIDI keyboard, an FM-1 on USB: they all end in the same
// four functions of the simulator), and lights the model's LEDs from the firmware's own LED matrix. Before the first click the glass says POWER ON (a browser
// only starts audio after a gesture).
(function () {
  'use strict';
  const LABID = ['octm', 'octp', 'fx', 'sel', 'env', 'lfo', 'edit', 'glo', 'home', 'save', 'arp', 'seq', 'play', 'rec'];      // firmware button index -> the model's id
  const BTN = {}; LABID.forEach((n, i) => { BTN[n] = i; });
  const ENC = { select: 0, algo: 1, k1: 2, k2: 3, k3: 4, k4: 5, presets: 6 };
  const WHITE_IDS = [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26], BLACK_IDS = [1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25];
  const KEYMAP = [
    [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1],
    [5, 11, 4, 10, 3, 9, 2, 8, -1, -1, -1],
    [34, 35, 36, 37, 38, 40, 39, 13, 7, 6, 12],
    [23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33],
    [0, 1, 15, 14, 17, 16, 19, 18, 20, 21, 22],
    [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1]];
  const W = { on: false, node: null, ctx: null, notes: 0, btns: 0, master: 820, ledv: new Uint8Array(11), dimv: new Uint8Array(11) };
  const send = (m) => { if (W.node) W.node.port.postMessage(m); };
  const input = () => send({ t: 'input', notes: W.notes, buttons: W.btns });
  W.btn = (id, down) => { const b = BTN[id]; if (b === undefined) return; if (down) W.btns |= 1 << b; else W.btns &= ~(1 << b); input(); };
  W.key = (i, black, down) => { const n = (black ? BLACK_IDS : WHITE_IDS)[i]; if (n === undefined) return; if (down) W.notes |= 1 << n; else W.notes &= ~(1 << n); input(); };
  W.knob = (id, dir) => {
    if (id === 'master') { W.master = Math.max(0, Math.min(1023, W.master + dir * 32)); send({ t: 'adc', ch: 4, v: W.master }); return; }
    const e = ENC[id]; if (e !== undefined) send({ t: 'enc', e, steps: dir > 0 ? 1 : -1 });
  };
  W.leds = (setLed) => {                                                       // the firmware's LED matrix onto the model (2 lit, 1 glow)
    const lv = {};
    for (let p = 0; p < 11; p++) for (let r = 1; r < 5; r++) {
      const id = KEYMAP[r][p]; if (id < 0) continue;
      const l = W.ledv[p] >> r & 1 ? 2 : W.dimv[p] >> r & 1 ? 1 : 0;
      if (id < 14) lv[LABID[id]] = l;
      else { const n = id - 14, wi = WHITE_IDS.indexOf(n), bi = BLACK_IDS.indexOf(n); if (wi >= 0) lv['w' + wi] = l; else if (bi >= 0) lv['b' + bi] = l; }
    }
    LABID.forEach((n) => setLed(n, lv[n] || 0));
    for (let i = 0; i < 16; i++) setLed('w' + i, lv['w' + i] || 0);
    for (let j = 0; j < 11; j++) setLed('b' + j, lv['b' + j] || 0);
  };
  window.FM1W = W;

  // ---- the glass
  const wait = setInterval(() => {
    const S = window.SIM; if (!S || !S.api || !S.api.lcdbox) return; clearInterval(wait);
    const box = S.api.lcdbox, cv = document.createElement('canvas'); cv.width = cv.height = 240;
    cv.style.cssText = 'position:absolute;inset:0;width:100%;height:100%;image-rendering:pixelated;z-index:9999;background:#000;cursor:pointer';
    box.style.position = box.style.position || 'relative';
    const vid = document.createElement('video');                                   // the demo loop: what the firmware does today; it goes away at the first click
    // the preview on the glass: village, the party against the new monsters, the goblin boss with an ult in its window, ONE hero's ult fullscreen
    // (the mage first; the other two are drawn at random, never repeated), the visualizer, the FX pages being edited; then it starts again with the same order
    const PL = ['village', 'map', 'boss_goblin', 'ULT', 'viz_sky', 'fx_pedals', 'slicer_towers'];
    const rest = [1, 2, 3, 4].sort(() => Math.random() - 0.5);
    const ULTS = [0, rest[0], rest[1]];                                              // 0 = the mage; 1..4 = archer, cleric, knight, red warrior
    let pi = 0, round = 0;
    const VQ = '__VQ__'.startsWith('__') ? '' : '__VQ__';                              // the build's content hash (page/build_page.py): the browser never shows an old preview
    const clip = () => { const n = PL[pi]; return (n === 'ULT' ? 'ult_' + ULTS[round % ULTS.length] + '.mp4' : 'p_' + n + '.mp4') + VQ; };
    vid.src = clip(); vid.muted = vid.autoplay = vid.playsInline = true; vid.setAttribute('aria-hidden', 'true');
    vid.onended = () => {
      pi++; if (pi >= PL.length) { pi = 0; round++; }
      vid.src = clip(); vid.play().catch(() => {});
    };
    vid.style.cssText = 'position:absolute;inset:0;width:100%;height:100%;object-fit:fill;image-rendering:pixelated;z-index:9998;background:#000';
    box.append(vid, cv); cv.style.background = 'transparent'; vid.play().catch(() => {});
    const keep = setInterval(() => { if (!vid.isConnected) { clearInterval(keep); return; } if (vid.paused) vid.play().catch(() => {}); }, 700);
    const x = cv.getContext('2d'), img = x.createImageData(240, 240);
    const say = (a, b) => { x.fillStyle = '#000'; x.fillRect(0, 0, 240, 240); x.fillStyle = '#f0c860'; x.font = '16px monospace'; x.textAlign = 'center'; x.fillText(a, 120, 112); x.fillStyle = '#a89080'; x.font = '10px monospace'; x.fillText(b, 120, 134); };
    const idle = () => { x.clearRect(0, 0, 240, 240); x.fillStyle = 'rgba(0,0,0,.62)'; x.fillRect(0, 214, 240, 26); x.fillStyle = '#f0c860'; x.font = '12px monospace'; x.textAlign = 'center'; x.fillText('CLICK TO POWER ON', 120, 232); };
    idle();
    const draw = (fb) => { const d = img.data; for (let k = 0; k < 57600; k++) { const v = fb[k]; d[4 * k] = (v >> 11 & 31) * 255 / 31 | 0; d[4 * k + 1] = (v >> 5 & 63) * 255 / 63 | 0; d[4 * k + 2] = (v & 31) * 255 / 31 | 0; d[4 * k + 3] = 255; } x.putImageData(img, 0, 0); };
    const idb = () => new Promise((res, rej) => { const r = indexedDB.open('fm1quest-web', 1); r.onupgradeneeded = () => r.result.createObjectStore('flash'); r.onsuccess = () => res(r.result); r.onerror = () => rej(r.error); });
    const loadFlash = async () => { try { const db = await idb(); return await new Promise((res) => { const q = db.transaction('flash').objectStore('flash').get('image'); q.onsuccess = () => res(q.result || null); q.onerror = () => res(null); }); } catch (e) { return null; } };
    const saveFlash = async (buf) => { try { const db = await idb(); db.transaction('flash', 'readwrite').objectStore('flash').put(buf, 'image'); } catch (e) {} };
    let starting = false;
    const start = async () => {
      if (starting || W.on) return; starting = true;
      try {
        const base = document.currentScript ? '' : '';
        W.ctx = new AudioContext({ sampleRate: 44100, latencyHint: 'interactive' });
        await W.ctx.audioWorklet.addModule('fm1-worklet.js');
        const [wasmBytes, flashImage] = await Promise.all([(await fetch('fm1quest.wasm' + VQ)).arrayBuffer(), loadFlash()]);
        W.node = new AudioWorkletNode(W.ctx, 'sloop', { outputChannelCount: [2], processorOptions: { wasmBytes, flashImage } });
        W.node.port.onmessage = (ev) => {
          const m = ev.data;
          if (m.t === 'frame') { if (vid.isConnected) vid.remove(); cv.style.background = '#000'; draw(m.fb); W.ledv.set(m.leds); W.dimv.set(m.dims); }
          else if (m.t === 'leds') { W.ledv.set(m.leds); W.dimv.set(m.dims); }
          else if (m.t === 'flash') saveFlash(m.data);
        };
        W.node.connect(W.ctx.destination);
        await W.ctx.resume();
        send({ t: 'adc', ch: 4, v: W.master });
        setTimeout(() => send({ t: 'start' }), 900);
        W.on = true; cv.style.cursor = 'default'; try { parent.postMessage({ fm1ga: 'sim_power_on' }, location.origin); } catch (e) {}
      } catch (e) { starting = false; say('ERROR', String(e.message || e).slice(0, 38)); }
    };
    cv.addEventListener('pointerdown', start);
    addEventListener('keydown', () => { if (!W.on) start(); }, { once: false });
    document.addEventListener('pointerdown', (ev) => { if (!W.on && ev.target.closest && ev.target.closest('.plate, .stage3d')) start(); }, true);
    // the sound of the model's old engine stays off: the firmware makes the sound now
  }, 100);
})();
