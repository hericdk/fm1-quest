// FM1 Quest — 3D faceplate (three.js). The DOM LCD of the simulator is kept as the real screen and pinned onto the model's glass with a CSS homography.
// Layout measured from the FM-1 top-down photo (1 world unit = 100 px of that photo, origin = centre of the body, +Z = up from the plate).
// Falls back to the 2D faceplate when three.js or WebGL is not available.
(function (G) {
'use strict';
const SIM = G.SIM, THREE = G.THREE; if (!SIM || !THREE) return;
const A = SIM.api, sim = SIM.sim, host = A.host;
let renderer; try { renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true, powerPreference: 'high-performance' }); } catch (e) { return; }
if (!renderer.getContext()) return;

// ---------------------------------------------------------------- measured layout (photo px)
const KPOS = { master: [147, 327], select: [237, 327], presets: [147, 415], algo: [237, 415], k1: [553, 327], k2: [647, 327], k3: [742, 327], k4: [838, 327] };
const KCOL = { k1: '#5a8ad0', k2: '#7ab04e', k3: '#f0c860', k4: '#e08a3a', master: '#e0a838' };
const KLAB = { master: 'MASTER', select: 'SELECT', presets: 'PRESETS', algo: 'ALGORITHM', k1: 'KNOB1', k2: 'KNOB2', k3: 'KNOB3', k4: 'KNOB4' };
const BPOS = { fx: [565, 420], sel: [618, 420], env: [670, 420], lfo: [723, 420], edit: [775, 420], glo: [829, 420], home: [565, 472], save: [618, 472], arp: [670, 472], seq: [723, 472], play: [775, 472], rec: [829, 472], octm: [160, 483], octp: [226, 483] };
const BTXT = { fx: 'FX', sel: 'SEL', env: 'ENV', lfo: 'LFO', edit: 'EDIT', glo: 'GLO', home: 'HOME', save: 'SAVE', arp: 'ARP', seq: 'SEQ', play: 'PLAY\nSTOP', rec: 'REC', octm: 'OCT −', octp: 'OCT +' };
const UPX = [167, 213, 262, 357, 405, 500, 548, 595, 690, 737, 835], UPY = 587, LOWY = 668, lowX = i => 145 + 47.53 * i;
const LCDC = [395, 392];                                        // LCD centre (photo px); the model's LCD is 15 % larger than the real one so the 240×240 art stays legible
const W = 8.4, H = 5.0, TOP = .3, BT = .05;
const P = (x, y) => [(x - 500) / 100, (500 - y) / 100];
const C = hex => new THREE.Color(hex).convertSRGBToLinear();   // r128 does not colour-manage hex strings: convert so dark stays dark
const colCache = {}, colOf = hex => colCache[hex] || (colCache[hex] = C(hex));
const clamp = (v, a, b) => Math.max(a, Math.min(b, v)), lerp = (a, b, t) => a + (b - a) * t;

// ---------------------------------------------------------------- stage (canvas + the DOM LCD pinned on the glass)
const stage = document.createElement('div'); stage.className = 'stage3d';
const cv = renderer.domElement; stage.append(cv);
const scr = document.createElement('div'); scr.className = 'scr3d'; stage.append(scr);
const labels = document.createElement('div'); labels.className = 'lab3d'; stage.append(labels);
host.insertBefore(stage, A.plate);
const pgchip = host.querySelector('.pgchip'), rowchip = host.querySelector('.rowchip');
function set3d(on) {
  host.classList.toggle('is3d', on); stage.style.display = on ? '' : 'none';
  if (on) { scr.append(A.lcdbox, pgchip, rowchip); resize(); } else { A.slot.append(A.lcdbox); A.plate.querySelector('.bezel').append(pgchip, rowchip); A.fit(); }
  try { localStorage.setItem('fm1lab.sim3d2', on ? '1' : '0'); } catch (e) {}
  const b = document.getElementById('s-3d'); if (b) b.textContent = on ? 'painel 2D' : 'painel 3D';
}
A.set3d = set3d;

// ---------------------------------------------------------------- scene, light, environment
renderer.setPixelRatio(Math.min(devicePixelRatio || 1, 2)); renderer.outputEncoding = THREE.sRGBEncoding; renderer.toneMapping = THREE.ACESFilmicToneMapping; renderer.toneMappingExposure = 1.3; renderer.setClearColor(0x000000, 0);
const scene = new THREE.Scene(), camera = new THREE.PerspectiveCamera(24, 2, .5, 120);
scene.add(new THREE.HemisphereLight(0xc8d0ff, 0x1c1620, .5));
const key = new THREE.DirectionalLight(0xfff2e0, 1.15); key.position.set(-5, 7, 9); scene.add(key);
const rim = new THREE.DirectionalLight(0x7f94ff, .5); rim.position.set(7, -3, 4); scene.add(rim);
(function env() {
  const c = document.createElement('canvas'); c.width = 512; c.height = 256; const x = c.getContext('2d');
  const g = x.createLinearGradient(0, 0, 0, 256); g.addColorStop(0, '#6b6f96'); g.addColorStop(.42, '#1b1a25'); g.addColorStop(1, '#050408'); x.fillStyle = g; x.fillRect(0, 0, 512, 256);
  x.fillStyle = '#ffffff'; x.globalAlpha = .85; x.fillRect(60, 46, 190, 22); x.fillRect(300, 74, 150, 14); x.globalAlpha = .55; x.fillRect(120, 100, 260, 6); x.globalAlpha = .35; x.fillRect(200, 18, 110, 10); x.fillStyle = '#7f94ff'; x.globalAlpha = .4; x.fillRect(430, 40, 60, 90);
  const t = new THREE.CanvasTexture(c); t.mapping = THREE.EquirectangularReflectionMapping; t.encoding = THREE.sRGBEncoding;
  const pm = new THREE.PMREMGenerator(renderer); scene.environment = pm.fromEquirectangular(t).texture; pm.dispose();
})();

