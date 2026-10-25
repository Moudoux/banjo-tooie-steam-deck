/* @recomp PC[8] secondary player bodies: 93864(mode1) from the main 93AB8 and late 93E18
 * routes. One shared route token authenticates the actual occurrence; one shared structural
 * row table admits finite resources by header tuple. No character-specific path. */
#define F3DEX_GBI_2
#include "silo_bridge.h"
#include "rt64_extended_gbi.h"
#include "player_secondary.h"
#if !defined(TOOIE_PLAYER_SECONDARY_ADMIT) || (TOOIE_PLAYER_SECONDARY_ADMIT != 0 && TOOIE_PLAYER_SECONDARY_ADMIT != 1)
#error Explicit boolean TOOIE_PLAYER_SECONDARY_ADMIT selection required
#endif
#if TOOIE_BRIDGE_PHYSICAL_LIST_BYTES == 0x8FC0u
/* Standalone storage has no protected partition: route provenance only, never admission. */
#define SECONDARY_ADMIT 0
#else
#define TOOIE_CAMERA_GUEST 1
#include "../camera-candidate/camera_identity.h"
#define SECONDARY_ADMIT TOOIE_PLAYER_SECONDARY_ADMIT
_Static_assert(TOOIE_BRIDGE_SECONDARY_BYTES==64u*3312u && TOOIE_BRIDGE_PHYSICAL_LIST_BYTES==TOOIE_CAMERA_M_BYTES,
    "secondary admission requires the M1 SECONDARY partition");
#endif
/* The player helper input section both linker scripts already place in .player_helper; the
 * owner object, its definitions and its closed imports stay separately pinned. */
#define SECONDARY __attribute__((noinline, section(".text.player_boundary")))
#define PC SS.player
#define SR SS.secondary_route
#define SL SS.secondary_ledger
#define INVALID_BONE_ROLE 0xFFFFFFFFu

/* Route faults clear the token and are counted. They never restore an earlier token and never
 * trip the global fatal word: an unauthenticated occurrence simply admits nothing. */
static inline __attribute__((always_inline)) void secondary_fault(void) {
    SR.state=TOOIE_SECONDARY_EMPTY; silo_count(&SR.faults);
}

/* 80093D40 / 80093E9C, before the 93864 jal: the route comes from the site (the a2=1 delay slot
 * has not run). owner is the original PlayerState a0 (s0); frame is the parent's sp. */
SECONDARY u32 tooie_secondary_route_begin(u32 route,u32 owner,u32 frame) {
    u32 view;
    if(!PC.enabled) return 0;
    if(SR.state!=TOOIE_SECONDARY_EMPTY) { secondary_fault(); return 0; }
    view=(u32)func_800A89F8();
    if((route!=TOOIE_SECONDARY_MAIN && route!=TOOIE_SECONDARY_LATE) || !SS.epoch ||
       SS.epoch==0xFFFFFFFFu || SS.active || SS.silo_depth || view>3u || !silo_range(owner,0x188u,3u)) {
        secondary_fault(); return 0;
    }
    SR.route=route; SR.owner=owner; SR.epoch=SS.epoch; SR.frame=frame; SR.view=view;
    SR.state=TOOIE_SECONDARY_OPEN; silo_count(&SR.opens);
    return 0;
}

/* 80093D48 after after_16 / 80093EA4 after after_7: owner is s0 (callee-saved through 93864).
 * Every exit clears; a mismatch is also counted. */
SECONDARY u32 tooie_secondary_route_end(u32 route,u32 owner,u32 frame) {
    if(SR.state==TOOIE_SECONDARY_EMPTY) return 0;
    if(SR.route!=route || SR.owner!=owner || SR.frame!=frame || SR.epoch!=SS.epoch) silo_count(&SR.faults);
    SR.state=TOOIE_SECONDARY_EMPTY;
    return 0;
}

