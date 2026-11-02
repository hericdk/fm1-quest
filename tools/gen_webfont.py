#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The FM-1's own font for the web editor: Terminus 8x16 (assets/fonts/ter-u16n.bdf, SIL OFL 1.1), as a
TrueType font whose outlines are the bitmap's pixels (each row's runs of lit pixels as rectangles), so the
editor shows the same letters as the screen, crisp at 16 px and its multiples. Written into web/editor.html
as a data: URI between the /*SLOOP-FONT*/ markers (no extra file: the editor also runs from file://, where
a font file next to it may not load).
  tools/gen_webfont.py [editor.html]
Needs fontTools (pip install fonttools)."""
import base64
import io
import re
import sys
from pathlib import Path

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen

ROOT = Path(__file__).resolve().parents[1]
BDF = ROOT / "assets" / "fonts" / "ter-u16n.bdf"
PX = 64                                    # font units per pixel: 16 px = 1024 units per em
KEEP = [(0x20, 0x7E), (0xA0, 0xFF), (0x2010, 0x2027), (0x2030, 0x203A), (0x2190, 0x21FF), (0x2200, 0x22FF),
        (0x2500, 0x25FF), (0x2600, 0x266F)]


def read_bdf(path):
    lines = path.read_text(errors="replace").splitlines()
    fbb = next(l for l in lines if l.startswith("FONTBOUNDINGBOX")).split()
    cw, ch, fx, fy = int(fbb[1]), int(fbb[2]), int(fbb[3]), int(fbb[4])
    glyphs, i = {}, 0
    while i < len(lines):
        if lines[i].startswith("STARTCHAR"):
            code, dw, bbx, rows = None, cw, None, []
            j = i + 1
            while not lines[j].startswith("ENDCHAR"):
                l = lines[j]
                if l.startswith("ENCODING"):
                    code = int(l.split()[1])
                elif l.startswith("DWIDTH"):
                    dw = int(l.split()[1])
                elif l.startswith("BBX"):
                    bbx = list(map(int, l.split()[1:5]))
                elif l.startswith("BITMAP"):
                    k = j + 1
                    while not lines[k].startswith("ENDCHAR"):
                        rows.append(int(lines[k], 16))
                        k += 1
                    j = k - 1
                j += 1
            if code is not None and code >= 0 and bbx and any(a <= code <= b for a, b in KEEP):
                glyphs[code] = (dw, bbx, rows)
            i = j
        i += 1
    return (cw, ch, fx, fy), glyphs


def build(path):
    (cw, ch, fx, fy), glyphs = read_bdf(path)
    ascent, descent = (ch + fy) * PX, -fy * PX          # 12 px above the baseline, 4 below
    order = [".notdef"] + [f"u{c:04X}" for c in sorted(glyphs)]
    cmap = {c: f"u{c:04X}" for c in glyphs}
    fb = FontBuilder(16 * PX, isTTF=True)
    fb.setupGlyphOrder(order)
    fb.setupCharacterMap(cmap)
    outlines, metrics = {}, {}
    pen = TTGlyphPen(None)
    pen.moveTo((0, 0)); pen.lineTo((0, 1)); pen.lineTo((1, 1)); pen.closePath()   # (an empty-ish .notdef)
    outlines[".notdef"] = TTGlyphPen(None).glyph()
    metrics[".notdef"] = (cw * PX, 0)
    for c, (dw, (bw, bh, bx, by), rows) in sorted(glyphs.items()):
        pen = TTGlyphPen(None)
        nbits = ((bw + 7) // 8) * 8
        for r, bits in enumerate(rows):
            y1 = (by + bh - r) * PX                      # the row's top, from the baseline
            x = 0
            while x < bw:
                if bits >> (nbits - 1 - x) & 1:
                    x0 = x
                    while x < bw and bits >> (nbits - 1 - x) & 1:
                        x += 1
                    a, b = (bx + x0) * PX, (bx + x) * PX
                    pen.moveTo((a, y1 - PX)); pen.lineTo((a, y1)); pen.lineTo((b, y1)); pen.lineTo((b, y1 - PX))
                    pen.closePath()
                else:
                    x += 1
        name = f"u{c:04X}"
        outlines[name] = pen.glyph()
        metrics[name] = (dw * PX, 0)
    fb.setupGlyf(outlines)
    fb.setupHorizontalMetrics({n: (metrics[n][0], outlines[n].xMin if hasattr(outlines[n], "xMin") and outlines[n].numberOfContours else 0) for n in order})
    fb.setupHorizontalHeader(ascent=ascent, descent=-descent)
    fb.setupNameTable({"familyName": "SLOOP Terminus", "styleName": "Regular",
                       "copyright": "Terminus Font (C) 2020 Dimitar Toshkov Zhekov, SIL Open Font License 1.1",
                       "licenseDescription": "SIL Open Font License, Version 1.1"})
    fb.setupOS2(sTypoAscender=ascent, sTypoDescender=-descent, usWinAscent=ascent, usWinDescent=descent,
                sTypoLineGap=0, fsType=0)
    fb.setupPost(isFixedPitch=1)
    buf = io.BytesIO()
    fb.save(buf)
    return buf.getvalue(), len(glyphs)


def main():
    editor = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "web" / "editor.html"
    ttf, n = build(BDF)
    uri = "data:font/ttf;base64," + base64.b64encode(ttf).decode()
    s = editor.read_text(encoding="utf-8")
    new, k = re.subn(r"/\*SLOOP-FONT\*/url\([^)]*\)/\*SLOOP-FONT\*/", f"/*SLOOP-FONT*/url({uri})/*SLOOP-FONT*/", s)
    if k != 1:
        sys.exit(f"{editor}: no /*SLOOP-FONT*/url(...)/*SLOOP-FONT*/ marker")
    editor.write_text(new, encoding="utf-8")
    print(f"web font: {n} glyphs, {len(ttf)} B TTF -> {editor.name}")


if __name__ == "__main__":
    main()
