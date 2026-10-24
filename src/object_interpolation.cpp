#include "object_interpolation.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <vector>

#include "recomp.h"
#define F3DEX_GBI_2
#include "rt64_extended_gbi.h"
#include "hle/rt64_rsp.h"
#include "hle/rt64_state.h"
#include "hle/rt64_workload_queue.h"

// Every actor drawn through func_8010A79C, and the player, gets its own RT64 matrix
// group, so RT64 matches its transforms only against the same object in the previous
// frame. Untagged draws are matched by draw-call hash across the whole frame, which
// pairs identical or similar models with each other and makes them jitter.
//
// Nothing is written to the game's display lists (its pool is sized for the game
// plus the guest owners' 6400 bytes): the draw hooks record which pool commands each
// object produced, and an observer in RT64's interpreter
// (patches/rt64/object_groups.delta.json) pushes the object's group while it runs
// commands inside that range and pops it on leaving. Commands outside the pool (the
// object's model display lists) keep whatever group is open. Guest owner groups
// inside an object still take precedence. linux/INTERPOLATION.md has the cases
// behind each group property and the trace below.
namespace RT64 { extern void (*tooieCommandObserver)(State* state, uint32_t rdramAddress); }

namespace {
constexpr std::uint32_t k_current_map = 0x80132DC2U;      // u16, as func_800EA05C returns.
constexpr std::uint32_t k_descriptors = 0x80132ED0U;      // D_80132ED0[200], 0x30 bytes each.
constexpr std::uint32_t k_descriptor_count = 200U;
constexpr std::uint32_t k_gfx_cursor = 0x80127658U;       // GraphicsBuffers.gfx (func_800A7180).
constexpr std::uint32_t k_gfx_pools = 0x8007BCD0U;        // D_8007BCD0[2] pool bases.
constexpr std::uint32_t k_gfx_override = 0x8007BCE8U;     // D_8007BCE8[0], nonzero redirects output.
constexpr std::uint32_t k_gfx_pool_index = 0x8007BEDAU;   // D_8007BEDA, s16.
constexpr std::uint32_t k_gfx_pool_bytes = 0x8FC0U;
constexpr std::uint32_t k_model_context = 0x8012C948U;    // D_8012C948, model render context.
constexpr std::uint32_t k_player_callback = 0x80092234U;  // D_8012C948[24] while drawing the player.
// Actor ids 0xE0000000 | slot << 16 | serial << 1 | 1, player ids 0xE1000000 | model >> 3.
// The guest owners use ids below 0xD0000000, the game select groups 0xD1xxxxxx.
constexpr std::uint32_t k_actor_ids = 0xE0000000U;
constexpr std::uint32_t k_player_ids = 0xE1000000U;
constexpr int k_max_depth = 8;
constexpr std::size_t k_max_ranges = 1024, k_max_draws = 512;

std::atomic<bool> active{false};
// TOOIE_INTERPOLATION_TRACE=1: F4 also logs the next three frames' objects, model
// draws and RT64 workloads (linux/INTERPOLATION.md).
const bool trace_enabled = [] {
    const char* value = std::getenv("TOOIE_INTERPOLATION_TRACE");
    return value && *value && *value != '0';
}();

struct Range { std::uint32_t start, end, id; };
struct Draw { std::uint32_t cursor, callback, model, object, objects_open; }; // Trace only.
struct Pool { std::uint32_t base = 0, cursor = 0; std::vector<Range> ranges; std::vector<Draw> draws; };

// Game thread: recording. The graphics owner reads the pools under the mutex.
std::mutex mutex;
std::array<Pool, 2> pools;
struct Open { int pool = -1; std::size_t index = 0; };
std::array<Open, k_max_depth> actor_open, model_open;
int actor_depth = 0, model_depth = 0;

gpr address(std::uint32_t value) { return static_cast<gpr>(static_cast<std::int32_t>(value)); }
std::uint32_t word(std::uint8_t* rdram, std::uint32_t at) { return static_cast<std::uint32_t>(MEM_W(0, address(at))); }

// The actor's descriptor slot and allocation serial (func_800EBED4 stores
// serial << 1 at +0x12): stable for the actor's life, new for a new actor.
std::uint32_t actor_id(std::uint8_t* rdram, std::uint32_t actor) {
    const std::uint32_t descriptor = word(rdram, actor);
    const std::uint32_t offset = descriptor - k_descriptors;
    if (descriptor < k_descriptors || offset >= k_descriptor_count * 0x30U || offset % 0x30U) return 0;
    const std::uint32_t serial = static_cast<std::uint16_t>(MEM_H(0x12, address(descriptor))) >> 1;
    return k_actor_ids | ((offset / 0x30U) << 16) | (serial << 1) | 1U;
}

// The pool index and cursor when the game draws into its own pool, else -1. The
// pools sit in the heap's expanded RDRAM, above the original 8 MiB.
int pool_cursor(std::uint8_t* rdram, std::uint32_t& base, std::uint32_t& cursor) {
    if (word(rdram, k_gfx_override) != 0) return -1;
    const auto index = static_cast<std::int16_t>(MEM_H(0, address(k_gfx_pool_index)));
    if (index < 0 || index > 1) return -1;
    base = word(rdram, k_gfx_pools + 4U * std::uint32_t(index));
    cursor = word(rdram, k_gfx_cursor);
    if ((base & 7U) || base < 0x80000000U || base > 0xFFFFFFFFU - k_gfx_pool_bytes ||
        cursor < base || cursor > base + k_gfx_pool_bytes) return -1;
    return index;
}

// The pool being drawn into, started over when a new frame begins at its base.
// Requires the mutex.
Pool& frame_pool(int index, std::uint32_t base, std::uint32_t cursor) {
    Pool& pool = pools[std::size_t(index)];
    if (pool.base != base || cursor < pool.cursor) { pool.base = base; pool.ranges.clear(); pool.draws.clear(); }
    pool.cursor = cursor;
    return pool;
}

// Opens a range for id at the current cursor, or leaves the entry closed when the
// game is not drawing into its pool.
void open_range(std::uint8_t* rdram, Open& entry, std::uint32_t id) {
    std::uint32_t base = 0, cursor = 0;
    const int index = pool_cursor(rdram, base, cursor);
    if (index < 0) return;
    std::lock_guard lock(mutex);
    Pool& pool = frame_pool(index, base, cursor);
    if (pool.ranges.size() >= k_max_ranges) return;
    pool.ranges.push_back({cursor, cursor, id});
    entry = {index, pool.ranges.size() - 1};
}

// Closes the range at the cursor when the draw stayed in the same pool and frame.
void close_range(std::uint8_t* rdram, Open& slot) {
    const Open entry = slot;
    slot = {};
    if (entry.pool < 0) return;
    std::uint32_t base = 0, cursor = 0;
    const int index = pool_cursor(rdram, base, cursor);
    std::lock_guard lock(mutex);
    Pool& pool = pools[std::size_t(entry.pool)];
    if (index != entry.pool || pool.base != base || entry.index >= pool.ranges.size() ||
        cursor < pool.ranges[entry.index].start) return;
    pool.ranges[entry.index].end = cursor;
    pool.cursor = cursor;
}

// The innermost open actor range's id, or 0.
std::uint32_t open_actor() {
    for (int i = std::min(actor_depth, k_max_depth) - 1; i >= 0; --i) {
        const Open& entry = actor_open[std::size_t(i)];
        if (entry.pool >= 0) return pools[std::size_t(entry.pool)].ranges[entry.index].id;
    }
    return 0;
}

// Graphics owner: the current display list's segments. Elementary address
// intervals of the pool, each with the chain of object groups (outermost first)
// that covers it.
struct Chain { std::array<std::uint32_t, k_max_depth> ids{}; int size = 0; };
struct Segments {
    std::uint32_t begin = 0, end = 0; // Physical pool range.
    std::vector<std::uint32_t> bounds; // Segment i is [bounds[i], bounds[i + 1]).
    std::vector<Chain> chains;
    Chain applied;
    std::size_t current = SIZE_MAX;
    bool ignored = false; // The current projection is an ignored orthographic one.
} segments;

void build(const Pool& pool) {
    segments.bounds.clear(); segments.chains.clear();
    segments.applied = {}; segments.current = SIZE_MAX; segments.ignored = false;
    segments.begin = pool.base - 0x80000000U;
    segments.end = segments.begin + k_gfx_pool_bytes;
    struct Event { std::uint32_t at; bool opens; std::uint32_t id; std::size_t order; };
    std::vector<Event> events;
    events.reserve(pool.ranges.size() * 2);
    for (std::size_t i = 0; i < pool.ranges.size(); ++i) {
        const auto& range = pool.ranges[i];
        if (range.end <= range.start) continue;
        events.push_back({range.start - 0x80000000U, true, range.id, i});
        events.push_back({range.end - 0x80000000U, false, range.id, i});
    }
    // Closes before opens at one address; opens in recording (outer first) order,
    // closes inner first.
    std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
        if (a.at != b.at) return a.at < b.at;
        if (a.opens != b.opens) return !a.opens;
        return a.opens ? a.order < b.order : a.order > b.order;
    });
    Chain chain;
    for (std::size_t i = 0; i < events.size();) {
        const std::uint32_t at = events[i].at;
        for (; i < events.size() && events[i].at == at; ++i) {
            if (events[i].opens) {
                if (chain.size < k_max_depth) chain.ids[chain.size] = events[i].id;
                ++chain.size;
            } else if (chain.size > 0) --chain.size;
        }
        Chain stored = chain;
        stored.size = std::min(chain.size, k_max_depth);
        segments.bounds.push_back(at);
        segments.chains.push_back(stored);
    }
    if (!segments.bounds.empty()) segments.bounds.push_back(segments.end);
}

