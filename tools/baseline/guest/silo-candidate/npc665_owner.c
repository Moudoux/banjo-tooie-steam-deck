/* Source-qualified original NPC665; original immutable asset parsing premise.
 * No per-draw graph scan, resource getter, native policy or private history. */
#define F3DEX_GBI_2
#include "silo_bridge.h"
#include "npc665_owner.h"
#include "../camera-candidate/camera_actor_admission.h"
#include "rt64_extended_gbi.h"
#if TOOIE_NPC665
#define NPC __attribute__((noinline,section(".tooie_npc665_helpers")))
#define NC tooie_npc665_state
#define AC SS.palette_context
#define INVALID_ROLE 0xFFFFFFFFu
#define MAIN_BYTES 0xA8C0u
#define REFUSE(reason) do { SS.silo_last_reason=(reason); silo_count(&SS.silo_refusals); return 0; } while(0)
volatile struct TooieNpc665Sidecar tooie_npc665_state
    __attribute__((section(".tooie_npc665_state"),aligned(4)))={.control=TOOIE_NPC665_IDLE};
extern s16 D_8011B910,D_8012BE62,D_8012BE64;
extern u32 D_8012B800[],D_8012CA18,D_8012C820,D_8012C850[];
extern void* D_80136E70;
void* func_800AE080(s16);
void* func_80104200(s16);

