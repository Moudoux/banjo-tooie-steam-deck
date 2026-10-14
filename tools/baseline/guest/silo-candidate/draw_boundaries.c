/* Original draw boundaries with scoped public-label envelopes for admitted draws. */
#include "silo_bridge.h"
#include "actor_palette.h"
#if !defined(TOOIE_ACTOR_PALETTE) || (TOOIE_ACTOR_PALETTE != 0 && TOOIE_ACTOR_PALETTE != 1)
#error Explicit boolean TOOIE_ACTOR_PALETTE selection required
#endif
#define PATCH __attribute__((section(".recomp_patch"), noinline))
typedef struct GraphicsBuffers GraphicsBuffers;
void func_800DF47C(s32 (*callback)(Actor*), Actor* actor);
f32* func_801027F4(Actor* actor);
f32* func_801018D8(Actor* actor);
s32 func_8010347C(void* descriptor);
GraphicsBuffers* func_800A7180(void);
void* func_800DE498(GraphicsBuffers*, f32*, f32*, f32, f32*, s32);
void* func_800DE448(f32*, f32*, f32, f32*, s32);
u32 tooie_player_begin(SiloBuffers*, u32);
void tooie_player_end(SiloBuffers*);
_Static_assert(sizeof(void*) == 4 && sizeof(f32) == 4 &&
    __builtin_offsetof(Actor, unk0) == 0 &&
    __builtin_offsetof(Actor, position) == 4 &&
    __builtin_offsetof(Actor, scale) == 0x38, "original draw boundary ABI");

PATCH void* func_80101808(Actor* actor, s32 (*callback)(Actor*)) {
    func_800DF47C(callback, actor);
    f32* rotation = func_801027F4(actor);
    f32* extra = func_801018D8(actor);
    s32 model = func_8010347C(actor->unk0);
    f32 scale = actor->scale;
    u32 owns=0, slot, offset;
    Unk80132ED0* descriptor=actor->unk0;
    if(!SC.actor && !SS.silo_depth) {
        offset=(u32)descriptor-(u32)D_80132ED0;
        if((u32)descriptor>=(u32)D_80132ED0 && offset<200u*0x30u && !(offset%0x30u)) {
            slot=offset/0x30u;
            if(actor_binding((u32)actor,(u32)descriptor,slot,SS.births[slot],silo_pair(descriptor))) {
                SC.actor=(u32)actor; SC.descriptor=(u32)descriptor; SC.slot=slot;
                SC.birth=SS.births[slot]; SC.pair=silo_pair(descriptor); SC.epoch=SS.epoch;
                SC.view=(u32)func_800A89F8(); SC.phase=(u32)func_800EB854();
                SC.admitted=0; SC.buffers=0; owns=1;
#if TOOIE_NPC665
                npc665_pending((u32)callback);
#endif
            }
        }
    }
    void* result=func_800DE448(actor->position, rotation, scale, extra, model);
    if(owns) {
#if TOOIE_NPC665
        npc665_reset();
#endif
        SC.actor=0; SC.admitted=0;
    }
    return result;
}

PATCH void* func_800DE448(f32* position, f32* rotation, f32 scale,
                         f32* extra, s32 model) {
    u32 increment=SS.silo_depth!=0xFFFFFFFFu;
    if(increment) SS.silo_depth+=1u;
    GraphicsBuffers* buffers = func_800A7180();
    if(increment && SS.silo_depth==1u && SC.actor) SC.buffers=(u32)buffers;
    u32 player=0;
    if(increment && SS.silo_depth==1u && D_8012C948[24]==0x80092234u)
        player=tooie_player_begin((SiloBuffers*)buffers,(u32)model);
    void* result=func_800DE498(buffers, position, rotation, scale, extra, model);
#if TOOIE_NPC665
    if(increment && SS.silo_depth==1u && SC.admitted==TOOIE_NPC665_ADMITTED)
        tooie_npc665_end((SiloBuffers*)buffers);
#endif
#if TOOIE_ACTOR_PALETTE
    if(increment && SS.silo_depth==1u && (SC.admitted==TOOIE_BRIDGE_PALETTE_ADMITTED ||
        SC.admitted==TOOIE_BRIDGE_CPU_ROOT_ADMITTED))
#else
    if(increment && SS.silo_depth==1u && SC.admitted==TOOIE_BRIDGE_PALETTE_ADMITTED)
#endif
        tooie_actor_palette_end((SiloBuffers*)buffers);
    if(player) tooie_player_end((SiloBuffers*)buffers);
    if(increment) SS.silo_depth-=1u;
    return result;
}
