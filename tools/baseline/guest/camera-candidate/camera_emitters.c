/* Source-defined projection producers and saved-camera scope. Remaining emitters appended in a new revision. */
#include "camera_internal_02.h"
extern volatile u32 D_8012D500,D_8012D504,D_8012D554,D_8012D558;
void* func_800CA334(void);u32 func_800CA314(void*,void*);u32 func_800CA364(void*);
CAM_INLINE void cam_backup_begin(u32 source,u32 destination) {
    struct TooieCameraBackup* b=&CAM_STATE.backup;
    struct TooieCameraRecord* c=cam_find(source);u32 depth=CAM_STATE.control.backup_depth;
    if(depth){b->valid=0;if(CAM_STATE.control.active)CAM_STATE.control.latch=1;
        if(depth!=0xFFFFFFFFu)CAM_STATE.control.backup_depth=depth+1u;return;}
    CAM_STATE.control.backup_depth=1;b->source=source;b->destination=destination;
    b->epoch=CAM_STATE.control.epoch;b->depth=1;b->valid=0;
    {struct TooieCameraRecord* temporary=cam_find(destination);if(c && temporary){b->birth=c->birth;b->revision=c->revision;b->reserved=temporary->birth;b->valid=1;}}
}
CAM_INLINE void cam_backup_validate(u32 source,u32 destination) {
    struct TooieCameraBackup* b=&CAM_STATE.backup;struct TooieCameraRecord* c=cam_find(source);struct TooieCameraRecord* temporary=cam_find(destination);
    if(CAM_STATE.control.backup_depth!=1u || !b->valid || b->source!=source ||
       b->destination!=destination || !c || c->birth!=b->birth || c->revision!=b->revision ||
       !temporary || temporary->birth!=b->reserved || temporary->revision!=0u){
        b->valid=0;if(CAM_STATE.control.active)CAM_STATE.control.latch=1;
    }
}
CAM_PATCH u32 func_800E41CC(void) {
    u32 temporary=(u32)func_800CA334();D_8012D504=temporary;
    {u32 source=D_8012D500;cam_backup_begin(source,temporary);
     return func_800CA314((void*)temporary,(void*)source);}
}
CAM_PATCH u32 func_800E4208(void) {
    u32 destination=D_8012D500,source=D_8012D504,result;
    cam_backup_validate(destination,source);
    func_800CA314((void*)destination,(void*)source);
    result=func_800CA364((void*)D_8012D504);D_8012D504=0;
    CAM_STATE.backup.valid=0;if(CAM_STATE.control.backup_depth)CAM_STATE.control.backup_depth--;
    return result;
}
CAM_PATCH u32 func_800E4628(void) {
    D_8012D554=0;D_8012D558=0;CAM_STATE.saved.valid=0;
    return (u32)&D_8012D554;
}

