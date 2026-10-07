/* SPDX-License-Identifier: GPL-3.0-only
 * SLOOP web emulator: main thread — faceplate, input, screen, LEDs.
 * The firmware itself runs in worklet.js (wasm inside the AudioWorklet).
 *
 * The faceplate geometry is traced from the FM-1 user manual's panel drawing
 * (page 1): all positions below are in drawing units with the origin at the
 * case's top-left corner, case = 963 x 588, scaled by S. */
'use strict';

const S = 1.25;                          // px per drawing unit
const DEV = { w: 963, h: 588 };

/* ---- FM1 Quest: the interface is the firmware's own screens (firmware/src/ui_quest*.c); there is no skin switch ---- */
const FW = 'sloop';
document.getElementById('title').innerHTML = '<b>FM1 QUEST</b> &middot; SLOOP 2.3 &middot; M-VAVE FM-1 &middot; browser emulator';
document.title = 'FM1 Quest — FM-1 emulator';
document.querySelector('#power small').textContent =
    'runs the real firmware, compiled to WebAssembly · sound on';

/* ---- panel mapping (firmware/src/panel.c PANEL_DEFAULT) ----
 * label order: FX SCL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC OCT- OCT+ */
const BTN = { 'OCT-': 0, 'OCT+': 1, FX: 2, SCL: 3, ENV: 4, LFO: 5, EDIT: 6, GLO: 7,
              HOME: 8, SAVE: 9, ARP: 10, SEQ: 11, PLAY: 12, REC: 13 };

/* key matrix map (firmware/hal/fm1_input.h): ids 0..13 buttons, 14..40 note keys */
const KEYMAP = [
    [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1],
    [ 5, 11,  4, 10,  3,  9,  2,  8, -1, -1, -1],
    [34, 35, 36, 37, 38, 40, 39, 13,  7,  6, 12],
    [23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33],
    [ 0,  1, 15, 14, 17, 16, 19, 18, 20, 21, 22],
    [-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1],
];

const NNOTES = 27;                       // note ids 0..26 = F3..G5 chromatic

/* ---- geometry traced from the manual drawing (drawing units) ---- */
const ENCODERS = [                       // [label, enc matrix index, cx, cy]
    ['SELECT', 0, 172, 100],
    ['PRESETS', 6, 67, 203],
    ['ALGORITHM', 1, 170, 203],
    ['KNOB1', 2, 538, 100, 'k1'],
    ['KNOB2', 3, 647, 100, 'k2'],
    ['KNOB3', 4, 757, 100, 'k3'],
    ['KNOB4', 5, 866, 100, 'k4'],
];
const MASTER = { cx: 70, cy: 100 };      // 300° potentiometer, the 8th knob
const KNOB_D = 46;                       // cap diameter
const SCREEN = { x: 241, y: 48, w: 226, h: 233 };
const BTN_PANE = { x: 508, y: 168, w: 387, h: 135 };
const BTN_SIZE = { w: 50, h: 46 };
const BTN_GRID = {                       // [print label, firmware label]
    cols: [550, 611, 672, 733, 793, 854],
    rows: [
        { y: 207, labels: [['FX', 'FX'], ['SEL', 'SCL'], ['ENV', 'ENV'], ['LFO', 'LFO'], ['EDIT', 'EDIT'], ['GLO', 'GLO']] },
        { y: 268, labels: [['HOME', 'HOME'], ['SAVE', 'SAVE'], ['ARP', 'ARP'], ['SEQ', 'SEQ'], ['PLAY|STOP', 'PLAY'], ['REC', 'REC']] },
    ],
};
const OCT_PANE = { x: 43, y: 258, w: 160, h: 45 };
const OCT_BTNS = [['OCT-', 85], ['OCT+', 161]];   // cy 280.5, 64x33
const KEY_PANE = { x: 20, y: 340, w: 885, h: 228 };
const WKEY = { y: 497, w: 44, h: 98, x0: 63, pitch: 54.53 };
const BKEY = { y: 396.5, w: 38, h: 86,
    xs: [92, 146, 201, 310, 365, 474, 529, 583, 692, 747, 856],
    labels: ['OP1', 'OP2', 'OP3', 'OP4', 'OP5', 'OP6', 'PIT', 'GLO', 'MONO', 'POLY', ''] };
