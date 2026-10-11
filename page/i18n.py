# -*- coding: utf-8 -*-
# Textos da página em 3 idiomas. A chave aparece no modelo como data-i="chave" (texto/HTML), data-ia="chave" (alt) ou data-it="chave" (title/aria-label).
T = {}
def k(key, pt, en, es): T[key] = {"pt": pt, "en": en, "es": es}

# ---------- topo e menu ----------
k("t_rollback", "Voltar ao firmware original", "Back to the original firmware", "Volver al firmware original")
k("a_menu", "Abrir menu", "Open menu", "Abrir menú")
k("a_close", "Fechar menu", "Close menu", "Cerrar menú")
k("a_gh", "Código no GitHub", "Code on GitHub", "Código en GitHub")
k("a_roll", "Voltar ao firmware original do FM-1", "Back to the FM-1 original firmware", "Volver al firmware original del FM-1")
k("m_goto", "Ir para", "Go to", "Ir a")
k("m_model", "Modelo 3D", "3D model", "Modelo 3D")
k("m_about", "O que é o FM1 Quest", "What FM1 Quest is", "Qué es FM1 Quest")
k("m_news", "Novidades da v0.2", "What's new in v0.2", "Novedades de la v0.2")
k("m_install", "Instalar", "Install", "Instalar")
k("m_back", "Voltar ao firmware original", "Back to the original firmware", "Volver al firmware original")
k("m_log", "Log do erro", "Error log", "Registro del error")
k("m_thanks", "Agradecimentos", "Thanks", "Agradecimientos")
k("m_comm", "Obrigado a toda a comunidade do <b>r/MVaveFM1</b>.", "Thank you to the whole <b>r/MVaveFM1</b> community.", "Gracias a toda la comunidad de <b>r/MVaveFM1</b>.")
k("m_reddit", "r/MVaveFM1 no Reddit", "r/MVaveFM1 on Reddit", "r/MVaveFM1 en Reddit")
k("m_fw", "E obrigado aos firmwares <b>Felucca</b> e <b>Sloop</b> pela inspiração.", "And thanks to the <b>Felucca</b> and <b>Sloop</b> firmwares for the inspiration.", "Y gracias a los firmwares <b>Felucca</b> y <b>Sloop</b> por la inspiración.")
k("m_baud", "Baud Girl · página oficial", "Baud Girl · official page", "Baud Girl · página oficial")
k("m_about_h", "Sobre o projeto", "About the project", "Sobre el proyecto")
k("love_short", "Este é um projeto pessoal, feito com amor e com um único objetivo: testar a tecnologia e explorar todo o potencial de um hardware incrível e acessível. Se você busca um parceiro para projetos de inovação digital e sites, fale comigo pela Raincake.",
  "This is a personal project, made with love and with a single goal: to test the technology and explore the full potential of an incredible, affordable piece of hardware. If you are looking for a partner for digital innovation projects and websites, talk to me through Raincake.",
  "Este es un proyecto personal, hecho con cariño y con un único objetivo: probar la tecnología y explorar todo el potencial de un hardware increíble y accesible. Si buscas un socio para proyectos de innovación digital y sitios web, habla conmigo a través de Raincake.")
k("ai", "Uma IA usada com criatividade e para o bem,<br>é uma IA a menos se acontecer uma revolução das máquinas.",
  "An AI used with creativity and for good,<br>is one AI less if the machines ever revolt.",
  "Una IA usada con creatividad y para el bien,<br>es una IA menos si ocurre una revolución de las máquinas.")

# ---------- modelo e apresentação ----------
k("hint", "Clique nos botões, gire os knobs, toque as teclas. Funciona também com o teclado do computador.",
  "Click the buttons, turn the knobs, play the keys. It also works with your computer keyboard.",
  "Haz clic en los botones, gira los knobs, toca las teclas. También funciona con el teclado del ordenador.")
k("a_model", "Modelo 3D do FM-1 com o firmware rodando", "3D model of the FM-1 running the firmware", "Modelo 3D del FM-1 con el firmware funcionando")
k("h1", "Firmware para o M-VAVE FM-1", "Firmware for the M-VAVE FM-1", "Firmware para el M-VAVE FM-1")
k("lead", "O seu groovebox como um RPG em pixel art. As quatro faixas são quatro heróis, cada passo aceso é um ataque, o andamento define a velocidade da caminhada do grupo, o FX ativa as habilidades na luta contra o boss e gravar um loop é uma luta de chefe.",
  "Your groovebox as a pixel-art RPG. The four tracks are four heroes, every lit step is an attack, the tempo sets how fast the party walks, the FX turns on the skills in the boss fight and recording a loop is a boss fight.",
  "Tu groovebox como un RPG en pixel art. Las cuatro pistas son cuatro héroes, cada paso encendido es un ataque, el tempo define la velocidad del grupo, el FX activa las habilidades en la pelea contra el jefe y grabar un loop es una pelea de jefe.")
k("tile1_b", "faixas", "tracks", "pistas")
k("tile1_s", "mago, arqueiro, clérigo, guerreiro e duas classes secretas", "mage, archer, cleric, warrior and two secret classes", "mago, arquero, clérigo, guerrero y dos clases secretas")
k("tile2_b", "alma de RPG", "RPG soul", "alma de RPG")
k("tile2_s", "Não tenha só um sintetizador com RPG dentro: tenha um RPG novel com mundo próprio, controlado por ritmos musicais a partir de um sintetizador.", "Don't settle for a synthesizer with an RPG inside: have an RPG novel with a world of its own, controlled by musical rhythms from a synthesizer.", "No tengas solo un sintetizador con un RPG dentro: ten una novela RPG con mundo propio, controlada por ritmos musicales desde un sintetizador.")
k("tile3_b", "fx", "fx", "fx")
k("tile3_s", "cada efeito tem um estilo próprio: ele afeta o seu preset quando você toca uma tecla e também os efeitos de mundo no cenário e nos personagens", "each effect has its own style: it affects your preset when you play a key and also the world effects on the scenery and the characters", "cada efecto tiene un estilo propio: afecta a tu preset cuando tocas una tecla y también a los efectos de mundo en el escenario y los personajes")
k("tile4_b", "rec", "rec", "rec")
k("tile4_s", "um loop selvagem aparece: grave para vencer o Rei Slime", "a wild loop appears: record it to beat the King Slime", "aparece un loop salvaje: grábalo para vencer al Rey Slime")

