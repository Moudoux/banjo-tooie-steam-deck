#pragma once
// Mechanical guest ABI marshalling only. Guest leaves own all camera policy.
// Optional shadow receipt below reads original state and changes no guest registers or memory.
#if BASELINE_CAMERA_IDENTITY
#include "demo_timing_layout.h"
#include "shadow_projection_layout.h"
#include "pre_background_layout.h"
#if TOOIE_BASELINE_CAMERA_RUNTIME_OBSERVER
#include "shadow_witness.hpp"
#endif
#if TOOIE_ACTOR_PALETTE_SELECTED
extern "C" void tooie_camera_cpu_root_begin(uint8_t*, recomp_context*);
#endif
extern "C" void tooie_camera_de498_ignore(uint8_t*, recomp_context*);
extern "C" void tooie_camera_construction_begin(uint8_t*, recomp_context*);
extern "C" void tooie_camera_construction_end(uint8_t*, recomp_context*);
#if TOOIE_SHADOW_SELECTED
extern "C" void tooie_shadow_reset(uint8_t*,recomp_context*);
extern "C" void tooie_shadow_source(uint8_t*,recomp_context*);
extern "C" void tooie_shadow_prepare(uint8_t*,recomp_context*);
extern "C" void tooie_shadow_de498(uint8_t*,recomp_context*);
extern "C" void tooie_shadow_rectangle_args(uint8_t*,recomp_context*);
extern "C" void tooie_shadow_mode_begin(uint8_t*,recomp_context*);
extern "C" void tooie_shadow_mode_end(uint8_t*,recomp_context*);
extern "C" void tooie_shadow_complete(uint8_t*,recomp_context*);
#endif
namespace {
void camera_clone_context(recomp_context& clone, const recomp_context& source) {
    clone = source;
    if (source.mips3_float_mode == 0 && source.f_odd == &source.f0.u32h)
        clone.f_odd = &clone.f0.u32h;
    else if (source.mips3_float_mode == 1 && source.f_odd == &source.f1.u32l)
        clone.f_odd = &clone.f1.u32l;
    else
        rejected_body("camera thunk unexpected FR/pointer pair");
    clone.r29 = SUB32(source.r29, 16);
}
}
extern "C" void baseline_camera_de498_ignore(uint8_t* rdram, recomp_context* ctx) {
    recomp_context clone;
    camera_clone_context(clone, *ctx);
    clone.r4 = (gpr)(int32_t)MEM_W(0xF8, ctx->r29);
    clone.r5 = ADD32(ctx->r29, 0xCC);
    clone.r6 = (gpr)(int32_t)MEM_W(0x10C, ctx->r29);
#if TOOIE_SHADOW_SELECTED
    tooie_shadow_de498(rdram, &clone);
#else
    tooie_camera_de498_ignore(rdram, &clone);
#endif
}
#if TOOIE_ACTOR_PALETTE_SELECTED
extern "C" void baseline_actor_cpu_root_begin(uint8_t* rdram, recomp_context* ctx) {
    recomp_context clone;
    camera_clone_context(clone, *ctx);
    clone.r4 = (gpr)(int32_t)MEM_W(0xF8, ctx->r29);
    clone.r5 = ADD32(ctx->r29, 0xCC);
    clone.r6 = (gpr)(int32_t)MEM_W(0x10C, ctx->r29);
    tooie_camera_cpu_root_begin(rdram, &clone);
}
#endif
#if TOOIE_BASELINE_HOST
extern "C" void tooie_baseline_widescreen_frame(uint8_t*,recomp_context*);
#endif
extern "C" void baseline_camera_construction_begin(uint8_t* rdram, recomp_context* ctx) {
    recomp_context clone;
    camera_clone_context(clone, *ctx);
    clone.r4 = ctx->r29;
    clone.r5 = (gpr)(int32_t)MEM_W(0x1C, ctx->r29);
    tooie_camera_construction_begin(rdram, &clone);
#if TOOIE_BASELINE_HOST
    camera_clone_context(clone,*ctx);
    tooie_baseline_widescreen_frame(rdram,&clone);
#endif
#if TOOIE_SHADOW_SELECTED
    camera_clone_context(clone,*ctx);clone.r4=ctx->r29;tooie_shadow_reset(rdram,&clone);
#endif
}
extern "C" void baseline_camera_construction_end(uint8_t* rdram, recomp_context* ctx) {
    recomp_context clone;
    camera_clone_context(clone, *ctx);
    clone.r4 = ctx->r29;
    tooie_camera_construction_end(rdram, &clone);
}

