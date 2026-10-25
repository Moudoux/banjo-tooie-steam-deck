/* Scoped ordinary PlayerState body experiment; slot/address is not a birth. */
#define F3DEX_GBI_2
#include "silo_bridge.h"
#include "player_secondary.h"
#include "rt64_extended_gbi.h"
#define PLAYER __attribute__((noinline, section(".text.player_boundary")))
#if TOOIE_BRIDGE_PHYSICAL_LIST_BYTES == 0x8FC0u
/* Standalone storage has no protected partition: players keep the shared 6400 ledger. */
#define PLAYER_PRIMARY_BYTES 0u
#else
#define TOOIE_CAMERA_GUEST 1
#include "../camera-candidate/camera_identity.h"
#define PLAYER_PRIMARY_BYTES TOOIE_BRIDGE_PRIMARY_BYTES
_Static_assert(PLAYER_PRIMARY_BYTES>=32u*3792u && TOOIE_BRIDGE_PHYSICAL_LIST_BYTES==TOOIE_CAMERA_M_BYTES,
    "PC[8] primary partition requires the M1 MAIN extent");
#endif
/* @recomp PC[8]: a retained identity of either family (or the global snapshot) tombstoned for
 * this owner address refuses before any dereference (player_secondary.h bounded scans). */
static inline __attribute__((always_inline)) u32 player_tombstoned(u32 owner) {
    return player_retained_tombstoned(owner);
}
#define PC SS.player
#define INVALID_BONE_ROLE 0xFFFFFFFFu

static inline __attribute__((always_inline)) u32 player_pose_count(u32 asset) {
    if (asset == 0x62Au) return 68u;
    if (asset == 0x62Bu) return 66u;
    if (asset == 0x665u) return 41u;
    if (asset == 0x838u) return 60u;
    if (asset == 0x839u) return 58u;
    /* Census rows: admitted only with their header tuple (player_census_model). */
    if (asset == 0x609u || asset == 0x60Du) return 48u;
    if (asset == 0x60Bu) return 12u;
    if (asset == 0x61Du) return 14u;
    if (asset == 0x626u) return 13u;
    if (asset == 0x60Cu) return 25u;
    if (asset == 0x60Eu) return 29u;
    if (asset == 0x624u) return 15u;
    return 0u;
}
/* Census rows carry full executed-graph bounds: bit16 census, bits8..15 DCF48 visits,
 * bits0..7 type24 list calls (all type12 alternatives summed, not the pose count).
 * The five accepted controls keep the common 78/59 envelope, whose prepay is 3792. */
static inline __attribute__((always_inline)) u32 player_limits(u32 asset) {
    if (asset == 0x609u || asset == 0x60Du) return 1u<<16 | 48u<<8 | 47u;
    if (asset == 0x60Bu) return 1u<<16 | 12u<<8 | 9u;
    if (asset == 0x61Du) return 1u<<16 | 14u<<8 | 9u;
    if (asset == 0x626u) return 1u<<16 | 13u<<8 | 7u;
    if (asset == 0x60Cu) return 1u<<16 | 25u<<8 | 12u;
    if (asset == 0x60Eu) return 1u<<16 | 29u<<8 | 21u;
    if (asset == 0x624u) return 1u<<16 | 15u<<8 | 6u;
    return 78u<<8 | 59u;
}
/* Immutable header tuple of each census row: produced extent, +4 geometry, +0xA flags
 * (0, or exactly 0x10 for the scratch-guarded rows), +0xC lists, +0x10 vertices,
 * +0x18 bone section and its pose count at +4. */
