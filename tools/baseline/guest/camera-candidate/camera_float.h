#ifndef TOOIE_CAMERA_FLOAT_H
#define TOOIE_CAMERA_FLOAT_H
/* BK float camera route for Tooie WORLD productions (six-F4 issues 2/3).
 *
 * Adapted from BanjoRecomp dbd52154bc1e00e56867b1fac94a8f907052f41b
 * patches/camera_transform_tagging.c viewport_setRenderPerspectiveMatrix: BK LOADs a pure
 * perspective into PROJECTION and MULs affine rotations after it, so RT64 RSP::matrixCommon files
 * the first as projection (isMatrixViewProj false) and the MULs as view (isMatrixAffine, not
 * identity) and never decomposes a combined matrix. Keep this attribution and the upstream licence
 * notice with the adapted code.
 *
 * @tooie Tooie's models are world-absolute (DE498 builds the model from the object position through
 * 80019750), so the RT64 extended view stays identity: BK's positive-camera gEXSetViewMatrixFloat
 * compensates BK's camera-relative models and is not copied. The factors are E443C's own:
 *   P  CB124's pure perspective (P33 = 0) after the host raster correction, as E443C consumed it;
 *   R  camera+0xA4 (CA7AC), the affine rotation CAF34/D965C wrote and E443C premultiplied;
 *   A  T(-p) * S(1/s): diagonal xyz 1/s, last row xyz -p/s, p from CA7E4 and s the effective scale
 *      E44FC selected (doubled after the 80018BC4 overflow branch).
 * E443C builds A * R * P (row vectors; 80018C50 premultiplies), so V = A * R and V * P is the
 * original product before its fixed conversion. */
#ifdef TOOIE_CAMERA_GUEST
#include "common.h"
typedef f32 TooieCameraFloat32;
#else
typedef float TooieCameraFloat32;
#endif
#include "camera_identity.h"

/* One production's block: three consecutive 64-byte Mtx arena slots after the production's two
 * original matrices. RT64 copies the floats when it parses the commands. */
#define TOOIE_CAMERA_FLOAT_BYTES 192u
#define TOOIE_CAMERA_FLOAT_VIEW 64u
#define TOOIE_CAMERA_FLOAT_COMMANDS 128u
struct TooieCameraFloat {
    TooieCameraFloat32 projection[16];  /* P, row-major m[row][col] (RSP::matrixFloat order) */
    TooieCameraFloat32 view[16];        /* V = A * R */
    TooieCameraWord commands[10];       /* LOAD P, MUL V, end */
    TooieCameraWord reserved[6];
};
/* gEXMatrixFloat (rt64_extended_gbi.h): opcode word, the F3DEX2 gSPMatrix parameter byte (RT64
 * matrixFloatV1 XORs pushMask exactly as F3DEX2 matrix does), zero, RSP address. */
#define TOOIE_CAMERA_FLOAT_OPCODE 0x64000030u
#define TOOIE_CAMERA_FLOAT_LOAD 0x07u   /* PROJECTION|LOAD|NOPUSH, as the original 0xDA380007 */
#define TOOIE_CAMERA_FLOAT_MUL 0x05u    /* PROJECTION|MUL|NOPUSH */
#define TOOIE_CAMERA_FLOAT_END 0xDF000000u

static inline TooieCameraWord cam_float_bits(TooieCameraFloat32 value) {
    union { TooieCameraFloat32 f; TooieCameraWord u; } bits;
    bits.f = value;
    return bits.u;
}

/* Fills `b` for one production and returns 1, or returns 0 without using the block when an input
 * is not the shape the route needs: s positive finite, P33 = 0 (CB124 forces it) and R's last
 * column exactly (0,0,0,1) (D965C stores it), which makes V's last column exactly (0,0,0,1) as
 * RT64 isMatrixAffine requires. `block_rsp` is the block's RSP address. */
