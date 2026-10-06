# Handoff: FM01 Quest — sidescroll RPG UI for the Sloop firmware (FM-1)

## Overview
FM01 Quest reskins the Sloop looper/sequencer UI (repo: `isod89/sloop-fm1`) as a 2D pixel-art sidescroll RPG. The **top part of every screen is a live game scene driven by the music**. The **bottom part is the Sloop HUD**: track rows, step grids, pads, knobs and menus. Every musical function stays where Sloop has it; the game layer only visualizes it.

Music → game mapping:
- **Tracks are heroes.** 1 ACID 303 = mage, 2 DUB CHORD = archer, 3 ATMOS PAD = cleric, 4 TECHNO (drums) = warrior.
- **Lit steps are attacks.** Each hero attacks when its track's step fires.
- **BPM sets walking speed.** The scroll moves at `bpm * 0.22` px/s (logical px).
- **Key / scale sets the biome.** Major = forest, minor = cave, dorian = coast, phrygian = ruins.
- **FX sets the weather and cleric buffs.** DST = lightning / +atk, CHO = mist / +def, DLY = echo ghosts / +spd, REV = rain / regen.
- **Recording a loop is a boss fight.** The boss loses HP each recorded bar.
- **Mute = the hero rests at the campfire.** Solo = a 1-on-1 duel screen.
- **Track edit = equip screen.** Presets are shown as items.

## About the design files
`FM01 Quest v3.dc.html` is a **design reference built in HTML**. It is a prototype of the intended look and behavior, not production code. Open it in a browser with `support.js` next to it. The task is to **recreate it in the Sloop firmware's own environment**: C, rendering to the FM-1's 240×240 display through the existing `firmware/src` UI pages and font/icon pipeline (`tools/gen_font.py`, `tools/gen_icons.py`).

In the reference each screen is shown at 2× (480×480 CSS px = 240×240 device px). **All sizes below are device pixels (1×)** unless marked CSS.

## Fidelity
**High-fidelity** for layout, palette, typography sizing, scene composition and animation timing. The pixel sprites are drawn procedurally in the reference (`figure()`, `slimeD()`, `spr()` in the logic class), so they are exact and can be ported 1:1 or baked to sprite sheets. Exception: the title background (`assets/title-bg.jpg`) is an illustration and must be downscaled/quantized to 240×240 for the device.

## Chosen direction: 1c TAVERN
- Dark warm UI with wood panels and gold borders, plus blue inventory-slot borders.
- Gothic display font for titles; a pixel sans for body text.
- A second direction, 1a GREENWOOD (4-tone green), still exists in code but is hidden. It is enabled through `SHOW=['1c']` in the logic.

## Screen layout (every screen, 240×240)
- **Status/title bar:** 19 px tall, 1 px bottom border `#5a3a28`, 6 px side padding.
- **Scene canvas:** 240×84 (title: full screen, duel: 240×110, menu: 240×60), 1 px bottom border.
- **HUD:** the remaining height, padding 4–5 px × 6 px.
- **Knob row:** pinned to the bottom of the HUD. 4 equal columns. Each column is a 20×20 knob, then a label (dim colour), then a value.

### Knob (circular range, like Sloop)
- 20×20 px.
- Track ring: radius 6.2–8.6, sweeping −135°…+135°. The filled part uses the knob's colour (knob 1 `#5a8ad0`, knob 2 `#7ab04e`, knob 3 `#f0c860`, knob 4 `#e08a3a`). The rest is `#2e2226`.
- Cap: radius < 5.2, fill `#6a4030`, rim `#8a5a38` at r 4.2–5.2.
- Pointer: 5 px line from the centre in `#f2e2c4`.
- No outer rim (removed by request).

## Screens (in order)
1. **TITLE.** Full-bleed `assets/title-bg.jpg` with a dark gradient at top and bottom. `assets/logo.png` (white "FM1 QUEST", transparent background) sits centred at the top, about 170 px wide, with a 1–2 px dark drop shadow. At the bottom is a "press play" panel (wood panel with a play triangle) and the text "based on Sloop" in cream with a dark outline.
2. **PARTY (tracks).**
   - Status bar: `130 bpm`, play triangle, `1.3`, 4 bar pips, title "Party".
   - Scene: the 4 heroes walk right while a stream of monsters approaches.
   - HUD: 4 track rows, each 17 px tall:
     - a 15×15 hero portrait with a 1 px border in the track colour;
     - track number (dim), name and class (track colour);
     - 16 step pips, 3.5×3.5 px;
     - "hp NN" with a 36×3 bar.
   - Knobs: swing 50%, level 81, steps 32, pan 0.
