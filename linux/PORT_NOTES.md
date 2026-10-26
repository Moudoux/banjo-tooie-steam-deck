# Linux / Steam Deck port: handoff notes

State of Banjo-Tooie: Steam Deck Port, the Linux fork of Banjo-Tooie: Scurvy
Dog Port, for whoever picks it up next. Upstream ships a Windows player and, since
Alpha 4.4, has its own Linux code paths (POSIX profile startup, a `linux-player`
preset); this fork builds and ships a Linux player tuned for the Steam Deck OLED.
Rebased onto upstream Alpha 4.4 (`05359b6`) on 2026-10-25.

## Repository layout

- Upstream: `https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port`
  (remote `upstream`). Fork: `git@github.com:Moudoux/banjo-tooie.git`
  (remote `origin`). Work lives on branch `linux`; update with
  `git fetch upstream && git rebase upstream/main`.
- Commits use the GitHub noreply address set in this repo's git config; the
  owner's real email is private and GitHub rejects pushes that expose it.
- `tools/check_repository.py` must keep passing. It rejects undeclared `docs/`
  files, so these notes live here. Upstream's version also rejected tracked
  `AGENTS.md`/`CLAUDE.md`; this fork allows them and keeps agent instructions in
  `AGENTS.md` (`CLAUDE.md` imports it). It also lists upstream's
  `docs/RENDERING.md`, which upstream's own list omits.

## Build

```
docker build -t tooie-toolchain:latest .
docker run --rm -it -v "$(pwd)":/workspace tooie-toolchain:latest linux/build.sh
```

- `Dockerfile` (Ubuntu 24.04): MIPS toolchains, clang 18 + libc++, SDL2,
  X11/Vulkan, GTK3, OpenSSL, Wine.
- `linux/build.sh` runs upstream's pipeline (`docs/WINDOWS.md` steps 1-4, 6):
  bootstrap deps, install the Windows-only `llvm_windows` archive through the
  bootstrap's own verifier, `generate_local.py`, `prepare_player_guest.py`, then
  CMake + Ninja into `build/linux-player`. CMake cache variables are read from
  the `windows-player` preset at build time, so preset changes carry over.
  Upstream's own `linux-player` preset (gcc, no matrix/present trace or
  acquire pacing) is not used: the player keeps the Windows feature set.
- Env: `TARGET=deck` (default: `-march=znver2`, `-DTOOIE_DECK_DEFAULTS=1`) or
  `generic`; `SYMBOLS=1` keeps debug info for crash backtraces (use with
  `BUILD=build/linux-player-debug` to keep the normal build); `CLEAN=1` moves old build dirs aside (needed after guest patch
  edits; guest prep refuses existing outputs); `JOBS`, `ROM`.
- Output: `build/linux-player/baseline/player-bin/BanjoTooieSteamDeckPort`
  plus `recompcontrollerdb.txt`. Docker writes as root; `chown` afterwards.
- A clean build takes about 15 minutes on 24 cores and ~10 GB of disk (the
  Windows LLVM alone is 7 GB). No CI: the build needs the user's ROM and its
  output contains game code, so it must not be built on public runners.

### Why it is built this way

- **Wine for guest code.** Guest prep pins and runs
  `deps/LLVM-19.1.3-Windows-X64/bin/clang.exe`/`ld.lld.exe` through `wslpath`.
  `linux/shims/bin/wslpath` maps paths to Wine's `Z:` drive and
  `linux/shims/python/sitecustomize.py` (via `PYTHONPATH`) prefixes `.exe`
  subprocesses with `wine`. No repo script is modified for this, and output
  matches the Windows guest image byte for byte.
- **libc++, linked statically.** A hash-pinned runtime timer patch
  static_asserts `high_resolution_clock == steady_clock` (true for MSVC and
  libc++, false for libstdc++). Static linking means the host needs no libc++.
- **`-ffp-contract=off`.** Zen 2 has FMA; contraction would change guest float
  rounding versus the N64/Windows build.
- **Release (-O3)** replaces the preset's RelWithDebInfo; the binary is
  stripped after the build, because upstream's post-link gate reads its
  symbol table.
- **Upstream's Linux gates run.** Since Alpha 4.4 `cmake/Baseline.cmake` gates
  the Linux link like the Windows one: raw archive qualification (step 5),
  the archive content gate and a post-link check that the native hooks and
  embedded guest image are the qualified ones. `build.sh` runs step 5 once per
  build tree with GNU tools (`linux/qualify_raw.py`, which follows
  `docs/WINDOWS.md`) and passes `TOOIE_BASELINE_BINUTILS_DIR=/usr/bin`, as
  upstream's `linux-player` preset does. Upstream's gate assumes gcc and
  libstdc++; for clang and static libc++ `tools/baseline/elf_binding.py`
  accepts the weak recompiled functions (clang's `RECOMP_FUNC` is weak by
  design; uniqueness and the single map definition are still checked) and
  libc++'s dotless `__lcxx_override` section. Packaging is skipped.

## RT64 pin

RT64 is pinned at `d9817c68` (upstream `main`, 2026-10-23), up from
upstream's `6f1c2d99`. It brings configurable present wait (Automatic turns it
off for NVIDIA on Linux, where it capped output at about 2/3 of the target;
on for AMD, so the Deck is unchanged), `gEXSetScissorAspect`, fill-rect
coverage and draw-area fixes, and ImGui 1.93 WIP.
- Every `patches/rt64/*.delta.json` recipe was moved to the new pin by
  relocating its exact original spans (Alpha 4.4's new `present_trace` and
  `vulkan_present_id` too: insertions only; one `present_trace` anchor sits
  next to RT64's renamed `ext.swapChainPresentWaitEnabled` check, and plume is
  the same submodule commit at both pins); none of their replacement text
  changed except the minimum host's SDL backend
  (`minimum_host_imgui_impl_sdl2_custom_cpp`, revision 2), rewritten for the
  new backend with the same external-mouse/display behaviour.
- Pins that follow RT64 files: `tools/prepare_rt64_matching.py` `PIN`,
  `dependencies.lock.json` (commit, imgui and implot submodules),
  `cmake/Rt64DxcOverlay.cmake` (CMakeLists hash; the upstream change is
  Apple-only), `tools/baseline/prepare_camera_runtime_observer.py`
  (`rt64_rsp.cpp`; its hunks are unchanged), `docs/ARCHITECTURE.md` and
  `patches/rt64/PROVENANCE.md`.
- ImGui now creates and updates the font atlas inside `RenderDrawData`, and
  its Vulkan backend submits to and waits on the shared `VkQueue` there, so
  `minimum_render.cpp` draws the menu under that queue's mutex (RT64 calls
  the draw hook while recording, before it takes the mutex to submit).
  `ImGui_ImplVulkan_CreateFontsTexture` no longer exists.

## Source changes (all keep the Windows build unchanged)

Linux enablement is upstream's since Alpha 4.4 (this fork's own versions were
dropped in the rebase): Linux branches in `cmake/Baseline.cmake`,
`tools/baseline/player_startup_posix.cpp` (profile folder held open and
`flock`ed for the process lifetime, reached through `/proc/self/fd`),
`SDL_WINDOW_VULKAN` in `devices.cpp`, `localtime_r` in `minimum_host.cpp`.
What this fork adds on top:
- `player_startup_posix.cpp`: first launch uses `baserom.us.z64` (or the only
  N64 ROM) beside the executable before opening the picker.

Steam Deck features:
- **Name: Banjo-Tooie: Steam Deck Port.** The Linux player's window title,
  dialogs, product string and executable (`BanjoTooieSteamDeckPort`) use it;
  the profile is `$XDG_DATA_HOME/BanjoTooieSteamDeckPort`. On normal play
  `rename_profile` (`player_startup_posix.cpp`, called from `main.cpp`
  before the Alpha 4 `migrate_profile`) moves a marked, unlocked
  `BanjoTooieScurvyDogPort` folder whole with `renameat2(RENAME_NOREPLACE)`,
  holding an exclusive `flock` on the old folder as upstream's profile lock
  does; an existing new folder wins and the old one is left alone. (Builds
  before the Alpha 4.4 rebase locked the marker file instead, so an old and a
  new build running at once do not exclude each other.) Upstream's Windows side (`player_startup.cpp`, the `.exe` name
  in `release/player-windows-files.json`, `docs/`, `release/`) keeps the
  Scurvy Dog name, to keep upstream merges simple.