# ---------- novidades ----------
k('n_h', 'O que há de novo na v0.2', 'What is new in v0.2', 'Novedades de la v0.2')
k('n_lead', 'Vou te contar tudo, na ordem em que um bom RPG conta: primeiro o golpe que para a tela, depois quem aparece pra te desafiar e como é o mundo ao redor. Cada novidade tem um pedacinho dela rodando no próprio firmware.', "I'll tell you everything in the order a good RPG tells it: first the move that stops the screen, then who shows up to challenge you and what the world around looks like. Every novelty has a little piece of it running on the firmware itself.", "I'll tell you everything in the order a good RPG tells it: first the move that stops the screen, then who shows up to challenge you and what the world around looks like. Every novelty has a little piece of it running on the firmware itself.")
k("a_heroes", "Os heróis", "The heroes", "Los héroes")
k("f_mago", "Mago", "Mage", "Mago"); k("f_arqueiro", "Arqueiro", "Archer", "Arquero"); k("f_clerigo", "Clérigo", "Cleric", "Clérigo"); k("f_guerreiro", "Guerreiro", "Warrior", "Guerrero")
k("a_mystery", "Um herói ainda desconhecido", "A hero not yet known", "Un héroe aún desconocido")
k("c1_a", "Os quatro heróis caminhando", "The four heroes walking", "Los cuatro héroes caminando")
k("c1_h", "Heróis em pixel art", "Pixel-art heroes", "Héroes en pixel art")
k("c1_p", "Quatro heróis desenhados em blocos de pixel inteiros, cada um com a sua cor. Andam, atacam, lançam magia, sentam e dançam, tudo com o mesmo esqueleto de animação.",
  "Four heroes drawn in whole-pixel blocks, each in its own colour. They walk, attack, cast spells, sit and dance, all with the same animation skeleton.",
  "Cuatro héroes dibujados con bloques de píxel enteros, cada uno con su color. Caminan, atacan, lanzan magia, se sientan y bailan, todo con el mismo esqueleto de animación.")
k("c2_a", "A taverna com a banda no palco", "The tavern with the band on stage", "La taberna con la banda en el escenario")
k("c2_h", "A taverna", "The tavern", "La taberna")
k("c2_p", "O menu virou uma taverna: os quatro heróis tocam no palco, cada um com um mini FM-1 da sua cor que pisca no ritmo. Os frequentadores dançam e a lareira bate com a música. Sem música, a festa continua. Um toque no HOME volta ao jogo; segurando, abre o menu.",
  "The menu is now a tavern: the four heroes play on stage, each with a tiny FM-1 in its own colour blinking to the beat. The regulars dance and the fireplace pulses with the music. With no music, the party goes on. Tap HOME to go back to the game; hold it to open the menu.",
  "El menú es ahora una taberna: los cuatro héroes tocan en el escenario, cada uno con un mini FM-1 de su color que parpadea al ritmo. Los parroquianos bailan y la chimenea late con la música. Sin música, la fiesta sigue. Un toque en HOME vuelve al juego; manteniéndolo se abre el menú.")
k("c3_a", "A luta contra o chefe, com os rostos e barras de carga", "The boss fight, with the faces and charge bars", "La pelea contra el jefe, con los rostros y barras de carga")
k("c3_h", "Luta e golpe final (ULT)", "Fight and final strike (ULT)", "Pelea y golpe final (ULT)")
k("c3_p", "Gravar um loop é uma luta de chefe. Os rostos dos heróis ficam ao lado, cada um com uma barra que enche enquanto você grava. Ao soltar o REC, o herói da frente solta o golpe final em tela cheia, em preto e branco com dither forte, só imagem, sem som. Aqui, os 3 primeiros segundos de cada um:",
  "Recording a loop is a boss fight. The heroes' faces sit at the side, each with a bar that fills while you record. When you release REC, the hero in front unleashes the final strike full screen, in black and white with heavy dither, picture only, no sound. Here, the first 3 seconds of each:",
  "Grabar un loop es una pelea de jefe. Los rostros de los héroes quedan al lado, cada uno con una barra que se llena mientras grabas. Al soltar REC, el héroe de enfrente lanza el golpe final a pantalla completa, en blanco y negro con dither fuerte, solo imagen, sin sonido. Aquí, los 3 primeros segundos de cada uno:")
k("a_ult1", "ULT del mago", "Mage ULT", "ULT del mago"); k("a_ult2", "ULT del arqueiro", "Archer ULT", "ULT del arquero"); k("a_ult3", "ULT del clérigo", "Cleric ULT", "ULT del clérigo"); k("a_ult4", "ULT del guerreiro", "Warrior ULT", "ULT del guerrero")
k("c4_a", "A rua da vila com parallax", "The village street with parallax", "La calle del pueblo con parallax")
k("c4_h", "Abertura em cenas", "Opening in scenes", "Apertura en escenas")
k("c4_p", "A abertura é uma sequência de cenas com camadas de profundidade: taverna, quadro de missões, saída, vila, colina, caminhada do grupo e luta.",
  "The opening is a sequence of scenes with depth layers: tavern, quest board, exit, village, hill, the party's walk and the fight.",
  "La apertura es una secuencia de escenas con capas de profundidad: taberna, tablón de misiones, salida, pueblo, colina, caminata del grupo y pelea.")
k("c5_a", "O grupo caminhando", "The party walking", "El grupo caminando")
k("c5_h", "A caminhada do grupo", "The party's walk", "La caminata del grupo")
k("c5_p", "O grupo anda ao ritmo do andamento da música, e o cenário muda conforme a tonalidade que você escolhe.",
  "The party walks to the tempo of the music, and the scenery changes with the key you choose.",
  "El grupo camina al ritmo del tempo de la música, y el escenario cambia según la tonalidad que elijas.")
k("c6_a", "Transições de tela: íris, VHS, pixelar e deslizar", "Screen transitions: iris, VHS, pixelate and slide", "Transiciones de pantalla: iris, VHS, pixelar y deslizar")
k("c6_h", "Transições de tela", "Screen transitions", "Transiciones de pantalla")
k("c6_p", "Doze transições feitas só de pixels, como íris, TV ligando, VHS, pixelar e deslizar, com início rápido e final suave.",
  "Twelve transitions made only of pixels, such as iris, TV turning on, VHS, pixelate and slide, with a fast start and a soft end.",
  "Doce transiciones hechas solo de píxeles, como iris, TV encendiéndose, VHS, pixelar y deslizar, con inicio rápido y final suave.")
k("c7_a", "Tela de efeitos de LED", "LED effects screen", "Pantalla de efectos de LED")
k("c7_h", "Luzes das teclas", "Key lights", "Luces de las teclas")
k("c7_p", "28 efeitos de LED, todos no mesmo amarelo, com brilho, piscada e fade-out suave. Gire o knob SELECT para percorrer a lista e veja o efeito nas teclas. Ao ligar, as luzes varrem o teclado junto com o som de abertura.",
  "28 LED effects, all in the same yellow, with brightness, blink and a soft fade-out. Turn the SELECT knob to browse the list and watch the effect on the keys. At power-on, the lights sweep the keyboard along with the opening sound.",
  "28 efectos de LED, todos en el mismo amarillo, con brillo, parpadeo y un fade-out suave. Gira el knob SELECT para recorrer la lista y mira el efecto en las teclas. Al encender, las luces recorren el teclado junto con el sonido de apertura.")
