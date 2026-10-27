// Included after bk_graph_thunks.hpp inside the existing camera native carrier.
// Sort seams (execution-facts48): node is s0; buffers are at the original handler's sp+0x20;
// node byte+0x21 bit0 selects one-sided mode, decided by the guest helpers. Mechanical marshalling only.
#if TOOIE_BK_PIPELINE_SELECTED
extern "C" void tooie_bk_sort_children(uint8_t*,recomp_context*);
extern "C" void tooie_bk_sort_onesided(uint8_t*,recomp_context*);
namespace bk_pipeline {
// Reversed-path entry of the invocation currently between its branch and its epilogue. Children
// never run in that interval (their JALs are NOPs), so one pending record cannot be nested over.
uint32_t sort_reversed_sp=0;
}
// DCA48 / DCC00 / DCDB8: every invocation passes this branch point. BK func_803385BC advances
// the offset once before the one-sided chosen-or-empty child (tooie_bk_sort_onesided).
extern "C" void baseline_bk_sort_onesided(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r16;
    tooie_bk_sort_onesided(rdram,&clone);
}
// DCB18 / DCCD0 / DCE88: the original's ordered comparison chose B then A.
extern "C" void baseline_bk_sort_reversed(uint8_t* rdram,recomp_context* ctx) {
    (void)rdram;
    bk_pipeline::sort_reversed_sp=uint32_t(ctx->r29);
}
// DCB48 / DCD00 / DCEB8: common epilogue, after the original scalar/distance work; the guest
// returns at once for a one-sided node.
extern "C" void baseline_bk_sort_children(uint8_t* rdram,recomp_context* ctx) {
    const bool reversed=bk_pipeline::sort_reversed_sp==uint32_t(ctx->r29);
    bk_pipeline::sort_reversed_sp=0;
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=(gpr)(int32_t)MEM_W(0x20,ctx->r29);
    clone.r5=ctx->r16;
    clone.r6=reversed?1:0;
    tooie_bk_sort_children(rdram,&clone);
}
#endif
