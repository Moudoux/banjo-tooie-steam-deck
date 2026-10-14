#pragma once
// Observation only. Included by exactly the two pinned synchronous parser TUs.
#include <cstdint>
#include <cstdio>
#include "hle/rt64_state.h"
#include "hle/rt64_rsp.h"
#include "hle/rt64_interpreter.h"
#include "shadow_witness.hpp"

namespace tooie_g6 {
using RT64::TransformGroup;
inline bool equal(const TransformGroup& a, const TransformGroup& b) noexcept {
    return a.matrixId == b.matrixId && a.decompose == b.decompose &&
        a.positionInterpolation == b.positionInterpolation && a.rotationInterpolation == b.rotationInterpolation &&
        a.scaleInterpolation == b.scaleInterpolation && a.skewInterpolation == b.skewInterpolation &&
        a.perspectiveInterpolation == b.perspectiveInterpolation && a.vertexInterpolation == b.vertexInterpolation &&
        a.texcoordInterpolation == b.texcoordInterpolation && a.tileInterpolation == b.tileInterpolation &&
        a.lookAtInterpolation == b.lookAtInterpolation && a.ordering == b.ordering &&
        a.aspectMode == b.aspectMode && a.editable == b.editable;
}
inline bool camera(uint32_t id) noexcept { return (id & 0xF0000000u) == 0xA0000000u; }
inline bool identity4(const hlslpp::float4x4& m) noexcept {
    float v[16]; hlslpp::store(m, v);
    for (unsigned i = 0; i < 16; ++i) if (v[i] != ((i / 4 == i % 4) ? 1.0f : 0.0f)) return false;
    return true;
}
inline bool identity3(const hlslpp::float3x3& m) noexcept {
    float v[9]; hlslpp::store(m, v);
    for (unsigned i = 0; i < 9; ++i) if (v[i] != ((i / 3 == i % 3) ? 1.0f : 0.0f)) return false;
    return true;
}
struct Record {
    const RT64::State* owner = nullptr;
    uint64_t sequence = 0, syncs = 0, assignments = 0, address_edits = 0, projections = 0;
    uint64_t owner_failures = 0, reset_failures = 0, group_failures = 0, incomplete = 0;
    uint64_t a_assignments = 0, a_returns = 0, default_or_ignore_projections = 0, a_post_projections = 0;
    uint64_t first_a_matrix_observations = 0, first_a_matrix_matches = 0, first_a_matrix_unattributed = 0;
    uint32_t latest_a_override_number = 0;
    bool first_a_matrix_seen = false;
    TransformGroup latest_a_override{};
    uint32_t last_a = 0, episodes = 0, episode_lines = 0, override_count = 0;
    bool initialized = false, saturated = false, camera_since_sync = false, current_sync_a = false;
    bool pending = false, returned = false, first_projection = false, post_projection = false;
    TransformGroup entry{}, first_override{}, latest_override{};
};
inline thread_local Record record;
inline const TransformGroup& current(const RT64::RSP& rsp) noexcept {
    return rsp.extended.viewProjMatrixIdStack[rsp.extended.viewProjMatrixIdStackSize - 1];
}
inline void tuple(const char* name, const TransformGroup& g) noexcept {
    std::fprintf(stderr, " %s={id:%08x,decompose:%u,pos:%u,rot:%u,scale:%u,skew:%u,persp:%u,vertex:%u,tc:%u,tile:%u,lookat:%u,order:%u,aspect:%u,editable:%u}",
        name, g.matrixId, unsigned(g.decompose), unsigned(g.positionInterpolation), unsigned(g.rotationInterpolation),
        unsigned(g.scaleInterpolation), unsigned(g.skewInterpolation), unsigned(g.perspectiveInterpolation),
        unsigned(g.vertexInterpolation), unsigned(g.texcoordInterpolation), unsigned(g.tileInterpolation),
        unsigned(g.lookAtInterpolation), unsigned(g.ordering), unsigned(g.aspectMode), unsigned(g.editable));
}
inline void prefix(const char* event, const RT64::State* state, uint32_t raw = 0, uint32_t physical = 0) noexcept {
    const auto& r = record;
    std::fprintf(stderr, "baseline camera-g6 event=%s seq=%llu sync=%llu episode=%u state=%p raw=%08x physical=%08x owner_failures=%llu reset_failures=%llu group_failures=%llu incomplete=%llu assignments=%llu address_edits=%llu projections=%llu a_assignments=%llu a_returns=%llu default_ignore_projections=%llu a_post_projections=%llu first_a_matrix_observations=%llu first_a_matrix_matches=%llu first_a_matrix_unattributed=%llu",
        event, (unsigned long long)r.sequence, (unsigned long long)r.syncs, r.episodes, static_cast<const void*>(state), raw, physical,
        (unsigned long long)r.owner_failures, (unsigned long long)r.reset_failures, (unsigned long long)r.group_failures,
        (unsigned long long)r.incomplete, (unsigned long long)r.assignments, (unsigned long long)r.address_edits,
        (unsigned long long)r.projections, (unsigned long long)r.a_assignments, (unsigned long long)r.a_returns,
        (unsigned long long)r.default_or_ignore_projections, (unsigned long long)r.a_post_projections,
        (unsigned long long)r.first_a_matrix_observations, (unsigned long long)r.first_a_matrix_matches, (unsigned long long)r.first_a_matrix_unattributed);
    tuple("current_group", current(*state->rsp));
}
inline void end_line() noexcept { std::fputc('\n', stderr); std::fflush(stderr); }
inline bool enter(const RT64::State* state) noexcept {
    auto& r = record; ++r.sequence;
    if (!r.initialized) {
        r.initialized = true; r.owner = state;
        prefix("armed", state); std::fprintf(stderr, " detailed_episode_cap=16 episode_line_cap=6 total_line_cap=98 gpu_completion_claim=0"); end_line();
    }
    if (r.owner != state) {
        ++r.owner_failures;
        if (!r.saturated) {
            r.saturated = true; prefix("owner-violation-saturated", state);
            std::fprintf(stderr, " expected_state=%p status=inconclusive detailed_capture_stopped=1", static_cast<const void*>(r.owner)); end_line();
        }
        return false;
    }
    return true;
}
inline bool line() noexcept {
    auto& r = record;
    if (!r.pending || r.saturated || r.episode_lines >= 6) return false;
    ++r.episode_lines; return true;
}
inline void close(const RT64::State* state) noexcept {
    auto& r = record; if (!r.pending) return;
    const bool complete = r.returned && r.override_count && r.post_projection;
    if (!complete) ++r.incomplete;
    if (line()) {
        prefix("episode-close", state);
        std::fprintf(stderr, " status=%s first_projection=%u overrides=%u assignments_after_first_override=%u post_projection=%u",
            complete ? "complete-observation" : "incomplete", unsigned(r.first_projection), r.override_count,
            r.override_count ? r.override_count - 1 : 0, unsigned(r.post_projection)); end_line();
    }
    r.pending = false;
}
inline void sync_entry(RT64::State* state) noexcept {
    if (!enter(state)) return; auto& r = record;
    close(state); ++r.syncs;
    r.current_sync_a = r.camera_since_sync;
    if (r.current_sync_a) {
        if (!r.saturated && r.episodes == 16) {
            r.saturated = true; prefix("saturated", state); std::fprintf(stderr, " detailed_capture_stopped=1 scalar_counters_continue=1"); end_line();
        }
        if (!r.saturated) ++r.episodes;
        r.episode_lines = 0; r.override_count = 0;
        r.latest_a_override_number = 0; r.first_a_matrix_seen = false;
        r.pending = true; r.returned = false; r.first_projection = false; r.post_projection = false;
        r.entry = current(*state->rsp);
        if (line()) { prefix("fullsync-entry", state); std::fprintf(stderr, " last_a=%08x", r.last_a); tuple("entry", r.entry); end_line(); }
    }
    r.camera_since_sync = false;
}
inline void sync_return(RT64::State* state) noexcept {
    if (!enter(state)) return; auto& r = record;
    if (!r.current_sync_a) return;
    const auto& e = state->rsp->extended;
    const bool group_default = equal(current(*state->rsp), TransformGroup{});
    const bool stack_one = e.viewProjMatrixIdStackSize == 1;
    const bool changed_false = !e.viewProjMatrixIdStackChanged;
    const bool index_zero = e.curViewProjMatrixIdGroupIndex == 0;
    const bool opcode_zero = state->ext.interpreter->extendedOpCode == 0;
    const bool vm = identity4(e.viewMatrix), pm = identity4(e.projMatrix), vp = identity4(e.viewProjMatrix);
    const bool iv = identity4(e.invViewMatrix), ip = identity4(e.invProjMatrix), ivp = identity4(e.invViewProjMatrix);
    const bool rotation = identity3(e.viewProjRotationMatrix);
    const bool ok = group_default && stack_one && changed_false && index_zero && opcode_zero && vm && pm && vp && iv && ip && ivp && rotation;
    if (!ok) ++r.reset_failures;
    ++r.a_returns; r.returned = true; r.current_sync_a = false;
    if (line()) {
        prefix("fullsync-return", state); tuple("after", current(*state->rsp));
        std::fprintf(stderr, " status=%s default_group=%u stack_one=%u changed_false=%u index_zero=%u opcode_zero=%u view4_identity=%u proj4_identity=%u combined4_identity=%u inv_view4_identity=%u inv_proj4_identity=%u inv_combined4_identity=%u rotation3_identity=%u gpu_completion_claim=0",
            ok ? "reset-observed" : "reset-violation", unsigned(group_default), unsigned(stack_one), unsigned(changed_false), unsigned(index_zero), unsigned(opcode_zero),
            unsigned(vm), unsigned(pm), unsigned(vp), unsigned(iv), unsigned(ip), unsigned(ivp), unsigned(rotation)); end_line();
    }
}
inline void assignment(RT64::State* state, uint32_t id, bool push, bool proj, bool idIsAddress, bool editGroup) noexcept {
    if (!enter(state)) return; auto& r = record;
    if (idIsAddress || editGroup) { ++r.address_edits; return; }
    if (!proj) return;
    ++r.assignments; const auto& g = current(*state->rsp);
    if (camera(g.matrixId)) { ++r.a_assignments; r.camera_since_sync = true; r.last_a = g.matrixId; }
    if (r.pending && r.returned) {
        ++r.override_count; r.latest_override = g;
        if (camera(g.matrixId)) { r.latest_a_override = g; r.latest_a_override_number = r.override_count; }
        if (r.override_count == 1) {
            r.first_override = g;
            if (line()) { prefix("first-projection-override", state); tuple("assigned", g); std::fprintf(stderr, " arg_id=%08x push=%u proj=%u idIsAddress=%u editGroup=%u", id, unsigned(push), unsigned(proj), unsigned(idIsAddress), unsigned(editGroup)); end_line(); }
        }
    }
}
inline void shadow_before_multiply(RT64::State* state,uint32_t raw,const hlslpp::float4x4& operand,const hlslpp::float4x4& camera) noexcept {
    static thread_local uint32_t rows=0;
    if(!shadow_witness::armed.load(std::memory_order_acquire) || current(*state->rsp).matrixId!=G_EX_ID_IGNORE || rows>=64)return;
    ++rows;const uint32_t physical=state->rsp->fromSegmentedMasked(raw);
    for(unsigned role=0;role<2;++role) {
        float m[16];hlslpp::store(role?camera:operand,m);
        std::fprintf(stderr,"baseline shadow-consumer event=before-mul row=%u raw=%08x physical=%08x role=%s id=%08x decompose=%u pos=%u rot=%u scale=%u skew=%u persp=%u float_hex=%a,%a,%a,%a,%a,%a,%a,%a,%a,%a,%a,%a,%a,%a,%a,%a\n",rows,raw,physical,role?"C":"R",current(*state->rsp).matrixId,unsigned(current(*state->rsp).decompose),unsigned(current(*state->rsp).positionInterpolation),unsigned(current(*state->rsp).rotationInterpolation),unsigned(current(*state->rsp).scaleInterpolation),unsigned(current(*state->rsp).skewInterpolation),unsigned(current(*state->rsp).perspectiveInterpolation),double(m[0]),double(m[1]),double(m[2]),double(m[3]),double(m[4]),double(m[5]),double(m[6]),double(m[7]),double(m[8]),double(m[9]),double(m[10]),double(m[11]),double(m[12]),double(m[13]),double(m[14]),double(m[15]));
    }
    std::fflush(stderr);
}
inline void projection(RT64::State* state, uint32_t raw, uint32_t physical, bool load, bool multiply) noexcept {
    if (!enter(state)) return; auto& r = record; ++r.projections;
    if (!r.pending || !r.returned) return;
    const auto& g = current(*state->rsp);
    if (g.matrixId == G_EX_ID_IGNORE || equal(g, TransformGroup{})) ++r.default_or_ignore_projections;
    if (!r.first_projection) {
        r.first_projection = true;
        if (!r.override_count && line()) {
            prefix("first-projection-before-override", state, raw, physical); tuple("observed", g);
            std::fprintf(stderr, " load=%u multiply=%u", unsigned(load), unsigned(multiply)); end_line();
        }
    }
    if (r.override_count && !r.post_projection) {
        r.post_projection = true;
        const bool same = equal(g, r.latest_override);
        if (!same) ++r.group_failures;
        if (same && camera(g.matrixId)) ++r.a_post_projections;
        if (line()) {
            prefix("post-override-projection", state, raw, physical); tuple("observed", g); tuple("first_override", r.first_override); tuple("latest_override", r.latest_override);
            std::fprintf(stderr, " load=%u multiply=%u status=%s overrides=%u intervening_assignments=%u",
                unsigned(load), unsigned(multiply), same ? "matches-latest-assignment" : "group-violation", r.override_count, r.override_count - 1); end_line();
        }
    }
    if (camera(g.matrixId) && !r.first_a_matrix_seen) {
        r.first_a_matrix_seen = true; ++r.first_a_matrix_observations;
        const bool valid = r.latest_a_override_number > 0 && r.latest_a_override_number <= r.override_count;
        const bool same = valid && r.latest_a_override_number == r.override_count &&
            equal(g, r.latest_a_override) && equal(r.latest_override, r.latest_a_override);
        if (same) ++r.first_a_matrix_matches; else ++r.first_a_matrix_unattributed;
        if (line()) {
            prefix("first-a-matrix", state, raw, physical); tuple("observed", g);
            tuple("latest_a_override", r.latest_a_override); tuple("latest_override", r.latest_override);
            std::fprintf(stderr, " load=%u multiply=%u status=%s latest_a_override_number=%u overrides=%u overrides_since_a=%u saved_a_number_valid=%u",
                unsigned(load), unsigned(multiply), same ? "matches-latest-a-assignment" : "unattributed",
                r.latest_a_override_number, r.override_count, valid ? r.override_count - r.latest_a_override_number : 0u, unsigned(valid)); end_line();
        }
    }
}
} // namespace tooie_g6
