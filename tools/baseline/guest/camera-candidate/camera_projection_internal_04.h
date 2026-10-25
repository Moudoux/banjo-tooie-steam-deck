#ifndef TOOIE_CAMERA_PROJECTION_INTERNAL_04_H
#define TOOIE_CAMERA_PROJECTION_INTERNAL_04_H
#include "camera_internal_02.h"
#include "rt64_extended_gbi.h"
extern volatile u32 D_8012D508;
extern volatile s32 D_80127674;
#define TOOIE_CAMERA_FIXED_LIST_SCOPE 0x80000000u
/* Public gEXMatrixGroup word2 fields. Policies are saved and compared as words;
 * cam_group_words emits them without unpacking them into statement-macro args. */
#define CAM_GROUP_FLAGS(mode,push,proj,pos,rot,scale,skew,persp,vert,tile,order,edit,aspect,tc,lookat) \
    (PARAM(push,1,0)|PARAM((proj)!=0,1,1)|PARAM(mode,1,2)|PARAM(pos,2,3)|PARAM(rot,2,5)| \
     PARAM(scale,2,7)|PARAM(skew,2,9)|PARAM(persp,2,11)|PARAM(vert,2,13)|PARAM(tile,2,15)| \
     PARAM(order,2,17)|PARAM(edit,1,19)|PARAM(aspect,2,20)|PARAM(tc,2,22)|PARAM(lookat,2,24))
/* @recomp BK gameplay perspective group (BanjoRecomp dbd52154 camera_transform_tagging.c 98): SIMPLE;
 * position, rotation, scale, skew, perspective and tile INTERPOLATE; vertices and texcoords SKIP. */
#define CAM_POLICY_WORLD CAM_GROUP_FLAGS(G_EX_INTERPOLATE_SIMPLE,G_EX_NOPUSH,G_MTX_PROJECTION, \
    G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE, \
    G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_INTERPOLATE,G_EX_ORDER_LINEAR, \
    G_EX_EDIT_NONE,G_EX_ASPECT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO)
#define CAM_POLICY_AUTO CAM_GROUP_FLAGS(G_EX_INTERPOLATE_DECOMPOSE,G_EX_NOPUSH,G_MTX_PROJECTION, \
    G_EX_COMPONENT_AUTO,G_EX_COMPONENT_AUTO,G_EX_COMPONENT_AUTO,G_EX_COMPONENT_AUTO, \
    G_EX_COMPONENT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO,G_EX_ORDER_AUTO, \
    G_EX_EDIT_NONE,G_EX_ASPECT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO)
#define CAM_POLICY_IGNORE CAM_GROUP_FLAGS(G_EX_INTERPOLATE_SIMPLE,G_EX_NOPUSH,G_MTX_PROJECTION, \
    G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP, \
    G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_ORDER_LINEAR, \
    G_EX_EDIT_NONE,G_EX_ASPECT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP)
#define CAM_GROUP_OPCODE 0x6400000Cu
_Static_assert(CAM_GROUP_OPCODE==(PARAM(RT64_EXTENDED_OPCODE,8,24)|PARAM(G_EX_MATRIXGROUP_V1,24,0)) &&
    CAM_POLICY_WORLD==0x02008AAAu && CAM_POLICY_AUTO==0x02031556u && CAM_POLICY_IGNORE==0x00000002u,
    "public gEXMatrixGroup words");
