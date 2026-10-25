// Insert inside the existing BASELINE_CAMERA_IDENTITY owner after camera_clone_context, beside
// cpu625_thunks.hpp. Every thunk clones the context; the original context is never modified.
// The route helpers return no stop signal: an unauthenticated occurrence just admits nothing.
#if !defined(TOOIE_PLAYER_SECONDARY_SELECTED) || (TOOIE_PLAYER_SECONDARY_SELECTED != 0 && TOOIE_PLAYER_SECONDARY_SELECTED != 1)
#error Explicit boolean TOOIE_PLAYER_SECONDARY_SELECTED required
#endif
#if TOOIE_PLAYER_SECONDARY_SELECTED
extern "C" void tooie_secondary_route_begin(uint8_t*,recomp_context*);
extern "C" void tooie_secondary_route_end(uint8_t*,recomp_context*);
extern "C" void tooie_secondary_route_arm(uint8_t*,recomp_context*);
extern "C" void tooie_secondary_route_disarm(uint8_t*,recomp_context*);
// 80093D40 (main) / 80093E9C (late): before the 93864 jal. a0 is the original PlayerState
// (= s0); the route is the site's constant, never r6 (its delay-slot assignment has not run).
static void secondary_route_begin(uint8_t* rdram,recomp_context* ctx,uint32_t route) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=route;clone.r5=ctx->r4;clone.r6=ctx->r29;
    tooie_secondary_route_begin(rdram,&clone);
}
// 80093D48 after after_16 / 80093EA4 after after_7: s0 survives 93864 (callee-saved); a0/a1 and
// ctx->r31 are not evidence of the call.
static void secondary_route_end(uint8_t* rdram,recomp_context* ctx,uint32_t route) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=route;clone.r5=ctx->r16;clone.r6=ctx->r29;
    tooie_secondary_route_end(rdram,&clone);
}
extern "C" void baseline_secondary_route_begin_main(uint8_t* rdram,recomp_context* ctx) {
    secondary_route_begin(rdram,ctx,1);
}
extern "C" void baseline_secondary_route_end_main(uint8_t* rdram,recomp_context* ctx) {
    secondary_route_end(rdram,ctx,1);
}
extern "C" void baseline_secondary_route_begin_late(uint8_t* rdram,recomp_context* ctx) {
    secondary_route_begin(rdram,ctx,2);
}
extern "C" void baseline_secondary_route_end_late(uint8_t* rdram,recomp_context* ctx) {
    secondary_route_end(rdram,ctx,2);
}
// 80093A9C in 93864, before its DE448 jal (the 80093AA0 delay store of the model to sp+10 has
// not run). 93864 keeps the owner in s0 and saves its mode in the o32 a2 home of its fixed
// 0x68 frame, sp+0x70.
extern "C" void baseline_secondary_route_arm(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r16;clone.r5=ctx->r29;clone.r6=(gpr)(int32_t)MEM_W(0x70,ctx->r29);
    tooie_secondary_route_arm(rdram,&clone);
}
// 80093AA4 after after_31 and L_80093AA4: reached from the DE448 return and from the 93A74
// null-model branch. r2 is not a draw result here.
extern "C" void baseline_secondary_route_disarm(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r16;clone.r5=ctx->r29;
    tooie_secondary_route_disarm(rdram,&clone);
}
#endif
