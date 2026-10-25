// Unmodified extracted original and compiled candidate bodies; no DE448 stub.
#include "recomp.h"
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <initializer_list>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iterator>
#include <vector>
#ifdef TOOIE_SKELETAL_FIXTURE
#include "npc665_selection_layout.h"
#endif
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
#include "camera_identity_layout.h"
#include "../tools/baseline/guest/camera-candidate/camera_identity.h"
#ifdef TOOIE_CAMERA_PRODUCTION_FIXTURE
#include "../tools/baseline/guest/camera-candidate/camera_float.h"
#endif
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
#include "shadow_projection_layout.h"
#endif
#ifdef TOOIE_CKCOPY_FIXTURE
#include "ckcopy_selection_layout.h"
#include "cpu625_selection_layout.h"
#endif
#endif
#ifdef TOOIE_PLAYER_STATE_ADDRESS
#include "../tools/baseline/guest/silo-candidate/render_bridge.h"
#include <cstddef>
#endif
#ifdef TOOIE_BK_DRAW_FIXTURE
#include "bk_pipeline_selection_layout.h"
#include "ckcopy_selection_layout.h"
#include "../tools/baseline/guest/silo-candidate/bk_pipeline.h"
static_assert(TOOIE_BK_PIPELINE_SELECTED==1u && TOOIE_CKCOPY_SELECTED==0u,
    "BK draw fixture follows the actual retired CKCOPY selection");
