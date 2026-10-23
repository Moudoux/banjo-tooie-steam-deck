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
  `defaults()`, `TOOIE_DECK_DEFAULTS`): A=A, X=B, LT/RT=Z, LB/RB=R, Y=C-up,
  B=C-down, right stick X=C-left/right, right stick down=C-down, up unbound,
  LS click=L, View=menu. Known gaps: talking is on X (XBLA B is contextual),
  camera is stepped C-buttons not analog, no zoom-in.

## Verified vs not

- Verified on the dev machine (Arch, Ryzen 9 3900X = Zen 2, RTX 4070 SUPER,
  Vulkan): boots, reaches map loading, clean shutdown and saves; 16:10 output
  observed; adjacent-ROM startup; controller DB gap-filling with synthetic
  entries; repository checks.
- Steam Deck (SteamOS 3.8.28, glibc 2.41, RADV): the first build crashed on
  the first frame (fixed by the Vulkan window flag); the fixed build is not
  yet confirmed on the Deck.
- Not verified: real gamepad input with the XBLA layout, battery and
  90 Hz pacing, extended gameplay.

## Testing gotcha: SDL on the dev machine

Arch ships `sdl2-compat` (SDL2 API on SDL3), which hides bugs that real SDL2
exposes; the missing `SDL_WINDOW_VULKAN` flag above only crashed on the Deck.
Test against real SDL2 by copying `libSDL2-2.0.so.0` out of the toolchain
image and running with `LD_LIBRARY_PATH` pointing at it. On the Deck,
`coredumpctl info -1` gives a symbolized backtrace for a `SYMBOLS=1` build.

## When pulling upstream

- Conflicts are most likely in the pin files above and `cmake/Baseline.cmake`.
  If upstream changes guest code, its new image hash replaces ours: re-apply
  the 16:10 edit, build with `CLEAN=1`, review the guest diff (mask addresses
  in `boot.c`, compare per function) and re-pin. Never re-pin without review.
- If upstream changes `windows-player` preset options, `build.sh` follows
  automatically; new Windows-only CMake code may need another `if(WIN32)`.
