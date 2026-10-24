# Linux / Steam Deck port: handoff notes

State of Banjo-Tooie: Steam Deck Port, the Linux fork of Banjo-Tooie: Scurvy
Dog Port, for whoever picks it up next. Upstream builds only for Windows; this fork builds a native Linux
player, tuned for the Steam Deck OLED.

## Repository layout

- Upstream: `https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port`
  (remote `upstream`). Fork: `git@github.com:Moudoux/banjo-tooie.git`
  (remote `origin`). Work lives on branch `linux`; update with
  `git fetch upstream && git rebase upstream/main`.
- Commits use the GitHub noreply address set in this repo's git config; the
  owner's real email is private and GitHub rejects pushes that expose it.
- `tools/check_repository.py` must keep passing. It rejects tracked
  `AGENTS.md`/`CLAUDE.md` and undeclared `docs/` files, so these notes live here.

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
  matched the Windows guest image byte for byte before the 16:10 change.
- **libc++, linked statically.** A hash-pinned runtime timer patch
  static_asserts `high_resolution_clock == steady_clock` (true for MSVC and
  libc++, false for libstdc++). Static linking means the host needs no libc++.
- **`-ffp-contract=off`.** Zen 2 has FMA; contraction would change guest float
  rounding versus the N64/Windows build.
- **Release (-O3, stripped)** replaces the preset's RelWithDebInfo.
- **Skipped on Linux:** raw-archive qualification (step 5) and packaging; both
  parse PE/MSVC maps. Gated by `if(... AND WIN32)` in `cmake/Baseline.cmake`.

## Source changes (all keep the Windows build unchanged)

Linux enablement:
- `cmake/Baseline.cmake`: allow Linux; system SDL2 + `RT64_SDL_WINDOW_VULKAN`
  (ultramodern's Linux `WindowHandle` is `SDL_Window*`); Windows-only libs,
  link flags, DLL copies and the qualification gate behind `if(WIN32)`;
  selects `player_startup_posix.cpp` + `nfd` on Linux.
- `tools/baseline/player_startup_posix.cpp` (new): POSIX twin of
  `player_startup.cpp`. Profile in `$XDG_DATA_HOME/BanjoTooieSteamDeckPort`
  (default `~/.local/share`), `flock` on the profile marker,
  `renameat2(RENAME_NOREPLACE)`, SDL message boxes, RT64's nativefiledialog
  for the ROM picker. Duplicates ~100 lines of platform-neutral logic
  (argument parsing, save import); keep in sync with upstream fixes there.
  Also: first launch uses `baserom.us.z64` (or the only N64 ROM) beside the
  executable before opening the picker.
- `tools/baseline/main.cpp`: `windows.h` guarded; `flock` ProfileLock for the
  engineering build. `devices.cpp`: no `objbase.h`, X11 macro `#undef`s after
  `SDL_syswm.h`, SDL window handle, `app->setup(0)`, and the window is created
  with `SDL_WINDOW_VULKAN` (without it real SDL2 refuses
  `SDL_Vulkan_CreateSurface`, and RADV segfaults in `VulkanSwapChain::resize`). `minimum_host.cpp`:
  `localtime_r`/`gmtime_r`.

Steam Deck features:
- **Name: Banjo-Tooie: Steam Deck Port.** The Linux player's window title,
  dialogs, product string and executable (`BanjoTooieSteamDeckPort`) use it;
  the profile is `$XDG_DATA_HOME/BanjoTooieSteamDeckPort`. On normal play
  `rename_profile` (`player_startup_posix.cpp`, called from `main.cpp`
  before the Alpha 4 `migrate_profile`) moves a marked, unlocked
  `BanjoTooieScurvyDogPort` folder whole with `renameat2(RENAME_NOREPLACE)`,
  holding its marker lock; an existing new folder wins and the old one is
  left alone. Upstream's Windows side (`player_startup.cpp`, the `.exe` name
  in `release/player-windows-files.json`, `docs/`, `release/`) keeps the
  Scurvy Dog name, to keep upstream merges simple.

