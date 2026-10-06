# FM1 Quest

An RPG sidescroll skin for the SLOOP firmware of the M-VAVE FM-1, plus a browser emulator to try it.
This folder lives in a fork of [isod89/sloop-fm1](https://github.com/isod89/sloop-fm1); the firmware sources in the
parent folder are used **unmodified**.

* `web/` — the emulator: the faceplate, audio worklet and LCD model are from
  [sabliran/sloop-web-emu](https://github.com/sabliran/sloop-web-emu) (GPL-3.0), running the real firmware compiled to
  WebAssembly. `quest.js` / `quest.css` / `js/scene.js` are the skin: the 15 screens of the TAVERN design, drawn from the
  firmware's own state. Top-left switch: SLOOP (the firmware's LCD) / QUEST (the skin).
* `src/sloop_wasm.c` — the web HAL (unchanged from sloop-web-emu). `src/quest_wasm.c` — includes it and exports a read-only
  snapshot of the firmware state. `tools/patch_hooks.py` — writes patched *copies* of `ui_layers.c` and `ui_draw.c` at build
  time (read-only capture calls); the repository's own files are never edited.
* `design/` — the design handoff (README, HTML reference, assets).

## Run
    python3 quest/serve.py 5193        # then open http://localhost:5193 and press POWER ON

## Rebuild the wasm
    cd quest && CC=zig ./build.sh      # or CC=clang with wasm-ld; needs python3 + numpy + pillow

## Animation / SPI budget
The scene animates at ~16 fps while the transport plays and at a slow ambient rate when stopped, paced by a token
bucket of LCD SPI bytes (12 MHz bus, `Q.cfg` in `web/quest.js`); frames that do not fit are dropped, not queued.
The badge at the bottom-left of the page shows the estimate. The values are placeholders until measured on a device.

## Status
All 15 screens are skinned in the emulator. Screens without a skin (roll / steps / song layers, hold, ABOUT, HOME,
other pages, drum-track pages) show the firmware's own LCD. Nothing here has been run on a real FM-1.

GPL-3.0-only, as the firmware. Installing custom firmware is at your own risk.