void push_group(RT64::State* state, std::uint32_t id) {
    if (id == G_EX_ID_IGNORE) {
        // Under an ignored orthographic projection (see observe).
        state->rsp->matrixId(id, true, false, true,
            G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP,
            G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP,
            G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR, G_EX_ASPECT_AUTO, G_EX_EDIT_NONE, false, false);
    } else if ((id & 0xFF000000U) == k_player_ids) {
        // The player: the guest player owner's bone group properties. Matrices are
        // blended, not decomposed; Stony's backpack flap hangs from pivots far from
        // its geometry, and a decomposed rotation swung it out behind him.
        state->rsp->matrixId(id, true, false, false,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP,
            G_EX_COMPONENT_SKIP, G_EX_COMPONENT_AUTO, G_EX_COMPONENT_AUTO,
            G_EX_ORDER_LINEAR, G_EX_ASPECT_AUTO, G_EX_EDIT_NONE, false, false);
    } else {
        // Actors: RT64's defaults for untagged draws (TransformGroup) except for
        // linear ordering (automatic ordering pairs an actor's transforms by draw-call
        // hash, which swapped Cheato's identical pages) and no texture offset
        // interpolation (menu icons animate by moving a quad's window over a sprite
        // sheet, and in-between offsets showed parts of neighbouring images).
        state->rsp->matrixId(id, true, false, true,
            G_EX_COMPONENT_AUTO, G_EX_COMPONENT_AUTO, G_EX_COMPONENT_AUTO, G_EX_COMPONENT_AUTO,
            G_EX_COMPONENT_AUTO, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP,
            G_EX_COMPONENT_AUTO, G_EX_ORDER_LINEAR, G_EX_ASPECT_AUTO, G_EX_EDIT_NONE, false, false);
    }
}