#if TOOIE_SHADOW_SELECTED
namespace {
void shadow_fix_receipt(uint8_t* rdram,const char* phase,uint32_t group=0) {
#if TOOIE_BASELINE_CAMERA_RUNTIME_OBSERVER
    if(!shadow_witness::armed.load(std::memory_order_acquire) || shadow_witness::lines>=256)return;
    const gpr state=(gpr)(int32_t)TOOIE_SHADOW_STATE;uint32_t words[4]{};
    if(group>=0x80000000u && group<=0x80FFFFF0u && !(group&7u))for(uint32_t i=0;i<4;++i)words[i]=uint32_t(MEM_W(i*4,(gpr)(int32_t)group));
    std::fprintf(stderr,"baseline shadow-fix phase=%s line=%u epoch=%u id=%08x group=%08x fullframe=%u mode_open=%u fault=%u observed_group=%08x words=%08x,%08x,%08x,%08x\n",phase,++shadow_witness::lines,
        uint32_t(MEM_W(4,state)),uint32_t(MEM_W(8,state)),uint32_t(MEM_W(20,state)),uint32_t(MEM_W(24,state)),uint32_t(MEM_W(28,state)),uint32_t(MEM_W(32,state)),group,words[0],words[1],words[2],words[3]);
#else
    (void)rdram;(void)phase;(void)group;
#endif
}
void shadow_source(uint8_t* rdram,recomp_context* ctx,uint32_t pc) {
    recomp_context clone;camera_clone_context(clone,*ctx);clone.r4=(gpr)(int32_t)pc;tooie_shadow_source(rdram,&clone);
} }
extern "C" void baseline_shadow_source_local(uint8_t* rdram,recomp_context* ctx) { shadow_source(rdram,ctx,0x800A6CF0u); }
extern "C" void baseline_shadow_source_fallback(uint8_t* rdram,recomp_context* ctx) { shadow_source(rdram,ctx,0x800A6D60u); }
extern "C" void baseline_shadow_prepare(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=(gpr)(int32_t)MEM_W(0x1CC,ctx->r29);clone.r5=(gpr)(int32_t)MEM_W(0x250,ctx->r29);clone.r6=(gpr)(int32_t)MEM_W(0x1C8,ctx->r29);
    tooie_shadow_prepare(rdram,&clone);shadow_fix_receipt(rdram,"prepare");
}
extern "C" void baseline_shadow_fullframe(uint8_t* rdram,recomp_context* ctx) {
    if(uint32_t(ctx->r4)!=0x80127658u)return; // Original A5EC4 buffers owner.
    recomp_context clone;camera_clone_context(clone,*ctx);
    // Five o32 arguments: reserve32 bytes, including the fifth outgoing stack word.
    clone.r29=SUB32(ctx->r29,32);clone.r4=(gpr)(int32_t)MEM_W(0,ctx->r4);
    clone.r5=ctx->r6;clone.r6=ctx->r7;clone.r7=(gpr)(int32_t)MEM_W(0x10,ctx->r29);
    const uint32_t group=uint32_t(MEM_W(20,(gpr)(int32_t)TOOIE_SHADOW_STATE));
    shadow_fix_receipt(rdram,"rectangle-before",group);
    MEM_W(0x10,clone.r29)=MEM_W(0x14,ctx->r29);tooie_shadow_rectangle_args(rdram,&clone);
    shadow_fix_receipt(rdram,"rectangle-after",group);
    if(uint32_t(clone.r2)!=TOOIE_SHADOW_ARGS)rejected_body("shadow rectangle result address");
    ctx->r6=(gpr)(int32_t)MEM_W(0,clone.r2);ctx->r7=(gpr)(int32_t)MEM_W(4,clone.r2);
    MEM_W(0x10,ctx->r29)=MEM_W(8,clone.r2);MEM_W(0x14,ctx->r29)=MEM_W(12,clone.r2);
}
extern "C" void baseline_shadow_mode_begin(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);clone.r4=ctx->r3;tooie_shadow_mode_begin(rdram,&clone);ctx->r3=clone.r2;shadow_fix_receipt(rdram,"mode-begin");
}
extern "C" void baseline_shadow_mode_end(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);clone.r4=ctx->r3;tooie_shadow_mode_end(rdram,&clone);ctx->r3=clone.r2;shadow_fix_receipt(rdram,"mode-end");
}
extern "C" void baseline_shadow_complete(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);clone.r4=(gpr)(int32_t)MEM_W(0,(gpr)(int32_t)0x80127658u);tooie_shadow_complete(rdram,&clone);shadow_fix_receipt(rdram,"complete");
}
#endif

