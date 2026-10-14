/* CPU625 public AUTO owner proposal. Integrator compiles only when selected. */
#include "../silo-candidate/silo_bridge.h"
#include "../camera-candidate/camera_projection_internal_04.h"
#include "cpu625_layout.h"
#if !defined(TOOIE_CPU625) || (TOOIE_CPU625 != 0 && TOOIE_CPU625 != 1)
#error Explicit boolean TOOIE_CPU625 selection required
#endif
#if TOOIE_CPU625
#define CPU625 __attribute__((section(".tooie_cpu625_helpers"),noinline))
#define CS tooie_cpu625_state
#define CX CS.context
extern u32 D_80135490[8],D_801354DC;
extern u8 D_801354B0[8];
extern volatile u32 D_8012D500,D_8012D558;
extern s16 D_80132E9C;
s32 func_80100A74(s16,u8);
s32 func_80100AC4(s16);
struct TooieCpu625State tooie_cpu625_state
    __attribute__((section(".tooie_cpu625_state"),aligned(4)))=
    {.magic=TOOIE_CPU625_MAGIC,.layout=TOOIE_CPU625_LAYOUT};

/* Both original lookup owners allocate12-byte inline FreeList elements.
 * Validate header before reading it and the full computed pool before getters. */
SI u32 c625_pool_entry(u32 pool,u32 index) {
    s32 capacity;
    if(!silo_range(pool,4u,3u) || *(volatile s16*)pool!=12)return 0;
    capacity=*(volatile s16*)(pool+2u);
    if(capacity<=1 || !index || index>=(u32)capacity ||
        !silo_range(pool,4u+12u*(u32)capacity,3u))return 0;
    return pool+4u+12u*index;
}
SI u32 c625_lookup_safe(u32 handle) {
    u32 e=c625_pool_entry(*(volatile u32*)0x80135A60u,handle),second;
    if(!e)return 0;
    second=*(volatile u8*)(e+8u);
    if(second && !c625_pool_entry(*(volatile u32*)0x80135A64u,second))return 0;
    return 1;
}
SI u32 c625_phase(u32 p) { return p==0u || p==1u || p==2u || p==5u || p==6u; }
SI u32 c625_slot(u32 self) {
    u32 slot;
    if(!silo_range(self,0x188u,3u))return 8u;
    slot=*(volatile u32*)(self+0x184u);
    return slot<8u && D_801354DC==slot && D_80135490[slot]==self &&
        !D_801354B0[slot] ? slot : 8u;
}
SI void c625_clear(void) {
    volatile u32* p=(volatile u32*)&CX;u32 i;for(i=0;i<12u;i++)p[i]=0;
}
SI u32 c625_trip(u32 reason) {
    silo_count(&CS.tripwires);CS.last_reason=reason;c625_clear();return 1u;
}
SI u32 c625_refuse(u32 reason) {
    u32 i;CS.last_reason=reason;silo_count(&CS.refusals);
    for(i=0;i<8u;i++)if(CS.owner[i].owner==CX.owner)CS.owner[i].idbase=0;
    return 0;
}
/* Same accepted camera law, with the lexical625 view instead of actor context. */
SI u32 c625_camera(u32 cursor) {
    struct TooieCameraControl* c=&CAM_STATE.control;
    struct TooieCameraProduction* p=&CAM_STATE.saved;
    struct TooieCameraRecord* r;
#if TOOIE_PRE_BACKGROUND
    if(tooie_pre_background.pending.depth)return 0;
#endif
    if(!CAM_STATE.owner_frame || !c->active || D_8012D508 || c->latch ||
        (cursor&7u) || cursor<c->main_begin || cursor>c->main_end-144u ||
        c->main_extra>TOOIE_CAMERA_EXTRA_BYTES-32u-48u)return 0;
    r=cam_find(p->camera);
    if(c->epoch!=SS.epoch || !p->valid || p->epoch!=c->epoch ||
        p->matrix!=D_8012D558 || p->camera!=D_8012D500 || p->view>=4u ||
        p->view!=CX.view || p->view!=(u32)D_80127674 ||
        p->id<TOOIE_CAMERA_ID_FIRST || p->id>=TOOIE_CAMERA_ID_END ||
        p->policy!=CAM_POLICY_WORLD || p->role!=TOOIE_CAMERA_WORLD_INITIAL ||
        !r || r->birth!=p->birth || r->revision!=p->revision ||
        r->view_id[p->view]!=p->id || r->view_scale[p->view]!=p->scale)return 0;
    return p->id;
}
/* Fixed13-node source class, not a new list decoder. Original parse validity
 * is inherited as for065E; ordered per-WORLD roles are a fixture obligation. */