3. **EQUIP (track edit).** Title "Equip", "1 ACID 303", "No. 22/68".
   - Left panel (86 px wide, wood): 4 small portraits with the selected one gold-bordered, then the hero at 3× scale on a pedestal with glow (animated), then "MAGE lv 81".
   - Right: 4×3 grid of preset items. Each slot is 26 px tall with a 6×6 waveform icon (saw/sine/square/tri/noise) and a short name. The selected slot is gold with dark text.
   - Below the grid: info panel ("Acid 303", "synth / analog", "oct+: equip / oct-: back").
   - Knobs: cutoff 62, reso 40, env 55, decay 30.
4. **BOSS (recording).** Status bar shows "rec" in red. The scene overlays the boss name "King Slime" plus an HP bar. The HUD has a command list (FIGHT/play, SPELL/punch, ITEM/kit, RUN/stop) with FIGHT highlighted, the party HP bars, and the party knobs. Scene choreography is under Interactions.
5. **DUEL (solo).** Diagonal camera: the ground line is `y = 102 − x·0.2` on a 240×110 canvas. Hero vs. a shadow rival, both at 2× scale. "combo x12" overlay. HUD: 16 large steps and knobs (sound kick, div 1/16, swing 50%, steps 16).
6. **REC READY.** A king slime with a "!" bubble. Dialog panel: "A wild LOOP appears!", "play a note: fight", "rec: flee". Compact track list with track 2 outlined in red. Knobs: mode tempo, length 2 bars, start note.
7. **COUNT-IN.** Same layout. A big "3" over the scene and a white frame flash on every beat. Dialog: "Battle in one bar!", "4 clicks", "rec: cancel".
8. **DUNGEON (drum grid).** Cave scene. 16×8 grid with orange cells on, an alternate-coloured playhead column and the red cursor. Knobs: sound hat, step 1, hit --, level --.
9. **KIT.** A chest with floating items. 4×4 pads (kick … bell).
10. **SPELLS (punch FX).** 4×4 pads (loop 4 … wobble), each row topped with a track colour. "loop16" is active. With loop16 on, the scene rewinds every bar.
11. **BIOME (key).** 4×4 note pads with D highlighted. Below them a biome legend (maj forest / min cave / dor coast / phr ruins).
12. **CAMP (mix).** Night campfire. The muted archer sits by the fire with "z"s while the others fight a bat. Grid: rest 1–4 (rest 2 active), duel 1–4, tap 113.
13. **BANISH (erase).** Pads with red top borders where notes exist; "hat" is selected in red. The warrior throws a bomb at two slimes, which explode and dissolve.
14. **WEATHER (FX).** Storm scene. 4 columns (DST/CHO/DLY/REV), each with weather name, knob, value, buff and a 10-segment meter. The cleric casts buff auras on the party.
15. **INN (menu).** The party sleeps around the fire. Sloop menu list: COLOR, LOWCUT, ZOOM, LIGHTS, KEYS, NOTES, USB AUDIO (cursor), HARDWARE CALIBRATION, ABOUT, BACK. Hints: presets move / knob 1 set / oct+ ok / oct- back.

## Interactions & behavior (scene logic)
Timing: `beat = t·bpm/60` and `step = beat·4`. Attack phase `p = (step − hitStep)/4`, so it is measured in beats.
- **Anticipation:** `p ∈ [−0.5, 0)`, starting 2 steps before a hit.
- **Strike:** `p ∈ [0, 0.25)`, a crescent smear arc.
- **Follow-through and fade:** `p ∈ [0.25, 0.6)`.
- **Recover:** `p ≥ 0.6`.

Hero behaviour:
- **Warrior:** attacks only when the nearest monster is within 44 px. He runs to it (fast legs, speed lines), strikes up close (star spark on contact), then jumps back to his slot (a 9 px arc). He backs off if a monster gets closer than 6 px.
- **Mage:** floats with a book. For `p < 0.35` a fireball is summoned above her head (growing, with orbiting sparks). Then it falls diagonally with ease-in onto the target's current position, and bursts.
- **Archer:** draws the string on anticipation. The arrow flies on a parabolic arc (peak 22 px) and stays aligned with its direction of flight.
- **Cleric:** raises her staff and casts aura rings plus up-arrows on every hero, coloured by the active buff.

