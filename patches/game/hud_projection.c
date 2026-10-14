/* Adapted from pinned core2/1EBCFC0.c:162-199. @recomp marks policy change. */
#define F3DEX_GBI_2
#include <ultra64.h>
#include <PR/gbi.h>
#include "gfx.h"
#define TOOIE_RECOMP_PATCH __attribute__((section(".recomp_patch")))

typedef struct {
    Gfx* unk0;
    Mtx* unk4;
} Unkfunc_800E44FC;

extern Mtx* D_8012D554;
extern u32 tooie_get_hud_half_width_bits(void);
_Static_assert(sizeof(u32) == sizeof(f32) && sizeof(f32) == 4,
               "HUD float representation ABI");
_Static_assert(sizeof(void*) == 4 && sizeof(Unkfunc_800E44FC) == 8 &&
               sizeof(Gfx) == 8 && sizeof(Mtx) == 64, "HUD graphics ABI");

TOOIE_RECOMP_PATCH void func_800E42F0(Unkfunc_800E44FC* arg0, s32 use_widescreen) {
    Gfx* cur_gfx = arg0->unk0;
    Mtx* cur_mtx = arg0->unk4;
    union { u32 bits; f32 value; } half_width;
    f32 left, right;
    /* @recomp Integer import transports representation, not a numeric value.
       The original live halfword remains guest-owned, independently of the
       host latched settings and caller's use_widescreen argument. */
    half_width.bits = tooie_get_hud_half_width_bits();
    if (widescreen_enabled && half_width.bits != 0) {
        left = -half_width.value;
        right = half_width.value;
    } else if (use_widescreen && widescreen_enabled) {
        left = -(s32)(SCREEN_WIDTH * 2 * WIDESCREEN_ADJUSTMENT);
        right = (s32)(SCREEN_WIDTH * 2 * WIDESCREEN_ADJUSTMENT);
    } else {
        left = -SCREEN_WIDTH * 2;
        right = SCREEN_WIDTH * 2;
    }
    guOrtho(cur_mtx, left, right, -SCREEN_HEIGHT * 2, SCREEN_HEIGHT * 2,
            -50.0f, 50.0f, 1.0f);
    gSPMatrix(cur_gfx++, OS_K0_TO_PHYSICAL(cur_mtx++),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    D_8012D554 = cur_mtx - 1;
    guMtxIdent(cur_mtx);
    gSPMatrix(cur_gfx++, OS_K0_TO_PHYSICAL(cur_mtx++),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    arg0->unk0 = cur_gfx;
    arg0->unk4 = cur_mtx;
}
