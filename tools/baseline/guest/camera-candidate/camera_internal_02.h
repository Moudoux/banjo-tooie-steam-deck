#ifndef TOOIE_CAMERA_INTERNAL_02_H
#define TOOIE_CAMERA_INTERNAL_02_H
#include "camera_internal.h"
CAM_INLINE void cam_revision(u32 pointer) {
    struct TooieCameraRecord* c=cam_find(pointer);u32 i;
    if(!c)return;
    if(c->revision==0xFFFFFFFEu){c->valid=0;CAM_STATE.diagnostics.overflow++;return;}
    c->revision++;
    for(i=0;i<4;i++){c->view_id[i]=0;c->seen_epoch[i]=0;c->seen_role[i]=0;}
    CAM_STATE.diagnostics.revisions++;
}
CAM_INLINE void cam_copy_revision(u32 destination,u32 source) {
    struct TooieCameraBackup* b=&CAM_STATE.backup;
    if(CAM_STATE.control.backup_depth==1u && b->valid &&
       ((destination==b->destination && source==b->source) ||
        (destination==b->source && source==b->destination)))return;
    cam_revision(destination);
}
CAM_INLINE struct TooieCameraWrapper* cam_wrapper(u32 pointer) {
    u32 i;for(i=0;i<8;i++)if(CAM_STATE.wrapper[i].valid && CAM_STATE.wrapper[i].pointer==pointer)
        return &CAM_STATE.wrapper[i];
    return 0;
}
CAM_INLINE void cam_pending_clear(struct TooieCameraWrapper* w) {
    w->pending_serial=0;w->pending_state=0;w->pending_birth=0;w->pending_session=0;
}
CAM_INLINE void cam_wrapper_birth(u32 pointer,u32 camera,u32 owner) {
    struct TooieCameraWrapper* w=cam_wrapper(pointer);
    struct TooieCameraRecord* c=cam_find(camera);u32 i,stamp=CAM_STATE.control.next_session;
    if(w)w->valid=0;
    if(!pointer || pointer<0x80000000u || pointer>0x807FFFC0u || (pointer&3u) || !c || owner>=8u)return;
    if(!stamp || stamp==0xFFFFFFFFu){CAM_STATE.diagnostics.overflow++;return;}
    for(i=0;i<8;i++)if(!CAM_STATE.wrapper[i].valid)break;
    if(i==8){CAM_STATE.diagnostics.overflow++;return;}
    w=&CAM_STATE.wrapper[i];{volatile u32* p=(volatile u32*)w;u32 n;for(n=0;n<12;n++)p[n]=0;}
    w->pointer=pointer;w->owner=owner;w->camera_birth=c->birth;w->session=stamp;
    w->mode=0xFFFFFFFFu; /* one factory-initialization transition, then ordinary modes */
    CAM_STATE.control.next_session=stamp+1u;w->valid=1;
}
CAM_INLINE void cam_wrapper_mode(u32 pointer,u32 mode) {
    struct TooieCameraWrapper* w=cam_wrapper(pointer);u32 stamp;
    if(!w)return;
    cam_pending_clear(w);w->state=0;w->handle=0;
    if(w->mode==0xFFFFFFFFu && mode==1u){w->mode=1u;return;}
    stamp=CAM_STATE.control.next_session;
    if(!stamp || stamp==0xFFFFFFFFu){w->valid=0;CAM_STATE.diagnostics.overflow++;return;}
    w->session=stamp;CAM_STATE.control.next_session=stamp+1u;w->mode=mode;
}
CAM_INLINE void cam_wrapper_handle(u32 pointer,s32 handle) {
    struct TooieCameraWrapper* w=cam_wrapper(pointer);
    if(w)w->handle=(u32)handle;
}
CAM_INLINE void cam_wrapper_retire(u32 pointer) {
    struct TooieCameraWrapper* w=cam_wrapper(pointer);if(w){cam_pending_clear(w);w->valid=0;}
}
CAM_INLINE void cam_wrapper_move(u32 old_pointer,u32 new_pointer,u32 camera) {
    struct TooieCameraWrapper* w=cam_wrapper(old_pointer);struct TooieCameraRecord* c=cam_find(camera);
    struct TooieCameraWrapper* collision;
    if(!w)return;
    if(new_pointer<0x80000000u || new_pointer>0x807FFFC0u || (new_pointer&3u) || !c || c->birth!=w->camera_birth){w->valid=0;return;}
    collision=cam_wrapper(new_pointer);if(collision && collision!=w)collision->valid=0;
    w->pointer=new_pointer;
}
#endif
