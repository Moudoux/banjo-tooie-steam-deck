/* Accepted S1a: original EBED4 and Honey with finite public correspondence. */
#define TOOIE_BRIDGE_GUEST 1
#define F3DEX_GBI_2
#include "render_bridge.h"
#include "rt64_extended_gbi.h"
#define PATCH __attribute__((section(".recomp_patch")))
#define INLINE static inline __attribute__((always_inline))
void bzero(void*, int) __asm__("_bzero");
extern Unk80132ED0 D_80132ED0[200];
extern u32 D_8007BCD0[2], D_8007BCE8[2];
extern s16 D_8007BEDA;
Unk80132ED0* func_800EBD78(void);
s32 func_800EC854(void), func_800A89F8(void), func_800EB854(void);
void func_800EBB50(void), func_800EBE74(void*, Unk80132ED0*, s32);
s32* func_800A7180(void);
void func_80101870(Actor*, void*), func_80103110(Actor*, u32);
_Static_assert(sizeof(void*) == 4 && sizeof(Unk80132ED0) == 0x30,
    "original guest o32 descriptor ABI");
volatile struct TooieRenderBridgeState tooie_render_bridge_state
    __attribute__((section(".tooie_render_bridge_state"), aligned(4))) = {
        .magic = TOOIE_BRIDGE_MAGIC, .layout = TOOIE_BRIDGE_LAYOUT, .birth_cursor = 1
    };
INLINE void count(volatile u32* value) {
    if (*value != 0xFFFFFFFFu) *value += 1u;
}
INLINE u32 selector_pair(const Unk80132ED0* descriptor) {
    return ((u32)descriptor->unk14 << 16) | descriptor->unk16;
}
INLINE int bind_block(u32 slot, u32 birth, u32 pair, u32* id) {
    volatile struct TooieBridgeSlot* entry = &tooie_render_bridge_state.slots[slot];
    u32 block;
    if ((entry->block_and_valid & TOOIE_BRIDGE_BLOCK_VALID) &&
        entry->birth == birth && entry->selector_pair == pair) {
        *id = TOOIE_BRIDGE_ID_BASE + (entry->block_and_valid & 0x7FFFFu) * 512u + 1u;
        return 1;
    }
    entry->block_and_valid = 0;
    block = tooie_render_bridge_state.next_block;
    if (block >= TOOIE_BRIDGE_BLOCK_COUNT) return 0;
    tooie_render_bridge_state.next_block = block + 1u;
    entry->birth = birth;
    entry->selector_pair = pair;
    entry->block_and_valid = block | TOOIE_BRIDGE_BLOCK_VALID;
    *id = TOOIE_BRIDGE_ID_BASE + block * 512u + 1u;
    return 1;
}
INLINE void refuse(u32 reason, const Actor* actor) {
    const u8* bytes = (const u8*)actor;
    count(&tooie_render_bridge_state.refused[reason]);
    tooie_render_bridge_state.raw_8c = (u32)(s32)*(const s16*)(bytes + 0x8C);
    tooie_render_bridge_state.raw_94 = *(const u32*)(bytes + 0x94);
    tooie_render_bridge_state.raw_7b = bytes[0x7B];
    tooie_render_bridge_state.raw_78 = *(const u32*)(bytes + 0x78);
    tooie_render_bridge_state.raw_70 = *(const u32*)(bytes + 0x70);
}

PATCH Unk80132ED0* func_800EBED4(void* arg0, s32 arg1, s32 arg2, s32 arg3) {
    Unk80132ED0* descriptor = func_800EBD78();
    u8* bytes = (u8*)descriptor;
    u32 serial, address, offset, birth;
    bzero(descriptor, 0x30);
    serial = (u32)func_800EC854();
    *(u16*)(bytes + 0x12) = (u16)((serial << 1) | (*(u16*)(bytes + 0x12) & 1u));
    *(u16*)(bytes + 0x24) = (u16)(((u32)arg2 << 6) | (*(u16*)(bytes + 0x24) & 0x3Fu));
    bytes[0x13] = (u8)(((u32)arg3 & 1u) | (bytes[0x13] & 0xFEu));
    *(u32*)(bytes + 8) = (u32)func_800EBB50;
    bytes[0x2A] |= 2u;
    address = (u32)descriptor;
    offset = address - (u32)D_80132ED0;
    if (address >= (u32)D_80132ED0 && offset < 200u * 0x30u && offset % 0x30u == 0) {
        birth = tooie_render_bridge_state.birth_cursor;
        tooie_render_bridge_state.births[offset / 0x30u] =
            birth != 0 && birth <= TOOIE_BRIDGE_MAX_BIRTH ? birth : 0u;
        if (birth != 0 && birth <= TOOIE_BRIDGE_MAX_BIRTH) {
            tooie_render_bridge_state.birth_cursor = birth + 1u;
            count(&tooie_render_bridge_state.birth_count);
        } else count(&tooie_render_bridge_state.birth_saturated);
    }
    func_800EBE74(arg0, descriptor, arg1);
    return descriptor;
}

