// BK selection, packet6 + 6R: the actual translated BK shadow scope helpers (bk_shadow_attach.inc), the
// generic root/float path for a header-bit40 draw (bk_pipeline.c, bk_float.inc, bk_graph.inc) and
// the camera DEFB0/compositor shadow helpers (shadow_projection.c BK branch). Getters, palette
// admission and the required stop are doubles. Helper/command evidence only: no DE448/DE498 body,
// light allocator, RT64, runtime or visual claim.
#include "recomp.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "../tools/baseline/guest/silo-candidate/render_bridge.h"
#include "../tools/baseline/guest/silo-candidate/bk_pipeline.h"
#include "../tools/baseline/menu_scope.hpp"
#include "bk_pipeline_selection_layout.h"
#include "camera_identity_layout.h"
#include "../tools/baseline/guest/camera-candidate/camera_identity.h"
#include "shadow_projection_layout.h"
static_assert(TOOIE_BK_PIPELINE_SELECTED==1u && TOOIE_SHADOW_SELECTED==1u,"BK shadow fixture requires the BK shadow image");
static_assert(TOOIE_SHADOW_HELPERS==6u && TOOIE_SHADOW_MV==0u && TOOIE_SHADOW_ARGS==TOOIE_SHADOW_STATE+36u,
    "BK shadow helper set and unchanged 56-byte state prefix");
static_assert(TOOIE_BK_SHADOW_LIGHT_SLOTS==64u && TOOIE_BK_SHADOW_LAYOUT==2u,"BK shadow state layout2");
#if !defined(TOOIE_PLAYER_STATE_ADDRESS) || !defined(TOOIE_BK_SHADOW_FIXTURE_FRAME_BYTES)
#error Define the bridge state address and the measured helper frame bound from the selected receipt
#endif
extern "C" {
void func_800A89F8(uint8_t*, recomp_context*);
void func_800EB854(uint8_t*, recomp_context*);
void tooie_actor_palette_begin(uint8_t*, recomp_context*);
}
namespace stop {
struct Required : std::runtime_error { uint32_t site; explicit Required(uint32_t s):std::runtime_error("required stop"),site(s){} };
}
namespace candidate {
// The native import cannot return: throw before any guest continuation.
void tooie_bridge_required_stop(uint8_t*, recomp_context* c) { throw stop::Required(uint32_t(c->r4)); }
#include "bk_shadow_fixture.inc"
}
namespace {
constexpr uint32_t state=TOOIE_PLAYER_STATE_ADDRESS, bk=TOOIE_BK_STATE, bks=TOOIE_BK_SHADOW, bkf=TOOIE_BK_FLOAT;
constexpr uint32_t cs=TOOIE_CAMERA_STATE_VRAM, cc=cs+offsetof(TooieCameraState,control), ss=TOOIE_SHADOW_STATE;
constexpr uint32_t buffers=0x80127658u, slot_word=0x807F0000u, stack=0x80F00000u, guard=1024u;
constexpr uint32_t receiver=0x80250000u, descriptors=0x80132ED0u, alive=0x80135450u;
constexpr uint32_t frame_a=0x807FE000u, frame_b=0x807FD000u;   // producer activations (original sp)
constexpr uint32_t model=0x80600000u, pose=0x80610000u, header=0x80620000u, command=0x80630000u;
constexpr uint32_t gfx=0x80300000u, main_base=TOOIE_CAMERA_MAIN_VRAM;
static_assert(TOOIE_BK_SHADOW_FIXTURE_FRAME_BYTES<guard,"finite measured helper frames");
static_assert(stack+32u<=bk || stack-guard>=bk+sizeof(TooieBkState),"stack disjoint from BK state");
static_assert(stack+32u<=bks || stack-guard>=bks+sizeof(TooieBkShadow),"stack disjoint from BK shadow state");
static_assert(stack+32u<=state || stack-guard>=state+sizeof(TooieRenderBridgeState),"stack disjoint from bridge state");
// tooie_bk_state / tooie_bk_shadow (bk_pipeline.h); tooie_bk_float layout1 (bk_float.inc).
constexpr uint32_t f_depth=bk+offsetof(TooieBkState,draw_depth), f_id=bk+offsetof(TooieBkState,cur_drawn_model_transform_id);
constexpr uint32_t f_skip=bk+offsetof(TooieBkState,cur_drawn_model_skip_interpolation);
constexpr uint32_t f_offset=bk+offsetof(TooieBkState,cur_model_transform_id_offset), f_bones=bk+offsetof(TooieBkState,cur_model_uses_bones);
constexpr uint32_t f_roots=bk+offsetof(TooieBkState,root_depth), f_scope=bk+offsetof(TooieBkState,scope_depth);
constexpr uint32_t f_kind=bk+offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,kind);
constexpr uint32_t f_key=bk+offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,base_attachment);
constexpr uint32_t f_pending=bk+offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,own_draw_pending);
constexpr uint32_t f_mode0=bk+offsetof(TooieBkState,roots)+offsetof(TooieBkRootFrame,pushed_group);
constexpr uint32_t f_late=bk+offsetof(TooieBkState,diagnostics)+offsetof(TooieBkDiagnostics,late_closes);
constexpr uint32_t s_route=bks+offsetof(TooieBkShadow,route), s_serial=bks+offsetof(TooieBkShadow,serial);
constexpr uint32_t s_resets=bks+offsetof(TooieBkShadow,resets), s_evictions=bks+offsetof(TooieBkShadow,evictions);
constexpr uint32_t s_body=bks+offsetof(TooieBkShadow,body_frame), s_projection=bks+offsetof(TooieBkShadow,projection);
constexpr uint32_t s_open=bks+offsetof(TooieBkShadow,projection_open), s_untagged=bks+offsetof(TooieBkShadow,untagged);
constexpr uint32_t s_lights=bks+offsetof(TooieBkShadow,lights);
constexpr uint32_t light_handle(uint32_t i){return s_lights+i*sizeof(TooieBkLightSlot)+offsetof(TooieBkLightSlot,handle);}
constexpr uint32_t light_birth(uint32_t i){return s_lights+i*sizeof(TooieBkLightSlot)+offsetof(TooieBkLightSlot,birth);}
constexpr uint32_t k_frame=bkf+64u, k_skinned=bkf+72u, k_velocities=bkf+76u, k_pos=bkf+16640u, k_vel=bkf+16640u+2u*65536u*4u;
// ShadowState prefix (shadow_state.h): composite +24, mode_open +28, fault +32, args +36.
constexpr uint32_t p_composite=ss+24u, p_mode=ss+28u, p_fault=ss+32u;
// Independent restatement of the shadow ID layout (bk_shadow_attach.inc): slot x 66 routes x 4 ordinals.
constexpr uint32_t sid(uint32_t slot,uint32_t route,uint32_t ordinal,uint32_t view,uint32_t phase){
    return 0x2001C000u+((slot*66u+route)*4u+ordinal)*7168u+(view*7u+phase)*256u;
}
static_assert(sid(8,0,0,0,0)==0x20E8C000u,"shadow domain end");
constexpr uint32_t fallback=0u, blob=1u;
constexpr uint32_t light_route(uint32_t route_slot){return 2u+route_slot;}
constexpr uint32_t shadow_kind=3u;

