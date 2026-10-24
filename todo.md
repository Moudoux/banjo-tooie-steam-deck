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
  - [ ] Menu logo jitter: currently worked around by presenting maps `0x18B`
    and `0x158` at the original rate. The real fix is tagging those draws
    with matrix groups in the guest, so menus can run at the display rate.
    More involved.

## Deck and controls

- [x] **Gyro aiming in first person (egg aiming).** Done as first-person mouse
  aiming, used with Steam Input's gyro-as-mouse; native SDL gyro did not work.
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
