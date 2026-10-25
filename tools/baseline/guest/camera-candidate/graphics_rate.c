/* Public input-rate hint from the divisor this frame's original wait will use. */
#ifndef F3DEX_GBI_2
#define F3DEX_GBI_2
#endif
#include <ultra64.h>
#include "rt64_extended_gbi.h"
#include "demo_timing.h"
#define RATE_PATCH __attribute__((section(".recomp_patch"), noinline))
extern s16 D_8007BEDA;
extern s32 func_80015184(void);
extern void func_800A9800(void);
/* @recomp Original E7EF4 emits sync/end inline, then calls func_800A9800. This replacement
 * writes a divisor-derived refresh hint plus sync/end into this per-slot tail, emits a
 * public Enable and branches to the tail; the original completion call is kept. */
__attribute__((section(".tooie_graphics_rate_state"), aligned(8)))
Gfx tooie_graphics_rate_tail[2][3] = {{{0}}};
_Static_assert(sizeof(tooie_graphics_rate_tail)==48, "two original graphics slots");

RATE_PATCH void func_800E7EF4(Gfx** gfx) {
    /* @recomp Intended cadence, as BK's viMgr_func_8024BFA0, never the completed VI count.
     * func_800A7840 reaches this before its func_80015410 wait, so this frame's recorded
     * demo divisor is still pending; without one, the wait uses func_80015184. */
    const s32 requested=func_80015184();
    const u32 pending=tooie_demo_timing_state.pending;
    const s32 divisor=pending!=0 ? (s32)pending : requested;
    const u32 rate=divisor>=1 && divisor<=60 ? 60u/(u32)divisor : 0xFFFFu;
    /* Index stays 0/1: selected func_8001A3A0 initializes it to 0; original
     * func_8001A4B8 stores 1-index at 0x8001A4DC. Both frame-entry toggles
     * precede this tail; completion/call ordering remains unchanged. */
    Gfx* tail=tooie_graphics_rate_tail[D_8007BEDA];
    gEXSetRefreshRate(tail, rate);
    gDPFullSync(tail+1);
    gSPEndDisplayList(tail+2);
    gEXEnable((*gfx)++);
    gSPBranchList((*gfx)++, OS_K0_TO_PHYSICAL(tail));
    func_800A9800();
}
