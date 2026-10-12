/* Banner da FM1 Arena para o site (teste local). Clique abre a arena; × fecha. Eventos vão para o Google Analytics. */
(function () {
  const base = (document.currentScript && document.currentScript.src || '').replace(/banner\.js.*$/, '') || 'arena/';
  if (window.__arenaBanner) return; window.__arenaBanner = 1;
  const SUP = ['pt', 'en', 'es', 'ja', 'ko'], norm = (s) => { s = String(s || '').toLowerCase().slice(0, 2); return SUP.includes(s) ? s : null; };
  const lang = (() => { let l = norm(document.documentElement.lang); if (l) return l; try { l = norm(localStorage.getItem('fm1lang')); if (l) return l; } catch (e) {}
    for (const x of (navigator.languages || [navigator.language])) { l = norm(x); if (l) return l; } return 'en'; })();
  const CAP = {
    en: 'Show off your synthesizer skills and challenge other players—even those on different firmware versions!',
    pt: 'Mostre suas habilidades no sintetizador e desafie outros jogadores, mesmo os que usam versões diferentes de firmware!',
    es: '¡Muestra tus habilidades con el sintetizador y desafía a otros jugadores, incluso a quienes usan otras versiones de firmware!',
    ja: 'シンセの腕前を披露して、別のファームウェアバージョンを使うプレイヤーにも挑戦しよう！',
    ko: '신디사이저 실력을 뽐내고, 다른 펌웨어 버전을 쓰는 플레이어에게도 도전하세요!' };
  const ALT = { en: 'v0.21 is out. FM1 ARENA IS HERE!', pt: 'v0.21 já saiu. A FM1 ARENA CHEGOU!', es: 'v0.21 ya salió. ¡LA ARENA FM1 YA LLEGÓ!', ja: 'v0.21 公開中。FM1 アリーナ登場！', ko: 'v0.21 출시. FM1 아레나 등장!' };
  const CLOSE = { en: 'Close', pt: 'Fechar', es: 'Cerrar', ja: '閉じる', ko: '닫기' };
  try { if (sessionStorage.getItem('arena.banner.closed')) return; } catch (e) {}
  const ga = (n, p) => { try { if (typeof gtag === 'function') gtag('event', n, p || {}); } catch (e) {} };
  const css = document.createElement('style');
  css.textContent = `
  #arnb{position:fixed;left:14px;top:50%;transform:translateY(-50%);z-index:130;width:210px;font:12px/1.35 ui-monospace,Menlo,monospace;color:#fff;
    transition:transform .5s cubic-bezier(.16,1,.3,1),opacity .4s}
  #arnb.off{transform:translate(-250px,-50%);opacity:0;pointer-events:none}
  #arnb a{display:block;position:relative;border:0;box-shadow:-4px 0 0 0 #000,4px 0 0 0 #000,0 -4px 0 0 #000,0 4px 0 0 #000,-8px 0 0 0 #d0463c,8px 0 0 0 #d0463c,0 -8px 0 0 #d0463c,0 8px 0 0 #d0463c,10px 12px 0 0 #000a;background:#000;overflow:hidden;aspect-ratio:3/4;cursor:pointer}
  #arnb img{width:100%;height:100%;object-fit:cover;object-position:50% 38%;image-rendering:pixelated;display:block;transition:transform .5s cubic-bezier(.16,1,.3,1)}
  @media(hover:hover){#arnb a:hover img{transform:scale(1.07)}}
  #arnb button{position:absolute;top:-10px;right:-10px;z-index:2;width:30px;height:30px;border:0;border-radius:0;background:#fff;padding:0;cursor:pointer;image-rendering:pixelated;box-shadow:-3px 0 0 0 #000,3px 0 0 0 #000,0 -3px 0 0 #000,0 3px 0 0 #000,6px 6px 0 0 #0008}
  #arnb button:hover{background:#f0c860}
  #arnb button svg{display:block;width:100%;height:100%;shape-rendering:crispEdges}
  #arnb p{margin:14px 2px 0;color:#fff;text-shadow:-1px 0 #000,1px 0 #000,0 -1px #000,0 1px #000;font:400 13px/1.3 'Pixelify Sans',ui-monospace,monospace}
  @media(max-width:700px){#arnb{width:140px;left:8px;top:auto;bottom:168px;transform:none}#arnb.off{transform:translateX(-190px)}#arnb p{display:none}}
  #arncur{position:fixed;inset:0;z-index:300;background:#14101a;pointer-events:none;clip-path:circle(0% at var(--cx,10%) var(--cy,50%));transition:clip-path .6s cubic-bezier(.16,1,.3,1)}
  #arncur.on{clip-path:circle(150% at var(--cx,10%) var(--cy,50%))}`;
  const ff = document.createElement('style'); ff.textContent = '@font-face{font-family:"Pixelify Sans";src:url(' + base + 'fonts/PixelifySans-400.ttf)}'; document.head.append(ff, css);
  const el = document.createElement('aside'); el.id = 'arnb'; el.setAttribute('aria-label', 'FM1 Arena');
  el.innerHTML = `<button type="button" aria-label="${CLOSE[lang]}"><svg viewBox="0 0 9 9"><path fill="#14101a" d="M0 0h2v2H0zM7 0h2v2H7zM1 1h2v2H1zM6 1h2v2H6zM2 2h2v2H2zM5 2h2v2H5zM3 3h3v3H3zM2 5h2v2H2zM5 5h2v2H5zM1 6h2v2H1zM6 6h2v2H6zM0 7h2v2H0zM7 7h2v2H7z"/></svg></button><a href="${base}"><img src="${base}img/banner.${lang}.webp" alt="${ALT[lang]}"></a>
    <p>${CAP[lang]}</p>`;
  document.body.append(el);
  ga('arena_banner_view');
  el.querySelector('button').onclick = (e) => { e.preventDefault(); e.stopPropagation(); el.classList.add('off'); try { sessionStorage.setItem('arena.banner.closed', '1'); } catch (x) {} ga('arena_banner_close'); };
  el.querySelector('a').onclick = (e) => {
    e.preventDefault(); ga('arena_banner_click'); ga('arena_page_change', { to: 'arena' });
    const go = () => { location.href = base; };
    if (!window.gsap) { go(); return; }
    // mosaico de pixels (GSAP) que sai do banner e cobre a página; a arena abre já coberta e o desfaz
    const m = document.createElement('div'); m.id = 'arnmos';
    m.style.cssText = 'position:fixed;inset:0;z-index:400;display:grid;grid-template-columns:repeat(16,1fr);grid-template-rows:repeat(9,1fr);pointer-events:all';
    const cols = ['#1e1418', '#2a1f26', '#3a2620', '#1e1418', '#5a3a28', '#1e1418', '#c8963c'], cells = [];
    for (let i = 0; i < 144; i++) { const d = document.createElement('i'); d.style.cssText = 'display:block;background:' + cols[(Math.imul(i + 1, 2654435761) >>> 24) % cols.length]; m.append(d); cells.push(d); }
    document.body.append(m);
    const r = el.getBoundingClientRect(), cx = Math.min(15, Math.max(0, Math.floor((r.left + r.width / 2) / innerWidth * 16))), cy = Math.min(8, Math.max(0, Math.floor((r.top + r.height / 2) / innerHeight * 9)));
    gsap.fromTo(cells, { scale: 0 }, { scale: 1.04, duration: .32, ease: 'expo.out', stagger: { each: .014, grid: [9, 16], from: cy * 16 + cx }, onComplete: go });
  };
})();
