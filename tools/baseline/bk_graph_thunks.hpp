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
#ifndef _WIN32
namespace bk_pipeline {
// Culling may only skip subtrees that leave cur_model_transform_id_offset alone, so later bones keep
// their ordinals. Leaf types (no DE2A4 call) and pure containers qualify: 6 (u32 +8), LOD 8 (u32 +0x1C,
// all three tables) and nested bbox 13 (s16 +0x14). Billboard 0, sort 1, bone 2, selector 12 and the
// undecoded containers 14/15 do not, nor does anything unreadable or deeper than the bounds.
inline bool bbox_node(uint32_t address) {
    return !(address&3u) && address>=0x80000000u && address<=0x80800000u-0x20u;
}
bool bbox_subtree_ordinal_free(uint8_t* rdram,uint32_t list) {
    uint32_t pending[32],top=0,visits=0;
    pending[top++]=list;
    while(top) {
        uint32_t node=pending[--top];
        for(;;) {
            if(++visits>1024 || !bbox_node(node))return false;
            const uint32_t type=uint32_t(MEM_W(0,(gpr)(int32_t)node));
            const uint32_t next=uint32_t(MEM_W(4,(gpr)(int32_t)node));
            uint32_t child=0;
            switch(type) {
            case 3: case 4: case 5: case 7: case 9: case 10: case 11: case 16: case 17:
            case 18: case 19: case 20: case 21: case 22: case 23: case 24: break;
            case 6: {
                const uint32_t offset=uint32_t(MEM_W(8,(gpr)(int32_t)node));
                if(!offset)return false;
                child=node+offset;
                break;
            }
            case 8: {
                const uint32_t offset=uint32_t(MEM_W(0x1C,(gpr)(int32_t)node));
                if(offset)child=node+offset;
                break;
            }
            case 13: {
                const int32_t offset=int16_t(MEM_H(0x14,(gpr)(int32_t)node));
                if(offset)child=node+uint32_t(offset);
                break;
            }
            default: return false;
            }
            if(child) {
                if(top==32)return false;
                pending[top++]=child;
            }
            if(!next)break;
            node+=next;
        }
    }
    return true;
}
}
#endif
// BK culling_patches GeoCmdD and transform_tagging's geo-wide frustum disable: the original bbox and
// in-graph sphere predicates retain their call side effects. At these exact post-call branches v0 alone
// decides child traversal; the guest decides it (tooie_bk_bbox_visible).
#ifdef _WIN32
extern "C" void baseline_bk_bbox_visible(uint8_t* rdram, recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    tooie_bk_bbox_visible(rdram,&clone);
    ctx->r2=clone.r2;
}
#else
// Linux: drawing every culled subtree doubled Jinjo Village's geometry and cost the Deck its
// 90 fps at 3x (linux/PORT_NOTES.md). A culled subtree stays culled when skipping it cannot shift
// BK ordinals; otherwise it is drawn, as BK does. The node is s0 in both bbox handlers (DDD14 sets it;
// DDC28 runs with DE2A4's s0, the node it dispatched). At the sphere seams (DDDC4, DDF14, DDFA0) s0 is
// that handler's own node, never type 13, so those are drawn as upstream draws them.
extern "C" void baseline_bk_bbox_visible(uint8_t* rdram,recomp_context* ctx) {
    if(ctx->r2!=0)return;
    const uint32_t node=uint32_t(ctx->r16);
    const bool keep_culled=bk_pipeline::bbox_node(node) && MEM_W(0,(gpr)(int32_t)node)==13 &&
        bk_pipeline::bbox_subtree_ordinal_free(rdram,node+uint32_t(int32_t(int16_t(MEM_H(0x14,(gpr)(int32_t)node)))));
    if(!keep_culled)ctx->r2=1;
}
#endif
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
