/* Optional independent right-stick controls for the original roaming FPS
 * update. Original guest channel, camera, animation and physics owners remain. */
#include "common.h"
#include "overlays/ba/stick.h"
#include "overlays/ba/key.h"
#include "overlays/bs/state.h"
#include "first_person_aim_abi.h"
#define PATCH __attribute__((section(".recomp_patch"), noinline))
#define HELPER __attribute__((section(".tooie_modern_fps_helpers"), noinline))
#define INLINE static __attribute__((always_inline)) inline
extern s32 func_800EA05C(void), func_800A9CAC(void);
extern s32 func_80091E80(PlayerState*, s32);
struct ModernFpsPacket { f32 right_x, right_y; };
/* Native absolute import0x8F000038. o32 returns high word in v0, low in v1. */
extern u64 tooie_baseline_first_person_aim(void);
extern void func_80804488_bsfirstp(PlayerState*, s32, s32, void*);
extern void func_808045FC_bsfirstp(void*, f32);
extern void _baeggcursor_entrypoint_10(PlayerState*, f32, f32);
extern void func_80801928_bsfirstp(PlayerState*), func_80801DC0_bsfirstp(PlayerState*);
extern void func_808023A0_bsfirstp(PlayerState*), func_80808DB0_bsfirstp(PlayerState*);
extern void func_80809D24_bsfirstp(PlayerState*), func_80800830_bsfirstp(PlayerState*);
extern void func_80808EF0_bsfirstp(PlayerState*), func_80802F20_bsfirstp(PlayerState*);
extern void func_80809A10_bsfirstp(PlayerState*), func_80809A7C_bsfirstp(PlayerState*);
extern void func_8080682C_bsfirstp(PlayerState*), func_808099B4_bsfirstp(PlayerState*);
extern f32 func_808068E8_bsfirstp(PlayerState*), func_80806928_bsfirstp(PlayerState*);
extern f32 func_80809D94_bsfirstp(PlayerState*), func_80013728(f32);
extern f32 func_800F122C(f32, f32, f32), func_80806AD0_bsfirstp(PlayerState*, f32, f32);
enum FirstPersonOffset {
    INPUT_ARRANGEMENT=0xC,
    ADD_PITCH_OFFSET=0x10,
    REDIRECT_STICK_Y=0x118,
    C_HORIZONTAL_RAMP=0x11C,
    C_VERTICAL_RAMP=0x124,
    STRAFE_CHANNEL=0x12C,
    FORWARD_CHANNEL=0x148,
    YAW_CHANNEL=0x164,
    PITCH_CHANNEL=0x180,
    CURSOR_MODE=0xA8,
    CURSOR_X=0xB8,
    CURSOR_Y=0xBC,
    CURSOR_YAW_OFFSET=0xC0,
    CURSOR_PITCH_OFFSET=0xC4,
    PITCH_CUBIC=0x184,
    /* Original 045FC channel+8 scalar and signed channel+C state (-1/0/1).
     * 069FC reads the scalar and tests the state for nonzero, not a boolean edge. */
    PITCH_HISTORY=0x188,
    PITCH_DIRECTION_CHANGED=0x18C,
    PITCH_DIRECT_RATE=0x1F0,
    PITCH_CURSOR_RATE=0x1F4,
    PITCH_DIRECT_ACCEL=0x1F8,
    PITCH_DIRECT_DECEL=0x200,
    PITCH_CURSOR_DECEL=0x204,
    PITCH_CURSOR_ACCEL=0x1FC,
    PITCH_UPPER_BOUND=0x1E8,
    PITCH_LOWER_BOUND=0x1EC,
    PITCH_TARGET=0x208,
    PITCH_CURRENT=0x20C,
    YAW_MODE=0x248,
};
struct ModernFpsScope { PlayerState *sampled, *active_owner; BaStick* stick;
    struct ModernFpsPacket packet; s32 pending; BanjoStateId state; };
__attribute__((section(".tooie_modern_fps_state"), aligned(4)))
struct ModernFpsScope tooie_modern_fps_scope = {0};
/* Original shared behavior storage at player+188; the decomp names this slot
 * kazfly, but the firstp overlay uses its own private state layout there. */
