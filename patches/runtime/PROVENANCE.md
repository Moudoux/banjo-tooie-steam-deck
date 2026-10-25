# Runtime source provenance

The files here are project changes to the pinned N64ModernRuntime (GPL-3.0).
Banjo-Kazooie uses its pinned runtime unmodified; each change here is a
Tooie-specific difference.

## Selected composer

The player configure (`TOOIE_BASELINE_TERMINAL_FENCE`) runs
`tools/prepare_runtime_continuous.py --baseline-selected-runtime`, which calls
`materialize` in `tools/prepare_runtime_baseline_selected.py`. It writes the
children under `build/<preset>/baseline/` and
`baseline/selected-runtime/manifest.json` (actual input/output paths and hashes).
Configure fails if the composer fails.

The composer checks, before writing anything:

- the runtime revision `ca568b6ad79b9029d14077f0c3ffa757727c5559`;
- the eight original sources in `INPUTS`: ultramodern `events`, `threads`,
  `threadqueue`, `mesgqueue`, `timer`, and librecomp `dp`, `recomp`, `pi`;
- every patch in `PATCHES` and `dp-freeze.hpp`
  (`32cad60bc4a471733db1be985ebbe0eb6ac3c0f44a382cadf056e5f8dce31639`);
- the recipe modules in `RECIPES`: `owned_timer.py`,
  `prepare_runtime_continuous.py` and `prepare_runtime_threadqueue.py`;
- the two recipe-owned patches in that module's `RECIPE_PATCHES` (below);
- each of the 13 children in `EXPECTED`.

Patches are applied by one strict applier, `apply_patch` in
`prepare_runtime_continuous.py`: exact `--- a/<file>`/`+++ b/<file>`
headers, the declared hunk count, `,N` line counts on both sides, and each hunk's
full before-text occurring exactly once. Hunk start line numbers are informational;
there is no fuzz or offset search. Hunk context need not be three lines.

## Tracked patches

| Patch | Target | Hunks | SHA-256 |
|---|---|---|---|
| `events-q-vi-lock` | `events.cpp` | 3 | in `PATCHES` |
| `events-next-framebuffer` | `events.cpp` | 1 | `6e17ed72a30d942af19ed4242c9e8a3f8f947bad9d709cc264498b1084b5ab27` |
| `events-dp-freeze` | `events.cpp` | 3 | `7f287aec255a5dcb9eaa2ae53082403ff9a4eaa67c22b0f56d65e4f5d5864cdf` |
| `events-sp-exception` | `events.cpp` | 2 | `73511703d287ce3fcf76eecafe16d97c8806a0dc448dca7f86529627dde2614f` |
| `events-guest-clock` | `events.cpp` | 3 | `594a6c424ac803ed54feb31c4c53e7fc5db8d4db97434324a8a552874fb4bd22` |
| `timer-guest-clock` | `timer.cpp` | 8 | in `PATCHES` |
| `threads-suspended-stop` | `ultramodern/src/threads.cpp` | 2 | `9438dd3c67452b673f4e5bfe494b2c54fa88574e6a11c73f0758a7df64401322` |
| `producer-quiescent-cleanup` | `ultramodern/src/threads.cpp` | 1 | `9b4261af055d9677c061065c1d7588e79fcec74f679dac6cf12d3c069e2319fb` |
| `threadqueue-removal` | `ultramodern/src/threadqueue.cpp` | 2 | `713659775b22eeac5c1e6f23322af35f68d66eec580819e6c0e41ee5a67b56da` |
| `dp-freeze` | `dp.cpp` | 2 | `4006e77ef9591ac7584e84ad348eb237b09799b56962b0ed70d45100d53cfa23` |
| `recomp-terminal-fence` | `recomp.cpp` | 4 | `eb157ab9876972d73b610edc5280cfd6475930591d119b3eb3877429cd12aae2` |
| `mesgqueue-required-admission` | `mesgqueue.cpp` | 4 | `912b8c1c6cd7e658ab63bcfbb6d4320c536c9104008e5512633abdd0919bf644` |
| `mesgqueue-lifecycle-wait` | `mesgqueue.cpp` | 3 | in `RECIPE_PATCHES` |
| `pi-save-persistence` | `pi.cpp` | 10 | in `RECIPE_PATCHES` |

`events-q-vi-lock` adds the Q-VI-LOCK `message_mutex` scope around the dummy-VI
seed and `update_vi()`, released before VI/AI delivery and callbacks, and the lock
in `osViGetCurrentFramebuffer`. `timer-guest-clock` is the F6 guest-clock map,
`ClockRateChanged` wake, engaged-only 2 ms worker slice, `guest_rate_changed`
(installed in `init_timers`, then applied once for any earlier request) and
notifier removal in `stop_and_join_timer`. It applies to the copied 1x timer
described below, not to the original file.

`events-sp-exception` wraps the SP worker's `rsp::run_task` call in a task-local
`try`. A thrown failure, or a `false` result (unsupported task) converted to
`std::runtime_error("Unsupported RSP task")` inside that `try`, goes to
`tooie::lifecycle::fail` and the worker returns without `sp_complete`. The
upstream worker quick-exits on a `false` result; BK's pinned runtime does the
same. The terminal fence, save drain and error presentation are unchanged.

