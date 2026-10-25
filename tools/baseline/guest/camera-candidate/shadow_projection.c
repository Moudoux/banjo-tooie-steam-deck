#define F3DEX_GBI_2
#if !defined(TOOIE_BK_PIPELINE) || (TOOIE_BK_PIPELINE != 0 && TOOIE_BK_PIPELINE != 1)
#error Explicit boolean TOOIE_BK_PIPELINE selection required
#endif
#if TOOIE_BK_PIPELINE
/* BK selection. The whole player-shadow producer (A6DAC: blob A5DB0; projected A6B24 fallback and
 * local lights; A6250 body and attachment) is owned by the generic BK shadow scope
 * (silo-candidate/bk_shadow_attach.inc): owner sample IDs, the shared float/graph path, the bit40
 * root frame and discontinuities. This file keeps only what BK has no counterpart for, the
 * projected-texture pass (execution-facts53 4):
 *  - the projection policy at DEFB0, before the original bit40 projection MUL/NOPUSH;
 *  - the compositor correction: a full 304x228 copy when this A6250 activation's body was tagged,
 *    and the texture-LUT NONE mode scope around the strip compositor (A5F84/A6180), checked
 *    closed at the A69DC completion join.
 * No resource, transformation, topology, selector, camera-production, once-per-construction,
 * fallback-only, attachment-refusal or byte-budget admission remains. E4640, the saved-camera
 * projection restore, is untouched. Original skip exits are untouched. */
#include "camera_projection_internal_04.h"
#include "../silo-candidate/silo_bridge.h"
#include "camera_actor_admission.h"
#include "../silo-candidate/bk_pipeline.h"
#include "rt64_extended_gbi.h"
_Static_assert(G_SETOTHERMODE_H==0xE3 && G_MDSFT_TEXTLUT==14,"F3DEX2 TEXTLUT");
/* @tooie Surface projection policy. The bit40 root multiplies the projection by a flattening
 * surface matrix, generally singular in one axis: decomposing it into translation/rotation/scale
 * is ill-defined, coefficient (SIMPLE) interpolation of the matrix between two nearby surface
 * poses is not. BK has no projected-texture root to copy. This is the accepted shadow word: the
 * former DECOMPOSE WORLD word (0x0201154E) with its decompose bit cleared, kept as that literal now
 * that WORLD is BK's SIMPLE gameplay group. Ordinary palette actors keep WORLD. */
#define SHADOW_POLICY 0x0201154Au
_Static_assert(SHADOW_POLICY==(0x0201154Eu^0x4u),"accepted shadow word: former WORLD without decompose");
#define SHADOW_HELPER __attribute__((section(".tooie_shadow_helpers"),noinline))
#include "shadow_state.h"
/* Unchanged 56-byte layout; the BK selection uses only epoch, composite, mode_open, fault (a
 * diagnostic: a mode push found open) and args. */
__attribute__((section(".tooie_shadow_state"),aligned(4),used))
struct ShadowState shadow_pending={.magic=0x53484403u};
/* Called once after accepted camera construction_begin establishes this epoch. */
SHADOW_HELPER void tooie_shadow_reset(u32 owner_frame){
 if(!owner_frame || CAM_STATE.owner_frame!=owner_frame || !CAM_STATE.control.active || CAM_STATE.control.latch)return;
 if(shadow_pending.mode_open)shadow_pending.fault=1;
 shadow_pending.epoch=CAM_STATE.control.epoch;shadow_pending.composite=0;shadow_pending.mode_open=0;
}
/* The BK shadow scope's own top-level draw (not a DE448 nested inside it). */
SI u32 shadow_owned(void){
 return tooie_bk_shadow.projection_open && tooie_bk_state.owner.kind==TOOIE_BK_OWNER_SHADOW &&
  tooie_bk_state.draw_depth==1u;
}
/* @tooie The authenticated menu scope's own bit40 draw: fxdialog3 80800174 -> DF944 -> DE498 (called
 * directly at DFAA0, so no DE448 depth) under tooie_bk_menu_model_begin, before its DF0B0 root. Only a
 * menu model-component root (menu_scope.hpp); transition masks (kind 4 from TOOIE_BK_MENU_ID_END) and the
 * reserved sprite component are excluded. Returns its projection ID, or 0. */
