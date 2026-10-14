#ifndef TOOIE_BASELINE_CAMERA_IDENTITY_H
#define TOOIE_BASELINE_CAMERA_IDENTITY_H
#ifdef TOOIE_CAMERA_GUEST
#include "common.h"
typedef u32 TooieCameraWord;
#else
#include <stdint.h>
#include <stddef.h>
typedef uint32_t TooieCameraWord;
#endif
#define TOOIE_CAMERA_MAGIC 0x43414D32u
#define TOOIE_CAMERA_LAYOUT 2u
#define TOOIE_CAMERA_STATE_BYTES 1380u
#define TOOIE_CAMERA_RECORDS 8u
#define TOOIE_CAMERA_ID_FIRST 0xA0000000u
#define TOOIE_CAMERA_ID_END 0xB0000000u
#define TOOIE_CAMERA_M_BYTES 0xA8C0u
#define TOOIE_CAMERA_R_BYTES 0x76C0u
#define TOOIE_CAMERA_EXTRA_BYTES 6400u
#define TOOIE_CAMERA_POOL_ALIGN 16u
#define TOOIE_CAMERA_HEAP_M_BYTES 0x8FC0u
#define TOOIE_CAMERA_HEAP_R_BYTES 0x5DC0u
#define TOOIE_CAMERA_WORLD_INITIAL 1u
#define TOOIE_CAMERA_PRE_WORLD 2u
struct TooieCameraRecord {
    TooieCameraWord pointer,birth,revision,valid;
    TooieCameraWord view_id[4],view_scale[4],seen_epoch[4],seen_role[4];
};
struct TooieCameraWrapper {
    TooieCameraWord pointer,owner,handle,state,camera_birth,session;
    TooieCameraWord pending_serial,pending_state,pending_birth,pending_session,valid,mode;
};
struct TooieCameraControl {
    TooieCameraWord magic,layout,epoch,active;
    TooieCameraWord main_base,main_begin,main_end,main_extra;
    TooieCameraWord main_reserve,latch,next_id,next_birth;
    TooieCameraWord next_session,next_serial,backup_depth,role;
};
struct TooieCameraProduction {
    TooieCameraWord matrix,camera,birth,view,revision,scale,id,role;
    TooieCameraWord epoch,valid,policy,source,near_word,far_word,reserved0,reserved1;
};
struct TooieCameraBackup {
    TooieCameraWord source,destination,birth,revision;
    TooieCameraWord epoch,valid,depth,reserved;
};
struct TooieCameraFill {
    TooieCameraWord base,end,begin,extra,reserve,valid;
    TooieCameraWord epoch,main_saved,slot,latched,closed,reserved;
};
struct TooieCameraDiagnostics {
    TooieCameraWord calls,productions,refusals,first_reason;
    TooieCameraWord raw0,raw1,raw2,raw3;
    TooieCameraWord tokenless,mismatch,reentry,overflow;
    TooieCameraWord births,revisions,id_highwater,pool_highwater;
};
struct TooieCameraHeapPlaceholders {
    TooieCameraWord main0,main1,redirected0,redirected1;
};
struct TooieCameraToken {
    TooieCameraWord slot_base,expected_main,epoch,paid;
};
struct TooieCameraState {
    struct TooieCameraRecord camera[8];
    struct TooieCameraWrapper wrapper[8];
    struct TooieCameraControl control;
    struct TooieCameraProduction saved;
    struct TooieCameraBackup backup;
    struct TooieCameraFill fill[2];
    struct TooieCameraDiagnostics diagnostics;
    struct TooieCameraHeapPlaceholders heap;
    struct TooieCameraToken token;
    TooieCameraWord owner_frame;
};
extern struct TooieCameraState tooie_camera_state;
extern unsigned char tooie_camera_main[2][TOOIE_CAMERA_M_BYTES];
extern unsigned char tooie_camera_redirected[2][TOOIE_CAMERA_R_BYTES];
#ifndef TOOIE_CAMERA_GUEST
#if defined(__cplusplus)
static_assert(sizeof(TooieCameraRecord)==80,"camera record ABI");
static_assert(sizeof(TooieCameraWrapper)==48,"wrapper record ABI");
static_assert(sizeof(TooieCameraState)==TOOIE_CAMERA_STATE_BYTES,"camera state ABI");
static_assert(offsetof(TooieCameraState,control)==1024,"control ABI");
static_assert(offsetof(TooieCameraState,saved)==1088,"saved ABI");
static_assert(offsetof(TooieCameraState,backup)==1152,"backup ABI");
static_assert(offsetof(TooieCameraState,fill)==1184,"fills ABI");
static_assert(offsetof(TooieCameraState,diagnostics)==1280,"diagnostics ABI");
static_assert(offsetof(TooieCameraState,heap)==1344,"placeholder ABI");
static_assert(offsetof(TooieCameraState,token)==1360,"token ABI");
static_assert(offsetof(TooieCameraState,owner_frame)==1376,"construction owner ABI");
static_assert(alignof(TooieCameraState)==4,"camera state alignment ABI");
#endif
#endif
#endif