// ---------------------------------------------------------------- helpers
function rr(w, h, r) { const s = new THREE.Shape(), x = -w / 2, y = -h / 2; r = Math.min(r, w / 2, h / 2); s.moveTo(x + r, y); s.lineTo(x + w - r, y); s.absarc(x + w - r, y + r, r, -Math.PI / 2, 0, false); s.lineTo(x + w, y + h - r); s.absarc(x + w - r, y + h - r, r, 0, Math.PI / 2, false); s.lineTo(x + r, y + h); s.absarc(x + r, y + h - r, r, Math.PI / 2, Math.PI, false); s.lineTo(x, y + r); s.absarc(x + r, y + r, r, Math.PI, Math.PI * 1.5, false); return s; }
const ext = (shape, depth, b) => new THREE.ExtrudeGeometry(shape, { depth, bevelEnabled: true, bevelThickness: b, bevelSize: b, bevelOffset: -b, bevelSegments: 3, curveSegments: 20 });
const mkTex = c => { const t = new THREE.CanvasTexture(c); t.encoding = THREE.sRGBEncoding; t.anisotropy = renderer.capabilities.getMaxAnisotropy(); return t; };
const cvs = (w, h) => { const c = document.createElement('canvas'); c.width = w; c.height = h; return c; };
const dev = new THREE.Group(); dev.rotation.order = 'YXZ'; const root = new THREE.Group(); root.add(dev); scene.add(root);
const pick = [], needFont = [];

// light that leaks out of the gap around a key: the side walls glow at the base and fade out towards the (dark) top
function glowMat(color, rough, H) {
  const m = new THREE.MeshPhysicalMaterial({ color: C(color), roughness: rough * .8, metalness: 0, clearcoat: .45, clearcoatRoughness: .42, envMapIntensity: .75 });
  const u = { uLevel: { value: 0 }, uColor: { value: C('#ffc46a') }, uH: { value: H } }; m.userData.u = u;
  m.onBeforeCompile = sh => {
    Object.assign(sh.uniforms, u);
    sh.vertexShader = sh.vertexShader.replace('#include <common>', '#include <common>\nvarying float vZ;').replace('#include <begin_vertex>', '#include <begin_vertex>\nvZ = position.z;');
    sh.fragmentShader = sh.fragmentShader.replace('#include <common>', '#include <common>\nvarying float vZ; uniform float uLevel; uniform vec3 uColor; uniform float uH;')
      .replace('#include <emissivemap_fragment>', '#include <emissivemap_fragment>\n float gk = clamp(vZ / uH, 0., 1.); totalEmissiveRadiance += uColor * uLevel * pow(1. - gk, 2.1) * 2.2;');
  };
  return m;
}
const haloTex = (() => { const c = cvs(128, 256), x = c.getContext('2d'); x.filter = 'blur(16px)'; x.fillStyle = '#fff'; const w = 56, h = 168, r = 28, X = 36, Y = 44; x.beginPath(); x.moveTo(X + r, Y); x.arcTo(X + w, Y, X + w, Y + h, r); x.arcTo(X + w, Y + h, X, Y + h, r); x.arcTo(X, Y + h, X, Y, r); x.arcTo(X, Y, X + w, Y, r); x.fill(); const t = new THREE.CanvasTexture(c); return t; })();

// ---------------------------------------------------------------- body
const bodyMat = new THREE.MeshPhysicalMaterial({ color: C('#060608'), roughness: .2, metalness: .05, clearcoat: 1, clearcoatRoughness: .08, envMapIntensity: 1.25 });   // smoked black, glossy
const body = new THREE.Mesh(ext(rr(W, H, .42), .5, BT), bodyMat); body.position.z = -.25; dev.add(body);

// printed top plate (one texture: panels, labels, markers)
const TW = 2490, TH = 1470, K = 3, plateCv = cvs(TW, TH), plateTex = mkTex(plateCv);
function drawPlate() {
  const x = plateCv.getContext('2d'), T = (px, py) => [(px - 500) * K + TW / 2, (py - 500) * K + TH / 2];
  const g = x.createLinearGradient(0, 0, TW, TH); g.addColorStop(0, '#222226'); g.addColorStop(.5, '#19191c'); g.addColorStop(1, '#131316'); x.fillStyle = g; x.fillRect(0, 0, TW, TH);
  for (let i = 0; i < 26000; i++) { x.fillStyle = `rgba(255,255,255,${Math.random() * .05})`; x.fillRect(Math.random() * TW, Math.random() * TH, 2, 2); }
  const panel = (x0, y0, x1, y1, r) => { const [a, b] = T(x0, y0), [c, d] = T(x1, y1); x.save(); x.beginPath(); x.roundRect ? x.roundRect(a, b, c - a, d - b, r * K) : x.rect(a, b, c - a, d - b); x.fillStyle = '#060607'; x.fill(); x.lineWidth = 5; x.strokeStyle = 'rgba(255,255,255,.09)'; x.stroke(); x.clip(); const sg = x.createLinearGradient(0, b, 0, b + 60); sg.addColorStop(0, 'rgba(0,0,0,.8)'); sg.addColorStop(1, 'rgba(0,0,0,0)'); x.fillStyle = sg; x.fillRect(a, b, c - a, 60); x.restore(); };
  panel(528, 388, 865, 505, 20); panel(125, 463, 260, 502, 14); panel(108, 535, 892, 718, 30);
  x.fillStyle = '#b9b9c4'; x.textAlign = 'center'; x.textBaseline = 'middle'; x.font = `500 ${15 * K}px "Pixelify Sans", monospace`;
  Object.entries(KPOS).forEach(([id, [px, py]]) => { const [a, b] = T(px + (id === 'presets' ? 3 : id === 'algo' ? 3 : 0), id === 'master' || id === 'select' || id.startsWith('k') ? 289 : 377); x.fillText(KLAB[id], a, b); });
  x.fillStyle = '#6a6a72'; [[695, 442], [310, 627], [452, 627], [640, 627], [785, 627]].forEach(([px, py]) => { const [a, b] = T(px, py); x.beginPath(); x.arc(a, b, 2.2 * K, 0, 7); x.fill(); });
  plateTex.needsUpdate = true;
}
drawPlate(); needFont.push(drawPlate);
{ const sh = rr(W - 2 * BT, H - 2 * BT, .37), geo = new THREE.ShapeGeometry(sh, 24), uv = geo.attributes.uv, pos = geo.attributes.position;
  for (let i = 0; i < uv.count; i++) uv.setXY(i, pos.getX(i) / (W - 2 * BT) + .5, pos.getY(i) / (H - 2 * BT) + .5);
  const plate = new THREE.Mesh(geo, new THREE.MeshPhysicalMaterial({ map: plateTex, roughness: .34, metalness: .1, clearcoat: .7, clearcoatRoughness: .22, envMapIntensity: .9 })); plate.position.z = TOP + .002; dev.add(plate); }

