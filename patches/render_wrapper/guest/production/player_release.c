/* Original F482C/F759C calls and completion stores, with per-slot lifetime
 * guards spanning post-free A91F4. Each seam restores only its own new bit. */
#include "common.h"
#include "overlays/ba/playerstate.h"
#include "core2/1E7BFA0.h"
#include "wrapper_policy.h"
#define TOOIE_RECOMP_PATCH __attribute__((section(".recomp_patch")))

extern PlayerState* D_80135490[8];
extern u8 D_801354B0[8], D_801354D9;
void func_800F3880(s32), func_800A91F4(s32), func_800F82C0(s32);
void func_80101238(s32, s32);

TOOIE_RECOMP_PATCH void func_800F482C(s32 filter) {
    s32 slot;
    for (slot = 0; slot < 8; ++slot) {
        u32 bit, previousOwnBit;
        if (!D_80135490[slot]) continue;
        if (filter && (s32)func_800A3274(D_80135490[slot]) != filter) continue;
        bit = 1u << slot;
        previousOwnBit = tooie_wrapper_state.releaseMask & bit;
        tooie_wrapper_state.releaseMask |= bit;
        func_800F3880((s32)(u32)D_80135490[slot]);
        func_800A91F4(slot);
        D_80135490[slot] = NULL;
        if (!previousOwnBit) tooie_wrapper_state.releaseMask &= ~bit;
    }
}

TOOIE_RECOMP_PATCH void func_800F759C(s32 slot) {
    u32 bit, previousOwnBit;
    if (D_801354D9) {
        func_800F82C0(slot);
        return;
    }
    func_80101238(0xB5, slot);
    /* Valid slot is the original caller contract. New shifts are bounded even
     * on malformed input; original indexing behavior is retained. */
    bit = (u32)slot < TOOIE_WRAPPER_SLOTS ? 1u << (u32)slot : 0;
    previousOwnBit = tooie_wrapper_state.releaseMask & bit;
    tooie_wrapper_state.releaseMask |= bit;
    func_800F3880((s32)(u32)D_80135490[slot]);
    func_800A91F4(slot);
    D_80135490[slot] = NULL;
    D_801354B0[slot] = 0;
    if (!previousOwnBit) tooie_wrapper_state.releaseMask &= ~bit;
}