void apply(RT64::State* state, const Chain& wanted) {
    Chain& applied = segments.applied;
    int common = 0;
    while (common < applied.size && common < wanted.size && applied.ids[common] == wanted.ids[common]) ++common;
    if (applied.size > common) state->rsp->popMatrixId(std::uint8_t(applied.size - common), false);
    for (int i = common; i < wanted.size; ++i) push_group(state, wanted.ids[i]);
    applied = wanted;
}

// The object chain for the current segment, plus an ignored group while the current
// projection is an ignored orthographic one. The guest's camera owner marks
// projections it does not track G_EX_ID_IGNORE, so RT64 steps them at the game
// rate; menu entries slide by moving their own orthographic projection, and an
// object's parts drawn under one must step too, or they drift from the parts that
// move with the projection (warp pad menu icons against their entry bars). Ignored
// perspective projections (game select's camera) keep their objects interpolated.
void observe(RT64::State* state, std::uint32_t at) {
    bool changed = false;
    if (at >= segments.begin && at < segments.end && !segments.bounds.empty()) {
        const std::size_t current = segments.current;
        if (current == SIZE_MAX || at < segments.bounds[current] || at >= segments.bounds[current + 1]) {
            const auto next = std::upper_bound(segments.bounds.begin(), segments.bounds.end(), at);
            const bool inside = next != segments.bounds.begin() && next != segments.bounds.end();
            segments.current = inside ? std::size_t(next - segments.bounds.begin()) - 1 : SIZE_MAX;
            changed = segments.current != current;
        }
    }
    const auto& extended = state->rsp->extended;
    const bool ignored = extended.viewProjMatrixIdStackSize > 0 &&
        extended.viewProjMatrixIdStack[std::size_t(extended.viewProjMatrixIdStackSize - 1)].matrixId == G_EX_ID_IGNORE &&
        state->rsp->getCurrentProjectionType() == RT64::Projection::Type::Orthographic;
    if (!changed && ignored == segments.ignored) return;
    segments.ignored = ignored;
    Chain wanted = segments.current == SIZE_MAX ? Chain{} : segments.chains[segments.current];
    if (ignored && wanted.size > 0 && wanted.size < k_max_depth) wanted.ids[std::size_t(wanted.size++)] = G_EX_ID_IGNORE;
    apply(state, wanted);
}