SI u32 shadow_menu_projection(void){
 u32 id=tooie_bk_state.owner.id;
 if(tooie_bk_state.owner.kind!=TOOIE_BK_OWNER_MENU || tooie_bk_state.draw_depth || tooie_bk_state.root_depth ||
    tooie_bk_state.cur_drawn_model_transform_id!=id || id<TOOIE_BK_MENU_ID_START || id>=TOOIE_BK_MENU_ID_END ||
    (id&(2u*TOOIE_BK_TRANSFORM_ID_COUNT-1u)))return 0;
 return id+TOOIE_BK_MENU_PROJECTION_OFFSET;
}
_Static_assert(TOOIE_BK_MENU_ID_START+TOOIE_BK_MENU_PROJECTION_OFFSET>=TOOIE_BK_MENU_ID_END+5u*TOOIE_BK_TRANSFORM_ID_COUNT &&
    TOOIE_BK_MENU_ID_START+TOOIE_BK_MENU_PROJECTION_OFFSET>=TOOIE_CAMERA_TRANSITION_ID_END &&
    TOOIE_BK_MENU_ID_END+TOOIE_BK_MENU_PROJECTION_OFFSET<=0x40000000u,
    "menu projection IDs follow the transition model/projection IDs, below the product labels");
/* DEFB0, before the original projection MUL/NOPUSH and after DE498's DE738 pre-draw callback. A
 * shadow-owned draw gets its owner sample's projection ID with the surface policy, or, when the draw
 * skips (a cut, a discontinuity or the projected body's own visible-branch decision, tooie_bk_shadow_project,
 * made here from the effective model), the same ID with every component SKIP: CAM_POLICY_IGNORE's word is
 * BK's gEXMatrixGroupSkipAll for a NOPUSH projection, and the kept ID lets the next workload map it.
 * The menu scope's own bit40 draw likewise gets its projection ID (shadow_menu_projection) with BK's
 * gEXMatrixGroupSimpleNormal word (CAM_POLICY_WORLD), or SkipAll under the menu cut, as BK scopes its
 * portrait ortho ID. Every other bit40 draw keeps the existing actor-palette or IGNORE decision. */
SHADOW_HELPER void tooie_shadow_de498(u32 graphics_buffers,u32 cursor_slot,u32 effective_model){
 struct TooieCameraControl* c=&CAM_STATE.control;u32 cursor,enabled=0,id=0,shadow,menu=0,skip=0;
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
 shadow=shadow_owned();
 if(shadow){id=tooie_bk_shadow_project(effective_model);skip=tooie_bk_state.cur_drawn_model_skip_interpolation;}
 else if((id=shadow_menu_projection())!=0u){menu=1;skip=tooie_bk_state.cur_drawn_model_skip_interpolation;}
#if TOOIE_ACTOR_PALETTE
 else {
     u32 camera_id = cam_actor_camera_id(cursor);
     if (camera_id)
         id = tooie_actor_palette_begin((struct TooieRenderBuffers*)graphics_buffers,
             (u32*)cursor_slot, effective_model, camera_id);
 }
 /* Actor admission may emit its paid modelview group before the projection. */
 cursor = *(volatile u32*)cursor_slot;
#endif
 *(volatile u32*)cursor_slot=cam_projection_policy(cursor,id,
  shadow?(id&&!skip?SHADOW_POLICY:CAM_POLICY_IGNORE):
  menu?(skip?CAM_POLICY_IGNORE:CAM_POLICY_WORLD):(id?CAM_POLICY_WORLD:CAM_POLICY_IGNORE));
}
/* A69D4, before the compositor argument transfer; frame = this A6250 activation's sp. A tagged body
 * can interpolate outside the current CPU-clipped rectangle, which neither tags nor floats can
 * enlarge, so its composite copies the whole 304x228 target (the accepted correction). */