static inline __attribute__((always_inline)) u32 player_census_model(u32 asset,u32 model) {
    u32 bytes,graph,lists,vertices,bones,flags=0u;
    if (asset == 0x609u) { bytes=73936u; graph=70320u; lists=26656u; vertices=48056u; bones=47280u; }
    else if (asset == 0x60Bu) { bytes=25504u; graph=24584u; lists=4120u; vertices=10720u; bones=10520u; }
    else if (asset == 0x60Du) { bytes=106392u; graph=102760u; lists=58416u; vertices=79312u; bones=78536u; }
    else if (asset == 0x61Du) { bytes=21232u; graph=20312u; lists=2032u; vertices=7864u; bones=7632u; }
    else if (asset == 0x626u) { bytes=29976u; graph=28944u; lists=7496u; vertices=15616u; bones=15400u; }
    else if (asset == 0x60Cu) { bytes=51072u; graph=49536u; lists=17304u; vertices=29448u; bones=29040u; flags=0x10u; }
    else if (asset == 0x60Eu) { bytes=21312u; graph=19576u; lists=4864u; vertices=11688u; bones=11216u; flags=0x10u; }
    else if (asset == 0x624u) { bytes=43648u; graph=42440u; lists=11832u; vertices=22952u; bones=22704u; flags=0x10u; }
    else return 0u;
    return silo_range(model,bytes,7u) && *(volatile u32*)(model+4u)==graph &&
        *(volatile u16*)(model+0xAu)==flags && *(volatile u32*)(model+0xCu)==lists &&
        *(volatile u32*)(model+0x10u)==vertices && *(volatile u32*)(model+0x18u)==bones &&
        *(volatile s16*)(model+bones+4u)==(s16)player_pose_count(asset);
}
/* Census rows only. DE498 binds segment 3 from the 80122DC8+8*CA28 pair (member by
 * C988+1C alpha). DF848 replaces the mode-4 pair, so only modes 0..3 with their original
 * bases reach the source-inspected state-only endpoints. C838 appearance must be the
 * CA11=1 row the NPC route authenticates. Returns OVERRIDE bits 7..9. */
static inline __attribute__((always_inline)) u32 player_census_binding(void) {
    u32 mode=D_8012C948[56],first,second,row=0x80123030u+12u;
    if (mode == 0u) { first=0x8011B9B0u; second=0x8011E530u; }
    else if (mode == 1u) { first=0x8011C830u; second=0x8011F3B0u; }
    else if (mode == 2u) { first=0x8011D6B0u; second=0x80120230u; }
    else if (mode == 3u) { first=0x801210B0u; second=0x80121F30u; }
    else return 1u;
    if (*(volatile u32*)(0x80122DC8u+mode*8u)!=first ||
        *(volatile u32*)(0x80122DCCu+mode*8u)!=second) return 2u;
    if (D_8012C838 && (*(volatile u32*)row!=0x8010E2C4u ||
        *(volatile u32*)(row+4u)!=0x8010E4C4u || *(volatile u32*)(row+8u)!=0x8010E7F4u)) return 4u;
    return 0u;
}
/* Header-0x10 census rows only. F070 asks 80100480 for N scalar (descriptor 1, 4-byte) and
 * N vector (descriptor 2, 12-byte) elements; N is the row's largest single request. With both
 * pointers set and both signed capacities >= N, 80100480 neither allocates nor grows, so the
 * draw cannot reach the allocator's maintenance callbacks (EF70/F270 context is not a stack).
 * Cold or retired caches refuse; admission never allocates. Returns OVERRIDE bit 10. */
static inline __attribute__((always_inline)) u32 player_census_scratch(u32 asset) {
    s32 n = asset == 0x60Cu ? 13 : asset == 0x60Eu ? 73 : asset == 0x624u ? 12 : 0;
    if (!n) return 0u;
    if (*(volatile s16*)0x801243ACu!=4 || *(volatile s16*)0x801243AEu<n ||
        !silo_range(*(volatile u32*)0x801243B4u,4u*(u32)n,3u) ||
        *(volatile s16*)0x801243B8u!=12 || *(volatile s16*)0x801243BAu<n ||
        !silo_range(*(volatile u32*)0x801243C0u,12u*(u32)n,3u)) return 8u;
    return 0u;
}
/* @recomp PC[8]: bounds of the open paid scope as poses|bones<<8|leaf calls<<16. While a
 * secondary scope is paid, only its own admitted row bounds it (0 trips). Primaries keep the
 * pose table and the player_limits envelope exactly as before. */