- **Deck defaults** (`TOOIE_DECK_DEFAULTS`, `minimum_host.cpp`): only for
  settings a profile has not saved: 3x resolution, output rate Display,
  widescreen On at 16:10, fullscreen.
- **Auto resolution:** resolution value 0 uses RT64 `WindowIntegerScale`
  (`minimum_presentation.cpp`, `minimum_menu.cpp`, `minimum_host.cpp`).
- **16:10 gameplay aspect** (native aspect index 4, field widened 2 → 3 bits):
  host in `src/widescreen.*`, `minimum_presentation.cpp`, `minimum_menu.cpp`;
  guest in `patches/game/native_aspect_projection.c` (scale
  `0x1.ccccccccccccep-1` = (16/10)/(16/9)) and `widescreen_settings_abi.h`.
  Literal checks updated in `tools/render_wrapper_validator.py` (4 literals;
  linked section is value-sorted, objects keep emission order),
  `tools/baseline/prepare_minimum_host.py`, `patches/game/heap.ld`,
  `tools/baseline/guest/camera-candidate/render_bridge.ld` (24 → 32 bytes).
- **Re-pinned guest image** `9ccd8db3…99a3a`, 297768 bytes, layout
  `c876981b…09549` in `prepare_player_guest.py`, `prepare_backpack621.py`,
  `cmake/Baseline.cmake`, `release/player-windows-files.json`. Reviewed: of 92
  recompiled guest functions only `func_80015D14` (projection) differs once
  addresses are masked. This build is no longer upstream's verified image.
