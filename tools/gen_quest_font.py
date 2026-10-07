#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""FM1 Quest fonts, through the firmware's font pipeline (tools/gen_font.py: same header format, same glyph encoding):
  Q   Pixelify Sans, body text, 8 px (README: 8 px at 1x), anti-aliased (16 alpha levels), Latin-1 32..255
  QT  Jacquarda Bastarda 9, titles, 16 px, 32..126 (Title Case: README, all-caps blackletter is unreadable)
tools/gen_quest_font.py build/gen/felucca_qfont.h   (TTFs in assets/fonts/, SIL OFL 1.1)
At 8 px a hard threshold breaks Pixelify's letters, so Q is rendered anti-aliased; QT keeps hard pixels."""
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_font as G

def render_aa(ttf, px, scale, pixel, first, last):
    """as gen_font.render for a TTF, minus the OpenType `tnum` feature (needs libraqm; Pixelify's digits are already even)"""
    font = ImageFont.truetype(str(G.FONTS / ttf), px)
    asc, desc = font.getmetrics()
    top = max(0, font.getbbox("A8|(")[1] - 1)
    h = asc + desc - top
    glyphs = []
    for c in range(first, last + 1):
        ch = chr(c)
        adv = int(round(font.getlength(ch)))
        bw = adv + 2 * G.PAD
        img = Image.new("L", (bw, asc + desc), 0)
        ImageDraw.Draw(img).text((G.PAD, 0), ch, font=font, fill=255)
        img = img.crop((0, top, bw, top + h))
        px_ = [(15 if v >= 128 else 0) if pixel else min(15, (v + 8) // 17) for v in img.tobytes()]
        glyphs.append((adv * scale, bw * scale, G.upscale(px_, bw, h, scale)))
    return h * scale, glyphs

G.render = render_aa
G.SIZES[:] = [("Q", "PixelifySans-400.ttf", 8, 1, False, 32, 255), ("QT", "JacquardaBastarda9.ttf", 16, 1, True, 32, 126)]
G.main(sys.argv[1])