{ const pts = rr(W - 2 * BT, H - 2 * BT, .37).getPoints(64).map(p => new THREE.Vector3(p.x, p.y, TOP + .006)); dev.add(new THREE.LineLoop(new THREE.BufferGeometry().setFromPoints(pts), new THREE.LineBasicMaterial({ color: C('#7a7a8a'), transparent: true, opacity: .5 }))); }
// front edge: logo, headphone, jack, USB-C, switch, LED (from the side photo)
{ const c = cvs(2532, 180), x = c.getContext('2d'); x.fillStyle = '#0c0c0f'; x.fillRect(0, 0, 2532, 180); const X = px => (px - 78) * 3;
  x.fillStyle = '#f2f2f2'; x.textBaseline = 'middle'; x.font = '700 92px "Avenir Next", "Trebuchet MS", system-ui, sans-serif'; x.fillText('M-VAVE', X(160), 92);
  x.fillStyle = '#1b1b20'; x.beginPath(); x.roundRect ? x.roundRect(X(497), 40, 293 * 3, 100, 18) : x.rect(X(497), 40, 879, 100); x.fill(); x.strokeStyle = '#34343a'; x.lineWidth = 3; x.stroke();
  x.fillStyle = '#050506'; x.beginPath(); x.arc(X(532), 92, 30, 0, 7); x.fill(); x.strokeStyle = '#3b3b42'; x.lineWidth = 6; x.stroke(); x.beginPath(); x.arc(X(596), 92, 14, 0, 7); x.fill();
  x.strokeStyle = '#8a8a92'; x.lineWidth = 5; x.beginPath(); x.roundRect ? x.roundRect(X(646), 78, 60, 28, 14) : x.rect(X(646), 78, 60, 28); x.stroke(); x.fillStyle = '#111'; x.fill();
  x.fillStyle = '#26262b'; x.beginPath(); x.roundRect ? x.roundRect(X(718), 70, 56, 44, 22) : x.rect(X(718), 70, 56, 44); x.fill(); x.fillStyle = '#aaa'; x.beginPath(); x.arc(X(736), 92, 11, 0, 7); x.fill();
  x.fillStyle = '#1d1d22'; x.beginPath(); x.arc(X(772), 92, 5, 0, 7); x.fill();
  const t = mkTex(c), m = new THREE.Mesh(new THREE.PlaneGeometry(8.44, .6), new THREE.MeshStandardMaterial({ map: t, roughness: .4, metalness: .1 })); m.rotation.x = Math.PI / 2; m.position.set(0, -H / 2 - .001, 0); dev.add(m);
  needFont.push(() => { x.fillStyle = '#0c0c0f'; x.fillRect(X(150), 30, 380, 120); x.fillStyle = '#f2f2f2'; x.font = '700 92px "Avenir Next", "Trebuchet MS", system-ui, sans-serif'; x.fillText('M-VAVE', X(160), 92); t.needsUpdate = true; }); }

// ---------------------------------------------------------------- LCD bezel + glass
const [lx, ly] = P(LCDC[0], LCDC[1]), GL = 1.61;
{ const bez = new THREE.Mesh(ext(rr(2.3, 2.3, .22), .05, .03), new THREE.MeshPhysicalMaterial({ color: C('#030304'), roughness: .12, metalness: .3, clearcoat: 1, clearcoatRoughness: .05, envMapIntensity: 1.3 })); bez.position.set(lx, ly, TOP - .01); dev.add(bez);
  const glass = new THREE.Mesh(new THREE.PlaneGeometry(GL + .02, GL + .02), new THREE.MeshPhysicalMaterial({ color: C('#040405'), roughness: .06, metalness: 0, clearcoat: 1, clearcoatRoughness: .04 })); glass.position.set(lx, ly, TOP + .066); dev.add(glass); }
{ const pts = rr(GL + .08, GL + .08, .1).getPoints(40).map(p => new THREE.Vector3(p.x + lx, p.y + ly, TOP + .069)); dev.add(new THREE.LineLoop(new THREE.BufferGeometry().setFromPoints(pts), new THREE.LineBasicMaterial({ color: C('#4a4a58'), transparent: true, opacity: .8 }))); }
const screenCorners = [[lx - GL / 2, ly + GL / 2], [lx + GL / 2, ly + GL / 2], [lx + GL / 2, ly - GL / 2], [lx - GL / 2, ly - GL / 2]].map(([x, y]) => new THREE.Vector3(x, y, TOP + .068));

