// Synthetic guest ABI fixture only. Root builds/runs this against selected
// composition layout headers; no compiled game helpers or original code needed.
#include "recomp.h"
#include "../tools/baseline/issue_evidence_guest.hpp"
#include "bk_pipeline_selection_layout.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {
using namespace baseline::issue_evidence;
namespace guest=baseline::issue_evidence::guest;
constexpr uint32_t sp=0x807FD000u, frame_sp=0x807FE000u;
constexpr uint32_t pool=0x80400000u, actor=0x80200000u, actor_pool=0x80210000u;
constexpr uint32_t camera_pointer=0x80300000u;
constexpr uint32_t slot=3u, birth=0x246u, epoch=0x789u, view=2u;
constexpr uint32_t control_address=TOOIE_CAMERA_STATE_VRAM+offsetof(TooieCameraState,control);
constexpr uint32_t saved_address=TOOIE_CAMERA_STATE_VRAM+offsetof(TooieCameraState,saved);
constexpr uint32_t context_address=TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,palette_context);
constexpr uint32_t record_address=TOOIE_CAMERA_STATE_VRAM+5u*sizeof(TooieCameraRecord);
constexpr uint32_t all=DrawRecord::Layout|DrawRecord::Frame|DrawRecord::Root|DrawRecord::Projection|DrawRecord::Context;
unsigned cases=0;
void require(bool ok,const char* what) {
    if(!ok) { std::fprintf(stderr,"FAIL: %s\n",what); std::exit(1); }
}
struct Fixture {
    std::vector<uint8_t> ram=std::vector<uint8_t>(16u*1024u*1024u);
    recomp_context ctx{};
    void put(uint32_t p,uint32_t value) { uint8_t* rdram=ram.data(); MEM_W(0,(gpr)(int32_t)p)=int32_t(value); }
    void put_half(uint32_t p,uint16_t value) { uint8_t* rdram=ram.data(); MEM_H(0,(gpr)(int32_t)p)=int16_t(value); }
    void put_byte(uint32_t p,uint8_t value) { uint8_t* rdram=ram.data(); MEM_B(0,(gpr)(int32_t)p)=int8_t(value); }
    template<class T> void put_abi(uint32_t p,const T& value) {
        const auto words=std::bit_cast<std::array<uint32_t,sizeof(T)/4>>(value);
        for(uint32_t i=0;i<words.size();++i) put(p+i*4u,words[i]);
    }
    Fixture() {
        ctx.r29=(gpr)(int32_t)sp; ctx.r4=0x12345678u; ctx.r16=0x76543210u;
        ctx.f_odd=&ctx.f0.u32h;
        TooieCameraControl c{};
        c.magic=TOOIE_CAMERA_MAGIC; c.layout=TOOIE_CAMERA_LAYOUT; c.epoch=epoch; c.active=1;
        c.main_base=TOOIE_CAMERA_MAIN_VRAM; c.main_begin=c.main_base+0x50u;
        c.main_end=c.main_base+TOOIE_CAMERA_M_BYTES; c.main_extra=0x50u;
        put_abi(control_address,c);
        put(TOOIE_CAMERA_STATE_VRAM+offsetof(TooieCameraState,owner_frame),frame_sp);
        put(TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,magic),TOOIE_BRIDGE_MAGIC);
        put(TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,layout),TOOIE_BRIDGE_LAYOUT);
        put(TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,epoch),epoch);
        put(TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,silo_depth),1);
        put(TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,births)+slot*4u,birth);
        TooieActorPaletteContext a{};
        a.actor=actor; a.descriptor=guest::descriptor_address+slot*guest::descriptor_stride;
        a.slot=slot; a.birth=birth; a.epoch=epoch; a.view=view; a.phase=6u;
        a.pair=0x06651234u; a.buffers=guest::buffers_address; a.admitted=0u;
        put_abi(context_address,a);
        put(actor,a.descriptor);
        put_half(a.descriptor+0x14u,uint16_t(a.pair>>16));
        put_half(a.descriptor+0x16u,uint16_t(a.pair));
        put_half(a.descriptor+0x1Au,0u);
        put_byte(guest::actor_bitmap_address+(slot>>3),uint8_t(0x80u>>(slot&7u)));
        put(guest::actor_pool_address,actor_pool);
        put(actor_pool,0xA0u); put(actor_pool+4u,actor);
        put(actor_pool+8u,actor+0xA0u); put(actor_pool+12u,actor+0x140u);
        put(0x8012C948u+25u*4u,actor);
        put_half(guest::pool_index_address,0u);
        put_half(guest::phase_address,6u); put(guest::view_address,view);
        put(guest::gfx_pool_address,TOOIE_CAMERA_MAIN_VRAM);
        put(guest::matrix_pool_address,pool);
        put(guest::buffers_address,TOOIE_CAMERA_MAIN_VRAM+0x80u); // deliberately not local cursor
        put(guest::buffers_address+4u,pool+64u);
        put(sp+0xF8u,guest::buffers_address);
        put(sp+0xCCu,TOOIE_CAMERA_MAIN_VRAM+0x100u);
        put(sp+0x10Cu,0x80501230u);
        put(guest::effective_asset_address,0x9876u); // deliberately not context pair's asset
        put(guest::camera_address,camera_pointer); put(guest::projection_address,pool);
        TooieCameraProduction p{};
        p.matrix=pool; p.camera=camera_pointer; p.birth=0x135u; p.view=view; p.revision=13u;
        p.scale=0x3FC00000u; p.id=0xA0B00C00u; p.role=TOOIE_CAMERA_WORLD_INITIAL;
        p.epoch=epoch; p.valid=1u; p.policy=guest::world_policy;
        put_abi(saved_address,p);
        TooieCameraRecord camera{};
        camera.pointer=p.camera; camera.birth=p.birth; camera.revision=p.revision; camera.valid=1;
        camera.view_id[view]=p.id; camera.view_scale[view]=p.scale;
        put_abi(record_address,camera);
        for(uint32_t i=0;i<16u;++i) { put(pool+i*4u,0xB2345670u+i); put(pool+64u+i*4u,0xC1234560u+i); }
        put_half(0x80132DC2u,0x4567u);
        put(guest::control_slot_address,7u);
        put(0x8007913Cu,0xFFFFFFFDu); put_half(0x80078F64u,0xFFFEu); put(0x80079138u,0x12345678u);
        TooiePlayerBoundary player{};
        player.owner=0x80600120u; player.model=0x80500230u; player.slot=4u;
        player.effective_model_asset=0x567u; player.admissions=0x345u; player.refusals=0x678u;
        player.last_reason=4u|(0x40u<<8);
        put_abi(TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,player),player);
        put(TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,next_block),0x1234u);
#if TOOIE_CPU625_SELECTED
        guest::Cpu625Header cpu{guest::cpu625_magic,guest::cpu625_layout,0x7FFu,epoch};
        put_abi(TOOIE_CPU625_STATE,cpu);
