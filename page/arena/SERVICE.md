# Serviço do ranking (opcional) — contrato

A arena funciona sem ele (os números ficam só no aparelho). Para o ranking compartilhado, aponte `web/config.js`:

    window.ARENA_CONFIG = { service: 'https://SEU-SERVICO' };

Endpoints (JSON, CORS liberado para https://fm1.quest):

- `GET  /rank` → `{ "conns": {"quest": 12, "quest_sw": 3, "sloop": 8, ...}, "wins": {"quest": 5, ...}, "ver": {"quest 0.2": 9, "quest_sw 0.2": 3, ...} }`
- `POST /rank/hit`  `{ "id": "<hash anônimo da pessoa>", "fw": "quest|quest_sw|sloop|felucca|baudgirl|mvave|other", "ver": "0.2" }`
  - o servidor também deve ignorar o mesmo `id`+`fw` por 24 h (o navegador já faz isso, o servidor é a segunda trava).
- `POST /rank/win`  `{ "fw": "quest|..." }`

Regras que o cliente já aplica: conta só quando a pessoa entra numa luta Player vs Player com um FM1 conectado; 1x por firmware a cada 24 h; quem passa para o FM1 Quest depois de lutar com outro firmware entra como `quest_sw` (uma vez) e a tela soma os dois em "FM1 Quest", mostrando quantos trocaram.
Nada pessoal é enviado: `id` é um hash e não há nome, e-mail, IP guardado nem música.
