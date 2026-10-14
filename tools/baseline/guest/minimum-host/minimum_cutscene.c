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
extern void tooie_baseline_presentation_publish(u32 generation,u32 pillarbox);
extern void set_widescreen(s32 enabled);

HOST_HELPER void tooie_baseline_cutscene_command(u32 command) {
    u32 settings=tooie_baseline_presentation_settings();
    u32 generation=settings>>BASELINE_PRESENT_GENERATION_SHIFT;
    u32 wide=(settings&BASELINE_PRESENT_WIDE)!=0;
    u32 bars=(settings&(BASELINE_PRESENT_ORIGINAL_CUTSCENE|BASELINE_PRESENT_PILLARBOX_CAPABLE))==
        (BASELINE_PRESENT_ORIGINAL_CUTSCENE|BASELINE_PRESENT_PILLARBOX_CAPABLE);
    struct BaselineCutsceneState* state=&tooie_baseline_cutscene_state;
    if(!generation)return;
    if(state->generation!=generation) {
        state->generation=generation;state->active=0;
        tooie_baseline_presentation_publish(generation,0);
    }
    if(!(settings&BASELINE_PRESENT_ACTIVE))return;
    switch(command) {
    case 2:
        state->active=1;
        if(bars && wide)set_widescreen(0);
        tooie_baseline_presentation_publish(generation,bars && wide);
        break;
    case 1:case 4:case 5: {
        u32 prior=state->active;state->active=0;
        if(prior && wide)set_widescreen(1);
        tooie_baseline_presentation_publish(generation,0);
        break;
    }
    default:break;
    }
}
#endif
