#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The FM1 Quest page: installer + change log + emulator, as static files (GitHub Pages).

    python3 page/build_page.py OUT_DIR PACKAGE.fwsc VERSION

writes OUT_DIR/ index.html (pt), en/index.html, es/index.html, assets/, media/, editor/ (backup, sounds, songs; the page's upload panel talks to it),
emulator/ (quest/web: the real firmware in WebAssembly, build it first with `cd quest && CC=zig sh build.sh`), firmware/<package>, robots, sitemap.
Nothing here links to the lab, the hub or any test page."""
import hashlib, html as H, json, re, shutil, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
sys.path.insert(0, str(HERE))
import i18n, changelog  # noqa: E402
T = i18n.T
BLK, KEEP, BLOCKS = 0x30, 0x2F, 20


def strip_module(src):
    src = re.sub(r"^export\s+", "", src, flags=re.M)
    return re.sub(r"^import .*?;\n", "", src, flags=re.M)


def product_of(raw):                                           # the package identity (web/make_site.py, fm1pkg.js productOf)
    return "".join(chr((m - i - 1) & 0xFF) for i in range(BLOCKS) if (m := raw[i * BLK + KEEP]) != 0x7D)


def build_sim(out):
    """the 3D model (page/sim-src, embed mode) with the real firmware behind its glass: quest/web/fm1quest.wasm in an AudioWorklet (sim-src/lab-fm1w.js)"""
    wasm = REPO / "quest" / "web" / "fm1quest.wasm"
    if not wasm.exists():
        raise SystemExit("quest/web/fm1quest.wasm is missing: build the emulator first (cd quest && CC=zig sh build.sh)")
    SIM = out / "sim"
    shutil.rmtree(SIM, ignore_errors=True)
    shutil.copytree(HERE / "sim-src", SIM)
    shutil.copy(wasm, SIM / "fm1quest.wasm")
    # Pages caches files for 10 minutes: version the glass script and the preview clips by content, so a new deploy is never shown stale
    h = hashlib.sha256()
    for f in sorted(SIM.glob("*")):
        if f.is_file():
            h.update(f.name.encode() + f.read_bytes())
    v = h.hexdigest()[:10]
    js = (SIM / "lab-fm1w.js").read_text(encoding="utf-8").replace("'__VQ__'", f"'?v={v}'")
    (SIM / "lab-fm1w.js").write_text(js, encoding="utf-8")
    ix = (SIM / "index.html").read_text(encoding="utf-8")
    assert 'src="lab-fm1w.js"' in ix
    (SIM / "index.html").write_text(ix.replace('src="lab-fm1w.js"', f'src="lab-fm1w.js?v={v}"'), encoding="utf-8")
    return v


def main(out, pkg, version):
    out, pkg = Path(out), Path(pkg)
    raw = pkg.read_bytes()
    product = product_of(raw)
    if not re.fullmatch(r"FM-1_9\d\d", product):
        raise SystemExit(f"{pkg}: identity {product!r} is not a Felucca package")
    name = "fm1-quest-" + re.sub(r"[^A-Za-z0-9.-]", "-", version).lower() + ".fwsc"
    out.mkdir(parents=True, exist_ok=True)
    (out / "firmware").mkdir(exist_ok=True)
    for old in (out / "firmware").glob("*.fwsc"):                    # web/make_site.py already put this package there (the plain installer uses it): keep that file, once
        if old.read_bytes() == raw:
            name = old.name
        else:
            old.unlink()
    if not (out / "firmware" / name).exists():
        shutil.copy(pkg, out / "firmware" / name)
    meta = {"version": version, "product": product, "pkg": "firmware/" + name, "sha256": hashlib.sha256(raw).hexdigest()}
    for d in ("assets", "media"):
        shutil.rmtree(out / d, ignore_errors=True)
        shutil.copytree(HERE / d, out / d)
    for f in ("favicon.ico", "favicon-32.png", "apple-touch-icon.png", "android-chrome-192.png", "android-chrome-512.png"):
        shutil.copy(HERE / f, out / f)
    simv = build_sim(out)

    lib = strip_module((REPO / "web" / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + strip_module((REPO / "web" / "fm1ota.js").read_text(encoding="utf-8"))
    esc = lambda v: H.escape(v, quote=True)
    tpl = (HERE / "index.template.html").read_text(encoding="utf-8")
    LANGS = {"pt": ("pt-BR", "pt_BR", ""), "en": ("en", "en_US", "en/"), "es": ("es", "es_ES", "es/")}
    BASE = "https://hericdk.github.io/fm1-quest/"

    def seo(lang, P):
        htmllang, loc, path = LANGS[lang]; url = BASE + path; g = lambda k: T[k][lang]
        alts = "".join(f'<link rel="alternate" hreflang="{l}" href="{BASE + LANGS[l][2]}">\n' for l in LANGS) + f'<link rel="alternate" hreflang="x-default" href="{BASE}">\n'
        return (f'<title>{esc(g("title"))}</title>\n<meta name="description" content="{esc(g("desc"))}">\n<meta name="robots" content="index, follow">\n'
                f'<meta name="theme-color" content="#1e1418">\n<meta name="color-scheme" content="dark">\n<link rel="canonical" href="{url}">\n{alts}'
                f'<meta property="og:type" content="website">\n<meta property="og:title" content="{esc(g("og_title"))}">\n<meta property="og:description" content="{esc(g("desc"))}">\n'
                f'<meta property="og:url" content="{url}">\n<meta property="og:image" content="{BASE}assets/og-image.png">\n'
                f'<link rel="icon" href="{P}favicon.ico" sizes="any">\n<link rel="icon" type="image/png" sizes="32x32" href="{P}favicon-32.png">\n'
                f'<link rel="apple-touch-icon" sizes="180x180" href="{P}apple-touch-icon.png">\n')

    def changelog_html(lang, P):
        L = "pt" if lang == "pt" else "en"                                # (Spanish reads the English text)
        parts = ['<div class="clog">']
        for e in changelog.ENTRIES:
            parts.append(f'<article class="rv" id="cl-{e["id"]}"><h3>{e[L + "_t"]}</h3>')
            if e["clips"]:
                def fig(c):
                    name, cap = (c, "") if isinstance(c, str) else (c[0], c[1] if L == "pt" else c[2])
                    return (f'<figure><video src="{P}media/{name}.mp4" autoplay loop muted playsinline preload="metadata" aria-hidden="true"></video>'
                            + (f'<figcaption>{cap}</figcaption>' if cap else "") + "</figure>")
                parts.append('<div class="clips">' + "".join(fig(c) for c in e["clips"]) + "</div>")
            parts.append(f'<p>{e[L]}</p>')
            if e["id"] == "sounds":
                items = "".join(
                    f'<li><button type="button" data-snd="{P}media/audio/{n}.mp3">&#9654; {T["cl_play"][lang]}</button><b>{pt if L == "pt" else en}</b><span class="sz">{sz}</span></li>'
                    for n, pt, en, sz in changelog.SOUNDS)
                parts.append(f'<details><summary>{T["cl_snd_h"][lang]}</summary><ul class="snd">{items}</ul></details>')
            parts.append("</article>")
        parts.append("</div>")
        parts.append('<script>document.addEventListener("click",function(e){var b=e.target.closest("[data-snd]");if(!b)return;'
                     'var a=window.__cl||(window.__cl=new Audio());a.src=b.dataset.snd;a.play();});</script>')
        return "\n".join(parts)

    for lang, (htmllang, loc, path) in LANGS.items():
        P = "" if lang == "pt" else "../"
        html = tpl.replace('<html lang="pt-BR">', f'<html lang="{htmllang}">').replace("<!--SEO-->", seo(lang, P))
        html = html.replace("<!--CHANGELOG-->", changelog_html(lang, P))

        def need(key):
            assert key in T, "i18n.py has no key " + key
            return T[key][lang]
        html = re.sub(r'(<(\w+)\b[^>]*?\sdata-i="(\w+)"[^>]*>)(</\2>)', lambda m: m.group(1) + need(m.group(3)) + m.group(4), html)
        html = re.sub(r'data-ia="(\w+)"', lambda m: 'alt="' + esc(need(m.group(1))) + '" data-ia="' + m.group(1) + '"', html)
        html = re.sub(r'data-it="(\w+):([\w-]+)"', lambda m: m.group(2) + '="' + esc(need(m.group(1))) + '" data-it="' + m.group(1) + ':' + m.group(2) + '"', html)
        for l in LANGS:
            html = html.replace("{{HREF_" + l + "}}", (P or "./") if l == "pt" else (P + LANGS[l][2] if P else LANGS[l][2]))
        m2 = dict(meta, pkg=P + "firmware/" + name)
        i18n_json = json.dumps(T, ensure_ascii=False).replace("</", "<\\/")
        html = html.replace("/*LIB*/", lib).replace("/*META*/", json.dumps(m2)).replace("/*I18N*/", i18n_json)
        if P:
            html = re.sub(r'(src|href)="(assets|editor|firmware|media)/', lambda mm: f'{mm.group(1)}="../{mm.group(2)}/', html)
            html = html.replace("url(assets/", "url(../assets/").replace('"sim/index.html?embed=1', '"../sim/index.html?embed=1')
        html = html.replace("sim/index.html?embed=1", "sim/index.html?embed=1&v=" + simv)
        html = html.replace("__BUILD__", simv).replace("__BUILDURL__", P + "build.json")
        d = out / path
        d.mkdir(parents=True, exist_ok=True)
        (d / "index.html").write_text(html, encoding="utf-8")
        print("page:", d / "index.html", len(html), "B")

    # the editor, with the bridge the upload panel of the page uses
    ed = (REPO / "web" / "editor.html").read_text(encoding="utf-8")
    end = "\n})();\n</script>\n</body>"
    assert ed.count(end) == 1, "the end of the editor script changed: update page/build_page.py"
    ed = ed.replace(end, "\n" + (HERE / "bridge.js").read_text(encoding="utf-8") + end, 1)
    (out / "editor").mkdir(exist_ok=True)
    (out / "editor" / "index.html").write_text(ed, encoding="utf-8")
    for f in ("fukiai.ttf", "FUKIAI-LICENSE.txt"):
        if (REPO / "web" / f).exists():
            shutil.copy(REPO / "web" / f, out / "editor" / f)
    (out / "build.json").write_text(json.dumps({"build": simv, "version": version}), encoding="utf-8")
    (out / "robots.txt").write_text("User-agent: *\nAllow: /\n", encoding="utf-8")
    (out / ".nojekyll").write_text("", encoding="utf-8")
    print("package:", name, product, meta["sha256"][:12])


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit(__doc__)
    main(*sys.argv[1:])