static inline __attribute__((always_inline)) u32 player_scope_bounds(void) {
    if (SS.secondary_ledger.paid) return player_secondary_limits();
    u32 limits = player_limits(PC.effective_model_asset);
    return player_pose_count(PC.effective_model_asset) | (((limits>>8)&255u)<<8) |
        ((limits&255u)<<16);
}
typedef struct { s16 pose; u8 pad2[10]; s16 selector; } PlayerBody;
typedef struct { u8 pad0[0x50]; PlayerBody* body; u8 pad54[0x130]; u32 slot; } PlayerOwner;
extern PlayerOwner* D_80135490[8];
extern u8 D_801354B0[8];
extern u32 D_801354DC;
_Static_assert(sizeof(void*)==4 && sizeof(Gfx)==8 && sizeof(GfxCommand)==8 &&
    __builtin_offsetof(PlayerBody,selector)==0xC &&
    __builtin_offsetof(PlayerOwner,body)==0x50 &&
    __builtin_offsetof(PlayerOwner,slot)==0x184,"original player body ABI");
#define REFUSE(reason) do { PC.last_reason=(reason); silo_count(&PC.refusals); return 0; } while(0)
/* Diagnostic MODEL_DATA sub-reasons. The low byte stays MODEL_DATA; every model-data
 * refusal also carries bit9 for an asset wider than 16 bits and the asset's low half in
 * bits16..31. These bits name the failed predicates; admission is unchanged. */
#define MODEL_ASSET_MISS 0x100u      /* no finite pose-table entry */
#define MODEL_HEADER_RANGE 0x400u    /* model header outside guest memory */
#define MODEL_HEADER_PALETTE 0x800u  /* header +0x0A bit40 */
#define MODEL_HEADER_DEFORM 0x1000u  /* header +0x28 CPU deform word */
#define MODEL_HANDLER_TABLE 0x2000u  /* original handler table differs */
#define MODEL_CENSUS_TUPLE 0x4000u   /* census row header tuple or its pose word */

