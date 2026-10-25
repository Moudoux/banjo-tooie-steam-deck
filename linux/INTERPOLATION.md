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

Since Alpha 4.4 the guest's BK pipeline (`tools/baseline/guest/silo-candidate/bk_*`,
`docs/RENDERING.md`) gives actors, the player and their bones their own groups,
and the camera emits explicit projection ids. This fork's `0xE0`/`0xE1` groups
("Interpolate all objects", an RT64 interpreter observer) covered the same
objects before the rebase; they clashed with upstream's identities in the pause
menu and were removed once cases 3-6 below were confirmed fine without them.

## Problems found and their fixes

Each was confirmed fine at Original output and diagnosed with the trace below.

1. **Game select picture frames jittered.** Several frames share a model; AUTO
   matching paired them with each other. Fix: per-actor groups on map `0x158`
   (`src/menu_interpolation.cpp`, the first use of the `func_8010A79C` hooks), and
   the map no longer forced to Original rate. The startup intro (`0x18B`) still is.
2. **Objects in general (ids by draw-call hash).** Fixed before the rebase by
   this fork's `0xE0` per-actor groups; now covered by upstream's BK pipeline.
   Cases 3-6 were found and fixed with those groups and were confirmed fine on
   the Deck with upstream's pipeline alone (2026-10-25).
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
   Still selected.
8. **The camera snapped for one game frame while turning** (Alpha 4.4). The sky
   and world scenes share the view and carry explicit camera ids; RT64 pairs
   scenes by matrix distance before comparing ids, and while the camera moves the
   world scene sometimes paired with the previous sky scene, so neither mapped.
   Seen in F4 reports as `player_projection` rows with `mapped: false` (flags
   `0x10033`) for three presentations, in pairs of workloads three apart. Fix:
   `owner_index_guard_scene_ids.delta.json` prefers pairs with equal explicit ids
   (`linux/PORT_NOTES.md`).
9. **The underwater view snapped wider for one game frame every few seconds.**
   Not interpolation (it shows at Original too): the game's own projection
   (`raw`) was 4:3 for that one workload and doubly widened otherwise, because
   the guest refused to narrow the swaying camera's aspect. Fix: "Underwater
   sway at the right width" in `linux/PORT_NOTES.md`. Check `raw` across
   workloads before blaming RT64 for a projection jump.

## Debugging a new case

1. **Confirm it is interpolation:** Original output rate looks right.
2. **Record it.** A phone's slow-motion video (240 fps) of the Deck screen shows
   individual displayed frames; extract them with
   `ffmpeg -i clip.mov -vf "select='between(n\,A\,B)',crop=...,tile=6x5" -frames:v 1 sheet.png`.
   Phone exposure can blend two displayed frames, so a double image alone proves
   nothing.
3. **Mark it on the Deck.** Map a spare button to keyboard F4 (Steam button >
   Controller settings > Edit layout > Buttons, for example L4 > Keyboard > F4).
   Each press writes upstream's issue-evidence report covering the previous ten
   seconds, `issue-evidence-*-N-0.jsonl` next to `player.log` (marker line
   `baseline issue-marker seq=N` in the log). Note which marker is which scene.
4. **Read the report.** Records are JSON lines by `type`: `frame` (map, game
   period, camera and owner state per game frame), `bk_draw` (BK pipeline roots:
   owner, ids, `skip_all`/`skip_model`), `player_projection` (per presentation:
   RT64's raw, prior and final projection, `mapped`, workload ids) and
   `player_transform` (the same per player/actor transform). `local/deck-perf/f4sum.py`
   summarises rates; `local/deck-perf/camsteps.py` checks the camera's per-
   presentation steps against the game frame's.
5. **Classify it:**
   - An object paired with a different object (identical parts under AUTO ids,
     or a changed group or count): **mismatching** (cases 1-3).
   - Pairing stable but blending swings parts far from the body: **pivots far
     from geometry** (case 4).
   - Positions identical but the image changes: **texture animation** (case 5).
   - Parts of one object stepping with an ignored projection: **mixed stepping**
     (case 6).
   - A projection whose scale jumps under one id: **projection jump** (case 7).
   - The whole view holding for a game frame (`mapped: false` on the camera's
     projection rows): **scene pairing** (case 8).
6. **Reproduce on the dev machine when possible.** Run a copy of the player with a
   scratch `--profile-dir` (marker `.encore-profile`, flat `tooie_imgui.json` with
   `baseline_fullscreen: 0`), `--seconds N --input-script FILE` (format in
   `tools/baseline/input.hpp`; START, START reaches game select), and
   `SDL_VIDEODRIVER=x11` so the window can be captured with
   `xdotool search --name "Banjo-Tooie: Steam Deck Port"` and
   `ffmpeg -f x11grab -window_id ID`. `linux/frame_motion.py` compares recordings: game select at 60 Hz
   measures max 0.478 / p95 0.392 with the groups (2.59 / 0.92 without); standing
   in Spiral Mountain max 0.222 with no spikes, on or off. Scripted runs drift with
   loading times, so check a screenshot before trusting a comparison.

## Code map

- `tools/baseline/guest/silo-candidate/bk_*` and `camera-candidate/camera_emitters.c`:
  upstream's object, bone and camera groups (`docs/RENDERING.md`).
- `src/menu_interpolation.cpp`: game select groups written into the display list,
  from `MENU_GROUP_HOOKS` in `tools/baseline/minimum_host_hooks.py` (`func_8010A79C`
  `0x8010A7E8`/`0x8010A7F0`). Hook changes need a `CLEAN=1` build.
- `patches/rt64/projection_residual.delta.json` revision 2 (projection jump) and,
  on Linux, `owner_index_guard_scene_ids.delta.json` (scene pairing), in the
  player's generic RT64 fixes (`cmake/Baseline.cmake`).
- Not covered by groups: particles and 2D HUD (RT64 does not interpolate
  rectangles, so HUD sprites move at the game rate).
