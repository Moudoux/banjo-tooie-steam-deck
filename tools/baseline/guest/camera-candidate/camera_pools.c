/* Original allocator traffic is retained. Only published command-buffer ownership changes. */
#include "camera_internal_02.h"
void* heap_alloc(s32);u32 heap_free(void*);
u32 func_8001A570(void);u32 func_800A9800(void);u32 func_8001A63C(void);
extern volatile u32 D_8007BCD0,D_8007BCD4,D_8007BCD8,D_8007BCDC,D_8007BCE0,D_8007BCE4;
extern volatile s16 D_8007BEDA;
extern volatile u8 D_801282E0[];
extern volatile u8 D_8011A760[],D_8011A7C0[];
s32 func_800EA05C(void);u32 func_800B5680(s32);
u32 func_800EF410(void*,void*);f32 func_800EEAA4(void*,void*);
u32 func_800EFA20(void*,void*,f32);u32 func_800EE7F8(void*,void*);
CAM_INLINE u32 cam_main_free_arg(u32 pointer,u32 slot) {
    if(pointer==(u32)&tooie_camera_main[slot][0])
        return slot?CAM_STATE.heap.main1:CAM_STATE.heap.main0;
    return pointer;
}
CAM_INLINE u32 cam_redirected_free_arg(u32 pointer,u32 slot) {
    if(pointer==(u32)&tooie_camera_redirected[slot][0])
        return slot?CAM_STATE.heap.redirected1:CAM_STATE.heap.redirected0;
    return pointer;
}
CAM_PATCH u32 func_8001A3A0(void) {
    if(!D_8007BCD0){
        CAM_STATE.heap.main0=(u32)heap_alloc(0x8FC0);D_8007BCD0=(u32)&tooie_camera_main[0][0];
        CAM_STATE.heap.main1=(u32)heap_alloc(0x8FC0);D_8007BCD4=(u32)&tooie_camera_main[1][0];
        D_8007BCD8=(u32)heap_alloc(0x12C00);D_8007BCDC=(u32)heap_alloc(0x12C00);
        D_8007BCE0=(u32)heap_alloc(0x1AE0);D_8007BCE4=(u32)heap_alloc(0x1AE0);
        func_8001A570();
    }
    D_8007BEDA=0;return func_800A9800();
}
CAM_PATCH u32 func_8001A324(void) {
    u32 pointer=D_8007BCD0;
    if(pointer){
        heap_free((void*)cam_main_free_arg(pointer,0));
        heap_free((void*)cam_main_free_arg(D_8007BCD4,1));
        heap_free((void*)D_8007BCD8);heap_free((void*)D_8007BCDC);
        heap_free((void*)D_8007BCE0);heap_free((void*)D_8007BCE4);
        D_8007BCD0=0;CAM_STATE.heap.main0=0;CAM_STATE.heap.main1=0;
        CAM_STATE.control.active=0;
    }
    return func_8001A63C();
}
CAM_PATCH u32 func_800BC690(void) {
    s32 map=func_800EA05C();volatile u8* config=D_8011A760;
    volatile u8* s=D_801282E0;f32 vector[3],x,y,z,negative_y,negative_z,twice_negative_y;
    volatile f32 twice_negative_z,twice_negative_x,zero;
    while(config!=D_8011A7C0){if(map==*(volatile s16*)config)break;config+=0x20;}
    if(config==D_8011A7C0)return (u32)map;
    if(*(volatile s16*)(config+6))func_800B5680((s32)*(volatile f32*)(config+0xC));
    {s16 first=*(volatile s16*)(config+2),second=*(volatile s16*)(config+4);
     s[0xD]=1;s[0x10]=1;s[0xE]=0;s[0xF]=0;
     *(volatile s16*)(s+0x12)=first;s[0x11]=(u8)second;}
    CAM_STATE.heap.redirected0=(u32)heap_alloc(0x5DC0);*(volatile u32*)(s+0)=(u32)&tooie_camera_redirected[0][0];
    CAM_STATE.heap.redirected1=(u32)heap_alloc(0x5DC0);*(volatile u32*)(s+4)=(u32)&tooie_camera_redirected[1][0];
    func_800EF410(vector,(void*)(config+0x14));x=vector[0];y=vector[1];z=vector[2];
    negative_y=-y;negative_z=-z;twice_negative_y=negative_y+negative_y;
    *(volatile f32*)(s+0x14)=1.f-x*(x+x);
    *(volatile f32*)(s+0x24)=x*twice_negative_y;
    twice_negative_z=negative_z+negative_z;twice_negative_x=-x;
    *(volatile f32*)(s+0x34)=x*twice_negative_z;
    twice_negative_x=twice_negative_x+twice_negative_x;
    *(volatile f32*)(s+0x18)=y*twice_negative_x;
    *(volatile f32*)(s+0x28)=1.f-y*(y+y);
    *(volatile f32*)(s+0x38)=y*twice_negative_z;
    *(volatile f32*)(s+0x1C)=z*twice_negative_x;
    *(volatile f32*)(s+0x2C)=z*twice_negative_y;
    *(volatile f32*)(s+0x40)=0.f;zero=*(volatile f32*)(s+0x40);
    *(volatile f32*)(s+0x3C)=1.f-z*(z+z);
    *(volatile f32*)(s+0x20)=zero;*(volatile f32*)(s+0x30)=zero;
    {f32 distance=func_800EEAA4((void*)(config+8),vector);
     func_800EFA20((void*)(s+0x44),vector,distance+distance);}
    *(volatile f32*)(s+0x50)=1.f;guMtxF2L((f32(*)[4])(s+0x14),(Mtx*)(s+0x58));
    func_800EE7F8((void*)(s+0xA4),vector);return func_800EE7F8((void*)(s+0x98),(void*)(config+8));
}
CAM_PATCH u32 func_800BC8A0(void) {
    volatile u8* s=D_801282E0;
    if(s[0xD]){
        heap_free((void*)cam_redirected_free_arg(*(volatile u32*)(s+0),0));
        heap_free((void*)cam_redirected_free_arg(*(volatile u32*)(s+4),1));
        *(volatile u32*)(s+4)=0;*(volatile u32*)(s+0)=0;s[0xD]=0;
        CAM_STATE.heap.redirected0=0;CAM_STATE.heap.redirected1=0;
        CAM_STATE.fill[0].valid=0;CAM_STATE.fill[1].valid=0;CAM_STATE.token.paid=0;
    }
    return (u32)s;
}
