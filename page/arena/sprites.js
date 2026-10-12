/* FM1 Arena · lutadores (CraftPix): troca de roupa pela cor da faixa, acessórios pela cor do firmware, sem contorno preto. */
(function () {
'use strict';
const A = window.A;
A.TRACKCOL = ['#f0c860', '#7ab04e', '#e08a3a', '#5a8ad0'];                 // as cores das 4 faixas no firmware
A.PLAYERCOL = ['#5a8ad0', '#e08a3a', '#f0c860', '#7ab04e'];   // 2x2: azul, laranja, amarelo, verde (um por jogador)
A.FWCOL = { quest: '#e8dcc8', sloop: '#38b8e8', felucca: '#a0a0a8', baudgirl: '#b53480', mvave: '#d0463c', other: '#707078' };
const hex = (h) => [parseInt(h.slice(1, 3), 16), parseInt(h.slice(3, 5), 16), parseInt(h.slice(5, 7), 16)];
const mul = (c, k) => c.map(v => Math.max(0, Math.min(255, Math.round(v * k))));
const lit = (c, k) => c.map(v => Math.round(v + (255 - v) * k));
const key = (r, g, b) => (r << 16) | (g << 8) | b;
const CELL = 64;
/* quadros (linha, coluna) da folha 6×12 de 64 px; todos olham para a direita */
A.FRAMES = {
  idle: [[0, 0], [0, 1], [0, 2], [0, 3], [0, 4], [0, 5]],
  walk: [[2, 0], [2, 1], [2, 2], [2, 3]],
  run:  [[4, 0], [4, 1], [4, 2], [4, 3]],
  punch0: [[5, 0], [5, 1], [5, 3], [5, 2]], punch1: [[6, 0], [6, 1], [6, 2], [6, 3]], punch2: [[7, 0], [7, 1], [7, 2], [7, 3]],
  kick0: [[8, 0], [8, 1], [8, 2], [8, 3]], kick1: [[9, 0], [9, 1], [9, 2], [9, 3]], kick2: [[9, 0], [9, 1], [9, 2], [9, 3]],
  hurt: [[10, 0]], flash: [[3, 0]], launch: [[10, 0], [10, 1]], down: [[10, 2], [10, 3]], win: [[11, 2], [11, 3]],
};
const sheets = {}, cache = {};
function load(n) {
  if (sheets[n]) return sheets[n];
  const im = new Image(); im.src = 'img/fighters/' + n + '.png'; sheets[n] = im; return im;
}
/* troca de cores + remoção do contorno (preto) */
function recolor(n, trk, fw) {
  const ck = n + '|' + trk + '|' + fw; if (cache[ck]) return cache[ck];
  const im = load(n); if (!im.complete || !im.naturalWidth) return null;
  const W = im.naturalWidth, H = im.naturalHeight, cv = document.createElement('canvas'); cv.width = W; cv.height = H;
  const c = cv.getContext('2d'); c.drawImage(im, 0, 0); const d = c.getImageData(0, 0, W, H), p = d.data;
  const T = hex(trk), F = hex(fw);
  const map = new Map([
    [key(0x46, 0xc6, 0x57), T], [key(0x2c, 0x5b, 0x6d), mul(T, .62)],            // roupa: cor da faixa (clara/escura)
    [key(0xcc, 0xe2, 0xe1), F], [key(0xc9, 0xec, 0x85), lit(F, .45)], [key(0x8b, 0xab, 0xbf), mul(F, .7)],   // faixa da cintura, luvas, sapatilhas: cor do firmware
  ]);
  const src = new Uint8ClampedArray(p);
  const A_ = (x, y) => (x < 0 || y < 0 || x >= W || y >= H) ? 0 : src[(y * W + x) * 4 + 3];
  for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
    const i = (y * W + x) * 4; if (!src[i + 3]) continue;
    const r = src[i], g = src[i + 1], b = src[i + 2];
    if (r === 0 && g === 0 && b === 0) {                                        // contorno
      if (!A_(x - 1, y) || !A_(x + 1, y) || !A_(x, y - 1) || !A_(x, y + 1)) { p[i + 3] = 0; continue; }
      // contorno interno vira um tom escuro da cor vizinha (separa membros sem linha preta)
      let best = null; for (const [dx, dy] of [[-1, 0], [1, 0], [0, -1], [0, 1]]) { const j = ((y + dy) * W + x + dx) * 4; if (src[j + 3] && !(src[j] === 0 && src[j + 1] === 0 && src[j + 2] === 0)) { best = [src[j], src[j + 1], src[j + 2]]; break; } }
      const o = mul(best || [60, 40, 40], .55); p[i] = o[0]; p[i + 1] = o[1]; p[i + 2] = o[2]; continue;
    }
    const m = map.get(key(r, g, b)); if (m) { p[i] = m[0]; p[i + 1] = m[1]; p[i + 2] = m[2]; }
  }
  c.putImageData(d, 0, 0); cache[ck] = cv; return cv;
}
A.sprite = {
  /* (c ctx, n 1..8 personagem, trk/fw cores, anim nome, t tempo na animação, x/y pés, face ±1, esc. de tela) */
  draw(c, n, trk, fw, anim, t, x, y, face, sc) {
    const sh = recolor(n, trk, fw); if (!sh) return;
    const fr = A.FRAMES[anim] || A.FRAMES.idle; let k;
    if (anim.startsWith('punch') || anim.startsWith('kick')) k = Math.min(fr.length - 1, Math.floor(t / .075));
    else if (anim === 'down' || anim === 'hurt' || anim === 'flash') k = Math.min(fr.length - 1, Math.floor(t / .2));
    else k = Math.floor(t * (anim === 'run' ? 14 : anim === 'walk' ? 8 : 7)) % fr.length;
    const [r, col] = fr[k];
    c.save(); c.translate(Math.round(x), Math.round(y)); c.scale(face * sc, sc);
    c.drawImage(sh, col * CELL, r * CELL, CELL, CELL, -30, -CELL + 1, CELL, CELL); c.restore();
  },
  preload() { for (let i = 1; i <= 8; i++) load(i); },
};
A.sprite.preload();
})();
