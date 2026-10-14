#pragma once
#include "recomp.h"
#ifdef __cplusplus
extern "C" {
#endif
void tooie_get_latched_widescreen_settings(uint8_t*,recomp_context*);
void tooie_baseline_presentation_settings(uint8_t*,recomp_context*);
void tooie_baseline_presentation_publish(uint8_t*,recomp_context*);
#ifdef __cplusplus
}
#endif
