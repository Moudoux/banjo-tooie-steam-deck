# Rendering

This page describes the selected renderer in Alpha 4.5. It does not describe every historical renderer file retained in the repository, and it does not imply that every scene has been visually verified.

## What the renderer receives

The original game still builds its display lists, poses and draw order. Selected guest C patches add RT64's public matrix-group and floating-point vertex commands to that stream. RT64 matches related transforms between submitted workloads and produces intermediate presentations. The game's simulation rate and the chosen output rate remain separate.

The shared implementation is in [bk_pipeline.c](../tools/baseline/guest/silo-candidate/bk_pipeline.c), [bk_pipeline.h](../tools/baseline/guest/silo-candidate/bk_pipeline.h) and its included helpers. The `bk_` names retain implementation provenance; the code is adapted to Tooie's data layouts and drawing paths. They are not a claim of feature parity or endorsement.

## Identity belongs to the object being drawn

Actor draws use the descriptor's creation generation, with separate ranges for view, phase and attachments. The creation stamp is recorded by [render_bridge.c](../tools/baseline/guest/render_bridge.c); the shared pipeline validates the descriptor before using it. The creation patch is `func_800EBED4`; the preparation recipe verifies the silo copy against this maintained source. This prevents ordinary slot reuse from automatically inheriting the previous actor's labels. The finite generation-to-ID mapping still has a wrap limit; it is not an unlimited identity guarantee.

Player identity follows the drawn player's roster slot, rather than whichever player currently receives input. Banjo and Kazooie therefore retain separate histories when split or when control changes. Owner scopes save and restore the enclosing identity. [draw_boundaries.c](../tools/baseline/guest/silo-candidate/draw_boundaries.c) enters those scopes, while [bk_attach.inc](../tools/baseline/guest/silo-candidate/bk_attach.inc) handles Tooie's separately drawn backpack attachment. Player-slot reuse and unusual unowned draw paths remain distinct lifecycle obligations.

The shared model root opens the appropriate group, resets traversal numbering and restores the enclosing state when it closes. Geometry helpers in [bk_graph.inc](../tools/baseline/guest/silo-candidate/bk_graph.inc) and [bk_sort.inc](../tools/baseline/guest/silo-candidate/bk_sort.inc) place labels throughout supported traversal. This replaces the earlier description of the selected renderer as only a set of per-character palette owners. Retained palette and numbered route files do not, by their presence alone, establish which branch is selected. The actor palette remains active in the player presets; the legacy player boundary is limited to engineering configurations:

- the actor palette gives a projection ID to the header-bit40 actor draws it recognizes, including the save-select hand and props; it adds its own modelview group only when the draw's BK actor owner is not current (runtime acceptance pending);
- the legacy player boundary, only where an engineering configuration enables it (the player does not), adds an outer, fully skipped group around the primary player body.