- **Deck defaults** (`TOOIE_DECK_DEFAULTS`, `minimum_host.cpp`): only for
  settings a profile has not saved: 3x resolution, output rate Display,
  widescreen On with gameplay aspect Fit window, fullscreen.
- **Auto resolution:** resolution value 0 uses RT64 `WindowIntegerScale`
  (`minimum_presentation.cpp`, `minimum_menu.cpp`, `minimum_host.cpp`).
- **16:10 via Fit window.** Alpha 4.4 added gameplay aspect 4, "Fit window",
  which follows the drawable's aspect live as an f32 scale relative to 16:9
  (`fit_aspect_bits`, `src/widescreen.cpp`). Upstream clamps it to 16:9..32:9;
  outside Windows the lower clamp is 16:10 (scale 0.9), so the Deck panel
  fills. That replaced this fork's own 16:10 entry, which also used index 4
  (so saved profiles carry over) but needed a wider guest settings field and a
  re-pinned guest image. The guest image is upstream's pinned image again.
- **Controller DB:** `minimum_host.cpp` loads `recompcontrollerdb.txt` from
  beside the executable but only adds mappings for GUIDs SDL/Steam have not
  already mapped (Steam's `SDL_GAMECONTROLLERCONFIG` wins). Upstream's player
  never loaded this file.
- **XBLA button layout** as the Deck default (`src/frontend_input_preference.cpp`
  `defaults()`, `TOOIE_DECK_DEFAULTS`), checked against the XBLA manual
  table: A=A, X and B=N64 B, LT/RT=Z, RB=R, Y=C-up, right stick X=C-left/
  right (inverted at the owner's request), right stick up or down and RS click=C-down, D-pad=C-buttons (no N64 D-pad: Tooie reads none),
  LB and LS click=L (XBLA had LB=R too; LB is L so Hold L to levitate is on
  a bumper; profiles saved before keep their bindings), View=menu. The camera is stepped C-buttons, not XBLA's analog
  camera. XBLA's Talon Trot chord: while two controller inputs bound to Z
  are held (LT + RT), `z_chord_down` adds C-left, but only once the previous
  poll already sent Z, so pressing both in the same frame crouches first.

- **Instant intro skip** (`tools/baseline/minimum_host_hooks.py`,
  `INTRO_SKIP_TEXT`): the startup intro ticker (`func_808005AC_chintroticker`)
  reads Start (`func_80015FA0(0)`) only when global flag `0xD56` is set
  (`func_800DA298`, block at `0x8012C78C`), so Start did nothing for the
  first part of the intro. A raw hook before `0x80800644` treats the flag as
  set; the transition-busy check (`func_800A8264`) and the original exit to
  the frontend (`gcfrontend` entrypoint 10) still run.

- **Start skips story cutscenes** (Controls menu,
  `tooie_start_skips_cutscenes`, default on; `START_SKIP_TEXT`). The same
  ticker runs the story cutscene chains in its map table at `D_808008C4`
  (intro `0xA1`-`0xAC`, the Jingaling chain from `0x18E`, the Bottles chain
  from `0x194`, the ending `0x191`/`0x196`, startup `0x18B`). The game
  already skips to a chain's end on a fresh Z (`func_80016B30(0, 1)`, button
  index 1 = `BUTTON_Z` in `include/buttons.h`). A raw hook before
  `0x80800684` also accepts a fresh Start (index 0 of controller 0's table at
  `0x800799E0`); `func_80800530_chintroticker` still looks the map up and
  exits. Cutscenes played inside a level (glcut camera scenes started by
  actors such as Jamjars or world doors) have no skip in the game and are
  unchanged.

- **Rumble** is upstream's since Alpha 4.4, the same design this fork had
  (virtual Rumble Pak on port 0 from the `osPfsInit` adapter, `rumble_strength`
  slider); the fork's copy was dropped in the rebase.

- **Anti-aliasing** (Video menu, `baseline_msaa` / `baseline_supersampling`):
  MSAA 2x/4x/8x and supersampling 2x/4x, applied live by the graphics owner
  in `update_screen` (`tools/baseline/devices.cpp`). MSAA follows the retired
  frontend: set the config, then `RT64::Application::updateMultisampling()`
  outside the configuration lock (it waits for the queues). Only the
  antialiasing fields are written into the shared config, because
  `updateUserConfig` would copy the whole app config over the aspect fields
  `reconcile_aspect` writes directly. Levels offered stop at the device's
  common color/depth sample count and need programmable sample locations
  (`render::max_msaa`, detected in `publish_renderer`). Supersampling renders
  at scale x factor with RT64's `downsampleMultiplier` resolving to the
  chosen scale (the retired frontend's mapping); it needs a fixed scale and
  is capped so the internal scale stays at or below 8x.

- **Picture options** (Video menu): RT64 settings the player used to leave
  at fixed values. Defaults keep the earlier output.
  - Color precision (`baseline_color_format`: 0 Standard, 1 High, 2
    Automatic, default Standard) sets `internalColorFormat` in
    `configure_startup`, overriding `devices.cpp`'s Standard; RT64 picks its
    target formats in `setup`, so it applies at the next launch. High cuts
    banding in fog and gradients; Automatic is High on GPUs with over 512 MB
    of device memory (the Deck qualifies). The MSAA levels offered are
    re-derived for the chosen format.
  - 2D upscaling (`baseline_upscale_2d`: Original, scaled only, all; default
    scaled only) and texture LOD scaling (`baseline_texture_lod`, default on,
    which the player already forced on) apply live: RT64's state reads them
    from the application copies on the graphics thread as it builds each
    workload, so `update_picture` (`minimum_presentation.cpp`) writes them
    from `send_dl` and mirrors them into the shared configuration.
  - Dither noise (`baseline_dither_noise`, 0-100%, default 100): RT64's
    high-resolution dither noise strength. RT64 clears its extended state
    after every workload, so `send_dl` sets it before each display list.
  The live values travel to the graphics owner as one packed atomic
  (`Picture` in `minimum_host.hpp`).

- **Draw distance and Disable LOD** (Video menu, `tooie_draw_distance` 1-12,
  `tooie_disable_lod`; `src/draw_distance.cpp`, raw hooks
  `GRAPHICS_HOOKS` in `tools/baseline/minimum_host_hooks.py`), after
  Lighthouse's `GraphicsPatches.cpp`. Every distance cull multiplies by the
  viewport's draw-distance scale, `func_800E42C0` (viewport `+0x164`):
  `func_800DE498` (model render: focus distance sp+0xDC against `0x8012C82C`,
  set per actor by `func_800DF428` from `func_80101970`), `func_800D3104`
  (draws the props and actors of cubes within the camera's cube +-4 x scale)
  with `func_800D2F44` (cube distance), and `func_800E1C00` (level geometry,
  fading over the last 500 units). A hook after its call (before
  `0x800E42D4`) multiplies the result by the setting (+0.1 past whole
  multiples, as Lighthouse, which stops at 6x; this player goes to 12x), so
  all of them extend together, as Lighthouse scales BK's model cull, cube
  width and cube distance. The first version scaled only the model cull,
  so entities in cubes past about 4 cubes still vanished. Confirmed on the
  Deck by the owner (2026-10-24): distant entities stay visible at 12x. With LOD disabled, a hook before `0x800DE628` skips the
  small-and-far shrink (4000 units, size x 50), and the three LOD geo
  commands (type 8 in the geo tables listed at `0x80123198`:
  `func_800DD214`, `func_800DD2E8`, `func_800DD37C`, which draw their branch
  for squared distance in (min^2, max^2]) see distance 1, picking the
  nearest branch. Not covered: actors the game has not spawned or keeps
  inactive far away, and level geometry culled by other means (fog is
  unchanged).

- **Bounding-box culling kept with upstream's BK pipeline**
  (`tools/baseline/bk_graph_thunks.hpp`, `baseline_bk_bbox_visible`; Linux
  only, Windows keeps upstream's). Alpha 4.4's BK pipeline ports BanjoRecomp's
  `culling_patches`: after the game's bounding-box test (`func_800E3D0C`) in
  the type-13 geo handlers `func_800DDC28` / `func_800DDD14` it forced
  "visible", so every culled sub-model and level chunk was drawn, including
  everything behind the camera. In Jinjo Village (owner's save, 8x) that
  took RT64 from about 1,140 to 2,650 draws and 19k to 39k vertices per
  presented frame (Vulkan layer counts, `local/deck-perf/stats.sh`), plus more
  decal/depth render-pass switches; at 3x the Deck dropped from 90 to about
  60 fps (RT64 skips interpolated frames it cannot render in time), while 2x
  and 1x stayed near 90. Upstream draws everything so that culling cannot
  shift BK's transform ordinals (`cur_model_transform_id_offset` advances on
  billboard 0, sort 1, bone 2 and player selector 12 nodes): skipping a
  subtree containing one would give later bones another ID for a frame, and
  RT64 would pair the wrong transforms. The thunk now keeps the game's result
  when the culled subtree is ordinal-free: it walks the subtree like
  `func_800DE2A4` (node `+4` links; children of 6 at `u32 +8`, LOD 8 at
  `u32 +0x1C`, nested bbox 13 at `s16 +0x14`) and accepts only leaf types
  (3, 4, 5, 7, 9, 10, 11, 16-24, none calls `func_800DE2A4`) and those
  containers. Anything else, unreadable, or past 1,024 nodes / 32 pending
  branches is drawn as upstream does. In Jinjo Village about 75% of bbox tests
  cull and 0.2% of culled subtrees fall back to drawn. The node is `s0` in
  both handlers (`func_800DDD14` sets it; `func_800DDC28` runs with the
  dispatcher's `s0`, the node it dispatched). Edge pop-in while turning
  relies on the frustum padding above, as before the rebase; the game's
  frustum still follows the widescreen setting (4:3 draws less). Worth
  reporting upstream: the same cost applies on Windows.

- **Camera scenes paired by their IDs** (RT64 recipe
  `patches/rt64/owner_index_guard_scene_ids.delta.json`, selected instead of
  `owner_index_guard` when not `WIN32`). Alpha 4.4's camera emits explicit
  matrix-group IDs for the sky (`a0000005` locally) and world (`a0000008`)
  projections, which share the view. RT64's `matchScenes` pairs current and
  previous scenes greedily by matrix distance and only then compares IDs
  position by position; while the camera moves, "current world vs previous
  sky" is sometimes the closest pair, so both pairs cross, nothing maps and
  the camera is not interpolated for one game frame (three presented frames
  hold, then jump). On the Deck this was the micro-stutter while turning
  (F4 reports 2026-10-25: camera unmapped in pairs of workloads three apart,
  flags `0x10033`). With AUTO IDs (before the rebase) any pairing mapped.
  The recipe keeps the distance sort and then moves pairs with different
  explicit IDs behind the rest; scenes without explicit IDs behave as before.
  Found with a throwaway scene logger in `src/rt64_matrix_trace.cpp` (not
  kept): the same scripted run snapped at workloads 884 and 887 every time
  without the recipe and never with it. Also worth reporting upstream (RT64).

- **Draw-call matching without the quadratic pair loop** (same recipe,
  revision 2). RT64's `matchScene` hashes each draw call (combiner, other
  mode, geometry mode, triangle count, matrix IDs) and then visited every
  current x previous call pair with the same hash, inserting transform, tile
  and look-at candidate pairs into `std::set`s. Level geometry with identical
  state shares one hash: in Hailfire Peaks' icy side, looking out of the ice
  cave toward the sky bridge (map `0x128`), one scene had 2,371 calls, a
  557-call bucket and 441,719 pairs per game frame, for 430 transform, ~16,800
  tile and 23 look-at pairs. That made `RT64_Workload` the Deck's limit (84%
  of a core, ~32 presents/s at Manual 60). The sets only depend on per-call
  properties, so revision 2 builds them per bucket from the distinct matrix
  ranges and look-at indices on each side and, for tiles, looks previous
  tiles up by (tile count, slot, matching, TMEM hash). Sorted unique vectors
  replace the sets and iterate in the same order, so the matching is
  unchanged: an A/B build computing both compared every scene (8,400 scenes at
  that view, 6,600 running round Jinjo Village, no mismatch). Dev machine,
  release build at that view: `RT64_Workload` 39% -> 8% of a core, at 6x and
  1x alike. Confirmed on the Deck by the owner (2026-10-25): no lag there at
  6x with LOD off, stable at 90 Hz (F4: 89.7 presents/s, game at 30 every
  frame; before, 31.7 presents/s). The game thread's parse still takes ~16.5 ms
  median there (texture loads and TMEM hashing are the largest part).

- **Pause backdrop without a shifted strip** (RT64 recipe
  `patches/rt64/tile_copy_edges.delta.json`, generic fixes, not `WIN32`).
  While paused the game copies the frozen frame with 32x32 texture rectangles
  (`func_800C0BC0`: into a saved buffer once by `func_800C0F8C`, back to the
  screen every paused frame by `func_800C0FD8`), and RT64 serves each from a
  hi-res tile copy. At fractional horizontal scales the rectangle is drawn
  between the rounded scaled edges, but `createTileCopySetup` rounded the left
  edge and the width separately, so one column's copy was a pixel narrower
  than its rectangle. At 1280x800 (scale 3.5987, 1094 px for the 304-pixel
  framebuffer) that is column 3, native x 96-127: 116 px drawn from a 115 px
  copy, twice, which showed as a full-height strip a bit more than a quarter
  in, shifted right about 2 px (owner report, 2026-10-26). The recipe takes
  the copy between the same rounded edges. Found with throwaway logging of the
  rectangles and tile copies (not kept). Verified locally: pausing in Jinjo
  Village (copy of the owner's save) showed the strip every time at 1280x800,
  1064x800 and 1600x1000 (1280x720, scale 4.0, never did); with the recipe
  the paused backdrop matches the last live frame pixel for pixel outside the
  menu and HUD at all three sizes. Confirmed on the Deck by the owner
  (2026-10-26): the pause backdrop looks right. Worth reporting upstream (RT64).

- **F4 reports carry a guest RAM snapshot** (`tools/baseline/issue_evidence.cpp`):
  F4 also copies the 8 MB guest RDRAM once and writes it next to the report as
  `<report>-rdram.bin` (recomp layout), plus a `game_state` row (map, player
  position). The rest of the report still keeps no guest memory. Use it to
  reproduce a reported spot exactly: the viewport is `*0x8012D500` (eye `+0`,
  forward `+0xC`, pitch/yaw `+0x18`, frustum and view basis to `+0xE0`) and
  Banjo's facing is player `+0x3F4`; `local/deck-perf/hfp/prof/drive.py`'s
  `pin` copies a report's viewport over a local run.

- **Heap: low-memory cleanup and a bigger pool.** Extended draw distance draws
  the props of many more cubes, and their models fill the game's heap in dense
  levels (Glitter Gulch Mine at 6x quit the player within seconds of entering).
  - *Cleanup* (`tools/baseline/adapters.cpp`, `baseline_overlay_boundary_refusal`
    at `0x800A9DA8`): when `heap_alloc` finds no free block it calls
    `func_800A9D90`, which unloads overlays idle for 3+ frames
    (`func_8008160C(2)`, then `ovl_unload`) and frees caches. Upstream refused
    this boundary and quit (`normal-overlay-refusal ... heap-pressure-cleanup`).
    It is now admitted on main after the level's bulk load, unless tidy's defrag
    is busy (`0x80126CBC`) or an ALT body/wrapper is running (they check list
    positions). Each unload goes through the existing external-prefree check
    and unpublication; an unload during a fresh load's allocation updates the
    pending frame's list count. Logged as `baseline heap-pressure-cleanup`.
    Confirmed on the Deck (2026-10-25): 123 cleanups in one session, play
    continued, until the pool itself ran out (`heap-exhaustion` at
    `0x8001A6C0`), which the bigger pool addresses.
  - *Bigger pool* (raw hook `HEAP_HOOKS` in `minimum_host_hooks.py`, before
    `0x8001A8CC` in `heap_setup`; `tooie_heap_extend` in `adapters.cpp`): the
    pool is the `0x2C8800` bytes below `0x80400000` (`func_8001E7E8`: top from
    `func_8001E7E0`, size from `func_8001E830`), so the game never touches the
    upper 4 MB that the runtime, RT64 and host code already map. The hook adds
    `0x400000` to the size before `heap_setup` stores it (`0x8007BF00`); every
    heap end derives from that word, so the pool keeps its start
    (`0x80137800`, which the pre-loader witness checks) and ends at
    `0x80800000`. Top-side allocations made at boot (VI buffers at
    `0x80078F70`, the free-list link at `0x801595F4`) move up by the same
    amount; the post-core2 witness adds `heap_extension` to those words.
    Overlays never reach the `0x80800000` "not loaded" value: the pool's last
    block is a fixed dummy below it. Logged as `baseline heap-extended`.
    Verified on the dev machine (2026-10-25): boot witnesses pass with the
    moved buffers, and a scripted run into Spiral Mountain at 12x renders and
    quits cleanly. Confirmed on the Deck by the owner (2026-10-25): the route
    and settings (6x, LOD off) that ran out in map `0xD6` now play past the
    old failure point with zero cleanups and quit normally.

- **Minimap** (Video menu "Minimap (bottom left, F5)", or F5 in game unless a
  game action is bound to F5; `baseline_minimap`, default on; `src/level_minimap.cpp`, raw hook `PLAYER_UPDATE_HOOKS` just
  before `func_800991B0` calls `bainput_update` at `0x80099228`, with a0
  loaded from s0; it was at `bainput_update`'s entry until Alpha 4.4 replaced
  that function with a guest patch for its Talon Trot toggle). A top-down map of the level's collision floors,
  3000 units around the player, rotated so the camera looks up the map, with
  an arrow for the player's facing. Upstream's `src/minimap.cpp` is a
  breadcrumb radar for the retired frontend and is not used. The hook runs on
  the guest thread for the controlled character (player table `0x80135490`,
  index `0x801354DC`; position `+0xE4`, yaw `+0xF8`; camera `0x8012D500`,
  which looks along -(sin, cos) of its yaw at `+0x1C`) and copies the
  geometry when the map (`0x80132DC2`) or a collision source changes, since
  the heap moves models; the overlay draws only that copy. Sources, all in
  Banjo-Kazooie's model format: the two level layers at `0x801285B0`
  (0x2C bytes each, opaque then water; collision `+0x00`, live vertex list
  `+0x10 + 4 * u8 +0x29`, used when `+0x2A` is set, as `func_800BEE40`
  queries them), and the live map sections (`+0x77` count, `+0x68` array of
  0x58 bytes, bit 30 of `+0x4C`; model = asset `+0x40` from the asset cache at
  `0x8012B800`, offset by the vec3 at `+0x00`, as
  `gcmapsects_entrypoint_21`). Floors (normal Y > 0.3) are drawn low to high,
  those more than 300 units above the player faded and first, so stacked
  rooms show the player's level; walls near the player's height are lines;
  ceilings are left out. The map hides when the player has not been updated
  for 300 ms (menus, cutscenes, loads, pause). Verified locally in Spiral
  Mountain and confirmed working on the Deck by the owner (2026-10-24);
  sectioned maps are not specifically verified.
  Additions (2026-10-25):
  - Area name above the map, from `src/map_names.cpp` (ids and areas from the
    decomp's `include/maps.h`; cutscenes, menus and minigame replays have none;
    ASCII only, since the overlay font has no en dash).
  - Zoom (Video menu "Minimap zoom", `baseline_minimap_zoom`, 0..5 = 6000, 4500,
    3000, 2000, 1500, 1000 units to the edge, default 2). Hold F5 and press A / B
    to zoom while the minimap is visible; those A / B presses (and the buttons until
    released) are kept from the game (`minimap_controls` in
    `tools/baseline/minimum_host.cpp`, on the mapped N64 buttons, so rebinding A / B
    still works). F5 now acts on release, and only if it did not zoom.
  - Markers from the actor list: a `Vector` (`include/vector.h`) whose pointer is at
    `0x80136EE0`, 0x9C-byte actors pushed by `func_80106668`; `func_80107C2C` stores
    the position (f32 x3) at `+0x04` and the descriptor's actor id (the
    `gemarkersDll` table index) in bits 9..20 of `+0x6C`. Ids: Jiggy `0x21F`
    (chjigsaw), note nest `0x1D7`, Treble Clef `0x1D8` (chnests), Jinjo `0x1F4`,
    warp pad `0x1E5`, Mumbo pad `0x2B0`, Cheato page `0x136`. Mumbo's Skull and
    Wumba's Wigwam are level geometry, so `k_doors` in `src/level_minimap.cpp` lists
    each one's door (where the player stands after walking out of its map; measured
    in every world and the Isle o' Hags). maps.h's `0x171` "GGM_MUMBO" is
    Terrydactyland's skull. The vector holds the
    whole map's actors, and a collected one leaves it (verified: collecting an egg
    nest in Mayahem Temple took it out), so the markers show what is still to find.
    Markers more than 400 units above or below the player are dimmed.
  - Full-screen map while paused: the graphics thread samples the game state
    (`D_80127630 +0x02` at `0x80127632`: 3 in play, 4 paused, with gcnewpause's
    instance at `0x80128AF0`) each screen. Paused, the corner shows "F5: Map" and F5
    opens the map fitted to the level (same orientation as the minimap); it takes
    all input: A / B zoom, the stick pans, Start or F5 return to the pause menu.
  - Research tools in `local/minimap/` (not tracked): `capture.py` launches the
    player, dumps RDRAM by signature and can warp (writes the pending transition as
    `func_800A791C` does) or send keys; `actors.py` lists a dump's actors with their
    overlay names. Actor census per world with the owner's save: `local/minimap/tour/`.
  - Verified locally (2026-10-25): area name, note-nest and Jinjo markers in
    Mayahem Temple, the pause hint. Confirmed on the Deck by the owner (2026-10-25):
    names, zoom chord, markers and the pause map; the en dash showed as "?", now
    a hyphen. The hyphenated names and the skull and wigwam markers were then
    confirmed on the Deck too (2026-10-25).
  - Explored ground (2026-10-25, for minigame mazes such as Glitter Gulch Mine's
    dynamite and Mayahem Temple's): Video menu "Minimap: shade explored ground"
    (`baseline_minimap_explored`, default off), or hold F5 and press X to toggle it
    and Y to clear it, on the minimap or the paused full map. X / Y are read from the
    selected controller's physical buttons (`minimap_controls`), since the Deck
    layout maps X to N64 B as well (B zooms out); the N64 buttons pressed in that
    tick are kept from the game. While on, the player-update hook marks 50-unit
    cells whose centre is within 150 units of the player, per map, with the height
    of the first visit; turning it off keeps them, Y (or a restart) forgets every
    map's. The SDL owner never touches the cells: clearing is a flag the guest
    thread applies. Rows of cells are merged into runs (republished at most every
    100 ms) and drawn orange over the floors and under the walls, dimmed more than
    400 units above or below the player; the full map's legend shows "Explored".
    Verified locally (shading in Spiral Mountain from the setting); confirmed on the
    Deck by the owner (2026-10-25), chord included.

- **Hold L to levitate** (Controls menu, `baseline_levitate`, default off;
  `src/levitate.cpp`, from the same hook as the minimap), Lighthouse's `Cheats.Levitate` (`Cheats.cpp`). For the controlled
  character, while `bakey_held(player, BUTTON_L)` (so the game's per-button
  suppression applies): gravity 0 and vertical velocity 500 (the physics
  struct at PlayerState `+0xC8`, fields `+0x34` and `+0x14`, as
  `baphysics_set_gravity` / `baphysics_set_vertical_velocity`), and
  `func_800956B8` (bafalldamage_start, as Flap Flip each frame) so the fall is
  measured from where L is released. On release, or when turned off while
  held, gravity returns to the default at `D_80117EF0`
  (`baphysics_reset_gravity`), once. As in Lighthouse, releasing L high up
  gives a damaging fall, and tapping L again lower down cancels it (the fall is
  measured from the tap); Lighthouse's explicit reset to the idle state was not
  needed. Not copied: Lighthouse pauses while talking (`balookat_getState`,
  which Tooie lacks); in Bottles' dialogue holding L changes the physics but
  Banjo stays put and falls back after. Verified locally in Spiral Mountain
  (scripted L: rises 500 units/s, gravity -2700 restored on release, splat
  from about 1300 units, normal landing after a tap at about 900). Confirmed on the
  Deck by the owner (2026-10-24), with L1 bound to L.
  `baseline_input_host_test` (not built by default) would also need
  `src/draw_distance.cpp`.

- **Autosave** (Controls menu "Autosave every", `baseline_autosave_minutes`,
  0..30, default 5, 0 = off; `src/autosave.cpp`, from the same hook as the minimap,
  so the timer only fires while the player is being updated).
  The game already saves by itself: every map change
  (`func_800A72A4` → `gcsectionDll_entrypoint_1(1)`) and the pause menu's
  Save & Exit call `func_800D389C`, which saves through
  `glsavegame_entrypoint_7` → `glgamestore` → `gleeprom` →
  `osEepromLongWrite`. Autosave calls that same wrapper on a timer, only when
  `func_800D3770` (the game's own "may save here": a file slot loaded and
  allowed worlds in `D_8012762C`) agrees and the file state `D_8012B3F0+9`
  is 0. A game save (the save generation in `src/save_persistence.hpp`
  changes) restarts the timer, and no autosave starts while a save is still
  being written. Before each autosave the save file is copied to
  `saves/bt.n64.us.1.0.bin.autosave.bak` (temp file, fsync, rename); if that
  fails the autosave is skipped. At startup a save of the wrong size (the
  runtime's `finalize_output_file_with_backup` copies over the save in place,
  so a crash there can truncate it, and the runtime only falls back to its
  own `.bak` when the save cannot be opened) is replaced by the autosave
  backup. "Saving..." shows in the bottom-right corner for at least 1.5 s and
  until the save is on disk. Log lines start with `player autosave`.

- **Pause-menu warp** (`src/pause_warp.cpp`, raw hooks `PAUSE_WARP_HOOKS` in
  `minimum_host_hooks.py`; always on, no setting). The game's own pause list
  (gcnewpause) gets a WARP row after its own rows. Choosing it turns the same
  list into the menu a warp pad opens (chwarppad state 3): the world's pad text
  block `0x185A + world` with its title (string 0, "Choose a destination") and
  portrait `0x6F` in the header box, the pad names (strings `pad + 9`) and the
  pad table's icons, only for the pads activated in the current world (save
  flags `0x3AB + world * 5 + pad`). Rows the current form may not use are
  greyed and answered with the pad's refusal text, as at a pad (the table's
  transformation mask through `func_80090178`, colour and texts from
  `D_80801344` / `D_80801080`). The pause HUD counts are hidden while the pad
  list shows. A on a usable row closes the pause like Return to Game; on the
  first player update back in play (the bainput_update hook; the player update
  also runs while the pause closes, game state 4) it sets
  `FLAG3_9FE_WARPING_VIA_WARPPAD` and calls `_gcgoto_entrypoint_1(map, 0x1E +
  pad - 1)`, so the destination pad places the player as after a real pad
  warp. B returns to the pause list with the cursor on WARP; Start closes the
  pause.
  - The row is hidden unless the world (`D_8012762C`) has warp pads, at least
    one activated pad is usable in the current form, the pause is a story
    variant (0..3, not replay or multiplayer), the map is not a boss arena,
    minigame (story or `MP_` copy), Chuffy's cab or wagon (`k_blocked_maps`),
    and the game's own "may talk to an object" test passes
    (`func_800F4B8C(player, -1, 4)`, the test a pad uses through
    subaddiedialog: steady footing, no cutscene or transition, not Clockwork
    Kazooie or the Golden Goliath). A minigame played on an open map (JRL's
    Mini-Sub Challenge) is only covered by that test.
  - How it is built without guest patches. The option table `D_80802070` is
    not changed: the row is the index one past the variant's rows, and the
    hooks answer that index. `gcnewpause_entrypoint_0` allocates `0x40` more
    bytes, which hold the feature's state (a marker, the world, the activated
    pads, the list flag, the label). Never keep host pointers to the pause or
    into it: the game's heap defragmentation moves the pause while it is open
    (seen in testing; a first version that pointed the table at the pause broke
    on it). Hooks: `func_80800A08` before `0x80800ADC` adds the row (count and
    icon `0x97`; at most five rows visible, since an OptionState has five row
    boxes at `+0x3C`: a six-row list scrolls, and a sixth visible row crashed
    the first Deck test); `func_80800C54` before `0x80800CB8` its text index `0x7E`;
    `gldbstring_entrypoint_1` at entry returns "WARP" for that index of the
    open pause's block (`D_80128AF0`); `gcnewpause_entrypoint_4` (the option
    callback) at entry runs the row's A/B/execute events as the original does
    for a row without a confirmation, and on execute rebuilds the pause's own
    OptionState as the pad list (mode 0, the pad's `gcnewoption` calls), then
    answers that list's events; `gcnewpause_entrypoint_2` before `0x80800184`
    marks Start hiding the list. Destinations are the pad table
    (`D_808010B0_chwarppad`) with `func_80800E10_chwarppad`'s ROM lookups
    resolved offline; the warppad overlay is not loaded during the pause.
  - Log: `player pause warp: world W pad P -> map M exit E`.
  - Verified locally (2026-10-26, owner's save copy, Mayahem Temple with one
    activated pad): the row shows after Save and Quit with the label; WARP
    opens the pad list with title, portrait, name and icon; A warps (the
    game's jigsaw transition, the area title, Banjo on the pad); B returns to
    the pause list; Start from the pad list resumes play. Not verified: a
    cross-map warp, several rows (scrolling past four), greyed rows and their
    refusal text, other worlds, the variants with Objects / Jinjos rows.
  - Steam Deck, confirmed by the owner (2026-10-26, build `a31b7984`, real
    save): the six-row pause list scrolls to WARP, warping works (log: Mayahem
    Temple pad 4 to Jade Snake Grove, map `0xC4`), and "everything looks good".
    The first Deck build crashed opening that pause (six visible rows); fixed.

- **First-person mouse aiming, for Steam Input gyro-as-mouse** (Camera
  settings, `tooie_mouse_aim` default on, `tooie_mouse_sensitivity` in
  hundredths of a degree per count, default 0.06, `tooie_mouse_invert_y`
  default on; defaults chosen on the Deck with Steam's gyro-as-mouse).
  A raw hook before `0x80800368` in `func_808002A4_bafpctrl` (the
  first-person controller: pitch/yaw -= stick x 90 x sensitivity x dt, then
  the pitch clamp; f12 = pitch, f6 = yaw) calls `tooie_first_person_aim`
  (`src/camera_analog.cpp`): it notes first-person control for the input
  module and applies the mouse motion taken since the last frame (pitch +=
  y, yaw -= x, matching the stick). That controller is not what normal
  first person uses: C-up first person aims eggs through
  `func_80800264_baeggaim` (the reticle follows the stick, `func_80800170`
  turns the view near the screen edge, then pitch is clamped unless flag
  0x3A locks it). Two more hooks there add the mouse turn: pitch before
  `0x808002E8` (unlocked path, before the clamp, sp+0x24) and yaw at
  `L_80800300` (every frame, sp+0x28). Relative-mode changes are logged as
  `input mouse-aim relative=.. wanted=..`.
  The cursor is hidden (`SDL_ShowCursor`) whenever the settings menu is
  closed, so gyro-as-mouse motion outside first person shows nothing; ImGui
  leaves it alone because of `ImGuiConfigFlags_NoMouseCursorChange`. The SDL owner
  (`src/frontend_input_preference.cpp`) holds relative mouse mode only while
  first-person control ran in the last 150 ms, game input is enabled and the
  window is focused, and accumulates `SDL_MOUSEMOTION` xrel/yrel only then;
  motion from before entering first person is dropped. On the Deck, keep
  Steam Input on and set the gyro to output as mouse in the layout. A native
  SDL gyro path (Steam Input off) was tried first and did not work on the
  owner's Deck.
  Three more places, each with its own Camera setting under the mouse aim
  (default on): **first-person flight** (`tooie_mouse_aim_flying`),
  **first-person swimming** (`tooie_mouse_aim_swimming`), the **submarine's
  first person** in Jolly Roger's Lagoon (`tooie_mouse_aim_submarine`) and
  the **Saucer of Peril** crosshair (`tooie_mouse_aim_saucer`). Deck (2026-10-25):
  the owner confirmed underwater, the submarine and on-foot egg aim feel
  the same on both axes after the camera-rate change below; flight and the
  Saucer of Peril crosshair not yet reported. Flight and swimming put
  bafpctrl in its body mode (state +0x11, set by `bafly_entrypoint_19` and
  `baswim_entrypoint_7`): `func_8080022C_bafpctrl` points the view along the
  player's own yaw and negated pitch, and egg aim skips `func_80800264`, so
  the hooks above never run there. The mouse steers instead: raw hooks before
  `yaw_setIdeal` in `bafly_entrypoint_9` (`0x8080055C`) and
  `baswim_entrypoint_11` (`0x8080039C`) add the mouse yaw to the ideal yaw,
  and hooks before `func_8009BF5C` on both stick-sign branches of
  `bafly_entrypoint_10` (`0x808005D4`, `0x808005F4`) and
  `baswim_entrypoint_12` (`0x808004D0`, `0x808004F4`) add a held mouse pitch
  to the stick's ideal pitch, kept inside the steering's range (flight
  `[arg1 - 360, arg2]`, -60..80 for Banjo, Kazooie and the bee; swimming -85..85).
  The submarine (`bssub`, also body mode through `bafpctrl_entrypoint_18`)
  steers the same way: yaw before `0x8080074C` in `func_80800634_bssub`,
  pitch before `0x808007FC`/`0x8080081C` in `func_80800768_bssub` (player at
  sp+0x30, range from sp+0x28/sp+0x24: -85..85 underwater, -35..30 on the
  surface). They act
  only while the PlayerState's bafpctrl state (+0x90) is in first person
  (+0x10 is 3) in body mode; 2 is third person with C-up first person
  available (`bafpctrl_entrypoint_15` enters 3 on C-up), and swimming and
  flight keep calling the steering there, so accepting 2 made the gyro
  steer Banjo outside first person (fixed 2026-10-25, Deck-confirmed for
  swimming and flight); the held pitch is dropped after 150 ms
  without the hook. The view follows the ideal yaw but the current pitch
  (PlayerState +0xDC: current +0, ideal +4), which eases to the ideal at a
  capped rate, so up/down felt slower than left/right on the Deck; the hook
  now moves the current pitch by the mouse's share too (the stick's share
  still eases). That was not enough: up/down still kept moving after the
  Deck stopped. In body mode the first-person camera eases its shown
  rotation (+0xC) to the target (+0x24) at capped rates, set by
  `ncba1p_entrypoint_1` through `func_80111590` (pitch 120, yaw 300 degrees
  per second, `func_80111410` in `func_808003DC_ncba1p`; on foot `bsfirstp`
  turns the easing off with `ncba1p_entrypoint_4`). A hook before that call
  (`0x8080042C`) first moves the shown pitch and yaw by the mouse's share of
  this frame's turn, never past the target, so only the stick's share is
  capped. On-foot egg aim keeps those default caps too (only the Breegull
  Blaster, `bsfirstp`, sets 360/360), and the owner noticed the same slower
  up/down there, so the egg-aim and first-person-look hooks hand their share
  to the same hook; a share older than 100 ms is dropped. The Saucer of Peril ride (`bsride`, map 0x124) holds
  Banjo in egg aim with the camera on rails, so the mouse moves the
  crosshair: a hook before `0x8080012C` in `func_808000B4_baeggaim` adds a
  held offset (5 cursor units per degree) to the stick's target (f14,
  sp+0x20; x 38..266, y 28.5..199.5) and moves the cursor (s0) by the same
  amount, so the game's easing does not trail it. On that map the egg-aim
  view hooks take no mouse motion. Still to confirm on the Deck: flight,
  and the Saucer of Peril crosshair's scale.
  The **Code Chamber** (map 0xBC, cheat codes) podium is egg aim with the
  camera locked: only `func_808000B4_baeggaim` runs, `func_80800264` does
  not (traced on the Deck). The same crosshair hook moves the cursor there
  whenever the egg-aim view hooks have not run in the last 100 ms (own
  setting `tooie_mouse_aim_code_chamber`, default on; Deck-confirmed
  2026-10-25). Away from the podium C-up there is plain first-person look.
  Reported but not reproduced: the camera in Jiggywiggy's puzzle moved with
  the gyro. Not traced (the scratch save cannot reach it); the owner will
  report if it still happens after the state fix above.

- **Twin-stick Breegull Blaster** is upstream's since Alpha 4.4: Controls
  "First-person aiming with right stick" (`tooie_first_person_aim`; guest
  patch `tools/baseline/guest/minimum-host/modern_fps.c`, which replaces
  `func_80804848`, `func_80808E78`, `func_808099B4`, `func_808069FC` and
  `func_80806928` in `bsfirstp`). Deck builds turn it on for profiles that
  have not saved it. This fork's own version (raw hooks in those functions,
  right stick turn 200 deg/s and look 160 deg/s through the analog camera
  settings, left stick X strafing) could not hook the replaced functions and
  was removed in the rebase; upstream's sensitivity and invert settings are
  separate from the camera's. Not yet compared on the Deck.

