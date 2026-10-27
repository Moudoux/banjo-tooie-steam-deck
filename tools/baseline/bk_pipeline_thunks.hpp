// Included after ckcopy_thunks.hpp inside the existing camera native carrier.
// B1 is native, calling the same runtime functions BK's game_draw imports
// (osCreateMesgQueue / osExQueueDisplaylistEvent / osRecvMesg), with the queue in guest-visible
// .tooie_bk_state. B2 thunks are mechanical o32 marshalling; guest helpers own the tag policy.
#include "bk_pipeline_selection_layout.h"
#if !defined(TOOIE_BK_PIPELINE_SELECTED) || (TOOIE_BK_PIPELINE_SELECTED != 0 && TOOIE_BK_PIPELINE_SELECTED != 1)
#error Explicit boolean TOOIE_BK_PIPELINE_SELECTED required
#endif
#if TOOIE_BK_PIPELINE_SELECTED
// The DF944 UI owner's DE498-entry half (bk_menu_thunks.hpp, included later in this carrier).
static void baseline_bk_ui_de498_entry(uint8_t* rdram,recomp_context* ctx);

#ifdef BASELINE_PLAYER
#include "issue_evidence_guest.hpp"
extern "C" void baseline_bk_origin_slot_begin(uint8_t* rdram,recomp_context* ctx) {
    baseline::issue_evidence::guest::world_producer_begin(rdram,0x800BE90Cu,uint32_t(ctx->r13));
}
extern "C" void baseline_bk_origin_slot_end(uint8_t*,recomp_context*) {
    baseline::issue_evidence::guest::world_producer_end();
}
extern "C" void baseline_bk_origin_resolved_begin(uint8_t* rdram,recomp_context* ctx) {
    baseline::issue_evidence::guest::world_producer_begin(rdram,0x800BFB40u,uint32_t(ctx->r14));
}
extern "C" void baseline_bk_origin_resolved_end(uint8_t*,recomp_context*) {
    baseline::issue_evidence::guest::world_producer_end();
}
extern "C" void baseline_bk_origin_background_begin(uint8_t* rdram,recomp_context* ctx) {
    baseline::issue_evidence::guest::world_producer_begin(rdram,0x800BFE70u,uint32_t(ctx->r11));
}
extern "C" void baseline_bk_origin_background_end(uint8_t*,recomp_context*) {
    baseline::issue_evidence::guest::world_producer_end();
}
// DE498 first instruction: the DF944 UI owner, then the F4 caller witness (its exact a1 and the caller's
// sp; never translated r31). The owner runs first, so an owned UI root is no longer an unowned witness row.
extern "C" void baseline_bk_de498_entry(uint8_t* rdram,recomp_context* ctx) {
    baseline_bk_ui_de498_entry(rdram,ctx);
    baseline::issue_evidence::guest::de498_entry(rdram,uint32_t(ctx->r29),uint32_t(ctx->r5));
}
#else
// Origin hooks exist in every BK selection; diagnostics are player-only.
extern "C" void baseline_bk_origin_slot_begin(uint8_t*,recomp_context*) {}
extern "C" void baseline_bk_origin_slot_end(uint8_t*,recomp_context*) {}
extern "C" void baseline_bk_origin_resolved_begin(uint8_t*,recomp_context*) {}
extern "C" void baseline_bk_origin_resolved_end(uint8_t*,recomp_context*) {}
extern "C" void baseline_bk_origin_background_begin(uint8_t*,recomp_context*) {}
extern "C" void baseline_bk_origin_background_end(uint8_t*,recomp_context*) {}
extern "C" void baseline_bk_de498_entry(uint8_t* rdram,recomp_context* ctx) { baseline_bk_ui_de498_entry(rdram,ctx); }
#endif
#include <cstddef>
#include <cstring>
#include "ultramodern/extensions.h"
#include "guest/silo-candidate/bk_pipeline.h"
#include "guest/camera-candidate/camera_identity.h"   // guarded; adapters.cpp includes it again later
#include "camera_identity_layout.h"
static_assert(TOOIE_BK_STATE_LINKED_BYTES==TOOIE_BK_STATE_BYTES && TOOIE_BK_STATE%4u==0u &&
    uint64_t(TOOIE_BK_STATE)+TOOIE_BK_STATE_BYTES<=0x81000000ull,"BK state layout1 placement");