#endif
    }
    FrameRecord frame(uint32_t owner=frame_sp) {
        const auto before=ram;
        recomp_context frame_ctx{}; frame_ctx.r29=(gpr)(int32_t)owner;
        const auto out=guest::read_frame(ram.data(),frame_ctx,guest::Begin);
        require(before==ram,"frame must not mutate guest memory");
        return out;
    }
    DrawRecord read() {
        const auto before=ram;
        std::array<uint8_t,sizeof(recomp_context)> context_before{};
        std::memcpy(context_before.data(),&ctx,sizeof(ctx));
        const auto out=guest::read_draw(ram.data(),ctx);
        require(before==ram,"draw must not mutate guest memory");
        require(std::memcmp(context_before.data(),&ctx,sizeof(ctx))==0,"draw must not mutate context");
        ++cases; return out;
    }
};
void invalid(uint32_t address,uint32_t value,uint32_t clear,const char* why) {
    Fixture f;
    f.put(address,value);
    const auto r=f.read();
    require(!(r.valid&clear),why);
    require(r.model==0x80501230u && r.context_admitted==0u,"invalid record retains reached raw source metadata");
    if(clear&DrawRecord::Projection) require(r.projection==std::array<uint32_t,16>{},"invalid projection is never copied");
    if(clear&DrawRecord::Root) require(r.root==std::array<uint32_t,16>{},"invalid root is never copied");
}
unsigned bk_cases=0;
#if TOOIE_BK_PIPELINE_SELECTED
// BK root-draw occurrences at the DF0B0/DF234 seams. The fixture writes TooieBkState words exactly as
// tooie_bk_de448_begin, tooie_bk_root_begin and tooie_bk_root_end leave them; only native reads run.
using Bk_=BkDrawRecord;
constexpr uint32_t bk_state=TOOIE_BK_STATE, owner_a=0x80610100u, owner_b=0x80610400u, sp_inner=sp-0x200u;
constexpr uint32_t bk_bound=Bk_::Layout|Bk_::Frame|Bk_::Owner|Bk_::Staged|Bk_::Route|Bk_::Roster|
    Bk_::Control|Bk_::ControlOwner|Bk_::End;
constexpr uint32_t bk_sample=2u*TOOIE_BK_PHASE_COUNT+6u; // view 2, phase 6
constexpr uint32_t player_id(uint32_t attachment) {
    return TOOIE_BK_PLAYER_ID_START+attachment*TOOIE_BK_OWNER_STRIDE+bk_sample*TOOIE_BK_TRANSFORM_ID_COUNT;
}
constexpr uint32_t actor_birth=0x246u;
constexpr uint32_t actor_id=TOOIE_BK_ACTOR_ID_START+(actor_birth-1u)*TOOIE_BK_ACTOR_OWNER_STRIDE+
    bk_sample*TOOIE_BK_TRANSFORM_ID_COUNT;
