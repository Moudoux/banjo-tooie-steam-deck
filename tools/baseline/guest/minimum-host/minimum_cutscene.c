/* Guest-only glcut policy; no renderer, camera interpolation or optional feature state. */
#include <ultra64.h>
#include "minimum_presentation_abi.h"
#if !defined(TOOIE_BASELINE_HOST) || (TOOIE_BASELINE_HOST!=0 && TOOIE_BASELINE_HOST!=1)
#error Explicit boolean TOOIE_BASELINE_HOST selection required
#endif
#if TOOIE_BASELINE_HOST
#define HOST_HELPER __attribute__((section(".baseline_host_helpers"), noinline))
struct BaselineCutsceneState { u32 generation, active; };
__attribute__((section(".baseline_host_state"), aligned(4)))
struct BaselineCutsceneState tooie_baseline_cutscene_state={0,0};
_Static_assert(sizeof(struct BaselineCutsceneState)==8,"baseline cutscene state extent");
extern u32 tooie_baseline_presentation_settings(void);
extern void tooie_baseline_presentation_publish(u32 generation,u32 flags);
extern void set_widescreen(s32 enabled);
extern s32 func_800DA298(s32 flag);
extern s16 widescreen_enabled;

static __attribute__((always_inline)) inline u32 host_settings(void) {
    u32 settings=tooie_baseline_presentation_settings();
    u32 generation=settings>>BASELINE_PRESENT_GENERATION_SHIFT;
    struct BaselineCutsceneState* state=&tooie_baseline_cutscene_state;
    if(!generation)return 0;
    if(!state->generation) {
        state->generation=generation;state->active=0;
    }
    return settings;
}

static __attribute__((always_inline)) inline void apply_widescreen(u32 settings) {
    u32 request=(settings>>BASELINE_PRESENT_WIDE_REQUEST_SHIFT)&BASELINE_PRESENT_WIDE_REQUEST_MASK;
    /* Original GFLAG_BB9_WIDESCREEN_ENABLED, resident global storage. */
    u32 base=request==BASELINE_PRESENT_USE_GAME ? func_800DA298(0xBB9)!=0 :
        request==BASELINE_PRESENT_FORCE_ON;
    u32 suppress=tooie_baseline_cutscene_state.active &&
        (settings&(BASELINE_PRESENT_ORIGINAL_CUTSCENE|BASELINE_PRESENT_PILLARBOX_CAPABLE))==
        (BASELINE_PRESENT_ORIGINAL_CUTSCENE|BASELINE_PRESENT_PILLARBOX_CAPABLE);
    u32 effective=base && !suppress;
    if(widescreen_enabled!=effective)set_widescreen(effective);
    tooie_baseline_presentation_publish(settings>>BASELINE_PRESENT_GENERATION_SHIFT,
        (base && suppress ? BASELINE_PUBLISH_PILLARBOX : 0u) |
        (effective ? BASELINE_PUBLISH_WIDE : 0u));
}

HOST_HELPER void tooie_baseline_widescreen_frame(void) {
    u32 settings=host_settings();
    if(settings)apply_widescreen(settings);
}

HOST_HELPER void tooie_baseline_cutscene_command(u32 command) {
    u32 settings=host_settings();
    if(!settings)return;
    switch(command) {
    case 2:tooie_baseline_cutscene_state.active=1;break;
    case 1:case 4:case 5:tooie_baseline_cutscene_state.active=0;break;
    default:break;
    }
    apply_widescreen(settings);
}
#endif
