#pragma once
// Read-only snapshots on the existing synchronous guest owners. No guest calls,
// pointers retained for later dereference, or route/admission decisions.
#include "issue_evidence.hpp"
#include "camera_identity_layout.h"
#include "render_bridge_layout.h"
#include "cpu625_selection_layout.h"
#include "shadow_projection_layout.h"
#include "guest/camera-candidate/camera_identity.h"
#include "guest/silo-candidate/render_bridge.h"
#include "bk_pipeline_selection_layout.h"
#if !defined(TOOIE_BK_PIPELINE_SELECTED) || (TOOIE_BK_PIPELINE_SELECTED != 0 && TOOIE_BK_PIPELINE_SELECTED != 1)
#error Explicit boolean TOOIE_BK_PIPELINE_SELECTED required
#endif
#if TOOIE_BK_PIPELINE_SELECTED
#include "guest/silo-candidate/bk_pipeline.h"
#endif
#include <bit>
#include <type_traits>

namespace baseline::issue_evidence::guest {
enum FramePhase : uint32_t { Begin=1, End=2 };
// FrameRecord.valid uses Layout/Frame from DrawRecord. Frame validates the
// active construction owner and selected command pool; it is not a completed frame.
inline constexpr uint32_t matrix_pool_bytes=0x2EE00u;
// Original scalar ABI, not selected helper-image addresses. The latter always
// come from generated layout headers above.
inline constexpr uint32_t buffers_address=0x80127658u;
inline constexpr uint32_t pool_index_address=0x8007BEDAu; // s16
inline constexpr uint32_t gfx_pool_address=0x8007BCD0u;
inline constexpr uint32_t matrix_pool_address=0x8007BCD8u;
inline constexpr uint32_t view_address=0x80127674u; // s32
inline constexpr uint32_t phase_address=0x80132E9Cu; // s16, original EB854 getter
inline constexpr uint32_t descriptor_address=0x80132ED0u;
inline constexpr uint32_t descriptor_stride=0x30u;
inline constexpr uint32_t actor_bitmap_address=0x80135450u;
inline constexpr uint32_t actor_pool_address=0x80136EE0u;
inline constexpr uint32_t control_slot_address=0x801354DCu; // u32, current control owner
inline constexpr uint32_t camera_address=0x8012D500u;
inline constexpr uint32_t redirected_address=0x8012D508u;
inline constexpr uint32_t projection_address=0x8012D558u;
inline constexpr uint32_t effective_asset_address=0x8012CA18u;
inline constexpr uint32_t world_policy=0x02008AAAu; // CAM_POLICY_WORLD, public packed GBI tuple
inline constexpr uint32_t draw_callback_address=0x8012C948u+24u*4u; // C9A8, DF47C-staged callback
inline constexpr uint32_t draw_owner_address=0x8012C948u+25u*4u;    // C9AC, its staged owner
inline constexpr uint32_t player_callback=0x80092234u;              // PlayerState body callback
inline constexpr uint32_t player_secondary_callback=0x80092224u;    // second top-level player route
inline constexpr uint32_t player_table_address=0x80135490u;         // original D_80135490[8]
inline constexpr uint32_t player_slot_offset=0x184u;                // PlayerOwner.slot (player_boundary.c)

inline bool range(uint32_t p, uint32_t bytes, uint32_t alignment=4) noexcept {
    return p>=0x80000000u && uint64_t(p)+bytes<=0x81000000ull && !(p&(alignment-1u));
}
inline uint32_t word(uint8_t* rdram, uint32_t p) noexcept {
    return uint32_t(MEM_W(0,(gpr)(int32_t)p));
}
inline uint32_t half(uint8_t* rdram, uint32_t p) noexcept {
    return uint32_t(MEM_HU(0,(gpr)(int32_t)p));
}
inline uint32_t byte(uint8_t* rdram, uint32_t p) noexcept {
    return uint32_t(MEM_BU(0,(gpr)(int32_t)p));
}
// Every ABI member is a u32. Decode guest words before forming native values;
// casting RDRAM to these structs would incorrectly bypass guest memory decoding.
template<class T> inline T abi(uint8_t* rdram, uint32_t p) noexcept {
    static_assert(std::is_trivially_copyable_v<T> && sizeof(T)%4==0 && alignof(T)==4);
    std::array<uint32_t,sizeof(T)/4> words{};
    for(size_t i=0;i<words.size();++i) words[i]=word(rdram,p+uint32_t(i)*4u);
    return std::bit_cast<T>(words);
}
inline uint32_t bridge_word(uint8_t* rdram, size_t offset) noexcept {
    return word(rdram,TOOIE_BRIDGE_STATE_VRAM+uint32_t(offset));
}
inline bool layout(uint8_t* rdram) noexcept {
    return rdram && range(TOOIE_CAMERA_STATE_VRAM,sizeof(TooieCameraState)) &&
        range(TOOIE_BRIDGE_STATE_VRAM,sizeof(TooieRenderBridgeState)) &&
        word(rdram,TOOIE_CAMERA_STATE_VRAM+offsetof(TooieCameraState,control)+offsetof(TooieCameraControl,magic))==TOOIE_CAMERA_MAGIC &&
        word(rdram,TOOIE_CAMERA_STATE_VRAM+offsetof(TooieCameraState,control)+offsetof(TooieCameraControl,layout))==TOOIE_CAMERA_LAYOUT &&
        bridge_word(rdram,offsetof(TooieRenderBridgeState,magic))==TOOIE_BRIDGE_MAGIC &&
        bridge_word(rdram,offsetof(TooieRenderBridgeState,layout))==TOOIE_BRIDGE_LAYOUT;
}
#if TOOIE_CPU625_SELECTED
// Leading words of guest cpu625_layout.h TooieCpu625State. That header is guest C
// only, so the native read mirrors its header and authenticates its magic/layout.
struct Cpu625Header { uint32_t magic, layout, next_block, epoch; };
inline constexpr uint32_t cpu625_magic=0x36323531u, cpu625_layout=1u;
static_assert(sizeof(Cpu625Header)==16 && TOOIE_CPU625_STATE_BYTES==464u,
    "generated CPU625 state extent and authenticated header");
#endif
#if TOOIE_SHADOW_SELECTED
// Guest shadow_projection.c ShadowState: 56 bytes by its own static assert, and the
// generator checks its image as this magic plus 52 zero bytes. Word offsets below.
inline constexpr uint32_t shadow_magic=0x53484403u, shadow_bytes=56u;
static_assert(TOOIE_SHADOW_ARGS==TOOIE_SHADOW_STATE+36u,"generated shadow args follow state+36");
#endif
inline TooieCameraControl control(uint8_t* rdram) noexcept {
    return abi<TooieCameraControl>(rdram,TOOIE_CAMERA_STATE_VRAM+offsetof(TooieCameraState,control));
}
inline uint32_t owner_frame(uint8_t* rdram) noexcept {
    return word(rdram,TOOIE_CAMERA_STATE_VRAM+offsetof(TooieCameraState,owner_frame));
}
inline bool frame(uint8_t* rdram, const TooieCameraControl& c, uint32_t pool) noexcept {
    if(pool>=2u || !c.active || !c.epoch || c.epoch==0xFFFFFFFFu ||
        c.epoch!=bridge_word(rdram,offsetof(TooieRenderBridgeState,epoch)) ||
        !range(owner_frame(rdram),0x20u)) return false;
    const uint32_t base=TOOIE_CAMERA_MAIN_VRAM+pool*TOOIE_CAMERA_M_BYTES;
    return range(base,TOOIE_CAMERA_M_BYTES,16) && c.main_base==base &&
        c.main_end==base+TOOIE_CAMERA_M_BYTES && c.main_begin>=base && c.main_begin<=c.main_end &&
        word(rdram,gfx_pool_address+pool*4u)==base;
}

#if TOOIE_BK_PIPELINE_SELECTED
inline bool transition_frame(uint8_t* rdram,uint32_t& frame_out) noexcept {
    if(!rdram || !range(TOOIE_BK_STATE,TOOIE_BK_STATE_BYTES) ||
       word(rdram,TOOIE_BK_STATE+offsetof(TooieBkState,magic))!=TOOIE_BK_MAGIC ||
       word(rdram,TOOIE_BK_STATE+offsetof(TooieBkState,layout))!=TOOIE_BK_LAYOUT)return false;
    frame_out=word(rdram,TOOIE_BK_STATE+offsetof(TooieBkState,diagnostics)+offsetof(TooieBkDiagnostics,frames));
    return true;
}
inline void transition_fade_after(uint8_t* rdram,const recomp_context& ctx,uint32_t site) noexcept {
    uint32_t frame_id=0;
    if(!transition_frame(rdram,frame_id))return;
    // 80800834 is also the non-kind9 bypass target. The source leaves kind unchanged through this draw.
    if(site==0x8080082Cu && (!range(uint32_t(ctx.r16),0x30u) || byte(rdram,uint32_t(ctx.r16)+8u)!=9u))return;
    auto& n=tooie_transition_fade::current;
    if(n.ram!=rdram || n.value.frame!=frame_id){n={};n.ram=rdram;n.value.frame=frame_id;}
    auto& r=n.value;const uint32_t view=word(rdram,view_address);
    if(r.attempts && r.view!=view)r.mixed_views=1;
    if(r.attempts!=UINT32_MAX)++r.attempts;
    r.valid=0;r.view=view;r.site=site;r.commands={};
    r.object=uint32_t(ctx.r16);r.gfx=0;r.begin=0;r.end=0;
    r.kind=r.flags=r.elapsed_word=r.duration_word=0;
    const auto refuse=[&](){if(r.refused!=UINT32_MAX)++r.refused;};
    const uint32_t sp=uint32_t(ctx.r29);
    if(!range(sp,0x78u) || !range(r.object,0x30u)){refuse();return;}
    r.kind=byte(rdram,r.object+8u);r.flags=byte(rdram,r.object+0x1Cu);
    r.elapsed_word=word(rdram,r.object+0x28u);r.duration_word=word(rdram,r.object+0xCu);
    r.gfx=word(rdram,sp+0x74u);
    if(!range(r.gfx,4u)){refuse();return;}
    r.end=word(rdram,r.gfx);
    if((r.end&7u) || r.end<40u || !range(r.end-40u,40u)){refuse();return;}
    r.begin=r.end-40u;
    std::array<uint32_t,10> words{};
    for(uint32_t i=0;i<10u;++i)words[i]=word(rdram,r.begin+4u*i);
    if(words[0]!=0xDE000000u || words[1]!=0x0011A6E0u || words[2]!=0xF8000000u ||
       (words[4]&0xFF000000u)!=0xE4000000u || (words[5]&0xFF000000u)!=0u ||
       words[6]!=0xE1000000u || words[7]!=0u || words[8]!=0xF1000000u || words[9]!=0x01000100u){refuse();return;}
    r.commands=words;r.valid=1;
}
#endif
inline FrameRecord read_frame(uint8_t* rdram, const recomp_context& ctx, uint32_t phase) noexcept {
    FrameRecord out{};
    out.menu=tooie_menu_witness::current;
#if TOOIE_BK_PIPELINE_SELECTED
    uint32_t transition_id=0;
    if(phase==End && transition_frame(rdram,transition_id) && tooie_transition_fade::current.ram==rdram &&
       tooie_transition_fade::current.value.frame==transition_id)
        out.transition_fade=tooie_transition_fade::current.value;
#endif
#if TOOIE_BK_PIPELINE_SELECTED
    if(rdram && range(TOOIE_BK_STATE,TOOIE_BK_STATE_BYTES) &&
       word(rdram,TOOIE_BK_STATE+offsetof(TooieBkState,magic))==TOOIE_BK_MAGIC &&
       word(rdram,TOOIE_BK_STATE+offsetof(TooieBkState,layout))==TOOIE_BK_LAYOUT){
        out.shadow_branch_valid=1u;
        out.shadow_branch_frame=word(rdram,TOOIE_BK_STATE+offsetof(TooieBkState,diagnostics)+offsetof(TooieBkDiagnostics,frames));
        out.shadow_cuts=word(rdram,TOOIE_BK_STATE+offsetof(TooieBkState,shadow_cuts));
        out.shadow_declined=word(rdram,TOOIE_BK_STATE+offsetof(TooieBkState,shadow_declined));
    }
#endif
    out.phase=phase;
    if(!rdram) return out;
    out.map=half(rdram,0x80132DC2u); // u16 original EA05C world-section getter
    out.pool_index=uint32_t(int32_t(int16_t(half(rdram,pool_index_address))));
    out.view=word(rdram,view_address);
    out.camera=word(rdram,camera_address);
    out.projection=word(rdram,projection_address);
    out.control_slot=word(rdram,control_slot_address);
    out.requested_period=int32_t(word(rdram,0x8007913Cu)); // s32 minimum period
    out.measured_period=int16_t(half(rdram,0x80078F64u)); // completed s16 VI count
    out.live_period=int32_t(word(rdram,0x80079138u)); // live s32 scheduler count
    if(!layout(rdram)) return out;
    out.valid=DrawRecord::Layout;
    const auto c=control(rdram);
    out.epoch=c.epoch;
    out.camera_active=c.active;
    out.camera_latch=c.latch;
    out.bridge_poisoned=bridge_word(rdram,offsetof(TooieRenderBridgeState,poisoned));
    const auto player=abi<TooiePlayerBoundary>(rdram,TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,player));
    // Later aggregate metadata, never the outcome of a native player-begin event.
    out.player_owner=player.owner;
    out.player_model=player.model;
    out.player_asset=player.effective_model_asset;
    out.player_admissions=player.admissions;
    out.player_refusals=player.refusals;
    out.player_last_reason=player.last_reason;
    out.player_view=player.view_phase&3u;
    out.player_primary_bytes=bridge_word(rdram,offsetof(TooieRenderBridgeState,primary_bytes));
    out.player_primary_highwater=bridge_word(rdram,offsetof(TooieRenderBridgeState,primary_highwater));
    out.secondary_opens=bridge_word(rdram,offsetof(TooieRenderBridgeState,secondary_route)+offsetof(TooieSecondaryRoute,opens));
    out.secondary_faults=bridge_word(rdram,offsetof(TooieRenderBridgeState,secondary_route)+offsetof(TooieSecondaryRoute,faults));
    out.secondary_seen_main=bridge_word(rdram,offsetof(TooieRenderBridgeState,secondary_ledger)+offsetof(TooieSecondaryLedger,seen_main));
    out.secondary_seen_late=bridge_word(rdram,offsetof(TooieRenderBridgeState,secondary_ledger)+offsetof(TooieSecondaryLedger,seen_late));
    out.secondary_unauthenticated=bridge_word(rdram,offsetof(TooieRenderBridgeState,secondary_ledger)+offsetof(TooieSecondaryLedger,unauthenticated));
    out.secondary_admissions=bridge_word(rdram,offsetof(TooieRenderBridgeState,secondary_ledger)+offsetof(TooieSecondaryLedger,admissions));
    out.secondary_ledger_bytes=bridge_word(rdram,offsetof(TooieRenderBridgeState,secondary_ledger)+offsetof(TooieSecondaryLedger,bytes));
    out.bridge_next_block=bridge_word(rdram,offsetof(TooieRenderBridgeState,next_block));
#if TOOIE_CPU625_SELECTED
    if(range(TOOIE_CPU625_STATE,TOOIE_CPU625_STATE_BYTES)) {
        const auto cpu=abi<Cpu625Header>(rdram,TOOIE_CPU625_STATE);
        if(cpu.magic==cpu625_magic && cpu.layout==cpu625_layout) {
            out.cpu625_next_block=cpu.next_block;
            out.valid|=FrameRecord::Cpu625;
        }
    }
#endif
    if(frame(rdram,c,out.pool_index) && owner_frame(rdram)==uint32_t(ctx.r29)) out.valid|=DrawRecord::Frame;
#if TOOIE_SHADOW_SELECTED
    // tooie_shadow_reset stamps this construction's epoch at Begin. At this owner's End,
    // a matching magic and epoch make the words this frame's final shadow state: the last
    // source callsite, any admission (seen_epoch), and leftovers. Earlier calls are not kept.
    constexpr uint32_t s=TOOIE_SHADOW_STATE;
    if(phase==End && (out.valid&DrawRecord::Frame) && c.epoch && range(s,shadow_bytes) &&
       word(rdram,s)==shadow_magic && word(rdram,s+4u)==c.epoch) {
        out.shadow_id=word(rdram,s+8u); out.shadow_seen_epoch=word(rdram,s+16u);
        out.shadow_group=word(rdram,s+20u); out.shadow_composite=word(rdram,s+24u);
        out.shadow_mode_open=word(rdram,s+28u); out.shadow_fault=word(rdram,s+32u);
        for(uint32_t i=0;i<4u;++i) out.shadow_args[i]=word(rdram,s+36u+i*4u);
        out.shadow_source_pc=word(rdram,s+52u);
        out.shadow_valid=1u;
    }
#endif
    return out;
}

