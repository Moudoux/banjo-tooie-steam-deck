#pragma once

/* Mechanical transport of the startup-wide and live gameplay-aspect preference.
   Inactive returns zero; active returns only the bits described here.
   This import is synchronous, never yields, and never enters guest code. */
#define TOOIE_WIDESCREEN_SETTINGS_ACTIVE 0x1u
#define TOOIE_WIDESCREEN_SETTINGS_ENABLED 0x2u
#define TOOIE_WIDESCREEN_SETTINGS_ASPECT_SHIFT 2u
#define TOOIE_WIDESCREEN_SETTINGS_ASPECT_MASK 0x3u
/* Baseline host only: FIT scale relative to 16:9 occupies f32 bits5..31;
   its low five mantissa bits and the legacy aspect enum are zero. */
#define TOOIE_WIDESCREEN_SETTINGS_FIT 0x10u
#define TOOIE_WIDESCREEN_SETTINGS_FIT_SCALE_MASK 0xFFFFFFE0u

#ifndef __cplusplus
#include <ultra64.h>
extern u32 tooie_get_latched_widescreen_settings(void);
#endif