- **Controller DB:** `minimum_host.cpp` loads `recompcontrollerdb.txt` from
  beside the executable but only adds mappings for GUIDs SDL/Steam have not
  already mapped (Steam's `SDL_GAMECONTROLLERCONFIG` wins). Upstream's player
  never loaded this file.
- **XBLA button layout** as the Deck default (`src/frontend_input_preference.cpp`
  `defaults()`, `TOOIE_DECK_DEFAULTS`), checked against the XBLA manual
  table: A=A, X and B=N64 B, LT/RT=Z, LB/RB=R, Y=C-up, right stick X=C-left/
  right (inverted at the owner's request), right stick up or down and RS click=C-down, D-pad=C-buttons (no N64 D-pad: Tooie reads none),
  LS click=L, View=menu. The camera is stepped C-buttons, not XBLA's analog
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

- **Rumble** (Controls menu, `rumble_strength` 0-100, default 100). The
  player's `osPfsInit` adapter (`tools/baseline/adapters.cpp`) still leaves
  memory paks absent, but returns `PFS_ERR_DEVICE` for channel 0, which is
  what `func_800184E8` takes as a Rumble Pak: it then calls `osMotorInit` and
  marks the motor present (`0x8007B2D0` + channel x `0x84`). Start/stop
  (`func_80017FC0` / `func_80018028`) reach librecomp's `__osMotorAccess`
  and ultramodern's `set_rumble`, which the host's input callbacks
  (`devices.cpp`) now route to the shared mapper's `set_rumble`. The mapper
  applies it to the selected controller with `SDL_GameControllerRumble`
  (refreshed every 500 ms) at the configured strength.

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

- **Draw distance and Disable LOD** (Video menu, `tooie_draw_distance` 1-12,
  `tooie_disable_lod`; `src/draw_distance.cpp`, raw hooks
  `GRAPHICS_HOOKS` in `tools/baseline/minimum_host_hooks.py`), after
  Lighthouse's `GraphicsPatches.cpp`. `func_800DE498` (model render) culls a
  model once its focus distance (sp+0xDC) reaches the draw distance at
  `0x8012C82C` (set per actor by `func_800DF428` from `func_80101970`, reset
  after every model by `func_800DE318`). A hook at `L_800DE68C`, reached once
  per model, multiplies it by the setting (+0.1 past whole multiples, as
  Lighthouse, which stops at 6x; this player goes to 12x). With LOD disabled, a hook before `0x800DE628` skips the
  small-and-far shrink (4000 units, size x 50), and the three LOD geo
  commands (type 8 in the geo tables listed at `0x80123198`:
  `func_800DD214`, `func_800DD2E8`, `func_800DD37C`, which draw their branch
  for squared distance in (min^2, max^2]) see distance 1, picking the
  nearest branch. Not covered: actors the game has not spawned or keeps
  inactive far away, and level geometry culled by other means.
  `baseline_input_host_test` (not built by default) would also need
  `src/draw_distance.cpp`.

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

- **Twin-stick Breegull Blaster** (analog camera on). The right stick turns
  and looks like the orbit: the same shaped stick, the game's frame delta,
  Invert camera X/Y (up looks up, the opposite of the orbit, where up raises
  the camera; the owner tried up-looks-down and preferred this),
  turn 200 deg/s and look 160 deg/s (the owner's tuning).
  The left stick's X strafes instead of turning. The Blaster (`bsfirstp`)
  uses neither `bafpctrl` nor `baeggaim`: `func_80804848_bsfirstp` feeds its
  inputs through `func_808045FC` (+0x164 turn from stick X, +0x180 look,
  +0x148 forward, +0x12C strafe from the C-left/C-right axis at +0x11C, -1
  left), and turning ramps up (`func_80809AF8`), so adding the right stick
  there (the first attempt) felt non-linear. The look angle (+0x20C of the
  state at player +0x188, clamped to +0x1EC..+0x1E8) eases back to a target:
  in control type 1 (+0xC) one set by the left stick's position
  (`func_80806928`, 1/12 per frame), in type 0 the level angle +0x25C while
  +0x254 is set (`func_808069BC`, 1/6 per frame). Raw hooks:
  - `func_80809AF8` before `0x80809BB8`: right-stick turn added to the
    frame's yaw delta (sp+0x28).
  - `func_8080682C` before `0x80806890`: right-stick look added to +0x20C
    before the clamp, and accumulated (clamped, reset after 0.5 s away).
  - `func_80806928` before `0x80806998` (f2) and `func_808069BC` before
    `0x808069D8` (f0): that accumulated look is added to the target, so the
    view stays where the right stick left it.
  - `func_80804848` before `0x80804938` / `L_8080499C`: `bastick_getX` added
    to the strafe input (a1); before `0x80804960` / `0x808049C4`: the turn
    input (a1) zeroed.
  Hook edits need a `CLEAN=1` build.

- **No on-screen keyboard at launch** (`tools/baseline/devices.cpp`, before
  `SDL_Init`): sdl2-compat before `30ee26c` (2026-06-07) starts text input
  on each new window without suppressing the screen keyboard, and SDL3 shows
  Steam's keyboard when no physical keyboard is attached, so it opened on
  every Deck launch. `SDL_ENABLE_SCREEN_KEYBOARD=0` at default priority (the
  environment variable still wins); text input events and Steam+X still work.

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

- **Menu maps at the original rate** (`tools/baseline/devices.cpp`,
  `update_screen`): untagged menu models (startup logos, save-select picture
  frames) jitter under RT64's automatic interpolation (`G_EX_ID_AUTO`). The
  renderer reads the current map (u16 at `0x80132DC2`, returned by
  `func_800EA05C`) each frame and presents maps `0x18B` (startup intro) and
  `0x158` (game select) at Original while Display/Manual is selected; the
  saved preference is untouched and gameplay keeps the chosen rate. Map
  changes are logged as `baseline map=0x... menu_rate_original=...`. A real
  fix would tag those draws with matrix groups in the guest.

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
- Not verified: real gamepad input with the XBLA layout, battery and
  90 Hz pacing, extended gameplay.

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

- Conflicts are most likely in the pin files above and `cmake/Baseline.cmake`.
  If upstream changes guest code, its new image hash replaces ours: re-apply
  the 16:10 edit, build with `CLEAN=1`, review the guest diff (mask addresses
  in `boot.c`, compare per function) and re-pin. Never re-pin without review.
- If upstream changes `windows-player` preset options, `build.sh` follows
  automatically; new Windows-only CMake code may need another `if(WIN32)`.