// ---------------------------------------------------------------- knobs
const grip = (() => { const c = cvs(64, 8), x = c.getContext('2d'); x.fillStyle = '#9a9aa2'; x.fillRect(0, 0, 64, 8); x.fillStyle = '#2a2a2e'; x.fillRect(0, 0, 32, 8); const t = new THREE.CanvasTexture(c); t.wrapS = THREE.RepeatWrapping; t.repeat.set(24, 1); return t; })();
const knobs = {};
Object.entries(KPOS).forEach(([id, [px, py]]) => {
  const g = new THREE.Group(); const [x, y] = P(px, py); g.position.set(x, y, TOP);
  const skirt = new THREE.Mesh(new THREE.CylinderGeometry(.215, .23, .05, 40), new THREE.MeshStandardMaterial({ color: C('#060607'), roughness: .6 })); skirt.rotation.x = Math.PI / 2; skirt.position.z = .025; g.add(skirt);
  const body = new THREE.Mesh(new THREE.CylinderGeometry(.185, .195, .3, 48), new THREE.MeshPhysicalMaterial({ color: C('#0a0a0c'), roughness: .38, metalness: .25, bumpMap: grip, bumpScale: 2.2, clearcoat: .3, clearcoatRoughness: .4, envMapIntensity: .9 })); body.rotation.x = Math.PI / 2; body.position.z = .17; g.add(body);
  const top = new THREE.Mesh(new THREE.CircleGeometry(.175, 48), new THREE.MeshPhysicalMaterial({ color: C('#101013'), roughness: .25, metalness: .3, clearcoat: .8, clearcoatRoughness: .15, envMapIntensity: 1.1 })); top.position.z = .321; g.add(top);
  const pg = new THREE.Group(); pg.position.z = .327; const ind = new THREE.Mesh(new THREE.BoxGeometry(.034, .15, .012), new THREE.MeshBasicMaterial({ color: C(KCOL[id] || '#e8e8ee') })); ind.position.y = .085; pg.add(ind); g.add(pg);
  dev.add(g); body.userData.pick = { kind: 'knob', id }; pick.push(body, top);  top.userData.pick = body.userData.pick;
  knobs[id] = { g, pg, x, y };
});

// ---------------------------------------------------------------- buttons and keys (dark, matte; light leaks out of the gap around them)
const lit = [];                                                  // {kind, id/i, mat, halo, grp, lv, hv, z}
const ledCol = C('#ffc46a'), recCol = C('#ff4a38');
function litThing(shape, depth, color, rough, gapW, gapH, px, py, o) {
  const [x, y] = P(px, py), geo = ext(shape, depth, .025), mat = glowMat(color, rough, depth + .05);
  const grp = new THREE.Group(); grp.position.set(x, y, TOP); dev.add(grp);
  const gap = new THREE.Mesh(new THREE.ShapeGeometry(rr(gapW, gapH, Math.min(gapW, gapH) / 2 * (o.pill ? 1 : .35)), 16), new THREE.MeshBasicMaterial({ color: C('#000') })); gap.position.z = .004; grp.add(gap);
  const halo = new THREE.Mesh(new THREE.PlaneGeometry(gapW + .5, gapH + .5), new THREE.MeshBasicMaterial({ map: haloTex, color: C('#000'), transparent: true, blending: THREE.AdditiveBlending, depthWrite: false })); halo.position.z = .006; halo.scale.set((gapW + .5) / (gapW + .5), 1, 1); grp.add(halo);
  const keyG = new THREE.Group(); keyG.position.z = -.02; grp.add(keyG); const m = new THREE.Mesh(geo, mat); keyG.add(m);
  const rec = Object.assign({ mat, halo, keyG, lv: 0, hv: 0, mesh: m, top: depth + .05 - .02, x, y }, o); m.userData.pick = { kind: o.kind, id: o.id, i: o.i }; pick.push(m); lit.push(rec); return rec;
}
function labelPlane(w, h, draw, z, parent) { const c = cvs(256, Math.round(256 * h / w)), x = c.getContext('2d'); draw(x, c.width, c.height); const t = mkTex(c), m = new THREE.Mesh(new THREE.PlaneGeometry(w, h), new THREE.MeshBasicMaterial({ map: t, transparent: true, depthWrite: false })); m.position.z = z; parent.add(m); needFont.push(() => { x.clearRect(0, 0, c.width, c.height); draw(x, c.width, c.height); t.needsUpdate = true; }); return m; }
Object.entries(BPOS).forEach(([id, [px, py]]) => {
  const oct = id.startsWith('oct'), w = oct ? .5 : .44, h = oct ? .3 : .36;
  const r = litThing(rr(w, h, .07), .09, '#0c0c0f', .5, w + .09, h + .09, px, py, { kind: 'btn', id, small: true, col: id === 'rec' ? recCol : ledCol, pressedZ: .035 });
  labelPlane(w * .9, h * .8, (x, W2, H2) => { x.fillStyle = '#e4e4ea'; x.textAlign = 'center'; x.textBaseline = 'middle'; const L = BTXT[id].split('\n'); x.font = `600 ${L.length > 1 ? 54 : 70}px "Pixelify Sans", monospace`; L.forEach((s, i) => x.fillText(s, W2 / 2, H2 / 2 + (i - (L.length - 1) / 2) * 58)); if (L.length > 1) x.fillRect(W2 * .3, H2 / 2 - 2, W2 * .4, 3); }, .09 + .05 - .02 + .001, r.keyG);
});
// the octave readout, centred above the two OCT buttons: dim "0" when nothing is shifted, gold "+1" / "−2" when it is
const octLab = (() => {
  const [ox, oy] = P((BPOS.octm[0] + BPOS.octp[0]) / 2, BPOS.octm[1] - 31), c = cvs(256, 96), x = c.getContext('2d'), t = mkTex(c), m = new THREE.Mesh(new THREE.PlaneGeometry(.9, .3375), new THREE.MeshBasicMaterial({ map: t, transparent: true, depthWrite: false })); m.position.set(ox, oy, TOP + .12); dev.add(m);
  const oc0 = () => parseInt(((document.getElementById('s-oct') || {}).textContent || 'OCT 0').replace(/[^\-+0-9]/g, '') || '0') || 0;
  const redraw = () => { const oc = oc0(), on = oc !== 0; x.clearRect(0, 0, 256, 96); x.textAlign = 'center'; x.textBaseline = 'middle'; x.fillStyle = on ? '#f0c860' : '#5a5a64'; x.font = '600 34px "Pixelify Sans", monospace'; x.fillText('OCT', 70, 50); x.font = '600 64px "Pixelify Sans", monospace'; x.fillText(oc > 0 ? '+' + oc : oc < 0 ? '\u2212' + (-oc) : '0', 168, 50); t.needsUpdate = true; };
  redraw(); needFont.push(redraw); return { redraw, last: 0 };
})();
const NN = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'], WO = [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26];
const octNow = () => parseInt(((document.getElementById('s-oct') || {}).textContent || 'OCT 0').replace(/[^\-+0-9]/g, '') || '0') || 0, kbdOn = () => { const c = document.getElementById('s-kbd'); return !!c && c.checked; };
const noteOf = (i, o) => { const m = 53 + WO[i] + 12 * o; return NN[m % 12] + (Math.floor(m / 12) - 1); };
const wLab = [];
for (let i = 0; i < 16; i++) { const r = litThing(rr(.43, .76, .215), .2, '#0e0e11', .6, .5, .83, lowX(i), LOWY, { kind: 'wk', i, col: ledCol, pressedZ: .045, pill: true });
  const c = cvs(256, 256), x = c.getContext('2d'), t = mkTex(c), m = new THREE.Mesh(new THREE.PlaneGeometry(.2, .2), new THREE.MeshBasicMaterial({ map: t, transparent: true, depthWrite: false })); m.position.set(0, -.2, .2 + .05 - .02 + .001); r.keyG.add(m);
  const redraw = () => { x.clearRect(0, 0, 256, 256); x.textAlign = 'center'; x.textBaseline = 'middle'; x.fillStyle = '#b4b4be'; x.font = '600 104px "Pixelify Sans", monospace'; x.fillText(noteOf(i, octNow()), 128, kbdOn() ? 92 : 128); if (kbdOn()) { x.fillStyle = '#6a6a74'; x.font = '600 70px "Pixelify Sans", monospace'; x.fillText(A.WK[i].toUpperCase(), 128, 198); } t.needsUpdate = true; };
  redraw(); needFont.push(redraw); wLab.push({ m, redraw }); }
