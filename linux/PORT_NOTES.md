# Linux / Steam Deck port: handoff notes

State of the Linux fork of Banjo-Tooie: Scurvy Dog Port, for whoever picks it
up next. Upstream builds only for Windows; this fork builds a native Linux
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
- Output: `build/linux-player/baseline/player-bin/BanjoTooieScurvyDogPort`
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
  `player_startup.cpp`. Profile in `$XDG_DATA_HOME/BanjoTooieScurvyDogPort`
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
  right (inverted at the owner's request), right stick up or down and RS click=C-down,
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
  (`func_802BE190`, gain 20); this orbit places it directly. A second raw hook before
  `0x8011066C` in `func_80110588` (`tooie_camera_orbit_apply`) writes the pose
  into the view copies (sp+0x34 position, sp+0x28 rotation) and the camera's
  own state (+0x74, +0x80, yaw controller +0x10/+0x14, velocity +0x18 = 0), so
  the game resumes from it. Tooie conventions: rotation (pitch, yaw, roll) in
  degrees, yaw = atan2(dx, dz) of position minus focus (`func_80112550`),
  pitch = 360 - elevation. Steps are refused when `func_8011349C(from, to)`
  reports the path between the two camera positions blocked; a blocked
  combined step retries yaw alone, then pitch alone. The orbit ends on a game yaw transition (C buttons), a
  camera mode change, or a frame without manual rotation (cutscene, menu,
  view toggle). The camera component is relocated by the heap, so its
  address changing in the same mode keeps the orbit. Measured with a virtual
  pad, before the libultraship shaping: constant 84.7 deg/s from the first
  frame at 0.53 shaped stick, rotation equal to yaw, zero motion after release, kept while walking. Unlike
  Lighthouse, no collision pull-in while walking during an orbit.

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
