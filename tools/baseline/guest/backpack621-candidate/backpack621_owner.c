/* Immutable original621, warm one-copy/no-remapped-DL source class.
 * The original graph/pose builders run once; this owner adds public labels. */
#define F3DEX_GBI_2
#include "../silo-candidate/silo_bridge.h"
#include "../camera-candidate/camera_projection_internal_04.h"
#include "backpack621_owner.h"
#include "rt64_extended_gbi.h"
#if TOOIE_BACKPACK621
#include "../cpu625-candidate/cpu625_layout.h"
_Static_assert(TOOIE_CAMERA_ID_END<=TOOIE_BACKPACK621_ID_FIRST,"621/camera domains");
_Static_assert(TOOIE_CPU625_ID_FIRST+512ULL*TOOIE_CPU625_BLOCKS<=TOOIE_BACKPACK621_ID_FIRST,
    "621/current CPU625 domains");
#define B621 __attribute__((section(".tooie_backpack621_helpers"),noinline))
#define BS tooie_backpack621_state
#define BX BS.context
#define BE BS.emission
#define B621_INVALID 0xFFFFFFFFu
/* At DEFA0: 416 original bytes + 592 added bytes; keep 48 camera bytes separate.
 * Added bytes: envelope 32 + 14 bone scopes * 24 + 7 hybrid scopes * 32. */
#define B621_SUFFIX 1008u
#define B621_TAIL 48u
enum { B621_BAD_LAYOUT=1,B621_NESTED,B621_BEGIN,B621_CONTEXT,B621_SLOT,
    B621_COMPONENT,B621_COPY,B621_MODEL,B621_POSE,B621_POOL,B621_CAMERA,
    B621_DUPLICATE,B621_BLOCKS,B621_CURSOR,B621_RETIRED,B621_CUT,
    B621_BUDGET,B621_ROLE };
extern u32 D_80135490[8],D_801354DC;
extern u8 D_801354B0[8];
extern volatile u32 D_8012D500,D_8012D558;
extern s16 D_80132E9C;
s32 func_80100A74(s16,u8);
s32 func_80100AC4(s16);
volatile struct TooieBackpack621State tooie_backpack621_state
    __attribute__((section(".tooie_backpack621_state"),aligned(4)))=
    {.magic=TOOIE_BACKPACK621_MAGIC,.layout=TOOIE_BACKPACK621_LAYOUT};

SI u32 b621_phase(u32 phase) {
    if(phase<=2u)return phase;
    if(phase==5u || phase==6u)return phase-2u;
    return B621_INVALID;
}
SI u32 b621_slot(u32 self) {
    if(!silo_range(self,0x188u,3u))return 8u;
    u32 slot=*(volatile u32*)(self+0x184u);
    return slot<8u && D_801354DC==slot && D_80135490[slot]==self &&
        !D_801354B0[slot] ? slot : 8u;
}
SI void b621_clear(void) {
    volatile u32* words=(volatile u32*)&BX;
    for(u32 i=0;i<13u;i++)words[i]=0;
    BE.bones=0;BE.current_role=B621_INVALID;BE.hybrids=0;BE.failed=0;
}
SI void b621_invalidate(u32 self) {
    for(u32 i=0;i<8u;i++)if(BS.owner[i].owner==self)BS.owner[i].flags=0;
}
SI u32 b621_refuse(u32 reason) {
    BS.last_reason=reason;silo_count(&BS.refusals);
    /* Keep same-epoch cut/duplicate suppression; neither issues a new range. */
    if(reason!=B621_CUT && reason!=B621_DUPLICATE)b621_invalidate(BX.owner);
    return 0;
}
SI void b621_fail(u32 reason) {
    if(!BE.failed)silo_count(&BS.tripwires);
    BE.failed=1;BS.last_reason=reason;b621_invalidate(BX.owner);
}
SI u32 b621_stop(u32 reason) {
    b621_fail(reason);b621_clear();return 1;
}
SI u32 b621_component(u32 self) {
    u32 part=*(volatile u32*)(self+20u);
    if(!silo_range(part,24u,3u) || *(volatile u32*)part!=0x621u ||
       *(volatile u32*)(part+12u)!=2u || *(volatile u32*)(part+16u)!=0x3F800000u ||
       !*(volatile u8*)(part+8u) || *(volatile u8*)(part+9u) || *(volatile u8*)(part+10u))return 0;
    return part;
}
/* Original 80100690 publishes the stride-12 FreeList at 80135A60.
 * A74/AC4 use freelist_at without bounds checks; preflight its four-byte header.
 * Entry +8 is AC4's secondary handle; requiring zero avoids that second lookup. */
