/* Producer-owned projection extension; original signed fields and f0 ABI. */
#include <ultra64.h>
#include "widescreen_settings_abi.h"

#define TOOIE_RECOMP_PATCH __attribute__((section(".recomp_patch")))

/* Only the consumed offsets are named; this is not a reconstructed viewport. */
typedef struct {
    u8 unused_at_0[8];
    s16 signed_at_8;
    s16 signed_at_A;
} TooieAspectInputs;

extern s16 widescreen_enabled;
extern f32 D_80041710;

_Static_assert(sizeof(void*) == 4 && sizeof(s16) == 2 &&
               sizeof(u32) == 4 && sizeof(f32) == 4 && sizeof(f64) == 8,
               "projection o32 scalar ABI");
_Static_assert(__builtin_offsetof(TooieAspectInputs, signed_at_8) == 8 &&
               __builtin_offsetof(TooieAspectInputs, signed_at_A) == 10 &&
               sizeof(TooieAspectInputs) == 12,
               "projection authenticated input offsets");

TOOIE_RECOMP_PATCH f32 func_80015D14(const TooieAspectInputs* input) {
    s16 game_wide = widescreen_enabled;
    f32 original = (f32)input->signed_at_8 / (f32)input->signed_at_A;
    u32 settings;
    u32 aspect;
    f64 scale;

    /* Original live game policy, including scripted 4:3, remains guest-owned.
       Both original arithmetic operations round as f32 before extension. */
    if (game_wide != 0) {
        original = original * D_80041710;
    }
    settings = tooie_get_latched_widescreen_settings();
    if (game_wide == 0 ||
        (settings & (TOOIE_WIDESCREEN_SETTINGS_ACTIVE |
                     TOOIE_WIDESCREEN_SETTINGS_ENABLED)) !=
        (TOOIE_WIDESCREEN_SETTINGS_ACTIVE |
         TOOIE_WIDESCREEN_SETTINGS_ENABLED)) {
        return original;
    }

    /* Exact binary64 values of the accepted host expression
       (double(n) / d) / (16.0 / 9.0); 21:9 must not become 21/16. */
    aspect = (settings >> TOOIE_WIDESCREEN_SETTINGS_ASPECT_SHIFT) &
             TOOIE_WIDESCREEN_SETTINGS_ASPECT_MASK;
    if (aspect == 0u) {
        return original;
    }
    if (aspect == 1u) {
        scale = 0x1.5000000000001p+0;
    } else if (aspect == 2u) {
        scale = 0x1.0000000000000p+1;
    } else if (aspect == 4u) {
        scale = 0x1.ccccccccccccep-1; /* 16:10 */
    } else {
        scale = 0x1.5800000000000p+0;
    }
    return (f32)((f64)original * scale);
}
