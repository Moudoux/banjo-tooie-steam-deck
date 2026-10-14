/* Original default bone handler; no labels or additional rendering state. */
#include "common.h"
#define PATCH __attribute__((section(".recomp_patch")))
typedef struct {
    Gfx* volatile gfx;
    Mtx* volatile mtx;
} BoneBuffers;
extern void* volatile D_8012C968;
extern u8 D_8012CE58[];
void* func_800ADE2C(void*, s32);
void func_80019AA0(void*, void*);
void mlMtxApply(Mtx*);
void func_800DE2A4(BoneBuffers*, u8*);
void mlMtxPop(void);
_Static_assert(sizeof(void*) == 4 && sizeof(BoneBuffers) == 8 &&
    __builtin_offsetof(BoneBuffers, mtx) == 4 && sizeof(Gfx) == 8 && sizeof(Mtx) == 64,
    "original bone handler guest ABI");
PATCH void func_800DCF48(BoneBuffers* buffers, u8* node) {
    void* animation = D_8012C968;
    if (animation) {
        void* matrix = func_800ADE2C(animation, *(volatile s8*)(node + 9));
        func_80019AA0(D_8012CE58, matrix);
        mlMtxApply(buffers->mtx);
        Gfx* cursor = buffers->gfx;
        buffers->gfx = cursor + 1;
        ((volatile u32*)cursor)[0] = 0xDA380002u;
        ((volatile u32*)cursor)[1] = (u32)buffers->mtx;
        buffers->mtx = (Mtx*)((u8*)buffers->mtx + 64);
    }
    u32 child = *(volatile u8*)(node + 8);
    if (child) func_800DE2A4(buffers, node + child);
    if (D_8012C968) {
        mlMtxPop();
        Gfx* cursor = buffers->gfx;
        buffers->gfx = cursor + 1;
        ((volatile u32*)cursor)[1] = 0x40u;
        ((volatile u32*)cursor)[0] = 0xD8380002u;
    }
}
