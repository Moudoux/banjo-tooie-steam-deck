#ifndef TOOIE_PRE_BACKGROUND_H
#define TOOIE_PRE_BACKGROUND_H
#include "camera_internal_02.h"
_Static_assert(!(TOOIE_CAMERA_PRE_WORLD&0x80000000u),"saved PRE role top bit must be clear");
#define CAM_POLICY_PRE_SIMPLE 0x0201152Au
#define CAM_POLICY_PRE_MODEL 0x02031529u
#define CAM_NEAR_WORD 0xECu /* original CA6C0/E3C8C camera lens words */
#define CAM_FAR_WORD 0xF0u
#define PRE_HEADER_FLAGS 0xAu
#define PRE_HEADER_BONES 0x18u
#define PRE_HEADER_DEFORM 0x28u
extern volatile u32 D_80127658,D_80128780[2];
struct TooiePreView { u32 birth,revision,scale,far_word,id,epoch; };
struct TooiePrePending { u32 graphics,begin,epoch,base,depth,mask,group; };
struct TooiePreBackground { struct TooiePreView view[4]; struct TooiePrePending pending; };
extern struct TooiePreBackground tooie_pre_background;
_Static_assert(sizeof(struct TooiePreBackground)==124,"96 PRE keys plus28 admitted scope");
CAM_INLINE u32 cam_pre_model_valid(u32 model) {
    return model>=0x80000000u && model<=0x80FFFFD7u && !(model&3u) &&
        !(*(volatile u16*)(model+PRE_HEADER_FLAGS)&0x40u) &&
        !*(volatile u32*)(model+PRE_HEADER_BONES) && !*(volatile u32*)(model+PRE_HEADER_DEFORM);
}
CAM_INLINE u32 cam_pre_admit(u32 graphics,u32 cursor) {
    struct TooieCameraControl* c=&CAM_STATE.control;struct TooiePrePending* p=&tooie_pre_background.pending;u32 slot,model,mask=0,cost=0,enable;
    if(graphics!=(u32)&D_80127658 || !c->active || c->latch || c->main_reserve || D_8012D508 || p->depth)return 0;
    if(p->epoch==c->epoch && p->mask)return 0; /* Never replace an unconsumed admission. */
    for(slot=0;slot<2u;slot++){model=D_80128780[slot];if(model){if(!cam_pre_model_valid(model))return 0;mask|=1u<<slot;cost+=24u;}}
    if(!(mask&1u))return 0;enable=c->main_extra?0u:8u;
    /* Include Enable, projection, two original DA words, and IGNORE margin. No charge yet. */
    if((cursor&7u) || cursor<c->main_base || cursor<c->main_begin || cursor>c->main_end-(cost+enable+48u) || c->main_extra>TOOIE_CAMERA_EXTRA_BYTES-(cost+enable+32u)){
        CAM_STATE.diagnostics.refusals++;return 0;}
    return mask;
}
CAM_INLINE u32 cam_pre_production_id(u32 camera,u32 view,u32 scale,u32 mask) {
    struct TooieCameraControl* c=&CAM_STATE.control;struct TooieCameraRecord* record;struct TooiePreView* e;u32 previous,far_word,id;
    if(!c->active || c->latch || view>=4u || !c->epoch || c->epoch==0xFFFFFFFFu)return 0;
    e=&tooie_pre_background.view[view];previous=e->epoch;
    if(previous==c->epoch)return 0; /* Preserve the first admitted identity/token. */
    e->epoch=c->epoch;if(!mask){e->id=0;return 0;}record=cam_find(camera);
    if(!record || (scale&0x7FFFFFFFu)==0 || (scale&0x7F800000u)==0x7F800000u){e->id=0;return 0;}
    if(*(volatile u32*)(camera+CAM_NEAR_WORD)!=0x40A00000u){e->id=0;return 0;}
    far_word=*(volatile u32*)(camera+CAM_FAR_WORD);if(far_word<=0x40A00000u || far_word>=0x7F800000u){e->id=0;return 0;}
    id=e->id;
    if(!id || previous+1u!=c->epoch || e->birth!=record->birth || e->revision!=record->revision || e->scale!=scale || e->far_word!=far_word){
        id=c->next_id;if(id<TOOIE_CAMERA_ID_FIRST || id>TOOIE_CAMERA_ID_END-3u){e->id=0;CAM_STATE.diagnostics.overflow++;return 0;}
        c->next_id=id+3u;e->id=id;e->birth=record->birth;e->revision=record->revision;e->scale=scale;e->far_word=far_word;CAM_STATE.diagnostics.id_highwater=id+2u;
    }
    return id;
}
CAM_INLINE void cam_pre_commit(u32 before,u32 after,u32 mask,u32 id) {
    struct TooieCameraControl* c=&CAM_STATE.control;struct TooiePrePending* p=&tooie_pre_background.pending;u32 group,cost;
    if(!id || !mask)return;group=after-16u;cost=((mask&1u)!=0u)*24u+((mask&2u)!=0u)*24u;
    if(c->latch || D_8012D508 || (after!=before+16u && after!=before+24u) || (group&7u) || group<c->main_base || group>c->main_end-16u ||
       *(volatile u32*)group!=CAM_GROUP_OPCODE || *(volatile u32*)(group+4u)!=id ||
       *(volatile u32*)(group+8u)!=CAM_POLICY_PRE_SIMPLE || *(volatile u32*)(group+12u) ||
       after>c->main_end-(cost+32u) || c->main_extra>TOOIE_CAMERA_EXTRA_BYTES-(cost+16u)){c->latch=1;return;}
    c->main_extra+=cost;p->epoch=c->epoch;p->base=c->main_base;p->mask=mask;p->group=group;
}
#endif
