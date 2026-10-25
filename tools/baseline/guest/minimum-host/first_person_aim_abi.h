#pragma once
/* First-person aim native0x8F000038, o32 u64:
 * high word = signed16 X in31..16, signed16 Y in15..0, each[-32767,32767].
 * low word = admitted policy. Zero means unavailable; ENABLED with sample0
 * means admitted neutral and MUST keep independent modern controls active.
 * Policy bits1/2 invertX/Y, bits15..8 gain percent25..200; all others zero. */
#define TOOIE_FP_AIM_ENABLED 0x1u
#define TOOIE_FP_AIM_INVERT_X 0x2u
#define TOOIE_FP_AIM_INVERT_Y 0x4u
#define TOOIE_FP_AIM_SENSITIVITY_SHIFT 8
#define TOOIE_FP_AIM_SENSITIVITY_MIN 25
#define TOOIE_FP_AIM_SENSITIVITY_MAX 200
#define TOOIE_FP_AIM_SENSITIVITY_DEFAULT 100