/* Exact f32 word for saved-key comparisons, not a numeric u32 conversion. */
CAM_INLINE u32 cam_float_word(f32 value){union {f32 f;u32 u;} bits;bits.f=value;return bits.u;}
/* Public gEXMatrixGroup: opcode/subcommand, ID, packed policy word2, reserved zero.
 * Saved-camera and PRE readers consume that same packed policy. */
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
#if TOOIE_PRE_BACKGROUND
#include "camera_pre_background.h"
#endif
/* Exact role decoding: unrelated role/fixed-list flags never become transition identity. */
CAM_INLINE u32 cam_transition_view(u32 role) {
    u32 plain=role & ~TOOIE_CAMERA_TRANSITION_ROLE_SKIP;
    return plain>=TOOIE_CAMERA_TRANSITION_ROLE_FIRST &&
        plain<TOOIE_CAMERA_TRANSITION_ROLE_FIRST+TOOIE_CAMERA_TRANSITION_VIEWS ?
        plain-TOOIE_CAMERA_TRANSITION_ROLE_FIRST : 0xFFFFFFFFu;
}
CAM_INLINE u32 cam_production_id(u32 camera,u32 view,u32 scale,u32 role,u32 pre_mask){
#if TOOIE_PRE_BACKGROUND
    if(role==TOOIE_CAMERA_PRE_WORLD)return cam_pre_production_id(camera,view,scale,pre_mask);
#endif
    struct TooieCameraControl* c=&CAM_STATE.control;struct TooieCameraRecord* record=cam_find(camera);u32 id;
    {u32 transition_view=cam_transition_view(role);
     if(transition_view<TOOIE_CAMERA_TRANSITION_VIEWS){
        if(!c->active || c->latch || D_8012D508 || !record ||
           (scale&0x7FFFFFFFu)==0 || (scale&0x7F800000u)==0x7F800000u)return 0;
        return TOOIE_CAMERA_TRANSITION_ID_FIRST+transition_view;
     }}
    if(!c->active || c->latch || !record || view>=4u || role!=TOOIE_CAMERA_WORLD_INITIAL)return 0;
    if((scale&0x7FFFFFFFu)==0 || (scale&0x7F800000u)==0x7F800000u)return 0;
    if(record->seen_epoch[view]==c->epoch && (record->seen_role[view]&1u))return 0;
    id=record->view_id[view];
    /* @recomp WORLD identity is camera birth/revision/view. Scale is lens data: original
       first-person zoom rewrites it without a camera reset, copy or forced update, so it
       no longer renews the ID. Every accepted production refreshes the published scale
       that the saved-production readers compare. PRE keeps its own scale-keyed contract. */
    if(!id){
        id=c->next_id;if(id<TOOIE_CAMERA_ID_FIRST || id>=TOOIE_CAMERA_ID_END){CAM_STATE.diagnostics.overflow++;return 0;}
        c->next_id=id+1u;record->view_id[view]=id;
        CAM_STATE.diagnostics.id_highwater=id;
    }
    record->view_scale[view]=scale;
    record->seen_epoch[view]=c->epoch;record->seen_role[view]=1;return id;
}
CAM_INLINE u32 cam_capture_perspective(u32 matrix,u32 camera,u32 scale,u32 source,u32 pre_mask){
    struct TooieCameraProduction* p=&CAM_STATE.saved;struct TooieCameraRecord* record=cam_find(camera);
    u32 view=(u32)D_80127674,role=CAM_STATE.control.role,id=cam_production_id(camera,view,scale,role,pre_mask);
    p->matrix=matrix;p->camera=camera;p->birth=record?record->birth:0;p->view=view;
    p->revision=record?record->revision:0;p->scale=scale;p->id=id;p->role=role&0x7FFFFFFFu;
    p->epoch=CAM_STATE.control.epoch;p->valid=1;p->policy=id?CAM_POLICY_WORLD:CAM_POLICY_IGNORE;
#if TOOIE_PRE_BACKGROUND
    if(id && role==TOOIE_CAMERA_PRE_WORLD)p->policy=CAM_POLICY_PRE_SIMPLE;
#endif
    if(id && cam_transition_view(role)<TOOIE_CAMERA_TRANSITION_VIEWS){
        p->view=cam_transition_view(role);
        /* Keep the same identified group on a cut; only component interpolation changes. */
        if(role&TOOIE_CAMERA_TRANSITION_ROLE_SKIP)p->policy=CAM_POLICY_IGNORE;
    }
    p->source=source;p->near_word=0;p->far_word=0;p->reserved0=0;p->reserved1=0;
    CAM_STATE.diagnostics.productions++;return id;
}
extern volatile u8 D_801282E0[];
/* Public gEXEnable hook opcode/magic, Enable operation and extended opcode. */
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