uint32_t view_value=1, phase_value=5;
unsigned palette_calls=0;
std::vector<uint8_t> ram;
gpr guest(uint32_t a){return S32(a);}
uint32_t low(gpr a){return uint32_t(a);}
uint32_t word(uint8_t* rdram,uint32_t a){return uint32_t(MEM_W(0,guest(a)));}
void put(uint8_t* rdram,uint32_t a,uint32_t v){MEM_W(0,guest(a))=v;}
uint8_t byte(uint8_t* rdram,uint32_t a){return uint8_t(MEM_BU(0,guest(a)));}
void put_byte(uint8_t* rdram,uint32_t a,uint8_t v){MEM_B(0,guest(a))=v;}
void put_f32(uint8_t* rdram,uint32_t a,float v){uint32_t w;std::memcpy(&w,&v,4);put(rdram,a,w);}
float f32_at(uint8_t* rdram,uint32_t a){uint32_t w=word(rdram,a);float v;std::memcpy(&v,&w,4);return v;}
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void clobber(recomp_context* c){
    c->r1=0x91929394;c->r2=0xA1A2A3A4;c->r3=0xB1B2B3B4;c->r4=0xC1C2C3C4;c->r5=0xD1D2D3D4;c->r6=0xE1E2E3E4;
    c->r7=0xF1F2F3F4;c->r8=0x81828384;c->r9=0x82838485;c->r10=0x83848586;c->r11=0x84858687;c->r12=0x85868788;
    c->r13=0x86878889;c->r14=0x8788898A;c->r15=0x88898A8B;c->r24=0x898A8B8C;c->r25=0x8A8B8C8D;c->hi=1;c->lo=2;
}
recomp_context context(){
    recomp_context c{};c.r29=guest(stack);c.r31=guest(0x8071C2C4);
    c.r16=16;c.r17=17;c.r18=18;c.r19=19;c.r20=20;c.r21=21;c.r22=22;c.r23=23;c.r28=28;c.r30=30;
    c.f20.u64=20;c.f21.u64=21;c.f22.u64=22;c.f23.u64=23;c.f24.u64=24;c.f25.u64=25;c.f26.u64=26;
    c.f27.u64=27;c.f28.u64=28;c.f29.u64=29;c.f30.u64=30;c.f31.u64=31;return c;
}
void saved(const recomp_context& c,const recomp_context& b){
    require(c.r16==b.r16&&c.r17==b.r17&&c.r18==b.r18&&c.r19==b.r19&&c.r20==b.r20&&c.r21==b.r21&&
        c.r22==b.r22&&c.r23==b.r23&&c.r28==b.r28&&c.r30==b.r30&&c.r29==b.r29&&c.r31==b.r31,"callee-saved GPR/SP/RA");
    require(c.f20.u64==20&&c.f21.u64==21&&c.f22.u64==22&&c.f23.u64==23&&c.f24.u64==24&&c.f25.u64==25&&
        c.f26.u64==26&&c.f27.u64==27&&c.f28.u64==28&&c.f29.u64==29&&c.f30.u64==30&&c.f31.u64==31,"callee-saved FPR");
}
// One actual helper call: o32 arguments a0..a3 plus the fifth stack word, stack canaries around it.
uint32_t call(uint8_t* rdram,recomp_func_t* fn,uint32_t a0=0,uint32_t a1=0,uint32_t a2=0,uint32_t a3=0,uint32_t a4=0){
    for(uint32_t a=stack-guard;a<stack+32;++a)put_byte(rdram,a,0xA5);
    put(rdram,stack+0x10,a4);
    auto c=context();c.r4=guest(a0);c.r5=guest(a1);c.r6=guest(a2);c.r7=guest(a3);const auto before=c;
    fn(rdram,&c);saved(c,before);
    for(uint32_t a=stack-guard;a<stack-TOOIE_BK_SHADOW_FIXTURE_FRAME_BYTES;++a)require(byte(rdram,a)==0xA5,"write below the measured frames");
    for(uint32_t a=stack+0x14;a<stack+32;++a)require(byte(rdram,a)==0xA5,"write above the argument homes");
    return low(c.r2);
}
uint8_t* fresh(){
    ram.assign(16*1024*1024,0);uint8_t* rdram=ram.data();view_value=1;phase_value=5;palette_calls=0;
    put(rdram,bk,TOOIE_BK_MAGIC);put(rdram,bk+4,TOOIE_BK_LAYOUT);put(rdram,bks,TOOIE_BK_SHADOW_MAGIC);put(rdram,bks+4,TOOIE_BK_SHADOW_LAYOUT);
    // Shadow actor: descriptor slot 7 with live creation generation 5; owner slot 3 (u16 +0x76 >> 7).
    put(rdram,receiver,descriptors+7u*0x30u);put(rdram,state+offsetof(TooieRenderBridgeState,births)+7u*4u,5);
    put_byte(rdram,alive,0x01);MEM_H(0x76,guest(receiver))=int16_t(3u<<7);
    put(rdram,buffers,gfx);
    call(rdram,candidate::tooie_bk_frame_begin);
    return rdram;
}
void next_frame(uint8_t* rdram){call(rdram,candidate::tooie_bk_frame_begin);}
void route(uint8_t* rdram,uint32_t local,uint32_t handle=0){call(rdram,candidate::tooie_bk_shadow_route,local,handle);}
void begin(uint8_t* rdram,uint32_t frame,uint32_t seam){call(rdram,candidate::tooie_bk_shadow_begin,receiver,frame,seam);}
void end(uint8_t* rdram,uint32_t frame){call(rdram,candidate::tooie_bk_shadow_end,frame);}
void born(uint8_t* rdram,uint32_t handle){call(rdram,candidate::tooie_bk_shadow_light_event,handle);}
void reset(uint8_t* rdram){call(rdram,candidate::tooie_bk_shadow_light_event,0);}
void idle(uint8_t* rdram,const char* why){
    require(!word(rdram,f_id)&&word(rdram,f_kind)==TOOIE_BK_OWNER_NONE&&!word(rdram,f_scope)&&!word(rdram,f_skip)&&
        !word(rdram,f_depth)&&!word(rdram,s_open)&&!word(rdram,s_projection),why);
}
// An owner-slot sample this frame: the scope's ID, skip and projection after begin, then closed.
struct Sample { uint32_t id, skip, projection; };
Sample sample(uint8_t* rdram,uint32_t frame,uint32_t seam){
    begin(rdram,frame,seam);
    require(word(rdram,f_kind)==shadow_kind&&word(rdram,f_key)==frame&&word(rdram,f_pending)==1u&&word(rdram,s_open)==1u,
        "shadow scope keyed by the producer frame, its direct DE448 pending");
    const Sample s{word(rdram,f_id),word(rdram,f_skip),word(rdram,s_projection)};
    end(rdram,frame);idle(rdram,"the shadow end restores the outer context");
    return s;
}
Sample light_sample(uint8_t* rdram,uint32_t handle){route(rdram,1,handle);return sample(rdram,frame_a,0);}
// A frame-1 fallback sample establishes the receiver birth so later frames show only light effects.
uint8_t* established(){uint8_t* rdram=fresh();route(rdram,0);sample(rdram,frame_a,0);next_frame(rdram);return rdram;}