// Trace (graphics owner): F4 arms three frames; begin_display_list prints the
// recorded objects and model draws, end_display_list the workload RT64 built.
std::atomic<int> trace_frames{0};
int trace_frame = -1;
unsigned trace_serial = 0;

void trace_recording(RT64::State* state, std::uint32_t physical_address) {
    trace_frame = 3 - trace_frames.fetch_sub(1, std::memory_order_acq_rel);
    if (trace_frame == 0) ++trace_serial;
    const unsigned map = *reinterpret_cast<const std::uint16_t*>(state->RDRAM + ((k_current_map ^ 2U) - 0x80000000U));
    std::lock_guard lock(mutex);
    std::fprintf(stderr, "trace %u.%d dl=%08X objects=%d map=%03X\n", trace_serial, trace_frame,
        physical_address, int(active.load(std::memory_order_acquire)), map);
    for (const auto& pool : pools) {
        const std::uint32_t begin = pool.base - 0x80000000U;
        if (!pool.base || physical_address < begin || physical_address >= begin + k_gfx_pool_bytes) continue;
        for (const auto& r : pool.ranges)
            std::fprintf(stderr, "trace %u.%d range id=%08X start=+%05X len=%u\n", trace_serial, trace_frame,
                r.id, r.start - pool.base, r.end - r.start);
        for (const auto& d : pool.draws)
            std::fprintf(stderr, "trace %u.%d model at=+%05X callback=%08X model=%08X object=%08X objects_open=%u\n",
                trace_serial, trace_frame, d.cursor - pool.base, d.callback, d.model, d.object, d.objects_open);
    }
}

void trace_workload(RT64::State* state) {
    const auto& queue = *state->ext.workloadQueue;
    const auto& w = queue.workloads[std::size_t((queue.writeCursor + WORKLOAD_QUEUE_SIZE - 1) % WORKLOAD_QUEUE_SIZE)];
    const auto& d = w.drawData;
    std::fprintf(stderr, "trace %u.%d workload fbpairs=%u world=%zu groups=%zu viewprojs=%zu\n", trace_serial, trace_frame,
        w.fbPairCount, d.worldTransforms.size(), d.transformGroups.size(), d.viewProjTransformGroups.size());
    for (std::uint32_t f = 0; f < w.fbPairCount && f < w.fbPairs.size(); ++f) {
        const auto& pair = w.fbPairs[f];
        for (std::uint32_t p = 0; p < pair.projectionCount && p < pair.projections.size(); ++p) {
            const auto& proj = pair.projections[p];
            const std::uint32_t t = proj.transformsIndex;
            std::uint32_t id = 0, order = 0;
            if (t < d.viewProjTransformGroups.size() && d.viewProjTransformGroups[t] < d.transformGroups.size()) {
                id = d.transformGroups[d.viewProjTransformGroups[t]].matrixId;
                order = d.transformGroups[d.viewProjTransformGroups[t]].ordering;
            }
            const float* m = t < d.projTransforms.size() ? reinterpret_cast<const float*>(&d.projTransforms[t]) : nullptr;
            const float* v = t < d.viewTransforms.size() ? reinterpret_cast<const float*>(&d.viewTransforms[t]) : nullptr;
            std::fprintf(stderr, "trace %u.%d proj fb=%u #%u type=%d calls=%u t=%u group=%08X/o%u scissor=%d,%d,%d,%d "
                "P=%.3f,%.3f,%.3f,%.3f V=%.1f,%.1f,%.1f\n",
                trace_serial, trace_frame, f, p, int(proj.type), proj.gameCallCount, t, id, order,
                proj.scissorRect.ulx, proj.scissorRect.uly, proj.scissorRect.lrx, proj.scissorRect.lry,
                m ? m[0] : 0.f, m ? m[5] : 0.f, m ? m[10] : 0.f, m ? m[14] : 0.f, v ? v[12] : 0.f, v ? v[13] : 0.f, v ? v[14] : 0.f);
            for (std::uint32_t c = 0; c < proj.gameCallCount && c < proj.gameCalls.size(); ++c) {
                const auto& call = proj.gameCalls[c].callDesc;
                if (call.minWorldMatrix > call.maxWorldMatrix || call.maxWorldMatrix >= d.worldTransformGroups.size()) continue;
                const std::uint32_t g = d.worldTransformGroups[call.minWorldMatrix];
                const auto& group = d.transformGroups[g < d.transformGroups.size() ? g : 0];
                const float* wm = reinterpret_cast<const float*>(&d.worldTransforms[call.minWorldMatrix]);
                std::fprintf(stderr, "trace %u.%d  call #%u world=%u-%u group=%08X/o%u tris=%u pos=%.1f,%.1f,%.1f\n",
                    trace_serial, trace_frame, c, call.minWorldMatrix, call.maxWorldMatrix, group.matrixId,
                    group.ordering, call.triangleCount, wm[12], wm[13], wm[14]);
            }
        }
    }
    std::fflush(stderr);
}
}

