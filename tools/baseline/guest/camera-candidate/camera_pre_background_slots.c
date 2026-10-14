/* Original main graphics cursor is a named global, never slot s0. */
#include "camera_projection_internal_04.h"
#include "camera_pre_background.h"
#define PRE_HELPER __attribute__((section(".tooie_pre_background_helpers"),noinline))
PRE_HELPER void tooie_pre_background_reset(void) {
    u32 i;struct TooiePrePending* p=&tooie_pre_background.pending;
    for(i=0;i<4u;i++){tooie_pre_background.view[i].id=0;tooie_pre_background.view[i].epoch=0;}
    if(p->depth)CAM_STATE.control.latch=1;else {p->mask=0;p->group=0;}
}
PRE_HELPER void tooie_pre_background_begin(u32 slot,u32 model) {
    struct TooieCameraControl* c=&CAM_STATE.control;struct TooieCameraProduction* p=&CAM_STATE.saved;
    struct TooiePrePending* pending=&tooie_pre_background.pending;u32 cursor,group=pending->group;
    if(pending->depth){if(pending->depth!=0xFFFFFFFFu)pending->depth++;c->latch=1;return;}
    if(!p->id || p->policy!=CAM_POLICY_PRE_SIMPLE || !pending->mask || !group)return; /* Projection refused: both original. */
    if(!c->active || c->latch || D_8012D508 || slot>=2u || !(pending->mask&(1u<<slot)) ||
       pending->epoch!=c->epoch || pending->base!=c->main_base || p->role!=TOOIE_CAMERA_PRE_WORLD ||
       !p->valid || p->epoch!=c->epoch || p->view>=4u || !p->id || tooie_pre_background.view[p->view].id!=p->id ||
       p->policy!=CAM_POLICY_PRE_SIMPLE || !cam_pre_model_valid(model) || model!=D_80128780[slot] ||
       (group&7u) || group<c->main_base || group>c->main_end-16u ||
       *(volatile u32*)group!=CAM_GROUP_OPCODE || *(volatile u32*)(group+4u)!=p->id ||
       *(volatile u32*)(group+8u)!=CAM_POLICY_PRE_SIMPLE || *(volatile u32*)(group+12u)){c->latch=1;return;}
    cursor=D_80127658;
    if((cursor&7u) || cursor<c->main_begin || cursor<c->main_base || cursor<group+16u || cursor>c->main_end-24u){c->latch=1;return;}
    pending->mask&=~(1u<<slot);pending->graphics=(u32)&D_80127658;pending->begin=cursor;pending->depth=1;
    cam_group_words(cursor,p->id+1u+slot,CAM_POLICY_PRE_MODEL);D_80127658=cursor+16u;c->main_begin=cursor+16u;
}
PRE_HELPER void tooie_pre_background_end(void) {
    struct TooieCameraControl* c=&CAM_STATE.control;struct TooiePrePending* p=&tooie_pre_background.pending;u32 cursor=D_80127658;
    if(!p->depth)return;if(p->depth>1u){p->depth--;return;}
    /* Prepaid POP remains owned after an unrelated latch. Failure contains a malformed frame. */
    if(p->graphics!=(u32)&D_80127658 || c->epoch!=p->epoch || c->main_base!=p->base || D_8012D508 ||
       (cursor&7u) || cursor<p->begin+16u || cursor<c->main_begin || cursor>c->main_end-8u)c->latch=1;
    else {*(volatile u32*)cursor=0x6400000Du;*(volatile u32*)(cursor+4u)=1u;D_80127658=cursor+8u;c->main_begin=cursor+8u;}
    p->graphics=0;p->depth=0;
}