#endif
#ifndef TOOIE_DRAW_101808_FRAME_BYTES
#error Define candidate frame sizes from actual translated prologues
#endif
#ifndef TOOIE_DRAW_DE448_FRAME_BYTES
#error Define candidate frame sizes from actual translated prologues
#endif
#ifndef TOOIE_DRAW_BK_NESTED_FRAME_BYTES
#error Define selected BK callee depth from actual translated prologues
#endif
extern "C" {
void func_800DF47C(uint8_t*, recomp_context*);
void func_801027F4(uint8_t*, recomp_context*);
void func_801018D8(uint8_t*, recomp_context*);
void func_8010347C(uint8_t*, recomp_context*);
void func_800A7180(uint8_t*, recomp_context*);
void func_800DE498(uint8_t*, recomp_context*);
// The original and patch recompiler name the same guest import differently (doubles below).
void osVirtualToPhysical(uint8_t*, recomp_context*);
void osVirtualToPhysical_recomp(uint8_t*, recomp_context*);
#ifdef TOOIE_PLAYER_STATE_ADDRESS
void func_800AE080(uint8_t*, recomp_context*);
void _babackpack_entrypoint_9(uint8_t*,recomp_context*);
void func_800936E8(uint8_t*,recomp_context*);
void func_80093700(uint8_t*,recomp_context*);
void func_800AE020(uint8_t*,recomp_context*);
void func_800E09B8(uint8_t*,recomp_context*);
void func_800AE6FC(uint8_t*,recomp_context*);
void func_800DBFF8(uint8_t*,recomp_context*);
void func_800EFD24(uint8_t*,recomp_context*);
void func_80093370(uint8_t*,recomp_context*);
void func_8009337C(uint8_t*,recomp_context*);
void func_80093230(uint8_t*,recomp_context*);
void func_800931AC(uint8_t*,recomp_context*);
void func_80092778(uint8_t*,recomp_context*);
void func_80092864(uint8_t*,recomp_context*);
void func_80098E64(uint8_t*,recomp_context*);
void heap_free(uint8_t*,recomp_context*);

void func_800A89F8(uint8_t*, recomp_context*);
void func_800EB854(uint8_t*, recomp_context*);
#endif
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
void func_80100368(uint8_t*, recomp_context*);
#ifdef TOOIE_SKELETAL_FIXTURE
void func_80100094(uint8_t*, recomp_context*);
#endif
void func_80104148(uint8_t*, recomp_context*);
void func_80104200(uint8_t*, recomp_context*);
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
void func_80101870(uint8_t*, recomp_context*);
void func_80103110(uint8_t*, recomp_context*);
#endif
#endif
#ifdef TOOIE_CAMERA_PRODUCTION_FIXTURE
#ifndef TOOIE_ACTOR_PALETTE_FIXTURE
#error The R16 camera production fixture requires the camera identity composition
#endif
void func_800CA7E4(uint8_t*, recomp_context*);
void func_800CA7AC(uint8_t*, recomp_context*);
void func_800CB124(uint8_t*, recomp_context*);
void func_800CA6E8(uint8_t*, recomp_context*);
void func_800E443C(uint8_t*, recomp_context*);
void func_80018BC4(uint8_t*, recomp_context*);
void mlMtxApply(uint8_t*, recomp_context*);
void guMtxIdent(uint8_t*, recomp_context*);
void tooie_get_latched_widescreen_settings(uint8_t*, recomp_context*);
void func_800A8984(uint8_t*, recomp_context*);
void func_800A8A04(uint8_t*, recomp_context*);
void func_800A8AD4(uint8_t*, recomp_context*);
void func_800A8AF0(uint8_t*, recomp_context*);
void func_80015D14(uint8_t*, recomp_context*);
void guOrtho(uint8_t*, recomp_context*);
#endif
}
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
namespace close_stop {
struct RequiredStop : std::runtime_error {
    uint32_t site;
    explicit RequiredStop(uint32_t value):std::runtime_error("unexpected bridge required stop"),site(value){}
};
unsigned calls=0;
uint32_t last_site=0;
}
#endif
namespace original {
void func_800DE448(uint8_t*, recomp_context*);
#include "original_101808_fixture.inc"
#include "original_de448_fixture.inc"
}
namespace candidate {
#ifdef TOOIE_SKELETAL_FIXTURE
void tooie_npc665_admit(uint8_t*,recomp_context*);
void tooie_npc665_bone_push(uint8_t*,recomp_context*);
void tooie_npc665_bone_pop(uint8_t*,recomp_context*);
void tooie_npc665_leaf_begin(uint8_t*,recomp_context*);
void tooie_npc665_leaf_end(uint8_t*,recomp_context*);
void tooie_actor_cpu_root_begin(uint8_t*,recomp_context*);
void tooie_actor_feather_leaf_id(uint8_t*,recomp_context*);
void func_800DD024(uint8_t*,recomp_context*);
#endif
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
// The real native import cannot return. Throw before any guest continuation.
void tooie_bridge_required_stop(uint8_t*,recomp_context* ctx) {
    ++close_stop::calls;close_stop::last_site=uint32_t(ctx->r4);
    throw close_stop::RequiredStop(close_stop::last_site);
}
#endif
void func_800DE448(uint8_t*, recomp_context*);
#ifdef TOOIE_PLAYER_STATE_ADDRESS
void tooie_npc665_end(uint8_t*, recomp_context*);
void tooie_cpu625_retire(uint8_t*, recomp_context*);
void tooie_backpack621_retire(uint8_t*, recomp_context*);
// Trot is outside this drawing fixture's state. Its dedicated production-MIPS
// fixture checks retirement behavior; the lifecycle fixture checks call order.
void tooie_trot_toggle_retire(uint8_t*, recomp_context*) {}
#ifdef TOOIE_NPC665_FIXTURE
// Preserve the native checked-end contract using the exact selected guest body.
void baseline_npc665_end_checked(uint8_t* rdram, recomp_context* ctx) {
    tooie_npc665_end(rdram, ctx);
    if(ctx->r2) throw std::runtime_error("NPC665 guest paid-close ownership tripwire");
}
#endif
void tooie_player_begin(uint8_t*, recomp_context*);
void tooie_player_bone_push(uint8_t*, recomp_context*);
void tooie_player_bone_pop(uint8_t*, recomp_context*);
void tooie_player_end(uint8_t*, recomp_context*);
void tooie_player_leaf_begin(uint8_t*, recomp_context*);
void tooie_player_leaf_end(uint8_t*, recomp_context*);
#ifdef TOOIE_PLAYER_SECONDARY_FIXTURE
void tooie_secondary_route_begin(uint8_t*, recomp_context*);
void tooie_secondary_route_end(uint8_t*, recomp_context*);
void tooie_secondary_route_arm(uint8_t*, recomp_context*);
void tooie_secondary_route_disarm(uint8_t*, recomp_context*);
void tooie_player_secondary_begin(uint8_t*, recomp_context*);
#endif
void tooie_actor_palette_end(uint8_t*, recomp_context*);
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
void tooie_camera_construction_begin(uint8_t*, recomp_context*);
void tooie_camera_construction_end(uint8_t*, recomp_context*);
void tooie_actor_palette_begin(uint8_t*, recomp_context*);
void func_8008B10C(uint8_t*, recomp_context*);
void anctrl_setSmoothTransition(uint8_t*, recomp_context*);
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
void func_80800380_chhoneycarrier(uint8_t*, recomp_context*);
void tooie_shadow_mode_begin(uint8_t*, recomp_context*);
void tooie_shadow_mode_end(uint8_t*, recomp_context*);
void tooie_shadow_complete(uint8_t*, recomp_context*);
void tooie_shadow_reset(uint8_t*, recomp_context*);
void tooie_shadow_de498(uint8_t*, recomp_context*);
void tooie_shadow_rectangle_args(uint8_t*, recomp_context*);
void tooie_shadow_mv_end(uint8_t*, recomp_context*);
#endif
#endif
#endif
#ifdef TOOIE_CAMERA_PRODUCTION_FIXTURE
void func_800E44FC(uint8_t*, recomp_context*);
void func_800E42F0(uint8_t*, recomp_context*);
void func_800E4640(uint8_t*, recomp_context*);
void func_800E4628(uint8_t*, recomp_context*);
#endif
#ifdef TOOIE_CKCOPY_FIXTURE
void tooie_ckcopy_root_begin(uint8_t*, recomp_context*);
void tooie_ckcopy_segment(uint8_t*, recomp_context*);
void tooie_ckcopy_root_end(uint8_t*, recomp_context*);
void tooie_ckcopy_release(uint8_t*, recomp_context*);
void tooie_cpu625_begin(uint8_t*, recomp_context*);
void tooie_cpu625_admit(uint8_t*, recomp_context*);
void tooie_cpu625_end(uint8_t*, recomp_context*);
void tooie_cpu625_attach_begin(uint8_t*, recomp_context*);
void tooie_cpu625_attach_end(uint8_t*, recomp_context*);
void tooie_shadow_prepare(uint8_t*, recomp_context*);
// The extracted CPU625 admit's original copy-manager getters: C-linkage doubles defined below.
extern "C" void func_80100A74(uint8_t*, recomp_context*);
extern "C" void func_80100AC4(uint8_t*, recomp_context*);
#endif
#include "candidate_draw_boundaries_fixture.inc"
}
namespace {
constexpr uint32_t actor=0x80111214, rotation=0x80212224, extra=0x80313234;
constexpr uint32_t buffers=0x80414244, returned=0x80515254, model=0x80616264;
constexpr uint32_t descriptor0=0x80717274, descriptor1=0x80718284;
constexpr uint32_t descriptor2=0x80719294, descriptor3=0x8071A2A4;
constexpr uint32_t callback=0x8071B2B4, stack=0x80F00000, guard=512;
static_assert(stack-guard>=0x80000000u && stack+32u<=0x81000000u,
    "fixture stack/canaries fit guest RDRAM");
constexpr uint32_t first_scale=0x41234567, second_scale=0xC2345678;
constexpr uint32_t third_scale=0x43345679, position_mutation=0xA1B2C3D4;
static_assert(TOOIE_DRAW_101808_FRAME_BYTES>0 && TOOIE_DRAW_DE448_FRAME_BYTES>0 &&
    TOOIE_DRAW_BK_NESTED_FRAME_BYTES>=0 &&
    TOOIE_DRAW_101808_FRAME_BYTES+TOOIE_DRAW_DE448_FRAME_BYTES+TOOIE_DRAW_BK_NESTED_FRAME_BYTES<guard,
    "finite candidate frames including selected BK callees");
struct Row { uint32_t scale;int zero;bool zero_callback; };
// zero: 1 rotation, 2 extra, 3 GraphicsBuffers, 4 DE498 result.
Row current{};bool chained;
#ifdef TOOIE_PLAYER_STATE_ADDRESS
// Source-owned body setup; these addresses are fixture objects, not runtime identity.
constexpr uint32_t player_buffers=0x80127658, player_owner=0x80220000;
constexpr uint32_t player_component=0x80221000, player_pose=0x80222000;
constexpr uint32_t player_dispatch=0x80223000, player_model=0x80617000;
constexpr uint32_t player_pool=0x80300000, player_cursor=player_pool+0x100;
constexpr uint32_t state=TOOIE_PLAYER_STATE_ADDRESS;
static_assert(stack+32u<=state || stack-guard>=state+sizeof(TooieRenderBridgeState),
    "fixture stack/canaries are disjoint from bridge globals");
constexpr uint32_t pc=state+offsetof(TooieRenderBridgeState,player);
// Lower admission span of the selected composition (standalone storage keeps 0x8FC0).
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
constexpr uint32_t player_lower=TOOIE_BRIDGE_LOWER_BYTES;
#else
constexpr uint32_t player_lower=0x8FC0u;
#endif
// PC[8]: player prepay ledger and its cap in the selected composition (standalone: shared 6400).
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
constexpr uint32_t player_ledger=offsetof(TooieRenderBridgeState,primary_bytes),player_cap=TOOIE_BRIDGE_PRIMARY_BYTES;
#else
constexpr uint32_t player_ledger=offsetof(TooieRenderBridgeState,extra_bytes),player_cap=6400u;
#endif
int player_case=-1;
uint32_t player_view=0; // func_800A89F8 double result for player cases; 0 keeps every prior case
// Modeled DE498 handler calls for the secondary shared-envelope fixture; empty for every other case.
std::function<void(uint8_t*,uint32_t)> player_draw_body;
static_assert(TOOIE_DRAW_DE448_FRAME_BYTES+TOOIE_PLAYER_BEGIN_FRAME_BYTES<guard &&
    TOOIE_DRAW_DE448_FRAME_BYTES+TOOIE_PLAYER_END_FRAME_BYTES<guard,"player nested frame guard");
#endif
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
constexpr uint32_t palette_actor=0x80280000, palette_model=0x80620000, palette_pose=0x80290000;
constexpr uint32_t palette_vector=0x80270000, palette_controller_pool=0x802A0000;
constexpr uint32_t palette_controller=palette_controller_pool+4+0x40, palette_data=0x802B0100;
constexpr uint32_t palette_cursor_slot=0x802B0000, palette_mtx_pool=0x80400000;
constexpr uint32_t palette_classes=0x80136D88, palette_descriptor=0x80132ED0;
constexpr uint32_t ac=state+offsetof(TooieRenderBridgeState,palette_context);
constexpr uint32_t palette_records=state+offsetof(TooieRenderBridgeState,palette);
bool palette_case=false;
unsigned palette_controller_getters=0;
unsigned palette_pose_getters=0;
uint32_t palette_pose_result=palette_pose;
#ifdef TOOIE_SKELETAL_FIXTURE
uint32_t skeletal_view=0,skeletal_phase=0;
int skeletal_boundary=-1;
unsigned skeletal_boundary_calls=0;
uint32_t skeletal_boundary_actor=0;
unsigned title_private_getters=0;
uint32_t title_private_result=0x802B1000u;
bool title_boundary=false;
uint32_t title_boundary_expected=0;
#endif
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
int honey_case=-1;
unsigned honey_draws=0;
unsigned honey_shows=0;
constexpr uint32_t honey_arg=0x80281000;
void require(bool ok,const char* why);
template<class Call> void expect_required_stop(uint32_t site,Call call) {
    const unsigned before=close_stop::calls;
    bool stopped=false;
    try {call();} catch(const close_stop::RequiredStop& stop) {
        require(stop.site==site,"required stop carries its exact guest site");stopped=true;
    }
    require(stopped && close_stop::calls==before+1 && close_stop::last_site==site,
        "unsafe paid close must synchronously stop exactly once");
}
#endif
static_assert(TOOIE_ACTOR_PALETTE_BEGIN_FRAME_BYTES<guard &&
    TOOIE_ACTOR_PALETTE_END_FRAME_BYTES<guard,"actual palette helper frame guards");
#endif
enum Kind { Bind, Rotation, Extra, Model, Buffers, Draw };
struct Event {
    Kind kind;std::array<uint32_t,9> words{};
    bool operator==(const Event& b)const{return kind==b.kind&&words==b.words;}
};
std::vector<Event> events;
gpr guest(uint32_t a){return S32(a);}
uint32_t low(gpr a){return uint32_t(a);}
uint32_t word(uint8_t* rdram,uint32_t a){return uint32_t(MEM_W(0,guest(a)));}
void put(uint8_t* rdram,uint32_t a,uint32_t v){MEM_W(0,guest(a))=v;}
uint8_t byte(uint8_t* rdram,uint32_t a){return uint8_t(MEM_BU(0,guest(a)));}
void put_byte(uint8_t* rdram,uint32_t a,uint8_t v){MEM_B(0,guest(a))=v;}
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void clobber(recomp_context* c){
    c->r1=0x91929394;c->r2=0xA1A2A3A4;c->r3=0xB1B2B3B4;
    c->r4=0xC1C2C3C4;c->r5=0xD1D2D3D4;c->r6=0xE1E2E3E4;c->r7=0xF1F2F3F4;
    c->r8=0x81828384;c->r9=0x82838485;c->r10=0x83848586;c->r11=0x84858687;
    c->r12=0x85868788;c->r13=0x86878889;c->r14=0x8788898A;c->r15=0x88898A8B;
    c->r24=0x898A8B8C;c->r25=0x8A8B8C8D;c->hi=0x8B8C8D8E;c->lo=0x8C8D8E8F;
    c->f0.u64=100;c->f1.u64=101;c->f2.u64=102;c->f3.u64=103;
    c->f4.u64=104;c->f5.u64=105;c->f6.u64=106;c->f7.u64=107;
    c->f8.u64=108;c->f9.u64=109;c->f10.u64=110;c->f11.u64=111;
    c->f12.u64=112;c->f13.u64=113;c->f14.u64=114;c->f15.u64=115;
    c->f16.u64=116;c->f17.u64=117;c->f18.u64=118;c->f19.u64=119;
}
recomp_context context(){
    recomp_context c{};c.r29=guest(stack);c.r31=guest(0x8071C2C4);
    c.r16=16;c.r17=17;c.r18=18;c.r19=19;c.r20=20;c.r21=21;c.r22=22;c.r23=23;
    c.r28=28;c.r30=30;c.f20.u64=20;c.f21.u64=21;c.f22.u64=22;c.f23.u64=23;
    c.f24.u64=24;c.f25.u64=25;c.f26.u64=26;c.f27.u64=27;c.f28.u64=28;c.f29.u64=29;
    c.f30.u64=30;c.f31.u64=31;return c;
}
void saved(const recomp_context& c,const recomp_context& b){
    require(c.r16==b.r16&&c.r17==b.r17&&c.r18==b.r18&&c.r19==b.r19&&c.r20==b.r20&&
        c.r21==b.r21&&c.r22==b.r22&&c.r23==b.r23&&c.r28==b.r28&&c.r30==b.r30&&
        c.r29==b.r29&&c.r31==b.r31,"callee-saved GPR/SP/RA");
    require(c.f20.u64==20&&c.f21.u64==21&&c.f22.u64==22&&c.f23.u64==23&&c.f24.u64==24&&
        c.f25.u64==25&&c.f26.u64==26&&c.f27.u64==27&&c.f28.u64==28&&c.f29.u64==29&&
        c.f30.u64==30&&c.f31.u64==31,"callee-saved FPR");
}
struct Output { std::vector<uint8_t> memory;std::vector<Event> trace;uint32_t result; };
Output execute(bool use_candidate,bool chain,Row row){
    current=row;chained=chain;events.clear();
    std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
    for(uint32_t a=stack-guard;a<stack+32;++a)put_byte(rdram,a,0xA5);
    put(rdram,actor,descriptor0);put(rdram,actor+0x38,first_scale);
    put(rdram,actor+4,0x91929394);put(rdram,rotation,0xA2A3A4A5);put(rdram,extra,0xB3B4B5B6);
    auto c=context();
    if(chain){c.r4=guest(actor);c.r5=guest(row.zero_callback?0:callback);}
    else {c.r4=guest(actor+4);c.r5=guest(rotation);c.r6=guest(row.scale);c.r7=guest(extra);put(rdram,stack+0x10,model);}
    const auto before=c;
    if(chain){if(use_candidate)candidate::func_80101808(rdram,&c);else original::func_80101808(rdram,&c);}
    else {if(use_candidate)candidate::func_800DE448(rdram,&c);else original::func_800DE448(rdram,&c);}
    saved(c,before);
    const uint32_t actor_frame=use_candidate?TOOIE_DRAW_101808_FRAME_BYTES:56;
    const uint32_t draw_frame=use_candidate?TOOIE_DRAW_DE448_FRAME_BYTES:32;
    const uint32_t bk_depth=use_candidate?TOOIE_DRAW_BK_NESTED_FRAME_BYTES:0;
    const uint32_t depth=(chain?actor_frame+draw_frame:draw_frame)+bk_depth;
    for(uint32_t a=stack-guard;a<stack-depth;++a)require(byte(rdram,a)==0xA5,"below nested compiled frames");
    // o32 incoming argument homes: actor+0..0F; direct model+0..13 (fifth word is caller input).
    const uint32_t allowed=chain?16:20;
    for(uint32_t a=stack+allowed;a<stack+32;++a)require(byte(rdram,a)==0xA5,"above exact argument homes");
    require(low(c.r2)==(row.zero==4?0:returned),"observable full32-bit returned pointer");
    require(events.size()==(chain?6:2),"exact original call count");
    if(chain)require(events[0].kind==Bind&&events[1].kind==Rotation&&events[2].kind==Extra&&
        events[3].kind==Model&&events[4].kind==Buffers&&events[5].kind==Draw,"ordered actor chain");
    else require(events[0].kind==Buffers&&events[1].kind==Draw,"ordered direct model chain");
    // Stack-local values differ across compilers; their bounds were checked above.
    for(uint32_t a=stack-guard;a<stack+32;++a)put_byte(rdram,a,0);
    return {std::move(ram),events,low(c.r2)};
}
void compare(bool chain,Row row){
    auto a=execute(false,chain,row),b=execute(true,chain,row);
    require(a.trace==b.trace,"original/candidate ordered arguments or pointed effects differ");
    require(a.memory==b.memory,"original/candidate guest memory effects differ");
    require(a.result==b.result,"original/candidate logical return differs");
}
#ifdef TOOIE_PLAYER_STATE_ADDRESS
// Actual translated paid-close helpers; prime only their already-admitted
// ownership seam. Original DE498/camera command production is not simulated.
unsigned player_paid_physical_fixtures() {
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
    constexpr uint32_t physical=TOOIE_CAMERA_M_BYTES;
#else
    constexpr uint32_t physical=0x8FC0u;
#endif
    unsigned count=0;
    for(auto fn:{candidate::tooie_player_bone_pop,candidate::tooie_player_end})
    for(uint32_t cursor:{player_pool+0x8FC0u,player_pool+physical-8u,
                         player_pool+physical,player_pool+0x8FC4u}) {
        std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
        put(rdram,state+offsetof(TooieRenderBridgeState,epoch),7);
        put(rdram,state+offsetof(TooieRenderBridgeState,active),1);
        put(rdram,state+offsetof(TooieRenderBridgeState,extra_bytes),3792);
        put(rdram,pc+offsetof(TooiePlayerBoundary,enabled),1);
        put(rdram,pc+offsetof(TooiePlayerBoundary,buffers),player_buffers);
        put(rdram,pc+offsetof(TooiePlayerBoundary,base),player_pool);
        put(rdram,pc+offsetof(TooiePlayerBoundary,stamp),7);
        put(rdram,pc+offsetof(TooiePlayerBoundary,begin),player_cursor);
        put(rdram,0x8007BCD0,player_pool);MEM_H(0,guest(0x8007BEDA))=0;
        put(rdram,player_buffers,cursor);
        const uint32_t aligned=cursor&~7u;
        for(uint32_t at=aligned-8;at<aligned+16;at+=4)put(rdram,at,0x13579BDF);
        auto c=context();c.r4=guest(player_buffers);c.r5=1;const auto before=c;
        fn(rdram,&c);saved(c,before);
        const bool paid=!(cursor&7u) && cursor<=player_pool+physical-8u;
        require(word(rdram,player_buffers)==cursor+(paid?8u:0u),"paid player close uses actual physical list");
        require(word(rdram,pc+offsetof(TooiePlayerBoundary,tripwires))==(paid?0u:1u),
            "physical-bound refusal alone controls player tripwire");
        require(word(rdram,state+offsetof(TooieRenderBridgeState,extra_bytes))==3792,
            "paid close does not grant additional player budget");
        for(uint32_t at=aligned-8;at<aligned+16;at+=4) {
            const uint32_t expected=paid&&at==cursor?0x6400000Du:paid&&at==cursor+4?1u:0x13579BDFu;
            require(word(rdram,at)==expected,"only exact prepaid POP words change");
        }
        if(fn==candidate::tooie_player_end)require(word(rdram,pc+offsetof(TooiePlayerBoundary,end))==
            (paid?cursor+8u:0u),"outer paid end publishes only a successful postcursor");
        ++count;
    }
    return count;
}

void player_fixture(int which,uint32_t dragon_asset=0,uint32_t dragon_count=0){
    player_case=which;events.clear();
    std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
    for(uint32_t a=stack-guard;a<stack+32;++a)put_byte(rdram,a,0xA5);
    put(rdram,state,TOOIE_BRIDGE_MAGIC);put(rdram,state+4,TOOIE_BRIDGE_LAYOUT);
    put(rdram,state+8,1);put(rdram,pc,1);
    put(rdram,pc+offsetof(TooiePlayerBoundary,emitted_bones),77);
    put(rdram,0x8012C9A8,0x80092234);put(rdram,0x8012C9AC,player_owner);
    put(rdram,player_owner+0x50,player_component);put(rdram,player_owner+0x184,0);
    put(rdram,0x80135490,player_owner);put(rdram,0x801354DC,0);
    MEM_H(0,guest(player_component))=1;MEM_H(0,guest(player_component+0xC))=0x62A;
    if(which>=8&&which<=14){MEM_H(0,guest(player_component+0xC))=0x607;put(rdram,player_component+0x10C,which==9?0x1234062A:0x62A);}
    put(rdram,player_pose,68);put(rdram,player_pose+4,68);
    if(which>=12&&which<=14){
        put(rdram,player_component+0x10C,0x62B);
        put(rdram,player_pose,which==13?68u:66u);
        put(rdram,player_pose+4,which==14?65u:68u);
    }
    if(dragon_asset){
        MEM_H(0xC,guest(player_component))=0x607;
        put(rdram,player_component+0x10C,dragon_asset);
        put(rdram,player_pose,which==21?dragon_count+1:dragon_count);
        put(rdram,player_pose+4,which==22?dragon_count:68u); // Exact and larger allocations.
    }
    put(rdram,0x8012C968,player_pose);put(rdram,0x8012CF8C,player_dispatch);
    put(rdram,0x80123198,player_dispatch);
    put(rdram,player_dispatch+2*4,0x800DCF48);
    put(rdram,player_dispatch+10*4,0x800DD410);
    put(rdram,player_dispatch+12*4,0x800DD56C);
    put(rdram,player_dispatch+17*4,0x800DD65C);
    put(rdram,player_dispatch+24*4,0x800DD998);
    put(rdram,0x8007BCD0,player_pool);MEM_H(0,guest(0x8007BEDA))=0;
    put(rdram,player_buffers,player_cursor);
    if(which==2)put(rdram,0x8007BCE8,1); // redirected owner refuses before a push
    if(which==3)put(rdram,0x8012C9B0,0x80224000); // arbitrary post-draw callback
    if(which==4)put_byte(rdram,0x801354B0,1); // queued deletion
    if(which==6||which==7){
        put(rdram,0x8012C838,1);put_byte(rdram,0x8012CA11,which==6?1:2);
    }
    if(which==11){put(rdram,state+12,1);put(rdram,state+player_ledger,player_cap-3792u+1u);}
    // Original root-mode word C9B8 (C948[28]): 23 mode1, 24 mode2, 25 mode3,
    // 26 mode1 with an arbitrary post-draw callback.
    if(which>=23&&which<=26)put(rdram,0x8012C9B8,which==24?2u:which==25?3u:1u);
    if(which==26)put(rdram,0x8012C9B0,0x80224000);
    // 27: synthetic direct selector outside the finite pose table. A supported asset is
    // then refused by 28: header bit40, 29: header deform word, 30: handler table.
    if(which==27)MEM_H(0xC,guest(player_component))=0x7FFF;
    if(which==28)MEM_H(0xA,guest(player_model))=0x40;
    if(which==29)put(rdram,player_model+0x28,3024);
    if(which==30)put(rdram,player_dispatch+24*4,0);
    if(which==0 && !dragon_asset) {
        // Reuse the complete existing admission setup, then restore it so the
        // existing draw/lifecycle checks retain their original counters/state.
        const auto original_ram=ram;
        const uint32_t near_end=player_pool+player_lower-3784u;
        put(rdram,state+offsetof(TooieRenderBridgeState,silo_depth),1);
        put(rdram,player_buffers,near_end);
        auto probe=context();probe.r4=guest(player_buffers);probe.r5=guest(player_model);
        candidate::tooie_player_begin(rdram,&probe);
        require(low(probe.r2)==0 && word(rdram,pc+offsetof(TooiePlayerBoundary,last_reason))==TOOIE_BRIDGE_CAPACITY,
            "physical expansion does not relax3792-byte admission room");
        require(word(rdram,player_buffers)==near_end && word(rdram,near_end)==0 &&
            word(rdram,state+offsetof(TooieRenderBridgeState,extra_bytes))==0 &&
            word(rdram,state+offsetof(TooieRenderBridgeState,next_block))==0,
            "admission refusal writes no label, consumes no budget or identity");
        ram=original_ram;rdram=ram.data();events.clear();
    }
    auto c=context();c.r4=guest(actor+4);c.r5=guest(rotation);c.r6=0;c.r7=guest(extra);
    put(rdram,stack+0x10,player_model);const auto before=c;
    candidate::func_800DE448(rdram,&c);saved(c,before);
    require(low(c.r2)==returned,"player preserves original return");
    require(events.size()==2&&events[0].kind==Buffers&&events[1].kind==Draw,"player preserves original call chain");
    const bool refused=(which>=2&&which<=4)||which==7||which==9||which==11||which==13||which==14||(dragon_asset&&which==21)||
        (which>=24&&which<=30);
    require(word(rdram,state+player_ledger)==(which==11?player_cap-3791u:refused?0u:3792u),
        dragon_asset?"Dragon exact current pose admission/refusal and reservation":"complete player reservation debited exactly once");
    require(word(rdram,state+28)==0,"player releases active ownership");
    require(word(rdram,pc+15*4)==(refused?0u:1u),"admission count");
    if(refused){
        require(word(rdram,pc+offsetof(TooiePlayerBoundary,emitted_bones))==77,
            "refusal does not reset an unadmitted occurrence counter");
        if(which==3||which==26)require(word(rdram,pc+17*4)==(4u|(2u<<8)),
            "OVERRIDE retains reason4 and exact post-draw predicate bit");
        if(which==24||which==25)require(word(rdram,pc+17*4)==(4u|(4u<<8)),
            "root modes2/3 retain the exact root-mode predicate bit");
        if(which==7)require(word(rdram,pc+17*4)==(4u|(64u<<8)),
            "OVERRIDE retains exact unsupported CPU type predicate bit");
        if(which==9)require(word(rdram,pc+17*4)==(TOOIE_BRIDGE_MODEL_DATA|0x100u|0x200u|(0x062Au<<16)),
            "full-width unsupported LOD asset refuses without low16 alias; diagnostic flags its width");
        if(which==27)require(word(rdram,pc+17*4)==(TOOIE_BRIDGE_MODEL_DATA|0x100u|(0x7FFFu<<16)),
            "finite-table miss records its asset above the MODEL_DATA low byte");
        if(which>=28&&which<=30)require(word(rdram,pc+17*4)==(TOOIE_BRIDGE_MODEL_DATA|(0x062Au<<16)|
                (which==28?0x800u:which==29?0x1000u:0x2000u)),
            "header bit40, deform and handler-table refusals each name their predicate and asset");
        if(which==13||which==14||(dragon_asset&&which==21))require(word(rdram,pc+17*4)==TOOIE_BRIDGE_ANIMATION,
            "asset exact current count and sufficient capacity required before custom writes");
        if(which==11)require(word(rdram,pc+17*4)==TOOIE_BRIDGE_CAPACITY,
            "3792 reservation refuses over shared model allowance");
        require(word(rdram,player_cursor)==0xDF123456&&word(rdram,player_cursor+4)==0x789ABCDE,
            "refusal leaves original command at original cursor");
        require(word(rdram,player_buffers)==player_cursor+8&&word(rdram,player_cursor+8)==0,
            "refusal emits no envelope");
        require(word(rdram,state+36)==0&&word(rdram,pc+16*4)==1,"refusal consumes no identity");
    }else{
        require(word(rdram,player_cursor)==0xE0525464&&word(rdram,player_cursor+4)==0x10000064,
            "public extended enable");
        require(word(rdram,player_cursor+8)==0x6400000C&&word(rdram,player_cursor+12)==0x90000081&&
            word(rdram,player_cursor+16)==1&&word(rdram,player_cursor+20)==0,
            "public outer129 modelview push with all components SKIP");
        require(word(rdram,pc+offsetof(TooiePlayerBoundary,emitted_bones))==0,
            "body admission resets dedicated bone counter");
        require(word(rdram,state+36)==1,"one identity block");
        if(which==5){
            require(word(rdram,state+24)==1&&word(rdram,pc+21*4)==1,
                "invalid postcursor poisons and trips run");
            require(word(rdram,player_buffers)==player_cursor+25&&word(rdram,player_cursor+24)==0&&
                word(rdram,player_cursor+28)==0,"invalid cursor receives no unsafe pop write");
        }else{
            uint32_t pop=player_cursor+24;
            if(which==0||which==6||which>=8){
                require(word(rdram,pop)==0xDF123456&&word(rdram,pop+4)==0x789ABCDE,
                    "original body command retained inside group");pop+=8;
            }
            require(word(rdram,pop)==0x6400000D&&word(rdram,pop+4)==1,
                "prepaid pop at actual normal or empty-return cursor");
            require(word(rdram,player_buffers)==pop+8&&word(rdram,pc+13*4)==pop+8,
                "postcursor and occurrence end agree");
        }
    }
    const uint32_t deepest=TOOIE_DRAW_DE448_FRAME_BYTES+
        (TOOIE_PLAYER_BEGIN_FRAME_BYTES>TOOIE_PLAYER_END_FRAME_BYTES?TOOIE_PLAYER_BEGIN_FRAME_BYTES:TOOIE_PLAYER_END_FRAME_BYTES);
    for(uint32_t a=stack-guard;a<stack-deepest;++a)require(byte(rdram,a)==0xA5,"below player nested frames");
    for(uint32_t a=stack+20;a<stack+32;++a)require(byte(rdram,a)==0xA5,"above player incoming homes");
    if(which==8||which==10){
        constexpr uint32_t asset_offset=offsetof(TooiePlayerBoundary,effective_model_asset);
        const uint32_t first_id=word(rdram,pc+8*4);
        require(word(rdram,pc+asset_offset)==0x62A,"supported full-width effective asset stored");
        require(word(rdram,pc+4*4)==0x06070001,"packed selector and pose remain unchanged");
        auto next=[&](uint32_t epoch,uint32_t asset,uint32_t actual_model){
            put(rdram,state+8,epoch);put(rdram,player_component+0x10C,asset);
            put(rdram,player_buffers,player_cursor);events.clear();
            auto next_context=context();next_context.r4=guest(actor+4);next_context.r5=guest(rotation);
            next_context.r6=0;next_context.r7=guest(extra);put(rdram,stack+0x10,actual_model);
            candidate::func_800DE448(rdram,&next_context);
            require(word(rdram,pc+7*4)==epoch,"consecutive source epoch recorded");
            require(word(rdram,state+player_ledger)==3792&&word(rdram,state+28)==0,"each occurrence prepays3792 and pops");
            require(word(rdram,pc+asset_offset)==asset,"accepted asset follows original selection");
            require(word(rdram,pc+3*4)==actual_model,"actual model pointer preserved");
            require(word(rdram,pc+4*4)==0x06070001,"asset key does not replace packed selector");
        };
        next(2,0x62A,which==10?player_model+0x100:player_model);
        require(word(rdram,pc+8*4)==(which==8?first_id:first_id+512),
            "stable asset keeps ID; changed model pointer reissues without frame gap");
        require(word(rdram,pc+18*4)==(which==8?0u:1u)&&word(rdram,pc+19*4)==(which==8?0u:1u),
            "actual key changes and reissues counted");
    }
    if(which==12){
        const uint32_t first_id=word(rdram,pc+32);
        require(word(rdram,pc+88)==0x62B&&word(rdram,player_pose)==66&&word(rdram,player_pose+4)==68,
            "62B accepts current66 capacity68 without modifying pose");
        for(uint32_t asset:{0x62Au,0x62Bu}){
            const uint32_t old_id=word(rdram,pc+32);
            put(rdram,state+8,word(rdram,state+8)+1);put(rdram,player_component+0x10C,asset);
            put(rdram,player_pose,asset==0x62A?68u:66u);put(rdram,player_buffers,player_cursor);
            auto next=context();next.r4=guest(actor+4);next.r5=guest(rotation);next.r6=0;next.r7=guest(extra);
            put(rdram,stack+0x10,player_model);candidate::func_800DE448(rdram,&next);
            require(word(rdram,pc+32)==old_id+512&&word(rdram,pc+88)==asset&&word(rdram,pc+12)==player_model,
                "observed asset switch reissues with unchanged model address");
            require(word(rdram,state+player_ledger)==3792&&word(rdram,state+28)==0,"LOD switch retains full common prepay and unwind");
        }
        require(word(rdram,pc+32)==first_id+1024,"two observed asset switches consume separate blocks");
    }
    if(dragon_asset&&which!=21){
        require(word(rdram,pc+88)==dragon_asset&&word(rdram,player_pose)==dragon_count&&
            word(rdram,player_pose+4)==(which==22?dragon_count:68u),
            "Dragon preserves exact current60/58 with exact or larger capacity");
        if(dragon_asset==0x838u&&which==20)for(uint32_t asset:{0x62Au,0x838u,0x839u}){
            const uint32_t old_id=word(rdram,pc+32);
            put(rdram,state+8,word(rdram,state+8)+1);put(rdram,player_component+0x10C,asset);
            put(rdram,player_pose,asset==0x62Au?68u:asset==0x838u?60u:58u);
            put(rdram,player_buffers,player_cursor);
            auto next=context();next.r4=guest(actor+4);next.r5=guest(rotation);next.r6=0;next.r7=guest(extra);
            put(rdram,stack+0x10,player_model);candidate::func_800DE448(rdram,&next);
            require(word(rdram,pc+32)==old_id+512&&word(rdram,pc+88)==asset&&word(rdram,pc+12)==player_model,
                "standard/Dragon and Dragon LOD switches renew at the same model address");
            require(word(rdram,state+player_ledger)==3792&&word(rdram,state+28)==0,
                "Dragon transitions preserve common prepay and paid unwind");
        }
    }
    if(which==23)require(word(rdram,0x8012C9B8)==1u,"mode1 admission leaves the original root-mode word");
    if(which>=15&&which<20&&!dragon_asset){
        const uint32_t first_id=word(rdram,pc+32);
        auto invoke=[&](recomp_func_t* fn,uint32_t a0,uint32_t a1=0u){
            auto call=context();call.r4=guest(a0);call.r5=guest(a1);fn(rdram,&call);
            require(call.r29==guest(stack),"lifecycle/admission stack balanced");return low(call.r2);
        };
        auto next_epoch=[&](){put(rdram,state+8,word(rdram,state+8)+1);
            put(rdram,state+offsetof(TooieRenderBridgeState,silo_depth),1);put(rdram,player_buffers,player_cursor);};
        auto init=[&](){invoke(candidate::func_80092898,player_owner);
            require(word(rdram,pc+32)==0&&word(rdram,pc+28)==0,"completed same-address init retires retained ID");
            MEM_H(0xC,guest(player_component))=0x62A;};
        if(which==15) init();
        if(which==16){
            invoke(candidate::func_800F3880,player_owner);
            require(word(rdram,pc+32)==0&&word(rdram,pc+28)==UINT32_MAX,"free establishes retained tombstone");
            // Invalid retained address must report tombstone before range/owner loads.
            put(rdram,pc+4,0x81234560);put(rdram,0x8012C9AC,0x81234560);next_epoch();
            require(invoke(candidate::tooie_player_begin,player_buffers,player_model)==0&&
                word(rdram,pc+68)==TOOIE_BRIDGE_BIRTH,"tombstone refusal precedes owner validation/dereference");
            put(rdram,pc+4,player_owner);put(rdram,0x8012C9AC,player_owner);init();
        }
        if(which==17){
            next_epoch();put_byte(rdram,0x801354B0,1);
            require(invoke(candidate::tooie_player_begin,player_buffers,player_model)==0&&
                word(rdram,pc+32)==0&&word(rdram,pc+28)!=UINT32_MAX,"pending invalidates ID without teardown tombstone");
            put_byte(rdram,0x801354B0,0); // Original queue cancellation, no initialization.
        }
        next_epoch();require(invoke(candidate::tooie_player_begin,player_buffers,player_model)==1,"post-lifecycle admission");
        if(which<18){
            require(word(rdram,pc+32)==first_id+512&&word(rdram,state+36)==2,"same tuple/address gets fresh block after lifecycle or cancellation");
            invoke(candidate::tooie_player_end,player_buffers);
        }else{
            const uint32_t token=invoke(candidate::tooie_player_bone_push,player_buffers,2);
            require(token==1,"paid bone scope admitted");
            const uint32_t paid_cursor=word(rdram,player_buffers),id=word(rdram,pc+32),stamp=word(rdram,pc+28);
            invoke(which==18?candidate::func_800F3880:candidate::func_80092898,player_owner);
            require(word(rdram,pc+84)!=0&&word(rdram,state+24)==1&&word(rdram,pc+32)==id&&
                word(rdram,pc+28)==stamp&&word(rdram,player_buffers)==paid_cursor,"active lifecycle trips persistently before paid identity/cursor mutation");
            invoke(candidate::tooie_player_bone_pop,player_buffers,token);invoke(candidate::tooie_player_end,player_buffers);
            require(word(rdram,player_buffers)==paid_cursor+16&&word(rdram,paid_cursor)==0x6400000D&&
                word(rdram,paid_cursor+8)==0x6400000D&&word(rdram,state+28)==0&&word(rdram,pc+96)==UINT32_MAX,
                "both prepaid pops survive active lifecycle poison and restore role");
            next_epoch();require(invoke(candidate::tooie_player_begin,player_buffers,player_model)==0&&
                word(rdram,state+24)==0&&word(rdram,pc+84)!=0&&word(rdram,pc+68)==TOOIE_BRIDGE_POISONED,
                "persistent tripwire refuses after normal epoch poison reset");
        }
    }
    player_case=-1;
}
// WP3 census rows through the actual translated begin and emitters. The model holds only
// the header words the boundary reads (row census values); segment-3 pairs are the original
// 80122DC8 table. The five controls keep 3792/78/59 in player_fixture above.
struct PlayerCensusRow { uint32_t asset,poses,bones,leaves,bytes,graph,lists,vertices,bone_section,flags,scratch; };
constexpr PlayerCensusRow player_census_rows[]={
    {0x609,48,48,47,73936,70320,26656,48056,47280,0,0},{0x60B,12,12,9,25504,24584,4120,10720,10520,0,0},
    {0x60D,48,48,47,106392,102760,58416,79312,78536,0,0},{0x61D,14,14,9,21232,20312,2032,7864,7632,0,0},
    {0x626,13,13,7,29976,28944,7496,15616,15400,0,0},
    // Header-0x10 rows: exact flags and the largest single F070 scratch request (PQ3c).
    {0x60C,25,25,12,51072,49536,17304,29448,29040,0x10,13},{0x60E,29,29,21,21312,19576,4864,11688,11216,0x10,73},
    {0x624,15,15,6,43648,42440,11832,22952,22704,0x10,12}};
unsigned player_census_fixtures() {
    // 0 exact physical fit, full bounded emission; 1 physical room 8 short; 2 ledger 1 short;
    // 3 tuple near-miss; 4 header pose word; 5 CA28=4; 6 replaced mode-1 pair; 7 changed
    // appearance row; 8 authenticated appearance row; 9 bone bound+1; 10 leaf bound+1;
    // 11 header flags toggled by 0x10; 12 ordinary mode-0 pair; 13 scalar scratch capacity N-1;
    // 14 null vector scratch; 15 wrong scalar element size. 13..15 refuse only scratch rows.
    const uint32_t pairs[4][2]={{0x8011B9B0,0x8011E530},{0x8011C830,0x8011F3B0},
                                {0x8011D6B0,0x80120230},{0x801210B0,0x80121F30}};
    unsigned cases=0;
    for(const auto& row:player_census_rows)for(int which=0;which<16;++which){
        std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
        const uint32_t prepay=32+24*row.bones+32*row.leaves;
        const uint32_t start=which==0?player_pool+player_lower-prepay:which==1?player_pool+player_lower-prepay+8u:player_cursor;
        player_case=100+which;
        put(rdram,state,TOOIE_BRIDGE_MAGIC);put(rdram,state+4,TOOIE_BRIDGE_LAYOUT);put(rdram,state+8,1);put(rdram,pc,1);
        put(rdram,state+offsetof(TooieRenderBridgeState,silo_depth),1);
        if(which==2){put(rdram,state+offsetof(TooieRenderBridgeState,seen_epoch),1);put(rdram,state+player_ledger,player_cap-prepay+1);}
        put(rdram,0x8012C9A8,0x80092234);put(rdram,0x8012C9AC,player_owner);
        put(rdram,player_owner+0x50,player_component);put(rdram,player_owner+0x184,0);
        put(rdram,0x80135490,player_owner);put(rdram,0x801354DC,0);
        MEM_H(0,guest(player_component))=1;MEM_H(0xC,guest(player_component))=int16_t(row.asset);
        put(rdram,player_pose,row.poses);put(rdram,player_pose+4,row.poses);put(rdram,0x8012C968,player_pose);
        put(rdram,0x8012CF8C,player_dispatch);put(rdram,0x80123198,player_dispatch);
        put(rdram,player_dispatch+2*4,0x800DCF48);put(rdram,player_dispatch+10*4,0x800DD410);
        put(rdram,player_dispatch+12*4,0x800DD56C);put(rdram,player_dispatch+17*4,0x800DD65C);
        put(rdram,player_dispatch+24*4,0x800DD998);
        put(rdram,player_model+4,row.graph);put(rdram,player_model+0xC,row.lists+(which==3?8u:0u));
        put(rdram,player_model+0x10,row.vertices);put(rdram,player_model+0x18,row.bone_section);
        MEM_H(0xA,guest(player_model))=int16_t(which==11?row.flags^0x10u:row.flags);
        MEM_H(row.bone_section+4,guest(player_model))=int16_t(row.poses-(which==4?1u:0u));
        for(uint32_t m=0;m<4;++m){put(rdram,0x80122DC8+8*m,pairs[m][0]);put(rdram,0x80122DCC+8*m,pairs[m][1]);}
        if(which==6)put(rdram,0x80122DD4,0x80224000); // mode-1 second base replaced
        put(rdram,0x8012CA28,which==5?4u:which==12?0u:1u); // regular 92444->923A8 stages mode 1
        put(rdram,0x8012303C,0x8010E2C4);put(rdram,0x80123040,which==7?0x8010E4C8u:0x8010E4C4u);
        put(rdram,0x80123044,0x8010E7F4);
        if(which==7||which==8){put(rdram,0x8012C838,1);put_byte(rdram,0x8012CA11,1);}
        put(rdram,0x8007BCD0,player_pool);MEM_H(0,guest(0x8007BEDA))=0;put(rdram,player_buffers,start);
        // 80100480 descriptors 1/2: element sizes 4/12, valid pointers, signed capacity exactly N.
        const int16_t capacity=int16_t(row.scratch?row.scratch:1);
        MEM_H(0,guest(0x801243AC))=int16_t(which==15?8:4);MEM_H(2,guest(0x801243AC))=int16_t(capacity-(which==13?1:0));
        put(rdram,0x801243B4,0x80230800);
        MEM_H(0,guest(0x801243B8))=12;MEM_H(2,guest(0x801243B8))=capacity;
        put(rdram,0x801243C0,which==14?0u:0x80231000u);
        auto invoke=[&](recomp_func_t* fn,uint32_t a1){
            auto c=context();c.r4=guest(player_buffers);c.r5=guest(a1);const auto before=c;
            fn(rdram,&c);saved(c,before);return low(c.r2);
        };
        const bool admitted=which==0||(which>=8&&which<=10)||which==12||(which>=13&&!row.scratch);
        require(invoke(candidate::tooie_player_begin,player_model)==(admitted?1u:0u),"census row admission");
        require(word(rdram,state+player_ledger)==
            (admitted?prepay:which==2?player_cap-prepay+1:0u),"census row prepays its own executed bounds once");
        if(!admitted){
            const uint32_t reason=which<=2?TOOIE_BRIDGE_CAPACITY:which>=5&&which<=7?(4u|(1u<<(which+10))):
                which>=13?(4u|(1u<<18)):(TOOIE_BRIDGE_MODEL_DATA|0x4000u|(row.asset<<16));
            require(word(rdram,pc+offsetof(TooiePlayerBoundary,last_reason))==reason &&
                word(rdram,player_buffers)==start && word(rdram,start)==0,
                "census refusal names its predicate and writes no label");
            ++cases;continue;
        }
        require(word(rdram,start)==0xE0525464 && word(rdram,start+8)==0x6400000C && word(rdram,start+16)==1,
            "census row opens the public outer 129 group");
        const uint32_t bone_limit=row.bones+(which==9?1u:0u),leaf_limit=row.leaves+(which==10?1u:0u);
        uint32_t bones=0,leaves=0;bool tripped=false;
        for(uint32_t i=0;i<bone_limit&&!tripped;++i){
            const uint32_t token=invoke(candidate::tooie_player_bone_push,i%row.poses);
            if(!token){tripped=true;break;}
            ++bones;
            if(i==0)for(uint32_t l=0;l<leaf_limit;++l){
                if(!invoke(candidate::tooie_player_leaf_begin,0)){tripped=true;break;}
                invoke(candidate::tooie_player_leaf_end,0);++leaves;
            }
            invoke(candidate::tooie_player_bone_pop,token);
        }
        invoke(candidate::tooie_player_end,0);
        const bool trip=which==9||which==10;
        require(tripped==trip && bones==(which==10?1u:row.bones) && leaves==row.leaves &&
            word(rdram,pc+offsetof(TooiePlayerBoundary,tripwires))==(trip?1u:0u) &&
            word(rdram,state+offsetof(TooieRenderBridgeState,poisoned))==(trip?1u:0u),
            "census emitters stop fail-closed exactly beyond the row's executed bounds");
        if(!trip)require(word(rdram,player_buffers)==start+prepay && word(rdram,start+prepay-8)==0x6400000D,
            "full bounded emission consumes exactly the row prepay and closes");
        ++cases;
    }
    player_case=-1;return cases;
}
// R21-3 under PC[8]: the actual DE448 and tooie_player_begin with two valid roster owners. Control
// no longer gates admission; each owner keeps its own (slot, view) entry and ID across control
// switches. DE498 is the existing double.
unsigned player_noncontrolled_fixtures() {
    constexpr uint32_t second_owner=0x80228000u,second_component=0x80229000u;
    unsigned cases=0;player_case=0;events.clear();
    std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
    put(rdram,state,TOOIE_BRIDGE_MAGIC);put(rdram,state+4,TOOIE_BRIDGE_LAYOUT);put(rdram,pc,1);
    put(rdram,0x8012C9A8,0x80092234);
    for(uint32_t slot=0;slot<2;++slot){
        const uint32_t owner=slot?second_owner:player_owner,component=slot?second_component:player_component;
        put(rdram,owner+0x50,component);put(rdram,owner+0x184,slot);put(rdram,0x80135490+slot*4,owner);
        MEM_H(0,guest(component))=1;MEM_H(0,guest(component+0xC))=0x62A;
    }
    put(rdram,player_pose,68);put(rdram,player_pose+4,68);put(rdram,0x8012C968,player_pose);
    put(rdram,0x8012CF8C,player_dispatch);put(rdram,0x80123198,player_dispatch);
    put(rdram,player_dispatch+2*4,0x800DCF48);put(rdram,player_dispatch+10*4,0x800DD410);
    put(rdram,player_dispatch+12*4,0x800DD56C);put(rdram,player_dispatch+17*4,0x800DD65C);
    put(rdram,player_dispatch+24*4,0x800DD998);
    put(rdram,0x8007BCD0,player_pool);MEM_H(0,guest(0x8007BEDA))=0;
    uint32_t cursor=player_cursor;
    const auto draw=[&](uint32_t epoch,uint32_t control,uint32_t owner){
        put(rdram,state+8,epoch);put(rdram,0x801354DC,control);put(rdram,0x8012C9AC,owner);
        put(rdram,player_buffers,cursor);events.clear();
        auto c=context();c.r4=guest(actor+4);c.r5=guest(rotation);c.r6=0;c.r7=guest(extra);
        put(rdram,stack+0x10,player_model);const auto before=c;
        candidate::func_800DE448(rdram,&c);saved(c,before);
        require(low(c.r2)==returned && events.size()==2 && events[0].kind==Buffers && events[1].kind==Draw,
            "both owners keep the original return and call chain");
        const uint32_t start=cursor;cursor=word(rdram,player_buffers);return start;
    };
    const auto field=[&](size_t offset){return word(rdram,pc+uint32_t(offset));};
    const auto bridge=[&](size_t offset){return word(rdram,state+uint32_t(offset));};
    // Two control-class bodies fit only the protected PRIMARY partition (player_cap); the
    // standalone shared 6400 refuses the second for CAPACITY.
    const bool both=player_cap>=2u*3792u;
    const uint32_t entries=offsetof(TooieRenderBridgeState,primary);
    // Q1: epoch1, control slot0. The valid slot1 owner admits under its own entry [1][0].
    uint32_t at=draw(1,0,second_owner);
    const uint32_t first=field(offsetof(TooiePlayerBoundary,idbase));
    require(field(offsetof(TooiePlayerBoundary,calls))==1 && field(offsetof(TooiePlayerBoundary,admissions))==1 &&
        field(offsetof(TooiePlayerBoundary,owner))==second_owner && field(offsetof(TooiePlayerBoundary,slot))==1 &&
        word(rdram,at+8)==0x6400000Cu && word(rdram,at+12)==first+129u && bridge(player_ledger)==3792 &&
        bridge(entries+(1u*4u+0u)*32u+24u)==first && !bridge(offsetof(TooieRenderBridgeState,active)),
        "a noncontrolled valid owner admits under its own slot/view entry");
    ++cases;
    // Q2: the controlled owner in the same epoch: a distinct entry and block when both fit.
    at=draw(1,0,player_owner);
    const uint32_t second=bridge(entries+24u);
    if(both)require(field(offsetof(TooiePlayerBoundary,admissions))==2 && second && second!=first &&
        word(rdram,at+12)==second+129u && bridge(player_ledger)==2u*3792u &&
        bridge(offsetof(TooieRenderBridgeState,next_block))==2,"both bodies admit in one epoch with distinct IDs");
    else require(field(offsetof(TooiePlayerBoundary,last_reason))==TOOIE_BRIDGE_CAPACITY && !second &&
        word(rdram,at)==0xDF123456u,"the standalone shared ledger refuses the second control-class body");
    ++cases;
    // Q3: a control switch changes no entry: both owners keep their IDs in the next epoch.
    at=draw(2,1,second_owner);
    require(word(rdram,at+12)==first+129u && !field(offsetof(TooiePlayerBoundary,changes)) &&
        !field(offsetof(TooiePlayerBoundary,reissues)),"the slot1 owner keeps its ID across a control switch");
    at=draw(2,1,player_owner);
    if(both)require(word(rdram,at+12)==second+129u,"the slot0 owner keeps its ID across the switch");
    ++cases;
    // Q4: switching back again consumes no block.
    at=draw(3,0,second_owner);
    require(word(rdram,at+12)==first+129u && bridge(offsetof(TooieRenderBridgeState,next_block))==(both?2u:1u) &&
        !field(offsetof(TooiePlayerBoundary,changes)),"no re-identification on control switches");
    ++cases;
    player_case=-1;return cases;
}
// PC[8] full view: the actual DE448 and tooie_player_begin over all 8 roster slots x 4 view
// indices of one epoch. Each (slot, view) entry takes its own block and ID, the protected
// PRIMARY ledger reaches exactly 32*3792 and the OTHER ledger is untouched. A later epoch in
// another order renews no entry; a repeat at the full ledger is DUPLICATE, not CAPACITY; a full
// OTHER ledger does not refuse a primary. DE498 is the existing double. Partitioned storage only:
// the standalone shared 6400 ledger has no full-view quota and runs no case here.
unsigned player_fullview_fixtures() {
    unsigned cases=0;
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
    static_assert(TOOIE_BRIDGE_PRIMARY_BYTES==32u*3792u && TOOIE_BRIDGE_BLOCK_COUNT>=32u,
        "full-view cases need the M1 primary partition and 32 ID blocks");
    constexpr uint32_t owners=0x80228000u,components=0x8022A000u; // 8 x 0x400 owners, 8 x 0x200 bodies
    player_case=0;player_view=0;events.clear();
    std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
    put(rdram,state,TOOIE_BRIDGE_MAGIC);put(rdram,state+4,TOOIE_BRIDGE_LAYOUT);put(rdram,pc,1);
    put(rdram,0x8012C9A8,0x80092234);put(rdram,0x801354DC,0);
    for(uint32_t slot=0;slot<8;++slot){
        const uint32_t owner=owners+slot*0x400u,component=components+slot*0x200u;
        put(rdram,owner+0x50,component);put(rdram,owner+0x184,slot);put(rdram,0x80135490+slot*4,owner);
        MEM_H(0,guest(component))=1;MEM_H(0,guest(component+0xC))=0x62A;
    }
    put(rdram,player_pose,68);put(rdram,player_pose+4,68);put(rdram,0x8012C968,player_pose);
    put(rdram,0x8012CF8C,player_dispatch);put(rdram,0x80123198,player_dispatch);
    put(rdram,player_dispatch+2*4,0x800DCF48);put(rdram,player_dispatch+10*4,0x800DD410);
    put(rdram,player_dispatch+12*4,0x800DD56C);put(rdram,player_dispatch+17*4,0x800DD65C);
    put(rdram,player_dispatch+24*4,0x800DD998);
    put(rdram,0x8007BCD0,player_pool);MEM_H(0,guest(0x8007BEDA))=0;
    uint32_t cursor=player_cursor;
    const auto draw=[&](uint32_t epoch,uint32_t slot,uint32_t view){
        put(rdram,state+8,epoch);player_view=view;put(rdram,0x8012C9AC,owners+slot*0x400u);
        put(rdram,player_buffers,cursor);events.clear();
        auto c=context();c.r4=guest(actor+4);c.r5=guest(rotation);c.r6=0;c.r7=guest(extra);
        put(rdram,stack+0x10,player_model);const auto before=c;
        candidate::func_800DE448(rdram,&c);saved(c,before);
        require(low(c.r2)==returned && events.size()==2 && events[0].kind==Buffers && events[1].kind==Draw,
            "every entry keeps the original return and call chain");
        const uint32_t start=cursor;cursor=word(rdram,player_buffers);return start;
    };
    const auto field=[&](size_t offset){return word(rdram,pc+uint32_t(offset));};
    const auto bridge=[&](size_t offset){return word(rdram,state+uint32_t(offset));};
    const auto entry=[&](uint32_t slot,uint32_t view,uint32_t offset){
        return bridge(offsetof(TooieRenderBridgeState,primary)+(slot*4u+view)*32u+offset);};
    constexpr uint32_t ledger_at=offsetof(TooieRenderBridgeState,primary_bytes);
    constexpr uint32_t other_at=offsetof(TooieRenderBridgeState,extra_bytes);
    // V1: epoch1, slot-major: 32 admissions under 32 entries and blocks. The global scope snapshots
    // each admitted entry and closes; PRIMARY alone pays and OTHER stays 0.
    uint32_t first=0;
    for(uint32_t i=0;i<32u;++i){
        const uint32_t slot=i>>2,view=i&3u,at=draw(1,slot,view),id=entry(slot,view,24);
        if(!i)first=id;
        require(id==first+512u*i && word(rdram,at+8)==0x6400000Cu && word(rdram,at+12)==id+129u &&
            entry(slot,view,0)==owners+slot*0x400u && (entry(slot,view,20)&3u)==view && entry(slot,view,28)==1,
            "each (slot, view) entry takes its own block and ID");
        require(field(offsetof(TooiePlayerBoundary,owner))==owners+slot*0x400u &&
            field(offsetof(TooiePlayerBoundary,slot))==slot && field(offsetof(TooiePlayerBoundary,idbase))==id &&
            !bridge(offsetof(TooieRenderBridgeState,primary_paid)) && !bridge(offsetof(TooieRenderBridgeState,active)) &&
            bridge(ledger_at)==3792u*(i+1u) && !bridge(other_at),
            "the global scope snapshots the admitted entry and only the PRIMARY ledger pays");
    }
    require(field(offsetof(TooiePlayerBoundary,admissions))==32 && bridge(offsetof(TooieRenderBridgeState,next_block))==32 &&
        bridge(ledger_at)==TOOIE_BRIDGE_PRIMARY_BYTES &&
        bridge(offsetof(TooieRenderBridgeState,primary_highwater))==TOOIE_BRIDGE_PRIMARY_BYTES,
        "32 occurrences fill the protected partition exactly");
    ++cases;
    // V2: epoch2 in view-major reverse order: every entry keeps its ID and nothing is reissued.
    for(uint32_t k=0;k<32u;++k){
        const uint32_t view=3u-(k>>3),slot=7u-(k&7u),at=draw(2,slot,view);
        require(word(rdram,at+12)==first+512u*(slot*4u+view)+129u && entry(slot,view,28)==2,
            "a different visit order keeps every entry's ID");
    }
    require(!field(offsetof(TooiePlayerBoundary,reissues)) && !field(offsetof(TooiePlayerBoundary,changes)) &&
        !field(offsetof(TooiePlayerBoundary,tripwires)) && bridge(offsetof(TooieRenderBridgeState,next_block))==32 &&
        bridge(ledger_at)==TOOIE_BRIDGE_PRIMARY_BYTES && !bridge(other_at),
        "stable entries renew no block and refill PRIMARY exactly");
    ++cases;
    // V3: at the full ledger a repeat of entry (5,2) is DUPLICATE before any capacity check. Its
    // original draw continues and the global tuple keeps the last admitted entry (0,0).
    {
        const uint32_t refusals=field(offsetof(TooiePlayerBoundary,refusals)),at=draw(2,5,2);
        require(field(offsetof(TooiePlayerBoundary,last_reason))==TOOIE_BRIDGE_DUPLICATE &&
            field(offsetof(TooiePlayerBoundary,refusals))==refusals+1u && field(offsetof(TooiePlayerBoundary,admissions))==64 &&
            word(rdram,at)==0xDF123456u && word(rdram,at+4)==0x789ABCDEu && cursor==at+8 && !word(rdram,at+8) &&
            bridge(ledger_at)==TOOIE_BRIDGE_PRIMARY_BYTES && entry(5,2,24)==first+512u*22u &&
            field(offsetof(TooiePlayerBoundary,owner))==owners && field(offsetof(TooiePlayerBoundary,idbase))==first,
            "a same-entry repeat at the full partition is DUPLICATE and writes no tuple");
    }
    ++cases;
    // V4: epoch3 begins with a fresh PRIMARY ledger and a full OTHER ledger: a primary still
    // admits, paying PRIMARY only.
    put(rdram,state+offsetof(TooieRenderBridgeState,seen_epoch),3);
    put(rdram,state+ledger_at,0);put(rdram,state+other_at,6400);
    {
        const uint32_t at=draw(3,0,0);
        require(word(rdram,at+12)==first+129u && bridge(ledger_at)==3792u && bridge(other_at)==6400u,
            "a full OTHER ledger neither refuses nor is charged by a primary");
    }
    ++cases;
    // V5: one byte short in PRIMARY refuses the next fresh entry CAPACITY; its original draw continues.
    put(rdram,state+ledger_at,TOOIE_BRIDGE_PRIMARY_BYTES-3792u+1u);
    {
        const uint32_t at=draw(3,0,1);
        require(field(offsetof(TooiePlayerBoundary,last_reason))==TOOIE_BRIDGE_CAPACITY &&
            word(rdram,at)==0xDF123456u && !word(rdram,at+8) && entry(0,1,28)==2 &&
            bridge(ledger_at)==TOOIE_BRIDGE_PRIMARY_BYTES-3792u+1u && bridge(other_at)==6400u,
            "the PRIMARY partition bound, not OTHER, refuses a primary");
    }
    ++cases;
    // V6: a secondary-only retained owner retired (its tombstone lives only in the secondary table)
    // while the global snapshot belongs to another owner: a primary attempt for that address
    // refuses BIRTH before the owner is dereferenced, and its original draw continues.
    {
        const uint32_t e6=state+offsetof(TooieRenderBridgeState,secondary)+((6u*4u+0u)*2u+0u)*48u;
        put(rdram,e6+offsetof(TooieSecondaryEntry,owner),owners+6u*0x400u);
        put(rdram,e6+offsetof(TooieSecondaryEntry,stamp),0xFFFFFFFFu);
        const uint32_t at=draw(3,6,0);
        require(field(offsetof(TooiePlayerBoundary,last_reason))==TOOIE_BRIDGE_BIRTH &&
            field(offsetof(TooiePlayerBoundary,owner))==owners && word(rdram,at)==0xDF123456u && !word(rdram,at+8),
            "a secondary tombstone guards the primary admission too");
        put(rdram,e6+offsetof(TooieSecondaryEntry,stamp),0);
    }
    ++cases;
    // V7: the primary admission observing a pending slot forces freshness of that slot's secondary
    // IDs as well as its primary ones; the foreign global snapshot keeps its ID.
    {
        const uint32_t e2=state+offsetof(TooieRenderBridgeState,secondary)+((2u*4u+1u)*2u+1u)*48u;
        put(rdram,e2+offsetof(TooieSecondaryEntry,idbase),0x90004000u);
        put_byte(rdram,0x801354B0u+2u,1);
        const uint32_t at=draw(3,2,1);
        require(field(offsetof(TooiePlayerBoundary,last_reason))==TOOIE_BRIDGE_BIRTH && word(rdram,at)==0xDF123456u &&
            !word(rdram,e2+offsetof(TooieSecondaryEntry,idbase)) && !entry(2,0,24) && !entry(2,1,24) &&
            !entry(2,2,24) && !entry(2,3,24) && field(offsetof(TooiePlayerBoundary,idbase))==first,
            "pending clears both families of the slot; the foreign snapshot keeps its ID");
        put_byte(rdram,0x801354B0u+2u,0);
    }
    ++cases;
    player_view=0;player_case=-1;
#endif
    return cases;
}
#ifdef TOOIE_PLAYER_SECONDARY_FIXTURE
// player_secondary.c admits only with the selected boolean over partitioned storage.
#if TOOIE_PLAYER_SECONDARY_ADMIT && defined(TOOIE_ACTOR_PALETTE_FIXTURE)
#define TOOIE_SECONDARY_FIXTURE_ADMIT 1
#else
#define TOOIE_SECONDARY_FIXTURE_ADMIT 0
#endif
// The measured admission frame runs nested in DE448 and stays inside the fixture stack guard.
static_assert(TOOIE_DRAW_DE448_FRAME_BYTES+TOOIE_PLAYER_SECONDARY_BEGIN_FRAME_BYTES<guard,"secondary nested frame guard");
// PC[8] secondary shared envelope. Exercised as compiled: the DE448 wrapper and its 92224 gateway,
// the five secondary helpers (route token, authentication, admission), the shared player bone/leaf
// emitters and tooie_player_end. Modeled, not executed: the original DE498 body. Its double writes
// one original command, then player_draw_body makes the emitter calls DCF48/DD998 would make, with
// explicit original-reload stand-ins. Helper evidence only; no runtime, RT64 or visual claim.
[[maybe_unused]] unsigned player_secondary_envelope_fixtures() {
    constexpr uint32_t sowner=0x80228000u,scomponent=0x8022A000u,other=0x8022C000u,smodel=0x80630000u;
    constexpr uint32_t freelist=0x80232000u,mtxpool=0x80400000u,parent=0x807F0000u,inner=parent-0x68u;
    constexpr uint32_t slot=3,manager=5,epoch=1,view=1;
    constexpr uint32_t sr=state+offsetof(TooieRenderBridgeState,secondary_route);
    constexpr uint32_t sl=state+offsetof(TooieRenderBridgeState,secondary_ledger);
    constexpr uint32_t secondaries=state+offsetof(TooieRenderBridgeState,secondary);
    constexpr uint32_t primaries=state+offsetof(TooieRenderBridgeState,primary); // 32-byte entries: owner +0, idbase +24, stamp +28
    struct Row { uint32_t resource,graph,lists,vertices,attributes,poses; };
    constexpr Row row83A{0x83A,53864,4160,23256,22496,47},row61C{0x61C,62200,4920,26232,25296,58};
    unsigned cases=0;
    std::vector<uint8_t> ram;uint8_t* rdram=nullptr;
    const auto field=[&](size_t o){return word(rdram,pc+uint32_t(o));};
    const auto ledger=[&](size_t o){return word(rdram,sl+uint32_t(o));};
    const auto bridge=[&](size_t o){return word(rdram,state+uint32_t(o));};
    const auto entry=[&](uint32_t v,uint32_t route){return secondaries+((slot*4u+v)*2u+route-1u)*48u;};
    const auto call=[&](recomp_func_t* fn,uint32_t a0,uint32_t a1=0,uint32_t a2=0,uint32_t sp=stack){
        auto c=context();c.r29=guest(sp);c.r4=guest(a0);c.r5=guest(a1);c.r6=guest(a2);fn(rdram,&c);return low(c.r2);};
    const auto emit=[&](recomp_func_t* fn,uint32_t sp,uint32_t a1=0){return call(fn,player_buffers,a1,0,sp-0x100u);};
    // 083A/061C structural rows, owner/component/body fields (signed16 +90/+9C, word +94, u8 +A0),
    // the filled zero-copy manager record, staged pose/export/table and the 92224 staging.
    const auto setup=[&](const Row& row){
        ram.assign(16*1024*1024,0);rdram=ram.data();player_case=0;player_view=view;events.clear();
        put(rdram,state,TOOIE_BRIDGE_MAGIC);put(rdram,state+4,TOOIE_BRIDGE_LAYOUT);put(rdram,state+8,epoch);put(rdram,pc,1);
        put(rdram,sowner+0x50,scomponent);put(rdram,sowner+0x184,slot);put(rdram,0x80135490+slot*4,sowner);
        put(rdram,other+0x50,scomponent);put(rdram,other+0x184,slot+1);put(rdram,0x80135490+(slot+1)*4,other);
        MEM_H(0x90,guest(scomponent))=1;put(rdram,scomponent+0x94,mtxpool+64);
        MEM_H(0x9C,guest(scomponent))=int16_t(row.resource);put_byte(rdram,scomponent+0xA0,manager);
        put(rdram,0x80135A60,freelist);MEM_H(0,guest(freelist))=12;MEM_H(2,guest(freelist))=16;
        put_byte(rdram,freelist+4+12*manager+9,1);
        put(rdram,smodel+4,row.graph);put(rdram,smodel+12,row.lists);put(rdram,smodel+16,row.vertices);
        put(rdram,smodel+0x18,row.attributes);MEM_H(4,guest(smodel+row.attributes))=int16_t(row.poses);
        put(rdram,player_pose,row.poses);put(rdram,player_pose+4,row.poses);put(rdram,0x8012C968,player_pose);
        put(rdram,0x8012CF8C,player_dispatch);put(rdram,0x80123198,player_dispatch);
        put(rdram,player_dispatch+2*4,0x800DCF48);put(rdram,player_dispatch+10*4,0x800DD410);
        put(rdram,player_dispatch+12*4,0x800DD56C);put(rdram,player_dispatch+17*4,0x800DD65C);
        put(rdram,player_dispatch+24*4,0x800DD998);
        MEM_H(0,guest(0x8007BEDA))=0;put(rdram,0x8007BCD0,player_pool);put(rdram,0x8007BCD8,mtxpool);
        put(rdram,player_buffers,player_cursor);put(rdram,player_buffers+4,mtxpool+64+64*row.poses);
        put(rdram,0x8012C948+14*4,mtxpool+64);put(rdram,0x8012C948+24*4,0x80092224);put(rdram,0x8012C948+25*4,sowner);
    };
    // The actual compiled DE448 wrapper; DE498 stays the double (one original command + player_draw_body).
    const auto de448=[&](uint32_t model){
        events.clear();const uint32_t start=word(rdram,player_buffers);
        auto c=context();c.r4=guest(actor+4);c.r5=guest(rotation);c.r6=0;c.r7=guest(extra);
        put(rdram,stack+0x10,model);const auto before=c;
        candidate::func_800DE448(rdram,&c);saved(c,before);
        require(low(c.r2)==returned && events.size()==2 && events[0].kind==Buffers && events[1].kind==Draw,
            "the secondary gateway keeps the original return and call chain");
        return start;
    };
    // The original order of one occurrence: parent begin, 93A9C arm, DE448, 93AA4 disarm, parent end.
    const auto occurrence=[&](uint32_t route,bool arm=true){
        call(candidate::tooie_secondary_route_begin,route,sowner,parent);
        if(arm)call(candidate::tooie_secondary_route_arm,sowner,inner,1);
        const uint32_t start=de448(smodel);
        call(candidate::tooie_secondary_route_disarm,sowner,inner);
        call(candidate::tooie_secondary_route_end,route,sowner,parent);
        return start;
    };
    // One DCF48 bone, optionally with a single-list DD998 hybrid, as the handlers call the emitters.
    const auto bone=[&](uint32_t sp,uint32_t index,bool hybrid,uint32_t id){
        uint32_t at=word(rdram,player_buffers);
        const uint32_t token=emit(candidate::tooie_player_bone_push,sp,index);
        require(token==1 && word(rdram,at)==0x6400000Cu && word(rdram,at+4)==id+index+1u &&
            word(rdram,at+8)==0x02010AA9u && !word(rdram,at+12),"shared bone group in the admitted ID page");
        if(hybrid){
            at=word(rdram,player_buffers);
            require(emit(candidate::tooie_player_leaf_begin,sp)==1 && word(rdram,at)==0x6400000Cu &&
                word(rdram,at+4)==id+256u+index+1u && word(rdram,at+16)==0xDA380003u &&
                word(rdram,at+20)==word(rdram,player_buffers+4),"shared hybrid group and LOAD NOPUSH");
            const uint32_t tail=word(rdram,player_buffers);
            put(rdram,tail,0xDA380003u);put(rdram,tail+4,word(rdram,player_buffers+4));put(rdram,player_buffers,tail+8);
            emit(candidate::tooie_player_leaf_end,sp);
            require(word(rdram,tail+8)==0x6400000Du && word(rdram,tail+12)==1,"hybrid pop after the original reload");
        }
        at=word(rdram,player_buffers);emit(candidate::tooie_player_bone_pop,sp,token);
        require(word(rdram,at)==0x6400000Du && word(rdram,at+4)==1,"bone pop balances");
    };
    // An unadmitted occurrence: the original command stands alone and the shared emitters stay inert.
    const auto unlabelled=[&](uint32_t start,const char* why){
        require(word(rdram,start)==0xDF123456u && word(rdram,start+4)==0x789ABCDEu &&
            word(rdram,player_buffers)==start+8 && !word(rdram,start+8) && !field(offsetof(TooiePlayerBoundary,tripwires)),why);
    };
    const auto inert=[&](uint8_t*,uint32_t sp){
        const uint32_t at=word(rdram,player_buffers);
        require(!emit(candidate::tooie_player_bone_push,sp,0) && word(rdram,player_buffers)==at,"emitters inert without a paid scope");
    };
#if TOOIE_SECONDARY_FIXTURE_ADMIT
    const uint32_t md=TOOIE_BRIDGE_MODEL_DATA;
    // E1: 083A main occurrence through the real gateway: root envelope, original command, one bone
    // with a hybrid in the secondary ID page, the shared close and the released secondary scope.
    {   setup(row83A);uint32_t id=0;
        player_draw_body=[&](uint8_t*,uint32_t sp){id=word(rdram,entry(view,1)+offsetof(TooieSecondaryEntry,idbase));bone(sp,4,true,id);};
        const uint32_t start=occurrence(1);player_draw_body=nullptr;
        const uint32_t end=word(rdram,player_buffers);
        require(id==TOOIE_BRIDGE_ID_BASE && word(rdram,start)==0xE0525464u && word(rdram,start+4)==0x10000064u &&
            word(rdram,start+8)==0x6400000Cu && word(rdram,start+12)==id+129u && word(rdram,start+16)==1 &&
            !word(rdram,start+20) && word(rdram,start+24)==0xDF123456u && word(rdram,start+28)==0x789ABCDEu,
            "secondary root envelope, then the original command");
        require(word(rdram,end-8)==0x6400000Du && word(rdram,end-4)==1 && field(offsetof(TooiePlayerBoundary,end))==end &&
            !ledger(offsetof(TooieSecondaryLedger,paid)) && !bridge(offsetof(TooieRenderBridgeState,active)) &&
            ledger(offsetof(TooieSecondaryLedger,bytes))==2824 && ledger(offsetof(TooieSecondaryLedger,admissions))==1 &&
            ledger(offsetof(TooieSecondaryLedger,seen_main))==1 && !bridge(offsetof(TooieRenderBridgeState,primary_bytes)) &&
            !field(offsetof(TooiePlayerBoundary,calls)) && !field(offsetof(TooiePlayerBoundary,tripwires)) &&
            !word(rdram,sr+offsetof(TooieSecondaryRoute,state)),"the shared close pays the root pop and releases the scope");
        ++cases;
    }
    // E2/E3/E4: while the secondary scope is paid the emitters are bounded by 083A's row only:
    // 59 bones, poses 0..46, 43 list calls. Each excess trips the unchanged global fatal word.
    for(int which=0;which<3;++which) {
        setup(row83A);
        player_draw_body=[&](uint8_t*,uint32_t sp){
            const uint32_t id=word(rdram,entry(view,1)+offsetof(TooieSecondaryEntry,idbase));
            if(which==0){for(uint32_t i=0;i<59u;++i)bone(sp,i%47u,false,id);
                require(!emit(candidate::tooie_player_bone_push,sp,0),"the 60th bone exceeds 083A's 59");}
            if(which==1)require(!emit(candidate::tooie_player_bone_push,sp,47),"bone index 47 is beyond 083A's 47 poses");
            if(which==2){const uint32_t token=emit(candidate::tooie_player_bone_push,sp,0);require(token==1,"bone 0");
                for(uint32_t i=0;i<43u;++i){require(emit(candidate::tooie_player_leaf_begin,sp)==1,"43 list calls admit");
                    const uint32_t tail=word(rdram,player_buffers);put(rdram,tail,0xDA380003u);
                    put(rdram,tail+4,word(rdram,player_buffers+4));put(rdram,player_buffers,tail+8);
                    emit(candidate::tooie_player_leaf_end,sp);}
                require(!emit(candidate::tooie_player_leaf_begin,sp),"the 44th list call exceeds 083A's 43");
                emit(candidate::tooie_player_bone_pop,sp,token);}
        };
        occurrence(1);player_draw_body=nullptr;
        require(field(offsetof(TooiePlayerBoundary,tripwires))==1 && bridge(offsetof(TooieRenderBridgeState,poisoned))==1 &&
            !ledger(offsetof(TooieSecondaryLedger,paid)),"the secondary row bound reaches the global fatal word");
        ++cases;
    }
    // E5/E6/E7: main and late are separate histories; a consecutive epoch keeps both IDs; a repeat of
    // an entry in its epoch is DUPLICATE, leaving the original command alone and the emitters inert.
    {   setup(row83A);occurrence(1);occurrence(2);
        require(word(rdram,entry(view,1)+offsetof(TooieSecondaryEntry,idbase))==TOOIE_BRIDGE_ID_BASE &&
            word(rdram,entry(view,2)+offsetof(TooieSecondaryEntry,idbase))==TOOIE_BRIDGE_ID_BASE+512u &&
            ledger(offsetof(TooieSecondaryLedger,bytes))==2*2824,"main and late are separate entries and charges");
        ++cases;
        put(rdram,state+8,epoch+1);occurrence(1);occurrence(2);
        require(word(rdram,entry(view,1)+offsetof(TooieSecondaryEntry,idbase))==TOOIE_BRIDGE_ID_BASE &&
            word(rdram,entry(view,2)+offsetof(TooieSecondaryEntry,idbase))==TOOIE_BRIDGE_ID_BASE+512u &&
            bridge(offsetof(TooieRenderBridgeState,next_block))==2 && ledger(offsetof(TooieSecondaryLedger,bytes))==2*2824,
            "a consecutive epoch renews nothing");
        ++cases;
        player_draw_body=inert;const uint32_t start=occurrence(1);player_draw_body=nullptr;
        unlabelled(start,"a repeated entry occurrence is DUPLICATE and unlabelled");
        require(ledger(offsetof(TooieSecondaryLedger,last_reason))==TOOIE_BRIDGE_DUPLICATE &&
            ledger(offsetof(TooieSecondaryLedger,bytes))==2*2824,"DUPLICATE charges nothing");
        ++cases;
    }
    // E8/E9: an unarmed occurrence (any DE448 before 93A9C) and a foreign staged owner after the arm
    // are unauthenticated: no admission work, no label, no charge.
    for(int which=0;which<2;++which) {
        setup(row83A);player_draw_body=inert;uint32_t start=0;
        if(which==0)start=occurrence(1,false);
        else{call(candidate::tooie_secondary_route_begin,1,sowner,parent);call(candidate::tooie_secondary_route_arm,sowner,inner,1);
            put(rdram,0x8012C948+25*4,other);start=de448(smodel);put(rdram,0x8012C948+25*4,sowner);
            call(candidate::tooie_secondary_route_disarm,sowner,inner);call(candidate::tooie_secondary_route_end,1,sowner,parent);}
        player_draw_body=nullptr;unlabelled(start,"an unauthenticated occurrence stays unlabelled");
        require(ledger(offsetof(TooieSecondaryLedger,unauthenticated))==1 && !ledger(offsetof(TooieSecondaryLedger,seen_main)) &&
            !ledger(offsetof(TooieSecondaryLedger,bytes)) && !bridge(offsetof(TooieRenderBridgeState,next_block)),
            "only the armed actual occurrence authenticates");
        ++cases;
    }
    // E10..E15: retained tombstones of either family, pending freshness of both families, the
    // protected SECONDARY quota, the segment7-gated 061C row and an unqualified tuple all refuse
    // through the real gateway with the original command alone.
    for(int which=0;which<6;++which) {
        setup(which==4?row61C:row83A);uint32_t reason=TOOIE_BRIDGE_BIRTH;
        const uint32_t p32=primaries+(slot*4u+2u)*32u;
        if(which==0){put(rdram,p32,sowner);put(rdram,p32+28,0xFFFFFFFFu);}
        if(which==1){put(rdram,entry(3,2)+offsetof(TooieSecondaryEntry,owner),sowner);
            put(rdram,entry(3,2)+offsetof(TooieSecondaryEntry,stamp),0xFFFFFFFFu);}
        if(which==2){put(rdram,p32+24,0x90000400u);
            put(rdram,entry(3,2)+offsetof(TooieSecondaryEntry,idbase),0x90000600u);put_byte(rdram,0x801354B0+slot,1);}
        if(which==3){put(rdram,state+offsetof(TooieRenderBridgeState,seen_epoch),epoch);
            put(rdram,sl+offsetof(TooieSecondaryLedger,bytes),TOOIE_BRIDGE_SECONDARY_BYTES-2824u+1u);reason=TOOIE_BRIDGE_CAPACITY;}
        if(which==4)reason=md|0x200u|(0x61Cu<<16);
        if(which==5){put(rdram,smodel+4,0);reason=md|0x100u;}
        player_draw_body=inert;const uint32_t start=occurrence(1);player_draw_body=nullptr;
        unlabelled(start,"a refused secondary occurrence stays unlabelled");
        require(ledger(offsetof(TooieSecondaryLedger,last_reason))==reason && !ledger(offsetof(TooieSecondaryLedger,paid)) &&
            !bridge(offsetof(TooieRenderBridgeState,next_block)),"exact refusal with no identity or charge");
        if(which==2)require(!word(rdram,p32+24) && !word(rdram,entry(3,2)+offsetof(TooieSecondaryEntry,idbase)),
            "pending clears both families of the slot");
        if(which==3)require(ledger(offsetof(TooieSecondaryLedger,bytes))==TOOIE_BRIDGE_SECONDARY_BYTES-2824u+1u,
            "the protected quota is unchanged by the refusal");
        ++cases;
    }
    // E16: after a secondary scope closes, a 62A primary through the same gateway uses the primary
    // rows again (60 bones within its 78), charges PRIMARY only, and leaves SECONDARY alone.
    {   setup(row83A);occurrence(1);
        put(rdram,player_owner+0x50,player_component);put(rdram,player_owner+0x184,0);put(rdram,0x80135490,player_owner);
        MEM_H(0,guest(player_component))=1;MEM_H(0xC,guest(player_component))=0x62A;
        put(rdram,player_pose,68);put(rdram,player_pose+4,68);
        put(rdram,0x8012C948+24*4,0x80092234);put(rdram,0x8012C948+25*4,player_owner);
        player_draw_body=[&](uint8_t*,uint32_t sp){
            const uint32_t id=word(rdram,primaries+(0u*4u+view)*32u+24u);
            for(uint32_t i=0;i<60u;++i)bone(sp,i%68u,false,id);};
        de448(player_model);player_draw_body=nullptr;
        require(!field(offsetof(TooiePlayerBoundary,tripwires)) && bridge(offsetof(TooieRenderBridgeState,primary_bytes))==3792 &&
            ledger(offsetof(TooieSecondaryLedger,bytes))==2824 && !bridge(offsetof(TooieRenderBridgeState,primary_paid)),
            "primary bounds and ledger return after the secondary close");
        ++cases;
    }
#else
    // S-A (admission compiled out): the armed occurrence is authenticated, counted and spent through
    // the real gateway with no label; an unarmed one is unauthenticated.
    (void)row61C;(void)primaries;(void)bridge;(void)entry;(void)bone;
    for(int which=0;which<2;++which) {
        setup(row83A);player_draw_body=inert;const uint32_t start=occurrence(1,which==0);player_draw_body=nullptr;
        unlabelled(start,"S-A never labels a secondary occurrence");
        require(ledger(offsetof(TooieSecondaryLedger,seen_main))==(which==0?1u:0u) &&
            ledger(offsetof(TooieSecondaryLedger,unauthenticated))==(which==0?0u:1u) &&
            !ledger(offsetof(TooieSecondaryLedger,admissions)) && !word(rdram,sr+offsetof(TooieSecondaryRoute,state)),
            "S-A route provenance only");
        ++cases;
    }
#endif
    player_case=-1;player_view=0;return cases;
}
#endif
#endif
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
// Real generated camera begin/end bodies; bridge epoch is the only bridge write.
void camera_epoch_fixtures() {
    constexpr uint32_t cs=TOOIE_CAMERA_STATE_VRAM;
    constexpr uint32_t cc=cs+offsetof(TooieCameraState,control);
    constexpr uint32_t cd=cs+offsetof(TooieCameraState,diagnostics);
    static_assert(state==TOOIE_BRIDGE_STATE_VRAM,"camera and player share bridge state");
    for(unsigned which=0;which<8;++which){
        std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
        for(uint32_t i=0;i<sizeof(TooieRenderBridgeState);++i)put_byte(rdram,state+i,uint8_t(i*37u+11u));
        const uint32_t initial=which==0?0u:which==2?UINT32_MAX-1u:which==3?UINT32_MAX:41u;
        const uint32_t next=initial==UINT32_MAX?initial:initial+1u;
        put(rdram,state+8,initial);put(rdram,cc+offsetof(TooieCameraControl,epoch),77);
        put(rdram,cd+offsetof(TooieCameraDiagnostics,refusals),11);
        put(rdram,cd+offsetof(TooieCameraDiagnostics,reentry),13);
        if(which==4)put(rdram,cs+offsetof(TooieCameraState,owner_frame),stack);
        if(which==6){put(rdram,cc+offsetof(TooieCameraControl,backup_depth),1);
            put(rdram,cs+offsetof(TooieCameraState,backup)+offsetof(TooieCameraBackup,valid),1);}
        MEM_H(0,guest(0x8007BEDA))=0;
        put(rdram,0x8007BCD0,TOOIE_CAMERA_MAIN_VRAM);put(rdram,player_buffers,TOOIE_CAMERA_MAIN_VRAM);
        auto invoke=[&](recomp_func_t* fn,uint32_t owner){
            std::array<uint8_t,sizeof(TooieRenderBridgeState)> before{};
            for(uint32_t i=0;i<before.size();++i)before[i]=byte(rdram,state+i);
            auto c=context();c.r4=guest(owner);c.r5=guest(TOOIE_CAMERA_MAIN_VRAM);
            const auto registers=c;fn(rdram,&c);saved(c,registers);
            for(uint32_t i=0;i<before.size();++i)if(i<8||i>=12)
                require(byte(rdram,state+i)==before[i],"camera helper changes no bridge byte outside epoch8..11");
        };
        invoke(candidate::tooie_camera_construction_begin,which==5?0u:stack);
        require(word(rdram,state+8)==next,"camera begin saturating epoch on every path");
        require(word(rdram,cc+offsetof(TooieCameraControl,epoch))==(which==4||which==5?77u:next),
            "camera epoch uses increment except unchanged reentry/no-owner paths");
        require(word(rdram,cd+offsetof(TooieCameraDiagnostics,refusals))==11u+(which==2||which==3||which==5),
            "saturation and no-owner refusal count advances once");
        require(word(rdram,cd+offsetof(TooieCameraDiagnostics,reentry))==13u+(which==4),"reentry count advances once");
        require(word(rdram,cc+offsetof(TooieCameraControl,latch))==unsigned(which==2||which==3||which==4||which==6),
            "saturation, reentry and backup depth latch");
        require(word(rdram,cc+offsetof(TooieCameraControl,active))==unsigned(which==0||which==1||which==7),
            "full main-pool admission activates camera at new epoch");
        if(which==6)require(word(rdram,cs+offsetof(TooieCameraState,backup)+offsetof(TooieCameraBackup,valid))==0,
            "backup depth invalidates backup after epoch increment");
        if(which==7){
            invoke(candidate::tooie_camera_construction_end,stack);
            require(word(rdram,state+8)==next&&word(rdram,cs+offsetof(TooieCameraState,owner_frame))==0,
                "camera end clears owner without increment");
            invoke(candidate::tooie_camera_construction_begin,stack);
            require(word(rdram,state+8)==43&&word(rdram,cc+offsetof(TooieCameraControl,epoch))==43&&
                word(rdram,cc+offsetof(TooieCameraControl,active))==1,"begin end begin advances twice and admits");
        }
    }
}
// Real translated actor helpers and reset setters; the original getter boundaries
// below are explicit doubles. This synthetic immutable asset is not a ROM dump.
struct PaletteFixture {
    std::vector<uint8_t> ram=std::vector<uint8_t>(16*1024*1024);
    uint8_t* rdram=ram.data();
    uint32_t cursor=player_cursor;
    PaletteFixture() {
        palette_case=true; palette_controller_getters=palette_pose_getters=0;
        palette_pose_result=palette_pose;
        put(rdram,state,TOOIE_BRIDGE_MAGIC); put(rdram,state+4,TOOIE_BRIDGE_LAYOUT);
        put(rdram,0x8007BCD0,player_pool); put(rdram,0x8007BCD8,palette_mtx_pool);
        put(rdram,0x80136EE0,palette_vector);
        put(rdram,palette_vector,0x100); put(rdram,palette_vector+4,palette_actor);
        put(rdram,palette_vector+8,palette_actor+0x200); put(rdram,palette_vector+12,palette_actor+0x200);
        put_byte(rdram,0x80135450,0xC0);
        for (uint32_t slot=0;slot<2;++slot) {
            const uint32_t d=palette_descriptor+slot*0x30;
            put(rdram,palette_actor+slot*0x100,d);
            MEM_H(0x14,guest(d))=0x880; MEM_H(0x16,guest(d))=0x1234;
            MEM_H(0x1A,guest(d))=slot<<5;
            put(rdram,state+offsetof(TooieRenderBridgeState,births)+slot*4,slot+1);
        }
        put(rdram,palette_data+0x38,0x80107C2C);
        put(rdram,palette_pose,2); put(rdram,palette_pose+4,2);
        for (uint32_t matrix=0;matrix<3;++matrix)
            for (uint32_t diagonal=0;diagonal<4;++diagonal)
                put(rdram,palette_pose+0x10+matrix*64+diagonal*20,0x3F800000);
        for (uint32_t diagonal=0;diagonal<4;++diagonal) put(rdram,0x8012CE58+diagonal*20,0x3F800000);
        put(rdram,0x80136E70,palette_controller_pool);
        MEM_H(0,guest(palette_controller_pool))=0x40; MEM_H(2,guest(palette_controller_pool))=4;
        class_record(1);
        put(rdram,palette_model+4,0x100); MEM_H(10,guest(palette_model))=0x40;
        put(rdram,palette_model+12,0x200); put(rdram,palette_model+16,0x400);
        for (uint32_t leaf=0;leaf<2;++leaf) {
            const uint32_t node=palette_model+0x100+leaf*32;
            put(rdram,node,12); put(rdram,node+4,leaf?0:32);
            MEM_H(8,guest(node))=1; MEM_H(10,guest(node))=leaf+1;
            put(rdram,node+12,16); put(rdram,node+16,3);
            MEM_H(24,guest(node))=leaf*8;
            const uint32_t dl=palette_model+0x208+leaf*64;
            const std::array<uint32_t,12> commands={0xDA380002,0x05000000+leaf*64,
                0x01001002,0x01000000,0xDE000000,0x03000040,0xDE000000,0x07000000,
                0xD8380002,64,0xDF000000,0};
            for (uint32_t w=0;w<commands.size();++w) put(rdram,dl+w*4,commands[w]);
        }
        put(rdram,player_dispatch+3*4,0x800DD024); put(rdram,player_dispatch+12*4,0x800DD56C);
        put(rdram,0x80123198,player_dispatch);
        put(rdram,0x80122DC8,0x8011B9B0); put(rdram,0x80122DCC,0x8011E530);
        for (uint32_t terminal:{0x8011B9F0u,0x8011E570u,0x80123418u,0x80123428u}) {
            put(rdram,terminal,terminal<0x80123418u?0xE200001C:0xD9FFFFFF);
            put(rdram,terminal+8,0xDF000000);
        }
        prepare(1);
    }
    ~PaletteFixture() { palette_case=false; }
    void class_record(uint32_t index) {
        const uint32_t slot=palette_classes+index*20;
        MEM_H(0,guest(slot))=1; MEM_H(2,guest(slot))=1;
        put_byte(rdram,slot+4,1); put_byte(rdram,slot+5,1);
        // Original config +4 producer mode0, separate +6 playback flags0009.
        MEM_H(0,guest(0x80124740+index*8+4))=0x730;
        MEM_H(0,guest(0x80124740+index*8+6))=9;
    }
    uint32_t record(uint32_t slot=0) const { return palette_records+slot*sizeof(TooieActorPaletteSlot); }
    uint32_t field(size_t offset) const { return word(rdram,state+uint32_t(offset)); }
    void prepare(uint32_t epoch,uint32_t slot=0,uint32_t view=0,uint32_t phase=0,uint32_t leaf=1) {
        put(rdram,state+8,epoch);
        put(rdram,state+offsetof(TooieRenderBridgeState,silo_depth),1);
        for (uint32_t offset=0;offset<sizeof(TooieActorPaletteContext);offset+=4) put(rdram,ac+offset,0);
        const std::array<uint32_t,8> owner={palette_actor+slot*0x100,palette_descriptor+slot*0x30,
            word(rdram,state+offsetof(TooieRenderBridgeState,births)+slot*4),0x08801234,
            view,phase,epoch,player_buffers};
        for (uint32_t i=0;i<owner.size();++i) put(rdram,ac+i*4,owner[i]);
        put(rdram,ac+offsetof(TooieActorPaletteContext,slot),slot);
        put(rdram,0x8012C9A8,0x801017D0); put(rdram,0x8012C9AC,owner[0]);
        put(rdram,0x8012C960,palette_model+0x200); put(rdram,0x8012C964,palette_model+0x400);
        put(rdram,0x8012C968,palette_pose); put(rdram,0x8012CF8C,player_dispatch);
        put(rdram,0x8012CA18,0x1234); put(rdram,0x8012C9A4,255);
        put(rdram,0x8012C850+4,leaf==1); put(rdram,0x8012C850+8,leaf==2);
        put(rdram,palette_cursor_slot,cursor);
        put(rdram,player_buffers,cursor-64); // Original b->gfx is stale at DEFB0.
        put(rdram,player_buffers+4,palette_mtx_pool);
    }
    uint32_t invoke(recomp_func_t* fn,uint32_t a0,uint32_t a1=0,uint32_t a2=0,uint32_t a3=0) {
        for (uint32_t a=stack-guard;a<stack+32;++a) put_byte(rdram,a,0xA5);
        auto c=context(); c.r4=guest(a0); c.r5=guest(a1); c.r6=guest(a2); c.r7=guest(a3);
        const auto before=c; fn(rdram,&c); saved(c,before);
        constexpr uint32_t frame=TOOIE_ACTOR_PALETTE_BEGIN_FRAME_BYTES>TOOIE_ACTOR_PALETTE_END_FRAME_BYTES?
            TOOIE_ACTOR_PALETTE_BEGIN_FRAME_BYTES:TOOIE_ACTOR_PALETTE_END_FRAME_BYTES;
        for (uint32_t a=stack-guard;a<stack-frame;++a)
            require(byte(rdram,a)==0xA5,"palette helper stays within measured maximum frame");
        for (uint32_t a=stack+16;a<stack+32;++a)
            require(byte(rdram,a)==0xA5,"palette helper preserves above o32 argument homes");
        return low(c.r2);
    }
    uint32_t begin(uint32_t camera=0x50000001) {
        return invoke(candidate::tooie_actor_palette_begin,player_buffers,palette_cursor_slot,palette_model,camera);
    }
    void admitted(uint32_t root,uint32_t role) {
        require(root!=0 && word(rdram,palette_cursor_slot)==cursor+24,"actor begin returns root and advances local cursor24");
        require(word(rdram,player_buffers)==cursor-64,"begin does not consume stale b->gfx");
        const std::array<uint32_t,6> expected={0xE0525464,0x10000064,0x6400000C,root+role,0x02010AA9,0};
        for (uint32_t i=0;i<expected.size();++i) require(word(rdram,cursor+i*4)==expected[i],"exact compiled actor enable/MODELVIEW words and ID");
        require(word(rdram,ac+offsetof(TooieActorPaletteContext,admitted))==TOOIE_BRIDGE_PALETTE_ADMITTED,
            "palette occurrence is disjoint from Silo admitted1");
        require(word(rdram,ac+offsetof(TooieActorPaletteContext,gfx_base))==player_pool &&
            word(rdram,ac+offsetof(TooieActorPaletteContext,pool_index))==0,"captured pool ownership");
    }
    // A model of the source-closed original suffix and camera policies, NOT an
    // execution of DE498 or camera_de498_ignore. Only actor begin/end are compiled here.
    void finish(uint32_t root,uint32_t role=1,uint32_t camera=0x50000001) {
        const uint32_t tail=word(rdram,palette_cursor_slot);
        const std::array<uint32_t,16> original={0x6400000C,root,0x0201154E,0,
            0xDA380005,palette_mtx_pool&0x1FFFFFFF,0xDB060014,(palette_mtx_pool+128)&0x1FFFFFFF,
            0xDE000000,(palette_model+0x208+(role-1)*64)&0x1FFFFFFF,
            0x6400000C,camera,0x02008AAA,0,0xDA380007,0x00410000};
        for (uint32_t i=0;i<original.size();++i) put(rdram,tail+i*4,original[i]);
        put(rdram,player_buffers,tail+64);
        for (uint32_t address:{0x8012C960u,0x8012C968u,0x8012C980u,0x8012CA18u,0x8012C854u,0x8012C858u}) put(rdram,address,0);
        invoke(candidate::tooie_actor_palette_end,player_buffers);
        for (uint32_t i=0;i<original.size();++i) require(word(rdram,tail+i*4)==original[i],"end preserves every supplied original/camera suffix word");
        require(word(rdram,tail+64)==0x6400000D && word(rdram,tail+68)==1 &&
            word(rdram,player_buffers)==tail+72,"paid modelview pop follows supplied original camera LOAD");
        require(field(offsetof(TooieRenderBridgeState,active))==0,"palette releases active owner");
        cursor=tail+72;
    }
    void refused(uint32_t reason,uint32_t camera=0x50000001) {
        std::array<uint32_t,24> unchanged{};
        for (uint32_t i=0;i<unchanged.size();++i) unchanged[i]=word(rdram,cursor+i*4);
        const uint32_t blocks=field(offsetof(TooieRenderBridgeState,next_block));
        const uint32_t extra=field(offsetof(TooieRenderBridgeState,seen_epoch))==field(offsetof(TooieRenderBridgeState,epoch))?
            field(offsetof(TooieRenderBridgeState,extra_bytes)):0;
        const uint32_t gfx=word(rdram,player_buffers),mtx=word(rdram,player_buffers+4);
        require(begin(camera)==0,"unsupported palette occurrence refuses");
        require(field(offsetof(TooieRenderBridgeState,silo_last_reason))==reason,"named palette refusal reason");
        require(word(rdram,palette_cursor_slot)==cursor && word(rdram,player_buffers)==gfx &&
            word(rdram,player_buffers+4)==mtx,"refusal leaves original local and buffer cursors intact");
        require(field(offsetof(TooieRenderBridgeState,next_block))==blocks &&
            field(offsetof(TooieRenderBridgeState,extra_bytes))==extra,"refusal consumes no identity or actor bytes");
        for (uint32_t i=0;i<unchanged.size();++i) require(word(rdram,cursor+i*4)==unchanged[i],"refusal preserves complete original fallback command region");
    }
};
#ifdef TOOIE_SKELETAL_FIXTURE
// Synthetic skeletal cases exercise the same extracted guest helpers.
#include TOOIE_SKELETAL_FIXTURE
#include "baseline_feather_cut_cases.inc"
#endif

// Preserve existing generic/prop fixtures. Begin is real; cursor advancement
// stands for already-produced original/camera bytes, not an executed renderer.
unsigned actor_paid_physical_fixtures() {
    unsigned count=0;
    for(uint32_t cursor:{player_pool+0x8FC0u,player_pool+TOOIE_CAMERA_M_BYTES-8u,
                         player_pool+TOOIE_CAMERA_M_BYTES,player_pool+0x8FC4u}) {
        PaletteFixture f;require(f.begin()!=0,"existing generic palette admission owns its paid pop");
        auto* rdram=f.rdram;put(rdram,player_buffers,cursor);
        const uint32_t aligned=cursor&~7u;
        for(uint32_t at=aligned-8;at<aligned+16;at+=4)put(rdram,at,0x2468ACE0);
        const bool paid=!(cursor&7u) && cursor<=player_pool+TOOIE_CAMERA_M_BYTES-8u;
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
        if(!paid)expect_required_stop(2,[&](){f.invoke(candidate::tooie_actor_palette_end,player_buffers);});
        else
#endif
            f.invoke(candidate::tooie_actor_palette_end,player_buffers);
        require(word(rdram,player_buffers)==cursor+(paid?8u:0u),"actor paid end uses camera physical extent");
        require(f.field(offsetof(TooieRenderBridgeState,tripwire_count))==(paid?0u:1u),
            "physical actor refusal retains its existing tripwire behavior");
        require(f.field(offsetof(TooieRenderBridgeState,extra_bytes))==32u,
            "actor paid end retains its exact32-byte precharge");
        for(uint32_t at=aligned-8;at<aligned+16;at+=4) {
            const uint32_t expected=paid&&at==cursor?0x6400000Du:paid&&at==cursor+4?1u:0x2468ACE0u;
            require(word(rdram,at)==expected,"actor close changes only its owned8-byte command");
        }
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
        require(f.field(offsetof(TooieRenderBridgeState,active))==unsigned(!paid),
            "only returning actor close releases occurrence; required stop has no cleanup continuation");
#else
        require(f.field(offsetof(TooieRenderBridgeState,active))==0,"actor end releases occurrence");
#endif
        ++count;
    }
    return count;
}

// Synthetic finite select-prop grammar; no decoded asset bytes or new parser.
// Real translated begin/end execute; original DE498/geometry tail stays MODELED.
void select_prop_model(PaletteFixture& f,uint32_t asset,uint32_t selector,
    uint32_t epoch=1,uint32_t view=0,uint32_t phase=0) {
    auto* rdram=f.rdram;
    f.prepare(epoch,0,view,phase);
    put(rdram,palette_data,0x015804A8);put(rdram,0x8012CA18,asset);
    // Explicit current-draw input, not an execution of DE498's fallback producer.
    put(rdram,0x8012C840,palette_pose);put(rdram,0x8012C824,0);
    const bool compound=asset<=0x80E || asset==0x812;
    const uint32_t count=asset==0x812?4:2,control=asset==0x812?3:asset-0x80B;
    put(rdram,palette_model+0x100,3);put(rdram,palette_model+0x104,compound?16:0);
    MEM_H(8,guest(palette_model+0x100))=0;
    const auto list=[&](uint32_t index,std::initializer_list<uint32_t> words) {
        uint32_t at=palette_model+0x208+index*8;
        for(uint32_t word:words) {put(rdram,at,word);at+=4;}
    };
    if(!compound) list(0,{0xDA380002,0x05000000,0x01001002,0x01000000,
        0xDE000000,0x03000010,0xD8380002,64,0xDF000000,0});
    else {
        list(0,{0xDA380002,0x05000000,0x01001002,0x01000000,
            0xDE000000,0x03000010,0xDF000000,0});
        const uint32_t select=palette_model+0x110,children=(12+count*4+7)&~7u,next=children+count*16;
        put(rdram,select,12);put(rdram,select+4,next);
        MEM_H(8,guest(select))=count;MEM_H(10,guest(select))=control;
        for(uint32_t child=0;child<count;++child) {
            const uint32_t relative=children+child*16,node=select+relative,index=8+child*8;
            put(rdram,select+12+child*4,relative);put(rdram,node,3);put(rdram,node+4,0);
            MEM_H(8,guest(node))=index;
            list(index,{0xDA380002,0x05000040,0x01001002,0x01000000,
                0xD8380002,64,0xDF000000,0});
        }
        const uint32_t tail=select+next,index=8+count*8;
        put(rdram,tail,3);put(rdram,tail+4,0);MEM_H(8,guest(tail))=index;
        list(index,{0xD8380002,64,0xDF000000,0});
        put(rdram,0x8012C850+control*4,selector);
    }
    for(uint32_t at:{0x8011B9C0u,0x8011E540u}) {
        put(rdram,at,0xE200001C);put(rdram,at+4,0);put(rdram,at+8,0xDF000000);put(rdram,at+12,0);
    }
}
unsigned select_prop_fixtures() {
    unsigned groups=0;
    {
        PaletteFixture f;
        auto* rdram=f.rdram;
        constexpr uint32_t moved_pose=palette_pose+0x400,direct_pose=palette_pose+0x800;
        const auto pose=[&](uint32_t address) {
            put(f.rdram,address,3);put(f.rdram,address+4,3);
            for(uint32_t matrix=0;matrix<4;++matrix)
                for(uint32_t diagonal=0;diagonal<4;++diagonal)
                    put(f.rdram,address+0x10+matrix*64+diagonal*20,0x3F800000);
        };
        for(uint32_t address:{palette_pose,moved_pose,direct_pose})pose(address);
        for(uint32_t index=0;index<11;++index)put_byte(f.rdram,palette_classes+index*20+4,0);
        put(f.rdram,0x80136E70,0); // Deliberately invalid, irrelevant to this actor-owned route.
        const auto input=[&](uint32_t epoch,int16_t handle,uint32_t scratch,uint32_t consumed,uint32_t animation) {
            select_prop_model(f,0x80F,1,epoch);
            MEM_H(0x10,guest(palette_descriptor))=handle;
            MEM_H(0x8C,guest(palette_actor))=0;
            put(f.rdram,0x8012C840,scratch);put(f.rdram,0x8012C968,consumed);
            put(f.rdram,0x8012C824,animation);
        };
        const auto key=[&](){return word(f.rdram,f.record()+offsetof(TooieActorPaletteSlot,pose));};
        input(1,0,palette_pose,palette_pose,0); // E1: actual finite current fallback input.
        const uint32_t first=f.begin();f.admitted(first,1);
        require(key()==1 && word(f.rdram,f.record()+offsetof(TooieActorPaletteSlot,controller))==0,
            "E1 default fallback stores input-class key1 and no controller key");
        f.finish(first);
        input(2,0,moved_pose,moved_pose,0); // E2: scratch relocation preserves owner ID.
        const uint32_t moved=f.begin();f.admitted(moved,1);f.finish(moved);
        require(moved==first && key()==1,"E2 scratch relocation preserves the same group ID");
        input(2,0,moved_pose,moved_pose,0); // E2': same-epoch duplicate cannot renew.
        std::array<uint32_t,sizeof(TooieActorPaletteSlot)/4> retained{};
        for(uint32_t i=0;i<retained.size();++i)retained[i]=word(f.rdram,f.record()+i*4);
        f.refused(TOOIE_BRIDGE_DUPLICATE);
        for(uint32_t i=0;i<retained.size();++i)
            require(word(f.rdram,f.record()+i*4)==retained[i],"E2 duplicate preserves every slot word");
        input(3,0,moved_pose,moved_pose,0x802B0200); // Opaque nonzero input; no producer claim.
        const uint32_t animated=f.begin();f.admitted(animated,1);f.finish(animated);
        require(animated==first+512 && key()==2,"E3 input class change renews to key2");
        input(4,1,moved_pose,direct_pose,0);
        palette_pose_result=direct_pose;
        const uint32_t direct=f.begin();f.admitted(direct,1);f.finish(direct);
        require(direct==animated+512 && key()==direct_pose && palette_pose_getters==1,
            "E4 positive actor handle joins actual consumed pose and renews");
        input(5,0,moved_pose,direct_pose,0); // E5: no direct join on a zero handle.
        f.refused(TOOIE_BRIDGE_ANIMATION);
        require(!word(f.rdram,ac+offsetof(TooieActorPaletteContext,open_group)) &&
            !f.field(offsetof(TooieRenderBridgeState,active)),"E5 refusal opens no group");
        for(int16_t handle:{int16_t(-1),int16_t(250)}) {
            input(6,handle,moved_pose,direct_pose,0);palette_pose_getters=0;
            f.refused(TOOIE_BRIDGE_ANIMATION);
            require(palette_pose_getters==0,"E6 invalid signed handles refuse before unchecked getter");
        }
        input(7,1,moved_pose,moved_pose,0);palette_pose_getters=0;
        f.refused(TOOIE_BRIDGE_ANIMATION);
        require(palette_pose_getters==0 && palette_controller_getters==0,
            "direct scratch alias refuses before pose getter; props never resolves a controller");
        ++groups;
    }
    {
        PaletteFixture f;put(f.rdram,0x80136E70,0);
        f.refused(TOOIE_BRIDGE_ANIMATION);
        require(palette_controller_getters==0,"generic palette still refuses invalid controller pool");
        ++groups;
    }
    for(uint32_t asset:{0x80Fu,0x80Cu,0x812u}) {
        PaletteFixture f;
        const uint32_t selector=asset==0x812?uint32_t(-3):1;
        select_prop_model(f,asset,selector);
        const uint32_t root=f.begin();f.admitted(root,1);f.finish(root);
        require(root!=0,"simple, compound and negative-bitmap select-prop roots admit");
        ++groups;
    }
    {
        PaletteFixture f;select_prop_model(f,0x812,uint32_t(-1));
        const uint32_t first=f.begin();f.admitted(first,1);f.finish(first);
        select_prop_model(f,0x812,uint32_t(-3),1,1);
        f.refused(TOOIE_BRIDGE_DUPLICATE); // No renewal may erase view0's seen bit.
        select_prop_model(f,0x812,uint32_t(-1),1,0);
        f.refused(TOOIE_BRIDGE_DUPLICATE);
        select_prop_model(f,0x812,uint32_t(-3),2,0);
        const uint32_t renewed=f.begin();f.admitted(renewed,1);f.finish(renewed);
        require(renewed==first+512,"next epoch selector change obtains one fresh existing block");
        put(f.rdram,state+offsetof(TooieRenderBridgeState,births),2);
        select_prop_model(f,0x812,uint32_t(-3),2,0);
        const uint32_t reborn=f.begin();f.admitted(reborn,1);f.finish(reborn);
        require(reborn==renewed+512,"same-epoch genuine new birth remains eligible");
        ++groups;
    }
    return groups;
}

unsigned palette_fixtures() {
    unsigned groups=0;
    {
        // Original texture segment emission precedes this helper; SOURCE_ONLY.
        PaletteFixture f; put_byte(f.rdram,0x8012C96C,2);
        const uint32_t root=f.begin(); f.admitted(root,1); f.finish(root);
        require(byte(f.rdram,0x8012C96C)==2 &&
            f.field(offsetof(TooieRenderBridgeState,extra_bytes))==32,
            "nonzero original texture handle preserved with exact actor debit");
        ++groups;
    }
    {
        PaletteFixture f;
        const uint32_t first=f.begin(); f.admitted(first,1); f.finish(first);
        require(first==0x90000000 && f.field(offsetof(TooieRenderBridgeState,extra_bytes))==32,"first generic palette block and exact debit32");
        f.prepare(1,0,0,0,2); const uint32_t crossfade=f.begin(); f.admitted(crossfade,2); f.finish(crossfade,2);
        require(crossfade==first && f.field(offsetof(TooieRenderBridgeState,next_block))==1,"separate crossfade leaf shares root without renewal");
        f.prepare(1,0,0,0,1); f.refused(TOOIE_BRIDGE_DUPLICATE);
        f.prepare(2,1); const uint32_t second=f.begin(); f.admitted(second,1); f.finish(second);
        require(second==first+512,"distinct live birth sharing same class pose gets another block");
        f.prepare(2,0,3,6,2); const uint32_t multiview=f.begin(); f.admitted(multiview,2); f.finish(multiview,2);
        require(multiview==first+480 && multiview+2-first==482,"view3 phase6 root and leaf remain fenced inside512");
        ++groups;
    }
    {
        PaletteFixture f; const uint32_t first=f.begin(); f.finish(first);
        // AAC0 -> C1C0 normally writes outer+2C without calling a reset setter.
        // Simulate that source-proved store and pose/root motion, not AAC0 execution.
        put(f.rdram,palette_controller+0x2C,0x3F7AE148);
        put(f.rdram,palette_controller+0x2C,0x3C23D70A);
        put(f.rdram,palette_pose+0x50+48,0x3F000000); put(f.rdram,0x8012CE58+48,0x41200000);
        f.prepare(3); require(f.begin()==first,"loop-store/pose/root motion and absent epoch preserve block"); f.finish(first);
        f.invoke(candidate::func_8008B10C,palette_controller,0x3E800000);
        require(word(f.rdram,palette_controller+0x2C)==0x3E800000 &&
            !(word(f.rdram,f.record()+8)&TOOIE_BRIDGE_BLOCK_VALID),"compiled explicit timer setter preserves payload and retires actual shared controller");
        f.prepare(4); const uint32_t reset=f.begin(); require(reset==first+512,"explicit reset reissues"); f.finish(reset);
        put_byte(f.rdram,palette_controller+0x1B,0xA5);
        f.invoke(candidate::anctrl_setSmoothTransition,palette_controller,1);
        require(byte(f.rdram,palette_controller+0x1B)==0xE5 && !(word(f.rdram,f.record()+8)&TOOIE_BRIDGE_BLOCK_VALID),"compiled smooth setter preserves unrelated bits and retires");
        f.prepare(5); const uint32_t smooth=f.begin(); require(smooth==reset+512,"smooth discontinuity reissues"); f.finish(smooth);
        put(f.rdram,state+offsetof(TooieRenderBridgeState,births),3);
        f.prepare(6); const uint32_t rebirth=f.begin(); require(rebirth==smooth+512,"same descriptor/address rebirth cannot inherit old block"); f.finish(rebirth);
        f.prepare(7); put(f.rdram,0x8012CA18,0x11234);
        const uint32_t selector=f.begin(); require(selector==rebirth+512,"full-width effective selector transition renews"); f.finish(selector);
        ++groups;
    }
    {
        PaletteFixture f; const uint32_t first=f.begin(); f.finish(first);
        f.prepare(2); f.refused(TOOIE_BRIDGE_VIEW,0x50000002);
        f.prepare(2,0,0,0,2); f.refused(TOOIE_BRIDGE_VIEW,0x50000002);
        require(word(f.rdram,f.record()+offsetof(TooieActorPaletteSlot,camera_cut_views))==1,"changed camera holds entire view epoch including sibling");
        f.prepare(2,0,1); require(f.begin(0x50000003)==first+128,"cut in view0 does not refuse view1"); f.finish(first+128,1,0x50000003);
        f.prepare(2); f.refused(TOOIE_BRIDGE_VIEW,0x50000002);
        f.prepare(3); require(f.begin(0x50000002)==first,"next epoch reuses identity after cut absence"); f.finish(first,1,0x50000002);
        require(f.field(offsetof(TooieRenderBridgeState,next_block))==1,"camera cuts allocate no actor blocks");
        ++groups;
    }
    // Minimal independent mutations: each must refuse before any actor byte.
    const std::array<uint32_t,13> reasons={TOOIE_BRIDGE_ANIMATION,TOOIE_BRIDGE_ANIMATION,
        TOOIE_BRIDGE_ANIMATION,TOOIE_BRIDGE_ANIMATION,TOOIE_BRIDGE_MODEL_DATA,
        TOOIE_BRIDGE_MODEL_DATA,TOOIE_BRIDGE_MODEL_DATA,TOOIE_BRIDGE_MODEL_DATA,
        TOOIE_BRIDGE_MODEL_DATA,TOOIE_BRIDGE_OVERRIDE,TOOIE_BRIDGE_MODEL_DATA,
        TOOIE_BRIDGE_OVERRIDE,TOOIE_BRIDGE_OVERRIDE};
    for (uint32_t mutation=0;mutation<reasons.size();++mutation) {
        PaletteFixture f;
        auto* rdram=f.rdram;
        switch (mutation) {
        case 0: MEM_H(0,guest(0x8012474C))=0x731; break; // Nonzero producer mode.
        case 1: put_byte(f.rdram,palette_classes+20+5,0); break;
        case 2: f.class_record(2); break; // Ambiguous live class pose owner.
        case 3: MEM_H(2,guest(palette_classes+20))=4; break; // Handle == capacity.
        case 4: put(f.rdram,0x8012C858,1); break; // Both leaves selected.
        case 5: put(f.rdram,palette_model+0x110+4,16); break; // Child has sibling.
        case 6: put(f.rdram,palette_model+0x208,0xDA380000); break;
        case 7: put(f.rdram,palette_model+0x208+36,128); break; // Pop underflow.
        case 8: put(f.rdram,palette_model+0x208+20,0x03000080); break;
        case 9: put(f.rdram,palette_model+0x28,0x600); break; // CPU deform section.
        case 10: put(f.rdram,0x8012C964,palette_model+0x410); break; // Vertex override.
        case 11: put(f.rdram,0x8012C9A8,0x801017D4); break; // Arbitrary callback still excluded.
        case 12: put(f.rdram,0x8012C838,1); break; // CPU post-processing still excluded.
        }
        // Seed genuine original commands; a refusing begin must not alter them.
        put(f.rdram,f.cursor,0xDA380005); put(f.rdram,f.cursor+4,0x00400000);
        f.refused(reasons[mutation]);
        if (mutation==3) require(palette_controller_getters==0,"out-of-range controller refused before getter double");
    }
    ++groups;
    {
        PaletteFixture f;
        f.cursor=player_pool+TOOIE_BRIDGE_LOWER_BYTES-96; f.prepare(1);
        put(f.rdram,player_buffers+4,palette_mtx_pool+0x12C00-256);
        put(f.rdram,state+offsetof(TooieRenderBridgeState,seen_epoch),1);
        put(f.rdram,state+offsetof(TooieRenderBridgeState,extra_bytes),6368);
        const uint32_t root=f.begin(); f.admitted(root,1); f.finish(root);
        require(word(f.rdram,player_buffers)==player_pool+TOOIE_BRIDGE_LOWER_BYTES &&
            f.field(offsetof(TooieRenderBridgeState,extra_bytes))==6400,"exact suffix/Mtx/actor-ledger boundaries admit and finish at end");
        ++groups;
    }
    {
        // PC[8] OTHER isolation: a full protected PRIMARY partition neither refuses nor pays for a
        // nonplayer admission; the same palette admission pays the same OTHER bytes either way.
        uint32_t paid[2]={};
        for(uint32_t full=0;full<2;++full){
            PaletteFixture f; f.prepare(1);
            put(f.rdram,state+offsetof(TooieRenderBridgeState,seen_epoch),1);
            if(full){put(f.rdram,state+offsetof(TooieRenderBridgeState,primary_bytes),TOOIE_BRIDGE_PRIMARY_BYTES);
                put(f.rdram,state+offsetof(TooieRenderBridgeState,primary_highwater),TOOIE_BRIDGE_PRIMARY_BYTES);}
            const uint32_t root=f.begin(); f.admitted(root,1); f.finish(root);
            paid[full]=f.field(offsetof(TooieRenderBridgeState,extra_bytes));
            require(f.field(offsetof(TooieRenderBridgeState,primary_bytes))==(full?TOOIE_BRIDGE_PRIMARY_BYTES:0u),
                "a nonplayer admission never touches the PRIMARY ledger");
        }
        require(paid[0] && paid[0]==paid[1] && paid[0]<=6400u,"OTHER payment is independent of PRIMARY");
    }
    for (uint32_t resource=0;resource<3;++resource) {
        PaletteFixture f;
        if (resource==0) { f.cursor=player_pool+TOOIE_BRIDGE_LOWER_BYTES-88; f.prepare(1); }
        if (resource==1) put(f.rdram,player_buffers+4,palette_mtx_pool+0x12C00-192);
        if (resource==2) {
            put(f.rdram,state+offsetof(TooieRenderBridgeState,seen_epoch),1);
            put(f.rdram,state+offsetof(TooieRenderBridgeState,extra_bytes),6369);
        }
        f.refused(TOOIE_BRIDGE_CAPACITY);
    }
    ++groups;
    {
        PaletteFixture f; const uint32_t root=f.begin(); f.finish(root);
        put(f.rdram,state+offsetof(TooieRenderBridgeState,next_block),TOOIE_BRIDGE_BLOCK_COUNT);
        f.prepare(2); require(f.begin()==root,"stable valid identity works at allocator exhaustion"); f.finish(root);
        f.prepare(2,1); f.refused(TOOIE_BRIDGE_BLOCKS); ++groups;
    }
    {
        PaletteFixture f; const uint32_t root=f.begin();
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
        const unsigned stops=close_stop::calls;
#endif
        put(f.rdram,state+offsetof(TooieRenderBridgeState,silo_depth),2);
        f.invoke(candidate::tooie_actor_palette_end,player_buffers);
        require(word(f.rdram,ac+offsetof(TooieActorPaletteContext,open_group))==1,"nested return cannot consume outer paid pop");
        put(f.rdram,state+offsetof(TooieRenderBridgeState,silo_depth),1);
        f.invoke(candidate::func_8008B10C,palette_controller,0);
        f.finish(root);
        require(f.field(offsetof(TooieRenderBridgeState,poisoned))==1 &&
            f.field(offsetof(TooieRenderBridgeState,tripwire_count))!=0,"late owner retirement trips but safe prepaid pop survives");
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
        require(close_stop::calls==stops,"binding tripwire with owned paid pop must not stop");
#endif
        ++groups;
    }
    {
        PaletteFixture f; const uint32_t root=f.begin(); f.admitted(root,1);
        put(f.rdram,player_buffers,f.cursor+25);
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
        expect_required_stop(2,[&](){f.invoke(candidate::tooie_actor_palette_end,player_buffers);});
#else
        f.invoke(candidate::tooie_actor_palette_end,player_buffers);
#endif
        require(word(f.rdram,player_buffers)==f.cursor+25 && word(f.rdram,f.cursor+24)==0 &&
            f.field(offsetof(TooieRenderBridgeState,poisoned))==1,"unsafe returned cursor suppresses pop and poisons");
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
        require(f.field(offsetof(TooieRenderBridgeState,active))==1 &&
            word(f.rdram,ac+offsetof(TooieActorPaletteContext,open_group))==1,
            "unsafe palette stop cannot continue to clear the paid occurrence");
#else
        require(f.field(offsetof(TooieRenderBridgeState,active))==0,"nonselected palette retains soft-error cleanup");
#endif
        ++groups;
    }
    return groups;
}
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
// The extracted Honey body uses the selected camera pools/control ABI. Only its
// original draw/show and getter boundaries are doubles, as in the older fixture.
unsigned honey_close_fixtures() {
    constexpr uint32_t cs=TOOIE_CAMERA_STATE_VRAM;
    constexpr uint32_t cc=cs+offsetof(TooieCameraState,control);
    constexpr uint32_t base=TOOIE_CAMERA_MAIN_VRAM,start=base+0x100;
    for(int which=0;which<4;++which) {
        std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
        honey_case=which;honey_draws=0;honey_shows=0;
        const unsigned stops=close_stop::calls;
        put(rdram,state,TOOIE_BRIDGE_MAGIC);put(rdram,state+4,TOOIE_BRIDGE_LAYOUT);
        put(rdram,state+offsetof(TooieRenderBridgeState,epoch),1);
        put(rdram,state+offsetof(TooieRenderBridgeState,seen_epoch),1);
        put(rdram,state+offsetof(TooieRenderBridgeState,births),1);
        // An unpaid backwards cursor poisons/refuses, but has no PUSH to close.
        if(which==1)put(rdram,state+offsetof(TooieRenderBridgeState,last_end),start+8);
        put(rdram,palette_actor,palette_descriptor);put(rdram,palette_actor+0x7C,0x1000);
        MEM_H(0x14,guest(palette_descriptor))=0x612;MEM_H(0x16,guest(palette_descriptor))=0x1234;
        MEM_H(0,guest(0x8007BEDA))=0;
        put(rdram,0x8007BCD0,base);put(rdram,palette_cursor_slot,start);
        put(rdram,cs+offsetof(TooieCameraState,owner_frame),stack);
        put(rdram,cc+offsetof(TooieCameraControl,epoch),1);
        put(rdram,cc+offsetof(TooieCameraControl,active),1);
        put(rdram,cc+offsetof(TooieCameraControl,main_base),base);
        put(rdram,cc+offsetof(TooieCameraControl,main_end),base+TOOIE_CAMERA_M_BYTES);
        for(uint32_t at=start;at<start+64;at+=4)put(rdram,at,0xA5A5A5A5);
        auto c=context();c.r4=guest(palette_actor);c.r5=guest(honey_arg);const auto before=c;
        auto invoke=[&](){candidate::func_80800380_chhoneycarrier(rdram,&c);};
        if(which==3)expect_required_stop(1,invoke);
        else {invoke();saved(c,before);require(close_stop::calls==stops,"returning Honey path must not stop");}
        require(honey_draws==1,"Honey always calls the original draw boundary once");
        require(honey_shows==unsigned(which!=3),"unsafe Honey close cannot reach optional show continuation");
        const uint32_t after=which==1?start+8:which==3?start+25:start+40;
        require(word(rdram,palette_cursor_slot)==after,"Honey preserves exact original/push/pop cursor extent");
        require(word(rdram,state+offsetof(TooieRenderBridgeState,active))==unsigned(which==3),
            "only returning Honey paths release their owned active token");
        if(which==1) {
            require(word(rdram,state+offsetof(TooieRenderBridgeState,admissions))==0 &&
                word(rdram,state+offsetof(TooieRenderBridgeState,refused)+4*TOOIE_BRIDGE_CURSOR_DROP)==1 &&
                word(rdram,start)==0xD0000000 && word(rdram,start+8)==0xA5A5A5A5,
                "unpaid cursor-drop refusal preserves original draw with no group commands or stop");
        } else {
            require(word(rdram,state+offsetof(TooieRenderBridgeState,admissions))==1 &&
                word(rdram,start+8)==0x6400000C && word(rdram,start+12)==TOOIE_BRIDGE_ID_BASE+1,
                "Honey fixture actually admitted and emitted its paid PUSH");
            if(which==3)require(word(rdram,start+24)==0xA5A5A5A5 &&
                word(rdram,state+offsetof(TooieRenderBridgeState,poisoned))==1 &&
                word(rdram,cc+offsetof(TooieCameraControl,latch))==1,
                "unsafe Honey close poisons/latches and writes no unsafe POP");
            // gEXPopMatrixGroup encodes count1 in word1 even with projection0.
            else require(word(rdram,start+32)==0x6400000D && word(rdram,start+36)==1,
                "normal and selector-tripwire Honey paths emit their paid POP");
        }
        if(which==2)require(word(rdram,state+offsetof(TooieRenderBridgeState,tripwire_count))==1 &&
            word(rdram,state+offsetof(TooieRenderBridgeState,slots)+offsetof(TooieBridgeSlot,block_and_valid))==0,
            "selector-only Honey tripwire retires the binding without suppressing its safe POP");
    }
    honey_case=-1;return 4;
}

#ifndef TOOIE_BK_SHADOW_FIXTURE
// BK-OFF composition only: the legacy shadow admission, its MODELVIEW group and paid-close state.
unsigned shadow_mode_fixtures() {
    // Private ShadowState offsets from maintained shadow_projection.c. The
    // generated args address ties this setup to the actual selected layout.
    constexpr uint32_t ss=TOOIE_SHADOW_STATE,cursor=player_cursor;
    constexpr uint32_t epoch=4,id=8,group=20,composite=24,mode_open=28,fault=32;
    static_assert(TOOIE_SHADOW_SELECTED && TOOIE_SHADOW_ARGS==ss+36,"actual selected shadow ABI");
    for(unsigned which=0;which<3;++which) {
        std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
        const unsigned stops=close_stop::calls;
        put(rdram,state+offsetof(TooieRenderBridgeState,epoch),7);
        put(rdram,ss,0x53484403);put(rdram,ss+epoch,7);put(rdram,ss+id,123);put(rdram,ss+group,456);
        put(rdram,ss+composite,which==1?0:1);
        for(uint32_t at=cursor;at<cursor+32;at+=4)put(rdram,at,0xA5A5A5A5);
        auto invoke=[&](recomp_func_t* fn,uint32_t at) {
            auto c=context();c.r4=guest(at);const auto before=c;
            fn(rdram,&c);saved(c,before);return low(c.r2);
        };
        uint32_t tail=invoke(candidate::tooie_shadow_mode_begin,cursor);
        require(tail==cursor+(which==1?0u:16u) && word(rdram,ss+mode_open)==unsigned(which!=1),
            "actual shadow begin opens only an admitted composite mode scope");
        if(which==2) {
            expect_required_stop(3,[&](){invoke(candidate::tooie_shadow_complete,tail);});
            require(word(rdram,ss+fault)==1 && word(rdram,state+offsetof(TooieRenderBridgeState,poisoned))==1 &&
                word(rdram,ss+mode_open)==1 && word(rdram,ss+id)==123 && word(rdram,ss+group)==456 &&
                word(rdram,tail)==0xA5A5A5A5,
                "open shadow mode stops before cleanup and writes no fabricated POP");
        } else {
            tail=invoke(candidate::tooie_shadow_mode_end,tail);
            invoke(candidate::tooie_shadow_complete,tail);
            require(close_stop::calls==stops && word(rdram,ss+mode_open)==0 && word(rdram,ss+fault)==0 &&
                word(rdram,ss+id)==0 && word(rdram,ss+group)==0 && word(rdram,ss+composite)==0 &&
                word(rdram,state+offsetof(TooieRenderBridgeState,last_end))==tail,
                "normal and unpaid shadow completion return without a stop and clear transient identity");
        }
        if(which==1)for(uint32_t at=cursor;at<cursor+32;at+=4)
            require(word(rdram,at)==0xA5A5A5A5,"unpaid shadow refusal emits no mode words");
        else {
            require(word(rdram,cursor)==0x64000019 && word(rdram,cursor+4)==0 &&
                word(rdram,cursor+8)==0xE3001001 && word(rdram,cursor+12)==0,
                "actual shadow PUSH and texture-LUT command words");
            if(which==0)require(tail==cursor+24 && word(rdram,cursor+16)==0x6400001A &&
                word(rdram,cursor+20)==0 && word(rdram,cursor+24)==0xA5A5A5A5,
                "actual shadow mode end writes only its prepaid POP");
        }
    }
    return 3;
}

// Actual extracted coefficient-route DE498 helper, A666C close and composite refusal.
// The model holds only the census header/type12 fields; the rest of the immutable graph is the premise.
unsigned shadow_modelview_fixtures() {
    static_assert(TOOIE_SHADOW_MV==1u,"required-stop fixture selects the coefficient route");
    constexpr uint32_t ss=TOOIE_SHADOW_STATE,cs=TOOIE_CAMERA_STATE_VRAM;
    constexpr uint32_t cc=cs+offsetof(TooieCameraState,control),sv=cs+offsetof(TooieCameraState,saved);
    constexpr uint32_t base=TOOIE_CAMERA_MAIN_VRAM,start=base+0x100,silhouette=0x80620000,table=0x80230000;
    constexpr uint32_t id=TOOIE_BRIDGE_ID_BASE+7*512+480,live=0x02010AA9,skip=0x00000001;
    constexpr uint32_t group=20,fault=32,mv_group=56,mv_epoch=60,mv_id=64,selectors=0x8012C850;
    uint32_t camera_view_id=TOOIE_CAMERA_ID_FIRST;
    std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
    auto call=[&](recomp_func_t* fn,uint32_t a0,uint32_t a1,uint32_t a2,uint32_t a3,uint32_t a4) {
        auto c=context();c.r4=guest(a0);c.r5=guest(a1);c.r6=guest(a2);c.r7=guest(a3);put(rdram,stack+0x10,a4);
        const auto before=c;fn(rdram,&c);saved(c,before);
    };
    auto model=[&](uint32_t asset) {
        const bool mumbo=asset==0x665;
        const uint32_t nodes[2][5][3]={{{14056,1,15},{14088,1,16},{14128,2,38},{14200,2,39},{14272,1,17}},
                                       {{20480,4,1},{20592,1,4},{20624,1,5},{20672,0,6},{0,0,0}}};
        for(uint32_t a=silhouette;a<silhouette+20684;a+=4)put(rdram,a,0);
        put(rdram,silhouette+4,mumbo?20464:14040);MEM_H(8,guest(silhouette))=80;MEM_H(0xA,guest(silhouette))=0x40;
        put(rdram,silhouette+0xC,88);put(rdram,silhouette+0x10,mumbo?5960:5184);put(rdram,silhouette+0x18,mumbo?5296:4440);
        for(const auto& n:nodes[mumbo])if(n[0]) {
            put(rdram,silhouette+n[0],12);MEM_H(8,guest(silhouette+n[0]))=int16_t(n[1]);MEM_H(10,guest(silhouette+n[0]))=int16_t(n[2]);
        }
        put(rdram,pc+offsetof(TooiePlayerBoundary,effective_model_asset),asset);
        // PC[8]: shadow_topology reads the entry prepare selected (idbase = shadow id - 480).
        put(rdram,state+offsetof(TooieRenderBridgeState,primary)+16u,asset);
        put(rdram,state+offsetof(TooieRenderBridgeState,primary)+24u,id-480u);
    };
    // One admitted shadow DE498 at epoch e, an 8-byte original-output stand-in, then the A666C close.
    auto frame=[&](uint32_t e,uint32_t revision,bool close) {
        put(rdram,cs+offsetof(TooieCameraState,owner_frame),stack);
        put(rdram,cc+offsetof(TooieCameraControl,epoch),e);put(rdram,cc+offsetof(TooieCameraControl,active),1);
        put(rdram,cc+offsetof(TooieCameraControl,main_base),base);put(rdram,cc+offsetof(TooieCameraControl,main_begin),base);
        put(rdram,cc+offsetof(TooieCameraControl,main_end),base+TOOIE_CAMERA_M_BYTES);
        put(rdram,cc+offsetof(TooieCameraControl,main_extra),8);
        put(rdram,sv+offsetof(TooieCameraProduction,valid),1);put(rdram,sv+offsetof(TooieCameraProduction,epoch),e);
        put(rdram,sv+offsetof(TooieCameraProduction,birth),3);put(rdram,sv+offsetof(TooieCameraProduction,revision),revision);
        put(rdram,sv+offsetof(TooieCameraProduction,id),camera_view_id);
        put(rdram,ss+4,e);put(rdram,ss+8,id);put(rdram,ss+12,silhouette);put(rdram,ss+group,0);
        for(uint32_t a=start;a<start+64;a+=4)put(rdram,a,0xA5A5A5A5);
        put(rdram,palette_cursor_slot,start);
        call(candidate::tooie_shadow_de498,player_buffers,palette_cursor_slot,silhouette,0,0);
        const uint32_t cursor=word(rdram,palette_cursor_slot);
        put(rdram,cursor,0xDE000000);put(rdram,cursor+4,0x03000000);put(rdram,player_buffers,cursor+8);
        if(close)call(candidate::tooie_shadow_mv_end,0,0,0,0,0);
        return cursor;
    };
    auto grouped=[&](uint32_t cursor,uint32_t policy,const char* why) {
        require(cursor==start+32 && word(rdram,start)==0x6400000C && word(rdram,start+4)==id &&
            word(rdram,start+8)==0x0201154A && word(rdram,start+12)==0 &&
            word(rdram,start+16)==0x6400000C && word(rdram,start+20)==id+1 && word(rdram,start+24)==policy &&
            word(rdram,start+28)==0 && word(rdram,start+40)==0x6400000D && word(rdram,start+44)==1 &&
            word(rdram,player_buffers)==start+48 && word(rdram,ss+mv_group)==0 &&
            word(rdram,state+offsetof(TooieRenderBridgeState,last_end))==start+48,why);
    };
    auto untouched=[&](uint32_t cursor,const char* why) {
        require(cursor==start+16 && word(rdram,start+4)==id && word(rdram,start+16)==0xDE000000 &&
            word(rdram,start+24)==0xA5A5A5A5 && word(rdram,player_buffers)==start+24 && word(rdram,ss+mv_group)==0,why);
    };
    put(rdram,0x8012CF8C,table);put(rdram,table+3*4,0x800DD024);put(rdram,table+12*4,0x800DD56C);
    put(rdram,ss,0x53484403);
    model(0x62A);
    for(uint32_t s:{15u,16u,17u,38u,39u})put(rdram,selectors+s*4,s&1);
    grouped(frame(5,9,true),skip,"first stable-ID silhouette group skips; balanced POP at the DE448 return");
    grouped(frame(6,9,true),live,"same ID/camera/selector words in the next epoch interpolate");
    put(rdram,selectors+38*4,2);
    grouped(frame(7,9,true),skip,"changed live selector word skips under the same ID");
    grouped(frame(8,9,true),live,"unchanged selector words interpolate again");
    grouped(frame(9,10,true),skip,"camera revision (cut) skips");
    grouped(frame(11,10,true),skip,"epoch gap skips");
    grouped(frame(12,10,true),live,"continuous history interpolates");
    put(rdram,cc+offsetof(TooieCameraControl,latch),1);
    call(candidate::tooie_shadow_rectangle_args,start+48,1,2,3,4);
    require(word(rdram,start+4)==0 && word(rdram,start+8)==0x00000002 && word(rdram,start+20)==id+1 &&
        word(rdram,start+24)==skip && word(rdram,ss+mv_id)==0,
        "refused composite rewrites projection IGNORE and the owned push SKIP, restarting history");
    put(rdram,cc+offsetof(TooieCameraControl,latch),0);
    grouped(frame(13,10,true),skip,"history restarted after composite refusal skips");
    model(0x665);
    for(uint32_t s:{1u,4u,5u,6u})put(rdram,selectors+s*4,s);
    grouped(frame(14,10,true),skip,"854 selector key change skips");
    grouped(frame(15,10,true),live,"854 continuous history interpolates");
    // Construction-begin reset clears per-draw identity only; modelview history survives.
    put(rdram,cc+offsetof(TooieCameraControl,epoch),16);
    call(candidate::tooie_shadow_reset,stack,0,0,0,0);
    require(word(rdram,ss+4)==16 && word(rdram,ss+8)==0 && word(rdram,ss+fault)==0 &&
        word(rdram,ss+mv_id)==id+1 && word(rdram,ss+mv_epoch)==15 && word(rdram,ss+mv_group)==0,
        "reset between frames preserves modelview history");
    grouped(frame(16,10,true),live,"history preserved across reset interpolates");
    camera_view_id=TOOIE_CAMERA_ID_FIRST+1;
    grouped(frame(17,10,true),skip,"saved camera view-ID change skips");
    grouped(frame(18,10,true),live,"unchanged new camera view ID interpolates");
    put(rdram,silhouette+0x18,5297);
    untouched(frame(19,10,true),"unsupported header keeps the projection-only route");
    model(0x665);MEM_H(10,guest(silhouette+20592))=7;
    untouched(frame(20,10,true),"unsupported type12 census keeps the projection-only route");
    model(0x838);
    untouched(frame(21,10,true),"unsupported asset keeps the projection-only route");
    model(0x62A);
    const uint32_t open=frame(23,10,false);
    require(open==start+32 && word(rdram,ss+mv_group)==start+16,"owned push remains open until its close");
    put(rdram,player_buffers,start+24);
    expect_required_stop(3,[&](){call(candidate::tooie_shadow_mv_end,0,0,0,0,0);});
    require(word(rdram,ss+fault)==1 && word(rdram,state+offsetof(TooieRenderBridgeState,poisoned))==1 &&
        word(rdram,start+24)==skip && word(rdram,player_buffers)==start+24 && word(rdram,ss+mv_group)==start+16,
        "unsafe close cursor stops before any fabricated POP");
    expect_required_stop(3,[&](){call(candidate::tooie_shadow_complete,start+40,0,0,0,0);});
    return 19;
}
#endif
#endif
#ifdef TOOIE_CAMERA_PRODUCTION_FIXTURE
// R16 camera production identity through the actual selected func_800E44FC.
// Its original callees are explicit doubles below. widescreen_enabled stays 0,
// so the HOST raster path returns before its callees. Revision and birth write
// the record state cam_revision/cam_birth leave (MODELED). No consumed-RT64
// interpolation or visual result is claimed.
constexpr uint32_t camera_fixture_pointer=0x80250000,camera_normalization=0x0001FFFE,camera_ident_v0=0x5A5A0001;
uint32_t camera_lens_scale=0;
bool camera_lens_doubled=false;
unsigned camera_scale_reads=0,camera_projection_builds=0;
bool camera_float_fixture_active=false;
unsigned camera_rotation_reads=0;
unsigned camera_production_fixtures() {
    constexpr uint32_t cs=TOOIE_CAMERA_STATE_VRAM,r0=cs+offsetof(TooieCameraState,camera);
    constexpr uint32_t cc=cs+offsetof(TooieCameraState,control),sv=cs+offsetof(TooieCameraState,saved);
    constexpr uint32_t cd=cs+offsetof(TooieCameraState,diagnostics);
    constexpr uint32_t base=TOOIE_CAMERA_MAIN_VRAM,start=base+0x100,graphics=0x80251000,mtx=0x80252000;
    constexpr uint32_t one=0x3F800000,zoom=0x3F4CCCCD,two=0x40000000,first=TOOIE_CAMERA_ID_FIRST;
    constexpr uint32_t view_id=offsetof(TooieCameraRecord,view_id),view_scale=offsetof(TooieCameraRecord,view_scale);
    constexpr uint32_t seen_epoch=offsetof(TooieCameraRecord,seen_epoch),seen_role=offsetof(TooieCameraRecord,seen_role);
    std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
    auto rec=[&](uint32_t field,uint32_t view){return word(rdram,r0+field+4*view);};
    auto next=[&](){return word(rdram,cc+offsetof(TooieCameraControl,next_id));};
    auto overflow=[&](){return word(rdram,cd+offsetof(TooieCameraDiagnostics,overflow));};
    // MODELED cam_birth result: a fresh valid record for the same camera pointer.
    auto birth=[&](uint32_t value){
        for(uint32_t a=r0;a<r0+sizeof(TooieCameraRecord);a+=4)put(rdram,a,0);
        put(rdram,r0+offsetof(TooieCameraRecord,pointer),camera_fixture_pointer);
        put(rdram,r0+offsetof(TooieCameraRecord,birth),value);put(rdram,r0+offsetof(TooieCameraRecord,valid),1);
    };
    unsigned cases=0;
    // One WORLD_INITIAL production at epoch e and view v; returns the saved ID.
    auto produce=[&](uint32_t e,uint32_t view,uint32_t lens,bool doubled,uint32_t latch=0){
        put(rdram,0x8012D500,camera_fixture_pointer);put(rdram,0x80127674,view);
        put(rdram,cc+offsetof(TooieCameraControl,epoch),e);put(rdram,cc+offsetof(TooieCameraControl,active),1);
        put(rdram,cc+offsetof(TooieCameraControl,latch),latch);put(rdram,cc+offsetof(TooieCameraControl,main_reserve),0);
        put(rdram,cc+offsetof(TooieCameraControl,role),TOOIE_CAMERA_WORLD_INITIAL);
        put(rdram,cc+offsetof(TooieCameraControl,main_base),base);put(rdram,cc+offsetof(TooieCameraControl,main_begin),base);
        put(rdram,cc+offsetof(TooieCameraControl,main_end),base+TOOIE_CAMERA_M_BYTES);
        put(rdram,cc+offsetof(TooieCameraControl,main_extra),8);
        for(uint32_t a=start;a<start+48;a+=4)put(rdram,a,0xA5A5A5A5);
        put(rdram,graphics,start);put(rdram,graphics+4,mtx);
        camera_lens_scale=lens;camera_lens_doubled=doubled;camera_scale_reads=0;camera_projection_builds=0;
        auto c=context();c.r4=guest(graphics);const auto before=c;
        candidate::func_800E44FC(rdram,&c);saved(c,before);
        const uint32_t id=word(rdram,sv+offsetof(TooieCameraProduction,id)),scale=doubled?two:lens;
        const uint32_t policy=id?0x02008AAAu:0x00000002u;
        require(camera_scale_reads==(doubled?2u:1u) && camera_projection_builds==(doubled?2u:1u),
            "original scale/projection calls, doubled only when 18BC4 is set");
        require(low(c.r2)==camera_ident_v0 && word(rdram,graphics)==start+40 && word(rdram,graphics+4)==mtx+128 &&
            word(rdram,0x8012D558)==mtx,"original command/matrix cursors, D_8012D558 and guMtxIdent v0 return");
        require(word(rdram,start)==0xDB0E0000u && word(rdram,start+4)==camera_normalization &&
            word(rdram,start+8)==0x6400000Cu && word(rdram,start+12)==id && word(rdram,start+16)==policy &&
            word(rdram,start+20)==0 && word(rdram,start+24)==0xDA380007u && word(rdram,start+28)==mtx+0x80000000u &&
            word(rdram,start+32)==0xDA380003u && word(rdram,start+36)==mtx+64+0x80000000u &&
            word(rdram,start+40)==0xA5A5A5A5,"projection group with the saved ID/policy precedes the original loads");
        require(word(rdram,sv+offsetof(TooieCameraProduction,valid))==1 &&
            word(rdram,sv+offsetof(TooieCameraProduction,epoch))==e &&
            word(rdram,sv+offsetof(TooieCameraProduction,camera))==camera_fixture_pointer &&
            word(rdram,sv+offsetof(TooieCameraProduction,view))==view &&
            word(rdram,sv+offsetof(TooieCameraProduction,matrix))==mtx &&
            word(rdram,sv+offsetof(TooieCameraProduction,scale))==scale &&
            word(rdram,sv+offsetof(TooieCameraProduction,policy))==policy &&
            word(rdram,sv+offsetof(TooieCameraProduction,source))==0x800E4598u,
            "saved production carries the produced scale word, view and policy");
        if(id)require(rec(view_id,view)==id && rec(view_scale,view)==scale && rec(seen_epoch,view)==e,
            "accepted production refreshes the record scale; reader record==saved scale equality holds");
        ++cases;return id;
    };
    birth(3);put(rdram,cc+offsetof(TooieCameraControl,next_id),first);
    require(produce(1,0,one,false)==first && next()==first+1 &&
        word(rdram,cd+offsetof(TooieCameraDiagnostics,id_highwater))==first,"new camera birth/view allocates one WORLD ID");
    require(produce(2,0,zoom,false)==first && next()==first+1 &&
        word(rdram,cd+offsetof(TooieCameraDiagnostics,id_highwater))==first,"lens-only scale change keeps the WORLD ID and allocates nothing");
    require(produce(3,0,one,false)==first && next()==first+1,"returning to the earlier scale keeps the same ID");
    require(produce(4,0,one,true)==first && rec(view_scale,0)==two && next()==first+1,
        "18BC4 doubled scale is refreshed metadata, not a cut");
    require(produce(4,0,zoom,false)==0 && rec(view_id,0)==first && rec(view_scale,0)==two &&
        rec(seen_epoch,0)==4 && next()==first+1,"duplicate same-view production refuses before publishing its scale");
    for(uint32_t bad:{0x00000000u,0x80000000u,0x7F800000u,0x7FC00000u})
        require(produce(5,0,bad,false)==0 && rec(view_id,0)==first && rec(view_scale,0)==two &&
            rec(seen_epoch,0)==4 && next()==first+1,"zero/non-finite scale refuses before publishing or consuming the epoch");
    require(produce(5,0,zoom,false)==first,"refused productions leave the epoch's view admissible");
    require(produce(6,1,one,false)==first+1 && next()==first+2,"a second view allocates its own WORLD ID");
    require(produce(6,0,zoom,false)==first && rec(view_id,1)==first+1,"views keep independent IDs in one epoch");
    // MODELED cam_revision result (camera_internal_02.h): revision+1, per-view ID/seen cleared.
    put(rdram,r0+offsetof(TooieCameraRecord,revision),1);
    for(uint32_t v=0;v<4;++v){put(rdram,r0+view_id+4*v,0);put(rdram,r0+seen_epoch+4*v,0);put(rdram,r0+seen_role+4*v,0);}
    require(produce(7,0,zoom,false)==first+2 && word(rdram,sv+offsetof(TooieCameraProduction,revision))==1 &&
        next()==first+3,"camera revision (cut) renews the WORLD ID at an unchanged scale");
    birth(4);
    require(produce(8,0,zoom,false)==first+3 && word(rdram,sv+offsetof(TooieCameraProduction,birth))==4 &&
        next()==first+4,"camera birth renews the WORLD ID");
    require(produce(9,0,one,false,1)==0 && rec(view_id,0)==first+3 && rec(view_scale,0)==zoom && next()==first+4,
        "latched control refuses without allocation or metadata change");
    put(rdram,cc+offsetof(TooieCameraControl,next_id),TOOIE_CAMERA_ID_END);
    const uint32_t overflows=overflow();
    require(produce(10,0,one,false)==first+3 && overflow()==overflows && rec(view_scale,0)==one,
        "a lens change needs no allocation even with the ID space exhausted");
    require(produce(10,1,one,false)==0 && overflow()==overflows+1 && rec(view_id,1)==0,
        "exhausted allocator still refuses a view without an ID");
    return cases;
}
// Actual translated E44FC/E4640/E4628, with deterministic P/position/R and fixed-conversion
// doubles. Tests command ownership/lifetime and fallback, not original camera math or GPU pixels.
unsigned camera_float_route_fixtures() {
#ifndef TOOIE_CAMERA_FIXTURE_BOOT_IMAGE
#error The actual float fixture requires its selected linked boot image for compiler literals
#endif
    std::ifstream image_file(TOOIE_CAMERA_FIXTURE_BOOT_IMAGE,std::ios::binary);
    require(bool(image_file),"selected camera boot image opens");
    const std::vector<uint8_t> image((std::istreambuf_iterator<char>(image_file)),std::istreambuf_iterator<char>());
    require(!image.empty() && image.size()<=0x7FF000u && !(image.size()&3u),"selected boot image extent and word alignment");
    constexpr uint32_t cs=TOOIE_CAMERA_STATE_VRAM,cc=cs+offsetof(TooieCameraState,control);
    constexpr uint32_t sv=cs+offsetof(TooieCameraState,saved),rec=cs+offsetof(TooieCameraState,camera);
    constexpr uint32_t main=0x80127658,legacy=0x80251000,arena0=0x80300000,arena1=0x80400000;
    constexpr uint32_t gfx_base=TOOIE_CAMERA_MAIN_VRAM,start=gfx_base+0x400;
    struct Case { uint32_t context,role,alternate,index,offset; bool active,bad_rotation,accept; const char* why; };
    const Case cases[]={
        {main,TOOIE_CAMERA_WORLD_INITIAL,0,0,0,true,false,true,"ordinary MAIN float production"},
        {main,TOOIE_CAMERA_WORLD_INITIAL,0,1,0,true,false,true,"second owned ordinary matrix arena"},
        {main,TOOIE_CAMERA_WORLD_INITIAL,0,0,TOOIE_CAMERA_MAIN_MATRIX_BYTES-320,true,false,true,"last complete five-slot production fits"},
        {main,TOOIE_CAMERA_WORLD_INITIAL,0,0,TOOIE_CAMERA_MAIN_MATRIX_BYTES-128,true,false,false,"two original slots fit but float block does not"},
        {main,TOOIE_CAMERA_WORLD_INITIAL,1,0,0,true,false,false,"alternate matrix owner stays fixed"},
        {legacy,TOOIE_CAMERA_WORLD_INITIAL,0,0,0,true,false,false,"non-MAIN graphics context stays fixed"},
        {main,TOOIE_CAMERA_PRE_WORLD,0,0,0,true,false,false,"PRE production stays fixed"},
        {main,TOOIE_CAMERA_WORLD_INITIAL,0,0,0,false,false,false,"inactive group emission stays fixed"},
        {main,TOOIE_CAMERA_WORLD_INITIAL,0,0,8,true,false,false,"misaligned arena-relative slots stay fixed"},
        {main,TOOIE_CAMERA_WORLD_INITIAL,0,0,0,true,true,false,"non-affine rotation refuses float publication"},
        {main,TOOIE_CAMERA_WORLD_INITIAL,0,2,0,true,false,false,"invalid matrix pool selector stays fixed"},
    };
    unsigned ran=0;
    for(const auto& k:cases){
        std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
        // Translated guest code reads linked .rodata, including reciprocal's 1.0f.
        // Load the actual selected image as N64 big-endian words in RDRAM.
        for(size_t i=0;i<image.size();i+=4)put(rdram,0x80801000u+uint32_t(i),
            (uint32_t(image[i])<<24)|(uint32_t(image[i+1])<<16)|(uint32_t(image[i+2])<<8)|image[i+3]);
        const uint32_t arena=k.index==1?arena1:arena0,mtx=arena+k.offset,block=mtx+128;
        put(rdram,0x8007BCD8,arena0);put(rdram,0x8007BCDC,arena1);
        MEM_H(0,guest(0x8007BEDA))=int16_t(k.index);put(rdram,0x8007BCF0,k.alternate);
        put(rdram,0x8012D500,camera_fixture_pointer);put(rdram,0x80127674,0);
        for(uint32_t i=0;i<16;++i)put(rdram,camera_fixture_pointer+0xA4+4*i,i%5==0?0x3F800000:0);
        if(k.bad_rotation)put(rdram,camera_fixture_pointer+0xA4+12,0x3F800000);
        put(rdram,rec+offsetof(TooieCameraRecord,pointer),camera_fixture_pointer);
        put(rdram,rec+offsetof(TooieCameraRecord,birth),1);put(rdram,rec+offsetof(TooieCameraRecord,valid),1);
        put(rdram,cc+offsetof(TooieCameraControl,epoch),7);put(rdram,cc+offsetof(TooieCameraControl,active),k.active);
        put(rdram,cc+offsetof(TooieCameraControl,next_id),TOOIE_CAMERA_ID_FIRST);
        put(rdram,cc+offsetof(TooieCameraControl,role),k.role);
        put(rdram,cc+offsetof(TooieCameraControl,main_base),gfx_base);
        put(rdram,cc+offsetof(TooieCameraControl,main_begin),gfx_base);
        put(rdram,cc+offsetof(TooieCameraControl,main_end),gfx_base+TOOIE_CAMERA_M_BYTES);
        put(rdram,cc+offsetof(TooieCameraControl,main_extra),8);
        for(uint32_t a=start;a<start+96;a+=4)put(rdram,a,0xA5A5A5A5);
        for(uint32_t a=mtx;a<block+192+4;a+=4)put(rdram,a,0xA5A5A5A5);
        put(rdram,k.context,start);put(rdram,k.context+4,mtx);
        camera_float_fixture_active=true;camera_rotation_reads=0;
        camera_lens_scale=0x3F800000;camera_lens_doubled=false;
        auto c=context();c.r4=guest(k.context);const auto before=c;
        candidate::func_800E44FC(rdram,&c);saved(c,before);
        require(low(c.r2)==camera_ident_v0 && word(rdram,0x8012D558)==mtx,k.why);
        for(uint32_t i=0;i<16;++i)require(word(rdram,mtx+4*i)==0xC0DE0000u+i,"original fixed product remains retained");
        const uint32_t end=word(rdram,k.context),load=end-16;
        require(word(rdram,load+8)==0xDA380003 && word(rdram,load+12)==mtx+64+0x80000000u,k.why);
        require(word(rdram,k.context+4)==mtx+128+(k.accept?192:0),k.why);
        require(word(rdram,sv+offsetof(TooieCameraProduction,reserved0))==(k.accept?block:0),k.why);
        require(word(rdram,load)==(k.accept?0xDE000000u:0xDA380007u) &&
            word(rdram,load+4)==(k.accept?block+128:mtx)+0x80000000u,k.why);
        if(k.accept){
            require(camera_rotation_reads==1 && word(rdram,block)==0x3F400000 &&
                word(rdram,block+64)==0x3F800000 && word(rdram,block+64+48)==0xC2F00000 &&
                word(rdram,block+64+60)==0x3F800000,([&](){std::ostringstream message;message<<"float block: case="<<k.why<<" rotation_reads="<<camera_rotation_reads
                <<std::hex<<" P00="<<word(rdram,block)<<" V00="<<word(rdram,block+64)
                <<" V30="<<word(rdram,block+112)<<" V33="<<word(rdram,block+124);return message.str();}()).c_str());
            const std::array<uint32_t,10> commands={0x64000030,7,0,block+0x80000000u,
                0x64000030,5,0,block+64+0x80000000u,0xDF000000,0};
            for(uint32_t i=0;i<commands.size();++i)require(word(rdram,block+128+4*i)==commands[i],"actual emitted float sublist words");
            require(word(rdram,block+192)==0xA5A5A5A5,"float block does not write beyond its owned three slots");
            std::array<uint32_t,48> frozen{};
            for(uint32_t i=0;i<48;++i)frozen[i]=word(rdram,block+i*4);
            auto restore=[&](uint32_t role,uint32_t epoch,uint32_t redirect,uint32_t expected,uint32_t target,uint32_t bytes){
                put(rdram,k.context,start+64);put(rdram,cc+offsetof(TooieCameraControl,role),role);
                put(rdram,cc+offsetof(TooieCameraControl,epoch),epoch);put(rdram,0x8012D508,redirect);
                auto r=context();r.r4=guest(k.context);const auto old=r;
                candidate::func_800E4640(rdram,&r);saved(r,old);
                const uint32_t out=word(rdram,k.context);
                require(out==start+64+bytes && word(rdram,out-8)==expected && word(rdram,out-4)==target,
                    "restore calls own immutable float sublist or the correct original fallback");
                for(uint32_t i=0;i<48;++i)require(word(rdram,block+i*4)==frozen[i],"restore never mutates production float block");
            };
            restore(TOOIE_CAMERA_WORLD_INITIAL,7,0,0xDE000000,block+128+0x80000000u,24);
            restore(0x80000000u,7,0,0xDE000000,block+128+0x80000000u,8);
            restore(0x80000000u,8,0,0xDA380007,mtx+0x80000000u,8);
            restore(0x80000000u,7,1,0xDE000000,0x08000000,8);
            auto reset=context();const auto old=reset;
            candidate::func_800E4628(rdram,&reset);saved(reset,old);
            require(word(rdram,0x8012D558)==0 && word(rdram,0x8012D554)==0 &&
                word(rdram,sv+offsetof(TooieCameraProduction,valid))==0 &&
                word(rdram,sv+offsetof(TooieCameraProduction,reserved0))==0,"reset invalidates fixed and float saved production together");
        }
        camera_float_fixture_active=false;++ran;
    }
    return ran;
}

#endif
#ifdef TOOIE_CAMERA_PRODUCTION_FIXTURE
// N2: the selected HOST func_800E42F0 (camera_emitters.c CAM_PATCH in boot.c) is ACTUAL for its
// host/bounds/policy decision, the inlined cam_projection_policy words, both DA38 loads,
// D_8012D554, the gfx/mtx cursor arithmetic and its return. DOUBLED: guOrtho and guMtxIdent
// record arguments only (no matrix math) and the settings import returns the case word.
// MODELED: an ordinary MAIN control context. Not claimed: Mtx contents, RT64 AUTO/STRETCH use.
bool camera_e42f0_active=false;
uint32_t camera_settings_word=0,camera_ortho_args[8]={},camera_ident_arg=0;
unsigned camera_settings_reads=0,camera_ortho_calls=0,camera_ident_calls=0;
unsigned e42f0_fixtures() {
    constexpr uint32_t cs=TOOIE_CAMERA_STATE_VRAM,cc=cs+offsetof(TooieCameraState,control);
    constexpr uint32_t base=TOOIE_CAMERA_MAIN_VRAM,start=base+0x200,graphics=0x80251000,mtx=0x80252000;
    constexpr uint32_t wide_flag=0x800799B8,reticle=0x8012D493,saved_projection=0x8012D554;
    constexpr uint32_t auto_policy=0x02031556,stretch_policy=0x02131556;
    constexpr uint32_t narrow_lr[2]={0xC4180000,0x44180000},wide_lr[2]={0xC44A8000,0x444A8000};
    // context: 0 inactive control, 1 active MAIN first policy (Enable), 2 active MAIN already enabled.
    struct Case { int16_t wide; uint32_t use; uint8_t reticle; uint32_t settings; int context;
                  bool reads,wide_bounds,stretch; const char* why; };
    const Case cases[]={
        {0,1,1,3,2,false,false,false,"wide flag off: no settings read, 4:3 bounds, AUTO"},
        {0x100,1,0,3,2,true,false,false,"16-bit wide flag and both settings bits: host keeps 4:3 bounds and AUTO for wide=1"},
        {1,2,0,1,2,true,true,false,"ACTIVE only is not host: any nonzero use_widescreen takes the original wide bounds"},
        {1,1,0,2,2,true,true,false,"ENABLED only is not host: original wide bounds"},
        {1,0,0x80,3,2,true,false,true,"reticle byte with host width selects STRETCH at 4:3 bounds"},
        {1,0,1,0xFFFFFFFCu,2,true,false,false,"upper settings bits without both mask bits are not host: AUTO"},
        {1,0,0,3,2,false,false,false,"a wide=0 loader without the reticle byte reads no settings: AUTO"},
        {1,0,1,3,1,true,false,true,"the first MAIN policy emits Enable before the STRETCH group"},
        {1,0,1,3,0,true,false,false,"inactive control emits no extended words; the original loads still follow"},
    };
    unsigned ran=0;
    for(const auto& k:cases){
        std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
        MEM_H(0,guest(wide_flag))=k.wide;put_byte(rdram,reticle,k.reticle);
        put(rdram,cc+offsetof(TooieCameraControl,epoch),1);put(rdram,cc+offsetof(TooieCameraControl,active),k.context?1:0);
        put(rdram,cc+offsetof(TooieCameraControl,main_base),base);put(rdram,cc+offsetof(TooieCameraControl,main_begin),start);
        put(rdram,cc+offsetof(TooieCameraControl,main_end),base+TOOIE_CAMERA_M_BYTES);
        put(rdram,cc+offsetof(TooieCameraControl,main_extra),k.context==2?8:0);
        for(uint32_t a=start;a<start+64;a+=4)put(rdram,a,0xA5A5A5A5);
        put(rdram,graphics,start);put(rdram,graphics+4,mtx);
        camera_e42f0_active=true;camera_settings_word=k.settings;
        camera_settings_reads=0;camera_ortho_calls=0;camera_ident_calls=0;
        auto c=context();c.r4=guest(graphics);c.r5=guest(k.use);const auto before=c;
        candidate::func_800E42F0(rdram,&c);saved(c,before);camera_e42f0_active=false;
        const uint32_t* bounds=k.wide_bounds?wide_lr:narrow_lr;
        const uint32_t policy=k.stretch?stretch_policy:auto_policy;
        const uint32_t loads=k.context==0?start:k.context==1?start+24:start+16;
        require(camera_settings_reads==(k.reads?1u:0u),k.why);
        require(camera_ortho_calls==1 && camera_ortho_args[0]==mtx && camera_ortho_args[1]==bounds[0] &&
            camera_ortho_args[2]==bounds[1] && camera_ortho_args[3]==0xC3E40000 && camera_ortho_args[4]==0x43E40000 &&
            camera_ortho_args[5]==0xC2480000 && camera_ortho_args[6]==0x42480000 && camera_ortho_args[7]==0x3F800000,k.why);
        if(k.context==1)require(word(rdram,start)==0xE0525464 && word(rdram,start+4)==0x10000064,k.why);
        if(k.context)require(word(rdram,loads-16)==0x6400000C && word(rdram,loads-12)==0xFFFFFFFF &&
            word(rdram,loads-8)==policy && word(rdram,loads-4)==0 &&
            word(rdram,cc+offsetof(TooieCameraControl,main_extra))==24,k.why);
        require(word(rdram,loads)==0xDA380007 && word(rdram,loads+4)==mtx+0x80000000u &&
            word(rdram,loads+8)==0xDA380003 && word(rdram,loads+12)==mtx+64+0x80000000u &&
            word(rdram,loads+16)==0xA5A5A5A5 && word(rdram,graphics)==loads+16 && low(c.r2)==loads+8 &&
            word(rdram,graphics+4)==mtx+128 && word(rdram,saved_projection)==mtx &&
            camera_ident_calls==1 && camera_ident_arg==mtx+64,k.why);
        ++ran;
    }
    return ran;
}
#endif
#ifdef TOOIE_CKCOPY_FIXTURE
// P04b actual extracted ckpack copy-storage helpers: S1 root begin, S2 segment word, S3 root
// end, S4 release. Synthetic roster, part, copy-manager FreeList and main-pool commands; the
// four hook seams (HPC1-4) are source facts, not executed here.
static_assert(TOOIE_CKCOPY_STATE_BYTES==122960u,"P04b copy-storage state ABI (layout 2)");
unsigned ckcopy_fixtures() {
    constexpr uint32_t ck=TOOIE_CKCOPY_STATE,entry0=ck+80,slot0=ck+80+2*64*96;
    constexpr uint32_t staged=0x8012C964,beda=0x8007BEDA,pools=0x8007BCD0,roster=0x80135490,freelist=0x80135A60;
    constexpr uint32_t owner=0x80224000,part=0x80225000,pool=0x80226000,copy_a=0x80228000,copy_b=0x8022A000;
    constexpr uint32_t record=pool+16,other=0x8022C000;
    struct Rig { std::vector<uint8_t> ram=std::vector<uint8_t>(16*1024*1024); };
    auto base=[](uint32_t p){return uint32_t(TOOIE_CAMERA_MAIN_VRAM+p*TOOIE_CAMERA_M_BYTES);};
    auto entry=[&](uint32_t p,uint32_t i){return entry0+(p*64+i)*96;};
    auto slot=[&](uint32_t p,uint32_t s){return slot0+(p*16+s)*3456;};
    auto phys=[](uint32_t a){return a&0x1FFFFFFFu;};
    auto setup=[&](Rig& g,uint32_t p,uint32_t count){
        auto* rdram=g.ram.data();
        put(rdram,ck,0x434B4350);put(rdram,ck+4,2);
        put(rdram,roster,owner);put(rdram,owner+20,part);put_byte(rdram,part+8,1);put_byte(rdram,part+10,1);
        put(rdram,freelist,pool);MEM_H(0,guest(pool))=12;MEM_H(2,guest(pool))=4;
        put(rdram,record,copy_a);put(rdram,record+4,copy_b);put_byte(rdram,record+9,1);
        MEM_H(22,guest(copy_a))=int16_t(2*count);MEM_H(22,guest(copy_b))=int16_t(2*count);
        put(rdram,pools,base(0));put(rdram,pools+4,base(1));MEM_H(0,guest(beda))=int16_t(p);
    };
    auto call=[](Rig& g,recomp_func_t* fn,uint32_t a0){
        auto c=context();c.r4=guest(a0);const auto before=c;fn(g.ram.data(),&c);saved(c,before);return low(c.r2);
    };
    auto w=[](Rig& g,uint32_t a){return word(g.ram.data(),a);};
    auto emit=[&](Rig& g,uint32_t cmd,uint32_t copy){
        put(g.ram.data(),staged,copy);put(g.ram.data(),cmd,0xDB060004);put(g.ram.data(),cmd+4,phys(copy+24));
        return call(g,candidate::tooie_ckcopy_segment,cmd);
    };
    auto root_begin=[&](Rig& g,uint32_t frame=0x80EFFF00u){return call(g,candidate::tooie_ckcopy_root_begin,frame);};
    auto root_end=[&](Rig& g,uint32_t frame=0x80EFFF00u){return call(g,candidate::tooie_ckcopy_root_end,frame);};
    auto release=[&](Rig& g,uint32_t at){return call(g,candidate::tooie_ckcopy_release,at);};
    auto fill=[](Rig& g,uint32_t at,uint32_t count,uint32_t tag){for(uint32_t i=0;i<16*count;i+=4)put(g.ram.data(),at+i,tag+i);};
    auto holds=[&](Rig& g,uint32_t at,uint32_t count,uint32_t tag){bool ok=true;for(uint32_t i=0;i<16*count;i+=4)ok=ok&&w(g,at+i)==tag+i;return ok;};
    unsigned groups=0;
    {Rig g;setup(g,1,212);put(g.ram.data(),ck+48,5);
     require(root_begin(g)==0 && w(g,ck+8)==1 && w(g,ck+12)==1 && w(g,ck+52)==0 && w(g,ck+48)==5,
        "S1 opens restoration authority for the selected pool only");++groups;}
    {Rig g;setup(g,0,212);root_begin(g);const uint32_t a=base(0)+0x100;
     require(emit(g,a,copy_a)==0 && w(g,a+4)==phys(slot(0,0)) && w(g,entry(0,0))==copy_a && w(g,entry(0,0)+4)==212 &&
        w(g,entry(0,0)+8)==1 && w(g,entry(0,0)+16)==1 && w(g,entry(0,0)+20)==a,
        "a protected copy's segment word is recorded and redirected to its pool slot");++groups;}
    {Rig g;setup(g,0,212);root_begin(g);const uint32_t a=base(0)+0x100,b=base(0)+0x200;emit(g,a,copy_a);emit(g,b,copy_a);
     require(w(g,a+4)==phys(slot(0,0)) && w(g,b+4)==phys(slot(0,0)) && w(g,ck+48)==1 && w(g,entry(0,0)+16)==2,
        "silhouette and body references to one copy share one slot");++groups;}
    for(int which=0;which<4;++which){
        Rig g;setup(g,0,212);auto* rdram=g.ram.data();
        if(which==0)put_byte(rdram,part+10,0);
        if(which==1)put(rdram,record,other);
        if(which==2)put_byte(rdram,record+9,0);
        root_begin(g);const uint32_t at=which==3?0x80300000u:base(0)+0x100;emit(g,at,copy_a);
        require(w(g,at+4)==phys(copy_a+24) && w(g,entry(0,0))==copy_a && w(g,entry(0,0)+8)==2,
            "an unprotected copy or a command outside the open main pool stays original and holds its address");
    }
    ++groups;
    {Rig g;setup(g,0,212);root_begin(g);const uint32_t a=base(0)+0x100,b=base(0)+0x200;
     emit(g,a,copy_a);fill(g,copy_a+24,212,0x11000000);emit(g,b,copy_a);fill(g,copy_a+24,212,0x22000000);
     require(root_end(g)==0 && w(g,ck+8)==0 && holds(g,slot(0,0),212,0x22000000) &&
        w(g,a+4)==phys(slot(0,0)) && w(g,b+4)==phys(slot(0,0)),
        "S3 publishes the copy's final root bytes for every reference and closes the construction");++groups;}
    {Rig g;setup(g,0,212);const uint32_t a=base(0)+0x100,b=base(1)+0x100;
     root_begin(g);emit(g,a,copy_a);fill(g,copy_a+24,212,0x33000000);root_end(g);
     auto* rdram=g.ram.data();MEM_H(0,guest(beda))=1;
     root_begin(g);emit(g,b,copy_a);fill(g,copy_a+24,212,0x44000000);root_end(g);
     require(holds(g,slot(0,0),212,0x33000000) && holds(g,slot(1,0),212,0x44000000) &&
        w(g,a+4)==phys(slot(0,0)) && w(g,b+4)==phys(slot(1,0)),"each pool keeps its own root's published slot");++groups;}
    {Rig g;setup(g,0,212);root_begin(g);
     for(uint32_t i=0;i<64;++i)emit(g,base(0)+0x100+8*i,0x80300000u+0x1000*i);
     const uint32_t a=base(0)+0x800,b=base(0)+0x808;emit(g,a,copy_a);emit(g,b,copy_a);
     require(w(g,ck+48)==64 && w(g,ck+24)==2 && w(g,a+4)==phys(copy_a+24) && w(g,b+4)==phys(copy_a+24),
        "a full entry table keeps a later copy original for the whole root");++groups;}
    {Rig g;setup(g,0,217);root_begin(g);const uint32_t a=base(0)+0x100;emit(g,a,copy_a);
     require(w(g,a+4)==phys(copy_a+24) && w(g,entry(0,0)+8)==2,"an extent beyond the 216-vertex protected class stays original");++groups;}
    {Rig g;setup(g,0,212);root_begin(g);const uint32_t a=base(0)+0x100,b=base(0)+0x200;emit(g,a,copy_a);emit(g,b,copy_a);
     put(g.ram.data(),a,0xE7000000);put(g.ram.data(),a+4,0);
     require(release(g,record)==1 && w(g,a)==0xE7000000 && w(g,a+4)==0 && w(g,b+4)==phys(slot(0,0)) &&
        w(g,ck+40)==1 && w(g,entry(0,0)+8)==1,"an unrestorable redirected word faults; nothing is half-restored or marked held");
     require(emit(g,base(0)+0x300,copy_a)==1 && root_end(g)==1,"a faulted construction stays rejected");++groups;}
    {Rig g;setup(g,0,212);root_begin(g);const uint32_t a=base(0)+0x100,b=base(0)+0x200;emit(g,a,copy_a);emit(g,b,copy_a);
     require(release(g,record)==0 && w(g,a+4)==phys(copy_a+24) && w(g,b+4)==phys(copy_a+24) &&
        w(g,entry(0,0)+8)==2 && w(g,ck+28)==1,"S4 restores every reference of a released copy and holds its address");++groups;}
    {Rig g;setup(g,0,212);root_begin(g);const uint32_t a=base(0)+0x100,b=base(0)+0x200;emit(g,a,copy_a);release(g,record);
     auto* rdram=g.ram.data();MEM_H(22,guest(copy_a))=432;fill(g,slot(0,0),216,0x5A000000);
     emit(g,b,copy_a);root_end(g);
     require(w(g,b+4)==phys(copy_a+24) && holds(g,slot(0,0),216,0x5A000000),
        "a released address reused by a 216-vertex copy stays original and publishes nothing");++groups;}
    {Rig g;setup(g,0,212);root_begin(g);const uint32_t a=base(0)+0x100,b=base(0)+0x200;emit(g,a,copy_a);
     auto* rdram=g.ram.data();MEM_H(22,guest(copy_a))=432;emit(g,b,copy_a);
     require(w(g,a+4)==phys(copy_a+24) && w(g,b+4)==phys(copy_a+24) && w(g,entry(0,0)+8)==2,
        "an extent change without a release reverts every reference");++groups;}
    {Rig g;setup(g,0,212);root_begin(g);const uint32_t a=base(0)+0x100;emit(g,a,copy_a);
     put(g.ram.data(),record,other);fill(g,slot(0,0),212,0x66000000);root_end(g);
     require(w(g,a+4)==phys(copy_a+24) && holds(g,slot(0,0),212,0x66000000) && w(g,entry(0,0)+8)==2,
        "S3 reverts a copy that is no longer live instead of publishing it");++groups;}
    {Rig g;setup(g,0,212);root_begin(g);bool original=true;
     for(uint32_t i=0;i<9;++i)emit(g,base(0)+0x100+16*i,copy_a);
     for(uint32_t i=0;i<9;++i)original=original&&w(g,base(0)+0x104+16*i)==phys(copy_a+24);
     require(original && w(g,ck+36)==1 && w(g,entry(0,0)+8)==2,"a ninth reference reverts all eight and holds the copy");++groups;}
    {Rig g;setup(g,0,212);root_begin(g);const uint32_t a=base(0)+0x100,b=base(0)+0x200;emit(g,a,copy_a);
     require(root_begin(g,0x80EFFE00u)==0 && w(g,a+4)==phys(copy_a+24) && w(g,ck+20)==1 && w(g,ck+44)==1 && w(g,ck+16)==0x80EFFF00u,
        "a nested begin reverts the open outer table only and keeps the outer owner frame");
     require(root_end(g,0x80EFFE00u)==0 && w(g,ck+8)==1,"the nested end does not close the outer construction");
     emit(g,b,copy_b);
     require(w(g,b+4)==phys(copy_b+24) && root_end(g)==0 && w(g,ck+8)==0,"a disqualified root redirects nothing and its owner's end closes it");++groups;}
    {Rig g;setup(g,0,212);root_begin(g);const uint32_t a=base(0)+0x100,b=base(0)+0x200;emit(g,a,copy_a);root_end(g);
     require(release(g,record)==0 && w(g,a+4)==phys(slot(0,0)) && w(g,ck+28)==0,
        "S4 outside an open construction leaves submitted commands and slots untouched");
     emit(g,b,copy_a);require(w(g,b+4)==phys(copy_a+24),"no redirection outside an open construction");++groups;}
    {Rig g;setup(g,1,212);auto* rdram=g.ram.data();const uint32_t prev=base(1)+0x100,a=base(0)+0x100;
     root_begin(g);emit(g,prev,copy_a);fill(g,copy_a+24,212,0x77000000);root_end(g);
     MEM_H(0,guest(beda))=0;root_begin(g);emit(g,a,copy_a);MEM_H(0,guest(beda))=1;
     require(root_begin(g,0x80EFFE00u)==0 && w(g,a+4)==phys(copy_a+24) && w(g,prev+4)==phys(slot(1,0)) &&
        holds(g,slot(1,0),212,0x77000000),"a nested begin on the other pool reverts only the open outer table");
     require(root_end(g,0x80EFFE00u)==0 && root_end(g)==0 && w(g,ck+8)==0 && w(g,prev+4)==phys(slot(1,0)) &&
        holds(g,slot(1,0),212,0x77000000),"nested and outer ends never touch the completed pool");++groups;}
    {Rig g;setup(g,0,212);root_begin(g);const uint32_t a=base(0)+0x100;emit(g,a,copy_a);
     require(root_end(g,0x80EFFC00u)==0 && w(g,ck+8)==1 && w(g,a+4)==phys(slot(0,0)),
        "an end from another owner frame neither publishes nor closes");
     fill(g,copy_a+24,212,0x88000000);
     require(root_end(g)==0 && w(g,ck+8)==0 && holds(g,slot(0,0),212,0x88000000),"the owning frame's end publishes and closes");++groups;}
    {Rig g;setup(g,0,212);auto* rdram=g.ram.data();root_begin(g);const uint32_t a=base(0)+0x100,b=base(0)+0x200;
     put_byte(rdram,part+10,0);emit(g,a,copy_a);
     require(release(g,record)==0 && w(g,entry(0,0)+8)==2,"a manager release of a never-redirected copy keeps its original disposition");
     put_byte(rdram,part+10,1);emit(g,b,copy_a);
     require(w(g,a+4)==phys(copy_a+24) && w(g,b+4)==phys(copy_a+24) && w(g,entry(0,0)+8)==2,
        "an address first used raw is never promoted later in the root");++groups;}
    {Rig g;setup(g,0,212);root_begin(g);const uint32_t b=base(0)+0x200;
     require(release(g,record)==0 && w(g,ck+48)==2 && w(g,entry(0,0)+8)==2 && w(g,entry(0,1)+8)==2,
        "an open-construction release holds both released addresses original");
     emit(g,b,copy_b);require(w(g,b+4)==phys(copy_b+24),"a copy later allocated at a released address stays original");++groups;}
    return groups;
}
// P04a: actual extracted CPU625 begin/admit/end/retire with the layout-2 ckcopy helpers. Row
// admission (FA12 resource AND state AND scale), exact physical and camera costs, the storage
// predicate (reason 17), the public NoInterpolate placeholder and S3 validate/publish/promote.
// Synthetic roster, copy manager, model graph, camera production and main pool; the two
// copy-manager getters are doubles. RT64's G_EX_ID_IGNORE handling is a pinned source fact.
uint32_t c625_copy_result=0,c625_ac4_result=0;
unsigned p04a_fixtures() {
    constexpr uint32_t ck=TOOIE_CKCOPY_STATE,entry0=ck+80,slot0=ck+80+2*64*96;
    constexpr uint32_t cs=TOOIE_CPU625_STATE,idbase0=cs+16+16,reason=cs+460;
    constexpr uint32_t staged=0x8012C964,beda=0x8007BEDA,pools=0x8007BCD0,roster=0x80135490,freelist=0x80135A60;
    constexpr uint32_t owner=0x80224000,part=0x80225000,pool=0x80226000,copy_a=0x80228000,copy_b=0x8022A000;
    constexpr uint32_t record=pool+16,other=0x8022C000,model=0x80230000,table=0x80240000,pose=0x80244000;
    constexpr uint32_t ac4=0x80246000,camera=0x80250000,matrix=0x80252000,buffers=0x80127658,phase_word=0x80132E9C;
    constexpr uint32_t cam=TOOIE_CAMERA_STATE_VRAM,cc=cam+offsetof(TooieCameraState,control);
    constexpr uint32_t sv=cam+offsetof(TooieCameraState,saved),r0=cam+offsetof(TooieCameraState,camera);
    constexpr uint32_t extra=cc+offsetof(TooieCameraControl,main_extra),depth=state+offsetof(TooieRenderBridgeState,silo_depth);
    constexpr uint32_t main0=TOOIE_CAMERA_MAIN_VRAM,main_end=main0+TOOIE_CAMERA_M_BYTES;
    constexpr uint32_t frame=0x80EFFF00u,placeholder=0x00000001u,root_policy=0x02014AA9u,id0=0xB1000001u;
    static_assert(TOOIE_CKCOPY_STATE_BYTES==122960u && TOOIE_CPU625_STATE_BYTES==464u,"layout-2 storage and CPU625 state ABI");
    struct RowFacts { uint32_t resource,state,bytes,geometry,vertex,bone,deform,poses,vertices,remaining; };
    constexpr RowFacts rows[4]={{0x625,3,9992,9776,5432,5152,3056,17,216,112},{0x60A,4,9432,9192,4928,4648,2520,17,212,136},
                                {0x61B,5,9408,9168,4904,4640,2520,16,212,136},{0x831,6,9384,9144,4880,4632,2520,15,212,136}};
    struct Rig { std::vector<uint8_t> ram=std::vector<uint8_t>(16*1024*1024); };
    auto entry=[&](uint32_t i){return entry0+i*96;};
    auto slot=[&](uint32_t s){return slot0+s*3456;};
    auto phys=[](uint32_t a){return a&0x1FFFFFFFu;};
    auto w=[](Rig& g,uint32_t a){return word(g.ram.data(),a);};
    auto call=[](Rig& g,recomp_func_t* fn,uint32_t a0,uint32_t a1=0,uint32_t a2=0,uint32_t a3=0){
        auto c=context();c.r4=guest(a0);c.r5=guest(a1);c.r6=guest(a2);c.r7=guest(a3);const auto before=c;
        fn(g.ram.data(),&c);saved(c,before);return low(c.r2);
    };
    auto fill=[](Rig& g,uint32_t at,uint32_t count,uint32_t tag){for(uint32_t i=0;i<16*count;i+=4)put(g.ram.data(),at+i,tag+i);};
    auto holds=[&](Rig& g,uint32_t at,uint32_t count,uint32_t tag){bool ok=true;for(uint32_t i=0;i<16*count;i+=4)ok=ok&&w(g,at+i)==tag+i;return ok;};
    auto group=[&](Rig& g,uint32_t at,uint32_t id,uint32_t policy){
        return w(g,at)==0x6400000C && w(g,at+4)==id && w(g,at+8)==policy && w(g,at+12)==0;
    };
    auto untouched=[&](Rig& g,uint32_t at){
        bool ok=w(g,buffers)==at;for(uint32_t a=at;a<at+32;a+=4)ok=ok&&w(g,a)==0xA5A5A5A5;return ok;
    };
    auto setup=[&](Rig& g,uint32_t r){
        auto* rdram=g.ram.data();const RowFacts& f=rows[r];
        put(rdram,ck,0x434B4350);put(rdram,ck+4,2);put(rdram,cs,0x36323531);put(rdram,cs+4,1);
        put(rdram,roster,owner);put(rdram,owner+0x184,0);put(rdram,0x801354DC,0);put(rdram,owner+20,part);
        put(rdram,part,f.resource);put_byte(rdram,part+8,1);put_byte(rdram,part+10,1);
        put(rdram,part+12,f.state);put(rdram,part+16,0x3DCCCCCD);
        put(rdram,freelist,pool);MEM_H(0,guest(pool))=12;MEM_H(2,guest(pool))=4;
        put(rdram,record,copy_a);put(rdram,record+4,copy_b);put_byte(rdram,record+9,1);
        MEM_H(22,guest(copy_a))=int16_t(2*f.vertices);MEM_H(22,guest(copy_b))=int16_t(2*f.vertices);
        put(rdram,pools,main0);put(rdram,pools+4,main_end);MEM_H(0,guest(beda))=0;
        put(rdram,model+4,f.geometry);MEM_H(8,guest(model))=80;put(rdram,model+12,96);
        put(rdram,model+16,f.vertex);put(rdram,model+24,f.bone);put(rdram,model+40,f.deform);
        MEM_H(4,guest(model+f.bone))=int16_t(f.poses);
        uint32_t node=model+f.geometry;
        if(r==0){put(rdram,node,10);put(rdram,node+4,24);node+=24;}
        for(uint32_t i=0,n=r?15:12;i<n;++i,node+=16){put(rdram,node,17);put(rdram,node+4,i+1<n?16:0);}
        put(rdram,0x8012CF8C,table);put(rdram,0x80123198,table);put(rdram,table+40,0x800DD410);put(rdram,table+68,0x800DD65C);
        put(rdram,0x8012C95C,model+80);put(rdram,0x8012C960,model+96);put(rdram,0x8012C968,pose);put(rdram,0x8012C974,ac4);
        put(rdram,pose,f.poses);put(rdram,pose+4,f.poses);c625_copy_result=copy_a;c625_ac4_result=ac4;
        put(rdram,0x8012D500,camera);put(rdram,0x8012D558,matrix);put(rdram,0x80127674,0);
        put(rdram,cam+offsetof(TooieCameraState,owner_frame),stack);
        put(rdram,cc+offsetof(TooieCameraControl,epoch),7);put(rdram,cc+offsetof(TooieCameraControl,active),1);
        put(rdram,cc+offsetof(TooieCameraControl,main_base),main0);put(rdram,cc+offsetof(TooieCameraControl,main_begin),main0);
        put(rdram,cc+offsetof(TooieCameraControl,main_end),main_end);put(rdram,extra,8);
        put(rdram,sv+offsetof(TooieCameraProduction,valid),1);put(rdram,sv+offsetof(TooieCameraProduction,epoch),7);
        put(rdram,sv+offsetof(TooieCameraProduction,matrix),matrix);put(rdram,sv+offsetof(TooieCameraProduction,camera),camera);
        put(rdram,sv+offsetof(TooieCameraProduction,birth),3);put(rdram,sv+offsetof(TooieCameraProduction,scale),0x3F800000);
        put(rdram,sv+offsetof(TooieCameraProduction,id),TOOIE_CAMERA_ID_FIRST);put(rdram,sv+offsetof(TooieCameraProduction,policy),0x02008AAA);
        put(rdram,sv+offsetof(TooieCameraProduction,role),TOOIE_CAMERA_WORLD_INITIAL);
        put(rdram,r0+offsetof(TooieCameraRecord,pointer),camera);put(rdram,r0+offsetof(TooieCameraRecord,birth),3);
        put(rdram,r0+offsetof(TooieCameraRecord,valid),1);put(rdram,r0+offsetof(TooieCameraRecord,view_id),TOOIE_CAMERA_ID_FIRST);
        put(rdram,r0+offsetof(TooieCameraRecord,view_scale),0x3F800000);put(rdram,state+offsetof(TooieRenderBridgeState,epoch),7);
    };
    auto open=[&](Rig& g){require(call(g,candidate::tooie_ckcopy_root_begin,frame)==0,"S1 opens the construction");};
    auto finish=[&](Rig& g,uint32_t f=frame){return call(g,candidate::tooie_ckcopy_root_end,f);};
    auto none=[](Rig&){};
    // One draw3 occurrence in original order: begin (808003FC), DE498's S2 segment word at cmd,
    // optional nested work, DEFA0 admit at current, the row's remaining original bytes and the
    // paid close (80800404). Returns the CPU625 last_reason.
    auto occurrence=[&](Rig& g,uint32_t r,uint32_t cmd,uint32_t current,uint32_t phase,bool segment,auto between){
        auto* rdram=g.ram.data();const uint32_t copy=c625_copy_result;
        for(uint32_t a=current;a<current+32;a+=4)put(rdram,a,0xA5A5A5A5);
        MEM_H(0,guest(phase_word))=int16_t(phase);put(rdram,staged,copy);put(rdram,depth,0);
        require(call(g,candidate::tooie_cpu625_begin,owner,model)==0,"CPU625 begin returns");
        put(rdram,depth,1);
        if(segment){
            put(rdram,cmd,0xDB060004);put(rdram,cmd+4,phys(copy+24));
            require(call(g,candidate::tooie_ckcopy_segment,cmd)==0,"S2 returns");
        }
        between(g);put(rdram,buffers,current);
        // a3: this DE498's own segment command, as the native admit transports it from sp+0xB8.
        require(call(g,candidate::tooie_cpu625_admit,buffers,buffers,model,cmd)==0,"CPU625 admit returns");
        if(w(g,buffers)==current+24)put(rdram,buffers,current+24+rows[r].remaining);
        require(call(g,candidate::tooie_cpu625_end,owner)==0,"CPU625 end returns");
        return w(g,reason);
    };
    unsigned groups=0;
    for(uint32_t r=1;r<4;++r){
        Rig g;setup(g,r);open(g);const uint32_t a=main0+0x100,at=main0+0x200,tail=at+24+rows[r].remaining;
        require(occurrence(g,r,a,at,0,true,none)==0 && w(g,at)==0xE0525464 && w(g,at+4)==0x10000064 &&
            group(g,at+8,0,placeholder) && w(g,tail)==0x6400000D && w(g,tail+4)==1 && w(g,buffers)==tail+8 &&
            w(g,extra)==40,"a new row admits at its exact costs and emits the public NoInterpolate placeholder");
        require(w(g,a+4)==phys(slot(0)) && w(g,entry(0)+52)==1 && w(g,entry(0)+56)==at+8 &&
            w(g,entry(0)+60)==id0 && w(g,ck+64)==0 && w(g,ck+68)==0,
            "the placeholder registers to the LIVE copy whose redirect this DEFA0 consumed");
        fill(g,copy_a+24,212,0x10000000*r);
        require(finish(g)==0 && group(g,at+8,id0,root_policy) && holds(g,slot(0),212,0x10000000*r) &&
            w(g,ck+72)==1 && w(g,ck+8)==0,"S3 publishes the slot, then promotes the label to its CPU625 ID and root policy");
        ++groups;
    }
    {Rig g;setup(g,0);open(g);const uint32_t a=main0+0x100,at=main0+0x200;
     require(occurrence(g,0,a,at,0,true,none)==0 && w(g,at)==0xE0525464 && w(g,at+4)==0x10000064 &&
        group(g,at+8,id0,root_policy) && w(g,at+136)==0x6400000D && w(g,at+140)==1 && w(g,buffers)==at+144 &&
        w(g,a+4)==phys(slot(0)) && w(g,entry(0)+52)==0,"0625 keeps its accepted words and 144 bytes and registers no label");
     require(finish(g)==0 && group(g,at+8,id0,root_policy) && w(g,ck+72)==0,"S3 leaves the 0625 group unchanged");
     Rig h;setup(h,0);
     require(occurrence(h,0,a,at,0,false,none)==0 && group(h,at+8,id0,root_policy),
        "0625 has no storage predicate: admitted without an open construction or redirect");
     ++groups;}
    {const uint32_t bad[4][3]={{0x625,4,0x3DCCCCCD},{0x60A,3,0x3DCCCCCD},{0x831,5,0x3DCCCCCD},{0x60A,4,0x3F800000}};
     for(const auto& b:bad){
         Rig g;setup(g,1);open(g);auto* rdram=g.ram.data();
         put(rdram,part,b[0]);put(rdram,part+12,b[1]);put(rdram,part+16,b[2]);
         occurrence(g,1,main0+0x100,main0+0x200,0,true,none);
         require(untouched(g,main0+0x200) && w(g,entry(0)+52)==0,
             "FA12: resource, state and scale select a row together; a retained binding selects none");
     }
     Rig g;setup(g,1);open(g);
     require(occurrence(g,1,main0+0x100,main0+0x200,0,true,[&](Rig& h){put(h.ram.data(),part,0x625);})==7 &&
        untouched(g,main0+0x200),"a binding change inside the bracket refuses at admission");
     ++groups;}
    {Rig g;setup(g,1);open(g);const uint32_t at=main_end-168;put(g.ram.data(),extra,6320);
     require(occurrence(g,1,main0+0x100,at,0,true,none)==0 && group(g,at+8,0,placeholder) &&
        w(g,main_end-8)==0x6400000D && w(g,buffers)==main_end && w(g,extra)==6352,
        "168 physical bytes and the 32+48 camera ledger admit exactly at their limits");
     require(finish(g)==0 && group(g,at+8,id0,root_policy),"a label at the list end promotes");
     Rig h;setup(h,1);open(h);
     require(occurrence(h,1,main0+0x100,main_end-160,0,true,none)==11 && untouched(h,main_end-160),
        "160 bytes of room refuse a 168-byte row");
     Rig k;setup(k,1);open(k);put(k.ram.data(),extra,6321);
     require(occurrence(k,1,main0+0x100,main0+0x200,0,true,none)==11 && untouched(k,main0+0x200),
        "the camera ledger keeps its 32-byte debit and 48-byte margin");
     ++groups;}
    for(int which=0;which<4;++which){
        Rig g;setup(g,1);open(g);auto* rdram=g.ram.data();
        if(which==0)put(rdram,model+rows[1].geometry,10);
        if(which==1)put(rdram,model+16,rows[1].vertex+8);
        if(which==2)put(rdram,pose,16);
        if(which==3)MEM_H(22,guest(copy_a))=432;
        require(occurrence(g,1,main0+0x100,main0+0x200,0,true,none)==(which==2?9u:8u) &&
            untouched(g,main0+0x200) && w(g,entry(0)+52)==0,
            "the new-row graph (type10 refused), header, pose count and copy extent are exact");
    }
    ++groups;
    for(int which=0;which<8;++which){
        Rig g;setup(g,1);auto* rdram=g.ram.data();const uint32_t at=main0+0x200,cmd=which==6?at+0x100:main0+0x100;
        if(which!=0)open(g);
        if(which==4)require(call(g,candidate::tooie_ckcopy_release,record)==0,"S4 holds both addresses");
        if(which==5)require(call(g,candidate::tooie_ckcopy_root_begin,frame-0x100)==0,"a nested S1 disqualifies");
        auto between=[&](Rig& h){
            if(which==2){
                put(rdram,staged,copy_b);put(rdram,main0+0x140,0xDB060004);put(rdram,main0+0x144,phys(copy_b+24));
                call(h,candidate::tooie_ckcopy_segment,main0+0x140);put(rdram,staged,copy_a);
            }
            if(which==3)call(h,candidate::tooie_cpu625_admit,buffers,buffers,model+0x40,main0+0x140);
            // 7: same-copy nesting with no nested DEFA0. The latest redirect is the nested draw's
            // command, not this draw's own (sp+0xB8), so the outer admission must refuse.
            if(which==7){put(rdram,main0+0x140,0xDB060004);put(rdram,main0+0x144,phys(copy_a+24));
                call(h,candidate::tooie_ckcopy_segment,main0+0x140);}
        };
        require(occurrence(g,1,cmd,at,0,which!=1,between)==17 && untouched(g,at) &&
            w(g,entry(0)+52)==0 && w(g,entry(1)+52)==0 && (which!=7 || w(g,entry(0)+16)==2),
            "a new row is admitted only for this draw's own LIVE redirect in the open construction");
    }
    ++groups;
    for(int which=0;which<6;++which){
        Rig g;setup(g,1);open(g);auto* rdram=g.ram.data();const uint32_t a=main0+0x100,at=main0+0x200;
        require(occurrence(g,1,a,at,0,true,none)==0 && group(g,at+8,0,placeholder) && w(g,entry(0)+52)==1,
            "an admitted, registered placeholder");
        fill(g,slot(0),212,0x5A000000);fill(g,copy_a+24,212,0x6B000000);
        if(which==0)require(call(g,candidate::tooie_ckcopy_release,record)==0,"S4 release reverts");
        if(which==1){
            MEM_H(22,guest(copy_a))=432;put(rdram,main0+0x400,0xDB060004);put(rdram,main0+0x404,phys(copy_a+24));
            call(g,candidate::tooie_ckcopy_segment,main0+0x400);
        }
        if(which==2)for(uint32_t i=0;i<8;++i){
            const uint32_t c=main0+0x400+8*i;put(rdram,c,0xDB060004);put(rdram,c+4,phys(copy_a+24));
            call(g,candidate::tooie_ckcopy_segment,c);
        }
        if(which==3){call(g,candidate::tooie_ckcopy_root_begin,frame-0x100);finish(g,frame-0x100);}
        if(which==4)require(finish(g,frame-0x200)==0 && w(g,ck+8)==1 && group(g,at+8,0,placeholder),
            "an end from another frame neither publishes nor promotes");
        if(which==5)put(rdram,record,other);
        require(finish(g)==0 && w(g,ck+8)==0,"the owning frame's end closes");
        if(which==4)require(group(g,at+8,id0,root_policy) && holds(g,slot(0),212,0x6B000000) && w(g,ck+72)==1,
            "the owning frame's end publishes and promotes");
        else require(group(g,at+8,0,placeholder) && holds(g,slot(0),212,0x5A000000) && w(g,ck+72)==0 &&
            w(g,a+4)==phys(copy_a+24),"a reverted or no-longer-live copy keeps its placeholder; nothing is published or promoted");
    }
    ++groups;
    {Rig g;setup(g,1);open(g);const uint32_t phases[5]={0,1,2,5,6};
     for(uint32_t k=0;k<5;++k){
         const uint32_t at=main0+0x400+0x200*k;
         require(occurrence(g,1,at-0x10,at,phases[k],true,none)==0 && group(g,at+8,0,placeholder),
             "five admitted placeholders of one copy");
     }
     require(w(g,entry(0)+16)==5 && w(g,entry(0)+52)==4 && w(g,ck+76)==1,
        "four labels register; the fifth is a counted placeholder");
     fill(g,copy_a+24,212,0x7C000000);
     require(finish(g)==0 && w(g,ck+72)==4 && holds(g,slot(0),212,0x7C000000),"S3 publishes once and promotes four");
     bool promoted=true;
     for(uint32_t k=0;k<4;++k)promoted=promoted&&group(g,main0+0x408+0x200*k,id0+16*phases[k],root_policy);
     require(promoted && group(g,main0+0x408+0x200*4,0,placeholder),"the unregistered fifth occurrence is never promoted");
     ++groups;}
    for(int which=0;which<6;++which){
        Rig g;setup(g,1);open(g);auto* rdram=g.ram.data();const uint32_t at_a=main0+0x400,at_b=main0+0x600;
        require(occurrence(g,1,at_a-0x10,at_a,0,true,none)==0,"copy A admitted");
        c625_copy_result=copy_b;
        require(occurrence(g,1,at_b-0x10,at_b,1,true,none)==0 && w(g,entry(1)+52)==1 && w(g,entry(1)+60)==id0+16,
            "copy B admitted in another phase");
        fill(g,slot(0),212,0x5A000000);fill(g,slot(1),212,0x5B000000);
        fill(g,copy_a+24,212,0x11000000);fill(g,copy_b+24,212,0x22000000);
        if(which<4)put(rdram,at_b+8+4*which,w(g,at_b+8+4*which)^0x100);
        if(which==4)put(rdram,entry(1)+60,0);
        if(which==5){
            put(rdram,entry(1)+56,main_end-8);put(rdram,main_end-8,0x6400000C);put(rdram,main_end-4,0);
            put(rdram,main_end,placeholder);put(rdram,main_end+4,0);
        }
        require(finish(g)==1 && w(g,ck+40)==1 && w(g,ck+72)==0 && group(g,at_a+8,0,placeholder) &&
            holds(g,slot(0),212,0x5A000000) && holds(g,slot(1),212,0x5B000000),
            "any invalid label word, ID or 16-byte range faults before any slot is published or label promoted");
        require(call(g,candidate::tooie_ckcopy_segment,at_a-0x10)==1,"the fault latch rejects the construction");
    }
    ++groups;
    {Rig g;setup(g,1);open(g);auto* rdram=g.ram.data();
     require(occurrence(g,1,main0+0x100,main0+0x200,0,true,none)==0 && w(g,idbase0)==0xB1000000,"a row identity block");
     require(call(g,candidate::tooie_cpu625_retire,owner,1,1)==0 && w(g,idbase0)==0,"leaving state 4 ends the identity");
     put(rdram,idbase0,0xB1000000);put(rdram,part+12,1);
     require(call(g,candidate::tooie_cpu625_retire,owner,5,1)==0 && w(g,idbase0)==0,"entering state 5 ends the identity");
     put(rdram,idbase0,0xB1000000);
     require(call(g,candidate::tooie_cpu625_retire,owner,2,1)==0 && w(g,idbase0)==0xB1000000,
        "a transition outside states 3..6 keeps it");
     ++groups;}
    return groups;
}
#ifndef TOOIE_REQUIRED_STOP_FIXTURE
#error N3 fixtures need the selected shadow layout (TOOIE_SHADOW_STATE)
#endif
// N3: actual extracted tooie_shadow_prepare (provisional whole-composite gate and its 24-byte
// reservation) and CPU625 attach_begin / admit (exclusive dispatch) / attach_end, with the
// layout-2 ckcopy helpers and the draw3 begin/admit/end for the shared identity. MODELED: the
// silhouette DE498's 480 group word (shadow_pending.group) and the attachment's original
// remaining bytes. DOUBLED: the copy getters. Not claimed: RT64 consumption or visuals.
unsigned n3_fixtures() {
    constexpr uint32_t ck=TOOIE_CKCOPY_STATE,entry0=ck+80,cs=TOOIE_CPU625_STATE,cx=cs+400,own0=cs+16;
    constexpr uint32_t sh=TOOIE_SHADOW_STATE,pcb=state+offsetof(TooieRenderBridgeState,player);
    constexpr uint32_t staged=0x8012C964,roster=0x80135490,freelist=0x80135A60,root_mode=0x8012C9B8;
    constexpr uint32_t owner=0x80224000,part=0x80225000,pool=0x80226000,copy_a=0x80228000,copy_b=0x8022A000;
    constexpr uint32_t record=pool+16,model=0x80230000,table=0x80240000,pose=0x80244000;
    constexpr uint32_t component=0x80226800,type_component=0x80226A00,receiver=0x80226C00,silhouette=0x80233000;
    constexpr uint32_t camera=0x80250000,matrix=0x80252000,buffers=0x80127658,phase_word=0x80132E9C;
    constexpr uint32_t cam=TOOIE_CAMERA_STATE_VRAM,cc=cam+offsetof(TooieCameraState,control);
    constexpr uint32_t sv=cam+offsetof(TooieCameraState,saved),r0=cam+offsetof(TooieCameraState,camera);
    constexpr uint32_t extra=cc+offsetof(TooieCameraControl,main_extra),depth=state+offsetof(TooieRenderBridgeState,silo_depth);
    constexpr uint32_t main0=TOOIE_CAMERA_MAIN_VRAM,frame=0x80EFFF00u,a6250=0x80EFF800u;
    constexpr uint32_t pid=0x90000000u,kind=0x41540000u,root_policy=0x02014AA9u,attach_id=0xB1000052u;
    constexpr uint32_t bridge=state+offsetof(TooieRenderBridgeState,extra_bytes);
    struct RowFacts { uint32_t resource,state,geometry,vertex,bone,deform,poses,vertices,remaining; };
    constexpr RowFacts rows[4]={{0x625,3,9776,5432,5152,3056,17,216,112},{0x60A,4,9192,4928,4648,2520,17,212,136},
                                {0x61B,5,9168,4904,4640,2520,16,212,136},{0x831,6,9144,4880,4632,2520,15,212,136}};
    struct Rig { std::vector<uint8_t> ram=std::vector<uint8_t>(16*1024*1024); };
    auto w=[](Rig& g,uint32_t a){return word(g.ram.data(),a);};
    auto entry=[&](uint32_t i){return entry0+i*96;};
    auto phys=[](uint32_t a){return a&0x1FFFFFFFu;};
    auto call=[](Rig& g,recomp_func_t* fn,uint32_t a0,uint32_t a1=0,uint32_t a2=0,uint32_t a3=0){
        auto c=context();c.r4=guest(a0);c.r5=guest(a1);c.r6=guest(a2);c.r7=guest(a3);const auto before=c;
        fn(g.ram.data(),&c);saved(c,before);return low(c.r2);
    };
    // PC[8]: the shadow/attachment consumers read the selected primary entry [slot0][view0], so
    // every retained-player field set here is mirrored into that entry.
    auto pcf=[&](Rig& g,uint32_t offset,uint32_t value){put(g.ram.data(),pcb+offset,value);
        const int32_t map=offset==offsetof(TooiePlayerBoundary,owner)?0:offset==offsetof(TooiePlayerBoundary,component)?4:
            offset==offsetof(TooiePlayerBoundary,model)?8:offset==offsetof(TooiePlayerBoundary,selector_handle)?12:
            offset==offsetof(TooiePlayerBoundary,effective_model_asset)?16:offset==offsetof(TooiePlayerBoundary,view_phase)?20:
            offset==offsetof(TooiePlayerBoundary,idbase)?24:offset==offsetof(TooiePlayerBoundary,stamp)?28:-1;
        if(map>=0)put(g.ram.data(),state+offsetof(TooieRenderBridgeState,primary)+uint32_t(map),value);};
    auto setup=[&](Rig& g,uint32_t r){
        auto* rdram=g.ram.data();const RowFacts& f=rows[r];
        put(rdram,ck,0x434B4350);put(rdram,ck+4,2);put(rdram,cs,0x36323531);put(rdram,cs+4,1);
        put(rdram,roster,owner);put(rdram,owner+0x184,0);put(rdram,0x801354DC,0);put(rdram,owner+20,part);
        put(rdram,part,f.resource);put_byte(rdram,part+8,1);put_byte(rdram,part+10,1);
        put(rdram,part+12,f.state);put(rdram,part+16,0x3DCCCCCD);
        put(rdram,freelist,pool);MEM_H(0,guest(pool))=12;MEM_H(2,guest(pool))=4;
        put(rdram,record,copy_a);put(rdram,record+4,copy_b);put_byte(rdram,record+9,1);
        MEM_H(22,guest(copy_a))=int16_t(2*f.vertices);MEM_H(22,guest(copy_b))=int16_t(2*f.vertices);
        put(rdram,0x8007BCD0,main0);put(rdram,0x8007BCD4,main0+TOOIE_CAMERA_M_BYTES);MEM_H(0,guest(0x8007BEDA))=0;
        put(rdram,model+4,f.geometry);MEM_H(8,guest(model))=80;put(rdram,model+12,96);
        put(rdram,model+16,f.vertex);put(rdram,model+24,f.bone);put(rdram,model+40,f.deform);
        MEM_H(4,guest(model+f.bone))=int16_t(f.poses);
        uint32_t node=model+f.geometry;
        if(r==0){put(rdram,node,10);put(rdram,node+4,24);node+=24;}
        for(uint32_t i=0,n=r?15:12;i<n;++i,node+=16){put(rdram,node,17);put(rdram,node+4,i+1<n?16:0);}
        put(rdram,0x8012CF8C,table);put(rdram,0x80123198,table);put(rdram,table+40,0x800DD410);put(rdram,table+68,0x800DD65C);
        // F7: the finite ckpack constructor stores a zero secondary index (D24 auxiliary 0), so
        // AC4(handle) is 0; DE318 cleared C974 after the silhouette draw and entry2 never restages
        // it. The reachable source conjunction is C974 == AC4 == 0 (no invented auxiliary).
        put(rdram,0x8012C95C,model+80);put(rdram,0x8012C960,model+96);put(rdram,0x8012C968,pose);put(rdram,0x8012C974,0);
        put(rdram,pose,f.poses);put(rdram,pose+4,f.poses);c625_copy_result=copy_a;c625_ac4_result=0;
        put(rdram,root_mode,3); // the shadow route stages root mode 3 (DF540) before A66BC
        put(rdram,0x8012D500,camera);put(rdram,0x8012D558,matrix);put(rdram,0x80127674,0);
        MEM_H(0,guest(phase_word))=5; // phase-5 silhouette (F4)
        put(rdram,cam+offsetof(TooieCameraState,owner_frame),stack);
        put(rdram,cc+offsetof(TooieCameraControl,epoch),7);put(rdram,cc+offsetof(TooieCameraControl,active),1);
        put(rdram,cc+offsetof(TooieCameraControl,main_base),main0);put(rdram,cc+offsetof(TooieCameraControl,main_begin),main0);
        put(rdram,cc+offsetof(TooieCameraControl,main_end),main0+TOOIE_CAMERA_M_BYTES);put(rdram,extra,8);
        put(rdram,sv+offsetof(TooieCameraProduction,valid),1);put(rdram,sv+offsetof(TooieCameraProduction,epoch),7);
        put(rdram,sv+offsetof(TooieCameraProduction,matrix),matrix);put(rdram,sv+offsetof(TooieCameraProduction,camera),camera);
        put(rdram,sv+offsetof(TooieCameraProduction,birth),3);put(rdram,sv+offsetof(TooieCameraProduction,scale),0x3F800000);
        put(rdram,sv+offsetof(TooieCameraProduction,id),TOOIE_CAMERA_ID_FIRST);put(rdram,sv+offsetof(TooieCameraProduction,policy),0x02008AAA);
        put(rdram,sv+offsetof(TooieCameraProduction,role),TOOIE_CAMERA_WORLD_INITIAL);
        put(rdram,r0+offsetof(TooieCameraRecord,pointer),camera);put(rdram,r0+offsetof(TooieCameraRecord,birth),3);
        put(rdram,r0+offsetof(TooieCameraRecord,valid),1);put(rdram,r0+offsetof(TooieCameraRecord,view_id),TOOIE_CAMERA_ID_FIRST);
        put(rdram,r0+offsetof(TooieCameraRecord,view_scale),0x3F800000);
        put(rdram,state+offsetof(TooieRenderBridgeState,epoch),7);put(rdram,state+offsetof(TooieRenderBridgeState,seen_epoch),7);
        // Retained controlled player and its fallback-shadow premises (shadow_projection.c:68-104).
        put(rdram,owner+0x50,component);put(rdram,owner+0xA0,type_component);put_byte(rdram,type_component+0x29,1);
        MEM_H(0,guest(component))=1;MEM_H(0xC,guest(component))=0x62A;put_byte(rdram,component+0x11,1);
        MEM_H(0x76,guest(receiver))=0;MEM_H(0xA,guest(silhouette))=0x40;
        pcf(g,offsetof(TooiePlayerBoundary,enabled),1);pcf(g,offsetof(TooiePlayerBoundary,idbase),pid);
        pcf(g,offsetof(TooiePlayerBoundary,stamp),7);pcf(g,offsetof(TooiePlayerBoundary,owner),owner);
        pcf(g,offsetof(TooiePlayerBoundary,slot),0);pcf(g,offsetof(TooiePlayerBoundary,component),component);
        pcf(g,offsetof(TooiePlayerBoundary,selector_handle),(0x62Au<<16)|1u);
        pcf(g,offsetof(TooiePlayerBoundary,effective_model_asset),0x62A);
        pcf(g,offsetof(TooiePlayerBoundary,view_phase),0u|(5u<<2));
        put(rdram,sh,0x53484403);put(rdram,sh+52,0x800A6D60);put(rdram,buffers,main0+0x100);
        put(rdram,ck+8,0); // no open ckcopy root unless a group opens it
    };
    auto prepare=[&](Rig& g){return call(g,candidate::tooie_shadow_prepare,owner,receiver,silhouette);};
    auto open=[&](Rig& g){require(call(g,candidate::tooie_ckcopy_root_begin,frame)==0,"S1 opens the construction");};
    auto finish=[&](Rig& g){return call(g,candidate::tooie_ckcopy_root_end,frame);};
    auto group=[&](Rig& g,uint32_t at,uint32_t id,uint32_t policy){
        return w(g,at)==0x6400000C && w(g,at+4)==id && w(g,at+8)==policy && w(g,at+12)==0;};
    // A6250 after a provisional prepare: the silhouette DE498 emitted its 480 group (MODELED), then
    // A66BC begin, the direct DE448 -> DE498 (S2 at cmd, DEFA0 at current), remaining bytes, A66C4.
    auto attachment=[&](Rig& g,uint32_t r,uint32_t cmd,uint32_t current,bool segment,auto between){
        auto* rdram=g.ram.data();
        put(rdram,sh+20,main0+0x80);
        for(uint32_t a=current;a<current+32;a+=4)put(rdram,a,0xA5A5A5A5);
        put(rdram,staged,c625_copy_result);put(rdram,depth,0);
        require(call(g,candidate::tooie_cpu625_attach_begin,owner,model,a6250)==0,"attachment begin returns");
        put(rdram,depth,1);
        if(segment){put(rdram,cmd,0xDB060004);put(rdram,cmd+4,phys(c625_copy_result+24));
            require(call(g,candidate::tooie_ckcopy_segment,cmd)==0,"S2 returns");}
        between(g);put(rdram,buffers,current);
        require(call(g,candidate::tooie_cpu625_admit,buffers,buffers,model,cmd)==0,"attachment DEFA0 returns");
        if(w(g,buffers)==current+16)put(rdram,buffers,current+16+rows[r].remaining);
        put(rdram,depth,0);
        require(call(g,candidate::tooie_cpu625_attach_end,owner,a6250)==0,"attachment end returns");
        return w(g,cs+460);
    };
    auto none=[](Rig&){};
    unsigned groups=0;
    // 1. Whole-composite gate (actual prepare): four rows reserve 24 camera-ledger bytes and the
    // context; any other attachment refuses the composite with nothing debited; limits exact.
    for(uint32_t r=0;r<4;++r){
        Rig g;setup(g,r);
        require(prepare(g)==pid+480 && w(g,extra)==8+24 && w(g,bridge)==984 && w(g,cx)==owner &&
            w(g,cx+16)==7 && w(g,cx+40)==kind && w(g,cx+44)==3,"a qualifying attachment reserves its bytes with the silhouette");
    }
    {const uint32_t bad[3][3]={{0x621,2,0x3DCCCCCD},{0x61C,7,0x3DCCCCCD},{0x60A,4,0x3F800000}};
     for(const auto& b:bad){
         Rig g;setup(g,1);put(g.ram.data(),part,b[0]);put(g.ram.data(),part+12,b[1]);put(g.ram.data(),part+16,b[2]);
         require(prepare(g)==0 && w(g,extra)==8 && w(g,bridge)==0 && w(g,cx)==0,"every other attachment refuses the whole composite");
     }
     Rig h;setup(h,1);put(h.ram.data(),extra,6400-56);
     require(prepare(h)==pid+480 && w(h,extra)==6400-32,"the reservation admits at its exact limit");
     Rig k;setup(k,1);put(k.ram.data(),extra,6400-55);
     require(prepare(k)==0 && w(k,extra)==6400-55,"one byte short refuses the composite");
     Rig m;setup(m,1);put(m.ram.data(),cx,0x80224400);
     require(prepare(m)==0,"an occupied CPU625 context refuses the composite");
     Rig n;setup(n,1);put_byte(n.ram.data(),component+0x11,0);
     require(prepare(n)==pid+480 && w(n,extra)==8 && w(n,cx)==0,"without an attachment the accepted prepare is unchanged");
     ++groups;}
    // 2. Registered placeholder and S3 promotion to role +2, for all four rows (0625 included).
    for(uint32_t r=0;r<4;++r){
        Rig g;setup(g,r);open(g);require(prepare(g)==pid+480,"prepared");const uint32_t at=main0+0x400;
        require(attachment(g,r,at-0x10,at,true,none)==0 && group(g,at,0,1) && w(g,entry(0)+52)==1 &&
            w(g,entry(0)+56)==at && w(g,entry(0)+60)==attach_id && w(g,at+16+rows[r].remaining)==0x6400000D &&
            w(g,cx)==0 && w(g,own0+16)==0xB1000000 && w(g,own0+28)==(1u|(2u<<5)) && w(g,own0+24)==0,
            "the attachment registers role +2 with its own flags bit and pays exactly one pop");
        require(finish(g)==0 && group(g,at,attach_id,root_policy),"S3 promotes the attachment after publishing its copy");
    }
    ++groups;
    // 3. 0625 has no attachment bypass: without this draw's LIVE redirect it stays unregistered.
    {Rig g;setup(g,0);require(prepare(g)==pid+480,"prepared");const uint32_t at=main0+0x400;
     require(attachment(g,0,at-0x10,at,true,none)==49 && group(g,at,0,1) && w(g,entry(0)+52)==0 &&
        w(g,at+16+112)==0x6400000D && w(g,own0+16)==0,"0625 without published storage keeps an unregistered placeholder");
     ++groups;}
    // 4. Late failures emit the reserved placeholder unregistered and never touch identity.
    for(int which=0;which<3;++which){
        Rig g;setup(g,1);open(g);require(prepare(g)==pid+480,"prepared");auto* rdram=g.ram.data();const uint32_t at=main0+0x400;
        if(which==0)put(rdram,root_mode,0);
        if(which==1)pcf(g,offsetof(TooiePlayerBoundary,view_phase),0);
        auto between=[&](Rig& h){if(which==2){put(rdram,main0+0x140,0xDB060004);put(rdram,main0+0x144,phys(copy_a+24));
            call(h,candidate::tooie_ckcopy_segment,main0+0x140);}};
        const uint32_t expected=which==0?47u:which==1?41u:49u;
        require(attachment(g,1,at-0x10,at,true,between)==expected && group(g,at,0,1) && w(g,entry(0)+52)==0 &&
            w(g,at+16+136)==0x6400000D && w(g,own0+16)==0 && w(g,cx)==0,
            "root mode, cell or another draw's redirect: explicit unregistered placeholder, paid pop, identity untouched");
    }
    ++groups;
    // 5. A foreign DEFA0 inside the bracket is ignored; the direct draw then admits.
    {Rig g;setup(g,1);open(g);require(prepare(g)==pid+480,"prepared");const uint32_t at=main0+0x400;
     auto between=[&](Rig& h){put(h.ram.data(),buffers,main0+0x300);
         call(h,candidate::tooie_cpu625_admit,buffers,buffers,model+0x40,0);
         require(w(h,buffers)==main0+0x300 && w(h,cx+44)==0,"foreign DEFA0 emits nothing and leaves the owner open");};
     require(attachment(g,1,at-0x10,at,true,between)==49 && group(g,at,0,1),
        "its consumption still refuses the outer storage predicate (no lending)");
     ++groups;}
    // 6. Broken MAIN premise: nothing emitted, counted drift, no pop.
    {Rig g;setup(g,1);open(g);require(prepare(g)==pid+480,"prepared");const uint32_t at=main0+0x400;
     auto between=[&](Rig& h){put(h.ram.data(),0x8007BCE8,1);};
     require(attachment(g,1,at-0x10,at,true,between)==40 && w(g,at)==0xA5A5A5A5 && w(g,buffers)==at &&
        w(g,cs+456)==1 && w(g,cx)==0,"a redirected list breaks the MAIN premise: no group, no pop, drift counted");
     ++groups;}
    // 7. Shared identity (F4/F6): attachment first, main draw3 later in the same epoch keeps both
    // bits; a same-epoch attachment repeat is refused; the next epoch resets the flags mask.
    {Rig g;setup(g,1);open(g);auto* rdram=g.ram.data();require(prepare(g)==pid+480,"prepared");
     require(attachment(g,1,main0+0x3F0,main0+0x400,true,none)==0,"attachment first");
     put(rdram,root_mode,0);put(rdram,staged,copy_a);put(rdram,depth,0);
     require(call(g,candidate::tooie_cpu625_begin,owner,model)==0,"draw3 begin");put(rdram,depth,1);
     put(rdram,main0+0x5F0,0xDB060004);put(rdram,main0+0x5F4,phys(copy_a+24));call(g,candidate::tooie_ckcopy_segment,main0+0x5F0);
     put(rdram,buffers,main0+0x600);
     require(call(g,candidate::tooie_cpu625_admit,buffers,buffers,model,main0+0x5F0)==0 && w(g,cs+460)==0,"draw3 admits");
     put(rdram,buffers,main0+0x600+24+136);put(rdram,depth,0);call(g,candidate::tooie_cpu625_end,owner);
     require(w(g,own0+16)==0xB1000000 && w(g,cs+8)==1 && w(g,own0+24)==(1u<<5) && w(g,own0+28)==(1u|(2u<<5)),
        "one block; main and attachment each hold only their own bit");
     // A second same-epoch attachment occurrence (MODELED reservation; the accepted prepare allows
     // one shadow per epoch) must not admit twice under the same cell.
     put(rdram,root_mode,3);
     put(rdram,cx,owner);put(rdram,cx+16,7);put(rdram,cx+40,kind);put(rdram,cx+44,3);put(rdram,sh+8,pid+480);put(rdram,sh+16,7);
     require(attachment(g,1,main0+0x7F0,main0+0x800,true,none)==12 && group(g,main0+0x800,0,1),
        "a same-epoch attachment repeat stays an unregistered placeholder");
     for(uint32_t a:{cc+offsetof(TooieCameraControl,epoch),sv+offsetof(TooieCameraProduction,epoch),
                     state+offsetof(TooieRenderBridgeState,epoch),state+offsetof(TooieRenderBridgeState,seen_epoch)})put(rdram,a,8);
     pcf(g,offsetof(TooiePlayerBoundary,stamp),8);put(rdram,sh+16,0);put(rdram,extra,8);
     require(prepare(g)==pid+480,"next epoch prepares");
     require(attachment(g,1,main0+0x9F0,main0+0xA00,true,none)==0 && w(g,own0+28)==(1u|(2u<<5)) && w(g,own0+24)==0 &&
        w(g,own0+20)==8 && w(g,own0+16)==0xB1000000,"the epoch advance resets both masks; the attachment admits again");
     ++groups;}
    // 8. Brackets: skip-branch end releases a reservation; an open draw3 trips the begin; a stale
    // reservation is released by draw3; a paid attachment cannot be closed by another frame.
    {Rig g;setup(g,1);require(prepare(g)==pid+480,"prepared");
     require(call(g,candidate::tooie_cpu625_attach_end,owner,a6250)==0 && w(g,cx)==0,"A6674 skip: the end releases the reservation");
     Rig h;setup(h,1);require(prepare(h)==pid+480,"prepared");auto* rdram=h.ram.data();put(rdram,root_mode,0);
     require(call(h,candidate::tooie_cpu625_begin,owner,model)==0 && w(h,cs+456)==1 && w(h,cx)==owner && w(h,cx+40)==0,
        "a stale reservation is released (counted) and draw3 begins");
     require(call(h,candidate::tooie_cpu625_attach_begin,owner,model,a6250)==1 && w(h,cs+452)==1,"an open draw3 trips the attachment begin");
     Rig k;setup(k,1);open(k);require(prepare(k)==pid+480,"prepared");auto* kr=k.ram.data();const uint32_t at=main0+0x400;
     put(kr,sh+20,main0+0x80);put(kr,staged,copy_a);put(kr,depth,0);
     call(k,candidate::tooie_cpu625_attach_begin,owner,model,a6250);put(kr,depth,1);
     put(kr,at-0x10,0xDB060004);put(kr,at-0xC,phys(copy_a+24));call(k,candidate::tooie_ckcopy_segment,at-0x10);
     put(kr,buffers,at);call(k,candidate::tooie_cpu625_admit,buffers,buffers,model,at-0x10);put(kr,buffers,at+16+136);put(kr,depth,0);
     require(call(k,candidate::tooie_cpu625_attach_end,owner,a6250-0x100)==1 && w(k,cs+452)==1,
        "a paid attachment closed from another frame trips");
     ++groups;}
    return groups;
}
#endif
#endif
}
#ifdef TOOIE_CKCOPY_FIXTURE
// Original copy-manager getters read by the extracted CPU625 admit: handle -> current copy and
// handle -> the staged C948[11] word. Explicit doubles returning the P04a rig's values.
extern "C" void func_80100A74(uint8_t*,recomp_context* c){
    require(low(c->r4)==1 && low(c->r5)==0,"copy getter receives the bound handle and second byte");
    clobber(c);c->r2=guest(c625_copy_result);
}
extern "C" void func_80100AC4(uint8_t*,recomp_context* c){
    require(low(c->r4)==1,"AC4 getter receives the bound handle");clobber(c);c->r2=guest(c625_ac4_result);
}
#endif
#ifdef TOOIE_CAMERA_PRODUCTION_FIXTURE
// Original callees of the selected func_800E44FC: explicit doubles.
extern "C" void func_800CA7E4(uint8_t* rdram,recomp_context* c){
    require(low(c->r4)==camera_fixture_pointer,"translation getter reads D_8012D500");
    if(camera_float_fixture_active){put(rdram,low(c->r5),0x42F00000);put(rdram,low(c->r5)+4,0x42A00000);put(rdram,low(c->r5)+8,0xC3AA0000);}
    clobber(c);
}
extern "C" void func_800CB124(uint8_t* rdram,recomp_context* c){
    require(low(c->r4)==camera_fixture_pointer,"CB124 reads D_8012D500");
    if(camera_float_fixture_active){
        for(uint32_t i=0;i<16;++i)put(rdram,low(c->r5)+4*i,0);
        put(rdram,low(c->r5),0x3F400000);put(rdram,low(c->r5)+20,0x3F800000);
        put(rdram,low(c->r5)+40,0xBF000000);put(rdram,low(c->r5)+44,0xBF000000);put(rdram,low(c->r5)+56,0xC1200000);
    }
    clobber(c);c->r2=guest(camera_normalization);
}
extern "C" void func_800CA7AC(uint8_t*,recomp_context* c){
    require(camera_float_fixture_active && low(c->r4)==camera_fixture_pointer,"float route reads the production camera rotation");
    ++camera_rotation_reads;clobber(c);c->r2=guest(camera_fixture_pointer+0xA4);
}
extern "C" void func_800CA6E8(uint8_t*,recomp_context* c){
    require(low(c->r4)==camera_fixture_pointer,"scale getter reads D_8012D500");
    ++camera_scale_reads;clobber(c);c->f0.u32l=camera_lens_scale;
}
extern "C" void func_800E443C(uint8_t*,recomp_context* c){++camera_projection_builds;clobber(c);}
extern "C" void func_80018BC4(uint8_t*,recomp_context* c){clobber(c);c->r2=camera_lens_doubled?1:0;}
extern "C" void mlMtxApply(uint8_t* rdram,recomp_context* c){
    if(camera_float_fixture_active)for(uint32_t i=0;i<16;++i)put(rdram,low(c->r4)+i*4,0xC0DE0000u+i);
    clobber(c);
}
extern "C" void guMtxIdent(uint8_t*,recomp_context* c){
    camera_ident_arg=low(c->r4);++camera_ident_calls;clobber(c);c->r2=guest(camera_ident_v0);
}
#define CAMERA_HOST_UNREACHED(name) extern "C" void name(uint8_t*,recomp_context*){ \
    require(false,"HOST raster callee reached with widescreen_enabled==0");}
