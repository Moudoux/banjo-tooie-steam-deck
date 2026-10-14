// Actual compiled/translated DCF48 only; expected operations from original source.
#include "recomp.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#ifdef TOOIE_PLAYER_STATE_ADDRESS
#include "../tools/baseline/guest/silo-candidate/render_bridge.h"
#include <cstddef>
#endif
#ifndef TOOIE_DCF48_FRAME_BYTES
#error Define TOOIE_DCF48_FRAME_BYTES from the actual compiled prologue
#endif
extern "C" {
void func_800ADE2C(uint8_t*, recomp_context*);
void func_80019AA0(uint8_t*, recomp_context*);
void mlMtxApply(uint8_t*, recomp_context*);
void func_800DE2A4(uint8_t*, recomp_context*);
void mlMtxPop(uint8_t*, recomp_context*);
}
void tooie_player_bone_push(uint8_t*, recomp_context*);
void tooie_player_bone_pop(uint8_t*, recomp_context*);
void func_800DCF48(uint8_t*, recomp_context*);
#include "dcf48_fixture.inc"
namespace {
constexpr uint32_t animation_global=0x8012C968, combined_matrix=0x8012CE58;
constexpr uint32_t buffers=0x80010000, node=0x80011000, animation=0x80012000;
constexpr uint32_t bone_matrix=0x80013000, gfx=0x80020000, mtx=0x80030000;
constexpr uint32_t stack=0x8007F000, stack_guard=512;
constexpr uint32_t child_opcode=0xA1234567, child_payload=0x89ABCDEF;
static_assert(TOOIE_DCF48_FRAME_BYTES>0 && TOOIE_DCF48_FRAME_BYTES<stack_guard,
    "finite compiled guest frame");
enum Event { Lookup, Combine, Apply, Child, Pop };
std::vector<Event> events;
bool child_enabled, mutate_node_after_lookup;
int mutation;
#ifdef TOOIE_PLAYER_STATE_ADDRESS
constexpr uint32_t state=TOOIE_PLAYER_STATE_ADDRESS;
constexpr uint32_t pc=state+offsetof(TooieRenderBridgeState,player);
bool semantic_mode=false, nested_child=false, poison_child=false;
unsigned apply_count=0;
#endif
gpr guest(uint32_t a) { return S32(a); }
uint32_t word(uint8_t* rdram, uint32_t a) { return uint32_t(MEM_W(0,guest(a))); }
void put(uint8_t* rdram, uint32_t a, uint32_t v) { MEM_W(0,guest(a))=v; }
uint8_t byte(uint8_t* rdram, uint32_t a) { return uint8_t(MEM_BU(0,guest(a))); }
void put_byte(uint8_t* rdram, uint32_t a, uint8_t v) { MEM_B(0,guest(a))=v; }
void require(bool yes, const char* why) { if(!yes) throw std::runtime_error(why); }
void clobber(recomp_context* c) {
    c->r1=0x101;c->r2=0xDEAD;c->r3=0xBEEF;c->r4=0x104;c->r5=0x105;
    c->r6=0x106;c->r7=0x107;c->r8=0x108;c->r9=0x109;c->r10=0x10A;
    c->r11=0x10B;c->r12=0x10C;c->r13=0x10D;c->r14=0x10E;c->r15=0x10F;
    c->r24=0x118;c->r25=0x119;c->hi=0x1234;c->lo=0x5678;
    c->f0.u64=0x100;c->f1.u64=0x101;c->f2.u64=0x102;c->f3.u64=0x103;
    c->f4.u64=0x104;c->f5.u64=0x105;c->f6.u64=0x106;c->f7.u64=0x107;
    c->f8.u64=0x108;c->f9.u64=0x109;c->f10.u64=0x10A;c->f11.u64=0x10B;
    c->f12.u64=0x10C;c->f13.u64=0x10D;c->f14.u64=0x10E;c->f15.u64=0x10F;
    c->f16.u64=0x110;c->f17.u64=0x111;c->f18.u64=0x112;c->f19.u64=0x113;
}
recomp_context context() {
    recomp_context c{};c.r4=guest(buffers);c.r5=guest(node);c.r29=guest(stack);c.r31=guest(0x80045678);
    c.r16=16;c.r17=17;c.r18=18;c.r19=19;c.r20=20;c.r21=21;c.r22=22;c.r23=23;
    c.r28=28;c.r30=30;c.f20.u64=20;c.f21.u64=21;c.f22.u64=22;c.f23.u64=23;
    c.f24.u64=24;c.f25.u64=25;c.f26.u64=26;c.f27.u64=27;c.f28.u64=28;c.f29.u64=29;
    c.f30.u64=30;c.f31.u64=31;
    return c;
}
void invoke(uint8_t* rdram) {
    for(uint32_t a=stack-stack_guard;a<stack+32;++a) put_byte(rdram,a,0xA5);
    auto c=context(), before=c;
    func_800DCF48(rdram,&c);
    require(c.r16==before.r16&&c.r17==before.r17&&c.r18==before.r18&&c.r19==before.r19&&
        c.r20==before.r20&&c.r21==before.r21&&c.r22==before.r22&&c.r23==before.r23&&
        c.r28==before.r28&&c.r30==before.r30&&c.r29==before.r29&&c.r31==before.r31,
        "callee-saved GPR/SP/RA restoration");
    require(c.f20.u64==20&&c.f21.u64==21&&c.f22.u64==22&&c.f23.u64==23&&c.f24.u64==24&&
        c.f25.u64==25&&c.f26.u64==26&&c.f27.u64==27&&c.f28.u64==28&&c.f29.u64==29&&
        c.f30.u64==30&&c.f31.u64==31,"callee-saved FPR restoration");
    uint32_t deepest=TOOIE_DCF48_FRAME_BYTES;
#ifdef TOOIE_PLAYER_STATE_ADDRESS
    deepest*=semantic_mode&&apply_count>1?2u:1u;
    deepest+=TOOIE_PLAYER_BONE_PUSH_FRAME_BYTES>TOOIE_PLAYER_BONE_POP_FRAME_BYTES?
        TOOIE_PLAYER_BONE_PUSH_FRAME_BYTES:TOOIE_PLAYER_BONE_POP_FRAME_BYTES;
#endif
    for(uint32_t a=stack-stack_guard;a<stack-deepest;++a)
        require(byte(rdram,a)==0xA5,"write below compiled frame");
    for(uint32_t a=stack;a<stack+32;++a) require(byte(rdram,a)==0xA5,"write above compiled frame");
}
void run(bool initial_animation, bool child, int child_mutation, bool late_child) {
    std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
    events.clear();child_enabled=child;mutation=child_mutation;mutate_node_after_lookup=late_child;
    put(rdram,animation_global,initial_animation?animation:0);
    put(rdram,buffers,gfx);put(rdram,buffers+4,mtx);
    put_byte(rdram,node+9,0xFF);put_byte(rdram,node+8,(child&&!late_child)?0x80:0);
    for(uint32_t a=gfx-16;a<gfx+96;a+=4) put(rdram,a,0xCCCCCCCC);
    for(uint32_t a=mtx-16;a<mtx+80;a+=4) put(rdram,a,0xDDDDDDDD);
    invoke(rdram);
    const bool final_animation=child_mutation==1?false:child_mutation==2?true:initial_animation;
    std::vector<Event> expected;
    std::vector<uint32_t> words;
    if(initial_animation) {
        expected.insert(expected.end(),{Lookup,Combine,Apply});
        words.insert(words.end(),{0xDA380002,mtx});
    }
    if(child) { expected.push_back(Child);words.insert(words.end(),{child_opcode,child_payload}); }
    if(final_animation) { expected.push_back(Pop);words.insert(words.end(),{0xD8380002,0x40}); }
    require(events==expected,"original ordered calls / independent conditions");
    require(word(rdram,buffers)==gfx+words.size()*4,"exact Gfx cursor advancement");
    require(word(rdram,buffers+4)==mtx+(initial_animation?64:0),"exact original Mtx advancement");
    for(unsigned i=0;i<words.size();++i) require(word(rdram,gfx+i*4)==words[i],"exact original command bytes");
    for(uint32_t a=gfx-16;a<gfx;a+=4) require(word(rdram,a)==0xCCCCCCCC,"Gfx leading canary");
    for(uint32_t a=gfx+uint32_t(words.size()*4);a<gfx+96;a+=4)
        require(word(rdram,a)==0xCCCCCCCC,"Gfx trailing canary / no additional commands");
    for(unsigned i=0;i<16;++i)
        require(word(rdram,mtx+4*i)==(initial_animation?0xABC00000u+i:0xDDDDDDDDu),"original fixed matrix bytes");
    for(uint32_t a: {mtx-16,mtx-12,mtx-8,mtx-4,mtx+64,mtx+68,mtx+72,mtx+76})
        require(word(rdram,a)==0xDDDDDDDD,"Mtx extent canary");
    require(word(rdram,animation_global)==(final_animation?animation:0),"child mutation retained");
}
#ifdef TOOIE_PLAYER_STATE_ADDRESS
uint32_t bone_push(uint8_t* rdram,int index){
    auto c=context();c.r5=guest(uint32_t(index));const auto before=c;
    tooie_player_bone_push(rdram,&c);
    require(c.r29==before.r29&&c.r31==before.r31&&c.r16==before.r16,"push helper saved registers");
    return uint32_t(c.r2);
}
void bone_pop(uint8_t* rdram,uint32_t token=1){auto c=context();c.r5=token;tooie_player_bone_pop(rdram,&c);}
void semantic_fixture(unsigned which){
    std::vector<uint8_t> ram(16*1024*1024);auto* rdram=ram.data();
    semantic_mode=true;nested_child=which==0;poison_child=which==1;apply_count=0;events.clear();
    child_enabled=nested_child||poison_child;mutate_node_after_lookup=false;mutation=0;
    put(rdram,pc+96,UINT32_MAX);
    put(rdram,state+8,1);put(rdram,state+16,3792);put(rdram,state+28,1);
    put(rdram,state+offsetof(TooieRenderBridgeState,silo_depth),1);
    put(rdram,pc,1);put(rdram,pc+7*4,1);put(rdram,pc+8*4,0x90000000);
    put(rdram,pc+9*4,buffers);put(rdram,pc+10*4,gfx-0x100);
    put(rdram,pc+12*4,gfx-24);
    put(rdram,0x8007BCD0,gfx-0x100);MEM_H(0,guest(0x8007BEDA))=0;
    put(rdram,animation_global,animation);put(rdram,animation,68);put(rdram,animation+4,68);
    put(rdram,buffers,gfx);put(rdram,buffers+4,mtx);
    put_byte(rdram,node+9,0);put_byte(rdram,node+8,child_enabled?128:0);
    put_byte(rdram,node+128+9,67);put_byte(rdram,node+128+8,0);
    auto check_group=[&](uint32_t at,uint32_t id){
        require(word(rdram,at)==0x6400000C&&word(rdram,at+4)==id&&
            word(rdram,at+8)==0x02010AA9&&word(rdram,at+12)==0,
            "semantic SIMPLE/LINEAR group skips local vertices/TC");
    };
    if(which<=1){
        invoke(rdram);check_group(gfx,0x90000001);
        require(word(rdram,gfx+16)==0xDA380002&&word(rdram,gfx+20)==mtx,"original absolute LOAD/PUSH retained");
        if(which==0){
            check_group(gfx+24,0x90000044);
            require(word(rdram,gfx+40)==0xDA380002&&word(rdram,gfx+44)==mtx+64,"nested bone matrix retained");
            require(word(rdram,gfx+48)==0xD8380002&&word(rdram,gfx+56)==0x6400000D&&word(rdram,gfx+60)==1&&
                word(rdram,gfx+64)==0xD8380002&&word(rdram,gfx+72)==0x6400000D&&word(rdram,gfx+76)==1,
                "nested original matrix pops and semantic pops unwind in order");
            require(word(rdram,buffers)==gfx+80&&word(rdram,pc+92)==2,"two nested successful scopes");
        }else{
            require(word(rdram,state+24)==1&&word(rdram,gfx+32)==0xD8380002&&
                word(rdram,gfx+40)==0x6400000D&&word(rdram,buffers)==gfx+48,
                "successful local scope unwinds after child poison");
            require(word(rdram,pc+96)==UINT32_MAX,"poisoned scope restores role context");
        }
    }else if(which==2){
        put(rdram,pc+92,77);
        require(bone_push(rdram,67)==1&&word(rdram,pc+92)==78,"78th push fits prepaid bound");
        check_group(gfx,0x90000044);const uint32_t cursor=word(rdram,buffers);
        require(bone_push(rdram,0)==0&&word(rdram,buffers)==cursor&&word(rdram,state+24)==1,
            "79th request trips before command emission");
        bone_pop(rdram);require(word(rdram,cursor)==0x6400000D,"78th successful scope pops despite poison");
    }else if(which==3){
        put(rdram,pc+13*4,gfx); // Completed body, then same-epoch Honey sets shared active.
        require(bone_push(rdram,0)==0&&word(rdram,buffers)==gfx&&word(rdram,pc+92)==0&&
            word(rdram,state+24)==0,"closed body cannot tag later Honey bones");
    }else if(which==4){
        require(bone_push(rdram,-1)==1,"ADE2C special root -1 is valid");
        check_group(gfx,0x90000000);
        require(bone_push(rdram,67)==2&&word(rdram,pc+96)==68,"nested token preserves role0");
        bone_pop(rdram,2);require(word(rdram,pc+96)==0,"nested pop restores role0");
        bone_pop(rdram);require(word(rdram,pc+96)==UINT32_MAX,"outer pop restores invalid role");
        const uint32_t cursor=word(rdram,buffers);
        require(bone_push(rdram,-2)==0&&word(rdram,buffers)==cursor&&word(rdram,state+24)==1,
            "unsupported negative index trips without push");
    }else if(which==6){
        put(rdram,pc+88,0x62B);
        require(bone_push(rdram,65)==1&&word(rdram,pc+96)==66,"62B index65 is final admitted bone");
        check_group(gfx,0x90000042);bone_pop(rdram);
        const uint32_t cursor=word(rdram,buffers);
        require(bone_push(rdram,66)==0&&word(rdram,buffers)==cursor&&word(rdram,state+24)==1,
            "62B index66 refuses before public writes");
    }else{
        require(bone_push(rdram,68)==0&&word(rdram,buffers)==gfx&&word(rdram,state+24)==1,
            "index at68-entry pose end trips without push");
    }
    require(word(rdram,state+16)==3792,"bone helpers use prepaid allowance without a second debit");
    semantic_mode=false;nested_child=false;poison_child=false;
}
#endif
}
extern "C" void func_800ADE2C(uint8_t* rdram, recomp_context* c) {
    events.push_back(Lookup);
    require(c->r4==guest(animation),"current C968 lookup argument");
#ifdef TOOIE_PLAYER_STATE_ADDRESS
    if(semantic_mode) require(c->r5==guest(byte(rdram,node+9))||c->r5==guest(67),"semantic original bone index");
    else
#endif
    require(c->r5==gpr(-1),"signed original bone -1");
    if(mutate_node_after_lookup) put_byte(rdram,node+8,0x80);
    clobber(c);c->r2=guest(bone_matrix);
}
extern "C" void func_80019AA0(uint8_t*, recomp_context* c) {
    events.push_back(Combine);
    require(c->r4==guest(combined_matrix)&&c->r5==guest(bone_matrix),"absolute CE58 and ADE2C result");
    clobber(c);
}
extern "C" void mlMtxApply(uint8_t* rdram, recomp_context* c) {
    events.push_back(Apply);uint32_t applied=mtx;
#ifdef TOOIE_PLAYER_STATE_ADDRESS
    if(semantic_mode) applied+=64*apply_count++;
#endif
    require(c->r4==guest(applied),"Apply current Mtx argument");
    for(unsigned i=0;i<16;++i) put(rdram,applied+4*i,0xABC00000+i);
    clobber(c);
}
extern "C" void func_800DE2A4(uint8_t* rdram, recomp_context* c) {
    events.push_back(Child);
    require(child_enabled&&c->r4==guest(buffers)&&c->r5==guest(node+128),"unsigned child offset and original buffers");
#ifdef TOOIE_PLAYER_STATE_ADDRESS
    if(semantic_mode){
        if(nested_child){nested_child=false;c->r4=guest(buffers);c->r5=guest(node+128);func_800DCF48(rdram,c);}
        else {uint32_t cursor=word(rdram,buffers);put(rdram,cursor,child_opcode);put(rdram,cursor+4,child_payload);
            put(rdram,buffers,cursor+8);put(rdram,state+24,1);}
        clobber(c);return;
    }
#endif
    const bool loaded=events.size()>1;
    require(word(rdram,buffers)==gfx+(loaded?8:0)&&word(rdram,buffers+4)==mtx+(loaded?64:0),
        "parent cursor stores visible before child");
    if(loaded) require(word(rdram,gfx)==0xDA380002&&word(rdram,gfx+4)==mtx,"parent command visible before child");
    uint32_t current=word(rdram,buffers);
    put(rdram,current,child_opcode);put(rdram,current+4,child_payload);put(rdram,buffers,current+8);
    if(mutation==1) put(rdram,animation_global,0);
    if(mutation==2) put(rdram,animation_global,animation);
    clobber(c);
}
extern "C" void mlMtxPop(uint8_t*, recomp_context* c) { events.push_back(Pop);clobber(c); }
#ifdef TOOIE_PLAYER_STATE_ADDRESS
extern "C" void func_800A89F8(uint8_t*,recomp_context*) { throw std::runtime_error("unexpected Silo view getter"); }
extern "C" void func_800EB854(uint8_t*,recomp_context*) { throw std::runtime_error("unexpected Silo phase getter"); }
#endif
int main() {
    try {
        run(false,true,0,false); // Null still dispatches unsigned child128.
        run(true,true,0,true);   // node8 sampled after helpers, signed bone-1, both original commands.
        run(true,true,1,false);  // Child clears C968: no pop.
        run(false,true,2,false); // Child installs C968: pop without entry load.
        run(true,false,0,false); // Leaf still pops; no dispatcher call.
        unsigned player_cases=0;
#ifdef TOOIE_PLAYER_STATE_ADDRESS
        for(unsigned which=0;which<7;++which){semantic_fixture(which);++player_cases;}
#endif
        std::cout<<"DCF48 actual-body fixture passed: 5 paths; player_cases="<<player_cases
            <<"; frame="<<TOOIE_DCF48_FRAME_BYTES<<'\n';
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