unsigned shadow_project_scope_cases(){
    uint8_t* rdram=established();route(rdram,0);begin(rdram,frame_a,0);
    call(rdram,candidate::tooie_bk_de448_begin,receiver+4,model);
    const uint32_t projection=word(rdram,s_projection);
    require(projection!=0 && !word(rdram,f_skip),"established shadow starts interpolating");
    // A direct nested DE498 does not pass DE448 again: draw depth and owner ID stay
    // unchanged, while its enclosing root is open. Unsupported nested geometry must
    // neither decide the enclosing body's policy nor stage its branch history.
    put(rdram,f_roots,1);
    std::vector<uint32_t> before;
    for(uint32_t o=offsetof(TooieBkState,shadow_cuts);o<sizeof(TooieBkState);o+=4)
        before.push_back(word(rdram,bk+o));
    require(call(rdram,candidate::tooie_bk_shadow_project,model)==projection,
        "direct nested root preserves enclosing shadow projection");
    require(!word(rdram,f_skip),"direct nested root cannot snap enclosing shadow body");
    for(uint32_t o=offsetof(TooieBkState,shadow_cuts),i=0;o<sizeof(TooieBkState);o+=4,++i)
        require(word(rdram,bk+o)==before[i],"direct nested root cannot stage body history or change its latch");
    put(rdram,f_roots,0);
    // Same owned draw at its actual top-level seam: the fixture has no canonical graph table, so the
    // preflight is unsupported. That proves no discontinuity: the continuous policy stays.
    const uint32_t declined=word(rdram,bk+offsetof(TooieBkState,shadow_declined));
    require(call(rdram,candidate::tooie_bk_shadow_project,model)==projection && !word(rdram,f_skip),
        "unsupported top-level shadow keeps its interpolated projection");
    require(!word(rdram,bk+offsetof(TooieBkState,shadow_body_snap)) &&
        word(rdram,bk+offsetof(TooieBkState,shadow_declined))==declined+1u,
        "unsupported is counted; nothing latched for this activation");
    call(rdram,candidate::tooie_bk_de448_end,0,buffers);end(rdram,frame_a);
    begin(rdram,frame_a,1);
    require(!word(rdram,f_skip) && word(rdram,s_projection)==sid(3,fallback,3,1,5),
        "same activation attachment keeps interpolating");
    end(rdram,frame_a);idle(rdram,"shadow witness scope restores owner state");
    return 3;
}
unsigned scope_cases(){
    unsigned cases=0;
    // S1: fallback body. A new receiver birth snaps the first frame (matrices, velocity and the
    // projection, which keeps its ID with SkipAll); the next frame interpolates under the same IDs.
    {   uint8_t* rdram=fresh();route(rdram,0);
        Sample a=sample(rdram,frame_a,0);
        require(a.id==sid(3,fallback,0,1,5)&&a.skip==1u&&a.projection==sid(3,fallback,2,1,5),"first sample of a receiver birth snaps under its kept projection ID");
        require(word(rdram,s_body)==frame_a,"a tagged body arms its activation's full-frame composite");
        next_frame(rdram);require(!word(rdram,s_body)&&!word(rdram,s_route),"frame begin clears route and composite key");
        route(rdram,0);Sample b=sample(rdram,frame_a,0);
        require(b.id==a.id&&!b.skip&&b.projection==sid(3,fallback,2,1,5),"continuous fallback body interpolates");
        ++cases;
    }
    // S2: two accepted local lights of one owner, view and phase, one of them above 64 (the allocator
    // has no count cap): each actual handle gets its own route slot, snapped when first assigned and
    // interpolating while it keeps the slot.
    {   uint8_t* rdram=established();
        Sample a=light_sample(rdram,3),b=light_sample(rdram,1000);
        route(rdram,0);Sample f=sample(rdram,frame_a,0);
        require(a.id==sid(3,light_route(0),0,1,5)&&b.id==sid(3,light_route(1),0,1,5)&&f.id==sid(3,fallback,0,1,5),
            "handles 3 and 1000 take route slots 0 and 1, distinct from fallback");
        require(a.skip==1u&&b.skip==1u&&!f.skip&&word(rdram,light_handle(1))==1000u,"a newly assigned light snaps");
        next_frame(rdram);
        Sample a2=light_sample(rdram,3),b2=light_sample(rdram,1000);
        require(a2.id==a.id&&b2.id==b.id&&!a2.skip&&!b2.skip&&b2.projection==sid(3,light_route(1),2,1,5),
            "the same handles keep their slots and interpolate");
        ++cases;
    }
    // S3: an allocation returning a tracked handle renews its birth and snaps that light through the
    // next frame; other lights and fallback are unaffected; an untracked handle changes nothing.
    {   uint8_t* rdram=established();light_sample(rdram,3);light_sample(rdram,1000);next_frame(rdram);
        born(rdram,3);born(rdram,77);
        require(word(rdram,s_serial)==2u&&word(rdram,light_birth(0))==1u&&!word(rdram,light_birth(1)),"birth serial on the returned handle");
        require(light_sample(rdram,3).skip==1u&&!light_sample(rdram,1000).skip,"the reborn handle snaps this frame, the other does not");
        route(rdram,0);require(!sample(rdram,frame_a,0).skip,"fallback is not a light sample");
        next_frame(rdram);require(light_sample(rdram,3).skip==1u,"and through the next frame");
        next_frame(rdram);Sample s=light_sample(rdram,3);
        require(!s.skip&&s.id==sid(3,light_route(0),0,1,5)&&s.projection==sid(3,light_route(0),2,1,5),"then interpolates again");
        ++cases;
    }
    // R1 correction: reuse of the same numeric handle after its old lifetime already drew in
    // this frame reserves that route. Exercise the actual event and scope helpers, both with a
    // spare route and with every route already emitted this frame (original-render fallback).
    for(uint32_t full=0;full<2;++full){
        uint8_t* rdram=established();
        light_sample(rdram,3);light_sample(rdram,1000);next_frame(rdram);
        const Sample old=light_sample(rdram,3),other=light_sample(rdram,1000);
        require(!old.skip&&!other.skip,"both pre-existing lights are continuous before rebirth");
        if(full)for(uint32_t h=100;h<162;++h)light_sample(rdram,h);
        const uint32_t drawn=word(rdram,s_lights+offsetof(TooieBkLightSlot,drawn_frame));
        born(rdram,3);
        require(!word(rdram,light_handle(0))&&!word(rdram,light_birth(0))&&
            word(rdram,s_lights+offsetof(TooieBkLightSlot,drawn_frame))==drawn&&drawn==word(rdram,k_frame),
            "same-frame rebirth invalidates the key but reserves its already-emitted route");
        if(!full){
            const Sample reborn=light_sample(rdram,3);
            require(reborn.id!=old.id&&reborn.id==sid(3,light_route(2),0,1,5)&&reborn.skip==1u&&reborn.projection==sid(3,light_route(2),2,1,5),
                "same-frame rebirth uses a new route and snaps matrix, projection and velocity policy");
            const Sample retained=light_sample(rdram,1000);
            require(retained.id==other.id&&!retained.skip&&retained.projection==other.projection,
                "same-frame rebirth leaves another continuous light unchanged");
        }else{
            const uint32_t untagged=word(rdram,s_untagged);
            route(rdram,1,3);begin(rdram,frame_a,0);
            require(!word(rdram,f_scope)&&!word(rdram,f_id)&&!word(rdram,s_open)&&
                word(rdram,s_untagged)==untagged+1u,
                "same-frame rebirth stays untagged when all routes are already emitted");
            end(rdram,frame_a);idle(rdram,"capacity refusal cannot leave a scope open");
        }
        next_frame(rdram);
        const Sample resumed=light_sample(rdram,3);
        require(resumed.skip==full,"a reserved route is reassigned snapped next frame, while a new held route can resume");
        ++cases;
    }
    // S4: a light vector reset (C90AC/C90D0/C90F4) ends every light lifetime: the next draw of a handle
    // is a new assignment and snaps, even under the same route slot.
    {   uint8_t* rdram=established();light_sample(rdram,3);next_frame(rdram);
        reset(rdram);require(word(rdram,s_resets)==1u&&!word(rdram,light_handle(0)),"reset empties the light table");
        Sample s=light_sample(rdram,3);
        require(s.skip==1u&&s.id==sid(3,light_route(0),0,1,5),"the same handle after a reset snaps");
        next_frame(rdram);require(!light_sample(rdram,3).skip,"then interpolates");
        ++cases;
    }
    // S5: 64 lights drawn in one frame fill every route slot; a 65th in the same frame stays untagged
    // (no two lights ever share an ID in one frame). Next frame it evicts the least recently drawn slot
    // and snaps; the evicted handle, drawn again, takes another slot and snaps too: no collision pairs.
    {   uint8_t* rdram=established();
        for(uint32_t h=100;h<164;++h)light_sample(rdram,h);
        const uint32_t untagged=word(rdram,s_untagged);
        route(rdram,1,164);begin(rdram,frame_a,0);
        require(!word(rdram,f_scope)&&word(rdram,s_untagged)==untagged+1u,"no free route slot within this frame");
        next_frame(rdram);
        Sample e=light_sample(rdram,164);
        require(e.skip==1u&&e.id==sid(3,light_route(0),0,1,5)&&word(rdram,s_evictions)==1u,"eviction of the oldest slot snaps");
        Sample r=light_sample(rdram,100);
        require(r.skip==1u&&r.id==sid(3,light_route(1),0,1,5)&&word(rdram,s_evictions)==2u,"the evicted handle snaps in a new slot");
        ++cases;
    }
    // S6: attachment after the body (ordinal 1, projection 3); a foreign frame and the skip leg are no-ops.
    {   uint8_t* rdram=established();route(rdram,0);sample(rdram,frame_a,0);
        begin(rdram,frame_a,1);
        require(word(rdram,f_id)==sid(3,fallback,1,1,5)&&word(rdram,s_projection)==sid(3,fallback,3,1,5),"attachment sample");
        end(rdram,frame_b);require(word(rdram,f_kind)==shadow_kind&&word(rdram,f_scope)==1u,"a foreign frame cannot close");
        end(rdram,frame_a);idle(rdram,"A66C4 closes");
        end(rdram,frame_a);idle(rdram,"the A6674 skip leg is a no-op");
        ++cases;
    }
    // S7: blob A5EA8 has its own route regardless of a pending projected route, and arms no composite.
    {   uint8_t* rdram=fresh();route(rdram,1,3);Sample s=sample(rdram,frame_b,2);
        require(s.id==sid(3,blob,0,1,5)&&!word(rdram,s_body),"blob route, no compositor key");
        ++cases;
    }
    // S8: refusals open nothing: open draw, open owner, dead receiver, owner slot 8, no route, handle 0.
    for(int which=0;which<6;++which){
        uint8_t* rdram=fresh();
        if(which==0)put(rdram,f_depth,1);
        if(which==1)put(rdram,f_kind,TOOIE_BK_OWNER_ACTOR);
        if(which==2)put(rdram,state+offsetof(TooieRenderBridgeState,births)+7u*4u,0);
        if(which==3)MEM_H(0x76,guest(receiver))=int16_t(8u<<7);
        if(which<4)route(rdram,0);
        if(which==5)route(rdram,1,0);
        const uint32_t untagged=word(rdram,s_untagged);
        begin(rdram,frame_a,0);
        require(!word(rdram,f_scope)&&!word(rdram,f_id)&&!word(rdram,s_open)&&!word(rdram,s_body),"a refused begin opens nothing");
        require(word(rdram,s_untagged)==untagged+(which>=2?1u:0u),"untagged draws are counted");
        end(rdram,frame_a);require(!word(rdram,f_scope),"the end after a refusal is inert");
        ++cases;
    }
    // S9: a DE448 nested in the shadow draw gets ID 0; the end cannot close over its open scope.
    {   uint8_t* rdram=fresh();route(rdram,0);begin(rdram,frame_a,0);
        require(!call(rdram,candidate::tooie_bk_de448_begin,receiver+4,model),"own draw: no nested token");
        const uint32_t token=call(rdram,candidate::tooie_bk_de448_begin,receiver+4,model);
        require(token==2u&&!word(rdram,f_id)&&word(rdram,f_kind)==shadow_kind,"nested draw under the shadow scope has ID 0");
        end(rdram,frame_a);require(word(rdram,f_scope)==2u,"the end leaves a nested scope alone");
        call(rdram,candidate::tooie_bk_de448_end,token,buffers);
        require(word(rdram,f_id)==sid(3,fallback,0,1,5)&&word(rdram,f_scope)==1u,"the nested close restores the shadow ID");
        call(rdram,candidate::tooie_bk_de448_end,0,buffers);end(rdram,frame_a);idle(rdram,"closed after nesting");
        ++cases;
    }
    return cases;
}