/* 80093A9C in 93864, before its common DE448 jal; all model, pose, copy, selector, appearance
 * and 923A8 staging precede it. Only the intended invocation arms: its frame is exactly the
 * parent's minus 0x68, its saved mode is 1, s0 and the staged C9AC are the token owner and
 * C9A8 is 92224. EMPTY is the ordinary mode0 primary call and is left alone. */
SECONDARY u32 tooie_secondary_route_arm(u32 owner,u32 frame,u32 mode) {
    if(SR.state==TOOIE_SECONDARY_EMPTY) return 0;
    if(SR.state!=TOOIE_SECONDARY_OPEN || SR.epoch!=SS.epoch || owner!=SR.owner ||
       frame!=SR.frame-TOOIE_SECONDARY_FRAME_DELTA || mode!=1u ||
       D_8012C948[24]!=0x80092224u || D_8012C948[25]!=SR.owner) { secondary_fault(); return 0; }
    SR.state=TOOIE_SECONDARY_ARMED;
    return 0;
}

/* 80093AA4 after after_31 and L_80093AA4: the DE448 return and the 93A74 null-model branch.
 * OPEN (never armed) and SPENT stay as they are; ARMED becomes SPENT. Nothing re-arms. */
SECONDARY u32 tooie_secondary_route_disarm(u32 owner,u32 frame) {
    if(SR.state==TOOIE_SECONDARY_EMPTY) return 0;
    if(owner!=SR.owner || frame!=SR.frame-TOOIE_SECONDARY_FRAME_DELTA || SR.epoch!=SS.epoch) {
        secondary_fault(); return 0;
    }
    if(SR.state==TOOIE_SECONDARY_ARMED) SR.state=TOOIE_SECONDARY_SPENT;
    return 0;
}

#if SECONDARY_ADMIT
#define SREFUSE(reason) do { SL.last_reason=(reason); silo_count(&SL.refusals); return 0; } while(0)
/* MODEL_DATA sub-reasons (low byte stays MODEL_DATA; bits16..31 carry the resource). */
#define SECONDARY_ROW_MISS 0x100u   /* tuple or body+9C resource is not a structural secondary row */
#define SECONDARY_SEGMENT7 0x200u   /* the row needs the unqualified incoming segment7 contract */
#define SECONDARY_HEADER 0x400u     /* header flags, deform word or pose count differ */
#define SECONDARY_MANAGER 0x800u    /* body+A0 is not the live filled zero-copy manager record */
#define SECONDARY_EXPORT 0x1000u    /* C980 / body+94 palette export interval not authenticated */
#define SECONDARY_TABLE 0x2000u     /* original handler table differs */
#define SECONDARY_APPEARANCE 0x4000u /* linked packed-appearance record is not the finite one */
/* Packed-appearance pool global (1006E0); the manager pool global is 80135A60 (100690). */
#define SECONDARY_APPEARANCE_POOL 0x80135A64u

/* Shared structural rows (execution-facts28): flags0, deform0, no type3, single-list hybrids,
 * each hybrid current-then-parent with one parent per current under every type12 alternative;
 * segment3 offsets 10/50 bind the exact original mode0..3 pairs. Returns
 * poses|bones<<8|leaf calls<<16. gfx is the original graph+root Gfx suffix bound; mtx is the
 * remaining original Mtx suffix 64*(1+bones+leaves) after the already allocated export. */
static inline __attribute__((always_inline)) u32 secondary_row(u32 model,u32* resource,u32* bytes,
                                                                u32* gfx,u32* mtx,u32* segment7) {
    u32 graph=*(volatile u32*)(model+4u),lists=*(volatile u32*)(model+12u);
    u32 vertices=*(volatile u32*)(model+16u),attributes=*(volatile u32*)(model+0x18u);
    if(graph==53864u && lists==4160u && vertices==23256u && attributes==22496u) {
        *resource=0x83Au; *bytes=58120u; *gfx=2064u; *mtx=6592u; *segment7=0u;
        return 47u|59u<<8|43u<<16;                                  /* 83A dd1f075f */
    }
    if(graph==62200u && lists==4920u && vertices==26232u && attributes==25296u) {
        *resource=0x61Cu; *bytes=67320u; *gfx=2528u; *mtx=7744u; *segment7=1u;
        return 58u|70u<<8|50u<<16;                                  /* 61C f817a5b4 */
    }
    return 0;
}
/* A positive index's 12-byte record in a source FreeList (execution-facts30): signed16 element
 * size 12 at +0, signed16 capacity at +2, records from +4; index 0 is the free-chain head. */