SHADOW_HELPER u32* tooie_shadow_rectangle_args(u32 frame,u32 x,u32 y,u32 width,u32 height){
 u32* a=shadow_pending.args;
 a[0]=x;a[1]=y;a[2]=width;a[3]=height;shadow_pending.composite=0;
 if(frame && tooie_bk_shadow.body_frame==frame){a[0]=0;a[1]=0;a[2]=304;a[3]=228;shadow_pending.composite=1;}
 tooie_bk_shadow.body_frame=0;
 return a;
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
/* A69DC completion join. A mode push still open here would leave the RDP other-mode stack
 * unbalanced: an emitted-command invariant, not an admission. */
SHADOW_HELPER void tooie_shadow_complete(u32 cursor){
 (void)cursor;
 tooie_bk_shadow.body_frame=0;
 if(shadow_pending.mode_open){
  shadow_pending.fault=1;
#if TOOIE_BRIDGE_REQUIRED_STOP
  tooie_bridge_required_stop(3u);
#endif
  return;
 }
 shadow_pending.composite=0;
}
#else
/* Original fallback shadow: guest-owned public projection and local mode scope. */
#include "camera_projection_internal_04.h"
#include "../silo-candidate/silo_bridge.h"
#include "camera_actor_admission.h"
#define PC SS.player
/* 920compositor+16projection+24local mode scope; coefficient route adds its 16+8 modelview push/pop. */
#define SHADOW_COST (960u+24u*TOOIE_SHADOW_COEFFICIENTS)
#include "rt64_extended_gbi.h"
_Static_assert(G_SETOTHERMODE_H==0xE3 && G_MDSFT_TEXTLUT==14,"F3DEX2 TEXTLUT");
#define SHADOW_ROLE_FIRST 480u
/* TOOIE_SHADOW_COEFFICIENTS=1 selects the player-accepted shadow route; 0 keeps prior compositions byte-identical. */
#if TOOIE_SHADOW_COEFFICIENTS
/* Shadow route only: the former DECOMPOSE WORLD word with its decompose mode bit (bit 2) cleared. */
#define SHADOW_POLICY 0x0201154Au
_Static_assert(SHADOW_POLICY==(0x0201154Eu^0x4u),"accepted shadow word: former WORLD without decompose");
/* Whole-silhouette MODELVIEW group: the actor-palette components, or all SKIP for a frame
 * without same-ID/camera/selector history (BK stable ID plus skip). */
#define SHADOW_MV_LIVE CAM_GROUP_FLAGS(G_EX_INTERPOLATE_SIMPLE,G_EX_PUSH,G_MTX_MODELVIEW, \
    G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE, \
    G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO,G_EX_ORDER_LINEAR, \
    G_EX_EDIT_NONE,G_EX_ASPECT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO)
#define SHADOW_MV_SKIP CAM_GROUP_FLAGS(G_EX_INTERPOLATE_SIMPLE,G_EX_PUSH,G_MTX_MODELVIEW, \
    G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP, \
    G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_ORDER_LINEAR, \
    G_EX_EDIT_NONE,G_EX_ASPECT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP)
