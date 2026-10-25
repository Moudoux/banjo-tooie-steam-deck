# To do

Possible enhancements for the Linux / Steam Deck player, compared with the
player menu (`tools/baseline/minimum_menu.cpp`) and Lighthouse's enhancements
(`~/Downloads/Lighthouse/src/port/UI/LighthouseMenuEnhancements.cpp`).
Effort estimates are rough.

Suggested first picks for the Deck: gyro aiming, texture pack menu, Talon
Trot toggle, MSAA as an option.

## Graphics

- [x] **Anti-aliasing (MSAA / supersampling).** RT64 supports both through the
  user configuration the player already sets for resolution scale and output
  rate (`tools/baseline/minimum_presentation.cpp`). Mostly a menu option. Off
  by default: costs battery and frame rate on the Deck.
- [ ] **Texture pack menu.** Enable, disable and order packs in game instead of
  moving `.rtz` files. Loading already exists (`register_texture_packs`,
  `apply_texture_packs` in `tools/baseline/devices.cpp`); only the UI is
  missing.
- [x] **Extended draw distance and a "disable LOD" option.** Both are in
  Lighthouse. `src/draw_distance.cpp` is left over from the old frontend and
  isn't linked into the player. Needs game hooks; medium effort.
- [ ] **Known graphics issues** (`docs/PLAYER_GUIDE.md`):
  - [ ] Mayahem Temple renders black in the distance. Possibly draw distance or
    fog, so it might share a fix with the item above.
  - [ ] The save-select arm animation runs at a low frame rate.
  - [ ] Startup intro (`0x18B`) at the display rate. It is still presented at
    the original rate (`devices.cpp` `update_screen`); game select (`0x158`)
    already follows the chosen rate. Tried on 2026-10-24 and reverted:
    - Lifting the force with Interpolate all objects on (since removed): the camera and the
      solid N64 logo (actor model `80206340`, its own `0xE0` group, 32 calls
      under the `A0000003` projection) interpolate, but the Rareware card
      turned at 20 fps: it turns by its vertices under one fixed matrix, and
      actor groups skip vertex positions. Interpolating vertex positions for
      actor groups on `0x18B` only fixed that at 60 Hz on the dev machine.
    - On the Deck (90 Hz, Cutscene Aspect "Follow game") the owner still saw
      the N64 and Rareware logos jitter vertically. Not reproduced at 60 Hz.
      The glow and the fading logos step at the game rate because colour
      and alpha are not interpolated (expected). Registration measurements
      on the spinning logo were unreliable, locally and on the slow-motion
      video.
    - Next: retry on Alpha 4.4 (upstream's camera ids and object groups may
      change the picture), with an F4 marker during the solid N64 logo to see
      whether its group or the camera's projection changes between frames.

## Deck and controls

- [x] **Gyro aiming in first person (egg aiming).** Done as first-person mouse
  aiming, used with Steam Input's gyro-as-mouse; native SDL gyro did not work.
- [x] **Gyro in first-person flight, underwater, the submarine and the Saucer
  of Peril.** A setting each. Underwater, submarine and on-foot egg aim
  confirmed on the Deck; flight and the saucer crosshair not yet checked.
- [ ] **Talon Trot toggle.** Press once instead of holding. Lighthouse:
  `Controls.TalonTrotToggle` in `src/port/Controller/ControlSchemes.cpp`.
  Small.
- [ ] **Separate first-person invert X / Y.** Small.
- [ ] **Wonderwing on the right stick while crouched.** Lighthouse's modern
  scheme: one activation per push (`port_shapeControllerInput`). Small. The
  D-pad already covers it (D-pad = C-buttons).

## Quality of life

- [x] **Cutscene skipping.** Start now skips the story cutscene chains (the
  game's own Z skip). In-level cutscenes have no skip in the game; skipping
  those (and the Jiggy dance, as Lighthouse does) would need per-scene work.
- [x] **Rumble.** Rumble Pak reported on controller 1; strength in Controls.
- [ ] **Fast-forward.** F6 is listed as unavailable in this player
  (`docs/PLAYER_GUIDE.md`); the input side already exists
  (`set_fast_forward_held` in `src/frontend_input_preference.cpp`).
- [ ] **Bug fixes from Lighthouse's list** (softlocks, missable Mumbo tokens,
  Game Over dialogs and so on). Lighthouse's fixes are for Banjo-Kazooie, so
  each would have to be found and fixed in Tooie separately. Larger, item by
  item.
