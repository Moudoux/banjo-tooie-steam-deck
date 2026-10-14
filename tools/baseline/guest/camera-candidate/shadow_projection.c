#define F3DEX_GBI_2
/* Original fallback shadow: guest-owned public projection and local mode scope. */
#include "camera_projection_internal_04.h"
#include "../silo-candidate/silo_bridge.h"
#include "camera_actor_admission.h"
#define PC SS.player
#define SHADOW_COST 960u /* 920compositor+16projection+24local mode scope */
#include "rt64_extended_gbi.h"
_Static_assert(G_SETOTHERMODE_H==0xE3 && G_MDSFT_TEXTLUT==14,"F3DEX2 TEXTLUT");
#define SHADOW_ROLE_FIRST 480u
/* TOOIE_SHADOW_COEFFICIENTS=1 selects the player-accepted shadow route; 0 keeps prior compositions byte-identical. */
#if TOOIE_SHADOW_COEFFICIENTS
/* Shadow route only: the public world policy with its decompose mode bit (bit 2) cleared. */
#define SHADOW_POLICY (CAM_POLICY_WORLD^0x4u)
_Static_assert(SHADOW_POLICY==0x0201154Au && (SHADOW_POLICY^CAM_POLICY_WORLD)==0x4u,"shadow policy differs only in decompose");
#endif
#define SHADOW_HELPER __attribute__((section(".tooie_shadow_helpers"),noinline))
struct ShadowState {u32 magic,epoch,id,model,seen_epoch,group,composite,mode_open,fault,args[4],source_pc;};
__attribute__((section(".tooie_shadow_state"),aligned(4),used))
struct ShadowState shadow_pending={.magic=0x53484403u};
_Static_assert(sizeof(struct ShadowState)==56,"shadow state extent");
extern volatile u32 D_80127658;
extern volatile s16 D_80132E9C;
extern volatile u32 D_8012C9B0; /* original post-draw function-pointer word */
/* Call once after accepted camera construction_begin establishes this epoch. */
SHADOW_HELPER void tooie_shadow_reset(u32 owner_frame){
 if(!owner_frame || CAM_STATE.owner_frame!=owner_frame || !CAM_STATE.control.active || CAM_STATE.control.latch)return;
 if(shadow_pending.mode_open)shadow_pending.fault=1;
 shadow_pending.epoch=CAM_STATE.control.epoch;shadow_pending.id=0;shadow_pending.model=0;
 shadow_pending.group=0;shadow_pending.composite=0;shadow_pending.mode_open=0;
 shadow_pending.seen_epoch=0xFFFFFFFFu;shadow_pending.source_pc=0;
}
extern volatile u32 D_80135490[8];
extern volatile u8 D_801354B0[8];
SHADOW_HELPER void tooie_shadow_source(u32 source_pc){shadow_pending.source_pc=source_pc;}
/* Called before the original body DE448, never after a changed command. */
SHADOW_HELPER u32 tooie_shadow_prepare(u32 owner,u32 actor,u32 model){
 struct TooieCameraControl* c=&CAM_STATE.control;
 struct TooieCameraProduction* p=&CAM_STATE.saved;
 struct TooieCameraRecord* camera=cam_find(p->camera);
 u32 component,cursor,selector,handle,packed,asset,type_component,base;
 u32 phase=(u32)(s32)D_80132E9C,slot,original_source_pc=shadow_pending.source_pc;
 shadow_pending.id=0;shadow_pending.group=0;
 if(!silo_range(actor,0x78u,1u))return 0;
 slot=*(volatile u16*)(actor+0x76u)>>7;
 silo_epoch();
 if(SS.active || SC.actor || shadow_pending.mode_open || shadow_pending.fault)return 0;
 if(original_source_pc!=0x800A6D60u || slot>=8u || D_801354B0[slot])return 0;
 if(shadow_pending.seen_epoch==SS.epoch)return 0;
 if(!PC.enabled || !PC.idbase || PC.stamp==0xFFFFFFFFu || SS.poisoned || PC.tripwires)return 0;
 if(PC.owner!=owner || D_80135490[slot]!=owner || PC.slot!=slot || !silo_range(owner,0x188u,3u))return 0;
 type_component=*(volatile u32*)(owner+0xA0u);
 if(!silo_range(type_component,0x2Au,0u) || *(volatile u8*)(type_component+0x29u)!=1u)return 0;
 component=*(volatile u32*)(owner+0x50u);
 if(component!=PC.component || !silo_range(component,0x118u,3u))return 0;
 if(*(volatile u8*)(component+0x11u))return 0; /* original optional backpack */
 selector=(u32)(s32)*(volatile s16*)(component+0xCu);
 handle=(u32)(u16)*(volatile s16*)component;
 packed=(selector<<16)|handle;
 asset=selector==0x607u?*(volatile u32*)(component+0x10Cu):selector;
 if(packed!=PC.selector_handle || asset!=PC.effective_model_asset)return 0;
 if(asset!=0x62Au && asset!=0x62Bu)return 0;
 if(!silo_range(model,0x2Cu,7u) || !(*(volatile u16*)(model+0xAu)&0x40u))return 0;
 /* Effective silhouette853 authentication belongs to the original selected
  * model argument/callsite contract, not a guessed pointer or new cache call. */
 if(!CAM_STATE.owner_frame || !c->active || c->latch || D_8012D508)return 0;
 if(c->epoch!=SS.epoch || !(PC.stamp==SS.epoch || PC.stamp+1u==SS.epoch))return 0;
 if(!p->valid || p->epoch!=c->epoch || p->role!=TOOIE_CAMERA_WORLD_INITIAL || p->policy!=CAM_POLICY_WORLD)return 0;
 if(p->view>=4u || p->view!=(u32)D_80127674 || PC.view_phase!=(p->view|(phase<<2)))return 0;
 if(!camera || camera->birth!=p->birth || camera->revision!=p->revision || camera->view_id[p->view]!=p->id)return 0;
 if(D_8007BEDA<0 || D_8007BEDA>1 || D_8007BCE8[0] || D_8007BCE8[1] || D_8007BCF0[0] || D_8007BCF0[1])return 0;
 base=D_8007BCD0[D_8007BEDA];cursor=D_80127658;
 if(!silo_range(base,0x8FC0u,7u) || cursor<base || cursor<SS.last_end || cursor>base+0x8FC0u-SHADOW_COST)return 0;
 if((cursor&7u) || cursor<c->main_begin || cursor>c->main_end-SHADOW_COST)return 0;
 if(!c->main_extra || c->main_extra>TOOIE_CAMERA_EXTRA_BYTES-32u || SS.extra_bytes>6400u-SHADOW_COST)return 0;
 SS.extra_bytes+=SHADOW_COST;
 if(SS.extra_bytes>SS.extra_highwater)SS.extra_highwater=SS.extra_bytes;
 shadow_pending.epoch=c->epoch;shadow_pending.model=model;shadow_pending.seen_epoch=c->epoch;
 shadow_pending.id=PC.idbase+SHADOW_ROLE_FIRST;
 return shadow_pending.id;
}
/* Existing de498 emitter calls this DECIDER, then cam_projection_policy ONCE.
 * Existing enabled gate precedes this call. */
