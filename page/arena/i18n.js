/* FM1 Arena · idiomas. O idioma vem da página do site (<html lang>), da escolha salva (fm1lang) ou do navegador; a URL é sempre /arena/. */
(function () {
'use strict';
const A = window.A = window.A || {};
const SUP = ['pt', 'en', 'es', 'ja', 'ko'];
A.detectLang = function () {
  const norm = (s) => { s = String(s || '').toLowerCase().slice(0, 2); return SUP.includes(s) ? s : null; };
  let q = null; try { q = new URLSearchParams(location.search).get('lang'); } catch (e) {}
  if (norm(q)) return norm(q);
  try { const s = norm(localStorage.getItem('fm1lang')); if (s) return s; } catch (e) {}     // o idioma que a pessoa escolheu no site
  for (const l of (navigator.languages || [navigator.language])) { const n = norm(l); if (n) return n; }
  return 'en';
};
A.lang = A.detectLang();
const D = {
  tag: { pt: 'Dispute lutas ritmadas com seu FM1. Quem toca no ritmo e com harmonia causa mais dano.', en: 'Fight rhythm duels with your FM1. Play on the beat and in harmony to deal more damage.', es: 'Disputa duelos rítmicos con tu FM1. Quien toca en el ritmo y con armonía hace más daño.', ja: 'FM1でリズム対戦。ビートに合わせ、ハーモニーを奏でるほど大ダメージ。', ko: 'FM1으로 리듬 대결을 펼치세요. 박자와 화음에 맞출수록 더 큰 피해를 줍니다.' },
  c_h: { pt: 'Conecte seu FM1 para competir', en: 'Connect your FM1 to compete', es: 'Conecta tu FM1 para competir', ja: 'FM1を接続して対戦', ko: 'FM1을 연결해 대결하세요' },
  c_p: { pt: 'Plugue o FM1 por USB, depois toque em <b>Conectar FM1</b> e permita o acesso MIDI no navegador (Chrome, Edge ou Opera).', en: 'Plug the FM1 in by USB, then press <b>Connect FM1</b> and allow MIDI access in the browser (Chrome, Edge or Opera).', es: 'Conecta el FM1 por USB, pulsa <b>Conectar FM1</b> y permite el acceso MIDI en el navegador (Chrome, Edge u Opera).', ja: 'FM1をUSBで接続し、<b>FM1を接続</b>を押してブラウザのMIDIアクセスを許可してください（Chrome / Edge / Opera）。', ko: 'FM1을 USB로 연결한 뒤 <b>FM1 연결</b>을 누르고 브라우저의 MIDI 접근을 허용하세요(Chrome, Edge, Opera).' },
  c_btn: { pt: 'Conectar FM1', en: 'Connect FM1', es: 'Conectar FM1', ja: 'FM1を接続', ko: 'FM1 연결' },
  m_cpu: { pt: 'Player vs CPU', en: 'Player vs CPU', es: 'Jugador vs CPU', ja: 'プレイヤー vs CPU', ko: '플레이어 vs CPU' },
  m_pvp: { pt: 'Player vs Player', en: 'Player vs Player', es: 'Jugador vs Jugador', ja: 'プレイヤー vs プレイヤー', ko: '플레이어 vs 플레이어' },
  m_train: { pt: 'Training', en: 'Training', es: 'Entrenamiento', ja: 'トレーニング', ko: '트레이닝' },
  m_rank: { pt: 'Ranking', en: 'Ranking', es: 'Ranking', ja: 'ランキング', ko: '랭킹' },
  rk_conns: { pt: 'Conexões por firmware', en: 'Connections by firmware', es: 'Conexiones por firmware', ja: 'ファーム別接続数', ko: '펌웨어별 접속' },
  rk_ver: { pt: 'Top 5 versões', en: 'Top 5 versions', es: 'Top 5 versiones', ja: 'バージョン上位5', ko: '버전 상위 5' },
  rk_wins: { pt: 'Mais vitórias', en: 'Most wins', es: 'Más victorias', ja: '勝利数', ko: '최다 승리' },
  th_fw: { pt: 'Firmware', en: 'Firmware', es: 'Firmware', ja: 'ファーム', ko: '펌웨어' },
  th_conns: { pt: 'Conexões', en: 'Connections', es: 'Conexiones', ja: '接続', ko: '접속' },
  th_wins: { pt: 'Vitórias', en: 'Wins', es: 'Victorias', ja: '勝利', ko: '승리' },
  rk_demo: { pt: 'Números de demonstração (teste local). No ar, o ranking guarda só estes contadores, sem dados pessoais.', en: 'Demo numbers (local test). Live, the ranking keeps only these counters, no personal data.', es: 'Números de demostración (prueba local). En línea, el ranking guarda solo estos contadores, sin datos personales.', ja: 'デモ用の数値（ローカルテスト）。公開時は個人情報を含まないカウンターのみ保存します。', ko: '데모용 수치(로컬 테스트). 공개 시에는 개인정보 없이 카운터만 저장합니다.' },
  lgpd: { pt: 'Privacidade (LGPD): ao plugar o FM1 precisamos saber qual firmware ele usa, só para somar ao ranking e à contagem de acessos por firmware. Não guardamos nome, e-mail, IP nem as músicas que você toca; o ranking guarda apenas números.', en: 'Privacy: when you plug in the FM1 we need to know which firmware it runs, only to add it to the ranking and the per-firmware access count. We do not store your name, e-mail, IP or the music you play; the ranking keeps only numbers.', es: 'Privacidad: al conectar el FM1 necesitamos saber qué firmware usa, solo para sumarlo al ranking y al conteo de accesos por firmware. No guardamos nombre, correo, IP ni la música que tocas; el ranking guarda solo números.', ja: 'プライバシー: FM1を接続すると、ランキングとファーム別アクセス数に加えるためだけに、使用中のファームウェアを確認します。名前・メール・IP・演奏内容は保存せず、数値のみを保持します。', ko: '개인정보: FM1을 연결하면 랭킹과 펌웨어별 접속 수 집계를 위해서만 사용 중인 펌웨어를 확인합니다. 이름, 이메일, IP, 연주 내용은 저장하지 않으며 숫자만 보관합니다.' },
  credits: { pt: 'Sprites dos lutadores: craftpix.com · Cenários: spriters-resource.com', en: 'Fighter sprites: craftpix.com · Stages: spriters-resource.com', es: 'Sprites de luchadores: craftpix.com · Escenarios: spriters-resource.com', ja: 'キャラクタースプライト: craftpix.com ・ ステージ: spriters-resource.com', ko: '캐릭터 스프라이트: craftpix.com · 스테이지: spriters-resource.com' },
  lb_h: { pt: 'Salas', en: 'Rooms', es: 'Salas', ja: 'ルーム', ko: '룸' },
  lb_sub: { pt: 'Escolha o formato. A sala só existe enquanto alguém estiver dentro dela.', en: 'Pick a format. A room only exists while someone is inside it.', es: 'Elige el formato. La sala solo existe mientras haya alguien dentro.', ja: '形式を選んでください。ルームは誰かが中にいる間だけ存在します。', ko: '형식을 선택하세요. 룸은 누군가 안에 있는 동안만 존재합니다.' },
  lb_1: { pt: '1 × 1 (2 jogadores)', en: '1 × 1 (2 players)', es: '1 × 1 (2 jugadores)', ja: '1 × 1（2人）', ko: '1 × 1 (2명)' },
  lb_2: { pt: '2 × 2 (4 jogadores)', en: '2 × 2 (4 players)', es: '2 × 2 (4 jugadores)', ja: '2 × 2（4人）', ko: '2 × 2 (4명)' },
  lb_back: { pt: 'Voltar', en: 'Back', es: 'Volver', ja: '戻る', ko: '뒤로' },
  lb_create: { pt: 'Criar sua sala', en: 'Create your room', es: 'Crear tu sala', ja: 'ルームを作る', ko: '룸 만들기' },
  lb_join: { pt: 'Procurar sala', en: 'Find a room', es: 'Buscar sala', ja: 'ルームを探す', ko: '룸 찾기' },
  lb_nofree: { pt: 'Nenhuma sala com vaga agora. Toque em <b>Criar sua sala</b> e espere um adversário.', en: 'No room with a free seat right now. Press <b>Create your room</b> and wait for an opponent.', es: 'No hay salas con lugar ahora. Pulsa <b>Crear tu sala</b> y espera a un rival.', ja: '空きのあるルームはありません。<b>ルームを作る</b>を押して対戦相手を待ちましょう。', ko: '지금 빈자리가 있는 룸이 없습니다. <b>룸 만들기</b>를 누르고 상대를 기다리세요.' },
  lb_you: { pt: 'Sua sala', en: 'Your room', es: 'Tu sala', ja: 'あなたのルーム', ko: '내 룸' },
  lb_wait: { pt: 'Esperando {n} jogador(es). Outra aba ou computador abre <b>Salas</b> e entra aqui.', en: 'Waiting for {n} player(s). Another tab or computer opens <b>Rooms</b> and joins here.', es: 'Esperando {n} jugador(es). Otra pestaña u ordenador abre <b>Salas</b> y entra aquí.', ja: 'あと{n}人を待っています。別のタブやPCで<b>ルーム</b>を開いて参加してください。', ko: '{n}명을 기다리는 중입니다. 다른 탭이나 컴퓨터에서 <b>룸</b>을 열어 입장하세요.' },
  lb_rename: { pt: 'Renomear', en: 'Rename', es: 'Renombrar', ja: '名前変更', ko: '이름 변경' },
  lb_leave: { pt: 'Sair da sala', en: 'Leave room', es: 'Salir de la sala', ja: 'ルームを出る', ko: '룸 나가기' },
  lb_arena: { pt: 'Arena:', en: 'Arena:', es: 'Arena:', ja: 'アリーナ:', ko: '아레나:' },
  lb_enter: { pt: 'Entrar', en: 'Join', es: 'Entrar', ja: '入る', ko: '입장' },
  lb_fight: { pt: 'Em luta', en: 'Fighting', es: 'En lucha', ja: '対戦中', ko: '대결 중' },
  lb_none: { pt: 'Nenhuma sala aberta. Toque em <b>Criar sua sala</b> e espere um adversário.', en: 'No open rooms. Press <b>Create your room</b> and wait for an opponent.', es: 'No hay salas abiertas. Pulsa <b>Crear tu sala</b> y espera a un rival.', ja: '開いているルームはありません。<b>ルームを作る</b>を押して待ちましょう。', ko: '열린 룸이 없습니다. <b>룸 만들기</b>를 누르고 기다리세요.' },
  co1: { pt: 'conexão', en: 'connection', es: 'conexión', ja: '接続', ko: '접속' },
  co: { pt: 'conexões', en: 'connections', es: 'conexiones', ja: '接続', ko: '접속' },
  me: { pt: 'você', en: 'you', es: 'tú', ja: 'あなた', ko: '나' },
  pl_help: { pt: 'O som do seu FM1 sai pelo próprio FM1. O navegador toca só o som do adversário e grava tudo no vídeo. Seus golpes: {c}.', en: 'The sound of your FM1 comes from the FM1 itself. The browser only plays your opponent and records everything for the video. Your hits: {c}.', es: 'El sonido de tu FM1 sale del propio FM1. El navegador solo reproduce al rival y graba todo en el vídeo. Tus golpes: {c}.', ja: 'あなたのFM1の音はFM1本体から出ます。ブラウザは相手の音だけを鳴らし、動画に録音します。操作キャラ: {c}。', ko: '내 FM1의 소리는 FM1에서 나옵니다. 브라우저는 상대의 소리만 재생하고 영상에 함께 녹화합니다. 조작 캐릭터: {c}.' },
  pl_c2: { pt: 'bonecos 1 e 2', en: 'fighters 1 and 2', es: 'luchadores 1 y 2', ja: 'キャラ1・2', ko: '캐릭터 1·2' },
  pl_call: { pt: 'Chamar pra luta', en: 'Call to fight', es: 'Llamar a pelear', ja: '対戦を呼びかける', ko: '대결 신청' },
  pl_exit: { pt: 'Sair', en: 'Leave', es: 'Salir', ja: '退出', ko: '나가기' },
  pl_wp: { pt: 'Esperando jogadores ({a}/{b})', en: 'Waiting for players ({a}/{b})', es: 'Esperando jugadores ({a}/{b})', ja: 'プレイヤー待ち ({a}/{b})', ko: '플레이어 대기 중 ({a}/{b})' },
  pl_wh: { pt: 'Esperando o dono da sala chamar para a luta...', en: 'Waiting for the room owner to call the fight...', es: 'Esperando que el dueño de la sala llame a la lucha...', ja: 'ルーム主の呼びかけを待っています…', ko: '룸 주인이 대결을 신청하길 기다리는 중…' },
  pl_calling: { pt: 'Chamando pra luta...', en: 'Calling the fight...', es: 'Llamando a la lucha...', ja: '呼びかけ中…', ko: '대결 신청 중…' },
  pl_all: { pt: 'Esperando todos aceitarem.', en: 'Waiting for everyone to accept.', es: 'Esperando que todos acepten.', ja: '全員の承認を待っています。', ko: '모두의 수락을 기다리는 중입니다.' },
  pl_called: { pt: 'Luta chamada!', en: 'Fight called!', es: '¡Lucha convocada!', ja: '対戦の呼びかけ！', ko: '대결 신청이 왔어요!' },
  pl_accept: { pt: 'Aceitar', en: 'Accept', es: 'Aceptar', ja: '承認', ko: '수락' },
  pl_others: { pt: 'Esperando os outros...', en: 'Waiting for the others...', es: 'Esperando a los demás...', ja: '他のプレイヤーを待っています…', ko: '다른 플레이어를 기다리는 중…' },
  r_win: { pt: 'VITÓRIA!', en: 'VICTORY!', es: '¡VICTORIA!', ja: '勝利！', ko: '승리!' },
  r_lose: { pt: 'DERROTA', en: 'DEFEAT', es: 'DERROTA', ja: '敗北', ko: '패배' },
  r_draw: { pt: 'EMPATE', en: 'DRAW', es: 'EMPATE', ja: '引き分け', ko: '무승부' },
  r_life: { pt: 'Vida', en: 'HP', es: 'Vida', ja: 'HP', ko: 'HP' },
  r_save: { pt: 'Salvar vídeo (MP4)', en: 'Save video (MP4)', es: 'Guardar vídeo (MP4)', ja: '動画を保存 (MP4)', ko: '영상 저장 (MP4)' },
  r_share: { pt: 'Compartilhar', en: 'Share', es: 'Compartir', ja: 'シェア', ko: '공유' },
  r_back: { pt: 'Voltar', en: 'Back', es: 'Volver', ja: '戻る', ko: '뒤로' },
  r_note: { pt: 'O vídeo é gerado no seu navegador e <b>não fica salvo no site</b>: ele é entregue a você e apagado em seguida.', en: 'The video is made in your browser and is <b>not stored on the site</b>: it is handed to you and then deleted.', es: 'El vídeo se genera en tu navegador y <b>no se guarda en el sitio</b>: se te entrega y se borra enseguida.', ja: '動画はブラウザ内で生成され、<b>サイトには保存されません</b>。お渡しした後に削除されます。', ko: '영상은 브라우저에서 생성되며 <b>사이트에 저장되지 않습니다</b>. 전달 후 바로 삭제됩니다.' },
  r_webm: { pt: 'Este navegador só gera WebM; no Chrome 126+, Edge e Safari o vídeo sai em MP4.', en: 'This browser only makes WebM; Chrome 126+, Edge and Safari produce MP4.', es: 'Este navegador solo genera WebM; en Chrome 126+, Edge y Safari el vídeo sale en MP4.', ja: 'このブラウザはWebMのみ対応です。Chrome 126以降・Edge・SafariではMP4になります。', ko: '이 브라우저는 WebM만 지원합니다. Chrome 126+, Edge, Safari에서는 MP4로 저장됩니다.' },
  r_nosh: { pt: 'Este navegador não compartilha arquivos direto: o vídeo foi salvo para você postar onde quiser.', en: 'This browser cannot share files directly: the video was saved so you can post it anywhere.', es: 'Este navegador no comparte archivos directamente: el vídeo se guardó para que lo publiques donde quieras.', ja: 'このブラウザはファイルを直接共有できないため、動画を保存しました。お好きな場所に投稿してください。', ko: '이 브라우저는 파일을 바로 공유할 수 없어 영상을 저장했습니다. 원하는 곳에 올려 주세요.' },
  r_nogr: { pt: 'Gravação indisponível neste navegador', en: 'Recording is not available in this browser', es: 'Grabación no disponible en este navegador', ja: 'このブラウザでは録画できません', ko: '이 브라우저에서는 녹화할 수 없습니다' },
  sh_t: { pt: 'Minha luta na FM1 Arena', en: 'My fight on the FM1 Arena', es: 'Mi lucha en la FM1 Arena', ja: 'FM1 アリーナでの対戦', ko: 'FM1 아레나에서의 내 대결' },
  sh_x: { pt: 'Joguei a FM1 Arena! fm1.quest', en: 'I played the FM1 Arena! fm1.quest', es: '¡Jugué en la FM1 Arena! fm1.quest', ja: 'FM1 アリーナで遊んだよ！ fm1.quest', ko: 'FM1 아레나를 플레이했어요! fm1.quest' },
  tr_help: { pt: 'Toque a tecla do número da barra quando ela chegar na linha dourada.', en: 'Press the key with the number on the bar when it reaches the gold line.', es: 'Toca la tecla con el número de la barra cuando llegue a la línea dorada.', ja: 'バーの数字のキーを、金色のラインに来たら押してください。', ko: '막대의 숫자 키를 금색 선에 닿을 때 누르세요.' },
  tr_hits: { pt: 'ACERTOS', en: 'HITS', es: 'ACIERTOS', ja: 'ヒット', ko: '적중' },
  tr_miss: { pt: 'ERROS', en: 'MISSES', es: 'FALLOS', ja: 'ミス', ko: '실수' },
  tr_dmg: { pt: 'DANO', en: 'DAMAGE', es: 'DAÑO', ja: 'ダメージ', ko: '피해' },
  tr_keys: { pt: 'números = teclas do FM1 (1 à esquerda)', en: 'numbers = FM1 keys (1 on the left)', es: 'números = teclas del FM1 (1 a la izquierda)', ja: '数字 = FM1の鍵盤（左が1）', ko: '숫자 = FM1 건반 (왼쪽이 1)' },
  tr_next: { pt: 'Trocar de música', en: 'Next song', es: 'Cambiar de canción', ja: '次の曲へ', ko: '다음 곡' },
  tr_song: { pt: 'Tocando agora: música {n} de {t} ({name})', en: 'Now playing: song {n} of {t} ({name})', es: 'Sonando ahora: canción {n} de {t} ({name})', ja: '再生中: {t}曲中 {n}曲目（{name}）', ko: '재생 중: {t}곡 중 {n}번째 ({name})' },
  sn_banda: { pt: 'a linha do baixo das lutas', en: 'the fight bassline', es: 'la línea de bajo de las luchas', ja: 'バトルのベースライン', ko: '전투 베이스라인' },
  sn_escala: { pt: 'escala', en: 'scale', es: 'escala', ja: 'スケール', ko: '스케일' },
  sn_groove: { pt: 'groove', en: 'groove', es: 'groove', ja: 'グルーヴ', ko: '그루브' },
  sn_arpejo: { pt: 'arpejo', en: 'arpeggio', es: 'arpegio', ja: 'アルペジオ', ko: '아르페지오' },
  sn_rapido: { pt: 'rápida', en: 'fast', es: 'rápida', ja: '速い曲', ko: '빠른 곡' },
  ph_wait: { pt: 'aguardando', en: 'waiting', es: 'esperando', ja: '待機', ko: '대기' },
  ph_dice: { pt: 'sorteio', en: 'draw', es: 'sorteo', ja: '抽選', ko: '추첨' },
  ph_prep: { pt: 'preparar', en: 'get ready', es: 'prepararse', ja: '準備', ko: '준비' },
  ph_count: { pt: 'contagem', en: 'count-in', es: 'cuenta', ja: 'カウント', ko: '카운트' },
  ph_turn: { pt: 'tocar!', en: 'play!', es: '¡toca!', ja: '演奏！', ko: '연주!' },
  ph_end: { pt: 'fim', en: 'end', es: 'fin', ja: '終了', ko: '종료' },
  side_a: { pt: 'LADO A', en: 'SIDE A', es: 'LADO A', ja: 'サイドA', ko: 'A팀' },
  side_b: { pt: 'LADO B', en: 'SIDE B', es: 'LADO B', ja: 'サイドB', ko: 'B팀' },
  g_prep: { pt: 'PREPARE-SE', en: 'GET READY', es: '¡PREPÁRATE!', ja: '準備しよう', ko: '준비하세요' },
  g_next: { pt: 'PRÓXIMO: {s}', en: 'NEXT: {s}', es: 'SIGUE: {s}', ja: '次: {s}', ko: '다음: {s}' },
  g_even: { pt: 'PAR → COMEÇA A ESQUERDA', en: 'EVEN → LEFT STARTS', es: 'PAR → EMPIEZA LA IZQUIERDA', ja: '偶数 → 左から', ko: '짝수 → 왼쪽 시작' },
  g_odd: { pt: 'ÍMPAR → COMEÇA A DIREITA', en: 'ODD → RIGHT STARTS', es: 'IMPAR → EMPIEZA LA DERECHA', ja: '奇数 → 右から', ko: '홀수 → 오른쪽 시작' },
  g_wins: { pt: '{s} VENCE', en: '{s} WINS', es: 'GANA {s}', ja: '{s}の勝ち', ko: '{s} 승리' },
  g_ult: { pt: 'ESPECIAL: SEGURE UMA TECLA', en: 'SPECIAL: HOLD A KEY', es: 'ESPECIAL: MANTÉN UNA TECLA', ja: '必殺: キーを長押し', ko: '필살기: 키를 길게 누르세요' },
  wm: { pt: 'Filmado no FM1.Quest', en: 'Filmed on FM1.Quest', es: 'Filmado en FM1.Quest', ja: 'FM1.Quest で撮影', ko: 'FM1.Quest에서 촬영' },
  left: { pt: 'O jogador saiu', en: 'Player left', es: 'El jugador salió', ja: 'プレイヤーが退出しました', ko: '플레이어가 나갔습니다' },
  lb_find: { pt: 'Procurar sala', en: 'Find a room', es: 'Buscar sala', ja: 'ルームを探す', ko: '룸 찾기' },
  lb_enter_room: { pt: 'Entrar na sala', en: 'Enter the room', es: 'Entrar a la sala', ja: 'ルームに入る', ko: '룸에 입장' },
  host_left: { pt: 'O dono da sala saiu. A luta terminou.', en: 'The room owner left. The fight is over.', es: 'El dueño de la sala salió. La lucha terminó.', ja: 'ルーム主が退出しました。対戦は終了です。', ko: '룸 주인이 나갔습니다. 대결이 종료되었습니다.' },
  rk_local: { pt: 'Estes números são só deste aparelho: o ranking compartilhado liga quando o serviço do site estiver conectado.', en: 'These numbers are for this device only: the shared ranking turns on when the site service is connected.', es: 'Estos números son solo de este dispositivo: el ranking compartido se activa cuando el servicio del sitio esté conectado.', ja: 'この数値はこの端末のみのものです。共有ランキングはサイトのサービスに接続されると有効になります。', ko: '이 수치는 이 기기에서만 집계됩니다. 공유 랭킹은 사이트 서비스가 연결되면 켜집니다.' },
  rk_sw: { pt: 'trocaram para o Quest depois de lutar com outro firmware', en: 'switched to Quest after fighting with another firmware', es: 'cambiaron a Quest tras pelear con otro firmware', ja: '他のファームで対戦後にQuestへ切り替え', ko: '다른 펌웨어로 대결한 뒤 Quest로 전환' },
  lb_full: { pt: 'Essa sala encheu ou não está mais disponível. Tente outra.', en: 'That room is full or no longer available. Try another.', es: 'Esa sala se llenó o ya no está disponible. Prueba otra.', ja: 'そのルームは満員か、すでに利用できません。別のルームを試してください。', ko: '그 룸은 가득 찼거나 더 이상 사용할 수 없습니다. 다른 룸을 시도하세요.' },
  fw_other: { pt: 'Outro', en: 'Other', es: 'Otro', ja: 'その他', ko: '기타' },
  fw_is: { pt: 'Firmware:', en: 'Firmware:', es: 'Firmware:', ja: 'ファーム:', ko: '펌웨어:' },
  au_ok: { pt: 'Som do FM1 conectado: o vídeo grava o áudio real, com os efeitos.', en: 'FM1 audio connected: the video records the real sound, effects included.', es: 'Audio del FM1 conectado: el vídeo graba el sonido real, con efectos.', ja: 'FM1の音声を接続しました。エフェクト込みの実音を録画します。', ko: 'FM1 오디오 연결됨: 이펙트가 포함된 실제 소리를 녹화합니다.' },
  au_no: { pt: 'Não achei o áudio USB do FM1: o vídeo vai levar só o som sintetizado do navegador.', en: 'FM1 USB audio not found: the video will carry only the browser synth sound.', es: 'No encontré el audio USB del FM1: el vídeo llevará solo el sonido sintetizado del navegador.', ja: 'FM1のUSB音声が見つかりません。動画にはブラウザの合成音のみが入ります。', ko: 'FM1 USB 오디오를 찾지 못했습니다. 영상에는 브라우저 합성음만 담깁니다.' },
  dev: { pt: 'modo teste · firmware:', en: 'test mode · firmware:', es: 'modo prueba · firmware:', ja: 'テストモード・ファーム:', ko: '테스트 모드 · 펌웨어:' },
  dev_k: { pt: 'teclas de teste na tela (não existem no jogo)', en: 'on-screen test keys (not part of the game)', es: 'teclas de prueba en pantalla (no existen en el juego)', ja: '画面上のテストキー（ゲームには含まれません）', ko: '화면 테스트 키(게임에는 없음)' },
};
A.t = function (k, v) {
  const e = D[k]; if (!e) return k;
  let s = e[A.lang] || e.en || e.pt || k;
  if (v) for (const x in v) s = s.split('{' + x + '}').join(v[x]);
  return s;
};
A.applyI18n = function (root) {
  (root || document).querySelectorAll('[data-i]').forEach(el => { el.innerHTML = A.t(el.dataset.i); });
  document.documentElement.lang = A.lang;
};
})();