`mesgqueue-lifecycle-wait` and `pi-save-persistence` are the former Python text of
`messages(text)` and `saving(text)`, moved unchanged. Each function has one
behaviour over one pinned original, so every caller (selected composer,
`--baseline-external-wait`, old-engine full mode) applies the same patch bytes.
Their pins live in `RECIPE_PATCHES` inside `prepare_runtime_continuous.py`, which
the composer pins in `RECIPES`; the composer also records both files for its
pre-publication input recheck. The unchanged `EXPECTED` hashes of
`lifecycle/mesgqueue.cpp` and `runtime-continuous/pi.cpp` are the byte-equivalence
check.

- `mesgqueue-lifecycle-wait`: the bounded external-message wait with lifecycle
  polling, plus an inactive `TOOIE_PERSISTENT_RUNTIME_EXPERIMENT` export/import
  block.
- `pi-save-persistence`: the save root, temporary-file promotion, generation
  tracking and final drain.

## Edits still made by Python recipes

These selected children still contain C++ text written by Python functions in
`tools/prepare_runtime_continuous.py` (pinned in `RECIPES`):

- `threads(text, True)`: lifecycle include, scheduler polling, the continuous
  worker entry, creation/launch/cleanup ownership and drain. It also writes inactive
  `#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT` blocks into the selected child.
- `baseline_timer(text)`: the steady-clock `static_assert`, `ContinuousTimerStop`,
  removal of the worker `detach()` and `stop_and_join_timer`. It then calls
  `owned_timer.ordinary`, which writes the owned timer worker, setters and request
  lock.

`tools/prepare_runtime_threadqueue.py::normalize_lf` normalizes line endings only.
These two stay Python: `threads` has a second, unpinned behaviour (`threads(text,
False)` in the old-engine full mode adds the `osStopThread` branch), and
`baseline_timer` ends in `owned_timer.ordinary`, which the unpinned old-engine
`timer` variant also calls. Two representations of one edit would drift.

## Children

| Child | Derivation | Used by |
|---|---|---|
| `scheduler-event/events.cpp` | original + `events-q-vi-lock` | configure check only (parent of the next) |
| `first-frame-sync/events.cpp` | + `events-next-framebuffer` | configure check only |
| `dp-freeze/events.cpp` | + `events-dp-freeze` | configure check; SP-task parent-negative fixture; non-fence arm |
| `lifecycle/events.cpp` | + `events-sp-exception` | configure check; non-player terminal-fence arm; SP-task fixture (`selected` pin) |
| `guest-clock/events.cpp` | + `events-guest-clock` | **player**; SP-task fixture in player builds (`player` pin) |
| `guest-clock/timer.cpp` | `baseline_timer` + `ordinary` + `timer-guest-clock` | **player** |
| `thread-refusals/threads.cpp` | original + `threads-suspended-stop` | non-fence arm; lifecycle fixture input |
| `lifecycle/threads.cpp` | + `producer-quiescent-cleanup`, then `threads(…, True)` | **player**; wait/save fixture |
| `controller-cycle/threadqueue.cpp` | LF-normalized original + `threadqueue-removal` | **player** |
| `lifecycle/mesgqueue.cpp` | original + `mesgqueue-lifecycle-wait` + `mesgqueue-required-admission` | **player**; MQ01 admission fixture |
| `dp-freeze/dp.cpp` | original + `dp-freeze` | **player**; DP Freeze regression |
| `lifecycle/recomp.cpp` | original + `recomp-terminal-fence` | **player** |
| `runtime-continuous/pi.cpp` | original + `pi-save-persistence` | **player** (`TOOIE_BASELINE_SAVE_COMPOSITION`); save fixtures |

`cmake/Baseline.cmake` re-checks the selected children's hashes independently of
`EXPECTED`. The non-fence arm's `recomp-retain-rdram.patch` input and its
`timer-owned/timer.cpp` child are outside this composer.

## Historical old-engine path

The old engine (`TooieRecompiled`, outside `TOOIE_BASELINE`) uses
`cmake/RuntimeCleanupPatch.cmake`. Its `tools/prepare_runtime_cleanup.py` applies
`producer-quiescent-cleanup.patch` to the original `threads.cpp` (normalized-LF
input `e3ba32a1e06ba1643adc10ccc29ef5e3429e8809270f787a181314fc5704e653`,
output `70582b56f42f0ad6935cb9821f7582a788a12da9f3dff5aa48a67109a9501f74`) and
substitutes it into the existing ultramodern target. It records
`runtime-cleanup/threads.provenance.json` and `source-substitution.txt`.
`cmake/RuntimeContinuousPatch.cmake` then runs the full
`prepare_runtime_continuous.py` mode over that output.

That drain joins the cleaner and drains queued native contexts. It does not stop
workers or prevent later enqueues. Callers must first forbid new producers, destroy
parked workers while synchronized, and await native exit from every cleanup
producer. The normal `recomp::start` shutdown path did not establish that
precondition and was not certified by that bounded diagnostic.
