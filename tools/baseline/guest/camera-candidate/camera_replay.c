/* Original BC8F8/BC948 operations plus bounded, paid camera replay envelopes. */
#include "camera_projection_internal_04.h"
void func_800E42E4(s32);u32 func_800E4640(volatile u32*);void func_800BCC90(void*);
void osWritebackDCache(void*,s32);u32 func_800E42A8(void);
s32 func_800CAA24(u32,void*,void*);void func_800E46E0(void*,void*);
void func_800E7FF8(void*,u32);u32 func_800D674C(s32);u32 func_800B2840(u32);
void func_800B21CC(u32,void*,void*);void func_800DF440(s32);void func_800DF830(s32);
void func_800DF410(s32);u32 func_800DE498(void*,void*,void*,f32,void*,u32);
extern volatile u8 D_80128390[],D_8012839C[];
extern volatile u32 D_8012D558;
/* Authored replay helpers: fixed-list restore scope, MAIN span check and paid closure. */
CAM_INLINE void cam_fixed_restore(volatile u32* cursor){
    u32 role=CAM_STATE.control.role;CAM_STATE.control.role=role|TOOIE_CAMERA_FIXED_LIST_SCOPE;
    func_800E4640(cursor);CAM_STATE.control.role=role;
}
#define CAM_REFUSE_REPLAY_SPAN 0x101u
CAM_INLINE u32 cam_replay_span(u32 cursor,u32 bytes){
    struct TooieCameraControl* c=&CAM_STATE.control;
    if(c->active && !(cursor&7u) && cursor>=c->main_base && cursor<=c->main_end && bytes<=c->main_end-cursor)return 1;
    c->latch=1;CAM_STATE.diagnostics.refusals++;
    if(!CAM_STATE.diagnostics.first_reason)CAM_STATE.diagnostics.first_reason=CAM_REFUSE_REPLAY_SPAN;
    CAM_STATE.diagnostics.raw0=cursor;CAM_STATE.diagnostics.raw1=c->main_base;
    CAM_STATE.diagnostics.raw2=c->main_end;CAM_STATE.diagnostics.raw3=bytes;return 0;
}
CAM_INLINE u32 cam_replay_closure(volatile u32* cursor,u32 paid){
    if(paid){u32 p=*cursor;if(!cam_replay_span(p,16))return 0;
        cam_group_words(p,0,CAM_POLICY_IGNORE);*cursor=p+16u;CAM_STATE.control.main_begin=*cursor;}
    return 1;
}
CAM_PATCH u32 func_800BC8F8(volatile u32* graphics){
    volatile u8* s=D_801282E0;u32 result=(u32)s;
    if(s[0xD]){
        u32 index=s[0xC],base;
        *(volatile u32*)(s+8)=*graphics;base=*(volatile u32*)(s+index*4u);*graphics=base;
        func_800E42E4(1);
        /* @recomp After the original redirection: fill identity, range/epoch closure checks,
           Enable reservation and the private fill cursor. */
        if(CAM_STATE.control.active && index<2u && base==(u32)&tooie_camera_redirected[index][0]){
            struct TooieCameraFill* fill=&CAM_STATE.fill[index];
            if(fill->valid && fill->epoch==CAM_STATE.control.epoch && !fill->closed){
                CAM_STATE.control.latch=1;fill->latched=1;
            }else{
                fill->base=base;fill->end=base+TOOIE_CAMERA_R_BYTES;fill->begin=base+8u;
                fill->extra=0;fill->reserve=0;fill->valid=0;fill->epoch=CAM_STATE.control.epoch;
                fill->main_saved=*(volatile u32*)(s+8);fill->slot=index;fill->latched=CAM_STATE.control.latch;
                fill->closed=0;fill->reserved=0;cam_enable_words(base);fill->extra=8;fill->valid=1;*graphics=base+8u;
            }
        }
    }
    return result;
}
CAM_PATCH u32 func_800BC948(volatile u32* graphics){
    u32 result,paid,payment_complete,token_base,token_main,token_epoch,fill_base,first_target,second_target,second_fixed;
    volatile u8* s;volatile u32 fixed_cursor;
    __asm__ volatile("move %0,$2":"=r"(result)::"memory");
    paid=0;payment_complete=1;token_base=0;token_main=0;token_epoch=0;s=D_801282E0;
    if(!s[0xD])return result;
    /* @recomp Snapshot the fill and paid token, and validate the token. */
    fill_base=*(volatile u32*)(s+(u32)s[0xC]*4u);
    if(CAM_STATE.control.active && CAM_STATE.token.paid==1u){
        paid=1;token_base=CAM_STATE.token.slot_base;token_main=CAM_STATE.token.expected_main;
        token_epoch=CAM_STATE.token.epoch;CAM_STATE.token.paid=2;
        if(token_epoch!=CAM_STATE.control.epoch || token_main!=*(volatile u32*)(s+8) ||
           s[0xC]>=2u || token_base!=*(volatile u32*)(s+(u32)s[0xC]*4u)){
            CAM_STATE.control.latch=1;CAM_STATE.diagnostics.mismatch++;
        }
    }else if(CAM_STATE.control.active){CAM_STATE.control.latch=1;CAM_STATE.diagnostics.tokenless++;}
    s[0x10]=0;
    {u32 cursor=*graphics;*graphics=cursor+8u;*(volatile u32*)(cursor+4u)=0;
     *(volatile u32*)cursor=0xDF000000u;}
    {u32 base=*(volatile u32*)(s+(u32)s[0xC]*4u),length=*graphics-base;
     osWritebackDCache((void*)base,(s32)length>>3<<3);}
    *graphics=*(volatile u32*)(s+8);func_800E42E4(0);
    /* @recomp Unpaid replay emits an IGNORE policy. */
    if(CAM_STATE.control.active && !paid)*graphics=cam_projection_policy(*graphics,0,CAM_POLICY_IGNORE);
    first_target=(u32)s+0xC8u+(u32)s[0xC]*48u;fixed_cursor=first_target;
    /* @recomp Both cam_fixed_restore calls run the original E4640 restore in fixed-list scope. */
    cam_fixed_restore(&fixed_cursor);func_800BCC90((void*)&fixed_cursor);
    {u32 cursor=fixed_cursor;fixed_cursor=cursor+8u;*(volatile u32*)(cursor+4u)=0;*(volatile u32*)cursor=0xDF000000u;}
    second_fixed=fixed_cursor;cam_fixed_restore(&fixed_cursor);
    {u32 cursor=fixed_cursor;fixed_cursor=cursor+8u;*(volatile u32*)(cursor+4u)=0;*(volatile u32*)cursor=0xDF000000u;}
    osWritebackDCache((void*)((u32)s+0xC8u+(u32)s[0xC]*48u),
                     (s32)(fixed_cursor-((u32)s+0xC8u+(u32)s[0xC]*48u))>>3<<3);
    second_target=second_fixed;
    /* @recomp Prepaid 56-byte replay wrappers carrying the saved-production policy. */
    if(paid){
        u32 block=*graphics,id=0,flags=CAM_POLICY_IGNORE;struct TooieCameraProduction* p=&CAM_STATE.saved;
        if(!CAM_STATE.control.latch && p->valid && p->matrix==D_8012D558 &&
           p->epoch==CAM_STATE.control.epoch){id=p->id;flags=p->policy;}
        /* Payment reserved56 bytes; a repointed cursor still must pass actual ownership/span. */
        if(cam_replay_span(block,56)){
        *(volatile u32*)block=0xDE010000u;*(volatile u32*)(block+4u)=(block+56u)+0x80000000u;
        /* Reflection is current-only; do not alter saved production or refusal state. */
        cam_group_words(block+8u,0,CAM_POLICY_IGNORE);*(volatile u32*)(block+24u)=0xDE010000u;
        *(volatile u32*)(block+28u)=first_target+0x80000000u;
        cam_group_words(block+32u,id,flags);*(volatile u32*)(block+48u)=0xDE010000u;
        *(volatile u32*)(block+52u)=second_target+0x80000000u;
        first_target=block+8u;second_target=block+32u;*graphics=block+56u;
        CAM_STATE.control.main_begin=*graphics;
        }else payment_complete=0;
    }
    if(!s[0xE] || func_800CAA24(func_800E42A8(),(void*)D_80128390,(void*)D_8012839C)){
        func_800E46E0((void*)graphics,(void*)first_target);
        /* @recomp Paid replay already established MAIN Enable. The ordinary reflection
         * retires its group within the existing one-shot reserve, never reseeds. */
        if(paid && CAM_STATE.control.active && CAM_STATE.control.main_extra){
            if(cam_replay_span(*graphics,16u))
                *graphics=cam_projection_policy(*graphics,0,CAM_POLICY_IGNORE);
            else payment_complete=0;
        }
        func_800BCC90((void*)graphics);
        {u32 cursor=*graphics;*graphics=cursor+8u;*(volatile u32*)(cursor+4u)=0;
         *(volatile u32*)cursor=0xD9FFF9FFu;}
        func_800E7FF8((void*)graphics,0x200);
        {u32 cursor=*graphics;*graphics=cursor+8u;*(volatile u32*)cursor=0xDE000000u;
         *(volatile u32*)(cursor+4u)=*(volatile u32*)(s+(u32)s[0xC]*4u)+0x80000000u;}
        /* @recomp Paid closure before the original restore. */
        if(!cam_replay_closure(graphics,paid))payment_complete=0;func_800E4640(graphics);
        {u32 cursor=*graphics;*graphics=cursor+8u;*(volatile u32*)(cursor+4u)=0;
         *(volatile u32*)cursor=0xD9FFF9FFu;}
        func_800E7FF8((void*)graphics,0x400);
        {s32 descriptor=*(volatile s16*)(s+0x12);if(descriptor){
            u32 model=func_800D674C(descriptor);
            if(!s[0xF]){u32 object=func_800B2840(model);func_800B21CC(object,(void*)D_80128390,(void*)D_8012839C);
                s[0xE]=1;s[0xF]=1;}
            func_800DF440(0);func_800DF830(1);func_800DF410(s[0x11]);
            func_800DE498((void*)graphics,0,0,1.f,0,model);
        }}
        s[0x10]=1;
    }
    func_800E46E0((void*)graphics,(void*)second_target);
    {u32 cursor=*graphics;*graphics=cursor+8u;*(volatile u32*)cursor=0xDE000000u;
     *(volatile u32*)(cursor+4u)=*(volatile u32*)(s+(u32)s[0xC]*4u)+0x80000000u;
     result=cursor;}
    /* @recomp Paid closure before the original slot toggle; then consume a fully paid
       token and close the fill. */
    if(!cam_replay_closure(graphics,paid))payment_complete=0;s[0xC]^=1u;
    if(paid && payment_complete && CAM_STATE.token.paid==2u && CAM_STATE.token.slot_base==token_base &&
       CAM_STATE.token.expected_main==token_main && CAM_STATE.token.epoch==token_epoch)CAM_STATE.token.paid=0;
    {u32 i;for(i=0;i<2;i++)if(CAM_STATE.fill[i].valid && CAM_STATE.fill[i].base==fill_base)
        CAM_STATE.fill[i].closed=1;}
    return result;
}