k("c8_a", "O toca-fitas da taverna", "The tavern's tape deck", "El casetero de la taberna")
k("c8_h", "Toca-fitas e XP", "Tape deck and XP", "Casetero y XP")
k("c8_p", "Fitas com músicas que abrem conforme o seu nível. Tocar, gravar loops e vencer chefes dá XP.",
  "Tapes with songs that unlock as you level up. Playing, recording loops and beating bosses earns XP.",
  "Cintas con canciones que se abren según tu nivel. Tocar, grabar loops y vencer jefes da XP.")
k("c9_a", "A ficha de status do herói", "The hero's status sheet", "La ficha de estado del héroe")
k("c9_h", "Status e dicas na tela", "Status and on-screen hints", "Estado y consejos en pantalla")
k("c9_p", "A ficha de cada herói mostra força, agilidade, inteligência e sorte, e OCT− e OCT+ trocam de herói. Cada tela ganhou um rodapé discreto com as dicas de botão e o simulador 3D virou o padrão.",
  "Each hero's sheet shows strength, agility, intelligence and luck, and OCT− and OCT+ switch heroes. Every screen got a discreet footer with button hints, and the 3D simulator became the default.",
  "La ficha de cada héroe muestra fuerza, agilidad, inteligencia y suerte, y OCT− y OCT+ cambian de héroe. Cada pantalla tiene un pie discreto con los consejos de botones y el simulador 3D pasó a ser el predeterminado.")

# ---------- instalar ----------
k("i_h", "Instalar", "Install", "Instalar")
k("i_p1", "Conecte o FM-1 por USB e clique em <b>Instalar no FM1</b> no topo da página (ou no botão abaixo). Funciona só no Chrome ou no Edge (Web MIDI), em uma página aberta por https.",
  "Connect the FM-1 by USB and click <b>Install on FM1</b> at the top of the page (or the button below). Chrome or Edge only (Web MIDI), on a page opened over https.",
  "Conecta el FM-1 por USB y haz clic en <b>Instalar en el FM1</b> arriba en la página (o en el botón de abajo). Solo en Chrome o Edge (Web MIDI), en una página abierta por https.")
k("i_p2", "Não desconecte o cabo USB durante a gravação.", "Do not unplug the USB cable while writing.", "No desconectes el cable USB durante la grabación.")
k("i_direct", "Conecte o cabo USB direto no computador. Não precisa de sonda nem de Transporter.", "Connect the USB cable directly to the computer. No probe or Transporter is needed.", "Conecta el cable USB directo al ordenador. No hace falta sonda ni Transporter.")
k("i_beta", "O FM1 Quest é um firmware personalizado experimental e pode ser instável. Ele não foi testado em todas as unidades. Os autores não se responsabilizam se o seu FM-1 for danificado ou se comportar mal por causa dele. Use por sua conta e risco e salve um backup antes.",
  "FM1 Quest is an experimental custom firmware and may be unstable. It has not been tried on every unit. The authors accept no responsibility if your FM-1 is damaged or misbehaves because of it. Use it at your own risk, and save a backup first.",
  "FM1 Quest es un firmware personalizado experimental y puede ser inestable. No se ha probado en todas las unidades. Los autores no se hacen responsables si tu FM-1 se daña o se comporta mal por su causa. Úsalo bajo tu propio riesgo y guarda una copia de seguridad antes.")
k("i_tested", "Em testes de usabilidade forcei o aparelho a travar e, nessa situação, ele só reiniciou, o que dá mais segurança no processo de instalação. Mas se algo acontecer, dá pra restaurar o backup original ou instalar outro custom firmware.",
  "In usability tests I forced the device to hang and, in that situation, it simply restarted, which makes the installation safer. But if anything happens, you can restore the original backup or install another custom firmware.",
  "En las pruebas de usabilidad forcé el aparato a bloquearse y, en esa situación, solo se reinició, lo que da más seguridad al proceso de instalación. Pero si algo ocurre, se puede restaurar la copia original o instalar otro custom firmware.")
k("i_editor", "Abrir o editor (backup, sons e sequências, pelo computador)", "Open the editor (back up, edit sounds and sequences from the computer)", "Abrir el editor (copia de seguridad, sonidos y secuencias, desde el ordenador)")
k("i_source", "Código no GitHub", "Source on GitHub", "Código en GitHub")
k("i_brick", "Se uma instalação falhar e o FM-1 não ligar mais, a recuperação precisa de um Transporter: <a href=\"https://github.com/kurogedelic/FM-1-transporter\" target=\"_blank\" rel=\"noopener\">FM-1-transporter</a>. Segure OCT− sozinho ao ligar o FM-1 para o resgate por USB.",
  "If an install fails and the FM-1 no longer starts, recovering it needs a Transporter: <a href=\"https://github.com/kurogedelic/FM-1-transporter\" target=\"_blank\" rel=\"noopener\">FM-1-transporter</a>. Hold OCT− alone while switching the FM-1 on for the USB rescue.",
  "Si una instalación falla y el FM-1 ya no arranca, la recuperación necesita un Transporter: <a href=\"https://github.com/kurogedelic/FM-1-transporter\" target=\"_blank\" rel=\"noopener\">FM-1-transporter</a>. Mantén OCT− solo mientras enciendes el FM-1 para el rescate por USB.")
k("r_sum", "Voltar ao firmware original (V15)", "Return to the official firmware (V15)", "Volver al firmware oficial (V15)")
k("r_p1", "Baixe o FM-1 V15 oficial no site da M-VAVE e escolha o arquivo FM-1.fwsc de dentro dele, sem modificar. O arquivo é verificado neste navegador e não é enviado a lugar nenhum.",
  "Download the official FM-1 V15 from M-VAVE and select the FM-1.fwsc inside it, unmodified. The file is checked in this browser; it is not sent anywhere.",
  "Descarga el FM-1 V15 oficial de M-VAVE y elige el FM-1.fwsc que viene dentro, sin modificar. El archivo se verifica en este navegador y no se envía a ningún sitio.")
k("r_p2", "O firmware oficial não abre músicas, projetos nem samples do FM1 Quest. Salve um backup no editor antes (Projetos → Backup); reinstale o FM1 Quest e restaure pelo editor quando quiser tê-los de volta.",
  "The official firmware cannot open FM1 Quest music, projects or samples. Save a backup with the editor first (Projects → Backup); reinstall FM1 Quest and restore it from the editor whenever you want them back.",
  "El firmware oficial no abre música, proyectos ni samples de FM1 Quest. Guarda antes una copia en el editor (Proyectos → Backup); reinstala FM1 Quest y restáurala desde el editor cuando quieras recuperarlos.")