#if TOOIE_DEMO_TIMING_SELECTED
extern "C" void tooie_demo_begin(uint8_t*,recomp_context*);
extern "C" void tooie_demo_record(uint8_t*,recomp_context*);
extern "C" void tooie_demo_wait(uint8_t*,recomp_context*);
namespace {
bool demo_receipt_active=false;
uint32_t demo_receipt_reads=0,demo_receipt_rows=0;
void demo_receipt(uint8_t* rdram,const char* phase,int32_t original,int32_t selected,int32_t available,int32_t recorded) {
    static const bool enabled=[] { const char* p=std::getenv("TOOIE_BASELINE_DEMO_RECEIPT");return p && std::string_view(p)=="1"; }();
    if(!enabled)return;
    if(available>0)++demo_receipt_reads;
    const bool unequal=available<0 && original!=selected;
    if(demo_receipt_rows>=32 || (!unequal && demo_receipt_rows>=16))return;
    std::fprintf(stderr,"baseline demo-timing phase=%s row=%u successful_reads=%u available=%d post_read_index=%d raw_recorded=%d original=%d selected=%d unequal=%u since_last_wait_vi=%u already_waited=%u\n",
        phase,++demo_receipt_rows,demo_receipt_reads,available,int32_t(MEM_W(0,(gpr)(int32_t)0x8012D5B8u)),recorded,original,selected,unsigned(unequal),uint32_t(MEM_W(0,(gpr)(int32_t)0x80079138u)),uint32_t(MEM_W(0,(gpr)(int32_t)0x80079704u)));
    std::fflush(stderr);
}
}
extern "C" void baseline_demo_begin(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);tooie_demo_begin(rdram,&clone);
    demo_receipt_active=false;
}
extern "C" void baseline_demo_record(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=(gpr)(int32_t)MEM_W(0x50,ctx->r29);clone.r5=ctx->r2;tooie_demo_record(rdram,&clone);
    const int32_t available=int32_t(ctx->r2),recorded=int32_t(MEM_W(0x50,ctx->r29));
    if(available)demo_receipt_active=true;
    if(demo_receipt_active)demo_receipt(rdram,"record",int32_t(MEM_W(0,(gpr)(int32_t)0x8007913Cu)),0,available,recorded);
}
namespace { void demo_wait(uint8_t* rdram,recomp_context* ctx,bool first) {
    recomp_context clone;camera_clone_context(clone,*ctx);clone.r4=ctx->r2;clone.r5=first;
    const int32_t original=int32_t(ctx->r2);
    tooie_demo_wait(rdram,&clone);ctx->r2=clone.r2;
    if(demo_receipt_active)demo_receipt(rdram,first?"first":"loop",original,int32_t(ctx->r2),-1,0);
} }
extern "C" void baseline_demo_first(uint8_t* rdram,recomp_context* ctx) { demo_wait(rdram,ctx,true); }
extern "C" void baseline_demo_loop(uint8_t* rdram,recomp_context* ctx) { demo_wait(rdram,ctx,false); }
#endif
#endif