static inline TooieCameraWord cam_float_build(volatile struct TooieCameraFloat* b, TooieCameraWord block_rsp,
    const TooieCameraFloat32 projection[16], const volatile TooieCameraFloat32 r[16],
    const TooieCameraFloat32 p[3], TooieCameraFloat32 s) {
    TooieCameraWord i, j, k, sbits = cam_float_bits(s);
    TooieCameraFloat32 inverse, a[16], sum;
    if ((sbits & 0x80000000u) || !sbits || sbits >= 0x7F800000u) return 0;
    if (cam_float_bits(projection[15]) & 0x7FFFFFFFu) return 0;
    if (((cam_float_bits(r[3]) | cam_float_bits(r[7]) | cam_float_bits(r[11])) & 0x7FFFFFFFu) ||
        cam_float_bits(r[15]) != 0x3F800000u) return 0;
    /* Refuse nonfinite source values; signed zero remains valid in affine components. */
    for (i = 0; i < 16u; i++)
        if ((cam_float_bits(projection[i]) & 0x7F800000u) == 0x7F800000u ||
            (cam_float_bits(r[i]) & 0x7F800000u) == 0x7F800000u) return 0;
    for (i = 0; i < 3u; i++)
        if ((cam_float_bits(p[i]) & 0x7F800000u) == 0x7F800000u) return 0;
    inverse = 1.0f / s;
    if ((cam_float_bits(inverse) & 0x7F800000u) == 0x7F800000u) return 0;
    /* E443C forms A before multiplication. Keep its negate-then-scale operation. */
    for (i = 0; i < 16u; i++) a[i] = 0.0f;
    a[0] = a[5] = a[10] = inverse; a[15] = 1.0f;
    for (i = 0; i < 3u; i++) {
        a[12u + i] = -p[i] * inverse;
        if ((cam_float_bits(a[12u + i]) & 0x7F800000u) == 0x7F800000u) return 0;
    }
    for (i = 0; i < 16u; i++) b->projection[i] = projection[i];
    /* 18C50's ordered four-term row/column accumulation, applied to A * R. */
    for (i = 0; i < 4u; i++) {
        for (j = 0; j < 4u; j++) {
            sum = 0.0f;
            for (k = 0; k < 4u; k++) sum += a[i * 4u + k] * r[k * 4u + j];
            if ((cam_float_bits(sum) & 0x7F800000u) == 0x7F800000u) return 0;
            b->view[i * 4u + j] = sum;
        }
    }
    b->commands[0] = TOOIE_CAMERA_FLOAT_OPCODE; b->commands[1] = TOOIE_CAMERA_FLOAT_LOAD;
    b->commands[2] = 0u; b->commands[3] = block_rsp;
    b->commands[4] = TOOIE_CAMERA_FLOAT_OPCODE; b->commands[5] = TOOIE_CAMERA_FLOAT_MUL;
    b->commands[6] = 0u; b->commands[7] = block_rsp + TOOIE_CAMERA_FLOAT_VIEW;
    b->commands[8] = TOOIE_CAMERA_FLOAT_END; b->commands[9] = 0u;
    for (i = 0; i < 6u; i++) b->reserved[i] = 0u;
    return 1;
}

#ifdef __cplusplus
static_assert(sizeof(struct TooieCameraFloat) == TOOIE_CAMERA_FLOAT_BYTES &&
    offsetof(struct TooieCameraFloat, view) == TOOIE_CAMERA_FLOAT_VIEW &&
    offsetof(struct TooieCameraFloat, commands) == TOOIE_CAMERA_FLOAT_COMMANDS, "camera float block");
#else
_Static_assert(sizeof(struct TooieCameraFloat) == TOOIE_CAMERA_FLOAT_BYTES &&
    __builtin_offsetof(struct TooieCameraFloat, view) == TOOIE_CAMERA_FLOAT_VIEW &&
    __builtin_offsetof(struct TooieCameraFloat, commands) == TOOIE_CAMERA_FLOAT_COMMANDS, "camera float block");
#endif
#endif
