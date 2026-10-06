#!/usr/bin/env python3
"""Build-time hooks for the FM1 Quest skin.

The firmware sources are NOT edited. This script writes patched COPIES of two files into
<out>/ (put it before firmware/src in the include path); each patch inserts one read-only call
that hands the skin the strings and numbers the firmware has just computed for its own LCD,
so the skin never re-derives them:

  ui_layers.c  layer_screen_draw(): the 16 tiles, the sub title and the 4 dials of a layer
  ui_draw.c    draw_column():       the label / value / unit / gauge of each of the 4 columns
               draw_foot():         engine name, preset name, page title

Fails loudly if an anchor line is missing (the firmware changed under us)."""
import sys, pathlib
src, out = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)

def patch(name, proto, edits):
    s = (src / name).read_text()
    for anchor, add, where in edits:
        assert s.count(anchor) == 1, f"{name}: anchor not unique/missing: {anchor!r}"
        s = s.replace(anchor, (add + "\n" + anchor) if where == "before" else (anchor + "\n" + add))
    s = proto + "\n" + s
    (out / name).write_text(s)

patch("ui_layers.c",
      "static void quest_cap_layer(uint32_t layer, const char *sub, uint16_t col, const void *tl, const char *const *lab, const char *const *val, const int32_t *ratio);",
      [("    layer_title(LAYER_NAME[layer % LY_COUNT], sub, col, &head);",
        "    quest_cap_layer(layer, sub, col, tl, lab, val, ratio);", "before")])
patch("ui_draw.c",
      "static void quest_cap_col(uint32_t c, const char *label, const char *val, const char *unit, int32_t ratio, uint16_t vc);\n"
      "static void quest_cap_foot(const char *ename, const char *pn, const char *ti);",
      [("    char l[8], v[8], u[8], key[32];", "    quest_cap_col(c, label, val, unit, ratio, vc);", "after"),
       ("    str_cpy(s, ename, sizeof s);", "    quest_cap_foot(ename, pn, ti);", "before")])
print("hooks written to", out)