// Camera construction with an enabled MAIN list (no Enable command pending).
void camera(uint8_t* rdram,uint32_t epoch){
    put(rdram,cs+offsetof(TooieCameraState,owner_frame),stack);
    put(rdram,cc+offsetof(TooieCameraControl,epoch),epoch);put(rdram,cc+offsetof(TooieCameraControl,active),1);
    put(rdram,cc+offsetof(TooieCameraControl,main_base),main_base);put(rdram,cc+offsetof(TooieCameraControl,main_begin),main_base);
    put(rdram,cc+offsetof(TooieCameraControl,main_end),main_base+TOOIE_CAMERA_M_BYTES);
    put(rdram,cc+offsetof(TooieCameraControl,main_extra),8);
}
void group(uint8_t* rdram,uint32_t at,uint32_t id,uint32_t policy,const char* why){
    require(word(rdram,at)==0x6400000Cu&&word(rdram,at+4)==id&&word(rdram,at+8)==policy&&!word(rdram,at+12),why);
}
void segment(uint8_t* rdram,uint32_t at,uint32_t mode,uint32_t data,uint32_t base,const char* why){
    require(word(rdram,at)==0x64000031u&&word(rdram,at+4)==mode&&word(rdram,at+8)==data&&word(rdram,at+12)==base,why);
}
// A static header-bit40 model with one deforming vertex: original integer (1,2,3), deform point
// (4,5,6) through pose -1 = identity plus (10,20,30) -> float (14,25,36).
void bit40_model(uint8_t* rdram){
    MEM_H(0xA,guest(model))=0x40;put(rdram,model+0x28,0x100);
    const uint32_t deform=model+0x100;
    MEM_H(0,guest(deform))=0;MEM_H(2,guest(deform))=1;
    MEM_H(4,guest(deform))=-1;MEM_H(6,guest(deform))=1;MEM_H(8,guest(deform))=0;
    MEM_H(10,guest(deform))=4;MEM_H(12,guest(deform))=5;MEM_H(14,guest(deform))=6;
    MEM_H(16,guest(deform))=0;MEM_H(18,guest(deform))=-2;
    for(uint32_t i=0;i<16;++i)put_f32(rdram,pose+0x10u+4u*i,(i%5u)==0u?1.0f:0.0f);
    put_f32(rdram,pose+0x10u+48u,10.0f);put_f32(rdram,pose+0x10u+52u,20.0f);put_f32(rdram,pose+0x10u+56u,30.0f);
    MEM_H(0x16,guest(header))=2;MEM_H(24,guest(header))=1;MEM_H(26,guest(header))=2;MEM_H(28,guest(header))=3;
    put(rdram,command+4,header+24u);
    put(rdram,0x8012C964,header);put(rdram,0x8012C968,pose);put(rdram,0x8012C978,0);
}
// An admitted two-choice graph for the continuity preflight. It is separate from the deform
// data and from the unsupported fixtures: this test drives helpers, not the original graph body.
void admitted_shadow_graph(uint8_t* rdram){
    constexpr uint32_t table=0x8012306Cu, node=model+0x40u;
    put(rdram,0x8012CF8Cu,table);
    put(rdram,table+2u*4u,0x800DCF48u);put(rdram,table+3u*4u,0x800DD024u);put(rdram,table+10u*4u,0x800DD410u);
    put(rdram,table+12u*4u,0x800DD56Cu);put(rdram,table+17u*4u,0x800DD65Cu);
    put(rdram,table+24u*4u,0x800DD998u);
    put(rdram,model+4u,0x40u);put(rdram,node,12u);put(rdram,node+4u,0u);
    MEM_H(8,guest(node))=2;MEM_H(10,guest(node))=1;
    put(rdram,node+12u,0x20u);put(rdram,node+16u,0x30u);
    put(rdram,node+0x20u,24u);put(rdram,node+0x24u,0u);
    put(rdram,node+0x30u,24u);put(rdram,node+0x34u,0u);
    put(rdram,0x8012C854u,1u); // control 1 selects the first alternative
    put(rdram,0x8012C838u,1u);put_byte(rdram,0x8012CA11u,0u);
    put(rdram,0x80123030u,0x8010EF70u);put(rdram,0x80123034u,0x8010F070u);
    put(rdram,0x80123038u,0x8010F270u); // exact ordered appearance row 0
}
// Simulate the submission boundary, using the production pure commit and the native thunk's
// guest-word marshaling. Advancing construction alone must not publish staged history.
void submit_shadow_history(uint8_t* rdram){
    constexpr uint32_t base=bk+offsetof(TooieBkState,shadow_branches);
    constexpr uint32_t words=sizeof(TooieBkBranch)*TOOIE_BK_BRANCH_ENTRIES/4u;
    uint32_t raw[words];TooieBkBranch table[TOOIE_BK_BRANCH_ENTRIES];
    for(uint32_t i=0;i<words;++i)raw[i]=word(rdram,base+4u*i);
    std::memcpy(table,raw,sizeof(table));
    require(table[0].id==sid(3,fallback,0,1,5)&&table[0].staged_valid&&
        !table[0].committed_valid&&table[0].staged.model==model&&
        table[0].staged.count==1u&&table[0].staged.masks[0]==1u,
        "actual shadow preflight stages the admitted first alternative before submission");
    tooie_bk_branch_commit(table,word(rdram,bk+offsetof(TooieBkState,diagnostics)+offsetof(TooieBkDiagnostics,frames)));
    require(table[0].committed_valid&&table[0].committed.model==model&&
        table[0].committed.count==1u&&table[0].committed.masks[0]==1u,
        "submission commits the actual staged shadow signature");
    std::memcpy(raw,table,sizeof(table));
    for(uint32_t i=0;i<words;++i)put(rdram,base+4u*i,raw[i]);
}
// @tooie The owned silhouette grammar of 853/854 (execution-facts17, execution-facts37): a type-12 selector
// over type-3 DD024 display-list leaves, dispatched by the default table with entry 3. The earlier fixture
// modelled only type 12 over type 24, which hid that the shadow preflight rejected every real silhouette.
void silhouette_graph(uint8_t* rdram,uint32_t control){
    constexpr uint32_t table=0x8012306Cu, node=model+0x40u;
    put(rdram,0x8012CF8Cu,table);
    put(rdram,table+2u*4u,0x800DCF48u);put(rdram,table+3u*4u,0x800DD024u);put(rdram,table+10u*4u,0x800DD410u);
    put(rdram,table+12u*4u,0x800DD56Cu);put(rdram,table+17u*4u,0x800DD65Cu);put(rdram,table+24u*4u,0x800DD998u);
    put(rdram,model+4u,0x40u);put(rdram,node,12u);put(rdram,node+4u,0u);
    MEM_H(8,guest(node))=2;MEM_H(10,guest(node))=1;
    put(rdram,node+12u,0x20u);put(rdram,node+16u,0x30u);
    put(rdram,node+0x20u,3u);put(rdram,node+0x24u,0u);MEM_H(0x28,guest(node))=0;
    put(rdram,node+0x30u,3u);put(rdram,node+0x34u,0u);MEM_H(0x38,guest(node))=1;
    put(rdram,0x8012C854u,control);
    put(rdram,0x8012C838u,1u);put_byte(rdram,0x8012CA11u,0u);
    put(rdram,0x80123030u,0x8010EF70u);put(rdram,0x80123034u,0x8010F070u);put(rdram,0x80123038u,0x8010F270u);
}
// The A78E8 commit of the shadow table (bk_pipeline_thunks.hpp branch_commit_table marshaling).
void commit_shadow(uint8_t* rdram){
    constexpr uint32_t base=bk+offsetof(TooieBkState,shadow_branches);
    constexpr uint32_t words=sizeof(TooieBkBranch)*TOOIE_BK_BRANCH_ENTRIES/4u;
    uint32_t raw[words];TooieBkBranch table[TOOIE_BK_BRANCH_ENTRIES];
    for(uint32_t i=0;i<words;++i)raw[i]=word(rdram,base+4u*i);
    std::memcpy(table,raw,sizeof(table));
    tooie_bk_branch_commit(table,word(rdram,bk+offsetof(TooieBkState,diagnostics)+offsetof(TooieBkDiagnostics,frames)));
    std::memcpy(raw,table,sizeof(table));
    for(uint32_t i=0;i<words;++i)put(rdram,base+4u*i,raw[i]);
}
// One fallback body activation through its DEFB0 decision: A6664 begin, DE448 entry, DEFB0, DE448 exit, A666C end.
struct Projected { uint32_t projection, skip, snap; };
Projected project(uint8_t* rdram){
    route(rdram,0);begin(rdram,frame_a,0);call(rdram,candidate::tooie_bk_de448_begin,receiver+4,model);
    const uint32_t projection=call(rdram,candidate::tooie_bk_shadow_project,model);
    const Projected r{projection,word(rdram,f_skip),word(rdram,bk+offsetof(TooieBkState,shadow_body_snap))};
    call(rdram,candidate::tooie_bk_de448_end,0,buffers);end(rdram,frame_a);idle(rdram,"projected body closed");
    return r;
}
// Regression (every ordinary Banjo shadow snapped each construction): only a supported change against a valid
// committed baseline snaps; first sight, unsupported and no-baseline draws keep the continuous policy.
unsigned silhouette_cases(){
    unsigned cases=0;
    uint8_t* rdram=established();silhouette_graph(rdram,1u);
    const uint32_t p=sid(3,fallback,2,1,5);
    constexpr uint32_t f_reserved=bk+offsetof(TooieBkState,select_roots), f_declined=bk+offsetof(TooieBkState,select_declined);
    Projected a=project(rdram);
    require(a.projection==p&&!a.skip&&!a.snap,"first sight of a supported silhouette: no proven change");
    commit_shadow(rdram);next_frame(rdram);
    for(unsigned i=0;i<300u;++i){
        a=project(rdram);require(a.projection==p&&!a.skip,"300 stable type-3 silhouette constructions interpolate");
        commit_shadow(rdram);next_frame(rdram);
    }
    ++cases;
    put(rdram,0x8012C854u,2u);
    a=project(rdram);require(a.projection==p&&a.skip&&a.snap,"proven visible-branch change: SkipAll before the projection, ID kept");
    begin(rdram,frame_a,1);
    require(word(rdram,f_skip)==1u&&word(rdram,s_projection)==sid(3,fallback,3,1,5),"same-activation attachment follows the proven change");
    end(rdram,frame_a);idle(rdram,"attachment closed");
    commit_shadow(rdram);next_frame(rdram);
    ++cases;
    a=project(rdram);require(a.projection==p&&!a.skip,"committed change interpolates again");
    commit_shadow(rdram);next_frame(rdram);
    ++cases;
    put_byte(rdram,0x8012CA11u,2u);   // appearance row 2: not certified
    for(unsigned i=0;i<300u;++i){
        a=project(rdram);require(a.projection==p&&!a.skip&&!a.snap,"unsupported appearance keeps the continuous policy");
        commit_shadow(rdram);next_frame(rdram);
    }
    put_byte(rdram,0x8012CA11u,0u);put(rdram,0x8012C854u,1u);
    a=project(rdram);require(a.projection==p&&!a.skip,"no comparison across the unsupported workloads");
    commit_shadow(rdram);next_frame(rdram);
    ++cases;
    // Numbering: the owned silhouette root reserves its type-12 numbering on its own type-3 grammar...
    const uint32_t declined=word(rdram,f_declined);
    route(rdram,0);begin(rdram,frame_a,0);call(rdram,candidate::tooie_bk_de448_begin,receiver+4,model);
    put(rdram,slot_word,gfx+0x800u);call(rdram,candidate::tooie_bk_root_begin,buffers,slot_word,model);
    require(word(rdram,f_reserved)==1u&&word(rdram,f_declined)==declined,"owned silhouette root reserves its type-12 numbering");
    put(rdram,buffers,gfx+0x900u);call(rdram,candidate::tooie_bk_root_end,buffers);
    call(rdram,candidate::tooie_bk_de448_end,0,buffers);end(rdram,frame_a);idle(rdram,"silhouette root closed");
    ++cases;
    // ...while a PLAYER root of the same graph keeps the accepted five-type grammar and original numbering.
    put(rdram,f_kind,TOOIE_BK_OWNER_PLAYER);put(rdram,f_id,0x20000100u);
    put(rdram,slot_word,gfx+0xA00u);call(rdram,candidate::tooie_bk_root_begin,buffers,slot_word,model);
    require(!word(rdram,f_reserved)&&word(rdram,f_declined)==declined+1u,"PLAYER grammar unchanged: type 3 still declined");
    put(rdram,buffers,gfx+0xB00u);call(rdram,candidate::tooie_bk_root_end,buffers);
    put(rdram,f_kind,TOOIE_BK_OWNER_NONE);put(rdram,f_id,0);
    ++cases;
    return cases;
}
// One shadow body draw of a bit40 model through the actual BK helpers in original PC order:
// A6664 begin, DE448 entry, DE8C8 segment, DEEE4 dbanim capture, DEFB0 projection policy, DF0B0 root
// begin, (graph), DF238 join, DE448 exit, A666C end. Returns the DEFB0 cursor.
uint32_t body_draw(uint8_t* rdram,uint32_t defb0,uint32_t df0b0,uint32_t df238){
    begin(rdram,frame_a,0);
    call(rdram,candidate::tooie_bk_de448_begin,receiver+4,model);
    call(rdram,candidate::tooie_bk_skin_segment,slot_word,command);
    call(rdram,candidate::tooie_bk_skin_capture,buffers,slot_word,model);
    put(rdram,slot_word,defb0);
    call(rdram,candidate::tooie_shadow_de498,buffers,slot_word,model);
    put(rdram,f_offset,5);put(rdram,f_bones,1);   // enclosing traversal context, restored at DF238
    put(rdram,slot_word,df0b0);
    call(rdram,candidate::tooie_bk_root_begin,buffers,slot_word,model);
    require(word(rdram,f_mode0)==2u&&word(rdram,f_roots)==1u&&!word(rdram,f_offset)&&!word(rdram,f_bones),
        "bit40 root: BIT40 frame, traversal offset and bones reset like every root");
    put(rdram,buffers,df238);
    call(rdram,candidate::tooie_bk_root_join,buffers,slot_word,model);
    require(!word(rdram,f_roots)&&word(rdram,f_offset)==5u&&word(rdram,f_bones)==1u&&word(rdram,buffers)==df238+32u,
        "DF238 closes the bit40 frame once and restores the enclosing context");
    segment(rdram,df238,0,0,0,"position segment off at the join");segment(rdram,df238+16,2,0,0,"velocity segment off at the join");
    call(rdram,candidate::tooie_bk_de448_end,0,buffers);
    end(rdram,frame_a);idle(rdram,"body scope closed");
    require(!word(rdram,f_late),"no late close: the join balanced the frame");
    return defb0;
}