PATCH void func_80800380_chhoneycarrier(Actor* actor, void* arg1) {
    const u8* bytes = (const u8*)actor;
    Unk80132ED0* descriptor = actor->unk0;
    u32 epoch = tooie_render_bridge_state.epoch;
    u32 reason = TOOIE_BRIDGE_REASON_COUNT, slot = 0, birth = 0, pair = 0, id = 0;
    u32 current = 0, base = 0, offset, end, returned;
    s16 index;
    s32* store = 0;
    GfxCommand* cursor;
    int admitted = 0, owns_active = 0;
    count(&tooie_render_bridge_state.calls);
    if (tooie_render_bridge_state.active) reason = TOOIE_BRIDGE_REENTRY;
    else {
        if (epoch != tooie_render_bridge_state.seen_epoch) {
            tooie_render_bridge_state.seen_epoch = epoch;
            tooie_render_bridge_state.extra_bytes = 0;
            tooie_render_bridge_state.last_end = 0;
            tooie_render_bridge_state.poisoned = 0;
        }
        if (epoch == 0 || epoch == 0xFFFFFFFFu) reason = TOOIE_BRIDGE_EPOCH;
        else if (tooie_render_bridge_state.poisoned) reason = TOOIE_BRIDGE_POISONED;
        else {
            index = D_8007BEDA;
            if (index < 0 || index > 1) reason = TOOIE_BRIDGE_POOL;
            else if (D_8007BCE8[0] != 0) reason = TOOIE_BRIDGE_OVERRIDE;
            else {
                base = D_8007BCD0[index];
                store = func_800A7180();
                current = (u32)*store;
                if (base > 0xFFFF703Fu || current < base || current - base > 0x8FC0u)
                    reason = TOOIE_BRIDGE_RANGE;
                else if ((current & 7u) != 0 || (base & 7u) != 0) reason = TOOIE_BRIDGE_ALIGN;
                else if (current < tooie_render_bridge_state.last_end) {
                    tooie_render_bridge_state.poisoned = 1;
                    count(&tooie_render_bridge_state.poison_count);
                    reason = TOOIE_BRIDGE_CURSOR_DROP;
                } else {
                    offset = (u32)descriptor - (u32)D_80132ED0;
                    if ((u32)descriptor < (u32)D_80132ED0 || offset >= 200u * 0x30u || offset % 0x30u != 0)
                        reason = TOOIE_BRIDGE_SLOT;
                    else {
                        slot = offset / 0x30u;
                        birth = tooie_render_bridge_state.births[slot];
                        pair = selector_pair(descriptor);
                        if (birth == 0) reason = TOOIE_BRIDGE_BIRTH;
                        else if (descriptor->unk14 != 0x612u) reason = TOOIE_BRIDGE_SELECTOR;
                        else if (func_800A89F8() != 0) reason = TOOIE_BRIDGE_VIEW;
                        else if (func_800EB854() != 0) reason = TOOIE_BRIDGE_PHASE;
                        else if (*(const s16*)(bytes + 0x8C) != 0) reason = TOOIE_BRIDGE_ANIMATION;
                        else if ((*(const u32*)(bytes + 0x94) & 0x000C0000u) != 0) reason = TOOIE_BRIDGE_EXPRESSION;
                        else if ((bytes[0x7B] & 0xC0u) != 0 || (*(const u32*)(bytes + 0x78) & 2u) != 0)
                            reason = TOOIE_BRIDGE_DEFORM;
                        else if ((*(const u32*)(bytes + 0x70) & 0x01200000u) != 0) reason = TOOIE_BRIDGE_MODEL_DATA;
                        else if (tooie_render_bridge_state.slots[slot].birth == birth &&
                            tooie_render_bridge_state.slots[slot].stamp == epoch) reason = TOOIE_BRIDGE_DUPLICATE;
                        else {
                            end = base + 0x8FC0u;
                            if (tooie_render_bridge_state.extra_bytes > 6368u || end - current < 32u)
                                reason = TOOIE_BRIDGE_CAPACITY;
                            else if (!bind_block(slot, birth, pair, &id)) reason = TOOIE_BRIDGE_BLOCKS;
                            else {
                                tooie_render_bridge_state.extra_bytes += 32u;
                                if (tooie_render_bridge_state.extra_bytes > tooie_render_bridge_state.extra_highwater)
                                    tooie_render_bridge_state.extra_highwater = tooie_render_bridge_state.extra_bytes;
                                tooie_render_bridge_state.slots[slot].stamp = epoch;
                                tooie_render_bridge_state.last_id = id;
                                tooie_render_bridge_state.last_slot = slot;
                                tooie_render_bridge_state.last_birth = birth;
                                count(&tooie_render_bridge_state.admissions);
                                cursor = (GfxCommand*)current;
                                gEXEnable(cursor);
                                gEXMatrixGroupDecomposed(cursor + 1, id, G_EX_PUSH, 0,
                                    G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                                    G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP,
                                    G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP,
                                    G_EX_ORDER_LINEAR, G_EX_EDIT_NONE,
                                    G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP);
                                *store = (s32)(u32)(cursor + 3);
                                admitted = 1;
                            }
                        }
                    }
                }
            }
        }
    }
    if (!admitted) refuse(reason, actor);
    if (!tooie_render_bridge_state.active) {
        tooie_render_bridge_state.active = 1;
        owns_active = 1;
    }
    func_80101870(actor, arg1);
    if (admitted) {
        if (selector_pair(descriptor) != pair) {
            count(&tooie_render_bridge_state.tripwire_count);
            tooie_render_bridge_state.slots[slot].block_and_valid = 0;
        }
        cursor = (GfxCommand*)(u32)*store;
        gEXPopMatrixGroup(cursor, 0);
        *store = (s32)(u32)(cursor + 1);
    }
    if (store && reason != TOOIE_BRIDGE_RANGE && reason != TOOIE_BRIDGE_ALIGN) {
        returned = (u32)*store;
        if (returned > tooie_render_bridge_state.last_end) tooie_render_bridge_state.last_end = returned;
    }
    if (owns_active) tooie_render_bridge_state.active = 0;
    if (actor->unk7C_12) func_80103110(actor, 1);
}
