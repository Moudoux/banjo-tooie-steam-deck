# Selected baseline RT64 repairs

The baseline player selects five core repair recipes through
`cmake/Baseline.cmake` under `TOOIE_BASELINE_RT64_GENERIC_FIXES`. They target
RT64 commit `6f1c2d99a4ea571c139f449c326fd176ba8f3496`, licensed under the MIT
License by the RT64 contributors. Each recipe records its upstream input,
authored spans and materialized output hashes; the dependency checkout is not
edited. The selected materialized sources live under the build directory's
`baseline/rt64_generic/`.

| Recipe | Selected consumer and purpose |
| --- | --- |
| `owner_index_guard` revision 2 | `src/hle/rt64_game_frame.cpp`, scene/workload mapping and `matchScene`: keep workload-local indices with their owning workload and reuse only compatible projection anchors. |
| `transform_upload_guard` revision 1 | `src/render/rt64_transform_processor.cpp`, `TransformProcessor::upload`: select interpolated arrays only when the current workload has a mapped predecessor, matching `process`. |
| `rotation_interpolation` revision 2 | `src/common/rt64_math.cpp`, quaternion interpolation: retain the caller's endpoint convention and supply the two independent spherical coefficients omitted by the pinned helper. |
| `pop_matrix_group_projection` revision 1 | `src/gbi/rt64_gbi_extended.cpp`, `GBI_EXTENDED::popMatrixGroupV1`: read the public projection selector from command word 1, matching the public encoding. |
| `projection_residual` revision 1 | `src/render/rt64_projection_processor.cpp`, `ProjectionProcessor::process`: preserve source combined matrices and mapped endpoint decomposition residuals. |

This selection does not include `game_frame_matching.delta.json` or the complete
`projection_preservation.delta.json` product bundle. The product-overlay history
below remains provenance for other configurations, not current baseline policy.
Recipe selection and source verification do not establish a live workload hit,
GPU delivery or visual acceptance.

## Presentation recipes

Two additional recipes target the same RT64 pin and record exact upstream,
authored-span and output hashes:

- `present_trace` revision 2 instruments `src/hle/rt64_present_queue.cpp`
  with bounded, opt-in presentation-stage evidence. Selection is controlled by
  `TOOIE_BASELINE_PRESENT_TRACE`; the trace is not scanout or visual proof.
- `vulkan_present_id` revision 1 materializes
  `src/contrib/plume/plume_vulkan.cpp`. It resets the present ID after a
  successful swap-chain replacement. On Windows, the separately selected
  `TOOIE_BASELINE_RT64_VULKAN_ACQUIRE_PACING` option uses acquire back-pressure
  instead of the present-wait throttle. The option defaults off; the Alpha 4.4
  Windows package explicitly selects it. This is a platform-specific pacing
  adaptation, not a game simulation-rate change.

The Alpha 4.4 package allowlist records both selected options. Local player
feedback supports smooth output; this is not a guarantee for every GPU,
driver or display configuration.

## Selected projection anchors

The owner-index guard admits scene candidates only with the mapped predecessor
of the current first workload. Inside `matchScene`, workload-local projection
indices remain restricted to their owning first workload. IGNORE projections do
not inherit a mapping. The first reusable projection anchor must have the same
effective projection key: equal ID, and, for explicit IDs, equal decomposition
and position/rotation/scale/skew/perspective component policies. AUTO retains its
AUTO-compatible behavior. Explicit current-to-previous mapping still requires
equal IDs.

Thus a WORLD camera and a later IGNORE or differently identified projection in
one scene do not share the WORLD camera's mapping merely because it was first.
This guard does not establish semantic scene assignment: candidate ranking is
still nearest-matrix and greedy. A crossed candidate can consume a rightful
predecessor before a later explicit-ID check refuses mapping. This is a source
limitation, not evidence that a reported wall-camera event reaches it.

## Selected transform upload

`TransformProcessor::process` clears its interpolation/previous arrays and fills
them only when a previous frame exists and this workload is mapped. The upload
guard uses that same predicate. With a previous frame but an unmapped current
workload, upload must select the N original matrices, rather than an empty
interpolation array with an N-element upload range. `BufferUploader::Upload`
checks a nonnull source and nonempty range; it does not check the source vector's
logical length. This repairs a conditional producer/consumer disagreement, not
an inevitable crash or an established Tooie artifact cause.

## Selected projection-pop decoder exception

The pinned public `gEXPopMatrixGroup` encoding puts count in command word 1
bits 0..7 and the boolean projection selector in word 1 bit 8. The upstream
decoder reads count correctly but reads the selector from word 0 bit 8. The
one-line recipe corrects only that selector read; public-command correctness
belongs in RT64 rather than game-owned identity policy.

