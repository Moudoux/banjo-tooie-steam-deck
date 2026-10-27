// Included after BK pipeline helpers in the existing adapters.cpp carrier.
#if TOOIE_BK_PIPELINE_SELECTED
#include "menu_scope.hpp"
#include "menu_witness.hpp"
#include "transition_scope.hpp"
#include "ui_scope.hpp"
extern "C" void tooie_bk_menu_model_begin(uint8_t*,recomp_context*);
extern "C" void tooie_bk_menu_model_end(uint8_t*,recomp_context*);
// Transition projection role (bk_pipeline.c): the guest camera role word is written only by the guest.
extern "C" void tooie_bk_transition_role_begin(uint8_t*,recomp_context*);
extern "C" void tooie_bk_transition_role_end(uint8_t*,recomp_context*);
namespace bk_menu {
namespace witness=tooie_menu_witness;
using namespace tooie_menu;
struct Activation { uint32_t sp,kind,box,gate,token,root_groups,all_roots,all_groups; Context previous; };
struct Resident {
    State state{}; uint8_t* ram=nullptr; uint32_t last_frame=0,depth=0,box=0,gate=0;
    Activation stack[32]{};
};
static Resident live;
static uint32_t frame(uint8_t* rdram) {
    const uint32_t raw=uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET,gpr(int32_t(TOOIE_BK_STATE))));
    if(live.ram!=rdram || raw<live.last_frame) { witness::hit(witness::Reset);live=Resident{};live.ram=rdram; }
    witness::current.bk_frame=raw;
    live.last_frame=raw;
    return raw==0xFFFFFFFFu?0:raw+1; // Allows initialization before construction1.
}
static void push(uint32_t sp,uint32_t kind) {
    // Original menu code has no recursive callbacks in these lexical scopes.
    // Overflow is a programming error: never inherit an owner's IDs silently.
    if(live.depth==32)rejected_body("menu native activation overflow");
    live.stack[live.depth++]={sp,kind,live.box,live.gate,0,0,0,0,live.state.active};
}
static bool top(uint32_t sp,uint32_t kind) {
    return live.depth && live.stack[live.depth-1].sp==sp && live.stack[live.depth-1].kind==kind;
}
static void pop() {
    const auto saved=live.stack[--live.depth];live.state.active=saved.previous;
    live.box=saved.box;live.gate=saved.gate;
}
static void box_begin(uint8_t* rdram,recomp_context* ctx,uint32_t role,uint32_t slot) {
    const uint32_t enclosing=live.state.active.valid;
    (void)rdram;witness::hit(witness::Box);push(uint32_t(ctx->r29),2+role);
    live.box=uint32_t(ctx->r4);live.gate=0;
    if(!live.box || uint32_t(ctx->r21)!=live.state.active.gfx)live.state.active.valid=0;
    occurrence(live.state,slot,role);
    witness::current.box=live.box;witness::current.slot=slot;witness::current.role=role;
    witness::current.active_valid=live.state.active.valid;
    // Only a box refused under an admitted option; an unadmitted option was already counted at enter.
    if(enclosing && !live.state.active.valid)witness::hit(witness::BoxInvalid);
}
static void box_end(recomp_context* ctx,uint32_t role) {
    if(top(uint32_t(ctx->r29),2+role))pop();
}
}
extern "C" void baseline_bk_menu_init(uint8_t* rdram,recomp_context* ctx) {
    const uint32_t f=bk_menu::frame(rdram);
    bk_menu::witness::hit(bk_menu::witness::Init);
    bk_menu::witness::current.init_owner=uint32_t(ctx->r4);
    if(!tooie_menu::initialize(bk_menu::live.state,uint32_t(ctx->r4),f))
        bk_menu::witness::hit(bk_menu::witness::InitRefused);
}
extern "C" void baseline_bk_menu_free(uint8_t* rdram,recomp_context* ctx) {
    bk_menu::witness::hit(bk_menu::witness::Free);
    bk_menu::witness::current.free_owner=uint32_t(ctx->r4);
    bk_menu::frame(rdram);tooie_menu::retire(bk_menu::live.state,uint32_t(ctx->r4));
}
extern "C" void baseline_bk_menu_enter(uint8_t* rdram,recomp_context* ctx) {
    const uint32_t f=bk_menu::frame(rdram);
    bk_menu::push(uint32_t(ctx->r29),1);
    namespace mw=tooie_menu_witness;
    mw::hit(mw::Enter);mw::current.owner=uint32_t(ctx->r4);mw::current.gfx=uint32_t(ctx->r5);
    bool registered=false,limit=false,gap=false;mw::current.live_slots=0;
    for(uint32_t i=0;i<tooie_menu::slots;++i)mw::current.registered[i]=bk_menu::live.state.entries[i].owner;
    for(const auto& e:bk_menu::live.state.entries){
        mw::current.live_slots+=e.owner!=0;
        if(e.owner==uint32_t(ctx->r4) && e.owner){
            registered=true;limit=e.frame==f && e.draws>=tooie_menu::invocations;gap=f>e.frame && f-e.frame>1u;
        }
    }
    tooie_menu::enter(bk_menu::live.state,uint32_t(ctx->r4),uint32_t(ctx->r5),f);
    if(!bk_menu::live.state.active.valid){
        if(!registered)mw::hit(mw::EnterUnregistered);
        else if(limit)mw::hit(mw::EnterInvocationLimit);
        else mw::hit(mw::EnterInvalid);
    }else if(!registered)mw::hit(mw::EnterAdmitted);  // registered[]/live_slots above are the pre-enter table
    else if(gap)mw::hit(mw::EnterGapCut);
    mw::current.active_valid=bk_menu::live.state.active.valid;
    bk_menu::live.box=0;bk_menu::live.gate=0;
}
extern "C" void baseline_bk_menu_leave(uint8_t*,recomp_context* ctx) {
    if(!bk_menu::top(uint32_t(ctx->r29)+0x38,1))rejected_body("menu option activation mismatch");
    bk_menu::pop();
}
extern "C" void baseline_bk_menu_primary(uint8_t* r,recomp_context* c) {bk_menu::box_begin(r,c,tooie_menu::primary,uint32_t(c->r19));}
extern "C" void baseline_bk_menu_primary_end(uint8_t*,recomp_context* c) {bk_menu::box_end(c,tooie_menu::primary);}
extern "C" void baseline_bk_menu_upper(uint8_t* r,recomp_context* c) {bk_menu::box_begin(r,c,tooie_menu::upper_wrap,(uint32_t(c->r16)-uint32_t(c->r17))/4);}
extern "C" void baseline_bk_menu_upper_end(uint8_t*,recomp_context* c) {bk_menu::box_end(c,tooie_menu::upper_wrap);}
extern "C" void baseline_bk_menu_lower(uint8_t* r,recomp_context* c) {bk_menu::box_begin(r,c,tooie_menu::lower_wrap,(uint32_t(c->r16)-uint32_t(c->r17))/4);}
extern "C" void baseline_bk_menu_lower_end(uint8_t*,recomp_context* c) {bk_menu::box_end(c,tooie_menu::lower_wrap);}
extern "C" void baseline_bk_menu_extra(uint8_t* r,recomp_context* c) {bk_menu::box_begin(r,c,tooie_menu::extra,0);}
extern "C" void baseline_bk_menu_extra_end(uint8_t*,recomp_context* c) {bk_menu::box_end(c,tooie_menu::extra);}
extern "C" void baseline_bk_menu_box_model(uint8_t*,recomp_context* ctx) {
    namespace mw=tooie_menu_witness;
    bk_menu::push(uint32_t(ctx->r29),6);
    bk_menu::live.gate=(bk_menu::live.box==uint32_t(ctx->r16) &&
        bk_menu::live.state.active.gfx==uint32_t(ctx->r4));
    // 80801424 serves every zoombox: only one inside an option box occurrence is a candidate.
    if(!bk_menu::live.box){mw::hit(mw::ModelOutsideOption);return;}
    mw::hit(mw::ModelScope);
    mw::current.model_box=uint32_t(ctx->r16);mw::current.model_gfx=uint32_t(ctx->r4);
    mw::current.gate=bk_menu::live.gate;
    if(!bk_menu::live.gate)mw::hit(mw::ModelGateRefused);
}
extern "C" void baseline_bk_menu_box_model_end(uint8_t*,recomp_context* ctx) {
    if(!bk_menu::top(uint32_t(ctx->r29),6))rejected_body("menu box activation mismatch");bk_menu::pop();
}
extern "C" void baseline_bk_menu_model_begin(uint8_t* rdram,recomp_context* ctx) {
    namespace mw=tooie_menu_witness;
    bk_menu::push(uint32_t(ctx->r29),7);
    // fxdialog3/DF944 also serves non-option dialogs: only a gated box-model scope is a candidate
    // (its refusal was counted at 8080148C). Ungated dialogs are taken at DF944 (bk_ui below).
    if(!bk_menu::live.gate)return;
    const uint32_t id=tooie_menu::identity(bk_menu::live.state,uint32_t(ctx->r4),tooie_menu::model);
    // Original entry3 resolved9F1 into its seventh DF944 argument atsp+18.
    mw::hit(mw::ModelCall);mw::current.id=id;mw::current.dialog_gfx=uint32_t(ctx->r4);
    mw::current.resource=uint32_t(MEM_W(0x18,ctx->r29));mw::current.token=0;
    if(!id){mw::hit(mw::IdentityRefused);return;}
    if(!mw::current.resource){mw::hit(mw::ResourceAbsent);return;}
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=id;clone.r5=bk_menu::live.state.active.cut;
    auto& a=bk_menu::live.stack[bk_menu::live.depth-1];
    a.root_groups=uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET+8,gpr(int32_t(TOOIE_BK_STATE))));
    a.all_roots=uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET+4,gpr(int32_t(TOOIE_BK_STATE))));
    a.all_groups=uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET+12,gpr(int32_t(TOOIE_BK_STATE))));
    tooie_bk_menu_model_begin(rdram,&clone);
    a.token=uint32_t(clone.r2);
    mw::current.token=uint32_t(clone.r2);
    mw::hit(clone.r2?mw::TokenAccepted:mw::TokenRefused);
}
extern "C" void baseline_bk_menu_model_end(uint8_t* rdram,recomp_context* ctx) {
    if(!bk_menu::top(uint32_t(ctx->r29),7))rejected_body("menu model activation mismatch");
    bk_menu::witness::hit(bk_menu::witness::ModelEnd);
    const auto& a=bk_menu::live.stack[bk_menu::live.depth-1];
    if(a.token){
        recomp_context clone;camera_clone_context(clone,*ctx);clone.r4=a.token;
        tooie_bk_menu_model_end(rdram,&clone);
        const uint32_t root_groups=uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET+8,gpr(int32_t(TOOIE_BK_STATE))));
        const uint32_t roots=uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET+4,gpr(int32_t(TOOIE_BK_STATE))));
        const uint32_t groups=uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET+12,gpr(int32_t(TOOIE_BK_STATE))));
        // A header-bit40 root opens no modelview root group, but its graph groups carry the menu ID:
        // either delta proves this workload emitted the owned commands.
        if(root_groups!=a.root_groups || groups!=a.all_groups)tooie_menu::emitted(bk_menu::live.state);
        auto& mw=bk_menu::witness::current;
        mw.group_delta=root_groups-a.root_groups;
        if(mw.group_delta)bk_menu::witness::hit(bk_menu::witness::RootGroupDelta);
        mw.root_delta=roots-a.all_roots;
        mw.graph_group_delta=groups-a.all_groups;
    }
    bk_menu::pop();
}
/* ---- Transition masks (BK transition_patches.c gctransition_draw model-tag parity, partial) ------
 * gctransitionDll entry0 80800324 draw(t=a0, gfx=a1); every exit passes 80800870 (s0 = t, frame 0x70).
 * Its mask model draws are the DE448 calls at 80800484 (state 2/7), 8080059C (5) and 808006CC (6),
 * returning at +8. Each owned call gets the existing BK UI owner scope with the view's ID; DE448 is
 * marked as the owner's own draw, so the generic root/graph/float path tags it under the existing MAIN
 * provisioning (no new command allocation). Identity is the view (transition_scope.hpp); lifetime is
 * the original 808000D8 / C9214 / C929C events; freshness clears only at an actual submission.
 * The 808003D8 projection, fades, scale, E41CC/E4208 and every original operand stay unchanged. */
