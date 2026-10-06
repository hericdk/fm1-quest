/* SPDX-License-Identifier: GPL-3.0-only
 * FM1 Quest skin. Draws the RPG screens (TAVERN) from the firmware's own state:
 * worklet.js posts a snapshot of song/trk/ui/seq variables (src/quest_wasm.c) with
 * every UI frame. Nothing here changes the firmware: input still goes through the
 * faceplate (emu.js) into the firmware, and the skin only reads.
 *
 * Screens not skinned yet fall back to the firmware's own pixels (the canvas).
 * Each skinned screen cites the firmware screen it replaces. */
'use strict';
(function (G) {
const C = { bg: '#1e1418', text: '#f2e2c4', dim: '#a89080', pdim: '#c8b090', line: '#5a3a28', off: '#2e2226', offHi: '#45343a', pad: '#24181c', padC: '#3a4a78',
  panel: '#3a2620', border: '#c8963c', hi: '#f0c860', hiText: '#1e1418', rec: '#e0503c', t: ['#5a8ad0', '#7ab04e', '#f0c860', '#e08a3a'], pbg: '#45343a' };
const cc = i => C.t[((i % 4) + 4) % 4];
const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
const CLASSES = ['mage', 'archer', 'cleric', 'warrior'];        // README: track 1..4 = mage, archer, cleric, warrior
const SC = { PAGE: 0, TRACKS: 1, REC_READY: 2, COUNT_IN: 3, TAKE: 4, DRUM: 5, LAYER: 6, MENU: 7, SONG: 8, HOLD: 9 };
// snapshot layout (src/quest_wasm.c)
const I = { scr: 1, layer: 2, menu: 4, playing: 9, sel: 10, rec: 11, solo: 12, bpm: 13, swing: 14, beat: 15, pos: 16, recWait: 17, ftOn: 18, ciOn: 19, ciBeat: 20,
  recCount: 21, recTempo: 22, beatU: 24, empty: 31, trk: 32, steps: 160 };
const T = (i, k) => I.trk + 24 * i + k;                          // per-track: 0 eng,1 preset,2 level,3 mute,4 pan,5 len,6 seq_idx,7 silent,8 root,9 scale,10-13 dst/chor/dly/rev

const Q = { active: false, snap: null, txt: null, ok: false, began: false, lcd: null, cv: null, sig: '', sp: 0, t0: performance.now(), last: performance.now() };
Q.setActive = on => { Q.active = on; if (Q.lcd) apply(); };
Q.started = () => { Q.began = true; };

// ------------------------------------------------------------------ snapshot
Q.onMsg = m => {
  if (!m.snap) return;
  Q.snap = m.snap;
  const names = [];
  for (let i = 0; i < 4; i++) names.push([0, 1].map(k => { let s = ''; for (let j = 0; j < 20; j++) { const c = m.txt[(i * 2 + k) * 20 + j]; if (!c) break; s += String.fromCharCode(c); } return s; }));
  Q.txt = names; Q.qb = m.qb; Q.ok = true;
};
const sv = k => Q.snap[k];
const beatNow = () => { const s = Q.snap; if (s[I.playing]) return s[I.beat] + s[I.pos] / s[I.beatU]; return (performance.now() - Q.t0) / 1000 * (s[I.bpm] || 120) / 60; };
const stepOn = (i, j) => (Q.snap[I.steps + 2 * i + (j >> 5)] >>> (j & 31)) & 1;
function patString(i) { const len = Q.snap[T(i, 5)]; let s = ''; for (let j = 0; j < len; j++) s += stepOn(i, j) ? '1' : '0'; return s; }

// ------------------------------------------------------------------ title art: 240x240, quantized to the 16-colour scene palette
let titleURL = '';
(function () {
  const img = new Image(); img.onload = () => {
    const c = document.createElement('canvas'); c.width = c.height = 240; const x = c.getContext('2d'); x.drawImage(img, 0, 0, 240, 240);
    const d = x.getImageData(0, 0, 240, 240), pal = Scene.PALS['1c'].map(h => [parseInt(h.slice(1, 3), 16), parseInt(h.slice(3, 5), 16), parseInt(h.slice(5, 7), 16)]);
    const B = [0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5];
    for (let y = 0; y < 240; y++) for (let xx = 0; xx < 240; xx++) {
      const i = (y * 240 + xx) * 4, th = (B[(y % 4) * 4 + (xx % 4)] / 16 - .5) * 28; let best = 0, bd = 1e9;
      const r = d.data[i] + th, g = d.data[i + 1] + th, b = d.data[i + 2] + th;
      for (let k = 0; k < 16; k++) { const dr = r - pal[k][0], dg = g - pal[k][1], db = b - pal[k][2], dd = dr * dr * .3 + dg * dg * .59 + db * db * .11; if (dd < bd) { bd = dd; best = k; } }
      d.data[i] = pal[best][0]; d.data[i + 1] = pal[best][1]; d.data[i + 2] = pal[best][2];
    }
    x.putImageData(d, 0, 0); titleURL = c.toDataURL(); Q.sig = '';
  }; img.src = 'assets/title-bg-240.png';
})();

// ------------------------------------------------------------------ html helpers
const tri = c => `<i class="tri" style="border-left-color:${c || C.text}"></i>`;
const panel = (inner, st) => `<div class="panel" style="${st || ''}">${inner}</div>`;
const title = (t, c) => `<span class="ttl"${c ? ` style="color:${c}"` : ''}>${t}</span>`;
const bpmBox = () => `<div class="bpm"><b>${sv(I.bpm)}</b><span style="color:${C.dim}">bpm</span></div>`;
function posBox(beat) {
  const playing = sv(I.playing), b = Math.floor(beat), bar = Math.floor(b / 4) + 1, bt = ((b % 4) + 4) % 4;
  const pips = [0, 1, 2, 3].map(i => `<u style="background:${playing && i === bt ? C.text : C.off}"></u>`).join('');
  return `<div class="posb">${playing ? tri() : `<i class="sq" style="background:${C.dim}"></i>`}<span>${playing ? bar + '.' + (bt + 1) : '--'}</span><div class="pips">${pips}</div></div>`;
}
function knobsHTML(list) {
  return `<div class="knobs" style="grid-template-columns:repeat(${list.length},1fr)">` + list.map(k => !k.l ? '<div class="kn"></div>' :
    `<div class="kn"><canvas data-knob="${clamp(k.v, 0, 1)}" data-col="${k.color}" data-dir="1c" width="20" height="20"></canvas><span style="color:${C.dim}">${k.l}</span><span>${k.t}</span></div>`).join('') + '</div>';
}
const portrait = (cls, bd, extra) => `<canvas class="por" data-portrait="${cls}" data-dir="1c" data-bg="${C.pbg}" width="15" height="15" style="border-color:${bd};${extra || ''}"></canvas>`;

// ------------------------------------------------------------------ screens
// PARTY replaces studio_tracks_draw() (ui_studio.c): 4 rows, 16 steps in view, level, the dials swing/level/steps/pan.
function trackRows(beat) {
  const sel = sv(I.sel), playing = sv(I.playing);
  return [0, 1, 2, 3].map(i => {
    const len = sv(T(i, 5)), pos = sv(T(i, 6)) % len, silent = sv(T(i, 7)), lvl = sv(T(i, 2)), isRec = (sv(I.rec) >> i) & 1, solo = (sv(I.solo) >> i) & 1;
    const bank = playing ? Math.floor(pos / 16) : 0, dead = silent || !lvl;
    let pips = '';
    for (let j = 0; j < 16; j++) {
      const p = bank * 16 + j; let on = 0;
      if (len <= 16) { if (p < len) on = stepOn(i, p) ? 2 : 1; }
      else if (playing) { if (p < len) on = stepOn(i, p) ? 2 : 1; }
      else { const a = Math.floor(j * len / 16), z = Math.floor((j + 1) * len / 16); on = 1; for (let k = a; k < z; k++) if (stepOn(i, k)) on = 2; }
      const col = playing && p === pos ? C.text : on === 2 ? (dead ? C.dim : cc(i)) : on === 1 ? (j % 4 === 0 ? C.offHi : C.off) : '#0e0a0c';
      pips += `<div class="pip" style="background:${col}"></div>`;
    }
    const tag = isRec ? 'rec' : solo ? 'duel' : silent ? 'rest' : CLASSES[i];
    return `<div class="trow${sel === i ? ' sel' : ''}">${portrait(CLASSES[i], sel === i ? C.hi : cc(i), dead ? 'opacity:.45' : '')}
      <div class="tm"><div class="tn"><span style="color:${C.dim}">${i + 1}</span><span style="color:${sel === i ? C.text : C.dim}">${esc(Q.txt[i][0])}</span><span style="color:${isRec ? C.rec : cc(i)}">${tag}</span></div><div class="pips16">${pips}</div></div>
      <div class="hp"><span style="color:${C.dim}">hp ${Math.round(lvl * 100 / 127)}</span><div class="hpb"><div style="width:${lvl * 100 / 127}%;background:${cc(i)}"></div></div></div></div>`;
  }).join('');
}
const esc = s => s.replace(/[&<>]/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;' }[c]));
function partyKnobs() {
  const sel = sv(I.sel), swing = sv(I.swing), lvl = sv(T(sel, 2)), mute = sv(T(sel, 3)), len = sv(T(sel, 5)), pan = sv(T(sel, 4));
  return [
    { l: 'swing', color: cc(0), v: swing / 100, t: (50 + Math.floor(swing / 4)) + '%' },          // F_SWING: 50 % + value / 4 (core.h / EDITOR_PROTOCOL)
    { l: 'level', color: cc(1), v: mute ? 0 : lvl / 127, t: mute ? 0 : Math.floor(lvl * 100 / 127) },
    { l: 'steps', color: cc(2), v: (len - 1) / 63, t: len },
    { l: 'pan', color: cc(3), v: (pan + 64) / 127, t: pan }];
}
const SCREENS = {
  title: () => ({ full: true, hud: `<div class="tbg" style="${titleURL ? `background-image:url(${titleURL})` : ''}"></div><div class="tgrad"></div>
    <img class="tlogo" src="assets/logo-340.png" alt="FM1 Quest">
    <div class="tbot">${panel(`${tri()}<span>press play</span>`, 'padding:5px 11px;display:flex;align-items:center;gap:5px')}<span class="outl">based on Sloop</span></div>` }),
  party: beat => ({ bar: `${bpmBox()}${posBox(beat)}<div class="fl"></div>${title('Party')}`, scene: { key: 'party', h: 84 },
    hud: `<div class="rows">${trackRows(beat)}</div>${knobsHTML(partyKnobs())}` }),
  // REC armed / count-in replace rec_screen_draw() (ui_studio.c). Copy follows the README ("A wild LOOP appears!"); the dials are the firmware's.
  recready: beat => {
    const sel = sv(I.sel), empty = sv(I.empty), recCount = sv(I.recCount), tempo = sv(I.recTempo), len = sv(T(sel, 5)), free = empty && !tempo;
    const m = free ? 0 : recCount ? 2 : 1;
    const l1 = ['play freely: warm up', 'play a note: fight', 'press play: fight'][m], l2 = ['then rec on the 1', 'it starts the loop', '4 clicks, then rec'][m];
    const knobs = [];
    if (empty) knobs.push({ l: 'mode', color: cc(0), v: tempo ? 1 : 0, t: tempo ? 'tempo' : 'free' });
    if (!free) {
      knobs.push({ l: 'length', color: cc(1), v: (len - 1) / 63, t: len % 16 === 0 ? (len / 16) + (len === 16 ? ' bar' : ' bars') : len + ' st' });
      knobs.push({ l: 'start', color: cc(2), v: recCount ? 1 : 0, t: recCount ? 'count' : 'note' });
    }
    return { bar: `${bpmBox()}<div class="posb"><i class="sq" style="background:${C.dim}"></i><span style="color:${C.dim}">loop</span></div><div class="fl"></div><span style="color:${C.rec}">rec ready</span>`,
      scene: { key: 'rec', h: 84 },
      hud: `<div class="hh">${panel(`<span style="color:${C.rec}">A wild LOOP appears!</span><div style="display:flex;gap:9px"><span>${l1}</span><span style="color:${C.pdim}">rec: flee</span></div><span style="color:${C.pdim}">${l2}</span>`, 'padding:3px 5px;display:flex;flex-direction:column;gap:2px')}${recList(sel)}${knobs.length ? knobsHTML(knobs) : ''}</div>` };
  },
  count: beat => {
    const sel = sv(I.sel), n = 4 - Math.min(sv(I.ciBeat), 3);
    return { bar: `${bpmBox()}<div class="posb"><i class="sq" style="background:${C.dim}"></i><span style="color:${C.dim}">loop</span></div><div class="fl"></div><span style="color:${C.rec}">count-in</span>`,
      scene: { key: 'count', h: 84 }, ov: `<div class="bign"><span>${n}</span></div>`,
      hud: `<div class="hh">${panel(`<span style="color:${C.rec}">Battle in one bar!</span><div style="display:flex;gap:9px"><span>4 clicks</span><span style="color:${C.pdim}">rec: cancel</span></div>`, 'padding:3px 5px;display:flex;flex-direction:column;gap:2px')}${recList(sel)}</div>` };
  },
};
function recList(rt) {   // rec_rows(): the four tracks, the recording one framed
  return `<div class="tl">` + [0, 1, 2, 3].map(i => {
    const len = Math.min(16, sv(T(i, 5)));
    const cells = Array.from({ length: 16 }, (_, k) => `<div style="flex:1;height:4px;background:${k < len && stepOn(i, k) ? cc(i) : C.off}"></div>`).join('');
    return `<div class="tlr" style="border-color:${i === rt ? C.rec : 'transparent'}"><div style="width:6px;height:6px;flex:none;background:${cc(i)}"></div><span style="width:59px;flex:none;white-space:nowrap;color:${i === rt ? C.text : C.dim}">${esc(Q.txt[i][0])}</span><div style="display:flex;gap:1px;flex:1">${cells}</div></div>`;
  }).join('') + '</div>';
}

// ------------------------------------------------------------------ firmware strings (byte buffer, src/quest_wasm.c QB_*)
const QB = { SUB: 0, TILES: 32, DIALS: 288, COLS: 416, FOOT: 576, LANES: 640, KIT: 768, GRID: 832, DDIALS: 1100, MENU: 1240, BANK: 1620, MISC: 1920 };
const bstr = (off, max) => { let s = ''; for (let i = 0; i < max; i++) { const c = Q.qb[off + i]; if (!c) break; s += String.fromCharCode(c); } return s; };
const bu16 = off => Q.qb[off] | Q.qb[off + 1] << 8;
const bi16 = off => { const v = bu16(off); return v > 32767 ? v - 65536 : v; };
const CK = { W: 264, B: 265, G1: 266, G2: 267, G3: 268, G4: 269, RED: 270, DRUM: 271, COL: 272, DIM: 276, MID: 280 };   // colour constants the firmware draws with
const col16 = v => { const s = Q.snap; if (v === 0) return null; if (v === (s[CK.W] & 0xFFFF)) return 'W'; if (v === (s[CK.RED] & 0xFFFF)) return 'R'; if (v === (s[CK.DRUM] & 0xFFFF)) return 'D';
  for (let k = 0; k < 4; k++) { if (v === (s[CK.COL + k] & 0xFFFF)) return 'C' + k; if (v === (s[CK.DIM + k] & 0xFFFF)) return 'd' + k; if (v === (s[CK.MID + k] & 0xFFFF)) return 'm' + k; }
  for (const [n, k] of [['g1', CK.G1], ['g2', CK.G2], ['g3', CK.G3], ['g4', CK.G4]]) if (v === (s[k] & 0xFFFF)) return n; return '?'; };
const dimOf = c => ({ [C.t[0]]: '#2a3a63', [C.t[1]]: '#3a5a28', [C.t[2]]: '#7a6420', [C.t[3]]: '#7a4a20' }[c] || C.off);
const midOf = c => ({ [C.t[0]]: '#44689c', [C.t[1]]: '#5a8a3c', [C.t[2]]: '#c0a048', [C.t[3]]: '#b46c2c' }[c] || C.off);
function bgOf(k) { if (!k) return 'transparent'; if (k === 'W') return C.hi; if (k === 'R') return C.rec; if (k === 'D') return C.t[3]; if (k[0] === 'C') return cc(+k[1]); if (k[0] === 'd') return dimOf(cc(+k[1])); if (k[0] === 'm') return midOf(cc(+k[1]));
  return { g1: C.pad, g2: C.off, g3: C.offHi, g4: C.dim, B: C.bg }[k] || C.pad; }
function fgOf(k) { if (!k || k === 'B') return C.hiText; if (k === 'W') return C.text; if (k === 'R') return C.rec; return { g1: C.dim, g2: C.off, g3: C.dim, g4: C.pdim }[k] || C.text; }
function tiles() {
  const out = [];
  for (let i = 0; i < 16; i++) { const o = QB.TILES + i * 16; out.push({ lab: bstr(o, 8), bg: col16(bu16(o + 8)), fg: col16(bu16(o + 10)), top: col16(bu16(o + 12)), marks: Q.qb[o + 14] }); }
  return out;
}
function dials(base, n, colors) {
  const out = [];
  for (let i = 0; i < n; i++) { const o = base + i * 32, r = bi16(o + 22); out.push({ l: bstr(o, 10), t: bstr(o + 10, 12), v: r < 0 ? 0 : r / 1000, color: (colors || [0, 1, 2, 3]).map(cc)[i] }); }
  return out;
}
function tileHTML(t, h, rename) {
  const bd = t.bg === 'R' ? C.rec : C.padC, top = t.top ? bgOf(t.top === 'W' ? 'W' : t.top) : bd;
  const lab = rename ? rename(t.lab) : t.lab;
  return `<div class="pad" style="background:${t.bg ? bgOf(t.bg) : 'transparent'};color:${fgOf(t.fg)};border-color:${t.bg ? bd : 'transparent'};${t.top ? `border-top-color:${top};border-top-width:2px;` : ''}height:${h}px"><span>${esc(lab)}</span></div>`;
}
const rightBar = () => `<div class="fl"></div>${sv(I.playing) ? tri() : ''}<span>${sv(I.bpm)}</span>`;
const LY = { FX: 1, ERASE: 2, ROLL: 3, STEP: 4, SCALE: 5, MIX: 6, SONG: 7 };
const PGI = { ENV: 1, LFO: 2, FX: 3, SCL: 4, EDIT: 5, GLO: 6, SAVE: 7, ARP: 8, SEQ: 9, TRK: 10 };
const colsCap = () => [0, 1, 2, 3].map(c => { const o = QB.COLS + c * 40, r = bi16(o + 32); return { l: bstr(o, 12), v: bstr(o + 12, 12), u: bstr(o + 24, 8), ratio: r }; });
const footCap = () => ({ ename: bstr(QB.FOOT, 12), pn: bstr(QB.FOOT + 12, 20), ti: bstr(QB.FOOT + 32, 20) });
const title1 = s => s.charAt(0).toUpperCase() + s.slice(1).toLowerCase();

// SPELLS / BANISH / BIOME / CAMP replace layer_screen_draw() (ui_layers.c): tiles and dials are the firmware's own.
Object.assign(SCREENS, {
  spells: () => { const tl = tiles();
    return { bar: `${title('Spells')}<span style="color:${C.dim}">${esc(bstr(QB.SUB, 24))}</span>${rightBar()}`, scene: { key: 'punch', h: 84 },
      hud: `<div class="hh"><div class="pads">${tl.map(t => tileHTML(t, 17)).join('')}</div>${knobsHTML(dials(QB.DIALS, 4))}</div>` }; },
  banish: () => { const tl = tiles();
    return { bar: `${title('Banish', C.rec)}<span style="color:${C.dim}">${esc(bstr(QB.SUB, 24))}</span>${rightBar()}`, scene: { key: 'erase', h: 84 },
      hud: `<div class="hh"><div class="pads">${tl.map(t => tileHTML(t, 17)).join('')}</div>${knobsHTML(dials(QB.DIALS, 4))}</div>` }; },
  biome: () => { const tl = tiles(), sc = sv(T(0, 9)), names = { 1: ['maj', 'forest'], 2: ['min', 'cave'], 3: ['dor', 'coast'], 8: ['phr', 'ruins'] };
    return { bar: `${title('Biome')}<span>${esc(bstr(QB.SUB, 24))}</span>${rightBar()}`, scene: { key: 'key', h: 84 },
      hud: `<div class="hh"><div class="pads">${tl.map(t => tileHTML(t, 15)).join('')}</div>
        <div class="legend">${[[1, 'maj', 'forest'], [2, 'min', 'cave'], [3, 'dor', 'coast'], [8, 'phr', 'ruins']].map(b => `<div style="border:1px solid ${b[0] === sc ? cc(1) : 'transparent'};color:${b[0] === sc ? cc(1) : C.dim}"><span>${b[1]}</span> <span>${b[2]}</span></div>`).join('')}</div>${knobsHTML(dials(QB.DIALS, 4))}</div>` }; },
  camp: () => { const all = tiles(), ren = l => l.replace(/^mute /, 'rest ').replace(/^solo /, 'duel ');
    // the firmware lights a mute tile while the track is HEARD; the skin lights "rest n" while the hero RESTS (README)
    const tl = [...all.slice(0, 8), ...all.slice(12, 16)].map((t, i) => i < 4 ? Object.assign({}, t, t.bg === 'g2' ? { bg: 'C' + i, fg: 'B' } : { bg: 'g1', fg: 'g4' }) : t);
    return { bar: `${title('Camp')}<span style="color:${C.dim}">rest</span><span style="color:${C.dim}">duel</span><span style="color:${C.dim}">tap</span>`, scene: { key: 'mix', h: 84 },
      hud: `<div class="hh"><div class="pads">${tl.map((t, i) => tileHTML(t, 17, ren)).join('')}</div>${knobsHTML(dials(QB.DIALS, 4))}</div>` }; },
  // DUNGEON / KIT replace drum_screen_draw() (ui_studio.c), its two pages: GRID and KIT.
  dungeon: () => { const lane = sv(27), cur = sv(26), len = sv(T(3, 5)), bank = Math.floor((cur % len) / 16), pg = lane >= 8 ? 8 : 0, lit = sv(304), playing = sv(I.playing), pos = sv(T(3, 6)) % len;
    let g = '';
    for (let r = 0; r < 8; r++) { const i = pg + r; g += `<div class="grow${i === lane ? ' gs' : ''}"><span class="gl8" style="color:${(lit >> i) & 1 ? C.text : i === lane ? C.t[3] : C.dim}">${esc(bstr(QB.LANES + i * 8, 8))}</span>`;
      for (let c = 0; c < 16; c++) { const v = Q.qb[QB.GRID + i * 16 + c], inl = v & 0x80, on = v & 1, lv = (v >> 1) & 3, p = bank * 16 + c;
        const colr = !inl ? 'transparent' : on ? [C.t[3], '#7a4a20', '#b46c2c', C.text][lv] : (playing && p === pos ? C.offHi : (c % 4 === 0 ? C.offHi : C.off));
        const isCur = p === cur && i === lane; g += `<div class="gc" style="background:${isCur && !on ? 'transparent' : colr};${isCur ? `box-shadow:inset 0 0 0 1px ${C.rec}` : ''}"></div>`; }
      g += '</div>'; }
    const kit = bstr(QB.KIT, 20), style = bstr(QB.KIT + 20, 16), kn = sv(28);
    return { bar: `<div class="kno" style="background:${C.t[3]};color:${C.hiText}">${kn + 1}</div>${title(title1(kit))}<span style="color:${C.dim}">${esc(style.toLowerCase())}</span><div class="fl"></div><span>grid</span><span style="color:${C.dim}">kit</span>`,
      scene: { key: 'steps', h: 84 }, hud: `<div class="hh"><div class="grid">${g}</div>${knobsHTML(dials(QB.DDIALS, 4))}</div>` }; },
  kit: () => { const lane = sv(27), lit = sv(304), kit = bstr(QB.KIT, 20), kn = sv(28);
    let p = ''; for (let i = 0; i < 16; i++) { const on = (lit >> i) & 1; p += `<div class="pad" style="background:${on ? C.t[3] : C.pad};color:${on ? C.hiText : i === lane ? C.text : C.dim};border-color:${C.padC};height:17px"><span>${esc(bstr(QB.LANES + i * 8, 8))}</span></div>`; }
    return { bar: `<div class="kno" style="background:${C.t[3]};color:${C.hiText}">${kn + 1}</div>${title(title1(kit))}<span style="color:${C.dim}">items</span><div class="fl"></div><span style="color:${C.dim}">grid</span><span>kit</span>`,
      scene: { key: 'kit', h: 84 }, hud: `<div class="hh"><div class="pads">${p}</div>${knobsHTML(dials(QB.DDIALS, 4))}</div>` }; },
  // EQUIP replaces the EDIT pages (draw_columns + draw_foot, ui_draw.c): the four columns are the firmware's; the item grid is its preset list (BANK, ui.c).
  equip: () => { const sel = sv(I.sel), f = footCap(), cols = colsCap(), cur = sv(288), total = sv(289), first = sv(306), ICON = { BASS: 'saw', KEYS: 'sine', ORGN: 'square', PAD: 'sine', LEAD: 'saw', PLCK: 'tri', STAB: 'square', FX: 'noise', USER: 'tri' };
    let items = ''; for (let i = 0; i < 12; i++) { const o = QB.BANK + i * 24, nm = bstr(o, 13); if (!nm) { items += `<div class="slot" style="opacity:.25"></div>`; continue; }
      const on = first + i === cur; items += `<div class="slot" style="background:${on ? C.hi : C.pad};color:${on ? C.hiText : C.text}"><canvas data-icon="${ICON[bstr(o + 13, 6)] || 'tri'}" data-col="${on ? C.hiText : cc(i)}" width="12" height="12"></canvas><span>${esc(nm.indexOf(' ') > 0 ? nm.split(' ')[0] : nm.slice(0, 7))}</span></div>`; }
    const kind = bstr(QB.BANK + (cur - first) * 24 + 13, 6);
    const knobs = cols.map((c, i) => !c.l ? { l: '' } : ({ l: c.l.toLowerCase(), color: cc(i), v: c.ratio < 0 ? 0 : c.ratio / 1000, t: c.v + (c.u || '') }));
    const HERO = { mage: 'MAGE', archer: 'ARCHER', cleric: 'CLERIC', warrior: 'WARRIOR' };
    return { bar: `${title('Equip')}<span>${sel + 1} ${esc(Q.txt[sel][0])}</span><div class="fl"></div><span style="color:${C.dim}">No. ${cur + 1}/${total}</span>`, scene: null,
      hud: `<div class="eqwrap"><div class="eqtop">${panel(`<div class="eqp">${[0, 1, 2, 3].map(i => portrait(CLASSES[i], i === sel ? C.hi : C.padC)).join('')}</div><div class="eqslot"></div><div class="eqlv"><span>${HERO[CLASSES[sel]]}</span><span style="color:${C.pdim}">lv ${Math.round(sv(T(sel, 2)) * 100 / 127)}</span></div>`, 'width:86px;padding:4px 3px;display:flex;flex-direction:column;gap:3px;align-items:center;flex:none')}
        <div class="eqr"><div class="slots">${items}</div>${panel(`${title(esc(title1(f.pn)))}<span style="color:${C.pdim}">${esc(f.ename.toLowerCase())}${kind ? ' / ' + kind.toLowerCase() : ''}</span><div style="display:flex;gap:8px"><span>presets: browse</span><span style="color:${C.pdim}">${esc(f.ti.toLowerCase())}</span></div>`, 'padding:4px 5px;display:flex;flex-direction:column;gap:2px;line-height:1')}</div></div>${knobsHTML(knobs)}</div>` }; },
  // WEATHER replaces the FX page (P_DIST P_CHOR P_DLY P_REV): README = DST storm +atk, CHO mist +def, DLY echo +spd, REV rain regen.
  weather: () => { const f = footCap(), cols = colsCap(), sel = sv(I.sel), W = [['storm', '+atk'], ['mist', '+def'], ['echo', '+spd'], ['rain', 'regen']];
    return { bar: `${title('Weather')}<span style="color:${C.dim}">buffs</span>${rightBar()}`, scene: { key: 'fx', h: 84 },
      hud: `<div class="fxg">${cols.map((c, i) => { const n = c.ratio < 0 ? 0 : Math.round(c.ratio / 100);
        return `<div class="fxc"><span style="color:${cc(i)}">${esc(c.l)}</span><span style="color:${C.dim}">${W[i][0]}</span><canvas data-knob="${c.ratio < 0 ? 0 : c.ratio / 1000}" data-col="${cc(i)}" data-dir="1c" width="20" height="20"></canvas><span>${esc(c.v)}${esc(c.u)}</span><span style="color:${cc(i)}">${W[i][1]}</span>
        <div class="seg">${Array.from({ length: 10 }, (_, j) => `<div style="background:${j >= 10 - n ? cc(i) : C.off}"></div>`).join('')}</div></div>`; }).join('')}</div>
        <div class="fxf"><span>${esc(Q.txt[sel][0])}</span><span style="color:${cc(sel)}">${CLASSES[sel]}</span><div class="fl"></div><span style="color:${C.dim}">${esc(f.ti.toLowerCase())}</span></div>` }; },
  // INN replaces draw_menu() (ui_menu.c): the ten rows and their values are the firmware's.
  inn: () => { const sel = sv(5);
    const rows = Array.from({ length: 10 }, (_, i) => { const nm = bstr(QB.MENU + i * 36, 24), v = bstr(QB.MENU + i * 36 + 24, 12), cur = i === sel;
      return `<div class="mrow"><i class="tri sm" style="border-left-color:${cur ? C.text : 'transparent'}"></i><span>${esc(nm)}</span><span style="color:${cur ? C.text : C.pdim}">${esc(v)}</span></div>`; }).join('');
    return { bar: `${title('Menu')}<span style="color:${C.dim}">the party rests</span>`, scene: { key: 'menu', h: 60 },
      hud: `<div class="hh">${panel(rows, 'padding:3px 5px;display:flex;flex-direction:column')}<div class="hint" style="color:${C.dim}"><div><span>presets: move</span><span>knob 1: set</span></div><div><span>oct+: ok</span><span>oct-: back</span></div></div></div>` }; },
  // BOSS: REC armed on a playing loop (song.rec) or a free take (ft_on). HP = what is left of the loop / of the take's fitted length.
  boss: beat => { const rt = sv(I.scr) === SC.TAKE ? sv(299) : [0, 1, 2, 3].find(i => (sv(I.rec) >> i) & 1) ?? sv(I.sel);
    let hp = 1;
    if (sv(I.scr) === SC.TAKE) { const bars = sv(297), bpm = sv(298), secs = sv(296) * sv(300) / sv(301); hp = bars ? clamp(1 - secs / (bars * 4 * 60 / bpm), 0, 1) : 1; }
    else { const len = sv(T(rt, 5)); hp = 1 - (sv(T(rt, 6)) % len) / len; }
    const cmds = [['FIGHT', 'play'], ['SPELL', 'punch'], ['ITEM', 'kit'], ['RUN', 'stop']].map((m, i) => `<div class="cmd" style="background:${i === 0 ? C.hi : 'transparent'};color:${i === 0 ? C.hiText : C.text}"><span>${m[0]}</span><div class="fl"></div><span style="color:${i === 0 ? C.hiText : C.pdim}">${m[1]}</span></div>`).join('');
    const p = Math.floor(beat), bt = ((p % 4) + 4) % 4;
    return { bar: `${bpmBox()}<div class="posb">${tri()}<span>${Math.floor(p / 4) + 1}.${bt + 1}</span><div class="pips">${[0, 1, 2, 3].map(i => `<u style="background:${i <= bt ? C.rec : C.off}"></u>`).join('')}</div></div><div class="fl"></div><span style="color:${C.rec}">rec</span>`,
      scene: { key: 'boss', h: 84 }, ov: `<div class="bossn">${title('King Slime', C.text)}<div class="bossb"><div style="width:${Math.round(hp * 100)}%;background:${C.rec}"></div></div></div>`,
      hud: `<div class="bhud"><div class="bcol">${panel(cmds, 'width:100px;padding:3px 4px;display:flex;flex-direction:column;gap:1px;flex:none')}<div class="bhp">${[0, 1, 2, 3].map(i => `<div class="bhr"><span style="color:${i === rt ? C.text : C.dim}">${esc(Q.txt[i][0])}</span><div class="hpb2"><div style="width:${sv(T(i, 2)) * 100 / 127}%;background:${cc(i)}"></div></div></div>`).join('')}</div></div>${knobsHTML(partyKnobs())}</div>` }; },
  // DUEL: a soloed track (song.solo). The steps are that track's; div / swing / steps are its pattern parameters (the STEP layer's dials).
  duel: beat => { const i = [0, 1, 2, 3].find(k => (sv(I.solo) >> k) & 1), len = sv(T(i, 5)), pos = sv(T(i, 6)) % len, playing = sv(I.playing), pg = playing ? Math.floor(pos / 16) : 0, pages = Math.ceil(len / 16);
    const cells = Array.from({ length: 16 }, (_, k) => { const p = pg * 16 + k; return `<div class="dcell" style="background:${p >= len ? 'transparent' : playing && p === pos ? C.text : stepOn(i, p) ? C.t[3] : (k % 4 === 0 ? C.offHi : C.off)}"></div>`; }).join('');
    const sound = i === 3 ? bstr(QB.LANES + sv(310) * 8, 8) : bstr(QB.MISC + 12, 8), div = bstr(QB.MISC + 40 + (sv(T(i, 16)) % 6) * 8, 8), sw = bstr(QB.MISC + 96 + i * 8, 8);
    const knobs = [{ l: i === 3 ? 'sound' : 'note', color: cc(0), v: i === 3 ? sv(310) / 15 : sv(311) / 127, t: sound }, { l: 'div', color: cc(1), v: (sv(T(i, 16)) % 6) / 5, t: div }, { l: 'swing', color: cc(2), v: sv(T(i, 17)) / 100, t: sw }, { l: 'steps', color: cc(3), v: (len - 1) / 63, t: len }];
    return { bar: `${title('Duel')}<span style="color:${C.dim}">solo ${i + 1}</span>${rightBar()}`, scene: { key: 'solo', h: 110 }, ov: `<div class="combo"><span>combo</span><b style="color:${C.hi}">x${Q.combo | 0}</b></div>`,
      hud: `<div class="dhud"><div class="dhead"><span style="color:${C.dim}">${i + 1}</span><span>${esc(Q.txt[i][0])}</span><span style="color:${cc(i)}">${CLASSES[i]}</span><div class="fl"></div><span style="color:${C.dim}">page ${pg + 1}/${pages}</span></div><div class="dsteps">${cells}</div>${knobsHTML(knobs)}</div>` }; },
});

function pickScreen() {
  if (!Q.began) return 'title';
  const s = Q.snap, scr = s[I.scr], sel = s[I.sel];
  switch (scr) {
    case SC.TRACKS: if (s[I.scr] === SC.TRACKS && s[I.rec] && s[I.playing]) return 'boss'; if (s[I.solo]) return 'duel'; return 'party';
    case SC.REC_READY: return 'recready';
    case SC.COUNT_IN: return 'count';
    case SC.TAKE: return 'boss';
    case SC.DRUM: return s[25] ? 'kit' : 'dungeon';
    case SC.MENU: return s[I.menu] === 1 ? 'inn' : null;       // ABOUT: the firmware's own screen
    case SC.LAYER: return { [LY.FX]: 'spells', [LY.ERASE]: 'banish', [LY.SCALE]: 'biome', [LY.MIX]: 'camp' }[s[I.layer]] || null;   // roll / steps / song: not skinned
    case SC.PAGE: {
      const fam = s[8], pg = s[7];
      if (sel === 3 || s[6]) return null;                       // drum track pages say "DRUM TRACK"; HOME is not skinned
      if (fam === PGI.EDIT && pg !== undefined && (pg === 10 || pg === 11)) return 'equip';   // EDIT 1 / EDIT 2: the engine's own parameters
      if (fam === PGI.FX && pg === 4) return 'weather';         // the FX page: DST CHO DLY REV
      return null;
    }
    default: return null;
  }
}

// ------------------------------------------------------------------ mount / loop
const el = {};
const eqc = document.createElement('canvas'); eqc.width = 72; eqc.height = 96; eqc.id = 'eqc';
Q.mount = (canvas) => {
  Q.cv = canvas;
  const host = canvas.parentNode;
  const lcd = document.createElement('div'); lcd.id = 'qlcd';
  lcd.innerHTML = '<div id="bar"></div><div id="scenew"><canvas id="sc" width="240" height="84"></canvas><div id="ov"></div></div><div id="hud"></div><div id="scan"></div>';
  const wrap = document.createElement('div'); wrap.id = 'qwrap'; wrap.appendChild(lcd); host.appendChild(wrap); Q.lcd = lcd;
  el.bar = lcd.querySelector('#bar'); el.sw = lcd.querySelector('#scenew'); el.sc = lcd.querySelector('#sc'); el.ov = lcd.querySelector('#ov'); el.hud = lcd.querySelector('#hud');
  new ResizeObserver(fit).observe(canvas); fit();
  requestAnimationFrame(frame);
};
function fit() { const w = Q.cv.getBoundingClientRect().width / (G.devScale || 1) || Q.cv.clientWidth; Q.lcd.style.zoom = (Q.cv.clientWidth || 226) / 240; }
function apply() { Q.lcd.classList.toggle('on', Q.active && !!Q.shown); Q.cv.style.visibility = (Q.active && Q.shown) ? 'hidden' : 'visible'; }
let lastFrame = 0, lastNow = performance.now(), lastCheck = 0;
/* Animation policy + SPI protection (what the device would do).
 * The LCD is on SPI1 at 12 MHz (lcd.c LCD_BAUD): every scene frame costs w*h*2 bytes of that bus, and the HUD shares it.
 * So the scene is paced by a token bucket of SPI bytes instead of by a bare timer:
 *   - transport playing (or a free take): PLAY_FPS, allowed up to BUDGET_PLAY of the bus;
 *   - nothing playing: a slow ambient tick (IDLE_FPS, set 0 for none), allowed up to BUDGET_IDLE;
 *   - a change of state (HUD rebuild, new screen) always draws and is charged to the bucket, so a burst of knob turns
 *     starves the ambient frames, not the controls;
 *   - a frame that is due but not affordable is dropped (counted), never queued: the load can not pile up. */
Q.cfg = { spiHz: 12e6, playFps: 16, idleFps: 4, budgetPlay: 0.55, budgetIdle: 0.12 };
Q.stats = { frames: 0, bytes: 0, dropped: 0, t0: performance.now(), fps: 0, spi: 0, mode: 'idle' };
let tokens = 0, lastRefill = performance.now();
function perfTick(bytes) {
  const s = Q.stats; s.frames++; s.bytes += bytes;
  const now = performance.now();
  if (now - s.t0 >= 1000) { const dt = (now - s.t0) / 1000; s.fps = s.frames / dt; s.spi = s.bytes * 8 / dt / Q.cfg.spiHz * 100; s.frames = 0; s.bytes = 0; s.t0 = now;
    const p = document.getElementById('perf'); if (p) p.textContent = `${s.mode}: scene ${s.fps.toFixed(1)} fps · LCD SPI @${Q.cfg.spiHz / 1e6} MHz ≈ ${s.spi.toFixed(0)} % · dropped ${s.dropped}`; }
}
function frame(now) {
  requestAnimationFrame(frame);
  if (now - lastCheck < 30) return; lastCheck = now;
  const dt = Math.min(.2, (now - lastNow) / 1000);
  if (!Q.active || !Q.ok) { lastFrame = now; if (Q.shown) { Q.shown = false; apply(); } return; }
  const name = pickScreen(), show = !!name;
  if (show !== Q.shown) { Q.shown = show; apply(); }
  if (!show) { lastFrame = now; return; }
  const playing = !!sv(I.playing), animating = playing || sv(I.scr) === SC.TAKE;
  const beat = beatNow(), t = (now - Q.t0) / 1000;
  if (name === 'duel' && playing) { const i = [0, 1, 2, 3].find(k => (sv(I.solo) >> k) & 1), n = Math.floor(sv(T(i, 6))); if (n !== Q.lastStep) { Q.lastStep = n; if (stepOn(i, n % sv(T(i, 5)))) Q.combo = Math.min(99, (Q.combo | 0) + 1); } }
  // the HUD: rebuilt when the state it shows changes (a beat tick only while the transport runs)
  const qsig = Q.qb ? Q.qb.reduce((h, v, i) => (h * 31 + v + i) | 0, 7) : 0;
  const sig = [name, qsig, sv(I.scr), sv(5), sv(26), sv(27), sv(288), sv(302), sv(304), playing ? Math.floor(beat * 2) : 0, playing, sv(I.sel), sv(I.rec), sv(I.solo), sv(I.ciBeat), sv(I.recCount), sv(I.bpm), sv(I.swing), titleURL ? 1 : 0,
    [0, 1, 2, 3].map(i => sv(T(i, 2)) + ':' + sv(T(i, 7)) + ':' + sv(T(i, 4)) + ':' + sv(T(i, 5)) + ':' + sv(I.steps + 2 * i) + ':' + sv(I.steps + 2 * i + 1)).join(',')].join('|');
  let changed = false;
  if (sig !== Q.sig || name !== Q.name) {
    changed = true; Q.sig = sig; Q.name = name;
    const def = SCREENS[name](beat);
    el.bar.innerHTML = def.bar || ''; el.bar.style.display = def.full ? 'none' : '';
    if (def.scene) { el.sw.style.display = ''; el.sc.width = 240; el.sc.height = def.scene.h; el.sw.style.height = def.scene.h + 1 + 'px'; } else el.sw.style.display = 'none';
    el.ov.innerHTML = def.ov || ''; el.hud.innerHTML = def.hud;
    el.hud.querySelectorAll('canvas[data-knob]').forEach(Scene.drawKnob);
    el.hud.querySelectorAll('canvas[data-portrait]').forEach(Scene.drawPortrait);
    el.hud.querySelectorAll('canvas[data-icon]').forEach(Scene.drawIcon);
    const slot = el.hud.querySelector('.eqslot'); if (slot) slot.appendChild(eqc);
    Q.def = def;
  }
  // the scene: paced by the SPI budget (see the policy above)
  const sceneH = Q.def && Q.def.scene ? Q.def.scene.h : 0, cost = 240 * sceneH * 2, hudCost = 240 * (240 - 19 - sceneH - 1) * 2, C = Q.cfg;
  const mode = animating ? 'play' : (C.idleFps > 0 && name !== 'title' ? 'idle' : 'static'); Q.stats.mode = mode;
  const fps = animating ? C.playFps : C.idleFps, share = animating ? C.budgetPlay : C.budgetIdle, nowMs = performance.now();
  tokens = Math.min(C.spiHz / 8 * 0.25, tokens + (nowMs - lastRefill) / 1000 * (C.spiHz / 8) * share); lastRefill = nowMs;
  const due = mode !== 'static' && fps > 0 && now - lastFrame >= 1000 / fps;
  if (changed) tokens = Math.max(-C.spiHz / 8 * 0.25, tokens - hudCost - cost);     // always drawn; charged so ambient frames back off
  else if (due) { if (tokens >= cost) tokens -= cost; else { Q.stats.dropped++; lastFrame = now; return; } }
  else return;
  lastFrame = now; lastNow = performance.now();
  const ST = sceneState(t, beat, playing, dt);
  if (name === 'equip') Scene.drawScene(eqc, '1c', 'equip', t, ST);
  if (Q.def && Q.def.scene) { Scene.drawScene(el.sc, '1c', Q.def.scene.key, t, ST); perfTick(cost); }
}
function duelState() {
  const i = [0, 1, 2, 3].find(k => (sv(I.solo) >> k) & 1); if (i === undefined) return { duelH: '', duelR: '' };
  const len = sv(T(i, 5)); let r = ''; for (let k = 0; k < len; k++) r += (k % 16 === 12) ? '1' : '0';   // README: the rival counters on step 12
  return { duelH: patString(i), duelR: r };
}
const SCALE_BIOME = { 1: 'forest', 2: 'cave', 3: 'coast', 8: 'ruins' };   // README: major forest, minor cave, dorian coast, phrygian ruins (firmware N_SCALE: MAJ=1 MIN=2 DOR=3 PHRY=8)
function sceneState(t, beat, playing, dt) {
  if (playing) Q.sp += dt * sv(I.bpm) * 0.22;                                // README: scroll = bpm * 0.22 px/s
  const pats = {}, mute = {}, hp = {};
  CLASSES.forEach((cl, i) => { pats[cl] = patString(i); mute[cl] = !!sv(T(i, 7)); hp[cl] = sv(T(i, 2)) / 127; });
  const mx = k => Math.max(...[0, 1, 2, 3].map(i => sv(T(i, k)))) / 127;
  return { t, beat, bpm: sv(I.bpm), sp: Q.sp, playing, pats, mute, hp, fx: { dst: mx(10), cho: mx(11), dly: mx(12), rev: mx(13) },
    biome: SCALE_BIOME[sv(T(0, 9))] || 'forest', eqCls: CLASSES[sv(I.sel)], loopOn: sv(302) >= 0 && sv(302) <= 3, ...duelState() };
}
G.Quest = Q;
})(window);

