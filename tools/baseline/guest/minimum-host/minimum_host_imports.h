#pragma once
// Declarations only: native absolute imports (0x8F00xxxx) called from generated boot.c.
#include "recomp.h"
#ifdef __cplusplus
extern "C" {
#endif
void tooie_get_latched_widescreen_settings(uint8_t*,recomp_context*);
void tooie_baseline_presentation_settings(uint8_t*,recomp_context*);
void tooie_baseline_presentation_publish(uint8_t*,recomp_context*);
void tooie_baseline_quiet_talon_trot(uint8_t*,recomp_context*);
void tooie_baseline_trot_toggle(uint8_t*,recomp_context*);
void tooie_baseline_music_volume(uint8_t*,recomp_context*);
void baseline_npc665_end_checked(uint8_t*,recomp_context*);
void tooie_baseline_first_person_aim(uint8_t*,recomp_context*);
void tooie_bridge_required_stop(uint8_t*,recomp_context*);
#ifdef __cplusplus
}
#endif