Other header-bit40 draws that are neither shadow- nor menu-owned keep an IGNORE projection. These are source facts listed in [Architecture](ARCHITECTURE.md#what-is-still-not-in-that-shape); whether RT64 consumes each label in play has not been established.

## Changing geometry and animation history

A model can change which geometry branches it draws. [bk_select.inc](../tools/baseline/guest/silo-candidate/bk_select.inc) reserves stable spans for supported selector alternatives in player and projected-shadow roots, so later bones do not receive different IDs merely because an earlier branch disappears. Its preflight authenticates the expected handler table, bounds traversal and respects the per-model ID window. Unsupported graphs keep the original traversal; they do not receive an invented correspondence. Actor roots keep the original traversal numbering, as in BK.

A separate Tooie-specific branch guard compares supported player choices with the last submitted baseline. On a discontinuity it can skip interpolation for the connected body and attachments for one workload, rather than mix old and newly visible parts. This is a conservative policy with a visible trade-off: a deliberate snap can remain. It is not a universal animation fix, and it must not be removed without showing that connected parts still agree.

## Floating-point deformation

Some vertices are deformed by the CPU before rendering. [bk_float.inc](../tools/baseline/guest/silo-candidate/bk_float.inc) reproduces the relevant position calculations in floating point and supplies POSITION and, when valid, VELOCITY streams. The original integer producer and its other vertex attributes remain intact.

Velocity requires the same owner sample, the same model and the immediately preceding construction frame, with no applicable interpolation cut. The existing 1024-record table searches for the actual owner and reuses only eligible stale storage. A new owner cannot evict another current or adjacent-frame owner merely because their hash buckets collide. At true capacity exhaustion the draw can retain floating-point positions without explicit velocity. This is a finite table, not unlimited per-object history.

Nested draws restore their enclosing streams. A draw that cannot use its own float stream suspends an inherited one rather than apply it to unrelated vertices. CPU-written leaf lists temporarily suspend stale float data after their CPU callbacks have run; the suspension bracket contains command writes and address conversion, not another guest model draw. Nesting beyond the root-state capacity disables inherited float streams until the outermost overflow draw returns. These safeguards preserve the original draw; they do not prove every unsupported nesting path has equivalent interpolation.

The selected graph path forces the visible result at four sphere-test branches, alongside the existing bbox traversal policy. Original predicate calls and separate null-child guards remain intact. This stabilizes which children are traversed; model-level culling remains a separate coverage item. Source, generated-code and native binding checks passed for this change; demo and visual re-acceptance remain pending.

## Camera and shadow transforms

[camera_emitters.c](../tools/baseline/guest/camera-candidate/camera_emitters.c) and [camera_float.h](../tools/baseline/guest/camera-candidate/camera_float.h) provide separate projection and affine view transforms where the qualified path supports them, preserving the original endpoint math. [camera_pools.c](../tools/baseline/guest/camera-candidate/camera_pools.c) supplies their existing guest allocation path and capacity contract. Fixed and unsupported paths retain their qualified fallback behavior.

[bk_cut.inc](../tools/baseline/guest/silo-candidate/bk_cut.inc) associates camera discontinuities with the relevant view. A cut changes interpolation policy rather than discarding an otherwise valid object's identity. The shadow producer follows that rule too: [bk_shadow_attach.inc](../tools/baseline/guest/silo-candidate/bk_shadow_attach.inc) and [shadow_projection.c](../tools/baseline/guest/camera-candidate/shadow_projection.c) preserve identity through a skipped update so the following workload can match again. Planar shadow transforms use coefficient interpolation where decomposition would be inappropriate. Shadow appearance and coverage still have known issues.

## Submission and data lifetime

The game can reuse memory after drawing. [bk_pipeline_hooks.py](../tools/baseline/bk_pipeline_hooks.py) and [bk_pipeline_thunks.hpp](../tools/baseline/bk_pipeline_thunks.hpp) register an acknowledgement for the exact submitted display-list root and wait before allowing its source data to be reused. A construction that is not submitted does not wait or commit the branch baseline.

The runtime's parsed acknowledgement follows its display-list processing call; this is source-data custody, not a monitor-present or scanout signal. Float history advances with construction, while branch continuity commits with submission. Those clocks serve different purposes and are not interchangeable without checking the consumer behavior.

## Limits and verification

Player testing has accepted ordinary models, split-player moves, camera motion, save-select animation and widescreen zone transitions. Backpack-shadow detachment and warp-menu opening jitter remain open; a tent-slope shadow issue and the Clockwork view are deferred. Model-level culling, unsupported geometry, reflected or repeated camera production, and unusual identity reuse need their own evidence. The implementation contains bounds and refusals; it does not promise complete correspondence for every draw.

F4 records selected guest and RT64 CPU-side evidence. It can help distinguish an ownership, history or projection problem, but the records are not GPU images or proof that a frame looked correct. A successful build, a passing fixture, a measured API presentation rate and a player's visual acceptance are separate results.

## Attribution

The implementation builds on N64Recomp, N64ModernRuntime and RT64, with rendering adaptations informed by and derived from credited BanjoRecomp work. Preserve source attribution and applicable licenses when changing these files. See [third-party notices](../THIRD_PARTY_NOTICES.md) and the [license](../LICENSE). This project is independent and has no upstream endorsement.