#if TOOIE_BASELINE_CAMERA_RUNTIME_OBSERVER
#include "camera_identity_layout.h"
#include "shadow_witness.hpp"
namespace { uint32_t shadow_observed_actor=0,shadow_light_kind=0;
uint32_t shadow_word(uint8_t* rdram,uint32_t offset) { return uint32_t(MEM_W(offset,(gpr)(int32_t)TOOIE_CAMERA_STATE_VRAM)); }
bool shadow_readable(uint32_t p,uint32_t n) { return p>=0x80000000u && uint64_t(p)+n<=0x81000000ull; }
void shadow_matrix(uint8_t* rdram,const char* role,uint32_t p) {
    if(!shadow_readable(p,64) || (p&7u) || shadow_witness::lines>=256)return;
    uint32_t w[16];for(unsigned i=0;i<16;++i)w[i]=MEM_W(i*4,(gpr)(int32_t)p);
    std::fprintf(stderr,"baseline shadow-witness event=matrix line=%u epoch=%u actor=%08x role=%s address=%08x physical=%08x fixed_words=%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x\n",++shadow_witness::lines,shadow_word(rdram,1032),shadow_observed_actor,role,p,p&0x1FFFFFFFu,w[0],w[1],w[2],w[3],w[4],w[5],w[6],w[7],w[8],w[9],w[10],w[11],w[12],w[13],w[14],w[15]);
    std::fflush(stderr);
}
}
#include "feather_stable_discriminator.hpp"
#include "mumbo665_observer.hpp"
extern "C" void baseline_actor_model_observer(uint8_t* rdram,recomp_context* ctx) { mumbo665_model(rdram,ctx);baseline_feather_stable_model(rdram,ctx); }
extern "C" void baseline_shadow_branch(uint8_t* rdram,recomp_context* ctx) {
    shadow_observed_actor=0;shadow_light_kind=0;const uint32_t actor=uint32_t(ctx->r16);
    if(shadow_witness::forward_mode() && !shadow_witness::forward_ready.load(std::memory_order_acquire))return;
    if(!shadow_readable(actor,0x78) || (actor&3u) || shadow_witness::lines>=250)return;
    shadow_observed_actor=actor;
    shadow_witness::armed.store(true,std::memory_order_release);
    const uint32_t slot=MEM_HU(0x76,(gpr)(int32_t)actor)>>7;
    const uint32_t owner=slot<8?uint32_t(MEM_W(slot*4,(gpr)(int32_t)0x80135490u)):0;
    const uint32_t body=!(owner&3u)&&shadow_readable(owner,0x54)?uint32_t(MEM_W(0x50,(gpr)(int32_t)owner)):0;
    const int32_t backpack=shadow_readable(body,0x12)?int32_t(MEM_BU(0x11,(gpr)(int32_t)body)):-1;
    std::fprintf(stderr,"baseline shadow-witness event=branch line=%u epoch=%u actor=%08x flags64=%08x detailed=%u view=%u main_matrix=%08x main_id=%08x main_policy=%u saved_epoch=%u saved_valid=%u slot=%u owner=%08x body=%08x backpack_byte=%d source_pc=800A6EC0\n",
        ++shadow_witness::lines,shadow_word(rdram,1032),actor,uint32_t(ctx->r24),unsigned(int32_t(ctx->r8)>=0),uint32_t(MEM_W(0,(gpr)(int32_t)0x80127674u)),shadow_word(rdram,1088),shadow_word(rdram,1112),shadow_word(rdram,1128),shadow_word(rdram,1120),shadow_word(rdram,1124),slot,owner,body,backpack);
    std::fflush(stderr);
}
extern "C" void baseline_shadow_rectangle(uint8_t* rdram,recomp_context* ctx) {
    if(!shadow_observed_actor || shadow_witness::lines>=256 || (uint32_t(ctx->r29)&3u) || !shadow_readable(uint32_t(ctx->r29),0x18))return;
    std::fprintf(stderr,"baseline shadow-witness event=rectangle line=%u epoch=%u actor=%08x light_kind=%u source=%08x source_physical=%08x x=%d y=%d width=%d height=%d alpha_arg=%08x source_pc=800A69D4\n",++shadow_witness::lines,shadow_word(rdram,1032),shadow_observed_actor,shadow_light_kind,uint32_t(ctx->r5),uint32_t(ctx->r5)&0x1FFFFFFFu,int32_t(ctx->r6),int32_t(ctx->r7),int32_t(MEM_W(0x10,ctx->r29)),int32_t(MEM_W(0x14,ctx->r29)),uint32_t(ctx->r13));
    std::fflush(stderr);
}
extern "C" void baseline_shadow_end(uint8_t*,recomp_context*) { shadow_observed_actor=0;shadow_light_kind=0; }
extern "C" void baseline_shadow_local(uint8_t*,recomp_context*) { shadow_light_kind=1; }
extern "C" void baseline_shadow_fallback(uint8_t*,recomp_context*) { shadow_light_kind=2; }
// Diagnostic original BFD slot seam; before BFE70 JAL and its BFE74 stack store.
extern "C" void baseline_pre_slot(uint8_t* rdram,recomp_context* ctx) {
    static uint32_t rows=0;const uint32_t slot=uint32_t(ctx->r18),model=uint32_t(ctx->r11),position=uint32_t(ctx->r4),rotation=uint32_t(ctx->r5);
    if(rows>=4 || !shadow_witness::armed.load(std::memory_order_acquire))return;
    if(shadow_witness::forward_mode() && !shadow_witness::forward_ready.load(std::memory_order_acquire))return;
    if(slot>=2 || shadow_word(rdram,1116)!=2 || !shadow_word(rdram,1124) || shadow_word(rdram,1120)!=shadow_word(rdram,1032))return;
    if((model&1u) || !shadow_readable(model,12) || (position&3u) || !shadow_readable(position,12) || (rotation&3u) || !shadow_readable(rotation,12))return;
    std::fprintf(stderr,"baseline pre-slot row=%u epoch=%u slot=%u model=%08x headerA=%04x view=%u matrix=%08x camera=%08x birth=%u revision=%u id=%08x policy=%u position_words=%08x,%08x,%08x rotation_words=%08x,%08x,%08x scale_word=%08x source_pc=800BFE70 before_delay_store=1\n",
        ++rows,shadow_word(rdram,1032),slot,model,unsigned(MEM_HU(0xA,(gpr)(int32_t)model)),shadow_word(rdram,1100),shadow_word(rdram,1088),shadow_word(rdram,1092),shadow_word(rdram,1096),shadow_word(rdram,1104),shadow_word(rdram,1112),shadow_word(rdram,1128),
        uint32_t(MEM_W(0,(gpr)(int32_t)position)),uint32_t(MEM_W(4,(gpr)(int32_t)position)),uint32_t(MEM_W(8,(gpr)(int32_t)position)),uint32_t(MEM_W(0,(gpr)(int32_t)rotation)),uint32_t(MEM_W(4,(gpr)(int32_t)rotation)),uint32_t(MEM_W(8,(gpr)(int32_t)rotation)),uint32_t(ctx->r6));
    std::fflush(stderr);
}
extern "C" void baseline_shadow_projection(uint8_t* rdram,recomp_context* ctx) {
    feather_stable_projection(rdram,ctx);
    if(!shadow_observed_actor || shadow_witness::lines>=256 || (uint32_t(ctx->r29)&3u) || !shadow_readable(uint32_t(ctx->r29),0x110))return;
    const uint32_t command=uint32_t(ctx->r15),model=uint32_t(MEM_W(0x10C,ctx->r29));
    if(!shadow_readable(command,8) || (command&3u) || !shadow_readable(model,12) || (model&1u))return;
    const uint32_t word0=MEM_W(0,(gpr)(int32_t)command),raw=MEM_W(4,(gpr)(int32_t)command);
    std::fprintf(stderr,"baseline shadow-witness event=projection line=%u epoch=%u actor=%08x light_kind=%u model=%08x headerA=%04x command=%08x word0=%08x raw=%08x physical=%08x main_matrix=%08x main_id=%08x source_pc=800DEFE0\n",
        ++shadow_witness::lines,shadow_word(rdram,1032),shadow_observed_actor,shadow_light_kind,model,unsigned(MEM_HU(0xA,(gpr)(int32_t)model)),command,word0,raw,raw&0x1FFFFFFFu,shadow_word(rdram,1088),shadow_word(rdram,1112));
    std::fflush(stderr);
    shadow_matrix(rdram,"R",raw|0x80000000u);
    shadow_matrix(rdram,"savedC",shadow_word(rdram,1088));
}
#endif

