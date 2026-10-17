// Unmodified extracted original and compiled candidate bodies; no DE448 stub.
#include "recomp.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
#include "camera_identity_layout.h"
#include "../tools/baseline/guest/camera-candidate/camera_identity.h"
#endif
#ifdef TOOIE_PLAYER_STATE_ADDRESS
#include "../tools/baseline/guest/silo-candidate/render_bridge.h"
#include <cstddef>
#endif
#ifndef TOOIE_DRAW_101808_FRAME_BYTES
#error Define candidate frame sizes from actual translated prologues
#endif
#ifndef TOOIE_DRAW_DE448_FRAME_BYTES
#error Define candidate frame sizes from actual translated prologues
#endif
extern "C" {
void func_800DF47C(uint8_t*, recomp_context*);
void func_801027F4(uint8_t*, recomp_context*);
void func_801018D8(uint8_t*, recomp_context*);
void func_8010347C(uint8_t*, recomp_context*);
void func_800A7180(uint8_t*, recomp_context*);
void func_800DE498(uint8_t*, recomp_context*);
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
void func_80104148(uint8_t*, recomp_context*);
void func_80104200(uint8_t*, recomp_context*);
#endif
}
namespace original {
void func_800DE448(uint8_t*, recomp_context*);
#include "original_101808_fixture.inc"
#include "original_de448_fixture.inc"
}
namespace candidate {
void func_800DE448(uint8_t*, recomp_context*);
#ifdef TOOIE_PLAYER_STATE_ADDRESS
void tooie_npc665_end_body(uint8_t*, recomp_context*);
void tooie_cpu625_retire(uint8_t*, recomp_context*);
void tooie_backpack621_retire(uint8_t*, recomp_context*);
// Trot is outside this drawing fixture's state. Its dedicated production-MIPS
// fixture checks retirement behavior; the lifecycle fixture checks call order.
void tooie_trot_toggle_retire(uint8_t*, recomp_context*) {}
#ifdef TOOIE_NPC665_FIXTURE
// Preserve the native checked-end contract using the exact selected guest body.
void baseline_npc665_end_checked(uint8_t* rdram, recomp_context* ctx) {
    tooie_npc665_end_body(rdram, ctx);
    if(ctx->r2) throw std::runtime_error("NPC665 guest paid-close ownership tripwire");
}
#endif
void tooie_player_begin(uint8_t*, recomp_context*);
void tooie_player_bone_push(uint8_t*, recomp_context*);
void tooie_player_bone_pop(uint8_t*, recomp_context*);
void tooie_player_end(uint8_t*, recomp_context*);
void tooie_actor_palette_end(uint8_t*, recomp_context*);
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
void tooie_camera_construction_begin(uint8_t*, recomp_context*);
void tooie_camera_construction_end(uint8_t*, recomp_context*);
void tooie_actor_palette_begin(uint8_t*, recomp_context*);
void func_8008B10C(uint8_t*, recomp_context*);
void anctrl_setSmoothTransition(uint8_t*, recomp_context*);
#endif
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
    TOOIE_DRAW_101808_FRAME_BYTES+TOOIE_DRAW_DE448_FRAME_BYTES<guard,"finite candidate frames");
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
int player_case=-1;
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
    const uint32_t depth=chain?actor_frame+draw_frame:draw_frame;
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
    if(which==11){put(rdram,state+12,1);put(rdram,state+16,2609);}
    auto c=context();c.r4=guest(actor+4);c.r5=guest(rotation);c.r6=0;c.r7=guest(extra);
    put(rdram,stack+0x10,player_model);const auto before=c;
    candidate::func_800DE448(rdram,&c);saved(c,before);
    require(low(c.r2)==returned,"player preserves original return");
    require(events.size()==2&&events[0].kind==Buffers&&events[1].kind==Draw,"player preserves original call chain");
    const bool refused=(which>=2&&which<=4)||which==7||which==9||which==11||which==13||which==14||(dragon_asset&&which==21);
    require(word(rdram,state+16)==(which==11?2609u:refused?0u:3792u),
        dragon_asset?"Dragon exact current pose admission/refusal and reservation":"complete player reservation debited exactly once");
    require(word(rdram,state+28)==0,"player releases active ownership");
    require(word(rdram,pc+15*4)==(refused?0u:1u),"admission count");
    if(refused){
        require(word(rdram,pc+offsetof(TooiePlayerBoundary,emitted_bones))==77,
            "refusal does not reset an unadmitted occurrence counter");
        if(which==3)require(word(rdram,pc+17*4)==(4u|(2u<<8)),
            "OVERRIDE retains reason4 and exact post-draw predicate bit");
        if(which==7)require(word(rdram,pc+17*4)==(4u|(64u<<8)),
            "OVERRIDE retains exact unsupported CPU type predicate bit");
        if(which==9)require(word(rdram,pc+17*4)==TOOIE_BRIDGE_MODEL_DATA,
            "full-width unsupported LOD asset refuses without low16 alias");
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
            require(word(rdram,state+16)==3792&&word(rdram,state+28)==0,"each occurrence prepays3792 and pops");
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
            require(word(rdram,state+16)==3792&&word(rdram,state+28)==0,"LOD switch retains full common prepay and unwind");
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
            require(word(rdram,state+16)==3792&&word(rdram,state+28)==0,
                "Dragon transitions preserve common prepay and paid unwind");
        }
    }
    if(which>=15&&!dragon_asset){
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
        palette_case=true; palette_controller_getters=0;
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
        for (uint32_t a=stack-guard;a<stack-TOOIE_ACTOR_PALETTE_BEGIN_FRAME_BYTES;++a)
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
            0x6400000C,camera,0x0201154E,0,0xDA380007,0x00410000};
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
        f.cursor=player_pool+0x8FC0-96; f.prepare(1);
        put(f.rdram,player_buffers+4,palette_mtx_pool+0x12C00-256);
        put(f.rdram,state+offsetof(TooieRenderBridgeState,seen_epoch),1);
        put(f.rdram,state+offsetof(TooieRenderBridgeState,extra_bytes),6368);
        const uint32_t root=f.begin(); f.admitted(root,1); f.finish(root);
        require(word(f.rdram,player_buffers)==player_pool+0x8FC0 &&
            f.field(offsetof(TooieRenderBridgeState,extra_bytes))==6400,"exact suffix/Mtx/actor-ledger boundaries admit and finish at end");
        ++groups;
    }
    for (uint32_t resource=0;resource<3;++resource) {
        PaletteFixture f;
        if (resource==0) { f.cursor=player_pool+0x8FC0-88; f.prepare(1); }
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
        put(f.rdram,state+offsetof(TooieRenderBridgeState,silo_depth),2);
        f.invoke(candidate::tooie_actor_palette_end,player_buffers);
        require(word(f.rdram,ac+offsetof(TooieActorPaletteContext,open_group))==1,"nested return cannot consume outer paid pop");
        put(f.rdram,state+offsetof(TooieRenderBridgeState,silo_depth),1);
        f.invoke(candidate::func_8008B10C,palette_controller,0);
        f.finish(root);
        require(f.field(offsetof(TooieRenderBridgeState,poisoned))==1 &&
            f.field(offsetof(TooieRenderBridgeState,tripwire_count))!=0,"late owner retirement trips but safe prepaid pop survives");
        ++groups;
    }
    {
        PaletteFixture f; const uint32_t root=f.begin(); f.admitted(root,1);
        put(f.rdram,player_buffers,f.cursor+25);
        f.invoke(candidate::tooie_actor_palette_end,player_buffers);
        require(word(f.rdram,player_buffers)==f.cursor+25 && word(f.rdram,f.cursor+24)==0 &&
            f.field(offsetof(TooieRenderBridgeState,poisoned))==1 &&
            f.field(offsetof(TooieRenderBridgeState,active))==0,"unsafe returned cursor suppresses pop and poisons");
        ++groups;
    }
    return groups;
}
#endif
}
extern "C" void func_800DF47C(uint8_t* rdram,recomp_context* c){
    events.push_back({Bind,{low(c->r4),low(c->r5)}});
    require(low(c->r4)==(current.zero_callback?0:callback)&&low(c->r5)==actor,"callback/actor fullword forwarding");
    put(rdram,actor,descriptor1);put(rdram,actor+0x38,second_scale);clobber(c);
}
extern "C" void func_801027F4(uint8_t* rdram,recomp_context* c){
    events.push_back({Rotation,{low(c->r4),word(rdram,actor)}});
    require(low(c->r4)==actor&&word(rdram,actor)==descriptor1,"rotation sees binding mutation");
    put(rdram,actor,descriptor2);put(rdram,actor+0x38,third_scale);put(rdram,rotation,0xC4C5C6C7);
    clobber(c);c->r2=guest(current.zero==1?0:rotation);
}
extern "C" void func_801018D8(uint8_t* rdram,recomp_context* c){
    events.push_back({Extra,{low(c->r4),word(rdram,actor)}});
    require(low(c->r4)==actor&&word(rdram,actor)==descriptor2,"extra sees rotation mutation");
    put(rdram,actor,descriptor3);put(rdram,actor+0x38,first_scale);put(rdram,extra,0xD5D6D7D8);
    clobber(c);c->r2=guest(current.zero==2?0:extra);
}
extern "C" void func_8010347C(uint8_t* rdram,recomp_context* c){
    events.push_back({Model,{low(c->r4)}});
    require(low(c->r4)==descriptor3,"Actor+0 read after both pointer helpers");
    put(rdram,actor+0x38,current.scale);clobber(c);c->r2=guest(model);
}
extern "C" void func_800A7180(uint8_t* rdram,recomp_context* c){
#ifdef TOOIE_PLAYER_STATE_ADDRESS
    if(player_case>=0){events.push_back({Buffers,{}});clobber(c);c->r2=guest(player_buffers);return;}
#endif
    events.push_back({Buffers,{}});put(rdram,actor+4,position_mutation);
    put(rdram,rotation,0xE6E7E8E9);put(rdram,extra,0xF7F8F9FA);
    clobber(c);c->r2=guest(current.zero==3?0:buffers);
}
extern "C" void func_800DE498(uint8_t* rdram,recomp_context* c){
#ifdef TOOIE_PLAYER_STATE_ADDRESS
    if(player_case>=0){
        require(low(c->r4)==player_buffers,"original body buffers forwarded");
        events.push_back({Draw,{}});
        uint32_t cursor=word(rdram,player_buffers);
        if(player_case==5)put(rdram,player_buffers,cursor+1);
        else if(player_case!=1){put(rdram,cursor,0xDF123456);put(rdram,cursor+4,0x789ABCDE);put(rdram,player_buffers,cursor+8);}
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
        clobber(c); c->r2=guest(palette_pose); return;
    }
#endif
    require(player_case>=0&&low(c->r4)==1,"typed signed pose resolver argument");
    clobber(c);c->r2=guest(player_pose);
}
extern "C" void func_800A89F8(uint8_t*,recomp_context* c){
    require(player_case>=0,"unexpected view getter in unbound differential");clobber(c);c->r2=0;
}
extern "C" void func_800EB854(uint8_t*,recomp_context* c){
    require(player_case>=0,"unexpected phase getter in unbound differential");clobber(c);c->r2=0;
}
#endif
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
// Deliberately explicit getter doubles: matching original offsets/arguments does
// not establish execution of the original getters (separately source-reviewed).
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
int main(){
    try{
        unsigned comparisons=0,player_cases=0,palette_groups=0;
        for(uint32_t bits:{0x3FC12345u,0u,0x80000000u,0x7FC12345u,0x7F812345u}){
            compare(true,{bits,0,false});compare(false,{bits,0,false});comparisons+=2;
        }
        for(int zero=1;zero<=4;++zero){compare(true,{0xC1234567u,zero,false});++comparisons;}
        compare(true,{0xC1234567u,0,true});++comparisons;
        compare(false,{0xC1234567u,3,false});compare(false,{0xC1234567u,4,false});comparisons+=2;
#ifdef TOOIE_PLAYER_STATE_ADDRESS
        for(int which=0;which<20;++which){player_fixture(which);++player_cases;}
        for(uint32_t asset:{0x838u,0x839u})for(int which:{20,21,22}){
            player_fixture(which,asset,asset==0x838u?60u:58u);++player_cases;
        }
#endif
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
        camera_epoch_fixtures();
        palette_groups=palette_fixtures();
#endif
        std::cout<<"Draw boundary actual differential fixture passed: "<<comparisons
            <<" comparisons; player_cases="<<player_cases
            <<"; palette_groups="<<palette_groups
            <<"; candidate frames="<<TOOIE_DRAW_101808_FRAME_BYTES<<','<<TOOIE_DRAW_DE448_FRAME_BYTES;
#ifdef TOOIE_PLAYER_STATE_ADDRESS
        std::cout<<"; player_state="<<state<<"; state_bytes="<<TOOIE_BRIDGE_STATE_BYTES
            <<"; player_frames="<<TOOIE_PLAYER_BEGIN_FRAME_BYTES<<','<<TOOIE_PLAYER_END_FRAME_BYTES;
#endif
#ifdef TOOIE_ACTOR_PALETTE_FIXTURE
        std::cout<<"; palette_frames="<<TOOIE_ACTOR_PALETTE_BEGIN_FRAME_BYTES<<','<<TOOIE_ACTOR_PALETTE_END_FRAME_BYTES
            <<"; getter_boundaries=DOUBLED; original_tail=MODELED; loop_timer_store=MODELED";
#endif
        std::cout<<'\n';
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
