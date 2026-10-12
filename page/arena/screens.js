/* FM1 Arena · telas: menu, salas, luta. */
(function () {
'use strict';
const A = window.A, $ = A.$, $$ = A.$$;
const esc = (s) => String(s).replace(/[&<>"]/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));
A.sel = A.sel || { mode: 'cpu', size: 2 };
let game = null, train = null, cur = null;

/* ---------- transições (GSAP): mosaico de pixels que cobre a tela, troca a seção e se desfaz ---------- */
const MC = 16, MR = 9; let cells = [];
function buildMosaic() {
  const m = $('#mosaic'); if (m.dataset.built) return; m.dataset.built = 1;
  const cols = ['#1e1418', '#2a1f26', '#3a2620', '#1e1418', '#5a3a28', '#1e1418', '#c8963c'];
  for (let i = 0; i < MC * MR; i++) {
    const d = document.createElement('i'); d.style.background = cols[(Math.imul(i + 1, 2654435761) >>> 24) % cols.length]; m.append(d); cells.push(d);
  }
  m.style.gridTemplateColumns = `repeat(${MC},1fr)`; m.style.gridTemplateRows = `repeat(${MR},1fr)`;
}
const covered = (on) => { $('#mosaic').classList.toggle('on', on); };
const once = (fn) => { let d = false; return () => { if (!d) { d = true; fn && fn(); } }; };
function cover(done) {          // pixels crescem do centro e cobrem a tela
  buildMosaic(); covered(true); done = once(done);
  if (!window.gsap) { done(); return; }
  setTimeout(done, 1100);        // (aba em segundo plano pausa o GSAP: não deixa a tela presa)
  gsap.killTweensOf(cells); gsap.set('#mlogo', { autoAlpha: 0 });
  gsap.fromTo(cells, { scale: 0 }, { scale: 1.04, duration: .32, ease: 'expo.out', stagger: { each: .014, grid: [MR, MC], from: 'center' }, onComplete: done });
  gsap.to('#mlogo', { autoAlpha: 1, duration: .2, delay: .18 });
}
function reveal(done) {         // pixels somem para as bordas, mostrando a nova tela
  buildMosaic(); const fin = once(() => { covered(false); done && done(); });
  if (!window.gsap) { fin(); return; }
  setTimeout(fin, 1300);
  gsap.killTweensOf(cells);
  gsap.to('#mlogo', { autoAlpha: 0, duration: .25 });
  gsap.to(cells, { scale: 0, duration: .36, ease: 'expo.in', stagger: { each: .012, grid: [MR, MC], from: 'edges' }, onComplete: fin });
}
function show(name) {
  $$('.scr').forEach(e => e.classList.toggle('on', e.dataset.screen === name));
  cur = name; A.applyI18n(); A.track('arena_page_view', { screen: name });
  window.scrollTo(0, 0);
  if (A.screens[name]) A.screens[name]();
}
const go = (name) => { A.track('arena_navigate', { to: name }); if (name === cur) return; cover(() => { show(name); reveal(); }); };

/* ---------- barra superior: 4 firmwares mais usados + o meu ---------- */
function renderTop() {
  const rk = A.rank.sorted('conns'), me = A.myFw(), top = rk.slice(0, 4);
  let h = top.map((r, i) => cell(r, i + 1, r.id === me)).join('');
  const mi = rk.findIndex(r => r.id === me);
  if (me && mi >= 4) h += cell(rk[mi], mi + 1, true);
  else if (me && mi >= 0 && mi < 4) h += '';
  $('#top').innerHTML = h;
  function cell(r, pos, mine) {
    const kind = pos === 1 ? 'gold' : pos === 2 ? 'silver' : pos === 3 ? 'bronze' : 'wood';
    return `<div class="fwcell${mine ? ' me' : ''}"><span class="pos">#${pos}${mine ? ' · ' + A.t('me') : ''}</span>
      <div class="plq" style="background-image:url(&quot;${A.plaque(kind)}&quot;)">${A.fwLogo(r.id, 26)}</div><span class="num">${r.n} ${A.t('co')}</span></div>`;
  }
}

/* ---------- menu ---------- */
A.screens.menu = function () {
  const connect = $('#connect'), menu = $('#menu');
  const refresh = () => {
    const ok = A.ready(); connect.hidden = ok; menu.hidden = !ok;
    if (ok) { A.rank.refresh(); renderTop(); tryRejoin(); }
  };
  A.net.stop(); A.on('rank', renderTop);
  refresh();
  $('#btnconnect').onclick = async () => {
    A.synth.init(); $('#cerr').textContent = '';
    try {
      await A.connect(); A.track('arena_connect', { fw: A.midi.fw }); refresh();
      const ok = await A.captureFM1Audio(); const st = $('#aust'); if (st) { st.hidden = false; const known = A.midi.fw && A.midi.fw !== 'other'; st.textContent = A.t(ok ? 'au_ok' : 'au_no') + '\n' + A.t('fw_is') + ' ' + ((A.FW[A.midi.fw] || {}).name || '') + (known ? '' : ' (' + (A.midi.info || A.midi.port) + ')'); }
    }
    catch (e) { $('#cerr').textContent = e.message; }
  };
  $$('#menu button').forEach(b => b.onclick = () => {
    A.synth.init(); const k = b.dataset.go;
    A.track('arena_menu', { option: k });
    if (k === 'rank') return showRank();
    A.sel.mode = k;
    go(k === 'pvp' ? 'lobby' : 'play');
  });
  A.on('fw', refresh);
  if (A.dev) devPanel();
  renderTop();
};
function tryRejoin() {        // saiu no meio de uma luta e reabriu a aba: o CPU devolve os personagens
  let last = null; try { last = JSON.parse(localStorage.getItem('arena.last')); } catch (e) {}
  if (!last || Date.now() - last.ts > 120000 || (A.net.room && A.net.room.id === last.room) || A.rejoining) return;
  A.rejoining = true; A.net.start();
  const un = A.on('net:resume', (m) => { un(); A.rejoining = false; A.net.room = Object.assign({}, m.room); A.resume = m; A.sel.mode = 'pvp'; go('play'); });
  let tries = 0; const iv = setInterval(() => { A.net.send({ t: 'rejoin', room: last.room, person: A.net.person }, last.host); if (++tries > 12 || !A.rejoining) { clearInterval(iv); un(); A.rejoining = false; } }, 2000);   // (o P2P leva alguns segundos para achar o dono)
  A.net.send({ t: 'rejoin', room: last.room, person: A.net.person }, last.host);
}
function showRank() {
  const box = $('#rankbox'); box.hidden = false; let kind = 'conns';
  const draw = () => {
    let rows;
    if (kind === 'ver') rows = A.rank.versions().slice(0, 5).map((r, i) => `<tr><td>${i + 1}</td><td>${A.fwLogo(r.id, 20)}</td><td>${esc(r.k)}</td><td class="n">${r.n}</td></tr>`);
    else rows = A.rank.sorted(kind).map((r, i) => `<tr><td>${i + 1}</td><td>${A.fwLogo(r.id, 20)}</td><td>${esc(A.FW[r.id].name)}${r.id === 'quest' && kind === 'conns' && A.rank.switched() ? ` <small style="color:var(--dim)">(+${A.rank.switched()} ${esc(A.t('rk_sw'))})</small>` : ''}</td><td class="n">${r.n}</td></tr>`);
    $('#rkbody').innerHTML = `<table class="rk"><tr><th>#</th><th></th><th>${A.t('th_fw')}</th><th class="n">${A.t(kind === 'wins' ? 'th_wins' : 'th_conns')}</th></tr>${rows.join('')}</table>`;
    $('#rkdemo').textContent = A.cfg.service ? '' : A.t('rk_local');
  };
  $$('.tabs button', box).forEach(b => b.onclick = () => { kind = b.dataset.k; $$('.tabs button', box).forEach(x => x.classList.toggle('on', x === b)); draw(); });
  draw(); box.scrollIntoView({ behavior: 'smooth' });
}
function devPanel() {          // só em ?dev=1: um "FM1 virtual" clicável para testar sem o aparelho (não existe no jogo)
  if ($('#dev')) return; const d = document.createElement('div'); d.id = 'dev';
  const ks = [[53, 'F'], [55, 'G'], [57, 'A'], [59, 'B'], [60, 'C'], [62, 'D'], [64, 'E'], [65, 'F']];
  d.innerHTML = `${A.t('dev')} <select id="fakefw">${Object.values(A.FW).map(f => `<option value="${f.id}">${f.name}</option>`).join('')}</select>
    <select id="fakech"><option value="0">T1</option><option value="1">T2</option><option value="2">T3</option><option value="3">T4</option></select>
    <br><span id="fk">${ks.map(([n, l], i) => `<button data-n="${n}" data-i="${i}">${n - 52}</button>`).join('')}</span> <button id="fkhold">hold</button><br>${A.t('dev_k')}`;
  document.body.append(d); const s = $('#fakefw'); s.value = A.myFw() || 'quest';
  s.onchange = () => { localStorage.setItem('arena.fakefw', s.value); renderTop(); };
  const ch = () => +$('#fakech').value; let hold = false; const down = {};
  $('#fkhold').onclick = (e) => { hold = !hold; e.target.style.background = hold ? '#f0c860' : ''; };
  $$('#fk button').forEach(b => {
    const n = +b.dataset.n;
    b.onpointerdown = () => { down[n] = 1; A.emit('note', { n, v: 100, ch: ch(), t: performance.now(), on: true }); if (!hold) b.onpointerup = b.onpointerleave = () => { if (down[n]) { down[n] = 0; A.emit('note', { n, v: 0, ch: ch(), t: performance.now(), on: false }); } }; else { b.onpointerup = b.onpointerleave = null; setTimeout(() => { down[n] = 0; A.emit('note', { n, v: 0, ch: ch(), t: performance.now(), on: false }); }, 1800); } };
  });
}

/* ---------- salas ---------- */
let arenaThumbs = null;
async function stageThumbs(host, sel, onPick) {
  if (!A.stages) A.stages = await (await fetch('stages.json')).json();
  host.innerHTML = '';
  Object.entries(A.stages).forEach(([id, st]) => {
    const b = document.createElement('button'); b.className = id === sel ? 'on' : ''; b.title = st.name;
    b.innerHTML = `<canvas width="160" height="90"></canvas><span>${esc(st.name)}</span>`;
    const c = b.firstChild.getContext('2d'); c.imageSmoothingEnabled = false; const sc = 90 / st.h;
    st.layers.forEach(l => { if (l.name === 'nuvens') return; const i = new Image(); i.onload = () => { if (l.loop) for (let x = 0; x < 160; x += l.w * sc) c.drawImage(i, x, 0, l.w * sc, l.h * sc); else c.drawImage(i, 0, l.y * sc, l.w * sc, l.h * sc); }; i.src = 'img/stages/' + l.src.split('/').pop(); });
    b.onclick = () => { $$('button', host).forEach(x => x.classList.toggle('on', x === b)); onPick(id); }; host.append(b);
  });
}
A.screens.lobby = function () {
  if (!A.ready()) return go('menu');
  A.sel.mode = 'pvp';
  A.net.start(); renderTop();
  const size = () => A.sel.size;
  $$('#sizes button').forEach(b => { b.classList.toggle('on', +b.dataset.n === size()); b.onclick = () => { A.sel.size = +b.dataset.n; $$('#sizes button').forEach(x => x.classList.toggle('on', x === b)); draw(); }; });
  $('#back').onclick = () => { A.net.leave(); go('menu'); };
  const cb = $('#create');
  const setCreated = (on) => { cb.classList.toggle('flag', on); cb.dataset.i = on ? 'lb_enter_room' : 'lb_create'; cb.textContent = A.t(cb.dataset.i); };
  setCreated(!!(A.net.room && A.net.room.host === A.net.id));
  cb.onclick = () => {
    if (A.net.room && A.net.room.host === A.net.id) { go('play'); return; }          // já criou: agora entra na sala
    A.track('arena_create', { size: size() }); A.net.create(size()); drawMine(); setCreated(true);
  };
  $('#quick').onclick = () => {
    const open = A.net.open(size())[0]; A.track('arena_quick', { size: size(), found: !!open });
    if (open) tryJoin(open);
    else flash(A.t('lb_nofree'));
  };
  const flash = (txt) => { const h = $('#rooms'); h.insertAdjacentHTML('afterbegin', '<p class="help" id="nofree">' + txt + '</p>'); setTimeout(() => { const n = $('#nofree'); n && n.remove(); }, 3500); };
  function tryJoin(r) {          // entra só depois que o dono confirma (a sala pode ter enchido ou sumido)
    A.net.join(r); A.track('arena_join', { room: r.id });
    let done = false; const fail = () => { if (done) return; done = true; clean(); A.net.room = null; flash(A.t('lb_full')); };
    const u1 = A.on('rooms', () => { const rr = A.net.rooms[r.id]; if (!done && rr && rr.players.some(p => p.id === A.net.id)) { done = true; clean(); go('play'); } });
    const u2 = A.on('full', fail), to = setTimeout(fail, 6000);
    function clean() { u1(); u2(); clearTimeout(to); }
  }
  const draw = () => {
    const own = A.net.room && A.net.room.host === A.net.id && A.net.room.size === size() ? [A.net.room] : [];
    const rs = own.concat(Object.values(A.net.rooms).filter(r => r.size === size() && r.id !== (own[0] && own[0].id)));
    $('#rooms').innerHTML = rs.length ? rs.map(r => `<div class="room"><div><b>${esc(r.name)}</b> <span class="pill">${r.size === 2 ? '1×1' : '2×2'}</span> <span class="pill">${esc((A.stages && A.stages[r.arena] && A.stages[r.arena].name) || '')}</span></div>
      <div class="pl">${r.players.map(p => A.fwLogo(p.fw, 18)).join('')}<span class="pill">${r.players.length}/${r.size}</span></div>
      ${r.host === A.net.id ? `<span class="pill">${A.t('lb_you')}</span>` : `<button class="btn gold" data-j="${r.id}" ${r.players.length >= r.size || r.phase !== 'wait' ? 'disabled' : ''}>${A.t(r.phase !== 'wait' ? 'lb_fight' : 'lb_enter')}</button>`}</div>`).join('')
      : '<p class="help">' + A.t('lb_none') + '</p>';
    $$('#rooms [data-j]').forEach(b => b.onclick = () => { const r = A.net.rooms[b.dataset.j]; if (!r) return; tryJoin(r); });
  };
  const drawMine = async () => {
    const m = $('#mine'), r = A.net.room;
    if (!r || r.host !== A.net.id) { m.hidden = true; return; }
    m.hidden = false;
    if (r.players.length >= r.size) { return go('play'); }
    m.innerHTML = `<h2>${A.t('lb_you')}</h2><p>${A.t('lb_wait', { n: r.size - r.players.length })}</p>
      <div class="row" style="justify-content:flex-start"><input type="text" id="rname" value="${esc(r.name)}" maxlength="24"><button class="btn" id="rsave">${A.t('lb_rename')}</button><button class="btn" id="rleave">${A.t('lb_leave')}</button></div>
      <p class="help">${A.t('lb_arena')}</p><div class="arenas" id="ars"></div>`;
    $('#rsave').onclick = () => { r.name = $('#rname').value.trim() || r.name; A.net.send({ t: 'room', room: { id: r.id, name: r.name, size: r.size, arena: r.arena, host: r.host, phase: r.phase, players: r.players } }); };
    $('#rleave').onclick = () => { A.net.leave(); m.hidden = true; setCreated(false); };
    stageThumbs($('#ars'), r.arena, (id) => { r.arena = id; });
  };
  A.on('rooms', draw); A.on('roomupdate', drawMine); draw(); drawMine();
  if (!A._lobbyTimer) A._lobbyTimer = setInterval(() => { if (cur === 'lobby') A.emit('rooms'); }, 1000);          // a lista se atualiza sozinha (salas novas e salas que sumiram)
  stageThumbs(document.createElement('div'), '', () => {});     // pré-carrega stages.json
  if (A.dev) devPanel();
};

/* ---------- luta (cpu / pvp / treino) ---------- */
A.screens.play = async function () {
  if (!A.ready()) return go('menu');
  A.synth.init(); renderTop(); if (game) { game.destroy(); game = null; } if (train) { train.destroy(); train = null; }
  const mode = A.sel.mode, cv = $('#cv'), ctl = $('#ctl'), help = $('#help'), ov = $('#overlay'), ovc = $('#ovc');
  const me = A.myFw();
  if (!A.stages) A.stages = await (await fetch('stages.json')).json();
  if (A.dev) devPanel();
  $('#hud').style.display = mode === 'train' ? 'none' : '';
  const forget = () => { try { localStorage.removeItem('arena.last'); } catch (e) {} };
  const back = () => { forget(); A.net.leave(); if (game) game.destroy(); if (train) train.destroy(); game = train = null; go(mode === 'pvp' ? 'lobby' : 'menu'); };
  const mkBtn = (txt, fn, gold) => { const b = document.createElement('button'); b.className = 'btn' + (gold ? ' gold' : ''); b.textContent = txt; b.onclick = fn; ctl.append(b); return b; };
  ctl.innerHTML = '';
  const arenaPick = document.createElement('div'); arenaPick.className = 'arenas'; arenaPick.style.maxWidth = '100%';

  if (mode === 'train') {
    $('#stagebox').style.background = '#14101a';
    train = new A.Training({ canvas: cv });
    help.innerHTML = A.t('tr_help'); 
    mkBtn(A.t('r_back'), back);
    return;
  }

  /* monta partida */
  let players, size = 2, stageId = '96', hostFlag = true, room = A.net.room;
  const resume = A.resume; A.resume = null;
  if (mode === 'cpu') {
    players = [{ id: A.net.id, fw: me, side: 0, chars: [0, 1] }, { id: 'cpu', fw: 'other', side: 1, chars: [0, 1], cpu: true }];
  } else if (resume) {
    room = A.net.room; size = room.size; stageId = room.arena; hostFlag = false; players = resume.players;
  } else {
    if (!room) return go('lobby');
    size = room.size; stageId = room.arena; hostFlag = room.host === A.net.id;
    players = room.players.map((p, i) => ({ id: p.id, person: p.person, fw: p.fw, side: i % 2, chars: size === 4 ? (i < 2 ? [0, 1] : [2, 3]) : [0, 1] }));
  }
  const mp = players.find(p => p.id === A.net.id) || players[0];
  const fwSides = [(players.find(p => p.side === 0) || {}).fw || 'other', (players.find(p => p.side === 1) || {}).fw || 'other'];
  let acc = new Set(); let started = !!resume;
  const remember = () => { if (mode === 'pvp' && A.net.room) { try { localStorage.setItem('arena.last', JSON.stringify({ room: A.net.room.id, host: A.net.room.host, ts: Date.now() })); } catch (e) {} } };
  const names = [A.t('side_a'), A.t('side_b')];
  $('#wa').innerHTML = A.fwLogo(fwSides[0], 18) + `<span>${names[0]}</span>`;
  $('#wb').innerHTML = A.fwLogo(fwSides[1], 18) + `<span>${names[1]}</span>`;
  const chosen = { id: stageId };
  game = new A.Game({
    canvas: cv, mode, stage: stageId, me: { side: mp.side, chars: mp.chars, id: A.net.id }, players, host: hostFlag, fw: fwSides,
    cpuSloppy: 90, resume: resume ? resume.s : null,
    onFrame: hud,
    onEnd: (s) => finish(s),
  });
  const g = game; A.game = g;
  let rec = null, chunks = [], recErr = null, recOn = false, endAt = 0, stopped = Promise.resolve();
  const RW = 768, RH = 72 + 432 + 36, rc = document.createElement('canvas'); rc.width = RW; rc.height = RH;
  const rx = rc.getContext('2d'); rx.imageSmoothingEnabled = false;
  const logoImg = {}; const lg = (id) => { const f = A.FW[id]; if (!f || !f.logo) return null; if (!logoImg[id]) { const im = new Image(); im.src = f.logo; logoImg[id] = im; } return logoImg[id]; };
  const rtxt = (t, x, y, col, sz, al) => { rx.font = `600 ${sz}px "Pixelify Sans",ui-monospace,monospace`; rx.textAlign = al || 'left'; rx.lineJoin = 'round'; rx.lineWidth = 4; rx.strokeStyle = '#000'; rx.strokeText(t, x, y); rx.fillStyle = col; rx.fillText(t, x, y); };
  function drawRec(gm) {
    if (!recOn) return; const st = gm.s;
    rx.fillStyle = '#1e1418'; rx.fillRect(0, 0, RW, RH);
    const bar = (x, y, w, h, frac, col, right) => {
      rx.fillStyle = '#000'; rx.fillRect(x - 3, y - 3, w + 6, h + 6); rx.fillStyle = '#2a1f26'; rx.fillRect(x, y, w, h);
      const fw = Math.round(w * Math.max(0, Math.min(1, frac))); rx.fillStyle = col; rx.fillRect(right ? x + w - fw : x, y, fw, h);
      rx.fillStyle = '#ffffff40'; rx.fillRect(right ? x + w - fw : x, y, fw, Math.max(2, h / 4));
      rx.fillStyle = '#0008'; for (let i = 12; i < w; i += 12) rx.fillRect(x + i, y, 1, h);
    };
    for (let sd = 0; sd < 2; sd++) {
      const x = sd ? RW - 8 - 330 : 8, id = fwSides[sd], im = lg(id);
      if (im && im.complete && im.naturalWidth) { const h = 22, w = Math.min(110, im.naturalWidth * h / im.naturalHeight); rx.fillStyle = '#000'; rx.fillRect(sd ? RW - 8 - w : 8, 6, w, h); rx.drawImage(im, sd ? RW - 8 - w : 8, 6, w, h); }
      else rtxt((A.FW[id] || {}).name || '', sd ? RW - 8 : 8, 24, '#f0c860', 14, sd ? 'right' : 'left');
      rtxt(A.t(sd ? 'side_b' : 'side_a'), sd ? RW - 130 : 130, 22, '#f2e2c4', 13, sd ? 'right' : 'left');
      bar(x, 34, 330, 16, st.hp[sd] / 100, '#4ac86a', sd === 1);
      bar(x, 56, 330, 7, Math.min(100, st.ult[sd]) / 100, '#f0c860', sd === 1);
    }
    rtxt(st.ph === 'wait' ? '--' : Math.ceil(st.total) + 's', RW / 2, 30, '#fff', 24, 'center');
    rx.drawImage(cv, 0, 72, 768, 432);
    rx.fillStyle = '#000'; rx.fillRect(0, 504, RW, 36);
    rtxt(A.t('wm').toUpperCase(), RW / 2, 529, '#f0c860', 16, 'center');
    if (endAt) { const t = (performance.now() - endAt); rx.fillStyle = 'rgba(0,0,0,.55)'; rx.fillRect(0, 72, RW, 432); rtxt(A.t('wm'), RW / 2, 72 + 216 + 8, '#f0c860', 34, 'center'); rtxt('fm1.quest', RW / 2, 72 + 216 + 44, '#fff', 20, 'center'); }
  }
  A.rec = { canvas: rc, draw: (gm) => { recOn = true; drawRec(gm); } };      // (usado pela captura offline do vídeo de apresentação)
  function startRec() {
    try {
      if (!window.MediaRecorder) throw new Error('sem MediaRecorder');
      recOn = true;
      const vs = rc.captureStream(30), as = A.synth.dest ? A.synth.dest.stream : null;
      const ms = new MediaStream([...vs.getVideoTracks(), ...(as ? as.getAudioTracks() : [])]);
      const mt = ['video/mp4;codecs=avc1.42E01E,mp4a.40.2', 'video/mp4;codecs=avc1', 'video/mp4', 'video/webm;codecs=vp9,opus', 'video/webm;codecs=vp8,opus', 'video/webm'].find(t => MediaRecorder.isTypeSupported(t));
      rec = new MediaRecorder(ms, { mimeType: mt, videoBitsPerSecond: 3000000 }); chunks = [];
      rec.ondataavailable = (e) => e.data.size && chunks.push(e.data); rec.start(500);
    } catch (e) { recErr = e.message; }
  }
  /* a gravação final só leva os turnos de tocar (e o sorteio e o fim): corta a preparação e a espera de cada lado, com o som fazendo fade na saída e na volta */
  function gateRec(ph) {
    if (!rec || rec.state === 'inactive' || !A.synth.ctx) return;
    const want = ph === 'turn' || ph === 'dice' || ph === 'end', gn = A.synth.rec.gain, t = A.synth.ctx.currentTime;
    if (want && rec.state === 'paused') { try { rec.resume(); } catch (e) {} gn.cancelScheduledValues(t); gn.setValueAtTime(Math.max(0.0001, gn.value), t); gn.linearRampToValueAtTime(.6, t + .35); }
    else if (!want && rec.state === 'recording' && !gateRec.pending) {
      gn.cancelScheduledValues(t); gn.setValueAtTime(gn.value, t); gn.linearRampToValueAtTime(0, t + .35);
      gateRec.pending = setTimeout(() => { gateRec.pending = 0; if (rec && rec.state === 'recording' && !(g.s.ph === 'turn' || g.s.ph === 'dice' || g.s.ph === 'end')) { try { rec.pause(); } catch (e) {} } }, 380);
    }
  }
  g.rec = startRec;
  help.innerHTML = A.t('pl_help', { c: mp.chars.length === 2 ? A.t('pl_c2') : '1-4' });

  function drawControls() {
    ctl.innerHTML = '';
    mkBtn(A.t('pl_exit'), back);
    if (g.s.ph !== 'wait') return;
    if (mode === 'cpu') { mkBtn(A.t('pl_call'), () => { g.start(); drawControls(); }, true); return; }
    if (!hostFlag) { const p = document.createElement('span'); p.className = 'help'; p.textContent = A.t('pl_wh'); ctl.append(p); return; }
    const full = A.net.room.players.length >= size;
    const b = mkBtn(full ? A.t('pl_call') : A.t('pl_wp', { a: A.net.room.players.length, b: size }), () => { A.net.room.phase = 'call'; acc = new Set([A.net.id]); A.net.send({ t: 'call', players: A.net.room.players.map((p, i) => ({ id: p.id, person: p.person, fw: p.fw, side: i % 2, chars: size === 4 ? (i < 2 ? [0, 1] : [2, 3]) : [0, 1] })) }); A.track('arena_call'); checkAcc(); showWait(); }, true);
    b.disabled = !full;
    if (!arenaPick.children.length || true) { stageThumbs(arenaPick, g.stage ? g.stage._id : stageId, (id) => { A.net.room.arena = id; g.loadStage(id); g.layoutIdle(true); }); ctl.append(arenaPick); }
  }
  const showWait = () => { ov.classList.add('on'); ovc.innerHTML = '<h2>' + A.t('pl_calling') + '</h2><p>' + A.t('pl_all') + '</p>'; };
  const checkAcc = () => { if (hostFlag && A.net.room && acc.size >= A.net.room.players.length) { ov.classList.remove('on'); A.net.send({ t: 'go' }); begin(); } };
  const begin = () => { if (started) return; started = true; ov.classList.remove('on'); g.start(); if (mode === 'pvp') A.rank.entered(me, A.midi.ver); if (!hostFlag) { /* convidado: o host manda o estado */ } };
  if (mode === 'pvp') {
    A.on('roomupdate', drawControls);
    A.on('rooms', () => { if (!hostFlag && A.net.rooms[A.net.room.id]) { A.net.room.players = A.net.rooms[A.net.room.id].players; } });
    A.on('net:call', (m) => { if (hostFlag) return; g.players = m.players; ov.classList.add('on'); ovc.innerHTML = '<h2>' + A.t('pl_called') + '</h2><div class="row"><button class="btn gold" id="ok">' + A.t('pl_accept') + '</button><button class="btn" id="no">' + A.t('pl_exit') + '</button></div>';
      $('#ok').onclick = () => { A.net.send({ t: 'acc' }, A.net.room.host); ovc.innerHTML = '<h2>' + A.t('pl_others') + '</h2>'; }; $('#no').onclick = back; });
    A.on('net:acc', (m) => { if (hostFlag) { acc.add(m.from); checkAcc(); } });
    A.on('net:gone', (m) => { if (hostFlag && game === g && g.s.ph !== 'wait' && g.s.ph !== 'end') g.markGone(m.from); });
    A.on('net:rejoin', (m) => {
      const pl = hostFlag && game === g && A.net.room && m.room === A.net.room.id && g.players.find(p => p.person && p.person === m.person);
      if (!pl) return;
      const old = pl.id; pl.id = m.from; (A.net.room.players.find(p => p.id === old) || {}).id = m.from;       // a pessoa voltou numa aba nova: troca o id de rede
      g.markBack(m.from); const r = A.net.room;
      A.net.send({ t: 'resume', room: { id: r.id, name: r.name, size: r.size, arena: r.arena, host: r.host, phase: r.phase, players: r.players },
        players: g.players.map(p => ({ id: p.id, fw: p.fw, side: p.side, chars: p.chars, cpu: p.cpu, gone: p.gone })), s: g.s }, m.from);
    });
    A.on('net:go', () => { if (!hostFlag) { started = true; ov.classList.remove('on'); g.s.ph = 'dice'; g.rec && g.rec(); A.rank.entered(me, A.midi.ver); } });
    A.net.shareAudio();
    A.on('net:stream', (m) => { A.net.shareAudio(); });
  }
  function finish(s) {
    forget();
    A.net.ch && mode === 'pvp' && A.net.send({ t: 'st', s: g.s });
    const w = s.hp[0] === s.hp[1] ? -1 : s.hp[0] > s.hp[1] ? 0 : 1;
    if (mode === 'pvp' && w >= 0 && w === mp.side) A.rank.win(me);       // cada vencedor soma uma vitória para o seu firmware
    A.track('arena_end', { mode, winner: w, hp0: Math.round(s.hp[0]), hp1: Math.round(s.hp[1]) });
    if (rec && rec.state !== 'inactive') {                    // mais 2,4 s com o cartão final "Filmado no FM1.Quest"
      endAt = performance.now();
      stopped = new Promise(res => setTimeout(() => { rec.onstop = () => { recOn = false; res(); }; try { rec.stop(); } catch (e) { recOn = false; res(); } }, 2400));
    }
    const mine = mp.side;
    ov.classList.add('on');
    ovc.innerHTML = `<h2>${A.t(w < 0 ? 'r_draw' : w === mine ? 'r_win' : 'r_lose')}</h2><p>${A.t('r_life')}: ${Math.round(s.hp[0])} × ${Math.round(s.hp[1])}</p>
      <div class="row"><button class="btn gold save" id="sv">${A.t('r_save')}</button><button class="btn gold share" id="sh">${A.t('r_share')}</button><button class="btn" id="bk">${A.t('r_back')}</button></div>
      <p class="help">${A.t('r_note')}</p><p class="help" id="vnote"></p>`;
    $('#bk').onclick = back;
    const build = () => {
      if (!chunks.length) { alert(A.t('r_nogr') + (recErr ? ' (' + recErr + ')' : '') + '.'); return null; }
      const type = (chunks[0].type || 'video/mp4').split(';')[0], ext = /mp4/.test(type) ? 'mp4' : 'webm';
      const blob = new Blob(chunks, { type }), f = new File([blob], 'fm1-arena.' + ext, { type });
      if (ext !== 'mp4') $('#vnote').textContent = A.t('r_webm');
      return { blob, f, ext };
    };
    $('#sv').onclick = async () => {
      await stopped; const v = build(); if (!v) return; A.track('arena_save', { ext: v.ext });
      const a = document.createElement('a'); a.href = URL.createObjectURL(v.blob); a.download = 'fm1-arena.' + v.ext; a.click();
      setTimeout(() => { URL.revokeObjectURL(a.href); finishDelete(); }, 1500);
    };
    $('#sh').onclick = async () => {
      await stopped; const v = build(); if (!v) return; A.track('arena_share', { ext: v.ext });
      try {
        if (navigator.canShare && navigator.canShare({ files: [v.f] })) { await navigator.share({ files: [v.f], title: A.t('sh_t'), text: A.t('sh_x') }); return finishDelete(); }
      } catch (e) { if (e && e.name === 'AbortError') return; }
      $('#vnote').textContent = A.t('r_nosh');
      const a = document.createElement('a'); a.href = URL.createObjectURL(v.blob); a.download = 'fm1-arena.' + v.ext; a.click();
    };
    const finishDelete = () => { chunks = []; };
  }
  function hud(gm) {
    drawRec(gm); const s = gm.s; gateRec(s.ph);
    if (mode === 'pvp') {
      const nw = performance.now();
      if (!gm._rem || nw - gm._rem > 4000) { gm._rem = nw; if (s.ph !== 'end') remember(); }
      if (!hostFlag && gm.lastSt && nw - gm.lastSt > 6000 && s.ph !== 'end' && !gm._hl) {      // o dono saiu: a luta acaba
        gm._hl = 1; forget(); ov.classList.add('on'); ovc.innerHTML = '<h2>' + A.t('host_left') + '</h2><div class="row"><button class="btn" id="hb">' + A.t('r_back') + '</button></div>'; $('#hb').onclick = back;
      }
    }
    $('#hpa').style.width = Math.max(0, s.hp[0]) + '%'; $('#hpb').style.width = Math.max(0, s.hp[1]) + '%';
    $('#ua').style.width = Math.min(100, s.ult[0]) + '%'; $('#ub').style.width = Math.min(100, s.ult[1]) + '%';
    $('#clock').firstChild.nodeValue = s.ph === 'wait' ? '--' : Math.ceil(s.total) + 's';
    const labels = { wait: A.t('ph_wait'), dice: A.t('ph_dice'), prep: A.t('ph_prep'), count: A.t('ph_count'), turn: A.t('ph_turn'), end: A.t('ph_end') };
    $('#phase').textContent = (labels[s.ph] || '') + (s.ph === 'turn' || s.ph === 'count' ? ` · ${s.turn + 1}/4` : '');
    const ring = $('#ring'), act = s.ph === 'turn' || s.ph === 'count' || s.ph === 'prep';
    ring.style.display = act ? 'block' : 'none';
    if (act) {
      ring.style.left = (s.side ? 75 : 25) + '%';
      const dur = s.ph === 'turn' ? 25 : s.ph === 'prep' ? 10 : 60 / s.bpm * 4;
      $('#rc').style.strokeDashoffset = (169.6 * (1 - s.tl / dur)).toFixed(1);
      $('#rn').textContent = s.ph === 'count' ? Math.max(1, 4 - Math.floor((1 - s.tl / dur) * 4)) : Math.ceil(s.tl);
    }
    $('#msg').textContent = '';
    if (!gm._ctl || gm._ctl !== s.ph) { gm._ctl = s.ph; if (!started && s.ph !== 'wait' && !hostFlag) { started = true; ov.classList.remove('on'); g.rec && g.rec(); } if (s.ph === 'end' && !gm.ended && !hostFlag) { gm.ended = true; finish(s); } drawControls(); }
  }
  drawControls();
  A.on('roomupdate', () => { if (game === g && g.s.ph === 'wait') drawControls(); });
};

/* ---------- inicialização ---------- */
function sky() {
  if ($('#sky')) return; const d = document.createElement('div'); d.id = 'sky';
  [[7, 3, 120, .9, 0], [20, 2, 170, .7, -60], [34, 4, 95, .8, -30], [12, 1.5, 230, .55, -150]].forEach(([top, k, dur, op, delay]) => {
    const i = document.createElement('i'); i.style.cssText = `top:${top}%;width:${330 * k}px;height:${36 * k}px;background-image:url(img/clouds.png);background-size:100% 100%;opacity:${op};animation-duration:${dur}s;animation-delay:${delay}s`; d.append(i); });
  document.body.insertBefore(d, $('#app'));
}
function boot() {
  sky(); A.applyI18n(); buildMosaic(); covered(true);
  $('#mlogo').textContent = 'FM1 Arena';
  // abertura da arena: o mosaico já começa cobrindo a tela (inclusive vindo do banner do site) e se abre com GSAP
  show('menu');
  const run = () => reveal();
  if (window.gsap) { gsap.set(cells, { scale: 1.04 }); gsap.set('#mlogo', { autoAlpha: 1 }); setTimeout(run, 550); } else run();
}
if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', boot); else boot();
})();
