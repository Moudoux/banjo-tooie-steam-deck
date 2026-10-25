// Actual compiled/retranslated PC[8] secondary helpers (player_secondary.o) against synthetic
// guest RAM. The six hook sites are driven in their original order with the registers their
// thunks pass; DE448/DE498 and the emitters are not simulated. No C reference model.
#include "recomp.h"
#include "../tools/baseline/guest/silo-candidate/render_bridge.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#if !defined(TOOIE_PLAYER_STATE_ADDRESS) || !defined(TOOIE_PLAYER_SECONDARY_ADMIT) || \
    !defined(TOOIE_TEST_SECONDARY_BYTES) || !defined(TOOIE_TEST_LOWER_BYTES)
#error Supply the selected state address, the admission boolean, the SECONDARY partition and the lower span
#endif
#if TOOIE_PLAYER_SECONDARY_ADMIT != 0 && TOOIE_PLAYER_SECONDARY_ADMIT != 1
#error Explicit boolean TOOIE_PLAYER_SECONDARY_ADMIT required
#endif
extern "C" {
void func_800A89F8(uint8_t*,recomp_context*);
void func_800EB854(uint8_t*,recomp_context*);
void func_800AE080(uint8_t*,recomp_context*);
}
#include "player_secondary_candidate_fixture.inc"
namespace {
constexpr uint32_t state=TOOIE_PLAYER_STATE_ADDRESS,pc=state+offsetof(TooieRenderBridgeState,player);
constexpr uint32_t sr=state+offsetof(TooieRenderBridgeState,secondary_route);
constexpr uint32_t sl=state+offsetof(TooieRenderBridgeState,secondary_ledger);
constexpr uint32_t entries=state+offsetof(TooieRenderBridgeState,secondary);
constexpr uint32_t primaries=state+offsetof(TooieRenderBridgeState,primary);
constexpr uint32_t owner=0x80220000,component=0x80221000,pose=0x80222000,table=0x80223000;
constexpr uint32_t freelist=0x80230000,appearance=0x80232000,packed_data=0x80260000;
constexpr uint32_t gfx_pool=0x80300000,mtx_pool=0x80400000,model=0x80600000;
constexpr uint32_t buffers=0x80127658,parent=0x807F0000,inner=parent-0x68,stack=0x80818000;
constexpr uint32_t slot=3,epoch=7,manager=5,handle=9,view0=1,phase0=2,link=2,vertex_count=100;
constexpr uint32_t EMPTY=0,OPEN=1,ARMED=2,SPENT=3; // player_secondary.h token states
static_assert(state>=0x80800000u,"fixture objects lie below the selected bridge state");
struct RowFacts { uint32_t resource,bytes,graph,lists,vertices,attributes,poses,bones,leaves,gfx,mtx; };
constexpr RowFacts row83A{0x83A,58120,53864,4160,23256,22496,47,59,43,2064,6592};
constexpr RowFacts row61C{0x61C,67320,62200,4920,26232,25296,58,70,50,2528,7744};
constexpr uint32_t prepay(const RowFacts& r) { return 32+24*r.bones+32*r.leaves; }
static_assert(prepay(row83A)==2824 && prepay(row61C)==3312,"execution-facts28 label prepay");
uint32_t g_view=view0,g_phase=phase0;
void require(bool ok,const char* why) { if(!ok) throw std::runtime_error(why); }
gpr guest(uint32_t p) { return S32(p); }
uint32_t word(uint8_t* rdram,uint32_t p) { return uint32_t(MEM_W(0,guest(p))); }
void put(uint8_t* rdram,uint32_t p,uint32_t v) { MEM_W(0,guest(p))=v; }
void half(uint8_t* rdram,uint32_t p,uint16_t v) { MEM_H(0,guest(p))=int16_t(v); }
void byte(uint8_t* rdram,uint32_t p,uint8_t v) { MEM_B(0,guest(p))=int8_t(v); }
void clobber(recomp_context* c) {
    c->r1=1;c->r3=3;c->r4=4;c->r5=5;c->r6=6;c->r7=7;c->r8=8;c->r9=9;c->r10=10;c->r11=11;
    c->r12=12;c->r13=13;c->r14=14;c->r15=15;c->r24=24;c->r25=25;
}
// One helper call with the thunk's argument registers; callee-saved state must survive.
uint32_t call(uint8_t* rdram,recomp_func_t* fn,uint32_t a0,uint32_t a1=0,uint32_t a2=0) {
    recomp_context c{};c.r4=guest(a0);c.r5=guest(a1);c.r6=guest(a2);
    c.r29=guest(stack);c.r31=guest(0x80045678);
    c.r16=16;c.r17=17;c.r18=18;c.r19=19;c.r20=20;c.r21=21;c.r22=22;c.r23=23;c.r28=28;c.r30=30;
    fn(rdram,&c);
    require(c.r16==16&&c.r17==17&&c.r18==18&&c.r19==19&&c.r20==20&&c.r21==21&&c.r22==22&&c.r23==23&&
        c.r28==28&&c.r30==30&&uint32_t(c.r29)==stack&&uint32_t(c.r31)==0x80045678,"callee-saved GPR/SP/RA");
    return uint32_t(c.r2);
}
uint32_t token(uint8_t* r,size_t field) { return word(r,sr+uint32_t(field)); }
uint32_t ledger(uint8_t* r,size_t field) { return word(r,sl+uint32_t(field)); }
uint32_t entry(uint32_t s,uint32_t v,uint32_t route) { return entries+((s*4u+v)*2u+route-1u)*48u; }
uint32_t primary(uint32_t s,uint32_t v) { return primaries+(s*4u+v)*32u; }
#define TK(f) token(r,offsetof(TooieSecondaryRoute,f))
#define LG(f) ledger(r,offsetof(TooieSecondaryLedger,f))
struct Rig {
    std::vector<uint8_t> ram=std::vector<uint8_t>(16*1024*1024);
    uint8_t* r() { return ram.data(); }
    explicit Rig(const RowFacts& row=row83A) {
        auto* rdram=ram.data();g_view=view0;g_phase=phase0;
        put(rdram,state,TOOIE_BRIDGE_MAGIC);put(rdram,state+4,TOOIE_BRIDGE_LAYOUT);put(rdram,state+8,epoch);
        put(rdram,state+offsetof(TooieRenderBridgeState,seen_epoch),epoch);
        put(rdram,pc+offsetof(TooiePlayerBoundary,enabled),1);
        put(rdram,owner+0x50,component);put(rdram,owner+0x184,slot);put(rdram,0x80135490+slot*4,owner);
        half(rdram,component+0x90,handle);half(rdram,component+0x9C,uint16_t(row.resource));
        byte(rdram,component+0xA0,manager);
        // 9359C's copy-manager record (stride 12): filled byte9, zero vertex-copy words, byte8 0
        // (no linked packed-appearance record).
        put(rdram,0x80135A60,freelist);half(rdram,freelist,12);half(rdram,freelist+2,16);
        byte(rdram,freelist+4+12*manager+9,1);
        put(rdram,model+4,row.graph);put(rdram,model+12,row.lists);put(rdram,model+16,row.vertices);
        put(rdram,model+0x18,row.attributes);half(rdram,model+row.attributes+4,uint16_t(row.poses));
        half(rdram,model+row.vertices+0x16,uint16_t(vertex_count<<1)); // canonical vertex count
        put(rdram,pose,row.poses);put(rdram,pose+4,row.poses);put(rdram,0x8012C968,pose);
        put(rdram,table+2*4,0x800DCF48);put(rdram,table+10*4,0x800DD410);put(rdram,table+12*4,0x800DD56C);
        put(rdram,table+17*4,0x800DD65C);put(rdram,table+24*4,0x800DD998);
        put(rdram,0x8012CF8C,table);put(rdram,0x80123198,table);
        half(rdram,0x8007BEDA,0);put(rdram,0x8007BCD0,gfx_pool);put(rdram,0x8007BCD8,mtx_pool);
        // 9375C exported pose_count+1 matrices from mtx_pool: body+94 is matrix1, the cursor follows.
        put(rdram,component+0x94,mtx_pool+64);put(rdram,0x8012C948+14*4,mtx_pool+64);
        put(rdram,buffers,gfx_pool+0x100);put(rdram,buffers+4,mtx_pool+64+64*row.poses);
        put(rdram,0x8012C948+24*4,0x80092224);put(rdram,0x8012C948+25*4,owner);
    }
    // 1007B8's stride-12 appearance record linked from manager byte8 (an index), and 100AC4's
    // C974 publication of its packed data (10 bytes per canonical vertex).
    uint32_t link_packed(uint32_t resource=0x83A) {
        auto* rdram=r();const uint32_t record=appearance+4+12*link;
        byte(rdram,freelist+4+12*manager+8,uint8_t(link));
        put(rdram,0x80135A64,appearance);half(rdram,appearance,12);half(rdram,appearance+2,16);
        put(rdram,record,packed_data);half(rdram,record+4,uint16_t(resource));half(rdram,record+6,0);
        byte(rdram,record+8,1);put(rdram,0x8012C948+11*4,packed_data);
        return record;
    }
    // DF660 mode1: C838=1, CA11=1 and DE498's exact mode1 appearance row.
    void mode1() {
        auto* rdram=r();put(rdram,0x8012C838,1);byte(rdram,0x8012C948+0xC9,1);
        put(rdram,0x80123030+12,0x8010E2C4);put(rdram,0x80123030+16,0x8010E4C4);put(rdram,0x80123030+20,0x8010E7F4);
    }
    void depth(uint32_t d) { put(r(),state+offsetof(TooieRenderBridgeState,silo_depth),d); }
    uint32_t begin(uint32_t route,uint32_t frame=parent) { return call(r(),tooie_secondary_route_begin,route,owner,frame); }
    uint32_t end(uint32_t route,uint32_t who=owner,uint32_t frame=parent) { return call(r(),tooie_secondary_route_end,route,who,frame); }
    uint32_t arm(uint32_t frame=inner,uint32_t mode=1,uint32_t who=owner) { return call(r(),tooie_secondary_route_arm,who,frame,mode); }
    uint32_t disarm(uint32_t frame=inner) { return call(r(),tooie_secondary_route_disarm,owner,frame); }
    uint32_t consume() { depth(1);const uint32_t v=call(r(),tooie_player_secondary_begin,buffers,model);depth(0);return v; }
    // The complete original order of one secondary occurrence.
    uint32_t occurrence(uint32_t route) { begin(route);arm();const uint32_t v=consume();disarm();end(route);return v; }
    // What tooie_player_end does to the shared paid scope (it lives in player_boundary.o).
    void close() { auto* rdram=r();put(rdram,state+offsetof(TooieRenderBridgeState,active),0);
        put(rdram,sl+offsetof(TooieSecondaryLedger,paid),0);put(rdram,buffers,word(rdram,buffers)+8); }
};

unsigned route_cases() {
    unsigned cases=0;
    // R1/R2: the main and late occurrences each open, arm, are counted once and clear.
    for(uint32_t route:{1u,2u}) {
        Rig g;auto* r=g.r();
        g.begin(route);
        require(TK(state)==OPEN && TK(route)==route && TK(owner)==owner && TK(epoch)==epoch &&
            TK(frame)==parent && TK(view)==view0 && TK(opens)==1,"the site opens the token for its route");
        g.arm();require(TK(state)==ARMED,"the intended 93864 invocation arms");
        g.consume();require(TK(state)==SPENT && (route==1?LG(seen_main):LG(seen_late))==1 && !LG(unauthenticated),
            "the armed DE448 occurrence is counted for its route and spent");
        g.disarm();require(TK(state)==SPENT,"the disarm never re-arms a spent token");
        g.end(route);require(TK(state)==EMPTY && !TK(faults),"the parent return clears without a fault");
        ++cases;
    }
    // R3: an overlapping open invalidates the token; nothing is restored.
    {Rig g;auto* r=g.r();g.begin(1);g.begin(2);
        require(TK(state)==EMPTY && TK(faults)==1 && TK(opens)==1,"an overlap clears and counts");++cases;}
    // R4: a mismatched parent return (frame, owner or route) still clears and is counted.
    for(int which=0;which<3;++which) {
        Rig g;auto* r=g.r();g.begin(1);
        if(which==0)g.end(1,owner,parent+16);
        if(which==1)g.end(1,owner+0x1000,parent);
        if(which==2)g.end(2,owner,parent);
        require(TK(state)==EMPTY && TK(faults)==1,"a mismatched end clears with a fault");++cases;
    }
    // R5: an end with no open token is a no-op.
    {Rig g;auto* r=g.r();g.end(1);require(TK(state)==EMPTY && !TK(faults),"an empty end is silent");++cases;}
    // R6: a DE448 reached before the arm (a pre-callback reentry) is unauthenticated and does not
    // spend the token; the actual occurrence after the arm still authenticates.
    {Rig g;auto* r=g.r();g.begin(1);
        require(!g.consume() && LG(unauthenticated)==1 && TK(state)==OPEN && !LG(seen_main),"pre-arm DE448 is not this occurrence");
        g.arm();g.consume();require(LG(seen_main)==1,"the armed occurrence still counts");++cases;}
    // R7: single use: a second depth-1 DE448 after the spend is unauthenticated.
    {Rig g;auto* r=g.r();g.begin(1);g.arm();g.consume();g.consume();
        require(LG(seen_main)==1 && LG(unauthenticated)==1,"a spent token admits one occurrence only");++cases;}
    // R8: the mode0 primary calls of 93864 meet an empty token and change nothing.
    {Rig g;auto* r=g.r();g.arm(inner,0);g.disarm();
        require(TK(state)==EMPTY && !TK(faults),"mode0 arm/disarm with no token is silent");++cases;}
    // R9: the 93A74 null-model leg reaches the disarm without an arm: OPEN stays, no fault.
    {Rig g;auto* r=g.r();g.begin(1);g.disarm();
        require(TK(state)==OPEN && !TK(faults),"null-model disarm leaves OPEN");
        g.end(1);require(TK(state)==EMPTY && !TK(faults) && !LG(seen_main),"the parent clears; nothing was drawn");++cases;}
    // R10/R11/R12: a deeper same-function call (frame), a mode0 invocation at the intended frame,
    // or a different staged owner while OPEN fails closed; the intended arm then finds EMPTY.
    for(int which=0;which<3;++which) {
        Rig g;auto* r=g.r();g.begin(1);
        if(which==0)g.arm(inner-0x100);
        if(which==1)g.arm(inner,0);
        if(which==2){put(r,0x8012C948+25*4,owner+0x1000);g.arm();put(r,0x8012C948+25*4,owner);}
        require(TK(state)==EMPTY && TK(faults)==1,"a non-intended arm clears with a fault");
        g.arm();g.consume();require(LG(unauthenticated)==1 && !LG(seen_main),"nothing restores the cleared token");++cases;
    }
    // R13/R14: an epoch or view change between the arm and the DE448 is unauthenticated.
    for(int which=0;which<2;++which) {
        Rig g;auto* r=g.r();g.begin(1);g.arm();
        if(which==0)put(r,state+8,epoch+1); else g_view=view0+1;
        g.consume();require(LG(unauthenticated)==1 && !LG(seen_main),"a stale epoch or another view fails closed");++cases;
    }
    // R15: labels disabled: the route never opens.
    {Rig g;auto* r=g.r();put(r,pc+offsetof(TooiePlayerBoundary,enabled),0);g.begin(1);
        require(TK(state)==EMPTY && !TK(opens) && !TK(faults),"disabled player labels open no route");++cases;}
    // R16: an open paid scope or a nonzero bridge depth at the parent site refuses to open.
    for(int which=0;which<2;++which) {
        Rig g;auto* r=g.r();
        if(which==0)put(r,state+offsetof(TooieRenderBridgeState,active),1); else g.depth(1);
        g.begin(1);require(TK(state)==EMPTY && TK(faults)==1,"the parent site must be outside any draw scope");++cases;
    }
    return cases;
}

#if TOOIE_PLAYER_SECONDARY_ADMIT
// Refusal of one isolated predicate: the token is spent, no label is written, nothing is charged.
void refused(Rig& g,uint32_t reason,const char* why) {
    auto* r=g.r();
    require(!g.occurrence(1) && LG(last_reason)==reason && LG(refusals)==1 && !LG(bytes) && !LG(paid) &&
        !word(r,state+offsetof(TooieRenderBridgeState,active)) && word(r,buffers)==gfx_pool+0x100,why);
}
unsigned admission_cases() {
    unsigned cases=0;
    const uint32_t md=TOOIE_BRIDGE_MODEL_DATA,res=0x83Au<<16;
    const uint32_t worker=TOOIE_BRIDGE_OVERRIDE|(1u<<14),publication=TOOIE_BRIDGE_OVERRIDE|(1u<<15);
    // B1: 083A on the main route: entry [slot][view][main], one block, SECONDARY charged, the
    // shared root envelope at the cursor, the primary diagnostics untouched.
    {
        Rig g;auto* r=g.r();
        require(g.occurrence(1)==1,"083A admits on the main route");
        const uint32_t e=entry(slot,view0,1),id=word(r,e+offsetof(TooieSecondaryEntry,idbase));
        require(id==TOOIE_BRIDGE_ID_BASE && word(r,e+offsetof(TooieSecondaryEntry,owner))==owner &&
            word(r,e+offsetof(TooieSecondaryEntry,effective_model_asset))==0x83A &&
            word(r,e+offsetof(TooieSecondaryEntry,selector_handle))==(0x83Au<<16|handle) &&
            word(r,e+offsetof(TooieSecondaryEntry,view_phase))==(view0|phase0<<2) &&
            word(r,e+offsetof(TooieSecondaryEntry,stamp))==epoch &&
            word(r,e+offsetof(TooieSecondaryEntry,limits))==(47u|59u<<8|43u<<16),"the route-keyed entry is recorded");
        require(LG(paid)==1+slot*8+view0*2 && LG(bytes)==2824 && LG(highwater)==2824 && LG(admissions)==1 &&
            LG(last_reason)==0xFFFFFFFFu && LG(last_route)==1 &&
            word(r,state+offsetof(TooieRenderBridgeState,active))==1 &&
            !word(r,state+offsetof(TooieRenderBridgeState,primary_paid)) &&
            !word(r,state+offsetof(TooieRenderBridgeState,primary_bytes)) &&
            !word(r,state+offsetof(TooieRenderBridgeState,extra_bytes)) &&
            word(r,state+offsetof(TooieRenderBridgeState,next_block))==1,"SECONDARY alone pays; one block");
        require(word(r,pc+offsetof(TooiePlayerBoundary,idbase))==id && word(r,pc+offsetof(TooiePlayerBoundary,owner))==owner &&
            word(r,pc+offsetof(TooiePlayerBoundary,effective_model_asset))==0x83A &&
            word(r,pc+offsetof(TooiePlayerBoundary,begin))==gfx_pool+0x100 &&
            word(r,pc+offsetof(TooiePlayerBoundary,current_bone_role))==0xFFFFFFFFu &&
            !word(r,pc+offsetof(TooiePlayerBoundary,calls)) && !word(r,pc+offsetof(TooiePlayerBoundary,admissions)),
            "the global paid scope snapshots the entry; the primary join serial is untouched");
        require(word(r,buffers)==gfx_pool+0x100+24 && word(r,gfx_pool+0x108)==0x6400000Cu &&
            word(r,gfx_pool+0x10C)==id+129u,"the shared enable/root envelope");
        // B2: the late occurrence of the same owner and view is its own entry and block.
        g.close();require(g.occurrence(2)==1,"late admits in the same epoch");
        require(word(r,entry(slot,view0,2)+offsetof(TooieSecondaryEntry,idbase))==TOOIE_BRIDGE_ID_BASE+512u &&
            LG(paid)==1+slot*8+view0*2+1 && LG(bytes)==2*2824,"main and late never share a history");
        // B3: the next epoch renews nothing; the ledger resets with the construction.
        g.close();put(r,state+8,epoch+1);require(g.occurrence(1)==1,"next epoch admits");
        require(word(r,entry(slot,view0,1)+offsetof(TooieSecondaryEntry,idbase))==TOOIE_BRIDGE_ID_BASE &&
            word(r,state+offsetof(TooieRenderBridgeState,next_block))==2 && LG(bytes)==2824 && !LG(reissues),
            "a consecutive epoch keeps the entry's ID");
        // B4: a second main occurrence of the entry in that epoch is DUPLICATE.
        g.close();require(!g.occurrence(1) && LG(last_reason)==TOOIE_BRIDGE_DUPLICATE && LG(bytes)==2824,
            "one paid occurrence per entry and epoch");
        cases+=4;
    }
    // B5: 061C matches its structural row and is refused until the shared segment7 contract.
    {Rig g(row61C);refused(g,md|0x200u|(0x61Cu<<16),"061C waits for the incoming segment7 contract");++cases;}
    // B6: the manager record must be the live filled zero-copy record; a linked appearance index
    // without a qualified appearance record refuses as APPEARANCE (manager byte8 is an index).
    for(int which=0;which<5;++which) {
        Rig g;auto* r=g.r();const uint32_t rec=freelist+4+12*manager;uint32_t reason=md|0x800u|res;
        if(which==0)byte(r,component+0xA0,0);            // no record (not the constructor contract)
        if(which==1)byte(r,rec+9,0);                     // not filled
        if(which==2)put(r,rec,0x80250000);               // a vertex copy
        if(which==3)put(r,rec+4,0x80250000);             // a second vertex copy
        if(which==4){byte(r,rec+8,1);reason=md|0x4000u|res;} // index into an absent appearance pool
        refused(g,reason,"copy-manager record qualification");++cases;
    }
    // B7: staged override words: C964 vertices, C838 enabled without packed data, C974 without a
    // linked record, callback.
    for(uint32_t bit:{5u,6u,7u,0u}) {
        Rig g;auto* r=g.r();
        if(bit==5)put(r,0x8012C948+7*4,0x80250000);
        if(bit==6)g.mode1();
        if(bit==7)put(r,0x8012C948+11*4,0x80250000);
        if(bit==0){g.begin(1);g.arm();put(r,0x8012C948+24*4,0x80092234);g.depth(1);
            require(!call(r,tooie_player_secondary_begin,buffers,model),"no admission");g.depth(0);
            require(LG(last_reason)==(TOOIE_BRIDGE_OVERRIDE|(1u<<8)),"callback must stay 92224");++cases;continue;}
        refused(g,TOOIE_BRIDGE_OVERRIDE|(1u<<(bit+8)),"staged override words");++cases;
    }
    // B8: the palette export: C980 must equal body+94, lie in the pool, aligned, below the cursor.
    for(int which=0;which<4;++which) {
        Rig g;auto* r=g.r();
        if(which==0)put(r,0x8012C948+14*4,mtx_pool+128);
        if(which==1){put(r,component+0x94,mtx_pool+72);put(r,0x8012C948+14*4,mtx_pool+72);}
        if(which==2){put(r,component+0x94,mtx_pool);put(r,0x8012C948+14*4,mtx_pool);}
        if(which==3)put(r,buffers+4,mtx_pool+64*47);
        refused(g,md|0x1000u|res,"export interval authentication");++cases;
    }
    // B9: room from the current cursors: Mtx suffix 6592 (export not re-counted), Gfx 2064+2824.
    for(int which=0;which<3;++which) {
        Rig g;auto* r=g.r();
        if(which==0){const uint32_t mtx=mtx_pool+0x12C00-6592+64;
            put(r,buffers+4,mtx);put(r,component+0x94,mtx-64*47);put(r,0x8012C948+14*4,mtx-64*47);}
        if(which==1)put(r,buffers,gfx_pool+TOOIE_TEST_LOWER_BYTES-(2064+2824)+8);
        if(which==2)put(r,sl+offsetof(TooieSecondaryLedger,bytes),TOOIE_TEST_SECONDARY_BYTES-2824+1);
        require(!g.occurrence(1) && LG(last_reason)==TOOIE_BRIDGE_CAPACITY,"bounded rooms and the SECONDARY ledger");++cases;
    }
    {   // The exact Mtx boundary admits: the export prefix is already allocated and not counted again.
        Rig g;auto* r=g.r();const uint32_t mtx=mtx_pool+0x12C00-6592;
        put(r,buffers+4,mtx);put(r,component+0x94,mtx-64*47);put(r,0x8012C948+14*4,mtx-64*47);
        require(g.occurrence(1)==1,"the remaining 6592 Mtx suffix alone is required");++cases;
    }
    // B10: header, resource and table predicates.
    for(int which=0;which<5;++which) {
        Rig g;auto* r=g.r();uint32_t reason=md|0x400u|res;
        if(which==0)half(r,model+10,0x10);
        if(which==1)put(r,model+0x28,0x100);
        if(which==2)half(r,model+row83A.attributes+4,46);
        if(which==3){half(r,component+0x9C,0x61C);reason=md|0x100u|res;}
        if(which==4){put(r,table+24*4,0);reason=md|0x2000u|res;}
        refused(g,reason,"header, resource and default handler table");++cases;
    }
    // B11: pose staging (C968 = AE080(body+90)), roster slot, and retained tombstones of BOTH
    // families checked before the owner is dereferenced.
    for(int which=0;which<5;++which) {
        Rig g;auto* r=g.r();uint32_t reason=TOOIE_BRIDGE_ANIMATION;
        if(which==0)put(r,0x8012C968,pose+0x100);
        if(which==1)put(r,pose,46);
        if(which==2){put(r,0x80135490+slot*4,owner+0x1000);reason=TOOIE_BRIDGE_SLOT;}
        if(which==3){put(r,entry(5,0,1)+offsetof(TooieSecondaryEntry,owner),owner);
            put(r,entry(5,0,1)+offsetof(TooieSecondaryEntry,stamp),0xFFFFFFFFu);reason=TOOIE_BRIDGE_BIRTH;}
        if(which==4){put(r,primary(6,2)+offsetof(TooiePlayerEntry,owner),owner);           // primary-only
            put(r,primary(6,2)+offsetof(TooiePlayerEntry,stamp),0xFFFFFFFFu);reason=TOOIE_BRIDGE_BIRTH;} // retirement
        refused(g,reason,"pose staging, roster slot and both tombstone families");++cases;
    }
    // B12: an observed pending slot is refused BIRTH and forces freshness of every retained ID of
    // the slot in both families, plus the global snapshot when it is this owner.
    {Rig g;auto* r=g.r();
        put(r,entry(slot,2,2)+offsetof(TooieSecondaryEntry,idbase),0x90000400);
        put(r,primary(slot,1)+offsetof(TooiePlayerEntry,idbase),0x90000600);
        put(r,primary(slot+1,1)+offsetof(TooiePlayerEntry,idbase),0x90000800);
        put(r,pc+offsetof(TooiePlayerBoundary,owner),owner);put(r,pc+offsetof(TooiePlayerBoundary,idbase),0x90000600);
        byte(r,0x801354B0+slot,1);refused(g,TOOIE_BRIDGE_BIRTH,"pending slot");
        require(!word(r,entry(slot,2,2)+offsetof(TooieSecondaryEntry,idbase)) &&
            !word(r,primary(slot,1)+offsetof(TooiePlayerEntry,idbase)) &&
            !word(r,pc+offsetof(TooiePlayerBoundary,idbase)) &&
            word(r,primary(slot+1,1)+offsetof(TooiePlayerEntry,idbase))==0x90000800,
            "pending clears this slot's secondary and primary IDs and its own snapshot, nothing else");++cases;}
    // B13: appearance states (execution-facts30). Stored packed data with the worker off, and the
    // exact mode1 worker over that record, both admit; the record does not enter the identity.
    {Rig g;g.link_packed();require(g.occurrence(1)==1,"retained packed record, worker off");++cases;}
    {Rig g;g.link_packed();g.mode1();require(g.occurrence(1)==1,"mode1 worker over the qualified packed record");++cases;}
    // B14: unqualified linked records refuse APPEARANCE; publication and worker mismatches refuse
    // bits 7 and 6. An enabled worker without packed data has no qualified consumer.
    for(int which=0;which<9;++which) {
        Rig g;auto* r=g.r();const uint32_t record=g.link_packed();uint32_t reason=md|0x4000u|res;
        if(which==0)byte(r,record+8,0);                                  // refcount 0: released
        if(which==1)half(r,record+4,0x61C);                              // another resource
        if(which==2)half(r,record+6,0x21);                               // palette-rebuilt record
        if(which==3)put(r,record,0x7F000000);                            // packed span outside RAM
        if(which==4)byte(r,freelist+4+12*manager+8,16);                  // index beyond capacity
        if(which==5){put(r,0x8012C948+11*4,packed_data+10);reason=publication;} // C974 not 100AC4's
        if(which==6){g.mode1();byte(r,0x8012C948+0xC9,2);reason=worker;}  // CA11 not mode1
        if(which==7){g.mode1();put(r,0x80123030+20,0);reason=worker;}     // mode1 row changed
        if(which==8){byte(r,freelist+4+12*manager+8,0);put(r,0x8012C948+11*4,0);g.mode1();reason=worker;} // no packed data
        refused(g,reason,"appearance record, publication and worker qualification");++cases;
    }
    return cases;
}
#endif
}
extern "C" void func_800A89F8(uint8_t*,recomp_context* c) { clobber(c);c->r2=g_view; }
extern "C" void func_800EB854(uint8_t*,recomp_context* c) { clobber(c);c->r2=g_phase; }
extern "C" void func_800AE080(uint8_t*,recomp_context* c) {
    const int32_t h=int16_t(c->r4);clobber(c);c->r2=guest(h==int32_t(handle)?pose:0u);
}
int main() {
    try {
        const unsigned route=route_cases();
        require(route==19,"all route-token cases ran");
#if TOOIE_PLAYER_SECONDARY_ADMIT
        const unsigned admission=admission_cases();
        require(admission==44,"all secondary admission cases ran");
#else
        const unsigned admission=0;
#endif
        std::cout<<"PC[8] secondary route_cases="<<route<<" admission_cases="<<admission
                 <<" admit="<<TOOIE_PLAYER_SECONDARY_ADMIT<<'\n';
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
