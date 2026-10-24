# High-frame-rate interpolation: problems found, fixes, and how to debug

At Display or Manual output rate RT64 renders frames between the game's own frames
(the game runs at 30 fps or less). It does this by matching every transform
(model matrix) and projection of the current game frame to one of the previous
game frame and blending between them. Everything here is about objects that look
right at Original output rate but wrong in between: when they were matched to
the wrong thing, or blended in a way that does not suit them.

**First check:** set Output rate to Original. If the problem disappears, it is an
interpolation problem and this document applies. If it stays, it is something else.

## How matching works

- **Matrix groups.** Display lists can tag transforms with a group id
  (`gEXMatrixGroup*`, pushed and popped like a stack). RT64 only matches a
  transform to one with the same id in the previous frame.
- **Untagged transforms** (`G_EX_ID_AUTO`) are matched across the whole frame by
  draw-call hash (combiner, other mode, geometry mode, triangle count). Identical or
  similar models anywhere on screen can pair with each other.
- **`G_EX_ID_IGNORE`** (id 0): never interpolated, shown at game-frame poses.
- **Ordering inside a group.** `G_EX_ORDER_LINEAR` pairs the n-th transform of the
  group with the n-th of the previous frame (a changed count leaves the extras
  unmatched for that frame). `G_EX_ORDER_AUTO` uses draw-call hashes inside the
  group, so identical parts of one object can still swap.