_Static_assert(SHADOW_MV_LIVE==0x02010AA9u && SHADOW_MV_SKIP==0x00000001u,"public modelview group words");
#define SHADOW_MV_OPEN shadow_pending.mv_group
#else
#define SHADOW_MV_OPEN 0u
#endif
#define SHADOW_HELPER __attribute__((section(".tooie_shadow_helpers"),noinline))
#include "shadow_state.h"
#if defined(TOOIE_CKCOPY) && TOOIE_CKCOPY
#include "../cpu625-candidate/cpu625_layout.h"
#include "../cpu625-candidate/ckpack_rows.h"
#endif
__attribute__((section(".tooie_shadow_state"),aligned(4),used))
struct ShadowState shadow_pending={.magic=0x53484403u};
extern volatile u32 D_80127658;
extern volatile s16 D_80132E9C;
extern volatile u32 D_8012C9B0; /* original post-draw function-pointer word */
/* Call once after accepted camera construction_begin establishes this epoch. */
SHADOW_HELPER void tooie_shadow_reset(u32 owner_frame){
 if(!owner_frame || CAM_STATE.owner_frame!=owner_frame || !CAM_STATE.control.active || CAM_STATE.control.latch)return;
 if(shadow_pending.mode_open || SHADOW_MV_OPEN)shadow_pending.fault=1;
#if TOOIE_SHADOW_COEFFICIENTS
 shadow_pending.mv_group=0; /* Modelview history survives; only the open push is per-draw. */
#endif
 shadow_pending.epoch=CAM_STATE.control.epoch;shadow_pending.id=0;shadow_pending.model=0;
 shadow_pending.group=0;shadow_pending.composite=0;shadow_pending.mode_open=0;
 shadow_pending.seen_epoch=0xFFFFFFFFu;shadow_pending.source_pc=0;
}
extern volatile u32 D_80135490[8];
extern volatile u8 D_801354B0[8];
SHADOW_HELPER void tooie_shadow_source(u32 source_pc){shadow_pending.source_pc=source_pc;}
#if defined(TOOIE_CKCOPY) && TOOIE_CKCOPY
/* N3: an attachment-enabled silhouette qualifies only with one of the four ckpack CPU-deform rows
 * (shared FA12 predicate) on a two-copy part; every other optional attachment keeps the whole
 * composite refused, exactly as before. */