// Cross-workload consumer contract: RT64 projection matching compares IDs, while SKIP controls
// interpolation. Exercise actual emitted commands, not just the helper's returned ID.
unsigned projection_recovery_cases(){
    uint8_t* rdram=established();silhouette_graph(rdram,1u);camera(rdram,1u);
    const uint32_t body_id=sid(3,fallback,2,1,5), attachment_id=sid(3,fallback,3,1,5);
    auto emit=[&](uint32_t seam,uint32_t cursor,uint32_t id,uint32_t flags){
        begin(rdram,frame_a,seam);
        call(rdram,candidate::tooie_bk_de448_begin,receiver+4,model);
        put(rdram,slot_word,cursor);
        call(rdram,candidate::tooie_shadow_de498,buffers,slot_word,model);
        if(word(rdram,cursor+4)!=id || word(rdram,cursor+8)!=flags)
            std::cerr<<"recovery seam="<<seam<<" cursor="<<std::hex<<cursor<<" actual ID/policy="<<word(rdram,cursor+4)<<"/"<<word(rdram,cursor+8)<<" expected="<<id<<"/"<<flags<<std::dec<<"\n";
        group(rdram,cursor,id,flags,"identified projection recovery: exact emitted ID/policy");
        call(rdram,candidate::tooie_bk_de448_end,0,buffers);end(rdram,frame_a);
        idle(rdram,"projection recovery scope closed");
    };
    // First establish a submitted signature, then stable -> branch cut -> stable. Body and the
    // same activation's attachment keep distinct but individually unchanged projection IDs.
    route(rdram,0);emit(0,main_base+0x100u,body_id,0x0201154Au);commit_shadow(rdram);next_frame(rdram);
    for(uint32_t i=0;i<3u;++i){
        camera(rdram,1u);silhouette_graph(rdram,i?2u:1u);route(rdram,0);
        const uint32_t flags=i==1u?0x00000002u:0x0201154Au;
        emit(0,main_base+0x200u+i*0x80u,body_id,flags);
        emit(1,main_base+0x220u+i*0x80u,attachment_id,flags);
        commit_shadow(rdram);next_frame(rdram);
    }
    // Seed a previously detected sticky WORLD cut, not a synthetic model skip. Detection itself is
    // covered by the camera/boundary fixture. The real begin/helper must carry it into the policy.
    put(rdram,0x80127674u,1u);
    for(uint32_t i=0;i<3u;++i){
        const uint32_t epoch=20u+i;
        put(rdram,state+offsetof(TooieRenderBridgeState,epoch),epoch);camera(rdram,epoch);
        put(rdram,TOOIE_BK_CUT+32u+4u,epoch); // epoch[view1]
        put(rdram,TOOIE_BK_CUT+48u+4u,i==1u); // sticky[view1]
        route(rdram,0);
        const uint32_t flags=i==1u?0x00000002u:0x0201154Au;
        emit(0,main_base+0x500u+i*0x80u,body_id,flags);
        emit(1,main_base+0x520u+i*0x80u,attachment_id,flags);
        commit_shadow(rdram);next_frame(rdram);
    }
    // A camera refusal still overrides identified SKIP/live policy with IGNORE, independently of
    // retained shadow identity. This paid-list refusal consumes only its reserved fallback word.
    camera(rdram,30u);put(rdram,cc+offsetof(TooieCameraControl,latch),1u);route(rdram,0);
    emit(0,main_base+0x800u,0u,0x00000002u);
    return 5u; // body+attachment topology recovery, body+attachment WORLD recovery, refusal
}

