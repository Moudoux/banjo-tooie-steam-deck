/* Reconstructs original core2/anctrl setters; @recomp calls invalidate port state.
 * 8008B10C folds original 8008C1C0(controller+1C): store raw a1 bits at +2C.
 * The replacement carries raw o32 a1 bits without an FPU operation. */
#include "silo_bridge.h"
#define PATCH __attribute__((section(".recomp_patch"), noinline))
PATCH void func_8008B10C(void* controller,u32 payload) {
    *(volatile u32*)((u8*)controller+0x2C)=payload;
    /* @recomp Invalidate retained correspondence after the original setter effect. */
    silo_invalidate((u32)controller);
}
PATCH void anctrl_setSmoothTransition(void* controller,u32 value) {
    volatile u8* byte=(volatile u8*)controller+0x1B;
    *byte=(u8)((*byte&0xBFu)|((value&1u)<<6));
    /* @recomp Invalidate retained correspondence after the original setter effect. */
    silo_invalidate((u32)controller);
}
