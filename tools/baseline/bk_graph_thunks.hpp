// Included after bk_pipeline_thunks.hpp inside the existing camera native carrier.
// Mechanical o32 marshalling for the graph seams; the guest helpers own the BK policy.
#if TOOIE_BK_PIPELINE_SELECTED
// The guest helper set is owned by prepare_silo_bridge.BK_HELPERS (exact definition order) and counted by
// the generated registry table; these thunks name the helpers they call and restate no count.
static_assert(!TOOIE_CKCOPY_SELECTED,"The P04b redirect is never selected with the BK pipeline");
extern "C" void tooie_bk_skin_segment(uint8_t*,recomp_context*);
extern "C" void tooie_bk_skin_capture(uint8_t*,recomp_context*);
extern "C" void tooie_bk_handler_begin(uint8_t*,recomp_context*);
extern "C" void tooie_bk_handler_end(uint8_t*,recomp_context*);
extern "C" void tooie_bk_root_join(uint8_t*,recomp_context*);
extern "C" void tooie_bk_bbox_visible(uint8_t*,recomp_context*);
extern "C" void tooie_bk_billboard_entry(uint8_t*,recomp_context*);
// DE498 DE8C8: key = the invocation's local-cursor slot (sp+0xCC, as root_begin receives it);
// sp+0xB8 is the segment-1 command pointer (stored DE8AC, reloaded DE8B8).
extern "C" void baseline_bk_skin_segment(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ADD32(ctx->r29,0xCC);
    clone.r5=(gpr)(int32_t)MEM_W(0xB8,ctx->r29);
    tooie_bk_skin_segment(rdram,&clone);
}
// BK culling_patches GeoCmdD and transform_tagging's geo-wide frustum disable: the original bbox and
// in-graph sphere predicates retain their call side effects. At these exact post-call branches v0 alone
// decides child traversal; the guest decides it (tooie_bk_bbox_visible).
extern "C" void baseline_bk_bbox_visible(uint8_t* rdram, recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    tooie_bk_bbox_visible(rdram,&clone);
    ctx->r2=clone.r2;
}
namespace bk_pipeline {
// Buffers sp+0xF8, key sp+0xCC, effective model sp+0x10C (camera_clone_de498_args).
void skin_capture(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_de498_args(rdram,clone,*ctx);
    tooie_bk_skin_capture(rdram,&clone);
}
}
extern "C" void baseline_bk_skin_capture_a(uint8_t* rdram,recomp_context* ctx) { bk_pipeline::skin_capture(rdram,ctx); }
extern "C" void baseline_bk_skin_capture_b(uint8_t* rdram,recomp_context* ctx) { bk_pipeline::skin_capture(rdram,ctx); }
// DE498 DF238: same three original-frame words; the guest closes only a bit40 invocation.
extern "C" void baseline_bk_root_join(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_de498_args(rdram,clone,*ctx);
#ifdef BASELINE_PLAYER
    baseline::issue_evidence::guest::bk_root_join(bk_pipeline::evidence_roots,rdram,uint32_t(ctx->r29),bk_pipeline::evidence);
#endif
    tooie_bk_root_join(rdram,&clone);
}
// Billboard entry (all three tables): BK func_803384A8 increments the traversal offset first
// (tooie_bk_billboard_entry).
extern "C" void baseline_bk_billboard_entry(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    tooie_bk_billboard_entry(rdram,&clone);
}
// After the original push and Mtx advance: buffers s0, node s1, the handler's own SP as key.
extern "C" void baseline_bk_billboard_begin(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r16;clone.r5=ctx->r17;clone.r6=ctx->r29;
    tooie_bk_handler_begin(rdram,&clone);
}
// After the original RSP pop; zero-child branches also arrive here without a begin.
extern "C" void baseline_bk_billboard_end(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r16;clone.r5=ctx->r29;
    tooie_bk_handler_end(rdram,&clone);
}
// Type-12 selector 800DD56C. Every hook passes the selector activation's own SP (r29 after its prologue).
extern "C" void tooie_bk_selector_begin(uint8_t*,recomp_context*);
extern "C" void tooie_bk_selector_child(uint8_t*,recomp_context*);
extern "C" void tooie_bk_selector_end(uint8_t*,recomp_context*);
// Before DD58C, the prologue saves done: node a1.
extern "C" void baseline_bk_selector_begin(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r5;clone.r5=ctx->r29;
    tooie_bk_selector_begin(rdram,&clone);
}
// Before the single-child DE2A4 JAL at DD5DC: child u32(a1)+u32(t1) (its delay-slot ADDU has not run), node s3.
extern "C" void baseline_bk_selector_single(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r19;clone.r5=(gpr)(int32_t)(uint32_t(ctx->r5)+uint32_t(ctx->r9));clone.r6=ctx->r29;
    tooie_bk_selector_child(rdram,&clone);
}
// Before the bitmask-child DE2A4 JAL at DD614: child u32(s3)+u32(t4) (delay slot not run), node s3.
extern "C" void baseline_bk_selector_mask(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r19;clone.r5=(gpr)(int32_t)(uint32_t(ctx->r19)+uint32_t(ctx->r12));clone.r6=ctx->r29;
    tooie_bk_selector_child(rdram,&clone);
}
// Before DD63C, the universal join of every path: node s3.
extern "C" void baseline_bk_selector_end(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r19;clone.r5=ctx->r29;
    tooie_bk_selector_end(rdram,&clone);
}
#endif
