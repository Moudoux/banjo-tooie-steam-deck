// BK selection after packet4 B4 and packet5: the actual translated DE448/80101808 bodies and the
// actual BK owner helpers (bk_pipeline.c, bk_attach.inc, bk_shadow_attach.inc). Original DE498,
// getters and the retained row helpers are doubles; retired row transports throw if reached.
// Body/helper evidence only: no DF0B0 root, float record, runtime, RT64 or visual claim.
#include "recomp.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
#include "../tools/baseline/guest/silo-candidate/render_bridge.h"
#include "../tools/baseline/guest/silo-candidate/bk_pipeline.h"
#include "bk_pipeline_selection_layout.h"
#include "camera_identity_layout.h"
#include "../tools/baseline/guest/camera-candidate/camera_identity.h"
#include "../tools/baseline/menu_text.hpp"   // L14-VR03: native reference of the guest option-text owner
static_assert(TOOIE_BK_PIPELINE_SELECTED==1u,"BK boundary fixture requires the BK image selection");
#ifndef TOOIE_PLAYER_STATE_ADDRESS
#error Define TOOIE_PLAYER_STATE_ADDRESS from the selected bridge receipt
#endif
#if !defined(TOOIE_DRAW_101808_FRAME_BYTES) || !defined(TOOIE_DRAW_DE448_FRAME_BYTES) || !defined(TOOIE_BK_HELPER_FRAME_BYTES)
#error Define the candidate and BK helper frames from the selected receipt
#endif
extern "C" {
void func_800DF47C(uint8_t*, recomp_context*);
void func_801027F4(uint8_t*, recomp_context*);
void func_801018D8(uint8_t*, recomp_context*);
void func_8010347C(uint8_t*, recomp_context*);
void func_800A7180(uint8_t*, recomp_context*);
void func_800DE498(uint8_t*, recomp_context*);
void func_800A89F8(uint8_t*, recomp_context*);
void func_800EB854(uint8_t*, recomp_context*);
void func_80100368(uint8_t*, recomp_context*);
void func_80100094(uint8_t*, recomp_context*);
void func_80104248(uint8_t*, recomp_context*);
void tooie_player_begin(uint8_t*, recomp_context*);
void tooie_player_end(uint8_t*, recomp_context*);
void tooie_actor_palette_end(uint8_t*, recomp_context*);
void tooie_player_secondary_begin(uint8_t*, recomp_context*);
void baseline_npc665_end_checked(uint8_t*, recomp_context*);
}
namespace candidate {
#include "bk_boundaries_fixture.inc"
}
namespace {
constexpr uint32_t state=TOOIE_PLAYER_STATE_ADDRESS, bk=TOOIE_BK_STATE;
constexpr uint32_t buffers=0x80127658u, cursor0=0x80300100u, mtx0=0x80400000u;   // canonical A7180 structure
constexpr uint32_t rotation=0x80212224u, extra=0x80313234u, returned=0x80515254u, model=0x80616264u;
constexpr uint32_t c948=0x8012C948u, descriptors=0x80132ED0u, alive=0x80135450u;
constexpr uint32_t actor=0x80250000u, receiver=0x80260000u, owner=0x80228000u, callback=0x8071B2B4u;
constexpr uint32_t owner_b=0x80229000u, roster=0x80135490u, controlled=0x801354DCu;   // 2nd PlayerState; D_80135490[8]; D_801354DC
constexpr uint32_t frame_a=0x807FE000u, frame_b=0x807FD000u;   // two A6250 activations (original sp)
constexpr uint32_t stack=0x80F00000u, guard=1024u;
constexpr uint32_t de448_depth=TOOIE_DRAW_DE448_FRAME_BYTES+TOOIE_BK_HELPER_FRAME_BYTES;
constexpr uint32_t chain_depth=TOOIE_DRAW_101808_FRAME_BYTES+de448_depth;
static_assert(chain_depth<guard,"finite candidate frames");
static_assert(stack+32u<=state || stack-guard>=state+sizeof(TooieRenderBridgeState),"stack disjoint from bridge state");
static_assert(stack+32u<=bk || stack-guard>=bk+sizeof(TooieBkState),"stack disjoint from BK state");
// tooie_bk_state layout1 (bk_pipeline.h) and bridge fields read by the BK boundaries.
constexpr uint32_t f_depth=bk+offsetof(TooieBkState,draw_depth);
constexpr uint32_t f_id=bk+offsetof(TooieBkState,cur_drawn_model_transform_id);
constexpr uint32_t f_scope=bk+offsetof(TooieBkState,scope_depth);
constexpr uint32_t f_crumb=bk+offsetof(TooieBkState,reserved0);
constexpr uint32_t f_kind=bk+offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,kind);
constexpr uint32_t f_key=bk+offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,base_attachment);
constexpr uint32_t f_birth=bk+offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,birth);
constexpr uint32_t f_view=bk+offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,view);
constexpr uint32_t f_phase=bk+offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,phase);
constexpr uint32_t f_untagged=bk+offsetof(TooieBkState,diagnostics)+offsetof(TooieBkDiagnostics,player_untagged);
constexpr uint32_t sc_actor=state+offsetof(TooieRenderBridgeState,silo_context);   // SC.actor, first word
constexpr uint32_t births=state+offsetof(TooieRenderBridgeState,births);
constexpr uint32_t silo_depth=state+offsetof(TooieRenderBridgeState,silo_depth);
constexpr uint32_t ledger=state+offsetof(TooieRenderBridgeState,secondary_ledger);
constexpr uint32_t shadow_kind=3u;   // bk_shadow_attach.inc TOOIE_BK_OWNER_SHADOW_ATTACHMENT
// Independent restatement of the BK ID layout: 256 IDs per sample, 4 views x 7 phases per owner.
constexpr uint32_t sample(uint32_t view,uint32_t phase){return (view*7u+phase)*256u;}
// Split-player identity: roster slot 0 keeps the original player range; slots 1..7 follow the shadow domain.
constexpr uint32_t player_id(uint32_t slot,uint32_t ordinal,uint32_t view,uint32_t phase){
    return (slot?0x20E8C000u+(slot-1u)*16u*7168u:0x20000000u)+ordinal*7168u+sample(view,phase);}
