// BK selection, L01/L02: the actual translated native-seam policy helpers (bk_pipeline.c tail) and the sort
// epilogue's one-sided return (bk_sort.inc). Each case compares the helper with the decision the native thunk
// made before this batch, restated in `former` below, and with the shared pure rule (bk_pipeline.h).
// Synthetic RAM only: no hook, DE498, RT64, runtime or visual claim.
#include "recomp.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "../tools/baseline/guest/silo-candidate/bk_pipeline.h"
#include "bk_pipeline_selection_layout.h"
#include "camera_identity_layout.h"
#include "../tools/baseline/guest/camera-candidate/camera_identity.h"
static_assert(TOOIE_BK_PIPELINE_SELECTED==1u && TOOIE_BK_HELPERS==46u,"BK policy fixture requires the BK image with its policy helpers");
#ifndef TOOIE_BK_POLICY_FIXTURE_FRAME_BYTES
#error Define the measured helper frame bound from the selected receipt
#endif
// The child traversal tooie_bk_sort_children calls: a recording double (defined below).
extern "C" void func_800DE2A4(uint8_t*, recomp_context*);
namespace candidate {
#include "bk_policy_fixture.inc"
}
namespace {
constexpr uint32_t bk=TOOIE_BK_STATE, cs=TOOIE_CAMERA_STATE_VRAM;
constexpr uint32_t stack=0x80700000u, guard=1024u;
constexpr uint32_t player=0x80400000u, body=0x80410000u, buffers=0x80420000u, gfx=0x80430000u, node=0x80500000u;
constexpr uint32_t f_offset=bk+offsetof(TooieBkState,cur_model_transform_id_offset);
constexpr uint32_t f_frames=bk+offsetof(TooieBkState,diagnostics)+offsetof(TooieBkDiagnostics,frames);
constexpr uint32_t f_commits=bk+offsetof(TooieBkState,branch_commits);
constexpr uint32_t f_branches=bk+offsetof(TooieBkState,branches), f_shadow=bk+offsetof(TooieBkState,shadow_branches);
constexpr uint32_t f_role=cs+offsetof(TooieCameraState,control)+offsetof(TooieCameraControl,role);
static_assert(TOOIE_BK_POLICY_FIXTURE_FRAME_BYTES<guard,"finite measured helper frames");
static_assert(stack+32u<=bk && stack+32u<=cs,"stack below the image state");
std::vector<uint8_t> ram;
unsigned child_calls=0; uint32_t child_node[2]{}, child_offset[2]{};
gpr guest(uint32_t a){return S32(a);}
uint32_t word(uint8_t* rdram,uint32_t a){return uint32_t(MEM_W(0,guest(a)));}
void put(uint8_t* rdram,uint32_t a,uint32_t v){MEM_W(0,guest(a))=v;}
uint8_t byte(uint8_t* rdram,uint32_t a){return uint8_t(MEM_BU(0,guest(a)));}
void put_byte(uint8_t* rdram,uint32_t a,uint8_t v){MEM_B(0,guest(a))=v;}
int16_t half(uint8_t* rdram,uint32_t a){return int16_t(MEM_H(0,guest(a)));}
void put_half(uint8_t* rdram,uint32_t a,int16_t v){MEM_H(0,guest(a))=v;}
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
uint32_t bits(float v){uint32_t w;std::memcpy(&w,&v,4);return w;}
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
// One actual helper call: o32 arguments a0..a2, stack canaries around the measured frames.
uint32_t call(uint8_t* rdram,recomp_func_t* fn,uint32_t a0=0,uint32_t a1=0,uint32_t a2=0){
    for(uint32_t a=stack-guard;a<stack+32;++a)put_byte(rdram,a,0xA5);
    auto c=context();c.r4=guest(a0);c.r5=guest(a1);c.r6=guest(a2);const auto before=c;
    fn(rdram,&c);saved(c,before);
    for(uint32_t a=stack-guard;a<stack-TOOIE_BK_POLICY_FIXTURE_FRAME_BYTES;++a)require(byte(rdram,a)==0xA5,"write below the measured frames");
    for(uint32_t a=stack+0x10;a<stack+32;++a)require(byte(rdram,a)==0xA5,"write above the argument homes");
    return uint32_t(c.r2);
}
uint8_t* fresh(){
    ram.assign(16*1024*1024,0);uint8_t* rdram=ram.data();child_calls=0;
    put(rdram,bk,TOOIE_BK_MAGIC);put(rdram,bk+4,TOOIE_BK_LAYOUT);return rdram;
}
constexpr uint32_t table_words=uint32_t(sizeof(TooieBkBranch)*TOOIE_BK_BRANCH_ENTRIES/4u);
void store(uint8_t* rdram,uint32_t at,const TooieBkBranch* table){
    uint32_t raw[table_words];std::memcpy(raw,table,sizeof(raw));
    for(uint32_t i=0;i<table_words;++i)put(rdram,at+4u*i,raw[i]);
}
void load(uint8_t* rdram,uint32_t at,TooieBkBranch* table){
    uint32_t raw[table_words];for(uint32_t i=0;i<table_words;++i)raw[i]=word(rdram,at+4u*i);
    std::memcpy(table,raw,sizeof(raw));
}
// The removed native decisions (bk_pipeline_thunks.hpp before L01), restated.
namespace former {
bool lod_word(uint32_t address,uint32_t bytes){return !(address&3u)&&address>=0x80000000u&&uint64_t(address)+bytes<=0x80800000ull;}
bool table_resource(uint32_t r){return r==0x62Au||r==0x62Bu||r==0x838u||r==0x839u;}
bool alias_body(uint8_t* rdram,uint32_t owner){
    if(!lod_word(owner,0x54))return false;
    const uint32_t b=word(rdram,owner+0x50);
    if(!lod_word(b,0x110))return false;
    return half(rdram,b+0x0C)==0x607&&table_resource(word(rdram,b+0x10C));
}
uint32_t initial(uint8_t* rdram,uint32_t owner,uint32_t tier){
    if(tier!=1u||!lod_word(owner,0x54))return tier;
    const uint32_t b=word(rdram,owner+0x50);
    return lod_word(b,0x10)&&half(rdram,b+0x0C)==0x607?0u:tier;
}
}

unsigned lod_cases(){
    unsigned cases=0;
    {   uint8_t* rdram=fresh();
        require(call(rdram,candidate::tooie_bk_lod_distance)==bits(2500.0f)&&TOOIE_BK_LOD_DISTANCE_BITS==bits(2500.0f),"GeoCmd8 squared distance 2500.0f");
        require(call(rdram,candidate::tooie_bk_bbox_visible)==1u,"every bbox-gated child is traversed");
        cases+=2;
    }
    require(TOOIE_BK_PLAYER_LOD_DISTANCE_BITS==bits(50.0f),"BK lod_patches linear distance 50.0f");
    const int16_t selectors[]={0x607,0x606,0x608,int16_t(-0x607),0};
    const uint32_t resources[]={0x62Au,0x62Bu,0x838u,0x839u,0x629u,0x62Cu,0x837u,0x83Au,0u,0xFFFFFFFFu};
    const uint32_t distance=bits(1234.5f);
    for(int16_t s:selectors)for(uint32_t r:resources){
        uint8_t* rdram=fresh();put(rdram,player+0x50,body);put_half(rdram,body+0x0C,s);put(rdram,body+0x10C,r);
        const uint32_t expect=former::alias_body(rdram,player)?bits(50.0f):distance;
        require(call(rdram,candidate::tooie_bk_player_lod_distance,player,distance)==expect,"player LOD distance equals the former native decision");
        require(tooie_bk_player_lod_distance_rule(s,r,distance)==expect,"shared distance rule equals the former native decision");
        for(uint32_t tier:{0u,1u,2u,0xFFFFFFFFu}){
            const uint32_t t=former::initial(rdram,player,tier);
            require(call(rdram,candidate::tooie_bk_player_lod_tier,player,tier)==t&&tooie_bk_player_lod_tier_rule(s,tier)==t,
                "initial LOD tier equals the former native decision");
        }
        ++cases;
    }
    // Unreadable owners or bodies: misaligned, null, past 8 MiB; alias data is present wherever RAM allows.
    const uint32_t owners[][2]={{player+2u,body},{0u,body},{0x807FFFB0u,body},{player,body+2u},
                                {player,0x807FFF00u},{player,0x807FFFF8u},{player,0x7FFFFFF0u}};
    for(const auto& o:owners){
        uint8_t* rdram=fresh();
        if(!(o[0]&3u)&&o[0]>=0x80000000u)put(rdram,o[0]+0x50,o[1]);
        if(o[1]>=0x80000000u){put_half(rdram,(o[1]&~3u)+0x0C,0x607);put(rdram,(o[1]&~3u)+0x10C,0x62Au);}
        const uint32_t expect=former::alias_body(rdram,o[0])?bits(50.0f):distance;
        require(call(rdram,candidate::tooie_bk_player_lod_distance,o[0],distance)==expect,"unreadable owner: distance as before");
        require(call(rdram,candidate::tooie_bk_player_lod_tier,o[0],1u)==former::initial(rdram,o[0],1u),"unreadable owner: tier as before");
        ++cases;
    }
    return cases;
}

unsigned traversal_cases(){
    unsigned cases=0;
    for(uint32_t start:{41u,0xFFFFFFFFu}){
        uint8_t* rdram=fresh();put(rdram,f_offset,start);
        call(rdram,candidate::tooie_bk_billboard_entry);
        require(word(rdram,f_offset)==start+1u,"billboard entry advances once, wrapping as the former word add");
        ++cases;
    }
    for(uint8_t flags:{uint8_t(0x00),uint8_t(0x01),uint8_t(0x02),uint8_t(0x03),uint8_t(0xFE),uint8_t(0xFF)}){
        uint8_t* rdram=fresh();put(rdram,f_offset,7u);put_byte(rdram,node+0x21u,flags);
        call(rdram,candidate::tooie_bk_sort_onesided,node);
        const uint32_t advance=(flags&1u)!=0u;   // former: (MEM_BU(0x21,s0)&1)!=0
        require(word(rdram,f_offset)==7u+advance&&tooie_bk_sort_one_sided(flags)==advance,"one-sided advance equals the former native decision");
        put_half(rdram,node+0x22u,0x10);put(rdram,node+0x24u,0x20u);put(rdram,buffers,gfx);
        const uint32_t before=word(rdram,f_offset);
        call(rdram,candidate::tooie_bk_sort_children,buffers,node,0u);
        if(advance)require(!child_calls&&word(rdram,f_offset)==before&&word(rdram,buffers)==gfx,"one-sided epilogue constructs nothing");
        else require(child_calls==2u&&child_node[0]==node+0x10u&&child_offset[0]==before+1u&&
            child_node[1]==node+0x20u&&child_offset[1]==before+2u&&word(rdram,f_offset)==before+2u,"two-sided epilogue: A then B");
        ++cases;
    }
    return cases;
}

unsigned branch_cases(){
    TooieBkBranch players[TOOIE_BK_BRANCH_ENTRIES],shadows[TOOIE_BK_BRANCH_ENTRIES],got[TOOIE_BK_BRANCH_ENTRIES];
    std::memset(players,0,sizeof(players));std::memset(shadows,0,sizeof(shadows));
    auto entry=[](TooieBkBranch& e,TooieBkWord id,TooieBkWord frame,TooieBkWord valid,TooieBkWord committed,TooieBkWord mask){
        e.id=id;e.staged_frame=frame;e.staged_valid=valid;e.committed_valid=committed;
        e.staged.model=0x802ABC30u;e.staged.count=1;e.staged.masks[0]=mask;
        e.committed.model=0x802ABC30u;e.committed.count=1;e.committed.masks[0]=mask^1u;
    };
    entry(players[0],0x20000500u,9,1,0,2);   // staged by the submitted construction: commits
    entry(players[1],0x20000600u,8,1,1,4);   // staged by an earlier construction: unknown
    entry(shadows[0],0x2001C000u,9,0,1,1);   // unsupported in this construction: unknown
    entry(shadows[3],0x2001C100u,9,1,1,8);   // supported: replaces an older baseline
    uint8_t* rdram=fresh();put(rdram,f_frames,9u);
    store(rdram,f_branches,players);store(rdram,f_shadow,shadows);
    call(rdram,candidate::tooie_bk_branch_submitted);
    tooie_bk_branch_commit(players,9u);tooie_bk_branch_commit(shadows,9u);
    load(rdram,f_branches,got);require(!std::memcmp(got,players,sizeof(got)),"player table equals the shared commit rule");
    load(rdram,f_shadow,got);require(!std::memcmp(got,shadows,sizeof(got)),"shadow table equals the shared commit rule");
    require(players[0].committed_valid&&!players[1].committed_valid&&!shadows[0].committed_valid&&shadows[3].committed_valid,"commit outcomes");
    require(word(rdram,f_commits)==1u,"one counted submission");
    put(rdram,f_commits,0xFFFFFFFFu);call(rdram,candidate::tooie_bk_branch_submitted);
    require(word(rdram,f_commits)==0xFFFFFFFFu,"the commit counter saturates as before");
    return 3;
}

unsigned role_cases(){
    unsigned cases=0;
    for(uint32_t enclosing:{uint32_t(TOOIE_CAMERA_WORLD_INITIAL),uint32_t(TOOIE_CAMERA_PRE_WORLD),0u})
    for(uint32_t view=0;view<=TOOIE_CAMERA_TRANSITION_VIEWS;++view)for(uint32_t cut=0;cut<2u;++cut){
        uint8_t* rdram=fresh();put(rdram,f_role,enclosing);
        const uint32_t previous=call(rdram,candidate::tooie_bk_transition_role_begin,view,cut);
        // former: TOOIE_CAMERA_TRANSITION_ROLE_FIRST+L.view | (fresh?TOOIE_CAMERA_TRANSITION_ROLE_SKIP:0u)
        const uint32_t expect=view<TOOIE_CAMERA_TRANSITION_VIEWS?
            (TOOIE_CAMERA_TRANSITION_ROLE_FIRST+view|(cut?TOOIE_CAMERA_TRANSITION_ROLE_SKIP:0u)):enclosing;
        require(previous==enclosing&&word(rdram,f_role)==expect,"transition role equals the former native write; none outside the views");
        call(rdram,candidate::tooie_bk_transition_role_end,previous);
        require(word(rdram,f_role)==enclosing,"the enclosing role is restored");
        ++cases;
    }
    return cases;
}
}
extern "C" void func_800DE2A4(uint8_t* rdram,recomp_context* ctx){
    if(child_calls<2u){child_node[child_calls]=uint32_t(ctx->r5);child_offset[child_calls]=word(rdram,f_offset);}
    ++child_calls;
}
int main(){
    try{
        const unsigned lod=lod_cases(),traversal=traversal_cases(),branch=branch_cases(),role=role_cases();
        require(lod==59u&&traversal==8u&&branch==3u&&role==36u,"every policy case ran");
        std::cout<<"baseline_bk_policy: ok ("<<lod<<" LOD, "<<traversal<<" traversal, "<<branch<<" branch, "<<role
                 <<" role cases; translated helpers vs the former native decisions)\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<"baseline_bk_policy: "<<e.what()<<"\n";return 1;}
}
