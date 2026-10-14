#ifndef TOOIE_CAMERA_PROJECTION_INTERNAL_03_H
#define TOOIE_CAMERA_PROJECTION_INTERNAL_03_H
#include "camera_internal_02.h"
extern volatile u32 D_8012D508;
extern volatile s32 D_80127674;
#define TOOIE_CAMERA_FIXED_LIST_SCOPE 0x80000000u
#define CAM_POLICY_WORLD 0x02011556u
#define CAM_POLICY_AUTO 0x02031556u
#define CAM_POLICY_IGNORE 0x00000002u
#define CAM_GROUP_OPCODE 0x6400000Cu
CAM_INLINE u32 cam_float_word(f32 value){union {f32 f;u32 u;} bits;bits.f=value;return bits.u;}
CAM_INLINE void cam_group_words(u32 cursor,u32 id,u32 flags){
    volatile u32* p=(volatile u32*)cursor;
    p[0]=CAM_GROUP_OPCODE;p[1]=id;p[2]=flags;p[3]=0;
}
CAM_INLINE void cam_enable_words(u32 cursor);
/* Begin is state-only; first ordinary MAIN policy materializes Enable. */
CAM_INLINE u32 cam_projection_policy(u32 cursor,u32 id,u32 flags){
    struct TooieCameraControl* c=&CAM_STATE.control;u32* extra;u32* reserve;u32* highwater;u32 end;
    struct TooieCameraFill* fill=0;
    if(!c->active)return cursor;
    if(D_8012D508){
        u32 i;for(i=0;i<2;i++)if(CAM_STATE.fill[i].valid && CAM_STATE.fill[i].epoch==c->epoch &&
            cursor>=CAM_STATE.fill[i].base && cursor<=CAM_STATE.fill[i].end){fill=&CAM_STATE.fill[i];break;}
        if(!fill){c->latch=1;return cursor;}
        extra=&fill->extra;reserve=&fill->reserve;highwater=&fill->begin;end=fill->end;
        if(!CAM_STATE.token.paid || CAM_STATE.token.epoch!=c->epoch ||
            CAM_STATE.token.slot_base!=fill->base)id=0,flags=CAM_POLICY_IGNORE;
    }else{
        if(cursor<c->main_base || cursor>c->main_end){c->latch=1;return cursor;}
        extra=&c->main_extra;reserve=&c->main_reserve;highwater=&c->main_begin;end=c->main_end;
    }
    if(!*extra){
        if(fill || (cursor&7u) || cursor<*highwater || cursor>end-24u){
            c->latch=1;CAM_STATE.diagnostics.refusals++;
            if(!CAM_STATE.diagnostics.first_reason)CAM_STATE.diagnostics.first_reason=0x102u;
            return cursor;
        }
        cam_enable_words(cursor);cursor+=8u;*extra=8u;*highwater=cursor;
    }
    if((cursor&7u) || cursor<*highwater){c->latch=1;if(fill)fill->latched=1;return cursor;}
    if(c->latch || (fill && fill->latched) || *extra>TOOIE_CAMERA_EXTRA_BYTES-32u || cursor>end-32u){
        c->latch=1;if(fill)fill->latched=1;
        if(!*reserve && *extra<=TOOIE_CAMERA_EXTRA_BYTES-16u && cursor<=end-16u){
            cam_group_words(cursor,0,CAM_POLICY_IGNORE);cursor+=16u;*extra+=16u;*reserve=1;
        }
        *highwater=cursor;return cursor;
    }
    cam_group_words(cursor,id,flags);cursor+=16u;*extra+=16u;*highwater=cursor;return cursor;
}
CAM_INLINE u32 cam_production_id(u32 camera,u32 view,u32 scale,u32 role){
    struct TooieCameraControl* c=&CAM_STATE.control;struct TooieCameraRecord* record=cam_find(camera);u32 id;
    if(!c->active || c->latch || !record || view>=4u || role!=TOOIE_CAMERA_WORLD_INITIAL)return 0;
    if((scale&0x7FFFFFFFu)==0 || (scale&0x7F800000u)==0x7F800000u)return 0;
    if(record->seen_epoch[view]==c->epoch && (record->seen_role[view]&1u))return 0;
    id=record->view_id[view];
    if(!id || record->view_scale[view]!=scale){
        id=c->next_id;if(id<TOOIE_CAMERA_ID_FIRST || id>=TOOIE_CAMERA_ID_END){CAM_STATE.diagnostics.overflow++;return 0;}
        c->next_id=id+1u;record->view_id[view]=id;record->view_scale[view]=scale;
        CAM_STATE.diagnostics.id_highwater=id;
    }
    record->seen_epoch[view]=c->epoch;record->seen_role[view]=1;return id;
}
CAM_INLINE u32 cam_capture_perspective(u32 matrix,u32 camera,u32 scale,u32 source){
    struct TooieCameraProduction* p=&CAM_STATE.saved;struct TooieCameraRecord* record=cam_find(camera);
    u32 view=(u32)D_80127674,role=CAM_STATE.control.role,id=cam_production_id(camera,view,scale,role);
    p->matrix=matrix;p->camera=camera;p->birth=record?record->birth:0;p->view=view;
    p->revision=record?record->revision:0;p->scale=scale;p->id=id;p->role=role&0x7FFFFFFFu;
    p->epoch=CAM_STATE.control.epoch;p->valid=1;p->policy=id?CAM_POLICY_WORLD:CAM_POLICY_IGNORE;
    p->source=source;p->near_word=0;p->far_word=0;p->reserved0=0;p->reserved1=0;
    CAM_STATE.diagnostics.productions++;return id;
}
extern volatile u8 D_801282E0[];
CAM_INLINE void cam_enable_words(u32 cursor){volatile u32* p=(volatile u32*)cursor;p[0]=0xE0525464u;p[1]=0x10000064u;}
CAM_INLINE u32 cam_precharge_replay(u32 cursor){
    struct TooieCameraControl* c=&CAM_STATE.control;u32 index,base;
    if(!c->active || !D_801282E0[0xD])return cursor;
    if(CAM_STATE.token.paid){c->latch=1;return cam_projection_policy(cursor,0,CAM_POLICY_IGNORE);}
    index=D_801282E0[0xC];
    if(index<2u){base=*(volatile u32*)(D_801282E0+index*4u);
        if(base==(u32)&tooie_camera_redirected[index][0] && !c->latch && c->main_extra!=0u &&
           !(cursor&7u) && cursor>=c->main_begin && cursor>=c->main_base &&
           cursor<=c->main_end-104u && c->main_extra<=TOOIE_CAMERA_EXTRA_BYTES-104u){
            c->main_extra+=88u;c->main_begin=cursor;
            CAM_STATE.token.slot_base=base;CAM_STATE.token.expected_main=cursor;
            CAM_STATE.token.epoch=c->epoch;CAM_STATE.token.paid=1;return cursor;
        }
    }
    c->latch=1;return cam_projection_policy(cursor,0,CAM_POLICY_IGNORE);
}
#endif