unsigned root_cases(){
    unsigned cases=0;
    uint8_t* rdram=fresh();bit40_model(rdram);admitted_shadow_graph(rdram);camera(rdram,1);
    const uint32_t pos0=k_pos+(word(rdram,k_frame)&1u)*65536u*4u;
    // R1: first frame of the receiver birth. DEFB0 emits the surface projection snapped (its own ID,
    // every component SKIP); DF0B0 still reruns the executed deform in float (no modelview root group); no
    // velocity because the draw skips.
    route(rdram,0);
    body_draw(rdram,main_base+0x100u,gfx,gfx+0x100u);
    group(rdram,main_base+0x100u,sid(3,fallback,2,1,5),0x00000002u,"snapped surface projection keeps its ID (BK SkipAll)");
    segment(rdram,gfx,1,pos0,header+24u,"float positions published at the original vertex-0 base");
    segment(rdram,gfx+16,2,0,0,"no velocity on a snapped sample");
    require(word(rdram,gfx+32)!=0x6400000Cu,"no modelview root group for a bit40 root");
    require(f32_at(rdram,pos0)==14.0f&&f32_at(rdram,pos0+4)==25.0f&&f32_at(rdram,pos0+8)==36.0f,"executed deform rerun in float");
    require(word(rdram,k_skinned)==1u&&!palette_calls,"one skinned draw; the shadow-owned DEFB0 never asks the palette");
    ++cases;
    submit_shadow_history(rdram);
    // R2: the next submitted frame interpolates: SIMPLE surface policy with the owner sample's projection ID,
    // and explicit velocity against the exact previous frame of the same ID and model.
    next_frame(rdram);
    const uint32_t pos1=k_pos+(word(rdram,k_frame)&1u)*65536u*4u;
    route(rdram,0);
    body_draw(rdram,main_base+0x200u,gfx+0x200u,gfx+0x300u);
    group(rdram,main_base+0x200u,sid(3,fallback,2,1,5),0x0201154Au,"surface projection, coefficient interpolation");
    segment(rdram,gfx+0x200u,1,pos1,header+24u,"second-frame float positions");
    segment(rdram,gfx+0x210u,3,k_vel,header+24u,"explicit velocity published");
    require(f32_at(rdram,k_vel)==0.0f&&word(rdram,k_velocities)==1u,"zero velocity for an unchanged pose");
    ++cases;
    // R3: a DEFB0 draw that is not the shadow scope's own draw keeps the existing decision (here no
    // palette camera: IGNORE), both unowned and nested inside the shadow draw.
    put(rdram,slot_word,main_base+0x300u);
    call(rdram,candidate::tooie_shadow_de498,buffers,slot_word,model);
    group(rdram,main_base+0x300u,0,0x00000002u,"unowned bit40 draw: existing IGNORE");
    route(rdram,0);begin(rdram,frame_a,0);
    call(rdram,candidate::tooie_bk_de448_begin,receiver+4,model);
    const uint32_t token=call(rdram,candidate::tooie_bk_de448_begin,receiver+4,model);
    put(rdram,slot_word,main_base+0x310u);
    call(rdram,candidate::tooie_shadow_de498,buffers,slot_word,model);
    group(rdram,main_base+0x310u,0,0x00000002u,"nested bit40 draw is not the shadow sample");
    call(rdram,candidate::tooie_bk_de448_end,token,buffers);call(rdram,candidate::tooie_bk_de448_end,0,buffers);
    end(rdram,frame_a);idle(rdram,"closed");
    require(!palette_calls,"no palette admission without a WORLD camera");
    ++cases;
    return cases;
}