const WHITE_IDS = [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26];
const BLACK_IDS = [1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25];

let node = null, ctx = null;
let notesMask = 0, buttonsMask = 0;
let masterVal = 820;

function sendInput() { if (node) node.port.postMessage({ t: 'input', notes: notesMask, buttons: buttonsMask }); }
function noteDown(n) { notesMask |= 1 << n; sendInput(); }
function noteUp(n) { notesMask &= ~(1 << n); sendInput(); }
function btnDown(b) { buttonsMask |= 1 << b; sendInput(); }
function btnUp(b) { buttonsMask &= ~(1 << b); sendInput(); }
function enc(e, steps) { if (node && steps) node.port.postMessage({ t: 'enc', e, steps }); }
function sendMaster() { if (node) node.port.postMessage({ t: 'adc', ch: 4, v: masterVal }); }

/* ---------------------------------------------------------- faceplate --- */
const device = document.getElementById('device');
device.style.width = DEV.w * S + 'px';
device.style.height = DEV.h * S + 'px';
const px = u => u * S + 'px';
const btnEls = {}, noteEls = [];

function el(cls, x, y, w, h) {           // centered at (x, y), size w x h
    const d = document.createElement('div');
    d.className = cls;
    d.style.left = px(x);
    d.style.top = px(y);
    if (w) { d.style.width = px(w); d.style.height = px(h); }
    device.appendChild(d);
    return d;
}

function label(text, x, y) {
    const d = el('lab', x, y);
    d.textContent = text;
    return d;
}

function knobDrag(cap, turn) {           // shared drag/wheel handling
    cap.appendChild(Object.assign(document.createElement('div'), { className: 'grab' }));
    let last = null, acc = 0, touch = false;
    /* Move the knob along the direction that reads as "up" on the device.
       When the device is rotated 90° (portrait phones), its up points to the
       right of the screen, so a horizontal drag turns the knob; otherwise a
       vertical drag does. Touch gets a shorter throw for finer control. */
    cap.addEventListener('wheel', ev => { ev.preventDefault(); turn(ev.deltaY < 0 ? 1 : -1); }, { passive: false });
    cap.addEventListener('pointerdown', ev => {
        ev.preventDefault();
        touch = ev.pointerType !== 'mouse';
        last = deviceRotated ? ev.clientX : ev.clientY;
        acc = 0;
        try { cap.setPointerCapture(ev.pointerId); } catch (e) {}
    });
    cap.addEventListener('pointermove', ev => {
        if (last === null) return;
        const cur = deviceRotated ? ev.clientX : ev.clientY;
        acc += deviceRotated ? (cur - last) : (last - cur);   // up / right = increase
        last = cur;
        const step = touch ? 14 : 9;                          // px per detent
        const s = Math.trunc(acc / step);
        if (s) { acc -= s * step; turn(s); }
    });
    const up = () => { last = null; };
    cap.addEventListener('pointerup', up);
    cap.addEventListener('pointercancel', up);
}

function buildKnobs() {
    const KCOL = { k1: '#4f8fe8', k2: '#46b96b', k3: '#e0c23e', k4: '#e08a3e' };
    for (const [name, e, cx, cy, col] of ENCODERS) {
        label(name, cx, cy - 36);
        const cap = el('cap', cx, cy, KNOB_D, KNOB_D);
        if (col) cap.style.setProperty('--pcol', KCOL[col]);
        let rot = 0;
        knobDrag(cap, s => { rot += s * 18; cap.style.setProperty('--rot', rot + 'deg'); enc(e, s); });
    }
    /* MASTER: a 300° pot read by the firmware's ADC */
    label('MASTER', MASTER.cx, MASTER.cy - 36);
    const cap = el('cap', MASTER.cx, MASTER.cy, KNOB_D, KNOB_D);
    cap.style.setProperty('--pcol', '#e8b84b');
    const show = () => cap.style.setProperty('--rot', (-150 + 300 * masterVal / 1023) + 'deg');
    show();
    knobDrag(cap, s => { masterVal = Math.max(0, Math.min(1023, masterVal + s * 32)); show(); sendMaster(); });
}

