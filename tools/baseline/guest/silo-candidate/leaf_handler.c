/* Original type-24 handler, reconstructed from 800DD998..800DDB34.
 * Original reads/calls are retained around the scoped player leaf bracket.
 */
#include "common.h"
#define TOOIE_BRIDGE_GUEST 1
#include "npc665_owner.h"
#if !defined(TOOIE_BACKPACK621) || (TOOIE_BACKPACK621 != 0 && TOOIE_BACKPACK621 != 1)
#error Explicit boolean TOOIE_BACKPACK621 selection required
#endif
#if TOOIE_BACKPACK621
#include "../backpack621-candidate/backpack621_owner.h"
#endif
#include "core1/mlmtx.h"
#define PATCH __attribute__((section(".recomp_patch"), noinline))

typedef struct {
    Gfx* volatile gfx;
    Mtx* volatile mtx;
} LeafBuffers;
typedef void (*LeafCallback)(MtxF*, s32*, s32);
extern LeafCallback volatile D_8012CEC4;
extern u8* volatile D_8012C960;
MtxF* func_80018BB4(void);
void func_8010F070(MtxF*, s32*, s32);
u32 tooie_player_leaf_begin(LeafBuffers*);
void tooie_player_leaf_end(LeafBuffers*);

_Static_assert(sizeof(void*) == 4 && sizeof(s16) == 2 && sizeof(s32) == 4 &&
    sizeof(LeafBuffers) == 8 && __builtin_offsetof(LeafBuffers, mtx) == 4 &&
    sizeof(Gfx) == 8 && sizeof(Mtx) == 64, "original leaf handler guest ABI");

PATCH void func_800DD998(LeafBuffers* buffers, u8* node) {
    /* Independent mutable positions, initialized before either matrix getter.
     * The node field's meaning is intentionally left as its original offset.
     */
    s32 transform_position = *(volatile s16*)(node + 0x2C);
    s32 callback_position = transform_position;
    MtxF* current = mlMtx_get_stack_pointer();
    MtxF* parent = func_80018BB4();
    u8* member = node;

    /* Copy the CPU current matrix once, before F070/callback processing.
     * Every leaf reload uses the then-current buffers->mtx; allocation is
     * advanced only after the complete original sentinel-terminated list.
     */
    mlMtxApply(buffers->mtx);
    do {
        s32 flags = *(volatile s16*)(node + 0x2E);
        s32 count;
        if (flags & 1) {
            count = *(volatile s16*)(member + 0x20);
            if (count) {
                func_8010F070(current, &transform_position, count);
            }
            count = *(volatile s16*)(member + 0x14);
            if (count) {
                func_8010F070(parent, &transform_position, count);
            }
        }

        LeafCallback callback = D_8012CEC4;
        if (callback) {
            count = *(volatile s16*)(member + 0x20);
            if (count) {
                callback(current, &callback_position, count);
            }
            count = *(volatile s16*)(member + 0x14);
            if (count) {
                /* Original reloads the target after the first callback.
                 * Do not insert a new null guard on this second call.
                 */
                D_8012CEC4(parent, &callback_position, count);
            }
        }

        u32 playerLeafToken=0;
#if TOOIE_NPC665
        u32 npcLeafToken=0;
#endif
#if TOOIE_BACKPACK621
        u32 backpack621LeafToken=0;
        if(backpack621_paid())
            backpack621LeafToken=tooie_backpack621_leaf_begin((struct TooieRenderBuffers*)buffers);
        else
#endif
#if TOOIE_NPC665
        if(tooie_render_bridge_state.silo_context.admitted==TOOIE_NPC665_ADMITTED)
            npcLeafToken=tooie_npc665_leaf_begin((struct TooieRenderBuffers*)buffers);
        else
#endif
        playerLeafToken=tooie_player_leaf_begin(buffers);
        Gfx* cursor = buffers->gfx;
        buffers->gfx = cursor + 1;
        ((volatile u32*)cursor)[0] = 0xDE000000u;
        s32 index = *(volatile s16*)(member + 8);
        u32 list = (u32)D_8012C960 + (u32)(index * 8) + 8u;
        ((volatile u32*)cursor)[1] = osVirtualToPhysical((void*)list);

        /* Reload gfx after the external conversion, as in the original. */
        cursor = buffers->gfx;
        buffers->gfx = cursor + 1;
        ((volatile u32*)cursor)[0] = 0xDA380002u;
        ((volatile u32*)cursor)[1] = (u32)buffers->mtx;
        if (playerLeafToken) tooie_player_leaf_end(buffers);
#if TOOIE_NPC665
        if(npcLeafToken) tooie_npc665_leaf_end((struct TooieRenderBuffers*)buffers);
#endif
#if TOOIE_BACKPACK621
        if(backpack621LeafToken) tooie_backpack621_leaf_end((struct TooieRenderBuffers*)buffers);
#endif
        member += 2;
    } while (*(volatile s16*)(member + 8));
    buffers->mtx = (Mtx*)((u8*)buffers->mtx + 64);
}
