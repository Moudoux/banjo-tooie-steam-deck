/* Guest-only glcut/intro-ticker policy; no renderer, camera interpolation or optional feature state. */
#include <ultra64.h>
#include "minimum_presentation_abi.h"
#if !defined(TOOIE_BASELINE_HOST) || (TOOIE_BASELINE_HOST!=0 && TOOIE_BASELINE_HOST!=1)
#error Explicit boolean TOOIE_BASELINE_HOST selection required
#endif
#if TOOIE_BASELINE_HOST
#define HOST_HELPER __attribute__((section(".baseline_host_helpers"), noinline))
/* Two independent original causes, either of which suppresses wide output:
 * glcut command2 until command1/4/5, and one claimed eligible chintroticker
 * actor until its destruction. Neither owner's event writes the other field. */
struct BaselineCutsceneState { u32 generation, glcut, ticker_owner; };
__attribute__((section(".baseline_host_state"), aligned(4)))
struct BaselineCutsceneState tooie_baseline_cutscene_state={0,0,0};
_Static_assert(sizeof(struct BaselineCutsceneState)==12,"baseline cutscene state extent");
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
        state->generation=generation;state->glcut=0;state->ticker_owner=0;
    }
    return settings;
}

static __attribute__((always_inline)) inline void apply_widescreen(u32 settings) {
    u32 request=(settings>>BASELINE_PRESENT_WIDE_REQUEST_SHIFT)&BASELINE_PRESENT_WIDE_REQUEST_MASK;
    /* Original GFLAG_BB9_WIDESCREEN_ENABLED, resident global storage. */
    u32 base=request==BASELINE_PRESENT_USE_GAME ? func_800DA298(0xBB9)!=0 :
        request==BASELINE_PRESENT_FORCE_ON;
    /* Nonzero while either original cutscene cause holds. */
    u32 cause=tooie_baseline_cutscene_state.glcut|tooie_baseline_cutscene_state.ticker_owner;
    u32 suppress=cause &&
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
    case 2:tooie_baseline_cutscene_state.glcut=1;break;
    case 1:case 4:case 5:tooie_baseline_cutscene_state.glcut=0;break;
    default:break;
    }
    apply_widescreen(settings);
}

/* Original chintroticker (descriptor identifiers 2C5/374): +0x34 init 802DC
 * stores the map classifier in actor+0x79 bits4-7, +0x0C update 805AC reads it
 * at 80614 (s3 is the actor), and +0x28 destruction 803BC runs once from FF3EC
 * before the actor is released. Eligible: mode2 (intro maps A1-AC) and the
 * listed cutscene groups3-5. Mode1 (startup 18B) also carries the title Start
 * acceptance and keeps the gameplay aspect. One owner: another live eligible
 * ticker claims at its next update after the owner retires. Claim applies at
 * once, like glcut command2, from the same ticker update that calls glcut. */
HOST_HELPER void tooie_baseline_cutscene_ticker_claim(u32 actor) {
    struct BaselineCutsceneState* state=&tooie_baseline_cutscene_state;
    u32 mode,settings;
    if(state->ticker_owner || !actor)return;
    mode=*(volatile u8*)(actor+0x79u)>>4;
    if(mode<2u || mode>5u)return;
    settings=host_settings();
    if(!settings)return;
    state->ticker_owner=actor;
    apply_widescreen(settings);
}

/* State only. Destruction runs inside FF3EC actor teardown; the per-frame owner
 * (tooie_baseline_widescreen_frame at camera construction begin) applies the
 * restored aspect, as it does for every live widescreen change. */
HOST_HELPER void tooie_baseline_cutscene_ticker_retire(u32 actor) {
    struct BaselineCutsceneState* state=&tooie_baseline_cutscene_state;
    if(state->ticker_owner==actor)state->ticker_owner=0;
}
#endif
