// Included after bk_attach_thunks.hpp inside the existing camera native carrier.
// Shadow producer seams (execution-facts53 2-3, execution-facts54, execution-facts55); the guest
// helpers own policy. The original context is never modified; each helper runs on a clone with its
// own outgoing frame.
#if TOOIE_BK_PIPELINE_SELECTED
extern "C" void tooie_bk_shadow_route(uint8_t*,recomp_context*);
extern "C" void tooie_bk_shadow_light_event(uint8_t*,recomp_context*);
extern "C" void tooie_bk_shadow_begin(uint8_t*,recomp_context*);
extern "C" void tooie_bk_shadow_end(uint8_t*,recomp_context*);
namespace {
void bk_shadow_route(uint8_t* rdram,recomp_context* ctx,uint32_t local) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=local;clone.r5=local?ctx->r16:0;
    tooie_bk_shadow_route(rdram,&clone);
}
// The guest seam values: receiver, the producer's original sp (its activation key), seam kind.
void bk_shadow_begin(uint8_t* rdram,recomp_context* ctx,gpr receiver,uint32_t seam) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=receiver;clone.r5=ctx->r29;clone.r6=seam;
    tooie_bk_shadow_begin(rdram,&clone);
}
// r2 here is the draw result (or stale on the A6674 skip leg); it is never read.
void bk_shadow_end(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r29;
    tooie_bk_shadow_end(rdram,&clone);
}
// handle > 0: an allocation returned it; 0: the light vector's lifetime ends.
void bk_shadow_light_event(uint8_t* rdram,recomp_context* ctx,gpr handle) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=handle;
    tooie_bk_shadow_light_event(rdram,&clone);
}
}
// A6CF0: s0 is the accepted light's one-based handle. A6D60: fallback, no light.
extern "C" void baseline_bk_shadow_route_local(uint8_t* rdram,recomp_context* ctx) { bk_shadow_route(rdram,ctx,1); }
extern "C" void baseline_bk_shadow_route_fallback(uint8_t* rdram,recomp_context* ctx) { bk_shadow_route(rdram,ctx,0); }
// A6664/A66BC: receiver is the A6250 incoming a0 home sp+0x250. The models (sp+0x1C8 and v0) are not
// needed: the shared DE498 capture authenticates the executed draw.
extern "C" void baseline_bk_shadow_body_begin(uint8_t* rdram,recomp_context* ctx) {
    bk_shadow_begin(rdram,ctx,(gpr)(int32_t)MEM_W(0x250,ctx->r29),0);
}
extern "C" void baseline_bk_shadow_body_end(uint8_t* rdram,recomp_context* ctx) { bk_shadow_end(rdram,ctx); }
extern "C" void baseline_bk_shadow_attachment_begin(uint8_t* rdram,recomp_context* ctx) {
    bk_shadow_begin(rdram,ctx,(gpr)(int32_t)MEM_W(0x250,ctx->r29),1);
}
extern "C" void baseline_bk_shadow_attachment_end(uint8_t* rdram,recomp_context* ctx) { bk_shadow_end(rdram,ctx); }
// A5EA8: the A5DB0 receiver is s0.
extern "C" void baseline_bk_shadow_blob_begin(uint8_t* rdram,recomp_context* ctx) { bk_shadow_begin(rdram,ctx,ctx->r16,2); }
extern "C" void baseline_bk_shadow_blob_end(uint8_t* rdram,recomp_context* ctx) { bk_shadow_end(rdram,ctx); }
// Before C8B78: r2 is C8A98's returned positive handle (reloaded at C8B74); r2/ra stay untouched.
extern "C" void baseline_bk_shadow_light_born(uint8_t* rdram,recomp_context* ctx) {
    if(int32_t(ctx->r2)>0)bk_shadow_light_event(rdram,ctx,ctx->r2);
}
// Entry of C90AC (vector init), C90D0 (vector free) and C90F4 (loader free+init).
extern "C" void baseline_bk_shadow_light_reset(uint8_t* rdram,recomp_context* ctx) { bk_shadow_light_event(rdram,ctx,0); }
#endif