namespace tooie::objects {
void set_enabled(bool enabled) noexcept { active.store(enabled, std::memory_order_release); }
bool enabled() noexcept { return active.load(std::memory_order_acquire); }
void request_trace() noexcept { if (trace_enabled) trace_frames.store(3, std::memory_order_release); }

void begin_display_list(RT64::State* state, std::uint32_t physical_address) {
    trace_frame = -1;
    if (trace_enabled && trace_frames.load(std::memory_order_acquire) > 0) trace_recording(state, physical_address);
    if (!enabled()) { RT64::tooieCommandObserver = nullptr; return; }
    Pool copy;
    {
        std::lock_guard lock(mutex);
        for (const auto& pool : pools) {
            const std::uint32_t begin = pool.base - 0x80000000U;
            if (pool.base && physical_address >= begin && physical_address < begin + k_gfx_pool_bytes) copy = pool;
        }
    }
    if (!copy.base) {
        // A display list outside the pools (or before any recording) runs untagged.
        RT64::tooieCommandObserver = nullptr;
        return;
    }
    build(copy);
    RT64::tooieCommandObserver = segments.bounds.empty() ? nullptr : &observe;
}

void end_display_list(RT64::State* state) {
    if (trace_frame >= 0) trace_workload(state);
}
}

// Raw hooks (ACTOR_DRAW_HOOKS in tools/baseline/minimum_host_hooks.py). Around the
// template draw call in func_8010A79C, the actor draw dispatcher; the actor range
// encloses the game select group commands, so those nest inside.
extern "C" void tooie_menu_group_begin(std::uint8_t* rdram, recomp_context* ctx);
extern "C" void tooie_menu_group_end(std::uint8_t* rdram, recomp_context* ctx);
extern "C" void tooie_actor_draw_begin(std::uint8_t* rdram, recomp_context* ctx) {
    const int level = actor_depth++;
    if (level < k_max_depth) {
        actor_open[level] = {};
        if (tooie::objects::enabled())
            if (const std::uint32_t id = actor_id(rdram, static_cast<std::uint32_t>(ctx->r4))) open_range(rdram, actor_open[level], id);
    }
    tooie_menu_group_begin(rdram, ctx);
}
extern "C" void tooie_actor_draw_end(std::uint8_t* rdram, recomp_context* ctx) {
    tooie_menu_group_end(rdram, ctx);
    if (actor_depth == 0) return;
    const int level = --actor_depth;
    if (level < k_max_depth) close_range(rdram, actor_open[level]);
}

// At the entry and the single return of func_800DE498 (model render). The player is
// the one model draw outside every actor range, made with the player callback in
// D_8012C948[24]; its group is keyed by the model, so a transformation starts a new one.
extern "C" void tooie_model_draw_begin(std::uint8_t* rdram, recomp_context* ctx) {
    const int level = model_depth++;
    if (level >= k_max_depth) return;
    model_open[level] = {};
    const std::uint32_t callback = word(rdram, k_model_context + 24U * 4U);
    // The sixth argument, on the caller's stack: the model.
    const std::uint32_t model = word(rdram, static_cast<std::uint32_t>(ctx->r29) + 0x14U);
    const std::uint32_t object = open_actor();
    if (trace_enabled) {
        std::uint32_t base = 0, cursor = 0;
        const int index = pool_cursor(rdram, base, cursor);
        if (index >= 0) {
            std::lock_guard lock(mutex);
            Pool& pool = frame_pool(index, base, cursor);
            if (pool.draws.size() < k_max_draws) pool.draws.push_back({cursor, callback, model, object, object != 0});
        }
    }
    if (!tooie::objects::enabled() || object || level != 0 || callback != k_player_callback) return;
    open_range(rdram, model_open[level], k_player_ids | ((model >> 3) & 0x00FFFFFFU));
}
extern "C" void tooie_model_draw_end(std::uint8_t* rdram, recomp_context*) {
    if (model_depth == 0) return;
    const int level = --model_depth;
    if (level < k_max_depth) close_range(rdram, model_open[level]);
}
