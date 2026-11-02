// FM1 Quest lab — Formation battle (same camera as Formation) + RPG skins for the stock pages (ENV, LFO, FX, SCL, roll).
(function (G) {
'use strict';
const T = G.GFX, Sc = G.Scene, PAL = Sc.PALS['1c'], { add, UI, TAU, clamp, lerp, fr, ease, hs, shade, floor7, outBack } = T, fig = Sc.fig, pct = v => Math.round(v * 100);
const CLS = ['mage', 'archer', 'cleric', 'warrior'], MON = Sc.MON['1c'], B4 = [0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5];
const scaled = (g, ox, oy, s) => (x, y, w, h, i) => g.R(ox + x * s, oy + y * s, w * s, h * s, i);

// ------------------------------------------------------------------ Formation · Battle
const PATS = ['1011010110110110', '0000001000000100', '1000000000000000', '1000100010001000'];
add({ name: 'Formation', tag: 'o grupo em formação: a posição de cada herói é o volume da faixa', tier: 't1', an: [240, 150],
  how: ['tap:EDIT (várias vezes) = Equip · Status · Character view · Formation · Party · Song Select', 'turn:KNOB 1–4 = posição do herói 1–4 (frente ↔ trás)'],
  fields: [1, 2, 3, 4].map((n, i) => ({ l: 'herói ' + n, d: 'posição do herói ' + n + ': do meio para a frente o volume da faixa é 100%; do meio para trás ele cai até 0, e a faixa fica muda com o herói descansando atrás do grupo.', v: .75, fmt: v => { const l = v < .5 ? v / .5 : 1; return l <= .02 ? 'mudo' : Math.round(l * 100) + '%'; } })),
  desc: 'O grupo no palco isométrico, batendo nos monstros sozinho: cada nota da faixa de um herói vira um ataque. A posição de cada herói decide o volume da faixa dele: do meio para a frente é 100%; do meio para trás o volume cai até 0. Em 0 a faixa fica muda e o herói senta e descansa atrás do grupo, sem atacar. Os golpes ficam mais fracos conforme o volume baixa.',
  bullets: ['Knob 1–4 = posição do herói 1–4: frente = 100%, meio = 100%, fundo = 0% (mudo)', 'Mudo = o herói fica sentado atrás do grupo e não ataca', 'O volume vale na música de verdade (mix das faixas)', 'Alvo = monstro mais próximo vivo; monstro derrotado reaparece depois de 1,5 s'],
  draw(g, t, k, st) {
    const E = G.Engine, bpm = (E && E.S.bpm) || 120, ne = 3, lv = 30, th = -.62, c = Math.cos(th), s = Math.sin(th), wid = 1, LV = [0, 1, 2, 3].map(h => k[h] < .5 ? k[h] / .5 : 1), RESTS = LV.map(v => v <= .02);
    if (E) { st.base = st.base || E.S.tracks.map(x => x.level); let ch = false; E.S.tracks.forEach((x, h) => { const nl = st.base[h] * LV[h], mu = RESTS[h]; if (Math.abs(x.level - nl) > .004) { x.level = nl; ch = true; } if (!!x.mute !== mu && (mu || st.muted && st.muted[h])) { x.mute = mu; ch = true; } }); st.muted = RESTS.slice(); if (ch) E.applyMix(); }                    // the position is the real level of the track
    const P = (x, z) => [120 + (x * c - z * s) * 27, 96 + (x * s + z * c) * 13];
    g.clear(0); g.sky(19, 197, [0, 0, 1, 0, 1]);
    for (let gx = -4; gx < 4; gx++) for (let gz = -3; gz < 3; gz++) g.poly([P(gx, gz), P(gx + 1, gz), P(gx + 1, gz + 1), P(gx, gz + 1)], (gx + gz) & 1 ? 4 : 5);
    const co = [P(-4, -3), P(4, -3), P(4, 3), P(-4, 3)]; for (let q = 0; q < 6; q++) g.poly([co[2], co[3], [co[3][0], co[3][1] + q], [co[2][0], co[2][1] + q]], q === 5 ? 0 : 4); g.polyO(co, 10);
    const SLOTZ = [-1.6, .9, -.4, 1.8], HP = [0, 1, 2, 3].map(h => ({ x: lerp(-4.7, -1.0, k[h]), z: SLOTZ[h] * wid })), EP = [{ x: 2.2, z: -1.4, k: 'slime' }, { x: 2.9, z: .8, k: 'bat' }, { x: 3.3, z: -.2, k: 'slime' }, { x: 2.0, z: 1.6, k: 'king' }].slice(0, ne).map(p => ({ ...p, z: p.z * wid }));
    st.e = st.e || []; EP.forEach((p, i) => { st.e[i] = st.e[i] || { hp: 1, dead: -9, hurt: -9 }; });
    st.n = st.n == null ? Math.floor(t * bpm / 15) : st.n; st.pr = st.pr || []; st.at = st.at || [-9, -9, -9, -9]; st.dm = st.dm || [];
    const nn = Math.floor(t * bpm / 15);
    while (st.n < nn) { st.n++; for (let h = 0; h < 4; h++) if (!RESTS[h] && ((E && E.patString ? E.patString(h) : PATS[h]).padEnd(16, '0'))[st.n % 16] === '1') { const alive = EP.map((p, i) => ({ i, d: Math.hypot(p.x - HP[h].x, p.z - HP[h].z), a: t - st.e[i].dead > 1.5 || st.e[i].hp > 0 })).filter(q => q.a && st.e[q.i].hp > 0).sort((a, b) => a.d - b.d); st.at[h] = t; if (alive.length) st.pr.push({ h, to: alive[0].i, t0: t }); } }
    st.pr = st.pr.filter(p => { const u = (t - p.t0) / .32; if (u >= 1) { const e = st.e[p.to]; if (e && e.hp > 0) { e.hp -= (.06 + lv * .0016 + hs(p.t0) * .04) * (.35 + .65 * LV[p.h]); e.hurt = t; st.dm.push({ i: p.to, v: Math.round((10 + hs(p.t0 * 7) * 60) * (1 + lv / 25)), t0: t, c: p.h }); if (e.hp <= 0) e.dead = t; } return false; } return true; });
    EP.forEach((p, i) => { const e = st.e[i]; if (e.hp <= 0 && t - e.dead > 1.6) { e.hp = 1; e.dead = -9; } }); st.dm = st.dm.filter(d => t - d.t0 < .9);
    const items = [];
    HP.forEach((p, i) => items.push({ y: P(p.x, p.z)[1], hero: i, p })); EP.forEach((p, i) => items.push({ y: P(p.x, p.z)[1], en: i, p })); items.sort((a, b) => a.y - b.y);
    for (const it of items) {
      const [sx, sy] = P(it.p.x, it.p.z);
      if (it.hero != null) { const h = it.hero, u = t - st.at[h], atk = u < .35, o = { s: 2, t, p: atk ? lerp(-.3, 1, u / .35) : null, sit: RESTS[h] }; let ox = 0; if (h === 3 && atk) ox = Math.round(Math.sin(u / .35 * Math.PI) * 26); g.ell(sx, sy + 1, 14, 5, 0); if (lv > 50) g.ellO(sx, sy + 1, 16 + Math.sin(t * 4) * 1.5, 6, 10); fig(g.R, CLS[h], sx - 17 + ox, sy - 46 + (RESTS[h] ? 8 : Math.round(Math.sin(t * 3 + h) * 1)), o); g.R(sx - 3, sy + 4, 7, 7, UI.t[h]); g.txt(h + 1, sx, sy + 4, 0, { al: 'c' }); if (RESTS[h]) { const zz = Math.floor(t * 1.5) % 3; g.txt('z' + (zz > 0 ? 'z' : '') + (zz > 1 ? 'Z' : ''), sx + 8, sy - 38 - zz * 3, UI.dim, { sz: 8 }); } }
      else {
        const i = it.en, e = st.e[i], kind = it.p.k, dead = e.hp <= 0, dp = dead ? clamp((t - e.dead) / .9, 0, 1) : 0, hurt = t - e.hurt < .15, solid = hurt ? 7 : null, skip = dp > 0 ? (a, b) => (B4[(b & 3) * 4 + (a & 3)] + .5) / 16 < dp : null, hop = Math.abs(Math.sin(t * 3 + i * 2)) * 6;
        g.ell(sx, sy + 1, kind === 'king' ? 24 : 12, 4, 0);
        if (dp < 1) { if (kind === 'slime') Sc.spr(scaled(g, sx - 12, sy - 20 - hop, 2), Sc.SLIME, 0, 0, MON.slime, { s: 1, solid, skip }); else if (kind === 'bat') Sc.spr(scaled(g, sx - 12, sy - 40 - Math.sin(t * 3 + i) * 5, 2), (Math.floor(t * 7) & 1) ? Sc.BATA : Sc.BATB, 0, 0, MON.bat, { s: 1, solid, skip }); else Sc.slimeKing(scaled(g, sx - 18, sy - 32, .75), 0, 0, MON, solid); if (!dead) { g.R(sx - 10, sy + 5, 20, 3, 0); g.R(sx - 9, sy + 6, 18 * clamp(e.hp, 0, 1), 1, e.hp > .5 ? 11 : e.hp > .25 ? 10 : 8); } }
      }
    }
    for (const p of st.pr) { const u = (t - p.t0) / .32, A = HP[p.h], B = EP[p.to], a = P(A.x, A.z), b = P(B.x, B.z), x = lerp(a[0], b[0], u), y = lerp(a[1] - 20, b[1] - 18, u) - Math.sin(u * Math.PI) * 14; if (p.h === 0) { const rr = 3 + (lv > 50 ? 2 : 0) + (lv > 85 ? 2 : 0); g.disc(x, y, rr, 8); g.disc(x, y, rr - 1, 9); g.px(x, y, 10); } else if (p.h === 1) { g.line(x - 6, y, x, y, 7); g.px(x + 1, y, 6); } else if (p.h === 2) { g.disc(x, y, 3, 14); g.px(x, y, 7); } }
    for (const d of st.dm) { const u = (t - d.t0) / .9, B = EP[d.i]; if (!B) continue; const q = P(B.x, B.z); g.txt(d.v, q[0], q[1] - 54 - u * 18, UI.t[d.c], { al: 'c', ol: 0 }); }
    g.panel(8, 170, 224, 22); g.txt('volume das faixas (posição do herói)', 18, 174, UI.dim, { sz: 8 }); g.txt(LV.map((v, i) => (i + 1) + ' ' + (RESTS[i] ? 'mudo' : Math.round(v * 100) + '%')).join('   '), 18, 183, UI.tx);
    g.bar('Formation', 'posição = volume', Math.round(bpm) + ' bpm');
    g.dials(['herói 1', 'herói 2', 'herói 3', 'herói 4'], LV.map((v, i) => RESTS[i] ? 'mudo' : Math.round(v * 100) + '%'), k);
  } });

// ------------------------------------------------------------------ RPG skins for the stock pages
const hud = (g, title, sub, right, lab, val, k) => { g.bar(title, sub, right); g.dials(lab, val, k); };
function adsr(k) { return { A: .05 + k[0] * .3, D: .06 + k[1] * .3, S: .15 + k[2] * .75, R: .1 + k[3] * .4 }; }
const prof = (u, e) => u < e.A ? u / e.A : u < e.A + e.D ? lerp(1, e.S, (u - e.A) / e.D) : u < 1 - e.R ? e.S : e.S * (1 - (u - (1 - e.R)) / e.R);
const SK = [];
// ENV 1/2 · spell scroll (knobs ATK DEC SUS REL, like the real ENV page)
add({ name: 'ENV · Feitiço 1/2', tag: 'ENV 1/2 · pergaminho de feitiço', tier: 'sk',
  how: ['tap:ENV = páginas ENV (1/2 e 2/2)', 'turn:KNOB 1–4 = ATK · DEC · SUS · REL'], prop: false,
  fields: [{ l: 'ATK', d: 'attack: tempo de subida do feitiço até a carga máxima (lado esquerdo da curva).', v: .25, fmt: v => Math.round(2 + v * 400) + ' ms' }, { l: 'DEC', d: 'decay: a queda depois do pico até o nível de sustain.', v: .4, fmt: v => Math.round(20 + v * 800) + ' ms' }, { l: 'SUS', d: 'sustain: nível em que a energia se mantém enquanto a tecla está apertada.', v: .55, fmt: v => pct(v) + '%' }, { l: 'REL', d: 'release: tempo para a energia apagar depois de soltar a tecla.', v: .5, fmt: v => Math.round(30 + v * 1200) + ' ms' }],
  desc: 'A curva ADSR da página ENV vira uma fita de energia mágica sobre um pergaminho ornamentado: uma orbe percorre a curva a cada nota, subindo no attack, caindo no decay, flutuando no sustain e se apagando no release. Os 4 knobs são os mesmos do aparelho.',
  bullets: ['Mesma edição da página ENV: ATK, DEC, SUS e REL', 'A orbe mostra o nível da energia em tempo real', 'Faíscas só nos picos: o resto é uma linha e quatro marcas', 'Moldura de pergaminho e cantoneiras douradas'],
  draw(g, t, k) {
    const e = adsr(k), WX = 28, WW = 184, BY = 168, BH = 100, u = fr(t * .3); g.clear(0); g.sky(19, 197, [0, 0, 1, 1, 0]);
    g.R(14, 26, 212, 140, 4); g.R(16, 28, 208, 136, 7); g.R(16, 28, 208, 2, 15); g.R(16, 162, 208, 2, 15); for (const [x, y] of [[16, 28], [214, 28], [16, 154], [214, 154]]) { g.R(x, y, 10, 10, 10); g.R(x + 2, y + 2, 6, 6, 4); }
    g.ell(120, 27, 106, 3, 15); g.ell(120, 165, 106, 3, 15); g.R(28, BY, WW, 1, 4); g.R(28, BY - BH, WW, 1, 15);
    const xs = [0, e.A, e.A + e.D, 1 - e.R, 1], nm = ['ATK', 'DEC', 'SUS', 'REL']; for (let i = 1; i < 5; i++) for (let y = BY; y > BY - BH; y -= 4) g.px(WX + xs[i] * WW, y, 9);
    for (let i = 0; i < 4; i++) g.txt(nm[i], WX + (xs[i] + xs[i + 1]) / 2 * WW, BY + 2, 4, { al: 'c', sz: 8 });
    let py = null; for (let x = 0; x < WW; x++) { const v = x / WW, y = BY - prof(v, e) * BH; if (py != null) { const a = Math.min(py, y), b = Math.max(py, y); g.R(WX + x, a - 1, 1, Math.max(1, b - a) + 2, 13); g.R(WX + x, a, 1, Math.max(1, b - a), 12); } py = y; }
    const ox = WX + u * WW, oy = BY - prof(u, e) * BH; g.disc(ox, oy, 6, 0); g.disc(ox, oy, 5, 10); g.disc(ox, oy, 3, 7); for (let q = 0; q < 6; q++) { const a = t * 6 + q * 1.05; g.px(ox + Math.cos(a) * 9, oy + Math.sin(a) * 9, q & 1 ? 10 : 9); } for (let q = 1; q < 8; q++) g.px(ox - q * 3, BY - prof(u - q * .012, e) * BH, 9);
    hud(g, 'Envelope', 'spell scroll', 'ENV 1/2', ['ATK', 'DEC', 'SUS', 'REL'], [Math.round(2 + k[0] * 400), Math.round(20 + k[1] * 800), pct(e.S) + '%', Math.round(30 + k[3] * 1200)], k);
  } });
add({ name: 'LFO · Sea', tag: 'LFO 1/2 · navio na onda', tier: 'sk', how: ['tap:LFO = páginas LFO (1/2 e 2/2)', 'turn:KNOB 1–4 = RATE · WAVE · PHASE · FADE'], prop: false,
  fields: [{ l: 'RATE', d: 'velocidade das ondas (taxa do LFO em Hz).', v: .4, fmt: v => (.2 + v * 6).toFixed(1) + ' Hz' }, { l: 'WAVE', d: 'forma da onda: SIN, TRI, SAW, SQR ou S&H (degraus aleatórios).', v: 0, fmt: v => ['SIN', 'TRI', 'SAW', 'SQR', 'S&H'][Math.round(v * 4)] }, { l: 'PHASE', d: 'fase inicial: desloca o desenho da onda para o lado (0–127).', v: 0, fmt: v => Math.round(v * 127) }, { l: 'FADE', d: 'tempo para o LFO entrar: as ondas crescem do zero até a altura cheia.', v: .3, fmt: v => v < .02 ? '0' : Math.round(v * 127) }],
  desc: 'O LFO é a superfície do mar; o navio sobe e desce sobre a onda e inclina com a inclinação dela. A forma de onda muda o desenho do mar, a fase desloca as ondas e o fade faz o mar "crescer" aos poucos a cada ciclo.',
  bullets: ['Os 4 knobs são os da página LFO: RATE, WAVE, PHASE e FADE', 'S&H vira ondas em degraus aleatórios', 'Fade: o mar começa calmo e cresce até a altura cheia', 'O navio segue a onda (uma conta por quadro)'],
  draw(g, t, k) {
    const rate = .2 + k[0] * 6, shape = Math.round(k[1] * 4), ph0 = k[2], fade = k[3], WC = 12, wv = (s, p) => { p = fr(p); return s === 0 ? Math.sin(p * TAU) : s === 1 ? (p < .5 ? p * 4 - 1 : 3 - p * 4) : s === 2 ? p * 2 - 1 : s === 3 ? (p < .5 ? 1 : -1) : (hs(Math.floor(p * 6) + 1) - .5) * 2; }, cyc = 7, amp = fade < .02 ? 1 : clamp((t % cyc) / (fade * 5 + .2), 0, 1);
    g.clear(0); g.sky(19, 100, [0, 1, 1, 2, 13]); g.disc(60, 50, 12, 7); g.disc(66, 46, 11, 1); for (let i = 0; i < 20; i++) g.px(hs(i) * 240, 22 + hs(i + 3) * 60, 7);
    for (let x = 0; x < 240; x++) { const ph = x / 240 * 2 - t * rate * .3 + ph0, y = 118 + wv(shape, ph) * 24 * .65 * amp; g.R(x, y, 1, 197 - y, WC); g.R(x, y, 1, 2, 7); if (((x + Math.floor(t * 20)) & 7) === 0) g.R(x, y + 8, 3, 1, shade(PAL[WC], .8)); }
    const sx = 120, p0 = sx / 240 * 2 - t * rate * .3 + ph0, y0 = 118 + wv(shape, p0) * 24 * .65 * amp, y1 = 118 + wv(shape, p0 + .02) * 24 * .65 * amp, tilt = (y1 - y0) / 4;
    g.poly([[sx - 18, y0 - 6 + tilt], [sx + 18, y0 - 6 - tilt], [sx + 12, y0 + 2 - tilt], [sx - 12, y0 + 2 + tilt]], 4); g.R(sx - 1, y0 - 30, 2, 26, 4); g.poly([[sx + 2, y0 - 29], [sx + 16, y0 - 12], [sx + 2, y0 - 12]], 7); g.poly([[sx - 2, y0 - 26], [sx - 12, y0 - 12], [sx - 2, y0 - 12]], 15);
    hud(g, 'LFO', 'the tide', 'LFO 1/2', ['RATE', 'WAVE', 'PHASE', 'FADE'], [rate.toFixed(1), ['SIN', 'TRI', 'SAW', 'SQR', 'S&H'][shape], Math.round(ph0 * 127), fade < .02 ? '0' : Math.round(fade * 127)], k);
  } });
// FX SLICER · houses grow into towers (knobs SLCR PAT RATE DEPTH)
add({ name: 'FX Slicer · Towers', tag: 'FX slicer · casas que viram torres', tier: 'sk', how: ['tap:FX = páginas FX', 'turn:KNOB 1–4 = SLCR · PAT · RATE · DEPTH'], prop: false,
  fields: [{ l: 'SLCR', d: 'modo do slicer: OFF (cidade apagada e baixa), GATE (alturas do gate) ou STUT (as janelas repetem piscando).', v: .5, fmt: v => ['OFF', 'GATE', 'STUT'][Math.round(v * 2)] }, { l: 'PAT', d: 'qual dos 16 desenhos de gate a cidade segue.', v: .3, fmt: v => 1 + Math.round(v * 15) }, { l: 'RATE', d: 'divisão do tempo (1/8, 1/16, 1/32, 8T, 16T, 32T): velocidade da tocha que percorre as torres.', v: .4, fmt: v => ['1/8', '1/16', '1/32', '8T', '16T', '32T'][Math.round(v * 5)] }, { l: 'DEPTH', d: 'profundidade do corte: casinhas baixas viram torres altas conforme sobe.', v: .7, fmt: v => pct(v) + '%' }],
  desc: 'No lugar das colunas, uma cidade de 16 construções: cada step é um prédio cuja altura é o nível do gate. Casas pequenas crescem para casas, torres e castelo conforme o DEPTH aumenta; uma tocha percorre as torres no ritmo do RATE.',
  bullets: ['Os 4 knobs são os da página SLICER: SLCR, PAT, RATE e DEPTH', 'Altura: choupana → casa → torre → castelo (só retângulos)', 'Janela acesa = step ativo; no modo STUT as janelas piscam', 'OFF deixa a cidade baixa e escura, como o slicer desligado'],
  draw(g, t, k) {
    const mode = Math.round(k[0] * 2), pat = Math.round(k[1] * 15), RT = [2, 4, 8, 2.67, 5.33, 10.67][Math.round(k[2] * 5)], depth = k[3], play = Math.floor(t * RT) % 16; g.clear(0); g.sky(19, 130, [0, 0, 1, 1, 2]); for (let i = 0; i < 20; i++) g.px(hs(i) * 240, 22 + hs(i + 3) * 70, i & 1 ? 7 : 10); g.disc(206, 44, 10, 7); g.disc(211, 41, 9, 0);
    g.R(0, 150, 240, 47, 3); for (let y = 152; y < 197; y += 6) g.R(0, y, 240, 1, 0);
    const val = i => { if (mode === 0) return .1; const b = hs(i * 3.3 + pat * 7.1) < .55 ? .35 + hs(i * 1.7 + pat) * .65 : .12; return clamp(b * (.3 + depth * 1.1), .08, 1); };
    for (let i = 0; i < 16; i++) {
      const v = val(i), x = 6 + i * 14.5, base = 168 - (i & 1) * 6, hgt = 8 + v * 78, lit = i === play, w = 11 + (v > .6 ? 3 : 0), tier = v > .85 ? 3 : v > .55 ? 2 : v > .3 ? 1 : 0;
      g.ell(x + w / 2, base + 2, w, 3, 0); g.R(x, base - hgt, w, hgt, tier === 3 ? 6 : 5); g.R(x, base - hgt, w, 2, 7); g.R(x + w - 2, base - hgt, 2, hgt, 0);
      if (tier === 0) g.poly([[x - 1, base - hgt], [x + w / 2, base - hgt - 6], [x + w + 1, base - hgt]], 8); else if (tier === 1) g.poly([[x - 2, base - hgt], [x + w / 2, base - hgt - 9], [x + w + 2, base - hgt]], 8); else if (tier === 2) { for (let q = 0; q < 3; q++) g.R(x + q * 4, base - hgt - 4, 3, 4, 5); g.R(x + w / 2, base - hgt - 14, 1, 10, 7); g.poly([[x + w / 2 + 1, base - hgt - 14], [x + w / 2 + 7, base - hgt - 11], [x + w / 2 + 1, base - hgt - 8]], 8); } else { for (let q = 0; q < 4; q++) g.R(x + q * 4 - 1, base - hgt - 5, 3, 5, 6); g.poly([[x + 2, base - hgt - 5], [x + w / 2, base - hgt - 20], [x + w - 2, base - hgt - 5]], 12); g.R(x + w / 2, base - hgt - 28, 1, 8, 7); g.poly([[x + w / 2 + 1, base - hgt - 28], [x + w / 2 + 8, base - hgt - 25], [x + w / 2 + 1, base - hgt - 22]], 8); }
      for (let q = 0; q < Math.floor(hgt / 12); q++) { const on = mode > 0 && (hs(i * 7 + q) < .6 || lit) && (mode !== 2 || (Math.floor(t * RT * 2 + q) & 1)); g.R(x + 3 + (q & 1) * 3, base - hgt + 8 + q * 11, 3, 4, on ? 10 : 1); }
      if (lit && mode > 0) { const f = 3 + Math.round(Math.sin(t * 20) * 1.5); g.R(x + w / 2 - 1, base - hgt - 36, 2, 5, 4); g.poly([[x + w / 2, base - hgt - 42 - f], [x + w / 2 - 3, base - hgt - 36], [x + w / 2 + 3, base - hgt - 36]], 9); g.poly([[x + w / 2, base - hgt - 40], [x + w / 2 - 1.5, base - hgt - 36], [x + w / 2 + 1.5, base - hgt - 36]], 10); }
    }
    hud(g, 'FX Slicer', 'the sleepless city', 'FX', ['SLCR', 'PAT', 'RATE', 'DEPTH'], [['OFF', 'GATE', 'STUT'][mode], pat + 1, ['1/8', '1/16', '1/32', '8T', '16T', '32T'][Math.round(k[2] * 5)], pct(depth) + '%'], k);
  } });
// FX Delay · the echoing party in the forest
add({ name: 'FX Delay · Floresta', tag: 'FX delay · ecos na floresta', tier: 'sk', how: ['tap:FX = páginas FX', 'turn:KNOB 1–4 = TIME · FDBK · COLR · MIX'], prop: false,
  fields: [{ l: 'TIME', d: 'divisão do tempo entre os ecos (1/8, 1/16…): distância entre os bonecos.', v: .5 }, { l: 'FDBK', d: 'quantos ecos aparecem antes de sumir.', v: .55, fmt: v => 1 + Math.round(v * 4) }, { l: 'COLR', d: 'color: escurece os ecos distantes (filtro do delay).', v: .5 }, { l: 'MIX', d: 'quão forte é o primeiro eco.', v: .6 }],
  desc: 'O herói grita numa floresta à noite e a voz volta como ecos: cada eco é uma cópia de silhueta, menor e mais escura, mais atrás na trilha, com o "hey" saindo em cima da cabeça no momento certo.',
  bullets: ['Cada eco = um boneco + um "hey" acima da cabeça (some junto com o boneco)', 'Distância entre os ecos = time · quantidade = feedback · escurecimento = tone', 'Herói principal fica à esquerda; os ecos recuam em perspectiva', 'Barato: poucos sprites de uma cor repetidos'],
  draw(g, t, k) {
    const n = 1 + Math.round(k[1] * 4), gap = 30 + k[0] * 18, ph = fr(t * .45); g.clear(0); g.sky(19, 120, [0, 0, 1, 1, 2]); for (let i = 0; i < 22; i++) g.px(hs(i) * 240, 22 + hs(i + 3) * 80, i & 1 ? 7 : 10); g.disc(204, 44, 10, 7); g.disc(209, 41, 9, 0);
    for (let i = 0; i < 14; i++) { const x = i * 19 + 4, h = 40 + hs(i) * 30; g.poly([[x, 120], [x + 9, 120 - h], [x + 18, 120]], i & 1 ? 3 : 1); }
    g.R(0, 120, 240, 77, 3); for (let y = 122; y < 197; y += 5) g.R(0, y, 240, 1, 0); g.poly([[0, 197], [100, 120], [140, 120], [240, 197]], 4);
    for (let i = n; i >= 0; i--) {
      const sc = i === 0 ? 3 : i === 1 ? 2 : 1, x = 26 + i * gap * (1 - i * .08), y = 168 - i * 9, vis = i === 0 ? 1 : clamp(.35 + k[3] * .65 - i * .1, .15, 1), w = 17 * sc, h2 = 24 * sc;
      g.ell(x + w / 2, y + 2, w * .55, 2 + sc, 0); const o = { s: sc, t }; if (i) { o.solid = k[2] > .5 && i > 2 ? 13 : 6; o.noOut = 1; } fig(g.R, 'archer', x, y - h2, o);
      const on = i === 0 ? fr(ph * 1) < .5 : fr(ph - i * .13) < .5, tx = x + w / 2, ty = y - h2 - 12 - (i ? 0 : 4); if (on || i === 0) { const lab = i === 0 ? 'HEY!' : 'hey', ww = g.tw(lab, 'p', i === 0 ? 12 : 10) + 8; g.R(tx - ww / 2, ty - 3, ww, i === 0 ? 14 : 12, i === 0 ? 7 : shade('#f2e2c4', vis)); g.poly([[tx - 2, ty + 11], [tx + 2, ty + 11], [tx, ty + 15]], i === 0 ? 7 : shade('#f2e2c4', vis)); g.txt(lab, tx, ty - 1, 0, { al: 'c', sz: i === 0 ? 12 : 10 }); }
    }
    for (let r = 0; r < n + 1; r++) { const u = fr(ph - r * .13); g.ellO(40 + u * 170, 176, 8 + u * 30, 3 + u * 8, r ? 14 : 7); }
    hud(g, 'FX Delay', 'echoes in the forest', 'FX', ['TIME', 'FDBK', 'COLR', 'MIX'], [Math.round(120 + k[0] * 600) + 'ms', n, pct(k[2]), pct(k[3])], k);
  } });
// FX Reverb/Chorus · side view of a gothic cathedral (stained-glass windows, pillars, pews, banners)
add({ name: 'FX Reverb · Catedral', tag: 'FX reverb e chorus · catedral gótica', tier: 'sk', how: ['tap:FX = páginas FX', 'turn:KNOB 1–4 = SIZE · DAMP · CRT · CDP'], prop: false,
  fields: [{ l: 'SIZE', d: 'tamanho da catedral: de 2 a 4 vitrais visíveis (nave pequena ou enorme).', v: .5, fmt: v => 2 + Math.round(v * 2) + ' vitrais' }, { l: 'DAMP', d: 'damping: quanto o som é absorvido; mais DAMP = nave mais escura e mais poeira (menos cauda).', v: .4, fmt: v => pct(v) + '%' }, { l: 'CRT', d: 'chorus rate: velocidade com que os coristas balançam.', v: .4, fmt: v => (.1 + v * 3).toFixed(1) + ' Hz' }, { l: 'CDP', d: 'chorus depth: quantos coristas aparecem e o quanto se afastam do cantor.', v: .5, fmt: v => pct(v) + '%' }],
  desc: 'Visto de lado, como nas referências: grandes vitrais góticos com painéis coloridos entre colunas escuras, bancos de igreja, piso de pedra com tapete vermelho e estandartes. Raios de luz colorida caem dos vitrais no chão, com poeira no ar; o cantor fica à frente e os coristas aparecem atrás dele.',
  bullets: ['SIZE muda o número de vitrais (2 a 4) e o espaçamento das colunas', 'DAMP escurece a nave e aumenta a poeira', 'Vitrais feitos de painéis de 5 px com chumbo preto: poucos retângulos', 'CRT e CDP controlam os coristas, como no chorus real'],
  draw(g, t, k) {
    const nw = 2 + Math.round(k[0] * 2), damp = k[1], crt = 1.5 + k[2] * 6, nch = 1 + Math.round(k[3] * 3), dim = 1 - damp * .45, GL = [12, 8, 10, 11, 13, 14], FL = 168; g.clear(0);
    g.R(0, 19, 240, 178, '#1c1a24'); for (let r = 0; r < 14; r++) for (let c = 0; c < 10; c++) g.R(c * 26 + (r & 1) * 13 - 10, 20 + r * 12, 25, 11, (r * 5 + c * 3) % 5 ? '#242030' : '#2c2638');
    const sp = 240 / nw, ww = Math.min(46, sp - 22), wt = 26, wh = FL - wt - 12;
    for (let i = 0; i <= nw; i++) { const x = i * sp; g.R(x - 8, 19, 16, FL - 19, '#3a3038'); g.R(x - 8, 19, 3, FL - 19, '#4a4048'); g.R(x + 5, 19, 3, FL - 19, '#241c24'); g.R(x - 10, FL - 14, 20, 14, '#2c242c'); g.R(x - 10, 19, 20, 8, '#2c242c'); }
    for (let i = 0; i < nw; i++) {
      const cx = i * sp + sp / 2, x0 = cx - ww / 2; g.poly([[x0 - 3, wt + wh], [x0 - 3, wt + 22], [cx, wt - 6], [x0 + ww + 3, wt + 22], [x0 + ww + 3, wt + wh]], '#0c0a10');
      for (let y = wt; y < wt + wh; y += 5) { const arch = y < wt + 24 ? Math.sqrt(Math.max(0, 1 - Math.pow((wt + 24 - y) / 24, 2))) : 1, hw = ww / 2 * arch; for (let x = cx - hw; x < cx + hw - 1; x += 5) { const q = hs(Math.floor(x / 5) * 3.1 + Math.floor(y / 5) * 7.7 + i * 11), band = y < wt + 26 ? 4 : y > wt + wh - 24 ? 2 : (Math.floor((x - x0) / (ww / 3))), c = GL[Math.floor(q * 3 + band) % 6]; g.R(x, y, 4, 4, shade(PAL[c], (.55 + q * .35) * dim)); } }
      g.R(cx - 1, wt + 24, 1, wh - 24, '#0c0a10'); for (const f of [.33, .66]) g.R(x0 + ww * f, wt + 24, 1, wh - 24, '#0c0a10'); g.R(x0, wt + 62, ww, 1, '#0c0a10'); g.R(x0, wt + 100, ww, 1, '#0c0a10'); g.disc(cx, wt + 12, 6, '#0c0a10'); g.disc(cx, wt + 12, 4, shade(PAL[GL[(i + 2) % 6]], .8 * dim)); g.R(x0 - 3, wt + wh, ww + 6, 4, '#4a4048');
      if (nw < 4 && i % 2 === 0) { g.R(cx - ww / 2 - 18, wt + 8, 12, 46, 0); g.R(cx - ww / 2 - 17, wt + 9, 10, 44, 8); g.R(cx - ww / 2 - 14, wt + 24, 4, 12, 10); g.poly([[cx - ww / 2 - 17, wt + 54], [cx - ww / 2 - 12, wt + 48], [cx - ww / 2 - 7, wt + 54]], 8); }
    }
    g.R(0, FL, 240, 29, '#3a3440'); g.R(0, FL, 240, 3, '#6a6470'); for (let r = 0; r < 2; r++) for (let c = 0; c < 12; c++) g.R(c * 22 + r * 11 - 4, FL + 6 + r * 11, 21, 10, (r + c) % 3 ? '#2c2834' : '#342e3c'); g.R(40, FL + 4, 170, 5, 8); g.R(40, FL + 4, 170, 1, 10); g.R(40, FL + 8, 170, 1, 10);
    for (let p = 0; p < 3; p++) { const px = 8 + p * 24; g.R(px, FL - 16, 22, 5, '#5a1a1c'); g.R(px, FL - 11, 22, 11, '#3a1014'); g.R(px - 2, FL - 18, 3, 18, '#4a2418'); g.R(px + 2, FL - 20, 18, 3, '#6a3a20'); }
    const rays = Math.round(nw); for (let i = 0; i < rays; i++) { const cx = i * sp + sp / 2, c = GL[(i * 2) % 6], bx = cx + 26, w = 14; for (let y = wt + 70; y < FL; y += 2) { const u = (y - wt - 70) / (FL - wt - 70), xl = cx - 6 + u * 40, xr = cx + 14 + u * 52; for (let x = Math.round(xl); x < xr; x += 2) if (((x + y) & 3) === 0) g.px(x, y, shade(PAL[c], .9 * dim)); } }
    for (let q = 0; q < Math.round(10 + damp * 40); q++) { const a = fr(t * (.04 + hs(q) * .05) + hs(q + 7)); g.px((hs(q * 3) * 240 + Math.sin(t + q) * 6) % 240, 40 + a * 124, 7); }
    for (let c = nch; c >= 0; c--) { const x = 120 - 17 + (c ? (c % 2 ? 1 : -1) * (24 + Math.ceil(c / 2) * 18 * (.4 + k[3])) : 0), o = { s: 2, t }; if (c) { o.solid = 13; o.noOut = 1; } fig(g.R, 'cleric', x + (c ? Math.sin(t * crt + c) * (1 + k[3] * 2) : 0), FL + 4 - 48, o); }
    g.R(0, 0, 0, 0, 0);
    hud(g, 'FX Reverb', 'the cathedral', 'REV/CHO', ['SIZE', 'DAMP', 'CRT', 'CDP'], [nw + ' win', pct(damp) + '%', (.1 + k[2] * 3).toFixed(1) + 'Hz', pct(k[3]) + '%'], k);
  } });
// SCL 1/2 + 2/2 · torches on a dungeon wall (knobs ROOT SCALE QNT CHORD, TRANS)
const SCLS = [[0, 2, 4, 5, 7, 9, 11], [0, 2, 3, 5, 7, 8, 10], [0, 2, 3, 5, 7, 9, 10], [0, 1, 3, 5, 7, 8, 10]], NN = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'], CHN = ['OFF', 'TRIAD', '7TH', '9TH', 'SUS4', 'POWER'], QN = ['OFF', 'SNAP', 'WHITE'];
add({ name: 'SCL · Torch wall', tag: 'SCL · tochas na parede (4 opções)', tier: 'sk', how: ['tap:SCL = páginas SCL (1/2 e 2/2)', 'hold:SCL + key:1–16 = tonalidade'], prop: false,
  fields: [{ l: 'ROOT', d: 'nota raiz: a tocha com a bandeira.', v: .17, fmt: v => NN[Math.round(v * 11)] }, { l: 'SCALE', d: 'maj, min, dor ou phr: quais das 12 tochas ficam acesas.', v: 0, fmt: v => ['MAJ', 'MIN', 'DOR', 'PHR'][Math.round(v * 3)] }, { l: 'QNT', d: 'quantização das teclas: OFF (todas tocam), SNAP (cai na escala) ou WHITE (só as teclas brancas).', v: .5, fmt: v => QN[Math.round(v * 2)] }, { l: 'CHORD', d: 'acorde de uma tecla: OFF, TRIAD, 7TH, 9TH, SUS4 ou POWER (fumaça sobre a raiz).', v: 0, fmt: v => CHN[Math.round(v * 5)] }],
  desc: 'As páginas SCL 1 e 2 numa só tela: as 12 notas da oitava são 12 tochas presas numa parede de masmorra. As tochas das notas da escala estão acesas, com chama e brilho amarelo; as outras estão apagadas. A raiz tem a bandeira, o QNT liga ou desliga tochas e o CHORD solta fumaça. São só 4 opções, uma por knob.',
  bullets: ['Só 4 opções, uma por knob: ROOT, SCALE, QNT e CHORD', 'Tocha acesa = nota dentro da escala; apagada = fora', 'QNT WHITE apaga as tochas dos sustenidos (só teclas brancas)', 'Brilho em pontos amarelos tracejados'],
  draw(g, t, k) {
    const root = Math.round(k[0] * 11), sc = SCLS[Math.round(k[1] * 3)], qn = Math.round(k[2] * 2), ch = Math.round(k[3] * 5), tr = 0; g.clear(0);
    g.R(0, 19, 240, 150, '#1d1520'); for (let r = 0; r < 11; r++) for (let c = 0; c < 9; c++) { const x = c * 28 + (r & 1) * 14 - 10, y = 21 + r * 14; g.R(x, y, 26, 12, (r * 3 + c) % 4 ? 5 : '#3a3040'); g.R(x, y, 26, 1, 6); }
    g.R(0, 169, 240, 28, 4); for (let x = 0; x < 240; x += 24) g.R(x, 169, 1, 28, 0); for (let i = 0; i < 4; i++) { const x = 8 + i * 76; g.R(x, 19, 8, 150, '#2a2030'); g.R(x + 8, 19, 2, 150, 0); }
    const BL = [1, 3, 6, 8, 10], torch = [];
    for (let n = 0; n < 12; n++) { const note = ((n - tr) % 12 + 12) % 12, black = BL.includes(n), inScale = sc.includes(((note - root) % 12 + 12) % 12), inS = qn === 0 ? true : qn === 2 ? inScale && !black : inScale, x = 20 + n * 18, y = black ? 96 : 128, isRoot = note === root, fl = 4 + Math.round(Math.sin(t * 12 + n * 2) * 1.5 + Math.sin(t * 7 + n) * 1);
      if (inS && (qn !== 0 || inScale)) for (let py = -18; py <= 18; py++) for (let px = -22; px <= 22; px++) { const d = Math.hypot(px / 22, py / 18); if (d < 1 && ((px + py) & 1) === 0 && (d < .55 || ((px >> 1) + (py >> 1)) % 2 === 0)) g.px(x + px, y - 6 + py, d < .55 ? 10 : 9); }
      torch.push({ n, x, y, inS: inS && (qn !== 0 || inScale), isRoot, fl, note, black }); }
    for (const q of torch) { const { x, y, inS, isRoot, fl, note, black } = q;
      g.R(x - 2, y + 12, 5, 12, 0); g.R(x - 1, y + 12, 3, 12, 4); g.R(x - 1, y + 12, 1, 12, 15); g.R(x - 6, y + 6, 13, 3, 0); g.R(x - 5, y + 7, 11, 1, 6); g.poly([[x - 6, y + 6], [x - 4, y - 1], [x + 5, y - 1], [x + 7, y + 6]], 0); g.poly([[x - 5, y + 5], [x - 3, y], [x + 4, y], [x + 6, y + 5]], 6); g.R(x - 4, y, 9, 1, 7);
      if (inS) { g.poly([[x, y - 12 - fl], [x - 5, y - 1], [x + 5, y - 1]], 8); g.poly([[x, y - 9 - fl * .8], [x - 3.5, y - 1], [x + 3.5, y - 1]], 9); g.poly([[x, y - 6 - fl * .5], [x - 2, y - 1], [x + 2, y - 1]], 10); g.px(x, y - 3, 7); } else { g.R(x - 2, y - 2, 5, 2, 0); g.px(x, y - 3, 5); }
      if (isRoot) { g.R(x + 10, y - 26, 1, 22, 7); g.poly([[x + 11, y - 26], [x + 21, y - 22], [x + 11, y - 18]], 8); }
      g.txt(NN[note], x, black ? 72 : 152, inS ? (isRoot ? UI.hi : UI.tx) : UI.dim, { al: 'c' });
      if (ch && isRoot) for (let z = 0; z < ch + 2; z++) { const a = fr(t * .5 + z / 6); g.R(x + Math.sin(a * 7 + z) * 6, y - 16 - a * 30, 3, 3, 6); } }
    hud(g, 'Scale', 'the torch wall', 'SCL 1·2', ['ROOT', 'SCALE', 'QNT', 'CHORD'], [NN[root], ['MAJ', 'MIN', 'DOR', 'PHR'][Math.round(k[1] * 3)], QN[qn], CHN[ch]], k);
  } });
// Layer roll · rune stones
add({ name: 'Layer Roll · Runas', tag: 'layer ARP (roll) · pedras rúnicas', tier: 'sk', how: ['hold:ARP + key:1–16 = note repeat nas pedras', 'hold:ARP + turn:KNOB 1 = RATE'], prop: false,
  fields: [{ l: 'rate', d: 'velocidade do note repeat: as pedras acendem em sequência.', v: .5, fmt: v => ['1/4', '1/8', '1/16', '1/32'][Math.round(v * 3)] }, { l: 'padrão', d: 'quais pedras estão ativas.', v: .4 }, { l: 'gate', d: 'duração do brilho de cada nota (curta = staccato).', v: .5 }, { l: 'oitava', d: 'altura da nota repetida: muda a cor da runa que brilha (−1, 0, +1).', v: .5, fmt: v => ['−1', '0', '+1'][Math.min(2, Math.round(v * 2))] }],
  desc: 'Os 16 pads do layer viram 16 pedras rúnicas num chão em perspectiva. As ativas brilham; o note repeat acende uma pedra de cada vez, soltando um anel de luz no chão.',
  bullets: ['Grade 4×4 de pedras (mesma ordem das teclas 1–16)', 'Runa ativa = pad ligado; anel de luz = nota tocando', 'Gate = quanto tempo cada pedra fica acesa', 'Substitui os quadros escuros por algo com cara de dungeon'],
  draw(g, t, k) {
    const hz = 56, rate = [2, 4, 8, 16][Math.round(k[0] * 3)], cur = Math.floor(t * rate) % 16, gate = .25 + k[2] * .7, oc = Math.min(2, Math.round(k[3] * 2)); g.clear(0); g.sky(19, hz, [0, 1, 1]); g.R(0, hz, 240, 197 - hz, '#1d1520'); floor7(g, hz, 197, 120, 30, 120, 0, 40, '#1d1520', '#241a26');
    const act = i => hs(i * 3 + Math.round(k[1] * 10)) < .5 || i % 5 === 0;
    for (let r = 0; r < 4; r++) for (let c = 0; c < 4; c++) { const i = r * 4 + c, z = 1.1 + (3 - r) * .55, cx = 120 + (c - 1.5) * 54 / z, cy = hz + 106 / z, w = 22 / z, h = 12 / z, on = act(i), cu = i === cur && fr(t * rate) < gate;
      g.poly([[cx - w, cy], [cx, cy - h], [cx + w, cy], [cx, cy + h]], on ? (cu ? 10 : 5) : 1); g.poly([[cx - w, cy], [cx, cy + h], [cx, cy + h + 3 / z], [cx - w, cy + 3 / z]], 0); g.poly([[cx + w, cy], [cx, cy + h], [cx, cy + h + 3 / z], [cx + w, cy + 3 / z]], 0);
      if (on) { const rc = [[12, 11, 10, 8], [11, 10, 8, 14], [13, 12, 11, 10]][oc][c]; g.R(cx - 1, cy - 3 / z * 2, 2, 6 / z, rc); g.R(cx - 3 / z, cy - 1, 6 / z, 2, rc); }
      if (cu && on) { const u = fr(t * rate) / gate; g.ellO(cx, cy, w + u * 22, h + u * 12, 10); g.ellO(cx, cy, w + u * 14, h + u * 8, 7); } }
    hud(g, 'Roll', 'rune stones', 'ARP', ['rate', 'pattern', 'gate', 'octave'], [['1/4', '1/8', '1/16', '1/32'][Math.round(k[0] * 3)], pct(k[1]), pct(gate), ['-1', '0', '+1'][oc]], k);
  } });
add({ name: 'Layer Steps · Placas', tag: 'layer SEQ (steps) · placas de pressão', tier: 'sk', how: ['hold:SEQ + key:1–16 = liga/desliga os steps 1–16', 'hold:SEQ + turn:KNOB 1–4 = NOTE · DIV · SWING · LENGTH'], prop: false,
  fields: [{ l: 'NOTE', d: 'qual nota (ou som, na bateria) está sendo editada nas placas.', v: .4, fmt: v => NN[Math.round(v * 11)] }, { l: 'DIV', d: 'divisão do tempo de cada step (1/8, 1/16, 1/32).', v: .5, fmt: v => ['1/8', '1/16', '1/32'][Math.round(v * 2)] }, { l: 'SWING', d: 'atraso dos steps pares: as placas pares afundam um pouco depois.', v: .2, fmt: v => (50 + Math.round(v * 25)) + '%' }, { l: 'LENGTH', d: 'quantos steps a sequência tem (1–16); os de fora racham e apagam.', v: 1, fmt: v => 1 + Math.round(v * 15) }],
  desc: 'Os 16 steps da página viram 16 placas de pressão de pedra numa masmorra isométrica. As ligadas brilham com uma runa; uma tocha percorre as placas no ritmo e cada nota afunda a placa. Placas além do LENGTH ficam rachadas.',
  bullets: ['Mesma ordem das teclas 1–16 (4 fileiras de 4)', 'Placa acesa = step ligado; chama = step tocando', 'SWING atrasa as placas pares; LENGTH racha as últimas', 'Diferente do Roll: aqui é edição de steps, lá é note repeat'],
  draw(g, t, k) {
    const len = 1 + Math.round(k[3] * 15), sw = k[2] * .3, div = [2, 4, 8][Math.round(k[1] * 2)], cur = Math.floor(t * div) % len, ISO = (x, y) => [120 + (x - y) * 22, 64 + (x + y) * 11]; g.clear(0); g.sky(19, 197, [0, 0, 1, 1, 0]); g.R(0, 40, 240, 120, '#1d1520'); for (let r = 0; r < 9; r++) for (let c = 0; c < 9; c++) g.R(c * 28 + (r & 1) * 14 - 10, 22 + r * 14, 26, 12, (r * 3 + c) % 4 ? '#2a2030' : '#34283c');
    const on = i => hs(i * 5 + Math.round(k[0] * 11)) < .5 || i % 4 === 0;
    for (let i = 0; i < 16; i++) { const r = i >> 2, c = i & 3, p = ISO(c * 1.1, r * 1.1), live = i < len, isCur = i === cur, dip = isCur && fr(t * div) < .3 ? 3 : (i & 1 ? sw * 6 * (isCur ? 1 : 0) : 0), x = p[0], y = p[1] + dip;
      g.poly([[x, y + 8], [x + 22, y + 19], [x, y + 30], [x - 22, y + 19]], 0); g.poly([[x - 22, y + 19], [x, y + 30], [x, y + 30 + (live ? 6 : 3) - dip], [x - 22, y + 19 + (live ? 6 : 3) - dip]], 0); g.poly([[x, y + 30], [x + 22, y + 19], [x + 22, y + 19 + (live ? 6 : 3) - dip], [x, y + 30 + (live ? 6 : 3) - dip]], 0);
      g.poly([[x, y + 10], [x + 19, y + 19], [x, y + 28], [x - 19, y + 19]], !live ? 1 : on(i) ? (isCur ? 10 : 5) : '#2f2a3a');
      if (!live) { g.line(x - 8, y + 16, x + 2, y + 22, 0); g.line(x + 2, y + 22, x - 2, y + 26, 0); } else if (on(i)) { const c2 = [12, 11, 10, 8][c]; g.R(x - 1, y + 14, 2, 10, c2); g.R(x - 5, y + 18, 10, 2, c2); if (isCur) { const f = 3 + Math.round(Math.sin(t * 20) * 1.5); g.poly([[x, y + 8 - f], [x - 4, y + 16], [x + 4, y + 16]], 9); g.poly([[x, y + 10 - f / 2], [x - 2, y + 16], [x + 2, y + 16]], 10); } } }
    hud(g, 'Steps', 'pressure plates', 'SEQ', ['NOTE', 'DIV', 'SWING', 'LENGTH'], [NN[Math.round(k[0] * 11)], ['1/8', '1/16', '1/32'][Math.round(k[1] * 2)], (50 + Math.round(k[2] * 25)) + '%', len], k);
  } });
// SAVE · song chests
add({ name: 'SAVE · Baús', tag: 'SAVE / SONG · baús como espaços de save', tier: 'sk', how: ['tap:SAVE = (na Party) tela SONG', 'hold:SAVE + key:1–4 = toca a seção A–D · key 5–8 = salva o loop na seção', 'turn:KNOB 1–4 = STEP · SECTION · BARS · STEPS'], prop: false,
  fields: [{ l: 'STEP', d: 'passo da corrente (1–8) que está selecionado: a etiqueta fica dourada.', v: 0, fmt: v => 1 + Math.round(v * 7) }, { l: 'SECTION', d: 'qual seção (A–D) esse passo toca: o baú correspondente se abre.', v: .34, fmt: v => 'ABCD'[Math.round(v * 3)] }, { l: 'BARS', d: 'quantos compassos o passo dura (1–8).', v: .3, fmt: v => 1 + Math.round(v * 7) }, { l: 'STEPS', d: 'quantos passos a corrente tem (1–8).', v: .5, fmt: v => 1 + Math.round(v * 7) }],
  desc: 'Cada seção da song (A–D) é um baú numa adega: baús cheios brilham e têm cadeado, o vazio está aberto e escuro. A corrente da song aparece como etiquetas A B A C… acima, e o baú do passo selecionado abre a tampa soltando luz e uma pilha de notas.',
  bullets: ['Baú = espaço de save; luz e moedas = o que está guardado', 'Corrente de passos como etiquetas penduradas', 'Os 4 knobs são os da tela SONG: STEP, SECTION, BARS e STEPS', 'REC guarda o loop no baú selecionado (uma faísca entra nele)'],
  draw(g, t, k) {
    const step = Math.round(k[0] * 7), sec = Math.round(k[1] * 3), bars = 1 + Math.round(k[2] * 7), nst = 1 + Math.round(k[3] * 7), FILL = [1, 1, 1, 0], chain = Array.from({ length: 8 }, (_, i) => 'ABAC'[i % 4] === 'C' && i > 5 ? 'D' : 'ABACABAD'[i]); g.clear(0); g.R(0, 19, 240, 150, '#1d1520'); for (let r = 0; r < 11; r++) for (let c = 0; c < 9; c++) g.R(c * 28 + (r & 1) * 14 - 10, 20 + r * 14, 26, 12, (r * 3 + c) % 4 ? '#2a2030' : '#34283c');
    g.R(0, 150, 240, 47, 4); for (let y = 152; y < 197; y += 6) g.R(0, y, 240, 1, 0); for (let i = 0; i < 8; i++) { const x = 14 + i * 28, on = i === step, ok = i < nst; g.R(x + 8, 24, 1, 8, 6); g.R(x, 32, 24, 14, ok ? (on ? 10 : '#7a6a58') : '#2a2030'); g.R(x, 32, 24, 1, 15); g.txt(chain[i], x + 12, 34, ok ? 0 : UI.dim, { al: 'c', sz: 11 }); if (ok) g.txt(on ? bars + 'b' : '', x + 12, 48, 10, { al: 'c', sz: 8 }); }
    for (let i = 0; i < 4; i++) {
      const x = 14 + i * 56, y = 112, open = i === sec, full = FILL[i], gl = open && full; if (gl) for (let q = 0; q < 70; q += 2) g.R(x + 18 + (q & 2 ? 0 : 1), y - 10 - q * .9, 12 - q / 8, 2, q < 30 ? 7 : 10);
      g.R(x, y + 14, 48, 32, 0); g.R(x + 1, y + 15, 46, 30, 4); g.R(x + 1, y + 15, 46, 2, 15); g.R(x + 6, y + 15, 4, 30, 6); g.R(x + 38, y + 15, 4, 30, 6);
      if (open) { g.R(x - 1, y - 6, 50, 6, 0); g.R(x, y - 8 - 2, 48, 8, 4); g.R(x + 6, y - 10, 4, 10, 6); g.R(x + 38, y - 10, 4, 10, 6); g.R(x + 2, y + 8, 44, 8, full ? 10 : '#1a1018'); if (full) { for (let q = 0; q < 6; q++) g.R(x + 6 + q * 7 + (q % 2), y + 4 + Math.sin(t * 4 + q) * 1.5, 5, 5, [10, 12, 11, 8, 14, 7][q]); } }
      else { g.R(x, y + 2, 48, 14, 0); g.R(x + 1, y + 3, 46, 12, 4); g.R(x + 1, y + 3, 46, 2, 15); g.R(x + 6, y + 3, 4, 12, 6); g.R(x + 38, y + 3, 4, 12, 6); }
      g.R(x + 20, y + 14, 8, 10, full ? 10 : 6); g.R(x + 22, y + 17, 4, 4, 0);
      g.txt('ABCD'[i], x + 24, y + 28, open ? 10 : UI.tx, { al: 'c', sz: 12 }); g.txt(full ? (i + 1) * 4 + ' bars' : 'empty', x + 24, y + 48, full ? UI.dim : 8, { al: 'c', sz: 8 }); if (open) { g.poly([[x + 18, y - 22 + Math.sin(t * 6) * 2], [x + 30, y - 22 + Math.sin(t * 6) * 2], [x + 24, y - 15 + Math.sin(t * 6) * 2]], 10); } }
    g.panel(8, 170, 224, 22); g.txt('step ' + (step + 1) + ' → section ' + 'ABCD'[sec] + ' · ' + bars + ' bars · chain of ' + nst, 16, 174, UI.tx, { sz: 8 }); g.txt('REC stores the loop in the open chest', 16, 183, UI.dim, { sz: 8 });
    hud(g, 'Song', 'the vault', 'SAVE', ['STEP', 'SECTION', 'BARS', 'STEPS'], [step + 1, 'ABCD'[sec], bars, nst], k);
  } });
// About
let LOGO = null; { const im = new Image(); im.onload = () => { LOGO = im; }; im.src = 'assets/logo-340.png'; }
add({ name: 'About', tag: 'créditos do aparelho', tier: 'sk', how: ['hold:HOME = abre o menu', 'turn:PRESETS = ABOUT → press:OCT+'], prop: false,
  fields: [{ l: 'página', d: 'alterna entre os agradecimentos (1), o sobre (2) e as licenças/versão (3).', v: 0, fmt: v => (Math.round(v * 2) + 1) + '/3' }],
  desc: 'Uma placa de pedra com o logo do FM1 Quest e os agradecimentos: aos firmwares que inspiraram este projeto (SLOOP, Felucca e Baud Girl) e à comunidade r/MVaveFM1. A página 2 conta em poucas linhas o que é o projeto e a página 3 traz as licenças e a versão. O endereço do site aparece como texto (não é um link).',
  bullets: ['Logo em 1 bit com contorno escuro (o mesmo da abertura)', 'Agradecimentos aos projetos que inspiraram e à comunidade', 'Página 2: licença GPL-3.0 e número do build', 'Tela estática: custo de SPI só ao abrir'],
  draw(g, t, k, st) {
    const pg = Math.round(k[0] * 2); g.clear(0); st.fc = (st.fc || 0) + 1; const nowS = performance.now(); if (!st.f0) st.f0 = nowS; if (nowS - st.f0 >= 1000) { st.fps = Math.round(st.fc * 1000 / (nowS - st.f0)); st.fc = 0; st.f0 = nowS; } g.R(0, 19, 240, 178, '#1d1520'); for (let r = 0; r < 13; r++) for (let c = 0; c < 9; c++) { const x = c * 28 + (r & 1) * 14 - 10, y = 20 + r * 14; g.R(x, y, 26, 12, (r * 3 + c) % 4 ? 5 : '#3a3040'); g.R(x, y, 26, 1, 6); }
    g.panel(10, 26, 220, 164); if (LOGO && pg !== 1) { g.c.imageSmoothingEnabled = false; g.c.drawImage(LOGO, 55, 32, 130, 98); }
    const P1 = [['Thanks to', UI.hi], ['SLOOP · Felucca · Baud Girl', UI.tx], ['for the inspiration, and to the', UI.tx], ['r/MVaveFM1 community', UI.tx], ['', UI.tx], ['fm1.quest', UI.hi]],
      P2 = [['Sobre', UI.hi], ['Este é um projeto pessoal, feito', UI.tx], ['com amor, só para testar a tecnologia', UI.tx], ['e explorar todo o potencial de um', UI.tx], ['hardware incrível e acessível.', UI.tx], ['fm1.quest', UI.hi]],
      P3 = [['License', UI.hi], ['GPL-3.0-only (firmware and editor)', UI.tx], ['groovebox firmware for the', UI.tx], ['M-VAVE FM-1', UI.tx], ['key lights: @renebohne · MIDI fix:', UI.dim], ['ChanceTheMaker, keremimo', UI.dim]], lines = [P1, P2, P3][pg], y0 = pg === 1 ? 40 : 132, dy = pg === 1 ? 13 : 9.5;
    lines.forEach((l, i) => g.txt(l[0], 120, y0 + i * dy, l[1], { al: 'c', sz: i === 0 ? 11 : 10 })); if (pg !== 1) { const fl = 2 + Math.round(Math.sin(t * 9)); for (const x of [22, 218]) { g.R(x - 1, 150, 3, 20, 4); g.poly([[x, 140 - fl], [x - 3, 150], [x + 3, 150]], 9); g.poly([[x, 144 - fl / 2], [x - 1.5, 150], [x + 1.5, 150]], 10); } }
    { const ST = G.STORE || { cap: 81920, usr: [0, 0, 0] }, used = ST.usr.reduce((a, b) => a + b, 0), tot = ST.cap * 3; g.txt('mem ' + (used / 1024).toFixed(1) + '/' + (tot / 1024).toFixed(0) + ' KiB', 6, 21, '#7a7080', { sz: 8, nofit: true }); g.txt((st.fps || 0) + ' fps', 234, 21, '#7a7080', { sz: 8, al: 'r', nofit: true }); }
    g.bar('About', 'FM1 Quest', (pg + 1) + '/3');
    g.dials(['page', 'build', 'base', 'engine'], [(pg + 1) + '/3', 15, 'FM-1', 'Quest'], [pg / 2, .15, .5, .5]);
  } });


// ------------------------------------------------------------------ Party · Blocos (full-screen variant, info in colour blocks)
const TN = ['ACID 303', 'DUB CHORD', 'ATMOS PAD', 'TECHNO'], PPAT = ['1011010110110110', '0000001000000100', '1000000000000000', '1000100010001000'], TCOL = ['#3a62c0', '#4a9a3a', '#d8a830', '#d87a30'];
add({ name: 'Party · Blocos', tag: 'variação da Party em tela cheia', tier: 'q', an: [240, 150],
  how: ['tap:HOME = volta para a Party (variação proposta)', 'turn:ALGORITHM = troca o herói selecionado'],
  fields: [{ l: 'selecionado', d: 'qual herói está em destaque (borda branca e salto).', v: 0, fmt: v => 1 + Math.round(v * 3) }, { l: 'estilo', d: 'personagens atuais (de lado) ou o novo desenho (de frente, estilo chibi).', v: 0, fmt: v => v > .5 ? 'novo' : 'atual' }, { l: 'inimigo', d: 'slime, morcego ou rei: o monstro grande no centro.', v: .5, fmt: v => ['slime', 'bat', 'king'][Math.min(2, Math.round(v * 2))] }, { l: 'bpm', d: 'andamento: ritmo dos golpes e do pulo do monstro.', v: .4, fmt: v => Math.round(70 + v * 110) }],
  desc: 'Inspirada nos RPGs de turno: o monstro enorme no centro sobre uma lua e um vitral de losangos, os quatro heróis enfileirados embaixo, cada um em cima de um bloco colorido que mostra os 16 steps da música sendo tocada (o bloco aceso é o step atual), e uma barra vertical de compasso à esquerda. As abas do topo trazem bpm, posição e swing.',
  bullets: ['Cada bloco tem a cor da faixa e 16 blocos de step: os acesos são notas, o branco é o step tocando agora', 'Ocupa a tela toda (a Party atual deixa um vazio entre a cena e os knobs)', 'Barra da esquerda = posição no compasso; as 4 bolinhas acendem a cada tempo', 'Compare "atual" e "novo" no campo estilo: o novo desenho é de frente, mais legível em tamanho pequeno', 'Por que blocos sobre a arte: cor sólida + contorno garantem contraste sem tirar espaço da cena'],
  draw(g, t, k, st) {
    const sel = Math.round(k[0] * 3), nw = k[1] > .5, kind = ['slime', 'bat', 'king'][Math.min(2, Math.round(k[2] * 2))], bpm = 70 + k[3] * 110, beat = t * bpm / 60, step = Math.floor(beat * 4), M = MON;
    if (!st.bg) { const oc = document.createElement('canvas'); oc.width = oc.height = 240; const b = T.makeG(oc); b.clear(0); b.R(0, 19, 240, 221, '#1a1030');
      for (let gy = 0; gy < 12; gy++) for (let gx = -1; gx < 11; gx++) { const cx = gx * 24 + (gy & 1) * 12, cy = 24 + gy * 15, c = ['#2b1a4a', '#3a2260', '#46286e', '#1e1240'][Math.floor(hs(gx * 7 + gy * 3) * 4)]; b.poly([[cx, cy - 8], [cx + 12, cy], [cx, cy + 8], [cx - 12, cy]], c); b.polyO([[cx, cy - 8], [cx + 12, cy], [cx, cy + 8], [cx - 12, cy]], '#120a24'); }
      b.disc(120, 104, 70, '#e8c040'); b.disc(120, 104, 66, '#f0d060'); for (let gy = 0; gy < 12; gy++) for (let gx = -1; gx < 11; gx++) { const cx = gx * 24 + (gy & 1) * 12, cy = 24 + gy * 15; if (Math.hypot(cx - 120, cy - 104) < 62) b.polyO([[cx, cy - 8], [cx + 12, cy], [cx, cy + 8], [cx - 12, cy]], '#d8b040'); }
      st.bg = oc; }
    g.c.drawImage(st.bg, 0, 0); g.pn = null;
    st.at = st.at || [-9, -9, -9, -9]; st.hp = st.hp == null ? 1 : st.hp; st.n = st.n == null ? step : st.n; st.hurt = st.hurt || -9; st.dead = st.dead || -9; st.pr = st.pr || [];
    while (st.n < step) { st.n++; for (let h = 0; h < 4; h++) if (PPAT[h][st.n % 16] === '1') { st.at[h] = t; st.pr.push({ h, t0: t }); } }
    st.pr = st.pr.filter(p => { if (t - p.t0 >= .3) { if (st.hp > 0) { st.hp -= .05; st.hurt = t; if (st.hp <= 0) st.dead = t; } return false; } return true; });
    if (st.hp <= 0 && t - st.dead > 1.6) { st.hp = 1; }
    const hurtF = t - st.hurt < .12, dp = st.hp <= 0 ? clamp((t - st.dead) / 1.1, 0, 1.1) : 0, sk = dp > 0 ? (x, y) => hs(x * 13 + y * 7 + 500) < dp : null, ec = 120, gy0 = 134;
    g.ell(ec, gy0 + 3, kind === 'king' ? 52 : 36, 7, '#0a0618');
    if (kind === 'slime') { const hp = Sc.hop(beat, 4), R = scaled(g, ec - 22 * 3.2, gy0 - 16 * 3.2 - 6, 3.2); Sc.slimeD(R, 22, 16 + 2, 22, 16, M.slime, { lift: dp > 0 ? 0 : hp.lift, sq: dp > 0 ? 0 : hp.sq, flash: hurtF, skip: sk, noShadow: true }); }
    else if (kind === 'bat') { const R = scaled(g, ec - 12 * 5, gy0 - 50 + Math.round(Math.sin(t * 4) * 4), 5); Sc.spr(R, Math.floor(beat * 2) % 2 ? Sc.BATB : Sc.BATA, 0, 0, M.bat, { s: 1, solid: hurtF ? 7 : null, skip: sk }); }
    else { const hp = Sc.hop(beat, 3), R = scaled(g, ec - 44 * 1.55 / 2 * 1.0 - 8, gy0 - 32 * 1.55 - 12, 1.55); Sc.slimeD(R, 44, 32 + 6, 44, 32, M.king, { crown: 1, lift: dp > 0 ? 0 : hp.lift, sq: dp > 0 ? 0 : hp.sq, flash: hurtF, skip: sk, noShadow: true }); }
    for (const p of st.pr) { const u = (t - p.t0) / .3, hx = 24 + p.h * 54 + 25, x = lerp(hx, ec, u), y = lerp(150, 100, u) - Math.sin(u * Math.PI) * 28; if (p.h === 0) { g.disc(x, y, 4, 8); g.disc(x, y, 2, 10); } else if (p.h === 1) { g.line(x - 8, y, x, y, 7); } else if (p.h === 2) { g.disc(x, y, 3, 14); g.px(x, y, 7); } else { g.R(x - 3, y - 1, 7, 2, 7); } }
    // top tabs
    g.R(0, 0, 240, 22, '#000'); const tabs = [['Party', 46, 7], ['' + Math.round(bpm) + ' bpm', 46, 10], ['bar ' + (1 + Math.floor(beat / 4) % 8) + '.' + (1 + Math.floor(beat) % 4), 46, 11], ['swing 50', 50, 12], ['2 bars', 40, 14]]; let tx = 3;
    tabs.forEach((tb, i) => { g.R(tx, 3, tb[1], 16, i === 0 ? '#e8a020' : '#2a2040'); g.R(tx + 1, 4, tb[1] - 2, 14, i === 0 ? '#3a2410' : '#3a3058'); g.txt(tb[0], tx + tb[1] / 2, 5, tb[2], { al: 'c', sz: i === 0 ? 12 : 11, ol: 0 }); tx += tb[1] + 3; });
    // enemy name + hp
    g.R(70, 25, 100, 5, 0); g.R(71, 26, 98, 3, '#2a2040'); g.R(71, 26, Math.max(0, 98 * st.hp), 3, st.hp > .5 ? 11 : st.hp > .25 ? 10 : 8); g.txt(kind === 'king' ? 'King Slime' : kind === 'bat' ? 'Bat' : 'Slime', 120, 32, 7, { al: 'c', sz: 11, ol: 0 });
    // left gauge
    const bp = (beat / 4) % 1; g.R(5, 26, 16, 150, '#000'); g.R(7, 28, 12, 146, '#2a2040'); g.R(7, 174 - 146 * bp, 12, 146 * bp, 12); g.poly([[13, 24], [16, 33], [24, 33], [18, 38], [20, 46], [13, 41], [6, 46], [8, 38], [2, 33], [10, 33]], 10);
    for (let i = 0; i < 4; i++) { const lit = Math.floor(beat) % 4 >= i; g.disc(13, 152 - i * 28, 8, 0); g.disc(13, 152 - i * 28, 6, lit ? [10, 9, 8, 8][i] : '#3a3058'); if (lit) g.px(11, 150 - i * 28, 7); }
    // heroes + colour blocks
    const lvl = [81, 81, 81, 78], steps = [16, 16, 16, 16];
    for (let h = 0; h < 4; h++) {
      const x0 = 26 + h * 53, w = 50, cx = x0 + w / 2, isS = h === sel, bounce = isS ? Math.abs(Math.sin(t * 5)) * 3 : 0, atk = t - st.at[h] < .35, y0 = 194;
      g.ell(cx, y0 - 1, 15, 3, '#0a0618');
      if (nw) T.chibi(g, CLS[h], cx, y0 - 2 - Math.round(bounce) - (atk ? 4 : 0), false, t); else fig(g.R, CLS[h], cx - 17, y0 - 48 - Math.round(bounce) - (atk ? 3 : 0), { s: 2, t, p: atk ? lerp(-.3, 1, (t - st.at[h]) / .35) : null, f: 0 });
      g.R(x0 - 1, y0 - 1, w + 2, 46, isS ? '#fff' : '#000'); const base = TCOL[h]; for (let y = 0; y < 44; y++) g.R(x0, y0 + y, w, 1, y & 1 ? base : shade(base, .86)); for (let y = 2; y < 44; y += 4) for (let x = 2; x < w; x += 4) g.px(x0 + x, y0 + y, shade(base, 1.2));
      g.R(x0, y0, w, 2, shade(base, 1.25)); g.txt(TN[h], x0 + 3, y0 + 3, 7, { sz: 8, ol: 0 });
      for (let q = 0; q < 16; q++) { const on = PPAT[h][q] === '1', cur = q === step % 16, row = q >> 3, cx2 = x0 + 3 + (q & 7) * 6, cy2 = y0 + 14 + row * 14; g.R(cx2 - 1, cy2 - 1, 7, 11, 0); g.R(cx2, cy2, 5, 9, cur ? 7 : on ? shade(base, .55) : shade(base, .6)); if (on) g.R(cx2 + 1, cy2 + 1, 3, 7, cur ? 10 : shade(base, 1.45)); if (cur && on) { g.R(cx2 - 1, cy2 - 3, 7, 1, 10); } }
      const now = PPAT[h][step % 16] === '1' && t - st.at[h] < .25; g.txt(now ? '♪ ' + ['C', 'D', 'E', 'G', 'A'][(h + step) % 5] : 'rest', x0 + w - 4, y0 + 3, now ? 10 : 7, { al: 'r', sz: 8, ol: 0 });
    }
  } });
})(window);