for (let j = 0; j < 11; j++) { const r = litThing(rr(.42, .77, .21), .28, '#070709', .62, .49, .84, UPX[j], UPY, { kind: 'bk', i: j, col: ledCol, pressedZ: .05, pill: true });
  if (A.BLAB[j]) labelPlane(.34, .17, (x, W2, H2) => { x.fillStyle = '#8c8c94'; x.textAlign = 'center'; x.textBaseline = 'middle'; x.font = `500 74px "Pixelify Sans", monospace`; x.fillText(A.BLAB[j], W2 / 2, H2 / 2); }, .28 + .05 - .02 + .001, r.keyG).position.y = -.25; }

// contact shadow on an imaginary floor
const shadowTex = (() => { const c = cvs(256, 128), x = c.getContext('2d'), g = x.createRadialGradient(128, 64, 4, 128, 64, 120); g.addColorStop(0, 'rgba(0,0,0,.75)'); g.addColorStop(1, 'rgba(0,0,0,0)'); x.fillStyle = g; x.save(); x.scale(1, .5); x.fillRect(0, 0, 256, 256); x.restore(); return new THREE.CanvasTexture(c); })();
const shadow = new THREE.Mesh(new THREE.PlaneGeometry(1, 1), new THREE.MeshBasicMaterial({ map: shadowTex, transparent: true, depthWrite: false })); shadow.rotation.x = -Math.PI / 2; shadow.position.y = -3.4; scene.add(shadow);

document.fonts && document.fonts.load('500 40px "Pixelify Sans"').then(() => document.fonts.load('600 40px "Pixelify Sans"')).then(() => needFont.forEach(f => f()));

// ---------------------------------------------------------------- floating labels (parameter names under the knobs)
const kl = {};
Object.entries(KPOS).forEach(([id, [px, py]]) => { const d = document.createElement('div'); d.className = 'kl'; d.innerHTML = '<b></b><i></i>'; d.style.setProperty('--c', KCOL[id] || '#b9b4c2'); labels.append(d); kl[id] = { d, b: d.firstChild, i: d.lastChild, p: new THREE.Vector3(knobs[id].x, knobs[id].y - .36, TOP + .02) }; });
const statusEl = document.createElement('div'); statusEl.className = 's3d-status'; stage.insertAdjacentElement('afterend', statusEl);

// ---------------------------------------------------------------- input: raycasting against the model
const ray = new THREE.Raycaster(), ndc = new THREE.Vector2(); let mouse = { x: 0, y: 0, in: false }, hover = null, act = null;
function hit(e) { const r = cv.getBoundingClientRect(); ndc.set(((e.clientX - r.left) / r.width) * 2 - 1, -((e.clientY - r.top) / r.height) * 2 + 1); ray.setFromCamera(ndc, camera); const h = ray.intersectObjects(pick, false)[0]; return h ? h.object.userData.pick : null; }
stage.addEventListener('pointermove', e => { const r = stage.getBoundingClientRect(); mouse.x = ((e.clientX - r.left) / r.width) * 2 - 1; mouse.y = ((e.clientY - r.top) / r.height) * 2 - 1; mouse.in = true;
  if (act && act.kind === 'knob') { act.acc += act.y0 - e.clientY; act.y0 = e.clientY; while (Math.abs(act.acc) >= 7) { A.knobTurn(act.id, act.acc > 0 ? 1 : -1); act.acc -= Math.sign(act.acc) * 7; } return; }
  if (e.target === cv && !act) { const nh = hit(e), same = nh && hover && nh.kind === hover.kind && nh.id === hover.id && nh.i === hover.i; hover = nh; cv.style.cursor = nh ? (nh.kind === 'knob' ? 'ns-resize' : 'pointer') : 'default';
    if (nh) { if (same) A.moveTip(e.clientX, e.clientY); else A.showTip(A.describe(nh.kind, nh.id, nh.i), e.clientX, e.clientY); } else A.hideTip(); }
  else if (e.target !== cv) { hover = null; A.hideTip(); } });