NPC void tooie_npc665_capture(u32 actor,u32 data,u32 event) {
    if(!(NC.control&TOOIE_NPC665_PENDING) || event!=0x1Fu || actor!=SC.actor ||
       SS.silo_depth!=1u || SC.epoch!=SS.epoch) return;
    if(NC.phase.pending.capture_count || !silo_range(data,0x44u,3u)) {
        NC.control|=TOOIE_NPC665_BAD_CAPTURE;
        return;
    }
    NC.phase.pending.actor_data=data; NC.phase.pending.capture_count=1;
    NC.phase.pending.event=event;
}
NPC void tooie_npc665_retire(u32 actor,u32 descriptor,u32 handle) {
    if(!silo_range(descriptor,0x30u,3u) || descriptor<(u32)D_80132ED0 ||
       descriptor>=(u32)D_80132ED0+200u*0x30u ||
       (descriptor-(u32)D_80132ED0)%0x30u) return;
    u32 slot=(descriptor-(u32)D_80132ED0)/0x30u;
    volatile struct TooieNpc665Slot* e=&SS.npc665[slot];
    if(e->flags==TOOIE_NPC665_ROUTE && e->pose_handle==handle &&
       actor_binding(actor,descriptor,slot,e->birth,e->pair))
        silo_retire(&SS.silo[slot]);
}
static inline __attribute__((always_inline)) int npc_cursor(SiloBuffers* b,u32 bytes) {
    if(!AC.open_group || AC.buffers!=(u32)b || AC.pool_index>=2u ||
       !silo_range((u32)b,8u,3u)) return 0;
    u32 current=(u32)b->gfx;
    return D_8007BEDA==(s32)AC.pool_index && D_8007BCD0[AC.pool_index]==AC.gfx_base &&
        SS.epoch==AC.epoch && current>=AC.gfx_begin+24u && !(current&7u) &&
        current<=AC.gfx_base+MAIN_BYTES-bytes;
}
static inline __attribute__((always_inline)) void npc_failed(void) {
    NC.control|=TOOIE_NPC665_FAILED;
}
static inline __attribute__((always_inline)) int npc_live(SiloBuffers* b,u32 bytes) {
    if(!(NC.control&TOOIE_NPC665_ACTIVE)) return 0;
    if((NC.control&TOOIE_NPC665_FAILED) || AC.admitted!=TOOIE_NPC665_ADMITTED ||
       AC.slot>=200u || !SS.active || SS.poisoned || SS.silo_depth!=1u ||
       SS.npc665[AC.slot].flags!=TOOIE_NPC665_ROUTE ||
       SS.npc665[AC.slot].birth!=AC.birth || SS.npc665[AC.slot].pair!=AC.pair ||
       !(SS.npc665[AC.slot].block_and_valid&TOOIE_BRIDGE_BLOCK_VALID) ||
       !actor_binding(AC.actor,AC.descriptor,AC.slot,AC.birth,AC.pair) || !npc_cursor(b,bytes)) {
        npc_failed(); return 0;
    }
    return 1;
}
static inline __attribute__((always_inline)) void npc_group(GfxCommand* p,u32 id) {
    gEXMatrixGroup(p,id,G_EX_INTERPOLATE_SIMPLE,G_EX_PUSH,G_MTX_MODELVIEW,
        G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,
        G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_SKIP,
        G_EX_COMPONENT_AUTO,G_EX_ORDER_LINEAR,G_EX_EDIT_NONE,G_EX_ASPECT_AUTO,
        G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO);
}
NPC u32 tooie_npc665_admit(SiloBuffers* b,u32* local_cursor,u32 model) {
    u32 current,base,mtxbase,mtx,data,table,pose,controller,pool,entry,seen,renew,view_bit,camera;
    s32 count,index,offset,pose_handle,controller_handle,stride,capacity;
    volatile struct TooieNpc665Slot* e;
    if(!(NC.control&TOOIE_NPC665_PENDING) || !AC.actor || AC.admitted ||
       SS.silo_depth!=1u || AC.buffers!=(u32)b) return 0;
    silo_count(&SS.silo_calls); silo_epoch();
    if(!SS.epoch || SS.epoch==0xFFFFFFFFu || AC.epoch!=SS.epoch) REFUSE(TOOIE_BRIDGE_EPOCH);
    if(SS.poisoned || SS.active) REFUSE(TOOIE_BRIDGE_POISONED);
    if(!actor_binding(AC.actor,AC.descriptor,AC.slot,AC.birth,AC.pair) ||
       (AC.pair>>16)!=0x665u) REFUSE(TOOIE_BRIDGE_BIRTH);
    if(AC.view>3u || (u32)func_800A89F8()!=AC.view) REFUSE(TOOIE_BRIDGE_VIEW);
    if(!(AC.phase==0u || AC.phase==1u || AC.phase==2u || AC.phase==5u || AC.phase==6u) ||
       (u32)func_800EB854()!=AC.phase) REFUSE(TOOIE_BRIDGE_PHASE);
    if(NC.control&TOOIE_NPC665_BAD_CAPTURE || NC.phase.pending.capture_count!=1u ||
       NC.phase.pending.event!=0x1Fu) REFUSE(TOOIE_BRIDGE_BIRTH);
    data=NC.phase.pending.actor_data;
    if(!silo_range(data,0x44u,3u) || data<0x80001B14u ||
       *(volatile u32*)(AC.actor+0x10u)!=0x80086980u ||
       *(volatile u16*)(data+4u)!=0x665u || *(volatile u32*)(data+0x38u)!=0x80107C2Cu ||
       *(volatile u32*)(data+0x40u)!=data-0x1B14u+0x4E8u ||
       *(volatile u32*)(data+0x14u)!=data-0x1B14u+0x7FCu) REFUSE(TOOIE_BRIDGE_BIRTH);
    if((u32)b!=0x80127658u || D_8007BEDA<0 || D_8007BEDA>1 ||
       D_8007BCE8[0] || D_8007BCE8[1] || D_8007BCF0[0] || D_8007BCF0[1] ||
       !silo_range((u32)local_cursor,4u,3u)) REFUSE(TOOIE_BRIDGE_POOL);
    base=D_8007BCD0[D_8007BEDA]; current=*local_cursor;
    if(!silo_range(base,MAIN_BYTES,7u) || current<base || current>base+MAIN_BYTES ||
       (current&7u) || current<SS.last_end) REFUSE(TOOIE_BRIDGE_RANGE);
    camera=cam_actor_camera_id(current);
    if(!camera) REFUSE(TOOIE_BRIDGE_VIEW);
    /* Original postcallback D674C publication, not a second cache lookup. */
    count=D_8012BE62; index=D_8011B910;
    if(count<1 || count>135 || index<0 || index>=count ||
       D_8012BE64!=0x665 || D_8012CA18!=0x665u) REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    entry=(u32)D_8012B800+(u32)index*12u;
    if(!silo_range(entry,12u,3u) || (*(volatile u32*)(entry+8u)>>18)!=0x665u ||
       *(volatile u32*)(entry+4u)!=model || !silo_range(model,48516u,7u)) REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    offset=*(volatile s16*)(model+8u);
    if(*(volatile u32*)(model+4u)!=44976u || *(volatile u32*)(model+12u)!=12144u ||
       *(volatile u32*)(model+16u)!=26248u || *(volatile u16*)(model+10u) ||
       *(volatile u32*)(model+0x28u) || offset<0 || (u32)offset>48516u-8u ||
       D_8012C948[5]!=model+(u32)offset || D_8012C948[6]!=model+12144u ||
       D_8012C948[7]!=model+26248u) REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    table=D_8012CF8C;
    if(table!=*(volatile u32*)0x80123198u || !silo_range(table,25u*4u,3u) ||
       *(volatile u32*)(table+2u*4u)!=0x800DCF48u || *(volatile u32*)(table+10u*4u)!=0x800DD410u ||
       *(volatile u32*)(table+12u*4u)!=0x800DD56Cu || *(volatile u32*)(table+17u*4u)!=0x800DD65Cu ||
       *(volatile u32*)(table+24u*4u)!=0x800DD998u) REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    if(D_8012C820 || D_8012C838 || D_8012CEC4 || D_8012C948[14] || D_8012C948[26] ||
       D_8012C948[28] || D_8012C948[24]!=0x801017D0u || D_8012C948[25]!=AC.actor ||
       (*(volatile u32*)(AC.actor+0x70u)&0x01200000u) ||
       (*(volatile u8*)(AC.actor+0x7Bu)&0xC0u) || (*(volatile u8*)(AC.actor+0x78u)&2u) ||
       (*(volatile u8*)(AC.actor+0x74u)&15u)) REFUSE(TOOIE_BRIDGE_OVERRIDE);
    if(D_8012C850[1]<1u || D_8012C850[1]>4u || D_8012C850[4]>1u ||
       D_8012C850[5]>1u) REFUSE(TOOIE_BRIDGE_EXPRESSION);
    pose_handle=*(volatile s16*)(AC.descriptor+0x10u);
    if(pose_handle<=0 || pose_handle>=250) REFUSE(TOOIE_BRIDGE_ANIMATION);
    pose=(u32)func_800AE080((s16)pose_handle);
    if(pose!=(u32)D_8012C968 || !silo_range(pose,0x50u+41u*64u,3u) ||
       *(volatile u32*)pose!=41u || *(volatile u32*)(pose+4u)<41u ||
       !silo_finite(pose+0x50u,41u*16u) ||
       !silo_finite((u32)D_8012CE58,16u)) REFUSE(TOOIE_BRIDGE_ANIMATION);
    /* At DEFA0 the current root/pose are complete; C948[0..3] are
     * published only at DF0C0..DF0D0 and still belong to the prior draw. */
    controller_handle=*(volatile s16*)(AC.actor+0x8Cu); pool=(u32)D_80136E70;
    if(!silo_range(pool,4u,3u)) REFUSE(TOOIE_BRIDGE_ANIMATION);
    stride=*(volatile s16*)pool; capacity=*(volatile s16*)(pool+2u);
    if(stride!=0x3C || capacity<=1 || controller_handle<=0 || controller_handle>=capacity ||
       !silo_range(pool,4u+(u32)stride*(u32)capacity,3u)) REFUSE(TOOIE_BRIDGE_ANIMATION);
    controller=(u32)func_80104200((s16)controller_handle);
    if(controller!=pool+4u+(u32)controller_handle*0x3Cu) REFUSE(TOOIE_BRIDGE_ANIMATION);
    mtxbase=D_8007BCD8[D_8007BEDA]; mtx=(u32)b->mtx;
    if(!silo_range(mtxbase,0x12C00u,7u) || mtx<mtxbase || mtx>mtxbase+0x12C00u ||
       ((mtx-mtxbase)&63u) || mtxbase+0x12C00u-mtx<5120u ||
       base+MAIN_BYTES-current<3872u || SS.extra_bytes>4168u) REFUSE(TOOIE_BRIDGE_CAPACITY);
    e=&SS.npc665[AC.slot]; seen=1u<<(AC.view*7u+AC.phase); view_bit=1u<<AC.view;
    renew=!(e->block_and_valid&TOOIE_BRIDGE_BLOCK_VALID) || e->flags!=TOOIE_NPC665_ROUTE ||
        e->birth!=AC.birth || e->pair!=AC.pair || e->model!=model || e->controller!=controller ||
        e->pose_handle!=(u32)pose_handle || e->controller_handle!=(u32)controller_handle ||
        (e->stamp!=SS.epoch && e->stamp+1u!=SS.epoch);
    if(!renew) {
        if(e->stamp!=SS.epoch) { e->stamp=SS.epoch; e->seen_mask=0; e->camera_cut_views=0; }
        if(e->camera_id[AC.view] && e->camera_id[AC.view]!=camera) e->camera_cut_views|=view_bit;
        e->camera_id[AC.view]=camera;
        if(e->camera_cut_views&view_bit) REFUSE(TOOIE_BRIDGE_VIEW);
        if(e->seen_mask&seen) REFUSE(TOOIE_BRIDGE_DUPLICATE);
    }
    if(renew) {
        if(SS.next_block>TOOIE_BRIDGE_BLOCK_COUNT-28u) REFUSE(TOOIE_BRIDGE_BLOCKS);
        silo_retire(&SS.silo[AC.slot]);
        volatile u32* words=(volatile u32*)e;
        for(u32 i=0;i<18u;++i) words[i]=0;
        e->birth=AC.birth; e->pair=AC.pair; e->model=model; e->controller=controller;
        e->flags=TOOIE_NPC665_ROUTE; e->pose_handle=(u32)pose_handle;
        e->controller_handle=(u32)controller_handle; e->stamp=SS.epoch;
        e->camera_id[AC.view]=camera; e->block_and_valid=SS.next_block|TOOIE_BRIDGE_BLOCK_VALID;
        SS.next_block+=28u; SS.silo_valid_count+=1u;
    }
    e->seen_mask|=seen;
    AC.idbase=TOOIE_BRIDGE_ID_BASE+(e->block_and_valid&0x7FFFFu)*512u+(AC.view*7u+AC.phase)*512u;
    AC.gfx_base=base; AC.pool_index=(u32)D_8007BEDA; AC.gfx_begin=current; AC.open_group=1;
    AC.admitted=TOOIE_NPC665_ADMITTED; SS.active=1;
    NC.phase.admitted.emitted_bones=0; NC.phase.admitted.current_bone_role=INVALID_ROLE;
    NC.phase.admitted.emitted_hybrid_calls=0; NC.control=TOOIE_NPC665_IDLE|TOOIE_NPC665_ACTIVE;
    SS.extra_bytes+=2232u;
    if(SS.extra_bytes>SS.extra_highwater) SS.extra_highwater=SS.extra_bytes;
    GfxCommand* p=(GfxCommand*)current; gEXEnable(p); npc_group(p+1,AC.idbase+129u);
    *local_cursor=(u32)(p+3); silo_count(&SS.silo_admissions);
    return 1;
}
NPC u32 tooie_npc665_bone_push(SiloBuffers* b,s32 index) {
    if(!npc_live(b,24u)) return 0;
    if(index<0 || index>=41 || NC.phase.admitted.emitted_bones>=41u) { npc_failed(); return 0; }
    u32 prior=NC.phase.admitted.current_bone_role;
    u32 token=prior==INVALID_ROLE ? 1u : prior+2u;
    GfxCommand* p=(GfxCommand*)b->gfx; npc_group(p,AC.idbase+(u32)index+1u);
    b->gfx=(Gfx*)(p+2); NC.phase.admitted.emitted_bones+=1u;
    NC.phase.admitted.current_bone_role=(u32)index+1u;
    return token;
}
NPC void tooie_npc665_bone_pop(SiloBuffers* b,u32 token) {
    NC.phase.admitted.current_bone_role=token==1u ? INVALID_ROLE : token-2u;
    if(!npc_cursor(b,8u)) { npc_failed(); return; }
    GfxCommand* p=(GfxCommand*)b->gfx; gEXPopMatrixGroup(p,G_MTX_MODELVIEW); b->gfx=(Gfx*)(p+1);
}
NPC u32 tooie_npc665_leaf_begin(SiloBuffers* b) {
    if(!npc_live(b,32u)) return 0;
    u32 role=NC.phase.admitted.current_bone_role;
    if(role<1u || role>41u || NC.phase.admitted.emitted_hybrid_calls>=38u) { npc_failed(); return 0; }
    GfxCommand* p=(GfxCommand*)b->gfx; npc_group(p,AC.idbase+256u+role);
    gSPMatrix((Gfx*)(p+2),b->mtx,G_MTX_MODELVIEW|G_MTX_LOAD|G_MTX_NOPUSH);
    b->gfx=(Gfx*)(p+3); NC.phase.admitted.emitted_hybrid_calls+=1u;
    return 1;
}
NPC void tooie_npc665_leaf_end(SiloBuffers* b) {
    if(!npc_cursor(b,8u)) { npc_failed(); return; }
    GfxCommand* p=(GfxCommand*)b->gfx; gEXPopMatrixGroup(p,G_MTX_MODELVIEW); b->gfx=(Gfx*)(p+1);
}
NPC u32 tooie_npc665_end(SiloBuffers* b) {
    u32 failed=(NC.control&TOOIE_NPC665_FAILED)!=0 || SS.poisoned ||
        NC.phase.admitted.current_bone_role!=INVALID_ROLE;
    if(!npc_cursor(b,8u)) failed=1;
    else {
        GfxCommand* p=(GfxCommand*)b->gfx; gEXPopMatrixGroup(p,G_MTX_MODELVIEW); b->gfx=(Gfx*)(p+1);
        if((u32)b->gfx>SS.last_end) SS.last_end=(u32)b->gfx;
    }
    AC.open_group=0; AC.admitted=0; SS.active=0; npc665_reset();
    return failed;
}
#endif
