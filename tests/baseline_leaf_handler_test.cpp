// Differential fixture: exact raw original versus compiled/retranslated guest C.
// The integrator supplies both includes; never replace either with a C model.
#include "recomp.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#ifdef TOOIE_PLAYER_STATE_ADDRESS
#include "../tools/baseline/guest/silo-candidate/render_bridge.h"
extern "C" {
void tooie_backpack621_leaf_begin(uint8_t*, recomp_context*);
void tooie_backpack621_leaf_end(uint8_t*, recomp_context*);
void tooie_npc665_leaf_begin(uint8_t*, recomp_context*);
void tooie_npc665_leaf_end(uint8_t*, recomp_context*);
}
#include "player_leaf_helpers_fixture.inc"
#endif
#ifdef TOOIE_BK_LEAF_FIXTURE
// BK selection: DD998 calls the BK list node instead of the row leaf labels (bk_graph.inc).
#include "bk_pipeline_selection_layout.h"
static_assert(TOOIE_BK_PIPELINE_SELECTED==1u,"BK leaf fixture requires the BK image selection");
#endif
#ifndef TOOIE_DD998_FRAME_BYTES
#error Define TOOIE_DD998_FRAME_BYTES from the candidate object prologue
#endif
#ifndef TOOIE_TEST_PLAYER_PHYSICAL_BYTES
#error "Supply the selected composition's physical player-list extent"
#endif
static_assert(TOOIE_TEST_PLAYER_PHYSICAL_BYTES==0x8FC0u || TOOIE_TEST_PLAYER_PHYSICAL_BYTES==0x5BEC0u,
    "player physical extent is one of the two supported bridge selections");