namespace bk_transition {
constexpr uint32_t entry0_frame=0x70, global_buffers=0x80127658u;
constexpr uint32_t registry=0x8012AB40u, registry_count=0x8012AB54u;   // u32 pointers[5], u8 count
struct Resident {
    uint8_t* ram=nullptr; uint32_t last_frame=0,object=0,gfx=0,entry_sp=0,view=0xFFFFFFFFu,token=0,sp=0,root_groups=0,groups=0;
    uint32_t projection_active=0,projection_sp=0,previous_role=0;
    tooie_transition::State state{};
};
static Resident live;
static uint32_t frame(uint8_t* rdram) {
    const uint32_t raw=uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET,static_cast<gpr>(static_cast<int32_t>(TOOIE_BK_STATE))));
    if(live.ram!=rdram || raw<live.last_frame) { live=Resident{};live.ram=rdram; }
    live.last_frame=raw;
    return raw==0xFFFFFFFFu?0:raw;
}
// The +0 view of a live transition object; out of range when the pointer is not plain RDRAM.
static uint32_t view_of(uint8_t* rdram,uint32_t object) {
    return bk_pipeline::lod_word(object,0x34)?uint32_t(MEM_W(0,static_cast<gpr>(static_cast<int32_t>(object)))):0xFFFFFFFFu;
}
// Read the original registry; only listed (live) pointers are dereferenced.
static uint32_t listed(uint8_t* rdram,uint32_t pointers[tooie_transition::slots],uint32_t views[tooie_transition::slots]) {
    const uint32_t count=uint32_t(MEM_BU(0,static_cast<gpr>(static_cast<int32_t>(registry_count))));
    for(uint32_t i=0;i<tooie_transition::slots;++i){pointers[i]=0;views[i]=0xFFFFFFFFu;}
    if(count>tooie_transition::slots)return count;
    for(uint32_t i=0;i<count;++i){
        pointers[i]=uint32_t(MEM_W(int32_t(4u*i),static_cast<gpr>(static_cast<int32_t>(registry))));
        views[i]=view_of(rdram,pointers[i]);
    }
    return count;
}
// The admitted MAIN list: the camera construction is active and unlatched, no redirected fill.
static bool camera_main(uint8_t* rdram) {
    constexpr uint32_t c=TOOIE_CAMERA_STATE_VRAM+offsetof(TooieCameraState,control);
    return MEM_W(int32_t(offsetof(TooieCameraControl,active)),static_cast<gpr>(static_cast<int32_t>(c))) &&
        !MEM_W(int32_t(offsetof(TooieCameraControl,latch)),static_cast<gpr>(static_cast<int32_t>(c))) &&
        !MEM_W(0,static_cast<gpr>(static_cast<int32_t>(0x8012D508u)));
}
}
// Entry0's first instruction: this activation's object (alive here), its view and the caller's sp.
extern "C" void baseline_bk_transition_enter(uint8_t* rdram,recomp_context* ctx) {
    auto& L=bk_transition::live;bk_transition::frame(rdram);
    L.object=uint32_t(ctx->r4);L.gfx=uint32_t(ctx->r5);L.entry_sp=uint32_t(ctx->r29);L.token=0;
    L.view=bk_transition::view_of(rdram,L.object);
}
// Before 80800870, the common exit of every path (state1 and the state2/7 restore included).
extern "C" void baseline_bk_transition_leave(uint8_t*,recomp_context*) {
    auto& L=bk_transition::live;L.object=0;L.entry_sp=0;L.view=0xFFFFFFFFu;L.token=0;
}
static_assert(tooie_transition::views==TOOIE_CAMERA_TRANSITION_VIEWS &&
    TOOIE_CAMERA_TRANSITION_ID_END==TOOIE_CAMERA_TRANSITION_ID_FIRST+TOOIE_CAMERA_TRANSITION_VIEWS,
    "transition role/projection IDs cover exactly the transition views");