PLAYER u32 tooie_player_begin(SiloBuffers* b,u32 model) {
    u32 owner,component,slot,packed,view,phase,view_phase,base,current,changed,overrides,effective_model_asset;
    s32 handle,selector;
    volatile struct TooiePlayerEntry* e;
    if(!PC.enabled) return 0;
    silo_count(&PC.calls);
    if(SS.silo_depth!=1u || SS.active || SC.actor) REFUSE(TOOIE_BRIDGE_REENTRY);
    silo_epoch();
    if(!SS.epoch || SS.epoch==0xFFFFFFFFu) REFUSE(TOOIE_BRIDGE_EPOCH);
    if(SS.poisoned || PC.tripwires) REFUSE(TOOIE_BRIDGE_POISONED);
    /* OVERRIDE low byte stays 4; high bits identify the refusing predicates:
     * 0 callback, 1 post-draw, 2 root mode, 3 texture, 4 DL, 5 vertices, 6 unsupported CPU mode;
     * census rows also 7 segment-3 mode, 8 segment-3 base pair, 9 appearance row, 10 F070 scratch.
     * DF660 stores the enable in C838 and the type byte in C948+0xC9 (CA11).
     * Root mode C948[28] (C9B8): DE498 folds mode1's PYR vector into CE58 before
     * DEF88 capture; the C9BC pointer it publishes at state+0x10 is read by no
     * handler or CPU type1 path admitted here. Modes2/3 remain refused. */
    overrides=(D_8012C948[24]!=0x80092234u) |
        ((D_8012C948[26]!=0u)<<1) | ((D_8012C948[28]>1u)<<2) |
        ((D_8012C948[5]!=0u)<<3) | ((D_8012C948[6]!=0u)<<4) |
        ((D_8012C948[7]!=0u)<<5) |
        ((!(D_8012C838==0u || (D_8012C838==1u &&
            ((volatile u8*)D_8012C948)[0xC9]==1u)))<<6);
    if(overrides) REFUSE(TOOIE_BRIDGE_OVERRIDE | (overrides<<8));
    owner=D_8012C948[25];
    /* Retained (global scope or any entry) tombstones refuse before dereferencing the owner. */
    if((PC.owner==owner && PC.stamp==0xFFFFFFFFu) || player_tombstoned(owner)) REFUSE(TOOIE_BRIDGE_BIRTH);
    if(!silo_range(owner,0x188u,3u)) REFUSE(TOOIE_BRIDGE_RANGE);
    slot=((PlayerOwner*)owner)->slot;
    /* @recomp PC[8]: every authenticated roster owner, not only the controlled slot. */
    if(slot>=8u || (u32)D_80135490[slot]!=owner)
        REFUSE(TOOIE_BRIDGE_SLOT);
    if(D_801354B0[slot]) {
        /* F73C4 can cancel pending without init: every view and route of the slot, both families. */
        player_slot_fresh(slot,owner);
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
    u32 model_data = TOOIE_BRIDGE_MODEL_DATA | ((effective_model_asset > 0xFFFFu) << 9) |
        (effective_model_asset << 16);
    if (!pose_count) REFUSE(model_data | MODEL_ASSET_MISS);
    /* AE080/1B798 only resolves the current handle. 92B04 is an impure cache lookup. */
    if(handle<=0 || handle>=250 || !D_8012C968 || func_800AE080((s16)handle)!=D_8012C968)
        REFUSE(TOOIE_BRIDGE_ANIMATION);
    if(!silo_range((u32)D_8012C968,0x50u+64u*pose_count,3u) ||
       *(volatile u32*)D_8012C968!=pose_count || *((volatile u32*)D_8012C968+1)<pose_count)
        REFUSE(TOOIE_BRIDGE_ANIMATION);
    if(!silo_range(model,0x2Cu,7u)) REFUSE(model_data | MODEL_HEADER_RANGE);
    /* Both header reads follow the range proof above. */
    u32 header = ((*(volatile u16*)(model+0xAu)&0x40u) ? MODEL_HEADER_PALETTE : 0u) |
        (*(volatile u32*)(model+0x28u) ? MODEL_HEADER_DEFORM : 0u);
    if(header) REFUSE(model_data | header);
    u32 limits = player_limits(effective_model_asset);
    if (limits >> 16) {
        if (!player_census_model(effective_model_asset, model)) REFUSE(model_data | MODEL_CENSUS_TUPLE);
        u32 binding = player_census_binding() | player_census_scratch(effective_model_asset);
        if (binding) REFUSE(TOOIE_BRIDGE_OVERRIDE | (binding << 15));
    }
    u32 prepay = 32u + 24u*((limits>>8)&255u) + 32u*(limits&255u);
    if(!silo_range(D_8012CF8C,25u*4u,3u) ||
       D_8012CF8C!=*(volatile u32*)0x80123198u ||
       *(volatile u32*)(D_8012CF8C+2u*4u)!=0x800DCF48u ||
       *(volatile u32*)(D_8012CF8C+10u*4u)!=0x800DD410u ||
       *(volatile u32*)(D_8012CF8C+12u*4u)!=0x800DD56Cu ||
       *(volatile u32*)(D_8012CF8C+17u*4u)!=0x800DD65Cu ||
       *(volatile u32*)(D_8012CF8C+24u*4u)!=0x800DD998u)
        REFUSE(model_data | MODEL_HANDLER_TABLE);
    view=(u32)func_800A89F8(); phase=(u32)func_800EB854();
    if(view>3u) REFUSE(TOOIE_BRIDGE_VIEW);
    e=&SS.primary[slot][view];
    if(!(phase==0u || phase==1u || phase==2u || phase==5u || phase==6u))
        REFUSE(TOOIE_BRIDGE_PHASE);
    if((u32)b!=0x80127658u || D_8007BEDA<0 || D_8007BEDA>1 ||
       D_8007BCE8[0] || D_8007BCE8[1] || D_8007BCF0[0] || D_8007BCF0[1])
        REFUSE(TOOIE_BRIDGE_POOL);
    base=D_8007BCD0[D_8007BEDA]; current=(u32)b->gfx;
    if(!silo_range(base,TOOIE_BRIDGE_PHYSICAL_LIST_BYTES,7u) || current<base || current>base+TOOIE_BRIDGE_LOWER_LIST_BYTES || (current&7u))
        REFUSE(TOOIE_BRIDGE_RANGE);
    if(current<SS.last_end) REFUSE(TOOIE_BRIDGE_CURSOR_DROP);
    /* Prepay 32+24*bones+32*leaf calls. Controls share62A's 78/59 envelope (3792), which
     * dominates62B's76/57,665's41/38, Dragon838's70/55 and839's68/53; census rows prepay
     * their own executed bounds. Original command space remains its owner's.
     * @recomp PC[8]: one paid occurrence per (slot, view) entry and epoch, checked first so the
     * reason is exact. Primaries charge only the protected PRIMARY partition: 32 entries with a
     * prepay <= 3792 cannot exhaust it. Standalone storage keeps the shared 6400 ledger. */
    if(e->idbase && e->stamp==SS.epoch) REFUSE(TOOIE_BRIDGE_DUPLICATE);
#if PLAYER_PRIMARY_BYTES
    if(prepay>PLAYER_PRIMARY_BYTES || SS.primary_bytes>PLAYER_PRIMARY_BYTES-prepay ||
       base+TOOIE_BRIDGE_LOWER_LIST_BYTES-current<prepay) REFUSE(TOOIE_BRIDGE_CAPACITY);
#else
    if(SS.extra_bytes>6400u-prepay || base+TOOIE_BRIDGE_LOWER_LIST_BYTES-current<prepay) REFUSE(TOOIE_BRIDGE_CAPACITY);
#endif
    packed=((u32)selector<<16)|(u32)handle; view_phase=view|(phase<<2);
    changed=e->owner!=owner || e->component!=component || e->model!=model ||
        e->selector_handle!=packed || e->view_phase!=view_phase ||
        e->effective_model_asset!=effective_model_asset;
    if(!e->idbase || changed || e->stamp+1u!=SS.epoch) {
        if(SS.next_block>=TOOIE_BRIDGE_BLOCK_COUNT) REFUSE(TOOIE_BRIDGE_BLOCKS);
        if(e->idbase) { silo_count(&PC.reissues); if(changed) silo_count(&PC.changes); }
        e->idbase=TOOIE_BRIDGE_ID_BASE+SS.next_block*512u;
        SS.next_block+=1u;
    }
    e->owner=owner; e->component=component; e->model=model; e->selector_handle=packed;
    e->effective_model_asset=effective_model_asset; e->view_phase=view_phase; e->stamp=SS.epoch;
    /* The single global scope snapshots the admitted entry; paid closes never re-select. */
    PC.owner=owner; PC.component=component; PC.model=model; PC.selector_handle=packed;
    PC.effective_model_asset=effective_model_asset; PC.idbase=e->idbase;
    PC.slot=slot; PC.view_phase=view_phase; PC.stamp=SS.epoch;
    PC.buffers=(u32)b; PC.base=base; PC.pool_index=(u32)D_8007BEDA;
    PC.begin=current; PC.end=0; PC.last_reason=0xFFFFFFFFu;
    PC.emitted_bones=0;
    PC.current_bone_role=INVALID_BONE_ROLE; PC.emitted_leaf_calls=0;
#if PLAYER_PRIMARY_BYTES
    SS.primary_bytes+=prepay;
    if(SS.primary_bytes>SS.primary_highwater) SS.primary_highwater=SS.primary_bytes;
#else
    SS.extra_bytes+=prepay;
    if(SS.extra_bytes>SS.extra_highwater) SS.extra_highwater=SS.extra_bytes;
#endif
    SS.primary_paid=1u+slot*4u+view;
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
        current<=PC.base+TOOIE_BRIDGE_PHYSICAL_LIST_BYTES-bytes && !(current&7u);
}

PLAYER u32 tooie_player_bone_push(SiloBuffers* b,s32 index) {
    if(!PC.enabled || !SS.active || PC.end || SC.actor || SS.silo_depth!=1u ||
       PC.stamp!=SS.epoch || PC.buffers!=(u32)b || SS.poisoned) return 0;
    /* ADE2C's only negative special case is-1; nonnegative indices address
     * the admitted asset's pose list. Repeats give equal absolute matrices. */
    u32 bounds = player_scope_bounds();
    s32 pose_count = (s32)(bounds&255u);
    if(!pose_count || index < -1 || index>=pose_count || PC.emitted_bones>=((bounds>>8)&255u) ||
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
    u32 bounds = player_scope_bounds();
    u32 pose_count = bounds&255u;
    if(role==0u || role>pose_count || PC.emitted_leaf_calls>=(bounds>>16) ||
       !player_cursor_owned(b,32u)) {
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
        SS.primary_paid=0;
        SS.secondary_ledger.paid=0;
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
    SS.primary_paid=0;
    SS.secondary_ledger.paid=0;
}
