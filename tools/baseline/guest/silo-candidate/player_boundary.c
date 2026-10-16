/* Scoped ordinary PlayerState body experiment; slot/address is not a birth. */
#define F3DEX_GBI_2
#include "silo_bridge.h"
#include "rt64_extended_gbi.h"
#define PLAYER __attribute__((noinline, section(".text.player_boundary")))
#define PC SS.player
#define INVALID_BONE_ROLE 0xFFFFFFFFu

static inline __attribute__((always_inline)) u32 player_pose_count(u32 asset) {
    if (asset == 0x62Au) return 68u;
    if (asset == 0x62Bu) return 66u;
    if (asset == 0x665u) return 41u;
    if (asset == 0x838u) return 60u;
    if (asset == 0x839u) return 58u;
    return 0u;
}
typedef struct { s16 pose; u8 pad2[10]; s16 selector; } PlayerBody;
typedef struct { u8 pad0[0x50]; PlayerBody* body; u8 pad54[0x130]; u32 slot; } PlayerOwner;
extern PlayerOwner* D_80135490[8];
extern u8 D_801354B0[8];
extern u32 D_801354DC;
void* func_800AE080(s16);
_Static_assert(sizeof(void*)==4 && sizeof(Gfx)==8 && sizeof(GfxCommand)==8 &&
    __builtin_offsetof(PlayerBody,selector)==0xC &&
    __builtin_offsetof(PlayerOwner,body)==0x50 &&
    __builtin_offsetof(PlayerOwner,slot)==0x184,"original player body ABI");
#define REFUSE(reason) do { PC.last_reason=(reason); silo_count(&PC.refusals); return 0; } while(0)