SI u32 tooie_shadow_projection_id(u32 cursor,u32 effective_model){
 if(!shadow_pending.id || shadow_pending.group || shadow_pending.model!=effective_model ||
    shadow_pending.epoch!=CAM_STATE.control.epoch || CAM_STATE.control.latch)return 0;
 /* Original853 has only type3/type12, including selector alternatives.
  * Pre-draw callback already ran; these handlers cannot enable later callbacks. */
 if(D_8012C838==1u || D_8012C9B0)return 0;
 if(!silo_range(D_8012CF8C,13u*4u,3u) ||
    *(volatile u32*)(D_8012CF8C+3u*4u)!=0x800DD024u ||
    *(volatile u32*)(D_8012CF8C+12u*4u)!=0x800DD56Cu)return 0;
 shadow_pending.group=cursor;return shadow_pending.id;
}
/* Prepared shadows take precedence; other actors reuse their existing admission. */
SHADOW_HELPER void tooie_shadow_de498(u32 graphics_buffers,u32 cursor_slot,u32 effective_model){
#if TOOIE_SHADOW_COEFFICIENTS
 struct TooieCameraControl* c=&CAM_STATE.control;u32 cursor,enabled=0,id,shadow;
#else
 struct TooieCameraControl* c=&CAM_STATE.control;u32 cursor,enabled=0,id;
#endif
#if !TOOIE_ACTOR_PALETTE
 (void)graphics_buffers;
#endif
 if(!CAM_STATE.owner_frame || !c->active)return;
 cursor=*(volatile u32*)cursor_slot;
 if(D_8012D508){u32 i;for(i=0;i<2;i++)if(CAM_STATE.fill[i].valid && CAM_STATE.fill[i].epoch==c->epoch &&
  cursor>=CAM_STATE.fill[i].base && cursor<=CAM_STATE.fill[i].end && CAM_STATE.fill[i].extra>=8u){enabled=1;break;}}
 else if(cursor>=c->main_base && cursor<=c->main_end && c->main_extra>=8u)enabled=1;
 if(!enabled){c->latch=1;CAM_STATE.diagnostics.refusals++;
  if(!CAM_STATE.diagnostics.first_reason)CAM_STATE.diagnostics.first_reason=0x102u;return;}
 id=tooie_shadow_projection_id(cursor,effective_model);
#if TOOIE_SHADOW_COEFFICIENTS
 shadow=id!=0u; /* Authenticated shadow route, recorded before any actor-palette fallback. */
#endif
#if TOOIE_ACTOR_PALETTE
 if (!id) {
     u32 camera_id = cam_actor_camera_id(cursor);
     if (camera_id)
         id = tooie_actor_palette_begin((struct TooieRenderBuffers*)graphics_buffers,
             (u32*)cursor_slot, effective_model, camera_id);
 }
 /* Actor admission may emit its paid modelview group before the projection. */
 cursor = *(volatile u32*)cursor_slot;
#endif
#if TOOIE_SHADOW_COEFFICIENTS
 *(volatile u32*)cursor_slot=cam_projection_policy(cursor,id,shadow?SHADOW_POLICY:(id?CAM_POLICY_WORLD:CAM_POLICY_IGNORE));
#else
 *(volatile u32*)cursor_slot=cam_projection_policy(cursor,id,id?CAM_POLICY_WORLD:CAM_POLICY_IGNORE);
#endif
}
/* Own exact16-byte group only; no search, allocator, renderer-state rollback. */
SI u32 tooie_shadow_owns_group(u32 cursor){
 struct TooieCameraControl* c=&CAM_STATE.control;u32 g=shadow_pending.group;
 volatile u32* words=(volatile u32*)g;
 return g && !(g&7u) && g>=c->main_base && g<=c->main_end-16u && g<=cursor-16u &&
#if TOOIE_SHADOW_COEFFICIENTS
  words[0]==CAM_GROUP_OPCODE && words[1]==shadow_pending.id && words[2]==SHADOW_POLICY && words[3]==0;
#else
  words[0]==CAM_GROUP_OPCODE && words[1]==shadow_pending.id && words[2]==CAM_POLICY_WORLD && words[3]==0;
#endif
}
/* Guest owns bbox decision and values. Native thunk only marshals args. */
SHADOW_HELPER u32* tooie_shadow_rectangle_args(u32 cursor,u32 x,u32 y,u32 width,u32 height){
 struct TooieCameraControl* c=&CAM_STATE.control;u32* a=shadow_pending.args;
 a[0]=x;a[1]=y;a[2]=width;a[3]=height;shadow_pending.composite=0;
 if(shadow_pending.id && shadow_pending.epoch==c->epoch && tooie_shadow_owns_group(cursor)){
  if(!c->latch && !(cursor&7u) && cursor>=SS.last_end && cursor>=c->main_begin && cursor<=c->main_end-944u){
   a[0]=0;a[1]=0;a[2]=304;a[3]=228;shadow_pending.composite=1;
  }else{
   /* Guarded original3/12 handlers cannot submit; outerA78E8 is later. */
   cam_group_words(shadow_pending.group,0,CAM_POLICY_IGNORE);
  }
 }
 shadow_pending.id=0;shadow_pending.group=0;return a;
}
SHADOW_HELPER u32 tooie_shadow_mode_begin(u32 cursor){
 if(!shadow_pending.composite)return cursor;
 gEXPushOtherMode((Gfx*)cursor);gDPSetTextureLUT((Gfx*)(cursor+8u),G_TT_NONE);
 shadow_pending.mode_open=1;return cursor+16u;
}
SHADOW_HELPER u32 tooie_shadow_mode_end(u32 cursor){
 if(!shadow_pending.mode_open)return cursor; /* Never recheck capacity or policy here. */
 gEXPopOtherMode((Gfx*)cursor);shadow_pending.mode_open=0;return cursor+8u;
}
SHADOW_HELPER void tooie_shadow_complete(u32 cursor){
 if(shadow_pending.mode_open){shadow_pending.fault=1;SS.poisoned=1;return;}
 if(shadow_pending.epoch==SS.epoch && cursor>SS.last_end)SS.last_end=cursor;
 shadow_pending.id=0;shadow_pending.group=0;shadow_pending.composite=0;
}

