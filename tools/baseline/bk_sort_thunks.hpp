// Included after bk_graph_thunks.hpp inside the existing camera native carrier.
// Sort seams (execution-facts48): node is s0; buffers are at the original handler's sp+0x20;
// node byte+0x21 bit0 selects one-sided mode. Mechanical marshalling only.
#if TOOIE_BK_PIPELINE_SELECTED
extern "C" void tooie_bk_sort_children(uint8_t*,recomp_context*);
namespace bk_pipeline {
// Reversed-path entry of the invocation currently between its branch and its epilogue. Children
// never run in that interval (their JALs are NOPs), so one pending record cannot be nested over.
uint32_t sort_reversed_sp=0;
bool sort_one_sided(uint8_t* rdram,const recomp_context* ctx) {
    return (MEM_BU(0x21,ctx->r16)&1u)!=0;
}
}
// DCA48 / DCC00 / DCDB8: every invocation passes this branch point. BK func_803385BC advances
// the offset once before the one-sided chosen-or-empty child.
extern "C" void baseline_bk_sort_onesided(uint8_t* rdram,recomp_context* ctx) {
    if(bk_pipeline::sort_one_sided(rdram,ctx))
        MEM_W(TOOIE_BK_OFFSET_WORD_OFFSET,bk_pipeline::state)=MEM_W(TOOIE_BK_OFFSET_WORD_OFFSET,bk_pipeline::state)+1;
}
// DCB18 / DCCD0 / DCE88: the original's ordered comparison chose B then A.
extern "C" void baseline_bk_sort_reversed(uint8_t* rdram,recomp_context* ctx) {
    (void)rdram;
    bk_pipeline::sort_reversed_sp=uint32_t(ctx->r29);
}
// DCB48 / DCD00 / DCEB8: common epilogue, after the original scalar/distance work.
extern "C" void baseline_bk_sort_children(uint8_t* rdram,recomp_context* ctx) {
    const bool reversed=bk_pipeline::sort_reversed_sp==uint32_t(ctx->r29);
    bk_pipeline::sort_reversed_sp=0;
    if(bk_pipeline::sort_one_sided(rdram,ctx))return;
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=(gpr)(int32_t)MEM_W(0x20,ctx->r29);
    clone.r5=ctx->r16;
    clone.r6=reversed?1:0;
    tooie_bk_sort_children(rdram,&clone);
}
#endif