PLAYER u32 tooie_player_begin(SiloBuffers* b,u32 model) {
    u32 owner,component,slot,packed,view,phase,view_phase,base,current,changed,overrides,effective_model_asset;
    s32 handle,selector;
    if(!PC.enabled) return 0;
    silo_count(&PC.calls);
    if(SS.silo_depth!=1u || SS.active || SC.actor) REFUSE(TOOIE_BRIDGE_REENTRY);
    silo_epoch();
    if(!SS.epoch || SS.epoch==0xFFFFFFFFu) REFUSE(TOOIE_BRIDGE_EPOCH);
    if(SS.poisoned || PC.tripwires) REFUSE(TOOIE_BRIDGE_POISONED);
    /* OVERRIDE low byte stays 4; high bits identify the refusing predicates:
     * 0 callback, 1 post-draw, 2 root mode, 3 texture, 4 DL, 5 vertices, 6 unsupported CPU mode.
     * DF660 stores the enable in C838 and the type byte in C948+0xC9 (CA11). */
    overrides=(D_8012C948[24]!=0x80092234u) |
        ((D_8012C948[26]!=0u)<<1) | ((D_8012C948[28]!=0u)<<2) |
        ((D_8012C948[5]!=0u)<<3) | ((D_8012C948[6]!=0u)<<4) |
        ((D_8012C948[7]!=0u)<<5) |
        ((!(D_8012C838==0u || (D_8012C838==1u &&
            ((volatile u8*)D_8012C948)[0xC9]==1u)))<<6);
    if(overrides) REFUSE(TOOIE_BRIDGE_OVERRIDE | (overrides<<8));
    owner=D_8012C948[25];
    /* Only the retained owner is protected; refuse before dereferencing it. */
    if(PC.owner==owner && PC.stamp==0xFFFFFFFFu) REFUSE(TOOIE_BRIDGE_BIRTH);
    if(!silo_range(owner,0x188u,3u)) REFUSE(TOOIE_BRIDGE_RANGE);
    slot=((PlayerOwner*)owner)->slot;
    if(slot>=8u || D_801354DC!=slot || (u32)D_80135490[slot]!=owner)
        REFUSE(TOOIE_BRIDGE_SLOT);
    if(D_801354B0[slot]) {
        /* F73C4 can cancel pending without init; force freshness, not a tombstone. */
        if(PC.owner==owner && !SS.active) PC.idbase=0;
        silo_count(&PC.pending_refusals); REFUSE(TOOIE_BRIDGE_BIRTH);
    }
    component=(u32)((PlayerOwner*)owner)->body;
    if(!silo_range(component,0x118u,3u)) REFUSE(TOOIE_BRIDGE_RANGE);
    handle=((PlayerBody*)component)->pose; selector=((PlayerBody*)component)->selector;
    if(selector<=0) REFUSE(TOOIE_BRIDGE_SELECTOR | ((u32)(u16)selector<<8));
    /* 607 resolves component+10C in 93F7C after original 94070 LOD selection.
     * Track observed full-width asset transitions even if the model address repeats;
     * lifecycle providers retire observed free/init; unobserved transitions remain a limit. */
    effective_model_asset=selector==0x607 ? *(volatile u32*)(component+0x10Cu) : (u32)selector;
    u32 pose_count = player_pose_count(effective_model_asset);
    if (!pose_count) REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    /* AE080/1B798 only resolves the current handle. 92B04 is an impure cache lookup. */
    if(handle<=0 || handle>=250 || !D_8012C968 || func_800AE080((s16)handle)!=D_8012C968)
        REFUSE(TOOIE_BRIDGE_ANIMATION);
    if(!silo_range((u32)D_8012C968,0x50u+64u*pose_count,3u) ||
       *(volatile u32*)D_8012C968!=pose_count || *((volatile u32*)D_8012C968+1)<pose_count)
        REFUSE(TOOIE_BRIDGE_ANIMATION);
    if(!silo_range(model,0x2Cu,7u) || (*(volatile u16*)(model+0xAu)&0x40u) ||
       *(volatile u32*)(model+0x28u))
        REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    if(!silo_range(D_8012CF8C,25u*4u,3u) ||
       D_8012CF8C!=*(volatile u32*)0x80123198u ||
       *(volatile u32*)(D_8012CF8C+2u*4u)!=0x800DCF48u ||
       *(volatile u32*)(D_8012CF8C+10u*4u)!=0x800DD410u ||
       *(volatile u32*)(D_8012CF8C+12u*4u)!=0x800DD56Cu ||
       *(volatile u32*)(D_8012CF8C+17u*4u)!=0x800DD65Cu ||
       *(volatile u32*)(D_8012CF8C+24u*4u)!=0x800DD998u)
        REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    view=(u32)func_800A89F8(); phase=(u32)func_800EB854();
    if(view>3u) REFUSE(TOOIE_BRIDGE_VIEW);
    if(!(phase==0u || phase==1u || phase==2u || phase==5u || phase==6u))
        REFUSE(TOOIE_BRIDGE_PHASE);
    if((u32)b!=0x80127658u || D_8007BEDA<0 || D_8007BEDA>1 ||
       D_8007BCE8[0] || D_8007BCE8[1] || D_8007BCF0[0] || D_8007BCF0[1])
        REFUSE(TOOIE_BRIDGE_POOL);
    base=D_8007BCD0[D_8007BEDA]; current=(u32)b->gfx;
    if(!silo_range(base,0x8FC0u,7u) || current<base || current>base+0x8FC0u || (current&7u))
        REFUSE(TOOIE_BRIDGE_RANGE);
    if(current<SS.last_end) REFUSE(TOOIE_BRIDGE_CURSOR_DROP);
    /* Original62A's78 bones/59 leaves dominate62B's76/57,665's41/38,
     * Dragon838's70/55 and839's68/53. Prepay common3792;
     * original command space remains its owner's. */
    if(SS.extra_bytes>2608u || base+0x8FC0u-current<3792u) REFUSE(TOOIE_BRIDGE_CAPACITY);
    if(PC.idbase && PC.stamp==SS.epoch) REFUSE(TOOIE_BRIDGE_DUPLICATE);
    packed=((u32)selector<<16)|(u32)handle; view_phase=view|(phase<<2);
    changed=PC.owner!=owner || PC.component!=component || PC.model!=model ||
        PC.selector_handle!=packed || PC.slot!=slot || PC.view_phase!=view_phase ||
        PC.effective_model_asset!=effective_model_asset;
    if(!PC.idbase || changed || PC.stamp+1u!=SS.epoch) {
        if(SS.next_block>=TOOIE_BRIDGE_BLOCK_COUNT) REFUSE(TOOIE_BRIDGE_BLOCKS);
        if(PC.idbase) { silo_count(&PC.reissues); if(changed) silo_count(&PC.changes); }
        PC.idbase=TOOIE_BRIDGE_ID_BASE+SS.next_block*512u;
        SS.next_block+=1u;
    }
    PC.owner=owner; PC.component=component; PC.model=model; PC.selector_handle=packed;
    PC.effective_model_asset=effective_model_asset;
    PC.slot=slot; PC.view_phase=view_phase; PC.stamp=SS.epoch;
    PC.buffers=(u32)b; PC.base=base; PC.pool_index=(u32)D_8007BEDA;
    PC.begin=current; PC.end=0; PC.last_reason=0xFFFFFFFFu;
    PC.emitted_bones=0;
    PC.current_bone_role=INVALID_BONE_ROLE; PC.emitted_leaf_calls=0;
    SS.extra_bytes+=3792u;
    if(SS.extra_bytes>SS.extra_highwater) SS.extra_highwater=SS.extra_bytes;
    SS.active=1;
    GfxCommand* cursor=(GfxCommand*)b->gfx;
    gEXEnable(cursor);
    gEXMatrixGroup(cursor+1,PC.idbase+129u,G_EX_INTERPOLATE_SIMPLE,
        G_EX_PUSH,G_MTX_MODELVIEW,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,
        G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,
        G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_ORDER_LINEAR,G_EX_EDIT_NONE,
        G_EX_ASPECT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP);
    b->gfx=(Gfx*)(cursor+3);
    silo_count(&PC.admissions);
    return 1;
}

