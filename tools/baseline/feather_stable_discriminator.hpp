// Notes-only Opus13-corrected candidate. Existing observer block, no guest writes.
#include <cstddef>
#include "guest/silo-candidate/render_bridge.h"
namespace {
struct FeatherLock {uint32_t actor=0,descriptor=0,birth=0,slot=0;unsigned models=0,projections=0;};
struct FeatherStable {uint32_t actor=0,descriptor=0,birth=0,slot=0,assetIndex=0,sp=0,model=0;int32_t index=0;bool active=false,authenticated=false;FeatherLock owners[2];} feather_stable;
static_assert(sizeof(TooieSiloContext)==64 && offsetof(TooieRenderBridgeState,silo_context)==18588);
uint32_t feather_bridge(uint8_t* rdram,size_t off){return uint32_t(MEM_W(off,(gpr)(int32_t)TOOIE_BRIDGE_STATE_VRAM));}
uint32_t feather_sc(uint8_t* rdram,size_t off){return feather_bridge(rdram,offsetof(TooieRenderBridgeState,silo_context)+off);}
bool feather_ready(){return !shadow_witness::forward_mode() || shadow_witness::forward_ready.load(std::memory_order_acquire);}
bool feather_live(uint8_t* rdram,recomp_context* ctx){auto& f=feather_stable;
 return f.active && feather_ready() && shadow_readable(uint32_t(ctx->r29),0x110) && !(uint32_t(ctx->r29)&3u) &&
 feather_bridge(rdram,offsetof(TooieRenderBridgeState,silo_depth))==1 && uint32_t(MEM_W(0,(gpr)(int32_t)f.actor))==f.descriptor &&
 MEM_HU(0x14,(gpr)(int32_t)f.descriptor)==(f.assetIndex?0x6EFu:0x6EAu) &&
 feather_sc(rdram,offsetof(TooieSiloContext,actor))==f.actor && feather_sc(rdram,offsetof(TooieSiloContext,descriptor))==f.descriptor &&
 feather_sc(rdram,offsetof(TooieSiloContext,birth))==f.birth && feather_sc(rdram,offsetof(TooieSiloContext,slot))==f.slot &&
 feather_bridge(rdram,offsetof(TooieRenderBridgeState,births)+4*f.slot)==f.birth &&
 feather_sc(rdram,offsetof(TooieSiloContext,epoch))==shadow_word(rdram,1032) && feather_sc(rdram,offsetof(TooieSiloContext,buffers))==uint32_t(MEM_W(0xF8,ctx->r29));
}
}
extern "C" void baseline_feather_stable_begin(uint8_t* rdram,recomp_context* ctx){auto& f=feather_stable;f.active=false;f.authenticated=false;
 if(!feather_ready())return;
 const uint32_t a=uint32_t(ctx->r16);if((a&3u) || !shadow_readable(a,0x7C) || int32_t(MEM_W(0x30,(gpr)(int32_t)a))==1)return;
 const uint32_t d=MEM_W(0,(gpr)(int32_t)a);if(d<0x80132ED0u || d>=0x80132ED0u+200u*0x30u || (d-0x80132ED0u)%0x30u)return;
 const uint32_t asset=MEM_HU(0x14,(gpr)(int32_t)d),slot=(d-0x80132ED0u)/0x30u;if(asset!=0x6EAu && asset!=0x6EFu)return;
 const uint32_t birth=feather_bridge(rdram,offsetof(TooieRenderBridgeState,births)+4*slot);if(!birth)return;
 f.assetIndex=asset==0x6EFu;const auto& owner=f.owners[f.assetIndex];if(owner.models==8)return;
 f.actor=a;f.descriptor=d;f.birth=birth;f.slot=slot;f.index=int32_t(MEM_W(0x1C,(gpr)(int32_t)a));f.active=true;
}
extern "C" void baseline_feather_stable_end(uint8_t*,recomp_context*){feather_stable.active=false;feather_stable.authenticated=false;}
extern "C" void baseline_feather_stable_model(uint8_t* rdram,recomp_context* ctx){auto& f=feather_stable;if(f.authenticated || !feather_live(rdram,ctx))return;
 const uint32_t m=uint32_t(ctx->r25);if((m&1u) || !shadow_readable(m,12))return;
 auto& o=f.owners[f.assetIndex];if(!o.actor){o.actor=f.actor;o.descriptor=f.descriptor;o.birth=f.birth;o.slot=f.slot;}
 if(o.actor!=f.actor || o.descriptor!=f.descriptor || o.birth!=f.birth || o.slot!=f.slot)return;
 f.authenticated=true;f.sp=uint32_t(ctx->r29);f.model=m;if(o.models==8)return;
 const uint32_t pose = uint32_t(MEM_W(0, (gpr)(int32_t)0x8012C968u));
 const bool pose_header_readable = !(pose & 3u) && shadow_readable(pose, 8u);
 const uint32_t pose_count = pose_header_readable ? uint32_t(MEM_W(0, (gpr)(int32_t)pose)) : 0;
 const uint32_t pose_capacity = pose_header_readable ? uint32_t(MEM_W(4, (gpr)(int32_t)pose)) : 0;
 std::fprintf(stderr,"baseline feather-stable event=model row=%u epoch=%u actor=%08x descriptor=%08x slot=%u birth=%u role=80800EBC index=%d pair=%08x view=%u phase=%u buffers=%08x de498_sp=%08x model=%08x headerA=%04x asset=%04x selector24=%04x pose=%08x pose_header_readable=%u pose_count=%u pose_capacity=%u depth=1 source_pc=800DEFA0\n",
 ++o.models,shadow_word(rdram,1032),f.actor,f.descriptor,f.slot,f.birth,f.index,feather_sc(rdram,offsetof(TooieSiloContext,pair)),feather_sc(rdram,offsetof(TooieSiloContext,view)),feather_sc(rdram,offsetof(TooieSiloContext,phase)),feather_sc(rdram,offsetof(TooieSiloContext,buffers)),f.sp,m,unsigned(MEM_HU(0xA,(gpr)(int32_t)m)),unsigned(MEM_HU(0x14,(gpr)(int32_t)f.descriptor)),unsigned(MEM_HU(0x24,(gpr)(int32_t)f.descriptor)),pose,unsigned(pose_header_readable),pose_count,pose_capacity);std::fflush(stderr);
}
// First call in existing baseline_shadow_projection, before its shadow-only gate.
void feather_stable_projection(uint8_t* rdram,recomp_context* ctx){auto& f=feather_stable;if(!f.authenticated || f.owners[f.assetIndex].projections==8 || uint32_t(ctx->r29)!=f.sp || !feather_live(rdram,ctx))return;
 const uint32_t c=uint32_t(ctx->r15),m=MEM_W(0x10C,ctx->r29);if(m!=f.model || (c&7u) || !shadow_readable(c,8) || (m&1u) || !shadow_readable(m,12))return;
 const uint32_t op=MEM_W(0,(gpr)(int32_t)c),raw=MEM_W(4,(gpr)(int32_t)c),r=raw|0x80000000u;if(op!=0xDA380005u || (r&7u) || !shadow_readable(r,64))return;
 std::fprintf(stderr,"baseline feather-stable event=projection row=%u epoch=%u actor=%08x birth=%u role=80800EBC index=%d de498_sp=%08x model=%08x headerA=%04x command=%08x word0=%08x raw=%08x physical=%08x main_id=%08x fixed_words=",
 ++f.owners[f.assetIndex].projections,shadow_word(rdram,1032),f.actor,f.birth,f.index,f.sp,m,unsigned(MEM_HU(0xA,(gpr)(int32_t)m)),c,op,raw,raw&0x1FFFFFFFu,shadow_word(rdram,1112));
 for(unsigned i=0;i<16;++i)std::fprintf(stderr,"%s%08x",i?",":"",uint32_t(MEM_W(i*4,(gpr)(int32_t)r)));std::fprintf(stderr," depth=1 source_pc=800DEFE0\n");std::fflush(stderr);
}
