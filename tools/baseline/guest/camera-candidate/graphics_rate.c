/* Public input-rate hint from the game's preceding completed graphics period. */
#ifndef F3DEX_GBI_2
#define F3DEX_GBI_2
#endif
#include <ultra64.h>
#include "rt64_extended_gbi.h"
#define RATE_PATCH __attribute__((section(".recomp_patch"), noinline))
extern s16 D_8007BEDA;
extern s32 func_80014F4C(void);
extern void func_800A9800(void);
__attribute__((section(".tooie_graphics_rate_state"), aligned(8)))
Gfx tooie_graphics_rate_tail[2][3] = {{{0}}};
_Static_assert(sizeof(tooie_graphics_rate_tail)==48, "two original graphics slots");

RATE_PATCH void func_800E7EF4(Gfx** gfx) {
    const s32 divisor=func_80014F4C();
    const u32 rate=divisor>=1 && divisor<=60 ? 60u/(u32)divisor : 0xFFFFu;
    /* The original buffer owner guarantees index0/1 and completion before reuse. */
    Gfx* tail=tooie_graphics_rate_tail[D_8007BEDA];
    gEXSetRefreshRate(tail, rate);
    gDPFullSync(tail+1);
    gSPEndDisplayList(tail+2);
    gEXEnable((*gfx)++);
    gSPBranchList((*gfx)++, OS_K0_TO_PHYSICAL(tail));
    func_800A9800();
}