Attack rules:
- Nobody attacks unless a living monster is visible.
- All attacks target the nearest living monster.

Monster stream:
- Monsters are spaced 62 px apart in world space in the order slime, slime, bat.
- Each dies (poof) when it reaches x = 124.
- **The HP bar shrinks with proximity** (`(x − 124)/(W − 30 − 124)`). It does not track damage.

Slimes:
- Translucent domes: an outline, a darker lower band, a white highlight at top-right, and two oval eyes.
- They hop each beat: stretch in the air (`sq −0.12`), squash on landing (`+0.22`), and turn red when hit.
- The king slime is blue (`#4a78b0` / `#2b2338`) with a gold crown.

Boss choreography (beat in bar):
- **Beat 0:** the warrior dashes in with afterimages and slashes. An impact frame of 1–2 frames fills the screen cream, with the boss and warrior shown as dark silhouettes. The screen shakes, and he jumps back.
- **Beat 1:** the mage summons 3 meteors above her and drops them in sequence.
- **Beat 2:** the archer fires a 6-arrow arcing volley.
- **Beat 3:** the boss jumps 16 px and slams. A shockwave runs left along the ground, and the cleric raises a dome barrier that flashes on contact.

Other scene effects:
- **Duel:** the hero dashes along the diagonal on steps `1000100010000000` and the rival counters on step 12 with a pink smear. Hit knockback is 10 px, with sparks and screen shake.
- **Banish (8-beat cycle):**
  - windup, then the bomb arcs (beats 0.6–1.5);
  - explosion, a dark disc of radius 24 with smoke and a cross flash, plus shake (1.5–2.8);
  - the slimes flash red, then dissolve pixel by pixel (1.8–3.2);
  - the slimes re-form (beats 6–7.6).

Frame rate: the reference ticks at about 16 fps (60 ms), which suits the retro look.

## State
The firmware already owns all of these: bpm, transport position, the per-track step patterns, track levels and mute/solo, punch FX, key/scale, the FX values and the recording state. The game layer reads them and keeps only cosmetic state: the walk scroll offset, the monster stream offset, and per-hero attack phases derived from the step clock.

## Design tokens (1c TAVERN)
- **Screen background:** `#1e1418`
- **Text:** `#f2e2c4`
- **Dim text:** `#a89080`
- **Panel dim text:** `#c8b090`
- **Lines:** `#5a3a28`
- **Panel:** `#3a2620`, border 1 px `#c8963c`, inner shadows `#1e1418` and `#6a4030`
- **Pad:** `#24181c`, border `#3a4a78`
- **Off cell:** `#2e2226`
- **Off-cell highlight:** `#45343a`
- **Selected:** fill `#f0c860`, text `#1e1418`
- **Rec / danger:** `#e0503c`
- **Track colours:** 1 `#5a8ad0`, 2 `#7ab04e`, 3 `#f0c860`, 4 `#e08a3a`
- **Scene palette (16):** `#1a1016 #2b2338 #5a3050 #2f4a3a #6a4030 #4a3c48 #a89aa0 #f2e2c4 #c8443c #e08a3a #f0c860 #6a9a4a #4a78b0 #8a6a9a #e09090 #f0b890`
- **Fonts:** body Pixelify Sans, 8 px at 1×. Titles use Jacquarda Bastarda 9 in title case at 16 px; all-caps blackletter is unreadable. For the device, bake both into the firmware's bitmap font pipeline.

## Assets
- `assets/title-bg.jpg`: title illustration, provided by the user.
- `assets/logo.png`: the "FM1 QUEST" logo (white, transparent background, 1024×765), provided by the user. Downscale to about 170 px wide for the device.
- All sprites, knobs, item icons and portraits are procedural; see the logic class in the HTML: `figure`, `slimeD`, `swoosh`, `meteor`, `arcArrow`, `drawKnob`, `drawIcon`.

## Files
- `FM01 Quest v3.dc.html`: the full design board. The template holds the HUD markup. The logic class holds every scene renderer, the sprite builders and the data (patterns, pads, menu).
- `support.js`: runtime needed to open the HTML in a browser.
- `assets/`: title background and logo.
