# Banjo-Tooie: Steam Deck Port: instructions for coding agents

This is the Linux / Steam Deck fork of Banjo-Tooie: Scurvy Dog Port (a recompiled
N64 game with the RT64 renderer). Work happens on branch `linux`; `upstream` is the
Windows-only original, `origin` is the fork.

## Read first

- `linux/PORT_NOTES.md`: handoff notes. Build, every Linux/Deck change and why,
  the RT64 pin, what is verified, how to merge upstream. Update it when you add or
  change a feature.
- `linux/INTERPOLATION.md`: anything that looks wrong only at Display/Manual output
  rate (and fine at Original). Lists the cases already solved and the trace.
- `docs/ARCHITECTURE.md`: upstream's architecture.

## The player is the baseline build

The shipped player is the `TOOIE_BASELINE` build (`tools/baseline/`, host code in
`tools/baseline/*.cpp`, guest code in `tools/baseline/guest/`). `TOOIE_NATIVE_HOST`
and `src/native_host_devices.cpp` are upstream's retired frontend; do not infer what
the player does from them. Shared code in `src/` is only in the player when
`cmake/Baseline.cmake` adds it.

## Build

```
docker run --rm -v "$(pwd)":/workspace tooie-toolchain:latest linux/build.sh
```

- Incremental by default. Use `-e CLEAN=1` after changing raw hooks
  (`tools/baseline/minimum_host_hooks.py`) or guest code; guest preparation refuses
  existing outputs. A clean build takes about 15 minutes.
- Output: `build/linux-player/baseline/player-bin/BanjoTooieSteamDeckPort`
  (`TARGET=deck` by default). Docker writes as root.
- The build needs the owner's ROM (`baserom.us.z64`, never committed) and its
  output contains game code: never build on public CI.

## Pins and patches

Almost every input is hash-pinned, and a mismatch stops the build on purpose.

- Dependencies: `dependencies.lock.json`; never edit `deps/` checkouts.
- RT64 changes are recipes in `patches/rt64/*.delta.json` (exact spans and hashes,
  materialized by `tools/prepare_rt64_matching.py`, which also pins the RT64
  commit). To change one: materialize it, edit the output, regenerate the
  operations against the pinned upstream file, bump `delta_revision`, check it
  materializes byte for byte, and describe it in `patches/rt64/PROVENANCE.md`. The
  player selects recipes in `cmake/Baseline.cmake` (generic fixes and
  `minimum_host_*`).
- Guest image pins live in `prepare_player_guest.py` and its siblings. Prefer raw
  hooks (C text injected before a guest instruction that calls native code;
  `minimum_host_hooks.py`) over guest patches: they leave the image and its pins
  unchanged. Read the decomp assembly (`deps/banjo-tooie/asm/`) to pick hook
  addresses, and check the hook landed in the generated C
  (`build/linux-player/baseline/render-bridge-camera-candidate/composition-v2/raw/`).

## Checks

- `python3 tools/check_repository.py` must pass (no ROMs, game output, logs or
  unlisted `docs/` files tracked; Python and JSON must parse).
- Keep the Windows build unchanged where possible (`if(WIN32)` / `#ifdef _WIN32`),
  and keep upstream's Windows naming, to keep upstream merges easy.

## Testing

- You can run the player on the dev machine: a copy of the binary with a scratch
  `--profile-dir` (marker file `.encore-profile` containing
  `Banjo-Tooie: Encore profile v1`, plus a flat `tooie_imgui.json` with
  `"baseline_fullscreen": 0`, since Deck builds default to fullscreen),
  `--rom baserom.us.z64 --seconds N`, and `--input-script FILE` for scripted N64
  input (format in `tools/baseline/input.hpp`). The log is `<profile>/player.log`.
  Never use the owner's real profile.
- Screenshots and recordings: run with `SDL_VIDEODRIVER=x11` and capture only the
  game window (`xdotool search --name "Banjo-Tooie: Steam Deck Port"`,
  `import -window ID`, `ffmpeg -f x11grab -window_id ID`). Do not capture the full
  desktop; it shows the owner's other monitors and apps.
- `linux/frame_motion.py` compares recordings; reference numbers are in
  `linux/INTERPOLATION.md`. Scripted runs drift with loading times, so check a
  screenshot before trusting a comparison.
- The owner tests on the Steam Deck. For anything you cannot reach locally, ask
  for a slow-motion phone video or F4 issue markers (evidence reports next to
  `player.log`, see `linux/INTERPOLATION.md`) rather than guessing; say what is
  verified and what is not.

## Commits

- Commit only when the owner asks. Commits use the GitHub noreply address already
  set in this repository's git config.
- Describe what changed and why, and record Deck confirmation in the message and in
  `linux/PORT_NOTES.md` once the owner gives it.