k("r_ed", "Abrir o editor para fazer o backup", "Open the editor to back up", "Abrir el editor para hacer la copia")
k("r_saved", "Salvei um backup neste computador", "I saved a backup on this computer", "Guardé una copia en este ordenador")
k("r_file", "O FM-1.fwsc oficial", "The official FM-1.fwsc", "El FM-1.fwsc oficial")
k("r_go", "Instalar o V15 oficial", "Install the official V15", "Instalar el V15 oficial")
k("r_m", "O atualizador da própria M-VAVE, o M-UPGRADE (Downloads → PC Software), também funciona. Se ele não achar o FM-1, feche todos os outros apps e abas do navegador que usam MIDI.",
  "M-VAVE's own updater, M-UPGRADE (Downloads → PC Software), works too. If it does not find the FM-1, close every other app and browser tab that uses MIDI.",
  "El actualizador de la propia M-VAVE, M-UPGRADE (Downloads → PC Software), también funciona. Si no encuentra el FM-1, cierra todas las demás apps y pestañas del navegador que usan MIDI.")
k("l_h", "Log do erro", "Error log", "Registro del error")
k("l_note", "Algo deu errado na instalação. O registro técnico está abaixo; é este o texto para enviar a quem for ajudar.", "Something went wrong with the install. The technical record is below; send this text to whoever is helping.", "Algo salió mal en la instalación. El registro técnico está abajo; es este el texto para enviar a quien te ayude.")
k("a_log", "Log do erro da instalação", "Install error log", "Registro del error de la instalación")

# ---------- rodapé ----------
k("f_love", "Este é um projeto pessoal, feito com amor e com um único objetivo: testar a tecnologia e explorar todo o potencial de um hardware incrível e acessível. Se você busca um parceiro para projetos de inovação digital e sites, fale comigo pela Raincake: <a href=\"https://raincake.com.br\" target=\"_blank\" rel=\"noopener\">raincake.com.br</a>",
  "This is a personal project, made with love and with a single goal: to test the technology and explore the full potential of an incredible, affordable piece of hardware. If you are looking for a partner for digital innovation projects and websites, talk to me through Raincake: <a href=\"https://raincake.com.br\" target=\"_blank\" rel=\"noopener\">raincake.com.br</a>",
  "Este es un proyecto personal, hecho con cariño y con un único objetivo: probar la tecnología y explorar todo el potencial de un hardware increíble y accesible. Si buscas un socio para proyectos de innovación digital y sitios web, habla conmigo a través de Raincake: <a href=\"https://raincake.com.br\" target=\"_blank\" rel=\"noopener\">raincake.com.br</a>")
k("f_legal", "Fontes: Pixelify Sans e Jacquarda Bastarda 9 (SIL OFL 1.1), Terminus (Dimitar Toshkov Zhekov, SIL OFL 1.1). Samples: Versilian Studios VSCO-2 Community Edition e VCSL (CC0 1.0), sprites dos goblins: Zneeke; e o Hügelton Sample Pack (&copy; Hügelton Instruments, não é CC0).<br>M-VAVE e FM-1 são marcas de seus respectivos donos; o FM1 Quest não é afiliado a eles.<br>GPL-3.0, sem garantia. Código: <a href=\"https://github.com/hericdk/fm1-quest\" target=\"_blank\" rel=\"noopener\">github.com/hericdk/fm1-quest</a>",
  "Fonts: Pixelify Sans and Jacquarda Bastarda 9 (SIL OFL 1.1), Terminus (Dimitar Toshkov Zhekov, SIL OFL 1.1). Samples: Versilian Studios VSCO-2 Community Edition and VCSL (CC0 1.0), goblin sprites: Zneeke; and the Hügelton Sample Pack (&copy; Hügelton Instruments, not CC0).<br>M-VAVE and FM-1 are trademarks of their respective owners; FM1 Quest is not affiliated with them.<br>GPL-3.0, no warranty. Source: <a href=\"https://github.com/hericdk/fm1-quest\" target=\"_blank\" rel=\"noopener\">github.com/hericdk/fm1-quest</a>",
  "Fuentes: Pixelify Sans y Jacquarda Bastarda 9 (SIL OFL 1.1), Terminus (Dimitar Toshkov Zhekov, SIL OFL 1.1). Samples: Versilian Studios VSCO-2 Community Edition y VCSL (CC0 1.0), sprites de los goblins: Zneeke; y el Hügelton Sample Pack (&copy; Hügelton Instruments, no es CC0).<br>M-VAVE y FM-1 son marcas de sus respectivos dueños; FM1 Quest no está afiliado a ellos.<br>GPL-3.0, sin garantía. Código: <a href=\"https://github.com/hericdk/fm1-quest\" target=\"_blank\" rel=\"noopener\">github.com/hericdk/fm1-quest</a>")