static inline __attribute__((always_inline)) void player_tripwire(void) {
    silo_count(&PC.tripwires); silo_count(&SS.tripwire_count); SS.poisoned=1;
}
static inline __attribute__((always_inline)) int player_cursor_owned(SiloBuffers* b,u32 bytes) {
    u32 current=(u32)b->gfx;
    return (u32)b==PC.buffers && PC.pool_index<2u &&
        D_8007BEDA==(s32)PC.pool_index && D_8007BCD0[PC.pool_index]==PC.base &&
        SS.epoch==PC.stamp && current>=PC.begin+24u &&
        current<=PC.base+0x8FC0u-bytes && !(current&7u);
}

PLAYER u32 tooie_player_bone_push(SiloBuffers* b,s32 index) {
    if(!PC.enabled || !SS.active || PC.end || SC.actor || SS.silo_depth!=1u ||
       PC.stamp!=SS.epoch || PC.buffers!=(u32)b || SS.poisoned) return 0;
    /* ADE2C's only negative special case is-1; nonnegative indices address
     * the admitted asset's pose list. Repeats give equal absolute matrices. */
    s32 pose_count = (s32)player_pose_count(PC.effective_model_asset);
    if(!pose_count || index < -1 || index>=pose_count || PC.emitted_bones>=78u ||
       !player_cursor_owned(b,24u)) { player_tripwire(); return 0; }
    /* The existing DCF local owns restoration, including role0 (index -1). */
    u32 prior=PC.current_bone_role;
    u32 token=prior==INVALID_BONE_ROLE ? 1u : prior+2u;
    GfxCommand* cursor=(GfxCommand*)b->gfx;
    gEXMatrixGroup(cursor,PC.idbase+(u32)(index+1),G_EX_INTERPOLATE_SIMPLE,
        G_EX_PUSH,G_MTX_MODELVIEW,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,
        G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,
        G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO,G_EX_ORDER_LINEAR,G_EX_EDIT_NONE,
        G_EX_ASPECT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO);
    b->gfx=(Gfx*)(cursor+2); PC.emitted_bones+=1u;
    PC.current_bone_role=(u32)(index+1);
    return token;
}

PLAYER void tooie_player_bone_pop(SiloBuffers* b,u32 token) {
    /* Restore ancestry even when the returned cursor makes a write unsafe. */
    PC.current_bone_role=token==1u ? INVALID_BONE_ROLE : token-2u;
    /* A successful local token owns this prepaid pop even after deeper poisoning. */
    if(!player_cursor_owned(b,8u)) { player_tripwire(); return; }
    GfxCommand* cursor=(GfxCommand*)b->gfx;
    gEXPopMatrixGroup(cursor,G_MTX_MODELVIEW);
    b->gfx=(Gfx*)(cursor+1);
}

PLAYER u32 tooie_player_leaf_begin(SiloBuffers* b) {
    if(!PC.enabled || !SS.active || PC.end || SC.actor || SS.silo_depth!=1u ||
       PC.stamp!=SS.epoch || PC.buffers!=(u32)b || SS.poisoned) return 0;
    u32 role=PC.current_bone_role;
    u32 pose_count = player_pose_count(PC.effective_model_asset);
    if(role==0u || role>pose_count || PC.emitted_leaf_calls>=59u || !player_cursor_owned(b,32u)) {
        player_tripwire(); return 0;
    }
    GfxCommand* cursor=(GfxCommand*)b->gfx;
    gEXMatrixGroup(cursor,PC.idbase+256u+role,G_EX_INTERPOLATE_SIMPLE,
        G_EX_PUSH,G_MTX_MODELVIEW,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,
        G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,
        G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO,G_EX_ORDER_LINEAR,G_EX_EDIT_NONE,
        G_EX_ASPECT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO);
    /* Public group selection alone does not create a WORLD. DD998 already
     * materialized this slot; LOAD NOPUSH dirties the unchanged physical top. */
    gSPMatrix((Gfx*)(cursor+2),b->mtx,G_MTX_MODELVIEW|G_MTX_LOAD|G_MTX_NOPUSH);
    b->gfx=(Gfx*)(cursor+3); PC.emitted_leaf_calls+=1u;
    return 1;
}

PLAYER void tooie_player_leaf_end(SiloBuffers* b) {
    /* The local success token owns this paid pop even after poisoning. */
    if(!player_cursor_owned(b,8u)) { player_tripwire(); return; }
    GfxCommand* cursor=(GfxCommand*)b->gfx;
    gEXPopMatrixGroup(cursor,G_MTX_MODELVIEW);
    b->gfx=(Gfx*)(cursor+1);
}

PLAYER void tooie_player_end(SiloBuffers* b) {
    PC.current_bone_role=INVALID_BONE_ROLE;
    if(!player_cursor_owned(b,8u)) {
        player_tripwire();
        SS.active=0;
        return;
    }
    /* Every supported normal/early return gets its prepaid pop at the actual
     * post-call cursor. A failed owner premise above invalidates/stops the run;
     * refusing an unsafe write is containment, not a rendering fallback. */
    GfxCommand* cursor=(GfxCommand*)b->gfx;
    gEXPopMatrixGroup(cursor,G_MTX_MODELVIEW);
    b->gfx=(Gfx*)(cursor+1); PC.end=(u32)b->gfx;
    if(PC.end>SS.last_end) SS.last_end=PC.end;
    SS.active=0;
}