constexpr uint32_t shadow_id(uint32_t slot,uint32_t view,uint32_t phase){return 0x2001C000u+slot*7168u+sample(view,phase);}
constexpr uint32_t actor_id(uint32_t birth,uint32_t view,uint32_t phase){return 0x50000000u+(birth-1u)*4u*7168u+sample(view,phase);}
static_assert(TOOIE_BK_PLAYER_SLOTS==8u&&TOOIE_BK_PLAYER_ID_END==0x2001C000u&&TOOIE_BK_PLAYER_SLOT_ID_START==0x20E8C000u&&
    TOOIE_BK_PLAYER_SLOT_ID_END==0x20F50000u&&player_id(1,0,0,0)==TOOIE_BK_PLAYER_SLOT_ID_START&&
    player_id(7,15,3,6)+255u<TOOIE_BK_PLAYER_SLOT_ID_END&&TOOIE_BK_PLAYER_SLOT_ID_END<=0x40000000u,
    "slot 0 keeps the player range; slots 1-7 sit after the shadow domain (0x2001C000-0x20E8C000), below 0x40000000");

enum Kind { Buffers, Begin, Draw, End, PaletteEnd };
std::vector<Kind> events;
struct Seen { uint32_t id, kind, depth, scope; } seen{};
uint32_t view_value=1, phase_value=5, begin_result=0, expected_position=0;
uint32_t end_scope=0xFFFFFFFFu, end_id=0xFFFFFFFFu;
std::vector<uint8_t> ram;
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
// Stack canaries: nothing below the measured frames, nothing above the incoming argument homes.
template<class F> void guarded(uint8_t* rdram,uint32_t depth,uint32_t homes,F call){
    for(uint32_t a=stack-guard;a<stack+32;++a)put_byte(rdram,a,0xA5);
    call();
    for(uint32_t a=stack-guard;a<stack-depth;++a)require(byte(rdram,a)==0xA5,"write below the measured frames");
    for(uint32_t a=stack+homes;a<stack+32;++a)require(byte(rdram,a)==0xA5,"write above the argument homes");
    for(uint32_t a=stack-guard;a<stack+32;++a)put_byte(rdram,a,0);
}
uint8_t* fresh(){
    ram.assign(16*1024*1024,0);uint8_t* rdram=ram.data();
    events.clear();seen={};end_scope=end_id=0xFFFFFFFFu;view_value=1;phase_value=5;begin_result=0;
    put(rdram,bk,TOOIE_BK_MAGIC);put(rdram,bk+4,TOOIE_BK_LAYOUT);
    put(rdram,buffers,cursor0);put(rdram,buffers+4,mtx0);
    // Actor slot 7: its descriptor, live creation generation 5 and its D_80135450 bit.
    put(rdram,actor,descriptors+7u*0x30u);put(rdram,births+7u*4u,5);put_byte(rdram,alive,0x01);
    // Shadow receiver of roster slot 3: u16(receiver+0x76) >> 7.
    MEM_H(0x76,guest(receiver))=int16_t(3u<<7);
    return rdram;
}
// A bound D_80135490 roster member: PlayerOwner.slot (+0x184) and its roster entry.
void bind(uint8_t* rdram,uint32_t o,uint32_t slot){put(rdram,o+0x184u,slot);put(rdram,roster+slot*4u,o);}
std::vector<uint32_t> snapshot(uint8_t* rdram){
    std::vector<uint32_t> v;for(uint32_t o=0;o<sizeof(TooieBkState);o+=4)v.push_back(word(rdram,bk+o));return v;
}
void idle(uint8_t* rdram,uint32_t id,const char* why){
    require(word(rdram,f_id)==id&&word(rdram,f_kind)==TOOIE_BK_OWNER_NONE&&!word(rdram,f_depth)&&
        !word(rdram,f_scope)&&!word(rdram,silo_depth),why);
}
uint32_t de448(uint8_t* rdram,uint32_t position){
    events.clear();expected_position=position;
    guarded(rdram,de448_depth,20,[&]{
        auto c=context();c.r4=guest(position);c.r5=guest(rotation);c.r6=0;c.r7=guest(extra);
        put(rdram,stack+0x10,model);const auto before=c;
        candidate::func_800DE448(rdram,&c);saved(c,before);
        require(low(c.r2)==returned,"original DE498 result returned");
    });
    return word(rdram,buffers);
}
void marker(uint8_t* rdram){
    events.clear();expected_position=actor+4;
    guarded(rdram,chain_depth,16,[&]{
        auto c=context();c.r4=guest(actor);c.r5=guest(callback);const auto before=c;
        candidate::func_80101808(rdram,&c);saved(c,before);
        require(low(c.r2)==returned,"original DE498 result returned through 80101808");
    });
}
uint32_t helper(uint8_t* rdram,recomp_func_t* fn,uint32_t a0=0,uint32_t a1=0,uint32_t a2=0){
    uint32_t result=0;
    guarded(rdram,TOOIE_BK_HELPER_FRAME_BYTES,16,[&]{
        auto c=context();c.r4=guest(a0);c.r5=guest(a1);c.r6=guest(a2);const auto before=c;
        fn(rdram,&c);saved(c,before);result=low(c.r2);
    });
    return result;
}

