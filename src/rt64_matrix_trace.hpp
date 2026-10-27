#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>

namespace RT64 {
struct GameFrame;
struct WorkloadQueue;

// Opt-in diagnostic: the next issue marker arms a bounded screen-X trace.
void tooieScreenXTraceSetLogDirectory(const std::filesystem::path &directory) noexcept;
void tooieScreenXTraceArm() noexcept;

// Diagnostic only: observes the CPU matrices selected for presentation.
// F4 retrospective rows run when a player sink is registered; other traces are separately gated.
void tooieMatrixTraceFrame(const WorkloadQueue &queue, const GameFrame &curFrame,
    const GameFrame &prevFrame, float curFrameWeight, float prevFrameWeight,
    bool projectionsProcessed, bool transformsProcessed) noexcept;

// F4 correspondence rows (legacy Player type names): all player groups plus bounded actor/menu roots. Copied on the RT64 presentation thread from CPU-side draw data and
// the frame map after processing (workloadMutex held). They are not GPU or pixel output. Matrices
// are f32 bit patterns, row-major [row*4+col]. Fields covered by an absent flag hold no evidence.
// Optional source-only transition observation. Existing counts/rings are unchanged.
struct TooieTransitionSourceDraw {
    enum Flags : uint32_t { Observed=1u, CurrentPose=2u, Source=4u, Bounds=8u,
        Invalid=16u, Truncated=32u, MixedOwnership=64u, MixedProjection=128u,
        Modified=256u, InvalidW=512u, MixedSource=1024u, CustomOrigin=2048u,
        ProcessedProjection=4096u };
    uint32_t flags{}; uint16_t calls{}, triangles{};
    int32_t scissor[4]{}, viewport[4]{}; // first matching call, quarter-pixel source bounds
    float ndc[4]{}; // current-pose X/Y minima/maxima; never final/interpolated pixel extent
};
static_assert(sizeof(TooieTransitionSourceDraw)==56);
struct TooiePlayerTransform {
    enum Flags : uint32_t {
        OwnerMapped = 1u << 0,      // frameMap workload entry mapped
        PriorSlotValid = 1u << 1,   // its prior slot is in prevFrame.workloads, not this slot or workload ID
        Mapped = 1u << 2,           // frameMap transform entry mapped
        PriorIndexValid = 1u << 3,  // prior slot valid and prevTransformIndex/group inside that workload
        Interpolated = 1u << 4,     // transforms processed and lerpWorldTransforms holds this index
        CurrentMatched = 1u << 5,
        PreviousMatched = 1u << 6,
        Paused = 1u << 7,
        ProjectionKnown = 1u << 8,  // an owned vertex in the allocation range has a valid projection
        ProjectionMixed = 1u << 9,  // another owned vertex in that range used another projection
        LerpDecompose = 1u << 10, LerpTranslation = 1u << 11, LerpRotation = 1u << 12,
        LerpScale = 1u << 13, LerpSkew = 1u << 14,
        Truncated = 1u << 15,       // the presentation's row limit was reached after this row
        ProjectionTruncated = 1u << 16, // referenced projection detail omitted by capacity
        TransformsProcessed = 1u << 17, // process flag, independent of output availability
        Representative = 1u << 19,    // first sampled matrix of this workload/group; count below
        ProjectionIncomplete = 1u << 18, // foreign/missing/invalid vertex association in range
        // Census AUTO-group world transform whose first owned vertex uses an orthographic scene projection
        // (rt64_player_rows.hpp warp-UI witness); group_matrix_count = eligible transforms in its workload.
        AutoOrtho = 1u << 20,
        // The orthographic witness row's group is not AUTO: IGNORE, or an explicit ID the player and supplemental-root
        // rows do not report (a non-root component such as a box model's second graph node).
        UnrootedOrtho = 1u << 21,
        VertexSkip = 1u << 22,      // its group's vertex interpolation is SKIP: RT64 draws vertex changes unblended
        VertexMeasured = 1u << 23   // vertex_delta was measured over matching nonempty ranges
    };
    uint64_t presentation = 0, workload_id = 0, previous_workload_id = 0;
    float weight = 0, previous_weight = 0;
    // previous_slot: the frame map's prevWorkloadIndex as recorded (UINT32_MAX when not mapped).
    uint32_t workload_slot = 0, previous_slot = UINT32_MAX, flags = 0;
    uint32_t index = 0, group_id = 0, vertices = 0, physical = 0; // vertices: allocation-range count
    uint32_t previous_index = UINT32_MAX, previous_group_id = 0, previous_vertices = 0;
    uint32_t projection = UINT32_MAX;
    uint32_t raw[16]{}, prior[16]{}, interpolated[16]{};
    uint32_t group_matrix_count = 0; // valid only with Representative or AutoOrtho; includes unsampled matrices
    TooieTransitionSourceDraw transition_source{};
    // Orthographic witness and supplemental-root rows: max |position - mapped prior position| over equally sized vertex
    // ranges, in model space (RT64 matchTransform's vertex velocity input); -1 when unmapped, unequal or unavailable.
    // Player rows keep -1.
    float vertex_delta = -1.0f;
};
// The projections those rows reference, once per presentation and workload slot, then the warp-UI
// orthographic scene representatives (OrthoScene).
struct TooiePlayerProjection {
    enum Flags : uint32_t {
        OwnerMapped = 1u << 0, PriorSlotValid = 1u << 1, Mapped = 1u << 2, PriorIndexValid = 1u << 3,
        Final = 1u << 4,            // projections processed and modViewProjTransforms holds this index
        GroupKnown = 1u << 5,
        LerpDecompose = 1u << 10, LerpTranslation = 1u << 11, LerpRotation = 1u << 12,
        ProjectionsProcessed = 1u << 16,
        OrthoScene = 1u << 17,      // representative projection of one RT64 orthographic scene; scene fields valid
        SceneCensus = 1u << 18,     // first presentation of this workload set: every scene in order, up to the cap
        SceneTruncated = 1u << 19,  // more qualifying scenes than this presentation's scene rows, after this row
        SceneCrossWorkload = 1u << 20, // the scene's projections span more than one workload
        PairKnown = 1u << 21        // previous-frame scene pairing recomputed from RT64 matchScenes inputs
    };
    uint64_t presentation = 0, workload_id = 0, previous_workload_id = 0;
    uint32_t workload_slot = 0, previous_slot = UINT32_MAX, flags = 0;
    uint32_t index = 0, group_id = 0, previous_index = UINT32_MAX;
    uint32_t raw[16]{}, prior[16]{}, final[16]{};
    float weight = 0;                   // the presentation's current-frame weight
    uint32_t previous_group_id = 0;     // valid with PriorIndexValid: group ID of the prior projection used
    uint32_t scene = 0;                 // OrthoScene: scene ordinal (low 16) | representative position (high 16)
    uint32_t scene_projections = 0;     // OrthoScene: projections in the scene
    uint32_t scene_counts = 0;          // OrthoScene: orthographic scenes now (low 16) | previous frame (high 16)
    uint32_t paired_scene = UINT32_MAX; // PairKnown: previous-frame scene ordinal paired by RT64, or none
    uint32_t paired_group_id = 0;       // PairKnown: group ID of that previous scene's first projection
};
// rowCount may be zero when only scene rows exist for a presentation.
using TooiePlayerSink = void (*)(const TooiePlayerTransform *rows, size_t rowCount,
    const TooiePlayerProjection *projections, size_t projectionCount) noexcept;
// Set once by the player (configure_startup) before rendering; read by tooieMatrixTraceFrame.
inline std::atomic<TooiePlayerSink> tooiePlayerSink{nullptr};
// L05 F4 run: accepted F4 markers (minimum_host mark_issue) increment it; the renderer starts its one run on the first.
inline std::atomic<uint32_t> tooieL05BridgeArm{0};
}