# ---------- meta e mensagens do instalador (usadas só no JS) ----------
k("title", "FM1 Quest · firmware para o M-VAVE FM-1", "FM1 Quest · firmware for the M-VAVE FM-1", "FM1 Quest · firmware para el M-VAVE FM-1")
k("desc", "Um RPG em pixel art para o seu FM-1: firmware personalizado para o M-VAVE FM-1, com instalação direta pelo navegador.", "A pixel-art RPG for your FM-1: custom firmware for the M-VAVE FM-1, installed straight from the browser.", "Un RPG en pixel art para tu FM-1: firmware personalizado para el M-VAVE FM-1, con instalación directa desde el navegador.")
k("j_install", "Instalar no FM1", "Install on FM1", "Instalar en el FM1")
k("j_plug", "Plugue o FM1", "Plug in the FM1", "Conecta el FM1")
k("j_plugmsg", "FM-1 não encontrado. Plugue o FM1 no computador por USB.", "FM-1 not found. Plug the FM1 into the computer by USB.", "FM-1 no encontrado. Conecta el FM1 al ordenador por USB.")
k("j_insecure", "Esta página precisa estar em HTTPS para falar com o FM-1. Se o certificado do endereço ainda está sendo emitido, tente de novo em alguns minutos.", "This page has to be served over HTTPS to talk to the FM-1. If the certificate for this address is still being issued, try again in a few minutes.", "Esta página necesita HTTPS para comunicarse con el FM-1. Si el certificado de esta dirección aún se está emitiendo, inténtalo de nuevo en unos minutos.")
k("j_nomidi", "Este navegador não consegue instalar. Abra a página no Chrome ou no Edge.", "This browser cannot do it. Open the page in Chrome or Edge.", "Este navegador no puede instalar. Abre la página en Chrome o Edge.")
k("j_loadfail", "Não foi possível carregar: ", "Could not load: ", "No se pudo cargar: ")
k("j_start", "Iniciando", "Starting", "Iniciando")
k("j_verify", "Verificando", "Verifying", "Verificando")
k("j_loader", "Entrando no modo de atualização", "Switching to update mode", "Entrando en modo de actualización")
k("j_write", "Gravando", "Writing", "Grabando")
k("j_reboot", "Reiniciando", "Restarting", "Reiniciando")
k("j_done", "Concluído", "Done", "Concluido")
k("j_stopped", "Interrompido. Reconecte o cabo USB e clique em Instalar de novo.", "Stopped. Replug the USB cable and press Install again.", "Interrumpido. Vuelve a conectar el cable USB y pulsa Instalar de nuevo.")
k("j_error", "Erro: ", "Error: ", "Error: ")
k("j_denied", "O acesso ao MIDI (SysEx) não foi permitido. Permita o MIDI nas configurações do site (barra de endereço) e clique em Instalar de novo.", "MIDI (SysEx) access was not allowed. Allow MIDI in the site settings (address bar) and press Install again.", "No se permitió el acceso a MIDI (SysEx). Permite MIDI en la configuración del sitio (barra de direcciones) y pulsa Instalar de nuevo.")
k("j_notfound", "FM-1 não encontrado. Confira o cabo USB (um cabo de dados), feche outros apps que usam MIDI e clique em Instalar de novo.", "FM-1 not found. Check the USB cable (a data cable), close other apps that use MIDI, and press Install again.", "FM-1 no encontrado. Revisa el cable USB (un cable de datos), cierra otras apps que usan MIDI y pulsa Instalar de nuevo.")
k("j_model", "O aparelho conectado não é um FM-1.", "The connected device is not an FM-1.", "El aparato conectado no es un FM-1.")
k("j_lost", "A conexão USB foi perdida. Reconecte e clique em Instalar de novo.", "The USB connection was lost. Replug it and press Install again.", "Se perdió la conexión USB. Vuelve a conectarla y pulsa Instalar de nuevo.")
k("j_noloader", "O FM-1 em modo de atualização não foi encontrado. Reconecte o cabo USB e clique em Instalar de novo (ele fica em modo de atualização até terminar).", "The FM-1 in update mode was not found. Replug the USB cable and press Install again (it stays in update mode until it is finished).", "No se encontró el FM-1 en modo de actualización. Vuelve a conectar el cable USB y pulsa Instalar de nuevo (se queda en modo de actualización hasta terminar).")
k("j_noreturn", "O FM-1 não voltou. Desligue e ligue o aparelho.", "The FM-1 did not come back. Power-cycle it.", "El FM-1 no volvió. Apágalo y enciéndelo.")
k("j_mismatch", "Gravado, mas o FM-1 informa outra versão: ", "Written, but the FM-1 reports another version: ", "Grabado, pero el FM-1 informa otra versión: ")
k("j_foreign", "O FM-1 está no modo de atualização de outro firmware. Termine essa atualização com o atualizador dele (M-UPGRADE) e clique em Instalar de novo: ", "The FM-1 is in the update mode of another firmware. Finish that update with its own updater (M-UPGRADE), then press Install again: ", "El FM-1 está en el modo de actualización de otro firmware. Termina esa actualización con su propio actualizador (M-UPGRADE) y pulsa Instalar de nuevo: ")
k("j_badpkg", "O arquivo a instalar está danificado. Recarregue a página.", "The file to install is damaged. Reload the page.", "El archivo a instalar está dañado. Recarga la página.")
k("j_leave", "Gravando. Não feche a página.", "Writing. Do not close the page.", "Grabando. No cierres la página.")
k("j_stockValid", "V15 oficial verificado", "Official V15 verified", "V15 oficial verificado")
k("j_stockBad", "Não é o V15 oficial: ", "Not the official V15: ", "No es el V15 oficial: ")
k("j_stockConfirm", "Instalar o V15 oficial no FM-1? O firmware oficial não abre músicas do FM1 Quest. Você salvou um backup?", "Install the official V15 on the FM-1? The official firmware cannot open FM1 Quest music. Did you save a backup?", "¿Instalar el V15 oficial en el FM-1? El firmware oficial no abre la música de FM1 Quest. ¿Guardaste una copia?")
k("j_stockFinish", "O FM-1 está no modo de atualização do FM1 Quest. Clique em Instalar para terminar essa atualização antes de voltar ao V15 oficial.", "The FM-1 is in the update mode of FM1 Quest. Press Install to finish that update first, then return to the official V15.", "El FM-1 está en el modo de actualización de FM1 Quest. Pulsa Instalar para terminar esa actualización antes de volver al V15 oficial.")
k("j_seelog", " Log do erro gerado no topo da página.", " Error log generated at the top of the page.", " Registro del error generado arriba en la página.")
k("j_unknown", "falha desconhecida", "unknown failure", "fallo desconocido")
k("j_interrupted", "interrompido", "interrupted", "interrumpido")

# ---------- vídeo, ciclo do dia e idioma (adicionados depois) ----------
k("m_video", "Vídeo", "Video", "Vídeo")
k("v_soon", "Veja o FM1 Quest em 5 minutos", "See FM1 Quest in 5 minutes", "Mira FM1 Quest en 5 minutos")
k("v_hint", "Clique para assistir", "Click to watch", "Haz clic para ver")
k("v_link", "Assistir no YouTube", "Watch on YouTube", "Ver en YouTube")
k("a_video", "Reproduzir o vídeo do FM1 Quest v0.2", "Play the FM1 Quest v0.2 video", "Reproducir el video de FM1 Quest v0.2")
k("c10_h", "Dia e noite na aventura", "Day and night in the adventure", "Día y noche en la aventura")
k("c10_p", "A aventura começa na vila, e a hora muda a cada vez que você liga o aparelho: o amanhecer e o fim da tarde são os favoritos, mas também pode ser tarde ou noite. No amanhecer, o sol sobe atrás das casas com círculos de brilho; à tarde, o sol brilha e tudo está iluminado; no pôr do sol, as tochas acendem conforme o grupo passa; à noite, aparece a lua crescente. Enquanto o grupo caminha, a música começa: quatro fileiras de blocos, uma para cada herói, formam o loop. Depois de alguns segundos (de 5 a 60, ajustável no menu Settings) o grupo segue sozinho para a floresta, onde enfrenta slimes e morcegos.",
  "The adventure starts in the village, and the hour changes every time you switch the device on: dawn and sunset are the favourites, but it can also be afternoon or night. At dawn the sun rises behind the houses with circles of light; in the afternoon the sun shines and everything is lit; at sunset the torches light up as the party walks past; at night a crescent moon appears. While the party walks, the music starts: four rows of blocks, one for each hero, build the loop. After a few seconds (5 to 60, adjustable in the Settings menu) the party goes on by itself into the forest, where it fights slimes and bats.",
  "La aventura empieza en el pueblo, y la hora cambia cada vez que enciendes el aparato: el amanecer y el atardecer son los favoritos, pero también puede ser tarde o noche. Al amanecer, el sol sube detrás de las casas con círculos de luz; por la tarde, el sol brilla y todo está iluminado; en el atardecer, las antorchas se encienden a medida que el grupo pasa; de noche aparece la luna creciente. Mientras el grupo camina, empieza la música: cuatro filas de bloques, una para cada héroe, forman el loop. Después de unos segundos (de 5 a 60, ajustable en el menú Settings) el grupo sigue solo hacia el bosque, donde se enfrenta a slimes y murciélagos.")
