// Run in the emulator page (needs js/scene.js): bakes the TAVERN sprites to palette-indexed bitmaps.
// Result: {frames:[{name,ox,oy,w,h,data(base64)}]}. quest/tools/make_sprites.py turns it into firmware/src/quest_sprites.h.
// 255 = transparent, 0..15 = the 16-colour scene palette (design README).
(function () {
  const S = Scene, W = 40, H = 44, OX = 12, OY = 16;            // canvas with room for hats, blades, bows
  const frames = [];
  function grab(name, draw) {
    const buf = new Uint8Array(W * H).fill(255);
    const R = (x, y, w, h, i) => { x = Math.round(x); y = Math.round(y); w = Math.max(1, Math.round(w)); h = Math.max(1, Math.round(h)); for (let yy = y; yy < y + h; yy++) for (let xx = x; xx < x + w; xx++) if (xx >= 0 && xx < W && yy >= 0 && yy < H) buf[yy * W + xx] = i; };
    draw(R);
    let x0 = W, y0 = H, x1 = -1, y1 = -1;
    for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) if (buf[y * W + x] !== 255) { x0 = Math.min(x0, x); x1 = Math.max(x1, x); y0 = Math.min(y0, y); y1 = Math.max(y1, y); }
    const w = x1 - x0 + 1, h = y1 - y0 + 1, out = new Uint8Array(w * h);
    for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) out[y * w + x] = buf[(y0 + y) * W + x0 + x];
    frames.push({ name, ox: x0 - OX, oy: y0 - OY, w, h, data: btoa(String.fromCharCode(...out)) });
  }
  const cls = ['mage', 'archer', 'cleric', 'warrior'];
  const poses = { mage: [['idle', null], ['cast', 0.1]], archer: [['idle', null], ['draw', -0.2]], cleric: [['idle', null], ['cast', 0.2]], warrior: [['idle', null], ['antic', -0.2], ['strike', 0.1], ['follow', 0.4]] };
  cls.forEach(c => {
    poses[c].forEach(([n, p]) => {
      [0, 1].forEach(f => grab(`${c}_${n}_${f}`, R => S.figure(R, c, OX, OY, { t: 0, p, f })));
    });
    grab(`${c}_seat`, R => S.figure(R, c, OX, OY, { t: 0, sit: 1 }));
  });
  // slimes: squash / stretch states (the hop), plain and hit-flash (red); King Slime is drawn by the scene code
  [[-0.12, 'air'], [0, 'rest'], [0.22, 'land']].forEach(([sq, n]) => [0, 1].forEach(fl => grab(`slime_${n}_${fl}`, R => S.slimeD(R, OX + 11, OY + 16, 22, 16, S.MON['1c'].slime, { sq, flash: fl, noShadow: 1 }))));
  grab('bat_0', R => S.spr(R, S.BATA, OX, OY, S.MON['1c'].bat, { s: 2 }));
  grab('bat_1', R => S.spr(R, S.BATB, OX, OY, S.MON['1c'].bat, { s: 2 }));
  return JSON.stringify({ frames });
})();
