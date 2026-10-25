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
    /* @recomp Begin the backup identity before the original copy into the temporary. */
    {u32 source=D_8012D500;cam_backup_begin(source,temporary);
     return func_800CA314((void*)temporary,(void*)source);}
}
CAM_PATCH u32 func_800E4208(void) {
    u32 destination=D_8012D500,source=D_8012D504,result;
    /* @recomp Validate the backup before the original restore copy; after the original
       free, retire the private backup and its depth. */
    cam_backup_validate(destination,source);
    func_800CA314((void*)destination,(void*)source);
    result=func_800CA364((void*)D_8012D504);D_8012D504=0;
    CAM_STATE.backup.valid=0;if(CAM_STATE.control.backup_depth)CAM_STATE.control.backup_depth--;
    return result;
}
CAM_PATCH u32 func_800E4628(void) {
    /* @recomp Invalidate the saved production and its float block (camera_float.h) alongside the
       original saved-matrix reset. */
    D_8012D554=0;D_8012D558=0;CAM_STATE.saved.valid=0;CAM_STATE.saved.reserved0=0;
    return (u32)&D_8012D554;
}

#include "camera_projection_internal_04.h"
extern s16 widescreen_enabled; /* Original gfx.h declaration, no new variable. */
void func_800CA7E4(u32,void*);u32 func_800CB124(u32,void*);f32 func_800CA6E8(u32);
void func_800E443C(void*,void*,f32);s32 func_80018BC4(void);void mlMtxApply(Mtx*);
/* BK float camera route (camera_float.h). CA7AC returns camera+0xA4, the affine R E443C consumes;
   D_8007BCD8/DC are the two TOOIE_CAMERA_MAIN_MATRIX_BYTES Mtx arenas camera_pools.c allocates per D_8007BEDA. */
#include "camera_float.h"
void* func_800CA7AC(u32);
extern volatile u32 D_8007BCD8[2],D_8007BCF0;extern volatile s16 D_8007BEDA;
#if defined(TOOIE_BASELINE_HOST) && TOOIE_BASELINE_HOST
#include "widescreen_settings_abi.h"
u32 func_800A8984(s32 view);
void func_800A8A04(s32 view,s32* x,s32* y,s32* width,s32* height);
f32 func_800A8AD4(s32 view);
f32 func_800A8AF0(s32 view); /* Original f0 return despite the decomp's void. */
f32 func_80015D14(const TooieAspectInputs* input);
#define CAM_HOST_WIDTH (TOOIE_WIDESCREEN_SETTINGS_ACTIVE|TOOIE_WIDESCREEN_SETTINGS_ENABLED)
CAM_INLINE u32 cam_positive_finite(f32 value){
    u32 word=cam_float_word(value);
    return !(word&0x80000000u) && word && word<0x7F800000u;
}
/* Only CB124's fresh raster copy changes. The game's aspect, culling planes,
 * aim and sprite consumers keep their original values. Unknown cameras and
 * derived ratios retain their inherited matrix; there is no epsilon test. */
CAM_INLINE void cam_unwiden_raster(u32 camera,f32 projection[4][4]){
    TooieAspectInputs unit;f32 aspect,stored,widening;u32 settings,row;
    s32 view=(s32)D_80127674,x,y,width,height;
    if(!widescreen_enabled)return;
    settings=tooie_get_latched_widescreen_settings();
    if((settings&CAM_HOST_WIDTH)!=CAM_HOST_WIDTH || view<0 || view>=5 ||
       func_800A8984(view)!=camera)return;
    /* Original viewport extents, not the host window size. RT64 AUTO does not
     * adapt narrow left/right views. Keep those and portrait/inset views. */
    func_800A8A04(view,&x,&y,&width,&height);
    if(x!=0 || width!=304 || y<0 || height<=0 || height>228-y)return;
    aspect=func_800A8AF0(view);stored=*(volatile f32*)(camera+0xE8u);
    if(!cam_positive_finite(aspect) || !cam_positive_finite(stored))return;
    if(cam_float_word(stored)!=cam_float_word(aspect)){
        f32 fov=func_800A8AD4(view),zero_offset;
        volatile f32 numerator,denominator;
        if(!cam_positive_finite(fov) ||
           cam_float_word(*(volatile f32*)(camera+0xE4u))!=cam_float_word(fov))return;
        /* C54F0 PCs55F8,560C,5600,5628: mul.s, add.s, add.s, div.s.
         * Volatile f32 temporaries retain each rounding and forbid fusion. */
        numerator=aspect*fov;numerator=numerator+0.0f;
        denominator=fov+0.0f;zero_offset=numerator/denominator;
        if(cam_float_word(stored)!=cam_float_word(zero_offset))return;
    }
    unit.signed_at_8=1;unit.signed_at_A=1;
    widening=func_80015D14(&unit);
    /* Live host changes must not mix two settings words within this copy. */
    if(!cam_positive_finite(widening) ||
       tooie_get_latched_widescreen_settings()!=settings)return;
    for(row=0;row<4u;row++)projection[row][0]*=widening;
}
/* Original D_8012D470+0x23 (D493): only the reticle route sets it (E24CC)
 * before E30E0, and E2E48's sole return clears it (E22F0->E23A4). */