- **No on-screen keyboard at launch** (`tools/baseline/devices.cpp`, before
  `SDL_Init`): sdl2-compat before `30ee26c` (2026-06-07) starts text input
  on each new window without suppressing the screen keyboard, and SDL3 shows
  Steam's keyboard when no physical keyboard is attached, so it opened on
  every Deck launch. `SDL_ENABLE_SCREEN_KEYBOARD=0` at default priority (the
  environment variable still wins); text input events and Steam+X still work.

- **No startup crash after a slow first RT64 workload** (`tools/baseline/devices.cpp`,
  `stop_dummy_workloads`, non-Windows only). Until the game starts, ultramodern's VI thread
  queues a dummy workload every VI: a 320x240 RGBA16 fill at `0x80700000` / `0x80725800`
  and a full sync, which writes that framebuffer back into RDRAM. The player started the
  game on the first VI, without waiting for them. When the graphics thread's first workload
  was slow (RT64 building its first pipelines on a cold shader cache), a dummy workload
  still in flight finished about 0.6 s after start, by when the game's heap owned
  `0x80700000+`: it zeroed `0x80715A90..0x80725800`, which holds the sky model loaded by
  `gcskyDll` (`func_800A72A4`). The first frame's `G_DL 0x8071F5A8` then ran through zero
  words (no-ops) into stray data until a `G_TRI2` with no vertex loaded, and
  `RSP::drawIndexedTri` read `tcFloats[0]` of an empty workload (SIGSEGV, about 1 launch
  in 3-4 on the dev machine, every launch with `__GL_SHADER_DISK_CACHE=0`). Proved with
  a hardware write watchpoint on the sky DL word (`local/`, not tracked): the game
  thread writes it, then `send_dummy_workload` -> `State::fullSync` ->
  `copyNativeToRAM` zeroes it. Now `send_dummy_workload` runs under a mutex and does
  nothing once stopped; the start path try-locks it each SDL update, sets "stopped" and
  only then calls `start_game` (logged as `baseline start_game deferred for a running
  dummy workload updates=N` when it had to wait). The bug is upstream's (the same code
  runs on Windows, where a warm driver cache hides it) and still to be reported
  there; the Windows build is left unchanged here.

