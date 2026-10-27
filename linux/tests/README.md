# Linux / Steam Deck test suite

Runs the player on the dev machine and checks this fork's features and fixes
(`linux/PORT_NOTES.md`, "Source changes"). For a fix, the test reproduces what
originally showed the bug and checks it no longer does; `--list` shows each
test's feature and trigger.

```
python3 linux/tests/run.py                 # static + quick (about 5 minutes)
python3 linux/tests/run.py --tier full     # everything, after a rebase (about 15 minutes)
python3 linux/tests/run.py -k minimap      # tests whose name contains "minimap"
python3 linux/tests/run.py --list --tier full
```

Build first (`linux/build.sh`); `--binary` picks another build. Results, player
logs, F4 reports and screenshots go to `local/tests/runs/<time>/`
(`results.json` plus one folder per test). The exit code is non-zero when a test
fails.

## Tiers

- **static**: the tree and the build output. Repository check, guest image pin,
  every fork raw hook in the generated C, the Linux RT64 recipes built byte for
  byte, the product name.
- **quick**: launches with a fresh profile; no save needed. Startup, profile,
  ROM and controller database, the input layout, intro and cutscene skips, video
  settings, 16:10, and a Spiral Mountain session (minimap, F4 reports, analog
  camera, levitate, mouse aim, frustum padding).
- **full**: also long runs and everything that needs the owner's save: GPU work
  in Jinjo Village (bounding-box culling), camera scene pairing, draw distance,
  the Hailfire Peaks ice cave, the pause backdrop, the pause-menu warp, the heap
  in Witchyworld, Atlantis (underwater aspect, mouse aim while swimming),
  autosave, game select interpolation and the 10-launch startup crash loop.

## Fixtures

Game data is never tracked. Tests that need it skip with the missing path:

- `local/tests/fixtures/owner-save.bin`: the owner's save (`saves/bt.n64.us.1.0.bin`,
  Jinjo Village; Mayahem Temple pad 1, Glitter Gulch Mine and Witchyworld pads
  activated).
- `local/tests/fixtures/hfp-icecave-rdram.bin`: the RDRAM snapshot of the Deck F4
  report at the Hailfire Peaks ice cave (`local/deck-perf/hfp/deck3/`).

`baserom.us.z64` must be in the repository root, as for the build.

## How it drives the player

The harness (`harness/player.py`) starts the player as a child process with a
scratch profile and these settings, and:

- **Test control** (`src/test_control.cpp`, Linux builds only): with
  `TOOIE_TEST_CONTROL=<fifo>` the player reads commands from that FIFO on its SDL
  thread: a virtual Xbox pad (so the real binding, XBLA layout and stick shaping
  are exercised), key events (F4, F5), mouse motion, a held camera pose (applied
  on the game thread in the analog camera's orbit hook, so nothing races the
  game's own camera), hint queries and quit. It
  stands in for window focus, so tests run in the background without taking the
  keyboard, mouse or focus, and the pointer is never captured. With the variable
  set but empty, errors only go to the log (no dialog on the desktop). Without
  it, nothing changes.
- **Guest RAM** through `/proc/PID/mem` (`harness/guest.py`): map, game state,
  player position, camera, controller state, and warps (the pending map
  transition, as `func_800A791C` writes it).
- **The window** (`SDL_VIDEODRIVER=x11`): screenshots and recordings of the game
  window only, never the desktop, and `xdotool windowsize` for 1280x800.
- **GPU work**: `vulkan/stats_layer.c`, a Vulkan layer counting draws, vertices
  and render passes per second, built on first use into `local/tests/build/`.
- **F4 reports** (`harness/evidence.py`): the projection rows behind the camera
  and underwater checks, and the RDRAM snapshot.

## Adding a test

Put it in `cases/` with `@test(tier, feature, trigger)`; for a fix, state the
trigger and reproduce it. A session that checks several features in one launch
uses `with ctx.part(name, feature):` for each, so each is reported on its own.
Prefer exact signals (guest RAM, log lines, F4 rows, GPU counts) over pixels; when
a check uses pixels or thresholds, validate that it fails on the bug (a synthetic
copy, or an older build) and record the measured numbers in the test.

## Not covered (Deck checks remain)

Real Deck hardware and Steam Input (gyro, battery, 90 Hz pacing, real pad
layouts), color precision (applies at the next launch), supersampling, the
"Saving..." indicator, minimap markers and area names, mouse aim in flight, the
submarine, the Saucer of Peril and the Code Chamber, the analog camera's wall
and flight handling, pause-warp greyed rows and longer lists, the heap's
low-memory cleanup (the bigger pool avoids it now), and the interpolation cases
in `linux/INTERPOLATION.md` other than game select.
