# FM1 Quest

The SLOOP firmware of the M-VAVE FM-1 with an RPG sidescroll interface: tracks are heroes, lit steps are attacks, BPM sets the
walk, the key sets the biome, effects are the weather, a recording is a boss fight (design: `design/`). **The interface replaces
SLOOP's screens; there is no switch back.** Every musical function stays where SLOOP has it, with the same controls.

* `firmware/src/ui_quest*.c` — the interface in C (the 240x240 `cv_*` canvas, integer maths): Title (`splash.c`), Party, Boss, Duel,
  Rec ready, Count-in, Dungeon, Kit, Spells, Banish, Biome, Camp, Weather, Equip, Inn. Screens SLOOP has and the design does not
  (the roll / steps / song layers, hold, ABOUT, the other pages) keep their layout in the TAVERN palette (`gfx.c`, `ui_studio.c`).
* `tools/gen_quest_*.py` — fonts (Pixelify Sans 8 px, Jacquarda Bastarda 9 16 px), the baked sprites (`assets/quest/sprites.json`,
  made from the design by `quest/web/tools/bake.html`) and the title art, through the repository's own generators.
* `quest/` — the browser emulator: `web/` is the faceplate, audio worklet and LCD model of
  [sabliran/sloop-web-emu](https://github.com/sabliran/sloop-web-emu) running this firmware as WebAssembly (`./build.sh`).
  `python3 quest/serve.py 5193`, open it, press POWER ON.
* `.github/workflows/build.yml` — builds the installable package with JieLi's toolchain, runs the host tests, publishes a release and
  the Pages site (browser installer + emulator).

## Install on an FM-1
Use the installer of the Pages site (or `web/make_site.py build/felucca.fwsc dev OUT` served from localhost), in Chrome or Edge,
connect the FM-1 by a USB data cable (no hub), press INSTALL. The package is checked (SHA-256 / CRC) before it is written; hold OCT-
at power-on for the USB rescue; SLOOP's own release can be installed over it. **At your own risk; not yet tried on a device.**

## Animation / SPI budget
The scene animates at ~16 fps while the transport plays and at a slow ambient rate when stopped, paced by a token bucket of LCD
SPI bytes (12 MHz bus, `q_scene_budget` in `ui_quest_screens.c`); a frame that does not fit is dropped, never queued.
The values are placeholders until measured on a device.

GPL-3.0-only, as the firmware.