Literal search of the authored emitters finds one projection-selector pop,
and the selected player depends on this repair for it. The BK menu-text
bracket (`tools/baseline/menu_text.hpp`) pushes a PROJECTION matrix group in
`open()` and pops it in `close()` with `0x6400000D` / `1 | (1 << 8)`: count
1, projection selector in word 1 bit 8. Opcode `0x0D` leaves word 0 bit 8
clear, so the unrepaired decoder would pop the MODELVIEW group stack and
leave the projection group pushed. The recipe is therefore **required**
while that emitter ships; it is not dormant and not a retirement candidate.
`cmake/Baseline.cmake` refuses a BK pipeline configuration without it
(`tooie-pop-projection-guard`). This is a source and selection fact: the
menu-text path is linked into the selected player, but its runtime behaviour
is not observed here. An upstream offer of the one-line fix stays optional.

# Historical product-overlay provenance

`game_frame_matching.delta.json` applies only to RT64 commit
`d9817c68ffd7328cd0af383af124971489366e18`, licensed under the MIT License
by the RT64 contributors. The recipe records the normalized upstream file and
span SHA-256 values and stores only the project-authored replacement span.

The overlay independently deduplicates transform spans, tile spans, and
resolved look-at indices inside each RT64 match hash bucket. It preserves the
existing ordered numeric pair sets and all downstream matching code. This is a
performance candidate pending measurement in the Terrydactyland reproduction;
the synthetic test demonstrates set-union equivalence and synthetic work
reduction, not game-frame-rate improvement.

Revision 5 additionally prevents first-workload projection, transform, tile
and look-at matching from using indices originating in another workload. It
rejects scene pairs whose first current workload lacks a sized previous map or
whose previous-workload index disagrees with the scene candidate before scene
distance ranking, so invalid pairs cannot consume scene slots. The inner
`matchScene` guard is retained. For
automatically identified transforms whose vertices are not interpolated, it
admits a previous transform
only when both nonempty local position sequences have equal length and hash.
Those cached geometry hashes are identity guards against pairing different
animated model parts that share a draw-call hash; dynamic local geometry falls
back to the current transform. The player-facing deformation report has not
yet been verified against this candidate.

`transform_upload_guard.delta.json` targets the same immutable pin and its
`src/render/rt64_transform_processor.cpp`. It makes upload select interpolated
world-transform buffers only for workloads that `process` mapped to a previous
workload. Unmapped workloads use their original transform buffer; otherwise
their cleared, empty interpolation buffers could be selected. This does not
establish a causal link to the reported character deformation.

## Projection preservation

`projection_preservation.delta.json` targets the same immutable RT64 pin and
MIT-licensed source, `src/render/rt64_projection_processor.cpp`. Its input and
replacement spans are hash checked by the existing materializer. The dependency
checkout remains unchanged.

When no previous projection is mapped and no debugger camera is applied, retain
the original combined view-projection matrix; apply any aspect adjustment directly
to its X column. Reconstructing a fixed-point combined matrix through its heuristic
view/projection decomposition can change depth enough to clip distant geometry.
The later mapped-projection correction preserves each source combined matrix's
decomposition residual through RT64's interpolation weights when camera and
projection interpolation are enabled. When RT64 skips projection interpolation,
it uses the current source residual for both composed matrices. This keeps the
existing interpolated view/projection motion while restoring the source
combined matrix at the endpoints. The debugger camera path remains pinned.
This addresses a demonstrated numeric gap in the mapped path; the separately
reported high-rate character deformation still needs a paired gameplay
observation before assigning its cause.

The CPU-only regression exercises the real projection processor with observed
numeric matrices, including aspect changes and the unaffected paths. This proves
matrix/depth preservation, not visual acceptance of the Jinjo Village artifact.

## Player projection residual preservation

`projection_residual.delta.json` revision 1 selected only operations 4 and 5 of
`projection_preservation.delta.json` revision 5 for the baseline player renderer.
It retains the same immutable RT64 pin and MIT-licensed input, with no auxiliary
camera fallback or IGNORE-group matching policy. Revision 2 adds
`projectionShapeJumped`: a mapped projection whose horizontal or vertical scale
changes by more than 15% between game frames (Tooie's cinematic letterbox
changes the vertical scale by a third) is not interpolated, at both places that
decide projection interpolation, so the image is not squashed for the
interpolated frames after the change. Its 231-line output is SHA256
`9df4f7ddff6983474a4c5ac222e3cdeac6cb05fee394b9bca851cad52788ece1`.

The unmapped path preserves the source combined matrix, scaling only its X
column for aspect adjustment. The mapped path preserves the decomposition
residual at processed endpoints within floating-point rounding while retaining
RT64's interpolation flags and weights. Intermediate residuals use the existing
product recipe's linear correction; this is not a visual acceptance claim.
The CPU matrix regression can prove source-matrix and clip-depth preservation,
not that Greg's Jinjo Village scene or other reported artifacts are resolved.
The full product recipe remains unchanged.

## Object groups observer

`object_groups.delta.json` targets the same pin and MIT-licensed
`src/hle/rt64_interpreter.cpp`. It adds `RT64::tooieCommandObserver`, null by
default, and calls it with each HLE display-list command's RDRAM address before
the command runs. The player sets it only while "Interpolate all objects" is
on (`src/object_interpolation.cpp`); with it null, interpretation is unchanged.