static_assert(TOOIE_BK_BRANCH_OFFSET+sizeof(TooieBkBranch)*TOOIE_BK_BRANCH_ENTRIES<=offsetof(TooieBkState,shadow_cuts) &&
    TOOIE_BK_SHADOW_BRANCH_OFFSET+sizeof(TooieBkBranch)*TOOIE_BK_BRANCH_ENTRIES==TOOIE_BK_STATE_BYTES &&
    offsetof(TooieBkState,branches)==TOOIE_BK_BRANCH_OFFSET &&
    offsetof(TooieBkState,shadow_branches)==TOOIE_BK_SHADOW_BRANCH_OFFSET &&
    offsetof(TooieBkState,branch_commits)==TOOIE_BK_BRANCH_COMMITS_OFFSET,"BK branch history layout");
extern "C" void tooie_bk_frame_begin(uint8_t*,recomp_context*);
extern "C" void tooie_bk_root_begin(uint8_t*,recomp_context*);
extern "C" void tooie_bk_root_end(uint8_t*,recomp_context*);
// Native-seam policy helpers (bk_pipeline.c): the guest decides, these thunks only marshal.
extern "C" void tooie_bk_branch_submitted(uint8_t*,recomp_context*);
extern "C" void tooie_bk_lod_distance(uint8_t*,recomp_context*);
extern "C" void tooie_bk_player_lod_distance(uint8_t*,recomp_context*);
extern "C" void tooie_bk_player_lod_tier(uint8_t*,recomp_context*);
namespace bk_pipeline {
constexpr gpr state=(gpr)(int32_t)TOOIE_BK_STATE;
constexpr uint32_t queue=TOOIE_BK_STATE+TOOIE_BK_PARSED_QUEUE_OFFSET;
constexpr uint32_t message=TOOIE_BK_STATE+TOOIE_BK_PARSED_MESSAGE_OFFSET;
void count(uint8_t* rdram,uint32_t word) {
    const uint32_t value=uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET+word,state));
    if(value!=0xFFFFFFFFu)MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET+word,state)=int32_t(value+1u);
}
// BK game_draw 125-142: fatal after construction when the physically backed list overflowed.
// This checks the selected Gfx pool. Float camera blocks are charged to the expanded ordinary
// matrix arenas by camera_emitters.c; their nested lists do not grow this Gfx cursor.
void construction_end_check(uint8_t* rdram) {
    const int index=MEM_H(0,int32_t(0x8007BEDA));
    if(index<0 || index>1 || MEM_W(0,int32_t(0x8007BCE8))!=0)return;
    const uint64_t slot=uint64_t(TOOIE_CAMERA_MAIN_VRAM)+uint64_t(index)*TOOIE_CAMERA_M_BYTES;
    if(uint32_t(MEM_W(index*4,int32_t(0x8007BCD0)))!=slot)return;
    const uint64_t cursor=uint32_t(MEM_W(0,int32_t(0x80127658)));
    if(cursor<slot || cursor>slot+TOOIE_CAMERA_M_BYTES)rejected_body("BK construction-end Gfx main pool overflow");
}
}
namespace bk_pipeline {
// Guest RAM range for native reads (transition view_of): KSEG0, 8 MiB, word-aligned.
inline bool lod_word(uint32_t address,uint32_t bytes) {
    return !(address&3u) && address>=0x80000000u && uint64_t(address)+bytes<=0x80800000ull;
}
// The guest LOD helpers return IEEE-754 bits in v0; f0 is written only when they change it.
inline void set_f0(recomp_context* ctx,uint32_t bits) {
    uint32_t current;std::memcpy(&current,&ctx->f0.fl,4);
    if(current!=bits)std::memcpy(&ctx->f0.fl,&bits,4);
}
}
// BK GeoCmd8 fixed distance (tooie_bk_lod_distance), after each original distance function returns;
// null-child and table0 ordinal-override routes never reach these seams. Only f0 changes.
extern "C" void baseline_bk_lod_distance(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    tooie_bk_lod_distance(rdram,&clone);
    bk_pipeline::set_f0(ctx,uint32_t(clone.r2));
}
// 94070 940B8: EEAD4 has returned the linear player-camera distance in f0 and s0 is the player; the
// guest keeps or fixes it (tooie_bk_player_lod_distance). Reaching this call already passed the
// original D90A4 timer gate. Only f0 changes.
extern "C" void baseline_bk_player_lod_distance(uint8_t* rdram,recomp_context* ctx) {
    uint32_t distance;std::memcpy(&distance,&ctx->f0.fl,4);
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r16;clone.r5=(gpr)(int32_t)distance;
    tooie_bk_player_lod_distance(rdram,&clone);
    bk_pipeline::set_f0(ctx,uint32_t(clone.r2));
}
// Fixed-detail initialization: exact 94038 caller scope authenticates 93F30 entry after
// its original delay slot loaded a0; translated r31 is not caller provenance. The caller's sp+0x18 holds
// the player; the guest decides the tier (tooie_bk_player_lod_tier) and the original install path runs unchanged.
// Native guest-thread lexical token, never inferred from translated r31.
struct BaselineBkInitialLodScope { uint8_t* rdram; recomp_context* ctx; bool armed; };
static thread_local BaselineBkInitialLodScope baseline_bk_initial_lod_scope{};
extern "C" void baseline_bk_player_lod_initial_begin(uint8_t* rdram,recomp_context* ctx) {
    baseline_bk_initial_lod_scope = {rdram, ctx, true};
}
extern "C" void baseline_bk_player_lod_initial_end(uint8_t*, recomp_context*) {
    baseline_bk_initial_lod_scope = {};
}
extern "C" void baseline_bk_player_lod_initial(uint8_t* rdram,recomp_context* ctx) {
    const auto scope = baseline_bk_initial_lod_scope;
    baseline_bk_initial_lod_scope = {}; // Consume before any refusal or guest call.
    if (!scope.armed || scope.rdram != rdram || scope.ctx != ctx) return;
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=(gpr)(int32_t)MEM_W(0x18,ctx->r29);clone.r5=ctx->r4;
    tooie_bk_player_lod_tier(rdram,&clone);
    if(uint32_t(clone.r2)!=uint32_t(ctx->r4))ctx->r4=clone.r2;
}
// A7888, appended after the camera construction begin.
extern "C" void baseline_bk_frame_begin(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    tooie_bk_frame_begin(rdram,&clone);
}
// A7840 before the 78E8 JAL to 1A270, appended after baseline_first_graphics_submit.
// BK registers PARSED for gfx_start before submission; Tooie's submitted start is the original
// A7840 sp+0x1C slot (= a0, = OSTask data_ptr through 1A1E0/13EF0). Reaching 78E8 proves the
// 789C branch chose submission: the suppression flag is never re-tested here.
extern "C" void baseline_bk_submit_register(uint8_t* rdram,recomp_context* ctx) {
    using namespace bk_pipeline;
    const uint32_t submitted=uint32_t(MEM_W(0x1C,ctx->r29));
    if(uint32_t(ctx->r4)!=submitted)rejected_body("BK parsed arm: a0 differs from the saved A7840 root");
    if(MEM_W(TOOIE_BK_PARSED_ARMED_OFFSET,state))rejected_body("BK parsed arm: previous acknowledgement not received");
    osCreateMesgQueue(rdram,int32_t(queue),int32_t(message),1);
    osExQueueDisplaylistEvent(int32_t(queue),NULLPTR,int32_t(submitted),OS_EX_DISPLAYLIST_EVENT_PARSED);
    MEM_W(TOOIE_BK_PARSED_ROOT_OFFSET,state)=int32_t(submitted);
    MEM_W(TOOIE_BK_PARSED_ARMED_OFFSET,state)=1;
    count(rdram,TOOIE_BK_DIAG_PARSED_ARMS);
    // This construction (diagnostics.frames, counted at A7888) is the workload RT64 receives: only
    // here does a construction commit its visible-branch baseline (tooie_bk_branch_submitted); a
    // suppressed one never does.
    recomp_context clone;camera_clone_context(clone,*ctx);
    tooie_bk_branch_submitted(rdram,&clone);
    baseline_bk_menu_submitted(rdram);
}
// A7840 78F0: after 1A270 returns, and the 789C suppression destination. Blocks only for an
// arm made by this invocation, before 15878/E4628 and every later same-thread guest write.
// The original 15410 wait is unchanged.
extern "C" void baseline_bk_submit_wait(uint8_t* rdram,recomp_context* ctx) {
    using namespace bk_pipeline;
    (void)ctx;
    construction_end_check(rdram);
    if(!MEM_W(TOOIE_BK_PARSED_ARMED_OFFSET,state)){count(rdram,TOOIE_BK_DIAG_SUPPRESSED_WAITS);return;}
    osRecvMesg(rdram,int32_t(queue),NULLPTR,OS_MESG_BLOCK);
    MEM_W(TOOIE_BK_PARSED_ARMED_OFFSET,state)=0;
    count(rdram,TOOIE_BK_DIAG_PARSED_WAITS);
}
#ifdef BASELINE_PLAYER
#include "issue_evidence_guest.hpp"
namespace bk_pipeline {
// F4 BK root evidence (read-only): native begin snapshots paired at DF234. Game thread only.
baseline::issue_evidence::guest::BkRoots evidence_roots{};
void evidence(const baseline::issue_evidence::BkDrawRecord& record) { baseline::issue_evidence::record(record); }
}
#endif
// DE498 DF0B0: buffers sp+0xF8, local cursor word sp+0xCC, effective model sp+0x10C.
extern "C" void baseline_bk_root_begin(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_de498_args(rdram,clone,*ctx);
#ifdef BASELINE_PLAYER
    const uint32_t evidence_depth=baseline::issue_evidence::guest::bk_root_before(bk_pipeline::evidence_roots,rdram,bk_pipeline::evidence);
#endif
    tooie_bk_root_begin(rdram,&clone);
#ifdef BASELINE_PLAYER
    baseline::issue_evidence::guest::bk_root_after(bk_pipeline::evidence_roots,rdram,uint32_t(ctx->r29),evidence_depth);
#endif
}
// DE498 DF234: the active b->gfx (buffers at sp+0xF8) is the post-pop cursor.
extern "C" void baseline_bk_root_end(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=(gpr)(int32_t)MEM_W(0xF8,ctx->r29);
#ifdef BASELINE_PLAYER
    // Before the helper pops: the pending begin of this depth and DE498 frame is this root's.
    baseline::issue_evidence::guest::bk_root_end(bk_pipeline::evidence_roots,rdram,uint32_t(ctx->r29),bk_pipeline::evidence);
#endif
    tooie_bk_root_end(rdram,&clone);
}
#endif