// Original transition E44FC at 808003D8; a0 is loaded only by its 3DC delay slot.
// Authenticate the saved outgoing graphics word at sp+74 instead of reading stale a0.
extern "C" void baseline_bk_transition_projection_begin(uint8_t* rdram,recomp_context* ctx) {
    auto& L=bk_transition::live;
    const uint32_t f=bk_transition::frame(rdram);
    if(L.projection_active)rejected_body("transition projection scope reentry");
    uint32_t pointers[tooie_transition::slots],views[tooie_transition::slots];
    const uint32_t count=bk_transition::listed(rdram,pointers,views);
    if(!f || !L.object || L.entry_sp!=uint32_t(ctx->r29)+bk_transition::entry0_frame ||
       L.gfx!=bk_transition::global_buffers || uint32_t(MEM_W(0x74,ctx->r29))!=L.gfx ||
       !bk_transition::camera_main(rdram) ||
       !tooie_transition::registered(count,pointers,views,L.object,L.view) ||
       L.state.v[L.view].drawn_frame==f)return;
    const uint32_t kind=uint32_t(MEM_BU(0x1C,static_cast<gpr>(static_cast<int32_t>(L.object))));
    if(kind!=2u && kind!=5u && kind!=6u && kind!=7u)return; // Model masks only; fades unchanged.
    // The camera production role of this view's mask projection is written by the guest
    // (tooie_bk_transition_role_begin); this scope keeps only the enclosing role to restore.
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=(gpr)(int32_t)L.view;clone.r5=L.state.v[L.view].fresh?1:0;
    tooie_bk_transition_role_begin(rdram,&clone);
    L.previous_role=uint32_t(clone.r2);
    L.projection_sp=uint32_t(ctx->r29);L.projection_active=1;
}
// First instruction after E44FC returns; always restore a paid role, even if policy emission latched.
extern "C" void baseline_bk_transition_projection_end(uint8_t* rdram,recomp_context* ctx) {
    auto& L=bk_transition::live;if(!L.projection_active)return;
    if(L.projection_sp!=uint32_t(ctx->r29))rejected_body("transition projection scope mismatch");
    recomp_context clone;camera_clone_context(clone,*ctx);clone.r4=(gpr)(int32_t)L.previous_role;
    tooie_bk_transition_role_end(rdram,&clone);
    L.projection_active=0;
}
// Before one of the three mask DE448 JALs: the view's owned scope, or the original draw.
extern "C" void baseline_bk_transition_model(uint8_t* rdram,recomp_context* ctx) {
    namespace mw=tooie_menu_witness;
    auto& L=bk_transition::live;
    const uint32_t f=bk_transition::frame(rdram);
    uint32_t pointers[tooie_transition::slots],views[tooie_transition::slots];
    L.token=0;L.sp=uint32_t(ctx->r29);mw::hit(mw::TransitionModel);
    const uint32_t count=bk_transition::listed(rdram,pointers,views);
    if(!f || !L.object || L.entry_sp!=uint32_t(ctx->r29)+bk_transition::entry0_frame ||
       L.gfx!=bk_transition::global_buffers || !bk_transition::camera_main(rdram) ||
       !tooie_transition::registered(count,pointers,views,L.object,L.view)){mw::hit(mw::TransitionRefused);return;}
    const tooie_transition::Decision d=tooie_transition::draw(L.state,L.view,f);
    if(!d.id){mw::hit(mw::TransitionRefused);return;}
    L.root_groups=uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET+8,static_cast<gpr>(static_cast<int32_t>(TOOIE_BK_STATE))));
    L.groups=uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET+12,static_cast<gpr>(static_cast<int32_t>(TOOIE_BK_STATE))));
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=d.id;clone.r5=d.cut|2u;   // bit1: the following DE448 is this owner's own draw
    tooie_bk_menu_model_begin(rdram,&clone);
    L.token=uint32_t(clone.r2);
    mw::hit(!L.token?mw::TransitionRefused:d.cut?mw::TransitionCut:mw::TransitionTagged);
}
// The DE448 return continuation (pc+8): commit only actual root or graph group emission.
// A root visit alone can carry no tagged commands, particularly on a bit40 path.
extern "C" void baseline_bk_transition_model_end(uint8_t* rdram,recomp_context* ctx) {
    auto& L=bk_transition::live;
    if(L.token && L.sp==uint32_t(ctx->r29)){
        recomp_context clone;camera_clone_context(clone,*ctx);clone.r4=L.token;
        tooie_bk_menu_model_end(rdram,&clone);
        if(uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET+8,static_cast<gpr>(static_cast<int32_t>(TOOIE_BK_STATE))))!=L.root_groups ||
           uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET+12,static_cast<gpr>(static_cast<int32_t>(TOOIE_BK_STATE))))!=L.groups)
            tooie_transition::emitted(L.state,L.view,bk_transition::frame(rdram));
    }
    L.token=0;
}
// 808000D8(a0=object, a1=state, a2=config) entry: the object is alive; it may be freed inside.
extern "C" void baseline_bk_transition_config(uint8_t* rdram,recomp_context* ctx) {
    auto& L=bk_transition::live;bk_transition::frame(rdram);
    const uint32_t view=bk_transition::view_of(rdram,uint32_t(ctx->r4));
    if(view<tooie_transition::views)tooie_transition::invalidate(L.state,view);
    else tooie_transition::invalidate_all(L.state);
}
// C9214 (free + compact) entry: invalidate only a matching live object, as the original free does.
extern "C" void baseline_bk_transition_free(uint8_t* rdram,recomp_context* ctx) {
    auto& L=bk_transition::live;bk_transition::frame(rdram);
    uint32_t pointers[tooie_transition::slots],views[tooie_transition::slots];
    const uint32_t count=bk_transition::listed(rdram,pointers,views);
    for(uint32_t i=0;i<count && i<tooie_transition::slots;++i)
        if(pointers[i]==uint32_t(ctx->r4) && views[i]<tooie_transition::views){tooie_transition::invalidate(L.state,views[i]);return;}
    // An unlisted pointer is an original no-op; preserve unrelated view continuity.
}
// C929C (registry reset) entry: every view.
extern "C" void baseline_bk_transition_reset(uint8_t* rdram,recomp_context*) {
    bk_transition::frame(rdram);tooie_transition::invalidate_all(bk_transition::live.state);
}
/* ---- Unowned DF944 UI models (BK projection_transform_tagging.c / hud_transform_tagging.c, partial) ----
 * Core DF944 draws one UI model by calling DE498 directly at DFAA0 (a1 = its sp+0x44 local position) and
 * resumes at DFAA8. Option boxes already own that draw through fxdialog3 80800174 (menu scope above).
 * Every other DF944 draw had no BK owner: its header-bit40 projection got IGNORE (unmapped, so it snaps)
 * and a plain root got no group. Post-4.4 F4: the save-select unowned bit40 roots and the warp menu's
 * ungated fxdialog3 dialog (ModelEnd minus ModelCall) all enter DE498 with position = DF944 sp+0x44.
 * BK gives every zoombox/HUD element an ID from its drawer's element and draw index; here the element is
 * the drawn model and its draw ordinal in this construction (ui_scope.hpp), under the existing menu owner
 * scope with an ID from the UI domain, so the existing DEFB0 menu projection (shadow_projection.c) and the
 * generic root/graph/float path tag it. DFAA0 only arms; the model is DE498's sixth argument, read at
 * DE498's own entry (after the call's delay slot) when that entry has DF944's exact sp. Only a top-level
 * draw with no BK owner, not a gated menu model, in the camera MAIN construction is taken; every other
 * draw, and every refusal, stays original. No guest memory is written here. */
