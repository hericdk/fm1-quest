/* FM1 Arena · luta: simulação (host), render, CPU e treino. */
(function () {
'use strict';
const A = window.A;
const VW = 384, VH = 216;
const COL = [['#ff5a4a', '#a8281c'], ['#4a8cff', '#2250b0'], ['#4ac86a', '#23803c'], ['#f0c840', '#a07c10']];
const KIND = ['Soco fraco', 'Soco médio', 'Soco forte', 'Chute fraco', 'Chute médio', 'Chute forte'];
const MAJ = [0, 2, 4, 5, 7, 9, 11];
const BASE = .9;

/* ============ sprites ============ */
function drawChar(c, x, y, f, st, t, tone) {
  // c: contexto; (x,y) = pés; f = +1 olhando para a direita, -1 para a esquerda; tone [claro, escuro]
  const [L, D] = tone, K = '#14101a';
  const P = (px, py, w, h, col) => {            // retângulo com contorno de 1 px; x espelhado por f
    const X = f > 0 ? x + px : x - px - w;
    c.fillStyle = K; c.fillRect(X - 1, y + py - 1, w + 2, h + 2);
    c.fillStyle = col; c.fillRect(X, y + py, w, h);
  };
  let bob = 0, lean = 0, la = 0, lb = 0, aa = [0, 0], ab = [0, 0], lyf = 0, headDy = 0;
  const wsin = Math.sin(t * 14);
  if (st === 'idle') { bob = Math.sin(t * 4) > 0 ? 0 : 1; }
  else if (st === 'walk') { bob = Math.abs(wsin) > .5 ? 0 : 1; la = Math.round(wsin * 3); lb = -la; aa = [Math.round(-wsin * 2), 0]; ab = [-aa[0], 0]; }
  else if (st === 'run') { lean = 2; la = Math.round(wsin * 5); lb = -la; aa = [-4, -1]; ab = [-5, 1]; bob = Math.abs(wsin) > .5 ? 0 : 1; }
  else if (st.startsWith('punch')) { const k = +st[5]; lean = 1 + k; aa = [6 + k * 2, -9]; ab = [-3, -6]; la = 2; lb = -3; }
  else if (st.startsWith('kick')) { const k = +st[4]; lean = -1; lyf = 1; la = 7 + k * 2; aa = [-3, -4]; ab = [-4, -2]; lb = -2; }
  else if (st === 'hurt') { lean = -3; headDy = -1; aa = [-4, -6]; ab = [-5, -4]; la = -2; lb = 2; }
  else if (st === 'launch') { lean = -4; aa = [-6, -10]; ab = [-7, -8]; la = 4; lb = 6; }
  else if (st === 'down') {
    P(-12, -6, 6, 6, L); P(-6, -7, 10, 7, D); P(4, -6, 8, 3, L); P(4, -3, 8, 3, D); return;
  }
  // costas -> frente (camadas: tom escuro atrás, claro na frente)
  P(-4 + lean, -22 + bob, 3, 8, D);                       // braço de trás
  P(-3, -9 + bob * 0, 3, 9 + lb * 0, D);                  // perna de trás
  if (lb) P(-3 + lb, -3, 4, 3, D);
  P(-3 + lean, -17 + bob, 8, 9, L);                       // tronco
  P(-4 + lean + (st === 'hurt' ? -1 : 0), -25 + bob + headDy, 8, 8, L);   // cabeça
  c.fillStyle = '#fff'; const ex = f > 0 ? x + 1 + lean : x - 1 - lean - 2; c.fillRect(ex, y - 22 + bob + headDy, 2, 2);
  P(1, -9, 3, 9, L);                                       // perna da frente
  if (la) P(1 + la, -3 + lyf * -3, 4 + (st.startsWith('kick') ? 3 : 0), 3, L);
  P(3 + lean + aa[0], -20 + bob + aa[1] * 0 + (st.startsWith('punch') ? aa[1] + 9 : 0), 3 + (st.startsWith('punch') ? 4 : 0), 3, L);   // braço da frente
}
function drawHand(c, x, y, dir) {
  const g = ['..WWW.....', '.WWWWW.WW.', '.WWWWWWWWW', 'WWWWWWWWWW', 'WWWWWWWWW.', '.WWWWWWWW.', '..WWWWWW..', '...BBBB...'];
  for (let j = 0; j < g.length; j++) for (let i = 0; i < 10; i++) {
    const ch = g[j][i]; if (ch === '.') continue;
    const X = dir > 0 ? x + i * 2 : x + (9 - i) * 2;
    c.fillStyle = '#14101a'; c.fillRect(X - 1, y + j * 2 - 1, 4, 4);
  }
  for (let j = 0; j < g.length; j++) for (let i = 0; i < 10; i++) {
    const ch = g[j][i]; if (ch === '.') continue;
    const X = dir > 0 ? x + i * 2 : x + (9 - i) * 2;
    c.fillStyle = ch === 'W' ? '#fff' : '#d0463c'; c.fillRect(X, y + j * 2, 2, 2);
  }
}
function drawDie(c, x, y, n, s) {
  c.fillStyle = '#14101a'; c.fillRect(x - 2, y - 2, s + 4, s + 4); c.fillStyle = '#fff'; c.fillRect(x, y, s, s);
  const p = { 1: [4], 2: [0, 8], 3: [0, 4, 8], 4: [0, 2, 6, 8], 5: [0, 2, 4, 6, 8], 6: [0, 2, 3, 5, 6, 8] }[n] || [];
  c.fillStyle = '#14101a'; const u = s / 4;
  p.forEach(k => c.fillRect(x + u * (.7 + (k % 3) * 1.1), y + u * (.7 + Math.floor(k / 3) * 1.1), u * .7, u * .7));
}
function drawMute(c, x, y) {
  c.fillStyle = '#888'; c.fillRect(x, y + 4, 4, 6); c.fillRect(x + 4, y + 2, 3, 10); c.fillRect(x + 7, y, 2, 14);
  c.fillStyle = '#d0463c'; for (let i = 0; i < 8; i++) { c.fillRect(x + 11 + i, y + 3 + i, 2, 2); c.fillRect(x + 18 - i, y + 3 + i, 2, 2); }
}

/* ============ jogo ============ */
class Game {
  constructor(o) {
    this.o = o; this.mode = o.mode;                       // 'cpu' | 'pvp'
    this.cv = o.canvas; this.cx = this.cv.getContext('2d'); this.cx.imageSmoothingEnabled = false;
    this.cv.width = VW * 2; this.cv.height = VH * 2;
    this.stage = null; this.imgs = {}; this.cam = 0; this.camT = 0;
    this.s = { gone: [], ph: 'wait', side: 0, turn: 0, tl: 0, total: 120, hp: [100, 100], ult: [0, 0], dice: [1, 1], bpm: 110, combo: [0, 0], fw: o.fw || ['quest', 'other'], names: [] };
    this.ch = [[], []];
    for (let sd = 0; sd < 2; sd++) for (let i = 0; i < 4; i++) this.ch[sd].push({ sd, i, x: 0, y: 0, st: 'idle', t: Math.random() * 6, tx: null, ty: 0, face: sd ? -1 : 1, busy: 0, vx: 0, vy: 0, z: 0 });
    this.owner = [[], []]; (this.players || []).forEach((p, pi) => (p.chars || []).forEach(ci => { this.owner[p.side][ci] = pi; }));
    this.fx = []; this.shake = 0; this.flash = 0; this.t0 = performance.now(); this.hist = [[], []]; this.seq = 0; this.kinds = [[], []];
    this.host = o.host !== false; this.players = o.players || [];
    this.me = o.me || { side: 0, chars: [0, 1], id: A.net.id };
    if (o.resume) { this.s = Object.assign(this.s, o.resume); this.lastSt = performance.now(); }
    this.loadStage(o.stage || '96');
    this.layoutIdle(true);
    this.raf = requestAnimationFrame((t) => this.loop(t));
    this.unNote = A.on('note', (e) => { if (e.on) this.localNote(e); else this.localOff(e); });
    this.cpuT = 0; this.ticked = -1; this.alive = true; this.held = {};
    if (A.net.ch) this.unNet = [A.on('net:in', (m) => this.host && this.hostNote(m.pid || m.from, m.n, m.v, m.tm, undefined, m.ch)), A.on('net:off', (m) => this.host && this.noteOff(m.from, m.n)), A.on('net:st', (m) => { if (!this.host) { this.s = Object.assign(this.s, m.s); this.lastSt = performance.now(); } }),
      A.on('net:ev', (m) => this.onEv(m.e))];
  }
  destroy() { this.alive = false; cancelAnimationFrame(this.raf); this.unNote && this.unNote(); (this.unNet || []).forEach(u => u()); }
  async loadStage(id) {
    if (!A.stages) A.stages = await (await fetch('stages.json')).json();
    this.stage = A.stages[id] || A.stages['96']; this.stage._id = id;
    this.stage.layers.forEach(l => { const i = new Image(); i.src = 'img/stages/' + l.src.split('/').pop(); this.imgs[l.src] = i; });
  }
  get W() { const st = this.stage; return st ? Math.max(VW, st.w * (VH * .98 / st.h)) : VW; }

  /* ---- posições ---- */
  floorY(k) {      // k 0..1: 0 = linha do chão da arena; 1 = 4% da altura do cenário mais acima (os bonecos podem subir para fugir/perseguir)
    const st = this.stage, H = VH * .98, fl = st && st.floor != null ? st.floor : .85;
    return Math.round(0 + H * fl - k * .04 * H);
  }
  layoutIdle(init) {
    const W = this.W;
    for (let sd = 0; sd < 2; sd++) for (const c of this.ch[sd]) {
      const lo = sd ? .56 : .12, hi = sd ? .88 : .44;
      c.tx = W * (lo + Math.random() * (hi - lo)); c.ty = this.floorY(c.i / 3);
      if (init) { c.x = c.tx; c.y = c.ty; }
    }
  }

  /* ---- entrada local ---- */
  charFor(e) {      // o FM1 manda cada tecla no canal da faixa tocada (1..4): faixa 1 e 2 controlam os bonecos do jogador
    const mine = this.me.chars, i = e.ch % 4;
    return i < mine.length ? mine[i] : null;
  }
  localOff(e) {
    if (this.mode === 'pvp' && !this.host) { A.net.send({ t: 'off', n: e.n }, A.net.room.host); return; }
    this.noteOff(this.me.id, e.n);
  }
  localNote(e) {
    if (!this.alive) return;
    A.synth.note(e.n, e.v, e.ch, true);
    if (this.mode === 'pvp' && !this.host) { A.net.send({ t: 'in', n: e.n, v: e.v, ch: e.ch, tm: performance.now() }, A.net.room.host); return; }
    const ci = this.charFor(e); if (ci == null) return;
    this.hostNote(this.me.id, e.n, e.v, e.t, ci);
  }

  /* ---- host: pontuação ---- */
  hostNote(pid, n, v, tm, chIdx, ch) {
    const s = this.s; if (s.ph !== 'turn') return;
    const pl = this.players.find(p => p.id === pid) || { side: this.me.side, chars: this.me.chars };
    if (pl.side !== s.side) return;
    let ci = chIdx;
    if (ci == null) { const i = (ch || 0) % 4; if (i >= pl.chars.length) return; ci = pl.chars[i]; }
    this.held[pid + ':' + n] = { t0: performance.now(), side: pl.side, ci };      // segurar a tecla carrega o especial
    this.judge(pl.side, ci, n, v, tm || performance.now());
  }
  noteOff(pid, n) { delete this.held[pid + ':' + n]; }
  special(side, ci) {          // especial: tecla segurada por 1,5 s com a barra cheia
    const s = this.s; if (s.ult[side] < 100 || s.ph !== 'turn') return;
    s.ult[side] = 0; const en = 1 - side; s.hp[en] = Math.max(0, +(s.hp[en] - 14).toFixed(2)); s.ult[en] = Math.min(100, s.ult[en] + 14 * 2.2);
    const ev = { k: 'hit', sd: side, ci, kind: 2, dmg: 14, q: 'perfect', ult: true, combo: false, n: 60, v: 127 };
    this.onEv(ev); A.net.ch && this.mode === 'pvp' && A.net.send({ t: 'ev', e: ev });
    if (s.hp[en] <= 0) this.phase('end', 0);
  }
  judge(side, ci, n, v, tm, scale) {
    const s = this.s, beat = 60000 / s.bpm, g = beat / 2;
    const ph = (((tm - this.turnT0) % g) + g) % g, e = Math.min(ph, g - ph);
    const q = e <= 35 ? 'perfect' : e <= 70 ? 'good' : e <= 110 ? 'ok' : 'off';
    const mq = { perfect: 1, good: .72, ok: .42, off: .2 }[q];
    const H = this.hist[side]; H.push({ n, t: tm }); if (H.length > 24) H.shift();
    const last = H.slice(-8), pcs = new Set(last.map(h => h.n % 12));
    let vr = .5 + .5 * Math.min(1, (pcs.size - 1) / 3);
    const rate = H.filter(h => tm - h.t < 1000).length; if (rate > 6) vr *= Math.pow(6 / rate, 1.2);
    let harm = 1; if (last.length >= 5) for (let r = 0; r < 12; r++) { if ([...pcs].every(p => MAJ.includes((p - r + 12) % 12))) { harm = 1.15; break; } }
    let intv = 1; if (H.length > 1) { const d = Math.abs(n - H[H.length - 2].n) % 12; if ([3, 4, 5, 7, 8, 9, 0].includes(d) && n !== H[H.length - 2].n) intv = 1.08; }
    let loop = 1; if (H.length >= 8) { const a = H.slice(-4).map(h => h.n).join(), b = H.slice(-8, -4).map(h => h.n).join(); if (a === b) loop = .5; }
    // golpe: soco/chute × fraco/médio/forte
    const str = v < 60 ? 0 : v < 100 ? 1 : 2, kind = ((this.seq++ + n) % 2) * 3 + str;
    const K = this.kinds[side]; K.push({ k: kind, t: tm }); while (K.length && tm - K[0].t > 1600) K.shift();
    const combo = new Set(K.map(x => x.k)).size >= 3 && K.length >= 3;
    let dmg = BASE * mq * vr * harm * intv * loop * (.8 + v / 127 * .4) * (combo ? 1.25 : 1) * (scale || 1);
    const ult = false;
    const en = 1 - side; s.hp[en] = Math.max(0, +(s.hp[en] - dmg).toFixed(2));
    s.ult[en] = Math.min(100, s.ult[en] + dmg * 2.2);
    s.combo[side] = combo ? (s.combo[side] || 0) + 1 : 0;
    const ev = { k: 'hit', sd: side, ci, kind, dmg: +dmg.toFixed(1), q, ult, combo, n, v, loop: loop < 1, vr: +vr.toFixed(2) };
    this.onEv(ev); A.net.ch && this.mode === 'pvp' && A.net.send({ t: 'ev', e: ev });
    if (s.hp[en] <= 0) this.phase('end', 0);
  }
  onEv(e) {
    if (e.k === 'note') { A.synth.note(e.n, e.v, e.ch, false); return; }
    if (e.k !== 'hit') return;
    if (this.mode === 'pvp' && !this.host && e.sd === this.me.side) { /* sinal visual local */ }
    this.attack(e);
    if (e.sd !== this.me.side || this.mode === 'cpu') { }
    A.synth.hit(e.ult);
    const rem = this.mode === 'pvp' && e.sd !== this.me.side && this.me.side != null;
    if (rem && !A.synth.remoteReal) A.synth.note(e.n, e.v, e.ci, false);
    this.fx.push({ k: 'txt', x: 0, y: 0, sd: 1 - e.sd, t: 0, text: (e.ult ? 'ULT ' : '') + '-' + e.dmg, c: e.ult ? '#ff4' : e.q === 'perfect' ? '#8f8' : '#fff' });
    if (e.combo) this.fx.push({ k: 'txt', x: 0, y: 0, sd: e.sd, t: 0, text: 'COMBO x' + (this.s.combo[e.sd] || 1), c: '#f0c860', up: 28 });
    this.lastQ = e.q;
  }

  /* ---- fases (host) ---- */
  phase(ph, dur) { this.s.ph = ph; this.s.tl = dur; this.phT0 = performance.now(); this.phD = dur; }
  start(opts) {
    const s = this.s; s.hp = [100, 100]; s.ult = [0, 0]; s.turn = 0; s.combo = [0, 0];
    s.dice = [1 + A.rnd(6), 1 + A.rnd(6)]; s.first = (s.dice[0] + s.dice[1]) % 2 === 0 ? 0 : 1;
    this.phase('dice', 3);
    this.rec && this.rec();
  }
  stepHost(now) {
    const s = this.s; if (!this.host || s.ph === 'wait' || s.ph === 'end') return;
    const el = (now - this.phT0) / 1000; s.tl = Math.max(0, this.phD - el);
    const beat = 60 / s.bpm;
    if (s.ph === 'count') { const k = Math.floor(el / beat); if (k !== this.ticked && k < 4) { this.ticked = k; A.synth.tick(k === 0); } }
    if (s.ph === 'turn') { const k = Math.floor((now - this.turnT0) / (beat * 1000)); if (k !== this.ticked) { this.ticked = k; A.synth.tick(false); } }
    if (el >= this.phD) {
      if (s.ph === 'dice') { s.side = s.first; this.phase('prep', 10); this.layoutIdle(); }
      else if (s.ph === 'prep') { s.turn = 0; s.side = s.first; this.phase('count', beat * 4); this.ticked = -1; }
      else if (s.ph === 'count') { this.turnT0 = now; this.ticked = -1; this.phase('turn', 25); this.cpuT = now; this.cpuK = -1; }
      else if (s.ph === 'turn') {
        s.turn++; if (s.turn >= 4) { this.phase('end', 0); }
        else { s.side = s.first ^ (s.turn & 1); this.phase('count', beat * 4); this.ticked = -1; this.retreat(1 - s.side); }
      }
    }
    // total restante
    const per = beat * 4 + 25;
    const rest = s.ph === 'dice' ? 10 + 4 * per : s.ph === 'prep' ? s.tl + 4 * per : s.ph === 'count' ? s.tl + 25 + (3 - s.turn) * per : s.ph === 'turn' ? s.tl + (3 - s.turn) * per : 0;
    s.total = Math.max(0, rest);
    if (s.ph === 'turn') {
      const cps = this.players.filter(p => p.cpu && p.side === s.side);        // CPU do adversário, ou jogador que saiu
      if (s.ult[s.side] >= 100) {
        for (const k in this.held) { const h = this.held[k]; if (h.side === s.side && now - h.t0 >= 1500) { delete this.held[k]; this.special(s.side, h.ci); break; } }
        if (cps.length && !this.cpuHold) this.cpuHold = now + 1700 + A.rnd(900);
        if (this.cpuHold && now > this.cpuHold && cps.length) { this.cpuHold = 0; this.special(s.side, cps[0].chars[A.rnd(cps[0].chars.length)]); }
      } else this.cpuHold = 0;
      if (cps.length) this.cpuPlay(now, s.side, cps);
    } else this.cpuHold = 0;
    if (s.ph === 'end' && !this.ended) { this.ended = true; this.o.onEnd && this.o.onEnd(this.s); }
    if (this.mode === 'pvp' && now - (this.lastSend || 0) > 110) { this.lastSend = now; A.net.send({ t: 'st', s: this.s }); }
  }
  /* CPU: toca uma pequena banda (kick/caixa/chimbal techno, baixo acid, acordes dub) e ataca em parte das notas.
     Fica mais fraca depois de tirar 35% da vida do jogador. O CPU de quem saiu luta, mas em silêncio. */
  cpuPlay(now, side, cps) {
    const s = this.s, step = 60000 / s.bpm / 4, k = Math.floor((now - this.turnT0) / step);
    if (k === this.cpuK || k < 0) return; this.cpuK = k;
    const bar = Math.floor(k / 16), st = k % 16, loud = !cps.every(p => p.gone);
    const ROOT = [110, 110, 98, 82.4][bar % 4];                                   // A, A, G, E: o baixo anda a cada compasso
    const ACID = [0, null, 3, null, 7, null, 3, 5, 0, null, 10, null, 7, null, 3, null];
    const eased = s.hp[1 - side] <= 65;                                           // já tirou 35% da vida do inimigo: alivia ainda mais
    const prob = eased ? .28 : .6, jit = eased ? 130 : 80;
    if (loud) {
      if (st % 4 === 0) A.synth.kick(1);
      if (st === 4 || st === 12) A.synth.snare();
      if (st % 2 === 0) A.synth.hat(st % 4 === 2, 1); else if (Math.random() < .5) A.synth.hat(false, .5);
      if (ACID[st] != null) A.synth.acid(ROOT * Math.pow(2, ACID[st] / 12), st % 8 === 0 || st === 10, .17);
      if (st === 3 || st === 10) A.synth.stab(ROOT * 2, bar % 2 ? [0, 3, 7] : [0, 3, 10]);
    }
    // ataques: nas notas do baixo e nos acordes (pouca chance; o jogador é quem manda na luta)
    if ((ACID[st] != null || st === 3 || st === 10) && Math.random() < prob) {
      const p = cps[A.rnd(cps.length)], n = 45 + (ACID[st] || 0), v = 50 + A.rnd(45);
      this.judge(side, p.chars[A.rnd(p.chars.length)], n, v, now + (Math.random() - .5) * jit, .9);
    }
  }
  markGone(pid) {              // o jogador saiu no meio da luta: um CPU assume os personagens dele
    const p = this.players.find(x => x.id === pid); if (!p || p.cpu) return;
    p.cpu = true; p.gone = true; this.refreshGone();
  }
  markBack(pid) {              // voltou: reassume os personagens
    const p = this.players.find(x => x.id === pid); if (!p) return;
    p.cpu = false; p.gone = false; this.refreshGone();
  }
  refreshGone() { this.s.gone = this.players.filter(p => p.gone).map(p => ({ side: p.side, chars: p.chars })); }

  /* ---- animação ---- */
  retreat(sd) {
    const W = this.W; for (const c of this.ch[sd]) { const lo = sd ? .5 : .1, hi = sd ? .9 : .5;
      c.tx = W * (lo + (hi - lo) * Math.random() * .8 + (sd ? 0 : .1)); c.ty = this.floorY(c.i / 3); c.queue = null; }
  }
  attack(e) {
    const c = this.ch[e.sd][e.ci % 4]; if (c.st === 'launch' || c.st === 'down') return;
    const tgt = this.ch[1 - e.sd][A.rnd(4)];
    c.target = tgt; c.kind = e.kind; c.ult = e.ult; c.state = 'go'; c.busy = 1; c.st = 'run';
  }
  hurt(t, ult, from) {
    if (ult) { t.st = 'launch'; t.t = 0; t.z = 0; t.vx = (from.x < t.x ? 1 : -1) * 190; t.vy = -150; this.shake = .7; this.flash = .15; A.synth.hit(true); }
    else if (t.st !== 'launch' && t.st !== 'down') { t.st = 'hurt'; t.t = 0; t.rec = .28; }
  }
  update(dt) {
    const s = this.s, W = this.W, act = s.ph === 'turn' ? s.side : -1;
    for (let sd = 0; sd < 2; sd++) for (const c of this.ch[sd]) {
      c.t += dt;
      if (c.st === 'launch') {
        c.vy += 520 * dt; c.x += c.vx * dt; c.z += c.vy * dt;
        if (c.x < 6) { c.x = 6; c.vx = 0; if (!c.hitWall) { c.hitWall = 1; this.shake = .9; A.synth.hit(true); } }
        if (c.x > W - 6) { c.x = W - 6; c.vx = 0; if (!c.hitWall) { c.hitWall = 1; this.shake = .9; A.synth.hit(true); } }
        if (c.z >= 0) { c.z = 0; c.st = 'down'; c.t = 0; c.hitWall = 0; }
        continue;
      }
      if (c.st === 'down') { if (c.t > .8) { c.st = 'idle'; c.t = 0; } continue; }
      if (c.st === 'hurt') { if (c.t > c.rec) { c.st = 'idle'; } continue; }
      if (c.busy) {
        const tg = c.target; if (!tg) { c.busy = 0; continue; }
        if (c.state === 'go') {
          const gx = tg.x + (c.x < tg.x ? -16 : 16), dx = gx - c.x; c.face = dx >= 0 ? 1 : -1; c.ty = tg.y;
          const sp = 190 * dt; if (Math.abs(dx) <= sp) { c.x = gx; c.state = 'hit'; c.st = (c.kind < 3 ? 'punch' : 'kick') + (c.kind % 3); c.t = 0; c.hitDone = 0; } else c.x += Math.sign(dx) * sp;
          c.y += Math.sign(c.ty - c.y) * Math.min(Math.abs(c.ty - c.y), 60 * dt);
        } else if (c.state === 'hit') {
          c.face = tg.x >= c.x ? 1 : -1;
          if (!c.hitDone && c.t > .1) { c.hitDone = 1; this.hurt(tg, c.ult, c); }
          if (c.t > .3) { c.state = 'stay'; c.st = 'idle'; c.t = 0; }
        } else if (c.state === 'stay') { if (c.t > .22) { c.busy = 0; c.ult = 0; } }
        continue;
      }
      // livre: anda
      const free = act === sd;
      if (free && (c.tx == null || Math.abs(c.tx - c.x) < 3) && Math.random() < dt * 1.2) { const lo = sd ? .52 : .08, hi = sd ? .92 : .48; c.tx = W * (lo + (hi - lo) * Math.random()); c.ty = this.floorY(Math.random()); }
      if (c.tx != null) {
        const dx = c.tx - c.x; const sp = (free ? 36 : 70) * dt;
        if (Math.abs(dx) > 1) { c.x += Math.sign(dx) * Math.min(Math.abs(dx), sp); c.face = dx > 0 ? 1 : -1; c.st = free ? 'walk' : 'run'; }
        else c.st = 'idle';
        c.y += Math.sign(c.ty - c.y) * Math.min(Math.abs(c.ty - c.y), 30 * dt);
        if (!free && Math.abs(dx) <= 1) c.face = sd ? -1 : 1;
      }
    }
    // câmera segue o meio da ação
    let mx = 0, n = 0; this.ch.forEach(g => g.forEach(c => { if (c.busy) { mx += c.x; n++; } }));
    const target = n ? mx / n : W / 2; this.camT = Math.max(0, Math.min(W - VW, target - VW / 2));
    this.cam += (this.camT - this.cam) * Math.min(1, dt * 3);
    this.shake = Math.max(0, this.shake - dt); this.flash = Math.max(0, this.flash - dt);
    this.fx.forEach(f => f.t += dt); this.fx = this.fx.filter(f => f.t < 1.1);
  }
  loop(now) {
    if (!this.alive) return;
    const dt = Math.min(.05, (now - (this.lastT || now)) / 1000); this.lastT = now;
    this.stepHost(performance.now()); this.update(dt); this.draw(performance.now()); this.o.onFrame && this.o.onFrame(this);
    this.raf = requestAnimationFrame((t) => this.loop(t));
  }

  /* ---- render ---- */
  txt(c, s, x, y, col, size, al, title) {      // texto pixel com borda preta (sempre legível)
    c.font = `600 ${size}px "Pixelify Sans", ui-monospace, monospace`; c.textAlign = al || 'left'; c.lineJoin = 'round';
    c.lineWidth = Math.max(3, size / 3); c.strokeStyle = '#000'; c.strokeText(s, x, y); c.fillStyle = col; c.fillText(s, x, y);
  }
  ring(c, x, y, frac, label) {          // relógio circular em pixels: disco escuro com borda preta e 24 quadradinhos em volta
    const N = 24, R = 15, disc = (r, col) => { c.fillStyle = col; for (let dy = -r; dy <= r; dy += 2) { const w = Math.floor(Math.sqrt(r * r - dy * dy) / 2) * 2; c.fillRect(x - w, y + dy - 1, w * 2, 2); } };
    disc(R + 6, '#000'); disc(R + 2, '#3a2620'); disc(R - 4, '#1e1418');
    for (let i = 0; i < N; i++) {
      const a = -Math.PI / 2 + i * Math.PI * 2 / N, px = Math.round(x + Math.cos(a) * R), py = Math.round(y + Math.sin(a) * R);
      c.fillStyle = i / N < frac ? '#f0c860' : '#5a3a28'; c.fillRect(px - 2, py - 2, 4, 4);
    }
    this.txt(c, String(label), x, y + 6, '#fff', 16, 'center');
  }
  draw(now) {
    const c = this.cx, st = this.stage, s = this.s; c.setTransform(2, 0, 0, 2, 0, 0); c.imageSmoothingEnabled = false;
    c.fillStyle = '#14101a'; c.fillRect(0, 0, VW, VH);
    if (!st) return;
    const sc = (VH * .98) / st.h, sx = (this.shake ? (Math.random() - .5) * this.shake * 10 : 0), sy = (this.shake ? (Math.random() - .5) * this.shake * 6 : 0);
    c.save(); c.translate(Math.round(sx), Math.round(sy));
    const camX = this.cam, topY = 0;
    if (st.bg) { c.fillStyle = st.bg; c.fillRect(-12, -12, VW + 24, VH + 24); }
    st.layers.forEach(l => {                        // camadas de trás para a frente: céu/nuvens (sprites) antes do cenário
      const im = this.imgs[l.src]; if (!im || !im.naturalWidth) return;
      const w = l.w * sc, h = l.h * sc, y = topY + l.y * sc;
      c.globalAlpha = l.a;
      const off = camX * l.par + (l.spd || 0) * (now / 1000);
      if (l.loop) { let x = -(((off % w) + w) % w); for (; x < VW; x += w) c.drawImage(im, Math.round(x), Math.round(y), Math.ceil(w), Math.round(h)); }
      else c.drawImage(im, -Math.round(Math.min(camX * l.par, Math.max(0, w - VW))), Math.round(y), Math.round(w), Math.round(h));
      c.globalAlpha = 1;
    });
    const bottom = topY + st.h * sc; if (bottom < VH) { c.fillStyle = '#14101a'; c.fillRect(0, bottom, VW, VH - bottom); }
    // lutadores (ordem por profundidade)
    const all = [].concat(this.ch[0], this.ch[1]).sort((a, b) => a.y - b.y), ended = s.ph === 'end', win = s.hp[0] === s.hp[1] ? -1 : s.hp[0] > s.hp[1] ? 0 : 1;
    for (const ch of all) {
      const x = Math.round(ch.x - camX), y = Math.round(ch.y + (ch.z || 0));
      c.fillStyle = '#0006'; c.fillRect(x - 14, Math.round(ch.y) - 2, 28, 4);
      let anim = ch.st; if (ended) anim = win === ch.sd ? 'win' : win < 0 ? 'idle' : 'down';
      if (anim === 'hurt' && ch.t < .09) anim = 'flash';
      const fw = (s.fw && s.fw[ch.sd]) || 'other';
      const own = this.owner[ch.sd][ch.i], duel = (this.players || []).length <= 2;      // 1x1: cor da faixa; 2x2: cada jogador na sua cor
      const col = duel || own == null ? A.TRACKCOL[ch.i] : A.PLAYERCOL[own % 4];
      A.sprite.draw(c, (ch.sd ? 5 : 1) + ch.i, col, A.FWCOL[fw] || A.FWCOL.other, anim, ch.t, x, y, ch.face, 1.5);
      if (s.ph === 'turn' && s.side !== ch.sd && ch.i === 0) drawMute(c, x - 9, Math.round(this.floorY(0)) - 92);
    }
    // aviso: o jogador saiu (texto branco, fundo preto, borda vermelha arredondada em pixels)
    for (const gn of (s.gone || [])) {
      const c0 = this.ch[gn.side][gn.chars[0]]; if (!c0) continue;
      const lab = A.t('left'); c.font = '600 11px "Pixelify Sans",ui-monospace,monospace'; const w = Math.ceil(c.measureText(lab).width) + 14, h = 20;
      const cx_ = Math.round(gn.chars.reduce((a, i) => a + this.ch[gn.side][i].x, 0) / gn.chars.length - camX), by = Math.round(c0.y) - 92, x0 = Math.max(2, Math.min(VW - w - 2, cx_ - w / 2));
      const R = (x, y, ww, hh, col) => { c.fillStyle = col; c.fillRect(Math.round(x), Math.round(y), ww, hh); };
      R(x0 + 2, by, w - 4, h, '#e0503c'); R(x0, by + 2, w, h - 4, '#e0503c'); R(x0 + 3, by + 2, w - 6, h - 4, '#000'); R(x0 + 2, by + 3, w - 4, h - 6, '#000');
      R(cx_ - 3, by + h, 6, 3, '#e0503c'); R(cx_ - 1, by + h, 2, 2, '#000');
      c.textAlign = 'center'; c.fillStyle = '#fff'; c.fillText(lab, x0 + w / 2, by + 14); c.textAlign = 'left';
    }
    for (const f of this.fx) {
      if (f.k === 'txt') this.txt(c, f.text, (f.sd ? .75 : .25) * VW, 74 + (f.up || 0) - f.t * 22, f.c, 12, 'center');
    }
    c.restore();
    if (this.flash) { c.fillStyle = `rgba(255,255,255,${this.flash * 3})`; c.fillRect(0, 0, VW, VH); }
    if (s.ph === 'turn' && s.ult[s.side] >= 100) {
      const hh = Object.values(this.held).filter(h => h.side === s.side).map(h => (performance.now() - h.t0) / 1500).sort((a, b) => b - a)[0] || 0;
      const x0 = (s.side ? .75 : .25) * VW - 30; c.fillStyle = '#000'; c.fillRect(x0 - 2, 70, 64, 10); c.fillStyle = '#5a3a28'; c.fillRect(x0, 72, 60, 6);
      c.fillStyle = '#f0c860'; c.fillRect(x0, 72, Math.round(60 * Math.min(1, hh)), 6);
      this.txt(c, A.t('g_ult'), (s.side ? .75 : .25) * VW, 66, '#f0c860', 10, 'center');
    }
    // relógio circular sobre o lado que toca + mão apontando
    if (s.ph === 'prep' || s.ph === 'count' || s.ph === 'turn') {
      const dur = s.ph === 'turn' ? 25 : s.ph === 'prep' ? 10 : 60 / s.bpm * 4;
      const lab = s.ph === 'count' ? Math.max(1, 4 - Math.floor((1 - s.tl / dur) * 4)) : Math.ceil(s.tl);
      this.ring(c, (s.side ? .75 : .25) * VW, 34, Math.max(0, s.tl / dur), lab);
      drawHand(c, Math.round(VW / 2 - 10 + (s.side ? 30 : -30) + Math.sin(now / 160) * 3 * (s.side ? 1 : -1)), 20, s.side ? 1 : -1);
      if (s.ph === 'prep') this.txt(c, A.t('g_prep'), VW / 2, 120, '#f0c860', 22, 'center');
      if (s.ph === 'count') this.txt(c, A.t('g_next', { s: A.t(s.side ? 'side_b' : 'side_a') }), VW / 2, 120, '#fff', 16, 'center');
    }
    if (s.ph === 'dice') {
      const el = 3 - s.tl, roll = el < 2, fall = Math.min(1, el / .6), y = -30 + 90 * fall + (fall >= 1 ? Math.abs(Math.sin(el * 9)) * -8 / (1 + el * 2) : 0);
      const a = roll ? 1 + Math.floor(now / 70 % 6) : s.dice[0], b = roll ? 1 + Math.floor((now / 90 + 3) % 6) : s.dice[1];
      drawDie(c, VW / 2 - 40, y, a, 30); drawDie(c, VW / 2 + 10, y + 4, b, 30);
      if (!roll) {
        const sum = s.dice[0] + s.dice[1];
        this.txt(c, `${s.dice[0]} + ${s.dice[1]} = ${sum}`, VW / 2, 126, '#f0c860', 18, 'center');
        this.txt(c, A.t(sum % 2 === 0 ? 'g_even' : 'g_odd'), VW / 2, 148, '#fff', 16, 'center');
      }
    }
    if (s.ph === 'end') { const t = s.hp[0] === s.hp[1] ? A.t('r_draw') : A.t('g_wins', { s: A.t(win === 0 ? 'side_a' : 'side_b') }); this.txt(c, t, VW / 2, 110, '#f0c860', 26, 'center'); }
  }
}
A.Game = Game; A.KIND = KIND; A.COL = COL; A.drawChar = drawChar;

/* ============ treino: barras caindo sobre as teclas numeradas do FM1 + espantalho ============ */
const PCW = [0, 2, 4, 5, 7, 9, 11];                       // C D E F G A B (classes de altura das teclas brancas)
const KB0 = 53;                                           // tecla 1 do FM1 = F (nota 53); são 27 teclas: 16 brancas + 11 pretas
function kbLayout() {
  const keys = []; let w = 0;
  for (let k = 0; k < 27; k++) {
    const pc = (KB0 + k) % 12, white = PCW.includes(pc);
    if (white) { keys.push({ k, white: 1, x: w * 24, cx: w * 24 + 12 }); w++; }
    else keys.push({ k, white: 0, x: 0, cx: 0 });
  }
  keys.forEach((e, i) => { if (!e.white) { const prev = keys[i - 1]; e.cx = prev.x + 24; e.x = e.cx - 7; } });
  return keys;
}
class Training {
  constructor(o) {
    this.cv = o.canvas; this.cx = this.cv.getContext('2d'); this.cv.width = VW * 2; this.cv.height = VH * 2; this.cx.imageSmoothingEnabled = false;
    this.o = o; this.bars = []; this.hits = 0; this.miss = 0; this.dmg = 0; this.flash = 0; this.shake = 0; this.txt = []; this.speed = 80;
    this.keys = kbLayout(); this.lit = {};
    // sequência em números de tecla (1..27): sobe e desce pela escala, com pretas no meio
    this.pat = [8, 10, 12, 13, 15, 17, 19, 20, 19, 17, 15, 13, 12, 10, 8, 9, 11, 14, 16, 18, 16, 14, 11, 9].map(n => n - 1); this.pi = 0;
    this.next = performance.now() + 800; this.bpm = 96; this.dummyT = 0; this.alive = true; this.combo = 0;
    this.un = A.on('note', (e) => e.on && this.press(e));
    this.raf = requestAnimationFrame((t) => this.loop(t)); this.last = performance.now();
  }
  destroy() { this.alive = false; cancelAnimationFrame(this.raf); this.un(); }
  press(e) {
    A.synth.note(e.n, e.v, e.ch, true);
    const pc = (((e.n - KB0) % 12) + 12) % 12, cand = [pc, pc + 12, pc + 24].filter(k => k < 27), hitY = VH - 52;
    cand.forEach(k => { this.lit[k] = performance.now(); });
    const c = this.bars.filter(b => cand.includes(b.k) && !b.done).sort((a, b) => Math.abs(a.y - hitY) - Math.abs(b.y - hitY))[0];
    if (c && Math.abs(c.y - hitY) < 22) {
      c.done = 1; const perfect = Math.abs(c.y - hitY) < 8; this.hits++; this.combo++;
      const d = Math.round((perfect ? 6 : 3) * (1 + Math.min(this.combo, 10) * .05)); this.dmg += d; this.flash = .12; this.shake = .25; this.dummyT = 0.35;
      this.txt.push({ t: 0, s: (perfect ? 'PERFEITO -' : 'BOM -') + d, c: perfect ? '#8f8' : '#fff', x: VW * .72 }); A.synth.hit(false);
    } else { this.miss++; this.combo = 0; this.txt.push({ t: 0, s: 'MISS', c: '#f66', x: 20 + cand[0] * 13 }); }
  }
  loop(now) {
    if (!this.alive) return; const dt = Math.min(.05, (now - this.last) / 1000); this.last = now;
    if (now > this.next) { this.bars.push({ k: this.pat[this.pi++ % this.pat.length], y: -10 }); this.next = now + 60000 / this.bpm / (this.pi % 8 === 7 ? 1 : 2); }
    const bottom = VH - 40;
    for (const b of this.bars) { b.y += this.speed * dt; if (!b.done && b.y > bottom) { b.done = 1; this.miss++; this.combo = 0; this.txt.push({ t: 0, s: 'MISS', c: '#f66', x: 20 + b.k * 13 }); } }
    this.bars = this.bars.filter(b => b.y < VH + 10 && !(b.done && b.y > bottom - 10)); this.txt.forEach(t => t.t += dt); this.txt = this.txt.filter(t => t.t < .8);
    this.flash = Math.max(0, this.flash - dt); this.shake = Math.max(0, this.shake - dt); this.dummyT = Math.max(0, this.dummyT - dt);
    this.draw(now); this.o.onFrame && this.o.onFrame(this); this.raf = requestAnimationFrame((t) => this.loop(t));
  }
  draw(now) {
    const c = this.cx; c.setTransform(2, 0, 0, 2, 0, 0); c.imageSmoothingEnabled = false; c.fillStyle = '#1e1418'; c.fillRect(0, 0, VW, VH);
    const T = (s, x, y, col, sz, al) => { c.font = `600 ${sz}px "Pixelify Sans",monospace`; c.textAlign = al || 'left'; c.lineWidth = 4; c.strokeStyle = '#000'; c.lineJoin = 'round'; c.strokeText(s, x, y); c.fillStyle = col; c.fillText(s, x, y); };
    c.fillStyle = '#2a1f26'; c.fillRect(0, VH * .62, VW, VH * .38);
    // espantalho
    const dx = VW * .78 + (this.dummyT ? Math.sin(now / 24) * 3 : 0), dy = VH * .62;
    c.fillStyle = '#000'; c.fillRect(dx - 1, dy - 62, 4, 64); c.fillRect(dx - 24, dy - 50, 50, 5);
    c.fillStyle = '#a06a30'; c.fillRect(dx, dy - 61, 2, 62); c.fillRect(dx - 23, dy - 49, 48, 3);
    c.fillStyle = '#000'; c.fillRect(dx - 9, dy - 74, 20, 20); c.fillStyle = this.flash ? '#fff' : '#e8c070'; c.fillRect(dx - 8, dy - 73, 18, 18);
    c.fillStyle = '#000'; c.fillRect(dx - 4, dy - 68, 3, 3); c.fillRect(dx + 3, dy - 68, 3, 3); c.fillRect(dx - 4, dy - 61, 10, 2);
    c.fillStyle = '#000'; c.fillRect(dx - 13, dy - 30, 28, 4); c.fillStyle = '#c0504a'; c.fillRect(dx - 12, dy - 29, 26, 2);
    // teclado do FM1 com o número de cada tecla (1..27); as barras caem sobre a tecla
    const ky = VH - 40, nowt = performance.now();
    for (const e of this.keys) {                       // pistas
      c.fillStyle = e.white ? '#ffffff0c' : '#ffffff18'; c.fillRect(e.cx - (e.white ? 12 : 7), 0, e.white ? 24 : 14, ky);
    }
    c.fillStyle = '#f0c860'; c.fillRect(0, ky - 2, VW, 2);
    for (const b of this.bars) {
      if (b.done) continue; const e = this.keys[b.k], w = e.white ? 18 : 12;
      c.fillStyle = '#000'; c.fillRect(e.cx - w / 2 - 1, b.y - 1, w + 2, 14); c.fillStyle = COL[b.k % 4][0]; c.fillRect(e.cx - w / 2, b.y, w, 12);
      c.font = '600 9px "Pixelify Sans",monospace'; c.textAlign = 'center'; c.fillStyle = '#1e1418'; c.fillText(String(b.k + 1), e.cx, b.y + 10);
    }
    for (const e of this.keys) if (e.white) {
      const on = nowt - (this.lit[e.k] || -1e9) < 160;
      c.fillStyle = '#000'; c.fillRect(e.x, ky, 24, 40); c.fillStyle = on ? '#f0c860' : '#f2e2c4'; c.fillRect(e.x + 1, ky + 1, 22, 38);
      c.font = '600 10px "Pixelify Sans",monospace'; c.textAlign = 'center'; c.fillStyle = '#1e1418'; c.fillText(String(e.k + 1), e.cx, ky + 35);
    }
    for (const e of this.keys) if (!e.white) {
      const on = nowt - (this.lit[e.k] || -1e9) < 160;
      c.fillStyle = '#000'; c.fillRect(e.x - 1, ky, 16, 26); c.fillStyle = on ? '#f0c860' : '#3a2620'; c.fillRect(e.x, ky, 14, 24);
      c.font = '600 8px "Pixelify Sans",monospace'; c.textAlign = 'center'; c.fillStyle = on ? '#1e1418' : '#f2e2c4'; c.fillText(String(e.k + 1), e.cx, ky + 18);
    }
    for (const t of this.txt) T(t.s, t.x, VH * .4 - t.t * 30, t.c, 12, 'center');
    T(`${A.t('tr_hits')} ${this.hits}   ${A.t('tr_miss')} ${this.miss}   ${A.t('tr_dmg')} ${this.dmg}   COMBO ${this.combo}`, 8, 14, '#fff', 12);
    T(A.t('tr_keys'), VW - 8, 14, '#f0c860', 10, 'right');
  }
}
A.Training = Training;
})();