- **Frustum padding** (`tools/baseline/minimum_host_hooks.py`, `FRUSTUM_HOOKS`):
  Tooie keeps its culling planes in the camera struct (FOV `+0xE4`, aspect
  `+0xE8`, plane normals from `+0xF8`), built by `func_800CA1B0`,
  `func_800CA218` and `func_800CA2A8` from exact half-angles in degrees
  (read by `func_800AB0BC`/`func_800AC978`). With no slack, widescreen edges
  and RT64's interpolated frames showed background where geometry was culled
  while the camera turned (upstream known issue). Three raw-corpus hooks add
  5 degrees to those angle arguments; projection is untouched. The hooks ride
  the existing `add_hooks`/`strip_and_verify` proof, so the guest image and
  its pins are unchanged. Found via Lighthouse (HarbourMasters' Banjo-Kazooie
  port), which pads its frustum the same way.
- **Underwater sway at the right width** (`src/underwater_aspect.cpp`,
  `UNDERWATER_ASPECT_HOOKS`). At host width upstream's guest
  `cam_unwiden_raster` (`camera_emitters.c`) narrows the world camera's raster
  projection to 4:3 so RT64's AUTO aspect does not widen it twice, but only
  when the camera aspect (`+0xE8`) is the view's own aspect. Underwater,
  `chcamerawibble` feeds `func_800C54F0` fov/aspect offsets every frame
  (`func_800C4B90`, `D_8012A9A0+0x78/+0x7C`), so the view stayed widened twice
  (about 17% too narrow at 16:10), and every few seconds, when both offsets
  were exactly zero, one game frame was drawn correctly: a visible snap.
  Seen in an F4 report as `player_projection` raw ratio exactly 1.3333 for one
  workload against about 1.6 otherwise. The aspect offset is scaled by the
  widened aspect, so the guest's widening factor is exact for swayed aspects
  too: one hook records what `func_800C54F0` writes when an offset is
  nonzero, and one at the end of `func_800CB124` (called only by the guest's
  `func_800E44FC`) narrows the raster copy under the same conditions as
  `cam_unwiden_raster`, leaving frames the guest narrows itself alone.
  Confirmed by the owner on the Deck (2026-10-25): no more snap underwater.

