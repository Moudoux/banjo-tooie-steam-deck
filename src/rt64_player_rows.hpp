#pragma once
// F4 player correspondence collector over RT64 CPU draw data (implemented in rt64_matrix_trace.cpp).
#include "rt64_matrix_trace.hpp"

#include "hle/rt64_game_frame.h"
#include "hle/rt64_workload.h"

#include <array>
#include <vector>

namespace RT64 {
inline constexpr size_t tooiePlayerRowsPerPresentation = 1024;
inline constexpr size_t tooiePlayerProjectionsPerPresentation = 8;
// Supplemental roots share existing storage, after all player workloads have been collected.
inline constexpr size_t tooieOwnedRootsPerPresentation = 64;
// Exact paired menu_scope proposal namespace; no inference for untagged UI models.
inline constexpr uint32_t tooieMenuModelIdStart = 0x30000000u, tooieMenuModelIdEnd = 0x30060000u;
// Five transition views, one 256-ID window each; root IDs only.
inline constexpr uint32_t tooieTransitionIdStart = 0x30060000u, tooieTransitionIdEnd = 0x30060500u;
// Option-box text glyph groups (tools/baseline/menu_text.hpp first_id..end_id), 128 IDs per text line.
inline constexpr uint32_t tooieMenuTextIdStart = 0x30200000u, tooieMenuTextIdEnd = 0x30230000u;
bool tooieIsSupplementalRootId(uint32_t id) noexcept;

// BK player transform IDs (guest/silo-candidate/bk_pipeline.h): roster slot 0
// [TOOIE_BK_PLAYER_ID_START, TOOIE_BK_PLAYER_ID_END) and slots 1..7
// [TOOIE_BK_PLAYER_SLOT_ID_START, TOOIE_BK_PLAYER_SLOT_ID_END). The shadow domain between them is excluded.
bool tooieIsPlayerTransformId(uint32_t id) noexcept;

// L05 evidence: the legacy player boundary's outer group, idbase+129 (player_boundary.c:240,264), where
// idbase = TOOIE_BRIDGE_ID_BASE + block*512 for block < TOOIE_BRIDGE_BLOCK_COUNT (render_bridge.h).
bool tooieIsBridgePlayerOuterId(uint32_t id) noexcept;
// One workload's world transforms: all of them, those whose group cannot be resolved, and bridge-domain +129 hits.
struct TooieBridgeOuterCount { uint64_t worlds = 0, players = 0, unresolved = 0, hits = 0; uint32_t firstIndex = UINT32_MAX, firstId = 0; };
void tooieCountBridgePlayerOuter(const DrawData &data, TooieBridgeOuterCount &count) noexcept;

// Prior slot rule: a mapped workload map whose prevWorkloadIndex is inside the queue, is a member of
// prevFrame.workloads, and is neither the current slot nor carries the current workload ID.
bool tooiePlayerPriorSlotValid(const GameFrameMap::WorkloadMap *map, uint32_t currentSlot, uint64_t currentId,
    const std::vector<uint32_t> &previousWorkloads, size_t queueWorkloads, uint64_t previousId) noexcept;

// Default pass appends one row per player-labelled world transform of `current`, plus the projections those
// rows reference. `previous` is non-null only when the prior slot rule held; `previousSlot` is the
// frame map's recorded prevWorkloadIndex (UINT32_MAX when unmapped). Never allocates: the caller
// Supplemental passes: one representative per (workload, group ID) across all workloads first,
// then remaining matrices. At most64 total; representativesOnly selects the first pass. Menu UI groups (menu model,
// sprite, text) retain every matrix in the first pass; supplemental rows carry vertex_delta and VertexSkip.
// Caller must finish all player workloads first. This does not reserve space against players.
// reserves tooiePlayerRowsPerPresentation and tooiePlayerProjectionsPerPresentation.
void tooieCollectPlayerRows(uint64_t presentation, const DrawData &current, uint32_t currentSlot,
    uint64_t currentId, const GameFrameMap::WorkloadMap *map, const DrawData *previous,
    uint32_t previousSlot, uint64_t previousId, float weight, float previousWeight, uint32_t frameFlags,
    bool projectionsProcessed, bool transformsProcessed, std::vector<TooiePlayerTransform> &rows,
    std::vector<TooiePlayerProjection> &projections, bool supplementalRoots = false, bool representativesOnly = false) noexcept;

// ---- Warp-UI witness: orthographic scenes and AUTO-ortho world transforms ----------------------
// Appended after every player and supplemental row under their own caps; existing caps are unchanged.
// Census presentation: the first presentation carrying a new first workload ID (RT64 workload identity
// only, never a guest bk_frame). It records every orthographic scene (up to the scene cap) and the AUTO-ortho
// transforms. Any other presentation records only AUTO-represented scenes RT64 is interpolating: an explicit-ID
// scene maps only to its own ID (rt64_game_frame.cpp matchScene), so its census rows already show any snap.
inline constexpr size_t tooieOrthoScenesPerPresentation = 16;
inline constexpr size_t tooieAutoOrthoRowsPerPresentation = 8;
inline constexpr uint32_t tooieOrthoPairingScenes = 32;   // scene pairing recomputed only up to 32 x 32
inline constexpr size_t tooieOrthoIndicesPerWorkload = 1024;

// RT64 GameFrame::match pairing: candidates in (current, previous) order, stable-sorted by difference and
// taken greedily when both sides are unpaired. paired[i] receives the previous ordinal or UINT32_MAX.
// False (nothing claimed) above the 32 x 32 limit or with a NaN difference, whose RT64 order is undefined.
bool tooiePairScenes(const float *difference, uint32_t currentScenes, uint32_t previousScenes,
    uint32_t *paired) noexcept;

// RT64 interpolates this projection: mapped, prior inside `previous`, and any prior word differs from raw.
bool tooieOrthoSceneInterpolating(const DrawData &current, const GameFrameMap::WorkloadMap *map,
    const DrawData *previous, uint32_t transformsIndex) noexcept;

struct TooieOrthoScene {
    uint32_t ordinal = 0, position = 0, projections = 0, scenes = 0, previousScenes = 0;
    uint32_t pairedScene = UINT32_MAX, pairedGroupId = 0;
    uint32_t flags = 0; // TooiePlayerProjection SceneCensus | SceneCrossWorkload | PairKnown
};
// One OrthoScene projection row for viewProj index `transformsIndex` of `current`, with the same slot, prior
// and map rules as tooieCollectPlayerRows. False only when `projections` has no capacity left.
bool tooieCollectOrthoScene(uint64_t presentation, const DrawData &current, uint32_t currentSlot,
    uint64_t currentId, const GameFrameMap::WorkloadMap *map, const DrawData *previous, uint32_t previousSlot,
    uint64_t previousId, float weight, bool projectionsProcessed, uint32_t transformsIndex,
    const TooieOrthoScene &scene, std::vector<TooiePlayerProjection> &projections) noexcept;

// Census AUTO-ortho selection across a presentation's workloads: AUTO, IGNORE and explicit non-root/non-player world transforms whose
// first owned vertex uses one of `ortho` (that workload's orthographic scene viewProj indices). Ranked by
// max(mapped translation displacement, model-space vertex delta) (nonfinite first, unmapped last, ties in draw order).
struct TooieAutoOrthoTop {
    std::array<TooiePlayerTransform, tooieAutoOrthoRowsPerPresentation> rows{};
    std::array<float, tooieAutoOrthoRowsPerPresentation> keys{};
    size_t count = 0;
    uint32_t eligible = 0; // every eligible transform of the presentation
    bool truncated = false; // an orthographic index list was capped
};
void tooieCollectAutoOrtho(uint64_t presentation, const DrawData &current, uint32_t currentSlot,
    uint64_t currentId, const GameFrameMap::WorkloadMap *map, const DrawData *previous, uint32_t previousSlot,
    uint64_t previousId, float weight, float previousWeight, uint32_t frameFlags, bool transformsProcessed,
    const uint32_t *ortho, size_t orthoCount, TooieAutoOrthoTop &top) noexcept;
// Appends the selection within remaining capacity; Truncated marks the last row when any eligible
// transform or orthographic index was left out.
void tooieEmitAutoOrtho(const TooieAutoOrthoTop &top, std::vector<TooiePlayerTransform> &rows) noexcept;
}