stage.addEventListener('pointerleave', () => { mouse.in = false; hover = null; A.hideTip(); });
cv.addEventListener('pointerdown', e => { const h = hit(e); if (!h) return; e.preventDefault(); A.hideTip(); cv.setPointerCapture(e.pointerId);
  if (h.kind === 'btn') { if (e.button === 2 || e.shiftKey) { A.toggleLatch(h.id); return; } if (sim.latched.has(h.id)) { A.toggleLatch(h.id); return; } act = h; A.btnDown(h.id); }
  else if (h.kind === 'wk') { act = h; A.keyPress(h.i, false); } else if (h.kind === 'bk') { act = h; A.keyPress(h.i, true); }
  else if (h.kind === 'knob') act = { kind: 'knob', id: h.id, acc: 0, y0: e.clientY }; });
const up = () => { if (!act) return; if (act.kind === 'btn') A.btnUp(act.id); else if (act.kind === 'wk') A.keyRelease(act.i, false); else if (act.kind === 'bk') A.keyRelease(act.i, true); act = null; };
cv.addEventListener('pointerup', up); cv.addEventListener('pointercancel', up); cv.addEventListener('contextmenu', e => e.preventDefault());
cv.addEventListener('wheel', e => { const h = hit(e); if (h && h.kind === 'knob') { e.preventDefault(); A.knobTurn(h.id, e.deltaY < 0 ? 1 : -1); } }, { passive: false });

// ---------------------------------------------------------------- DOM LCD pinned to the glass (square → quad homography as a CSS matrix3d)
const tmp = new THREE.Vector3(); let sw = 1, sh = 1;
function homography(q) {
  const [[x0, y0], [x1, y1], [x2, y2], [x3, y3]] = q, dx1 = x1 - x2, dx2 = x3 - x2, dx3 = x0 - x1 + x2 - x3, dy1 = y1 - y2, dy2 = y3 - y2, dy3 = y0 - y1 + y2 - y3, den = dx1 * dy2 - dy1 * dx2;
  const g = (dx3 * dy2 - dy3 * dx2) / den, h = (dx1 * dy3 - dy1 * dx3) / den, a = x1 - x0 + g * x1, b = x3 - x0 + h * x3, d = y1 - y0 + g * y1, e = y3 - y0 + h * y3, s = 1 / 240;
  return `matrix3d(${a * s},${d * s},0,${g * s},${b * s},${e * s},0,${h * s},0,0,1,0,${x0},${y0},0,1)`;
}
const proj = v => { tmp.copy(v).applyMatrix4(dev.matrixWorld).project(camera); return [(tmp.x + 1) / 2 * sw, (1 - tmp.y) / 2 * sh]; };
let shown = false;
function syncScreen() { const q = screenCorners.map(proj); scr.style.transform = homography(q); if (!shown) { shown = true; scr.style.visibility = 'visible'; } }