SI u32 shadow_attachment_row(u32 owner){
 u32 part=*(volatile u32*)(owner+20u);
 return silo_range(part,24u,3u) && ckpack_row(part)<=3u && *(volatile u8*)(part+8u) &&
  *(volatile u8*)(part+9u)<=1u && *(volatile u8*)(part+10u)==1u;
}
#endif
/* Called before the original body DE448, never after a changed command. */
SHADOW_HELPER u32 tooie_shadow_prepare(u32 owner,u32 actor,u32 model){
 struct TooieCameraControl* c=&CAM_STATE.control;
 struct TooieCameraProduction* p=&CAM_STATE.saved;
 struct TooieCameraRecord* camera=cam_find(p->camera);
 u32 component,cursor,selector,handle,packed,asset,type_component,transformation,base;
 u32 phase=(u32)(s32)D_80132E9C,slot,original_source_pc=shadow_pending.source_pc;
 volatile struct TooiePlayerEntry* e;
#if defined(TOOIE_CKCOPY) && TOOIE_CKCOPY
 u32 attach;
#endif
 shadow_pending.id=0;shadow_pending.group=0;
 if(!silo_range(actor,0x78u,1u))return 0;
 slot=*(volatile u16*)(actor+0x76u)>>7;
 silo_epoch();
 if(SS.active || SC.actor || shadow_pending.mode_open || SHADOW_MV_OPEN || shadow_pending.fault)return 0;
 if(original_source_pc!=0x800A6D60u || slot>=8u || D_801354B0[slot])return 0;
 if(shadow_pending.seen_epoch==SS.epoch)return 0;
 /* @recomp PC[8]: the authenticated owner's retained primary entry for the current view. The single
  * shadow seen/history and lexical scratch are unchanged: still one shadow per construction. */
 if(!PC.enabled || SS.poisoned || PC.tripwires || (u32)D_80127674>3u)return 0;
 if(D_80135490[slot]!=owner || !silo_range(owner,0x188u,3u))return 0;
 e=&SS.primary[slot][(u32)D_80127674];
 if(!e->idbase || e->stamp==0xFFFFFFFFu || e->owner!=owner)return 0;
 type_component=*(volatile u32*)(owner+0xA0u);
 if(!silo_range(type_component,0x2Au,0u))return 0;
 transformation=*(volatile u8*)(type_component+0x29u);
 component=*(volatile u32*)(owner+0x50u);
 if(component!=e->component || !silo_range(component,0x118u,3u))return 0;
#if defined(TOOIE_CKCOPY) && TOOIE_CKCOPY
 attach=*(volatile u8*)(component+0x11u);
 if(attach && !shadow_attachment_row(owner))return 0; /* original optional backpack: four rows only */
#else
 if(*(volatile u8*)(component+0x11u))return 0; /* original optional backpack */
#endif
 selector=(u32)(s32)*(volatile s16*)(component+0xCu);
 handle=(u32)(u16)*(volatile s16*)component;
 packed=(selector<<16)|handle;
 asset=selector==0x607u?*(volatile u32*)(component+0x10Cu):selector;
 if(packed!=e->selector_handle || asset!=e->effective_model_asset)return 0;
 /* @recomp Original A6250 selects silhouette853 for Banjo and854 for Mumbo.
  * Both complete graphs use only the guarded type3/type12 handlers below.
  * Keep transformation/body pairs exact; other forms and light routes refuse. */
 if(!((transformation==1u && (asset==0x62Au || asset==0x62Bu)) ||
      (transformation==13u && asset==0x665u)))return 0;
 if(!silo_range(model,0x2Cu,7u) || !(*(volatile u16*)(model+0xAu)&0x40u))return 0;
 /* Effective silhouette853/854 authentication belongs to the original selected
  * model argument/callsite contract, not a guessed pointer or new cache call. */
 if(!CAM_STATE.owner_frame || !c->active || c->latch || D_8012D508)return 0;
 if(c->epoch!=SS.epoch || !(e->stamp==SS.epoch || e->stamp+1u==SS.epoch))return 0;
 if(!p->valid || p->epoch!=c->epoch || p->role!=TOOIE_CAMERA_WORLD_INITIAL || p->policy!=CAM_POLICY_WORLD)return 0;
 if(p->view>=4u || p->view!=(u32)D_80127674 || e->view_phase!=(p->view|(phase<<2)))return 0;
 if(!camera || camera->birth!=p->birth || camera->revision!=p->revision || camera->view_id[p->view]!=p->id)return 0;
 if(D_8007BEDA<0 || D_8007BEDA>1 || D_8007BCE8[0] || D_8007BCE8[1] || D_8007BCF0[0] || D_8007BCF0[1])return 0;
 base=D_8007BCD0[D_8007BEDA];cursor=D_80127658;
 if(!silo_range(base,TOOIE_BRIDGE_LOWER_BYTES,7u) || cursor<base || cursor<SS.last_end || cursor>base+TOOIE_BRIDGE_LOWER_BYTES-SHADOW_COST)return 0;
 if((cursor&7u) || cursor<c->main_begin || cursor>c->main_end-SHADOW_COST)return 0;
 if(!c->main_extra || c->main_extra>TOOIE_CAMERA_EXTRA_BYTES-32u || SS.extra_bytes>6400u-SHADOW_COST)return 0;
#if defined(TOOIE_CKCOPY) && TOOIE_CKCOPY
 /* N3: the attachment's group 16 + pop 8 are reserved now, before the silhouette is labelled, so
  * a later attachment failure still has room for its explicit placeholder. The CPU625 context
  * must be free (draw3 and the attachment brackets are disjoint). */
 if(attach && (tooie_cpu625_state.context.owner || cursor>c->main_end-SHADOW_COST-24u ||
    c->main_extra>TOOIE_CAMERA_EXTRA_BYTES-32u-24u))return 0;
#endif
 SS.extra_bytes+=SHADOW_COST;
 if(SS.extra_bytes>SS.extra_highwater)SS.extra_highwater=SS.extra_bytes;
#if defined(TOOIE_CKCOPY) && TOOIE_CKCOPY
 if(attach){
  struct TooieCpu625Context* x=&tooie_cpu625_state.context;
  c->main_extra+=24u;x->owner=owner;x->epoch=c->epoch;
  x->depth=TOOIE_CPU625_ATTACH;x->paid=TOOIE_CPU625_ATTACH_RESERVED;
 }
#endif
 shadow_pending.epoch=c->epoch;shadow_pending.model=model;shadow_pending.seen_epoch=c->epoch;
 shadow_pending.id=e->idbase+SHADOW_ROLE_FIRST;
 return shadow_pending.id;
}
/* Existing de498 emitter calls this DECIDER, then cam_projection_policy ONCE.
 * Existing enabled gate precedes this call. */