// Actual guest menu scope helpers: flags2/3 must preserve the producer identity through DE448.
unsigned transition_scope_cases(){
    constexpr uint32_t pending=bk+offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,own_draw_pending);
    constexpr uint32_t owner_id=bk+offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,id);
    constexpr uint32_t skip=bk+offsetof(TooieBkState,cur_drawn_model_skip_interpolation);
    unsigned cases=0;
    for(uint32_t flags:{2u,3u})for(uint32_t prior_skip:{0u,1u}){
        uint8_t* rdram=fresh();put(rdram,f_kind,TOOIE_BK_OWNER_ACTOR);put(rdram,f_id,0x50001000u);
        put(rdram,owner_id,0x50001000u);put(rdram,pending,1);put(rdram,skip,prior_skip);
        const uint32_t token=helper(rdram,candidate::tooie_bk_menu_model_begin,0x30060400u,flags);
        require(token&&word(rdram,f_kind)==4&&word(rdram,f_id)==0x30060400u&&word(rdram,pending)==1&&
            word(rdram,skip)==(flags&1u),"transition flags set owned pending and independent skip");
        const uint32_t draw=helper(rdram,candidate::tooie_bk_de448_begin,actor+4,model);
        require(draw==0&&word(rdram,pending)==0&&word(rdram,f_id)==0x30060400u&&word(rdram,f_scope)==1,
            "own DE448 consumes pending without changing transition identity or scope");
        helper(rdram,candidate::tooie_bk_de448_end,draw,buffers);
        helper(rdram,candidate::tooie_bk_menu_model_end,token);
        require(word(rdram,f_kind)==TOOIE_BK_OWNER_ACTOR&&word(rdram,f_id)==0x50001000u&&
            word(rdram,owner_id)==0x50001000u&&word(rdram,pending)==1&&word(rdram,skip)==prior_skip&&
            !word(rdram,f_scope)&&!word(rdram,f_depth),"scope end restores prior owner, pending, skip and depths");
        ++cases;
    }
    for(uint32_t bad:{0x30060500u,0x30070000u,0x2FFFFF00u,0x30060001u}){
        uint8_t* rdram=fresh();const auto before=snapshot(rdram);
        require(!helper(rdram,candidate::tooie_bk_menu_model_begin,bad,2)&&snapshot(rdram)==before,
            "outside exact menu plus five-view domain or unaligned ID is mutation-free refusal");++cases;
    }
    {uint8_t* rdram=fresh();const auto before=snapshot(rdram);
        require(!helper(rdram,candidate::tooie_bk_menu_model_begin,0x30060000u,4)&&snapshot(rdram)==before,
            "unknown flags refused without mutation");++cases;}
    // Unowned DF944 UI models (bk_menu_thunks.hpp bk_ui): the UI domain under the same owner scope. DF944 calls
    // DE498 directly, so flags 0/1 set no own-DE448 pending; the DFAA8 end restores the unowned context exactly.
    for(uint32_t flags:{0u,1u}){
        constexpr uint32_t ui=TOOIE_BK_UI_ID_START+0x7E00u;   // the last model/ordinal component
        uint8_t* rdram=fresh();put(rdram,f_id,0x12345600u);
        const uint32_t token=helper(rdram,candidate::tooie_bk_menu_model_begin,ui,flags);
        require(token&&word(rdram,f_kind)==TOOIE_BK_OWNER_MENU&&word(rdram,f_id)==ui&&word(rdram,owner_id)==ui&&
            !word(rdram,pending)&&word(rdram,skip)==flags&&word(rdram,f_scope)==1u,
            "UI model: menu owner kind, its UI ID, no pending own draw, the cut as its skip");
        helper(rdram,candidate::tooie_bk_menu_model_end,token);
        idle(rdram,0x12345600u,"UI model end restores the unowned context");
        require(!word(rdram,skip)&&!word(rdram,owner_id),"UI model end restores the incoming skip and owner ID");
        ++cases;
    }
    for(uint32_t bad:{TOOIE_BK_UI_ID_END,TOOIE_BK_UI_ID_START-0x100u}){
        uint8_t* rdram=fresh();const auto before=snapshot(rdram);
        require(!helper(rdram,candidate::tooie_bk_menu_model_begin,bad,0)&&snapshot(rdram)==before,
            "outside the UI domain is mutation-free refusal");++cases;
    }
    return cases;
}

#include "bk_sprite_group_cases.inc"
#include "bk_text_cases.inc"   // L14-VR03

