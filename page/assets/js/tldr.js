/* TL;DR: what went into each category. One mini FM-1 (2D) for the whole block: the tabs only change the content, the buttons change the screen. */
(function () {
  var root = document.getElementById("tl"); if (!root) return;
  var L = (document.documentElement.lang || "pt").slice(0, 2); if (L !== "en" && L !== "es") L = "pt";
  var B = L === "pt" ? "" : "../";
  var T = {
    howh: { pt: "Como acessar", en: "How to reach it", es: "Cómo acceder" },
    times: { pt: "= quantas vezes você aperta", en: "= how many times you press", es: "= cuántas veces pulsas" },
    hold: { pt: "segure", en: "hold", es: "mantén" }, turn: { pt: "gire", en: "turn", es: "gira" }, menu: { pt: "menu", en: "menu", es: "menú" },
    keys: { pt: "teclas", en: "keys", es: "teclas" }, site: { pt: "no site", en: "on the site", es: "en el sitio" }, screen: { pt: "tela", en: "screen", es: "pantalla" }
  };
  /* item: [title en, pt, es, line en, pt, es, screens]; screen: [media, label?, how-to tokens] */
  var C = [
   ["rpg", "Combat and RPG", "Combate e RPG", "Combate y RPG", [
    ["ULTIMATE bar with a pixel-art animation", "Barra de ULTIMATE com animação em pixel art", "Barra de ULTIMATE con animación en pixel art",
     "Every note fills the bar; when full, the hero unleashes a 10 s animation.", "Cada nota enche a barra; cheia, o herói solta uma animação de 10 s.", "Cada nota llena la barra; al llenarse, el héroe lanza una animación de 10 s.",
     [["boss_goblin", "", [["PLAY"], ["KEYS"]]]]],
    ["Ult modes (side window, fullscreen, off)", "Modos da ult (janela, tela cheia, desligada)", "Modos de la ult (ventana, pantalla completa, apagada)",
     "Choose how the ultimate shows up.", "Escolha como a ultimate aparece.", "Elige cómo aparece la ultimate.",
     [["boss_goblin", "SIDE", [["hold", "HOME"], ["menu", "ULT ANIMATION"]]], ["tl_ult0", "FULLSCREEN", [["hold", "HOME"], ["menu", "ULT ANIMATION"]]]]],
    ["The boss's broken defence system", "O sistema de defesa quebrada do chefe", "El sistema de defensa rota del jefe",
     "After the ult hits, the shield cracks and damage goes up.", "Depois da ult, o escudo racha e o dano aumenta.", "Tras la ult, el escudo se agrieta y el daño sube.",
     [["boss_ult", "", [["PLAY"], ["KEYS"]]]]],
    ["New monsters (slimes, mini slimes, bats, goblins)", "Monstros novos (slimes, mini slimes, morcegos, goblins)", "Monstruos nuevos (slimes, mini slimes, murciélagos, goblins)",
     "They walk the map in sequence; you set how many in MAP ENEMIES.", "Passam pelo mapa em sequência; MAP ENEMIES define quantos.", "Pasan por el mapa en secuencia; MAP ENEMIES define cuántos.",
     [["map", "", [["HOME", 2]]]]],
    ["New bosses (slime, wyvern, goblin) with BOSS SPEED", "Chefes novos (slime, wyvern, goblin) com BOSS SPEED", "Jefes nuevos (slime, wyvern, goblin) con BOSS SPEED",
     "Recording a loop is a boss fight; one setting controls all speeds.", "Gravar um loop é luta de chefe; uma opção controla todas as velocidades.", "Grabar un loop es una pelea de jefe; una opción controla las velocidades.",
     [["boss_slime_blue", "SLIME", [["REC"]]], ["boss_wyvern", "WYVERN", [["REC"]]], ["boss_goblin", "GOBLIN", [["REC"]]]]],
    ["Damage on monsters and damage numbers", "Dano nos monstros e números de dano", "Daño en los monstruos y números de daño",
     "Every hit shows its number (option DAMAGE NUMBERS).", "Cada golpe mostra o número (opção DAMAGE NUMBERS).", "Cada golpe muestra su número (opción DAMAGE NUMBERS).",
     [["boss_slime_blue", "", [["hold", "HOME"], ["menu", "DAMAGE NUMBERS"]]]]],
    ["Combo system: hit the keys in rhythm and you damage the boss too", "Sistema de combo: bata nas teclas no ritmo e você também dá dano no chefe", "Sistema de combo: toca las teclas al ritmo y tú también dañas al jefe",
     "The steadier your rhythm, the bigger your hit.", "Quanto mais constante o ritmo, maior o seu golpe.", "Cuanto más constante el ritmo, mayor tu golpe.",
     [["boss_slime_blue", "", [["KEYS"]]]]],
    ["Roles, buffs, healing and debuff (cleric)", "Papéis, buffs, cura e debuff (clériga)", "Roles, buffs, curación y debuff (clériga)",
     "Warrior front line, mage damage, archer agility, cleric buffs.", "Guerreiro na frente, mago com dano, arqueiro ágil, clériga com buffs.", "Guerrero al frente, mago con daño, arquero ágil, clériga con buffs.",
     [["map", "", [["HOME"]]]]],
    ["Menu › Party and two secret classes (the first is the bard)", "Menu › Party e duas classes secretas (a primeira é o bardo)", "Menu › Party y dos clases secretas (la primera es el bardo)",
     "Swap the heroes in your party.", "Troque os heróis do seu grupo.", "Cambia los héroes de tu grupo.",
     [["classes", "", [["hold", "HOME"], ["menu", "PARTY"]]]]],
    ["Unlockable red warrior (your second one, after the first boss: the achievement window appears)", "Guerreiro vermelho desbloqueável (o segundo, depois do primeiro chefe: aparece a janela de conquista)", "Guerrero rojo desbloqueable (el segundo, tras el primer jefe: aparece la ventana de logro)",
     "Beat your first boss to unlock him.", "Derrote o primeiro chefe para liberar.", "Derrota al primer jefe para desbloquearlo.",
     [["ult_warrior", "", [["REC"]]]]]
   ]],
   ["world", "World and visuals", "Mundo e visual", "Mundo y visual", [
    ["Living village with a day and night cycle", "Vila viva com ciclo de dia e noite", "Aldea viva con ciclo de día y noche",
     "96 s from dawn to late night.", "96 s do amanhecer à madrugada.", "96 s del amanecer a la madrugada.",
     [["village", "", [["HOME", 1]]]]],
    ["Villagers with randomness patterns (like a mini AI)", "Moradores com padrões de aleatoriedade (como uma mini IA)", "Aldeanos con patrones de aleatoriedad (como una mini IA)",
     "Each one decides when to leave, walk, run and chat.", "Cada um decide quando sair, andar, correr e conversar.", "Cada uno decide cuándo salir, caminar, correr y charlar.",
     [["village", "", [["HOME", 1]]]]],
    ["The heroes camp: the elf mage fills the area with flowers, the knight trains, the others walk, train and talk", "O grupo acampa: a maga enche a área de flores, o cavaleiro treina, os outros andam, treinam e conversam", "El grupo acampa: la maga llena la zona de flores, el caballero entrena, los demás caminan, entrenan y charlan",
     "Stop the music and they rest.", "Pare a música e eles descansam.", "Para la música y descansan.",
     [["camp", "", [["PLAY"]]]]],
    ["REC is a campfire with a tiny FM-1 (the picture is small; it improves in the next version)", "REC virou uma fogueira com um mini FM-1 (a imagem ficou pequena; melhora na próxima versão)", "REC es una fogata con un mini FM-1 (la imagen es pequeña; mejora en la próxima versión)",
     "The fire breathes; red eyes blink while recording.", "O fogo respira; olhos vermelhos piscam gravando.", "El fuego respira; ojos rojos parpadean al grabar.",
     [["rec", "", [["REC"]]]]],
    ["FX as places (stompboxes, slicer, forge, sea, torches)", "FX como lugares (pedaleiras, slicer, forja, mar, tochas)", "FX como lugares (pedales, slicer, fragua, mar, antorchas)",
     "Each effect page is a scene that reacts to it.", "Cada página de efeito é uma cena que reage a ele.", "Cada página de efecto es una escena que reacciona.",
     [["fx_pedals", "PEDALS", [["FX", 1]]], ["slicer_towers", "SLICER", [["FX", 2]]], ["scale", "SCALE", [["SCL"]]]]],
    ["Visualizers (Sky, Tides, Ring, Canyon, Scope)", "Visualizers (Sky, Tides, Ring, Canyon, Scope)", "Visualizers (Sky, Tides, Ring, Canyon, Scope)",
     "They follow each character's level.", "Seguem o nível de cada personagem.", "Siguen el nivel de cada personaje.",
     [["viz_sky", "SKY", [["hold", "HOME"], ["menu", "VISUALIZER"]]], ["viz_tides", "TIDES", [["hold", "HOME"], ["menu", "VISUALIZER"], ["turn", "PRESETS"]]], ["viz_canyon", "CANYON", [["hold", "HOME"], ["menu", "VISUALIZER"], ["turn", "PRESETS"]]]]],
    ["New title screen with sound!!", "Tela de título nova, com som!!", "¡Pantalla de título nueva, con sonido!",
     "It shows when you switch on.", "Aparece ao ligar o aparelho.", "Aparece al encender.",
     [["tl_title", "", [["POWER"]]]]],
    ["Colour filters", "Filtros de cor", "Filtros de color",
     "Game Boy, 1 bit, VHS, Noir...", "Game Boy, 1 bit, VHS, Noir...", "Game Boy, 1 bit, VHS, Noir...",
     [["filters", "", [["hold", "HOME"], ["menu", "FILTER"]]]]]
   ]],
   ["sound", "Sound and music", "Som e música", "Sonido y música", [
    ["26 synthesized Brazilian percussion sounds", "26 sons de percussão brasileira sintetizados", "26 sonidos de percusión brasileña sintetizados",
     "Kits BRASIL and BRASIL 2, made from scratch.", "Kits BRASIL e BRASIL 2, feitos do zero.", "Kits BRASIL y BRASIL 2, hechos desde cero.",
     [["tl_kit", "", [["turn", "ALGORITHM"], ["SEQ"]]]]],
    ["Cavaquinho, agogô, accordion and berimbau", "Cavaquinho, agogô, sanfona e berimbau", "Cavaquinho, agogô, acordeón y berimbau",
     "Playable on any note.", "Tocáveis em qualquer nota.", "Tocables en cualquier nota.",
     [["sound_select", "", [["hold", "PLAY"]]]]],
    ["9 sounds made for funk", "9 sons feitos para funk", "9 sonidos hechos para funk",
     "The Rio tamborzão.", "O tamborzão carioca.", "El tamborzão carioca.",
     [["tl_kit", "", [["turn", "ALGORITHM"], ["SEQ"]]]]],
    ["Sound picking on a cassette tape: like an auto copy and paste", "Escolha de sons em fita cassete: um copia e cola automático", "Elección de sonidos en casete: un copiar y pegar automático",
     "Turn, let go, it plays; it is copied to the hero.", "Gire, solte, toca; e copia para o herói.", "Gira, suelta, suena; y se copia al héroe.",
     [["sound_select", "", [["hold", "PLAY"]]]]],
    ["Boot sound chosen in the menu", "Som de abertura escolhido no menu", "Sonido de arranque elegido en el menú",
     "BOOT SOUND in the menu.", "BOOT SOUND no menu.", "BOOT SOUND en el menú.",
     [["tl_title", "", [["hold", "HOME"], ["menu", "BOOT SOUND"]]]]],
    ["Sample importer in the browser (WAV, MP3, OGG, FLAC, M4A, AAC)", "Importador de samples no navegador (WAV, MP3, OGG, FLAC, M4A, AAC)", "Importador de samples en el navegador (WAV, MP3, OGG, FLAC, M4A, AAC)",
     "Plug the FM-1 by USB and send files from this page.", "Plugue o FM-1 por USB e envie arquivos por esta página.", "Conecta el FM-1 por USB y envía archivos desde esta página.",
     [["sound_select", "", [["USB"], ["site", "IMPORTER"]]]]]
   ]],
   ["games", "Games, keys and controls", "Jogos, teclas e controles", "Juegos, teclas y controles", [
    ["FM Hero", "FM Hero", "FM Hero",
     "Imitates a synthesizer: scales on the 4 pads.", "Imita um sintetizador: escalas nos 4 pads.", "Imita un sintetizador: escalas en los 4 pads.",
     [["game_fm", "", [["hold", "HOME"], ["menu", "GAMES"]]]]],
    ["Piano Hero (imports songs from old games)", "Piano Hero (importa músicas de jogos antigos)", "Piano Hero (importa canciones de juegos antiguos)",
     "It only continues when you play the right note.", "Só continua quando você toca a nota certa.", "Solo continúa cuando tocas la nota correcta.",
     [["game_piano", "", [["hold", "HOME"], ["menu", "GAMES"]]], ["games_flip", "COVERS", [["hold", "HOME"], ["menu", "GAMES"]]]]],
    ["Real brightness on the keys (16 levels)", "Brilho real nas teclas (16 níveis)", "Brillo real en las teclas (16 niveles)",
     "The LEDs are no longer just on or off.", "Os LEDs não são mais só ligado ou desligado.", "Los LEDs ya no son solo encendido o apagado.",
     [["leds", "", [["hold", "HOME"], ["menu", "KEYBOARD LED"]]]]],
    ["Six light effects (waves, fade, explosion, rain, comet, fireworks)", "Seis efeitos de luz (ondas, fade, explosão, chuva, cometa, fogos)", "Seis efectos de luz (ondas, fade, explosión, lluvia, cometa, fuegos)",
     "Speed and size are adjustable.", "Velocidade e tamanho ajustáveis.", "Velocidad y tamaño ajustables.",
     [["leds", "", [["hold", "HOME"], ["menu", "KEYBOARD LED"]]]]],
    ["Buttons with a second function and HOLD", "Botões com segunda função e HOLD", "Botones con segunda función y HOLD",
     "Hold a button and the keys and knobs change job.", "Segure um botão e teclas e knobs mudam de função.", "Mantén un botón y teclas y knobs cambian de función.",
     [["tl_punch", "FX", [["hold", "FX"], ["KEYS"]]], ["tl_mix", "GLO", [["hold", "GLO"], ["KEYS"]]], ["tl_erase", "EDIT", [["hold", "EDIT"], ["KEYS"]]]]],
    ["New menu (map enemies, damage, speed and more)", "Menu novo (inimigos do mapa, dano, velocidade e mais)", "Menú nuevo (enemigos del mapa, daño, velocidad y más)",
     "MAP ENEMIES, DAMAGE NUMBERS, BOSS SPEED, ULT ANIMATION, FILTER.", "MAP ENEMIES, DAMAGE NUMBERS, BOSS SPEED, ULT ANIMATION, FILTER.", "MAP ENEMIES, DAMAGE NUMBERS, BOSS SPEED, ULT ANIMATION, FILTER.",
     [["tl_equip", "", [["hold", "HOME"]]]]]
   ]],
   ["site", "Site and tools", "Site e ferramentas", "Sitio y herramientas", [
    ["3D model running the real firmware", "Modelo 3D com o firmware real rodando", "Modelo 3D con el firmware real",
     "The screen on the model is the real firmware.", "A tela do modelo é o firmware de verdade.", "La pantalla del modelo es el firmware real.",
     [["tl_title", "", [["site", "3D"]]]]],
    ["Safe installer in the browser", "Instalador seguro no navegador", "Instalador seguro en el navegador",
     "It checks the package before it writes.", "Confere o pacote antes de gravar.", "Verifica el paquete antes de grabar.",
     [["tl_title", "", [["USB"], ["site", "INSTALL"]]]]],
    ["Pixel/dither lock button on the model", "Botão de fixar e pixel/dither no modelo", "Botón de fijar y pixel/dither en el modelo",
     "Locks the FM-1 in the centre; only the body gets the filter.", "Fixa o FM-1 no centro; só a carcaça recebe o filtro.", "Fija el FM-1 al centro; solo la carcasa recibe el filtro.",
     [["tl_title", "", [["site", "PIXEL"]]]]],
    ["Going back to the official firmware from the site", "Voltar ao firmware oficial pelo site", "Volver al firmware oficial desde el sitio",
     "Pick M-VAVE's own file; a backup comes first.", "Escolha o arquivo da M-VAVE; antes vem um backup.", "Elige el archivo de M-VAVE; antes va una copia.",
     [["tl_title", "", [["USB"], ["site", "BACK"]]]]],
    ["Control with the FM-1, a MIDI keyboard, the computer keyboard, an iPad and a phone", "Controle com o FM-1, teclado MIDI, teclado do computador, iPad e celular", "Control con el FM-1, teclado MIDI, teclado del ordenador, iPad y móvil",
     "Click the model, play the keys, turn the knobs.", "Clique no modelo, toque as teclas, gire os knobs.", "Haz clic en el modelo, toca las teclas, gira los knobs.",
     [["tl_title", "", [["site", "3D"]]]]]
   ]]
  ];
  var idx = L === "en" ? 0 : L === "pt" ? 1 : 2;
  var cat = 0, item = 0, scr = 0, ctx = null;
  var el = function (t, c, h) { var e = document.createElement(t); if (c) e.className = c; if (h != null) e.innerHTML = h; return e; };
  function click() {
    try { ctx = ctx || new (window.AudioContext || window.webkitAudioContext)(); var o = ctx.createOscillator(), g = ctx.createGain(); o.type = "square"; o.frequency.setValueAtTime(220, ctx.currentTime); o.frequency.exponentialRampToValueAtTime(90, ctx.currentTime + .05);
      g.gain.setValueAtTime(.05, ctx.currentTime); g.gain.exponentialRampToValueAtTime(.0001, ctx.currentTime + .07); o.connect(g); g.connect(ctx.destination); o.start(); o.stop(ctx.currentTime + .08); } catch (e) {}
  }
  /* ---- skeleton */
  root.innerHTML = '<div class="tl-tabs" role="tablist"></div><div class="tl-frame"><div class="tl-main"><div class="tl-left"><ol class="tl-items"></ol></div><div class="tl-right"><div class="tl-lcd"><video muted loop playsinline autoplay disablepictureinpicture controlslist="nopictureinpicture"></video><div class="tl-cap"></div></div><div class="tl-how"></div></div></div></div>';
  var tabs = root.querySelector(".tl-tabs"), list = root.querySelector(".tl-items"), vid = root.querySelector(".tl-lcd video"), cap = root.querySelector(".tl-cap"), how = root.querySelector(".tl-how");
  function applyKnobs() {}
  function tok(t) { return Array.isArray(t) ? t : [t]; }
  function allTokens(s) { var out = {}; s[2].forEach(function (t) { var a = tok(t); out[(a[0] === "hold" || a[0] === "turn" || a[0] === "menu" || a[0] === "site") ? a[1] : a[0]] = 1; }); return out; }
  function paintHow(s) {
    how.innerHTML = '<div class="tl-hh">' + T.howh[L] + '</div><div class="tl-chips"></div>';
    var box = how.querySelector(".tl-chips");
    s[2].forEach(function (t, i) {
      var a = tok(t), c = el("span", "chip"); if (i) box.append(el("span", "plus", "+"));
      if (a[0] === "hold" || a[0] === "turn" || a[0] === "menu" || a[0] === "site") c.innerHTML = '<small>' + T[a[0]][L] + '</small><b>' + a[1] + '</b>';
      else if (a[0] === "KEYS") c.innerHTML = '<small>' + T.keys[L] + '</small><b>♪</b>';
      else c.innerHTML = '<b>' + a[0] + '</b>';
      box.append(c);
      if (a.length > 1 && typeof a[1] === "number") { box.append(el("span", "plus", "+")); box.append(el("span", "chip num", "<b>" + a[1] + "</b>")); box.append(el("span", "tl-txt", T.times[L] + " " + a[0])); }
    });
  }
  function media(name) { return B + "media/" + name + ".mp4"; }
  var warmed = {};
  function warm(ci) {                                    /* fetch the clips of a category into the cache (low priority) before they are asked for */
    C[ci][4].forEach(function (it) { it[6].forEach(function (x) { if (!warmed[x[0]]) { warmed[x[0]] = 1; try { fetch(media(x[0]), { priority: "low" }).catch(function () {}); } catch (e) {} } }); });
  }
  if ("IntersectionObserver" in window) { var wio = new IntersectionObserver(function (es) { if (es[0].isIntersecting) { warm(cat); wio.disconnect(); } }, { rootMargin: "700px 0px" }); wio.observe(root); } else warm(cat);
  var lcd = root.querySelector(".tl-lcd"), spin = el("div", "tl-load", "<i></i><i></i><i></i><i></i>"); lcd.append(spin);
  vid.addEventListener("playing", function () { lcd.classList.remove("loading"); });
  vid.addEventListener("waiting", function () { lcd.classList.add("loading"); });
  function show() {
    var it = C[cat][4][item], s = it[6][scr];
    [vid].forEach(function (v) {
      if (v.dataset.n !== s[0]) {
        v.dataset.n = s[0]; lcd.classList.add("loading"); v.src = media(s[0]);
        if (window.__vidWatch) window.__vidWatch(v);
        var p = v.play(); if (p && p.catch) p.catch(function () {});
      }
    });
    if (vid.paused && !document.hidden) { var pp = vid.play(); if (pp && pp.catch) pp.catch(function () {}); }
    cap.textContent = s[1] ? s[1] : ""; cap.hidden = !s[1];
    paintHow(s);
    list.querySelectorAll("li").forEach(function (li, i) { li.classList.toggle("on", i === item); });
    root.querySelectorAll(".tl-sw").forEach(function (b) { b.remove(); });
    if (it[6].length > 1) {
      var w = el("div", "tl-sw"); w.setAttribute("aria-label", T.screen[L]);
      it[6].forEach(function (x, i) { var b = el("button", "pxb" + (i === scr ? " on" : ""), x[1] || String(i + 1)); b.type = "button"; b.onclick = function () { click(); scr = i; show(); }; w.append(b); });
      root.querySelector(".tl-right").insertBefore(w, how);
    }
  }
  function build() {
    list.innerHTML = "";
    C[cat][4].forEach(function (it, i) {
      var li = el("li"), b = el("button", "pxb item", '<span class="gb"></span><span class="tt">' + it[idx] + '</span>'); b.type = "button"; b.title = it[3 + idx];
      b.onclick = function () { click(); item = i; scr = 0; show(); };
      li.append(b); list.append(li);
    });
    show();
  }
  var ICON = {
    rpg: '<svg viewBox="0 0 24 24"><path d="M19 2h3v3l-9 9-3-3zM8 12l4 4-2 2-1 3-3-3 3-1 2-2-3-3z" fill="currentColor"/></svg>',
    world: '<svg viewBox="0 0 24 24"><path d="M3 11l9-8 9 8v10h-6v-6H9v6H3z" fill="currentColor"/></svg>',
    sound: '<svg viewBox="0 0 24 24"><path d="M9 3h10v13a3 3 0 1 1-2-2.8V7H11v11a3 3 0 1 1-2-2.8z" fill="currentColor"/></svg>',
    games: '<svg viewBox="0 0 24 24"><path d="M3 8h18v9a3 3 0 0 1-3 3h-2l-2-3h-4l-2 3H6a3 3 0 0 1-3-3zm4 2v2H5v2h2v2h2v-2h2v-2H9v-2zm9 1.5a1.5 1.5 0 1 0 0 3 1.5 1.5 0 0 0 0-3z" fill="currentColor"/></svg>',
    site: '<svg viewBox="0 0 24 24"><path d="M3 4h18v12H3zm2 2v8h14V6zM8 18h8v2H8z" fill="currentColor"/></svg>'
  };
  C.forEach(function (c, i) {
    var b = el("button", "tl-tab", '<span class="ti">' + (ICON[c[0]] || "") + '</span><span class="tx">' + c[1 + idx] + '</span> <small>(' + c[4].length + ')</small>'); b.type = "button"; b.setAttribute("role", "tab"); b.setAttribute("aria-label", c[1 + idx]); b.title = c[1 + idx];
    b.onclick = function () { click(); cat = i; item = 0; scr = 0; warm(i); tabs.querySelectorAll(".tl-tab").forEach(function (x, k) { x.classList.toggle("on", k === i); x.setAttribute("aria-selected", k === i); }); build(); };
    tabs.append(b);
  });
  tabs.firstChild.click();
})();
