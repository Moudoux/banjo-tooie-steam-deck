/* Original Honeycarrier draw, with a bounded RT64 label transport experiment. */
#ifndef F3DEX_GBI_2
#define F3DEX_GBI_2
#endif
#include "ch/honeycarrier.h"
#include "rt64_extended_gbi.h"

#ifndef TOOIE_HONEY_LABEL_EMIT
#define TOOIE_HONEY_LABEL_EMIT 0
#endif

extern s32* func_800A7180(void);
extern s32 func_800A89F8(void);
extern s32 func_800EB854(void);
extern u32 D_8007BCE8[2];
extern s16 D_8007BEDA;
extern u32 D_8007BCD0[2];
extern Unk80132ED0 D_80132ED0[200];

typedef char honey_pointer_size_check[sizeof(void*) == 4 ? 1 : -1];
typedef char honey_s32_size_check[sizeof(s32) == 4 ? 1 : -1];
typedef char honey_descriptor_size_check[sizeof(Unk80132ED0) == 0x30 ? 1 : -1];
typedef char honey_command_size_check[sizeof(GfxCommand) == 8 ? 1 : -1];

__attribute__((section(".recomp_patch")))
void func_80800380_chhoneycarrier(Actor* arg0, void* arg1)
{
#if TOOIE_HONEY_LABEL_EMIT
    /* @recomp: bracket this normal-pool model using public matrix-group commands. */
    Unk80132ED0* descriptor = arg0->unk0;
    u32 slot = descriptor - D_80132ED0;
    u32 serial = (descriptor->unk10 >> 1) & 0x7FFF;
    u32 id = 0x40000000u | (slot << 15) | serial;
    s32* gfx_store = func_800A7180();
    s16 index = D_8007BEDA;
    u32 current = (u32)*gfx_store;
    u32 base = index >= 0 && index < 2 ? D_8007BCD0[index] : 0;
    s32 admitted = func_800A89F8() == 0 && func_800EB854() == 0 &&
        D_8007BCE8[0] == 0 && descriptor->unk14 == 0x612 &&
        index >= 0 && index < 2 && current >= base &&
        current - base <= 0x8FC0u - 32;
    GfxCommand* cursor;

    if (admitted) {
        cursor = (GfxCommand*)(u32)*gfx_store;
        gEXEnable(cursor);
        gEXMatrixGroupDecomposed(cursor + 1, id, G_EX_PUSH, 0,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
            G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP,
            G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP,
            G_EX_ORDER_LINEAR, G_EX_EDIT_NONE,
            G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP);
        *gfx_store = (s32)(u32)(cursor + 3);
    }

#endif

    func_80101870(arg0, arg1);

#if TOOIE_HONEY_LABEL_EMIT

    if (admitted) {
        cursor = (GfxCommand*)(u32)*gfx_store;
        gEXPopMatrixGroup(cursor, 0);
        *gfx_store = (s32)(u32)(cursor + 1);
    }
#endif
    if (arg0->unk7C_12) {
        func_80103110(arg0, 1);
    }
}
