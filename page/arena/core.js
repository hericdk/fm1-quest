/* FM1 Arena · núcleo: firmware (WebMIDI), sintetizador, rede de salas, ranking, analytics. Teste local. */
(function () {
'use strict';
const A = window.A = window.A || {};
A.dev = (() => { try { if (/[?&]dev=1/.test(location.search)) localStorage.setItem('arena.dev', '1'); if (/[?&]dev=0/.test(location.search)) localStorage.removeItem('arena.dev'); return localStorage.getItem('arena.dev') === '1'; } catch (e) { return false; } })();
A.$ = (s, r) => (r || document).querySelector(s);
A.$$ = (s, r) => [...(r || document).querySelectorAll(s)];
A.rnd = (n) => Math.floor(Math.random() * n);

/* ---------- firmwares conhecidos ---------- */
A.FW = {
  quest:    { id: 'quest',    name: 'FM1 Quest',    logo: null },
  sloop:    { id: 'sloop',    name: 'Sloop',        logo: 'img/logo-sloop.png' },
  felucca:  { id: 'felucca',  name: 'Felucca',      logo: 'img/logo-felucca.png' },
  baudgirl: { id: 'baudgirl', name: 'Baud Girl',    logo: 'img/logo-baudgirl.png' },
  mvave:    { id: 'mvave',    name: 'M-VAVE (original)', logo: null },
  other:    { id: 'other',    get name() { return A.t ? A.t('fw_other') : 'Other'; }, logo: null },
};
A.fwLogo = (id, h) => {
  const f = A.FW[id] || A.FW.other;
  return f.logo ? `<img class="fwlogo" src="${f.logo}" alt="${f.name}" title="${f.name}" style="height:${h || 22}px">`
                : `<span class="fwtxt" title="${f.name}" style="font-size:${Math.round((h || 22) * .55)}px">${f.name.toUpperCase()}</span>`;
};
/* Como o firmware é reconhecido (ajuste aqui quando soubermos o que cada um responde):
   1) SysEx do editor Felucca/Sloop  F0 7D 46 4C 01 F7  -> "FELUCCA <versão>"  (o FM1 Quest responde "FELUCCA 0.2x")
   2) Identity Request universal     F0 7E 7F 06 01 F7
   3) nome da porta MIDI                                                          */
A.classify = (info, port) => {
  port = port || '';
  if (info) {
    if (/baud/i.test(info)) return 'baudgirl';
    if (/sloop/i.test(info)) return 'sloop';
    const m = /^FELUCCA\s+([\d.]+)/i.exec(info);
    if (m) return /^0\.2/.test(m[1]) ? 'quest' : /^2\./.test(m[1]) ? 'sloop' : 'felucca';
  }
  if (/baud/i.test(port)) return 'baudgirl';
  if (/sloop/i.test(port)) return 'sloop';
  if (/felucca/i.test(port)) return 'felucca';
  if (/fm-?1|m-?vave/i.test(port)) return 'mvave';
  return 'other';
};

/* ---------- placas do ranking (escudo/placa em pixel art: ouro, prata, bronze, madeira) ---------- */
const PAL = {
  gold:   { o: '#4a2c08', b: '#e8b830', h: '#fff0a0', s: '#a8781c', l: '#cc9a24' },
  silver: { o: '#2c2c36', b: '#c6cad4', h: '#ffffff', s: '#7c828e', l: '#a4aab6' },
  bronze: { o: '#3c1c08', b: '#c47a38', h: '#f0b27a', s: '#7a4418', l: '#a2602a' },
  wood:   { o: '#24160c', b: '#85582e', h: '#aa7640', s: '#573616', l: '#6c4624' },
};
const PL_W = 36, PL_H = 24, PL_X = [[3, 33], [1, 35]].concat(Array(16).fill([0, 36])).concat([[1, 35], [3, 33], [6, 30], [10, 26], [14, 22], [17, 19]]);
const plCache = {};
A.plaque = function (kind) {
  if (plCache[kind]) return plCache[kind];
  const P = PAL[kind] || PAL.wood, inS = (x, y) => y >= 0 && y < PL_H && x >= PL_X[y][0] && x < PL_X[y][1];
  let svg = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${PL_W} ${PL_H}" shape-rendering="crispEdges">`;
  for (let y = 0; y < PL_H; y++) {
    let run = null;
    const push = () => { if (run) svg += `<rect x="${run.x}" y="${y}" width="${run.w}" height="1" fill="${run.c}"/>`; run = null; };
    for (let x = PL_X[y][0]; x < PL_X[y][1]; x++) {
      const edge = !inS(x - 1, y) || !inS(x + 1, y) || !inS(x, y - 1) || !inS(x, y + 1);
      let col = P.b;
      if (edge) col = P.o;
      else {
        const e2 = (dx, dy) => !inS(x + dx, y + dy) || (!inS(x + dx * 2, y + dy * 2) && false);
        const nearTop = !inS(x, y - 2) || !inS(x - 1, y - 2), nearLeft = !inS(x - 2, y), nearBot = !inS(x, y + 2), nearRight = !inS(x + 2, y);
        if (nearTop || nearLeft) col = P.h; else if (nearBot || nearRight) col = P.s;
        if (kind === 'wood' && y > 2 && y < 16 && (y - 3) % 4 === 3) col = P.s;                 // tábuas
        if (kind !== 'wood' && (y === 18 || y === 19) && x > 8 && x < 28 && col === P.b) col = P.l;
        if ((x === 3 || x === 32) && (y === 3 || y === 15) && kind === 'wood') col = P.o;      // pregos
      }
      if (run && run.c === col && run.x + run.w === x) run.w++; else { push(); run = { x, w: 1, c: col }; }
    }
    push();
  }
  svg += '</svg>';
  return plCache[kind] = 'data:image/svg+xml;utf8,' + encodeURIComponent(svg);
};

/* ---------- MIDI ---------- */
const listeners = {};
A.on = (ev, fn) => ((listeners[ev] = listeners[ev] || []).push(fn), () => { listeners[ev] = listeners[ev].filter(f => f !== fn); });
A.emit = (ev, d) => (listeners[ev] || []).slice().forEach(f => { try { f(d); } catch (e) { console.error(e); } });
A.midi = { access: null, in: null, out: null, info: null, port: '', fw: null, chans: new Set(), clock: 0 };

A.connect = async function () {
  if (!navigator.requestMIDIAccess) throw new Error('Este navegador não tem WebMIDI. Use Chrome, Edge ou Opera.');
  const acc = A.midi.access || (A.midi.access = await navigator.requestMIDIAccess({ sysex: true }));
  const ins = [...acc.inputs.values()], outs = [...acc.outputs.values()];
  const pick = (l) => l.find(p => /fm-?1|m-?vave|felucca|sloop|baud/i.test(p.name)) || l[0];
  const inp = pick(ins), out = pick(outs);
  if (!inp) throw new Error('Nenhuma porta MIDI encontrada. Plugue o FM1 por USB e tente de novo.');
  A.midi.in = inp; A.midi.out = out || null; A.midi.port = inp.name;
  let info = null;
  inp.onmidimessage = null;
  const wait = new Promise((res) => {
    const t = setTimeout(() => res(null), 900);
    inp.onmidimessage = (e) => {
      const d = e.data;
      if (d[0] === 0xF0 && d[1] === 0x7D && d[2] === 0x46 && d[3] === 0x4C && d[4] === 0x01) {
        let s = ''; for (let i = 5; i < d.length && d[i] !== 0 && d[i] !== 0xF7; i++) s += String.fromCharCode(d[i]);
        clearTimeout(t); res(s);
      }
    };
  });
  if (out) { try { out.send([0xF0, 0x7D, 0x46, 0x4C, 0x01, 0xF7]); } catch (e) {} }
  info = await wait;
  A.midi.info = info;
  A.midi.fw = A.classify(info, inp.name);
  A.midi.ver = ((/^\w+\s+([\d.]+)/.exec(info || '')) || [])[1] || '';
  inp.onmidimessage = onMidi;
  A.emit('fw', A.midi);
  return A.midi;
};
function onMidi(e) {
  const [s, a, b] = e.data, c = s & 15, k = s & 0xF0;
  if (s === 0xF8) { A.midi.clock++; return; }
  if (k === 0x90 && b > 0) { A.midi.chans.add(c); A.emit('note', { n: a, v: b, ch: c, t: performance.now(), on: true }); }
  else if (k === 0x80 || (k === 0x90 && b === 0)) A.emit('note', { n: a, v: 0, ch: c, t: performance.now(), on: false });
}
/* sem teclado nem atalhos: a arena só reage ao FM1 (em modo de teste há teclas na tela, ver screens.js) */
A.audio = { ok: false, label: '' };
A.captureFM1Audio = async function () {
  try {
    if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) return false;
    A.synth.init();
    let devs = await navigator.mediaDevices.enumerateDevices();
    if (!devs.some(d => d.kind === 'audioinput' && d.label)) {            // rótulos só aparecem depois da permissão
      const t = await navigator.mediaDevices.getUserMedia({ audio: true }); t.getTracks().forEach(x => x.stop());
      devs = await navigator.mediaDevices.enumerateDevices();
    }
    const d = devs.find(x => x.kind === 'audioinput' && /fm-?1|m-?vave|felucca|sloop|baud/i.test(x.label));
    if (!d) return false;                                                  // não achou o áudio do FM1: não grava microfone nenhum
    const st = await navigator.mediaDevices.getUserMedia({ audio: { deviceId: { exact: d.deviceId }, echoCancellation: false, noiseSuppression: false, autoGainControl: false, channelCount: 2 } });
    const src = A.synth.ctx.createMediaStreamSource(st), g = A.synth.ctx.createGain(); g.gain.value = 1;
    src.connect(g); g.connect(A.synth.rec);                                // só para a gravação: o som já sai pelo próprio FM1
    A.audio.ok = true; A.audio.label = d.label; A.synth.fm1cap = true; A.net.audio = st; return true;
  } catch (e) { return false; }
};
A.ready = () => A.dev || !!A.midi.fw;                      // só entra quem tem FM1 (ou modo teste)
A.myFw = () => A.midi.fw || (A.dev ? (localStorage.getItem('arena.fakefw') || 'quest') : null);

/* ---------- sintetizador do navegador (sons dos outros jogadores / CPU / vídeo) ---------- */
const S = A.synth = { ctx: null, spk: null, rec: null, dest: null, mute: false };
S.init = function () {
  if (S.ctx) { if (S.ctx.state === 'suspended') S.ctx.resume(); return; }
  const c = S.ctx = new (window.AudioContext || window.webkitAudioContext)();
  S.spk = c.createGain(); S.spk.gain.value = .5; S.spk.connect(c.destination);
  S.rec = c.createGain(); S.rec.gain.value = .6; S.dest = c.createMediaStreamDestination(); S.rec.connect(S.dest);
};
const WAVES = ['square', 'sawtooth', 'triangle', 'square'];
S.note = function (n, v, track, local) {
  if (!S.ctx) return; const c = S.ctx, t = c.currentTime;
  const f = 440 * Math.pow(2, (n - 69) / 12), o = c.createOscillator(), g = c.createGain();
  o.type = WAVES[(track | 0) & 3]; o.frequency.value = f;
  const peak = .12 * (v / 127 + .3);
  g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(peak, t + .008); g.gain.exponentialRampToValueAtTime(.0008, t + .38);
  if (local && S.fm1cap) return;                                  // o som real do FM1 já está sendo gravado
  o.connect(g); if (local) g.connect(S.rec); else { g.connect(S.rec); g.connect(S.spk); }
  o.start(t); o.stop(t + .42);
};
S.noise = function (len, vol, hp, local) {
  if (!S.ctx) return; const c = S.ctx, n = Math.floor(c.sampleRate * len), b = c.createBuffer(1, n, c.sampleRate), d = b.getChannelData(0);
  for (let i = 0; i < n; i++) d[i] = (Math.random() * 2 - 1) * (1 - i / n);
  const s = c.createBufferSource(); s.buffer = b; const g = c.createGain(); g.gain.value = vol;
  const f = c.createBiquadFilter(); f.type = 'highpass'; f.frequency.value = hp || 800;
  s.connect(f); f.connect(g); g.connect(S.spk); s.start();                    // som de dano: só nos alto-falantes, nunca no vídeo
};
S.tick = (hi) => { if (!S.ctx) return; const o = S.ctx.createOscillator(), g = S.ctx.createGain(), t = S.ctx.currentTime;
  o.type = 'square'; o.frequency.value = hi ? 1320 : 880; g.gain.setValueAtTime(.08, t); g.gain.exponentialRampToValueAtTime(.001, t + .06);
  o.connect(g); g.connect(S.rec); g.connect(S.spk); o.start(t); o.stop(t + .07); };
S.hit = (big) => S.noise(big ? .35 : .12, big ? .5 : .25, big ? 200 : 900);

/* ---------- ranking: começa em zero; só conta quem entra numa luta Player vs Player com um FM1 conectado ---------- */
const CFG = A.cfg = window.ARENA_CONFIG || {};
const RK = 'arena.rank.v2', PERSON = 'arena.person.v1', COOLDOWN = 24 * 3600 * 1000;
const sha = async (s) => { try { const b = await crypto.subtle.digest('SHA-256', new TextEncoder().encode(s)); return [...new Uint8Array(b)].slice(0, 12).map(x => x.toString(16).padStart(2, '0')).join(''); } catch (e) { return s; } };
const ZERO = () => ({ conns: {}, wins: {}, ver: {} });
A.rank = {
  remote: false,                                  // true quando os números vêm do serviço (compartilhado entre todos)
  load() { try { return JSON.parse(localStorage.getItem(RK)) || ZERO(); } catch (e) { return ZERO(); } },
  data() { return A.rank._d || (A.rank._d = A.rank.load()); },
  save() { try { localStorage.setItem(RK, JSON.stringify(A.rank._d)); } catch (e) {} },
  /* total por firmware; o "FM1 Quest" soma os que já estavam nele e os que trocaram para ele depois de lutar com outro firmware */
  total(kind, fw) { const d = A.rank.data()[kind] || {}; return (d[fw] || 0) + (fw === 'quest' ? (d.quest_sw || 0) : 0); },
  switched() { return (A.rank.data().conns.quest_sw || 0); },
  sorted(kind) {
    const d = A.rank.data()[kind] || {}, ids = ['quest', 'sloop', 'felucca', 'baudgirl', 'mvave', 'other'];
    return ids.map(k => ({ id: k, n: A.rank.total(kind, k) })).sort((a, b) => b.n - a.n || ids.indexOf(a.id) - ids.indexOf(b.id));
  },
  versions() { const d = A.rank.data().ver || {}; return Object.keys(d).map(k => ({ k, id: k.split(' ')[0].replace('quest_sw', 'quest'), n: d[k] })).filter(r => r.n > 0).sort((a, b) => b.n - a.n); },
  async refresh() {                               // traz os números compartilhados, se houver serviço configurado
    if (!CFG.service) return;
    try { const r = await fetch(CFG.service.replace(/\/$/, '') + '/rank', { cache: 'no-store' }); if (!r.ok) return; const j = await r.json();
      A.rank._d = { conns: j.conns || {}, wins: j.wins || {}, ver: j.ver || {} }; A.rank.remote = true; A.emit('rank'); } catch (e) {}
  },
  async post(path, body) {
    if (!CFG.service) return;
    try { await fetch(CFG.service.replace(/\/$/, '') + path, { method: 'POST', headers: { 'content-type': 'application/json' }, body: JSON.stringify(body), keepalive: true }); } catch (e) {}
  },
  person() { try { return JSON.parse(localStorage.getItem(PERSON)) || { counted: {}, swDone: false }; } catch (e) { return { counted: {}, swDone: false }; } },
  /* a pessoa entrou numa luta PvP com um FM1 conectado: conta 1x por firmware a cada 24 h; quem passa a usar o FM1 Quest depois de lutar com outro firmware conta como "FM1 Quest (trocou)" uma vez */
  async entered(fw, ver) {
    if (!fw || !A.midi.fw) return null;           // só com FM1 de verdade (nada no modo de teste)
    const p = A.rank.person(), now = Date.now();
    if (p.counted[fw] && now - p.counted[fw] < COOLDOWN) return null;
    let kind = fw;
    if (fw === 'quest' && !p.swDone && Object.keys(p.counted).some(k => k !== 'quest')) { kind = 'quest_sw'; p.swDone = true; }
    p.counted[fw] = now; try { localStorage.setItem(PERSON, JSON.stringify(p)); } catch (e) {}
    const d = A.rank.data(); d.conns[kind] = (d.conns[kind] || 0) + 1;
    const vk = kind + ' ' + (ver || 'original'); d.ver[vk] = (d.ver[vk] || 0) + 1; A.rank.save();
    A.rank.post('/rank/hit', { id: await sha('fm1arena|' + A.net.id), fw: kind, ver: ver || 'original' });
    A.emit('rank'); return kind;
  },
  win(fw) { const d = A.rank.data(); d.wins[fw] = (d.wins[fw] || 0) + 1; A.rank.save(); A.rank.post('/rank/win', { fw }); A.emit('rank'); },
};

/* ---------- analytics (mesmo consentimento do site) ---------- */
A.track = function (name, p) {
  try {
    if (typeof gtag === 'function') { gtag('event', name, p || {}); return; }
    if (/^(localhost|127\.0\.0\.1|\[::1\])$/.test(location.hostname)) { console.log('[ga:local]', name, p || ''); return; }
    window.dataLayer = window.dataLayer || []; window.gtag = function () { dataLayer.push(arguments); };
    const ck = localStorage.getItem('fm1q.cookies');
    gtag('consent', 'default', { analytics_storage: ck === 'yes' ? 'granted' : 'denied', ad_storage: 'denied', ad_user_data: 'denied', ad_personalization: 'denied' });
    const s = document.createElement('script'); s.async = true; s.src = 'https://www.googletagmanager.com/gtag/js?id=G-8CREY6XFYZ'; document.head.append(s);
    gtag('js', new Date()); gtag('config', 'G-8CREY6XFYZ'); gtag('event', name, p || {});
  } catch (e) {}
};

/* ---------- rede: BroadcastChannel agora (abas do mesmo navegador); WebRTC P2P depois, mesma interface ---------- */
const pid = (() => { try { const st = A.dev ? sessionStorage : localStorage; let v = st.getItem('arena.pid'); if (!v) { v = Math.random().toString(36).slice(2, 9); st.setItem('arena.pid', v); } return v; } catch (e) { return Math.random().toString(36).slice(2, 9); } })();
A.ICE = [{ urls: 'stun:stun.l.google.com:19302' } /* , { urls: 'turn:SEU-TURN:3478', username: '...', credential: '...' } */];   // WebRTC: STUN do Google (grátis); TURN é opcional e precisa de um servidor próprio
const ROOM_TTL = 30000;                                           // a sala some 30 s depois que o dono some (sem avisar)
const N = A.net = { id: pid, ch: null, tx: null, rooms: {}, room: null, peers: {}, peerOf: {}, pidOf: {}, mode: '', audio: null };
let p2pRoom = null, p2pAct = null, started = false;
const FW_APP = 'fm1quest-arena-v021';          // todos os jogadores da arena se encontram neste "salão" (P2P)
N.start = function () {
  if (started) return; started = true;
  const local = /[?&]net=local/.test(location.search) || !window.Trystero;
  if (local) {                                               // teste: abas do mesmo navegador
    const ch = new BroadcastChannel('fm1arena'); N.mode = 'local';
    ch.onmessage = (e) => { const m = e.data; if (!m || m.from === N.id || (m.to && m.to !== N.id)) return; handle(m); };
    N.tx = { send: (m, to) => ch.postMessage(Object.assign({ from: N.id }, to ? { to } : {}, m)), stop() { ch.close(); } };
  } else {                                                   // WebRTC de verdade: sinalização por trackers públicos (BitTorrent), STUN do Google
    N.mode = 'p2p';
    p2pRoom = Trystero.joinRoom({ appId: FW_APP, rtcConfig: { iceServers: A.ICE } }, 'lobby');
    p2pAct = p2pRoom.makeAction('m');
    p2pAct.onMessage = (m, meta) => {
      const peerId = meta && meta.peerId; if (!m || m.from === N.id) return;
      N.peerOf[m.from] = peerId; N.pidOf[peerId] = m.from;
      if (m.to && m.to !== N.id) return; handle(m);
    };
    p2pRoom.onPeerJoin = (peerId) => {                       // quem chegou já recebe a sala que eu hospedo
      if (N.room && N.room.host === N.id) p2pAct.send({ from: N.id, to: undefined, t: 'room', room: pub(N.room) }, { target: peerId });
      if (N.audio && N.room && N.room.players.some(p => p.id === N.pidOf[peerId])) p2pRoom.addStream(N.audio, { target: peerId });
      A.emit('net:peers', { n: Object.keys(N.pidOf).length });
    };
    p2pRoom.onPeerLeave = (peerId) => {
      const pid2 = N.pidOf[peerId]; delete N.pidOf[peerId]; if (pid2) delete N.peerOf[pid2];
      if (pid2) handle({ t: 'leave', from: pid2, room: N.room && N.room.id });      // (a sala dele ainda fica 30 s na lista, como combinado)
      A.emit('net:peers', { n: Object.keys(N.pidOf).length });
    };
    p2pRoom.onPeerStream = (stream, peerId) => A.emit('net:stream', { pid: N.pidOf[peerId], stream, peerId });
    N.tx = {
      send: (m, to) => { const t = to && N.peerOf[to]; p2pAct.send(Object.assign({ from: N.id }, to ? { to } : {}, m), t ? { target: t } : undefined); },
      stop() { try { p2pRoom.leave(); } catch (e) {} },
    };
  }
  N.ch = N.tx;
  N.timer = setInterval(() => { // anuncia a sala que eu hospedo; os outros jogadores mandam "estou aqui"
    if (N.room && N.room.host === N.id) N.send({ t: 'room', room: pub(N.room) });
    else if (N.room) N.send({ t: 'hb' }, N.room.host);
    const now = Date.now(); let ch = false; for (const k in N.rooms) if (now - N.rooms[k].seen > ROOM_TTL) { delete N.rooms[k]; ch = true; }
    if (ch) A.emit('rooms');
    if (N.room && N.room.host === N.id) for (const p of N.room.players) { if (p.id !== N.id && N.peers[p.id] && now - N.peers[p.id] > 4000) A.emit('net:gone', { from: p.id }); }
  }, 1000);
  addEventListener('pagehide', () => N.leave());
};
N.stop = function () {                 // sai do salão P2P (quando a pessoa volta ao menu)
  if (!started || N.room) return; started = false; clearInterval(N.timer);
  try { N.tx && N.tx.stop(); } catch (e) {} N.tx = N.ch = null; N.rooms = {}; N.peerOf = {}; N.pidOf = {};
};
/* o som real do FM1 vai só para quem está na mesma sala */
N.shareAudio = function () {
  if (N.mode !== 'p2p' || !N.audio || !N.room) return;
  const ids = N.room.players.map(p => N.peerOf[p.id]).filter(Boolean);
  if (ids.length) { try { p2pRoom.addStream(N.audio, { target: ids }); } catch (e) {} }
};
const pub = (r) => ({ id: r.id, name: r.name, size: r.size, arena: r.arena, host: r.host, phase: r.phase, players: r.players });
N.send = (m, to) => { if (N.tx) N.tx.send(m, to); };
function handle(m) {
  N.peers[m.from] = Date.now();
  if (m.t === 'room') { N.rooms[m.room.id] = Object.assign({ seen: Date.now() }, m.room); A.emit('rooms'); }
  else if (m.t === 'gone') { delete N.rooms[m.room]; A.emit('rooms'); }
  else if (m.t === 'join' && N.room && N.room.host === N.id && N.room.id === m.room) {
    if (N.room.players.length >= N.room.size || N.room.phase !== 'wait') { N.send({ t: 'full', room: m.room }, m.from); return; }
    N.room.players.push({ id: m.from, fw: m.fw }); N.send({ t: 'room', room: pub(N.room) }); A.emit('roomupdate', N.room);
  }
  else if (m.t === 'leave' && N.room && N.room.host === N.id) {
    if (N.room.phase === 'wait') { N.room.players = N.room.players.filter(p => p.id !== m.from); A.emit('roomupdate', N.room); N.send({ t: 'room', room: pub(N.room) }); }
    else A.emit('net:gone', { from: m.from });                   // no meio da luta: um CPU assume o personagem
  }
  else if (m.t === 'hb') { /* só atualiza N.peers */ }
  else if (m.t === 'full') A.emit('full', m);
  else A.emit('net:' + m.t, m);
}
N.create = function (size, name) {
  N.start();
  N.room = { id: 'r' + Math.random().toString(36).slice(2, 7), name: name || 'Sala ' + (1 + A.rnd(99)), size, arena: '96', host: N.id, phase: 'wait', players: [{ id: N.id, fw: A.myFw() }] };
  N.send({ t: 'room', room: pub(N.room) }); return N.room;
};
N.join = function (r) {
  N.start(); N.room = Object.assign({}, r, { players: r.players.slice() });
  N.send({ t: 'join', room: r.id, fw: A.myFw() }, r.host);
};
N.leave = function (soft) {
  if (!N.room) return;
  if (N.room.host === N.id) N.send({ t: 'gone', room: N.room.id }); else N.send({ t: 'leave', room: N.room.id }, N.room.host);
  N.room = null; if (N.audio && p2pRoom) { try { p2pRoom.removeStream(N.audio); } catch (e) {} }
};
N.open = (size) => Object.values(N.rooms).filter(r => r.size === size && r.phase === 'wait' && r.players.length < r.size);

A.on('net:stream', (m) => {              // o som do FM1 do adversário chega pela rede: toca e entra na gravação, no lugar das notas sintetizadas
  try {
    S.init(); const a = new Audio(); a.srcObject = m.stream; a.muted = true; a.play().catch(() => {});      // (o Chrome só entrega o áudio remoto se houver um elemento ligado)
    const src = S.ctx.createMediaStreamSource(m.stream); src.connect(S.rec); src.connect(S.spk); S.remoteReal = true;
  } catch (e) {}
});
A.screens = {};
})();