unsigned boundary_cases(){
    unsigned cases=0;
    // P0/P1: retained primary route 92234 staged with roster slot 0's PlayerState. BK player owner (view 1,
    // phase 5) during the body draw, the body-route record with its roster identity, and the primary end
    // only after a successful begin, after BK closed.
    for(uint32_t ok=0;ok<2;++ok){
        uint8_t* rdram=fresh();begin_result=ok;put(rdram,c948+24*4,0x80092234u);put(rdram,c948+25*4,owner);bind(rdram,owner,0);
        const uint32_t end=de448(rdram,actor+4);
        require(events==(ok?std::vector<Kind>{Buffers,Begin,Draw,End}:std::vector<Kind>{Buffers,Begin,Draw}),
            "primary begin, original draw, and the primary end only after a successful begin");
        require(seen.id==player_id(0,0,1,5)&&seen.kind==TOOIE_BK_OWNER_PLAYER&&seen.depth==1u&&seen.scope==1u&&
            word(rdram,f_untagged)==0u,"BK player_draw owner during the body draw: roster slot 0 keeps the original IDs");
        if(ok)require(end_scope==0u&&end_id==0u,"BK closes before the retained primary close");
        idle(rdram,0,"primary draw restores the BK context");
        require(word(rdram,f_crumb)==(0x100u|(1u<<16)),"the body draw records its route and roster identity for 92710");
        require(word(rdram,cursor0)==0xDF123456u&&end==cursor0+8u,"no BK command at the DE448 boundary itself");
        ++cases;
    }
    // S: 92224 keeps BK's secondary player sample under its own staged roster owner (slot 1); the retired
    // PC[8] secondary begin is never reached.
    {   uint8_t* rdram=fresh();put(rdram,c948+24*4,0x80092224u);put(rdram,c948+25*4,owner_b);bind(rdram,owner_b,1);de448(rdram,actor+4);
        require(events==std::vector<Kind>{Buffers,Draw},"no primary or secondary row begin on 92224");
        require(seen.id==player_id(1,8,1,5)&&seen.kind==TOOIE_BK_OWNER_PLAYER,"BK secondary player sample of roster slot 1");
        for(uint32_t o=0;o<sizeof(TooieSecondaryLedger);o+=4)require(!word(rdram,ledger+o),"no secondary ledger work");
        idle(rdram,0,"secondary draw restores the BK context");
        require(word(rdram,f_crumb)==(0x108u|(2u<<16)),"the 92224 body records its own route and roster identity");
        ++cases;
    }
    // W/W': direct-world draw (C948[24] 801017D0, position C948[25]+4): BK actor owner from the creation
    // generation; the retired NPC665 direct-world capture leaves SC untouched. No live birth, no owner.
    for(uint32_t live=0;live<2;++live){
        uint8_t* rdram=fresh();if(!live)put(rdram,births+7u*4u,0);
        put(rdram,c948+24*4,0x801017D0u);put(rdram,c948+25*4,actor);de448(rdram,actor+4);
        require(events==std::vector<Kind>{Buffers,Draw},"original direct-world chain");
        require(live?(seen.id==actor_id(5,1,5)&&seen.kind==TOOIE_BK_OWNER_ACTOR):(!seen.id&&seen.kind==TOOIE_BK_OWNER_NONE),
            "BK __marker_draw generation owner for a live direct-world actor only");
        require(!word(rdram,sc_actor),"no direct-world row capture");
        idle(rdram,0,"direct-world draw restores the BK context");
        ++cases;
    }
    // A: 80101808 (__marker_draw): the actor owner covers its own DE448 draw and is restored after.
    {   uint8_t* rdram=fresh();marker(rdram);
        require(events==std::vector<Kind>{Buffers,Draw},"original 80101808 draw chain");
        require(seen.id==actor_id(5,1,5)&&seen.kind==TOOIE_BK_OWNER_ACTOR&&seen.depth==1u&&seen.scope==1u,
            "the marker owner's own draw takes its ID without a nested scope");
        require(!word(rdram,sc_actor),"the retained capture is released");
        idle(rdram,0,"marker draw restores the BK context");
        ++cases;
    }
    // U: an unowned top-level draw never inherits a stale ID; the outer value is restored after.
    {   uint8_t* rdram=fresh();put(rdram,f_id,0x12345600u);de448(rdram,actor+4);
        require(!seen.id&&seen.scope==1u,"unowned draw runs with ID 0");
        idle(rdram,0x12345600u,"unowned draw restores the outer ID");
        ++cases;
    }
    // T: split players. Two different roster owners drawn in one frame, view and phase take their own
    // roster slots' IDs; the controlled slot (D_801354DC) changes neither, before or after a swap.
    for(uint32_t control=0;control<2;++control){
        uint8_t* rdram=fresh();bind(rdram,owner,0);bind(rdram,owner_b,1);put(rdram,controlled,control);
        put(rdram,c948+24*4,0x80092234u);put(rdram,c948+25*4,owner);de448(rdram,actor+4);
        const uint32_t banjo=seen.id;
        put(rdram,c948+25*4,owner_b);de448(rdram,actor+4);
        const uint32_t kazooie=seen.id;
        require(banjo==player_id(0,0,1,5)&&kazooie==player_id(1,0,1,5),"each drawn roster owner keeps its own ID under either control");
        require(banjo<0x2001C000u&&kazooie>=0x20E8C000u,"both player IDs stay outside the shadow domain");
        require(word(rdram,f_crumb)==(0x100u|(2u<<16)),"the latest body records its own roster identity for 92710");
        idle(rdram,0,"split draws restore the BK context");
        ++cases;
    }
    // I: no bound roster owner, no player identity. The roster entry names another owner, the staged
    // pointer is out of range, its slot is past the roster, or the callback is gone (DE318 clears only
    // C9A8, so a retained C948[25] is never trusted). The draw runs with ID 0 and records no body route.
    for(uint32_t miss=0;miss<4;++miss){
        uint8_t* rdram=fresh();bind(rdram,owner,0);put(rdram,f_crumb,0x100u|(1u<<16));
        put(rdram,c948+24*4,miss==3?0u:0x80092234u);put(rdram,c948+25*4,owner);
        if(miss==0)put(rdram,roster,owner_b);
        if(miss==1)put(rdram,c948+25*4,0x00001234u);
        if(miss==2)put(rdram,owner+0x184u,8u);
        de448(rdram,actor+4);
        require(!seen.id&&seen.kind==TOOIE_BK_OWNER_NONE,"no player identity without a bound staged roster owner");
        require(word(rdram,f_untagged)==(miss<3?1u:0u),"an unbound player route is counted");
        require(word(rdram,f_crumb)==(miss<3?0u:(0x100u|(1u<<16))),"an unbound player route records no body route");
        idle(rdram,0,"unbound player draw restores the BK context");
        ++cases;
    }
    // R: roster reuse. A different PlayerState later bound at slot 0 takes slot 0's IDs: the identity is the
    // roster slot (BK player_draw's one constant ID per drawn player), never the pointer or the control.
    {   uint8_t* rdram=fresh();bind(rdram,owner_b,0);put(rdram,controlled,1u);
        put(rdram,c948+24*4,0x80092234u);put(rdram,c948+25*4,owner_b);de448(rdram,actor+4);
        require(seen.id==player_id(0,0,1,5),"a reused roster slot keeps that slot's IDs");
        idle(rdram,0,"reused-slot draw restores the BK context");
        ++cases;
    }
    // C: a model nested inside a split player's draw takes the next ordinal under that player's own slot;
    // without a player identity (birth 0) it takes ID 0, never another slot's IDs.
    for(uint32_t slot=0;slot<2;++slot){
        uint8_t* rdram=fresh();
        put(rdram,f_kind,TOOIE_BK_OWNER_PLAYER);put(rdram,f_birth,slot?2u:0u);put(rdram,f_view,1u);put(rdram,f_phase,5u);
        put(rdram,f_depth,1u);put(rdram,f_id,slot?player_id(1,0,1,5):0x12345600u);
        const uint32_t token=helper(rdram,candidate::tooie_bk_de448_begin,actor+4,model);
        require(token==1u&&word(rdram,f_depth)==2u,"a nested DE448 opens an attachment scope");
        require(word(rdram,f_id)==(slot?player_id(1,1,1,5):0u),"nested ordinal 1 under the drawing player's own slot only");
        ++cases;
    }
    return cases;
}