k("c10_a1", "Amanhecer na vila: o sol nasce atrás das casas e o painel de música com as quatro fileiras", "Dawn in the village: the sun rises behind the houses and the music panel with its four rows", "Amanecer en el pueblo: el sol sale detrás de las casas y el panel de música con sus cuatro filas")
k("c10_a2", "Poente na vila: as tochas acendem quando o grupo passa", "Sunset in the village: the torches light up as the party passes", "Atardecer en el pueblo: las antorchas se encienden cuando pasa el grupo")
k("c10_a3", "Os quatro horários: amanhecer, tarde, poente e noite", "The four hours: dawn, afternoon, sunset and night", "Las cuatro horas: amanecer, tarde, atardecer y noche")

# ---------- controles (abaixo do modelo 3D) ----------
k("m_ctrl", "Controles", "Controls", "Controles")
k("ct_h", "Controle o aparelho", "Control the device", "Controla el aparato")
k('ct_midi', 'Clique em <b>POWER ON</b> no aparelho (o som liga) e toque. Plugue o seu FM1 ou um teclado MIDI por USB e as notas dele tocam o emulador, junto com o teclado do computador.', 'Click <b>POWER ON</b> on the device (the sound starts) and play. Plug your FM1 or a MIDI keyboard by USB and its notes play the emulator, together with the computer keyboard.', 'Click <b>POWER ON</b> on the device (the sound starts) and play. Plug your FM1 or a MIDI keyboard by USB and its notes play the emulator, together with the computer keyboard.')
k("ct_kbd", "Também dá para tocar pelo teclado do computador. As teclas <kbd>1</kbd> a <kbd>0</kbd> tocam os pads 1 a 10; segure <kbd>Shift</kbd> e use <kbd>5</kbd> a <kbd>0</kbd> para os pads 11 a 16.",
  "You can also play with the computer keyboard. Keys <kbd>1</kbd> to <kbd>0</kbd> play pads 1 to 10; hold <kbd>Shift</kbd> and use <kbd>5</kbd> to <kbd>0</kbd> for pads 11 to 16.",
  "También puedes tocar con el teclado del ordenador. Las teclas <kbd>1</kbd> a <kbd>0</kbd> tocan los pads 1 a 10; mantén <kbd>Shift</kbd> y usa <kbd>5</kbd> a <kbd>0</kbd> para los pads 11 a 16.")
k("ct_btn", "Botões", "Buttons", "Botones")
k("ct_knob", "Os quatro knobs: <kbd>Shift</kbd>+<kbd>1</kbd> a <kbd>4</kbd>, depois mova o mouse para cima ou para baixo (ou role): o knob gira e a página não rola. <kbd>Esc</kbd> solta. Os outros quatro knobs: clique e arraste.",
  "The four knobs: <kbd>Shift</kbd>+<kbd>1</kbd> to <kbd>4</kbd>, then move the mouse up or down (or scroll): the knob turns and the page does not scroll. <kbd>Esc</kbd> releases it. The other four knobs: click and drag.",
  "Los cuatro knobs: <kbd>Shift</kbd>+<kbd>1</kbd> a <kbd>4</kbd>, luego mueve el ratón arriba o abajo (o desplaza): el knob gira y la página no se desplaza. <kbd>Esc</kbd> lo suelta. Los otros cuatro knobs: haz clic y arrastra.")
k('ct_tip', "Teclado do computador: A S D F G H J K L ; ' = teclas brancas, W E T Y U O P ] = pretas · 1 2 3 4 5 6 = FX SEL ENV LFO EDIT GLO · 7 8 9 0 = HOME SAVE ARP SEQ · Z X = OCT− OCT+ · Espaço = PLAY · R = REC · ↑↓ = SELECT · ←→ = ALGORITHM · [ ] = PRESETS. Os knobs 1–4 giram com o mouse (arraste ou use a roda).", "Computer keyboard: A S D F G H J K L ; ' = white keys, W E T Y U O P ] = black · 1 2 3 4 5 6 = FX SEL ENV LFO EDIT GLO · 7 8 9 0 = HOME SAVE ARP SEQ · Z X = OCT− OCT+ · Space = PLAY · R = REC · ↑↓ = SELECT · ←→ = ALGORITHM · [ ] = PRESETS. Knobs 1–4 turn with the mouse (drag or wheel).", "Computer keyboard: A S D F G H J K L ; ' = white keys, W E T Y U O P ] = black · 1 2 3 4 5 6 = FX SEL ENV LFO EDIT GLO · 7 8 9 0 = HOME SAVE ARP SEQ · Z X = OCT− OCT+ · Space = PLAY · R = REC · ↑↓ = SELECT · ←→ = ALGORITHM · [ ] = PRESETS. Knobs 1–4 turn with the mouse (drag or wheel).")
k('ct_note', 'Esse emulador roda o <b>firmware de verdade</b>, compilado pra WebAssembly: o que aparece aqui é o que o FM-1 faz. Os atalhos do teclado funcionam mesmo com um FM1 ou teclado MIDI plugado.', 'This emulator runs the <b>real firmware</b>, compiled to WebAssembly: what you see here is what the FM-1 does. The keyboard shortcuts keep working with an FM1 or a MIDI keyboard plugged in.', 'This emulator runs the <b>real firmware</b>, compiled to WebAssembly: what you see here is what the FM-1 does. The keyboard shortcuts keep working with an FM1 or a MIDI keyboard plugged in.')

# ---------- SEO e subtítulo dos controles ----------
k('ct_sub', 'com o seu M-VAVE FM1, um teclado MIDI ou o teclado do computador', 'with your M-VAVE FM1, a MIDI keyboard or the computer keyboard', 'with your M-VAVE FM1, a MIDI keyboard or the computer keyboard')
k("seo_kw", "FM1 Quest, M-VAVE FM-1, firmware, groovebox, SLOOP, Felucca, RPG, pixel art, instalar firmware, Web MIDI, sintetizador, FM-1 firmware personalizado",
  "FM1 Quest, M-VAVE FM-1, firmware, groovebox, SLOOP, Felucca, RPG, pixel art, install firmware, Web MIDI, synthesizer, FM-1 custom firmware",
  "FM1 Quest, M-VAVE FM-1, firmware, groovebox, SLOOP, Felucca, RPG, pixel art, instalar firmware, Web MIDI, sintetizador, firmware personalizado FM-1")