CAMERA_HOST_UNREACHED(func_800A8984)
CAMERA_HOST_UNREACHED(func_800A8A04)
CAMERA_HOST_UNREACHED(func_800A8AD4)
CAMERA_HOST_UNREACHED(func_800A8AF0)
CAMERA_HOST_UNREACHED(func_80015D14)
#undef CAMERA_HOST_UNREACHED
// Reached only inside the E42F0 fixture; func_800E44FC keeps widescreen_enabled==0.
extern "C" void tooie_get_latched_widescreen_settings(uint8_t*,recomp_context* c){
    require(camera_e42f0_active,"HOST settings import reached outside the E42F0 fixture");
    ++camera_settings_reads;clobber(c);c->r2=guest(camera_settings_word);
}
// Argument recorder only: a0 matrix, a1..a3 left/right/bottom and caller-stack top/near/far/scale.
extern "C" void guOrtho(uint8_t* rdram,recomp_context* c){
    require(camera_e42f0_active,"guOrtho reached outside the E42F0 fixture");
    camera_ortho_args[0]=low(c->r4);camera_ortho_args[1]=low(c->r5);camera_ortho_args[2]=low(c->r6);camera_ortho_args[3]=low(c->r7);
    for(uint32_t i=0;i<4;++i)camera_ortho_args[4+i]=uint32_t(MEM_W(0x10+4*i,c->r29));
    ++camera_ortho_calls;clobber(c);
}
#endif
extern "C" void func_800DF47C(uint8_t* rdram,recomp_context* c){
#ifdef TOOIE_SKELETAL_FIXTURE
    if(skeletal_boundary>=0){
        require(low(c->r4)==0x801017D0u && low(c->r5)==palette_actor,"nested ordinary wrapper callback");
        put(rdram,0x8012C9A8,low(c->r4));put(rdram,0x8012C9AC,low(c->r5));clobber(c);return;
    }
#endif
    events.push_back({Bind,{low(c->r4),low(c->r5)}});
    require(low(c->r4)==(current.zero_callback?0:callback)&&low(c->r5)==actor,"callback/actor fullword forwarding");
    put(rdram,actor,descriptor1);put(rdram,actor+0x38,second_scale);clobber(c);
}
extern "C" void func_801027F4(uint8_t* rdram,recomp_context* c){
#ifdef TOOIE_SKELETAL_FIXTURE
    if(skeletal_boundary>=0){clobber(c);c->r2=guest(rotation);return;}
#endif
    events.push_back({Rotation,{low(c->r4),word(rdram,actor)}});
    require(low(c->r4)==actor&&word(rdram,actor)==descriptor1,"rotation sees binding mutation");
    put(rdram,actor,descriptor2);put(rdram,actor+0x38,third_scale);put(rdram,rotation,0xC4C5C6C7);
    clobber(c);c->r2=guest(current.zero==1?0:rotation);
}
extern "C" void func_801018D8(uint8_t* rdram,recomp_context* c){
#ifdef TOOIE_SKELETAL_FIXTURE
    if(skeletal_boundary>=0){clobber(c);c->r2=guest(extra);return;}
#endif
    events.push_back({Extra,{low(c->r4),word(rdram,actor)}});
    require(low(c->r4)==actor&&word(rdram,actor)==descriptor2,"extra sees rotation mutation");
    put(rdram,actor,descriptor3);put(rdram,actor+0x38,first_scale);put(rdram,extra,0xD5D6D7D8);
    clobber(c);c->r2=guest(current.zero==2?0:extra);
}
extern "C" void func_8010347C(uint8_t* rdram,recomp_context* c){
#ifdef TOOIE_SKELETAL_FIXTURE
    if(skeletal_boundary>=0){clobber(c);c->r2=guest(palette_model);return;}
#endif
    events.push_back({Model,{low(c->r4)}});
    require(low(c->r4)==descriptor3,"Actor+0 read after both pointer helpers");
    put(rdram,actor+0x38,current.scale);clobber(c);c->r2=guest(model);
}
extern "C" void func_800A7180(uint8_t* rdram,recomp_context* c){
#ifdef TOOIE_SKELETAL_FIXTURE
    if(skeletal_boundary>=0){clobber(c);c->r2=guest(player_buffers);return;}
#endif
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
    if(honey_case>=0){clobber(c);c->r2=guest(palette_cursor_slot);return;}
#endif
#ifdef TOOIE_PLAYER_STATE_ADDRESS
    if(player_case>=0){events.push_back({Buffers,{}});clobber(c);c->r2=guest(player_buffers);return;}
#endif
    events.push_back({Buffers,{}});put(rdram,actor+4,position_mutation);
    put(rdram,rotation,0xE6E7E8E9);put(rdram,extra,0xF7F8F9FA);
    clobber(c);c->r2=guest(current.zero==3?0:buffers);
}
extern "C" void func_800DE498(uint8_t* rdram,recomp_context* c){
#ifdef TOOIE_SKELETAL_FIXTURE
    if(title_boundary){title_boundary_draw(rdram,c);return;}
    if(skeletal_boundary>=0){skeletal_boundary_draw(rdram,c);return;}
#endif
#ifdef TOOIE_PLAYER_STATE_ADDRESS
    if(player_case>=0){
        require(low(c->r4)==player_buffers,"original body buffers forwarded");
        events.push_back({Draw,{}});
        uint32_t cursor=word(rdram,player_buffers);
        if(player_case==5)put(rdram,player_buffers,cursor+1);
        else if(player_case!=1){put(rdram,cursor,0xDF123456);put(rdram,cursor+4,0x789ABCDE);put(rdram,player_buffers,cursor+8);}
        if(player_draw_body)player_draw_body(rdram,low(c->r29));
        clobber(c);c->r2=guest(returned);return;
    }
#endif
    const uint32_t fourth=word(rdram,low(c->r29)+0x10),fifth=word(rdram,low(c->r29)+0x14);
    events.push_back({Draw,{low(c->r4),low(c->r5),low(c->r6),low(c->r7),fourth,fifth,
        word(rdram,actor+4),word(rdram,rotation),word(rdram,extra)}});
    require(low(c->r4)==(current.zero==3?0:buffers)&&low(c->r5)==actor+4&&
        low(c->r6)==((chained&&current.zero==1)?0:rotation)&&low(c->r7)==current.scale&&
        fourth==((chained&&current.zero==2)?0:extra)&&fifth==model,"fullword arguments/raw scale forwarding");
    require(word(rdram,actor+4)==position_mutation&&word(rdram,rotation)==0xE6E7E8E9&&
        word(rdram,extra)==0xF7F8F9FA,"current pointed arrays not copied before helper");
    clobber(c);c->r2=guest(current.zero==4?0:returned);
}
#ifdef TOOIE_PLAYER_STATE_ADDRESS
extern "C" void func_800AE080(uint8_t*,recomp_context* c){
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
    if (palette_case) {
        require(low(c->r4)==1,"palette getter double receives bounded positive pose handle");
        ++palette_pose_getters;clobber(c); c->r2=guest(palette_pose_result); return;
    }
#endif
    require(player_case>=0&&low(c->r4)==1,"typed signed pose resolver argument");
    clobber(c);c->r2=guest(player_pose);
}
extern "C" void func_800A89F8(uint8_t*,recomp_context* c){
#ifdef TOOIE_SKELETAL_FIXTURE
    if(palette_case){clobber(c);c->r2=skeletal_view;return;}
#endif
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
    if(honey_case>=0){clobber(c);c->r2=0;return;}
#endif
    require(player_case>=0,"unexpected view getter in unbound differential");clobber(c);c->r2=player_view;
}
extern "C" void func_800EB854(uint8_t*,recomp_context* c){
#ifdef TOOIE_SKELETAL_FIXTURE
    if(palette_case){clobber(c);c->r2=skeletal_phase;return;}
#endif
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
    if(honey_case>=0){clobber(c);c->r2=0;return;}
#endif
    require(player_case>=0,"unexpected phase getter in unbound differential");clobber(c);c->r2=0;
}
#endif
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
extern "C" void func_80101870(uint8_t* rdram,recomp_context* c) {
    require(honey_case>=0 && low(c->r4)==palette_actor && low(c->r5)==honey_arg,
        "Honey original draw boundary receives unchanged actor and argument");
    ++honey_draws;
    const uint32_t cursor=word(rdram,palette_cursor_slot);
    if(honey_case==3)put(rdram,palette_cursor_slot,cursor+1);
    else {
        put(rdram,cursor,0xD0000000);put(rdram,cursor+4,0xD0000001);
        put(rdram,palette_cursor_slot,cursor+8);
    }
    if(honey_case==2)MEM_H(0x16,guest(palette_descriptor))=0x5678;
    clobber(c);
}
extern "C" void func_80103110(uint8_t*,recomp_context* c) {
    require(honey_case>=0 && low(c->r4)==palette_actor && low(c->r5)==1,
        "Honey optional show receives original actor and flag");
    ++honey_shows;clobber(c);
}
#endif
// Deliberately explicit getter doubles: matching original offsets/arguments does
// not establish execution of the original getters (separately source-reviewed).
#ifdef TOOIE_SKELETAL_FIXTURE
extern "C" void func_80100094(uint8_t* rdram,recomp_context* c) {
    require(palette_case && low(c->r4)==palette_actor && low(c->r5)==2,
        "title private getter receives original actor/slot2 arguments");
    const int16_t handle=MEM_H(0x86,guest(palette_actor));
    require(handle>0 && handle<250,"title private getter is never called for an invalid handle");
    ++title_private_getters;clobber(c);c->r2=guest(title_private_result);
}
#endif
extern "C" void func_80100368(uint8_t*,recomp_context* c) {
    require(palette_case && (low(c->r4)==palette_actor || low(c->r4)==palette_actor+0x100),
        "palette ActorData getter double receives captured actor");
    clobber(c); c->r2=guest(palette_data);
}
extern "C" void func_80104148(uint8_t*,recomp_context* c) {
    require(palette_case && low(c->r4)<11,"palette class getter double index within original11");
    const uint32_t address=palette_classes+low(c->r4)*20;
    clobber(c); c->r2=guest(address);
}
extern "C" void func_80104200(uint8_t* rdram,recomp_context* c) {
    const uint32_t pool=word(rdram,0x80136E70),handle=low(c->r4);
    require(palette_case && handle>0 && handle<uint32_t(MEM_H(2,guest(pool))),
        "palette controller getter double only receives a bounded handle");
    const uint32_t address=pool+4+handle*uint32_t(MEM_H(0,guest(pool)));
    ++palette_controller_getters; clobber(c); c->r2=guest(address);
}
#endif
#ifdef TOOIE_PLAYER_STATE_ADDRESS
// Original-callee boundaries; exact call/memory differential is a separate fixture.
#define LIFECYCLE_STUB(name) extern "C" void name(uint8_t*,recomp_context* c){clobber(c);c->r2=1;}
LIFECYCLE_STUB(_babackpack_entrypoint_9)
LIFECYCLE_STUB(func_800936E8)
LIFECYCLE_STUB(func_80093700)
LIFECYCLE_STUB(func_800AE020)
LIFECYCLE_STUB(func_800E09B8)
LIFECYCLE_STUB(func_800AE6FC)
LIFECYCLE_STUB(func_800DBFF8)
LIFECYCLE_STUB(func_800EFD24)
LIFECYCLE_STUB(func_80093370)
LIFECYCLE_STUB(func_8009337C)
LIFECYCLE_STUB(func_80093230)
LIFECYCLE_STUB(func_800931AC)
LIFECYCLE_STUB(func_80092778)
LIFECYCLE_STUB(func_80092864)
LIFECYCLE_STUB(func_80098E64)
LIFECYCLE_STUB(heap_free)
#undef LIFECYCLE_STUB
#endif
// Physical-address double for the extracted func_800DD024 leaf DE (KSEG0 fixture lists only).
extern "C" void osVirtualToPhysical_recomp(uint8_t*,recomp_context* c){
    const uint32_t p=low(c->r4);
    require(p>=0x80000000u && p<0xA0000000u,"feather leaf list is a KSEG0 fixture address");
    clobber(c);c->r2=guest(p&0x1FFFFFFFu);
}
extern "C" void osVirtualToPhysical(uint8_t* rdram,recomp_context* c){osVirtualToPhysical_recomp(rdram,c);}
int main(){
    try{
        unsigned comparisons=0,player_cases=0,palette_groups=0,paid_bound_cases=0;
        for(uint32_t bits:{0x3FC12345u,0u,0x80000000u,0x7FC12345u,0x7F812345u}){
            compare(true,{bits,0,false});compare(false,{bits,0,false});comparisons+=2;
        }
        for(int zero=1;zero<=4;++zero){compare(true,{0xC1234567u,zero,false});++comparisons;}
        compare(true,{0xC1234567u,0,true});++comparisons;
        compare(false,{0xC1234567u,3,false});compare(false,{0xC1234567u,4,false});comparisons+=2;
#ifdef TOOIE_PLAYER_STATE_ADDRESS
        paid_bound_cases+=player_paid_physical_fixtures();
        for(int which=0;which<20;++which){player_fixture(which);++player_cases;}
        for(int which:{23,24,25,26,27,28,29,30}){player_fixture(which);++player_cases;}
        for(uint32_t asset:{0x838u,0x839u})for(int which:{20,21,22}){
            player_fixture(which,asset,asset==0x838u?60u:58u);++player_cases;
        }
        const unsigned player_census_cases=player_census_fixtures();
        require(player_census_cases==128,"all player census row cases ran");
        const unsigned player_noncontrolled_cases=player_noncontrolled_fixtures();
        require(player_noncontrolled_cases==4,"all R21-3 current noncontrolled-owner cases ran");
        const unsigned player_fullview_cases=player_fullview_fixtures();
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
        require(player_fullview_cases==7,"all PC[8] full-view entry cases ran");
#else
        require(player_fullview_cases==0,"standalone storage runs no full-view quota case");
#endif
#if defined(TOOIE_PLAYER_SECONDARY_FIXTURE) && !defined(TOOIE_BK_B4_FIXTURE)
        // B4 BK selection: DE448 no longer reaches the PC[8] secondary begin. BaselineBkBoundariesTest
        // covers its 92224 BK route; these envelope cases run in the BK-OFF composition, and the
        // secondary helpers keep their own BaselinePlayerSecondaryTest controls.
        const unsigned player_secondary_cases=player_secondary_envelope_fixtures();
        require(player_secondary_cases==(TOOIE_SECONDARY_FIXTURE_ADMIT?16u:2u),"all secondary shared-envelope cases ran");
#endif
#endif
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
        camera_epoch_fixtures();
        paid_bound_cases+=actor_paid_physical_fixtures();
        palette_groups=palette_fixtures()+select_prop_fixtures();
#endif
#ifdef TOOIE_SKELETAL_FIXTURE
#ifndef TOOIE_BK_B4_FIXTURE
        const unsigned skeletal_cases=skeletal_fixtures();
        require(skeletal_cases==168,"all skeletal compiled guest cases ran");
        const unsigned title_cases=title53_fixtures();
        require(title_cases==50,"all finite title53 compiled helper cases ran");
#else
        // B4 BK selection: DE448/80101808 no longer reach the NPC665 direct-world, pending and
        // title53 admissions these groups drive. They run in the BK-OFF composition; the BK owner
        // behaviour of the same boundaries is BaselineBkBoundariesTest. Reported here as not run.
        const unsigned skeletal_cases=0,title_cases=0;
#endif
        const unsigned feather_cases=feather_cut_fixtures();
        require(feather_cases==10,"all N1 feather cut helper/emitter cases ran");
#endif
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
#ifndef TOOIE_BK_SHADOW_FIXTURE
        const unsigned honey_cases=honey_close_fixtures(),shadow_cases=shadow_mode_fixtures();
        const unsigned shadow_mv_cases=shadow_modelview_fixtures();
#else
        // BK selection: the generic BK shadow scope, DEFB0 policy and compositor helpers are
        // BaselineBkShadowTest; the legacy shadow cases run in the BK-OFF composition (reported 0).
        const unsigned honey_cases=honey_close_fixtures(),shadow_cases=0,shadow_mv_cases=0;
#endif
#endif
#ifdef TOOIE_CAMERA_PRODUCTION_FIXTURE
        const unsigned camera_production_cases=camera_production_fixtures();
        require(camera_production_cases==17,"all R16 camera production cases ran");
        const unsigned camera_float_cases=camera_float_route_fixtures();
        require(camera_float_cases==11,"all focused float emitter/restore ownership cases ran");
        const unsigned e42f0_cases=e42f0_fixtures();
        require(e42f0_cases==9,"all N2 selected E42F0 cases ran");
#endif
#ifdef TOOIE_CKCOPY_FIXTURE
        const unsigned ckcopy_cases=ckcopy_fixtures();
        require(ckcopy_cases==20,"all P04b ckpack copy-storage groups ran");
        const unsigned p04a_cases=p04a_fixtures();
        require(p04a_cases==12,"all P04a CPU625 row and label-promotion groups ran");
        const unsigned n3_cases=n3_fixtures();
        require(n3_cases==8,"all N3 shadow-attachment groups ran");
#endif
        std::cout<<"Draw boundary actual differential fixture passed: "<<comparisons
            <<" comparisons; player_cases="<<player_cases
            <<"; paid_bound_cases="<<paid_bound_cases
            <<"; palette_groups="<<palette_groups
            <<"; candidate frames="<<TOOIE_DRAW_101808_FRAME_BYTES<<','<<TOOIE_DRAW_DE448_FRAME_BYTES;
#ifdef TOOIE_PLAYER_STATE_ADDRESS
        std::cout<<"; player_state="<<state<<"; state_bytes="<<TOOIE_BRIDGE_STATE_BYTES
            <<"; player_frames="<<TOOIE_PLAYER_BEGIN_FRAME_BYTES<<','<<TOOIE_PLAYER_END_FRAME_BYTES
            <<"; player_census_cases="<<player_census_cases
            <<"; player_noncontrolled_cases="<<player_noncontrolled_cases<<"; noncontrolled=PC8_ENTRY"
            <<"; player_fullview_cases="<<player_fullview_cases;
#endif
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
        std::cout<<"; palette_frames="<<TOOIE_ACTOR_PALETTE_BEGIN_FRAME_BYTES<<','<<TOOIE_ACTOR_PALETTE_END_FRAME_BYTES
            <<"; getter_boundaries=DOUBLED; original_tail=MODELED; loop_timer_store=MODELED";
#endif
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
        std::cout<<"; honey_close_cases="<<honey_cases<<"; shadow_mode_cases="<<shadow_cases
            <<"; shadow_modelview_cases="<<shadow_mv_cases
            <<"; required_stops="<<close_stop::calls<<"; required_stop=THROWING_IMPORT_DOUBLE";
#endif
#ifdef TOOIE_CAMERA_PRODUCTION_FIXTURE
        std::cout<<"; camera_production_cases="<<camera_production_cases<<"; camera_callees=DOUBLED; revision_birth=MODELED";
        std::cout<<"; e42f0_cases="<<e42f0_cases<<"; e42f0_callees=DOUBLED; e42f0_policy=ACTUAL";
#endif
#ifdef TOOIE_CKCOPY_FIXTURE
        std::cout<<"; ckcopy_cases="<<ckcopy_cases<<"; hook_seams=SOURCE_FACTS";
        std::cout<<"; p04a_cases="<<p04a_cases<<"; copy_getters=DOUBLED; rt64_ignore=SOURCE_FACT";
        std::cout<<"; n3_cases="<<n3_cases<<"; silhouette_group=MODELED; attachment_seams=SOURCE_FACTS";
#endif
        std::cout<<'\n';
#ifdef TOOIE_SKELETAL_FIXTURE
        std::cout<<"Title53 compiled helper cases="<<title_cases<<"; producer=SOURCE_PROVEN; DE498=DOUBLE\n";
        std::cout<<"Skeletal compiled guest cases="<<skeletal_cases
            <<"; getter_boundaries=DOUBLED; graph_visits=SYNTHETIC; DE498=DOUBLE\n";
#ifdef TOOIE_BK_DRAW_FIXTURE
        std::cout<<"Feather cut compiled cases="<<feather_cases<<"; retained_helper+BK_DD024=ACTUAL; cut_input=EXPLICIT; leaf_list=SYNTHETIC; RT64=NOT_RUN\n";
#else
        std::cout<<"Feather cut compiled cases="<<feather_cases<<"; helper+DD024=ACTUAL; leaf_list=SYNTHETIC; RT64=NOT_RUN\n";
#endif
#endif
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