#if TOOIE_PRE_SELECTED
extern "C" void tooie_pre_background_begin(uint8_t*,recomp_context*);
extern "C" void tooie_pre_background_end(uint8_t*,recomp_context*);
extern "C" void tooie_pre_background_reset(uint8_t*,recomp_context*);
#if TOOIE_BASELINE_CAMERA_RUNTIME_OBSERVER
namespace { void pre_scope_receipt(uint8_t* rdram,const char* phase) {
 static unsigned rows=0;if(rows>=8 || !shadow_witness::armed.load(std::memory_order_acquire))return;
 const gpr p=(gpr)(int32_t)TOOIE_PRE_STATE;const uint32_t cursor=uint32_t(MEM_W(0,(gpr)(int32_t)0x80127658u)),command=std::string_view(phase)=="begin"?uint32_t(MEM_W(100,p)):cursor-8;uint32_t a=0,b=0;
 if(command>=0x80000000u && command<=0x80FFFFF8u && !(command&7u)){a=MEM_W(0,(gpr)(int32_t)command);b=MEM_W(4,(gpr)(int32_t)command);}
 std::fprintf(stderr,"baseline pre-scope row=%u phase=%s guest_epoch=%u saved_id=%08x remaining_mask=%u depth=%u latch=%u command=%08x words=%08x,%08x generic_refusals=%u\n",++rows,phase,uint32_t(MEM_W(104,p)),shadow_word(rdram,1112),uint32_t(MEM_W(116,p)),uint32_t(MEM_W(112,p)),shadow_word(rdram,1060),command,a,b,shadow_word(rdram,1288));std::fflush(stderr);
} }
#endif
extern "C" void baseline_pre_background_begin(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);clone.r4=ctx->r18;clone.r5=ctx->r11;tooie_pre_background_begin(rdram,&clone);
#if TOOIE_BASELINE_CAMERA_RUNTIME_OBSERVER
    pre_scope_receipt(rdram,"begin");
#endif
}
extern "C" void baseline_pre_background_end(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);tooie_pre_background_end(rdram,&clone);
#if TOOIE_BASELINE_CAMERA_RUNTIME_OBSERVER
    pre_scope_receipt(rdram,"end");
#endif
}
extern "C" void baseline_pre_background_reset(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);tooie_pre_background_reset(rdram,&clone);
}
#endif