#if TOOIE_BK_PIPELINE_SELECTED
#ifdef BASELINE_PLAYER
extern "C" void baseline_transition_fade_80800734(uint8_t* rdram,recomp_context* ctx) { baseline::issue_evidence::guest::transition_fade_after(rdram,*ctx,0x8080072Cu); }
extern "C" void baseline_transition_fade_80800778(uint8_t* rdram,recomp_context* ctx) { baseline::issue_evidence::guest::transition_fade_after(rdram,*ctx,0x80800770u); }
extern "C" void baseline_transition_fade_808007f0(uint8_t* rdram,recomp_context* ctx) { baseline::issue_evidence::guest::transition_fade_after(rdram,*ctx,0x808007E8u); }
extern "C" void baseline_transition_fade_80800834(uint8_t* rdram,recomp_context* ctx) { baseline::issue_evidence::guest::transition_fade_after(rdram,*ctx,0x8080082Cu); }
#else
extern "C" void baseline_transition_fade_80800734(uint8_t*,recomp_context*) {}
extern "C" void baseline_transition_fade_80800778(uint8_t*,recomp_context*) {}
extern "C" void baseline_transition_fade_808007f0(uint8_t*,recomp_context*) {}
extern "C" void baseline_transition_fade_80800834(uint8_t*,recomp_context*) {}
#endif
#endif