// Diagnostic clone of actor_binding's lifetime predicates. This does not invoke
// the guest helper or establish any interpolation-route admission.
inline bool binding(uint8_t* rdram, const TooieActorPaletteContext& a) noexcept {
    if(a.slot>=TOOIE_BRIDGE_SLOT_COUNT || a.descriptor!=descriptor_address+a.slot*descriptor_stride ||
        !a.birth || a.birth>TOOIE_BRIDGE_MAX_BIRTH ||
        bridge_word(rdram,offsetof(TooieRenderBridgeState,births)+a.slot*4u)!=a.birth ||
        !(byte(rdram,actor_bitmap_address+(a.slot>>3))&(0x80u>>(a.slot&7u))) ||
        a.actor<0x80000000u || a.actor>0x807FFF64u || !range(a.actor,0x9Cu) ||
        word(rdram,a.actor)!=a.descriptor ||
        ((half(rdram,a.descriptor+0x14u)<<16)|half(rdram,a.descriptor+0x16u))!=a.pair) return false;
    const uint32_t pool=word(rdram,actor_pool_address);
    if(pool<0x80000000u || pool>0x807FFFF0u || !range(pool,16u)) return false;
    const uint32_t stride=word(rdram,pool), begin=word(rdram,pool+4u),
        end=word(rdram,pool+8u), mem_end=word(rdram,pool+12u);
    if(stride<0x9Cu || stride>0xFFFFu || begin<0x80000000u || begin>=0x80800000u ||
        end<begin || end>=0x80800000u || mem_end<end || mem_end>=0x80800000u) return false;
    const uint32_t index=half(rdram,a.descriptor+0x1Au)>>5;
    return (index+1u)*stride<=end-begin && begin+index*stride==a.actor;
}
inline bool matrix(uint32_t p, uint32_t base) noexcept {
    return range(base,matrix_pool_bytes,8) && range(p,64u,8) && p>=base &&
        uint64_t(p)+64u<=uint64_t(base)+matrix_pool_bytes && !((p-base)&63u);
}
inline void copy_matrix(uint8_t* rdram,uint32_t p,std::array<uint32_t,16>& out) noexcept {
    for(uint32_t i=0;i<out.size();++i) out[i]=word(rdram,p+i*4u);
}
inline DrawRecord read_draw(uint8_t* rdram, const recomp_context& ctx) noexcept {
    DrawRecord out{};
    if(!rdram) return out;
    out.pool_index=uint32_t(int32_t(int16_t(half(rdram,pool_index_address))));
    out.view=word(rdram,view_address);
    out.asset=word(rdram,effective_asset_address); // original current asset publication
    const uint32_t sp=uint32_t(ctx.r29);
    const bool stack=range(sp,0x110u);
    if(stack) {
        out.buffers=word(rdram,sp+0xF8u);
        out.cursor=word(rdram,sp+0xCCu); // original local cursor, not buffers->gfx
        out.model=word(rdram,sp+0x10Cu); // postcallback/cache effective model
    }
    if(!layout(rdram)) return out;
    out.valid=DrawRecord::Layout;
    const auto c=control(rdram);
    const auto a=abi<TooieActorPaletteContext>(rdram,TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,palette_context));
    const auto p=abi<TooieCameraProduction>(rdram,TOOIE_CAMERA_STATE_VRAM+offsetof(TooieCameraState,saved));
    out.epoch=c.epoch;
    out.actor=a.actor; out.descriptor=a.descriptor; out.slot=a.slot; out.birth=a.birth;
    out.context_epoch=a.epoch; out.context_admitted=a.admitted; // raw context, not a helper result
    out.camera=p.camera; out.camera_birth=p.birth; out.camera_revision=p.revision;
    out.camera_id=p.id; out.camera_scale=p.scale; out.camera_policy=p.policy;
    out.projection_address=p.matrix;
    const bool current_frame=stack && frame(rdram,c,out.pool_index);
    if(current_frame) out.valid|=DrawRecord::Frame;
    const bool current_buffers=current_frame && out.buffers==buffers_address &&
        !(out.cursor&7u) && out.cursor>=c.main_base && out.cursor>=c.main_begin && out.cursor<=c.main_end &&
        !word(rdram,redirected_address) && !word(rdram,0x8007BCE8u) && !word(rdram,0x8007BCECu) &&
        !word(rdram,0x8007BCF0u) && !word(rdram,0x8007BCF4u);
    uint32_t mtxbase=0;
    if(current_buffers) {
        // Original GraphicsBuffers has gfx at +0 and mtx at +4. DEF94's
        // mlMtxApply has written this root before this first DEFA0 transport.
        out.root_address=word(rdram,out.buffers+4u);
        mtxbase=word(rdram,matrix_pool_address+out.pool_index*4u);
        if(matrix(out.root_address,mtxbase)) {
            copy_matrix(rdram,out.root_address,out.root);
            out.valid|=DrawRecord::Root;
        }
    }
    const uint32_t phase=uint32_t(int32_t(int16_t(half(rdram,phase_address))));
    // Both selected capture owners clear admitted before this first transport.
    // A nonzero value remains visible above, but cannot authenticate a fresh call.
    const bool current_context=current_buffers && a.actor && !a.admitted && a.buffers==out.buffers &&
        bridge_word(rdram,offsetof(TooieRenderBridgeState,silo_depth))==1u && a.epoch==c.epoch &&
        a.view==out.view && a.view<4u && a.phase==phase &&
        (phase==0u || phase==1u || phase==2u || phase==5u || phase==6u) && binding(rdram,a) &&
        word(rdram,0x8012C948u+25u*4u)==a.actor;
    if(current_context) out.valid|=DrawRecord::Context;
    // Last saved WORLD production only. These mirror the camera identity/key
    // checks used by cam_actor_camera_id, but do not test PRE pending admission
    // or assert this is every view, this draw's effective stack, or an admit result.
    if(!current_buffers || !(out.valid&DrawRecord::Root) || c.latch ||
        bridge_word(rdram,offsetof(TooieRenderBridgeState,poisoned)) ||
        !p.valid || p.epoch!=c.epoch || p.matrix!=word(rdram,projection_address) ||
        p.camera!=word(rdram,camera_address) || p.view!=out.view || p.view>=4u ||
        p.id<TOOIE_CAMERA_ID_FIRST || p.id>=TOOIE_CAMERA_ID_END || p.policy!=world_policy ||
        p.role!=TOOIE_CAMERA_WORLD_INITIAL || !p.birth || p.camera>0x807FFE98u || !range(p.camera,0x168u) ||
        !matrix(p.matrix,mtxbase)) return out;
    for(uint32_t i=0;i<TOOIE_CAMERA_RECORDS;++i) {
        const auto camera=abi<TooieCameraRecord>(rdram,TOOIE_CAMERA_STATE_VRAM+offsetof(TooieCameraState,camera)+i*sizeof(TooieCameraRecord));
        if(camera.valid && camera.pointer==p.camera) {
            if(camera.birth==p.birth && camera.revision==p.revision && camera.view_id[p.view]==p.id &&
                camera.view_scale[p.view]==p.scale) {
                copy_matrix(rdram,p.matrix,out.projection);
                out.valid|=DrawRecord::Projection;
            }
            break;
        }
    }
    return out;
}
// DE448 calls tooie_player_begin only at bridge depth 1 with C9A8 == 92234, then calls
// DE498 directly; this DEFA0 is the first transport after that call. Nested model draws
// run at depth 2 and the DF944 UI route at depth 0. A nested direct DE498 can follow
// only after this DEFA0, so attribute just the first DEFA0 per begin call. Unknown stays unset.
inline void join_player(uint8_t* rdram, DrawRecord& out) noexcept {
    static uint32_t joined_calls=0; // Native latch; DEFA0 runs on the one game thread.
    if(!(out.valid&DrawRecord::Layout) ||
       bridge_word(rdram,offsetof(TooieRenderBridgeState,silo_depth))!=1u ||
       word(rdram,draw_callback_address)!=player_callback) return;
    const auto pc=abi<TooiePlayerBoundary>(rdram,TOOIE_BRIDGE_STATE_VRAM+offsetof(TooieRenderBridgeState,player));
    // Disabled begin returns before counting or writing a reason; saturated calls prove nothing.
    if(!pc.enabled || !pc.calls || pc.calls==0xFFFFFFFFu || pc.calls==joined_calls) return;
    joined_calls=pc.calls;
    const uint32_t owner=word(rdram,draw_owner_address);
    // Admission writes 0xFFFFFFFF; it must agree with the live admitted tuple.
    if(pc.last_reason==0xFFFFFFFFu &&
       (bridge_word(rdram,offsetof(TooieRenderBridgeState,active))!=1u ||
        pc.stamp!=bridge_word(rdram,offsetof(TooieRenderBridgeState,epoch)) || pc.owner!=owner)) return;
    out.player_owner=owner;
    out.player_outcome=pc.last_reason;
    out.player_control_slot=word(rdram,control_slot_address);
    out.valid|=DrawRecord::PlayerJoin;
    if(range(owner,player_slot_offset+4u)) {
        const uint32_t slot=word(rdram,owner+player_slot_offset);
        out.player_owner_slot=slot;
        out.player_table_match=slot<8u && word(rdram,player_table_address+slot*4u)==owner;
        out.valid|=DrawRecord::PlayerSlot;
    }
}
struct SkeletalCounters { uint32_t calls, admissions, refusals; };
inline SkeletalCounters skeletal_counters(uint8_t* rdram) noexcept {
    return {bridge_word(rdram,offsetof(TooieRenderBridgeState,silo_calls)),
            bridge_word(rdram,offsetof(TooieRenderBridgeState,silo_admissions)),
            bridge_word(rdram,offsetof(TooieRenderBridgeState,silo_refusals))};
}
// Every skeletal_admit/tooie_npc665_admit exit after its silo_calls count either refuses
// (counting silo_refusals, writing silo_last_reason, returning 0) or admits (counting
// silo_admissions, returning 1, leaving silo_last_reason stale). Silent nonclass returns
// count nothing and stay unattributed. A FFFFFFFE->FFFFFFFF step is still exact.
inline void join_skeletal(uint8_t* rdram, const SkeletalCounters& before, uint32_t returned,
    DrawRecord& out) noexcept {
    if(!(out.valid&DrawRecord::Layout) || !layout(rdram) || before.calls==0xFFFFFFFFu ||
       before.admissions==0xFFFFFFFFu || before.refusals==0xFFFFFFFFu) return;
    const auto after=skeletal_counters(rdram);
    const uint32_t calls=after.calls-before.calls, admissions=after.admissions-before.admissions,
        refusals=after.refusals-before.refusals;
    const bool admitted=calls==1u && admissions==1u && refusals==0u && returned==1u;
    const bool refused=calls==1u && admissions==0u && refusals==1u && returned==0u;
    if(!admitted && !refused) return;
    out.skeletal_calls=calls; out.skeletal_admissions=admissions; out.skeletal_refusals=refusals;
    // The serializer emits null for an admission; only a proven refusal carries the raw reason.
    out.skeletal_reason=refused ? bridge_word(rdram,offsetof(TooieRenderBridgeState,silo_last_reason)) : 0u;
    out.valid|=DrawRecord::SkeletalJoin;
}
#if TOOIE_BK_PIPELINE_SELECTED
// BK root draws at the existing DE498 seams: DF0B0 (baseline_bk_root_begin) and DF234
// (baseline_bk_root_end). The guest helpers own every tag decision; these only read their state
// words on the game thread. A root's fields are its begin snapshot, kept as native POD and paired
// at its own end: no saved guest pointer is dereferenced later and no global is re-read as owner.
inline constexpr uint32_t bk_root_group=1u; // bk_pipeline.c TOOIE_BK_ROOT_GROUP: the helper pushed the ID
inline uint32_t bk_word(uint8_t* rdram,size_t offset) noexcept {
    return word(rdram,TOOIE_BK_STATE+uint32_t(offset));
}
inline uint32_t bk_diag(uint8_t* rdram,size_t field) noexcept {
    return bk_word(rdram,offsetof(TooieBkState,diagnostics)+field);
}
inline bool bk_layout(uint8_t* rdram) noexcept {
    return rdram && range(TOOIE_BK_STATE,TOOIE_BK_STATE_BYTES) &&
        bk_word(rdram,offsetof(TooieBkState,magic))==TOOIE_BK_MAGIC &&
        bk_word(rdram,offsetof(TooieBkState,layout))==TOOIE_BK_LAYOUT;
}
// Exact core callsite witness only: never assigns a rendering owner or mutates guest state.
struct WorldProducerWitness {
    uint8_t* ram{};uint32_t frame{},site{},model{},root_depth{},draw_depth{};bool consumed{};
};
inline std::array<WorldProducerWitness,8> world_producers{};
inline uint32_t world_producer_depth=0;
inline void world_producer_begin(uint8_t* rdram,uint32_t site,uint32_t model) noexcept {
    const uint32_t depth=world_producer_depth++;
    if(depth>=world_producers.size())return; // Nested overflow masks the enclosing witness.
    auto& p=world_producers[depth];p={};
    if(!bk_layout(rdram))return;
    p.ram=rdram;p.frame=bk_diag(rdram,offsetof(TooieBkDiagnostics,frames));p.site=site;p.model=model;
    p.root_depth=bk_word(rdram,offsetof(TooieBkState,root_depth));
    p.draw_depth=bk_word(rdram,offsetof(TooieBkState,draw_depth));
}
inline void world_producer_end() noexcept {
    if(!world_producer_depth)return;
    if(--world_producer_depth<world_producers.size())world_producers[world_producer_depth]={};
}
inline void world_producer_join(uint8_t* rdram,BkDrawRecord& out) noexcept {
    if(!world_producer_depth || world_producer_depth>world_producers.size())return;
    auto& p=world_producers[world_producer_depth-1];
    if(p.consumed || p.ram!=rdram || !p.site || p.model!=out.model ||
       p.frame!=out.bk_frame || out.depth!=p.root_depth+1u || out.draw_depth!=p.draw_depth+1u)return;
    out.producer_site=p.site;out.producer_model=p.model;p.consumed=true;
}
// One pending begin per BK root frame, bounded by TOOIE_BK_ROOT_DEPTH.
struct BkRoots {
    struct Pending { bool active; uint32_t sp, groups, offset_overflows; BkDrawRecord record; };
    std::array<Pending,TOOIE_BK_ROOT_DEPTH> pending{};
    struct Witness { bool valid{}; uint32_t declined{}; std::array<uint32_t,14> words{}; };
    std::array<Witness,TOOIE_BK_ROOT_DEPTH> witness{};
};
// Pending roots at depth index `from` or deeper missed their matching native end/join:
// exported once, unclosed, deepest first.
template<class Emit> inline void bk_flush(BkRoots& roots,uint32_t from,Emit& emit) noexcept {
    for(uint32_t i=TOOIE_BK_ROOT_DEPTH;i>from;--i) {
        auto& pending=roots.pending[i-1u];
        if(!pending.active) continue;
        pending.record.valid|=BkDrawRecord::Unclosed;
        emit(pending.record);
        pending.active=false;
    }
}
// DF0B0 before tooie_bk_root_begin: the root depth this occurrence will occupy.
template<class Emit> inline uint32_t bk_root_before(BkRoots& roots,uint8_t* rdram,Emit&& emit) noexcept {
    if(!bk_layout(rdram)) return 0xFFFFFFFFu;
    const uint32_t depth=bk_word(rdram,offsetof(TooieBkState,root_depth));
    bk_flush(roots,depth<TOOIE_BK_ROOT_DEPTH ? depth : TOOIE_BK_ROOT_DEPTH,emit);
    if(depth<TOOIE_BK_ROOT_DEPTH) {
        auto& w=roots.witness[depth]; w={};
        const auto owner=abi<TooieBkOwner>(rdram,TOOIE_BK_STATE+uint32_t(offsetof(TooieBkState,owner)));
        // bk_float.inc layout1 static assertions: capture96, segments128, records256.
        // Scalar state only; no pose/vertex arrays. No guest call or state mutation.
        if((owner.kind==TOOIE_BK_OWNER_PLAYER ||
            (owner.kind==TOOIE_BK_OWNER_SHADOW && owner.attachments==1u)) &&
            range(TOOIE_BK_FLOAT,16640u)) {
            for(uint32_t i=0;i<8u;++i) w.words[i]=word(rdram,TOOIE_BK_FLOAT+96u+4u*i);
            const uint32_t id=bk_word(rdram,offsetof(TooieBkState,cur_drawn_model_transform_id));
            // The production record of this exact ID, read-only (bk_float.inc bk_record: linear search of
            // all 1024 records from the multiplicative bucket; an ID is held at most once). Zeros if absent.
            const uint32_t start=((id>>8u)*2654435761u)>>22u;
            for(uint32_t n=0;id && n<1024u;++n) {
                const uint32_t slot=(start+n)&1023u;
                if(word(rdram,TOOIE_BK_FLOAT+256u+16u*slot)!=id) continue;
                for(uint32_t i=0;i<4u;++i) w.words[8u+i]=word(rdram,TOOIE_BK_FLOAT+256u+16u*slot+4u*i);
                break;
            }
            w.words[12]=word(rdram,TOOIE_BK_FLOAT+64u); w.words[13]=id;
            w.declined=bk_word(rdram,offsetof(TooieBkState,select_declined));
            w.valid=true;
        }
    }
    return depth;
}
// DF0B0 after tooie_bk_root_begin: that helper's decisions for this occurrence. `sp` is DE498's frame.
inline void bk_root_after(BkRoots& roots,uint8_t* rdram,uint32_t sp,uint32_t depth) noexcept {
    // A root past the depth bound has no frame and keeps original commands only.
    if(depth>=TOOIE_BK_ROOT_DEPTH || !bk_layout(rdram) ||
       bk_word(rdram,offsetof(TooieBkState,root_depth))!=depth+1u) return;
    BkDrawRecord out{};
    out.source=BkDrawRecord::Root;
    out.valid=BkDrawRecord::Layout|BkDrawRecord::Owner;
    out.sp=sp; out.depth=depth+1u;
    out.view=word(rdram,view_address);
    out.phase=uint32_t(int32_t(int16_t(half(rdram,phase_address))));
    out.asset=word(rdram,effective_asset_address);
    if(range(sp,0x110u)) { out.buffers=word(rdram,sp+0xF8u); out.model=word(rdram,sp+0x10Cu); }
    if(layout(rdram)) {
        const auto c=control(rdram);
        if(frame(rdram,c,uint32_t(int32_t(int16_t(half(rdram,pool_index_address)))))) {
            out.epoch=c.epoch; out.valid|=BkDrawRecord::Frame;
        }
    }
    const size_t root=offsetof(TooieBkState,roots)+depth*sizeof(TooieBkRootFrame);
    out.push=bk_word(rdram,root+offsetof(TooieBkRootFrame,pushed_group));
    out.offset=bk_word(rdram,root+offsetof(TooieBkRootFrame,outer_offset));
    out.uses_bones=bk_word(rdram,root+offsetof(TooieBkRootFrame,outer_uses_bones));
    out.skip_all=bk_word(rdram,offsetof(TooieBkState,skip_all_interpolation));
    out.skip_model=bk_word(rdram,offsetof(TooieBkState,cur_drawn_model_skip_interpolation));
    out.current_id=bk_word(rdram,offsetof(TooieBkState,cur_drawn_model_transform_id));
    out.emitted_id=out.push==bk_root_group ? out.current_id : 0u;
    out.draw_depth=bk_word(rdram,offsetof(TooieBkState,draw_depth));
    const auto owner=abi<TooieBkOwner>(rdram,TOOIE_BK_STATE+uint32_t(offsetof(TooieBkState,owner)));
    out.owner_kind=owner.kind; out.owner_birth=owner.birth; out.owner_view=owner.view;
    out.owner_phase=owner.phase; out.owner_base=owner.base_attachment;
    out.owner_attachments=owner.attachments; out.owner_id=owner.id;
    auto& witness=roots.witness[depth];
    if(witness.valid && witness.words[13]==out.current_id &&
       (owner.kind==TOOIE_BK_OWNER_PLAYER ||
        (owner.kind==TOOIE_BK_OWNER_SHADOW && owner.attachments==1u)) && range(TOOIE_BK_FLOAT,16640u)) {
        for(uint32_t i=0;i<14u;++i) out.root_witness[i]=witness.words[i];
        for(uint32_t i=0;i<4u;++i)
            out.root_witness[14u+i]=word(rdram,TOOIE_BK_FLOAT+128u+16u*depth+4u*i);
        out.root_witness[18]=(bk_word(rdram,offsetof(TooieBkState,select_roots))>>depth)&1u;
        out.root_witness[19]=bk_word(rdram,offsetof(TooieBkState,select_declined))-witness.declined;
        out.valid|=BkDrawRecord::RootWitness;
    }
    witness.valid=false;
    // Arithmetic decode of bk_player_id/bk_actor_id; it names no model or semantic part.
    const uint32_t id=out.current_id;
    if(id>=TOOIE_BK_PLAYER_ID_START && id<TOOIE_BK_PLAYER_ID_END) {
        const uint32_t sample=id-TOOIE_BK_PLAYER_ID_START;
        out.id_domain=BkDrawRecord::PlayerId; out.id_attachment=sample/TOOIE_BK_OWNER_STRIDE;
        out.id_sample=sample%TOOIE_BK_OWNER_STRIDE/TOOIE_BK_TRANSFORM_ID_COUNT;
    }
    else if(id>=TOOIE_BK_PLAYER_SLOT_ID_START && id<TOOIE_BK_PLAYER_SLOT_ID_END) {
        // Split players: roster slots 1..7 (bk_pipeline.h); owner.birth carries the drawing slot + 1.
        const uint32_t sample=(id-TOOIE_BK_PLAYER_SLOT_ID_START)%TOOIE_BK_PLAYER_SLOT_STRIDE;
        out.id_domain=BkDrawRecord::PlayerId; out.id_attachment=sample/TOOIE_BK_OWNER_STRIDE;
        out.id_sample=sample%TOOIE_BK_OWNER_STRIDE/TOOIE_BK_TRANSFORM_ID_COUNT;
    }
    else if(id>=TOOIE_BK_ACTOR_ID_START && id<TOOIE_BK_ACTOR_ID_END) {
        const uint32_t sample=(id-TOOIE_BK_ACTOR_ID_START)%TOOIE_BK_ACTOR_OWNER_STRIDE;
        out.id_domain=BkDrawRecord::ActorId; out.id_attachment=sample/TOOIE_BK_OWNER_STRIDE;
        out.id_sample=sample%TOOIE_BK_OWNER_STRIDE/TOOIE_BK_TRANSFORM_ID_COUNT;
    }
    else if(id) out.id_domain=BkDrawRecord::OtherId;
    out.bk_frame=bk_diag(rdram,offsetof(TooieBkDiagnostics,frames));
    out.root_serial=bk_diag(rdram,offsetof(TooieBkDiagnostics,roots));
    out.late_closes=bk_diag(rdram,offsetof(TooieBkDiagnostics,late_closes));
    out.root_overflows=bk_diag(rdram,offsetof(TooieBkDiagnostics,root_overflow));
    out.scope_overflows=bk_diag(rdram,offsetof(TooieBkDiagnostics,scope_overflow));
    // Raw DF47C staging: stale for every draw except the one that staged it.
    out.staged_callback=word(rdram,draw_callback_address);
    out.staged_owner=word(rdram,draw_owner_address);
    out.valid|=BkDrawRecord::Staged;
    // Reconfirm BK's top-level callback staging; this is a candidate association only.
    // Neither the callback guard nor roster membership authenticates the original producer.
    const bool route=owner.kind==TOOIE_BK_OWNER_PLAYER && !owner.attachments && !owner.own_draw_pending &&
        out.draw_depth==1u && depth==0u && id && id==owner.id &&
        ((owner.base_attachment==0u && out.staged_callback==player_callback) ||
         (owner.base_attachment==TOOIE_BK_PLAYER_SECONDARY && out.staged_callback==player_secondary_callback));
    if(route) {
        out.drawn_owner=out.staged_owner;
        out.valid|=BkDrawRecord::Route;
        if(range(out.drawn_owner,player_slot_offset+4u)) {
            out.drawn_slot=word(rdram,out.drawn_owner+player_slot_offset);
            out.drawn_table_match=out.drawn_slot<8u &&
                word(rdram,player_table_address+out.drawn_slot*4u)==out.drawn_owner;
            out.valid|=BkDrawRecord::Roster;
        }
    }
    // The current control slot and its roster entry, independent of the drawn owner.
    out.control_slot=word(rdram,control_slot_address);
    out.valid|=BkDrawRecord::Control;
    if(out.control_slot<8u) {
        out.control_owner=word(rdram,player_table_address+out.control_slot*4u);
        out.valid|=BkDrawRecord::ControlOwner;
    }
    world_producer_join(rdram,out);
    roots.pending[depth]={true,sp,bk_diag(rdram,offsetof(TooieBkDiagnostics,groups)),
        bk_diag(rdram,offsetof(TooieBkDiagnostics,offset_overflow)),out};
}
// DF234 before tooie_bk_root_end: pairs only this depth's pending begin from the same DE498 frame
// and BK frame; anything else at or below it is exported unclosed.
template<class Emit> inline void bk_root_end(BkRoots& roots,uint8_t* rdram,uint32_t sp,Emit&& emit) noexcept {
    if(!bk_layout(rdram)) return;
    const uint32_t depth=bk_word(rdram,offsetof(TooieBkState,root_depth));
    if(!depth || depth>TOOIE_BK_ROOT_DEPTH) return;
    bk_flush(roots,depth,emit);
    auto& pending=roots.pending[depth-1u];
    if(!pending.active) return;
    auto& out=pending.record;
    if(pending.sp==sp && out.bk_frame==bk_diag(rdram,offsetof(TooieBkDiagnostics,frames))) {
        out.end_offset=bk_word(rdram,offsetof(TooieBkState,cur_model_transform_id_offset));
        out.end_uses_bones=bk_word(rdram,offsetof(TooieBkState,cur_model_uses_bones));
        out.groups=bk_diag(rdram,offsetof(TooieBkDiagnostics,groups))-pending.groups;
        out.offset_overflows=bk_diag(rdram,offsetof(TooieBkDiagnostics,offset_overflow))-pending.offset_overflows;
        out.valid|=BkDrawRecord::End;
    }
    else out.valid|=BkDrawRecord::Unclosed;
    emit(out);
    pending.active=false;
}
// DF238 is also reached after ordinary DF234. Only an open bit40 occurrence at
// this exact activation may finish here; never consume an enclosing normal root.
template<class Emit> inline void bk_root_join(BkRoots& roots,uint8_t* rdram,uint32_t sp,Emit&& emit) noexcept {
    if(!bk_layout(rdram)) return;
    const uint32_t depth=bk_word(rdram,offsetof(TooieBkState,root_depth));
    if(!depth || depth>TOOIE_BK_ROOT_DEPTH) return;
    const auto& pending=roots.pending[depth-1u];
    if(!pending.active || pending.sp!=sp || pending.record.push!=2u ||
       pending.record.bk_frame!=bk_diag(rdram,offsetof(TooieBkDiagnostics,frames))) return;
    bk_root_end(roots,rdram,sp,emit);
}
#endif
inline void record_frame(uint8_t* rdram,const recomp_context& ctx,uint32_t phase) noexcept {
    record(read_frame(rdram,ctx,phase));
}
} // namespace baseline::issue_evidence::guest