k("og_alt", "FM1 Quest: o seu groovebox como um RPG em pixel art", "FM1 Quest: your groovebox as a pixel-art RPG", "FM1 Quest: tu groovebox como un RPG en pixel art")
k("og_title", "FM1 Quest · seu groovebox como um RPG em pixel art", "FM1 Quest · your groovebox as a pixel-art RPG", "FM1 Quest · tu groovebox como un RPG en pixel art")

# ---------- botões MIDI e som no topo ----------
k("ov_midi", "Conectar MIDI", "Connect MIDI", "Conectar MIDI")
k("ov_snd", "Ligar o som", "Turn sound on", "Activar el sonido")
k("ov_snd_off", "Som ligado (clique para desligar)", "Sound on (click to turn off)", "Sonido activado (clic para desactivar)")
k("ov_snd_on", "Som desligado (clique para ligar)", "Sound off (click to turn on)", "Sonido desactivado (clic para activar)")

k("j_replug", "Desconecte e conecte o cabo USB do FM-1 uma vez: assim o computador reconhece a entrada de áudio USB.", "Unplug and plug the FM-1 USB cable back in once: that is how the computer finds its USB audio input.", "Desconecta y vuelve a conectar el cable USB del FM-1 una vez: así el ordenador encuentra su entrada de audio USB.")

# ---------- painel de envio (USB) e extração da gravação ----------
k("d_title", "FM-1 conectado", "FM-1 connected", "FM-1 conectado")
k("d_sub", "Os arquivos vão só pelo cabo USB para o aparelho. Nada é enviado para este site.", "Files go only over the USB cable to the device. Nothing is sent to this website.", "Los archivos van solo por el cable USB al aparato. No se envía nada a este sitio web.")
k("d_tab1", "Samples", "Samples", "Samples")
k("d_tab2", "Piano Hero", "Piano Hero", "Piano Hero")
k("d_tab3", "FM Hero", "FM Hero", "FM Hero")
k("d_slot", "Slot", "Slot", "Slot")
k("d_space", "Espaço dos samples", "Sample space", "Espacio de los samples")
k("d_send", "Enviar arquivo", "Send file", "Enviar archivo")
k("d_sending", "Enviando…", "Sending…", "Enviando…")
k("d_erase", "Apagar", "Erase", "Borrar")
k("d_erase_q", "Apagar o sample do slot USR{n}? Isso não dá para desfazer.", "Erase the sample in slot USR{n}? This cannot be undone.", "¿Borrar el sample del slot USR{n}? No se puede deshacer.")
k("d_empty", "vazio", "empty", "vacío")
k("d_ok1", "<ul><li>WAV (o melhor), MP3, OGG, FLAC, M4A</li><li>Sugestão: até 1 MB. O que vale é a duração: até 6 s</li><li>Mais longo? O site corta no limite e avisa</li><li>Vira mono, 22 kHz, 64 KiB por slot</li></ul>", "<ul><li>WAV (best), MP3, OGG, FLAC, M4A</li><li>Suggested: up to 1 MB. What counts is the length: up to 6 s</li><li>Longer? The site cuts it at the limit and tells you</li><li>Becomes mono, 22 kHz, 64 KiB per slot</li></ul>", "<ul><li>WAV (el mejor), MP3, OGG, FLAC, M4A</li><li>Sugerido: hasta 1 MB. Lo que cuenta es la duración: hasta 6 s</li><li>¿Más largo? El sitio lo corta en el límite y avisa</li><li>Pasa a mono, 22 kHz, 64 KiB por slot</li></ul>")
k("d_ok2", "<ul><li>Só .mid</li><li>Sugestão: até 10 KB (cabem até 960 notas)</li><li>Com notas demais, fica só a melodia</li><li>6 espaços; todos ocupados: escolha um para trocar</li></ul>", "<ul><li>.mid only</li><li>Suggested: up to 10 KB (up to 960 notes fit)</li><li>Too many notes: only the melody stays</li><li>6 places; all taken: pick one to replace</li></ul>", "<ul><li>Solo .mid</li><li>Sugerido: hasta 10 KB (caben hasta 960 notas)</li><li>Con demasiadas notas, queda solo la melodía</li><li>6 espacios; todos ocupados: elige uno para cambiar</li></ul>")
k("d_ok3", "<ul><li>Só .mid</li><li>Sugestão: até 10 KB (cabem até 960 notas)</li><li>Com notas demais, fica só a melodia</li><li>6 espaços; todos ocupados: escolha um para trocar</li></ul>", "<ul><li>.mid only</li><li>Suggested: up to 10 KB (up to 960 notes fit)</li><li>Too many notes: only the melody stays</li><li>6 places; all taken: pick one to replace</li></ul>", "<ul><li>Solo .mid</li><li>Sugerido: hasta 10 KB (caben hasta 960 notas)</li><li>Con demasiadas notas, queda solo la melodía</li><li>6 espacios; todos ocupados: elige uno para cambiar</li></ul>")
k("d_soon", "Em breve", "Coming soon", "Próximamente")
k("d_bad", "Arquivo não aceito: escolha um dos tipos listados.", "File not accepted: choose one of the listed types.", "Archivo no aceptado: elige uno de los tipos indicados.")
k("d_big", "Arquivo grande demais (máximo 8 MB).", "File too large (8 MB at most).", "Archivo demasiado grande (máximo 8 MB).")
k("d_done", "Enviado para {n}.", "Sent to {n}.", "Enviado a {n}.")
k("d_fail", "Falhou o envio: ", "Sending failed: ", "Falló el envío: ")
k("d_toobig", "O som é longo demais para o slot (até 6 s).", "The sound is too long for the slot (up to 6 s).", "El sonido es demasiado largo para el slot (hasta 6 s).")
k("d_space_s", "Músicas", "Songs", "Canciones")
k("d_erase_s", "Apagar", "Erase", "Borrar")
k("d_erase_s_q", "Apagar a música {n}? Isso não dá para desfazer.", "Erase song {n}? This cannot be undone.", "¿Borrar la canción {n}? No se puede deshacer.")
k("d_over_q", "{n} já tem “{name}”. Sobrescrever? (Para escolher outro lugar, cancele e clique em outro espaço.)", "{n} already has “{name}”. Write over it? (To pick another place, cancel and click another one.)", "{n} ya tiene “{name}”. ¿Sobrescribir? (Para elegir otro lugar, cancela y haz clic en otro espacio.)")
k("d_full", "Todos os 6 espaços estão ocupados: clique no que quer substituir e envie o arquivo.", "All 6 places are taken: click the one to replace and send the file.", "Los 6 espacios están ocupados: haz clic en el que quieres reemplazar y envía el archivo.")
k("d_old_fw", "Este firmware ainda não recebe músicas: instale o FM1 Quest v0.2.", "This firmware does not receive songs yet: install FM1 Quest v0.2.", "Este firmware aún no recibe canciones: instala FM1 Quest v0.2.")
k("d_s_many", "Música com notas demais (máximo 960, mesmo só com a melodia). Use uma versão mais curta ou simples.", "Too many notes (960 at most, even with the melody only). Use a shorter or simpler version.", "Demasiadas notas (máximo 960, incluso solo con la melodía). Usa una versión más corta o simple.")
k("d_s_empty", "Esse MIDI não tem notas.", "This MIDI has no notes.", "Este MIDI no tiene notas.")
k("d_s_smpte", "Esse MIDI usa tempo SMPTE, que não é aceito.", "This MIDI uses SMPTE time, which is not supported.", "Este MIDI usa tiempo SMPTE, que no se admite.")
k("d_s_long", "Essa música é longa demais.", "This song is too long.", "Esta canción es demasiado larga.")
k("d_erased", "{n} apagado.", "{n} erased.", "{n} borrado.")
k("d_conn_perm", "O navegador não liberou o MIDI. Clique no cadeado ao lado do endereço, permita o MIDI e tente de novo.", "The browser did not allow MIDI. Click the padlock next to the address, allow MIDI and try again.", "El navegador no permitió el MIDI. Haz clic en el candado junto a la dirección, permite el MIDI e inténtalo de nuevo.")
k("d_conn_none", "Não achei o FM-1. Confira o cabo USB e feche outros programas que usam o MIDI (editor, DAW) e tente de novo.", "FM-1 not found. Check the USB cable, close other programs that use MIDI (editor, DAW) and try again.", "No encuentro el FM-1. Revisa el cable USB, cierra otros programas que usen MIDI (editor, DAW) e inténtalo de nuevo.")
k("d_conn_noreply", "O FM-1 não respondeu. Reconecte o cabo USB e tente de novo.", "The FM-1 did not answer. Replug the USB cable and try again.", "El FM-1 no respondió. Vuelve a conectar el cable USB e inténtalo de nuevo.")
k("d_conn_fail", "Não consegui conectar ao FM-1: {n}", "Could not connect to the FM-1: {n}", "No pude conectar con el FM-1: {n}")
k("d_trim", "Cortado em {n} s (o limite do slot).", "Cut to {n} s (the slot limit).", "Cortado en {n} s (el límite del slot).")
k("d_play", "▶ Ouvir", "▶ Listen", "▶ Escuchar")
k("d_pause", "⏸ Pausar", "⏸ Pause", "⏸ Pausar")
k("d_hide", "Minimizar o painel de upload", "Minimise the upload panel", "Minimizar el panel de subida")
k("d_show", "Abrir o painel de upload", "Open the upload panel", "Abrir el panel de subida")
k("r_title", "Extrair gravação", "Extract recording", "Extraer grabación")
k("r_hint", "Grava o que o FM-1 toca (entrada de áudio USB). O arquivo fica só neste computador.", "Records what the FM-1 plays (USB audio input). The file stays on this computer only.", "Graba lo que toca el FM-1 (entrada de audio USB). El archivo se queda solo en este ordenador.")
k("r_rec", "Gravar", "Record", "Grabar")
k("r_stop", "Parar", "Stop", "Parar")
k("r_noin", "Entrada de áudio do FM-1 não encontrada. Confira o cabo e o menu USB AUDIO do FM-1 (MASTER ou FULL).", "The FM-1 audio input was not found. Check the cable and the FM-1's USB AUDIO menu (MASTER or FULL).", "No se encuentra la entrada de audio del FM-1. Revisa el cable y el menú USB AUDIO del FM-1 (MASTER o FULL).")
k("r_denied", "O navegador não liberou o microfone/entrada de áudio.", "The browser did not allow the microphone / audio input.", "El navegador no permitió el micrófono / entrada de audio.")
k("r_play", "Tocar", "Play", "Reproducir")
k("r_pause", "Pausar", "Pause", "Pausar")
k("r_vol", "Volume", "Volume", "Volumen")
k("r_dl", "Baixar WAV", "Download WAV", "Descargar WAV")
k("r_again", "Gravar de novo", "Record again", "Grabar de nuevo")