function buildButtons() {
    el('pane', BTN_PANE.x + BTN_PANE.w / 2, BTN_PANE.y + BTN_PANE.h / 2, BTN_PANE.w, BTN_PANE.h);
    const keyhint = { FX: '1', SCL: '2', ENV: '3', LFO: '4', EDIT: '5', GLO: '6', HOME: '7',
                      SAVE: '8', ARP: '9', SEQ: '0', PLAY: '␣', REC: 'R', 'OCT-': 'Z', 'OCT+': 'X' };
    const makeBtn = (print, fw, cx, cy, w, h) => {
        const id = BTN[fw];
        const b = el('btn', cx, cy, w, h);
        b.innerHTML = print.split('|').join('<br>') + '<kbd>' + keyhint[fw] + '</kbd>';
        b.addEventListener('pointerdown', ev => { ev.preventDefault(); try { b.setPointerCapture(ev.pointerId); } catch (e) {} b.classList.add('down'); btnDown(id); });
        const up = () => { b.classList.remove('down'); btnUp(id); };
        b.addEventListener('pointerup', up);
        b.addEventListener('pointercancel', up);
        btnEls[id] = b;
    };
    for (const row of BTN_GRID.rows)
        row.labels.forEach(([print, fw], i) => makeBtn(print, fw, BTN_GRID.cols[i], row.y, BTN_SIZE.w, BTN_SIZE.h));
    el('pane', OCT_PANE.x + OCT_PANE.w / 2, OCT_PANE.y + OCT_PANE.h / 2, OCT_PANE.w, OCT_PANE.h);
    for (const [name, cx] of OCT_BTNS)
        makeBtn(name, name, cx, 280.5, 64, 33);
}

function buildKeyboard() {
    el('pane', KEY_PANE.x + KEY_PANE.w / 2, KEY_PANE.y + KEY_PANE.h / 2, KEY_PANE.w, KEY_PANE.h);
    const makeKey = (n, cls, cx, cy, w, h, print) => {
        const k = el('pill ' + cls, cx, cy, w, h);
        k.style.borderRadius = px(w / 2);
        k.innerHTML = '<div class="slot"></div>' + (print ? '<i>' + print + '</i>' : '');
        k.addEventListener('pointerdown', ev => { ev.preventDefault(); try { k.setPointerCapture(ev.pointerId); } catch (e) {} k.classList.add('down'); noteDown(n); });
        const up = () => { k.classList.remove('down'); noteUp(n); };
        k.addEventListener('pointerup', up);
        k.addEventListener('pointercancel', up);
        noteEls[n] = k;
    };
    WHITE_IDS.forEach((n, i) => makeKey(n, 'wk', WKEY.x0 + WKEY.pitch * i, WKEY.y, WKEY.w, WKEY.h, ''));
    BLACK_IDS.forEach((n, i) => makeKey(n, 'bk', BKEY.xs[i], BKEY.y, BKEY.w, BKEY.h, BKEY.labels[i]));
}

function buildScreen() {
    const bz = el('', SCREEN.x + SCREEN.w / 2, SCREEN.y + SCREEN.h / 2, SCREEN.w, SCREEN.h);
    bz.id = 'bezel';
    bz.style.transform = 'translate(-50%, -50%)';
    const cv = document.createElement('canvas');
    cv.id = 'screen';
    cv.width = 240;
    cv.height = 240;
    const side = Math.min(SCREEN.w, SCREEN.h) - 16;
    cv.style.width = cv.style.height = px(side);
    bz.appendChild(cv);
    return cv;
}

/* --------------------------------------------------- computer keyboard --- */
/* middle row = white keys from C4 (note id 7), top row = black keys */
const KEYNOTES = { a: 7, s: 9, d: 11, f: 12, g: 14, h: 16, j: 18, k: 19, l: 21, ';': 23, "'": 24,
                   w: 8, e: 10, t: 13, y: 15, u: 17, o: 20, p: 22, ']': 25 };