static inline __attribute__((always_inline)) u32 secondary_record(u32 pool,u32 index) {
    s32 capacity;
    if(!index || !silo_range(pool,4u,3u) || *(volatile s16*)pool!=12) return 0;
    capacity=*(volatile s16*)(pool+2u);
    if((s32)index>=capacity || !silo_range(pool,4u+12u*(u32)capacity,3u)) return 0;
    return pool+4u+12u*index;
}
/* 9359C gives every nonzero secondary resource a manager record (80100D24 via 100690) whose u8
 * index it stores at body+A0; a nonzero handle is the normal case. Deform0: byte9 filled, both
 * vertex-copy words zero (so 100A74 stages C964=0). Liveness rests on the named source premise
 * that 009C4 clears byte9 before freelist_erase; the generic free chain is not walked. */
static inline __attribute__((always_inline)) u32 secondary_manager(u32 component) {
    u32 record=secondary_record(*(volatile u32*)0x80135A60u,*(volatile u8*)(component+0xA0u));
    if(!record || !*(volatile u8*)(record+9u) || *(volatile u32*)record || *(volatile u32*)(record+4u)) return 0;
    return record;
}
/* What 100AC4 staged into C974 for this manager, for the finite qualified record only:
 *  - manager byte8 (an index into the appearance pool) 0: no packed record, publication 0;
 *  - otherwise the stride-12 appearance record must hold refcount byte8 != 0, the current
 *    resource at signed +4, alternate key +6 == 0 (the unchanged original secondary-constructor
 *    record; palette-rebuilt 100F0C records are outside this finite scope) and a packed span of
 *    10 bytes per canonical vertex (the vertex header's u16 at +0x16 >> 1) in guest memory.
 * Returns the packed data pointer, 0, or 0xFFFFFFFF for an unqualified linked record. */
static inline __attribute__((always_inline)) u32 secondary_packed(u32 manager,u32 resource,u32 model) {
    u32 index=*(volatile u8*)(manager+8u),record,data,count;
    if(!index) return 0u;
    record=secondary_record(*(volatile u32*)SECONDARY_APPEARANCE_POOL,index);
    if(!record || !*(volatile u8*)(record+8u)) return 0xFFFFFFFFu;
    data=*(volatile u32*)record;
    count=(u32)(*(volatile u16*)(model+*(volatile u32*)(model+16u)+0x16u)>>1);
    if((u32)(s32)*(volatile s16*)(record+4u)!=resource || *(volatile s16*)(record+6u) ||
       !data || !count || !silo_range(data,10u*count,1u)) return 0xFFFFFFFFu;
    return data;
}
/* DE498's mode1 appearance row (D_80123030 + 12), the shared exact table the NPC and census routes
 * already authenticate. The worker writes vertex colour/normal/packed scratch only. */
static inline __attribute__((always_inline)) u32 secondary_mode1_row(void) {
    return *(volatile u32*)(0x80123030u+12u)==0x8010E2C4u && *(volatile u32*)(0x80123030u+16u)==0x8010E4C4u &&
        *(volatile u32*)(0x80123030u+20u)==0x8010E7F4u;
}
#endif

/* DE448 at bridge depth 1 with C9A8 == 92224. Only the occurrence the token armed is counted;
 * it is spent here whether or not the body is admitted. */
