// FM1 Quest · simulator. The FM-1 3D faceplate (fm1-3d.js, from the FM1 Quest lab, as in winampFM) drives the REAL FM1 Quest firmware: fm1quest.wasm
// (firmware/ built by firmware/build-wasm.sh) runs inside an AudioWorklet (fm1-worklet.js), its 240x240 framebuffer goes onto the glass of the
// model, every press / key / knob turn (mouse, computer keyboard, a MIDI keyboard or an FM-1 on USB) goes into the firmware, and the model's LEDs
// are the firmware's own LED matrix. What you see here is what the device draws.
(function (G) {
  'use strict';
  const host = document.getElementById('sim'); if (!host) return;
  const KN = [['master', 'MASTER'], ['select', 'SELECT'], ['presets', 'PRESETS'], ['algo', 'ALGORITHM'], ['k1', 'KNOB 1'], ['k2', 'KNOB 2'], ['k3', 'KNOB 3'], ['k4', 'KNOB 4']];
  host.innerHTML = `<div class="stool"><div class="r1"></div></div>
<div class="plate"><div class="bezel"><div class="lcdslot"><div class="lcdbox"></div></div><div class="pgchip" hidden></div><div class="rowchip" hidden></div></div>
<div class="kvals" hidden>${KN.map(([id, l]) => `<span data-l="${id}">${l}</span><span data-v="${id}"></span>`).join('')}<span id="s-oct">OCT 0</span></div></div>`;
  const plate = host.querySelector('.plate'), slot = host.querySelector('.lcdslot'), lcdbox = host.querySelector('.lcdbox');
  const clamp = (v, a, b) => Math.max(a, Math.min(b, v));

  // ---------------------------------------------------------------- the firmware
  const LABID = ['octm', 'octp', 'fx', 'sel', 'env', 'lfo', 'edit', 'glo', 'home', 'save', 'arp', 'seq', 'play', 'rec'];   // firmware button index -> the model's id
  const BTNI = {}; LABID.forEach((n, i) => { BTNI[n] = i; });
  const ENC = { select: 0, algo: 1, k1: 2, k2: 3, k3: 4, k4: 5, presets: 6 };
  const WHITE_IDS = [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26], BLACK_IDS = [1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25];
  const KEYMAP = [
    [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1],
    [5, 11, 4, 10, 3, 9, 2, 8, -1, -1, -1],
    [34, 35, 36, 37, 38, 40, 39, 13, 7, 6, 12],
    [23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33],
    [0, 1, 15, 14, 17, 16, 19, 18, 20, 21, 22],
    [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1]];
  const W = { on: false, node: null, ctx: null, notes: 0, btns: 0, master: 820, ledv: new Uint8Array(11), dimv: new Uint8Array(11), frames: 0 };
  const send = (m) => { if (W.node) W.node.port.postMessage(m); };
  const input = () => send({ t: 'input', notes: W.notes, buttons: W.btns });
  W.btn = (id, down) => { const b = BTNI[id]; if (b === undefined) return; if (down) W.btns |= 1 << b; else W.btns &= ~(1 << b); input(); };
  W.key = (i, black, down) => { const n = (black ? BLACK_IDS : WHITE_IDS)[i]; if (n === undefined) return; if (down) W.notes |= 1 << n; else W.notes &= ~(1 << n); input(); };
  W.knob = (id, dir) => {
    if (id === 'master') { W.master = clamp(W.master + dir * 32, 0, 1023); send({ t: 'adc', ch: 4, v: W.master }); return; }
    const e = ENC[id]; if (e !== undefined) send({ t: 'enc', e, steps: dir > 0 ? 1 : -1 });
  };
  W.midi = (d) => { const st = d[0] | 0; if (st < 0x80 || st >= 0xF0) return; send({ t: 'midi', p: ((st >> 4) & 15) | st << 8 | (d[1] & 127) << 16 | (d[2] & 127) << 24 }); };   // one USB-MIDI packet: CIN, status, data
  G.FM1W = W;
  // the line under the model: PRESET n · TRACK n · OCT n · MASTER n (read from the firmware)
  W.status = () => { const st = W.st | 0, sel = st & 15, oct = ((st >> 4) & 15) - 8; return { preset: ((st >> 8) & 4095) + 1, track: sel < 3 ? String(sel + 1) : 'D', oct, master: Math.round(W.master * 100 / 1023) }; };

  // the glass: the firmware's framebuffer, or "click to power on" (a browser starts audio only after a gesture)
  const cv = document.createElement('canvas'); cv.width = cv.height = 240; cv.className = 'cv';
  cv.style.cssText = 'position:absolute;inset:0;width:100%;height:100%;image-rendering:pixelated;background:#000;cursor:pointer';
  lcdbox.append(cv);
  const x = cv.getContext('2d'), img = x.createImageData(240, 240);
  const say = (a, b) => { x.fillStyle = '#000'; x.fillRect(0, 0, 240, 240); x.textAlign = 'center'; x.fillStyle = '#1ce61c'; x.font = '16px "Pixelify Sans", monospace'; x.fillText(a, 120, 112); x.fillStyle = '#b4b6ca'; x.font = '11px "Pixelify Sans", monospace'; x.fillText(b, 120, 134); };
  const EN = window.WFM_LANG === 'en';
  say('FM1 QUEST', EN ? 'CLICK TO POWER ON' : 'CLIQUE PARA LIGAR');
  W.screen = cv;                                                       // (the launch page's floating recorder reads it, with W.tap)
  const draw = (fb) => { const d = img.data; for (let k = 0; k < 57600; k++) { const v = fb[k]; d[4 * k] = (v >> 11 & 31) * 255 / 31 | 0; d[4 * k + 1] = (v >> 5 & 63) * 255 / 63 | 0; d[4 * k + 2] = (v & 31) * 255 / 31 | 0; d[4 * k + 3] = 255; } x.putImageData(img, 0, 0); };
  const idb = () => new Promise((res, rej) => { const r = indexedDB.open('fm1quest-sim', 1); r.onupgradeneeded = () => r.result.createObjectStore('flash'); r.onsuccess = () => res(r.result); r.onerror = () => rej(r.error); });
  const loadFlash = async () => { try { const db = await idb(); return await new Promise((res) => { const q = db.transaction('flash').objectStore('flash').get('image'); q.onsuccess = () => res(q.result || null); q.onerror = () => res(null); }); } catch (e) { return null; } };
  const saveFlash = async (buf) => { try { const db = await idb(); db.transaction('flash', 'readwrite').objectStore('flash').put(buf, 'image'); } catch (e) {} };
  W.resetFlash = async () => { try { const db = await idb(); db.transaction('flash', 'readwrite').objectStore('flash').delete('image'); } catch (e) {} location.reload(); };
  let starting = false;
  const start = async () => {
    if (starting || W.on) return; starting = true;
    try {
      say('FM1 QUEST', EN ? 'POWERING ON…' : 'LIGANDO…');
      W.ctx = new AudioContext({ sampleRate: 44100, latencyHint: 'interactive' });
      await W.ctx.audioWorklet.addModule('fm1-worklet.js?v=' + Date.now());
      const [wasmBytes, flashImage] = await Promise.all([(await fetch('fm1quest.wasm?v=' + Date.now(), { cache: 'no-store' })).arrayBuffer(), loadFlash()]);
      W.node = new AudioWorkletNode(W.ctx, 'sloop', { outputChannelCount: [2], processorOptions: { wasmBytes, flashImage } });
      W.node.port.onmessage = (ev) => {
        const m = ev.data;
        if (m.fx) W.fx = m.fx;
        if (m.t === 'frame') { W.frames++; draw(m.fb); W.ledv.set(m.leds); W.dimv.set(m.dims); W.st = m.st; W.age = m.age; }
        else if (m.t === 'leds') { W.ledv.set(m.leds); W.dimv.set(m.dims); W.st = m.st; W.age = m.age; }
        if (m.st !== undefined) { const bl = (m.st >> 24) & 15; if (bl && bl !== W.bl) { W.bl = bl; lcdbox.style.filter = bl >= 10 ? '' : `brightness(${bl / 10})`; } }   // the menu's BRIGHTNESS
        else if (m.t === 'flash') saveFlash(m.data);
      };
      W.gain = W.ctx.createGain(); W.gain.gain.value = W.muted ? 0 : 1;
      W.node.connect(W.gain); W.gain.connect(W.ctx.destination);
      try { W.tap = W.ctx.createMediaStreamDestination(); W.node.connect(W.tap); } catch (e) {}       // a copy of the sound (for a recorder)
      W.an = W.ctx.createAnalyser(); W.an.fftSize = 512; W.gain.connect(W.an);                     // (the tutorial's level meter)
      await W.ctx.resume();
      send({ t: 'adc', ch: 4, v: W.master });
      W.boot = () => send({ t: 'start' });
      if (!/[?&]splash\b/.test(location.search)) setTimeout(W.boot, 900);      // ?splash: the boot screen stays until FM1W.boot() (to look at it)
      W.on = true; cv.style.cursor = 'default';
      document.dispatchEvent(new CustomEvent('wfm-power'));
    } catch (e) { starting = false; say('ERRO', String(e.message || e).slice(0, 38)); }
  };
  W.start = start;
  W.setMaster = (v) => { W.master = clamp(Math.round(v), 0, 1023); send({ t: 'adc', ch: 4, v: W.master }); };   // (the tutorial's volume knob)
  W.setMute = (m) => { W.muted = m; if (W.gain) W.gain.gain.value = m ? 0 : 1; };
  cv.addEventListener('pointerdown', start);

  // ---------------------------------------------------------------- the api the 3D model talks to (the same names as the Quest lab's simulator)
  const press = {}, wDown = new Set(), bDown = new Set(), leds = {};
  const sim = { latched: new Set(), ang: {} };
  const BLAB = ['OP1', 'OP2', 'OP3', 'OP4', 'OP5', 'OP6', 'PIT', 'GLO', 'MONO', 'POLY', ''];
  const WK = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '⇧5', '⇧6', '⇧7', '⇧8', '⇧9', '⇧0'];
  const btnDown = (id) => { if (!W.on) start(); if (press[id]) return; press[id] = true; W.btn(id, 1); };
  const btnUp = (id) => { if (!press[id]) return; delete press[id]; W.btn(id, 0); };
  const toggleLatch = (id) => { if (sim.latched.has(id)) { sim.latched.delete(id); btnUp(id); } else { sim.latched.add(id); btnDown(id); } };
  const keyPress = (i, black) => { if (!W.on) start(); (black ? bDown : wDown).add(i); W.key(i, black, 1); };
  const keyRelease = (i, black) => { (black ? bDown : wDown).delete(i); W.key(i, black, 0); };
  const knobTurn = (id, dir) => {
    if (!W.on) start();
    sim.ang[id] = ((sim.ang[id] || 0) + dir * (id === 'master' ? 9 : 15));
    if (id === 'master') sim.ang.master = (W.master / 1023) * 300 - 150;
    W.knob(id, dir);
  };
  // what each control does in FM1 Quest (the balloon over the model), English or Portuguese
  const KD = EN ? {
    master: 'master volume',
    select: 'on a page: the next page of its family|on HOME: the tempo',
    presets: 'the sound of the selected hero: the CDs roll by',
    algo: 'the hero: 1 · 2 · 3 · DRUMS',
    k1: 'on HOME: swing (on the Party menu: FORM, who is on stage)|on a page: its first value', k2: 'on HOME: level (on the Party menu: LEAD)|on a page: its second value',
    k3: 'on HOME: steps (on the Party menu: TURN, every N bars)|on a page: its third value', k4: 'on HOME: pan (on the Party menu: STYLE)|on a page: its fourth value',
  } : {
    master: 'volume geral',
    select: 'numa página: a próxima da família|na HOME: o tempo',
    presets: 'o som do herói selecionado: os CDs rolam',
    algo: 'o herói: 1 · 2 · 3 · DRUMS',
    k1: 'na HOME: swing (no menu Party: FORM, quem está no palco)|numa página: o primeiro valor', k2: 'na HOME: nível (no menu Party: LEAD)|numa página: o segundo valor',
    k3: 'na HOME: passos (no menu Party: TURN, a cada N compassos)|numa página: o terceiro valor', k4: 'na HOME: pan (no menu Party: STYLE)|numa página: o quarto valor',
  };
  const BD = EN ? {
    home: 'tapped on the tracks page: the next home (village, party, boss, flight)|held: the menu',
    play: 'plays / stops the music (the party sets out)', rec: 'arms the REC: the boss fight · again: closes the loop|held: clears the track',
    fx: 'held + key: a performance effect', sel: 'held + key: the scale', glo: 'held + key: mute / solo · KNOB 1–4: the levels',
    edit: 'held + key: erase that note', seq: 'held + key: a step on / off', arp: 'held + key: the arpeggiator (it stays on when the sound changes)', save: 'held: song sections',
    env: 'the envelope pages', lfo: 'the LFO pages',
    octm: 'click: octave down|in the flight: boost', octp: 'click: octave up|in the flight: fire the missiles',
  } : {
    home: 'tocado na página das faixas: a próxima home (vila, grupo, chefe, voo)|segurado: o menu',
    play: 'toca / para a música (o grupo parte)', rec: 'arma o REC: a luta com o chefe · de novo: fecha o loop|segurado: apaga a faixa',
    fx: 'segurado + tecla: efeito de performance', sel: 'segurado + tecla: a escala', glo: 'segurado + tecla: mute / solo · KNOB 1–4: os volumes',
    edit: 'segurado + tecla: apaga a nota', seq: 'segurado + tecla: liga / desliga o passo', arp: 'segurado + tecla: o arpejador (continua ligado ao trocar o som)', save: 'segurado: seções da música',
    env: 'as páginas de envelope', lfo: 'as páginas de LFO',
    octm: 'clique: oitava abaixo|no voo: boost', octp: 'clique: oitava acima|no voo: dispara os mísseis',
  };
  const BSC = { fx: 'F', sel: 'Enter', env: 'E', lfo: 'L', edit: 'D', glo: 'G', home: 'H', save: 'S', arp: 'A', seq: 'Q', play: 'Espaço', rec: 'R', octm: 'Z', octp: 'X' };
  const BNM = { fx: 'FX', sel: 'SEL', env: 'ENV', lfo: 'LFO', edit: 'EDIT', glo: 'GLO', home: 'HOME', save: 'SAVE', arp: 'ARP', seq: 'SEQ', play: 'PLAY/STOP', rec: 'REC', octm: 'OCT−', octp: 'OCT+' };
  const KSC = { select: '[ ]', presets: '; \'', algo: ', .', master: '- =', k1: '⇧1 + ↑↓', k2: '⇧2 + ↑↓', k3: '⇧3 + ↑↓', k4: '⇧4 + ↑↓' };
  const describe = (kind, id, i) => {
    if (kind === 'btn') return { label: BNM[id] || id, sc: BSC[id], lines: (BD[id] || '').split('|') };
    if (kind === 'knob') return { label: (KN.find(k => k[0] === id) || [0, id])[1], sc: KSC[id], lines: (KD[id] || '').split('|').concat(['arraste para cima/baixo ou role o mouse']) };
    if (kind === 'wk') return { label: 'TECLA ' + (i + 1), sc: WK[i], lines: ['toca a nota (com um botão segurado: a camada dele)'] };
    if (kind === 'bk') return { label: BLAB[i] || '—', lines: ['tecla de cima'] };
    return { label: '', lines: [] };
  };
  const tip = document.createElement('div'); tip.className = 'tip'; tip.hidden = true; document.body.append(tip);
  const showTip = (d, px, py) => { tip.innerHTML = `<div class="h"><b>${d.label}</b>${d.sc ? `<kbd>${d.sc}</kbd>` : ''}</div>` + d.lines.filter(Boolean).map(l => `<div class="d">${l}</div>`).join(''); tip.hidden = false; moveTip(px, py); };
  const moveTip = (px, py) => { const w = tip.offsetWidth, h = tip.offsetHeight; let a = px + 16, b = py + 18; if (a + w > innerWidth - 8) a = px - w - 12; if (b + h > innerHeight - 8) b = py - h - 12; tip.style.left = Math.max(4, a) + 'px'; tip.style.top = Math.max(4, b) + 'px'; };
  const hideTip = () => { tip.hidden = true; };
  const fit = () => {};
  // the firmware's LED matrix -> the model (2 lit, 1 glow)
  const setLed = (id, l) => { leds[id] = { l }; };
  setInterval(() => {
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
  }, 33);

  G.SIM = { sim, api: { btnDown, btnUp, keyPress, keyRelease, knobTurn, toggleLatch, press, wDown, bDown, leds, BLAB, WK, host, plate, slot, lcdbox, fit, set3d: null, describe, showTip, moveTip, hideTip } };

  // ---------------------------------------------------------------- the computer keyboard (the same letters as the FM1 Quest simulator)
  const CODEB = { KeyF: 'fx', Enter: 'sel', NumpadEnter: 'sel', KeyE: 'env', KeyL: 'lfo', KeyD: 'edit', KeyG: 'glo', KeyH: 'home', KeyS: 'save', KeyA: 'arp', KeyQ: 'seq', KeyP: 'play', Space: 'play', KeyR: 'rec', KeyZ: 'octm', KeyX: 'octp' };
  const DIG = ['Digit1', 'Digit2', 'Digit3', 'Digit4', 'Digit5', 'Digit6', 'Digit7', 'Digit8', 'Digit9', 'Digit0'];
  const KNK = { BracketLeft: ['select', -1], BracketRight: ['select', 1], Semicolon: ['presets', -1], Quote: ['presets', 1], Comma: ['algo', -1], Period: ['algo', 1], Minus: ['master', -1], Equal: ['master', 1] };
  const heldKey = {}; let armed = null;
  const typing = (e) => /input|textarea|select/i.test((e.target && e.target.tagName) || '');
  addEventListener('keydown', (e) => {
    if (typing(e) || e.metaKey || e.ctrlKey || e.altKey) return;
    if (!W.on && !e.repeat) start();
    const di = DIG.indexOf(e.code);
    if (di >= 0) { e.preventDefault(); if (e.repeat) return; if (e.shiftKey && di < 4) { armed = 'k' + (di + 1); G.dispatchEvent(new CustomEvent('wfm-armed', { detail: armed })); return; } const w = e.shiftKey ? 10 + (di - 4) : di; heldKey[e.code] = w; keyPress(w, false); return; }
    if (KNK[e.code]) { e.preventDefault(); knobTurn(KNK[e.code][0], KNK[e.code][1]); return; }
    if (armed && (e.code === 'ArrowUp' || e.code === 'ArrowDown')) { e.preventDefault(); knobTurn(armed, e.code === 'ArrowUp' ? 1 : -1); return; }
    const b = CODEB[e.code]; if (b) { e.preventDefault(); if (!e.repeat) btnDown(b); }
  }, true);
  addEventListener('keyup', (e) => {
    const hw = heldKey[e.code]; if (hw !== undefined) { delete heldKey[e.code]; keyRelease(hw, false); return; }
    const b = CODEB[e.code]; if (b) { e.preventDefault(); btnUp(b); }
  }, true);

  // ---------------------------------------------------------------- MIDI: notes of a MIDI keyboard (or a real FM-1 on USB) go into the firmware
  W.midiOn = async () => {
    if (!navigator.requestMIDIAccess) throw new Error('este navegador não tem Web MIDI');
    const acc = await navigator.requestMIDIAccess();
    const bind = () => { let n = 0; acc.inputs.forEach((p) => { n++; p.onmidimessage = (m) => { if (!W.on) return; const d = Array.from(m.data); if (d[0] >= 0xF8) return; W.midi(d); }; }); return n; };
    acc.onstatechange = bind; return bind();
  };
})(window);
