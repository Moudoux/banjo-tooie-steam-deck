/* Startup-latched Talon Trot movement-slot volume policy; original OFF call retained. */
#include "common.h"
typedef struct player_state_s PlayerState;
extern void func_8009DE38(PlayerState*, s32, f32);
extern void func_8009DCCC(PlayerState*, s32, s32, f32, s32);
extern u32 tooie_baseline_quiet_talon_trot(void); /* proposed 0x8F00002C */
_Static_assert(sizeof(void*) == 4 && sizeof(u32) == 4 && sizeof(f32) == 4,
    "original guest o32 ABI required");

/* Typed recovery of 9DE38's argument preparation, with volume exposed.
 * volume=0x55F0 recovers the original; volume=0 is the proposed ON path.
 * No validity guard: the original assumes self and player+104 are valid.
 */
static inline void fb05_slot_call(PlayerState* self, s32 sound, f32 pitch, s32 volume) {
    const u8* state104 = *(const u8* const*)((const u8*)self + 0x104u);
    func_8009DCCC(self, (s32)state104[2], sound, pitch, volume);
}

__attribute__((section(".recomp_patch"), noinline))
void func_808000DC_bsbtrot(PlayerState* self, s32 selector) {
    union { u32 word; f32 value; } pitch;
    pitch.word = selector ? 0x3F75C28Fu : 0x3F851EB8u;
    if (!tooie_baseline_quiet_talon_trot()) {
        func_8009DE38(self, 0x4433, pitch.value);
        return;
    }
    fb05_slot_call(self, 0x4433, pitch.value, 0);
}