constexpr size_t bk_diag(size_t field) { return offsetof(TooieBkState,diagnostics)+field; }
struct Bk {
    Fixture f;
    guest::BkRoots roots{};
    std::vector<BkDrawRecord> out;
    void w(size_t offset,uint32_t value) { f.put(bk_state+uint32_t(offset),value); }
    uint32_t r(size_t offset) { return guest::bk_word(f.ram.data(),offset); }
    Bk() {
        w(offsetof(TooieBkState,magic),TOOIE_BK_MAGIC); w(offsetof(TooieBkState,layout),TOOIE_BK_LAYOUT);
        w(bk_diag(offsetof(TooieBkDiagnostics,frames)),40u);
        // Two different player owners bound at their own D_80135490 slots; control selects slot 0.
        f.put(owner_a+guest::player_slot_offset,0u); f.put(guest::player_table_address,owner_a);
        f.put(owner_b+guest::player_slot_offset,1u); f.put(guest::player_table_address+4u,owner_b);
        f.put(guest::control_slot_address,0u);
    }
    // The owner and current ID that tooie_bk_de448_begin (or the attachment scope) leaves.
    void owner(uint32_t kind,uint32_t attachments,uint32_t owner_id,uint32_t current,uint32_t birth=0u,uint32_t base=0u) {
        TooieBkOwner o{}; o.kind=kind; o.birth=birth; o.view=2u; o.phase=6u; o.base_attachment=base;
        o.attachments=attachments; o.id=owner_id;
        f.put_abi(bk_state+uint32_t(offsetof(TooieBkState,owner)),o);
        w(offsetof(TooieBkState,cur_drawn_model_transform_id),current);
        w(offsetof(TooieBkState,draw_depth),1u);
    }
    // A top-level player route draw: DF47C staged C9A8 (route callback) and C9AC (owner).
    void body(uint32_t staged,uint32_t base=0u) {
        owner(TOOIE_BK_OWNER_PLAYER,0u,player_id(base),player_id(base),0u,base);
        f.put(guest::draw_callback_address,base ? guest::player_secondary_callback : guest::player_callback);
        f.put(guest::draw_owner_address,staged);
    }
    // DF0B0 around the helper; `helper` false models a root whose depth did not advance.
    void begin(uint32_t at,uint32_t push=1u,bool helper=true) {
        const auto emit=[this](const BkDrawRecord& record) { out.push_back(record); };
        auto before=f.ram;
        const uint32_t depth=guest::bk_root_before(roots,f.ram.data(),emit);
        require(before==f.ram,"root-begin flush must not mutate guest memory");
        if(helper) {
            // tooie_bk_root_begin: below the depth bound, save the enclosing context in this depth's
            // frame and push; at the bound, only count the depth.
            const uint32_t d=r(offsetof(TooieBkState,root_depth));
            if(d<TOOIE_BK_ROOT_DEPTH) {
                const size_t frame=offsetof(TooieBkState,roots)+d*sizeof(TooieBkRootFrame);
                w(frame+offsetof(TooieBkRootFrame,pushed_group),push);
                w(frame+offsetof(TooieBkRootFrame,outer_offset),r(offsetof(TooieBkState,cur_model_transform_id_offset)));
                w(frame+offsetof(TooieBkRootFrame,outer_uses_bones),r(offsetof(TooieBkState,cur_model_uses_bones)));
                w(offsetof(TooieBkState,cur_model_transform_id_offset),0u);
                w(offsetof(TooieBkState,cur_model_uses_bones),0u);
            }
            w(offsetof(TooieBkState,root_depth),d+1u);
            w(bk_diag(offsetof(TooieBkDiagnostics,roots)),r(bk_diag(offsetof(TooieBkDiagnostics,roots)))+1u);
        }
        before=f.ram;
        guest::bk_root_after(roots,f.ram.data(),at,depth);
        require(before==f.ram,"root-begin snapshot must not mutate guest memory");
        ++bk_cases;
    }
    // DF234: this root consumed `consumed` bone IDs/groups, then tooie_bk_root_end pops it.
    void end(uint32_t at,uint32_t consumed=0u,uint32_t bones=0u) {
        const auto emit=[this](const BkDrawRecord& record) { out.push_back(record); };
        const size_t groups=bk_diag(offsetof(TooieBkDiagnostics,groups));
        w(offsetof(TooieBkState,cur_model_transform_id_offset),consumed);
        w(offsetof(TooieBkState,cur_model_uses_bones),bones);
        w(groups,r(groups)+consumed);
        const auto before=f.ram;
        guest::bk_root_end(roots,f.ram.data(),at,emit);
        require(before==f.ram,"root-end snapshot must not mutate guest memory");
        pop();
        ++bk_cases;
    }
    // tooie_bk_root_end, including its bit40 DF238 caller.
    void pop() { if(const uint32_t d=r(offsetof(TooieBkState,root_depth))) w(offsetof(TooieBkState,root_depth),d-1u); }
    BkDrawRecord only() { require(out.size()==1u,"exactly one occurrence"); return out[0]; }
};
void bk_root_cases() {
    {
        Bk b;auto* ram=b.f.ram.data();auto& c=b.f.ctx;c.r16=(gpr)(int32_t)0x80200000u;
        const uint32_t object=uint32_t(c.r16),gfx=0x80400000u,cmd=0x80401000u;
        b.f.put(uint32_t(c.r29)+0x74u,gfx);b.f.put(gfx,cmd+40u);
        b.f.put_byte(object+8u,3);b.f.put_byte(object+0x1Cu,0);
        b.f.put(object+0x28u,0x3F000000u);b.f.put(object+0xCu,0x3F800000u);
        const std::array<uint32_t,10> words{0xDE000000u,0x0011A6E0u,0xF8000000u,0x00000080u,
            0xE44C0390u,0u,0xE1000000u,0u,0xF1000000u,0x01000100u};
        for(uint32_t i=0;i<10;++i)b.f.put(cmd+4*i,words[i]);
        tooie_transition_fade::current={};const auto before=b.f.ram;const auto before_ctx=c;
        guest::transition_fade_after(ram,c,0x8080072Cu);
        require(tooie_transition_fade::current.value.valid==1 && tooie_transition_fade::current.value.commands==words,
            "transition copies actual authenticated rectangle commands");
        require(before==b.f.ram && std::memcmp(&before_ctx,&c,sizeof(c))==0,"transition witness changes neither guest memory nor registers");
        auto snap=guest::read_frame(ram,c,guest::End);
        require(snap.transition_fade.valid && snap.transition_fade.kind==3 && snap.transition_fade.attempts==1,"transition copied at construction end");
        require(!guest::read_frame(ram,c,guest::Begin).transition_fade.attempts,"transition absent at begin snapshot");
        b.f.put(guest::view_address,7);guest::transition_fade_after(ram,c,0x80800770u);
        require(tooie_transition_fade::current.value.mixed_views==1 && tooie_transition_fade::current.value.attempts==2,"multiple views explicitly ambiguous");
        b.f.put(cmd+16,0xE5000000u);guest::transition_fade_after(ram,c,0x808007E8u);
        require(!tooie_transition_fade::current.value.valid && tooie_transition_fade::current.value.refused==1,"wrong rectangle opcode refused");
        b.f.put(gfx,16);guest::transition_fade_after(ram,c,0x8080072Cu);
        require(!tooie_transition_fade::current.value.valid && tooie_transition_fade::current.value.refused==2,"cursor underflow refused");
        const auto attempts=tooie_transition_fade::current.value.attempts;
        guest::transition_fade_after(ram,c,0x8080082Cu);
        require(tooie_transition_fade::current.value.attempts==attempts,"shared return join excludes non-kind9 bypass");
        b.f.put_byte(object+8u,9);b.f.put(gfx,cmd+40u);b.f.put(cmd+16,words[4]);
        guest::transition_fade_after(ram,c,0x8080082Cu);
        require(tooie_transition_fade::current.value.valid && tooie_transition_fade::current.value.attempts==attempts+1,"kind9 actual fade admitted at shared return join");
        b.f.put(TOOIE_BK_STATE+offsetof(TooieBkState,diagnostics)+offsetof(TooieBkDiagnostics,frames),41);
        require(!guest::read_frame(ram,c,guest::End).transition_fade.attempts,"previous construction does not inherit fade");
    }

    {
        Bk b;auto* ram=b.f.ram.data();
        guest::world_producer_depth=0;
        const auto before=b.f.ram;
        guest::world_producer_begin(ram,0x800BE90Cu,0x80600000u);
        BkDrawRecord r{};r.model=0x80600000u;r.bk_frame=40;r.depth=2;r.draw_depth=2;
        guest::world_producer_join(ram,r);
        require(!r.producer_site,"nested root cannot consume enclosing producer");
        r.depth=1;r.draw_depth=1;guest::world_producer_join(ram,r);
        require(r.producer_site==0x800BE90Cu&&r.producer_model==r.model,"exact first producer root joins");
        r.producer_site=0;r.producer_model=0;guest::world_producer_join(ram,r);
        require(!r.producer_site,"same producer occurrence consumed only once");
        guest::world_producer_end();
        guest::world_producer_begin(ram,0x800BFB40u,r.model);
        ++r.bk_frame;guest::world_producer_join(ram,r);
        require(!r.producer_site,"different construction cannot inherit producer");
        --r.bk_frame;++r.model;guest::world_producer_join(ram,r);
        require(!r.producer_site,"different model cannot inherit producer");
        --r.model;guest::world_producer_end();
        for(unsigned i=0;i<9;++i)guest::world_producer_begin(ram,0x800BFE70u,r.model);
        guest::world_producer_join(ram,r);require(!r.producer_site,"scope overflow masks outer producer");
        for(unsigned i=0;i<9;++i)guest::world_producer_end();
        guest::world_producer_join(ram,r);require(!r.producer_site,"return clears producer scope");
        require(before==b.f.ram,"producer witness never changes guest memory");
        tooie_menu_witness::current={};tooie_menu_witness::hit(tooie_menu_witness::EnterUnregistered);
        tooie_menu_witness::current.owner=0x80200000u;
        const auto snap=guest::read_frame(ram,b.f.ctx,guest::End);
        tooie_menu_witness::current={};
        require(snap.menu.owner==0x80200000u&&snap.menu.counts[tooie_menu_witness::EnterUnregistered]==1u,
            "F4 owns a value snapshot independent of later native witness changes");
    }

    {
        // Two different roster owners under one control slot, then a control swap.
        Bk b;
        for(const uint32_t control:{0u,1u}) {
            b.f.put(guest::control_slot_address,control);
            b.body(owner_a); b.begin(sp); b.end(sp,5u,1u);
            b.body(owner_b); b.begin(sp); b.end(sp,5u,1u);
        }
        require(b.out.size()==4u,"four paired body roots");
        for(size_t i=0;i<b.out.size();++i) {
            const auto& o=b.out[i];
            const bool second=i&1u;
            const uint32_t control=i<2u ? 0u : 1u;
            require((o.valid&bk_bound)==bk_bound && !(o.valid&Bk_::Unclosed),"paired body root carries all guarded staged-association groups");
            require(o.drawn_owner==(second ? owner_b : owner_a) && o.drawn_slot==(second ? 1u : 0u) && o.drawn_table_match==1u,
                "each roster owner keeps its own drawn identity");
            require(o.control_slot==control && o.control_owner==(control ? owner_b : owner_a),
                "the control slot and its owner are recorded apart from the drawn owner");
            require(o.source==Bk_::Root && o.owner_kind==TOOIE_BK_OWNER_PLAYER && !o.owner_attachments &&
                o.owner_id==player_id(0u) && o.current_id==player_id(0u) && o.emitted_id==o.current_id && o.push==1u,
                "a body root emits its owner's root group ID");
            require(o.id_domain==Bk_::PlayerId && o.id_attachment==0u && o.id_sample==bk_sample,
                "the emitted ID decodes only to attachment and view x phase sample");
            require(o.end_offset==5u && o.end_uses_bones==1u && o.groups==5u && !o.offset_overflows,
                "end values are this root's own");
            require((o.valid&Bk_::Frame) && o.epoch==epoch && o.model==0x80501230u && o.buffers==guest::buffers_address &&
                o.asset==0x9876u && o.view==view && o.phase==6u && o.sp==sp && o.depth==1u && o.draw_depth==1u &&
                o.bk_frame==40u,"occurrence metadata from the DE498 frame and original words");
        }
        require(b.out[0].emitted_id==b.out[1].emitted_id && b.out[0].drawn_owner!=b.out[1].drawn_owner,
            "two staged owner candidates share one emitted ID without changing tag behavior");
        require(b.out[0].root_serial+1u==b.out[1].root_serial,"root serials order occurrences");
    }
    {
        // The secondary 92224 route authenticates its own staged owner.
        Bk b; b.body(owner_b,TOOIE_BK_PLAYER_SECONDARY); b.begin(sp); b.end(sp);
        const auto o=b.only();
        require((o.valid&bk_bound)==bk_bound && o.drawn_owner==owner_b && o.drawn_slot==1u &&
            o.id_attachment==TOOIE_BK_PLAYER_SECONDARY,"the secondary callback staging is identified");
    }
    {
        // Backpack: the attachment scope keeps the body's owner ID while C948 still holds the body's staging.
        Bk b; b.owner(TOOIE_BK_OWNER_PLAYER,1u,player_id(0u),player_id(1u));
        b.f.put(guest::draw_callback_address,guest::player_callback); b.f.put(guest::draw_owner_address,owner_a);
        b.begin(sp); b.end(sp);
        const auto o=b.only();
        require(!(o.valid&(Bk_::Route|Bk_::Roster)) && !o.drawn_owner && (o.valid&Bk_::Staged) &&
            o.staged_owner==owner_a && o.id_attachment==1u && o.emitted_id==player_id(1u),
            "an attachment keeps its ID; stale staging is never its drawn owner");
    }
    {
        Bk b; b.owner(TOOIE_BK_OWNER_SHADOW,0u,player_id(0u),player_id(0u));
        b.f.put(guest::draw_callback_address,guest::player_callback); b.f.put(guest::draw_owner_address,owner_a);
        b.begin(sp); b.end(sp);
        const auto o=b.only();
        require(!(o.valid&(Bk_::Route|Bk_::Roster)) && !o.drawn_owner && o.owner_kind==TOOIE_BK_OWNER_SHADOW,
            "a shadow root names no drawn owner");
    }
    {
        Bk b; b.owner(TOOIE_BK_OWNER_ACTOR,0u,actor_id,actor_id,actor_birth);
        b.begin(sp); b.end(sp);
        const auto o=b.only();
        require(!(o.valid&Bk_::Route) && o.owner_birth==actor_birth && o.id_domain==Bk_::ActorId &&
            o.id_attachment==0u && o.id_sample==bk_sample,"an actor root keeps its creation birth, not a player owner");
    }
    {
        // Split-player identity: a roster-slot-1 player ID decodes as a player ID with its own attachment and
        // sample; owner.birth carries the drawing player's roster slot + 1.
        const uint32_t slot1=TOOIE_BK_PLAYER_SLOT_ID_START+TOOIE_BK_OWNER_STRIDE+bk_sample*TOOIE_BK_TRANSFORM_ID_COUNT;
        Bk b; b.owner(TOOIE_BK_OWNER_PLAYER,1u,slot1-TOOIE_BK_OWNER_STRIDE,slot1,2u); b.begin(sp); b.end(sp);
        const auto o=b.only();
        require(o.id_domain==Bk_::PlayerId && o.id_attachment==1u && o.id_sample==bk_sample && o.owner_birth==2u,
            "a roster-slot-1 player ID decodes to its own attachment and sample");
    }
    struct Miss { void (*setup)(Bk&); const char* why; };
    const Miss misses[]={
        {[](Bk& b) { b.f.put(guest::draw_callback_address,guest::player_secondary_callback); },"route callback must match the owner's route"},
        {[](Bk& b) { b.w(offsetof(TooieBkState,draw_depth),2u); },"a nested DE448 is not the body route"},
        {[](Bk& b) { b.w(offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,own_draw_pending),1u); },"a pending own draw is not this root"},
        {[](Bk& b) { b.w(offsetof(TooieBkState,cur_drawn_model_transform_id),0u); },"no current ID, no body route"},
        {[](Bk& b) { b.w(offsetof(TooieBkState,root_depth),1u); },"an enclosed root is not the body root"}};
    for(const auto& miss:misses) {
        Bk b; b.body(owner_a); miss.setup(b); b.begin(sp); b.end(sp);
        const auto o=b.only();
        require(!(o.valid&(Bk_::Route|Bk_::Roster)) && !o.drawn_owner && (o.valid&Bk_::Staged) &&
            (o.valid&Bk_::Control),miss.why);
    }
    {
        Bk b; b.body(0x1234u); b.begin(sp); b.end(sp);
        const auto o=b.only();
        require((o.valid&Bk_::Route) && !(o.valid&Bk_::Roster) && o.drawn_owner==0x1234u && !o.drawn_slot,
            "an out-of-range staged owner stays route-staged, with no invented slot or binding");
    }
    {
        Bk b; b.body(owner_a); b.f.put(guest::player_table_address,owner_b); b.begin(sp); b.end(sp);
        const auto o=b.only();
        require((o.valid&Bk_::Roster) && o.drawn_slot==0u && !o.drawn_table_match && o.control_owner==owner_b,
            "a roster table mismatch is exported unbound");
    }
    {
        Bk b; b.body(owner_a); b.f.put(owner_a+guest::player_slot_offset,9u); b.begin(sp); b.end(sp);
        const auto o=b.only();
        require((o.valid&Bk_::Roster) && o.drawn_slot==9u && !o.drawn_table_match,"an out-of-roster slot is never bound");
    }
    {
        Bk b; b.f.put(guest::control_slot_address,9u); b.body(owner_a); b.begin(sp); b.end(sp);
        const auto o=b.only();
        require((o.valid&Bk_::Control) && !(o.valid&Bk_::ControlOwner) && o.control_slot==9u && !o.control_owner &&
            (o.valid&Bk_::Roster) && o.drawn_table_match==1u,"an out-of-roster control slot has no control owner");
    }
    {
        Bk b; b.f.put(control_address+offsetof(TooieCameraControl,epoch),epoch+1u); b.body(owner_a); b.begin(sp); b.end(sp);
        const auto o=b.only();
        require(!(o.valid&Bk_::Frame) && !o.epoch &&
            (o.valid&(bk_bound&~uint32_t(Bk_::Frame)))==(bk_bound&~uint32_t(Bk_::Frame)),
            "a stale camera frame withholds only the epoch");
    }
    for(const size_t offset:{offsetof(TooieBkState,magic),offsetof(TooieBkState,layout)}) {
        Bk b; b.body(owner_a); b.w(offset,0u); b.begin(sp); b.end(sp);
        require(b.out.empty(),"unauthenticated BK state records nothing");
    }
    {
        Bk b; b.body(owner_a); b.begin(sp,1u,false); b.end(sp);
        require(b.out.empty(),"a root whose depth did not advance is not snapshotted");
    }
    {
        // Nested roots close inner first; the outer keeps its begin association although the globals and
        // the drawn owner's own guest words change before its end.
        Bk b; b.body(owner_a); b.begin(sp);
        b.owner(TOOIE_BK_OWNER_ACTOR,0u,actor_id,actor_id,actor_birth); b.w(offsetof(TooieBkState,draw_depth),2u);
        b.begin(sp_inner); b.end(sp_inner,2u);
        b.body(owner_b); b.f.put(owner_a+guest::player_slot_offset,7u); b.f.put(guest::player_table_address,0u);
        b.end(sp,3u);
        require(b.out.size()==2u && b.out[0].depth==2u && b.out[1].depth==1u,"the inner root closes first");
        require(b.out[0].owner_kind==TOOIE_BK_OWNER_ACTOR && !(b.out[0].valid&Bk_::Route) && b.out[0].sp==sp_inner &&
            (b.out[0].valid&Bk_::End) && b.out[0].groups==2u,"inner actor root");
        const auto& o=b.out[1];
        require((o.valid&bk_bound)==bk_bound && o.drawn_owner==owner_a && o.drawn_slot==0u && o.drawn_table_match==1u &&
            o.owner_kind==TOOIE_BK_OWNER_PLAYER && o.end_offset==3u && o.groups==5u,
            "the outer root keeps its begin snapshot; its group delta includes nested roots");
    }
    {
        // Bit40 completes at DF238; the normal DF234 path must not be emitted twice.
        Bk b; b.body(owner_a); b.begin(sp,2u);
        const auto emit=[&](const BkDrawRecord& record) { b.out.push_back(record); };
        guest::bk_root_join(b.roots,b.f.ram.data(),sp_inner,emit);
        require(b.out.empty(),"another activation cannot close the projected root");
        guest::bk_root_join(b.roots,b.f.ram.data(),sp,emit); b.pop();
        b.body(owner_b); b.begin(sp); b.end(sp);
        guest::bk_root_join(b.roots,b.f.ram.data(),sp,emit);
        require(b.out.size()==2u,"projected root plus normal root, no double close");
        require(!(b.out[0].valid&Bk_::Unclosed) && (b.out[0].valid&Bk_::End) && b.out[0].push==2u &&
            !b.out[0].emitted_id && b.out[0].drawn_owner==owner_a,"projected root retains its begin association at join");
        require((b.out[1].valid&Bk_::End) && !(b.out[1].valid&Bk_::Unclosed) && b.out[1].drawn_owner==owner_b,
            "the next root pairs normally");
    }
    {
        Bk b; b.body(owner_a); b.begin(sp); b.end(sp_inner);
        require((b.only().valid&Bk_::Unclosed) && !(b.only().valid&Bk_::End),"a DF234 from another DE498 frame never closes this root");
    }
    {
        Bk b; b.body(owner_a); b.begin(sp); b.w(bk_diag(offsetof(TooieBkDiagnostics,frames)),41u); b.end(sp);
        require((b.only().valid&Bk_::Unclosed) && !(b.only().valid&Bk_::End),"a root never pairs across BK frames");
    }
    {
        // Bounded: a root past TOOIE_BK_ROOT_DEPTH keeps no pending state.
        Bk b; b.body(owner_a);
        for(uint32_t i=0;i<=TOOIE_BK_ROOT_DEPTH;++i) b.begin(sp-i*0x200u);
        for(uint32_t i=TOOIE_BK_ROOT_DEPTH+1u;i-->0u;) b.end(sp-i*0x200u);
        require(b.out.size()==TOOIE_BK_ROOT_DEPTH,"only the bounded depths are recorded");
        for(const auto& o:b.out) require((o.valid&Bk_::End) && !(o.valid&Bk_::Unclosed),"bounded depths pair");
    }
}
#endif
}
int main() {
    {
        Fixture f;
        const auto r=f.read();
        require(r.valid==all,"valid current occurrence");
        require(r.buffers==guest::buffers_address && r.cursor==TOOIE_CAMERA_MAIN_VRAM+0x100u &&
            r.model==0x80501230u && r.asset==0x9876u,"exact DE498 stack and current asset sources");
        require(r.actor==actor && r.slot==slot && r.birth==birth && r.context_epoch==epoch && r.view==view,"current actor context sources");
        require(r.camera==camera_pointer && r.camera_birth==0x135u && r.camera_revision==13u &&
            r.camera_id==0xA0B00C00u && r.camera_scale==0x3FC00000u && r.camera_policy==guest::world_policy,"saved camera metadata");
        for(uint32_t i=0;i<16u;++i) require(r.root[i]==0xC1234560u+i && r.projection[i]==0xB2345670u+i,"all 16 nonzero fixed matrix words decoded");
        const auto before=f.ram;
        recomp_context frame_ctx{}; frame_ctx.r29=(gpr)(int32_t)frame_sp;
        std::array<uint8_t,sizeof(recomp_context)> before_ctx{};
        std::memcpy(before_ctx.data(),&frame_ctx,sizeof(frame_ctx));
        const auto begin=guest::read_frame(f.ram.data(),frame_ctx,guest::Begin);
        const auto end=guest::read_frame(f.ram.data(),frame_ctx,guest::End);
        require((begin.valid&~uint32_t(FrameRecord::Cpu625))==(DrawRecord::Layout|DrawRecord::Frame) &&
            begin.phase==1 && end.phase==2,"frame phases and owner validity");
        require(begin.map==0x4567u && begin.requested_period==-3 && begin.measured_period==-2 && begin.live_period==0x12345678,"original scalar widths and signs");
        require(begin.control_slot==7u && begin.player_owner==0x80600120u && begin.player_model==0x80500230u &&
            begin.player_asset==0x567u && begin.player_admissions==0x345u && begin.player_refusals==0x678u,"current control slot distinct from aggregate player slot");
        require(before==f.ram && std::memcmp(before_ctx.data(),&frame_ctx,sizeof(frame_ctx))==0,"frame must not mutate guest or registers");
        frame_ctx.r29=(gpr)(int32_t)(frame_sp+16u);
        require((guest::read_frame(f.ram.data(),frame_ctx,guest::Begin).valid&~uint32_t(FrameRecord::Cpu625))==
            DrawRecord::Layout,"wrong frame owner invalid");
    }
    {
        // Schema-2 frame words: bridge words need Layout; CPU625 needs its own authenticated header.
        constexpr uint32_t cpu_bit=TOOIE_CPU625_SELECTED ? FrameRecord::Cpu625 : 0u;
        Fixture f;
        const auto r=f.frame();
        require(r.valid==(DrawRecord::Layout|DrawRecord::Frame|cpu_bit),"frame validity includes selected CPU625 only");
        require(r.player_last_reason==(4u|(0x40u<<8)) && r.bridge_next_block==0x1234u,"latest player reason and bridge cursor");
        require(r.cpu625_next_block==(TOOIE_CPU625_SELECTED ? 0x7FFu : 0u),"CPU625 cursor only from selected state");
        require(f.frame(frame_sp+16u).valid==(DrawRecord::Layout|cpu_bit) &&
            f.frame(frame_sp+16u).bridge_next_block==0x1234u,"wrong frame owner keeps layout-authenticated words");
#if TOOIE_CPU625_SELECTED
        for(const uint32_t offset:{0u,4u}) {
            Fixture bad;
            bad.put(TOOIE_CPU625_STATE+offset,offset ? guest::cpu625_layout+1u : 0u);
            const auto b=bad.frame();
            require(!(b.valid&FrameRecord::Cpu625) && b.cpu625_next_block==0u && b.bridge_next_block==0x1234u,
                "unauthenticated CPU625 magic/layout yields no cursor");
        }
#endif
        Fixture bad;
        bad.put(TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,layout),TOOIE_BRIDGE_LAYOUT+1u);
        const auto b=bad.frame();
        require(b.valid==0u && b.player_last_reason==0u && b.bridge_next_block==0u && b.cpu625_next_block==0u,
            "invalid bridge layout records no schema-2 words");
    }
    {
        // PC[8] layout 4: the protected primary ledger and the last admitted view are exported read-only.
        Fixture f;
        f.put(TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,primary_bytes),2u*3792u);
        f.put(TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,primary_highwater),3u*3792u);
        f.put(TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,player)+offsetof(TooiePlayerBoundary,view_phase),
            2u|(5u<<2));
        const auto r=f.frame();
        require(r.player_primary_bytes==2u*3792u && r.player_primary_highwater==3u*3792u && r.player_view==2u,
            "primary ledger and last admitted view are exported");
    }
    {
        // PC[8] layout 5: secondary route counters and the SECONDARY ledger are exported read-only.
        Fixture f;
        constexpr uint32_t route=TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,secondary_route);
        constexpr uint32_t ledger=TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,secondary_ledger);
        f.put(route+offsetof(TooieSecondaryRoute,opens),5u); f.put(route+offsetof(TooieSecondaryRoute,faults),2u);
        f.put(ledger+offsetof(TooieSecondaryLedger,seen_main),3u); f.put(ledger+offsetof(TooieSecondaryLedger,seen_late),1u);
        f.put(ledger+offsetof(TooieSecondaryLedger,unauthenticated),4u); f.put(ledger+offsetof(TooieSecondaryLedger,admissions),2u);
        f.put(ledger+offsetof(TooieSecondaryLedger,bytes),2u*2824u);
        const auto r=f.frame();
        require(r.secondary_opens==5u && r.secondary_faults==2u && r.secondary_seen_main==3u && r.secondary_seen_late==1u &&
            r.secondary_unauthenticated==4u && r.secondary_admissions==2u && r.secondary_ledger_bytes==2u*2824u,
            "secondary route counters and ledger are exported");
    }
    {
        // Optional frame-end shadow words: authenticated End sample only, outside `valid`.
        const auto end=[](Fixture& f,uint32_t owner=frame_sp) {
            const auto before=f.ram;
            recomp_context frame_ctx{}; frame_ctx.r29=(gpr)(int32_t)owner;
            const auto out=guest::read_frame(f.ram.data(),frame_ctx,guest::End);
            require(before==f.ram,"shadow frame read must not mutate guest memory");
            return out;
        };
#if TOOIE_SHADOW_SELECTED
        constexpr uint32_t s=TOOIE_SHADOW_STATE;
        const auto shadow=[&](Fixture& f) {
            const uint32_t words[14]={guest::shadow_magic,epoch,0u,0x80500230u,epoch,0u,0u,0u,0u,
                0u,0u,304u,228u,0x800A6D60u};
            for(uint32_t i=0;i<14u;++i) f.put(s+i*4u,words[i]);
        };
        {
            Fixture f; shadow(f);
            const auto r=end(f);
            require(r.shadow_valid==1u && r.shadow_source_pc==0x800A6D60u && r.shadow_seen_epoch==epoch &&
                !r.shadow_id && !r.shadow_group && !r.shadow_composite && !r.shadow_mode_open && !r.shadow_fault &&
                r.shadow_args==std::array<uint32_t,4>{0u,0u,304u,228u},"End sample exports final shadow words");
            require((r.valid&~uint32_t(FrameRecord::Cpu625))==(DrawRecord::Layout|DrawRecord::Frame),
                "shadow words do not change frame validity bits");
            require(!f.frame().shadow_valid,"Begin sample never exports shadow words");
            require(!end(f,frame_sp+16u).shadow_valid,"wrong construction owner exports nothing");
        }
        for(const uint32_t offset:{0u,4u}) {
            Fixture f; shadow(f); f.put(s+offset,offset ? epoch-1u : 0u);
            const auto r=end(f);
            require(!r.shadow_valid && !r.shadow_source_pc && !r.shadow_seen_epoch,
                "unauthenticated magic or stale epoch exports nothing");
        }
#else
        Fixture f;
        require(!end(f).shadow_valid,"unselected shadow route exports nothing");
#endif
    }
    {
        // Synchronous DEFA0 outcome joins: read-only, first DEFA0 per begin call, unknown stays unset.
        constexpr uint32_t pc=TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,player);
        constexpr uint32_t bridge=TOOIE_BRIDGE_STATE_VRAM;
        constexpr uint32_t player_bits=DrawRecord::PlayerJoin|DrawRecord::PlayerSlot;
        const auto body=[&](Fixture& f,uint32_t calls,uint32_t reason) {
            f.put(guest::draw_callback_address,guest::player_callback);
            f.put(pc+offsetof(TooiePlayerBoundary,enabled),1u);
            f.put(pc+offsetof(TooiePlayerBoundary,calls),calls);
            f.put(pc+offsetof(TooiePlayerBoundary,last_reason),reason);
            f.put(pc+offsetof(TooiePlayerBoundary,owner),actor);
            f.put(pc+offsetof(TooiePlayerBoundary,stamp),epoch);
            f.put(bridge+offsetof(TooieRenderBridgeState,active),reason==0xFFFFFFFFu ? 1u : 0u);
            f.put(actor+guest::player_slot_offset,2u);
            f.put(guest::player_table_address+2u*4u,actor);
        };
        const auto joined=[&](Fixture& f) {
            auto r=f.read();
            const auto before=f.ram;
            guest::join_player(f.ram.data(),r);
            require(before==f.ram,"player join must not mutate guest memory");
            return r;
        };
        {
            Fixture f; body(f,100u,0xFFFFFFFFu);
            const auto r=joined(f);
            require((r.valid&player_bits)==player_bits && r.player_owner==actor && r.player_owner_slot==2u &&
                r.player_table_match==1u && r.player_control_slot==7u && r.player_outcome==0xFFFFFFFFu,
                "admitted body joins with owner, slot, table binding and control slot");
            require(!(joined(f).valid&player_bits),"a later DEFA0 under the same begin call is not attributed");
        }
        {
            Fixture f; body(f,101u,TOOIE_BRIDGE_SLOT); f.put(guest::player_table_address+2u*4u,actor+0xA0u);
            const auto r=joined(f);
            require((r.valid&player_bits)==player_bits && r.player_table_match==0u &&
                r.player_outcome==TOOIE_BRIDGE_SLOT,"refused body keeps its own reason and table mismatch");
        }
        {
            Fixture f; body(f,102u,TOOIE_BRIDGE_VIEW); f.put(guest::draw_owner_address,0x1234u);
            const auto r=joined(f);
            require((r.valid&player_bits)==DrawRecord::PlayerJoin && r.player_owner==0x1234u,
                "out-of-range owner joins without inventing a slot or table result");
        }
        {
            // PC[8]: the one monotonic begin serial joins successive begins of different (slot, view)
            // entries once each and suppresses a repeated sample. A refused repeat of an earlier entry
            // joins its own staged owner and reason, never the last admitted tuple.
            constexpr uint32_t other=actor+0x1000u;
            Fixture f; body(f,400u,0xFFFFFFFFu);
            const auto a=joined(f);
            require((a.valid&player_bits)==player_bits && a.player_owner==actor && a.player_owner_slot==2u &&
                a.player_table_match==1u,"the first entry's begin joins");
            f.put(guest::draw_owner_address,other); f.put(pc+offsetof(TooiePlayerBoundary,owner),other);
            f.put(pc+offsetof(TooiePlayerBoundary,calls),401u);
            f.put(other+guest::player_slot_offset,5u); f.put(guest::player_table_address+5u*4u,other);
            const auto b=joined(f);
            require((b.valid&player_bits)==player_bits && b.player_owner==other && b.player_owner_slot==5u &&
                b.player_table_match==1u && b.player_outcome==0xFFFFFFFFu,"the next entry's begin joins once");
            require(!(joined(f).valid&player_bits),"a repeated sample of that begin joins nothing");
            f.put(guest::draw_owner_address,actor); f.put(pc+offsetof(TooiePlayerBoundary,calls),402u);
            f.put(pc+offsetof(TooiePlayerBoundary,last_reason),TOOIE_BRIDGE_DUPLICATE);
            f.put(bridge+offsetof(TooieRenderBridgeState,active),0u);
            const auto c=joined(f);
            require((c.valid&player_bits)==player_bits && c.player_owner==actor && c.player_owner_slot==2u &&
                c.player_outcome==TOOIE_BRIDGE_DUPLICATE,"a refused repeat joins its staged owner, not the last tuple");
            require(!(joined(f).valid&player_bits),"the refused repeat is joined once");
        }
        struct Miss { uint32_t address,value; const char* why; };
        const Miss misses[]={
            {pc+offsetof(TooiePlayerBoundary,enabled),0u,"disabled begin writes no outcome"},
            {bridge+offsetof(TooieRenderBridgeState,silo_depth),0u,"depth0 DF944 UI route is not a body"},
            {bridge+offsetof(TooieRenderBridgeState,silo_depth),2u,"nested depth2 draw is not this body"},
            {guest::draw_callback_address,0x801015D0u,"non-PlayerState callback is not joined"},
            {pc+offsetof(TooiePlayerBoundary,calls),0xFFFFFFFFu,"saturated begin count proves nothing"},
            {pc+offsetof(TooiePlayerBoundary,owner),actor+0xA0u,"admitted owner must match staged owner"},
            {pc+offsetof(TooiePlayerBoundary,stamp),epoch-1u,"admitted stamp must be this epoch"},
            {bridge+offsetof(TooieRenderBridgeState,active),0u,"admitted outcome needs an active bridge"},
            {bridge+offsetof(TooieRenderBridgeState,layout),TOOIE_BRIDGE_LAYOUT+1u,"invalid bridge layout joins nothing"}};
        uint32_t calls=200u;
        for(const auto& miss:misses) {
            Fixture f; body(f,calls++,0xFFFFFFFFu); f.put(miss.address,miss.value);
            require(!(joined(f).valid&player_bits),miss.why);
        }
        const auto skeletal=[&](uint32_t start,uint32_t dc,uint32_t da,uint32_t dr,uint32_t returned) {
            Fixture f;
            f.put(bridge+offsetof(TooieRenderBridgeState,silo_calls),start);
            auto r=f.read();
            const auto before=guest::skeletal_counters(f.ram.data());
            f.put(bridge+offsetof(TooieRenderBridgeState,silo_calls),before.calls+dc);
            f.put(bridge+offsetof(TooieRenderBridgeState,silo_admissions),before.admissions+da);
            f.put(bridge+offsetof(TooieRenderBridgeState,silo_refusals),before.refusals+dr);
            f.put(bridge+offsetof(TooieRenderBridgeState,silo_last_reason),TOOIE_BRIDGE_ANIMATION);
            guest::join_skeletal(f.ram.data(),before,returned,r);
            return r;
        };
        // The stored reason is ANIMATION in every case: an admission must not export it.
        const auto admitted=skeletal(5u,1u,1u,0u,1u), refused=skeletal(5u,1u,0u,1u,0u);
        require((admitted.valid&DrawRecord::SkeletalJoin) && admitted.skeletal_admissions==1u &&
            admitted.skeletal_refusals==0u && admitted.skeletal_reason==0u,"admitted skeletal call");
        require((refused.valid&DrawRecord::SkeletalJoin) && refused.skeletal_refusals==1u &&
            refused.skeletal_reason==TOOIE_BRIDGE_ANIMATION,"refused skeletal call keeps its reason");
        require(!(skeletal(5u,0u,0u,0u,0u).valid&DrawRecord::SkeletalJoin),"silent nonclass draw stays unattributed");
        require(!(skeletal(5u,1u,0u,0u,0u).valid&DrawRecord::SkeletalJoin) &&
            !(skeletal(5u,2u,1u,1u,1u).valid&DrawRecord::SkeletalJoin),"unexplained counter deltas are not classified");
        require(!(skeletal(5u,1u,1u,0u,0u).valid&DrawRecord::SkeletalJoin),"admission counters with return0 are invalid");
        require(!(skeletal(5u,1u,0u,1u,1u).valid&DrawRecord::SkeletalJoin),"refusal counters with return1 are invalid");
        require(!(skeletal(0xFFFFFFFFu,0u,1u,0u,1u).valid&DrawRecord::SkeletalJoin),"saturated counters are invalid");
        const auto last_step=skeletal(0xFFFFFFFEu,1u,1u,0u,1u);
        require((last_step.valid&DrawRecord::SkeletalJoin) && last_step.skeletal_calls==1u,
            "FFFFFFFE->FFFFFFFF remains one exact counted call");
    }
    invalid(control_address+offsetof(TooieCameraControl,magic),0u,all,"bad camera ABI rejected");
    invalid(TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,layout),TOOIE_BRIDGE_LAYOUT+1u,all,"bad bridge ABI rejected");
    invalid(control_address+offsetof(TooieCameraControl,epoch),epoch+1u,DrawRecord::Frame|DrawRecord::Root|DrawRecord::Projection|DrawRecord::Context,"mismatched frame epoch");
    invalid(sp+0xF8u,guest::buffers_address+8u,DrawRecord::Root|DrawRecord::Projection|DrawRecord::Context,"readable unrelated buffers rejected");
    invalid(guest::matrix_pool_address,0x80FFF000u,DrawRecord::Root|DrawRecord::Projection,"full pool extent required");
    invalid(guest::buffers_address+4u,pool+guest::matrix_pool_bytes,DrawRecord::Root|DrawRecord::Projection,"matrix at pool end rejected");
    invalid(guest::buffers_address+4u,pool+8u,DrawRecord::Root|DrawRecord::Projection,"matrix relative alignment required");
    invalid(context_address+offsetof(TooieActorPaletteContext,buffers),guest::buffers_address+8u,DrawRecord::Context,"context buffers must match");
    invalid(context_address+offsetof(TooieActorPaletteContext,epoch),epoch-1u,DrawRecord::Context,"stale context epoch rejected");
    invalid(TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,births)+slot*4u,birth+1u,DrawRecord::Context,"actor birth must match");
    invalid(0x8012C948u+25u*4u,actor+0xA0u,DrawRecord::Context,"current call actor must match");
    invalid(saved_address+offsetof(TooieCameraProduction,valid),0u,DrawRecord::Projection,"cleared saved projection rejected");
    invalid(saved_address+offsetof(TooieCameraProduction,epoch),epoch-1u,DrawRecord::Projection,"saved projection epoch must match");
    invalid(saved_address+offsetof(TooieCameraProduction,view),view+1u,DrawRecord::Projection,"saved projection view must match");
    invalid(record_address+offsetof(TooieCameraRecord,birth),0x136u,DrawRecord::Projection,"camera birth must match");
    invalid(record_address+offsetof(TooieCameraRecord,revision),14u,DrawRecord::Projection,"camera revision must match");
    invalid(saved_address+offsetof(TooieCameraProduction,policy),0x00000002u,DrawRecord::Projection,"IGNORE policy rejected");
    invalid(record_address+offsetof(TooieCameraRecord,view_scale)+view*4u,0x3F800000u,DrawRecord::Projection,"camera scale key must match");
    invalid(guest::projection_address,pool+128u,DrawRecord::Projection,"live projection pointer must match");
    invalid(control_address+offsetof(TooieCameraControl,latch),1u,DrawRecord::Projection,"latched projection rejected");
    {
        Fixture f;
        // Nonzero signed halfword pool selector, with an unrelated adjacent
        // halfword, must choose the second generated command/matrix pools.
        f.put_half(guest::pool_index_address-2u,0xABCDu);
        f.put_half(guest::pool_index_address,1u);
        auto c=guest::control(f.ram.data());
        c.main_base+=TOOIE_CAMERA_M_BYTES; c.main_begin+=TOOIE_CAMERA_M_BYTES;
        c.main_end+=TOOIE_CAMERA_M_BYTES;
        f.put_abi(control_address,c);
        f.put(guest::gfx_pool_address+4u,c.main_base);
        f.put(guest::matrix_pool_address+4u,pool);
        f.put(sp+0xCCu,c.main_base+0x100u);
        require(f.read().valid==all,"second pool uses signed halfword and generated stride");
        f.put_half(guest::pool_index_address,0xFFFFu);
        const auto invalid_pool=f.read();
        require(invalid_pool.pool_index==0xFFFFFFFFu && invalid_pool.valid==DrawRecord::Layout,"negative pool selector retained and rejected");
    }
    {
        Fixture f;
        f.put(context_address+offsetof(TooieActorPaletteContext,admitted),6u);
        const auto r=f.read();
        require(r.context_admitted==6u && !(r.valid&DrawRecord::Context),"raw admitted retained without authenticating a fresh pre-admit call");
        f.ctx.r29=(gpr)(int32_t)0x80FFFF00u;
        const auto bad=f.read();
        require(bad.valid==DrawRecord::Layout && bad.buffers==0u,"unreadable original stack retained as invalid record");
    }
#if TOOIE_BK_PIPELINE_SELECTED
    bk_root_cases();
#endif
    std::printf("PASS baseline guest issue evidence (%u draw cases plus frame metadata; cpu625_selected=%u; "
        "bk_selected=%u bk_root_cases=%u)\n",
        cases,unsigned(TOOIE_CPU625_SELECTED),unsigned(TOOIE_BK_PIPELINE_SELECTED),bk_cases);
}
