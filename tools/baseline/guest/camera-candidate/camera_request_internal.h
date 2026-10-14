#ifndef TOOIE_CAMERA_REQUEST_INTERNAL_H
#define TOOIE_CAMERA_REQUEST_INTERNAL_H
#include "camera_internal_02.h"
extern volatile u32 D_801276D0[];
extern volatile u32 D_8007BF10[];
CAM_INLINE u32 cam_guest_span(u32 p,u32 size) {
    return p>=0x80000000u && p<=0x80800000u-size && !(p&3u);
}
CAM_INLINE struct TooieCameraWrapper* cam_join(u32 state,u32 expected_wrapper) {
    u32 owner,wp,cp; s32 handle;struct TooieCameraWrapper* w;struct TooieCameraRecord* c;
    if(!cam_guest_span(state,0x48u))return 0;
    owner=*(volatile u32*)(state+0x38u);if(owner>=8u)return 0;
    wp=D_801276D0[0x58u/4u+owner];
    if(expected_wrapper && expected_wrapper!=wp)return 0;
    if(!cam_guest_span(wp,0x40u))return 0;
    w=cam_wrapper(wp);if(!w || w->owner!=owner || w->mode!=8u)return 0;
    if(*(volatile u32*)(wp+0x1Cu)!=owner || *(volatile s16*)(wp+0x16u)!=8)return 0;
    handle=*(volatile s16*)(wp+0x14u);if(handle<=0 || handle>=250 || w->handle!=(u32)handle)return 0;
    if(D_8007BF10[(u32)handle*2u]!=state)return 0;
    cp=*(volatile u32*)wp;
    if(cp!=D_801276D0[0x38u/4u+owner])return 0;
    c=cam_find(cp);if(!c || c->birth!=w->camera_birth)return 0;
    return w;
}
CAM_INLINE void cam_request(u32 state,u32 supported) {
    struct TooieCameraWrapper* w;u32 i,serial;
    for(i=0;i<8;i++)if(CAM_STATE.wrapper[i].valid &&
      (CAM_STATE.wrapper[i].state==state || CAM_STATE.wrapper[i].pending_state==state))
        cam_pending_clear(&CAM_STATE.wrapper[i]);
    w=cam_join(state,0);if(!w)return;
    w->state=state;cam_pending_clear(w);if(!supported)return;
    serial=CAM_STATE.control.next_serial;
    if(!serial || serial==0xFFFFFFFFu){CAM_STATE.diagnostics.overflow++;return;}
    CAM_STATE.control.next_serial=serial+1u;
    w->pending_serial=serial;w->pending_state=state;
    w->pending_birth=w->camera_birth;w->pending_session=w->session;
}
CAM_INLINE void cam_apply(u32 state,u32 wrapper,u32 serial,u32 session,u32 birth) {
    struct TooieCameraWrapper* w=cam_join(state,wrapper);
    if(!w || !serial || w->session!=session || w->camera_birth!=birth ||
       w->pending_serial!=serial || w->pending_state!=state ||
       w->pending_session!=session || w->pending_birth!=birth)return;
    cam_pending_clear(w);cam_revision(*(volatile u32*)wrapper);
}
#endif