// Projected-shadow attachment root policy (SHADOW_FACTS 2-6). A6250 stages one singular flattening
// projector (DF540 mode3) for its body (A6664, bit40: DEFB0 projection, SHADOW_POLICY) and for its backpack
// (A66BC, header bit40 clear: the ordinary DF080 MODELVIEW LOAD/PUSH). The authenticated attachment's own
// top-level root takes the same coefficient (SIMPLE) mode, every other component unchanged. Ordinary PLAYER
// roots (the visible backpack included) and blob roots stay DECOMPOSE; bit40, skip and nested roots are unchanged.
// Packed gEXMatrixGroup word (rt64_extended_gbi.h), restated: push | mode<<2 | pos<<3 | rot<<5 | scale<<7 |
// skew<<9 | persp<<11 | vert<<13 | tile<<15 | order<<17 | edit<<19 | aspect<<20 | tc<<22 | lookat<<24.
constexpr uint32_t decompose_root=1u|1u<<2|1u<<3|1u<<5|1u<<7|1u<<9|1u<<11|1u<<15|1u<<22|2u<<24; // BK DecomposedNormalTcs
constexpr uint32_t simple_root=decompose_root&~(1u<<2), skip_root=1u;
static_assert(decompose_root==0x02408AADu&&simple_root==0x02408AA9u,"root group words");
constexpr uint32_t plain=0x80640000u;   // a header-bit40-clear model (zeroed RAM)
constexpr uint32_t f_attach=bk+offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,attachments);
constexpr uint32_t f_mode1=f_mode0+sizeof(TooieBkRootFrame);
// Camera construction, an established receiver birth (no snap) and the next BK frame.
uint8_t* ready(){uint8_t* rdram=fresh();camera(rdram,1);route(rdram,0);sample(rdram,frame_a,0);next_frame(rdram);return rdram;}
// DF0B0 at cursor `at` (no dbanim capture: no float segments); returns that root frame's mode.
uint32_t root_at(uint8_t* rdram,uint32_t at,uint32_t m,uint32_t mode_word){
    put(rdram,slot_word,at);call(rdram,candidate::tooie_bk_root_begin,buffers,slot_word,m);return word(rdram,mode_word);
}
// DF234 with the live b->gfx at `at`.
void close_root(uint8_t* rdram,uint32_t at){put(rdram,buffers,at);call(rdram,candidate::tooie_bk_root_end,buffers);}
// One shadow scope's own ordinary draw at `seam`: begin, DE448 entry, DF0B0, DF234, DE448 exit, end.
void shadow_draw(uint8_t* rdram,uint32_t seam,uint32_t id,uint32_t policy,const char* why){
    begin(rdram,frame_a,seam);call(rdram,candidate::tooie_bk_de448_begin,receiver+4,plain);
    require(root_at(rdram,gfx+0x600u,plain,f_mode0)==1u&&word(rdram,slot_word)==gfx+0x610u,"one modelview root group");
    group(rdram,gfx+0x600u,id,policy,why);
    close_root(rdram,gfx+0x700u);require(!word(rdram,f_roots),"DF234 closes the root");
    call(rdram,candidate::tooie_bk_de448_end,0,buffers);end(rdram,frame_a);idle(rdram,"the shadow scope closes");
}
unsigned attachment_root_cases(){
    unsigned cases=0;
    // A1/A2: the authenticated projected attachment, fallback and local-light routes.
    {   uint8_t* rdram=ready();route(rdram,0);
        shadow_draw(rdram,1,sid(3,fallback,1,1,5),simple_root,"fallback attachment root: coefficient interpolation");
        ++cases;
    }
    {   uint8_t* rdram=ready();light_sample(rdram,3);next_frame(rdram);route(rdram,1,3);
        shadow_draw(rdram,1,sid(3,light_route(0),1,1,5),simple_root,"light attachment root: coefficient interpolation");
        ++cases;
    }
    // A3: the blob (A5EA8) keeps the generic BK root.
    {   uint8_t* rdram=ready();
        shadow_draw(rdram,2,sid(3,blob,0,1,5),decompose_root,"blob root stays DECOMPOSE");
        ++cases;
    }
    // A4: an ordinary PLAYER root, the visible backpack attachment (ordinal 1) included, stays DECOMPOSE.
    {   uint8_t* rdram=fresh();camera(rdram,1);
        put(rdram,f_kind,TOOIE_BK_OWNER_PLAYER);put(rdram,f_attach,1u);put(rdram,f_depth,1u);put(rdram,f_id,0x20002100u);
        require(root_at(rdram,gfx+0x600u,plain,f_mode0)==1u,"player root group");
        group(rdram,gfx+0x600u,0x20002100u,decompose_root,"visible player backpack root stays DECOMPOSE");
        ++cases;
    }
    // A5: a snapped attachment (first frame of a receiver birth) keeps the SkipAll root.
    {   uint8_t* rdram=fresh();camera(rdram,1);route(rdram,0);
        shadow_draw(rdram,1,sid(3,fallback,1,1,5),skip_root,"skip path unchanged");
        ++cases;
    }
    // A6: a header-bit40 attachment keeps its no-modelview-group frame and closes at DF238.
    {   uint8_t* rdram=ready();bit40_model(rdram);route(rdram,0);begin(rdram,frame_a,1);
        call(rdram,candidate::tooie_bk_de448_begin,receiver+4,model);
        require(root_at(rdram,gfx+0x600u,model,f_mode0)==2u&&word(rdram,slot_word)==gfx+0x600u&&
            word(rdram,gfx+0x600u)!=0x6400000Cu,"bit40 attachment: no modelview root group");
        put(rdram,buffers,gfx+0x700u);call(rdram,candidate::tooie_bk_root_join,buffers,slot_word,model);
        require(!word(rdram,f_roots),"DF238 closes the bit40 root");
        call(rdram,candidate::tooie_bk_de448_end,0,buffers);end(rdram,frame_a);idle(rdram,"bit40 attachment closed");
        ++cases;
    }
    // A7: a root nested inside the attachment's own root does not inherit the attachment policy.
    {   uint8_t* rdram=ready();route(rdram,0);begin(rdram,frame_a,1);
        call(rdram,candidate::tooie_bk_de448_begin,receiver+4,plain);
        require(root_at(rdram,gfx+0x600u,plain,f_mode0)==1u,"outer attachment root");
        group(rdram,gfx+0x600u,sid(3,fallback,1,1,5),simple_root,"outer attachment root is SIMPLE");
        require(root_at(rdram,gfx+0x640u,plain,f_mode1)==1u&&word(rdram,f_roots)==2u,"nested root");
        group(rdram,gfx+0x640u,sid(3,fallback,1,1,5),decompose_root,"nested root keeps DECOMPOSE");
        close_root(rdram,gfx+0x700u);close_root(rdram,gfx+0x720u);require(!word(rdram,f_roots),"both roots close");
        call(rdram,candidate::tooie_bk_de448_end,0,buffers);end(rdram,frame_a);idle(rdram,"nested attachment closed");
        ++cases;
    }
    return cases;
}

