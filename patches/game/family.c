/* Pinned core2/1EC4CC0 and 1EE4020 bodies; @recomp additions assign instance
 * births and label only the source-identified minisubgame model family. */
#define F3DEX_GBI_2
#include "common.h"
#include "rt64_extended_gbi.h"
#include "render_abi.h"
#define TOOIE_RECOMP_PATCH __attribute__((section(".recomp_patch")))
void bzero(void*, int) __asm__("_bzero");

volatile struct TooieFamilyBirthState tooie_family_birth_state
    __attribute__((section(".tooie_family_birth_state"), aligned(4))) = {{0}, 1};
extern volatile GfxCommand tooie_gfx_pools[2][TOOIE_GFX_POOL_COMMANDS];
_Static_assert(sizeof(u32) == 4 && sizeof(void*) == 4, "guest o32 ABI");
_Static_assert(sizeof(struct TooieFamilyBirthState) == 804 &&
    __builtin_offsetof(struct TooieFamilyBirthState, cursor) == 800, "birth layout");
_Static_assert(sizeof(Unk80132ED0) == 0x30 && __builtin_offsetof(Actor, unk0) == 0,
               "original descriptor layout");
extern Unk80132ED0 D_80132ED0[200];
extern u8 D_80135450[25];
extern u32 D_8007BCD0[2], D_8007BCE8[2];
extern s16 D_8007BEDA;
Unk80132ED0* func_800EBD78(void);
s32 func_800EC854(void), func_800A89F8(void), func_800EB854(void);
void func_800EBB50(void), func_800EBE74(void*, Unk80132ED0*, s32);
void* func_80100368(Actor*);
s32* func_800A7180(void);

TOOIE_RECOMP_PATCH Unk80132ED0* func_800EBED4(void* arg0, s32 arg1, s32 arg2, s32 arg3) {
    Unk80132ED0* descriptor = func_800EBD78();
    u8* bytes = (u8*)descriptor;
    u32 serial, slot, birth;
    bzero(descriptor, 0x30);
    serial = (u32)func_800EC854();
    *(u16*)(bytes + 0x12) = (u16)((serial << 1) | (*(u16*)(bytes + 0x12) & 1u));
    *(u16*)(bytes + 0x24) = (u16)(((u32)arg2 << 6) | (*(u16*)(bytes + 0x24) & 0x3Fu));
    bytes[0x13] = (u8)(((u32)arg3 & 1u) | (bytes[0x13] & 0xFEu));
    *(u32*)(bytes + 8) = (u32)func_800EBB50;
    bytes[0x2A] |= 2u;
    // @recomp Assign this allocation's birth before EBE74 publishes its links.
    slot = ((u32)descriptor - (u32)D_80132ED0) / 0x30u;
    if (descriptor != 0 && slot < TOOIE_FAMILY_BIRTH_COUNT) {
        birth = tooie_family_birth_state.cursor;
        tooie_family_birth_state.births[slot] = birth <= TOOIE_FAMILY_MAX_BIRTH ? birth : 0u;
        if (birth <= TOOIE_FAMILY_MAX_BIRTH) tooie_family_birth_state.cursor = birth + 1u;
    }
    func_800EBE74(arg0, descriptor, arg1);
    return descriptor;
}

TOOIE_RECOMP_PATCH void func_8010A79C(Actor* actor, void* arg1) {
    u8* bytes = (u8*)actor;
    void* actor_template;
    void (*draw)(Actor*, void*);
    s32* gfx_store = 0;
    GfxCommand* cursor;
    s32 admitted = 0;
    if (*(s16*)(bytes + 0x7E) >= 0 || (*(u32*)(bytes + 0x64) & 0x00020000u) != 0 ||
        (*(u16*)(bytes + 0x64) & 1u) == 0) return;
    actor_template = func_80100368(actor);
    draw = *(void (**)(Actor*, void*))((u8*)actor_template + 0x14);
    // @recomp Authored template identity; no absolute overlay data import.
    if ((*(u32*)actor_template & 0xFFFFu) == 0x4AEu && (u32)draw == 0x80101870u &&
        (bytes[0x77] & 0x10u) == 0 && (*(u16*)(bytes + 0x7E) & 0x0FFFu) == 0) {
        u32 address = (u32)actor->unk0;
        u32 offset = address - (u32)D_80132ED0;
        if (address >= (u32)D_80132ED0 && offset < 200u * 0x30u && offset % 0x30u == 0) {
            u32 slot = offset / 0x30u;
            u32 birth = tooie_family_birth_state.births[slot];
            u16 model = *(u16*)((u8*)actor->unk0 + 0x14);
            s32 view = func_800A89F8();
            s16 index = D_8007BEDA;
            if ((D_80135450[slot >> 3] & (0x80u >> (slot & 7u))) != 0 &&
                model >= 0x76Eu && model <= 0x770u && birth != 0 &&
                birth <= TOOIE_FAMILY_MAX_BIRTH && func_800EB854() == 0 &&
                view >= 0 && view < 4 && index >= 0 && index < 2 && D_8007BCE8[0] == 0) {
                u32 base = (u32)tooie_gfx_pools[index];
                u32 end = base + TOOIE_GFX_POOL_BYTES;
                gfx_store = func_800A7180();
                cursor = (GfxCommand*)(u32)*gfx_store;
                offset = (u32)cursor - base;
                if (D_8007BCD0[index] == base && (u32)cursor >= base + 8 &&
                    (u32)cursor <= end && (offset & 7u) == 0 &&
                    tooie_gfx_pools[index][0].values.word0 == TOOIE_POLICY_WORD0 &&
                    tooie_gfx_pools[index][0].values.word1 == (TOOIE_POLICY_MAGIC | TOOIE_POLICY_MODEL_LABELS) &&
                    end - (u32)cursor >= TOOIE_GFX_ORIGINAL_BYTES + 32u + 8u * (offset / 24u)) {
                    // @recomp Reserve original commands and every pending POP.
                    u32 id = TOOIE_FAMILY_ID_BASE + (birth << 2) + (u32)view;
                    gEXEnable(cursor);
                    gEXMatrixGroup(cursor + 1, id, G_EX_INTERPOLATE_SIMPLE, G_EX_PUSH, 0,
                        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_AUTO,
                        G_EX_ORDER_LINEAR, G_EX_EDIT_NONE, G_EX_ASPECT_AUTO,
                        G_EX_COMPONENT_SKIP, G_EX_COMPONENT_AUTO);
                    *gfx_store = (s32)(u32)(cursor + 3);
                    admitted = 1;
                }
            }
        }
    }
    draw(actor, arg1);
    if (admitted) {
        cursor = (GfxCommand*)(u32)*gfx_store;
        gEXPopMatrixGroup(cursor, 0);
        *gfx_store = (s32)(u32)(cursor + 1);
    }
}