SI u32 b621_primary(u32 handle) {
    u32 pool=*(volatile u32*)0x80135A60u;
    if(!silo_range(pool,4u,3u) || *(volatile s16*)pool!=12)return 0;
    s32 capacity=*(volatile s16*)(pool+2u);
    if(capacity<=1 || !handle || handle>=(u32)capacity ||
       !silo_range(pool,4u+12u*(u32)capacity,3u))return 0;
    u32 entry=pool+4u+12u*handle;
    return *(volatile u8*)(entry+8u)==0u ? entry : 0u;
}
/* Same source-owned saved WORLD camera law as625, without its camera debit. */
SI u32 b621_camera(void) {
    struct TooieCameraControl* c=&CAM_STATE.control;
    struct TooieCameraProduction* p=&CAM_STATE.saved;
    struct TooieCameraRecord* r;
#if TOOIE_PRE_BACKGROUND
    if(tooie_pre_background.pending.depth)return 0;
#endif
    if(!CAM_STATE.owner_frame || !c->active || D_8012D508 || c->latch)return 0;
    r=cam_find(p->camera);
    if(c->epoch!=SS.epoch || !p->valid || p->epoch!=c->epoch ||
       p->matrix!=D_8012D558 || p->camera!=D_8012D500 || p->view>=4u ||
       p->view!=BX.view || p->view!=(u32)D_80127674 ||
       p->id<TOOIE_CAMERA_ID_FIRST || p->id>=TOOIE_CAMERA_ID_END ||
       p->policy!=CAM_POLICY_WORLD || p->role!=TOOIE_CAMERA_WORLD_INITIAL ||
       !r || r->birth!=p->birth || r->revision!=p->revision ||
       r->view_id[p->view]!=p->id || r->view_scale[p->view]!=p->scale)return 0;
    return p->id;
}
/* Fixed immutable 0x621 fingerprint: 8096 bytes, geometry +7384, DL +1400,
 * vertices +3840 and bones +3608/count 14. This is not general graph validation;
 * the original publication and fixed node guards below remain required. */