namespace bk_ui {
struct Resident { uint8_t* ram=nullptr; uint32_t last_frame=0,armed_sp=0,active_sp=0,token=0; tooie_ui::State state{}; };
static Resident live;
static uint32_t frame(uint8_t* rdram) {
    const uint32_t raw=uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET,static_cast<gpr>(static_cast<int32_t>(TOOIE_BK_STATE))));
    if(live.ram!=rdram || raw<live.last_frame) { live=Resident{};live.ram=rdram; }
    live.last_frame=raw;
    return raw==0xFFFFFFFFu?0:raw+1;
}
static uint32_t bk_word(uint8_t* rdram,size_t offset) {
    return uint32_t(MEM_W(int32_t(offset),static_cast<gpr>(static_cast<int32_t>(TOOIE_BK_STATE))));
}
}
static_assert(tooie_ui::first_id==TOOIE_BK_UI_ID_START && tooie_ui::end_id==TOOIE_BK_UI_ID_END &&
    tooie_ui::stride==2u*TOOIE_BK_TRANSFORM_ID_COUNT,"native UI identities are the guest UI domain's model components");
// DF944 before its direct DE498 JAL at DFAA0: arm this activation's draw, or leave it original.
extern "C" void baseline_bk_ui_model_begin(uint8_t* rdram,recomp_context* ctx) {
    auto& L=bk_ui::live;
    const uint32_t f=bk_ui::frame(rdram);
    L.armed_sp=0;
    if(!f || L.active_sp ||
       bk_ui::bk_word(rdram,offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,kind))!=TOOIE_BK_OWNER_NONE ||
       bk_ui::bk_word(rdram,offsetof(TooieBkState,draw_depth)) || bk_ui::bk_word(rdram,offsetof(TooieBkState,root_depth)) ||
       (bk_menu::live.depth && bk_menu::live.stack[bk_menu::live.depth-1].kind==7 && bk_menu::live.gate) ||
       !bk_transition::camera_main(rdram))return;
    L.armed_sp=uint32_t(ctx->r29);
}
// DE498 first instruction (baseline_bk_de498_entry, bk_pipeline_thunks.hpp): the armed DF944 call only.
static void baseline_bk_ui_de498_entry(uint8_t* rdram,recomp_context* ctx) {
    auto& L=bk_ui::live;
    const uint32_t f=bk_ui::frame(rdram);
    if(!L.armed_sp || L.armed_sp!=uint32_t(ctx->r29))return;
    L.armed_sp=0;
    const tooie_ui::Decision d=tooie_ui::draw(L.state,uint32_t(MEM_W(0x14,ctx->r29)),f);
    if(!d.id)return;
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=d.id;clone.r5=d.cut;   // direct DE498: no own-DE448 flag
    tooie_bk_menu_model_begin(rdram,&clone);
    L.token=uint32_t(clone.r2);
    if(L.token)L.active_sp=uint32_t(ctx->r29);
}
// DFAA8, DF944's DE498 return continuation: close only this activation's owned draw.
extern "C" void baseline_bk_ui_model_end(uint8_t* rdram,recomp_context* ctx) {
    auto& L=bk_ui::live;
    L.armed_sp=0;
    if(!L.active_sp || L.active_sp!=uint32_t(ctx->r29))return;
    recomp_context clone;camera_clone_context(clone,*ctx);clone.r4=L.token;
    tooie_bk_menu_model_end(rdram,&clone);
    L.active_sp=0;L.token=0;
}
// A78E8 (baseline_bk_submit_register): this construction is the submitted workload.
static void baseline_bk_menu_submitted(uint8_t* rdram) {
    const uint32_t f=bk_menu::frame(rdram);if(f)tooie_menu::submitted(bk_menu::live.state,f);
    tooie_transition::submitted(bk_transition::live.state,bk_transition::frame(rdram));
}
// Option-box sprite component owner (warp opening): after bk_menu, bk_transition and bk_ui.
#include "bk_menu_sprite_thunks.hpp"
// Option-box text owner (warp opening): after the sprite owner.
#include "bk_menu_text_thunks.hpp"
#endif