// ---------------------------------------------------------------- motion: floats and sways; calmer on hover; leans towards the side the cursor is on
const st = { hv: 0, tx: 0, ty: 0, zoom: 0, zt: 0, tv: 0, top: false, float: true, lock: false, pix: false, pose: null }, BASE = -.6, SIGN = 1;       // SIGN = 1: the side near the cursor comes toward you; -1 flips it
// ---- pixel-art / ordered-dither pass: ONLY the 3D body is redrawn (the LCD is a DOM layer above it and is never filtered)
const pcv = document.createElement('canvas'); pcv.className = 'pix3d'; pcv.style.cssText = 'position:absolute;left:0;top:0;width:100%;height:100%;image-rendering:pixelated;pointer-events:none;display:none';
cv.insertAdjacentElement('afterend', pcv);
const pctx = pcv.getContext('2d', { willReadFrequently: true }), BAY = [0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5];
const pix = { size: 3, levels: 6, gamma: .62, gain: 1.15 };
function pixPass() {
  if (!st.pix) { if (pcv.style.display !== 'none') { pcv.style.display = 'none'; cv.style.visibility = ''; } return; }
  const w = Math.max(1, Math.round(sw / pix.size)), h = Math.max(1, Math.round(sh / pix.size));
  if (pcv.width !== w || pcv.height !== h) { pcv.width = w; pcv.height = h; }
  pctx.imageSmoothingEnabled = true; pctx.imageSmoothingQuality = 'high'; pctx.clearRect(0, 0, w, h); pctx.drawImage(cv, 0, 0, w, h);
  const im = pctx.getImageData(0, 0, w, h), d = im.data, L = pix.levels - 1;
  for (let y = 0, i = 0; y < h; y++) for (let x = 0; x < w; x++, i += 4) {
    const a = d[i + 3]; if (a < 96) { d[i + 3] = 0; continue; }
    const t = (BAY[(y & 3) * 4 + (x & 3)] + .5) / 16 - .5;                       // ordered dither threshold, -0.5 .. 0.5
    for (let c = 0; c < 3; c++) { const v = Math.pow(Math.min(1, d[i + c] / 255 * (a < 255 ? 255 / a : 1) * pix.gain), pix.gamma); d[i + c] = Math.max(0, Math.min(255, Math.round(Math.floor(Math.min(1, v) * L + .5 + t * .9) / L * 255))); }
    d[i + 3] = 255;
  }
  pctx.putImageData(im, 0, 0);
  pcv.style.display = ''; cv.style.visibility = 'hidden';
}
function setPix(on) { st.pix = !!on; stage.classList.toggle('pix', st.pix); }
let D = 16, last = performance.now();
function resize() { const w = Math.max(320, host.clientWidth || stage.clientWidth), h = clamp(Math.round(w * .52), 360, 680); stage.style.height = h + 'px'; sw = w; sh = h; renderer.setSize(w, h, true); camera.aspect = w / h; camera.fov = 24; const vf = camera.fov * Math.PI / 180; D = (W * 1.02 / 2) / (Math.tan(vf / 2) * camera.aspect); D = Math.max(D, (H * 1.12 / 2) / Math.tan(vf / 2)); camera.updateProjectionMatrix(); }
addEventListener('resize', () => { if (host.classList.contains('is3d')) resize(); }); if (window.ResizeObserver) new ResizeObserver(() => { if (host.classList.contains('is3d') && host.clientWidth !== sw) resize(); }).observe(host);
const scrC = new THREE.Vector3(lx, ly, TOP), camTgt = new THREE.Vector3(), camPos = new THREE.Vector3(), scrW = new THREE.Vector3();
function frame(now) {
  requestAnimationFrame(frame); if (!host.classList.contains('is3d') || !host.offsetParent || document.hidden) { last = now; return; }
  if (host.clientWidth && Math.abs(host.clientWidth - sw) > 1) resize();
  const dt = Math.min(.05, (now - last) / 1000); last = now; const t = now / 1000;
  st.hv += ((mouse.in && !st.lock ? 1 : 0) - st.hv) * (1 - Math.exp(-dt * 5)); st.zoom += (st.zt - st.zoom) * (1 - Math.exp(-dt * 5.5)); st.tv += ((st.top ? 1 : 0) - st.tv) * (1 - Math.exp(-dt * 5));
  const amp = st.float && !st.lock && !st.pose ? (1 - .8 * st.hv) * (1 - .65 * st.zoom) * (1 - .75 * st.tv) : 0;
  st.tx += ((-mouse.x * SIGN * .2 * st.hv) - st.tx) * (1 - Math.exp(-dt * 7)); st.ty += ((mouse.y * .12 * st.hv) - st.ty) * (1 - Math.exp(-dt * 7));
  dev.rotation.y = st.tx + Math.sin(t * .55) * .24 * amp + Math.sin(t * .21 + 2) * .05 * amp;
  dev.rotation.x = lerp(BASE, 0, st.tv) - st.ty * (1 - .5 * st.tv) + Math.sin(t * .43 + 1) * .05 * amp;
  dev.rotation.z = Math.sin(t * .37) * .025 * amp;
  if (st.pose) { dev.rotation.x = st.pose[0]; dev.rotation.y = st.pose[1]; dev.rotation.z = st.pose[2]; }
  const bob = st.lock || st.pose ? 0 : Math.sin(t * .9) * .09 * Math.max(amp, st.float ? .25 : 0); root.position.y = bob;
  shadow.scale.set(9.6 - bob * 3, 3.6 - bob, 1); shadow.material.opacity = clamp(.55 - bob * 1.2, .2, .7); shadow.position.y = -3.4 - bob * .3;
  root.updateMatrixWorld(true);
  // camera: full view, or glide towards the screen
  scrW.copy(scrC).applyMatrix4(dev.matrixWorld); const e = st.zoom * st.zoom * (3 - 2 * st.zoom);
  camTgt.set(lerp(0, scrW.x, e), lerp(0, scrW.y, e), lerp(0, scrW.z, e)); camPos.set(lerp(0, scrW.x, e), lerp(lerp(1.7, 0, st.tv), scrW.y + .9, e), lerp(D, scrW.z + D * .46, e)); camera.position.copy(camPos); camera.lookAt(camTgt);
  // state from the simulator
  lit.forEach(r => {
    const L = r.kind === 'btn' ? A.leds[r.id] : r.kind === 'wk' ? A.leds['w' + r.i] : A.leds['b' + r.i];
    const down = r.kind === 'btn' ? !!A.press[r.id] || sim.latched.has(r.id) : r.kind === 'wk' ? A.wDown.has(r.i) : A.bDown.has(r.i);
    const hov = hover && hover.kind === r.kind && hover.id === r.id && hover.i === r.i ? (r.small ? .22 : .14) : 0;
    const tgt = clamp(((L ? L.l : 0) === 2 ? 1 : (L ? L.l : 0) === 1 ? .36 : 0) + hov + (down ? .35 : 0), 0, 1.3);
    r.lv += (tgt - r.lv) * (tgt > r.lv ? .55 : .16); r.hv = r.lv;
    const col = L && L.c ? colOf(L.c) : r.col; r.mat.userData.u.uLevel.value = r.lv * (r.small ? .8 : 1); r.mat.userData.u.uColor.value.copy(col);
    r.halo.material.color.copy(col).multiplyScalar(clamp(r.lv, 0, 1.3) * (r.small ? .55 : .75));
    r.keyG.position.z = -.02 - (down ? r.pressedZ : 0);
  });
  const oc = octNow(), kb = kbdOn(); if (oc !== octLab.last) { octLab.last = oc; octLab.redraw(); } if (oc !== wLab.last || kb !== wLab.lastK) { wLab.last = oc; wLab.lastK = kb; wLab.forEach(l => l.redraw()); }
  Object.entries(knobs).forEach(([id, k]) => { k.pg.rotation.z = -(sim.ang[id] || 0) * Math.PI / 180; });
  renderer.render(scene, camera); pixPass(); syncScreen(); mirror();
  // labels
  Object.entries(kl).forEach(([id, o]) => { const q = proj(o.p), src = id.startsWith('k') ? (host.querySelector(`[data-l=${id}]`) || {}).textContent : '', val = (host.querySelector(`[data-v=${id}]`) || {}).textContent || ''; o.b.textContent = id.startsWith('k') ? src : ''; o.i.textContent = id.startsWith('k') ? val : ''; o.d.style.transform = `translate(${q[0]}px,${q[1]}px) translate(-50%,0)`; o.d.style.opacity = id.startsWith('k') && (src === '—' || !src) ? .3 : 1; });
  const sv = n => ((host.querySelector(`[data-v=${n}]`) || {}).textContent || '').trim(), line = `PRESET ${sv('presets').replace(/\D+/g, '') || '1'}  ·  HERO ${sv('algo').replace(/\D+/g, '') || '1'}  ·  OCT ${octNow() > 0 ? '+' : ''}${octNow()}  ·  MASTER ${sv('master') || ''}`; if (statusEl.textContent !== line) statusEl.textContent = line;
}
requestAnimationFrame(frame);