SI u32 b621_model(u32 model) {
    u32 table=D_8012CF8C;
    if(!silo_range(model,8096u,7u) || *(volatile u32*)(model+4u)!=7384u ||
       *(volatile u16*)(model+10u) || *(volatile u32*)(model+12u)!=1400u ||
       *(volatile u32*)(model+16u)!=3840u || *(volatile u32*)(model+24u)!=3608u ||
       *(volatile s16*)(model+3612u)!=14 || *(volatile u32*)(model+40u))return 0;
    s32 texture=*(volatile s16*)(model+8u);
    if(texture<0 || (u32)texture>8096u-8u || D_8012C948[5]!=model+(u32)texture ||
       D_8012C948[6]!=model+1400u || !silo_range(table,25u*4u,3u) ||
       table!=*(volatile u32*)0x80123198u ||
       *(volatile u32*)(table+2u*4u)!=0x800DCF48u ||
       *(volatile u32*)(table+10u*4u)!=0x800DD410u ||
       *(volatile u32*)(table+12u*4u)!=0x800DD56Cu ||
       *(volatile u32*)(table+17u*4u)!=0x800DD65Cu ||
       *(volatile u32*)(table+24u*4u)!=0x800DD998u)return 0;
    /* Exact retained offsets: four64-byte pairs, then the24-byte type10 gap.
     * Compute fixed addresses without compiler-emitted local array constants. */
    for(u32 i=0;i<8u;i++) {
        u32 node=model+7608u+64u*i+(i>=4u ? 24u : 0u);
        if(*(volatile u32*)node!=17u || *(volatile s16*)(node+14u))return 0;
    }
    for(u32 i=0;i<7u;i++) {
        u32 node=model+7624u+64u*i+(i>=4u ? 24u : 0u);
        if(*(volatile u32*)node!=24u || *(volatile s16*)(node+46u))return 0;
    }
    return *(volatile u32*)(model+7864u)==10u &&
        *(volatile s16*)(model+7872u)==4 && *(volatile s16*)(model+7874u)==9;
}
SI void b621_group(GfxCommand* p,u32 id) {
    gEXMatrixGroup(p,id,G_EX_INTERPOLATE_SIMPLE,G_EX_PUSH,G_MTX_MODELVIEW,
        G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,
        G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_SKIP,
        G_EX_COMPONENT_AUTO,G_EX_ORDER_LINEAR,G_EX_EDIT_NONE,G_EX_ASPECT_AUTO,
        G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO);
}
SI void b621_reload(GfxCommand* p,SiloBuffers* b) {
    gSPMatrix((Gfx*)p,b->mtx,G_MTX_MODELVIEW|G_MTX_LOAD|G_MTX_NOPUSH);
}
/* Ownership-only check: paid tokens remain writable after retirement/failure. */
SI u32 b621_cursor(SiloBuffers* b,u32 bytes,u32 outer_close) {
    u32 pool=BX.pool_index,base=BX.gfx_base;
    struct TooieCameraControl* c=&CAM_STATE.control;
    if(!BX.paid || BX.buffers!=(u32)b || BX.buffers!=0x80127658u || pool>=2u ||
       BX.epoch!=SS.epoch || BX.epoch!=c->epoch || D_8007BEDA!=(s32)pool ||
       D_8007BCD0[pool]!=base || base!=(u32)&tooie_camera_main[pool][0] ||
       !silo_range(base,TOOIE_CAMERA_M_BYTES,7u) || c->main_base!=base ||
       c->main_end!=base+TOOIE_CAMERA_M_BYTES || BX.gfx_begin<base ||
       BX.gfx_begin>base+TOOIE_CAMERA_M_BYTES-B621_SUFFIX-B621_TAIL)return 0;
    u32 limit=c->main_end-B621_TAIL;
    if(limit>BX.gfx_begin+B621_SUFFIX)limit=BX.gfx_begin+B621_SUFFIX;
    if(!outer_close)limit-=8u;
    u32 current=(u32)b->gfx;
    return !(current&7u) && current>=BX.gfx_begin+24u && current<=limit-bytes;
}
SI u32 b621_live(SiloBuffers* b,u32 bytes) {
    if(!BX.paid)return 0;
    if(BE.failed)return 0;
    if(BX.slot>=8u || b621_slot(BX.owner)!=BX.slot || SS.active || SC.actor ||
       SC.admitted || SS.poisoned || SS.silo_depth!=1u || BX.depth ||
       BX.view!=(u32)D_80127674 || BX.phase!=(u32)(s32)D_80132E9C ||
       !b621_cursor(b,bytes,0u)) {b621_fail(B621_CONTEXT);return 0;}
    u32 matrix_base=D_8007BCD8[BX.pool_index],matrix=(u32)b->mtx;
    if(!silo_range(matrix_base,0x12C00u,7u) || matrix<matrix_base ||
       matrix>matrix_base+0x12C00u-64u || ((matrix-matrix_base)&63u)) {
        b621_fail(B621_POOL);return 0;
    }
    volatile struct TooieBackpack621Owner* e=&BS.owner[BX.slot];
    u32 part=b621_component(BX.owner),camera=b621_camera();
    if(!(e->flags&TOOIE_BACKPACK621_VALID) || e->owner!=BX.owner || !part ||
       e->backpack!=part || e->model!=BX.model ||
       e->handle!=*(volatile u8*)(part+8u) || e->stamp!=BX.epoch || !camera ||
       e->camera_id[BX.view]!=camera || D_8012C948[7]!=e->copy ||
       D_8012C948[6]!=e->display_list || *(volatile u32*)0x8012C824u!=e->animation ||
       *(volatile u32*)0x8012C844u!=e->vector_cache) {b621_fail(B621_CONTEXT);return 0;}
    return 1;
}
B621 u32 tooie_backpack621_begin(u32 self,u32 model) {
    if(BS.magic!=TOOIE_BACKPACK621_MAGIC || BS.layout!=TOOIE_BACKPACK621_LAYOUT)
        return b621_stop(B621_BAD_LAYOUT);
    if(BS.tripwires)return 1;
    u32 slot=b621_slot(self);
    if(slot>=8u) {b621_invalidate(self);return 0;}
    u32 part=b621_component(self);
    if(!part)return 0;
    if(BX.owner)return b621_stop(B621_NESTED);
    BX.owner=self;BX.model=model;BX.slot=slot;BX.view=(u32)D_80127674;
    BX.phase=(u32)(s32)D_80132E9C;BX.epoch=SS.epoch;BX.depth=SS.silo_depth;
    /* C840 is the original reusable pose slot. DE498 builds it before DEFA0;
     * begin checks it is warm, admit checks the completed C968 publication. */
    u32 scratch=*(volatile u32*)0x8012C840u;
    if(BX.depth || SS.active || SC.actor || SC.admitted || D_8012C948[24] ||
       !BX.epoch || BX.epoch==B621_INVALID || BX.view>=4u ||
       b621_phase(BX.phase)==B621_INVALID || D_8012C968 ||
       !silo_range(scratch,0x3D0u,3u) || *(volatile u32*)(scratch+4u)<14u) {
        BX.depth=B621_INVALID;return b621_refuse(B621_BEGIN);
    }
    return 0;
}
B621 u32 tooie_backpack621_admit(u32 buffers,u32 cursor_slot,u32 model) {
    if(!BX.owner || BX.paid)return 0;
    if(BX.epoch!=SS.epoch || !BX.epoch || BX.epoch==B621_INVALID)
        return b621_refuse(B621_CONTEXT);
    silo_epoch(); /* Must precede shared poison/highwater/budget observations. */
    if(BS.tripwires || BX.depth || SS.silo_depth!=1u || SS.active || SC.actor || SC.admitted ||
       SS.poisoned || BX.model!=model || BX.view!=(u32)D_80127674 ||
       BX.phase!=(u32)(s32)D_80132E9C)return b621_refuse(B621_CONTEXT);
    if(b621_slot(BX.owner)!=BX.slot)return b621_refuse(B621_SLOT);
    u32 part=b621_component(BX.owner);
    if(!part)return b621_refuse(B621_COMPONENT);
    u32 handle=*(volatile u8*)(part+8u);
    if(!b621_primary(handle))return b621_refuse(B621_COPY);
    u32 copy=(u32)func_80100A74((s16)handle,0u);
    if(!silo_range(copy,2840u,7u) || D_8012C948[7]!=copy ||
       (*(volatile u16*)(copy+22u)>>1)!=176u || func_80100AC4((s16)handle) ||
       D_8012C948[11])return b621_refuse(B621_COPY);
    if(!b621_model(model) || D_8012C838 || D_8012CEC4 || D_8012C948[14] ||
       D_8012C948[24] || D_8012C948[26] || D_8012C948[28] ||
       *(volatile u32*)0x8012C848u)return b621_refuse(B621_MODEL);
    /* Original entry3 sends backpack +4 through DF738 to C844;
     * DD410 reads it for DC028 vector-cache writes. Preserve that binding. */
    u32 pose=(u32)D_8012C968,vector=*(volatile u32*)0x8012C844u;
    if(pose!=*(volatile u32*)0x8012C840u || !silo_range(pose,0x3D0u,3u) ||
       *(volatile u32*)pose!=14u || *(volatile u32*)(pose+4u)<14u ||
       !silo_finite(pose+0x50u,224u) || !silo_finite((u32)D_8012CE58,16u) ||
       vector!=*(volatile u32*)(part+4u) || !silo_range(vector,0x118u,3u))return b621_refuse(B621_POSE);
    if(buffers!=0x80127658u || !silo_range(cursor_slot,4u,3u) ||
       D_8007BEDA<0 || D_8007BEDA>1 || D_8007BCE8[0] || D_8007BCE8[1] ||
       D_8007BCF0[0] || D_8007BCF0[1])return b621_refuse(B621_POOL);
    u32 pool=(u32)D_8007BEDA,base=D_8007BCD0[pool],current=*(volatile u32*)cursor_slot;
    u32 mtxbase=D_8007BCD8[pool],mtx=*(volatile u32*)(buffers+4u),camera=b621_camera();
    struct TooieCameraControl* c=&CAM_STATE.control;
    if(!camera || base!=(u32)&tooie_camera_main[pool][0] || c->main_base!=base ||
       c->main_end!=base+TOOIE_CAMERA_M_BYTES || !silo_range(base,TOOIE_CAMERA_M_BYTES,7u) ||
       current<base || current<c->main_begin || current<SS.last_end || (current&7u) ||
       current>c->main_end-B621_SUFFIX-B621_TAIL ||
       c->main_extra>TOOIE_CAMERA_EXTRA_BYTES-B621_TAIL)return b621_refuse(B621_CAMERA);
    /* 22 original matrix slots need 1408 bytes; the model budget is 6400 - 592.
     * Physical suffix/tail checks above are separate from this shared model debit. */
    if(!silo_range(mtxbase,0x12C00u,7u) || mtx<mtxbase || ((mtx-mtxbase)&63u) ||
       mtx>mtxbase+0x12C00u-1408u || SS.extra_bytes>5808u)return b621_refuse(B621_BUDGET);
    volatile struct TooieBackpack621Owner* e=&BS.owner[BX.slot];
    /* DF41C publishes the decoded animation input at C824 for DE498.
     * Refresh its address each occurrence; renewal below compares nullness only. */
    u32 animation=*(volatile u32*)0x8012C824u,seen=1u<<(BX.view*5u+b621_phase(BX.phase));
    /* Decoded-buffer address is occurrence input; only nullness changes producer class. */
    u32 renew=!(e->flags&TOOIE_BACKPACK621_VALID) || e->owner!=BX.owner ||
        e->backpack!=part || e->model!=model || e->handle!=handle || e->copy!=copy ||
        e->display_list!=model+1400u || (e->animation==0u)!=(animation==0u) || e->vector_cache!=vector ||
        (e->stamp!=SS.epoch && e->stamp+1u!=SS.epoch);
    if(!renew) {
        if(e->stamp!=SS.epoch) {e->stamp=SS.epoch;e->seen=0;e->flags=TOOIE_BACKPACK621_VALID;}
        if(e->camera_id[BX.view] && e->camera_id[BX.view]!=camera)e->flags|=1u<<(BX.view+8u);
        e->camera_id[BX.view]=camera;
        if(e->flags&(1u<<(BX.view+8u)))return b621_refuse(B621_CUT);
        if(e->seen&seen)return b621_refuse(B621_DUPLICATE);
    }
    if(renew) {
        if(BS.next_block>TOOIE_BACKPACK621_BLOCKS-TOOIE_BACKPACK621_LIFE_BLOCKS) {
            e->flags=0;return b621_refuse(B621_BLOCKS); /* No old-range fallback. */
        }
        volatile u32* words=(volatile u32*)e;
        for(u32 i=0;i<16u;i++)words[i]=0;
        e->owner=BX.owner;e->backpack=part;e->model=model;e->handle=handle;
        e->idbase=TOOIE_BACKPACK621_ID_FIRST+BS.next_block*512u;
        BS.next_block+=TOOIE_BACKPACK621_LIFE_BLOCKS;
        e->stamp=SS.epoch;e->flags=TOOIE_BACKPACK621_VALID;
        e->copy=copy;e->display_list=model+1400u;e->animation=animation;e->vector_cache=vector;
    }
    e->animation=animation;
    e->seen|=seen;e->camera_id[BX.view]=camera;
    BX.buffers=buffers;BX.pool_index=pool;BX.gfx_base=base;BX.gfx_begin=current;
    BX.group_id=e->idbase+(BX.view*5u+b621_phase(BX.phase))*512u;BX.paid=1;BS.epoch=SS.epoch;
    BE.bones=0;BE.current_role=B621_INVALID;BE.hybrids=0;BE.failed=0;
    SS.extra_bytes+=592u;
    if(SS.extra_bytes>SS.extra_highwater)SS.extra_highwater=SS.extra_bytes;
    GfxCommand* p=(GfxCommand*)current;gEXEnable(p);
    gEXMatrixGroup(p+1,BX.group_id+129u,G_EX_INTERPOLATE_SIMPLE,G_EX_PUSH,G_MTX_MODELVIEW,
        G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,
        G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_ORDER_LINEAR,
        G_EX_EDIT_NONE,G_EX_ASPECT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP);
    *(volatile u32*)cursor_slot=current+24u;c->main_begin=current+24u;BS.last_reason=0;
    return 0;
}
B621 u32 tooie_backpack621_bone_push(SiloBuffers* b,s32 index) {
    if(!b621_live(b,24u))return 0;
    if(index<0 || index>=14 || BE.bones>=14u) {b621_fail(B621_ROLE);return 0;}
    u32 prior=BE.current_role,token=prior==B621_INVALID ? 1u : prior+2u;
    GfxCommand* p=(GfxCommand*)b->gfx;b621_group(p,BX.group_id+(u32)index+1u);
    b->gfx=(Gfx*)(p+2);BE.current_role=(u32)index+1u;BE.bones+=1u;return token;
}
B621 void tooie_backpack621_bone_pop(SiloBuffers* b,u32 token) {
    BE.current_role=token==1u ? B621_INVALID : token-2u;
    if(!b621_cursor(b,8u,0u)) {b621_fail(B621_CURSOR);return;}
    GfxCommand* p=(GfxCommand*)b->gfx;gEXPopMatrixGroup(p,G_MTX_MODELVIEW);b->gfx=(Gfx*)(p+1);
}
B621 u32 tooie_backpack621_leaf_begin(SiloBuffers* b) {
    if(!b621_live(b,32u))return 0;
    if(BE.current_role<1u || BE.current_role>14u || BE.hybrids>=7u) {b621_fail(B621_ROLE);return 0;}
    GfxCommand* p=(GfxCommand*)b->gfx;b621_group(p,BX.group_id+256u+BE.current_role);
    b621_reload(p+2,b);b->gfx=(Gfx*)(p+3);BE.hybrids+=1u;return 1;
}
B621 void tooie_backpack621_leaf_end(SiloBuffers* b) {
    if(!b621_cursor(b,8u,0u)) {b621_fail(B621_CURSOR);return;}
    GfxCommand* p=(GfxCommand*)b->gfx;gEXPopMatrixGroup(p,G_MTX_MODELVIEW);b->gfx=(Gfx*)(p+1);
}
B621 u32 tooie_backpack621_end(u32 self) {
    if(!BX.owner)return BS.tripwires ? 1u : 0u;
    if(!BX.paid) {b621_clear();return BS.tripwires ? 1u : 0u;}
    SiloBuffers* b=(SiloBuffers*)BX.buffers;
    if(self!=BX.owner || !b621_cursor(b,8u,1u))return b621_stop(B621_CURSOR);
    if(BE.current_role!=B621_INVALID)b621_fail(B621_ROLE);
    if(BX.view!=(u32)D_80127674 || BX.phase!=(u32)(s32)D_80132E9C)silo_count(&BS.drifts);
    GfxCommand* p=(GfxCommand*)b->gfx;gEXPopMatrixGroup(p,G_MTX_MODELVIEW);b->gfx=(Gfx*)(p+1);
    u32 end=(u32)b->gfx;
    if(CAM_STATE.control.main_begin<end)CAM_STATE.control.main_begin=end;
    if(SS.last_end<end)SS.last_end=end;
    u32 failed=BE.failed || BS.tripwires;b621_clear();return failed ? 1u : 0u;
}
B621 u32 tooie_backpack621_retire(u32 self,u32 requested,u32 reason) {
    if(reason==1u) {
        if(!silo_range(self,24u,3u))return 0;
        u32 part=*(volatile u32*)(self+20u);
        if(!silo_range(part,24u,3u))return 0;
        if(*(volatile u32*)(part+12u)!=2u && requested!=2u)return 0;
    }
    b621_invalidate(self);
    if(BX.owner==self) {
        if(BX.paid)b621_fail(B621_RETIRED);
        else BX.depth=B621_INVALID;
    }
    return 0;
}
#endif