unsigned compositor_cases(){
    unsigned cases=0;
    for(int which=0;which<2;++which){
        uint8_t* rdram=fresh();camera(rdram,1);
        call(rdram,candidate::tooie_shadow_reset,stack);
        route(rdram,0);begin(rdram,frame_a,0);end(rdram,frame_a);   // this activation's body was tagged
        const uint32_t a=call(rdram,candidate::tooie_shadow_rectangle_args,which?frame_b:frame_a,10,20,30,40);
        require(a==TOOIE_SHADOW_ARGS&&!word(rdram,s_body),"the composite key is consumed at A69D4");
        if(!which)require(word(rdram,a)==0&&word(rdram,a+4)==0&&word(rdram,a+8)==304u&&word(rdram,a+12)==228u&&
            word(rdram,p_composite)==1u,"tagged body: full 304x228 composite");
        else require(word(rdram,a)==10u&&word(rdram,a+4)==20u&&word(rdram,a+8)==30u&&word(rdram,a+12)==40u&&
            !word(rdram,p_composite),"another activation keeps its original rectangle");
        const uint32_t cursor=gfx+0x400u;
        for(uint32_t at=cursor;at<cursor+32;at+=4)put(rdram,at,0xA5A5A5A5u);
        uint32_t tail=call(rdram,candidate::tooie_shadow_mode_begin,cursor);
        if(!which){
            require(tail==cursor+16u&&word(rdram,cursor)==0x64000019u&&!word(rdram,cursor+4)&&
                word(rdram,cursor+8)==0xE3001001u&&!word(rdram,cursor+12)&&word(rdram,p_mode)==1u,"other-mode push and TLUT NONE");
            tail=call(rdram,candidate::tooie_shadow_mode_end,tail);
            require(tail==cursor+24u&&word(rdram,cursor+16)==0x6400001Au&&!word(rdram,cursor+20)&&!word(rdram,p_mode),
                "other-mode pop at A6180");
        }else require(tail==cursor&&word(rdram,cursor)==0xA5A5A5A5u,"no mode scope without a full-frame composite");
        call(rdram,candidate::tooie_shadow_complete,tail);
        require(!word(rdram,p_composite)&&!word(rdram,p_mode)&&!word(rdram,p_fault),"balanced completion");
        ++cases;
    }
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
    {   uint8_t* rdram=fresh();camera(rdram,1);route(rdram,0);begin(rdram,frame_a,0);end(rdram,frame_a);
        call(rdram,candidate::tooie_shadow_rectangle_args,frame_a,1,2,3,4);
        call(rdram,candidate::tooie_shadow_mode_begin,gfx+0x500u);
        bool stopped=false;
        try{call(rdram,candidate::tooie_shadow_complete,gfx+0x510u);}catch(const stop::Required& s){stopped=s.site==3u;}
        require(stopped&&word(rdram,p_fault)==1u&&word(rdram,p_mode)==1u,"an open mode push at A69DC stops before continuing");
        ++cases;
    }
#endif
    return cases;
}
// Menu bit40 projection (MENU_SOURCE_FINDING): DF944's direct DE498 at DEFB0 under the authenticated menu
// scope (tooie_bk_menu_model_begin's state, poked) takes its model ID displaced into the menu projection
// range with BK SimpleNormal (CAM_POLICY_WORLD), or SkipAll (CAM_POLICY_IGNORE) on the menu cut. Every other
// bit40 draw keeps the existing IGNORE (no saved WORLD camera here, so no palette admission).
static_assert(tooie_menu::first_id==TOOIE_BK_MENU_ID_START && tooie_menu::end_id==TOOIE_BK_MENU_ID_END &&
    tooie_menu::component_stride==TOOIE_BK_TRANSFORM_ID_COUNT && tooie_menu::components==2u,
    "guest menu domain is menu_scope.hpp's model/sprite layout");
unsigned menu_projection_cases(){
    unsigned cases=0;
    constexpr uint32_t f_owner_id=bk+offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,id);
    struct Case{uint32_t kind,owner,current,skip,depth,roots,id,policy;const char* why;};
    const Case table[]={
        {TOOIE_BK_OWNER_MENU,0x30000600u,0x30000600u,0,0,0,0x30080600u,0x02008AAAu,"menu model: displaced stable ID, SimpleNormal"},
        {TOOIE_BK_OWNER_MENU,0x30000600u,0x30000600u,1,0,0,0x30080600u,0x00000002u,"menu cut: the same ID, SkipAll"},
        {TOOIE_BK_OWNER_MENU,0x3005FE00u,0x3005FE00u,0,0,0,0x300DFE00u,0x02008AAAu,"last menu model block stays in range"},
        {TOOIE_BK_OWNER_MENU,0x30060400u,0x30060400u,0,1,0,0,0x00000002u,"transition kind4 mask draw: existing IGNORE"},
        {TOOIE_BK_OWNER_MENU,0x30060400u,0x30060400u,0,0,0,0,0x00000002u,"transition ID outside the menu domain: IGNORE"},
        {TOOIE_BK_OWNER_MENU,0x30000700u,0x30000700u,0,0,0,0,0x00000002u,"reserved sprite component: IGNORE"},
        {TOOIE_BK_OWNER_MENU,0x30000600u,0x30000601u,0,0,0,0,0x00000002u,"not the scope's own root ID: IGNORE"},
        {TOOIE_BK_OWNER_MENU,0x30000600u,0x30000600u,0,1,0,0,0x00000002u,"nested DE448 draw: IGNORE"},
        {TOOIE_BK_OWNER_MENU,0x30000600u,0x30000600u,0,0,1,0,0x00000002u,"inside an open root: IGNORE"},
        {TOOIE_BK_OWNER_NONE,0x30000600u,0x30000600u,0,0,0,0,0x00000002u,"menu-range ID without the menu owner: IGNORE"},
    };
    for(const Case& k:table){
        uint8_t* rdram=fresh();camera(rdram,1);
        put(rdram,f_kind,k.kind);put(rdram,f_owner_id,k.owner);put(rdram,f_id,k.current);
        put(rdram,f_skip,k.skip);put(rdram,f_depth,k.depth);put(rdram,f_roots,k.roots);
        put(rdram,slot_word,main_base+0x400u);
        call(rdram,candidate::tooie_shadow_de498,buffers,slot_word,model);
        group(rdram,main_base+0x400u,k.id,k.policy,k.why);
        require(word(rdram,slot_word)==main_base+0x410u&&!palette_calls,"one 16-byte group; the palette is never asked");
        ++cases;
    }
    return cases;
}
}
extern "C" void func_800A89F8(uint8_t*,recomp_context* c){clobber(c);c->r2=view_value;}
extern "C" void func_800EB854(uint8_t*,recomp_context* c){clobber(c);c->r2=phase_value;}
extern "C" void tooie_actor_palette_begin(uint8_t*,recomp_context* c){++palette_calls;clobber(c);c->r2=0;}
int main(){
    try{
        const unsigned recovery=projection_recovery_cases();require(recovery==5u,"all projection recovery cases ran");
        const unsigned project_scopes=shadow_project_scope_cases(); require(project_scopes==3,"all shadow project scope cases ran");
        const unsigned silhouettes=silhouette_cases(); require(silhouettes==6,"all silhouette regression cases ran");
        const unsigned scopes=scope_cases(),roots=root_cases(),attachments=attachment_root_cases(),compositor=compositor_cases(),menus=menu_projection_cases();
#ifdef TOOIE_REQUIRED_STOP_FIXTURE
        const unsigned expected_compositor=3;
#else
        const unsigned expected_compositor=2;
#endif
        require(scopes==16&&roots==3&&attachments==7&&compositor==expected_compositor&&menus==10u,"all BK shadow cases ran");
        std::cout<<"BK shadow actual helper fixture passed: scope_cases="<<scopes<<"; root_cases="<<roots<<"; attachment_root_cases="<<attachments
            <<"; compositor_cases="<<compositor<<"; menu_projection_cases="<<menus<<"; frame_bound="<<TOOIE_BK_SHADOW_FIXTURE_FRAME_BYTES
            <<"; DE448/DE498=NOT_RUN; light_allocator=NOT_RUN; RT64=NOT_RUN; runtime=NOT_RUN\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}