const KEYBTNS = { 1: 'FX', 2: 'SCL', 3: 'ENV', 4: 'LFO', 5: 'EDIT', 6: 'GLO', 7: 'HOME',
                  8: 'SAVE', 9: 'ARP', 0: 'SEQ', z: 'OCT-', x: 'OCT+', ' ': 'PLAY', r: 'REC' };
const KEYENC = { ArrowUp: [0, 1], ArrowDown: [0, -1], ArrowRight: [1, 1], ArrowLeft: [1, -1],
                 '[': [6, -1] };
KEYENC['\\'] = [6, 1];

const heldKeys = new Set();
window.addEventListener('keydown', ev => {
    if (ev.repeat) { if (ev.key in KEYENC && KEYENC[ev.key]) { enc(...KEYENC[ev.key]); } ev.preventDefault(); return; }
    const k = ev.key.toLowerCase();
    if (k in KEYNOTES) { heldKeys.add(k); noteDown(KEYNOTES[k]); ev.preventDefault(); }
    else if (k in KEYBTNS) { heldKeys.add(k); btnDown(BTN[KEYBTNS[k]]); btnEls[BTN[KEYBTNS[k]]]?.classList.add('down'); ev.preventDefault(); }
    else if (ev.key in KEYENC && KEYENC[ev.key]) { enc(...KEYENC[ev.key]); ev.preventDefault(); }
});
window.addEventListener('keyup', ev => {
    const k = ev.key.toLowerCase();
    if (k in KEYNOTES && heldKeys.delete(k)) noteUp(KEYNOTES[k]);
    else if (k in KEYBTNS && heldKeys.delete(k)) { btnUp(BTN[KEYBTNS[k]]); btnEls[BTN[KEYBTNS[k]]]?.classList.remove('down'); }
});
window.addEventListener('blur', () => { heldKeys.clear(); notesMask = 0; buttonsMask = 0; sendInput(); });

/* ------------------------------------------------------------- screen --- */
let c2d = null, img = null;

function drawFB(fb) {
    const d = img.data;
    for (let k = 0; k < 240 * 240; k++) {
        const v = fb[k];
        d[4 * k] = (v >> 11 & 31) * 255 / 31 | 0;
        d[4 * k + 1] = (v >> 5 & 63) * 255 / 63 | 0;
        d[4 * k + 2] = (v & 31) * 255 / 31 | 0;
        d[4 * k + 3] = 255;
    }
    c2d.putImageData(img, 0, 0);
}

/* ---------------------------------------------------------------- LEDs --- */
function applyLeds(leds, dims) {
    const lit = new Set(), glow = new Set();
    for (let p = 0; p < 11; p++)
        for (let r = 1; r < 5; r++) {
            const id = KEYMAP[r][p];
            if (id < 0) continue;
            if (leds[p] >> r & 1) lit.add(id);
            else if (dims[p] >> r & 1) glow.add(id);
        }
    for (let b = 0; b < 14; b++) {
        const el = btnEls[b];
        if (!el) continue;
        el.classList.toggle('lit', lit.has(b));
        el.classList.toggle('glow', !lit.has(b) && glow.has(b));
    }
    for (let n = 0; n < NNOTES; n++) {
        const id = 14 + n, el = noteEls[n];
        if (!el) continue;
        el.classList.toggle('lit', lit.has(id));
        el.classList.toggle('glow', !lit.has(id) && glow.has(id));
    }
}

/* ----------------------------------------------------- local saves --- */
/* The firmware's flash image lives in IndexedDB: restored before boot,
 * written back whenever the firmware saves (autosave, projects, presets). */