#include "camera_projection_internal_04.h"
extern s16 widescreen_enabled; /* Original gfx.h declaration, no new variable. */
void func_800CA7E4(u32,void*);u32 func_800CB124(u32,void*);f32 func_800CA6E8(u32);
void func_800E443C(void*,void*,f32);s32 func_80018BC4(void);void mlMtxApply(Mtx*);
struct TooieCameraGraphics {volatile u32 gfx,mtx;};
_Static_assert(sizeof(struct TooieCameraGraphics)==8,"original two-pointer graphics ABI");
CAM_PATCH u32 func_800E44FC(struct TooieCameraGraphics* graphics){
    f32 projection[4][4],translation[3],scale;u32 normalization,cursor,matrix,camera,id,result;
    func_800CA7E4(D_8012D500,translation);
    normalization=func_800CB124(D_8012D500,projection);
    cursor=graphics->gfx;graphics->gfx=cursor+8u;
    *(volatile u32*)(cursor+4u)=normalization;*(volatile u32*)cursor=0xDB0E0000u;
    scale=func_800CA6E8(D_8012D500);func_800E443C(projection,translation,scale);
    if(func_80018BC4()){
        scale=func_800CA6E8(D_8012D500);scale=scale+scale;
        func_800E443C(projection,translation,scale);
    }
    mlMtxApply((Mtx*)graphics->mtx);
    matrix=graphics->mtx;camera=D_8012D500;
#if TOOIE_PRE_BACKGROUND
    {u32 before=graphics->gfx,pre_mask=CAM_STATE.control.role==TOOIE_CAMERA_PRE_WORLD?cam_pre_admit((u32)graphics,before):0;
     id=cam_capture_perspective(matrix,camera,cam_float_word(scale),0x800E4598u,pre_mask);
     cursor=cam_projection_policy(before,id,CAM_STATE.saved.policy);
     if(CAM_STATE.control.role==TOOIE_CAMERA_PRE_WORLD)cam_pre_commit(before,cursor,pre_mask,id);}
#else
    id=cam_capture_perspective(matrix,camera,cam_float_word(scale),0x800E4598u,0);
    cursor=cam_projection_policy(graphics->gfx,id,CAM_STATE.saved.policy);
#endif
    graphics->gfx=cursor+8u;*(volatile u32*)cursor=0xDA380007u;
    *(volatile u32*)(cursor+4u)=graphics->mtx+0x80000000u;
    matrix=graphics->mtx;graphics->mtx=matrix+64u;D_8012D558=matrix;
    guMtxIdent((Mtx*)graphics->mtx);
    __asm__ volatile("move %0,$2":"=r"(result)::"memory");
    cursor=graphics->gfx;graphics->gfx=cursor+8u;*(volatile u32*)cursor=0xDA380003u;
    *(volatile u32*)(cursor+4u)=graphics->mtx+0x80000000u;
    matrix=graphics->mtx;graphics->mtx=matrix+64u;return result;
}
CAM_PATCH u32 func_800E42F0(struct TooieCameraGraphics* graphics,s32 use_widescreen){
    u32 cursor=graphics->gfx,matrix=graphics->mtx;
    if(use_widescreen && widescreen_enabled)
        guOrtho((Mtx*)matrix,-810.f,810.f,-456.f,456.f,-50.f,50.f,1.f);
    else guOrtho((Mtx*)matrix,-608.f,608.f,-456.f,456.f,-50.f,50.f,1.f);
    cursor=cam_projection_policy(cursor,0xFFFFFFFFu,CAM_POLICY_AUTO);
    *(volatile u32*)cursor=0xDA380007u;*(volatile u32*)(cursor+4u)=matrix+0x80000000u;
    matrix+=64u;cursor+=8u;D_8012D554=matrix-64u;
    guMtxIdent((Mtx*)matrix);
    {u32 result=cursor;*(volatile u32*)cursor=0xDA380003u;*(volatile u32*)(cursor+4u)=matrix+0x80000000u;
     graphics->gfx=cursor+8u;graphics->mtx=matrix+64u;return result;}
}
CAM_PATCH u32 func_800E4640(volatile u32* gfx){
    u32 result,cursor;
    __asm__ volatile("move %0,$2":"=r"(result)::"memory");
    if(D_8012D508){cursor=*gfx;*gfx=cursor+8u;*(volatile u32*)(cursor+4u)=0x08000000u;
        *(volatile u32*)cursor=0xDE000000u;return result;}
    {struct TooieCameraProduction* p=&CAM_STATE.saved;u32 id=0,flags=CAM_POLICY_IGNORE;
     if(p->valid && p->matrix==D_8012D558 && p->epoch==CAM_STATE.control.epoch){id=p->id;flags=p->policy;}
     cursor=*gfx;if(!(CAM_STATE.control.role&TOOIE_CAMERA_FIXED_LIST_SCOPE))cursor=cam_projection_policy(cursor,id,flags);}
    *gfx=cursor+8u;*(volatile u32*)cursor=0xDA380007u;
    *(volatile u32*)(cursor+4u)=D_8012D558+0x80000000u;return result;
}
extern volatile s16 D_80132DCC;extern volatile u32 D_80127EF4;
struct TooieCameraGraphics* func_800A7180(void);
void func_800BE9B0(void);void func_800BFD6C(void*,u32);void func_800D2A44(void*);
u32 func_800BC8F8(void*);void func_800BE9E4(void*);void func_800BD594(void*);
void func_800EB51C(s32,void*);void func_800F5008(void*);void func_800B525C(void*);
s32 func_800BF3E4(void);void func_800BEA24(void*);void func_800B7BB0(void);
void func_800F50D0(void*);void func_800B52EC(void*);u32 func_800BC948(void*);
void _sufade_entrypoint_1(void*);void func_800A8EF0(void*,u32);u32 func_800F51CC(void);
CAM_PATCH u32 func_800E9F20(u32 arg0,u32 arg1){
    struct TooieCameraGraphics* graphics=func_800A7180();u32 role,result;
    (void)arg1;
    if(!D_80132DCC){
        role=CAM_STATE.control.role;CAM_STATE.control.role=TOOIE_CAMERA_WORLD_INITIAL;
        result=func_800E44FC(graphics);CAM_STATE.control.role=role;return result;
    }
    func_800BE9B0();
    role=CAM_STATE.control.role;CAM_STATE.control.role=TOOIE_CAMERA_PRE_WORLD;
    func_800BFD6C(graphics,arg0);CAM_STATE.control.role=role;
    role=CAM_STATE.control.role;CAM_STATE.control.role=TOOIE_CAMERA_WORLD_INITIAL;
    func_800E44FC(graphics);CAM_STATE.control.role=role;
    func_800D2A44(graphics);
    graphics->gfx=cam_precharge_replay(graphics->gfx);
    func_800BC8F8(graphics);func_800BE9E4(graphics);func_800BD594(graphics);
    func_800EB51C(1,graphics);func_800EB51C(5,graphics);func_800F5008(graphics);
    func_800EB51C(0,graphics);func_800B525C(graphics);
    if(func_800BF3E4())func_800BEA24(graphics);
    func_800B7BB0();func_800F50D0(graphics);func_800EB51C(2,graphics);
    func_800B52EC(graphics);func_800BC948(graphics);func_800EB51C(6,graphics);
    if(D_80127EF4)_sufade_entrypoint_1(graphics);
    func_800A8EF0(graphics,arg0);return func_800F51CC();
}
