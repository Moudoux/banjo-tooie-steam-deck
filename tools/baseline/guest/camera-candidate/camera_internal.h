#ifndef TOOIE_CAMERA_INTERNAL_H
#define TOOIE_CAMERA_INTERNAL_H
#define TOOIE_CAMERA_GUEST 1
#include "camera_identity.h"
#define CAM_INLINE static __attribute__((always_inline)) inline
#define CAM_PATCH __attribute__((section(".recomp_patch"),noinline))
#define CAM_STATE tooie_camera_state
CAM_INLINE struct TooieCameraRecord* cam_find(u32 pointer) {
    u32 i;
    for(i=0;i<TOOIE_CAMERA_RECORDS;i++)
        if(CAM_STATE.camera[i].valid && CAM_STATE.camera[i].pointer==pointer)
            return &CAM_STATE.camera[i];
    return 0;
}
CAM_INLINE void cam_zero_record(struct TooieCameraRecord* record) {
    volatile u32* p=(volatile u32*)record;
    u32 i;for(i=0;i<20u;i++)p[i]=0;
}
CAM_INLINE void cam_birth(u32 pointer) {
    struct TooieCameraRecord* record=cam_find(pointer);
    u32 i,birth=CAM_STATE.control.next_birth;
    if(record)record->valid=0;
    if(!pointer || pointer<0x80000000u || pointer>0x807FFE98u || (pointer&3u))return;
    if(!birth || birth==0xFFFFFFFFu){CAM_STATE.diagnostics.overflow++;return;}
    for(i=0;i<TOOIE_CAMERA_RECORDS;i++)if(!CAM_STATE.camera[i].valid)break;
    if(i==TOOIE_CAMERA_RECORDS){CAM_STATE.diagnostics.overflow++;return;}
    record=&CAM_STATE.camera[i];cam_zero_record(record);
    CAM_STATE.control.next_birth=birth+1u;
    record->pointer=pointer;record->birth=birth;record->valid=1;
    CAM_STATE.diagnostics.births++;
}
CAM_INLINE void cam_retire(u32 pointer) {
    struct TooieCameraRecord* record=cam_find(pointer);
    if(record)record->valid=0;
}
CAM_INLINE void cam_move(u32 old_pointer,u32 new_pointer) {
    struct TooieCameraRecord* old=cam_find(old_pointer);
    struct TooieCameraRecord* other;
    if(!old)return;
    if(!new_pointer || new_pointer<0x80000000u || new_pointer>0x807FFE98u || (new_pointer&3u)){
        old->valid=0;return;
    }
    other=cam_find(new_pointer);
    if(other && other!=old)other->valid=0;
    old->pointer=new_pointer;
}
#endif
