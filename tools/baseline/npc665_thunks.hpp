// Included after camera_thunks.hpp inside the existing camera native carrier.
#if !defined(TOOIE_NPC665_SELECTED) || (TOOIE_NPC665_SELECTED != 0 && TOOIE_NPC665_SELECTED != 1)
#error Explicit boolean TOOIE_NPC665_SELECTED required
#endif
#if TOOIE_NPC665_SELECTED
extern "C" void tooie_npc665_capture(uint8_t*,recomp_context*);
extern "C" void tooie_npc665_retire(uint8_t*,recomp_context*);
extern "C" void tooie_npc665_admit(uint8_t*,recomp_context*);
extern "C" void tooie_npc665_end_body(uint8_t*,recomp_context*);
extern "C" void baseline_npc665_capture(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r16;clone.r5=ctx->r2;
    clone.r6=(gpr)(int32_t)MEM_W(0x34,ctx->r29);
    tooie_npc665_capture(rdram,&clone);
}
extern "C" void baseline_npc665_retire(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r16;clone.r5=ctx->r2;clone.r6=ctx->r4;
    tooie_npc665_retire(rdram,&clone);
}
extern "C" void baseline_npc665_admit(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=(gpr)(int32_t)MEM_W(0xF8,ctx->r29);
    clone.r5=ADD32(ctx->r29,0xCC);
    clone.r6=(gpr)(int32_t)MEM_W(0x10C,ctx->r29);
    tooie_npc665_admit(rdram,&clone);
}
// This is the sole caller of the renamed guest body. Use the real context:
// normal return must equal the compiled guest body's complete output context.
extern "C" void baseline_npc665_end_checked(uint8_t* rdram,recomp_context* ctx) {
    tooie_npc665_end_body(rdram,ctx);
    if(ctx->r2)rejected_body("NPC665 guest paid-close ownership tripwire");
}
#endif