SI u32 c625_graph(u32 model) {
    u32 node=model+9776u,i,type,step,n10=0,n17=0,size,table=D_8012CF8C;
    if(!silo_range(model,9992u,7u) || *(volatile u32*)(model+4u)!=9776u ||
        *(volatile u16*)(model+10u)!=0u || *(volatile u32*)(model+16u)!=5432u ||
        *(volatile u32*)(model+24u)!=5152u || *(volatile u32*)(model+40u)!=3056u ||
        *(volatile s16*)(model+5156u)!=17 ||
        !silo_range(table,18u*4u,3u) || table!=*(volatile u32*)0x80123198u ||
        *(volatile u32*)(table+40u)!=0x800DD410u ||
        *(volatile u32*)(table+68u)!=0x800DD65Cu)return 0;
    for(i=0;i<13u;i++) {
        if(node<model+9776u || node>model+9992u-16u)return 0;
        type=*(volatile u32*)node;step=*(volatile u32*)(node+4u);
        if(type==10u) {size=24u;n10++;}
        else if(type==17u) {
            size=16u;n17++;
            if(*(volatile s16*)(node+8u)<0 || (*(volatile u16*)(node+14u)&1u))return 0;
        } else return 0;
        if(node>model+9992u-size || (i<12u ? step!=size : step!=0u))return 0;
        node+=size;
    }
    return n10==1u && n17==12u && node==model+9992u;
}
CPU625 u32 tooie_cpu625_begin(u32 self,u32 model) {
    u32 slot,backpack;
    if(CS.magic!=TOOIE_CPU625_MAGIC || CS.layout!=TOOIE_CPU625_LAYOUT)return c625_trip(1u);
    if(CX.owner)return c625_trip(2u);
    if(CS.tripwires)return 1u;
    slot=c625_slot(self);
    if(slot>=8u) {
        for(slot=0;slot<8u;slot++)if(CS.owner[slot].owner==self)CS.owner[slot].idbase=0;
        return 0;
    }
    backpack=*(volatile u32*)(self+20u);
    if(!silo_range(backpack,24u,3u) || *(volatile u32*)(backpack+12u)!=3u ||
        *(volatile u32*)backpack!=0x625u || *(volatile u32*)(backpack+16u)!=0x3DCCCCCDu)return 0;
    CX.owner=self;CX.model=model;CX.view=(u32)D_80127674;
    CX.phase=(u32)(s32)D_80132E9C;CX.epoch=SS.epoch;CX.depth=SS.silo_depth;
    if(CX.depth || SS.active || SC.actor || SC.admitted || D_8012C948[24] ||
        !SS.epoch || SS.epoch==0xFFFFFFFFu || CX.view>=4u || !c625_phase(CX.phase)) {
        c625_refuse(3u);
        /* Failed lexical begin never creates a paid token. */
        CX.depth=0xFFFFFFFFu;
    }
    return 0;
}
CPU625 u32 tooie_cpu625_admit(u32 buffers,u32 cursor_slot,u32 model) {
    u32 slot,backpack,handle,copy,camera,base,current,seen,renew,i;
    s32 texture_offset;
    struct TooieCpu625Owner* e;
    struct TooieCameraControl* c=&CAM_STATE.control;
    if(!CX.owner || CX.paid)return 0;
    if(CS.tripwires || CX.depth || SS.silo_depth!=1u || SS.active || SC.actor || SC.admitted)
        return c625_refuse(4u);
    if(CX.model!=model || CX.epoch!=SS.epoch || CX.view!=(u32)D_80127674 ||
        CX.phase!=(u32)(s32)D_80132E9C)return c625_refuse(5u);
    slot=c625_slot(CX.owner);if(slot>=8u)return c625_refuse(6u);
    backpack=*(volatile u32*)(CX.owner+20u);
    if(!silo_range(backpack,24u,3u) || *(volatile u32*)(backpack+12u)!=3u ||
        *(volatile u32*)backpack!=0x625u || *(volatile u32*)(backpack+16u)!=0x3DCCCCCDu ||
        !*(volatile u8*)(backpack+8u) || *(volatile u8*)(backpack+9u)>1u ||
        *(volatile u8*)(backpack+10u)!=1u)return c625_refuse(7u);
    handle=*(volatile u8*)(backpack+8u);
    if(!c625_lookup_safe(handle))return c625_refuse(16u);
    copy=(u32)func_80100A74((s16)handle,*(volatile u8*)(backpack+9u));
    if(!silo_range(copy,3480u,7u) || D_8012C948[7]!=copy ||
        D_8012C948[11]!=(u32)func_80100AC4((s16)handle) || !c625_graph(model) ||
        *(volatile u16*)(copy+22u)>>1!=216u)return c625_refuse(8u);
    /* graph above proves the full model range before this original signed field. */
    texture_offset=*(volatile s16*)(model+8u);
    if(texture_offset<0 || (u32)texture_offset>9992u-8u || D_8012C838 || D_8012CEC4 ||
        D_8012C948[24] || D_8012C948[26] || D_8012C948[28] ||
        D_8012C948[5]!=model+(u32)texture_offset ||
        D_8012C948[6]!=model+*(volatile u32*)(model+12u) ||
        D_8012C948[6]<model || D_8012C948[6]>model+9992u-8u ||
        *(volatile u32*)0x8012C848u || !D_8012C968 ||
        !silo_range((u32)D_8012C968,0x50u+17u*64u,3u) ||
        *(volatile u32*)D_8012C968!=17u || *((volatile u32*)D_8012C968+1)<17u)
        return c625_refuse(9u);
    if(buffers!=0x80127658u || !silo_range(cursor_slot,4u,3u) ||
        D_8007BEDA<0 || D_8007BEDA>1 || D_8007BCE8[0] || D_8007BCE8[1] ||
        D_8007BCF0[0] || D_8007BCF0[1])return c625_refuse(10u);
    base=D_8007BCD0[D_8007BEDA];current=*(volatile u32*)cursor_slot;
    camera=c625_camera(current);
    if(!camera || base!=(u32)&tooie_camera_main[D_8007BEDA][0] ||
        c->main_base!=base || c->main_end!=base+TOOIE_CAMERA_M_BYTES ||
        !silo_range(base,TOOIE_CAMERA_M_BYTES,7u) || current<base ||
        current>base+TOOIE_CAMERA_M_BYTES-144u || (current&7u) || SS.extra_bytes>6400u)
        return c625_refuse(11u);
    e=&CS.owner[slot];seen=1u<<(CX.view*7u+CX.phase);
    renew=!e->idbase || e->owner!=CX.owner || e->backpack!=backpack ||
        e->model!=model || e->handle!=handle ||
        (e->stamp!=SS.epoch && e->stamp+1u!=SS.epoch) ||
        (e->camera_id[CX.view] && e->camera_id[CX.view]!=camera);
    if(!renew && e->stamp==SS.epoch && (e->seen&seen))return c625_refuse(12u);
    if(renew) {
        if(CS.next_block>=TOOIE_CPU625_BLOCKS)return c625_refuse(13u);
        e->owner=CX.owner;e->backpack=backpack;e->model=model;e->handle=handle;
        e->idbase=TOOIE_CPU625_ID_FIRST+512u*CS.next_block++;
        e->stamp=SS.epoch;e->seen=0;e->flags=1u;
        for(i=0;i<4u;i++)e->camera_id[i]=0;
    } else if(e->stamp!=SS.epoch) {e->stamp=SS.epoch;e->seen=0;}
    e->seen|=seen;e->camera_id[CX.view]=camera;
    CX.buffers=buffers;CX.pool_index=(u32)D_8007BEDA;CX.gfx_base=base;CX.gfx_begin=current;
    CX.group_id=e->idbase+CX.view*128u+CX.phase*16u+1u;CX.paid=1u;CS.epoch=SS.epoch;
    /* 24 prefix+112 remaining original+8 paidclose=144 physical bytes. */
    c->main_extra+=32u;c->main_begin=current+24u;
    cam_enable_words(current);cam_group_words(current+8u,CX.group_id,0x02014AA9u);
    *(volatile u32*)cursor_slot=current+24u;CS.last_reason=0;return 0;
}
CPU625 u32 tooie_cpu625_end(u32 self) {
    u32 current,base,pool;
    if(!CX.owner)return CS.tripwires?1u:0u;
    if(!CX.paid){c625_clear();return CS.tripwires?1u:0u;}
    pool=CX.pool_index;base=CX.gfx_base;
    current=CX.buffers==0x80127658u ? *(volatile u32*)CX.buffers : 0u;
    if(self!=CX.owner || CX.epoch!=SS.epoch || CX.epoch!=CAM_STATE.control.epoch || pool>=2u || D_8007BEDA!=(s32)pool ||
        D_8007BCD0[pool]!=base || base!=(u32)&tooie_camera_main[pool][0] ||
        CX.buffers!=0x80127658u || !silo_range(base,TOOIE_CAMERA_M_BYTES,7u) ||
        CX.gfx_begin<base || CX.gfx_begin>base+TOOIE_CAMERA_M_BYTES-144u ||
        current<CX.gfx_begin+24u || current>base+TOOIE_CAMERA_M_BYTES-8u || (current&7u))
        return c625_trip(14u);
    if(CX.view!=(u32)D_80127674 || CX.phase!=(u32)(s32)D_80132E9C)silo_count(&CS.drifts);
    /* Public pop word0=64|POP opcode, word1=count1/MODELVIEW. Exact macro fixture binds it. */
    {volatile u32* words=(volatile u32*)current;words[0]=0x6400000Du;words[1]=1u;}
    *(volatile u32*)CX.buffers=current+8u;
    if(CAM_STATE.control.main_begin<current+8u)CAM_STATE.control.main_begin=current+8u;
    c625_clear();return CS.tripwires?1u:0u;
}
CPU625 u32 tooie_cpu625_retire(u32 self,u32 requested,u32 reason) {
    u32 i,backpack,old=0;
    if(reason==1u) {
        if(!silo_range(self,24u,3u))return 0;
        backpack=*(volatile u32*)(self+20u);
        if(!silo_range(backpack,24u,3u))return 0;
        old=*(volatile u32*)(backpack+12u);
        if(old!=3u && requested!=3u)return 0;
    }
    for(i=0;i<8u;i++)if(CS.owner[i].owner==self)CS.owner[i].idbase=0;
    /* Retained identity may retire during a paid bracket; its saved token survives. */
    if(CX.owner==self) {silo_count(&CS.tripwires);CS.last_reason=15u;}
    return 0;
}
#endif
