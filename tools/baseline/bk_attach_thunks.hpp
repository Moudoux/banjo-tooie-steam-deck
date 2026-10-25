// Included after bk_sort_thunks.hpp inside the existing camera native carrier.
// func_80092444 backpack seams (execution-facts24): JAL 92710 to ckpack entry3, return 92718.
#if TOOIE_BK_PIPELINE_SELECTED
extern "C" void tooie_bk_attachment_begin(uint8_t*,recomp_context*);
extern "C" void tooie_bk_attachment_end(uint8_t*,recomp_context*);
extern "C" void baseline_bk_attachment_begin(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r16;   // func_80092444 s0 (live through 9271C): the PlayerState whose backpack this is
    tooie_bk_attachment_begin(rdram,&clone);
}
extern "C" void baseline_bk_attachment_end(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    tooie_bk_attachment_end(rdram,&clone);
}
#endif
