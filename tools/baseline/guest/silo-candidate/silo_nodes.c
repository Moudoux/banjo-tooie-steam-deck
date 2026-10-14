/* Two original-preserving reset boundaries; raw o32 a1 payload, no FPU. */
#include "silo_bridge.h"
#define PATCH __attribute__((section(".recomp_patch"), noinline))
PATCH void func_8008B10C(void* controller,u32 payload) {
    *(volatile u32*)((u8*)controller+0x2C)=payload;
    silo_invalidate((u32)controller);
}
PATCH void anctrl_setSmoothTransition(void* controller,u32 value) {
    volatile u8* byte=(volatile u8*)controller+0x1B;
    *byte=(u8)((*byte&0xBFu)|((value&1u)<<6));
    silo_invalidate((u32)controller);
}