- **Blend mode.** Decomposed (`G_EX_INTERPOLATE_DECOMPOSE`, RT64's default):
  translation moves in a straight line, rotation slerps around it. Simple
  (`G_EX_INTERPOLATE_SIMPLE`): the two matrices are blended, so geometry moves
  straight between its real positions.
- **Components** (`G_EX_COMPONENT_*`) per group: position, rotation, scale, skew,
  perspective, vertices, texture coordinates, **tile** (texture offsets), look-at.
  AUTO rotation always interpolates (RT64's own FIXME); AUTO tile interpolation
  slides texture windows when the texture is the same.
- **Projections** have groups too (`viewProjTransformGroups`). The projection
  (field of view, letterbox) is blended whenever the camera's view is.

## Who tags what in the player

| Id range | Owner | Covers |
| --- | --- | --- |
| `0x90000000`-`0xA0000000` | guest render bridge (`tools/baseline/guest`) | honey carrier, palette props, Banjo's body (`player_boundary.c`, 5 body models only), NPC `0x665` |
| `0xA0000000`-`0xB0000000` | guest camera candidate | world cameras; other projections it marks `G_EX_ID_IGNORE` |
| `0xB1000000`, `0xC0000000`-`0xD0000000` | guest cpu625, backpack621 | specific models (Banjo's backpack: one exact model) |
| `0xD1xxxxxx` | `src/menu_interpolation.cpp` | game select actors (written into the display list) |
| `0xE0xxxxxx` | `src/object_interpolation.cpp` | every actor drawn by `func_8010A79C`, slot + spawn serial |
| `0xE1xxxxxx` | `src/object_interpolation.cpp` | the player, by model |

The guest owners only admit exact models and only under the WORLD camera, so in
cutscenes, menus and transformations most things used to fall back to AUTO. The
`0xE0`/`0xE1` groups ("Interpolate all objects", Video menu, on by default) cover
those cases. They are applied without writing to the game's display-list pool
(it has no room: `0x8FC0` = the original `0x76C0` plus the owners' 6400 bytes):
raw hooks record each object's command range in the pool, and an observer that
RT64 calls before every command (`patches/rt64/object_groups.delta.json`) pushes
and pops the group as execution enters and leaves the range. Guest owner groups
nest inside and take precedence.

## Problems found and their fixes

Each was confirmed fine at Original output and diagnosed with the trace below.

1. **Game select picture frames jittered.** Several frames share a model; AUTO
   matching paired them with each other. Fix: per-actor groups on map `0x158`
   (`src/menu_interpolation.cpp`, the first use of the `func_8010A79C` hooks), and
   the map no longer forced to Original rate. The startup intro (`0x18B`) still is.
2. **Objects in general (ids by draw-call hash).** Fix: the `0xE0` per-actor
   groups.
3. **Cheato's pages kinked into zig-zags.** The pages are identical parts of one
   actor; with `G_EX_ORDER_AUTO` inside the group they still paired with each other
   by hash. Fix: `G_EX_ORDER_LINEAR` for actor groups (the guest owners use it too).
   *Tried and wrong:* not interpolating bones that turn more than 90 degrees per
   frame; the pages were mismatched, not fast.
4. **Stony's backpack flap swung too far back.** The player is not drawn through
   the actor dispatcher (it is the one `func_800DE498` model draw outside all actors,
   callback `0x80092234`), and Banjo's body owner refuses Stony's model, so the
   player was AUTO. With a player group the pairing was right, but the flap's
   matrices pivot far from its geometry (87 units behind the body in one frame), and
   decomposed blending swung it along a wide arc. Fix: the `0xE1` player group uses
   the body owner's properties: simple blending, components interpolated.
5. **Warp pad menu icons flickered between images.** Each icon is a flat quad at a
   fixed position whose texture window steps over a sprite sheet; AUTO tile
   interpolation slid the window between images. Fix: actor groups skip tile
   interpolation (scrolling textures on actors now move at the game rate).
6. **Warp pad menu entries shifted.** Entry bars and the title slide by moving their
   own orthographic projection, which the guest camera owner marks
   `G_EX_ID_IGNORE` (steps at the game rate), while each icon moves by its own
   matrix in the menu actor's group (interpolated). Fix: while the current
   projection is an ignored orthographic one, the observer adds an ignored group
   inside the object, so those parts step with their projection. Ignored
   perspective projections (game select's camera) keep their objects interpolated;
   the first version also caught those and made game select worse.
7. **The scene squashed for a moment when the letterbox opened.** The world
   camera keeps its id while its vertical scale jumps (1.374 to 1.821), so RT64
   blended the projection across the change while the bars snapped in. Fix:
   `projection_residual.delta.json` revision 2 does not interpolate a projection
   whose horizontal or vertical scale changes by more than 15% between game frames.
   This applies with the option off as well.

## Debugging a new case

1. **Confirm it is interpolation:** Original output rate looks right.
2. **Record it.** A phone's slow-motion video (240 fps) of the Deck screen shows
   individual displayed frames; extract them with
   `ffmpeg -i clip.mov -vf "select='between(n\,A\,B)',crop=...,tile=6x5" -frames:v 1 sheet.png`.
   Phone exposure can blend two displayed frames, so a double image alone proves
   nothing.
3. **Trace it on the Deck.** Add `TOOIE_INTERPOLATION_TRACE=1 %command%` to the
   game's Steam launch options (Properties > General > Launch options). Map a spare
   button to keyboard F4 (Steam button > Controller settings > Edit layout >
   Buttons, for example L4 > Keyboard > F4). Each F4 press shows "Issue marker #N"
   and writes the next three frames to `~/.local/share/BanjoTooieSteamDeckPort/player.log`
   (copy it before the next launch, which replaces it). Several presses in one
   session are fine; note which marker is which scene. Remove the launch option
   afterwards; each press writes a few hundred kilobytes.
4. **Read the trace.** Lines are `trace <press>.<frame> ...`:
   - `range id=... start=+offset len=` — each object range recorded in the pool
     (`E0` actors, `E1` player; `len=0` drew nothing).
   - `model at=... callback=... model=... object=...` — every `func_800DE498` model
     draw; `object=0` means outside every actor (the player has callback `80092234`).
   - `workload ...`, then per framebuffer pair `proj fb=F #N type=T` (1
     perspective, 2 orthographic, 3 rectangle), its group id and ordering
     (`/o0` linear, `/o1` auto), scissor (letterbox bars show here), `P=` projection
     scales `[0][0],[1][1],[2][2],[3][2]` and `V=` view translation.
   - Under each projection, `call #n world=a-b group=ID/oN tris=T pos=x,y,z`: the
     call's world matrices, their group and the first matrix's translation.
5. **Compare the three frames** for the object that looks wrong:
   - Different groups or counts across frames, or identical parts (same `tris`)
     under AUTO ids or `/o1`: **mismatching** (cases 1-3). Give it its own group
     with linear ordering.
   - Pairing stable but translations far from the body or jumping much more than
     the object moves: **pivots far from geometry** (case 4). Use simple blending.
   - Positions identical every frame but the image still changes: **texture
     animation** (case 5). Skip tile interpolation.
   - Parts of one object under different projections where one projection is
     `group=00000000` (ignored) and its `V=` moves: **mixed stepping** (case 6).
   - A projection with the same group whose `P=` scales jump: **projection jump**
     (case 7).
6. **Reproduce on the dev machine when possible.** Run a copy of the player with a
   scratch `--profile-dir` (marker `.encore-profile`, flat `tooie_imgui.json` with
   `baseline_fullscreen: 0`), `--seconds N --input-script FILE` (format in
   `tools/baseline/input.hpp`; START, START reaches game select), and
   `SDL_VIDEODRIVER=x11` so the window can be captured with
   `xdotool search --name "Banjo-Tooie: Steam Deck Port"` and
   `ffmpeg -f x11grab -window_id ID`. `xdotool key --window ID F4` triggers the
   trace. `linux/frame_motion.py` compares recordings: game select at 60 Hz
   measures max 0.478 / p95 0.392 with the groups (2.59 / 0.92 without); standing
   in Spiral Mountain max 0.222 with no spikes, on or off. Scripted runs drift with
   loading times, so check a screenshot before trusting a comparison.

## Code map

- `src/object_interpolation.cpp`: recording hooks (`tooie_actor_draw_*`,
  `tooie_model_draw_*`), observer and group properties (`push_group`), trace.
- `src/menu_interpolation.cpp`: game select groups written into the display list.
- `tools/baseline/minimum_host_hooks.py` `ACTOR_DRAW_HOOKS`: `func_8010A79C`
  (`0x8010A7E8`/`0x8010A7F0`) and `func_800DE498` (entry and `L_800DF310`). Hook
  changes need a `CLEAN=1` build.
- `tools/baseline/devices.cpp` `send_dl`: `begin_display_list` before and
  `end_display_list` after RT64 interprets the frame.
- `patches/rt64/object_groups.delta.json` (interpreter observer) and
  `projection_residual.delta.json` revision 2 (projection jump), both in the
  player's generic RT64 fixes (`cmake/Baseline.cmake`).
- Not covered by object groups: level geometry, particles and 2D HUD (RT64 does
  not interpolate rectangles, so HUD sprites move at the game rate).