unsigned attachment_cases(){
    unsigned cases=0;
    uint32_t main_id=0;
    // M: packet4 92710/92718 around the backpack's own DE448. The producer (func_80092444 s0) is the body's
    // roster owner: slot 0 keeps the original backpack IDs, slot 1 takes its own.
    for(uint32_t slot=0;slot<2;++slot){
        uint8_t* rdram=fresh();bind(rdram,owner,0);bind(rdram,owner_b,1);put(rdram,f_crumb,0x100u|((slot+1u)<<16));
        helper(rdram,candidate::tooie_bk_attachment_begin,slot?owner_b:owner);
        require(!word(rdram,f_crumb),"92710 consumes the body route");
        de448(rdram,actor+4);
        if(!slot)main_id=seen.id;
        require(seen.id==player_id(slot,1,1,5)&&seen.kind==TOOIE_BK_OWNER_PLAYER&&seen.scope==1u,
            "main attachment: the producer's own roster slot, ordinal 1, its own draw");
        require(word(rdram,f_id)==seen.id&&word(rdram,f_scope)==1u,"the scope stays open until 92718");
        helper(rdram,candidate::tooie_bk_attachment_end);idle(rdram,0,"92718 closes");
        ++cases;
    }
    // N: a secondary (92224) body's backpack: base 8 + ordinal 1 under its own roster slot, disjoint from
    // the primary attachment of that slot and from the other slot's.
    {   uint8_t* rdram=fresh();bind(rdram,owner_b,1);put(rdram,f_crumb,0x108u|(2u<<16));
        helper(rdram,candidate::tooie_bk_attachment_begin,owner_b);de448(rdram,actor+4);
        require(seen.id==player_id(1,9,1,5)&&seen.id!=player_id(1,1,1,5)&&seen.id!=player_id(0,9,1,5),
            "secondary attachment of roster slot 1");
        helper(rdram,candidate::tooie_bk_attachment_end);idle(rdram,0,"92718 closes");
        ++cases;
    }
    // X: the authentic producer must be the body's bound roster owner. Another owner, or a producer no
    // longer bound in the roster, opens no player scope and is counted; the backpack draws with ID 0.
    for(uint32_t miss=0;miss<2;++miss){
        uint8_t* rdram=fresh();bind(rdram,owner,0);bind(rdram,owner_b,1);put(rdram,f_crumb,0x100u|(1u<<16));
        if(miss)put(rdram,roster,0u);
        helper(rdram,candidate::tooie_bk_attachment_begin,miss?owner:owner_b);
        require(!word(rdram,f_crumb)&&!word(rdram,f_scope)&&word(rdram,f_untagged)==1u,"no player scope; counted");
        de448(rdram,actor+4);
        require(!seen.id&&seen.kind==TOOIE_BK_OWNER_NONE,"the backpack draws without a borrowed identity");
        helper(rdram,candidate::tooie_bk_attachment_end);idle(rdram,0,"92718 leaves nothing open");
        ++cases;
    }
    // The shadow producer scope (blob, body, attachment, fallback and light routes) is
    // BaselineBkShadowTest (packet6); the main attachment stays in the player domain below it.
    require(main_id==player_id(0,1,1,5)&&main_id<0x2001C000u,"main attachment below the shadow domain");
    (void)receiver;(void)owner;(void)frame_a;(void)frame_b;(void)shadow_kind;(void)f_key;(void)&snapshot;(void)&shadow_id;
    ++cases;
    return cases;
}
// Type-12 selector numbering (bk_select.inc) through the actual tooie_bk_root_begin/root_end and the
// actual selector helpers. Each script names the children the ORIGINAL 800DD56C executes, in its order;
// the original control/count/bitmask decisions themselves are proven by the root's actual-original
// DD56C harness, so this fixture never re-derives them. Synthetic graph nodes only (GRAMMAR_FACTS):
// type u32 +0, next-sibling u32 +4 (0 ends); bone child u8 +8; selector count s16 +8, offsets u32 +0x0C.
constexpr uint32_t sel_model=0x80520000u, sel_graph=sel_model+0x100u, sel_slot=0x807F0000u, sel_cursor=0x80310000u;
constexpr uint32_t handler_table=0x8012306Cu, dispatcher=0x8012CF8Cu, sel_sp=0x807E0000u;
constexpr uint32_t f_offset=bk+offsetof(TooieBkState,cur_model_transform_id_offset);
constexpr uint32_t f_roots=bk+offsetof(TooieBkState,root_depth), f_handlers=bk+offsetof(TooieBkState,handler_depth);
constexpr uint32_t f_select=bk+offsetof(TooieBkState,select_roots), f_declined=bk+offsetof(TooieBkState,select_declined);
void gnode(uint8_t* rdram,uint32_t at,uint32_t type,uint32_t next=0){put(rdram,at,type);put(rdram,at+4u,next?next-at:0u);}
void gbone(uint8_t* rdram,uint32_t at,uint32_t next=0,uint32_t child=0){gnode(rdram,at,2u,next);put_byte(rdram,at+8u,uint8_t(child?child-at:0u));}
void gselect(uint8_t* rdram,uint32_t at,uint32_t next,std::initializer_list<uint32_t> children){
    gnode(rdram,at,12u,next);MEM_H(8,guest(at))=int16_t(children.size());MEM_H(0xA,guest(at))=9;
    uint32_t i=0;for(const uint32_t c:children)put(rdram,at+0xCu+4u*i++,c-at);
}
// A bone chain of `n` nodes at `at`, 16 bytes apart; returns the first node.
uint32_t gchain(uint8_t* rdram,uint32_t at,uint32_t n){for(uint32_t i=0;i<n;++i)gbone(rdram,at+16u*i,i+1u<n?at+16u*(i+1u):0u);return at;}
// The canonical dispatcher (D_8012CF8C -> table0, five default handlers) and a PLAYER owner drawing sel_model.
uint8_t* selector_fresh(uint32_t kind=TOOIE_BK_OWNER_PLAYER){
    uint8_t* rdram=fresh();
    put(rdram,dispatcher,handler_table);
    put(rdram,handler_table+2u*4u,0x800DCF48u);put(rdram,handler_table+10u*4u,0x800DD410u);
    put(rdram,handler_table+12u*4u,0x800DD56Cu);put(rdram,handler_table+17u*4u,0x800DD65Cu);
    put(rdram,handler_table+24u*4u,0x800DD998u);
    put(rdram,sel_model+4u,sel_graph-sel_model);
    put(rdram,f_kind,kind);put(rdram,f_id,kind==TOOIE_BK_OWNER_PLAYER?0x20000500u:actor_id(5,1,5));put(rdram,f_depth,1u);
    return rdram;
}
// Graph A: point(10) -> S{c0=[b0,b1], c1=[leaf(24), b2]} -> plain list(17) -> common bone C.
constexpr uint32_t ga_s=sel_graph+0x20u, ga_c=sel_graph+0x60u, ga_b0=sel_graph+0x100u, ga_b1=sel_graph+0x110u,
    ga_leaf=sel_graph+0x200u, ga_b2=sel_graph+0x210u;
