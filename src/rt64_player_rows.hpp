#pragma once
// F4 player correspondence collector over RT64 CPU draw data (implemented in rt64_matrix_trace.cpp).
#include "rt64_matrix_trace.hpp"

#include "hle/rt64_game_frame.h"
#include "hle/rt64_workload.h"

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
bool tooieIsSupplementalRootId(uint32_t id) noexcept;

// BK player transform IDs (guest/silo-candidate/bk_pipeline.h): roster slot 0
// [TOOIE_BK_PLAYER_ID_START, TOOIE_BK_PLAYER_ID_END) and slots 1..7
// [TOOIE_BK_PLAYER_SLOT_ID_START, TOOIE_BK_PLAYER_SLOT_ID_END). The shadow domain between them is excluded.
bool tooieIsPlayerTransformId(uint32_t id) noexcept;

// Prior slot rule: a mapped workload map whose prevWorkloadIndex is inside the queue, is a member of
// prevFrame.workloads, and is neither the current slot nor carries the current workload ID.
bool tooiePlayerPriorSlotValid(const GameFrameMap::WorkloadMap *map, uint32_t currentSlot, uint64_t currentId,
    const std::vector<uint32_t> &previousWorkloads, size_t queueWorkloads, uint64_t previousId) noexcept;

// Default pass appends one row per player-labelled world transform of `current`, plus the projections those
// rows reference. `previous` is non-null only when the prior slot rule held; `previousSlot` is the
// frame map's recorded prevWorkloadIndex (UINT32_MAX when unmapped). Never allocates: the caller
// Supplemental passes: one representative per (workload, group ID) across all workloads first,
// then remaining matrices. At most64 total; representativesOnly selects the first pass.
// Caller must finish all player workloads first. This does not reserve space against players.
// reserves tooiePlayerRowsPerPresentation and tooiePlayerProjectionsPerPresentation.
void tooieCollectPlayerRows(uint64_t presentation, const DrawData &current, uint32_t currentSlot,
    uint64_t currentId, const GameFrameMap::WorkloadMap *map, const DrawData *previous,
    uint32_t previousSlot, uint64_t previousId, float weight, float previousWeight, uint32_t frameFlags,
    bool projectionsProcessed, bool transformsProcessed, std::vector<TooiePlayerTransform> &rows,
    std::vector<TooiePlayerProjection> &projections, bool supplementalRoots = false, bool representativesOnly = false) noexcept;
}