extern volatile u8 D_8012D470[];
#define CAM_POLICY_AUTO_STRETCH CAM_GROUP_FLAGS(G_EX_INTERPOLATE_DECOMPOSE,G_EX_NOPUSH,G_MTX_PROJECTION, \
    G_EX_COMPONENT_AUTO,G_EX_COMPONENT_AUTO,G_EX_COMPONENT_AUTO,G_EX_COMPONENT_AUTO, \
    G_EX_COMPONENT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO,G_EX_ORDER_AUTO, \
    G_EX_EDIT_NONE,G_EX_ASPECT_STRETCH,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO)
_Static_assert(CAM_POLICY_AUTO_STRETCH==(CAM_POLICY_AUTO|0x00100000u) &&
    CAM_POLICY_AUTO_STRETCH==0x02131556u,"STRETCH variant of the public AUTO ortho word");
#endif
struct TooieCameraGraphics {volatile u32 gfx,mtx;};
_Static_assert(sizeof(struct TooieCameraGraphics)==8,"original two-pointer graphics ABI");
/* @recomp BK float camera route (camera_float.h) for one normal WORLD production in the MAIN
   context (0x80127658, arena D_8007BCD8[D_8007BEDA]) whose group was emitted, so the extended
   GBI is enabled ahead of it. Its block takes the arena slots after the production's two original
   matrices and lives exactly as long as they do. Redirected fills (D_8012D508), PRE and other
   roles, other contexts, a full arena or a refused input keep the original fixed LOAD. */
