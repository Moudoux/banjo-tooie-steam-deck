#pragma once

#include <ultra64.h>

/* Mechanical transport of the existing session-latched preference.
   Inactive returns zero; active returns only the bits described here.
   This import is synchronous, never yields, and never enters guest code. */
#define TOOIE_WIDESCREEN_SETTINGS_ACTIVE 0x1u
#define TOOIE_WIDESCREEN_SETTINGS_ENABLED 0x2u
#define TOOIE_WIDESCREEN_SETTINGS_ASPECT_SHIFT 2u
#define TOOIE_WIDESCREEN_SETTINGS_ASPECT_MASK 0x7u

extern u32 tooie_get_latched_widescreen_settings(void);
