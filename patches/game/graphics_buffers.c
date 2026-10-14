/* Original allocation, release and root construction order from the pinned
 * core1/1E31B40 and core2/1E80A70 assembly; changes marked @recomp. */
#define F3DEX_GBI_2
#include "common.h"
#include "core1/heap.h"
#include "memory.h"
#include "rt64_extended_gbi.h"
#include "render_abi.h"
#define TOOIE_RECOMP_PATCH __attribute__((section(".recomp_patch")))

volatile GfxCommand tooie_gfx_pools[2][TOOIE_GFX_POOL_COMMANDS]
    __attribute__((section(".tooie_gfx_buffers"), aligned(8))) = {{{{0, 0}}}};
_Static_assert(sizeof(void*) == 4 && sizeof(GfxCommand) == 8, "guest command ABI");
_Static_assert(sizeof(tooie_gfx_pools) == 112016 && G_NOOP == TOOIE_POLICY_WORD0,
               "guest pool/tag ABI");
extern u32 D_8007BCD0[2], D_8007BCD8[2], D_8007BCE0[2], D_8007BCE8[2];
extern s16 D_8007BEDA, D_80127634;
extern u32 D_80127658[3], D_80127650;
extern f32 D_801255F4;
void func_8001A570(void), func_800A9800(void), func_8001A63C(void);
void func_8001A4B8(u32*);
s32 func_80014F58(void), func_8001BD50(f32);
void func_800A73F4(s32, s32), func_80015410(void), func_800A7FD4(void);
void func_800E82EC(void), func_8001A270(u32, u32);
void func_80015878(void), func_800E4628(void);
extern u32 tooie_get_construction_model_policy(void);

TOOIE_RECOMP_PATCH void func_8001A3A0(void) {
    if (D_8007BCD0[0] == 0) {
        // @recomp Only Gfx moves out of the original game heap.
        D_8007BCD0[0] = (u32)tooie_gfx_pools[0];
        D_8007BCD0[1] = (u32)tooie_gfx_pools[1];
        D_8007BCD8[0] = (u32)heap_alloc(0x12C00);
        D_8007BCD8[1] = (u32)heap_alloc(0x12C00);
        D_8007BCE0[0] = (u32)heap_alloc(0x1AE0);
        D_8007BCE0[1] = (u32)heap_alloc(0x1AE0);
        func_8001A570();
    }
    D_8007BEDA = 0;
    func_800A9800();
}

TOOIE_RECOMP_PATCH void func_8001A324(void) {
    if (D_8007BCD0[0] != 0) {
        // @recomp Static Gfx has no heap allocation to release.
        heap_free((void*)D_8007BCD8[0]);
        heap_free((void*)D_8007BCD8[1]);
        heap_free((void*)D_8007BCE0[0]);
        heap_free((void*)D_8007BCE0[1]);
        D_8007BCD0[0] = 0;
    }
    func_8001A63C();
}

TOOIE_RECOMP_PATCH void func_800A7840(s32 arg0) {
    u32 begin, end;
    func_8001A4B8(D_80127658);
    if (D_80127634 != 0) func_8001A4B8(D_80127658);
    begin = D_80127658[0];
    // @recomp Include the construction snapshot in the submitted root.
    if (D_8007BCE8[0] == 0 &&
        (begin == (u32)tooie_gfx_pools[0] || begin == (u32)tooie_gfx_pools[1])) {
        u32 policy = tooie_get_construction_model_policy();
        gDPNoOpTag((Gfx*)begin, TOOIE_POLICY_MAGIC | policy);
        D_80127658[0] = begin + 8;
    }
    func_800A73F4(func_80014F58(), arg0);
    if (D_80127634 == 0) {
        end = D_80127658[0];
        func_80015410();
        if (D_80127650 == 0 && func_8001BD50(D_801255F4)) func_800A7FD4();
        func_800E82EC();
        func_8001A270(begin, end);
    }
    func_80015878();
    func_800E4628();
    D_80127634 = 0;
    D_80127650 = 0;
}