CAM_INLINE u32 cam_float_production(struct TooieCameraGraphics* graphics,u32 grouped,u32 block,
    f32 projection[4][4],const f32 translation[3],f32 scale){
    u32 index=(u32)(s32)D_8007BEDA,base;
    if(!grouped || D_8012D508 || CAM_STATE.control.role!=TOOIE_CAMERA_WORLD_INITIAL ||
       (u32)graphics!=0x80127658u || index>1u || D_8007BCF0)return 0;
    base=D_8007BCD8[index];
    /* Ordinary arena only; alternate matrices retain the complete original fixed route.
       Validate owned extent before subtraction, then reserve aligned production slots. */
    if(base<0x80000000u || base>0x81000000u-TOOIE_CAMERA_MAIN_MATRIX_BYTES || (base&7u) ||
       block<base || ((block-base)&63u) ||
       block-base>TOOIE_CAMERA_MAIN_MATRIX_BYTES-TOOIE_CAMERA_FLOAT_BYTES)return 0;
    return cam_float_build((volatile struct TooieCameraFloat*)block,block+0x80000000u,&projection[0][0],
        (const volatile f32*)func_800CA7AC(D_8012D500),translation,scale)?block:0u;
}
CAM_PATCH u32 func_800E44FC(struct TooieCameraGraphics* graphics){
    f32 projection[4][4],translation[3],scale;u32 normalization,cursor,matrix,camera,id,result,floats;
    func_800CA7E4(D_8012D500,translation);
    normalization=func_800CB124(D_8012D500,projection);
    /* @recomp HOST only: raster-copy compensation after the original CB124. */
#if defined(TOOIE_BASELINE_HOST) && TOOIE_BASELINE_HOST
    cam_unwiden_raster(D_8012D500,projection);
#endif
    cursor=graphics->gfx;graphics->gfx=cursor+8u;
    *(volatile u32*)(cursor+4u)=normalization;*(volatile u32*)cursor=0xDB0E0000u;
    scale=func_800CA6E8(D_8012D500);func_800E443C(projection,translation,scale);
    if(func_80018BC4()){
        scale=func_800CA6E8(D_8012D500);scale=scale+scale;
        func_800E443C(projection,translation,scale);
    }
    mlMtxApply((Mtx*)graphics->mtx);
    matrix=graphics->mtx;camera=D_8012D500;
    /* @recomp Capture the camera production and emit its public policy, including PRE
       admission/commit; the original matrix writes follow. */
#if TOOIE_PRE_BACKGROUND
    {u32 before=graphics->gfx,pre_mask=CAM_STATE.control.role==TOOIE_CAMERA_PRE_WORLD?cam_pre_admit((u32)graphics,before):0;
     id=cam_capture_perspective(matrix,camera,cam_float_word(scale),0x800E4598u,pre_mask);
     cursor=cam_projection_policy(before,id,CAM_STATE.saved.policy);
     if(CAM_STATE.control.role==TOOIE_CAMERA_PRE_WORLD)cam_pre_commit(before,cursor,pre_mask,id);}
#else
    id=cam_capture_perspective(matrix,camera,cam_float_word(scale),0x800E4598u,0);
    cursor=cam_projection_policy(graphics->gfx,id,CAM_STATE.saved.policy);
#endif
    /* @recomp BK route: a WORLD production with a float block calls its LOAD P / MUL V list in the
       8 bytes of the original projection LOAD; D_8012D558 and the fixed product stay original. */
    floats=cam_float_production(graphics,cursor!=graphics->gfx,matrix+128u,projection,translation,scale);
    graphics->gfx=cursor+8u;
    if(floats){*(volatile u32*)cursor=0xDE000000u;
        *(volatile u32*)(cursor+4u)=floats+TOOIE_CAMERA_FLOAT_COMMANDS+0x80000000u;CAM_STATE.saved.reserved0=floats;}
    else{*(volatile u32*)cursor=0xDA380007u;*(volatile u32*)(cursor+4u)=graphics->mtx+0x80000000u;}
    matrix=graphics->mtx;graphics->mtx=matrix+64u;D_8012D558=matrix;
    guMtxIdent((Mtx*)graphics->mtx);
    __asm__ volatile("move %0,$2":"=r"(result)::"memory");
    cursor=graphics->gfx;graphics->gfx=cursor+8u;*(volatile u32*)cursor=0xDA380003u;
    *(volatile u32*)(cursor+4u)=graphics->mtx+0x80000000u;
    matrix=graphics->mtx;graphics->mtx=matrix+64u;
    /* @recomp The float block owns the next TOOIE_CAMERA_FLOAT_BYTES of the arena. */
    if(floats)graphics->mtx=matrix+64u+TOOIE_CAMERA_FLOAT_BYTES;
    return result;
}
CAM_PATCH u32 func_800E42F0(struct TooieCameraGraphics* graphics,s32 use_widescreen){
    u32 cursor=graphics->gfx,matrix=graphics->mtx;
#if defined(TOOIE_BASELINE_HOST) && TOOIE_BASELINE_HOST
    /* @recomp With host width, RT64 AUTO widens full-width ortho and the view aspect is
     * the host aspect. DF944, the only wide=1 loader, still spreads translation
     * by view/(4/3), which AUTO cancels, so that class takes 4:3 bounds. The
     * reticle keeps its E2DF0 scale compensation; STRETCH stops AUTO repeating
     * it. Direct wide=0 owners, fills and wide OFF keep AUTO and their bounds. */
    u32 policy=CAM_POLICY_AUTO,host=widescreen_enabled && (use_widescreen || D_8012D470[0x23]) &&
        (tooie_get_latched_widescreen_settings()&CAM_HOST_WIDTH)==CAM_HOST_WIDTH;
    if(use_widescreen && widescreen_enabled && !host)
        guOrtho((Mtx*)matrix,-810.f,810.f,-456.f,456.f,-50.f,50.f,1.f);
    else guOrtho((Mtx*)matrix,-608.f,608.f,-456.f,456.f,-50.f,50.f,1.f);
    if(host && !use_widescreen)policy=CAM_POLICY_AUTO_STRETCH;
    cursor=cam_projection_policy(cursor,0xFFFFFFFFu,policy);
#else
    if(use_widescreen && widescreen_enabled)
        guOrtho((Mtx*)matrix,-810.f,810.f,-456.f,456.f,-50.f,50.f,1.f);
    else guOrtho((Mtx*)matrix,-608.f,608.f,-456.f,456.f,-50.f,50.f,1.f);
    /* @recomp Public AUTO ortho policy; the bounds choice above is original. */
    cursor=cam_projection_policy(cursor,0xFFFFFFFFu,CAM_POLICY_AUTO);
#endif
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
    /* @recomp Validate the saved production and epoch, then emit its restore policy outside
       fixed-list scope; the segmented branch above is original. That production's own float block
       (camera_float.h) is restored by calling its list in the 8 bytes of the original fixed LOAD,
       so MAIN, fills and the 48-byte fixed-list slots (camera_replay.c) keep their sizes;
       otherwise the original LOAD of D_8012D558. */
    {struct TooieCameraProduction* p=&CAM_STATE.saved;u32 id=0,flags=CAM_POLICY_IGNORE,floats=0;
     if(p->valid && p->matrix==D_8012D558 && p->epoch==CAM_STATE.control.epoch){id=p->id;flags=p->policy;floats=p->reserved0;}
     cursor=*gfx;if(!(CAM_STATE.control.role&TOOIE_CAMERA_FIXED_LIST_SCOPE))cursor=cam_projection_policy(cursor,id,flags);
     *gfx=cursor+8u;
     if(floats){*(volatile u32*)cursor=0xDE000000u;
         *(volatile u32*)(cursor+4u)=floats+TOOIE_CAMERA_FLOAT_COMMANDS+0x80000000u;}
     else{*(volatile u32*)cursor=0xDA380007u;*(volatile u32*)(cursor+4u)=D_8012D558+0x80000000u;}}
    return result;
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
    /* @recomp Scope WORLD/PRE roles around the original producers, and prepay the redirected
       replay (cam_precharge_replay) before the original redirected-list work. */
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
