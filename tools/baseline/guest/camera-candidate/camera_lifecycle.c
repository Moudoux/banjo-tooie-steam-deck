/* First actual lifecycle slice: original operations plus resident identity metadata. */
#include "camera_internal_02.h"
void* heap_alloc(s32);
u32 heap_free(void*);
void* defrag(void*);
u32 func_800CA440(void*);
_Static_assert(sizeof(void*)==4 && sizeof(struct TooieCameraState)==TOOIE_CAMERA_STATE_BYTES,
               "camera guest ABI");
CAM_PATCH void* func_800CA334(void) {
    void* camera=heap_alloc(0x168);
    func_800CA440(camera);
    /* @recomp Publish camera birth after the original allocation and init. */
    cam_birth((u32)camera);
    return camera;
}
CAM_PATCH u32 func_800CA364(void* camera) {
    u32 original_result=heap_free(camera);
    /* @recomp Retire the camera identity after the original free. */
    cam_retire((u32)camera);
    return original_result;
}
CAM_PATCH void* func_800CA384(void* camera) {
    void* relocated=defrag(camera);
    /* @recomp Move the camera identity after the original defrag. */
    cam_move((u32)camera,(u32)relocated);
    return relocated;
}

u32 aligned4_memcpy(void*,void*,u32);
void func_800CA5F8(void*,f32,f32,f32);void func_800CA688(void*,f32,f32,f32);
void func_800CA6C0(void*,f32,f32);void func_800CA6F0(void*,f32);
f32 func_800139F8(f32);void func_800CA218(void*,f32);void func_800CA2A8(void*,f32);u32 func_800CAF34(void*);
extern volatile f32 D_80125970;
u32 func_80114BD4(void);u32 func_800DA298(s32);u32 func_80115118(void*);u32 func_80110424(void*,s32);
u32 _ncfixposrot_entrypoint_1(u32);u32 _ncpod_entrypoint_1(s32);u32 func_80110B68(u32,s32);
u32 _ncfixposrot_entrypoint_0(void);u32 _ncpod_entrypoint_0(s32);
u32 func_801104F4(u32);u32 func_801150F8(u32);u32 func_80114C7C(u32);
u32 func_80110FCC(u32,void*);u32 func_80115324(u32,void*);u32 func_80114CD4(u32);u32 func_8010FC38(void*);
CAM_PATCH u32 func_800CA314(void* destination,void* source) {
    u32 result=aligned4_memcpy(destination,source,0x168);
    /* @recomp Copy the sidecar revision after the original 0x168-byte copy. */
    cam_copy_revision((u32)destination,(u32)source);return result;
}
CAM_PATCH u32 func_800CA440(void* camera) {
    volatile u8* c=(volatile u8*)camera;f32 half_angle,result;
    func_800CA5F8(camera,0.f,0.f,0.f);func_800CA688(camera,0.f,0.f,0.f);
    func_800CA6C0(camera,1.f,10000.f);func_800CA6F0(camera,1.f);
    *(volatile f32*)(c+0xE4)=40.f;half_angle=20.f;
    result=func_800139F8(half_angle);
    *(volatile f32*)(c+0xF4)=1.f/result;
    *(volatile f32*)(c+0xE8)=D_80125970;
    func_800CA218(camera,half_angle);func_800CA2A8(camera,half_angle);
    /* @recomp Advance the camera revision after the original reset operations. */
    {u32 original_result=func_800CAF34(camera);cam_revision((u32)camera);return original_result;}
}
CAM_PATCH u32 func_8010F9C0(void* wrapper,s32 mode) {
    volatile u8* w=(volatile u8*)wrapper;s32 old;u32 result;
    /* Preserve incoming v0 when no original call runs; only original calls replace it,
       never cam_* sidecar updates. */
    __asm__ volatile("move %0,$2" : "=r"(result) : : "memory");
    old=*(volatile s16*)(w+0x16);
    if(mode==old)return result;
    /* @recomp cam_wrapper_mode and the two mode-8 cam_wrapper_handle calls track the mode
       and ncpod handle in the wrapper sidecar; original calls alone set the result. */
    cam_wrapper_mode((u32)wrapper,(u32)(s32)(s16)mode);
    if(old==3){result=_ncfixposrot_entrypoint_1(*(volatile u32*)(w+8));*(volatile u32*)(w+8)=0;}
    else if(old==8){result=_ncpod_entrypoint_1(*(volatile s16*)(w+0x14));*(volatile s16*)(w+0x14)=0;cam_wrapper_handle((u32)wrapper,0);}
    else if(old==2){u32 child=*(volatile u32*)(w+4);if(child)result=func_80110B68(child,0);}
    *(volatile s16*)(w+0x16)=(s16)mode;
    if(mode==3){result=_ncfixposrot_entrypoint_0();*(volatile u32*)(w+8)=result;}
    else if(mode==2){u32 child=*(volatile u32*)(w+4);if(child)result=func_80110B68(child,1);else{result=func_80110424(wrapper,*(volatile s32*)(w+0x1C));*(volatile u32*)(w+4)=result;}}
    else if(mode==8){result=_ncpod_entrypoint_0(*(volatile s32*)(w+0x1C));*(volatile s16*)(w+0x14)=(s16)result;cam_wrapper_handle((u32)wrapper,(s16)result);}
    return result;
}
CAM_PATCH void* func_8010FB34(void* camera,s32 owner) {
    volatile u8* w=(volatile u8*)heap_alloc(0x40);
    w[0x18]=0;w[0x19]=0;w[0x20]=0;*(volatile u32*)(w+0)=(u32)camera;
    *(volatile u32*)(w+0x10)=func_80114BD4();
    if(func_800DA298(0x6B5))*(volatile u32*)(w+0xC)=func_80115118((void*)w);
    else *(volatile u32*)(w+0xC)=0;
    *(volatile s32*)(w+0x1C)=owner;
    *(volatile u32*)(w+4)=func_80110424((void*)w,owner);
    *(volatile u32*)(w+8)=0;
    /* @recomp Publish wrapper birth after the original owner and child fields. */
    cam_wrapper_birth((u32)w,(u32)camera,(u32)owner);
    *(volatile s16*)(w+0x14)=0;*(volatile s16*)(w+0x16)=0;
    func_8010F9C0((void*)w,1);return (void*)w;
}
CAM_PATCH u32 func_8010FBD4(void* wrapper) {
    volatile u8* w=(volatile u8*)wrapper;u32 child;
    func_8010F9C0(wrapper,1);
    child=*(volatile u32*)(w+4);if(child)func_801104F4(child);
    child=*(volatile u32*)(w+0xC);if(child)func_801150F8(child);
    func_80114C7C(*(volatile u32*)(w+0x10));
    /* @recomp Retire the wrapper identity before the original wrapper free. */
    cam_wrapper_retire((u32)wrapper);return heap_free(wrapper);
}
CAM_PATCH void* func_80110068(void* wrapper,void* camera) {
    volatile u8* w=(volatile u8*)defrag(wrapper);u32 child=*(volatile u32*)(w+4);
    *(volatile u32*)(w+0)=(u32)camera;
    if(child)*(volatile u32*)(w+4)=func_80110FCC(*(volatile u32*)(w+4),(void*)w);
    child=*(volatile u32*)(w+0xC);if(child)*(volatile u32*)(w+0xC)=func_80115324(child,(void*)w);
    child=*(volatile u32*)(w+0x10);if(child)*(volatile u32*)(w+0x10)=func_80114CD4(child);
    /* @recomp Publish the sidecar move after the original synchronous wrapper and child repairs. */
    cam_wrapper_move((u32)wrapper,(u32)w,(u32)camera);return (void*)w;
}
CAM_PATCH u32 func_8010FD08(void* wrapper) {
    volatile u8* w=(volatile u8*)wrapper;u32 result;
    w[0x19]=1;result=func_8010FC38(wrapper);
    /* @recomp Advance the camera revision between the original update and wrapper flag clear. */
    cam_revision(*(volatile u32*)(w+0));w[0x19]=0;return result;
}
