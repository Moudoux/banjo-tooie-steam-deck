/* Original ncpod operations plus request/apply metadata. No driver behavior changes. */
#include "camera_request_internal.h"
u32 _ncpoddialog_entrypoint_3(s32);u32 _ncpoducam_entrypoint_6(s32);u32 _ncpodfixpos_entrypoint_1(s32);
u32 _ncpoddialog_entrypoint_2(void);u32 _ncpoducam_entrypoint_0(void);u32 _ncpodfixpos_entrypoint_0(void);
void* ncpod_entrypoint_2(s32);
u32 _ncpoddialog_entrypoint_1(s32,void*);u32 _ncpoducam_entrypoint_7(s32,void*);u32 _ncpodfixpos_entrypoint_6(s32,void*);
void* func_800A940C(s32);u32 func_8010FE14(void*,void*,void*);
u32 func_800C5148(void*,f32);u32 func_800C51A4(void*,f32);
CAM_PATCH u32 ncpod_entrypoint_12(void* state,s32 mode) {
    volatile u8* s=(volatile u8*)state;u32 old=s[0x41],result;
    if(old==1u){_ncpoddialog_entrypoint_3(*(volatile s16*)(s+0));*(volatile s16*)(s+0)=0;}
    else if(old==2u){_ncpoducam_entrypoint_6(*(volatile s16*)(s+2));*(volatile s16*)(s+2)=0;}
    else if(old==3u){_ncpodfixpos_entrypoint_1(*(volatile s16*)(s+4));*(volatile s16*)(s+4)=0;}
    result=(u32)mode&255u;s[0x41]=(u8)mode;
    /* @recomp cam_request publishes request state after each original mode constructor
       (1-3) and cancels it for other modes. */
    if(result==1u){result=_ncpoddialog_entrypoint_2();*(volatile s16*)(s+0)=(s16)result;cam_request((u32)state,1);}
    else if(result==2u){result=_ncpoducam_entrypoint_0();*(volatile s16*)(s+2)=(s16)result;cam_request((u32)state,1);}
    else if(result==3u){result=_ncpodfixpos_entrypoint_0();*(volatile s16*)(s+4)=(s16)result;cam_request((u32)state,1);}
    else cam_request((u32)state,0);
    return result;
}
CAM_PATCH u32 ncpod_entrypoint_14(s32 handle,void* wrapper) {
    volatile u8* s=(volatile u8*)ncpod_entrypoint_2((s16)handle);
    /* @recomp Join the wrapper sidecar and snapshot its request identity. */
    struct TooieCameraWrapper* w=cam_join((u32)s,(u32)wrapper);
    u32 serial=0,session=0,birth=0,result=(u32)s,action=2,mode;
    if(w){serial=w->pending_serial;session=w->session;birth=w->camera_birth;}
    mode=s[0x41];
    if(mode==1u){result=_ncpoddialog_entrypoint_1(*(volatile s16*)(s+0),wrapper);action=result;}
    else if(mode==2u){result=_ncpoducam_entrypoint_7(*(volatile s16*)(s+2),wrapper);action=result;}
    else if(mode==3u){result=_ncpodfixpos_entrypoint_6(*(volatile s16*)(s+4),wrapper);action=result;}
    if(action!=4u && s[0x40]){
        void* auxiliary=func_800A940C(*(volatile s32*)(s+0x38));
        func_8010FE14(wrapper,(void*)(s+0x18),(void*)(s+0x24));
        /* @recomp Publish the applied request after the original transform application. */
        cam_apply((u32)s,(u32)wrapper,serial,session,birth);
        func_800C5148(auxiliary,*(volatile f32*)(s+0x30));
        if(*(volatile f32*)(s+0x30)==40.f && *(volatile f32*)(s+0x34)!=1.f)
            result=func_800C51A4(auxiliary,*(volatile f32*)(s+0x34));
        else result=func_800C51A4(auxiliary,1.f);
    }
    return result;
}