INLINE u8* first_person_state(PlayerState* self) { return *(u8**)((u8*)self + 0x188); }
INLINE f32 read_float(const u8* state, u32 offset) { return *(const f32*)(state+offset); }
INLINE void write_float(u8* state, u32 offset, f32 value) { *(f32*)(state+offset)=value; }
INLINE s32 read_word(const u8* state, u32 offset) { return *(const s32*)(state+offset); }
INLINE s32 is_roaming_fps_state(PlayerState* self) {
    switch ((u32)self->state->current) {
    case 0xCA: case 0xD1: case 0xD2: case 0xD6: case 0xD8: return 1;
    default: return 0;
    }
}
INLINE s32 has_supported_arrangement(u8* first_person) { return read_word(first_person,INPUT_ARRANGEMENT) == 0 || read_word(first_person,INPUT_ARRANGEMENT) == 1; }
INLINE s32 modern_fps_active(PlayerState* self) { return tooie_modern_fps_scope.active_owner == self; }
_Static_assert(sizeof(void*)==4 && sizeof(f32)==4 && sizeof(struct ModernFpsPacket)==8,"o32 packet");
_Static_assert(__builtin_offsetof(PlayerState,state)==0x120,"state owner");
_Static_assert(__builtin_offsetof(PlayerState,stick)==0x128,"stick owner");
_Static_assert(__builtin_offsetof(PlayerState,kazfly)==0x188,"shared behavior storage");
_Static_assert(__builtin_offsetof(BanjoState,current)==4,"state ID");
_Static_assert(sizeof(struct ModernFpsScope)==28,"ephemeral sample layout");

/* Neutral is valid admission. Only policy0/unavailable restores original input.
 * Apply the configured response gain/inversion only to these aim channels;
 * the original045FC channel bounds and game time integrators stay in charge. */
INLINE s32 read_modern_aim(struct ModernFpsPacket* axes) {
    u64 packed=tooie_baseline_first_person_aim();
    u32 sample=(u32)(packed>>32), policy=(u32)packed;
    u32 sensitivity=(policy>>TOOIE_FP_AIM_SENSITIVITY_SHIFT)&0xFFu;
    s32 x=(s16)(sample>>16), y=(s16)sample;
    f32 gain;
    if (!(policy&TOOIE_FP_AIM_ENABLED) || (policy&~0xFF07u) ||
        sensitivity<TOOIE_FP_AIM_SENSITIVITY_MIN || sensitivity>TOOIE_FP_AIM_SENSITIVITY_MAX ||
        x == -32768 || y == -32768) return 0;
    gain=(f32)sensitivity/100.0f;
    axes->right_x=(f32)x/32767.0f*gain;
    axes->right_y=(f32)y/32767.0f*gain;
    if (policy&TOOIE_FP_AIM_INVERT_X) axes->right_x=-axes->right_x;
    if (policy&TOOIE_FP_AIM_INVERT_Y) axes->right_y=-axes->right_y;
    if (axes->right_x>1.0f) axes->right_x=1.0f;
    if (axes->right_x< -1.0f) axes->right_x=-1.0f;
    if (axes->right_y>1.0f) axes->right_y=1.0f;
    if (axes->right_y< -1.0f) axes->right_y=-1.0f;
    return 1;
}

/* Single before-instruction hook at original func_800991B0 PC80099220.
 * This observes unk67 before bastick_update consumes the one-tick reset. */
