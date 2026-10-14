/* Original draw boundaries only: no added renderer state or commands. */
#include "common.h"
#define PATCH __attribute__((section(".recomp_patch"), noinline))
typedef struct GraphicsBuffers GraphicsBuffers;
void func_800DF47C(s32 (*callback)(Actor*), Actor* actor);
f32* func_801027F4(Actor* actor);
f32* func_801018D8(Actor* actor);
s32 func_8010347C(void* descriptor);
GraphicsBuffers* func_800A7180(void);
void* func_800DE498(GraphicsBuffers*, f32*, f32*, f32, f32*, s32);
void* func_800DE448(f32*, f32*, f32, f32*, s32);
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
    return func_800DE448(actor->position, rotation, scale, extra, model);
}

PATCH void* func_800DE448(f32* position, f32* rotation, f32 scale,
                         f32* extra, s32 model) {
    GraphicsBuffers* buffers = func_800A7180();
    return func_800DE498(buffers, position, rotation, scale, extra, model);
}