function idb() {
    return new Promise((res, rej) => {
        const r = indexedDB.open('sloop-emu', 1);
        r.onupgradeneeded = () => r.result.createObjectStore('flash');
        r.onsuccess = () => res(r.result);
        r.onerror = () => rej(r.error);
    });
}
const FLASH_KEY = 'image-' + FW;
async function loadFlash() {
    try {
        const db = await idb();
        const get = key => new Promise(res => {
            const q = db.transaction('flash').objectStore('flash').get(key);
            q.onsuccess = () => res(q.result || null);
            q.onerror = () => res(null);
        });
        return (await get(FLASH_KEY)) ||
               (FW === 'sloop' ? await get('image') : null);   // pre-switcher saves
    } catch (e) { return null; }
}
async function saveFlash(buf) {
    try {
        const db = await idb();
        db.transaction('flash', 'readwrite').objectStore('flash').put(buf, FLASH_KEY);
    } catch (e) {}
}

/* ------------------------------------------------------------ power on --- */
async function powerOn() {
    document.getElementById('power').remove();
    ctx = new AudioContext({ sampleRate: 44100, latencyHint: 'interactive' });
    await ctx.audioWorklet.addModule('worklet.js');
    const [wasmBytes, flashImage] = await Promise.all([
        (await fetch(FW + '.wasm')).arrayBuffer(),
        loadFlash(),
    ]);
    node = new AudioWorkletNode(ctx, 'sloop', {
        outputChannelCount: [2],
        processorOptions: { wasmBytes, flashImage },
    });
    node.port.onmessage = ev => {
        const m = ev.data;
        if (m.t === 'frame') { drawFB(m.fb); applyLeds(m.leds, m.dims); }
        else if (m.t === 'leds') { applyLeds(m.leds, m.dims); }
        else if (m.t === 'flash') saveFlash(m.data);
    };
    node.connect(ctx.destination);
    await ctx.resume();
    sendMaster();
    setTimeout(() => node.port.postMessage({ t: 'start' }), 900);   // splash dwell, as fm1_main
    initMidi();
}
document.getElementById('power').addEventListener('click', powerOn, { once: true });

/* --------------------------------------------------------------- MIDI --- */
function initMidi() {
    if (!navigator.requestMIDIAccess) return;
    navigator.requestMIDIAccess({ sysex: false }).then(acc => {
        const hook = () => acc.inputs.forEach(inp => {
            inp.onmidimessage = ev => {
                const d = ev.data;
                if (!d || !d.length || d[0] >= 0xF0) return;
                const pkt = (d[0] >> 4) | (d[0] << 8) | ((d[1] || 0) << 16) | ((d[2] || 0) << 24);
                node.port.postMessage({ t: 'midi', p: pkt >>> 0 });
            };
        });
        hook();
        acc.onstatechange = hook;
    }).catch(() => {});
}

/* ----------------------------------------------- fit small screens --- */
/* On phones and tablets the device fills the whole viewport, stretched
   on each axis independently so there are no bars — any screen shape,
   fullscreen included. In portrait it is rotated 90° first. deviceRotated
   tells knobDrag which screen axis is the device's "up". */