k("cl_snd_h", "Ver os sons que saíram do banco", "See the sounds that left the bank", "Ver los sonidos que salieron del banco")
k("cl_play", "ouvir", "play", "oír")
k("ig_tip", "<b>Side quests da vida real</b><br>Meu espaço pessoal para compartilhar o progresso dos meus estudos de tecnologia. Devo compartilhar meu processo aprendendo synth, melhorias e testes com o Quest e protótipos de robótica com ESP32.",
  "<b>Real-life side quests</b><br>My personal space to share the progress of my technology studies. I plan to share my process of learning synths, improvements and tests with Quest, and robotics prototypes with ESP32.",
  "<b>Side quests de la vida real</b><br>Mi espacio personal para compartir el progreso de mis estudios de tecnología. Voy a compartir mi proceso aprendiendo sintetizadores, mejoras y pruebas con Quest, y prototipos de robótica con ESP32.")
k("yt_tip", "<b>Demonstrações e Atualizações</b><br>Vídeos de atualizações do Quest em ação, guias de desenvolvimento para que mais pessoas possam criar com a sua criatividade e talvez até bastidores do que eu ando fazendo com robótica (estou estudando para montar uma arena de robôs feitos em impressão 3D e placa mãe barata com ESP32, numa vibe de Big Hero 6 com Megaman, rs).",
  "<b>Demos and Updates</b><br>Videos of Quest updates in action, development guides so more people can create with their own creativity, and maybe even behind the scenes of what I am doing with robotics (I am studying to build an arena of robots made with 3D printing and a cheap ESP32 board, in a Big Hero 6 meets Mega Man vibe, lol).",
  "<b>Demostraciones y actualizaciones</b><br>Videos de las actualizaciones de Quest en acción, guías de desarrollo para que más gente pueda crear con su creatividad y quizá hasta el detrás de cámaras de lo que ando haciendo con robótica (estoy estudiando para montar una arena de robots impresos en 3D con una placa barata ESP32, con vibra de Big Hero 6 y Mega Man, jaja).")

# ---------- cookies (Google Analytics only)
k("ck_text", "Este site usa cookies apenas para o Google Analytics (contar visitas e cliques). Nada é usado para anúncios.",
  "This site uses cookies only for Google Analytics (counting visits and clicks). Nothing is used for ads.",
  "Este sitio usa cookies solo para Google Analytics (contar visitas y clics). No se usa nada para anuncios.")
k("ck_yes", "Aceitar", "Accept", "Aceptar")
k("ck_no", "Recusar", "Decline", "Rechazar")
k("ck_open", "Cookies e estatísticas (Google Analytics)", "Cookies and statistics (Google Analytics)", "Cookies y estadísticas (Google Analytics)")