SI u32 tooie_shadow_projection_id(u32 cursor,u32 effective_model){
 if(!shadow_pending.id || shadow_pending.group || shadow_pending.model!=effective_model ||
    shadow_pending.epoch!=CAM_STATE.control.epoch || CAM_STATE.control.latch)return 0;
 /* Original853/854 have only type3/type12, including selector alternatives.
  * Pre-draw callback already ran; these handlers cannot enable later callbacks. */
 if(D_8012C838==1u || D_8012C9B0)return 0;
 if(!silo_range(D_8012CF8C,13u*4u,3u) ||
    *(volatile u32*)(D_8012CF8C+3u*4u)!=0x800DD024u ||
    *(volatile u32*)(D_8012CF8C+12u*4u)!=0x800DD56Cu)return 0;
 shadow_pending.group=cursor;return shadow_pending.id;
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
#if TOOIE_SHADOW_COEFFICIENTS
extern volatile u32 D_8012C850[60];
/* Census type12 node (model-relative, signed16 count +8/selector +10); key gets its live C850 word. */
SI u32 shadow_selector(u32 model,u32 node,s32 count,s32 selector,u32* key){
 if(*(volatile u32*)(model+node)!=12u || *(volatile s16*)(model+node+8u)!=count ||
    *(volatile s16*)(model+node+10u)!=selector)return 0;
 *key=D_8012C850[selector];return 1;
}
SI u32 shadow_header(u32 model,u32 bytes,u32 geometry,u32 vertices,u32 bones){
 return silo_range(model,bytes,7u) && *(volatile u32*)(model+4u)==geometry &&
  *(volatile u16*)(model+8u)==80u && *(volatile u16*)(model+0xAu)==0x40u &&
  *(volatile u32*)(model+0xCu)==88u && *(volatile u32*)(model+0x10u)==vertices &&
  *(volatile u32*)(model+0x18u)==bones && !*(volatile u32*)(model+0x28u);
}
/* @recomp Immutable silhouette853/854 graphs are type3/type12 only; their transform order
 * is fixed by these exact raw selector words, which A0714 writes before DE448. */
SI u32 shadow_topology(u32 model,u32 key[5]){
 /* @recomp PC[8]: the asset of the entry prepare selected (its idbase is unique), not the last
  * admitted occurrence of another owner or view. */
 u32 asset=0,i,base=shadow_pending.id-SHADOW_ROLE_FIRST;
 for(i=0;i<32u;++i)if(SS.primary[i>>2][i&3u].idbase==base)asset=SS.primary[i>>2][i&3u].effective_model_asset;
 key[4]=0;
 if(asset==0x665u)return shadow_header(model,20684u,20464u,5960u,5296u) &&
  shadow_selector(model,20480u,4,1,&key[0]) && shadow_selector(model,20592u,1,4,&key[1]) &&
  shadow_selector(model,20624u,1,5,&key[2]) && shadow_selector(model,20672u,0,6,&key[3]);
 return (asset==0x62Au || asset==0x62Bu) && shadow_header(model,14316u,14040u,5184u,4440u) &&
  shadow_selector(model,14056u,1,15,&key[0]) && shadow_selector(model,14088u,1,16,&key[1]) &&
  shadow_selector(model,14128u,2,38,&key[2]) && shadow_selector(model,14200u,2,39,&key[3]) &&
  shadow_selector(model,14272u,1,17,&key[4]);
}
/* Stable PC.idbase+481 after the owned projection group. LIVE only when the previous epoch
 * emitted this ID with the same saved camera and selector words; otherwise the same ID SKIPs. */
SI u32 shadow_mv_begin(u32 cursor){
 struct TooieCameraControl* c=&CAM_STATE.control;struct TooieCameraProduction* p=&CAM_STATE.saved;
 u32 key[5],id=shadow_pending.id+1u,live,i;
 if(!tooie_shadow_owns_group(cursor) || c->latch || SS.active || shadow_pending.mv_group ||
    (cursor&7u) || cursor>c->main_end-24u || !p->valid || p->epoch!=c->epoch ||
    !shadow_topology(shadow_pending.model,key))return cursor;
 live=shadow_pending.mv_id==id && shadow_pending.mv_epoch+1u==c->epoch &&
  shadow_pending.mv_camera[0]==p->birth && shadow_pending.mv_camera[1]==p->revision &&
  shadow_pending.mv_camera[2]==p->id;
 for(i=0;i<5u;i++){live=live && shadow_pending.mv_selectors[i]==key[i];shadow_pending.mv_selectors[i]=key[i];}
 shadow_pending.mv_id=id;shadow_pending.mv_epoch=c->epoch;
 shadow_pending.mv_camera[0]=p->birth;shadow_pending.mv_camera[1]=p->revision;shadow_pending.mv_camera[2]=p->id;
 cam_group_words(cursor,id,live?SHADOW_MV_LIVE:SHADOW_MV_SKIP);
 shadow_pending.mv_group=cursor;return cursor+16u;
}
#endif
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
 if(shadow)*(volatile u32*)cursor_slot=shadow_mv_begin(*(volatile u32*)cursor_slot);
#else
 *(volatile u32*)cursor_slot=cam_projection_policy(cursor,id,id?CAM_POLICY_WORLD:CAM_POLICY_IGNORE);
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
#if TOOIE_SHADOW_COEFFICIENTS
   /* The bbox-clipped silhouette snaps too: its owned push follows the group; restart history. */
   {volatile u32* mv=(volatile u32*)(shadow_pending.group+16u);
    if(shadow_pending.mv_id==shadow_pending.id+1u && shadow_pending.mv_epoch==c->epoch &&
       mv[0]==CAM_GROUP_OPCODE && mv[1]==shadow_pending.mv_id && !mv[3]){mv[2]=SHADOW_MV_SKIP;shadow_pending.mv_id=0;}}
#endif
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
 if(shadow_pending.mode_open || SHADOW_MV_OPEN){
  shadow_pending.fault=1;SS.poisoned=1;
#if TOOIE_BRIDGE_REQUIRED_STOP
  tooie_bridge_required_stop(3u);
#endif
  return;
 }
 if(shadow_pending.epoch==SS.epoch && cursor>SS.last_end)SS.last_end=cursor;
 shadow_pending.id=0;shadow_pending.group=0;shadow_pending.composite=0;
}
#if TOOIE_SHADOW_COEFFICIENTS
/* A666C, after the original DE448 call returns and before the optional backpack DE448.
 * Pops only this epoch's owned push at the published cursor; never a fabricated POP. */
SHADOW_HELPER void tooie_shadow_mv_end(void){
 u32 g=shadow_pending.mv_group,cursor=D_80127658;
 if(!g)return;
 if((cursor&7u) || cursor<g+16u || cursor>CAM_STATE.control.main_end-8u ||
    shadow_pending.mv_epoch!=CAM_STATE.control.epoch){
  shadow_pending.fault=1;SS.poisoned=1;
#if TOOIE_BRIDGE_REQUIRED_STOP
  tooie_bridge_required_stop(3u);
#endif
  return;
 }
 gEXPopMatrixGroup((Gfx*)cursor,G_MTX_MODELVIEW);
 D_80127658=cursor+8u;shadow_pending.mv_group=0;
 if(cursor+8u>SS.last_end)SS.last_end=cursor+8u;
}
#endif
#endif