let deviceRotated = false;
function safeInsets() {
    const cs = getComputedStyle(document.documentElement);
    const p = n => parseFloat(cs.getPropertyValue(n)) || 0;
    return { t: p('--sat'), b: p('--sab'), l: p('--sal'), r: p('--sar') };
}
function fitDevice() {
    const W = DEV.w * S, H = DEV.h * S;
    /* the true viewport: visualViewport beats innerWidth/Height on iOS
       (stale after home-screen launches), and in standalone mode the web
       view covers the physical screen, so screen.width/height — oriented
       to match — is the authoritative size (iOS under-reports the viewport
       there by the status-bar strip) */
    const vv = window.visualViewport;
    let vw = Math.max(vv ? vv.width : 0, window.innerWidth);
    let vh = Math.max(vv ? vv.height : 0, window.innerHeight);
    const standalone = window.navigator.standalone === true ||
        window.matchMedia('(display-mode: standalone)').matches ||
        window.matchMedia('(display-mode: fullscreen)').matches;
    if (standalone && window.screen) {
        const big = Math.max(screen.width, screen.height);
        const small = Math.min(screen.width, screen.height);
        if (vh >= vw) {                            // portrait
            vw = Math.max(vw, small);
            vh = Math.max(vh, big);
        } else {
            vw = Math.max(vw, big);
            vh = Math.max(vh, small);
        }
    }
    /* the fit rectangle: clear of the Dynamic Island / status bar (and a
       side notch in landscape), but flush with the bottom edge — the home
       indicator floats translucently over the keys */
    const si = safeInsets();
    const ax = si.l, ay = si.t;
    const aw = vw - si.l - si.r, ah = vh - si.t;
    const sFlat = Math.min(aw / W, ah / H);        // uniform, as-is
    const sRot = Math.min(aw / H, ah / W);         // uniform, rotated 90°
    const fs = !!(document.fullscreenElement || document.webkitFullscreenElement);
    if (sFlat >= 1 && !fs) {                       // fits at full size in a normal page: no scaling
        document.body.classList.remove('compact');
        device.style.transform = '';
        device.style.left = device.style.top = '';
        deviceRotated = false;
        document.body.classList.remove('rotated');
        return;
    }
    document.body.classList.add('compact');
    device.style.left = (ax + aw / 2) + 'px';
    device.style.top = (ay + ah / 2) + 'px';
    const rotate = ah > aw && sRot > sFlat && matchMedia('(pointer: coarse)').matches;   // portrait phones only; a desktop window never rotates
    /* ONE scale on both axes (the device keeps its proportions) and centred;
       fullscreen may scale it up to fill the width or the height, whichever comes first */
    const k = Math.min(rotate ? sRot : sFlat, 3);
    if (rotate) device.style.transform = `translate(-50%, -50%) rotate(90deg) scale(${k})`;
    else device.style.transform = `translate(-50%, -50%) scale(${k})`;
    deviceRotated = rotate;
    document.body.classList.toggle('rotated', rotate);
}
window.addEventListener('resize', fitDevice);
window.addEventListener('orientationchange', () => setTimeout(fitDevice, 100));
if (window.visualViewport)
    window.visualViewport.addEventListener('resize', fitDevice);
/* iOS standalone (home-screen) launches settle their viewport late and may
   never fire resize: re-fit a few times after load, and on return visits */
window.addEventListener('pageshow', () => setTimeout(fitDevice, 50));
for (const t of [150, 500, 1200, 2500])
    setTimeout(fitDevice, t);

/* ------------------------------------------------- fullscreen toggle --- */
/* A corner button on phones. Where the Fullscreen API exists (Android
   Chrome) it toggles true fullscreen. On iPhone, where WebKit has no page
   fullscreen, it shows how to Add to Home Screen instead — and once the
   page is launched that way (standalone), the button is hidden since it is
   already chrome-free. */
(function () {
    const fsBtn = document.getElementById('fs');
    const tip = document.getElementById('fstip');
    const root = document.documentElement;
    const req = root.requestFullscreen || root.webkitRequestFullscreen;
    const exit = document.exitFullscreen || document.webkitExitFullscreen;
    const current = () => document.fullscreenElement || document.webkitFullscreenElement;
    const standalone = window.navigator.standalone === true ||
        window.matchMedia('(display-mode: standalone)').matches ||
        window.matchMedia('(display-mode: fullscreen)').matches;
    if (standalone) return;                        // already chrome-free: no button needed
    document.body.classList.add('fs-ok');          // show the corner button in compact mode
    const sync = () => { fsBtn.innerHTML = current() ? '&#x2715;' : '&#x26F6;'; setTimeout(fitDevice, 50); };
    fsBtn.addEventListener('click', () => {
        if (req) {                                 // real fullscreen (Android etc.)
            if (current()) exit.call(document);
            else req.call(root).catch(() => {});
        } else {                                    // iPhone: explain Add to Home Screen
            tip.classList.add('show');
        }
    });
    tip.addEventListener('click', () => tip.classList.remove('show'));
    document.addEventListener('fullscreenchange', sync);
    document.addEventListener('webkitfullscreenchange', sync);
})();

buildKnobs();
buildButtons();
buildKeyboard();
const screenCanvas = buildScreen();
c2d = screenCanvas.getContext('2d');

img = c2d.createImageData(240, 240);
fitDevice();