HELPER void tooie_modern_fps_capture(PlayerState* self) {
    u8* first_person;
    tooie_modern_fps_scope.pending=0; tooie_modern_fps_scope.active_owner=0;
    if (!self || !self->state || !self->stick || !is_roaming_fps_state(self)) return;
    if (bakey_getControllerIndex(self)!=0 || func_800A9CAC()) return;
    if (func_800EA05C()==0xBC || func_800EA05C()==0x124) return;
    if (self->stick->unk64 || self->stick->unk65 || self->stick->unk67 ||
        !func_80091E80(self,0x40)) return;
    first_person=first_person_state(self);
    if (!first_person || !has_supported_arrangement(first_person) ||
        !read_modern_aim(&tooie_modern_fps_scope.packet)) return;
    tooie_modern_fps_scope.sampled=self;
    tooie_modern_fps_scope.stick=self->stick;
    tooie_modern_fps_scope.state=self->state->current;
    tooie_modern_fps_scope.pending=1;
}
INLINE void begin_modern_fps_update(PlayerState* self) {
    s32 pending=tooie_modern_fps_scope.pending;
    tooie_modern_fps_scope.pending=0;
    tooie_modern_fps_scope.active_owner=0;
    if (pending && tooie_modern_fps_scope.sampled==self && tooie_modern_fps_scope.stick==self->stick &&
        is_roaming_fps_state(self) && tooie_modern_fps_scope.state==self->state->current &&
        !func_800A9CAC() && has_supported_arrangement(first_person_state(self)))
        tooie_modern_fps_scope.active_owner=self;
}

/* Reconstructs original bs/firstp/func_80804848; the mode -1/0/1 fallback remains.
 * Original digital ramps still advance before the port override. */
PATCH void func_80804848_bsfirstp(PlayerState* self, s32 mode) {
    u8* first_person=first_person_state(self); f32 forward,pitch;
    func_80804488_bsfirstp(self,0xA,0xD,first_person+C_HORIZONTAL_RAMP);
    func_80804488_bsfirstp(self,0xB,0xC,first_person+C_VERTICAL_RAMP);
    /* @recomp Split left-stick movement from captured right-stick aim;
     * each value still passes through the original 045FC channel updater. */
    if (modern_fps_active(self) && (mode==0 || mode==1)) {
        func_808045FC_bsfirstp(first_person+STRAFE_CHANNEL,bastick_getX(self));
        func_808045FC_bsfirstp(first_person+FORWARD_CHANNEL,bastick_getY(self));
        func_808045FC_bsfirstp(first_person+YAW_CHANNEL,tooie_modern_fps_scope.packet.right_x);
        func_808045FC_bsfirstp(first_person+PITCH_CHANNEL,tooie_modern_fps_scope.packet.right_y);
        return;
    }
    if (mode==-1) {
        func_808045FC_bsfirstp(first_person+STRAFE_CHANNEL,0);
        func_808045FC_bsfirstp(first_person+FORWARD_CHANNEL,0);
        func_808045FC_bsfirstp(first_person+YAW_CHANNEL,0);
        func_808045FC_bsfirstp(first_person+PITCH_CHANNEL,0);
    } else if (mode==0) {
        forward=read_word(first_person,REDIRECT_STICK_Y) ? 0 : bastick_getY(self);
        pitch=read_word(first_person,REDIRECT_STICK_Y) ? bastick_getY(self) : read_float(first_person,C_VERTICAL_RAMP);
        func_808045FC_bsfirstp(first_person+STRAFE_CHANNEL,read_float(first_person,C_HORIZONTAL_RAMP));
        func_808045FC_bsfirstp(first_person+FORWARD_CHANNEL,forward);
        func_808045FC_bsfirstp(first_person+YAW_CHANNEL,bastick_getX(self));
        func_808045FC_bsfirstp(first_person+PITCH_CHANNEL,pitch);
    } else if (mode==1) {
        forward=read_word(first_person,REDIRECT_STICK_Y) ? 0 : read_float(first_person,C_VERTICAL_RAMP);
        func_808045FC_bsfirstp(first_person+STRAFE_CHANNEL,read_float(first_person,C_HORIZONTAL_RAMP));
        func_808045FC_bsfirstp(first_person+FORWARD_CHANNEL,forward);
        func_808045FC_bsfirstp(first_person+YAW_CHANNEL,bastick_getX(self));
        func_808045FC_bsfirstp(first_person+PITCH_CHANNEL,bastick_getY(self));
    }
}
/* Reconstructs original bs/firstp/func_80808E78 call order.
 * @recomp Captured-input scope and the active-only cursor refresh are additions. */