void graph_a(uint8_t* rdram,uint32_t leaf_type=24u){
    gnode(rdram,sel_graph,10u,ga_s);gselect(rdram,ga_s,sel_graph+0x40u,{ga_b0,ga_leaf});
    gnode(rdram,sel_graph+0x40u,17u,ga_c);gbone(rdram,ga_c);
    gbone(rdram,ga_b0,ga_b1);gbone(rdram,ga_b1);gnode(rdram,ga_leaf,leaf_type,ga_b2);gbone(rdram,ga_b2);
}
std::map<uint32_t,std::vector<uint32_t>> runs;   // selector node -> executed child indices (original order)
std::map<uint32_t,uint32_t> ordinals;             // bone node -> emitted ordinal
// DE2A4 with an 800DCF48 double (one advance, then its child list) and the four 800DD56C hooks.
void play(uint8_t* rdram,uint32_t list,uint32_t sp){
    for(uint32_t at=list;;){
        const uint32_t type=word(rdram,at);
        if(type==2u){
            put(rdram,f_offset,word(rdram,f_offset)+1u);ordinals[at]=word(rdram,f_offset);
            if(const uint32_t c=byte(rdram,at+8u))play(rdram,at+c,sp-0x40u);
        }else if(type==12u){
            helper(rdram,candidate::tooie_bk_selector_begin,at,sp);
            for(const uint32_t i:runs[at]){
                const uint32_t child=at+word(rdram,at+0xCu+4u*i);
                helper(rdram,candidate::tooie_bk_selector_child,at,child,sp);
                play(rdram,child,sp-0x40u);
            }
            helper(rdram,candidate::tooie_bk_selector_end,at,sp);
        }
        const uint32_t next=word(rdram,at+4u);
        if(!next)return;
        at+=next;
    }
}
void root_open(uint8_t* rdram,uint32_t m=sel_model){put(rdram,sel_slot,sel_cursor);helper(rdram,candidate::tooie_bk_root_begin,buffers,sel_slot,m);}
void root_close(uint8_t* rdram){put(rdram,buffers,cursor0);helper(rdram,candidate::tooie_bk_root_end,buffers);}
// One root of sel_model: returns whether it was reserved; the helpers never write the graph.
bool play_root(uint8_t* rdram){
    ordinals.clear();
    std::vector<uint32_t> before;for(uint32_t a=sel_model;a<sel_model+0x4000u;a+=4)before.push_back(word(rdram,a));
    root_open(rdram);
    const bool reserved=word(rdram,f_select)==1u;
    play(rdram,sel_graph,sel_sp);
    require(!word(rdram,f_handlers),"every selector frame is popped at its own universal join");
    root_close(rdram);
    require(!word(rdram,f_roots)&&!word(rdram,f_select),"the root's selector decision ends with the root");
    std::vector<uint32_t> after;for(uint32_t a=sel_model;a<sel_model+0x4000u;a+=4)after.push_back(word(rdram,a));
    require(before==after,"selector helpers never write the graph");
    return reserved;
}
unsigned selector_cases(){
    unsigned cases=0;
    const std::vector<std::vector<uint32_t>> choices={{},{0},{1},{0,1}};   // none, single, single, bitmask
    // Y1: reserved PLAYER root. Every executed choice keeps C and each child bone at fixed ordinals; points,
    // plain lists and leaves count zero.
    {   uint8_t* rdram=selector_fresh();graph_a(rdram);
        for(const auto& choice:choices){
            runs={{ga_s,choice}};
            require(play_root(rdram),"supported PLAYER root is reserved");
            require(ordinals.at(ga_c)==4u,"later common bone keeps 1 + span(c0) + span(c1)");
            require((!ordinals.count(ga_b0)||(ordinals.at(ga_b0)==1u&&ordinals.at(ga_b1)==2u))&&
                (!ordinals.count(ga_b2)||ordinals.at(ga_b2)==3u),"each child keeps its reserved span");
        }
        require(!word(rdram,f_declined),"nothing declined");
        ++cases;
    }
    // Y2: nested selector. S{c0=[S2{x0=[bx0], x1=[bx1]}, d], c1=[e]} -> C: outer and inner choices independent.
    {   uint8_t* rdram=selector_fresh();
        const uint32_t s=sel_graph,c=sel_graph+0x40u,s2=sel_graph+0x100u,d=sel_graph+0x140u,
            bx0=sel_graph+0x180u,bx1=sel_graph+0x190u,e=sel_graph+0x200u;
        gselect(rdram,s,c,{s2,e});gbone(rdram,c);gselect(rdram,s2,d,{bx0,bx1});gbone(rdram,d);
        gbone(rdram,bx0);gbone(rdram,bx1);gbone(rdram,e);
        for(const auto& outer:choices)for(const auto& inner:choices){
            runs={{s,outer},{s2,inner}};
            require(play_root(rdram)&&ordinals.at(c)==5u,"nested reservation keeps the later common bone");
            require((!ordinals.count(bx0)||ordinals.at(bx0)==1u)&&(!ordinals.count(bx1)||ordinals.at(bx1)==2u)&&
                (!ordinals.count(d)||ordinals.at(d)==3u)&&(!ordinals.count(e)||ordinals.at(e)==4u),"nested ordinals fixed");
        }
        ++cases;
    }
    // Y3: an unsupported node anywhere in the root, even in a child that never executes, keeps the WHOLE
    // root on the original numbering, counted; it is never counted as zero.
    {   uint8_t* rdram=selector_fresh();graph_a(rdram,99u);
        runs={{ga_s,{0}}};
        require(!play_root(rdram)&&ordinals.at(ga_c)==3u&&word(rdram,f_declined)==1u,"unsupported root: original numbering");
        runs={{ga_s,{}}};
        require(!play_root(rdram)&&ordinals.at(ga_c)==1u&&word(rdram,f_declined)==2u,"and for every choice");
        ++cases;
    }
    // Y4: a non-default dispatcher entry, a moved table, or a repeated child pointer: whole root declined.
    for(int which=0;which<3;++which){
        uint8_t* rdram=selector_fresh();graph_a(rdram);runs={{ga_s,{0}}};
        if(which==0)put(rdram,handler_table+12u*4u,0x800DD570u);
        if(which==1)put(rdram,dispatcher,handler_table+0x100u);
        if(which==2)put(rdram,ga_s+0x10u,ga_b0-ga_s);
        require(!play_root(rdram)&&ordinals.at(ga_c)==3u&&word(rdram,f_declined)==1u,"unauthenticated grammar: original numbering");
        ++cases;
    }
    // Y5: actor roots are out of scope: untouched and not counted.
    {   uint8_t* rdram=selector_fresh(TOOIE_BK_OWNER_ACTOR);graph_a(rdram);runs={{ga_s,{0}}};
        require(!play_root(rdram)&&ordinals.at(ga_c)==3u&&!word(rdram,f_declined),"actor root keeps the original traversal");
        ++cases;
    }
    // Y6: the whole root must fit the 256-ID sample (offset < 256): 127 + 128 bones fit, 127 + 129 do not.
    for(const uint32_t second:{128u,129u}){
        uint8_t* rdram=selector_fresh();
        const uint32_t c0=gchain(rdram,sel_graph+0x1000u,127u),c1=gchain(rdram,sel_graph+0x2000u,second);
        gselect(rdram,sel_graph,0u,{c0,c1});runs={{sel_graph,{1}}};
        const bool reserved=play_root(rdram);
        require(reserved==(second==128u)&&ordinals.at(c1)==(reserved?128u:1u)&&word(rdram,f_declined)==(reserved?0u:1u),
            "reserve only when every alternative of the root fits the window");
        if(reserved)require(ordinals.at(c1+16u*127u)==255u,"the last reserved ordinal is 255");
        ++cases;
    }
    // Y7: a nested root of another owner inside a reserved selector child keeps its own original numbering,
    // leaves the enclosing selector frame alone, and restores the enclosing ordinal at its close.
    {   uint8_t* rdram=selector_fresh();graph_a(rdram);
        constexpr uint32_t inner=0x80530000u;
        put(rdram,inner+4u,0x100u);gselect(rdram,inner+0x100u,0u,{inner+0x140u});gbone(rdram,inner+0x140u);
        root_open(rdram);require(word(rdram,f_select)==1u,"outer PLAYER root reserved");
        helper(rdram,candidate::tooie_bk_selector_begin,ga_s,sel_sp);
        helper(rdram,candidate::tooie_bk_selector_child,ga_s,ga_leaf,sel_sp);
        require(word(rdram,f_offset)==2u&&word(rdram,f_handlers)==1u,"second child starts after the first child's span");
        put(rdram,f_kind,TOOIE_BK_OWNER_ACTOR);root_open(rdram,inner);
        require(word(rdram,f_select)==1u&&!word(rdram,f_offset),"nested actor root: not reserved, its own offset");
        helper(rdram,candidate::tooie_bk_selector_begin,inner+0x100u,sel_sp-0x40u);
        helper(rdram,candidate::tooie_bk_selector_child,inner+0x100u,inner+0x140u,sel_sp-0x40u);
        put(rdram,f_offset,word(rdram,f_offset)+1u);
        helper(rdram,candidate::tooie_bk_selector_end,inner+0x100u,sel_sp-0x40u);
        require(word(rdram,f_offset)==1u&&word(rdram,f_handlers)==1u,"nested selector: original numbering, no frame");
        root_close(rdram);put(rdram,f_kind,TOOIE_BK_OWNER_PLAYER);
        require(word(rdram,f_offset)==2u&&word(rdram,f_select)==1u,"the nested close restores the enclosing context");
        put(rdram,f_offset,word(rdram,f_offset)+1u);   // b2
        helper(rdram,candidate::tooie_bk_selector_end,ga_s,sel_sp);
        require(word(rdram,f_offset)==3u&&!word(rdram,f_handlers),"outer selector ends past every child");
        root_close(rdram);require(!word(rdram,f_select)&&!word(rdram,f_declined),"closed");
        ++cases;
    }
    // Y8: no free handler slot for the root's selectors: the whole root keeps the original numbering.
    {   uint8_t* rdram=selector_fresh();graph_a(rdram);
        put(rdram,f_handlers,TOOIE_BK_HANDLER_DEPTH);root_open(rdram);
        require(!word(rdram,f_select)&&word(rdram,f_declined)==1u,"declined before any numbering change");
        root_close(rdram);
        ++cases;
    }
    // Y9: common header readable, selector count/header beyond guest RAM: decline before reading count.
    {   uint8_t* rdram=selector_fresh();
        constexpr uint32_t edge=0x80FFFFF8u;
        put(rdram,sel_model+4u,edge-sel_model);gnode(rdram,edge,12u);
        root_open(rdram);
        require(!word(rdram,f_select)&&word(rdram,f_declined)==1u,"short selector header declined");
        root_close(rdram);++cases;
    }
    // Y10: readable maximum signed count with distinct child offsets: reject before pairwise work.
    {   uint8_t* rdram=selector_fresh();gnode(rdram,sel_graph,12u);
        MEM_H(8,guest(sel_graph))=32767;MEM_H(10,guest(sel_graph))=9;
        for(uint32_t i=0;i<32767u;++i)put(rdram,sel_graph+12u+4u*i,0x30000u+16u*i);
        root_open(rdram);
        require(!word(rdram,f_select)&&word(rdram,f_declined)==1u,"excess selector fan-out declined");
        root_close(rdram);++cases;
    }
    return cases;
}
}
extern "C" void func_800A7180(uint8_t*,recomp_context* c){events.push_back(Buffers);clobber(c);c->r2=guest(buffers);}
extern "C" void func_800DE498(uint8_t* rdram,recomp_context* c){
    require(low(c->r4)==buffers&&low(c->r5)==expected_position,"original DE498 arguments forwarded");
    events.push_back(Draw);
    seen={word(rdram,f_id),word(rdram,f_kind),word(rdram,f_depth),word(rdram,f_scope)};
    const uint32_t cursor=word(rdram,buffers);
    put(rdram,cursor,0xDF123456u);put(rdram,cursor+4,0x789ABCDEu);put(rdram,buffers,cursor+8);
    clobber(c);c->r2=guest(returned);
}
extern "C" void func_800A89F8(uint8_t*,recomp_context* c){clobber(c);c->r2=view_value;}
extern "C" void func_800EB854(uint8_t*,recomp_context* c){clobber(c);c->r2=phase_value;}
extern "C" void tooie_player_begin(uint8_t*,recomp_context* c){
    require(low(c->r4)==buffers&&low(c->r5)==model,"retained primary begin receives buffers and model");
    events.push_back(Begin);clobber(c);c->r2=begin_result;
}
extern "C" void tooie_player_end(uint8_t* rdram,recomp_context* c){
    require(low(c->r4)==buffers,"retained primary end receives buffers");
    events.push_back(End);end_scope=word(rdram,f_scope);end_id=word(rdram,f_id);clobber(c);
}
extern "C" void tooie_actor_palette_end(uint8_t*,recomp_context* c){events.push_back(PaletteEnd);clobber(c);}
// 80101808 argument-resolution doubles.
extern "C" void func_800DF47C(uint8_t*,recomp_context* c){
    require(low(c->r4)==callback&&low(c->r5)==actor,"callback/actor forwarded");clobber(c);
}
extern "C" void func_801027F4(uint8_t*,recomp_context* c){require(low(c->r4)==actor,"rotation getter actor");clobber(c);c->r2=guest(rotation);}
extern "C" void func_801018D8(uint8_t*,recomp_context* c){require(low(c->r4)==actor,"extra getter actor");clobber(c);c->r2=guest(extra);}
extern "C" void func_8010347C(uint8_t* rdram,recomp_context* c){
    require(low(c->r4)==word(rdram,actor),"model getter receives the descriptor");clobber(c);c->r2=guest(model);
}
#define RETIRED(name) extern "C" void name(uint8_t*,recomp_context*){throw std::runtime_error("retired row transport reached from a BK boundary: " #name);}
RETIRED(tooie_player_secondary_begin)
RETIRED(baseline_npc665_end_checked)
#undef RETIRED
#define UNEXPECTED(name) extern "C" void name(uint8_t*,recomp_context*){throw std::runtime_error("unexpected getter: " #name);}
UNEXPECTED(func_80100368)
UNEXPECTED(func_80100094)
UNEXPECTED(func_80104248)
#undef UNEXPECTED
int main(){
    try{
        const unsigned boundaries=boundary_cases(),attachments=attachment_cases(),selectors=selector_cases(),transitions=transition_scope_cases();
        require(boundaries==16&&attachments==6&&selectors==13&&transitions==13,"all BK boundary, attachment and selector cases ran");
        const unsigned sprites=sprite_group_cases();
        require(sprites==4,"all option-box sprite group cases ran (L14-VR01b billboard ordinal case)");
        const unsigned texts=text::text_cases();
        require(texts==3,"all L14-VR03 option-text equivalence groups ran");
        std::cout<<"BK boundary actual fixture passed: boundary_cases="<<boundaries<<"; attachment_cases="<<attachments
            <<"; selector_cases="<<selectors<<"; transition_scope_cases="<<transitions<<"; sprite_group_cases="<<sprites<<"; text_equivalence_groups="<<texts
            <<"; frames="<<TOOIE_DRAW_101808_FRAME_BYTES<<','<<TOOIE_DRAW_DE448_FRAME_BYTES<<','<<TOOIE_BK_HELPER_FRAME_BYTES
            <<"; DE498=DOUBLE; root=TESTED; float=NOT_RUN; RT64=NOT_RUN\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
