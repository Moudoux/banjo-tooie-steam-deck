// P04b ckpack copy-storage transports. Included after cpu625_thunks.hpp inside the camera
// owner; the guest helpers own every storage decision, native only marshals the hook seams.
#if !defined(TOOIE_CKCOPY_SELECTED) || (TOOIE_CKCOPY_SELECTED != 0 && TOOIE_CKCOPY_SELECTED != 1)
#error Explicit boolean TOOIE_CKCOPY_SELECTED required
#endif
#if TOOIE_CKCOPY_SELECTED
#if !TOOIE_CPU625_SELECTED
#error ckpack copy storage requires the selected CPU625 composition
#endif
extern "C" void tooie_ckcopy_root_begin(uint8_t*,recomp_context*);
extern "C" void tooie_ckcopy_segment(uint8_t*,recomp_context*);
extern "C" void tooie_ckcopy_root_end(uint8_t*,recomp_context*);
extern "C" void tooie_ckcopy_release(uint8_t*,recomp_context*);
// Guest return 1: corrupted state or an unrestorable redirected word (persistent fault latch).
static void ckcopy_checked(recomp_context& clone) {
    if(clone.r2)rejected_body("ckpack copy storage fault: qualification rejected");
}
// 800A7884: both pool toggles done; the A7840 stack frame identifies the construction owner.
extern "C" void baseline_ckcopy_root_begin(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r29;
    tooie_ckcopy_root_begin(rdram,&clone);ckcopy_checked(clone);
}
// 800A7890: A73F4 returned; the same A7840 frame publishes and closes before the 15410 wait.
extern "C" void baseline_ckcopy_root_end(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r29;
    tooie_ckcopy_root_end(rdram,&clone);ckcopy_checked(clone);
}
// 800DE8C8: DE498 saved the segment-1 command pointer at sp+0xB8 (stored DE8AC, reloaded DE8B8).
extern "C" void baseline_ckcopy_segment(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=(gpr)(int32_t)MEM_W(0xB8,ctx->r29);
    tooie_ckcopy_segment(rdram,&clone);ckcopy_checked(clone);
}
// 801009E0: a0 is the copy-manager record (also saved at sp+0x28); both copies are still intact.
extern "C" void baseline_ckcopy_release(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r4;
    tooie_ckcopy_release(rdram,&clone);ckcopy_checked(clone);
}
#endif
