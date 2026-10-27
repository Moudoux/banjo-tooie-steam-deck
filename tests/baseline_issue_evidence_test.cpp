// Include the implementation for deterministic, private ring and writer tests.
// This target must not separately compile issue_evidence.cpp.
#include "../tools/baseline/issue_evidence.cpp"
#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace evidence = baseline::issue_evidence;
using Json = nlohmann::json;
static void require(bool ok, const char* why) {
    if (!ok) throw std::runtime_error(why);
}
static evidence::Completion completion() {
    for (int i=0; i<10000; ++i) {
        if (auto value=evidence::poll()) return std::move(*value);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    throw std::runtime_error("worker completion timeout");
}
static std::vector<Json> read_report(const std::filesystem::path& path) {
    std::ifstream file(path);
    require(bool(file), "report readable");
    std::vector<Json> rows;
    std::string line;
    while (std::getline(file,line)) rows.push_back(Json::parse(line));
    require(!rows.empty(), "report has rows");
    return rows;
}
int main(int argc, char** argv) {
    using namespace evidence;
    using namespace evidence::detail;
    try {
        static_assert(sizeof(RT64::TooiePlayerTransform)==336);
        static_assert(sizeof(PlayerTransformRecord)==352);
        std::printf("transition source player_ring_bytes=%zu\n",player_ring_bytes);
        PlayerTransformRecord source_probe{};
        require(!row(source_probe).contains("transition_source_draw"),"ordinary rows unchanged");
        PlayerTransformRecord part_probe{};
        using T=RT64::TooiePlayerTransform;
        part_probe.row.flags=T::AutoOrtho|T::UnrootedOrtho|T::VertexSkip|T::VertexMeasured;
        part_probe.row.vertex_delta=24.0f;
        auto part_json=row(part_probe);
        require(part_json["vertex_delta"]==24.0f && part_json["unrooted_ortho"]==true && part_json["vertex_skip"]==true,
            "ortho part vertex evidence serializes");
        part_probe.row.flags=T::Representative|T::VertexMeasured|T::VertexSkip;
        require(row(part_probe)["vertex_delta"]==24.0f && row(part_probe)["auto_ortho"]==false,
            "supplemental menu vertex measurement serializes without AUTO flag");
        part_probe.row.vertex_delta=-1.0f;
        require(row(part_probe)["vertex_delta"].is_null(),"unavailable vertex delta withheld");
        part_probe.row.vertex_delta=std::numeric_limits<float>::infinity();
        require(row(part_probe)["vertex_delta"].is_null() && row(part_probe)["vertex_delta_nonfinite"]==true,
            "nonfinite vertex delta remains explicit and JSON safe");
        part_probe.row.flags=0;part_probe.row.vertex_delta=24.0f;
        require(row(part_probe)["vertex_delta"].is_null(),"ordinary row does not claim ortho vertex evidence");
        using O=RT64::TooieTransitionSourceDraw;
        source_probe.row.transition_source.flags=O::Observed|O::CurrentPose|O::Invalid;
        auto source_json=row(source_probe)["transition_source_draw"];
        require(source_json["current_pose_rsp_ndc_xy"].is_null() && source_json["source_scissor_first_q4"].is_null(),"invalid source values withheld");
        source_probe.row.transition_source.flags=O::Observed|O::CurrentPose|O::Source|O::Bounds|O::ProcessedProjection;
        source_probe.row.transition_source.calls=1;source_probe.row.transition_source.triangles=2;
        source_probe.row.transition_source.ndc[2]=1.25f;
        source_json=row(source_probe)["transition_source_draw"];
        require(source_json["pose"]=="current_raw_world" && source_json["projection"]=="processed" && source_json["current_pose_rsp_ndc_xy"][2]==1.25f,"source-only semantics and numeric bounds");

        require(safe_version("test-1.2.3+4")=="test-1.2.3+4","build version retained");
        require(safe_version("C:\\private\\profile")=="unavailable" &&
            safe_version("/private/profile")=="unavailable","paths cannot enter version metadata");
        FrameRecord menu_frame{};menu_frame.menu.counts[tooie_menu_witness::TokenAccepted]=3;
        menu_frame.menu.root_delta=1;menu_frame.menu.graph_group_delta=4;
        menu_frame.shadow_branch_valid=1;menu_frame.shadow_branch_frame=42;
        menu_frame.shadow_cuts=7;menu_frame.shadow_declined=3;
        const auto menu_json=row(menu_frame);
        require(menu_json["menu_admission"]["counts"][tooie_menu_witness::TokenAccepted]==3 &&
            menu_json["menu_admission"]["graph_group_delta"]==4,"menu admission counters serialize");
        require(menu_json["shadow_branch"]["cuts"]==7 && menu_json["shadow_branch"]["declined"]==3,
            "existing shadow branch counters serialize independently");
        BkDrawRecord producer_record{};
        require(row(producer_record)["producer"].is_null(),"unobserved producer is absent");
        producer_record.producer_site=0x800BE90Cu;producer_record.producer_model=0x80600000u;
        require(row(producer_record)["producer"]["site"]==0x800BE90Cu &&
            row(producer_record)["producer"]["model"]==0x80600000u,"exact producer witness serializes");
        BkDrawRecord caller_record{};
        require(row(caller_record)["caller"].is_null(),"unjoined caller is absent");
        caller_record.valid=BkDrawRecord::Caller;caller_record.caller_position=0x807FD200u;
        require(row(caller_record)["caller"]["position"]==0x807FD200u && row(caller_record)["caller"]["actor"].is_null() &&
            row(caller_record)["caller"]["pair"].is_null(),"a position without a live actor names no owner");
        caller_record.caller_actor=0x80200000u;caller_record.caller_pair=0x06651234u;
        require(row(caller_record)["caller"]["actor"]==0x80200000u && row(caller_record)["caller"]["pair"]==0x06651234u,
            "a validated actor and its resource pair serialize");
        FrameRecord fade_frame{};
        require(!row(fade_frame).contains("transition_fade"),"no transition attempt serializes absent");
        fade_frame.transition_fade.attempts=1;fade_frame.transition_fade.valid=1;
        fade_frame.transition_fade.commands[4]=0xE44C0390u;
        require(row(fade_frame)["transition_fade"]["commands"][4]==0xE44C0390u,"transition emitted words serialize");
        Ring<FrameRecord,4> ring;
        for (unsigned i=0; i<7; ++i) {
            FrameRecord frame{};
            frame.epoch=100+i;
            frame.valid=(i==4) ? DrawRecord::Layout : DrawRecord::Layout|DrawRecord::Frame;
            frame.sequence=999; frame.time_us=999; // Producer cannot forge header.
            ring.record(frame,100+i);
        }
        auto history=ring.snapshot(110);
        require(history.written==7 && history.overwritten==3, "overwrite accounting");
        require(history.truncated && history.events.size()==4, "capacity truncation");
        require(history.invalid_total==1, "lifetime invalid accounting");
        for (unsigned i=0; i<4; ++i) {
            require(history.events[i].sequence==4+i && history.events[i].time_us==103+i,
                "original order and recorder header");
            require(history.events[i].epoch==103+i, "latest payload retained");
        }
        auto expired=ring.snapshot(window_us+111);
        require(expired.events.empty() && !expired.truncated, "expired overwrite is not truncation");
        Ring<FrameRecord,2> boundary;
        boundary.record(FrameRecord{},100);
        require(boundary.snapshot(window_us+100).events.size()==1,"ten second boundary retained");
        require(boundary.snapshot(window_us+101).events.empty(),"older than ten seconds excluded");
        Ring<HostRecord,32> concurrent;
        std::atomic<bool> done=false;
        std::jthread producer([&] {
            for (unsigned i=1; i<=20000; ++i) {
                HostRecord h{}; h.width=i; h.height=-int(i);
                concurrent.record(h,i);
            }
            done=true;
        });
        do {
            const auto copy=concurrent.snapshot(20001);
            uint64_t prior=0;
            for (const auto& h:copy.events) {
                require(h.width==-h.height, "snapshot must not tear a producer record");
                require(h.sequence>prior, "concurrent snapshot ordered");
                prior=h.sequence;
            }
        } while (!done);
        producer.join();
        require(concurrent.snapshot(20001).written==20000, "all concurrent writes observed");

        require(!capture(1), "capture requires start");
        require(completion().error=="not_started", "missing start explicit");
        const auto temp=std::filesystem::temp_directory_path()/
            ("baseline-issue-evidence-test-"+std::to_string(baseline::sound_trace::now_us()));
        std::filesystem::create_directory(temp);
        // No application profile is used. A sentinel is evidence that unrelated bytes survive.
        { std::ofstream file(temp/"profile-sentinel"); file<<"unchanged"; }
        start(temp,"test-1.2.3");
        FrameRecord f{}; f.epoch=123456; f.map=42;
        f.valid=3|FrameRecord::Cpu625|FrameRecord::LegacyPlayer|FrameRecord::SecondaryRoute;
        f.player_last_reason=4u|(64u<<8); f.bridge_next_block=29; f.cpu625_next_block=2047;
        f.player_view=2; f.player_primary_bytes=7584; f.player_primary_highwater=11376;
        f.secondary_opens=3; f.secondary_faults=1; f.secondary_seen_main=2; f.secondary_seen_late=1;
        f.secondary_unauthenticated=4; f.secondary_admissions=2; f.secondary_ledger_bytes=5648;
        f.shadow_valid=1; f.shadow_source_pc=0x800A6CF0u; f.shadow_seen_epoch=0xFFFFFFFFu;
        f.shadow_args={1,2,3,4}; record(f);
        // Unauthenticated sources serialize as null even if a producer left words behind.
        FrameRecord no_layout{}; no_layout.epoch=654321; no_layout.valid=DrawRecord::Frame;
        no_layout.player_last_reason=no_layout.bridge_next_block=no_layout.cpu625_next_block=5; record(no_layout);
        FrameRecord no_cpu{}; no_cpu.epoch=654322; no_cpu.valid=3|FrameRecord::LegacyPlayer; no_cpu.cpu625_next_block=6;
        no_cpu.shadow_source_pc=0x800A6D60u; no_cpu.secondary_opens=8; record(no_cpu); // Words without their bits stay absent.
        // Schema 3: a disabled legacy owner's leftover words never serialize as observations.
        FrameRecord no_player{}; no_player.epoch=654323; no_player.valid=3|FrameRecord::Cpu625|FrameRecord::SecondaryRoute;
        no_player.player_owner=no_player.player_model=no_player.player_asset=no_player.player_admissions=
            no_player.player_refusals=no_player.player_last_reason=no_player.player_primary_bytes=no_player.secondary_opens=7;
        no_player.bridge_next_block=30; no_player.cpu625_next_block=31; record(no_player);
        DrawRecord d{}; d.model=765432; d.valid=31;
        for (unsigned i=0; i<16; ++i) { d.root[i]=1000+i; d.projection[i]=2000+i; }
        d.player_owner=d.skeletal_calls=9; // No join bit: words left behind must not serialize.
        record(d);
        DrawRecord admitted=d; admitted.model=765433;
        admitted.valid=31|DrawRecord::PlayerJoin|DrawRecord::PlayerSlot;
        admitted.player_owner=0x80200000u; admitted.player_owner_slot=2; admitted.player_table_match=1;
        admitted.player_control_slot=0; admitted.player_outcome=0xFFFFFFFFu; record(admitted);
        DrawRecord unbound=d; unbound.model=765434; unbound.valid=31|DrawRecord::PlayerJoin;
        unbound.player_owner=0x1234u; unbound.player_owner_slot=5; unbound.player_outcome=7; record(unbound);
        DrawRecord refused=d; refused.model=765435; refused.valid=31|DrawRecord::SkeletalJoin;
        refused.skeletal_calls=1; refused.skeletal_refusals=1; refused.skeletal_reason=12; record(refused);
        DrawRecord skeletal=d; skeletal.model=765436; skeletal.valid=31|DrawRecord::SkeletalJoin;
        skeletal.skeletal_calls=1; skeletal.skeletal_admissions=1; skeletal.skeletal_reason=99; record(skeletal);
        HostRecord h{}; h.width=1921; h.height=1081; h.stick_x=-17; h.flags=9; record(h);
        GraphicsRecord g{}; g.kind=GraphicsRecord::ParseBegin; g.task_sequence=77; record(g);
        PublicationRecord p{}; p.generation=99; p.flags=7; record(p);
        // BK root occurrences: each group serializes only with its bit; provenance is explicit.
        BkDrawRecord bk_bound{}; bk_bound.source=BkDrawRecord::Root; bk_bound.root_serial=11; bk_bound.epoch=0x789;
        bk_bound.valid=BkDrawRecord::Layout|BkDrawRecord::Frame|BkDrawRecord::Owner|BkDrawRecord::Staged|
            BkDrawRecord::Route|BkDrawRecord::Roster|BkDrawRecord::Control|BkDrawRecord::ControlOwner|BkDrawRecord::End;
        bk_bound.owner_kind=2; bk_bound.current_id=bk_bound.emitted_id=0x20001400u; bk_bound.push=1;
        bk_bound.id_domain=BkDrawRecord::PlayerId; bk_bound.id_sample=20;
        bk_bound.staged_callback=0x80092234u; bk_bound.staged_owner=bk_bound.drawn_owner=0x80600100u;
        bk_bound.drawn_slot=0; bk_bound.drawn_table_match=1; bk_bound.control_slot=1; bk_bound.control_owner=0x80600400u;
        bk_bound.end_offset=5; bk_bound.end_uses_bones=1; bk_bound.groups=5;
        bk_bound.valid|=BkDrawRecord::RootWitness;
        for(uint32_t i=0;i<bk_bound.root_witness.size();++i) bk_bound.root_witness[i]=100u+i;
        bk_bound.root_witness[17]=0u; // Observed disabled segment, distinct from unavailable witness.
        record(bk_bound);
        BkDrawRecord bk_staged=bk_bound; bk_staged.root_serial=12; bk_staged.drawn_owner=0x1234u;
        bk_staged.valid&=~uint32_t(BkDrawRecord::Roster|BkDrawRecord::RootWitness); record(bk_staged);
        // Words left behind without their bits must not serialize as evidence.
        BkDrawRecord bk_unproven{}; bk_unproven.source=BkDrawRecord::Root; bk_unproven.root_serial=13; bk_unproven.owner_kind=3;
        bk_unproven.valid=BkDrawRecord::Layout|BkDrawRecord::Owner|BkDrawRecord::Unclosed;
        bk_unproven.epoch=bk_unproven.drawn_owner=bk_unproven.drawn_slot=bk_unproven.staged_owner=
            bk_unproven.control_slot=bk_unproven.control_owner=bk_unproven.end_offset=9; record(bk_unproven);
        BkDrawRecord bk_actor{}; bk_actor.source=BkDrawRecord::Root; bk_actor.root_serial=14; bk_actor.owner_kind=1;
        bk_actor.owner_birth=0x246; bk_actor.valid=BkDrawRecord::Layout|BkDrawRecord::Owner; record(bk_actor);
        BkDrawRecord bk_incomplete{}; bk_incomplete.root_serial=15; bk_incomplete.valid=BkDrawRecord::Layout; record(bk_incomplete);
        baseline::sound_trace::ring.record(baseline::sound_trace::Kind::SoundId,44,55,66);
        // Player rows: absent fields serialize as null; NaN words survive as hex.
        {
            using PlayerRow=RT64::TooiePlayerTransform;
            PlayerRow mapped{};
            mapped.presentation=41; mapped.workload_id=900; mapped.workload_slot=2; mapped.previous_slot=1;
            mapped.previous_workload_id=899; mapped.weight=0.25f; mapped.index=5; mapped.group_id=0x20000503u;
            mapped.flags=PlayerRow::OwnerMapped|PlayerRow::PriorSlotValid|PlayerRow::Mapped|PlayerRow::PriorIndexValid|
                PlayerRow::Interpolated|PlayerRow::ProjectionKnown|PlayerRow::Representative;
            mapped.group_matrix_count=7;
            mapped.previous_index=7; mapped.previous_group_id=0x20000503u; mapped.projection=0;
            mapped.raw[0]=0x3f800000u; mapped.prior[0]=0x3f000000u; mapped.interpolated[0]=0x3f200000u;
            PlayerRow unmatched{};
            unmatched.presentation=41; unmatched.group_id=TOOIE_BK_PLAYER_SLOT_ID_START+0x101u;
            unmatched.raw[0]=0x7fc00000u; // Quiet NaN bits.
            unmatched.group_matrix_count=999; // Unflagged value must not serialize as evidence.
            unmatched.previous_index=12; unmatched.previous_slot=3; // Words without flags must not serialize.
            RT64::TooiePlayerProjection projection{};
            projection.presentation=41; projection.flags=RT64::TooiePlayerProjection::Final |
                RT64::TooiePlayerProjection::OrthoScene | RT64::TooiePlayerProjection::SceneCensus |
                RT64::TooiePlayerProjection::PairKnown;
            projection.scene=3u|(2u<<16); projection.scene_counts=6u|(4u<<16);
            projection.scene_projections=5; projection.paired_scene=1; projection.paired_group_id=0xA0000010u;
            projection.weight=0.25f;
            projection.raw[15]=0x3f800000u;
            const PlayerRow player_rows[2]={mapped,unmatched};
            record_player(player_rows,2,&projection,1);
        }
        require(capture(17), "first capture accepted");
        require(!capture(18), "one outstanding capture, no queue");
        const auto result=completion();
        require(result.success && result.error.empty(), "export succeeds");
        require(result.filename==std::filesystem::path(result.filename).filename().string(), "basename only");
        require(result.bytes==std::filesystem::file_size(temp/result.filename) && result.bytes<=report_max_bytes,
            "size is actual report bytes");
        auto rows=read_report(temp/result.filename);
        require(rows.front().at("schema")==3 && rows.front().at("marker")==17, "versioned marker header");
        require(rows.back().at("type")=="completion" && rows.back().at("complete")==true, "last completion footer");
        require(rows.front().at("record_bytes").at("bk_draw")==sizeof(BkDrawRecord),"BK record size declared");
        require(rows.front().at("record_bytes").at("player_transform")==sizeof(PlayerTransformRecord) &&
            rows.front().at("player_ring_bytes")==player_ring_bytes,"player record and ring sizes declared");
        bool frame=false,draw=false,host=false,graphics=false,publication=false,audio=false,missing=false;
        bool frame_no_layout=false,frame_no_cpu=false,frame_no_player=false;
        unsigned joins=0;
        unsigned bk_rows=0;
        bool bk_domain=false,bk_coverage=false;
        unsigned player_rows=0; bool player_domain=false,player_coverage=false;
        for (const auto& row:rows) {
            const auto text=row.dump();
            require(text.find(temp.string())==std::string::npos, "no root path in report");
            require(!row.contains("executable_path"), "no executable path");
            if (row.at("type")=="frame" && row.at("epoch")==123456)
                frame=row.at("player_last_reason")==(4u|(64u<<8)) && row.at("bridge_next_block")==29 &&
                    row.at("cpu625_next_block")==2047 && row.at("shadow").at("source_pc")==0x800A6CF0u &&
                    row.at("shadow").at("seen_epoch")==0xFFFFFFFFu && row.at("shadow").at("id")==0 &&
                    row.at("shadow").at("args")==Json{1,2,3,4} && row.at("shadow").size()==8 &&
                    row.at("player_primary").at("view")==2 && row.at("player_primary").at("ledger_bytes")==7584 &&
                    row.at("player_primary").at("ledger_highwater")==11376 && row.at("player_primary").size()==3 &&
                    row.at("secondary_route").at("opens")==3 && row.at("secondary_route").at("faults")==1 &&
                    row.at("secondary_route").at("seen_main")==2 && row.at("secondary_route").at("seen_late")==1 &&
                    row.at("secondary_route").at("unauthenticated")==4 && row.at("secondary_route").at("admissions")==2 &&
                    row.at("secondary_route").at("ledger_bytes")==5648 && row.at("secondary_route").size()==7;
            if (row.at("type")=="frame" && row.at("epoch")==654321)
                frame_no_layout=row.at("player_last_reason").is_null() && row.at("bridge_next_block").is_null() &&
                    row.at("cpu625_next_block").is_null() && !row.contains("player_primary") &&
                    !row.contains("secondary_route") && row.at("player_owner").is_null();
            if (row.at("type")=="frame" && row.at("epoch")==654322)
                frame_no_cpu=row.at("player_last_reason")==0 && row.at("bridge_next_block")==0 &&
                    row.at("cpu625_next_block").is_null() && !row.contains("shadow") &&
                    row.at("player_owner")==0 && row.at("player_primary").size()==3 && !row.contains("secondary_route");
            if (row.at("type")=="frame" && row.at("epoch")==654323) {
                frame_no_player=row.at("bridge_next_block")==30 && row.at("cpu625_next_block")==31 &&
                    !row.contains("player_primary") && !row.contains("secondary_route") && row.at("valid")==(3u|4u|16u);
                for (const char* name:{"player_owner","player_model","player_asset","player_admissions",
                        "player_refusals","player_last_reason"})
                    frame_no_player=frame_no_player && row.at(name).is_null();
            }
            if (row.at("type")=="draw" && row.at("model")==765432)
                draw=row.at("root")[15]==1015 && row.at("projection")[0]==2000 &&
                    !row.contains("player_owner") && !row.contains("skeletal_calls");
            if (row.at("type")=="draw" && row.at("model")==765433)
                joins+=row.at("player_owner")==0x80200000u && row.at("player_owner_slot")==2 &&
                    row.at("player_table_match")==1 && row.at("player_control_slot")==0 &&
                    row.at("player_outcome")==0xFFFFFFFFu && !row.contains("skeletal_calls");
            if (row.at("type")=="draw" && row.at("model")==765434)
                joins+=row.at("player_owner")==0x1234u && row.at("player_owner_slot").is_null() &&
                    row.at("player_table_match").is_null() && row.at("player_outcome")==7;
            if (row.at("type")=="draw" && row.at("model")==765435)
                joins+=row.at("skeletal_calls")==1 && row.at("skeletal_admissions")==0 &&
                    row.at("skeletal_refusals")==1 && row.at("skeletal_reason")==12 && !row.contains("player_owner");
            if (row.at("type")=="draw" && row.at("model")==765436)
                joins+=row.at("skeletal_admissions")==1 && row.at("skeletal_refusals")==0 &&
                    row.at("skeletal_reason").is_null();
            if (row.at("type")=="domain" && row.at("domain")=="bk_draw")
                bk_domain=row.at("capacity")==bk_draw_capacity && row.at("invalid_total")==1 &&
                    row.at("validity")=="missing_layout_or_owner_is_incomplete";
            if (row.at("type")=="bk_draw" && row.at("root_serial")==11)
                bk_rows+=row.at("source")=="bk_root" && row.at("provenance")=="callback_staged_roster_bound" &&
                    row.at("drawn_owner")==0x80600100u && row.at("drawn_slot")==0 && row.at("drawn_table_match")==1 &&
                    row.at("control_slot")==1 && row.at("control_owner")==0x80600400u && row.at("epoch")==0x789 &&
                    row.at("staged_raw").at("owner")==0x80600100u && row.at("ids").at("emitted")==0x20001400u &&
                    row.at("ids").at("sample")==20 && row.at("end").at("offset")==5 && row.at("end").at("groups")==5 &&
                    row.at("unclosed")==false && !row.contains("player_owner") && !row.contains("root") &&
                    row.at("root_witness")==Json{
                        {"capture",{{"key",100},{"command",101},{"deform",102},{"header",103},
                            {"pose",104},{"entry",105},{"valid",106},{"model",107}}},
                        {"history_before",{{"id",108},{"frame",109},{"model",110},{"start",111}}},
                        {"float_frame",112},{"draw_id_before",113},
                        {"segment_after",{{"position",114},{"velocity",115},{"base",116},{"enabled",0}}},
                        {"selector_reserved",118},{"selector_declined_delta",119}};
            if (row.at("type")=="bk_draw" && row.at("root_serial")==12)
                bk_rows+=row.at("provenance")=="callback_staged_unbound" && row.at("drawn_owner")==0x1234u &&
                    row.at("drawn_slot").is_null() && row.at("drawn_table_match").is_null() &&
                    row.at("root_witness").is_null();
            if (row.at("type")=="bk_draw" && row.at("root_serial")==13)
                bk_rows+=row.at("provenance")=="unproven" && row.at("drawn_owner").is_null() &&
                    row.at("drawn_slot").is_null() && row.at("staged_raw").is_null() && row.at("control_slot").is_null() &&
                    row.at("control_owner").is_null() && row.at("end").is_null() && row.at("epoch").is_null() &&
                    row.at("unclosed")==true;
            if (row.at("type")=="bk_draw" && row.at("root_serial")==14)
                bk_rows+=row.at("provenance")=="actor_birth" && row.at("owner").at("birth")==0x246 &&
                    row.at("drawn_owner").is_null();
            if (row.at("type")=="bk_draw" && row.at("root_serial")==15)
                bk_rows+=row.at("provenance")=="unproven" && row.at("valid")==1;
            if (row.at("type")=="host") host|=row.at("width")==1921 && row.at("stick_x")==-17;
            if (row.at("type")=="graphics") graphics|=row.at("task_sequence")==77;
            if (row.at("type")=="publication") publication|=row.at("generation")==99;
            if (row.at("type")=="audio") audio|=row.at("a")==44 && row.at("b")==55 && row.at("aux")==66;
            if (row.at("type")=="player_transform" && row.at("presentation")==41 && row.at("index")==5)
                player_rows+=row.at("representative")==true && row.at("group_matrix_count")==7 &&
                    row.at("previous_index")==7 && row.at("prior_index_valid")==true &&
                    row.at("prior").get<std::string>().rfind("3f000000 ",0)==0 &&
                    row.at("interpolated").get<std::string>().rfind("3f200000 ",0)==0 &&
                    row.at("raw").get<std::string>().size()==143 && row.at("prior_workload").at("id")==899 &&
                    row.at("label")==Json{{"roster_slot",0},{"attachment",0},{"sample",5},{"offset",3}};
            if (row.at("type")=="player_transform" && row.at("presentation")==41 && row.at("index")==0)
                player_rows+=row.at("representative")==false && row.at("group_matrix_count").is_null() &&
                    row.at("mapped")==false && row.at("previous_index").is_null() &&
                    row.at("prior").is_null() && row.at("interpolated").is_null() &&
                    row.at("prior_workload").at("recorded_slot").is_null() && row.at("projection").is_null() &&
                    row.at("raw").get<std::string>().rfind("7fc00000 ",0)==0 && row.at("label").at("roster_slot")==1;
            if (row.at("type")=="player_projection" && row.at("presentation")==41)
                player_rows+=row.at("final").get<std::string>().size()==143 && row.at("prior").is_null() &&
                    row.at("group_id").is_null() && row.at("mapped")==false && row.at("weight")==0.25f &&
                    row.at("ortho_scene").at("scene")==3 && row.at("ortho_scene").at("representative_position")==2 &&
                    row.at("ortho_scene").at("census")==true && row.at("ortho_scene").at("scenes")==6 &&
                    row.at("ortho_scene").at("previous_scenes")==4 && row.at("ortho_scene").at("projections")==5 &&
                    row.at("ortho_scene").at("paired").at("previous_scene")==1 &&
                    row.at("ortho_scene").at("paired").at("first_group_id")==0xA0000010u;
            if (row.at("type")=="domain" && row.at("domain")=="player_transform")
                player_domain=row.at("capacity")==player_transform_capacity && row.at("retained")==2 &&
                    row.at("validity")=="flags_declare_absent_fields";
            if (row.at("type")=="coverage") {
                missing=row.at("unavailable").size()>=7;
                require(row.at("projection_semantics")=="last_saved_world_production" &&
                    row.at("host_stick_units")=="Q15" && row.at("graphics_enum_domain")=="actual_rt64",
                    "projection and input/settings domain semantics are explicit");
                require(row.at("frame_valid_bits")==Json{{"layout",1},{"frame",2},{"cpu625",4},{"legacy_player",8},
                    {"secondary_route",16}},"schema-3 frame validity bits declared");
                require(row.at("draw_valid_bits")==Json{{"layout",1},{"frame",2},{"root",4},{"projection",8},
                    {"context",16},{"player_join",32},{"player_slot",64},{"skeletal_join",128}},
                    "schema-2 draw join bits declared");
                require(row.at("shadow_semantics").at("source_fallback")==0x800A6D60u &&
                    row.at("shadow_semantics").at("fullframe_composite_args")==Json{0,0,304,228},
                    "optional shadow words declare their frame-end semantics");
                const auto& limits=row.at("limits");
                const auto limited=[&](const char* name) { return std::find(limits.begin(),limits.end(),name)!=limits.end(); };
                bk_coverage=row.at("bk_draw_valid_bits")==Json{{"layout",1},{"frame",2},{"owner",4},{"staged",8},
                    {"route",16},{"roster",32},{"control",64},{"control_owner",128},{"end",256},{"unclosed",512},{"root_witness",1024},
                    {"caller",2048}} &&
                    limited("bk_draw_is_cpu_emitted_root_tags_not_rt64_consumed_mapping_or_pixels") &&
                    limited("bk_model_is_observed_address_not_semantic_name") &&
                    limited("bk_caller_is_unowned_top_level_de498_entry_a1_keyed_by_frame_not_return_address_or_owner") &&
                    limited("bk_caller_actor_pair_only_for_live_pooled_actor_at_exact_position_minus4") &&
                    limited("bk_drawn_owner_is_callback_staged_candidate_not_authenticated_producer_owner") &&
                    limited("bk_control_slot_is_independent_of_drawn_owner") &&
                    limited("bk_fields_are_not_legacy_admission") &&
                    limited("bk_root_witness_scalar_before_after_not_consumed_vertices") &&
                    limited("bk_segment_pointers_valid_only_when_enabled_1") &&
                    limited("bk_history_before_requires_matching_id_frame_model");
                player_coverage=row.at("player_transform_flag_bits").at("prior_index_valid")==8 &&
                    row.at("player_matrix_encoding")=="f32_bits_hex_row_major" &&
                    limited("player_rows_are_rt64_cpu_draw_data_and_frame_map_after_processing_not_gpu_or_pixels") &&
                    limited("player_rows_join_guest_rings_by_time_only_no_guest_task_or_bk_frame_join") &&
                    limited("legacy_player_transform_and_projection_names_include_supplemental_owned_roots") &&
                    limited("players_collected_first_all_workloads_then_at_most64_actor_or_menu_model_roots") &&
                    limited("supplemental_roots_use_remaining_capacity_and_shorten_ring_time_retention");
            }
        }
        require(joins==4,"join groups serialize only with their bits; unbound slot and admitted reason are null");
        require(bk_rows==5 && bk_domain && bk_coverage,"BK root rows: groups only with bits, explicit provenance, domain and coverage");
        require(frame&&draw&&host&&graphics&&publication&&audio&&missing,"all allowlisted domains and explicit gaps");
        require(player_rows==3 && player_domain && player_coverage,"player rows: actual words, absent fields null, domain and coverage");
        require(frame_no_layout && frame_no_cpu && frame_no_player,
            "unauthenticated or unavailable frame words serialize as null or absent");
        // Export twice retains the first file exactly and generates a distinct name.
        const auto before=tooie::platform::file_sha256(temp/result.filename);
        require(capture(19),"second capture accepted");
        const auto second=completion();
        require(second.success && second.filename!=result.filename,"unique second report");
        require(before==tooie::platform::file_sha256(temp/result.filename),"old report preserved");
        for(unsigned i=2;i<report_limit;++i) {
            require(capture(100+i),"bounded capture accepted");
            require(completion().success,"bounded capture completed");
        }
        require(!capture(200),"run limit enforced");
        require(completion().error=="report_limit","limit is explicit");
        stop();

        // One synthetic full-capacity capture, never a gameplay performance claim.
        // Maximum-width numeric payloads also exercise the actual serialized byte cap.
        start(temp,"test-saturated");
        constexpr auto maximum=std::numeric_limits<uint32_t>::max();
        constexpr auto minimum=std::numeric_limits<int32_t>::min();
        FrameRecord full_frame{};
        full_frame.phase=full_frame.epoch=full_frame.map=full_frame.pool_index=full_frame.view=
            full_frame.camera=full_frame.projection=full_frame.camera_active=full_frame.camera_latch=
            full_frame.bridge_poisoned=full_frame.control_slot=full_frame.player_owner=
            full_frame.player_model=full_frame.player_asset=full_frame.player_admissions=
            full_frame.player_refusals=full_frame.player_last_reason=full_frame.bridge_next_block=
            full_frame.cpu625_next_block=full_frame.player_view=full_frame.player_primary_bytes=
            full_frame.player_primary_highwater=full_frame.secondary_opens=full_frame.secondary_faults=
            full_frame.secondary_seen_main=full_frame.secondary_seen_late=full_frame.secondary_unauthenticated=
            full_frame.secondary_admissions=full_frame.secondary_ledger_bytes=maximum; // Optional objects at full width.
        full_frame.requested_period=full_frame.measured_period=full_frame.live_period=minimum;
        full_frame.valid=3|FrameRecord::Cpu625|FrameRecord::LegacyPlayer|FrameRecord::SecondaryRoute; // Every frame word at full width.
        full_frame.shadow_valid=1; // The optional shadow object also bounds the worst frame row.
        full_frame.shadow_source_pc=full_frame.shadow_seen_epoch=full_frame.shadow_id=full_frame.shadow_group=
            full_frame.shadow_composite=full_frame.shadow_mode_open=full_frame.shadow_fault=maximum;
        full_frame.shadow_args.fill(maximum);
        DrawRecord full_draw{};
        full_draw.epoch=full_draw.buffers=full_draw.cursor=full_draw.model=full_draw.asset=
            full_draw.pool_index=full_draw.root_address=full_draw.actor=full_draw.descriptor=
            full_draw.slot=full_draw.birth=full_draw.view=full_draw.context_epoch=
            full_draw.context_admitted=full_draw.camera=full_draw.camera_birth=
            full_draw.camera_revision=full_draw.camera_id=full_draw.camera_scale=
            full_draw.camera_policy=full_draw.projection_address=maximum;
        // Both join groups at full width bound the worst serialized draw row.
        full_draw.player_owner=full_draw.player_owner_slot=full_draw.player_control_slot=
            full_draw.player_outcome=full_draw.skeletal_reason=maximum;
        full_draw.player_table_match=1; full_draw.skeletal_calls=1; full_draw.skeletal_refusals=1;
        full_draw.valid=31|DrawRecord::PlayerJoin|DrawRecord::PlayerSlot|DrawRecord::SkeletalJoin;
        full_draw.root.fill(maximum);
        full_draw.projection.fill(maximum);
        HostRecord full_host{};
        full_host.width=full_host.height=full_host.wide=full_host.aspect=full_host.cutscene_aspect=
            full_host.resolution=full_host.output_rate=full_host.manual_hz=full_host.vsync=
            full_host.texture_filter=full_host.output_filter=full_host.stick_x=full_host.stick_y=minimum;
        full_host.published_aspect=full_host.guest_publication=full_host.buttons=full_host.flags=maximum;
        GraphicsRecord full_graphics{};
        full_graphics.kind=GraphicsRecord::ConfigApplied;
        full_graphics.task_sequence=full_graphics.data_ptr=full_graphics.data_size=
            full_graphics.swap_chain_rate=maximum;
        full_graphics.resolution=full_graphics.output_rate=full_graphics.manual_hz=
            full_graphics.texture_filter=full_graphics.output_filter=minimum;
        PublicationRecord full_publication{};
        full_publication.generation=full_publication.flags=maximum;
        BkDrawRecord full_bk{};
        full_bk.source=full_bk.valid=full_bk.epoch=full_bk.bk_frame=full_bk.root_serial=full_bk.view=full_bk.phase=
            full_bk.sp=full_bk.buffers=full_bk.model=full_bk.asset=full_bk.depth=full_bk.draw_depth=
            full_bk.owner_kind=full_bk.owner_birth=full_bk.owner_view=full_bk.owner_phase=full_bk.owner_base=
            full_bk.owner_attachments=full_bk.owner_id=full_bk.current_id=full_bk.emitted_id=full_bk.offset=
            full_bk.uses_bones=full_bk.push=full_bk.skip_all=full_bk.skip_model=full_bk.id_domain=
            full_bk.id_attachment=full_bk.id_sample=full_bk.staged_callback=full_bk.staged_owner=full_bk.drawn_owner=
            full_bk.drawn_slot=full_bk.drawn_table_match=full_bk.control_slot=full_bk.control_owner=full_bk.end_offset=
            full_bk.end_uses_bones=full_bk.groups=full_bk.offset_overflows=full_bk.late_closes=
            full_bk.root_overflows=full_bk.scope_overflows=full_bk.caller_position=full_bk.caller_actor=
            full_bk.caller_pair=maximum; // Every BK group at full width.
        const auto enqueue_begin=baseline::sound_trace::now_us();
        for (size_t i=0;i<frame_capacity;++i) record(full_frame);
        for (size_t i=0;i<draw_capacity;++i) record(full_draw);
        for (size_t i=0;i<host_capacity;++i) record(full_host);
        for (size_t i=0;i<graphics_capacity;++i) record(full_graphics);
        for (size_t i=0;i<publication_capacity;++i) record(full_publication);
        for (size_t i=0;i<bk_draw_capacity;++i) record(full_bk);
        RT64::TooiePlayerTransform full_player{};
        full_player.presentation=full_player.workload_id=full_player.previous_workload_id=
            std::numeric_limits<uint64_t>::max();
        full_player.weight=full_player.previous_weight=-std::numeric_limits<float>::max();
        full_player.workload_slot=full_player.previous_slot=full_player.flags=full_player.index=
            full_player.vertices=full_player.physical=full_player.previous_index=full_player.previous_group_id=
            full_player.previous_vertices=full_player.projection=maximum;
        full_player.group_id=TOOIE_BK_PLAYER_SLOT_ID_END-1u; // Widest label; every optional field present.
        RT64::TooiePlayerProjection full_projection{};
        full_projection.presentation=full_projection.workload_id=full_projection.previous_workload_id=
            std::numeric_limits<uint64_t>::max();
        full_projection.workload_slot=full_projection.previous_slot=full_projection.flags=full_projection.index=
            full_projection.group_id=full_projection.previous_index=maximum;
        for (size_t i=0;i<player_transform_capacity;++i) record_player(&full_player,1,nullptr,0);
        for (size_t i=0;i<player_projection_capacity;++i) record_player(nullptr,0,&full_projection,1);
        for (size_t i=0;i<baseline::sound_trace::capacity;++i)
            baseline::sound_trace::ring.record(baseline::sound_trace::Kind::AudioChunk,
                std::numeric_limits<uint64_t>::max(),std::numeric_limits<uint64_t>::max(),
                std::numeric_limits<uint64_t>::max());
        const auto enqueue_end=baseline::sound_trace::now_us();
        require(capture(202),"saturated capture accepted");
        const auto snapshot_end=baseline::sound_trace::now_us();
        stop(); // No poll before shutdown: an accepted pending job must drain.
        const auto drain_end=baseline::sound_trace::now_us();
        const auto drained=poll();
        require(drained.has_value() && drained->marker==202,"stop publishes accepted job completion before returning");
        require(!poll().has_value(),"shutdown completion delivered exactly once");
        require(drained->success,"raised cap holds every saturated domain, both player domains included");
        require(drained->bytes==std::filesystem::file_size(temp/drained->filename) &&
            drained->bytes<=report_max_bytes,"saturated actual bytes obey hard cap");
        const std::array<const char*,9> domain_names={"frame","draw","bk_draw","host","graphics","publication","audio",
            "player_transform","player_projection"};
        const std::array<size_t,9> expected_counts={frame_capacity,draw_capacity,bk_draw_capacity,host_capacity,
            graphics_capacity,publication_capacity,baseline::sound_trace::capacity,player_transform_capacity,
            player_projection_capacity};
        std::array<size_t,9> serialized_counts{};
        size_t domain_count=0;
        std::ifstream saturated_file(temp/drained->filename);
        std::string saturated_line;
        Json saturated_footer;
        while (std::getline(saturated_file,saturated_line)) {
            const auto parsed=Json::parse(saturated_line);
            const auto type=parsed.at("type").get<std::string>();
            for (size_t i=0;i<domain_names.size();++i) {
                if (type==domain_names[i]) ++serialized_counts[i];
                if (type=="domain" && parsed.at("domain")==domain_names[i]) {
                    ++domain_count;
                    require(parsed.at("retained")==expected_counts[i],"saturated snapshot retains actual capacity");
                }
            }
            saturated_footer=parsed;
        }
        saturated_file.close();
        require(saturated_footer.at("type")=="completion" &&
            saturated_footer.at("complete")==drained->success,"shutdown report has final honest completion footer");
        if (drained->success) {
            require(domain_count==domain_names.size() && serialized_counts==expected_counts,
                "successful saturated export contains every retained record");
        } else {
            require(saturated_footer.at("error")=="byte_cap","saturated cap failure cannot claim successful truncation");
        }
        std::cout<<"synthetic_not_gameplay=1 ring_enqueue_us="<<(enqueue_end-enqueue_begin)
            <<" capture_snapshot_enqueue_us="<<(snapshot_end-enqueue_end)
            <<" worker_export_drain_stop_us="<<(drain_end-snapshot_end)
            <<" worker_time_includes_startup_hash_wait=1 saturated_report_bytes="<<drained->bytes
            <<" saturated_report_success="<<drained->success<<" saturated_report_error="<<drained->error<<'\n';
        if (argc>1) {
            // Optional, explicitly requested fixture artifact; never a player profile.
            // The caller creates the output directory. Only this fixed synthetic basename may be replaced.
            const auto output=std::filesystem::path(argv[1]);
            require(std::filesystem::is_directory(output),"synthetic artifact output directory must exist");
            std::filesystem::copy_file(temp/drained->filename,output/"synthetic-saturated.jsonl",
                std::filesystem::copy_options::overwrite_existing);
        }

        // A full writer must report failure, and cannot write a successful completion footer.
        FILE* small=std::tmpfile();
        require(small!=nullptr,"temporary file");
        BoundedWriter limited(small,512);
        require(!limited.append(Json{{"type","oversized"},{"data",std::string(400,'a')}}),"byte cap refuses whole row");
        require(std::string(limited.error)=="byte_cap","byte cap explicit");
        require(limited.finish(false,limited.error),"failure footer has reserved room");
        require(limited.bytes<=512,"hard cap includes footer");
        std::rewind(small);
        char footer[256]{};
        require(std::fgets(footer,sizeof(footer),small)!=nullptr,"failed export retains footer");
        const auto failed_footer=Json::parse(footer);
        require(failed_footer.at("complete")==false && failed_footer.at("error")=="byte_cap",
            "cap failure cannot claim successful truncation");
        std::fclose(small);
        // Exclusive file creation never touches an existing report, including restart collisions.
        const auto occupied=temp/"occupied.jsonl";
        {std::ofstream file(occupied);file<<"original";}
        require(open_exclusive(occupied)==nullptr,"exclusive claim refuses existing file");
        std::ifstream original(occupied); std::string original_text; original>>original_text;
        require(original_text=="original","collision bytes preserved"); original.close();
        const auto collision=temp/"issue-evidence-collision-1-0.jsonl";
        {std::ofstream file(collision); file<<"old-report";}
        Job colliding_job{}; colliding_job.marker=300; colliding_job.ordinal=1;
        const auto claimed=export_job(colliding_job,temp,"collision","test","","");
        require(claimed.success && claimed.filename=="issue-evidence-collision-1-1.jsonl",
            "restart collision claims another unique name");
        std::ifstream old_report(collision); old_report>>original_text;
        require(original_text=="old-report","restart collision preserves prior report"); old_report.close();
        std::ifstream sentinel(temp/"profile-sentinel"); sentinel>>original_text;
        require(original_text=="unchanged","unrelated profile sentinel preserved"); sentinel.close();
        // Non-directory log root is a fixed-code export failure, without leaking the path.
        start(occupied,"test"); require(capture(201),"worker handles bad root asynchronously");
        const auto failed=completion();
        require(!failed.success && failed.error=="file_create" && failed.filename.empty(),"file failure explicit/private");
        stop();
        // Every removal names a test-created file; no recursive deletion or application profile.
        for (const auto& entry:std::filesystem::directory_iterator(temp)) std::filesystem::remove(entry.path());
        std::filesystem::remove(temp);
        std::cout<<"issue evidence record sizes: "<<sizeof(FrameRecord)<<','<<sizeof(DrawRecord)<<','
            <<sizeof(HostRecord)<<','<<sizeof(GraphicsRecord)<<','<<sizeof(PublicationRecord)
            <<" rings="<<ring_bytes<<'\n';
        return 0;
    } catch(const std::exception& e) {
        stop(); std::cerr<<e.what()<<'\n'; return 1;
    }
}