SECONDARY u32 tooie_player_secondary_begin(SiloBuffers* b,u32 model) {
    u32 route,view;
    if(!PC.enabled) return 0;
    view=(u32)func_800A89F8();
    if(SR.state!=TOOIE_SECONDARY_ARMED || SR.epoch!=SS.epoch || SR.owner!=D_8012C948[25] ||
       SR.view!=view || SS.silo_depth!=1u) { silo_count(&SL.unauthenticated); return 0; }
    SR.state=TOOIE_SECONDARY_SPENT; route=SR.route;
    silo_count(route==TOOIE_SECONDARY_MAIN ? &SL.seen_main : &SL.seen_late);
#if !SECONDARY_ADMIT
    (void)b; (void)model;
    return 0;
#else
    u32 owner,component,slot,phase,view_phase,base,current,resource,bytes,gfx,mtx_suffix,segment7;
    u32 limits,poses,prepay,handle,packed,pose,palette,mtxbase,mtx,overrides,changed,manager,data;
    volatile struct TooieSecondaryEntry* e;
    SL.last_route=route;
    if(SS.active || SC.actor) SREFUSE(TOOIE_BRIDGE_REENTRY);
    silo_epoch();
    if(!SS.epoch || SS.epoch==0xFFFFFFFFu) SREFUSE(TOOIE_BRIDGE_EPOCH);
    if(SS.poisoned || PC.tripwires) SREFUSE(TOOIE_BRIDGE_POISONED);
    /* OVERRIDE bits 0 callback, 1 post-draw, 2 root mode, 3 texture, 4 DL, 5 vertices (C964).
     * Bits 6 (appearance worker) and 7 (packed publication C974) follow the record checks below.
     * C980 is authenticated, not refused: flags0 DE498 branches to DF080 and never consumes it. */
    overrides=(D_8012C948[24]!=0x80092224u) | ((D_8012C948[26]!=0u)<<1) | ((D_8012C948[28]>1u)<<2) |
        ((D_8012C948[5]!=0u)<<3) | ((D_8012C948[6]!=0u)<<4) | ((D_8012C948[7]!=0u)<<5);
    if(overrides) SREFUSE(TOOIE_BRIDGE_OVERRIDE|(overrides<<8));
    owner=D_8012C948[25];
    /* Retained tombstones of either family refuse before the owner is dereferenced. */
    if(player_retained_tombstoned(owner)) SREFUSE(TOOIE_BRIDGE_BIRTH);
    if(!silo_range(owner,0x188u,3u)) SREFUSE(TOOIE_BRIDGE_RANGE);
    slot=*(volatile u32*)(owner+0x184u);
    if(slot>=8u || *(volatile u32*)(0x80135490u+slot*4u)!=owner) SREFUSE(TOOIE_BRIDGE_SLOT);
    if(*(volatile u8*)(0x801354B0u+slot)) { player_slot_fresh(slot,owner); SREFUSE(TOOIE_BRIDGE_BIRTH); }
    component=*(volatile u32*)(owner+0x50u);
    if(!silo_range(component,0x118u,3u)) SREFUSE(TOOIE_BRIDGE_RANGE);
    if(!silo_range(model,0x2Cu,7u) || !(limits=secondary_row(model,&resource,&bytes,&gfx,&mtx_suffix,&segment7)))
        SREFUSE(TOOIE_BRIDGE_MODEL_DATA|SECONDARY_ROW_MISS);
    /* Signed16 body+9C equals the finite positive resource (execution-facts30 R-S5). */
    if((u32)(s32)*(volatile s16*)(component+0x9Cu)!=resource || !silo_range(model,bytes,7u))
        SREFUSE(TOOIE_BRIDGE_MODEL_DATA|SECONDARY_ROW_MISS|(resource<<16));
    poses=limits&255u;
    if(*(volatile u16*)(model+10u) || *(volatile u32*)(model+0x28u) ||
       *(volatile s16*)(model+*(volatile u32*)(model+0x18u)+4u)!=(s32)poses)
        SREFUSE(TOOIE_BRIDGE_MODEL_DATA|SECONDARY_HEADER|(resource<<16));
    if(segment7) SREFUSE(TOOIE_BRIDGE_MODEL_DATA|SECONDARY_SEGMENT7|(resource<<16));
    if(!silo_range(D_8012CF8C,25u*4u,3u) || D_8012CF8C!=*(volatile u32*)0x80123198u ||
       *(volatile u32*)(D_8012CF8C+2u*4u)!=0x800DCF48u || *(volatile u32*)(D_8012CF8C+10u*4u)!=0x800DD410u ||
       *(volatile u32*)(D_8012CF8C+12u*4u)!=0x800DD56Cu || *(volatile u32*)(D_8012CF8C+17u*4u)!=0x800DD65Cu ||
       *(volatile u32*)(D_8012CF8C+24u*4u)!=0x800DD998u)
        SREFUSE(TOOIE_BRIDGE_MODEL_DATA|SECONDARY_TABLE|(resource<<16));
    /* 93864(mode1) staged C968=AE080(signed16 body+90); DCF48 reads C968 for every bone. */
    handle=(u32)(s32)*(volatile s16*)(component+0x90u); pose=(u32)D_8012C968;
    if((s32)handle<=0 || (s32)handle>=250 || !pose || (u32)func_800AE080((s16)handle)!=pose ||
       !silo_range(pose,0x50u+64u*poses,3u) || *(volatile u32*)pose!=poses || *(volatile u32*)(pose+4u)<poses)
        SREFUSE(TOOIE_BRIDGE_ANIMATION);
    if(!(manager=secondary_manager(component))) SREFUSE(TOOIE_BRIDGE_MODEL_DATA|SECONDARY_MANAGER|(resource<<16));
    data=secondary_packed(manager,resource,model);
    if(data==0xFFFFFFFFu) SREFUSE(TOOIE_BRIDGE_MODEL_DATA|SECONDARY_APPEARANCE|(resource<<16));
    /* Bit7: C974 is exactly 100AC4's publication for this manager. Bit6: the current worker is
     * off (C838 0, with or without retained packed data), or exactly the mode1 row (C838 1,
     * CA11 1) over a qualified linked packed record; an enabled worker with no packed data has no
     * qualified consumer and refuses. Stored packed data never implies the worker is enabled. */
    overrides=((D_8012C948[11]!=data)<<7) |
        ((!(D_8012C838==0u || (D_8012C838==1u && data && ((volatile u8*)D_8012C948)[0xC9]==1u &&
            secondary_mode1_row())))<<6);
    if(overrides) SREFUSE(TOOIE_BRIDGE_OVERRIDE|(overrides<<8));
    phase=(u32)func_800EB854();
    if(view>3u) SREFUSE(TOOIE_BRIDGE_VIEW);
    if(!(phase==0u || phase==1u || phase==2u || phase==5u || phase==6u)) SREFUSE(TOOIE_BRIDGE_PHASE);
    if((u32)b!=0x80127658u || D_8007BEDA<0 || D_8007BEDA>1 ||
       D_8007BCE8[0] || D_8007BCE8[1] || D_8007BCF0[0] || D_8007BCF0[1]) SREFUSE(TOOIE_BRIDGE_POOL);
    base=D_8007BCD0[D_8007BEDA]; current=(u32)b->gfx;
    if(!silo_range(base,TOOIE_BRIDGE_PHYSICAL_LIST_BYTES,7u) || current<base ||
       current>base+TOOIE_BRIDGE_LOWER_LIST_BYTES || (current&7u)) SREFUSE(TOOIE_BRIDGE_RANGE);
    if(current<SS.last_end) SREFUSE(TOOIE_BRIDGE_CURSOR_DROP);
    /* 9375C(mode1) published body+94=old+64 and AE598 wrote pose_count+1 matrices from old.
     * The whole produced range [body94-64, body94+64*poses) lies in this pool, aligned to its
     * base, and ends at or below the current Mtx cursor. Arithmetic is checked before use. */
    palette=*(volatile u32*)(component+0x94u);
    mtxbase=D_8007BCD8[D_8007BEDA]; mtx=(u32)b->mtx;
    if(D_8012C948[14]!=palette || !silo_range(mtxbase,0x12C00u,7u) || mtx<mtxbase ||
       mtx>mtxbase+0x12C00u || ((mtx-mtxbase)&63u) || palette<mtxbase+64u || palette>mtx ||
       ((palette-mtxbase)&63u) || mtx-palette<64u*poses)
        SREFUSE(TOOIE_BRIDGE_MODEL_DATA|SECONDARY_EXPORT|(resource<<16));
    e=&SS.secondary[slot][view][route-1u];
    /* One paid occurrence per (slot, view, route) entry and epoch, checked first. */
    if(e->idbase && e->stamp==SS.epoch) SREFUSE(TOOIE_BRIDGE_DUPLICATE);
    /* Room from the current cursors: the export is already allocated prefix and is not counted
     * again; the original suffixes are the row's own bounds, not NPC's 1640. Labels charge the
     * protected SECONDARY partition only. */
    prepay=32u+24u*((limits>>8)&255u)+32u*(limits>>16);
    if(mtxbase+0x12C00u-mtx<mtx_suffix || base+TOOIE_BRIDGE_LOWER_LIST_BYTES-current<gfx+prepay ||
       prepay>TOOIE_BRIDGE_SECONDARY_BYTES || SL.bytes>TOOIE_BRIDGE_SECONDARY_BYTES-prepay)
        SREFUSE(TOOIE_BRIDGE_CAPACITY);
    packed=(resource<<16)|(handle&0xFFFFu); view_phase=view|(phase<<2);
    /* Appearance/packed storage is not identity: it changes no ordered matrix correspondence. */
    changed=e->owner!=owner || e->component!=component || e->model!=model || e->selector_handle!=packed ||
        e->view_phase!=view_phase || e->effective_model_asset!=resource || e->limits!=limits;
    if(!e->idbase || changed || e->stamp+1u!=SS.epoch) {
        if(SS.next_block>=TOOIE_BRIDGE_BLOCK_COUNT) SREFUSE(TOOIE_BRIDGE_BLOCKS);
        if(e->idbase) { silo_count(&SL.reissues); if(changed) silo_count(&SL.changes); }
        e->idbase=TOOIE_BRIDGE_ID_BASE+SS.next_block*512u;
        SS.next_block+=1u;
    }
    e->owner=owner; e->component=component; e->model=model; e->selector_handle=packed;
    e->effective_model_asset=resource; e->view_phase=view_phase; e->stamp=SS.epoch; e->limits=limits;
    /* The single global paid scope snapshots this entry; the shared emitters and close use it.
     * The primary begin serial, counters and reason (the native join source) are untouched. */
    PC.owner=owner; PC.component=component; PC.model=model; PC.selector_handle=packed;
    PC.effective_model_asset=resource; PC.idbase=e->idbase;
    PC.slot=slot; PC.view_phase=view_phase; PC.stamp=SS.epoch;
    PC.buffers=(u32)b; PC.base=base; PC.pool_index=(u32)D_8007BEDA;
    PC.begin=current; PC.end=0;
    PC.emitted_bones=0; PC.current_bone_role=INVALID_BONE_ROLE; PC.emitted_leaf_calls=0;
    SL.bytes+=prepay;
    if(SL.bytes>SL.highwater) SL.highwater=SL.bytes;
    SL.paid=1u+slot*8u+view*2u+(route-1u);
    SS.active=1;
    /* The same 24-byte enable/root envelope as tooie_player_begin (player_boundary.c 257-264). */
    GfxCommand* cursor=(GfxCommand*)b->gfx;
    gEXEnable(cursor);
    gEXMatrixGroup(cursor+1,e->idbase+129u,G_EX_INTERPOLATE_SIMPLE,
        G_EX_PUSH,G_MTX_MODELVIEW,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,
        G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,
        G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_ORDER_LINEAR,G_EX_EDIT_NONE,
        G_EX_ASPECT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP);
    b->gfx=(Gfx*)(cursor+3);
    SL.last_reason=0xFFFFFFFFu; silo_count(&SL.admissions);
    return 1;
#endif
}
