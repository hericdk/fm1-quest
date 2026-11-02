/* FM1.quest: the floating recorder. Mirrors the simulator's little screen (a 240x240 canvas), records it (with sound) as a square video, and lets you
   trim it, fade the sound in/out and add the fm1.quest logo at the end. Everything runs in the browser (MediaRecorder + WebAudio): no server, no ffmpeg. */
(function () {
  'use strict';
  if (window.__fmrec) return; window.__fmrec = 1;
  const cs = document.currentScript;
  const ROOT = cs && cs.src ? cs.src.replace(/assets\/js\/fmrec\.js.*$/, '') : '';
  let L = ((document.documentElement.dataset.l || document.documentElement.lang || 'en') + '').slice(0, 2);
  const S = {
    title: { pt: 'FM1.quest', en: 'FM1.quest', es: 'FM1.quest' },
    rec: { pt: 'Gravar a telinha', en: 'Record the little screen', es: 'Grabar la pantallita' },
    stop: { pt: 'Parar', en: 'Stop', es: 'Parar' },
    sim: { pt: 'Simulador', en: 'Simulator', es: 'Simulador' },
    none: { pt: 'Sem som', en: 'No sound', es: 'Sin sonido' },
    ph: { pt: 'Fonte do som', en: 'Sound source', es: 'Fuente' },
    find: { pt: 'USB…', en: 'USB…', es: 'USB…' },
    srct: { pt: 'De onde vem o som', en: 'Where the sound comes from', es: 'De dónde viene el sonido' },
    kin: { pt: 'INÍCIO', en: 'START', es: 'INICIO' },
    kout: { pt: 'FIM', en: 'END', es: 'FIN' },
    kfade: { pt: 'FADE', en: 'FADE', es: 'FUNDIDO' },
    logo: { pt: 'logo', en: 'logo', es: 'logo' },
    logot: { pt: 'Mostrar o logo fm1.quest no fim', en: 'Show the fm1.quest logo at the end', es: 'Mostrar el logo fm1.quest al final' },
    play: { pt: 'Ver / pausar', en: 'Play / pause', es: 'Ver / pausar' },
    dl: { pt: 'Baixar', en: 'Download', es: 'Descargar' },
    drop: { pt: 'Descartar e gravar de novo', en: 'Discard and record again', es: 'Descartar y grabar de nuevo' },
    busy: { pt: 'Gerando…', en: 'Rendering…', es: 'Generando…' },
    min: { pt: 'Recolher', en: 'Collapse', es: 'Contraer' },
  };
  const t = (k) => (S[k] && (S[k][L] || S[k].en)) || k;
  const IC = {                                                          /* pixel icons, 12 x 12, centred */
    play: '<svg viewBox="0 0 12 12" width="12" height="12" shape-rendering="crispEdges"><path d="M3 1h2v1h1v1h1v1h1v1h1v2h-1v1h-1v1h-1v1h-1v1h-2z"/></svg>',
    pause: '<svg viewBox="0 0 12 12" width="12" height="12" shape-rendering="crispEdges"><path d="M2 1h3v10h-3zM7 1h3v10h-3z"/></svg>',
    trash: '<svg viewBox="0 0 12 12" width="12" height="12" shape-rendering="crispEdges"><path d="M4 0h4v1h3v2h-10v-2h3zM2 4h8v8h-8zM4 5v6h1v-6zM7 5v6h1v-6z" fill-rule="evenodd"/></svg>',
    dl: '<svg viewBox="0 0 12 12" width="12" height="12" shape-rendering="crispEdges"><path d="M5 0h2v5h2v1h-1v1h-1v1h-2v-1h-1v-1h-1v-1h2zM0 8h2v2h8v-2h2v4h-12z"/></svg>',
  };

  /* ---------- DOM ---------- */
  const root = document.createElement('aside'); root.id = 'fmrec'; root.className = 'noedit'; root.setAttribute('aria-label', 'FM1.quest recorder');
  root.innerHTML = `<div class="fr-stage">
    <div class="fr-win"><div class="fr-title"><span>${t('title')}</span><i><u data-a="min" title="${t('min')}">_</u><u>&#9643;</u><u data-a="min">×</u></i></div></div>
    <canvas class="fr-view" width="240" height="240" title="${t('play')}"></canvas>
    <canvas class="fr-tl" width="240" height="14" title="${t('kin')} · ${t('kout')} · ${t('kfade')}"></canvas>
    <div class="fr-edit"><button type="button" data-a="play" title="${t('play')}">${IC.pause}</button><button type="button" data-a="drop" title="${t('drop')}">${IC.trash}</button><span class="fr-sp"></span><button type="button" data-a="dl" class="fr-dl" title="${t('dl')} (${t('logot').toLowerCase()})">${IC.dl}</button></div>
    <div class="fr-busy"><span>${t('busy')}</span></div>
    <div class="fr-ctl"><button type="button" class="fr-rec" data-a="rec" title="${t('rec')}" aria-label="${t('rec')}"><i></i></button><span class="fr-time">00:00</span>
      <canvas class="fr-meter" width="26" height="12"></canvas>
      <div class="fr-sel"><button type="button" class="fr-selb" aria-haspopup="listbox"><span></span><i>▼</i></button><ul class="fr-list" role="listbox"></ul></div>
      <select class="fr-src" hidden title="${t('srct')}"><option value="sim">${t('sim')}</option><option value="none">${t('none')}</option><option value="find">${t('find')}</option></select></div>
    <div class="fr-knobs"></div>
  </div>`;
  document.body.append(root);
  const $ = (s) => root.querySelector(s);
  const stage = $('.fr-stage'), view = $('.fr-view'), vctx = view.getContext('2d'), recBtn = $('.fr-rec'), timeEl = $('.fr-time'),
    meter = $('.fr-meter'), mctx = meter.getContext('2d'), srcSel = $('.fr-src'), knobsEl = $('.fr-knobs'), playBtn = $('[data-a=play]'), tl = $('.fr-tl'), tctx = tl.getContext('2d');
  vctx.imageSmoothingEnabled = false;
  const rc = document.createElement('canvas'); rc.width = rc.height = 720; const rctx = rc.getContext('2d'); rctx.imageSmoothingEnabled = false;   // what is recorded: square

  /* ---------- the simulator's little screen ---------- */
  const simW = () => { try { const f = document.getElementById('simframe'); return (f && f.contentWindow && f.contentWindow.FM1W) || null; } catch (e) { return null; } };
  function paint(ctx, size) {
    ctx.fillStyle = '#000'; ctx.fillRect(0, 0, size, size);
    const W = simW();
    if (!W || !W.screen) return false;
    try { if (W.vid && W.vid.isConnected && W.vid.readyState >= 2) ctx.drawImage(W.vid, 0, 0, size, size); } catch (e) {}   // (the preview clip, until the simulator is switched on)
    try { ctx.drawImage(W.screen, 0, 0, size, size); } catch (e) {}
    return true;
  }

  /* ---------- sound ---------- */
  let ac = null, mix = null, dest = null, an = null, srcNode = null, usbStream = null, src = 'sim', anBuf = null;
  function ensureAudio() {
    if (ac) return;
    ac = new (window.AudioContext || window.webkitAudioContext)();
    mix = ac.createGain(); dest = ac.createMediaStreamDestination(); an = ac.createAnalyser(); an.fftSize = 256; anBuf = new Uint8Array(an.fftSize);
    mix.connect(dest); mix.connect(an);
  }
  async function setSource(kind, devId) {
    if (srcNode) { try { srcNode.disconnect(); } catch (e) {} srcNode = null; }
    if (usbStream) { usbStream.getTracks().forEach((x) => x.stop()); usbStream = null; }
    src = kind; if (kind === 'none') return;
    ensureAudio();
    if (kind === 'sim') { const W = simW(); if (W && W.tap) { srcNode = ac.createMediaStreamSource(W.tap.stream); srcNode.connect(mix); } }
    else if (kind === 'usb') {
      usbStream = await navigator.mediaDevices.getUserMedia({ audio: { deviceId: { exact: devId }, echoCancellation: false, noiseSuppression: false, autoGainControl: false, channelCount: 2 } });
      srcNode = ac.createMediaStreamSource(usbStream); srcNode.connect(mix);
    }
  }
  async function findDevices() {
    try {
      const tmp = await navigator.mediaDevices.getUserMedia({ audio: true }); tmp.getTracks().forEach((x) => x.stop());      // (only to unlock the device names)
      const devs = (await navigator.mediaDevices.enumerateDevices()).filter((d) => d.kind === 'audioinput');
      srcSel.innerHTML = `<option value="sim">${t('sim')}</option><option value="none">${t('none')}</option>` + devs.map((d) => `<option value="usb:${d.deviceId}">${(d.label || 'USB').replace(/</g, '')}</option>`).join('') + `<option value="find">${t('find')}</option>`;
      const pick = devs.find((d) => /fm-?1|m-?vave|felucca|sloop|baud/i.test(d.label));
      srcSel.value = pick ? 'usb:' + pick.deviceId : 'sim';
    } catch (e) { srcSel.value = 'sim'; }
    srcSel.dispatchEvent(new Event('change'));
  }
  srcSel.addEventListener('change', async () => {
    const v = srcSel.value; srcSel.title = t('srct') + ': ' + (srcSel.selectedOptions[0] ? srcSel.selectedOptions[0].textContent : ''); ensureAudio(); if (ac.state === 'suspended') ac.resume().catch(() => {});
    if (v === 'find') { await findDevices(); return; }
    try { if (v.startsWith('usb:')) await setSource('usb', v.slice(4)); else await setSource(v); } catch (e) { srcSel.value = 'sim'; await setSource('sim'); }
  });
  setInterval(() => { if (src === 'sim' && !srcNode && ac) setSource('sim'); }, 1000);          // the simulator's sound exists after the first click on its screen

  /* ---------- recording ---------- */
  const pickMime = () => ['video/mp4;codecs=avc1.42E01E,mp4a.40.2', 'video/mp4;codecs=avc1', 'video/mp4', 'video/webm;codecs=vp9,opus', 'video/webm;codecs=vp8,opus', 'video/webm']
    .find((m) => window.MediaRecorder && MediaRecorder.isTypeSupported(m)) || '';
  let R = null, mode = 'live', clip = null;          // R: the running recording; clip: { url, dur, tin, tout, fade, video }
  const fmt = (s) => String(Math.floor(s / 60)).padStart(2, '0') + ':' + String(Math.floor(s % 60)).padStart(2, '0');
  async function startRec() {
    ensureAudio(); await ac.resume().catch(() => {});
    if (!usbStream && !srcNode && src !== 'none') await setSource(src).catch(() => {});
    const mt = pickMime();
    const stream = new MediaStream([...rc.captureStream(30).getVideoTracks(), ...dest.stream.getAudioTracks()]);
    const mr = new MediaRecorder(stream, mt ? { mimeType: mt, videoBitsPerSecond: 3000000 } : {});
    const r = R = { mr, chunks: [], t0: performance.now(), type: mt };
    mr.ondataavailable = (e) => { if (e.data && e.data.size) r.chunks.push(e.data); };
    mr.onstop = () => finishRec(r);
    mr.start(1000); root.classList.add('rec'); recBtn.title = t('stop');
  }
  function stopRec() { if (!R) return; const r = R; r.dur = (performance.now() - r.t0) / 1000; try { r.mr.stop(); } catch (e) { finishRec(r); } }
  function finishRec(r) {
    if (R === r) R = null; root.classList.remove('rec'); recBtn.title = t('rec'); timeEl.textContent = '00:00';
    if (!r.chunks.length) return;
    const type = (r.chunks[0].type || r.type || 'video/mp4').split(';')[0];
    const blob = new Blob(r.chunks, { type }), url = URL.createObjectURL(blob), v = document.createElement('video');
    v.src = url; v.playsInline = true; v.preload = 'auto'; v.loop = false;
    const dur = Math.max(1, r.dur || 1);
    clip = { url, blob, ext: /mp4/.test(type) ? 'mp4' : 'webm', dur, tin: 0, tout: dur, fade: 0.6, video: v };
    v.addEventListener('loadedmetadata', () => { if (!isFinite(v.duration)) { v.currentTime = 1e7; v.addEventListener('timeupdate', function f() { v.removeEventListener('timeupdate', f); v.currentTime = 0; }); } });
    enterReview();
  }
  function enterReview() {
    mode = 'review'; root.classList.remove('noedit', 'min'); root.classList.add('review'); $('.fr-edit').style.display = 'flex'; setKnobs(); keepOnScreen(); if (window.gsap && !REDUCE) gsap.fromTo('.fr-k', { scale: 0.6 }, { scale: 1, duration: 0.5, ease: 'back.out(3)', stagger: 0.08, clearProps: 'transform' });
    clip.video.currentTime = clip.tin; clip.video.play().catch(() => {}); playBtn.innerHTML = IC.pause;
  }
  function leaveReview() {
    if (clip) { try { clip.video.pause(); } catch (e) {} URL.revokeObjectURL(clip.url); clip = null; }
    mode = 'live'; root.classList.add('noedit'); root.classList.remove('review'); $('.fr-edit').style.display = 'none';
  }
  $('.fr-edit').style.display = 'none';

  /* ---------- knobs (IN / OUT / FADE) ---------- */
  const K = {};
  const REDUCE = window.matchMedia && matchMedia('(prefers-reduced-motion:reduce)').matches;
  function mkKnob(id, label, get, set, min, max, step) {
    const el = document.createElement('div'); el.className = 'fr-k'; el.tabIndex = 0; el.setAttribute('role', 'slider');
    const pt = (deg, r) => { const q = (deg - 90) * Math.PI / 180; return [(22 + r * Math.cos(q)).toFixed(2), (22 + r * Math.sin(q)).toFixed(2)]; };
    const arc = (d0, d1, r) => { const [x0, y0] = pt(d0, r), [x1, y1] = pt(d1, r); return `M${x0} ${y0}A${r} ${r} 0 ${d1 - d0 > 180 ? 1 : 0} 1 ${x1} ${y1}`; };
    let ticks = ''; for (let i = 0; i <= 10; i++) { const [x0, y0] = pt(-135 + 27 * i, 19.5), [x1, y1] = pt(-135 + 27 * i, 22); ticks += `<path d="M${x0} ${y0}L${x1} ${y1}"/>`; }
    el.innerHTML = `<svg viewBox="0 0 44 44" width="44" height="44" aria-hidden="true"><g class="kt">${ticks}</g><path class="kb" d="${arc(-135, 135, 16)}"/><path class="kv" d=""/><circle class="kc" cx="22" cy="22" r="10"/><g class="kn"><path d="M22 22V11.5"/></g></svg><span>${label}</span><em></em>`;
    knobsEl.append(el);
    const kv = el.querySelector('.kv'), kn = el.querySelector('.kn'), em = el.querySelector('em'); let shown = null;
    const draw = () => {
      const v = get(), a = -135 + 270 * ((v - min()) / Math.max(0.001, max() - min()));
      kv.setAttribute('d', a > -134 ? arc(-135, a, 16) : '');
      if (window.gsap && !REDUCE) { gsap.to(kn, { rotation: a, svgOrigin: '22 22', duration: 0.22, ease: 'power3.out', overwrite: true });
        if (shown !== null && Math.abs(v - shown) > 0.01) gsap.fromTo(em, { scale: 1.35 }, { scale: 1, duration: 0.25, ease: 'back.out(3)', overwrite: true }); }
      else kn.setAttribute('transform', `rotate(${a} 22 22)`);
      shown = v; em.textContent = v.toFixed(1) + 's'; el.setAttribute('aria-valuenow', v.toFixed(1));
    };
    const bump = (d) => { set(Math.max(min(), Math.min(max(), get() + d))); redrawKnobs(); };
    let y0 = null, v0 = 0;
    el.addEventListener('pointerdown', (e) => { el.setPointerCapture(e.pointerId); y0 = e.clientY; v0 = get(); e.preventDefault(); });
    el.addEventListener('pointermove', (e) => { if (y0 == null) return; const d = (y0 - e.clientY) / 80 * (max() - min()); set(Math.max(min(), Math.min(max(), v0 + d))); redrawKnobs(); });
    el.addEventListener('pointerup', () => { y0 = null; });
    el.addEventListener('wheel', (e) => { e.preventDefault(); bump(e.deltaY < 0 ? step : -step); }, { passive: false });
    el.addEventListener('keydown', (e) => { if (e.key === 'ArrowUp' || e.key === 'ArrowRight') { bump(step); e.preventDefault(); } else if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') { bump(-step); e.preventDefault(); } });
    K[id] = draw; return el;
  }
  const redrawKnobs = () => Object.values(K).forEach((f) => f());
  function setKnobs() { redrawKnobs(); }
  mkKnob('in', t('kin'), () => (clip ? clip.tin : 0), (v) => { if (clip) { clip.tin = Math.min(v, clip.tout - 0.5); try { clip.video.currentTime = clip.tin; } catch (e) {} } }, () => 0, () => (clip ? Math.max(0.5, clip.tout - 0.5) : 1), 0.2);
  mkKnob('out', t('kout'), () => (clip ? clip.tout : 0), (v) => { if (clip) clip.tout = Math.max(v, clip.tin + 0.5); }, () => (clip ? clip.tin + 0.5 : 0), () => (clip ? clip.dur : 1), 0.2);
  mkKnob('fade', t('kfade'), () => (clip ? clip.fade : 0), (v) => { if (clip) clip.fade = v; }, () => 0, () => 3, 0.2);
  redrawKnobs();

  /* ---------- export: re-record the trimmed clip (real time) with fades and the logo ---------- */
  let logoImg = null;
  const loadLogo = () => new Promise((res) => { if (logoImg) return res(logoImg); const i = new Image(); i.onload = () => res(logoImg = i); i.onerror = () => res(null); i.src = ROOT + 'assets/logo.png'; });
  const every = (fn) => new Promise((res) => { const id = setInterval(() => { let done = true; try { done = fn(); } catch (e) {} if (done) { clearInterval(id); res(); } }, 33); });   // a timer, not rAF: keeps going in a background tab
  async function exportClip() {
    if (!clip || root.classList.contains('busy')) return;
    root.classList.add('busy'); try { clip.video.pause(); } catch (e) {}
    const c = clip, mt = pickMime(), withLogo = true, logo = await loadLogo();   /* (the logo always closes the clip) */
    const v = document.createElement('video'); v.src = c.url; v.playsInline = true; v.preload = 'auto';
    await new Promise((res) => { v.onloadeddata = res; v.load(); });
    if (!isFinite(v.duration)) { v.currentTime = 1e7; await new Promise((res) => { v.ontimeupdate = () => { v.ontimeupdate = null; res(); }; setTimeout(res, 1500); }); }
    ensureAudio(); await ac.resume().catch(() => {});
    const a2 = new (window.AudioContext || window.webkitAudioContext)(), d2 = a2.createMediaStreamDestination(), g = a2.createGain(); g.gain.value = 0; await a2.resume().catch(() => {});
    try { a2.createMediaElementSource(v).connect(g); g.connect(d2); } catch (e) {}
    const cv = document.createElement('canvas'); cv.width = cv.height = 720; const x = cv.getContext('2d'); x.imageSmoothingEnabled = false;
    const rec = new MediaRecorder(new MediaStream([...cv.captureStream(30).getVideoTracks(), ...d2.stream.getAudioTracks()]), mt ? { mimeType: mt, videoBitsPerSecond: 3000000 } : {});
    const chunks = []; rec.ondataavailable = (e) => { if (e.data && e.data.size) chunks.push(e.data); };
    const done = new Promise((res) => { rec.onstop = res; });
    v.currentTime = c.tin; await new Promise((res) => { v.onseeked = () => { v.onseeked = null; res(); }; setTimeout(res, 800); });
    const len = c.tout - c.tin, fade = Math.min(c.fade, len / 2), now = a2.currentTime + 0.05;
    g.gain.cancelScheduledValues(0); g.gain.setValueAtTime(fade > 0 ? 0 : 1, now);
    if (fade > 0) { g.gain.linearRampToValueAtTime(1, now + fade); g.gain.setValueAtTime(1, now + len - fade); g.gain.linearRampToValueAtTime(0, now + len); }
    rec.start(500); await v.play().catch(() => {});
    await every(() => { x.fillStyle = '#000'; x.fillRect(0, 0, 720, 720); try { x.drawImage(v, 0, 0, 720, 720); } catch (e) {}
      if (v.currentTime >= c.tout - 0.03 || v.ended) { v.pause(); return true; } return false; });
    if (logo) {                                                           // the fm1.quest logo, 1.8 s, silence
      g.gain.setValueAtTime(0, a2.currentTime);
      const t0 = performance.now();
      await every(() => { const p = (performance.now() - t0) / 1800;
        x.fillStyle = '#1e1418'; x.fillRect(0, 0, 720, 720);
        const w = 520, h = w * logo.naturalHeight / logo.naturalWidth; x.globalAlpha = Math.min(1, p * 4); x.drawImage(logo, (720 - w) / 2, 250 - h / 2, w, h);
        x.font = '700 44px "Pixelify Sans",monospace'; x.textAlign = 'center'; x.fillStyle = '#f0c860'; x.fillText('fm1.quest', 360, 560); x.globalAlpha = 1;
        return p >= 1; });
    }
    rec.stop(); await done; try { a2.close(); } catch (e) {}
    const type = (chunks[0] && chunks[0].type || mt || 'video/mp4').split(';')[0], ext = /mp4/.test(type) ? 'mp4' : 'webm';
    const a = document.createElement('a'); a.href = URL.createObjectURL(new Blob(chunks, { type })); const d = new Date(), p2 = (n) => String(n).padStart(2, '0');
    a.download = `fm1quest-tela-${d.getFullYear()}${p2(d.getMonth() + 1)}${p2(d.getDate())}-${p2(d.getHours())}${p2(d.getMinutes())}.${ext}`;
    document.body.append(a); a.click(); setTimeout(() => { URL.revokeObjectURL(a.href); a.remove(); }, 4000);
    root.classList.remove('busy');
    try { c.video.currentTime = c.tin; c.video.play(); } catch (e) {}
    try { (window.gtag || (() => {}))('event', 'fmrec_download', { ext, logo: !!withLogo }); } catch (e) {}
  }

  /* ---------- buttons ---------- */
  root.addEventListener('click', (e) => {
    const b = e.target.closest('[data-a]'); if (!b) { if (e.target === view && mode === 'review') playBtn.click(); return; }
    const a = b.dataset.a;
    if (a === 'rec') { if (R) stopRec(); else { if (mode === 'review') leaveReview(); startRec().catch(() => {}); } }
    else if (a === 'min') { root.classList.toggle('min'); keepOnScreen(); }
    else if (a === 'play' && clip) { if (clip.video.paused) { if (clip.video.currentTime >= clip.tout - 0.05) clip.video.currentTime = clip.tin; clip.video.play(); playBtn.innerHTML = IC.pause; } else { clip.video.pause(); playBtn.innerHTML = IC.play; } }
    else if (a === 'dl') exportClip();
    else if (a === 'drop') leaveReview();
  });

  /* ---------- size, position and dragging (the window can be moved anywhere; the spot is remembered in this browser) ---------- */
  const POSK = 'fmrec.pos';
  let px = null, py = null;
  const fit = () => { const v = Math.max(80, Math.min(112, Math.floor((innerHeight - 16) / 3.2))); root.style.setProperty('--v', (innerWidth < 760 ? Math.min(v, 96) : v) + 'px'); };
  function place(x, y) {
    const w = root.offsetWidth, h = stage.offsetHeight || root.offsetHeight;
    px = Math.round(Math.max(4, Math.min(innerWidth - w - 4, x))); py = Math.round(Math.max(4, Math.min(innerHeight - h - 4, y)));
    root.style.left = px + 'px'; root.style.top = py + 'px';
  }
  function keepOnScreen() { if (px != null) place(px, py); }
  function homePos() { const w = root.offsetWidth, h = root.offsetHeight; place(innerWidth - w - 16, (innerHeight - h) / 2); }
  fit();
  let saved = null; try { saved = JSON.parse(localStorage.getItem(POSK) || 'null'); } catch (e) {}
  if (saved && isFinite(saved.x) && isFinite(saved.y)) place(saved.x, saved.y); else homePos();
  let moved = !!saved;
  addEventListener('resize', () => { fit(); if (moved) keepOnScreen(); else homePos(); });
  let dg = null;
  root.addEventListener('pointerdown', (e) => {
    if (e.button || e.target.closest('button,select,input,label,.fr-k,.fr-edit,u')) return;
    if (e.target === view && mode === 'review') return;
    dg = { x: e.clientX, y: e.clientY, px, py, on: false, id: e.pointerId };
  });
  addEventListener('pointermove', (e) => {
    if (!dg || e.pointerId !== dg.id) return;
    const dx = e.clientX - dg.x, dy = e.clientY - dg.y;
    if (!dg.on) { if (Math.abs(dx) + Math.abs(dy) < 4) return; dg.on = true; root.classList.add('drag'); try { root.setPointerCapture(dg.id); } catch (er) {} }
    place(dg.px + dx, dg.py + dy); moved = true;
  });
  const endDrag = (e) => {
    if (!dg || (e && e.pointerId !== dg.id)) return;
    if (dg.on) { try { localStorage.setItem(POSK, JSON.stringify({ x: px, y: py })); } catch (er) {} root.classList.remove('drag'); }
    dg = null;
  };
  addEventListener('pointerup', endDrag); addEventListener('pointercancel', endDrag);

  /* ---------- the sound-source box (a readable 98-style list; the hidden <select> keeps the logic) ---------- */
  const selB = $('.fr-selb'), selL = $('.fr-list'), selT = selB.firstElementChild;
  let picked = false;
  const syncSel = () => { const o = srcSel.selectedOptions[0]; selT.textContent = picked && o ? o.textContent : t('ph'); selB.classList.toggle('ph', !picked); selB.title = t('srct') + (picked && o ? ': ' + o.textContent : ''); };
  function closeList() { root.classList.remove('open'); }
  function openList() {
    selL.innerHTML = ''; [...srcSel.options].forEach((o) => { const li = document.createElement('li'); li.textContent = o.textContent; li.setAttribute('role', 'option');
      if (o.value === srcSel.value) li.className = 'on'; li.addEventListener('click', () => { picked = true; srcSel.value = o.value; srcSel.dispatchEvent(new Event('change')); syncSel(); closeList(); }); selL.append(li); });
    root.classList.add('open');
    if (window.gsap && !REDUCE) gsap.fromTo(selL.children, { x: -8, opacity: 0 }, { x: 0, opacity: 1, duration: 0.18, stagger: 0.03, ease: 'steps(4)', overwrite: true });
  }
  selB.addEventListener('click', () => (root.classList.contains('open') ? closeList() : openList()));
  addEventListener('pointerdown', (e) => { if (!e.target.closest || !e.target.closest('.fr-sel')) closeList(); });
  addEventListener('keydown', (e) => { if (e.key === 'Escape') closeList(); });
  srcSel.addEventListener('change', syncSel); setInterval(syncSel, 600); syncSel();
  addEventListener('fm1-lang', () => {                                    // the page switched EN / PT: the widget follows
    L = ((document.documentElement.dataset.l || 'en') + '').slice(0, 2);
    [...srcSel.options].forEach((o) => { if (o.value === 'sim') o.textContent = t('sim'); else if (o.value === 'none') o.textContent = t('none'); else if (o.value === 'find') o.textContent = t('find'); });
    const ks = knobsEl.querySelectorAll('.fr-k span'), kk = ['kin', 'kout', 'kfade']; ks.forEach((e, i) => { if (kk[i]) e.textContent = t(kk[i]); });
    recBtn.title = t('rec'); playBtn.title = t('play'); $('[data-a=drop]').title = t('drop'); $('[data-a=dl]').title = t('dl'); syncSel();
  });

  /* ---------- opening: the window unfolds like an old dialog (gsap, steps keep it pixelly) ---------- */
  if (window.gsap && !REDUCE) {
    gsap.from(root, { scaleY: 0.04, scaleX: 0.6, opacity: 0, transformOrigin: '50% 0%', duration: 0.45, ease: 'steps(7)', delay: 0.4, clearProps: 'transform,opacity' });
    gsap.from('.fr-k', { y: 14, opacity: 0, duration: 0.5, ease: 'back.out(2.2)', stagger: 0.09, delay: 0.9, clearProps: 'transform,opacity' });
  }

  /* ---------- frame loop ---------- */
  function loop() {
    requestAnimationFrame(loop);
    tick();
  }
  function drawTL() {
    const W2 = 240, H2 = 14, c = clip; tctx.fillStyle = '#000'; tctx.fillRect(0, 0, W2, H2);
    if (!c) return;
    const X = (s) => Math.round(s / c.dur * W2), a = X(c.tin), b = X(c.tout), f = Math.max(1, X(c.tin + c.fade) - a);
    tctx.fillStyle = '#303040'; tctx.fillRect(0, 3, W2, 8);                          // the whole take
    tctx.fillStyle = '#000080';                                                       // kept: the fades are its slopes
    for (let x = a; x < b; x++) { const k = Math.min(1, (x - a) / f, (b - x) / f), h = Math.max(1, Math.round(12 * k)); tctx.fillRect(x, 13 - h, 1, h); }
    tctx.fillStyle = '#c0c0c0'; tctx.fillRect(a, 0, 1, H2); tctx.fillRect(b - 1, 0, 1, H2);   // the cuts
    tctx.fillStyle = '#fff'; tctx.fillRect(X(c.video.currentTime || 0), 0, 1, H2);           // where it plays
  }
  function tick() {
    if (mode === 'review' && clip) drawTL();
    if (mode === 'review' && clip) {
      const v = clip.video;
      if (v.currentTime >= clip.tout - 0.03 && !v.paused) { v.currentTime = clip.tin; }
      try { vctx.drawImage(v, 0, 0, 240, 240); } catch (e) {}
    } else paint(vctx, 240);
    if (R) { paint(rctx, 720); const s = (performance.now() - R.t0) / 1000; timeEl.textContent = fmt(s); if (s > 180) stopRec(); }
    if (an) {                                                                         // level meter: 6 pixel bars
      an.getByteTimeDomainData(anBuf); let m = 0; for (let i = 0; i < anBuf.length; i++) m = Math.max(m, Math.abs(anBuf[i] - 128));
      const n = Math.min(6, Math.round(m / 128 * 12)); mctx.fillStyle = '#000'; mctx.fillRect(0, 0, 26, 12);
      for (let i = 0; i < 6; i++) { mctx.fillStyle = i < n ? (i > 3 ? '#e0503c' : '#7ab04e') : '#222'; mctx.fillRect(1 + i * 4, 2, 3, 8); }
    }
  }
  requestAnimationFrame(loop);
  setInterval(() => { if (document.hidden) tick(); }, 50);                            // hidden tab: rAF stops, the recording must not
})();
