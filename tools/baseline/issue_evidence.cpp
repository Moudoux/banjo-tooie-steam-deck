#include "issue_evidence.hpp"
#include "sound_trace.hpp"
#include "platform_support.hpp"
#include "guest/silo-candidate/bk_pipeline.h"
#include <json/json.hpp>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <ctime>
#include <memory>
#include <mutex>
#include <thread>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace baseline::issue_evidence {
namespace detail {
using Json = nlohmann::json;
inline bool invalid(const FrameRecord& record) {
    constexpr uint32_t required=DrawRecord::Layout|DrawRecord::Frame;
    return (record.valid&required)!=required;
}
inline bool invalid(const DrawRecord& record) { return (record.valid&31)!=31; }
inline bool invalid(const BkDrawRecord& record) {
    constexpr uint32_t required=BkDrawRecord::Layout|BkDrawRecord::Owner;
    return (record.valid&required)!=required;
}
inline bool invalid(const HostRecord&) { return false; }
inline bool invalid(const GraphicsRecord& record) { return record.kind<1 || record.kind>7; }
inline bool invalid(const PublicationRecord&) { return false; }
// Player rows carry explicit absent-field flags instead of an invalid state.
inline bool invalid(const PlayerTransformRecord&) { return false; }
inline bool invalid(const PlayerProjectionRecord&) { return false; }

template<class Record> struct Snapshot {
    uint64_t end_us{}, written{}, overwritten{}, invalid_total{}, record_failures{};
    bool truncated{};
    std::vector<Record> events;
};
// Each producer ring owns exactly one mutex. Copying one ring never locks another.
template<class Record, size_t Capacity> class Ring {
    std::mutex mutex;
    std::array<Record,Capacity> events{};
    uint64_t written{}, last_overwritten_us{}, invalid_total{};
    std::atomic<uint64_t> record_failures{};
public:
    void record(Record record, uint64_t time_us=0) noexcept {
        try {
            std::lock_guard guard(mutex);
            if (written>=Capacity) last_overwritten_us=events[written%Capacity].time_us;
            record.sequence=++written;
            record.time_us=time_us ? time_us : sound_trace::now_us();
            invalid_total+=invalid(record);
            events[(written-1)%Capacity]=record;
        } catch (...) { ++record_failures; }
    }
    Snapshot<Record> snapshot(uint64_t end_us=0) {
        Snapshot<Record> result{};
        result.events.reserve(Capacity); // Allocation does not hold the producer lock.
        std::lock_guard guard(mutex);
        if (!end_us) end_us=sound_trace::now_us();
        result.end_us=end_us;
        result.written=written;
        result.overwritten=written>Capacity ? written-Capacity : 0;
        result.invalid_total=invalid_total;
        result.record_failures=record_failures.load();
        result.truncated=result.overwritten && end_us>=last_overwritten_us &&
            end_us-last_overwritten_us<=window_us;
        for (uint64_t i=result.overwritten; i<written; ++i) {
            const auto& record=events[i%Capacity];
            if (record.time_us<=end_us && end_us-record.time_us<=window_us)
                result.events.push_back(record);
        }
        return result;
    }
};
using FrameRing=Ring<FrameRecord,frame_capacity>;
using DrawRing=Ring<DrawRecord,draw_capacity>;
using BkDrawRing=Ring<BkDrawRecord,bk_draw_capacity>;
using HostRing=Ring<HostRecord,host_capacity>;
using GraphicsRing=Ring<GraphicsRecord,graphics_capacity>;
using PublicationRing=Ring<PublicationRecord,publication_capacity>;
constexpr size_t ring_bytes=sizeof(FrameRing)+sizeof(DrawRing)+sizeof(HostRing)+
    sizeof(GraphicsRing)+sizeof(PublicationRing)+sizeof(BkDrawRing);
static_assert(ring_bytes<=3*1024*1024, "Issue evidence rings exceed the agreed memory cap");
FrameRing frames;
DrawRing draws;
BkDrawRing bk_draws;
HostRing hosts;
GraphicsRing graphics;
PublicationRing publications;
// New player domains have their own memory cap; the existing rings keep theirs.
using PlayerTransformRing=Ring<PlayerTransformRecord,player_transform_capacity>;
using PlayerProjectionRing=Ring<PlayerProjectionRecord,player_projection_capacity>;
constexpr size_t player_ring_bytes=sizeof(PlayerTransformRing)+sizeof(PlayerProjectionRing);
// One f32 witness adds 8 bytes after alignment per transform row: 256 KiB for the unchanged 32768-row ring.
static_assert(player_ring_bytes<=12*1024*1024+256*1024, "Player rows exceed their bounded witness memory cap");
PlayerTransformRing player_transforms;
PlayerProjectionRing player_projections;

struct Job {
    unsigned marker{}, ordinal{};
    uint64_t trigger_us{};
    std::string utc;
    Snapshot<FrameRecord> frames;
    Snapshot<DrawRecord> draws;
    Snapshot<BkDrawRecord> bk_draws;
    Snapshot<HostRecord> hosts;
    Snapshot<GraphicsRecord> graphics;
    Snapshot<PublicationRecord> publications;
    Snapshot<PlayerTransformRecord> player_transforms;
    Snapshot<PlayerProjectionRecord> player_projections;
    sound_trace::Snapshot audio;
};
struct State {
    std::mutex mutex;
    std::condition_variable wake;
    std::thread worker;
    std::filesystem::path root;
    std::string version, run;
    bool started{}, stopping{}, busy{};
    unsigned reports{};
    std::unique_ptr<Job> pending;
    std::optional<Completion> completion;
};
State state;

std::string utc_now() {
    const auto now=std::chrono::system_clock::now();
    const auto seconds=std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
#ifdef _WIN32
    if (gmtime_s(&utc,&seconds)!=0) return "unavailable";
#else
    if (!gmtime_r(&seconds,&utc)) return "unavailable";
#endif
    char text[32]{};
    if (!std::strftime(text,sizeof(text),"%Y-%m-%dT%H:%M:%SZ",&utc)) return "unavailable";
    return text;
}
std::string safe_version(const std::string& version) {
    if (version.empty() || version.size()>96) return "unavailable";
    for (unsigned char c:version) {
        if (!((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') ||
              c=='.' || c=='_' || c=='-' || c=='+')) return "unavailable";
    }
    return version;
}
// No error string from a filesystem/platform exception is ever exported.
FILE* open_exclusive(const std::filesystem::path& path, bool* created=nullptr) noexcept {
    if (created) *created=false;
#ifdef _WIN32
    int descriptor=-1;
    if (_wsopen_s(&descriptor,path.c_str(),_O_CREAT|_O_EXCL|_O_WRONLY|_O_BINARY,
        _SH_DENYRW,_S_IREAD|_S_IWRITE)!=0) return nullptr;
    if (created) *created=true;
    FILE* file=_wfdopen(descriptor,L"wb");
    if (!file) _close(descriptor);
#else
    const int descriptor=::open(path.c_str(),O_CREAT|O_EXCL|O_WRONLY|O_CLOEXEC,0600);
    if (descriptor<0) return nullptr;
    if (created) *created=true;
    FILE* file=fdopen(descriptor,"wb");
    if (!file) ::close(descriptor);
#endif
    return file;
}
class BoundedWriter {
    FILE* file;
    size_t limit;
public:
    uint64_t bytes{};
    const char* error="";
    explicit BoundedWriter(FILE* file, size_t limit=report_max_bytes):file(file),limit(limit) {}
    bool append(const Json& value) {
        if (*error) return false;
        std::string line=value.dump();
        line+='\n';
        // Normal rows cannot spend the footer reserve or partially enter the report.
        const auto row_limit=limit>256 ? limit-256 : 0;
        if (line.size()>row_limit || bytes>row_limit-line.size()) { error="byte_cap"; return false; }
        const auto written=std::fwrite(line.data(),1,line.size(),file);
        bytes+=written;
        if (written!=line.size()) { error="file_write"; return false; }
        return true;
    }
    bool finish(bool success, const char* failure) {
        // Fixed codes only. Even allocation failure can append this footer without allocating.
        char line[192];
        const auto length=std::snprintf(line,sizeof(line),
            "{\"type\":\"completion\",\"complete\":%s,\"error\":\"%s\"}\n",success ? "true" : "false",failure);
        if (length<0 || size_t(length)>=sizeof(line) || size_t(length)>limit || bytes>limit-size_t(length)) {
            error="byte_cap"; return false;
        }
        const auto written=std::fwrite(line,1,size_t(length),file);
        bytes+=written;
        if (written!=size_t(length)) { error="file_write"; return false; }
        return true;
    }
};

template<class Record> Json header(const char* type, const Record& r) {
    return Json{{"type",type},{"sequence",r.sequence},{"time_us",r.time_us}};
}
Json row(const FrameRecord& r) {
    auto j=header("frame",r);
    // Schema 2: an unauthenticated source is null, never a zero allocator cursor. Schema 3: the legacy
    // player words likewise need LegacyPlayer, and the secondary group SecondaryRoute.
    const bool layout=r.valid&DrawRecord::Layout, cpu625=layout && (r.valid&FrameRecord::Cpu625),
        player=layout && (r.valid&FrameRecord::LegacyPlayer),
        secondary=player && (r.valid&FrameRecord::SecondaryRoute);
    const auto known=[](bool valid,uint32_t value) { return valid ? Json(value) : Json(nullptr); };
    j.update(Json{{"phase",r.phase},{"epoch",r.epoch},{"map",r.map},{"pool_index",r.pool_index},
        {"view",r.view},{"camera",r.camera},{"projection",r.projection},{"camera_active",r.camera_active},
        {"camera_latch",r.camera_latch},{"bridge_poisoned",r.bridge_poisoned},{"control_slot",r.control_slot},
        {"player_owner",known(player,r.player_owner)},{"player_model",known(player,r.player_model)},
        {"player_asset",known(player,r.player_asset)},{"player_admissions",known(player,r.player_admissions)},
        {"player_refusals",known(player,r.player_refusals)},
        {"requested_period",r.requested_period},{"measured_period",r.measured_period},
        {"live_period",r.live_period},{"valid",r.valid}});
    j.update(Json{{"player_last_reason",known(player,r.player_last_reason)},
        {"bridge_next_block",known(layout,r.bridge_next_block)},
        {"cpu625_next_block",known(cpu625,r.cpu625_next_block)}});
    // Optional (PC[8] layout 4): last admitted occurrence view and ledger.
    if (player)
        j["player_primary"]=Json{{"view",r.player_view},{"ledger_bytes",r.player_primary_bytes},
            {"ledger_highwater",r.player_primary_highwater}};
    // Optional (PC[8] layout 5): secondary route provenance and ledger, only where that route is selected outside BK.
    if (secondary)
        j["secondary_route"]=Json{{"opens",r.secondary_opens},{"faults",r.secondary_faults},
            {"seen_main",r.secondary_seen_main},{"seen_late",r.secondary_seen_late},
            {"unauthenticated",r.secondary_unauthenticated},{"admissions",r.secondary_admissions},
            {"ledger_bytes",r.secondary_ledger_bytes}};
    // Native guest-thread snapshot, cumulative within this process. Last values are not per-root evidence.
    j["menu_admission"]=Json{{"counts",r.menu.counts},{"bk_frame",r.menu.bk_frame},
        {"owner",r.menu.owner},{"gfx",r.menu.gfx},{"box",r.menu.box},{"slot",r.menu.slot},
        {"role",r.menu.role},{"id",r.menu.id},{"resource",r.menu.resource},{"token",r.menu.token},
        {"live_slots",r.menu.live_slots},{"active_valid",r.menu.active_valid},
        {"gate",r.menu.gate},{"model_box",r.menu.model_box},{"model_gfx",r.menu.model_gfx},
        {"dialog_gfx",r.menu.dialog_gfx},{"group_delta",r.menu.group_delta},
        {"root_delta",r.menu.root_delta},{"graph_group_delta",r.menu.graph_group_delta},
        {"init_owner",r.menu.init_owner},{"free_owner",r.menu.free_owner},{"registered",r.menu.registered}};
    if(r.shadow_branch_valid)
        j["shadow_branch"]=Json{{"bk_frame",r.shadow_branch_frame},{"cuts",r.shadow_cuts},{"declined",r.shadow_declined}};
    if(r.transition_fade.attempts){
        const auto& t=r.transition_fade;
        j["transition_fade"]=Json{{"valid",t.valid},{"bk_frame",t.frame},{"view",t.view},{"site",t.site},
            {"object",t.object},{"kind",t.kind},{"flags",t.flags},{"elapsed_word",t.elapsed_word},
            {"duration_word",t.duration_word},{"gfx",t.gfx},{"begin",t.begin},{"end",t.end},
            {"attempts",t.attempts},{"refused",t.refused},{"mixed_views",t.mixed_views},{"commands",t.commands}};
    }
    // Optional, schema-2 compatible: present only on an authenticated End sample.
    if (r.shadow_valid)
        j["shadow"]=Json{{"source_pc",r.shadow_source_pc},{"seen_epoch",r.shadow_seen_epoch},
            {"id",r.shadow_id},{"group",r.shadow_group},{"composite",r.shadow_composite},
            {"mode_open",r.shadow_mode_open},{"fault",r.shadow_fault},{"args",r.shadow_args}};
    return j;
}
Json row(const DrawRecord& r) {
    auto j=header("draw",r);
    j.update(Json{{"epoch",r.epoch},{"buffers",r.buffers},{"cursor",r.cursor},{"model",r.model},
        {"asset",r.asset},{"pool_index",r.pool_index},{"root_address",r.root_address},{"actor",r.actor},
        {"descriptor",r.descriptor},{"slot",r.slot},{"birth",r.birth},{"view",r.view},
        {"context_epoch",r.context_epoch},{"context_admitted",r.context_admitted},{"camera",r.camera},
        {"camera_birth",r.camera_birth},{"camera_revision",r.camera_revision},{"camera_id",r.camera_id},
        {"camera_scale",r.camera_scale},{"camera_policy",r.camera_policy},
        {"projection_address",r.projection_address},{"valid",r.valid},{"root",r.root},{"projection",r.projection}});
    // Schema 2 join groups appear only with their bit, so unjoined rows add no bytes.
    if (r.valid&DrawRecord::PlayerJoin) {
        const bool slot=r.valid&DrawRecord::PlayerSlot;
        j.update(Json{{"player_owner",r.player_owner},
            {"player_owner_slot",slot ? Json(r.player_owner_slot) : Json(nullptr)},
            {"player_table_match",slot ? Json(r.player_table_match) : Json(nullptr)},
            {"player_control_slot",r.player_control_slot},{"player_outcome",r.player_outcome}});
    }
    if (r.valid&DrawRecord::SkeletalJoin)
        j.update(Json{{"skeletal_calls",r.skeletal_calls},{"skeletal_admissions",r.skeletal_admissions},
            {"skeletal_refusals",r.skeletal_refusals},
            {"skeletal_reason",r.skeletal_refusals ? Json(r.skeletal_reason) : Json(nullptr)}});
    return j;
}
Json row(const BkDrawRecord& r) {
    // A BK root occurrence, never a legacy DEFA0 transport or admission. Each group appears only
    // with its bit; a word left behind without its bit serializes as null.
    auto j=header("bk_draw",r);
    const auto has=[&](uint32_t bit) { return (r.valid&bit)!=0; };
    const auto when=[&](uint32_t bit,uint32_t value) { return has(bit) ? Json(value) : Json(nullptr); };
    const char* provenance="unproven";
    if (has(BkDrawRecord::Owner) && has(BkDrawRecord::Route))
        provenance=has(BkDrawRecord::Roster) && r.drawn_table_match ? "callback_staged_roster_bound" : "callback_staged_unbound";
    else if (has(BkDrawRecord::Owner) && r.owner_kind==1) provenance="actor_birth"; // TOOIE_BK_OWNER_ACTOR
    j.update(Json{{"source",r.source==BkDrawRecord::Root ? "bk_root" : "unknown"},{"valid",r.valid},
        {"provenance",provenance},{"epoch",when(BkDrawRecord::Frame,r.epoch)},{"bk_frame",r.bk_frame},
        {"root_serial",r.root_serial},{"view",r.view},{"phase",r.phase},{"sp",r.sp},{"buffers",r.buffers},
        {"model",r.model},{"asset",r.asset},{"depth",r.depth},{"draw_depth",r.draw_depth},
        {"owner",Json{{"kind",r.owner_kind},{"birth",r.owner_birth},{"view",r.owner_view},{"phase",r.owner_phase},
            {"base_attachment",r.owner_base},{"attachments",r.owner_attachments},{"id",r.owner_id}}},
        {"ids",Json{{"current",r.current_id},{"emitted",r.emitted_id},{"offset",r.offset},
            {"uses_bones",r.uses_bones},{"push",r.push},{"skip_all",r.skip_all},{"skip_model",r.skip_model},
            {"domain",r.id_domain},{"attachment",r.id_attachment},{"sample",r.id_sample}}},
        {"producer",r.producer_site ? Json{{"site",r.producer_site},{"model",r.producer_model}} : Json(nullptr)},
        {"caller",has(BkDrawRecord::Caller) ? Json{{"position",r.caller_position},
            {"actor",r.caller_actor ? Json(r.caller_actor) : Json(nullptr)},
            {"pair",r.caller_actor ? Json(r.caller_pair) : Json(nullptr)}} : Json(nullptr)},
        {"root_witness",has(BkDrawRecord::RootWitness) ? Json{
            {"capture",Json{{"key",r.root_witness[0]},{"command",r.root_witness[1]},
                {"deform",r.root_witness[2]},{"header",r.root_witness[3]},{"pose",r.root_witness[4]},
                {"entry",r.root_witness[5]},{"valid",r.root_witness[6]},{"model",r.root_witness[7]}}},
            {"history_before",Json{{"id",r.root_witness[8]},{"frame",r.root_witness[9]},
                {"model",r.root_witness[10]},{"start",r.root_witness[11]}}},
            {"float_frame",r.root_witness[12]},{"draw_id_before",r.root_witness[13]},
            {"segment_after",Json{{"position",r.root_witness[14]},{"velocity",r.root_witness[15]},
                {"base",r.root_witness[16]},{"enabled",r.root_witness[17]}}},
            {"selector_reserved",r.root_witness[18]},{"selector_declined_delta",r.root_witness[19]}
        } : Json(nullptr)},
        {"staged_raw",has(BkDrawRecord::Staged) ?
            Json{{"callback",r.staged_callback},{"owner",r.staged_owner}} : Json(nullptr)},
        {"drawn_owner",when(BkDrawRecord::Route,r.drawn_owner)},
        {"drawn_slot",when(BkDrawRecord::Roster,r.drawn_slot)},
        {"drawn_table_match",when(BkDrawRecord::Roster,r.drawn_table_match)},
        {"control_slot",when(BkDrawRecord::Control,r.control_slot)},
        {"control_owner",when(BkDrawRecord::ControlOwner,r.control_owner)},
        {"end",has(BkDrawRecord::End) ? Json{{"offset",r.end_offset},{"uses_bones",r.end_uses_bones},
            {"groups",r.groups},{"offset_overflows",r.offset_overflows}} : Json(nullptr)},
        {"unclosed",has(BkDrawRecord::Unclosed)},
        {"session",Json{{"late_closes",r.late_closes},{"root_overflows",r.root_overflows},
            {"scope_overflows",r.scope_overflows}}}});
    return j;
}
Json row(const HostRecord& r) {
    auto j=header("host",r);
    j.update(Json{{"width",r.width},{"height",r.height},{"wide",r.wide},{"aspect",r.aspect},
        {"cutscene_aspect",r.cutscene_aspect},{"resolution",r.resolution},{"output_rate",r.output_rate},
        {"manual_hz",r.manual_hz},{"vsync",r.vsync},{"texture_filter",r.texture_filter},
        {"output_filter",r.output_filter},{"published_aspect",r.published_aspect},
        {"guest_publication",r.guest_publication},{"buttons",r.buttons},{"flags",r.flags},
        {"stick_x",r.stick_x},{"stick_y",r.stick_y}});
    return j;
}
Json row(const GraphicsRecord& r) {
    auto j=header("graphics",r);
    j.update(Json{{"kind",r.kind},{"task_sequence",r.task_sequence},{"data_ptr",r.data_ptr},
        {"data_size",r.data_size},{"swap_chain_rate",r.swap_chain_rate},{"resolution",r.resolution},
        {"output_rate",r.output_rate},{"manual_hz",r.manual_hz},
        {"texture_filter",r.texture_filter},{"output_filter",r.output_filter}});
    return j;
}
Json row(const PublicationRecord& r) {
    auto j=header("publication",r);
    j.update(Json{{"generation",r.generation},{"flags",r.flags}});
    return j;
}
// Player rows: f32 matrix words as 16 space-separated hex words (row-major), exact including NaN.
std::string hex16(const uint32_t (&words)[16]) {
    std::string text;
    text.reserve(16*9);
    char word[9];
    for (int i=0; i<16; ++i) {
        std::snprintf(word,sizeof(word),"%08x",words[i]);
        if (i) text+=' ';
        text+=word;
    }
    return text;
}
// BK ID arithmetic only (bk_pipeline.h), decoded as the guest reader does; never a bone identity.
Json player_label(uint32_t id) {
    uint32_t slot=0, relative=0;
    if (id>=TOOIE_BK_PLAYER_ID_START && id<TOOIE_BK_PLAYER_ID_END) relative=id-TOOIE_BK_PLAYER_ID_START;
    else if (id>=TOOIE_BK_PLAYER_SLOT_ID_START && id<TOOIE_BK_PLAYER_SLOT_ID_END) {
        slot=1+(id-TOOIE_BK_PLAYER_SLOT_ID_START)/TOOIE_BK_PLAYER_SLOT_STRIDE;
        relative=(id-TOOIE_BK_PLAYER_SLOT_ID_START)%TOOIE_BK_PLAYER_SLOT_STRIDE;
    }
    else return nullptr;
    return Json{{"roster_slot",slot},{"attachment",relative/TOOIE_BK_OWNER_STRIDE},
        {"sample",relative%TOOIE_BK_OWNER_STRIDE/TOOIE_BK_TRANSFORM_ID_COUNT},
        {"offset",relative%TOOIE_BK_TRANSFORM_ID_COUNT}};
}
Json row(const PlayerTransformRecord& record) {
    using Row=RT64::TooiePlayerTransform;
    const auto& r=record.row;
    const auto has=[&](uint32_t bit) { return (r.flags&bit)!=0; };
    auto j=header("player_transform",record);
    j.update(Json{{"presentation",r.presentation},{"flags",r.flags},
        {"workload",Json{{"slot",r.workload_slot},{"id",r.workload_id}}},
        {"prior_workload",Json{{"recorded_slot",has(Row::OwnerMapped) ? Json(r.previous_slot) : Json(nullptr)},
            {"valid",has(Row::PriorSlotValid)},
            {"id",has(Row::PriorSlotValid) ? Json(r.previous_workload_id) : Json(nullptr)}}},
        {"weight",r.weight},{"previous_weight",r.previous_weight},
        {"index",r.index},{"group_id",r.group_id},{"label",player_label(r.group_id)},
        {"representative",has(Row::Representative)},{"auto_ortho",has(Row::AutoOrtho)},
        {"unrooted_ortho",has(Row::UnrootedOrtho)},{"vertex_skip",has(Row::VertexSkip)},
        {"vertex_delta",has(Row::VertexMeasured) && r.vertex_delta >= 0.0f && std::isfinite(r.vertex_delta) ? Json(r.vertex_delta) : Json(nullptr)},
        {"vertex_delta_nonfinite",has(Row::VertexMeasured) && !std::isfinite(r.vertex_delta)},
        {"group_matrix_count",has(Row::Representative) || has(Row::AutoOrtho) ? Json(r.group_matrix_count) : Json(nullptr)},
        {"vertices",r.vertices},{"physical",r.physical},{"mapped",has(Row::Mapped)},
        {"previous_index",has(Row::Mapped) ? Json(r.previous_index) : Json(nullptr)},
        {"prior_index_valid",has(Row::PriorIndexValid)},
        {"previous_group_id",has(Row::PriorIndexValid) ? Json(r.previous_group_id) : Json(nullptr)},
        {"previous_vertices",has(Row::PriorIndexValid) ? Json(r.previous_vertices) : Json(nullptr)},
        {"projection",has(Row::ProjectionKnown) ? Json(r.projection) : Json(nullptr)},
        {"raw",hex16(r.raw)},
        {"prior",has(Row::PriorIndexValid) ? Json(hex16(r.prior)) : Json(nullptr)},
        {"interpolated",has(Row::Interpolated) ? Json(hex16(r.interpolated)) : Json(nullptr)}});
    if(r.transition_source.flags & RT64::TooieTransitionSourceDraw::Observed) {
        const auto& o=r.transition_source;using O=RT64::TooieTransitionSourceDraw;
        j["transition_source_draw"]=Json{{"flags",o.flags},{"calls",o.calls},{"triangles",o.triangles},
            {"pose","current_raw_world"},{"projection",(o.flags&O::ProcessedProjection)?"processed":"raw"},
            {"source_scissor_first_q4",(o.flags&O::Source)?Json(o.scissor):Json(nullptr)},
            {"source_viewport_first_q4",(o.flags&O::Source)?Json(o.viewport):Json(nullptr)},
            {"current_pose_rsp_ndc_xy",(o.flags&O::Bounds)?Json(o.ndc):Json(nullptr)}};
    }
    return j;
}
Json row(const PlayerProjectionRecord& record) {
    using Row=RT64::TooiePlayerProjection;
    const auto& r=record.row;
    const auto has=[&](uint32_t bit) { return (r.flags&bit)!=0; };
    auto j=header("player_projection",record);
    j.update(Json{{"presentation",r.presentation},{"flags",r.flags},
        {"workload",Json{{"slot",r.workload_slot},{"id",r.workload_id}}},
        {"prior_workload",Json{{"recorded_slot",has(Row::OwnerMapped) ? Json(r.previous_slot) : Json(nullptr)},
            {"valid",has(Row::PriorSlotValid)},
            {"id",has(Row::PriorSlotValid) ? Json(r.previous_workload_id) : Json(nullptr)}}},
        {"weight",r.weight},
        {"index",r.index},{"group_id",has(Row::GroupKnown) ? Json(r.group_id) : Json(nullptr)},
        {"mapped",has(Row::Mapped)},
        {"previous_index",has(Row::Mapped) ? Json(r.previous_index) : Json(nullptr)},
        {"previous_group_id",has(Row::PriorIndexValid) ? Json(r.previous_group_id) : Json(nullptr)},
        {"raw",hex16(r.raw)},
        {"prior",has(Row::PriorIndexValid) ? Json(hex16(r.prior)) : Json(nullptr)},
        {"final",has(Row::Final) ? Json(hex16(r.final)) : Json(nullptr)}});
    // Warp-UI witness: one RT64 orthographic scene's representative projection.
    if (has(Row::OrthoScene))
        j["ortho_scene"]=Json{{"scene",r.scene&0xFFFFu},{"representative_position",r.scene>>16},
            {"projections",r.scene_projections},{"scenes",r.scene_counts&0xFFFFu},
            {"previous_scenes",r.scene_counts>>16},{"census",has(Row::SceneCensus)},
            {"truncated_after",has(Row::SceneTruncated)},{"cross_workload",has(Row::SceneCrossWorkload)},
            {"paired",has(Row::PairKnown) ? Json{
                {"previous_scene",r.paired_scene==UINT32_MAX ? Json(nullptr) : Json(r.paired_scene)},
                {"first_group_id",r.paired_scene==UINT32_MAX ? Json(nullptr) : Json(r.paired_group_id)}} : Json(nullptr)}};
    return j;
}
Json row(const sound_trace::Event& r) {
    auto j=header("audio",r);
    j.update(Json{{"kind",static_cast<unsigned>(r.kind)},{"a",r.a},{"b",r.b},{"aux",r.aux}});
    return j;
}
template<class Record> bool write_domain(BoundedWriter& writer, const char* name,
    const Snapshot<Record>& snapshot, size_t capacity, const char* validity) {
    uint64_t invalid_retained=0;
    for (const auto& event:snapshot.events) invalid_retained+=invalid(event);
    const auto first=snapshot.events.empty() ? 0 : snapshot.events.front().time_us;
    const auto last=snapshot.events.empty() ? 0 : snapshot.events.back().time_us;
    if (!writer.append(Json{{"type","domain"},{"domain",name},{"capacity",capacity},
        {"observed_total",snapshot.written},{"invalid_total",snapshot.invalid_total},
        {"invalid_retained",invalid_retained},{"validity",validity},
        {"record_failures",snapshot.record_failures},
        {"overwritten_total",snapshot.overwritten},{"capacity_truncated",snapshot.truncated},
        {"window_us",window_us},{"end_us",snapshot.end_us},{"first_us",first},{"last_us",last},
        {"retained",snapshot.events.size()},{"retained_span_us",last-first}})) return false;
    for (const auto& event:snapshot.events) if (!writer.append(row(event))) return false;
    return true;
}
bool write_audio(BoundedWriter& writer, const sound_trace::Snapshot& audio) {
    const auto first=audio.events.empty() ? 0 : audio.events.front().time_us;
    const auto last=audio.events.empty() ? 0 : audio.events.back().time_us;
    // Existing audio ring has no validity predicate or lifetime observed counter.
    // overwritten + retained is not that counter once old retained rows expire.
    if (!writer.append(Json{{"type","domain"},{"domain","audio"},{"capacity",sound_trace::capacity},
        {"observed_total",nullptr},{"invalid_total",nullptr},{"validity","not_instrumented"},
        {"overwritten_total",audio.overwritten},{"capacity_truncated",audio.truncated},
        {"window_us",sound_trace::window_us},{"end_us",audio.end_us},{"first_us",first},{"last_us",last},
        {"retained",audio.events.size()},{"retained_span_us",last-first},{"frequency",audio.frequency},
        {"clock_rate",audio.clock_rate},{"clock_revision",audio.clock_revision}})) return false;
    for (const auto& event:audio.events) if (!writer.append(row(event))) return false;
    return true;
}
Json coverage() {
    return Json{{"type","coverage"},
        {"unavailable",{"full_bone_pose","per_helper_admission","collision","surface_texture_selection",
            "all_camera_views","ui_draw_internals","rt64_present_timing"}},
        {"limits",{"independent_domain_snapshots_not_atomic","defa0_selected_transport_only",
            "context_is_owner_match_not_semantic_admission","matrix_words_are_fixed_n64_not_rt64_output",
            "input_is_mapped_sdl_actions_not_guest_consumption","graphics_times_are_cpu_callbacks",
            "no_footer_or_incomplete_footer_means_incomplete","empty_domain_is_no_observation",
            "player_last_reason_is_latest_begin_call_not_per_frame_or_per_owner",
            "player_tuple_fields_are_last_admitted_occurrence_any_owner_or_view",
            "player_tuple_fields_include_admitted_secondary_route_occurrences",
            "secondary_route_counters_are_session_cumulative_ledger_is_per_construction",
            "legacy_player_words_null_or_absent_unless_legacy_player_bit_boundary_enabled",
            "secondary_route_absent_unless_selected_outside_bk_not_runtime_installation_proof",
            "next_block_is_allocator_cursor_not_live_identity_count",
            "player_join_is_first_depth1_defa0_after_its_begin_call",
            "skeletal_join_is_npc_skeletal_admit_only_not_cpu_root_or_cpu625",
            "unjoined_draw_is_unknown_not_refused",
            "shadow_is_authenticated_frame_end_state_not_per_draw_events",
            "shadow_source_pc_is_last_callsite_in_frame_zero_none",
            "shadow_args_persist_until_the_next_rectangle_call",
            "shadow_branch_cuts_and_declined_are_session_cumulative_not_per_root_reasons",
            "menu_admission_is_native_guest_thread_cumulative_counters_and_last_values_not_per_root",
            "menu_counts_order_reset_init_initrefused_free_enter_unregistered_invocationlimit_invalid_box_boxinvalid_modelscope_gaterefused_modelcall_identityrefused_resourceabsent_tokenrefused_tokenaccepted_modelend_rootgroupdelta_modeloutsideoption_transitionmodel_transitiontagged_transitioncut_transitionrefused_enteradmitted_entergapcut_textscope_textpushed_textowned_textadmitted_textconverted",
            "bk_draw_is_cpu_emitted_root_tags_not_rt64_consumed_mapping_or_pixels",
            "transition_fade_is_last_source_authenticated_cpu_attempt_per_construction_not_pixels_or_effective_scissor",
            "transition_fade_multiple_attempts_overwrite_last_values_mixed_views_marks_ambiguity",
            "bk_root_witness_scalar_before_after_not_consumed_vertices",
            "bk_segment_pointers_valid_only_when_enabled_1",
            "bk_history_before_requires_matching_id_frame_model",
            "bk_model_is_observed_address_not_semantic_name",
            "bk_caller_is_unowned_top_level_de498_entry_a1_keyed_by_frame_not_return_address_or_owner",
            "bk_caller_actor_pair_only_for_live_pooled_actor_at_exact_position_minus4",
            "bk_drawn_owner_is_callback_staged_candidate_not_authenticated_producer_owner",
            "bk_staged_raw_is_c948_snapshot_not_owner_proof",
            "bk_control_slot_is_independent_of_drawn_owner",
            "bk_fields_are_not_legacy_admission",
            "bk_unclosed_root_had_no_matching_native_end_or_join",
            "bk_end_groups_include_nested_roots",
            "bk_id_decode_uses_current_id_emitted_root_id_only_when_push_1",
            "player_rows_are_rt64_cpu_draw_data_and_frame_map_after_processing_not_gpu_or_pixels",
            "player_rows_join_guest_rings_by_time_only_no_guest_task_or_bk_frame_join",
            "player_presentation_is_hook_call_ordinal_joining_player_transform_and_projection_rows",
            "legacy_player_transform_and_projection_names_include_supplemental_owned_roots",
            "players_collected_first_all_workloads_then_at_most64_actor_or_menu_model_roots",
            "shared_presentation_cap1024_transform_ring32768_projection_cap8_unchanged",
            "supplemental_roots_use_remaining_capacity_and_shorten_ring_time_retention",
            "actor_supplement_domain50000000_90000000_root_low8_zero_bones_excluded",
            "menu_supplement_proposed_domain30000000_30060000_model_and_sprite256_blocks_root_only",
            "transition_supplement_domain30060000_30060500_all_five256_blocks_root_only_shared64_cap",
            "untagged_id0_excluded_shadow_roots_in_shared_supplement_no_pointer_to_object_inference",
            "root_id_ordinal0_can_repeat_for_list_identity_matrix_keep_workload_and_index",
            "root_group_id_correlates_bk_draw_id_with_time_not_unique_matrix_or_exact_guest_frame",
            "player_label_is_only_for_player_id_arithmetic_not_actor_or_menu_or_bone_identity",
            "player_projection_is_first_owned_vertex_in_allocation_range_mixed_and_incomplete_flagged",
            "player_vertices_is_allocation_range_count_not_exact_drawn_count_zero_may_be_unavailable",
            "player_projection_rows_include_other_valid_owned_projections_in_that_range",
            "player_processed_flags_distinguish_raw_path_from_missing_processed_output",
            "player_rows_per_presentation_1024_projections_8_truncation_flagged",
            "player_prior_and_interpolated_absent_unless_flagged",
            "ortho_scene_rows_follow_player_projections_own_cap16_per_presentation_truncated_after_flagged",
            "ortho_scene_census_is_first_presentation_with_a_new_first_rt64_workload_id_not_bk_frame",
            "ortho_scene_census_lists_every_scene_in_order_other_presentations_only_interpolating_auto_scenes",
            "ortho_scene_representative_is_first_projection_mapped_in_its_own_workload_map_else_first",
            "ortho_scene_prior_is_the_index_rt64_processing_reads_in_the_recorded_prior_workload",
            "ortho_scene_pairing_recomputed_from_rt64_matchscenes_inputs_up_to32x32_absent_means_not_claimed",
            "auto_ortho_rows_are_census_only_own_cap8_after_player_rows_truncated_after_flagged",
            "auto_ortho_includes_auto_ignore_and_explicit_nonroot_nonplayer_parts_on_orthographic_scene_projections",
            "auto_ortho_rank_is_max_translation_and_modelspace_vertex_delta_nonfinite_first_unmapped_last_draw_order_ties",
            "auto_ortho_group_matrix_count_is_eligible_transforms_in_that_workload"}},
        {"shadow_semantics",{{"source_local",0x800A6CF0u},{"source_fallback",0x800A6D60u},
            {"admitted_this_frame","seen_epoch_equals_epoch"},{"fullframe_composite_args",{0,0,304,228}},
            {"nonzero_id_or_group_at_end","admitted_shadow_did_not_reach_rectangle_or_complete"}}},
        {"projection_semantics","last_saved_world_production"},
        {"projection_not_observed",{"effective_projection_stack","per_model_projection"}},
        {"host_stick_units","Q15"},
        {"host_enum_domain","player_preferences"},
        {"graphics_enum_domain","actual_rt64"},
        {"footer_scope","serialized_rows_complete_not_durable_persistence"},
        {"frame_valid_bits",{{"layout",1},{"frame",2},{"cpu625",4},{"legacy_player",8},{"secondary_route",16}}},
        {"draw_valid_bits",{{"layout",1},{"frame",2},{"root",4},{"projection",8},{"context",16},
            {"player_join",32},{"player_slot",64},{"skeletal_join",128}}},
        {"bk_draw_valid_bits",{{"layout",1},{"frame",2},{"owner",4},{"staged",8},{"route",16},{"roster",32},
            {"control",64},{"control_owner",128},{"end",256},{"unclosed",512},{"root_witness",1024},{"caller",2048}}},
        {"bk_draw_provenance",{"callback_staged_roster_bound","callback_staged_unbound","actor_birth","unproven"}},
        {"host_flag_bits",{{"focused",1},{"menu",2},{"suppressed",4},{"game_enabled",8},
            {"withheld",16},{"fullscreen",32}}},
        {"graphics_kinds",{{"parse_begin",1},{"parse_end",2},{"screen_begin",3},{"screen_end",4},
            {"dummy_begin",5},{"dummy_end",6},{"config_applied",7}}},
        {"player_transform_flag_bits",{{"owner_mapped",1},{"prior_slot_valid",2},{"mapped",4},
            {"prior_index_valid",8},{"interpolated",16},{"current_matched",32},{"previous_matched",64},
            {"paused",128},{"projection_known",256},{"projection_mixed",512},{"lerp_decompose",1024},
            {"lerp_translation",2048},{"lerp_rotation",4096},{"lerp_scale",8192},{"lerp_skew",16384},
            {"truncated_after",32768},{"projection_truncated",65536},{"transforms_processed",131072},
            {"projection_incomplete",262144},{"representative",524288},{"auto_ortho",1048576},{"unrooted_ortho",2097152},{"vertex_skip",4194304},{"vertex_measured",8388608}}},
        {"player_projection_flag_bits",{{"owner_mapped",1},{"prior_slot_valid",2},{"mapped",4},
            {"prior_index_valid",8},{"final",16},{"group_known",32},{"lerp_decompose",1024},
            {"lerp_translation",2048},{"lerp_rotation",4096},{"projections_processed",65536},
            {"ortho_scene",131072},{"scene_census",262144},{"scene_truncated_after",524288},
            {"scene_cross_workload",1048576},{"pair_known",2097152}}},
        {"transition_source_draw_flag_bits",{{"observed",1},{"current_raw_world_pose",2},{"first_source_rects",4},
            {"qualified_current_pose_rsp_ndc_xy",8},{"invalid_input",16},{"scan_truncated",32},{"mixed_triangle_ownership",64},
            {"mixed_projection",128},{"modified_screen_vertex_or_modify_cap",256},{"invalid_or_nonpositive_w",512},
            {"mixed_source_rects_first_retained",1024},{"custom_origin_not_applied",2048},{"processed_projection",4096}}},
        {"transition_source_draw_semantics","indexed RSP triangles; current raw world pose with selected CPU projection; RSP NDC y=-clip_y/w; no velocity, clipping, raster conversion or opaque coverage"},
        {"player_matrix_encoding","f32_bits_hex_row_major"}};
}
struct FileCloser { void operator()(FILE* file) const noexcept { if(file) std::fclose(file); } };
Completion export_job(const Job& job, const std::filesystem::path& root,
    const std::string& run, const std::string& version, const std::string& hash, const char* hash_error) {
    Completion result{};
    result.marker=job.marker;
    // If a collision exists (including a process restart), claim another basename.
    // Exclusive creation is the only operation; existing files are never opened.
    std::unique_ptr<FILE,FileCloser> file;
    for (unsigned attempt=0; attempt<64; ++attempt) {
        const auto filename="issue-evidence-"+run+"-"+std::to_string(job.ordinal)+"-"+
            std::to_string(attempt)+".jsonl";
        result.filename=filename; // Allocate the reported basename before creating any file.
        bool created=false;
        file.reset(open_exclusive(root/filename,&created));
        if (file) break;
        // A descriptor-to-stream failure already owns one incomplete file. Do not
        // create another: at most one new filename is owned by each accepted job.
        if (created) { result.error="file_stream"; return result; }
        result.filename.clear();
        std::error_code error;
        if (!std::filesystem::exists(root/filename,error)) break;
    }
    if (!file) { result.error="file_create"; return result; }
    // Keep enough of the hard cap for the completion footer, even on a byte failure.
    BoundedWriter writer(file.get());
    bool ok=false;
    const char* failure="";
    try {
        ok=writer.append(Json{{"type","report"},{"schema",3},{"run",run},{"marker",job.marker},
            {"ordinal",job.ordinal},{"utc",job.utc},{"trigger_us",job.trigger_us},{"version",version},
            {"clock","steady_microseconds"},{"executable_sha256_file_on_disk",hash},
            {"executable_hash_error",hash_error},{"ring_bytes",ring_bytes},
            {"player_ring_bytes",player_ring_bytes},
            {"record_bytes",{{"frame",sizeof(FrameRecord)},{"draw",sizeof(DrawRecord)},
                {"host",sizeof(HostRecord)},{"graphics",sizeof(GraphicsRecord)},{"publication",sizeof(PublicationRecord)},
                {"bk_draw",sizeof(BkDrawRecord)},{"player_transform",sizeof(PlayerTransformRecord)},
                {"player_projection",sizeof(PlayerProjectionRecord)}}},
            {"window_us",window_us},{"max_report_bytes",report_max_bytes},{"max_reports_per_run",report_limit}});
        if (ok) ok=writer.append(coverage());
        if (ok) ok=write_domain(writer,"frame",job.frames,frame_capacity,"missing_layout_or_frame_is_incomplete");
        if (ok) ok=write_domain(writer,"draw",job.draws,draw_capacity,"missing_valid_bits_is_incomplete");
        if (ok) ok=write_domain(writer,"bk_draw",job.bk_draws,bk_draw_capacity,"missing_layout_or_owner_is_incomplete");
        if (ok) ok=write_domain(writer,"host",job.hosts,host_capacity,"not_applicable");
        if (ok) ok=write_domain(writer,"graphics",job.graphics,graphics_capacity,"unknown_kind_is_invalid");
        if (ok) ok=write_domain(writer,"publication",job.publications,publication_capacity,"not_applicable");
        if (ok) ok=write_audio(writer,job.audio);
        // Player domains last: every existing domain is written ahead of them under the same cap.
        if (ok) ok=write_domain(writer,"player_transform",job.player_transforms,player_transform_capacity,
            "flags_declare_absent_fields");
        if (ok) ok=write_domain(writer,"player_projection",job.player_projections,player_projection_capacity,
            "flags_declare_absent_fields");
        failure=ok ? "" : writer.error;
        // Both destinations use job.audio, captured once on F4. The worker does all I/O.
        if (!sound_trace::dump(stderr,job.marker,job.audio) && ok) { ok=false; failure="audio_log"; }
    } catch(const std::bad_alloc&) { ok=false; failure="allocation"; }
      catch(...) { ok=false; failure="export_failed"; }
    // Permit only the reserved footer bytes beyond the normal-row limit.
    // A cap failure has already refused its row in full.
    if (std::fflush(file.get())!=0) { ok=false; failure="file_flush"; }
    if (!writer.finish(ok,failure)) { ok=false; failure=writer.error; }
    if (std::fflush(file.get())!=0) { ok=false; failure="file_flush"; }
    FILE* closing=file.release();
    if (std::fclose(closing)!=0) { ok=false; failure="file_close"; }
    result.bytes=writer.bytes;
    result.success=ok;
    if (!ok) result.error=*failure ? failure : "file_write";
    return result;
}
void publish(Completion completion) {
    std::lock_guard guard(state.mutex);
    state.completion=std::move(completion);
}
void worker_main() noexcept {
    std::string hash;
    const char* hash_error="";
    try { hash=tooie::platform::file_sha256(tooie::platform::executable_path()); }
    catch (...) { hash_error="hash_failed"; }
    for (;;) {
        std::unique_ptr<Job> job;
        try {
            {
                std::unique_lock lock(state.mutex);
                state.wake.wait(lock,[]{return state.stopping || bool(state.pending);});
                if (!state.pending) return;
                job=std::move(state.pending);
            }
            auto completion=export_job(*job,state.root,state.run,state.version,hash,hash_error);
            publish(std::move(completion));
        } catch (const std::bad_alloc&) {
            Completion failure{}; failure.marker=job ? job->marker : 0; failure.error="allocation";
            try { publish(std::move(failure)); } catch (...) {}
        } catch (...) {
            Completion failure{}; failure.marker=job ? job->marker : 0; failure.error="export_failed";
            try { publish(std::move(failure)); } catch (...) {}
        }
    }
}
} // namespace detail

void record(FrameRecord r) noexcept { detail::frames.record(r); }
void record(DrawRecord r) noexcept { detail::draws.record(r); }
void record(BkDrawRecord r) noexcept { detail::bk_draws.record(r); }
void record(HostRecord r) noexcept { detail::hosts.record(r); }
void record(GraphicsRecord r) noexcept { detail::graphics.record(r); }
void record(PublicationRecord r) noexcept { detail::publications.record(r); }
void record_player(const RT64::TooiePlayerTransform* rows, size_t row_count,
    const RT64::TooiePlayerProjection* projections, size_t projection_count) noexcept {
    // One timestamp per presentation: its rows share it exactly (0 lets each ring stamp).
    uint64_t now=0;
    try { now=sound_trace::now_us(); } catch (...) {}
    for (size_t i=0; i<row_count; ++i) {
        PlayerTransformRecord entry{}; entry.row=rows[i];
        detail::player_transforms.record(entry,now);
    }
    for (size_t i=0; i<projection_count; ++i) {
        PlayerProjectionRecord entry{}; entry.row=projections[i];
        detail::player_projections.record(entry,now);
    }
}

void start(const std::filesystem::path& log_root, const std::string& version) noexcept {
    const char* failure_code="start_failed";
    try {
        std::lock_guard guard(detail::state.mutex);
        if (detail::state.started) return;
        detail::state.root=log_root;
        detail::state.version=detail::safe_version(version);
        const auto system_us=std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        detail::state.run=std::to_string(system_us)+"-"+std::to_string(sound_trace::now_us());
        detail::state.stopping=false;
        detail::state.busy=false;
        detail::state.reports=0;
        detail::state.completion.reset();
        detail::state.started=true;
        try { detail::state.worker=std::thread(detail::worker_main); }
        catch (const std::bad_alloc&) { detail::state.started=false; throw; }
        catch (...) { detail::state.started=false; failure_code="thread_start"; throw; }
    } catch (const std::bad_alloc&) {
        try { detail::publish(Completion{0,false,"","allocation",0}); } catch (...) {}
    } catch (...) {
        try {
            Completion failure{}; failure.error=failure_code;
            detail::publish(std::move(failure));
        } catch (...) {}
    }
}
void stop() noexcept {
    try {
        {
            std::lock_guard guard(detail::state.mutex);
            if (!detail::state.started) return;
            detail::state.stopping=true;
        }
        detail::state.wake.notify_one();
        if (detail::state.worker.joinable()) detail::state.worker.join();
        std::lock_guard guard(detail::state.mutex);
        detail::state.started=false;
        detail::state.busy=false;
    } catch (...) {}
}
bool capture(unsigned marker) noexcept {
    try {
        unsigned ordinal{};
        {
            std::lock_guard guard(detail::state.mutex);
            if (detail::state.busy || detail::state.completion) return false;
            const char* failure="";
            if (!detail::state.started || detail::state.stopping) failure="not_started";
            else if (detail::state.reports>=report_limit) failure="report_limit";
            if (*failure) {
                detail::state.completion=Completion{marker,false,"",failure,0};
                return false;
            }
            detail::state.busy=true;
            ordinal=++detail::state.reports;
        }
        auto job=std::make_unique<detail::Job>();
        job->marker=marker;
        job->ordinal=ordinal;
        job->trigger_us=sound_trace::now_us();
        job->utc=detail::utc_now();
        job->audio=sound_trace::ring.snapshot();
        job->frames=detail::frames.snapshot();
        job->draws=detail::draws.snapshot();
        job->bk_draws=detail::bk_draws.snapshot();
        job->hosts=detail::hosts.snapshot();
        job->graphics=detail::graphics.snapshot();
        job->publications=detail::publications.snapshot();
        job->player_transforms=detail::player_transforms.snapshot();
        job->player_projections=detail::player_projections.snapshot();
        {
            std::lock_guard guard(detail::state.mutex);
            if (detail::state.stopping) {
                detail::state.completion=Completion{marker,false,"","stopped",0};
                return false;
            }
            detail::state.pending=std::move(job);
        }
        detail::state.wake.notify_one();
        return true;
    } catch (const std::bad_alloc&) {
        try { detail::publish(Completion{marker,false,"","allocation",0}); } catch (...) {}
        return false;
    } catch (...) {
        try { detail::publish(Completion{marker,false,"","capture_failed",0}); } catch (...) {}
        return false;
    }
}
std::optional<Completion> poll() {
    std::lock_guard guard(detail::state.mutex);
    auto result=std::move(detail::state.completion);
    detail::state.completion.reset();
    if (result) detail::state.busy=false;
    return result;
}
} // namespace baseline::issue_evidence
