// Real MIPS-compiled / canonically retranslated guest body; no host SDK ABI model.
#include "recomp.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <memory>
#include "gbi/rt64_gbi_f3dex2.h"
#include "hle/rt64_interpreter.h"
#include "hle/rt64_rsp.h"
#include "hle/rt64_workload_queue.h"
extern "C" {
void func_80014F4C(uint8_t*,recomp_context*);
void func_800A9800(uint8_t*,recomp_context*);
}
#include "graphics_rate_candidate_fixture.inc"
namespace {
constexpr uint32_t store=0x80020000,root=0x80030000,stack=0x8007F000;
constexpr uint32_t tail=TOOIE_GRAPHICS_RATE_TAIL_ADDRESS;
constexpr unsigned frame=TOOIE_GRAPHICS_RATE_FRAME_BYTES,guard=256;
static_assert(tail>=0x80801000 && tail+48<=0x81000000 && !(tail&7));
static_assert(frame>0 && frame<guard);
int32_t sample;
unsigned reads,resets;
gpr guest(uint32_t p) { return S32(p); }
uint32_t word(uint8_t* rdram,uint32_t p) { return uint32_t(MEM_W(0,guest(p))); }
void put(uint8_t* rdram,uint32_t p,uint32_t v) { MEM_W(0,guest(p))=v; }
void require(bool yes,const char* why) { if(!yes)throw std::runtime_error(why); }
// Fixture-only boundary: the real decoder must reach this exact FullSync.
// Do not execute FullSync or claim workload publication without GPU infrastructure.
RT64::DisplayList* expected_fullsync=nullptr;
unsigned fullsync_boundaries=0;
void stop_before_fullsync(RT64::State*,RT64::DisplayList** cursor) {
    require(*cursor==expected_fullsync,"real parser follows branch to exact emitted FullSync");
    ++fullsync_boundaries;*cursor=nullptr;
}
void no_interrupts() {}
struct RateTransport {
    uint32_t mi_interrupt=0;
    std::unique_ptr<RT64::State> state;
    std::unique_ptr<RT64::Interpreter> interpreter;
    std::unique_ptr<RT64::WorkloadQueue> queue;
    RT64::EnhancementConfiguration enhancement;
    RT64::GBI gbi;
    explicit RateTransport(uint8_t* rdram) {
        state=std::make_unique<RT64::State>(rdram,&mi_interrupt,&no_interrupts);
        interpreter=std::make_unique<RT64::Interpreter>();
        queue=std::make_unique<RT64::WorkloadQueue>();
        state->ext.workloadQueue=queue.get();state->ext.enhancementConfig=&enhancement;
        state->ext.interpreter=interpreter.get();interpreter->setup(state.get());
        gbi.ucode=RT64::GBIUCode::F3DEX2;RT64::GBI_F3DEX2::setup(&gbi);
        gbi.map[0xE9]=&stop_before_fullsync;interpreter->hleGBI=&gbi;
        // No framebuffer/GPU is created in this focused command-decoder fixture.
        state->rdramCheckPending=false;state->rsp->segments[0]=0;
    }
    void check(uint8_t* rdram,uint32_t selected,uint32_t expected_rate) {
        state->extended.refreshRate=0xABCD;interpreter->extendedOpCode=0;
        expected_fullsync=reinterpret_cast<RT64::DisplayList*>(rdram+(selected&0x1FFFFFFF)+8);
        fullsync_boundaries=0;
        interpreter->processDisplayLists(root&0x1FFFFFFF,
            reinterpret_cast<RT64::DisplayList*>(rdram+(root&0x1FFFFFFF)));
        require(fullsync_boundaries==1,"one fixture-only stop before FullSync");
        require(interpreter->extendedOpCode==0x64,"real Enable installs extended decoder");
        require(state->extended.refreshRate==expected_rate,"actual emitted rate arrives through RT64 decoder");
        require(state->returnAddressStack.empty(),"real nonpush branch adds no return address");
    }
};
void clobber(recomp_context* c) {
    c->r1=1;c->r2=2;c->r3=3;c->r4=4;c->r5=5;c->r6=6;c->r7=7;
    c->r8=8;c->r9=9;c->r10=10;c->r11=11;c->r12=12;c->r13=13;
    c->r14=14;c->r15=15;c->r24=24;c->r25=25;c->hi=0x1234;c->lo=0x5678;
}
}
extern "C" void func_80014F4C(uint8_t*,recomp_context* c) {
    ++reads;clobber(c);c->r2=S32(sample);
}
extern "C" void func_800A9800(uint8_t* rdram,recomp_context* c) {
    ++resets;
    require(word(rdram,store)==root+16,"original side effect follows both root writes/cursor updates");
    require(word(rdram,root)==0xE0525464 && word(rdram,root+8)==0xDE010000,"original side effect follows public terminal replacement");
    clobber(c);
}
int main() try {
    std::vector<uint8_t> memory(16*1024*1024,0xCD);auto* rdram=memory.data();
    RateTransport transport(rdram);
    const std::array<int32_t,8> samples={2,3,1,0,-1,61,2,2};
    const std::array<uint32_t,8> rates={30,20,60,0xFFFF,0xFFFF,0xFFFF,30,30};
    for(unsigned i=0;i<samples.size();++i) {
        // Last two calls deliberately reuse the same slot and equal sample.
        const unsigned index=i==7?0:i%2;const uint32_t selected=tail+24*index,other=tail+24*(1-index);
        std::array<uint32_t,6> untouched{};for(unsigned k=0;k<6;++k)untouched[k]=word(rdram,other+4*k);
        sample=samples[i];reads=resets=0;put(rdram,store,root);MEM_H(0,guest(0x8007BEDA))=int16_t(index);
        for(unsigned k=0;k<8;++k)put(rdram,root-8+4*k,0xCCCCCCCC);
        for(uint32_t p=stack-guard;p<stack+32;++p)MEM_B(0,guest(p))=0xA5;
        recomp_context c{};c.r4=guest(store);c.r29=guest(stack);c.r31=guest(0x80045678);
        c.r16=16;c.r17=17;c.r18=18;c.r19=19;c.r20=20;c.r21=21;c.r22=22;c.r23=23;c.r28=28;c.r30=30;
        const auto before=c;func_800E7EF4(rdram,&c);
        require(reads==1 && resets==1 && word(rdram,store)==root+16,"one sample, one original side effect, exact16byte cursor");
        require(word(rdram,root)==0xE0525464 && word(rdram,root+4)==0x10000064,"actual Enable command bytes");
        require(word(rdram,root+8)==0xDE010000 && word(rdram,root+12)==(selected&0x1FFFFFFF),"actual nonpush physical branch");
        require(word(rdram,selected)==0x64000009 && word(rdram,selected+4)==rates[i],"actual public refresh command bytes/rate");
        require(word(rdram,selected+8)==0xE9000000 && word(rdram,selected+12)==0,"actual FullSync after rate");
        require(word(rdram,selected+16)==0xDF000000 && word(rdram,selected+20)==0,"actual End after FullSync");
        require(word(rdram,root-4)==0xCCCCCCCC && word(rdram,root+16)==0xCCCCCCCC,"original terminal footprint guard");
        for(unsigned k=0;k<6;++k)require(word(rdram,other+4*k)==untouched[k],"other in-flight slot untouched");
        require(c.r16==before.r16&&c.r17==before.r17&&c.r18==before.r18&&c.r19==before.r19&&
            c.r20==before.r20&&c.r21==before.r21&&c.r22==before.r22&&c.r23==before.r23&&
            c.r28==before.r28&&c.r29==before.r29&&c.r30==before.r30&&c.r31==before.r31,"callee-saved registers/SP/RA");
        for(uint32_t p=stack-guard;p<stack-frame;++p)require(uint8_t(MEM_B(0,guest(p)))==0xA5,"compiled stack lower guard");
        for(uint32_t p=stack;p<stack+32;++p)require(uint8_t(MEM_B(0,guest(p)))==0xA5,"compiled stack upper guard");
        transport.check(rdram,selected,rates[i]);
        put(rdram,selected+4,0xDEAD); // Equal-sample next call must publish again.
    }
    std::cout<<"Actual compiled/retranslated graphics-rate body + real RT64 pre-FullSync decoder transport: 8 cases pass\n";
    return 0;
} catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<"\n";return 1; }