PATCH void func_80808E78_bsfirstp(PlayerState* self) {
    u8* first_person=first_person_state(self);
    /* @recomp Consume the captured sample for this common update only. */
    begin_modern_fps_update(self);
    func_80804848_bsfirstp(self,read_word(first_person,INPUT_ARRANGEMENT));
    func_80801928_bsfirstp(self);
    func_80801DC0_bsfirstp(self);
    /* @recomp Center both the firstp record and authoritative baeggcursor XY,
     * then invoke original 023A0 for those centered values. */
    if (modern_fps_active(self)) {
        write_float(first_person,CURSOR_X,152.0f);
        write_float(first_person,CURSOR_Y,114.0f);
        write_float(first_person,CURSOR_YAW_OFFSET,0.0f);
        write_float(first_person,CURSOR_PITCH_OFFSET,0.0f);
        _baeggcursor_entrypoint_10(self,152,114);
        func_808023A0_bsfirstp(self);
    }
    func_80808DB0_bsfirstp(self);
    func_80809D24_bsfirstp(self);
    func_8080682C_bsfirstp(self);
    func_808099B4_bsfirstp(self);
    func_80800830_bsfirstp(self);
    func_80808EF0_bsfirstp(self);
    func_80802F20_bsfirstp(self);
    /* @recomp End this update's captured-input scope. */
    tooie_modern_fps_scope.active_owner=0;
}
/* Reconstructs original bs/firstp/func_808099B4 signed mode 1/2 dispatch. */
PATCH void func_808099B4_bsfirstp(PlayerState* self) {
    switch (*(s16*)(first_person_state(self)+YAW_MODE)) {
    case 1: func_80809A10_bsfirstp(self); break;
    case 2:
        /* @recomp Active mode 2 uses the original direct-input yaw updater. */
        if (modern_fps_active(self)) func_80809A10_bsfirstp(self);
        else func_80809A7C_bsfirstp(self);
        break;
    }
}
/* Reconstructs original bs/firstp/func_808069FC pitch paths and 06AD0 call. */
PATCH f32 func_808069FC_bsfirstp(PlayerState* self) {
    u8* first_person=first_person_state(self);
    f32 target,accel;
    /* @recomp Active split input selects the original direct-channel path. */
    if (read_word(first_person,CURSOR_MODE)==0 || modern_fps_active(self)) {
        target=func_800F122C(-read_float(first_person,PITCH_CHANNEL),0,read_float(first_person,PITCH_DIRECT_RATE));
        accel=read_word(first_person,PITCH_DIRECTION_CHANGED) ?
            read_float(first_person,PITCH_DIRECT_ACCEL)/(1+read_float(first_person,PITCH_HISTORY)) :
            read_float(first_person,PITCH_DIRECT_DECEL);
    } else {
        target=func_800F122C(-read_float(first_person,CURSOR_PITCH_OFFSET),0,read_float(first_person,PITCH_CURSOR_RATE));
        accel=read_float(first_person,CURSOR_PITCH_OFFSET)==0 ?
            read_float(first_person,PITCH_CURSOR_DECEL) :
            read_float(first_person,PITCH_CURSOR_ACCEL);
    }
    return func_80806AD0_bsfirstp(self,target,accel);
}
/* Replace only arrangement1's existing leaf. Leave 0682C, including its unknown
 * mode path and original outer limits, entirely unchanged. */
PATCH f32 func_80806928_bsfirstp(PlayerState* self) {
    u8* first_person=first_person_state(self);
    f32 axis,target;
    /* @recomp Active arrangement 1 uses original 068E8; fallback below is original. */
    if (modern_fps_active(self)) return func_808068E8_bsfirstp(self);
    axis=-read_float(first_person,PITCH_CUBIC);
    target=axis<0 ?
        read_float(first_person,PITCH_UPPER_BOUND)*axis :
        -read_float(first_person,PITCH_LOWER_BOUND)*axis;
    if (read_word(first_person,ADD_PITCH_OFFSET)) target+=func_80809D94_bsfirstp(self);
    write_float(first_person,PITCH_TARGET,target);
    return func_80013728(target-read_float(first_person,PITCH_CURRENT))/12.0f;
}