#ifndef TOOIE_DD998_HELPER_FRAME_BYTES
#define TOOIE_DD998_HELPER_FRAME_BYTES 0
#endif
#ifdef TOOIE_BK_LEAF_FIXTURE
#ifndef TOOIE_BK_LEAF_HELPER_FRAME_BYTES
#error Define TOOIE_BK_LEAF_HELPER_FRAME_BYTES from the bk object frames (list_begin + set_model_matrix_group)
#endif
// Under BK the candidate's deepest callee chain is DD998 -> list_begin -> set_model_matrix_group.
#define TOOIE_DD998_CANDIDATE_FRAME_BYTES (TOOIE_DD998_FRAME_BYTES+TOOIE_BK_LEAF_HELPER_FRAME_BYTES)
#else
#define TOOIE_DD998_CANDIDATE_FRAME_BYTES (TOOIE_DD998_FRAME_BYTES+TOOIE_DD998_HELPER_FRAME_BYTES)
#endif
extern "C" {
void mlMtx_get_stack_pointer(uint8_t*, recomp_context*);
void func_80018BB4(uint8_t*, recomp_context*);
void mlMtxApply(uint8_t*, recomp_context*);
void func_8010F070(uint8_t*, recomp_context*);
void osVirtualToPhysical_recomp(uint8_t*, recomp_context*);
void osVirtualToPhysical(uint8_t*, recomp_context*);
}
#define func_800DD998 original_DD998
#include "dd998_original_fixture.inc"
#undef func_800DD998
#include "dd998_candidate_fixture.inc"
namespace {
constexpr uint32_t buffers=0x80010000, node=0x80011000, gfx=0x80020000;
constexpr uint32_t mtx=0x80030000, current=0x80040040, parent=0x80040000;
constexpr uint32_t lists=0x80050000, stack=0x8007F000, guard=512;
constexpr uint32_t callback_global=0x8012CEC4, lists_global=0x8012C960;
constexpr uint32_t callback_a=0x8010E4C4, callback_b=0x8010E4C8;
static_assert(TOOIE_DD998_FRAME_BYTES>0 && TOOIE_DD998_FRAME_BYTES<guard);
enum Kind { Current, Parent, Apply, Transform, CallbackA, CallbackB, Physical };
// Plain: no owner/label state. Row: the selected row player leaf bracket (row composition only).
// BkOwned: BK owner ID set, no enclosing float root. BkFloat: also inside a float-enabled root.
enum Mode { Plain, Row, BkOwned, BkFloat };
struct Event {
    Kind kind; uint32_t matrix; int32_t count, position;
    bool operator==(const Event& b) const {
        return kind==b.kind && matrix==b.matrix && count==b.count && position==b.position;
    }
};
struct Case {
    std::array<int16_t,4> indices;
    std::array<int16_t,3> current_counts, parent_counts;
    int16_t start, flags;
    bool callback, mutate_transform, replace_callback, move_cursors;
};
std::vector<Event> events;
Case active{};
uint32_t transform_position, callback_position;
unsigned transforms, callbacks, physicals;
void require(bool value, const char* why) { if(!value) throw std::runtime_error(why); }
gpr guest(uint32_t p) { return S32(p); }
uint32_t word(uint8_t* rdram,uint32_t p) { return uint32_t(MEM_W(0,guest(p))); }
void put(uint8_t* rdram,uint32_t p,uint32_t v) { MEM_W(0,guest(p))=v; }
void half(uint8_t* rdram,uint32_t p,int16_t v) { MEM_H(0,guest(p))=v; }
#ifdef TOOIE_BK_LEAF_FIXTURE
// tooie_bk_state layout1 (bk_pipeline.h) and tooie_bk_float layout1 (bk_float.inc) offsets.
constexpr uint32_t bk_state=TOOIE_BK_STATE, bk_float=TOOIE_BK_FLOAT;
constexpr uint32_t bk_id=0x20000000u, bk_offset=3u;                   // player own sample 0
constexpr uint32_t seg_pos=0x80060000u, seg_vel=0x80068000u, seg_base=0x80070018u;
// SIMPLE (bones in use), PUSH, MODELVIEW; position/rotation/scale/skew/perspective INTERPOLATE;
// vertices SKIP; tile INTERPOLATE; LINEAR; no edit; aspect AUTO; texcoords SKIP; look-at AUTO.
constexpr uint32_t bk_bone_policy=0x02008AA9u;
#endif
void clobber(recomp_context* c) {
    c->r1=1;c->r2=2;c->r3=3;c->r4=4;c->r5=5;c->r6=6;c->r7=7;
    c->r8=8;c->r9=9;c->r10=10;c->r11=11;c->r12=12;c->r13=13;
    c->r14=14;c->r15=15;c->r24=24;c->r25=25;c->hi=0x1234;c->lo=0x5678;
}
void process(Kind kind,uint8_t* rdram,recomp_context* c) {
    const uint32_t matrix=uint32_t(c->r4), position=uint32_t(c->r5);
    const int32_t count=int32_t(c->r6);
    uint32_t& saved=kind==Transform?transform_position:callback_position;
    if(!saved) saved=position;
    require(saved==position,"one persistent position local per processing path");
    require(!transform_position || !callback_position || transform_position!=callback_position,
            "transform and callback positions must be independent");
    events.push_back({kind,matrix,count,int32_t(word(rdram,position))});
    put(rdram,position,word(rdram,position)+uint32_t(count));
    if(kind==Transform && active.mutate_transform && transforms++==0) {
        // Second count is read after the first call; next loop reloads flags.
        half(rdram,node+0x14,9);half(rdram,node+0x2E,2);
        put(rdram,callback_global,callback_a);
    }
    if(kind!=Transform && active.replace_callback && callbacks++==0) {
        // Original reloads CEC4 for the parent call, not just once per leaf.
        put(rdram,callback_global,callback_b);half(rdram,node+0x14,-5);
    }
    clobber(c);
}
void callback_one(uint8_t* r,recomp_context* c) { process(CallbackA,r,c); }
void callback_two(uint8_t* r,recomp_context* c) { process(CallbackB,r,c); }
struct Result { std::vector<uint8_t> ram; std::vector<Event> trace; };
// Bytes each leaf member emits, and where its original DE sits within them.
uint32_t member_bytes(Mode mode) { return mode==Plain?16u:mode==BkFloat?112u:48u; }
uint32_t de_offset(Mode mode) { return mode==Plain?0u:mode==BkFloat?56u:24u; }
Result run(const Case& test,recomp_func_t* function,unsigned frame,Mode mode=Plain) {
    Result result{std::vector<uint8_t>(16*1024*1024),{}};
    auto* rdram=result.ram.data();active=test;events.clear();
    transform_position=callback_position=0;transforms=callbacks=physicals=0;
    put(rdram,buffers,gfx);put(rdram,buffers+4,mtx);
    put(rdram,lists_global,lists);put(rdram,callback_global,test.callback?callback_a:0);
#ifdef TOOIE_PLAYER_STATE_ADDRESS
    constexpr uint32_t state=TOOIE_PLAYER_STATE_ADDRESS, pc=state+18740;
    if(mode==Row) {
        put(rdram,state+8,1);put(rdram,state+16,3792);put(rdram,state+28,1);
        put(rdram,state+18584,1);put(rdram,pc,1);put(rdram,pc+28,1);
        put(rdram,pc+32,0x90000000);put(rdram,pc+36,buffers);
        put(rdram,pc+40,gfx-0x100);put(rdram,pc+48,gfx-24);put(rdram,pc+96,3);
        put(rdram,pc+88,0x62A); // Effective model has 68 poses; required by real leaf helper.
        put(rdram,0x8007BCD0,gfx-0x100);half(rdram,0x8007BEDA,0);
    }
#endif
#ifdef TOOIE_BK_LEAF_FIXTURE
    if(mode==BkOwned || mode==BkFloat) {
        // Owned player draw inside a boned model: ID, traversal offset and bones flag.
        put(rdram,bk_state+52,bk_id);put(rdram,bk_state+60,bk_offset);put(rdram,bk_state+64,1);
        put(rdram,bk_state+68,mode==BkFloat?1u:0u);                      // root_depth
        if(mode==BkFloat) {
            // Enclosing root 0 published its own POSITION/VELOCITY segments.
            put(rdram,bk_float+128,seg_pos);put(rdram,bk_float+132,seg_vel);
            put(rdram,bk_float+136,seg_base);put(rdram,bk_float+140,1);
        }
    }
#endif
    for(unsigned i=0;i<4;++i) half(rdram,node+8+2*i,test.indices[i]);
    for(unsigned i=0;i<3;++i) {
        half(rdram,node+0x20+2*i,test.current_counts[i]);
        half(rdram,node+0x14+2*i,test.parent_counts[i]);
    }
    half(rdram,node+0x2C,test.start);half(rdram,node+0x2E,test.flags);
    for(uint32_t p=gfx-16;p<gfx+128;p+=4) put(rdram,p,0xCCCCCCCC);
    for(uint32_t p=mtx-16;p<mtx+160;p+=4) put(rdram,p,0xDDDDDDDD);
    for(uint32_t p=stack-guard;p<stack+32;++p) MEM_B(0,guest(p))=0xA5;
    recomp_context c{};c.r4=guest(buffers);c.r5=guest(node);c.r29=guest(stack);
    c.r31=guest(0x80045678);c.r16=16;c.r17=17;c.r18=18;c.r19=19;
    c.r20=20;c.r21=21;c.r22=22;c.r23=23;c.r28=28;c.r30=30;
    c.f20.u64=20;c.f21.u64=21;c.f22.u64=22;c.f23.u64=23;
    c.f24.u64=24;c.f25.u64=25;c.f26.u64=26;c.f27.u64=27;
    c.f28.u64=28;c.f29.u64=29;c.f30.u64=30;c.f31.u64=31;
    const auto before=c;
    function(rdram,&c);
    require(c.r16==before.r16&&c.r17==before.r17&&c.r18==before.r18&&
        c.r19==before.r19&&c.r20==before.r20&&c.r21==before.r21&&
        c.r22==before.r22&&c.r23==before.r23&&c.r28==before.r28&&
        c.r29==before.r29&&c.r30==before.r30&&c.r31==before.r31,"callee-saved GPR/SP/RA");
    require(c.f20.u64==20&&c.f21.u64==21&&c.f22.u64==22&&c.f23.u64==23&&
        c.f24.u64==24&&c.f25.u64==25&&c.f26.u64==26&&c.f27.u64==27&&
        c.f28.u64==28&&c.f29.u64==29&&c.f30.u64==30&&c.f31.u64==31,"callee-saved FPR");
    for(uint32_t p=stack-guard;p<stack-frame;++p)
        require(uint8_t(MEM_BU(0,guest(p)))==0xA5,"below-frame sentinel");
    for(uint32_t p=stack;p<stack+32;++p)
        require(uint8_t(MEM_BU(0,guest(p)))==0xA5,"above-frame sentinel");
    require(events.size()>=4 && events[0].kind==Current && events[1].kind==Parent &&
        events[2].kind==Apply,"get current, get parent, apply once before leaves");
    const uint32_t stride=member_bytes(mode);
    require(word(rdram,buffers)==gfx+physicals*stride+(test.move_cursors?8:0),"gfx advancement");
    require(word(rdram,buffers+4)==mtx+64+(test.move_cursors?64:0),"single final Mtx allocation");
    require(physicals>=1,"do/while emits first leaf even for index zero");
    for(unsigned i=0;i<physicals;++i) {
        const uint32_t de=gfx+i*stride+de_offset(mode)+(test.move_cursors && i>0?8:0);
        const uint32_t da=de+8+(test.move_cursors && i==0?8:0);
        const uint32_t address=lists+uint32_t(int32_t(test.indices[i])*8)+8;
        require(word(rdram,de)==0xDE000000 && word(rdram,de+4)==(address&0x1FFFFFFF),
            "signed list index, DE and converted physical payload");
        require(word(rdram,da)==0xDA380002 &&
            word(rdram,da+4)==mtx+(test.move_cursors?64:0),"post-leaf GPU reload order and matrix");
        if(mode==Row) {
            const uint32_t begin=de-24;
            require(word(rdram,begin)==0x6400000C&&word(rdram,begin+4)==0x90000103&&
                word(rdram,begin+8)==0x02010AA9&&word(rdram,begin+12)==0,
                "leaf public SIMPLE LINEAR group");
            require(word(rdram,begin+16)==0xDA380003&&word(rdram,begin+20)==mtx,
                "forced ordinary LOAD NOPUSH uses existing materialized slot");
            require(word(rdram,da+8)==0x6400000D&&word(rdram,da+12)==1,
                "leaf paid POP follows original LOAD PUSH");
        }
#ifdef TOOIE_BK_LEAF_FIXTURE
        if(mode==BkOwned || mode==BkFloat) {
            const uint32_t group=de-24;
            // BK func_80338904 node in the BT pair form: group, then identity MUL/NOPUSH; the
            // original list and LOAD/PUSH reload stay adjacent; the group pops after the reload.
            require(word(rdram,group)==0x6400000C&&word(rdram,group+4)==bk_id+bk_offset&&
                word(rdram,group+8)==bk_bone_policy&&word(rdram,group+12)==0,
                "BK leaf group: owner ID plus traversal offset, bone policy");
            require(word(rdram,group+16)==0xDA380001&&word(rdram,group+20)==bk_float,
                "BK identity MUL NOPUSH binds the group without changing the matrix stack");
            require(word(rdram,da+8)==0x6400000D&&word(rdram,da+12)==1,
                "BK group pop follows the original LOAD PUSH reload");
        }
        if(mode==BkFloat) {
            const uint32_t off=de-56, on=da+16;
            // F070/callback-written vertices never index the enclosing float arena.
            require(word(rdram,off)==0x64000031&&word(rdram,off+4)==0&&word(rdram,off+8)==0&&word(rdram,off+12)==0,
                "POSITION segment disabled before the CPU-written leaf");
            require(word(rdram,off+16)==0x64000031&&word(rdram,off+20)==2&&word(rdram,off+24)==0&&word(rdram,off+28)==0,
                "VELOCITY segment disabled before the CPU-written leaf");
            require(word(rdram,on)==0x64000031&&word(rdram,on+4)==1&&word(rdram,on+8)==seg_pos&&word(rdram,on+12)==seg_base,
                "enclosing POSITION segment restored after the leaf pair");
            require(word(rdram,on+16)==0x64000031&&word(rdram,on+20)==3&&word(rdram,on+24)==seg_vel&&word(rdram,on+28)==seg_base,
                "enclosing VELOCITY segment restored after the leaf pair");
        }
#endif
    }
#ifdef TOOIE_BK_LEAF_FIXTURE
    if(mode==BkOwned || mode==BkFloat)
        require(word(rdram,bk_state+728+12)==physicals,"one BK group per leaf member");
    require(word(rdram,bk_float+88)==(mode==BkFloat?physicals:0u),"float suspension only inside a float root");
    require(word(rdram,bk_state+60)==(mode==Plain?0u:bk_offset),"DL nodes do not advance the BK offset");
#endif
    result.trace=events;return result;
}
void compare(const Case& test) {
    auto a=run(test,original_DD998,112);
    auto b=run(test,func_800DD998,TOOIE_DD998_CANDIDATE_FRAME_BYTES);
    require(a.trace==b.trace,"original/candidate call and position trace");
    // Stack frames/volatile return registers may differ. All non-stack RAM must agree: with no
    // owner state the candidate emits exactly the original commands and writes no BK state.
    for(size_t i=0;i<a.ram.size();++i) {
        const size_t begin=(stack-guard)&0xFFFFFF, end=(stack+32)&0xFFFFFF;
        if(i>=begin && i<end) continue;
        require(a.ram[i]==b.ram[i],"original/candidate non-stack RAM");
    }
}
#ifdef TOOIE_BK_LEAF_FIXTURE
void bk_owned(const Case& test) {
    // Same original reads, calls and positions as the original; only the BK commands are added.
    auto a=run(test,original_DD998,112);
    for(Mode mode:{BkOwned,BkFloat}) {
        auto b=run(test,func_800DD998,TOOIE_DD998_CANDIDATE_FRAME_BYTES,mode);
        require(a.trace==b.trace,"BK leaf keeps the original call and position trace");
    }
}
#endif
#ifdef TOOIE_PLAYER_STATE_ADDRESS
// Row player helpers stay compiled until retirement (the bone handler still calls the bone pair),
// so their controls run in both selections; only DD998 itself stops calling the leaf pair under BK.
void hybrid_helpers() {
    std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
    constexpr uint32_t state=TOOIE_PLAYER_STATE_ADDRESS,pc=state+18740,base=gfx-0x100;
    auto reset=[&] {
        std::fill(ram.begin(),ram.end(),0);put(rdram,buffers,gfx);put(rdram,buffers+4,mtx);
        put(rdram,state+8,1);put(rdram,state+16,3792);put(rdram,state+28,1);put(rdram,state+18584,1);
        put(rdram,pc,1);put(rdram,pc+28,1);put(rdram,pc+32,0x90000000);put(rdram,pc+36,buffers);
        put(rdram,pc+40,base);put(rdram,pc+48,gfx-24);put(rdram,pc+96,3);
        put(rdram,pc+88,0x62A); // Default helper setup uses the supported 68-pose effective model.
        put(rdram,0x8007BCD0,base);half(rdram,0x8007BEDA,0);
    };
    auto call=[&](recomp_func_t* fn,uint32_t arg=0) {
        recomp_context c{};c.r4=guest(buffers);c.r5=guest(arg);c.r29=guest(stack);c.r31=guest(0x80045678);
        fn(rdram,&c);require(uint32_t(c.r29)==stack&&uint32_t(c.r31)==0x80045678,"helper restores SP/RA");return uint32_t(c.r2);
    };
    reset();put(rdram,pc+100,58);require(call(tooie_player_leaf_begin)==1&&word(rdram,pc+100)==59,"59th leaf allowed");
    const uint32_t after=word(rdram,buffers);require(call(tooie_player_leaf_begin)==0&&word(rdram,buffers)==after&&word(rdram,state+24)==1,"60th leaf refuses before writes");
    call(tooie_player_leaf_end);require(word(rdram,after)==0x6400000D,"paid leaf pop after poison");
    reset();put(rdram,pc+88,0x62B);put(rdram,pc+96,66);
    require(call(tooie_player_leaf_begin)==1&&word(rdram,gfx+4)==0x90000142,"62B final role66 leaf accepted");
    call(tooie_player_leaf_end);
    reset();put(rdram,pc+88,0x62B);put(rdram,pc+96,67);
    require(call(tooie_player_leaf_begin)==0&&word(rdram,buffers)==gfx&&word(rdram,state+24)==1,
        "62B role67 leaf refuses before command writes");
    for(uint32_t asset:{0x838u,0x839u}){
        const uint32_t count=asset==0x838u?60u:58u;
        reset();put(rdram,pc+88,asset);put(rdram,pc+96,count);
        require(call(tooie_player_leaf_begin)==1&&word(rdram,gfx+4)==0x90000100u+count,
            "Dragon final pose role admits its own hybrid ID");
        call(tooie_player_leaf_end);
        for(uint32_t role:{0u,count+1}){
            reset();put(rdram,pc+88,asset);put(rdram,pc+96,role);
            require(call(tooie_player_leaf_begin)==0&&word(rdram,buffers)==gfx&&word(rdram,gfx)==0&&word(rdram,state+24)==1,
                "Dragon role0 or past-pose role trips before command writes");
        }
    }
    reset();put(rdram,pc+96,0);require(call(tooie_player_leaf_begin)==0&&word(rdram,buffers)==gfx&&word(rdram,state+24)==1,"leaf role0 refused");
    reset();put(rdram,pc+96,UINT32_MAX);require(call(tooie_player_leaf_begin)==0&&word(rdram,buffers)==gfx,"invalid leaf context refused");
    constexpr uint32_t physical_end=base+TOOIE_TEST_PLAYER_PHYSICAL_BYTES,canary=0x7B5D3F19u;
    require(size_t(physical_end-0x80000000u)+8u<=ram.size(),"boundary canary fits backing RAM");
    reset();put(rdram,buffers,physical_end-24);
    for(uint32_t at=physical_end-24;at<physical_end+8;at+=4)put(rdram,at,canary);
    require(call(tooie_player_leaf_begin)==0&&word(rdram,buffers)==physical_end-24&&word(rdram,state+24)==1,
        "leaf reserves complete32-byte augmentation");
    for(uint32_t at=physical_end-24;at<physical_end+8;at+=4)
        require(word(rdram,at)==canary,"boundary refusal preserves command span");
    reset();put(rdram,buffers,physical_end-32);
    require(call(tooie_player_leaf_begin)==1&&word(rdram,buffers)==physical_end-8&&word(rdram,state+16)==3792,
        "32-byte boundary accepted with existing reserve");
    call(tooie_player_leaf_end);
    require(word(rdram,buffers)==physical_end&&word(rdram,state+16)==3792,"boundary paid pop fits without charge");
    reset();put(rdram,buffers,gfx+1);call(tooie_player_bone_pop,2);
    require(word(rdram,pc+96)==0&&word(rdram,state+24)==1&&word(rdram,buffers)==gfx+1,"unsafe bone pop restores role0 context without write");
    reset();put(rdram,pc+96,UINT32_MAX);const uint32_t outer=call(tooie_player_bone_push,uint32_t(-1));
    require(outer==1&&word(rdram,pc+96)==0,"bone role0 valid token1");
    const uint32_t nested=call(tooie_player_bone_push,2);require(nested==2&&word(rdram,pc+96)==3,"nested role0 restoration token2");
    call(tooie_player_bone_pop,nested);require(word(rdram,pc+96)==0,"nested role0 restoration");
    call(tooie_player_bone_pop,outer);require(word(rdram,pc+96)==UINT32_MAX,"outer invalid restoration");
}
#endif
}
extern "C" recomp_func_t* get_function(int32_t address) {
    require(uint32_t(address)==callback_a || uint32_t(address)==callback_b,"callback target");
    return uint32_t(address)==callback_a?callback_one:callback_two;
}
extern "C" void mlMtx_get_stack_pointer(uint8_t*,recomp_context* c) {
    events.push_back({Current,0,0,0});clobber(c);c->r2=guest(current);
}
extern "C" void func_80018BB4(uint8_t*,recomp_context* c) {
    events.push_back({Parent,0,0,0});clobber(c);c->r2=guest(parent);
}
extern "C" void mlMtxApply(uint8_t* rdram,recomp_context* c) {
    require(uint32_t(c->r4)==mtx,"apply original matrix cursor before processing");
    events.push_back({Apply,uint32_t(c->r4),0,0});
    for(unsigned i=0;i<64;i+=4) put(rdram,mtx+i,0xA0000000+i);
    clobber(c);
}
extern "C" void func_8010F070(uint8_t* r,recomp_context* c) { process(Transform,r,c); }
extern "C" void osVirtualToPhysical_recomp(uint8_t* rdram,recomp_context* c) {
    const uint32_t p=uint32_t(c->r4), cursor=word(rdram,buffers);
    require(word(rdram,cursor-8)==0xDE000000,"DE opcode and advanced cursor before conversion");
    events.push_back({Physical,p,0,0});
    if(active.move_cursors && physicals==0) {
        put(rdram,buffers,cursor+8);put(rdram,buffers+4,mtx+64);
    }
    ++physicals;clobber(c);c->r2=p&0x1FFFFFFF;
}
// The original and patch recompiler name the same guest import differently.
extern "C" void osVirtualToPhysical(uint8_t* rdram,recomp_context* c) {
    osVirtualToPhysical_recomp(rdram,c);
}
int main() {
    try {
        Case base{{3,0,0,0},{0,0,0},{0,0,0},-7,0,false,false,false,false};
        compare(base); // No processing calls, one leaf.
        auto t=base;t.indices[0]=0;compare(t); // Initial zero is not an empty-list guard.
        t=base;t.indices={-2,4,7,0};compare(t); // Signed indices and three leaf pairs.
        t=base;t.flags=2;t.current_counts={5,0,0};t.parent_counts={6,0,0};compare(t);
        t.flags=1;compare(t); // Odd flag enables F070 current then parent.
        t.current_counts[0]=0;compare(t); // Parent-only processing.
        t.parent_counts[0]=0;t.current_counts[0]=-3;compare(t); // Signed nonzero count.
        t=base;t.callback=true;t.current_counts[0]=2;t.parent_counts[0]=4;compare(t);
        t.flags=-1;t.indices={2,3,0,0};t.current_counts[1]=7;t.parent_counts[1]=-2;compare(t);
        t.replace_callback=true;compare(t); // Reload callback and second count after first callback.
        t=base;t.flags=1;t.current_counts[0]=1;t.parent_counts[0]=2;
        t.indices={1,2,0,0};t.mutate_transform=true;compare(t); // Late flags/count/callback.
        t=base;t.move_cursors=true;t.indices={1,2,0,0};compare(t); // Reload cursors across conversion.
#ifdef TOOIE_BK_LEAF_FIXTURE
        // Owned leaves, with and without an enclosing float root: one leaf, three signed leaves,
        // and processing with callbacks (the CPU-written case the suspension exists for).
        bk_owned(base);
        t=base;t.indices={-2,4,7,0};bk_owned(t);
        t=base;t.flags=1;t.callback=true;t.current_counts[0]=2;t.parent_counts[0]=4;t.indices={1,2,0,0};bk_owned(t);
        std::cout<<"BK leaf group/identity/segment-suspension cases passed\n";
#ifdef TOOIE_PLAYER_STATE_ADDRESS
        hybrid_helpers();
        std::cout<<"Hybrid leaf command/token/boundary controls passed\n";
#endif
#elif defined(TOOIE_PLAYER_STATE_ADDRESS)
        t=base;t.callback=true;t.current_counts[0]=2;t.parent_counts[0]=4;
        run(t,func_800DD998,TOOIE_DD998_CANDIDATE_FRAME_BYTES,Row);
        hybrid_helpers();
        std::cout<<"Hybrid leaf command/token/boundary controls passed\n";
#endif
        std::cout<<"DD998 focused differential cases passed\n";return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
