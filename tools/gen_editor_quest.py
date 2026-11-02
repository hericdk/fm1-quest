#!/usr/bin/env python3
"""FM1 Quest's full web editor: the SLOOP editor (isod89/sloop-fm1, web/editor.html, GPL-3.0) with FM1 Quest's name and look.
Same SysEx, same file formats (a backup or a song moves between the firmwares); nothing of how it works changes.
  python3 tools/gen_editor.py   ->  build/editor-full/index.html
To update it: put the new upstream file in tools/editor/ and change SRC. Every replacement must match, or this stops."""
import json, os, re, shutil
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'tools/editor/editor-sloop-2.8.4.html')
OUT = os.path.join(ROOT, 'build/editor-full')
s = open(SRC, encoding='utf-8').read()

def sub(old, new):
    global s
    assert old in s, old[:70]
    s = s.replace(old, new)

def rx(pat, new):
    global s
    s, n = re.subn(pat, new, s, flags=re.S)
    assert n, pat[:70]

sub('<title>SLOOP Editor</title>', '<title>FM1 Quest Editor</title>')
# no install-as-app or offline copy: it lives on the site
rx(r'<!--PWA-->.*?<!--/PWA-->', '<meta name="theme-color" content="#1a1016">')
rx(r'<!--DL-->.*?<!--/DL-->', '')
rx(r'/\*PWA\*/.*?/\*/PWA\*/', '')
# the name
rx(r'<a class="brand" href="#sound" aria-label="SLOOP">.*?</a>',
   '<a class="brand" href="#sound" aria-label="FM1 Quest editor"><i></i><b>FM1 Quest</b><span>editor</span></a>')
sub('<a class="dim" href="../installer/" data-t="installer"></a>', '<a class="dim" href="../#install" data-t="installer"></a>')
sub('installer: "SLOOP インストーラー"', 'installer: "FM1 Quest インストーラー"')
sub('installer: "SLOOP installer"', 'installer: "FM1 Quest installer"')
sub('nodevice: "SLOOP が見つかりません。', 'nodevice: "FM-1 が見つかりません。')
sub('nodevice: "SLOOP not found. ', 'nodevice: "FM-1 not found. ')
# the credit stays (GPL-3.0)
sub('<footer>SLOOP, based on Felucca by Leo Kuroshita (@kurogedelic),',
    '<footer>The FM1 Quest editor is <a href="https://github.com/isod89/sloop-fm1" target="_blank" rel="noopener">the SLOOP editor by isod89</a> with another name and colours '
    '(<a href="https://github.com/hericdk/fm1-quest" target="_blank" rel="noopener">source</a>). SLOOP is based on Felucca by Leo Kuroshita (@kurogedelic),')
# the files it saves
sub('`SLOOP-backup-${today()}.json`', '`fm1quest-backup-${today()}.json`')
sub('`SLOOP-live-${stamp}.wav`', '`fm1quest-live-${stamp}.wav`')
sub('`sloop-track${tr + 1}.mid`', '`fm1quest-track${tr + 1}.mid`')
sub('name: `SLOOP track ${tr + 1}`', 'name: `FM1 Quest track ${tr + 1}`')
sub('`sloop-playlist-${today()}.sloopset`', '`fm1quest-playlist-${today()}.sloopset`')
sub('`sloop-fm6-bank-${today()}.syx`', '`fm1quest-fm6-bank-${today()}.syx`')
# the look: the site's navy and orange
sub('</head>', '''<style>
  @font-face { font-family: "Pixelify Sans"; src: url(../assets/PixelifySans-400.ttf) format("truetype"); font-weight: 400 700; }
  :root { --bg: #1a1016; --s1: #241820; --s2: #2e2026; --s3: #4a3c48; --line: #3a2c34; --dim: #a89080; --fg2: #e8d8c4; --fg: #f2f2f2; }
  .brand { align-items: center; gap: 8px; font-family: "Pixelify Sans", monospace; }
  .brand i { width: 14px; height: 20px; background: #f0c860; clip-path: polygon(45% 0, 100% 0, 62% 38%, 92% 38%, 20% 100%, 40% 52%, 8% 52%); }
  .brand b { font-size: 21px; font-weight: 500; color: #f0c860; }
  .brand span { padding: 0; font-size: 13px; }
  .langsel { background: #241820; color: #e8d8c4; border: 1px solid #3a2c34; border-radius: 6px; padding: 3px 6px; font-size: 13px; }
  .pri { background: #f0c860 !important; border-color: #f0c860 !important; color: #1a1016 !important; }
  [data-tab="theme"] { display: none !important; }   /* (the Quest screens have their own colours: a theme would not reach them) */
</style>
</head>''')

# the languages: English and Japanese come with it, the others from tools/editor/i18n/ (what is missing falls back to English)
extra = {}
for c in ('pt', 'ko', 'de', 'fr'):
    f = json.load(open(os.path.join(ROOT, 'tools/editor/i18n', c + '.json'), encoding='utf-8'))
    extra[c] = {k: v.replace('No Man Sky FM', 'FM1 Quest') for k, v in f.items()}
sub('let lang = (navigator.language || "en").toLowerCase().startsWith("ja") ? "ja" : "en";',
    'const EXTRA = ' + json.dumps(extra, ensure_ascii=False, separators=(',', ':')) + ';\n'
    'for (const c in EXTRA) TEXT[c] = Object.assign({}, TEXT.en, EXTRA[c]);\n'
    'let lang = "en"; try { lang = localStorage.getItem("fm1q-lang") || ""; } catch (e) {}\n'
    'if (!TEXT[lang]) { const n = (navigator.language || "en").toLowerCase().slice(0, 2); lang = TEXT[n] ? n : "en"; }')
sub('const t = (k) => TEXT[lang][k] ?? k;', 'const t = (k) => TEXT[lang][k] ?? TEXT.en[k] ?? k;')
sub('<button class="lang" id="lang"></button>', '<select class="langsel" id="lang" aria-label="Language"><option value="en">EN</option><option value="pt">PT</option><option value="ko">한국어</option><option value="ja">日本語</option><option value="de">DE</option><option value="fr">FR</option></select>')
sub('$("lang").textContent = t("other");', '$("lang").value = lang;')
sub('$("lang").addEventListener("click", () => { lang = lang === "ja" ? "en" : "ja"; applyLang(); });', '$("lang").addEventListener("change", (e) => { lang = e.target.value; try { localStorage.setItem("fm1q-lang", lang); } catch (x) {} applyLang(); });')
os.makedirs(OUT, exist_ok=True)
open(os.path.join(OUT, 'index.html'), 'w', encoding='utf-8').write(s)
for f in ('FUKIAI-LICENSE.txt', 'fukiai.ttf'):
    shutil.copy(os.path.join(ROOT, 'tools/editor', f), OUT)
print('ok', os.path.join(OUT, 'index.html'), len(s.encode()), 'B')
