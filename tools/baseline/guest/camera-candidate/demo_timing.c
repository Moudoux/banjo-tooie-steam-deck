/* Guest-owned recorded divisor for the original replay wait. */
#include "demo_timing.h"
typedef unsigned int u32;
typedef int s32;
__attribute__((section(".tooie_demo_timing_state")))
struct DemoTimingState tooie_demo_timing_state = {0, 0};
#define DEMO_HELPER __attribute__((section(".tooie_demo_timing_helpers")))

DEMO_HELPER void tooie_demo_begin(void) {
    tooie_demo_timing_state.pending = 0;
    tooie_demo_timing_state.active = 0;
}

DEMO_HELPER void tooie_demo_record(s32 divisor, s32 available) {
    /* Original D8FA0 calls F0D90 with signed bounds 1 and 15. */
    tooie_demo_timing_state.pending = !available ? 0 :
        divisor < 1 ? 1 : divisor > 15 ? 15 : (u32)divisor;
}

DEMO_HELPER s32 tooie_demo_wait(s32 original, s32 first) {
    u32 active;
    if (first) {
        tooie_demo_timing_state.active = tooie_demo_timing_state.pending;
        tooie_demo_timing_state.pending = 0;
    }
    active = tooie_demo_timing_state.active;
    return active != 0 ? (s32)active : original;
}
