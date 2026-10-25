// Actual raw originals versus compiled/retranslated replacements; no C reference model.
#include "recomp.h"
#include "../tools/baseline/guest/silo-candidate/render_bridge.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#if !defined(TOOIE_PLAYER_STATE_ADDRESS) || !defined(TOOIE_PLAYER_INIT_FRAME_BYTES) || !defined(TOOIE_PLAYER_FREE_FRAME_BYTES)
#error Supply selected guest state address and both measured candidate frame sizes
#endif
#if !defined(TOOIE_TROT_SELECTED) || (TOOIE_TROT_SELECTED != 0 && TOOIE_TROT_SELECTED != 1)
#error Supply an explicit boolean selected Trot fixture flag
#endif
extern "C" {
void _babackpack_entrypoint_9(uint8_t*,recomp_context*);
void func_800936E8(uint8_t*,recomp_context*);
void func_80093700(uint8_t*,recomp_context*);
void func_800AE020(uint8_t*,recomp_context*);
void func_800AE080(uint8_t*,recomp_context*);
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
void tooie_cpu625_retire(uint8_t*,recomp_context*);
void tooie_backpack621_retire(uint8_t*,recomp_context*);
#if TOOIE_TROT_SELECTED
void tooie_trot_toggle_retire(uint8_t*,recomp_context*);
#endif
}
#define func_80092898 original_92898
#define func_800F3880 original_F3880
#include "player_init_original_fixture.inc"
#include "player_free_original_fixture.inc"
#undef func_80092898
#undef func_800F3880
#include "player_lifecycle_candidate_fixture.inc"
namespace {
constexpr uint32_t owner=0x80220000, bodies=0x80221000, pose=0x80228000;
constexpr uint32_t stack=0x80818000, guard=512, body_count=20, body_stride=0x200;
constexpr uint32_t state=TOOIE_PLAYER_STATE_ADDRESS;
constexpr uint32_t pc=state+offsetof(TooieRenderBridgeState,player);
#define PF(field) (pc+offsetof(TooiePlayerBoundary,field))
#define SF(field) (state+offsetof(TooieRenderBridgeState,field))
// PC[8]: entry [0][0] is the open paid entry when running; [1][2] is an inactive retained entry of
// the same owner that must retire (never on the paid path).
constexpr uint32_t paid_entry=state+offsetof(TooieRenderBridgeState,primary);
constexpr uint32_t other_entry=paid_entry+(1u*4u+2u)*32u;
static_assert(TOOIE_PLAYER_INIT_FRAME_BYTES>0 && TOOIE_PLAYER_INIT_FRAME_BYTES<guard);
static_assert(TOOIE_PLAYER_FREE_FRAME_BYTES>0 && TOOIE_PLAYER_FREE_FRAME_BYTES<guard);
enum Kind { Backpack, Set936, Set937, Allocate, Resolve, Resource, Bind, Create,
    Clear, Set93370, Set9337C, Float93230, Set931, Lookup, Float92864, Teardown, Free };
struct Case { uint32_t allocated, resource; bool mutate; };
struct Event {
    Kind kind; uint32_t a0,a1;
    std::vector<uint8_t> memory;
    bool operator==(const Event& b) const {
        return kind==b.kind && a0==b.a0 && a1==b.a1 && memory==b.memory;
    }
};
enum RetireKind { CpuRetire, BackpackRetire, TrotRetire };
struct RetireEvent {
    RetireKind kind; uint32_t self, requested, reason, idbase, stamp;
    size_t original_calls;
};
struct Output { std::vector<uint8_t> ram; std::vector<Event> trace; std::vector<RetireEvent> retire; };
Case active{};
std::vector<Event> events;
std::vector<RetireEvent> retire_events;
bool inspect_retirement=false, inspect_active=false;
gpr guest(uint32_t p) { return S32(p); }
uint32_t word(uint8_t* rdram,uint32_t p) { return uint32_t(MEM_W(0,guest(p))); }
void put(uint8_t* rdram,uint32_t p,uint32_t v) { MEM_W(0,guest(p))=v; }
uint8_t byte(uint8_t* rdram,uint32_t p) { return uint8_t(MEM_BU(0,guest(p))); }
void require(bool ok,const char* why) { if(!ok) throw std::runtime_error(why); }
uint32_t narrow(uint32_t v) { return uint32_t(int32_t(int16_t(v))); }
uint32_t body(uint8_t* rdram) { return word(rdram,owner+0x50); }
std::vector<uint8_t> snapshot(uint8_t* rdram) {
    std::vector<uint8_t> out;
    for(uint32_t p=owner-16;p<owner+0x198;++p) out.push_back(byte(rdram,p));
    for(uint32_t i=0;i<body_count;++i)
        for(uint32_t p=bodies+i*body_stride-16;p<bodies+i*body_stride+0x128;++p)
            out.push_back(byte(rdram,p));
    return out;
}
void clobber(recomp_context* c) {
    c->r1=1;c->r2=2;c->r3=3;c->r4=4;c->r5=5;c->r6=6;c->r7=7;
    c->r8=8;c->r9=9;c->r10=10;c->r11=11;c->r12=12;c->r13=13;
    c->r14=14;c->r15=15;c->r24=24;c->r25=25;c->hi=0x1234;c->lo=0x5678;
    c->f0.u64=100;c->f1.u64=101;c->f2.u64=102;c->f3.u64=103;
    c->f4.u64=104;c->f5.u64=105;c->f6.u64=106;c->f7.u64=107;
    c->f8.u64=108;c->f9.u64=109;c->f10.u64=110;c->f11.u64=111;
    c->f12.u64=112;c->f13.u64=113;c->f14.u64=114;c->f15.u64=115;
    c->f16.u64=116;c->f17.u64=117;c->f18.u64=118;c->f19.u64=119;
}
void stub(Kind kind,uint8_t* rdram,recomp_context* c) {
    uint32_t a0=uint32_t(c->r4), a1=uint32_t(c->r5), b=body(rdram), result=0;
    // Compare defined arguments only; caller-saved unused registers may differ.
    if(kind==Allocate || kind==Create) a0=a1=0;
    else if(kind==Backpack || kind==Resolve || kind==Resource || kind==Clear ||
            kind==Lookup || kind==Teardown || kind==Free) a1=0;
    if(kind==Backpack || kind==Set936 || kind==Set937 || kind==Set93370 ||
       kind==Set9337C || kind==Float93230 || kind==Set931 || kind==Lookup ||
       kind==Float92864 || kind==Teardown || kind==Free) require(a0==owner,"stable self argument");
    if(kind==Set936) {
        require(a1==0,"936E8 zero argument");
        for(auto offset:{0x11u,0x10u,0x16u,0x12u,0x13u,0xA2u})
            require(byte(rdram,b+offset)==0,"initial byte stores visible before call");
        require(byte(rdram,b+0x14)==0xFF && MEM_HU(0xC,guest(b))==0 &&
            MEM_HU(0x9C,guest(b))==0,"initial FF/halfword and delay-slot stores");
    }
    if(kind==Set937 || kind==Set93370 || kind==Set9337C || kind==Set931)
        require(a1==1,"original enable argument");
    if(kind==Resolve) { require(a0==narrow(active.allocated),"stored/reloaded signed AE020 halfword");result=pose; }
    if(kind==Resource) { require(a0==10,"resource argument10");result=active.resource; }
    if(kind==Bind) require(a0==pose && a1==narrow(active.resource),"pose survives call; signed E09B8 narrowing");
    if(kind==Allocate) result=active.allocated;
    if(kind==Create) { require(byte(rdram,b+0x88)==0,"88 delay-slot clear before DBFF8");result=0x89ABCDEF; }
    if(kind==Clear) {
        const auto ordinal=unsigned(std::count_if(events.begin(),events.end(),[](const Event& e){return e.kind==Clear;}));
        require(a0==b+0x2C+12*ordinal,"three separately reloaded vector arguments");
        if(ordinal==0) require(word(rdram,b+4)==0x89ABCDEF && word(rdram,b+0x84)==0 &&
            byte(rdram,b+0x7C)==0 && byte(rdram,b+0x81)==0,"post-DBFF8 stores before first vector call");
    }
    if(kind==Set93370) require(word(rdram,b+0x20)==0 && word(rdram,b+0x24)==0 &&
        word(rdram,b+0x28)==0,"three exact positive-zero payloads before93370");
    if(kind==Float93230 || kind==Float92864) require(a1==0x3F800000,"exact1.0 o32 a1 bits");
    if(kind==Set931) require(byte(rdram,b+0x15)==0,"15 delay-slot store before931AC");
    if(inspect_retirement) require(word(rdram,PF(idbase))==0 && word(rdram,PF(stamp))==UINT32_MAX,
        "retirement before every original call; completion only after final call");
    if(inspect_active) require(word(rdram,PF(idbase))==0x90000200 && word(rdram,PF(stamp))==7 &&
        word(rdram,PF(tripwires))!=0 && word(rdram,SF(poisoned))==1,"active event trips without retiring live identity");
    events.push_back({kind,a0,a1,snapshot(rdram)});
    // Deliberate imported-call effects, not reconstructions of those callees.
    if(kind==Clear) for(uint32_t p=a0;p<a0+12;p+=4) put(rdram,p,0);
    if(kind==Float92864) put(rdram,b+0x78,a1);
    if(kind==Free) put(rdram,owner,0xDEADBEEF);
    if(active.mutate) put(rdram,owner+0x50,bodies+uint32_t(events.size())*body_stride);
    clobber(c);c->r2=guest(result);
}
void retire(RetireKind kind,uint8_t* rdram,recomp_context* c) {
    retire_events.push_back({kind,uint32_t(c->r4),uint32_t(c->r5),uint32_t(c->r6),
        word(rdram,PF(idbase)),word(rdram,PF(stamp)),events.size()});
    c->r2=0; // Accepted helper result; producer retirement effects are outside this fixture.
}
void check_retirements(const Output& out,bool freeing) {
    require(out.retire.size()==2+TOOIE_TROT_SELECTED,
        "exact CPU625/backpack621/selected Trot retirement calls");
    for(unsigned i=0;i<2;++i) {
        const auto& e=out.retire[i];
        require(e.kind==(i==0?CpuRetire:BackpackRetire),"CPU625 retires before backpack621");
        require(e.self==owner && e.requested==0 && e.reason==(freeing?4u:3u),
            "retirement owner, request and init/free reason");
        require(e.original_calls==0 && e.idbase==0x90000200 && e.stamp==7,
            "CPU625/backpack621 retirements precede original calls and lifecycle tombstone");
    }
#if TOOIE_TROT_SELECTED
    const auto& trot=out.retire[2];
    require(trot.kind==TrotRetire && trot.self==owner,
        "Trot retires after CPU625/backpack621 with the same self");
    // Trot retirement is unary; unused a1/a2 are not part of its ABI contract.
    require(trot.original_calls==0 && trot.idbase==0x90000200 && trot.stamp==7,
        "Trot retirement precedes original calls and lifecycle tombstone");
#endif
}
recomp_context context() {
    recomp_context c{};c.r4=guest(owner);c.r29=guest(stack);c.r31=guest(0x80045678);
    c.r16=16;c.r17=17;c.r18=18;c.r19=19;c.r20=20;c.r21=21;c.r22=22;c.r23=23;c.r28=28;c.r30=30;
    c.f20.u64=20;c.f21.u64=21;c.f22.u64=22;c.f23.u64=23;c.f24.u64=24;c.f25.u64=25;
    c.f26.u64=26;c.f27.u64=27;c.f28.u64=28;c.f29.u64=29;c.f30.u64=30;c.f31.u64=31;
    return c;
}
void invoke(uint8_t* rdram,recomp_func_t* fn,unsigned frame) {
    for(uint32_t p=stack-guard;p<stack+32;++p) MEM_B(0,guest(p))=0xA5;
    auto c=context();fn(rdram,&c);
    require(c.r16==16&&c.r17==17&&c.r18==18&&c.r19==19&&c.r20==20&&c.r21==21&&
        c.r22==22&&c.r23==23&&c.r28==28&&c.r30==30&&uint32_t(c.r29)==stack&&
        uint32_t(c.r31)==0x80045678,"callee-saved GPR/SP/RA");
    require(c.f20.u64==20&&c.f21.u64==21&&c.f22.u64==22&&c.f23.u64==23&&c.f24.u64==24&&
        c.f25.u64==25&&c.f26.u64==26&&c.f27.u64==27&&c.f28.u64==28&&c.f29.u64==29&&
        c.f30.u64==30&&c.f31.u64==31,"callee-saved FPR");
    for(uint32_t p=stack-guard;p<stack-frame;++p) require(byte(rdram,p)==0xA5,"below measured frame sentinel");
    // o32 may spill incoming self in its caller-owned16-byte argument home.
    for(uint32_t p=stack+16;p<stack+32;++p) require(byte(rdram,p)==0xA5,"above incoming homes sentinel");
}
Output run(Case test,recomp_func_t* fn,unsigned frame,bool matching=false,bool running=false) {
    Output out{std::vector<uint8_t>(16*1024*1024),{}, {}};auto* rdram=out.ram.data();
    active=test;events.clear();retire_events.clear();inspect_retirement=matching&&!running;inspect_active=matching&&running;
    for(uint32_t p=owner-16;p<owner+0x198;++p) MEM_B(0,guest(p))=0xCC;
    for(uint32_t i=0;i<body_count;++i)
        for(uint32_t p=bodies+i*body_stride-16;p<bodies+i*body_stride+0x128;++p) MEM_B(0,guest(p))=0xA7;
    put(rdram,owner+0x50,bodies);put(rdram,PF(owner),matching?owner:owner+0x1000);
    put(rdram,PF(idbase),0x90000200);put(rdram,PF(stamp),7);put(rdram,SF(epoch),7);
    put(rdram,SF(next_block),2);put(rdram,SF(active),running?1:0);
    put(rdram,paid_entry,matching?owner:owner+0x1000);put(rdram,paid_entry+24,0x90000200);put(rdram,paid_entry+28,7);
    put(rdram,other_entry,matching?owner:owner+0x1000);put(rdram,other_entry+24,0x90000400);
    put(rdram,other_entry+28,7);put(rdram,SF(primary_paid),running?1u:0u);
    put(rdram,PF(buffers),0x80300000);put(rdram,PF(base),0x80300100);
    put(rdram,PF(pool_index),1);put(rdram,PF(begin),0x80300200);put(rdram,PF(end),0);
    put(rdram,PF(current_bone_role),3);put(rdram,PF(emitted_bones),5);put(rdram,PF(emitted_leaf_calls),6);
    invoke(rdram,fn,frame);out.trace=events;out.retire=retire_events;
    inspect_retirement=inspect_active=false;return out;
}
void compare(Case test,bool freeing=false,bool running=false) {
    auto a=run(test,freeing?original_F3880:original_92898,freeing?24:40,false,running);
    auto b=run(test,freeing?func_800F3880:func_80092898,
        freeing?TOOIE_PLAYER_FREE_FRAME_BYTES:TOOIE_PLAYER_INIT_FRAME_BYTES,false,running);
    require(a.retire.empty(),"original has no candidate retirement imports");
    check_retirements(b,freeing);
    require(a.trace==b.trace,"actual original/candidate calls, arguments and intermediate memory");
    const std::vector<Kind> init={Backpack,Set936,Set937,Allocate,Resolve,Resource,Bind,Create,
        Clear,Clear,Clear,Set93370,Set9337C,Float93230,Set931,Lookup,Float92864};
    require(a.trace.size()==(freeing?2:17),"exact original call count");
    for(unsigned i=0;i<a.trace.size();++i)
        require(a.trace[i].kind==(freeing?(i==0?Teardown:Free):init[i]),"explicit original call order");
    for(size_t i=0;i<a.ram.size();++i) {
        if(i>=((stack-guard)&0xFFFFFF)&&i<((stack+32)&0xFFFFFF)) continue;
        require(a.ram[i]==b.ram[i],"all non-stack RAM including width canaries");
    }
}
void lifecycle(bool freeing,bool running) {
    auto out=run({0x12340001,0xABCD8001,false},freeing?func_800F3880:func_80092898,
        freeing?TOOIE_PLAYER_FREE_FRAME_BYTES:TOOIE_PLAYER_INIT_FRAME_BYTES,true,running);
    check_retirements(out,freeing);
    auto* rdram=out.ram.data();
    require(word(rdram,SF(next_block))==2,"lifecycle does not allocate/reset issuer");
    if(!running) require(word(rdram,PF(idbase))==0 && word(rdram,PF(stamp))==(freeing?UINT32_MAX:0),
        "destruction tombstone versus completed init reopening");
    if(!running) require(word(rdram,paid_entry+24)==0 && word(rdram,other_entry+24)==0 &&
        word(rdram,paid_entry+28)==(freeing?UINT32_MAX:0) && word(rdram,other_entry+28)==(freeing?UINT32_MAX:0),
        "every retained entry of the owner retires, not only the last admitted one");
    else {
        require(word(rdram,PF(idbase))==0x90000200 && word(rdram,PF(stamp))==7 &&
            word(rdram,PF(buffers))==0x80300000 && word(rdram,PF(base))==0x80300100 &&
            word(rdram,PF(pool_index))==1 && word(rdram,PF(begin))==0x80300200 &&
            word(rdram,PF(end))==0 && word(rdram,PF(current_bone_role))==3 &&
            word(rdram,PF(emitted_bones))==5 && word(rdram,PF(emitted_leaf_calls))==6 &&
            word(rdram,SF(active))==1,"active lifecycle preserves all paid unwind fields");
        require(word(rdram,PF(tripwires))!=0 && word(rdram,SF(tripwire_count))!=0 &&
            word(rdram,SF(poisoned))==1,"persistent player/global fatal signal and epoch poison");
        require(word(rdram,paid_entry+24)==0x90000200 && word(rdram,other_entry+24)==0x90000400,
            "the paid owner's trip preserves every entry");
    }
}
// PC[8] paid-entry interface: primary_paid (1+slot*4+view) names the open paid entry. Entry [5][2]
// is the paid or last admitted entry; self also retains [0][1] and [3][3]. The original calls and
// the CPU625/backpack621/Trot retirements are unchanged in every case.
uint32_t entry(uint32_t slot,uint32_t view) { return SF(primary)+(slot*4u+view)*32u; }
constexpr uint32_t foreign=owner+0x1000;
Output fullview_run(bool freeing,uint32_t paid_owner,uint32_t paid,uint32_t open) {
    Output out{std::vector<uint8_t>(16*1024*1024),{},{}};auto* rdram=out.ram.data();
    active={0x12340001,0xABCD8001,false};events.clear();retire_events.clear();
    for(uint32_t p=owner-16;p<owner+0x198;++p) MEM_B(0,guest(p))=0xCC;
    for(uint32_t i=0;i<body_count;++i)
        for(uint32_t p=bodies+i*body_stride-16;p<bodies+i*body_stride+0x128;++p) MEM_B(0,guest(p))=0xA7;
    put(rdram,owner+0x50,bodies);put(rdram,SF(epoch),7);put(rdram,SF(next_block),40);
    put(rdram,SF(active),open);put(rdram,SF(primary_paid),paid);
    // The global scope snapshots entry [5][2] with the identity check_retirements expects.
    put(rdram,PF(owner),paid_owner);put(rdram,PF(idbase),0x90000200);put(rdram,PF(stamp),7);
    put(rdram,entry(5,2),paid_owner);put(rdram,entry(5,2)+24,0x90000200);put(rdram,entry(5,2)+28,7);
    put(rdram,entry(0,1),owner);put(rdram,entry(0,1)+24,0x90000400);put(rdram,entry(0,1)+28,7);
    put(rdram,entry(3,3),owner);put(rdram,entry(3,3)+24,0x90000600);put(rdram,entry(3,3)+28,6);
    invoke(rdram,freeing?func_800F3880:func_80092898,freeing?TOOIE_PLAYER_FREE_FRAME_BYTES:TOOIE_PLAYER_INIT_FRAME_BYTES);
    out.trace=events;out.retire=retire_events;check_retirements(out,freeing);
    require(out.trace.size()==(freeing?2u:17u),"original calls continue in every paid-entry case");
    return out;
}
void fullview_lifecycle(bool freeing) {
    const uint32_t retired=freeing?UINT32_MAX:0u;
    // F1: self owns the paid entry [5][2] (primary_paid 23): unsupported reentry reaches the
    // unchanged global fatal word and retires nothing.
    {
        auto out=fullview_run(freeing,owner,1u+5u*4u+2u,1u);auto* rdram=out.ram.data();
        require(word(rdram,PF(tripwires))!=0 && word(rdram,SF(tripwire_count))!=0 && word(rdram,SF(poisoned))==1,
            "a paid (5,2) owner reentry reaches the global fatal word");
        require(word(rdram,entry(5,2)+24)==0x90000200 && word(rdram,entry(0,1)+24)==0x90000400 &&
            word(rdram,entry(3,3)+24)==0x90000600 && word(rdram,PF(idbase))==0x90000200 &&
            word(rdram,SF(active))==1 && word(rdram,SF(primary_paid))==23,"the paid trip retires nothing");
    }
    // F2: another owner's (5,2) scope is paid: self's inactive entries retire; that scope, its
    // entry and the fatal words are untouched.
    {
        auto out=fullview_run(freeing,foreign,23u,1u);auto* rdram=out.ram.data();
        require(!word(rdram,PF(tripwires)) && !word(rdram,SF(tripwire_count)) && !word(rdram,SF(poisoned)),
            "a foreign paid scope is not this owner's reentry");
        require(!word(rdram,entry(0,1)+24) && word(rdram,entry(0,1)+28)==retired &&
            !word(rdram,entry(3,3)+24) && word(rdram,entry(3,3)+28)==retired,"every inactive entry of self retires");
        require(word(rdram,entry(5,2))==foreign && word(rdram,entry(5,2)+24)==0x90000200 &&
            word(rdram,entry(5,2)+28)==7 && word(rdram,PF(owner))==foreign && word(rdram,PF(idbase))==0x90000200 &&
            word(rdram,PF(stamp))==7 && word(rdram,SF(active))==1 && word(rdram,SF(primary_paid))==23,
            "the foreign paid scope and its entry survive");
    }
    // F3: a nonplayer scope is open (active, primary_paid 0) and self was last admitted: every
    // entry and the global snapshot of self retire without a trip; the open scope stays open.
    {
        auto out=fullview_run(freeing,owner,0u,1u);auto* rdram=out.ram.data();
        require(!word(rdram,PF(tripwires)) && !word(rdram,SF(tripwire_count)) && !word(rdram,SF(poisoned)),
            "another family's open scope is not this owner's reentry");
        for(const uint32_t e:{entry(5,2),entry(0,1),entry(3,3)})
            require(!word(rdram,e+24) && word(rdram,e+28)==retired,"all entries of self retire");
        require(!word(rdram,PF(idbase)) && word(rdram,PF(stamp))==retired && word(rdram,SF(active))==1 &&
            !word(rdram,SF(primary_paid)),"the global snapshot retires and the nonplayer scope stays open");
    }
}
}
#define STUB(name,kind) extern "C" void name(uint8_t* r,recomp_context* c) { stub(kind,r,c); }
STUB(_babackpack_entrypoint_9,Backpack)
STUB(func_800936E8,Set936)
STUB(func_80093700,Set937)
STUB(func_800AE020,Allocate)
STUB(func_800AE080,Resolve)
STUB(func_800E09B8,Resource)
STUB(func_800AE6FC,Bind)
STUB(func_800DBFF8,Create)
STUB(func_800EFD24,Clear)
STUB(func_80093370,Set93370)
STUB(func_8009337C,Set9337C)
STUB(func_80093230,Float93230)
STUB(func_800931AC,Set931)
STUB(func_80092778,Lookup)
STUB(func_80092864,Float92864)
STUB(func_80098E64,Teardown)
STUB(heap_free,Free)
#undef STUB
extern "C" void tooie_cpu625_retire(uint8_t* r,recomp_context* c) { retire(CpuRetire,r,c); }
extern "C" void tooie_backpack621_retire(uint8_t* r,recomp_context* c) { retire(BackpackRetire,r,c); }
#if TOOIE_TROT_SELECTED
extern "C" void tooie_trot_toggle_retire(uint8_t* r,recomp_context* c) {
    // Only r4 is an argument of this unary helper; never read unused a1/a2.
    require(uint32_t(c->r4)==owner,"Trot retirement stable self argument");
    retire_events.push_back({TrotRetire,uint32_t(c->r4),0,0,
        word(r,PF(idbase)),word(r,PF(stamp)),events.size()});
    c->r2=0;
}
#endif
int main() {
    try {
        const std::array<uint32_t,2> edges[]={{0x12340001,0xABCD0001},{0xFFFF7FFF,0x12347FFF},
            {0x00008000,0xABCD8000},{0x1234FFFF,0xFFFF8001}};
        for(const auto& values:edges)
            for(bool mutate:{false,true}) compare({values[0],values[1],mutate});
        compare({1,1,false},true);compare({1,1,true},true);
        compare({1,1,false},false,true);compare({1,1,false},true,true); // Unaffected active owner.
        for(bool freeing:{false,true}) for(bool running:{false,true}) lifecycle(freeing,running);
        for(bool freeing:{false,true}) fullview_lifecycle(freeing);
        std::cout<<"Player lifecycle actual-body differential and retirement controls passed\n";return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
