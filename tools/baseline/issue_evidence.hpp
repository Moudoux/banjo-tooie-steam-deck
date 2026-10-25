#pragma once
// Retrospective, numeric diagnostics only. Producers retain no guest memory.
#include <array>
#include "menu_witness.hpp"
#include "transition_fade_snapshot.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
// RT64 player correspondence row types and sink (no RT64 dependency).
#include "../../src/rt64_matrix_trace.hpp"

namespace baseline::issue_evidence {
struct FrameRecord {
    // Frame bits Layout/Frame are DrawRecord's values; Cpu625 is frame-only.
    enum Valid : uint32_t { Cpu625=4 };
    uint64_t sequence{}, time_us{};
    uint32_t phase{}, epoch{}, map{}, pool_index{}, view{}, camera{}, projection{},
        camera_active{}, camera_latch{}, bridge_poisoned{}, control_slot{},
        player_owner{}, player_model{}, player_asset{}, player_admissions{}, player_refusals{};
    int32_t requested_period{}, measured_period{}, live_period{};
    uint32_t valid{};
    // Schema 2. Layout covers the two bridge words; Cpu625 covers its allocator cursor.
    // last_reason is the latest begin call's word (0xFFFFFFFF admitted), not a per-frame result.
    uint32_t player_last_reason{}, bridge_next_block{}, cpu625_next_block{};
    // PC[8] layout 4: last admitted occurrence's view and the protected primary ledger.
    uint32_t player_view{}, player_primary_bytes{}, player_primary_highwater{};
    // PC[8] layout 5: secondary route counters (session-cumulative) and the SECONDARY ledger.
    uint32_t secondary_opens{}, secondary_faults{}, secondary_seen_main{}, secondary_seen_late{};
    uint32_t secondary_unauthenticated{}, secondary_admissions{}, secondary_ledger_bytes{};
    // Optional shadow_pending words, read only at an authenticated End: the frame's final
    // state after its draws, never a per-draw event list. Not part of `valid`.
    uint32_t shadow_valid{}, shadow_source_pc{}, shadow_seen_epoch{}, shadow_id{}, shadow_group{},
        shadow_composite{}, shadow_mode_open{}, shadow_fault{};
    std::array<uint32_t,4> shadow_args{};
    tooie_menu_witness::Snapshot menu{};
    tooie_transition_fade::Snapshot transition_fade{};
    uint32_t shadow_branch_valid{}, shadow_branch_frame{}, shadow_cuts{}, shadow_declined{};
};
struct DrawRecord {
    // Join bits are schema 2: synchronous outcomes of the calls around this DEFA0 transport.
    enum Valid : uint32_t { Layout=1, Frame=2, Root=4, Projection=8, Context=16,
        PlayerJoin=32, PlayerSlot=64, SkeletalJoin=128 };
    uint64_t sequence{}, time_us{};
    uint32_t epoch{}, buffers{}, cursor{}, model{}, asset{}, pool_index{}, root_address{},
        actor{}, descriptor{}, slot{}, birth{}, view{}, context_epoch{}, context_admitted{},
        camera{}, camera_birth{}, camera_revision{}, camera_id{}, camera_scale{}, camera_policy{},
        projection_address{}, valid{};
    // Original fixed N64 Mtx words, decoded as native integers; no float conversion.
    // Projection is the last saved WORLD production, not the effective stack/per-model matrix.
    std::array<uint32_t,16> root{}, projection{};
    // PlayerJoin: this body's tooie_player_begin outcome (0xFFFFFFFF admitted, else its reason).
    // PlayerSlot: the owner was in range, so its slot and table binding were read.
    uint32_t player_owner{}, player_owner_slot{}, player_table_match{}, player_control_slot{}, player_outcome{};
    // SkeletalJoin: exact counter deltas of the NPC/skeletal admit called at this transport.
    uint32_t skeletal_calls{}, skeletal_admissions{}, skeletal_refusals{}, skeletal_reason{};
};
// BK root draw occurrence: DF0B0 begin snapshot, paired with DF234 end or bit40 DF238 join.
// CPU-emitted tag state only; not a legacy DEFA0 transport, admission or RT64-consumed mapping.
struct BkDrawRecord {
    // Owner: the BK owner and root frame were read. Staged: raw C9A8/C9AC words, never owner proof.
    // Route: guarded top-level callback staging, NOT authenticated original producer identity.
    // drawn_owner is only that staged candidate; Roster checks its slot/table membership.
    // Control is independent. End: matching DF234 or bit40 DF238. Unclosed: no matching close.
    enum Valid : uint32_t { Layout=1, Frame=2, Owner=4, Staged=8, Route=16, Roster=32, Control=64,
        ControlOwner=128, End=256, Unclosed=512, RootWitness=1024 };
    enum Source : uint32_t { Root=1 };
    enum IdDomain : uint32_t { NoId=0, PlayerId=1, ActorId=2, OtherId=3 };
    uint64_t sequence{}, time_us{};
    uint32_t source{}, valid{}, epoch{}, bk_frame{}, root_serial{}, view{}, phase{}, sp{}, buffers{}, model{},
        asset{}, depth{}, draw_depth{};
    uint32_t owner_kind{}, owner_birth{}, owner_view{}, owner_phase{}, owner_base{}, owner_attachments{}, owner_id{};
    // offset/uses_bones are the enclosing values the helper saved; push is its root frame mode.
    uint32_t current_id{}, emitted_id{}, offset{}, uses_bones{}, push{}, skip_all{}, skip_model{},
        id_domain{}, id_attachment{}, id_sample{};
    uint32_t staged_callback{}, staged_owner{}, drawn_owner{}, drawn_slot{}, drawn_table_match{};
    uint32_t control_slot{}, control_owner{};
    // End: this root's own final offset/bones; group and overflow deltas include nested roots.
    uint32_t end_offset{}, end_uses_bones{}, groups{}, offset_overflows{};
    uint32_t producer_site{}, producer_model{}; // Exact synchronous core callsite, not semantic owner.
    uint32_t late_closes{}, root_overflows{}, scope_overflows{}; // Session-cumulative at begin.
    // Before: capture[8], history[id,frame,model,start], float frame, draw ID.
    // After: segment[pos,vel,base,enabled], selector reserved, decline counter delta.
    std::array<uint32_t,20> root_witness{};
};
struct HostRecord {
    uint64_t sequence{}, time_us{};
    int32_t width{}, height{}, wide{}, aspect{}, cutscene_aspect{}, resolution{}, output_rate{},
        manual_hz{}, vsync{}, texture_filter{}, output_filter{};
    uint32_t published_aspect{}, guest_publication{}, buttons{}, flags{};
    int32_t stick_x{}, stick_y{}; // Q15 mapped action axes; enums above are player preferences.
};
struct GraphicsRecord {
    // Graphics configuration enums are actual RT64 values, unlike HostRecord preferences.
    enum Kind : uint32_t { ParseBegin=1, ParseEnd=2, ScreenBegin=3, ScreenEnd=4,
        DummyBegin=5, DummyEnd=6, ConfigApplied=7 };
    uint64_t sequence{}, time_us{};
    uint32_t kind{}, task_sequence{}, data_ptr{}, data_size{}, swap_chain_rate{};
    int32_t resolution{}, output_rate{}, manual_hz{}, texture_filter{}, output_filter{};
};
struct PublicationRecord {
    uint64_t sequence{}, time_us{};
    uint32_t generation{}, flags{};
};
// RT64 CPU-side correspondence (legacy Player names; players plus actor/menu root supplement) (rt64_matrix_trace.hpp), recorded per presentation on the
// RT64 thread. Not guest-emitted state, not GPU or pixel output; joins guest rings by time only.
struct PlayerTransformRecord {
    uint64_t sequence{}, time_us{};
    RT64::TooiePlayerTransform row{};
};
struct PlayerProjectionRecord {
    uint64_t sequence{}, time_us{};
    RT64::TooiePlayerProjection row{};
};
inline constexpr uint64_t window_us = 10000000;
inline constexpr size_t frame_capacity=1024, draw_capacity=4096, host_capacity=1024,
    graphics_capacity=2048, publication_capacity=256, bk_draw_capacity=4096;
// Shared unchanged ring: player transforms plus up to64 supplemental roots per presentation.
// Extra roots shorten retained wall time; the cap remains32768 records.
inline constexpr size_t player_transform_capacity=32768, player_projection_capacity=4096;
// The existing domains (written first, formerly 8 MiB) plus both saturated player domains.
inline constexpr size_t report_max_bytes=64*1024*1024;
inline constexpr unsigned report_limit=32;
struct Completion {
    unsigned marker{};
    bool success{};
    std::string filename; // Basename only, including on failure.
    std::string error;    // Fixed code, never an exception message or path.
    uint64_t bytes{};
};
void record(FrameRecord) noexcept;
void record(DrawRecord) noexcept;
void record(BkDrawRecord) noexcept;
void record(HostRecord) noexcept;
void record(GraphicsRecord) noexcept;
void record(PublicationRecord) noexcept;
// RT64::TooiePlayerSink: one call per presentation with that presentation's rows.
void record_player(const RT64::TooiePlayerTransform* rows, size_t row_count,
    const RT64::TooiePlayerProjection* projections, size_t projection_count) noexcept;
// Main/SDL serializes lifecycle and capture calls. stop drains the one accepted
// job before its acquired log root is released. A pending completion holds busy.
void start(const std::filesystem::path& log_root, const std::string& version) noexcept;
void stop() noexcept;
bool capture(unsigned marker) noexcept;
std::optional<Completion> poll();
} // namespace baseline::issue_evidence
