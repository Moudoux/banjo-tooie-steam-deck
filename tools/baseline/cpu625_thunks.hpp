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
#if !defined(TOOIE_CKCOPY_SELECTED) || (TOOIE_CKCOPY_SELECTED != 0 && TOOIE_CKCOPY_SELECTED != 1)
#error Explicit boolean TOOIE_CKCOPY_SELECTED required
#endif
#if !defined(TOOIE_BK_PIPELINE_SELECTED) || (TOOIE_BK_PIPELINE_SELECTED != 0 && TOOIE_BK_PIPELINE_SELECTED != 1)
#error Explicit boolean TOOIE_BK_PIPELINE_SELECTED required
#endif
#if !defined(TOOIE_BACKPACK621_SECTION_PRESENT) || TOOIE_BACKPACK621_SECTION_PRESENT!=(TOOIE_BACKPACK621_SELECTED && !TOOIE_BK_PIPELINE_SELECTED)
#error L06c-3 backpack621 section-presence contract
#endif
#if !defined(TOOIE_CPU625_SECTION_PRESENT) || TOOIE_CPU625_SECTION_PRESENT!=(TOOIE_CPU625_SELECTED && !TOOIE_BK_PIPELINE_SELECTED)
#error L06c-4 CPU625 section-presence contract
#endif
// L06c-4: every transport below (set_state/init included; their hooks are BK-OFF) needs the linked section.
#if TOOIE_CPU625_SECTION_PRESENT
#if TOOIE_BACKPACK621_SECTION_PRESENT
#if !TOOIE_BK_PIPELINE_SELECTED
// L06b-2: BK installs none of these transports and its image builds only the 621 retire.
extern "C" void tooie_backpack621_begin(uint8_t*,recomp_context*);
extern "C" void tooie_backpack621_admit(uint8_t*,recomp_context*);
extern "C" void tooie_backpack621_end(uint8_t*,recomp_context*);
#endif
extern "C" void tooie_backpack621_retire(uint8_t*,recomp_context*);
static void backpack621_checked(recomp_context& clone) {
    if(clone.r2)rejected_body("Backpack621 guest ownership tripwire: qualification rejected");
}
#endif
#if !TOOIE_BK_PIPELINE_SELECTED
extern "C" void tooie_cpu625_begin(uint8_t*,recomp_context*);
extern "C" void tooie_cpu625_admit(uint8_t*,recomp_context*);
extern "C" void tooie_cpu625_end(uint8_t*,recomp_context*);
#endif
extern "C" void tooie_cpu625_retire(uint8_t*,recomp_context*);
// Guest return is only a required-stop signal; native owns no matching/policy.
static void cpu625_checked(recomp_context& clone) {
    if(clone.r2)rejected_body("CPU625 guest ownership tripwire: qualification rejected");
}
#if !TOOIE_BK_PIPELINE_SELECTED
// L06b-4: these three transports are installed only with BK off (cpu625_hooks gate in
// prepare_camera_bridge.prepare_raw), and the BK image builds none of their guest bodies.
extern "C" void baseline_cpu625_begin(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r16;clone.r5=(gpr)(int32_t)MEM_W(0x10,ctx->r29);
    tooie_cpu625_begin(rdram,&clone);cpu625_checked(clone);
#if TOOIE_BACKPACK621_SELECTED && !TOOIE_BK_PIPELINE_SELECTED
    recomp_context next;camera_clone_context(next,*ctx);
    next.r4=ctx->r16;next.r5=(gpr)(int32_t)MEM_W(0x10,ctx->r29);
    tooie_backpack621_begin(rdram,&next);backpack621_checked(next);
#endif
}
// Original DE498 frame words via camera_clone_de498_args (camera_thunks.hpp).
// Backpack621 gets its own fresh clone of the untouched ctx, not CPU625's output.
extern "C" void baseline_cpu625_admit(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_de498_args(rdram,clone,*ctx);
#if TOOIE_CKCOPY_SELECTED
    // P04a: this DE498's own segment-1 command pointer (sp+0xB8; stored DE8AC, read by S2 at DE8C8).
    clone.r7=(gpr)(int32_t)MEM_W(0xB8,ctx->r29);
#endif
    tooie_cpu625_admit(rdram,&clone);cpu625_checked(clone);
#if TOOIE_BACKPACK621_SELECTED && !TOOIE_BK_PIPELINE_SELECTED
    recomp_context next;camera_clone_de498_args(rdram,next,*ctx);
    tooie_backpack621_admit(rdram,&next);backpack621_checked(next);
#endif
}
extern "C" void baseline_cpu625_end(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);clone.r4=ctx->r16;
    tooie_cpu625_end(rdram,&clone);cpu625_checked(clone);
#if TOOIE_BACKPACK621_SELECTED && !TOOIE_BK_PIPELINE_SELECTED
    recomp_context next;camera_clone_context(next,*ctx);next.r4=ctx->r16;
    tooie_backpack621_end(rdram,&next);backpack621_checked(next);
#endif
}
#endif
extern "C" void baseline_cpu625_set_state(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r4;clone.r5=ctx->r5;clone.r6=1;
    tooie_cpu625_retire(rdram,&clone);cpu625_checked(clone);
#if TOOIE_BACKPACK621_SECTION_PRESENT
    recomp_context next;camera_clone_context(next,*ctx);
    next.r4=ctx->r4;next.r5=ctx->r5;next.r6=1;
    tooie_backpack621_retire(rdram,&next);backpack621_checked(next);
#endif
}
extern "C" void baseline_cpu625_init(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r4;clone.r5=0;clone.r6=2;
    tooie_cpu625_retire(rdram,&clone);cpu625_checked(clone);
#if TOOIE_BACKPACK621_SECTION_PRESENT
    recomp_context next;camera_clone_context(next,*ctx);
    next.r4=ctx->r4;next.r5=0;next.r6=2;
    tooie_backpack621_retire(rdram,&next);backpack621_checked(next);
#endif
}
#if TOOIE_CKCOPY_SELECTED
extern "C" void tooie_cpu625_attach_begin(uint8_t*,recomp_context*);
extern "C" void tooie_cpu625_attach_end(uint8_t*,recomp_context*);
// N3 A66BC (func_800A6250, before the direct DE448 jal; its A66C0 delay slot has not yet stored the
// model to sp+0x10). The model is the untouched original r2; the owner is the A6250 frame word
// sp+0x1CC; the frame itself is the lexical evidence. The original context is never modified.
extern "C" void baseline_cpu625_attach_begin(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=(gpr)(int32_t)MEM_W(0x1CC,ctx->r29);clone.r5=ctx->r2;clone.r6=ctx->r29;
    tooie_cpu625_attach_begin(rdram,&clone);cpu625_checked(clone);
}
// N3 A66C4 (both the DE448 return and the A6674 skip branch). r2 is not the model here.
extern "C" void baseline_cpu625_attach_end(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=(gpr)(int32_t)MEM_W(0x1CC,ctx->r29);clone.r5=ctx->r29;
    tooie_cpu625_attach_end(rdram,&clone);cpu625_checked(clone);
}
#endif
#endif