// ---------------------------------------------------------------- 2D miniature of the screen (draggable) + icon tools
const mini = document.createElement('div'); mini.className = 'mini3d'; mini.hidden = true;
mini.innerHTML = '<div class="mh"><span class="grip"></span><b>tela</b><button class="ms" title="mudar o tamanho">+</button><button class="mx" title="fechar">×</button></div><div class="mb"></div>';
const mb = mini.querySelector('.mb'); stage.append(mini);
const MS = [1.2, 1.6, 2]; const mst = { on: false, s: 0, x: null, y: 14 }; try { Object.assign(mst, JSON.parse(localStorage.getItem('fm1lab.mini') || '{}')); } catch (e) {}
let pairs = [], mDirty = true;
new MutationObserver(() => { mDirty = true; }).observe(A.lcdbox, { childList: true, subtree: true, characterData: true, attributes: true, attributeFilter: ['style', 'class'] });
function mirror() {
  if (mini.hidden) return;
  if (mDirty) { mDirty = false; const cl = A.lcdbox.cloneNode(true); cl.classList.add('mirror'); mb.replaceChildren(cl); const src = [...A.lcdbox.querySelectorAll('canvas')], dst = [...cl.querySelectorAll('canvas')]; pairs = src.map((c, i) => [c, dst[i]]); }
  pairs.forEach(([c, d]) => { if (d && c.width) { const x = d.getContext('2d'); x.imageSmoothingEnabled = false; x.clearRect(0, 0, d.width, d.height); x.drawImage(c, 0, 0); } });
}
function placeMini() { mb.style.setProperty('--z', MS[mst.s]); const sr = stage.getBoundingClientRect(); const w = mini.offsetWidth || 300; if (mst.x == null || mst.x > sr.width - 40) mst.x = Math.max(8, sr.width - w - 14); mini.style.left = clamp(mst.x, 0, Math.max(0, sr.width - w)) + 'px'; mini.style.top = clamp(mst.y, 0, Math.max(0, sr.height - 60)) + 'px'; }
const saveMini = () => { try { localStorage.setItem('fm1lab.mini', JSON.stringify(mst)); } catch (e) {} };
function setMini(on) { mst.on = on; mini.hidden = !on; mDirty = true; if (on) placeMini(); saveMini(); tools.querySelector('[data-t=mini]').classList.toggle('on', on); }
let drag = null; const mh = mini.querySelector('.mh');
mh.addEventListener('pointerdown', e => { if (e.target.closest('button')) return; mh.setPointerCapture(e.pointerId); const r = mini.getBoundingClientRect(); drag = { ox: e.clientX - r.left, oy: e.clientY - r.top }; });
mh.addEventListener('pointermove', e => { if (!drag) return; const sr = stage.getBoundingClientRect(); mst.x = clamp(e.clientX - sr.left - drag.ox, 0, sr.width - mini.offsetWidth); mst.y = clamp(e.clientY - sr.top - drag.oy, 0, sr.height - 40); mini.style.left = mst.x + 'px'; mini.style.top = mst.y + 'px'; });
mh.addEventListener('pointerup', () => { drag = null; saveMini(); });
mini.querySelector('.mx').onclick = () => setMini(false);
mini.querySelector('.ms').onclick = () => { mst.s = (mst.s + 1) % MS.length; placeMini(); saveMini(); };
const SVG = { mini: '<svg viewBox="0 0 24 24"><rect x="2.5" y="4" width="19" height="14" rx="2.5" fill="none" stroke="currentColor" stroke-width="1.8"/><rect x="12" y="10" width="8" height="6" rx="1.2" fill="currentColor"/></svg>', zoom: '<svg viewBox="0 0 24 24"><circle cx="10.5" cy="10.5" r="6.2" fill="none" stroke="currentColor" stroke-width="1.8"/><path d="M15.2 15.2 21 21M10.5 7.8v5.4M7.8 10.5h5.4" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/></svg>', top: '<svg viewBox="0 0 24 24"><rect x="2.5" y="6" width="19" height="12" rx="2.5" fill="none" stroke="currentColor" stroke-width="1.8"/><rect x="9" y="8.4" width="6" height="4" rx="1" fill="currentColor"/><path d="M5 15.5h14" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-dasharray="1.6 2.2"/></svg>' };
const tools = document.createElement('div'); tools.className = 'tools3d';
const LANGQ = (new URLSearchParams(location.search).get('lang') || document.documentElement.lang || 'pt').slice(0, 2);
tools.innerHTML = [['mini', 'miniatura da tela (arraste para onde quiser)'], ['zoom', 'aproximar a tela'], ['top', 'vista de cima']].map(([k, t]) => `<button data-t="${k}" title="${t}">${SVG[k]}</button>`).join(''); stage.append(tools);
function setZoom(on) { st.zt = on ? 1 : 0; tools.querySelector('[data-t=zoom]').classList.toggle('on', on); const z = document.getElementById('s-zoom'); if (z) { z.classList.toggle('on', on); z.textContent = on ? 'ver painel todo' : 'aproximar tela'; } }
tools.onclick = e => { const b = e.target.closest('button'); if (!b) return; const k = b.dataset.t; if (k === 'mini') setMini(mini.hidden); else if (k === 'zoom') setZoom(!st.zt); else { st.top = !st.top; b.classList.toggle('on', st.top); } };

// ---------------------------------------------------------------- toolbar
const r1 = host.querySelector('.stool .r1');
const mk = (tag, html, id) => { const e = document.createElement(tag); e.innerHTML = html; if (id) e.id = id; return e; };
r1.prepend(mk('button', 'painel 3D', 's-3d'));
const z = mk('button', 'aproximar tela', 's-zoom'); const fl = mk('label', '<input type="checkbox" id="s-float" checked> flutuar'); const kb = mk('label', '<input type="checkbox" id="s-kbd" checked> letras do teclado');
r1.append(z, fl, kb);
document.getElementById('s-3d').onclick = () => set3d(!host.classList.contains('is3d'));
z.onclick = () => setZoom(!st.zt);
document.getElementById('s-float').onchange = e => { st.float = e.target.checked; };
let on = true; try { on = localStorage.getItem('fm1lab.sim3d2') !== '0'; } catch (e) {}
set3d(on); if (mst.on) setMini(true);
G.SIM3D = { scene, camera, renderer, dev, st, setPix, pix, corners: () => screenCorners.map(proj), size: () => [sw, sh] };
})(window);