- **Interpolation at Display/Manual output** (`linux/INTERPOLATION.md` has the
  details, every case found so far and the debugging workflow). Game select's
  actors get matrix groups (`src/menu_interpolation.cpp`) and follow the chosen
  rate; the startup intro (map `0x18B`) is still presented at Original
  (`devices.cpp` `update_screen`, logged as `baseline map=0x... menu_rate_original=`).
  `projection_residual` revision 2 stops the letterbox from squashing the
  scene as it opens. This fork's **Interpolate all objects** (a matrix group
  per actor and for the player, `src/object_interpolation.cpp` with the RT64
  recipe `object_groups`) was removed after the Alpha 4.4 rebase: upstream's
  BK pipeline now gives actors and the player their own identities, the two
  clashed in the pause menu (honeycomb health pops, Banjo-Kazooie head
  ghosts), and with ours off the owner confirmed on the Deck (2026-10-25)
  that Cheato's pages, Stony's backpack menu and the warp pad icons, the
  cases it was written for, still look right. Its raw hooks at
  `func_8010A79C` are back to the game-select menu groups only, and the
  `func_800DE498` entry/return hooks are gone.

- **Analog camera** (Deck default on, invert X on): the retired frontend's
  `src/camera_analog.cpp`, unchanged, linked into the player
  (`cmake/Baseline.cmake`, built with `TOOIE_NATIVE_HOST=1` for that file).
  A raw hook in `minimum_host_hooks.py` before `0x800A4908` in `func_800A4878`
  (the original C-left/C-right gate) calls `tooie_camera_analog_apply`, which
  feeds right-stick X proportionally into the original rotate routine
  (`func_80114D98`, `func_801111D0`). Settings `tooie_analog_camera_enabled`
  `tooie_camera_invert_x` and `tooie_camera_invert_y` (menu checkboxes);
  with it on, both right-stick axes leave the C-button bindings and the Deck
  layout's view toggle (C-down) is on the right stick click.
  `baseline_input_host_test` (not built by default) now needs
  `camera_analog.cpp` too.
  The player's `tooie::input::snapshot()` used to hard-code the right stick
  to zero, which made the first Deck build's analog camera dead; it now
  publishes `published_rx`/`ry`.
  The camera is a Lighthouse-style orbit (`FreeLookCamera.cpp`), owned in
  native code while active: entered at a 0.30 stick, yaw changes by stick x
  160 deg/s x dt
  and elevation by stick y x 100 deg/s x dt (up raises the camera, as in
  Lighthouse; clamped to Lighthouse's pitch range, -40 to 85 degrees of
  elevation, and an entry angle outside it only moves inwards), distance and
  elevation are captured on entry, the aim height is smoothed at
  rate 8, and it stays where it was left. The stick is shaped as Lighthouse's
  free-look receives it (`frontend_input_preference.cpp`): libultraship's
  default `ControllerStick::Process` (20% circular deadzone on its +-85
  scale, N64 octagon, int8), Lighthouse's `controller_getRightStick` divide
  by 127 (a full push reads 0.67), then `ReadStick` (radial 0.15 deadzone,
  rescaled). Full tilt is therefore 98 deg/s yaw and 61 deg/s pitch, and
  nothing moves below about 38% deflection. The first Deck builds skipped
  the libultraship and /127 stages and turned 1.6x faster at full tilt.
  Lighthouse also eases the camera to its orbit target with a spring
  (`func_802BE190`, gain 20); this orbit places it directly.
  A second raw hook at `0x8011061C` in `func_80110588`
  (`tooie_camera_orbit_apply`), after the mode update and where both of its
  paths meet, writes the camera's own state (+0x74, +0x80, yaw controller
  +0x10/+0x14, velocity +0x18 = 0) before the view copies are taken, so the
  game's shake (`0x800893E0`) and roll (`func_80116024`) still apply and the
  game resumes from the pose. (It used to sit at `0x8011066C` and overwrite
  the copies, dropping both.) Tooie conventions: rotation (pitch, yaw, roll)
  in degrees, yaw = atan2(dx, dz) of position minus focus
  (`func_80112550`), pitch = 360 - elevation. Distance: eased (rate 4) from
  the live distance on entry to the far end of the mode's range
  (`func_80115358`: +0xC..+0x10 of the block at +0x14, set per view by
  `func_800A3BA8`, so it follows the view toggle and the statue's 1.5x)
  times the **Camera distance** setting (`tooie_camera_distance`, 50-150%,
  default 100), standing in for Lighthouse's zoom distance. Tooie's follow
  camera sits anywhere in that range, so the old capture-and-clamp often
  orbited at the near end.
  Flight: `bsbfly`/`bskazfly` select mode 3 (`ncbafly`), whose rule entry is
  zero (no C-button turns), so the input hook admits mode 3 explicitly, the
  yaw-transition exit is skipped there, and the orbit hands back to the
  chase camera 0.6 s after the stick is released. The input hook is skipped
  on some frames (a move's camera lock via `func_8009CC68`, camera zones,
  flight), so the orbit only ends after 0.5 s without it (or on a mode
  change, checked in the orbit hook); ending on the first missed frame made
  it drop out and re-capture the game's pose. The stick is read in the orbit
  hook, every camera frame, rather than accumulated by the input hook.
  Time steps use the game's own frame delta (`D_8012C760`, `time_getDelta`,
  as Lighthouse does), not wall-clock time between hook calls: the
  recompiled game runs each frame in a burst and then waits for the display,
  so wall-clock steps were uneven and the turn looked jagged.
  Several camera components can be updated per frame (Mumbo next to his
  Golden Goliath, split-up). The orbit belongs to the player the input hook
  saw (s0 of `func_800A4878`); the orbit hook ignores other components and
  only the input hook adopts a relocated one. It used to adopt any component
  in the same mode, so a second camera took the input frame and ended the
  orbit.
  Collision mirrors Lighthouse's `port_freeLookCamera_update`, using Tooie's
  counterpart of each BK routine (Tooie's own follow mode, `ncbapivot`, runs
  the same chain through `func_80111CC8`):
  - `func_8011329C` = `func_802BE60C`: sweep a 45-unit sphere from the
    position `func_8011347C` saved at the start of the frame (pointer at
    +0x28) to the orbit target and slide; honours the no-collision flag
    (+0x64 -> +0x19).
  - On a colliding frame only, `func_802BC84C`'s probe: ray
    (`func_8011349C`) from the camera to the focus, then 100 units to each
    side, then 100 above; five blocked in a row call `func_801138F4`, whose
    `func_80112E98` is `func_802BC640` (angle table at `0x80124A38`, 150 /
    100 clearance), to move the camera to a clear spot.
  - Otherwise `func_802C03BC` (Tooie `func_80111C04`, reimplemented rather
    than called so the game's own guard state at +0x34 is untouched): if the
    slide turned against the intended move this frame or on the last
    colliding frame, the camera stays at the start position.
  - `func_802C04B0`: the orbit yaw is re-read from the resolved position.
    Pitch is kept, as in Lighthouse.
  The orbit ends on a player yaw transition (yaw controller +0x0C nonzero
  with priority +0x0E = 2: C-left/C-right via `func_801111D0`, R centring
  via `func_80111154`), a camera mode change, or 0.5 s without manual
  rotation (cutscene, dialogue, view toggle). The pivot camera's own wall
  avoidance (`func_80113978`, called every frame by `ncbapivot` and other
  follow modes) starts transitions of the same kind at priority 1; those
  used to end the orbit whenever Banjo walked near a wall. They are now
  cancelled each orbit frame (+0x0C and +0x0E cleared, as `func_80112A84`
  does on arrival), since the orbit has its own collision; a frame
  with +0x72 set (mode update skipped) leaves the camera alone. The camera
  component is relocated by the heap, so its address changing in the same
  mode keeps the orbit. Measured with a virtual pad, before the libultraship
  shaping and collision: constant 84.7 deg/s from the first frame at 0.53
  shaped stick, rotation equal to yaw, zero motion after release, kept while
  walking.

## Verified vs not

- Verified on the dev machine (Arch, Ryzen 9 3900X = Zen 2, RTX 4070 SUPER,
  Vulkan): boots, reaches map loading, clean shutdown and saves; 16:10 output
  observed; adjacent-ROM startup; controller DB gap-filling with synthetic
  entries; repository checks.
- Steam Deck (SteamOS 3.8.28, glibc 2.41, RADV), confirmed by the owner:
  runs after the Vulkan window flag fix, and the frustum padding removed the
  green edge pop-in while turning the camera; the menu-map rate override
  stopped the logo and picture-frame jitter with gameplay still at 90 Hz.
- RT64 `d9817c68` on the dev machine (scripted input, `SDL_VIDEODRIVER=x11`
  for window capture): boots, menu renders through the new ImGui font path,
  intro, game select, a new game into Spiral Mountain, pause menu and
  dialogue; the 16:10 HUD showed no clipped or misplaced element there, so
  no scissor/viewport alignment commands were added. Picture options apply
  (logged as `baseline picture config-applied`); clean shutdown.
- Steam Deck, confirmed by the owner after the RT64 update: the build runs,
  game select, the picture options and the settings menu work. With
  Interpolate all objects: Cheato, Stony and the warp pad menu (including the
  letterbox opening) look right at Display rate, and nothing looked worse (the option was removed after the Alpha 4.4 rebase; see above).
- Not verified: real gamepad input with the XBLA layout, battery and
  90 Hz pacing, extended gameplay.
- Alpha 4.4 rebase (2026-10-25), dev machine only: the clean build passes all
  of upstream's gates, and the embedded guest image is upstream's pinned
  `263e1d27…`. A scripted run boots, reaches game select and a new game in
  Spiral Mountain with the minimap drawn (the moved player-update hook), and
  shuts down cleanly with the save persisted. Not yet verified, and needed
  from the Deck: Fit window at 16:10 in gameplay, upstream's right-stick
  first-person aim in place of the old twin-stick Blaster, levitate and
  autosave from the moved hook, and the profile rename with upstream's folder lock.
- Bounding-box culling with the BK pipeline (2026-10-25), dev machine only:
  RT64's draws, vertices and render passes in Jinjo Village match the
  pre-rebase build exactly (68.4k draws/s at 60 presents); four concurrent
  copies at 8x + MSAA 8x run faster than without it (68 vs 42 and 235 vs 93
  fps total in two noisy rounds); a recorded sweep around the village shows
  the same edge behaviour as without it (no single-frame edge spikes in
  either). Steam Deck, confirmed by the owner (2026-10-25): holds 90 fps at
  3x in Jinjo Village. With the camera scene pairing recipe added
  (build `60d0f39f`), confirmed by the owner the same day: the micro-stutter
  while turning is gone and 90 fps holds. With Interpolate all objects off,
  the pause-menu jitter (honeycomb pops, head ghosts) is gone, and Cheato's
  pages, Stony's backpack menu and the warp pad icons still look right, so
  upstream's per-object identities now cover what that option was for.
- Startup crash fix (2026-10-25), dev machine only: with the NVIDIA shader
  disk cache off (`__GL_SHADER_DISK_CACHE=0`), the trigger, the unfixed build
  `c02fd344` crashed 10 of 10 launches and the fixed build 0 of 30; with the
  normal cache, 0 of 30. A 45 s run with the cache off waited for the dummy
  workload (833 updates), then loaded the save into Jinjo Village and rendered
  normally. Not verified on the Deck: the crash was never seen there, but a
  cold RADV/Fossilize cache (first launch after an update) could trigger it.

## Testing gotcha: SDL on the dev machine

Arch ships `sdl2-compat` (SDL2 API on SDL3), which hides bugs that real SDL2
exposes; the missing `SDL_WINDOW_VULKAN` flag above only crashed on the Deck.
Test against real SDL2 by copying `libSDL2-2.0.so.0` out of the toolchain
image and running with `LD_LIBRARY_PATH` pointing at it. On the Deck,
`coredumpctl info -1` gives a symbolized backtrace for a `SYMBOLS=1` build.

## Testing input without a gamepad

You are in the `input` group, so a script can create a virtual Xbox 360 pad
through `/dev/uinput` (raw ioctls, no extra packages) and hold any stick.
The game only accepts input while its window has focus; desktop test runs
usually lack it, so a throwaway build that ignores focus is needed for such
runs (never commit it). QA `--input-script` files can only press N64 inputs.

## When pulling upstream

**The most important rule: every Steam Deck feature in "Source changes" above
must survive the rebase and keep working.** A merge that builds but loses or
breaks one of them is not done. In practice:

- Before the rebase, list the features from "Source changes" (Deck defaults,
  16:10, XBLA layout, intro and cutscene skips, picture and anti-aliasing
  options, draw distance and LOD, frustum padding, underwater sway width,
  minimap, levitate,
  autosave, pause-menu warp, heap, analog camera, mouse/gyro aim, the culling and camera
  fixes, draw-call matching speed, the pause backdrop tile copies, the F4 RAM snapshot, the startup
  dummy-workload fix, and anything added since).
- When a conflict or a moved/failed raw hook touches one, carry it over (move
  the hook, re-anchor the code); never drop it to make the build pass.
- Drop ours only where upstream now provides the same feature (as with rumble,
  Linux enablement and the twin-stick Blaster in Alpha 4.4), say so here, and
  have the owner confirm upstream's version on the Deck.
- Afterwards, check each feature on the dev machine where possible and list
  the rest for the owner's Deck test; record what was confirmed.

- `git fetch upstream`, back up the branch, then `git rebase upstream/main`
  with `git config rerere.enabled true`: most commits touch
  `tools/baseline/minimum_host.{cpp,hpp}` (the `Preferences` struct, the
  `Preference` enum, the settings atomics) and `minimum_menu.cpp`, and each one
  conflicts the same way. Keep both sides; drop ours where upstream now does
  the same thing (Alpha 4.4: Linux enablement, rumble, the Vulkan window flag,
  `localtime_r`, three-point filtering, 16:10).
- The guest image is upstream's pinned image: this fork has no guest patch
  edits, only raw hooks. Keep it that way so upstream's pins stay valid.
  Upstream can replace a raw function with a guest patch at any time; a raw
  hook in it then fails with a `KeyError` on the function name in
  `minimum_host_hooks.strip_and_verify`. Move the hook to a caller or drop it.
- Upstream's guest preparation is not always reproducible from a clean tree:
  some checks pin intermediate outputs of its own older build tree. Alpha 4.4
  needed four fixes (`prepare_silo_bridge.py`: the plain bone object, its
  helper body and prologue offset, measured from Alpha 4.4 source;
  `prepare_camera_bridge.py`: the clean `prior/boot.bin` accepted next to the
  legacy one, and `D_801354DC` added to the reference symbols, which the
  legacy silo symbol table supplied; `prepare_player_guest.py`: the carrier
  CPU625 object without ckpack copy storage, as the BK-pipeline image builds
  it). None of them may change the output: the gate is the final guest image,
  which must still equal upstream's `IMAGE_SHA256` and `LAYOUT_SHA256`.
- RT64 recipes: any recipe upstream adds or changes is written against its
  RT64 pin, not this fork's. Materialize every recipe against `deps/rt64`
  (`tools/prepare_rt64_matching.py`'s `materialize`), relocate the ones that
  fail by their exact spans, and record it under "RT64 pin" above.
- `tools/check_repository.py` differs from upstream (it allows `AGENTS.md` and
  `CLAUDE.md`, and lists `docs/RENDERING.md`); keep that when upstream changes
  the file.
- If upstream changes `windows-player` preset options, `build.sh` follows
  automatically; new Windows-only CMake code may need another `if(WIN32)`.
