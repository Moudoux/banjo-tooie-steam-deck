// Insert inside existing BASELINE_CAMERA_IDENTITY owner after camera_clone_context.
// Selected boolean is independent of TOOIE_ACTOR_PALETTE_SELECTED.
#if !defined(TOOIE_CPU625_SELECTED) || (TOOIE_CPU625_SELECTED != 0 && TOOIE_CPU625_SELECTED != 1)
#error Explicit boolean TOOIE_CPU625_SELECTED required
#endif
#if !defined(TOOIE_BACKPACK621_SELECTED) || (TOOIE_BACKPACK621_SELECTED != 0 && TOOIE_BACKPACK621_SELECTED != 1)
#error Explicit boolean TOOIE_BACKPACK621_SELECTED required
#endif
#if TOOIE_BACKPACK621_SELECTED && !TOOIE_CPU625_SELECTED
#error Backpack621 requires the existing CPU625 transports
#endif
#if TOOIE_CPU625_SELECTED
#if TOOIE_BACKPACK621_SELECTED
extern "C" void tooie_backpack621_begin(uint8_t*,recomp_context*);
extern "C" void tooie_backpack621_admit(uint8_t*,recomp_context*);
extern "C" void tooie_backpack621_end(uint8_t*,recomp_context*);
extern "C" void tooie_backpack621_retire(uint8_t*,recomp_context*);
static void backpack621_checked(recomp_context& clone) {
    if(clone.r2)rejected_body("Backpack621 guest ownership tripwire: qualification rejected");
}
#endif
extern "C" void tooie_cpu625_begin(uint8_t*,recomp_context*);
extern "C" void tooie_cpu625_admit(uint8_t*,recomp_context*);
extern "C" void tooie_cpu625_end(uint8_t*,recomp_context*);
extern "C" void tooie_cpu625_retire(uint8_t*,recomp_context*);
// Guest return is only a required-stop signal; native owns no matching/policy.
static void cpu625_checked(recomp_context& clone) {
    if(clone.r2)rejected_body("CPU625 guest ownership tripwire: qualification rejected");
}
extern "C" void baseline_cpu625_begin(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r16;clone.r5=(gpr)(int32_t)MEM_W(0x10,ctx->r29);
    tooie_cpu625_begin(rdram,&clone);cpu625_checked(clone);
#if TOOIE_BACKPACK621_SELECTED
    recomp_context next;camera_clone_context(next,*ctx);
    next.r4=ctx->r16;next.r5=(gpr)(int32_t)MEM_W(0x10,ctx->r29);
    tooie_backpack621_begin(rdram,&next);backpack621_checked(next);
#endif
}
extern "C" void baseline_cpu625_admit(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=(gpr)(int32_t)MEM_W(0xF8,ctx->r29);
    clone.r5=ADD32(ctx->r29,0xCC);
    clone.r6=(gpr)(int32_t)MEM_W(0x10C,ctx->r29);
    tooie_cpu625_admit(rdram,&clone);cpu625_checked(clone);
#if TOOIE_BACKPACK621_SELECTED
    recomp_context next;camera_clone_context(next,*ctx);
    next.r4=(gpr)(int32_t)MEM_W(0xF8,ctx->r29);next.r5=ADD32(ctx->r29,0xCC);
    next.r6=(gpr)(int32_t)MEM_W(0x10C,ctx->r29);
    tooie_backpack621_admit(rdram,&next);backpack621_checked(next);
#endif
}
extern "C" void baseline_cpu625_end(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);clone.r4=ctx->r16;
    tooie_cpu625_end(rdram,&clone);cpu625_checked(clone);
#if TOOIE_BACKPACK621_SELECTED
    recomp_context next;camera_clone_context(next,*ctx);next.r4=ctx->r16;
    tooie_backpack621_end(rdram,&next);backpack621_checked(next);
#endif
}
extern "C" void baseline_cpu625_set_state(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r4;clone.r5=ctx->r5;clone.r6=1;
    tooie_cpu625_retire(rdram,&clone);cpu625_checked(clone);
#if TOOIE_BACKPACK621_SELECTED
    recomp_context next;camera_clone_context(next,*ctx);
    next.r4=ctx->r4;next.r5=ctx->r5;next.r6=1;
    tooie_backpack621_retire(rdram,&next);backpack621_checked(next);
#endif
}
extern "C" void baseline_cpu625_init(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r4;clone.r5=0;clone.r6=2;
    tooie_cpu625_retire(rdram,&clone);cpu625_checked(clone);
#if TOOIE_BACKPACK621_SELECTED
    recomp_context next;camera_clone_context(next,*ctx);
    next.r4=ctx->r4;next.r5=0;next.r6=2;
    tooie_backpack621_retire(rdram,&next);backpack621_checked(next);
#endif
}
#endif
