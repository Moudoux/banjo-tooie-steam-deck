// Included after camera_thunks.hpp inside the existing camera native carrier.
#if !defined(TOOIE_NPC665_SELECTED) || (TOOIE_NPC665_SELECTED != 0 && TOOIE_NPC665_SELECTED != 1)
#error Explicit boolean TOOIE_NPC665_SELECTED required
#endif
#if TOOIE_NPC665_SELECTED
extern "C" void tooie_npc665_capture(uint8_t*,recomp_context*);
extern "C" void tooie_npc665_retire(uint8_t*,recomp_context*);
extern "C" void tooie_npc665_admit(uint8_t*,recomp_context*);
extern "C" void tooie_npc665_end(uint8_t*,recomp_context*);
// 8010108C before 801010E8: s0 holds the actor; v0 is func_80100368's return.
// SP+0x34 is the saved incoming a1 (store 80101098, reload 801010F0).
// Read ctx's frame: camera_clone_context gives clone a separate outgoing area.
extern "C" void baseline_npc665_capture(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;
    camera_clone_context(clone,*ctx);
    clone.r4=ctx->r16;
    clone.r5=ctx->r2;
    clone.r6=(gpr)(int32_t)MEM_W(0x34,ctx->r29);
    tooie_npc665_capture(rdram,&clone);
}
extern "C" void baseline_npc665_retire(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r16;clone.r5=ctx->r2;clone.r6=ctx->r4;
    tooie_npc665_retire(rdram,&clone);
}
// Original DE498 frame words via camera_clone_de498_args (camera_thunks.hpp).
extern "C" void baseline_npc665_admit(uint8_t* rdram,recomp_context* ctx) {
#ifdef BASELINE_PLAYER
    namespace guest=baseline::issue_evidence::guest;
    // First selected DEFA0 transport: the payload is read before any NPC/CPU-root/CPU625
    // admission; the two outcome joins are read-only and published after the admit call.
    auto draw=guest::read_draw(rdram,*ctx);
    guest::join_player(rdram,draw);
    const auto counters=guest::skeletal_counters(rdram);
#endif
    recomp_context clone;camera_clone_de498_args(rdram,clone,*ctx);
    tooie_npc665_admit(rdram,&clone);
#ifdef BASELINE_PLAYER
    guest::join_skeletal(rdram,counters,uint32_t(clone.r2),draw); // Guest u32 return in v0.
    baseline::issue_evidence::record(draw);
#endif
}
// Absolute import called by the guest pair; sole caller of the guest end. Use the real
// context: normal return must equal the compiled guest body's complete output context.
extern "C" void baseline_npc665_end_checked(uint8_t* rdram,recomp_context* ctx) {
    tooie_npc665_end(rdram,ctx);
    if(ctx->r2)rejected_body("NPC665 guest paid-close ownership tripwire");
}
#endif
