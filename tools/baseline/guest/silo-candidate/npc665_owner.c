/* Source-qualified skeletal owners; original immutable asset parsing premise.
 * No per-draw graph scan, resource getter, native policy or private history. */
#define F3DEX_GBI_2
#include "silo_bridge.h"
#include "npc665_owner.h"
#include "../camera-candidate/camera_actor_admission.h"
#include "rt64_extended_gbi.h"
#if !defined(TOOIE_BK_PIPELINE) || (TOOIE_BK_PIPELINE != 0 && TOOIE_BK_PIPELINE != 1)
#error Explicit boolean TOOIE_BK_PIPELINE selection required
#endif
#if TOOIE_NPC665 && !TOOIE_BK_PIPELINE /* L06c-2: BK links no NPC665 helper, END or sidecar */
#define NPC __attribute__((noinline,section(".tooie_npc665_helpers")))
#define NC tooie_npc665_state
#define AC SS.palette_context
#define INVALID_ROLE 0xFFFFFFFFu
#define MAIN_BYTES TOOIE_CAMERA_M_BYTES
_Static_assert(TOOIE_BRIDGE_PHYSICAL_LIST_BYTES==0x8FC0u || (TOOIE_BRIDGE_PHYSICAL_LIST_BYTES==TOOIE_CAMERA_M_BYTES &&
    TOOIE_BRIDGE_LOWER_LIST_BYTES==TOOIE_BRIDGE_LOWER_BYTES),"silo extents follow the camera MAIN partition");
#define REFUSE(reason) do { SS.silo_last_reason=(reason); silo_count(&SS.silo_refusals); return 0; } while(0)
/* Diagnostic skeletal MODEL_DATA sub-reasons. The low byte stays MODEL_DATA; bits16..23
 * carry the matched row number (1..3 the unchanged witnesses, 4..16 qualified rows below). */
#define SKELETAL_TEXTURE 0x100u        /* C948[5] is not the model's own texture section */
#define SKELETAL_LIST 0x200u           /* C948[6] is not the model's own display lists */
#define SKELETAL_VERTEX 0x400u         /* C948[7] is neither own vertices nor this actor's copy */
#define SKELETAL_POSES 0x800u          /* pose attributes disagree with the witness */
#define SKELETAL_TEXTURE_RANGE 0x1000u /* header texture offset outside the witness */
#define SKELETAL_TABLE 0x2000u         /* original handler table differs */
/* Sub-reasons 0x4000 and 0x8000 are retired: C948[11] and the staging callback are not
 * vertex predicates. Keep them unused so older reason values still decode. */
/* Original D_80123030 appearance rows: {initializer, worker, cleanup}, one per CA11 mode. */
#define APPEARANCE_TABLE 0x80123030u
#define APPEARANCE_ROW_BYTES 12u
volatile struct TooieNpc665Sidecar tooie_npc665_state
    __attribute__((section(".tooie_npc665_state"),aligned(4)))={.control=TOOIE_NPC665_IDLE};
extern s16 D_8011B910,D_8012BE62,D_8012BE64;
extern u32 D_8012B800[],D_8012CA18,D_8012C820,D_8012C850[];
extern void* D_80136E70;
void* func_80104200(s16);

#if !TOOIE_BK_PIPELINE
/* L06b-3: BK installs none of the capture/retire/DEFA0 transports (npc665_hooks gate) and its bone/leaf
 * bodies carry no NPC labels, so only END below is built under BK. */
NPC void tooie_npc665_capture(u32 actor,u32 data,u32 event) {
    if(!(NC.control&TOOIE_NPC665_PENDING) || event!=0x1Fu || actor!=SC.actor ||
       SS.silo_depth!=1u || SC.epoch!=SS.epoch) return;
    if(NC.phase.pending.capture_count || !silo_range(data,0x44u,3u)) {
        NC.control|=TOOIE_NPC665_BAD_CAPTURE;
        return;
    }
    NC.phase.pending.actor_data=data; NC.phase.pending.capture_count=1;
    NC.phase.pending.event=event;
}
NPC void tooie_npc665_retire(u32 actor,u32 descriptor,u32 handle) {
    if(!silo_range(descriptor,0x30u,3u) || descriptor<(u32)D_80132ED0 ||
       descriptor>=(u32)D_80132ED0+200u*0x30u ||
       (descriptor-(u32)D_80132ED0)%0x30u) return;
    u32 slot=(descriptor-(u32)D_80132ED0)/0x30u;
    volatile struct TooieNpc665Slot* e=&SS.npc665[slot];
    if(e->flags==TOOIE_NPC665_ROUTE && e->pose_handle==handle &&
       actor_binding(actor,descriptor,slot,e->birth,e->pair))
        silo_retire(&SS.silo[slot]);
}
#endif
static inline __attribute__((always_inline)) int npc_cursor(SiloBuffers* b,u32 bytes) {
    if(!AC.open_group || AC.buffers!=(u32)b || AC.pool_index>=2u ||
       !silo_range((u32)b,8u,3u)) return 0;
    u32 current=(u32)b->gfx;
    return D_8007BEDA==(s32)AC.pool_index && D_8007BCD0[AC.pool_index]==AC.gfx_base &&
        SS.epoch==AC.epoch && current>=AC.gfx_begin+24u && !(current&7u) &&
        current<=AC.gfx_base+MAIN_BYTES-bytes;
}
static inline __attribute__((always_inline)) void npc_failed(void) {
    NC.control|=TOOIE_NPC665_FAILED;
}
static inline __attribute__((always_inline)) int npc_live(SiloBuffers* b,u32 bytes) {
    if(!(NC.control&TOOIE_NPC665_ACTIVE)) return 0;
    if((NC.control&TOOIE_NPC665_FAILED) || AC.admitted!=TOOIE_NPC665_ADMITTED ||
       AC.slot>=200u || !SS.active || SS.poisoned || SS.silo_depth!=1u ||
       (SS.npc665[AC.slot].flags!=TOOIE_NPC665_ROUTE &&
        SS.npc665[AC.slot].flags!=TOOIE_SKELETAL_ROUTE) ||
       SS.npc665[AC.slot].birth!=AC.birth || SS.npc665[AC.slot].pair!=AC.pair ||
       !(SS.npc665[AC.slot].block_and_valid&TOOIE_BRIDGE_BLOCK_VALID) ||
       !actor_binding(AC.actor,AC.descriptor,AC.slot,AC.birth,AC.pair) || !npc_cursor(b,bytes)) {
        npc_failed(); return 0;
    }
    return 1;
}
/* Read only after npc_live validates the slot and route. NPC665 retains its constants. */
static inline __attribute__((always_inline)) u32 npc_limits(void) {
    volatile struct TooieNpc665Slot* e=&SS.npc665[AC.slot];
    return e->flags==TOOIE_SKELETAL_ROUTE ? e->skeletal_limits : 41u|41u<<8|38u<<16;
}
static inline __attribute__((always_inline)) void npc_group(GfxCommand* p,u32 id) {
    gEXMatrixGroup(p,id,G_EX_INTERPOLATE_SIMPLE,G_EX_PUSH,G_MTX_MODELVIEW,
        G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,
        G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_SKIP,
        G_EX_COMPONENT_AUTO,G_EX_ORDER_LINEAR,G_EX_EDIT_NONE,G_EX_ASPECT_AUTO,
        G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO);
}
/* Source-qualified skeletal rows. Each has no deform section; default 2/3/10/12/17/24
 * handlers; unique non-negative bone indices; one distinct current bone per hybrid node, whose
 * list members (row 14 has two) share its current/parent roles and each count against the bound;
 * state-only segment3 targets for modes0..3 (mode4 refuses below as C948[56]>3);
 * no segment7; F070 writes ST only; type12 selections only add or remove whole bone IDs,
 * never reorder the phases of a reused ID. Limits are executed DCF48 visits and type24
 * list calls over every type12 alternative, not pose counts. Rows 1..3 are the accepted
 * witnesses, unchanged. Rows 4..8 are census rows from the shared offline classifier
 * (execution-facts01; immutable resource SHA-256 prefixes noted). A row qualifies only the
 * resource; identity remains the owner below. No per-draw graph or display-list scan. */
/* Row 16 marks the shared static root-only family in the witness result only. skeletal_admit strips
 * it, so the stored limits are 0: no pose, bone or list-call bound, and any DCF48/type24 call trips. */
#define SKELETAL_ROOT_ONLY 0x01000000u
#define SKELETAL_ROW_ROOT_ONLY 16u
static inline __attribute__((always_inline)) u32 skeletal_witness(u32 model,u32* bytes,u32* row) {
    u32 graph=*(volatile u32*)(model+4u),lists=*(volatile u32*)(model+12u);
    u32 vertices=*(volatile u32*)(model+16u),attributes=*(volatile u32*)(model+0x18u),limits;
    if(graph==41408u && lists==6304u && vertices==21024u && attributes==16392u)
        { *row=1u; *bytes=43280u; limits=23u|23u<<8|18u<<16; }  /* 707 */
    else if(graph==16536u && lists==760u && vertices==6832u && attributes==6600u)
        { *row=2u; *bytes=17656u; limits=14u|14u<<8|13u<<16; }  /* 671 */
    else if(graph==41672u && lists==6352u && vertices==21488u && attributes==16824u)
        { *row=3u; *bytes=43560u; limits=23u|23u<<8|18u<<16; }  /* 90C */
    else if(graph==4616u && lists==192u && vertices==1712u && attributes==1592u)
        { *row=4u; *bytes=5016u; limits=7u|7u<<8|4u<<16; }      /* 616 c62c5ea6 */
    else if(graph==6032u && lists==912u && vertices==3248u && attributes==3160u)
        { *row=5u; *bytes=6272u; limits=5u|5u<<8|2u<<16; }      /* 617 0c7ac3f8 */
    else if(graph==784u && lists==384u && vertices==616u && attributes==592u)
        { *row=6u; *bytes=836u; limits=1u|1u<<8; }              /* 627 18120d9d: type3/10 */
    else if(graph==6040u && lists==2176u && vertices==4128u && attributes==4008u)
        { *row=7u; *bytes=6292u; limits=7u|7u<<8; }             /* 651 5ec11549: type3/12 */
    else if(graph==5152u && lists==2176u && vertices==4296u && attributes==4208u)
        { *row=8u; *bytes=5340u; limits=5u|5u<<8; }             /* 656 6525b84a: type3/12 */
    /* Row 9: shared ordinary row from the WP7 selector-prefix family (execution-facts21). */
    else if(graph==9432u && lists==4264u && vertices==7024u && attributes==6840u)
        { *row=9u; *bytes=9804u; limits=11u|11u<<8; }           /* 92A 302b02e3: type3 */
    /* Rows 10..14: shared ordinary rows of the same family (execution-facts25 W26). */
    else if(graph==13528u && lists==760u && vertices==5064u && attributes==4784u)
        { *row=10u; *bytes=14424u; limits=17u|17u<<8|6u<<16; }  /* 676 9517d1a6 */
    else if(graph==22752u && lists==4864u && vertices==10104u && attributes==9792u)
        { *row=11u; *bytes=23512u; limits=19u|19u<<8; }         /* 88F 6845cab8 */
    else if(graph==24256u && lists==4192u && vertices==9432u && attributes==9296u)
        { *row=12u; *bytes=24544u; limits=8u|8u<<8; }           /* 7F8 2d7236c8 */
    else if(graph==21696u && lists==14128u && vertices==17392u && attributes==17336u)
        { *row=13u; *bytes=21888u; limits=3u|3u<<8; }           /* 801 5d310b12 */
    else if(graph==15064u && lists==5032u && vertices==7936u && attributes==7880u)
        { *row=14u; *bytes=15336u; limits=3u|3u<<8|2u<<16; }    /* 915 8df347dc: type3, one 2-list node */
    /* Row 15: the shared header-0x0100 setup row (execution-facts25/27/29). Its header filter and
     * C820 permission are row-scoped in skeletal_admit; no other row gains either. */
    else if(graph==12280u && lists==2984u && vertices==7264u && attributes==7096u)
        { *row=15u; *bytes=12724u; limits=10u|10u<<8; }          /* 7C2 bd0db0f9: type3, header 0x0100 */
    /* Row 16: the shared static root-only family (execution-facts37 R0-1..4). Flags 0, deform 0,
     * attributes 0 (no pose section exists); two type12 selectors over two type3 DD024 lists, no
     * bone/type17/type24 node; segment3+0x10 only, no segment7. DE498's root LOAD/PUSH and its pop
     * are the only matrix work, and the root group encloses them as for every skeletal row. */
    else if(graph==8040u && lists==4264u && vertices==6160u && attributes==0u)
        { *row=16u; *bytes=8100u; limits=SKELETAL_ROOT_ONLY; }  /* 658 daf18dee: type12/type3 only */
    else return 0;
    if(!silo_range(model,*bytes,7u)) return 0;
    return limits;
}
/* Rows 6..16 retain the default type3 table check (rows 10..13 have no type3; kept conservatively;
 * row 15 has 13 type3 calls, row 16 two). */
#define SKELETAL_ROW_TYPE3_FIRST 6u
/* Row 15 only (header exactly 0x0100). DE498 installed C820=80122F70 at DEC58 and completed the
 * returning setup call at its local cursor before DEFA0: DEC80 word0 DE000000, DEC90 word1
 * 00122F70 (C820+0x80000000). It is therefore the 8 bytes at current-8, inside the proven MAIN
 * range and above the last paid end; current>=base+8 is proven before the read. Its 13 static
 * commands are not active-stream bytes. No other C820 value or command admits. */
#define SKELETAL_ROW_HEADER0100 15u
static inline __attribute__((always_inline)) u32 skeletal_header0100_setup(u32 base,u32 current) {
    return D_8012C820==0x80122F70u && current>=base+8u && current-8u>=SS.last_end &&
        *(volatile u32*)(current-8u)==0xDE000000u && *(volatile u32*)(current-4u)==0x00122F70u;
}
/* Returns 1 when list is one of the copies original 80100D24 made for this actor, read
 * without calling the producer. The s16 handle at actor+0x60 selects a record in the
 * stride-12 FreeList at 80135A60 (original 80100A74 lookup). Record byte9 marks it
 * filled, and the copies sit at record+0 and record+4. Each copy is a complete list
 * with the canonical vertex count. Count-halfword bit0 is a transient dirty flag, not
 * copy identity: 80100D24 sets it and F68C's RGB restore clears it, so it is not read.
 * The byte70/71 trigger, selector and per-frame toggle only choose which copy is drawn.
 * Both copies carry the same bones under vertex-SKIP labels, so either is accepted. */
static inline __attribute__((always_inline)) u32 skeletal_owned_copy(u32 vertices,u32 list) {
    u32 pool=*(volatile u32*)0x80135A60u,record,count,copy,index;
    s32 handle=*(volatile s16*)(AC.actor+0x60u),capacity;
    if(!list) return 0;
    if(!silo_range(pool,4u,3u) || *(volatile s16*)pool!=12) return 0;
    capacity=*(volatile s16*)(pool+2u);
    if(handle<=0 || handle>=capacity || !silo_range(pool,4u+12u*(u32)capacity,3u)) return 0;
    record=pool+4u+12u*(u32)handle;
    if(!*(volatile u8*)(record+9u)) return 0;
    count=*(volatile u16*)(vertices+0x16u)>>1;
    for(index=0;index<2u;++index) {
        copy=*(volatile u32*)(record+4u*index);
        if(copy!=list) continue;
        if(!silo_range(copy,24u+16u*count,3u)) return 0;
        if((*(volatile u16*)(copy+0x16u)>>1)!=count) return 0;
        return 1;
    }
    return 0;
}
/* DF660 stages C838 and the CA11 mode byte before DEFA0. DE498 consumes them at DF150,
 * after admission: it runs the D_80123030 row's initializer and installs its worker.
 * Original source traces modes 1..3 as writing only vertex RGB, normals and packed
 * scratch. They write no rendered XYZ, Gfx, segment or b->mtx. Mode 0, mode 4, any other
 * C838 value or a changed row stays refused. */
static inline __attribute__((always_inline)) u32 skeletal_appearance_qualified(void) {
    u32 mode,row,initializer,worker,cleanup;
    if(D_8012C838==0u) return 1;
    if(D_8012C838!=1u) return 0;
    mode=((volatile u8*)D_8012C948)[0xC9];
    if(mode<1u || mode>3u) return 0;
    row=APPEARANCE_TABLE+APPEARANCE_ROW_BYTES*mode;
    initializer=*(volatile u32*)row;
    worker=*(volatile u32*)(row+4u);
    cleanup=*(volatile u32*)(row+8u);
    if(mode==1u) return initializer==0x8010E2C4u && worker==0x8010E4C4u && cleanup==0x8010E7F4u;
    if(mode==2u) return initializer==0x8010EECCu && worker==0u && cleanup==0u;
    return initializer==0x8010F950u && worker==0u && cleanup==0u;
}
/* Each view/phase owns one 512-ID page: bones 1..P, root 129, hybrids 256+role.
 * Four views times seven phases reserve 28 pages from the existing monotonic allocator. */
_Static_assert(41u<128u && 129u<256u && 256u+41u<512u && 4u*7u==28u,
    "skeletal roles and view/phase pages are disjoint");
static inline __attribute__((always_inline)) u32 skeletal_admit(SiloBuffers* b,u32* local_cursor,u32 model) {
    u32 limits,bytes=0,row=0,poses,paid,current,base,mtxbase,mtx,table,pose,key,camera,seen,view_bit,renew,model_data;
    u32 vertices,overrides;
    s32 offset;
    volatile struct TooieNpc665Slot* e;
    /* Nonclass draws are silent. NPC665 and Silo retain their own routes and counters. */
    if(!AC.actor || AC.admitted || SS.silo_depth!=1u || AC.buffers!=(u32)b ||
       (AC.pair>>16)==0x665u || (AC.pair>>16)==0x917u ||
       !silo_range(model,0x2Cu,7u) || *(volatile u32*)(model+0x28u) ||
       !(limits=skeletal_witness(model,&bytes,&row)) ||
       (row==SKELETAL_ROW_HEADER0100 ? *(volatile u16*)(model+10u)!=0x100u :
        row==SKELETAL_ROW_ROOT_ONLY ? *(volatile u16*)(model+10u)!=0u :
        (*(volatile u16*)(model+10u)&~0x10u)!=0u)) return 0;
    silo_count(&SS.silo_calls); silo_epoch();
    /* The row-16 marker only selects the root-only checks below; it is never a stored bound. */
    limits&=~SKELETAL_ROOT_ONLY;
    if(!SS.epoch || SS.epoch==0xFFFFFFFFu || AC.epoch!=SS.epoch) REFUSE(TOOIE_BRIDGE_EPOCH);
    if(SS.poisoned || SS.active) REFUSE(TOOIE_BRIDGE_POISONED);
    if(!actor_binding(AC.actor,AC.descriptor,AC.slot,AC.birth,AC.pair)) REFUSE(TOOIE_BRIDGE_BIRTH);
    if(AC.view>3u || (u32)func_800A89F8()!=AC.view) REFUSE(TOOIE_BRIDGE_VIEW);
    if(!(AC.phase==0u || AC.phase==1u || AC.phase==2u || AC.phase==5u || AC.phase==6u) ||
       (u32)func_800EB854()!=AC.phase) REFUSE(TOOIE_BRIDGE_PHASE);
    if((u32)b!=0x80127658u || D_8007BEDA<0 || D_8007BEDA>1 ||
       D_8007BCE8[0] || D_8007BCE8[1] || D_8007BCF0[0] || D_8007BCF0[1] ||
       !silo_range((u32)local_cursor,4u,3u)) REFUSE(TOOIE_BRIDGE_POOL);
    base=D_8007BCD0[D_8007BEDA]; current=*local_cursor;
    if(!silo_range(base,MAIN_BYTES,7u) || current<base || current>base+MAIN_BYTES ||
       (current&7u) || current<SS.last_end) REFUSE(TOOIE_BRIDGE_RANGE);
    camera=cam_actor_camera_id(current);
    if(!camera) REFUSE(TOOIE_BRIDGE_VIEW);
    poses=limits&255u; offset=*(volatile s16*)(model+8u);
    /* Every read below lies inside the witness-proven model range, so all failed section
     * predicates are reported together; any one still refuses, as before. */
    model_data=TOOIE_BRIDGE_MODEL_DATA|(row<<16);
    if(offset<0 || (u32)offset>bytes-8u) model_data|=SKELETAL_TEXTURE_RANGE;
    else if(D_8012C948[5]!=model+(u32)offset) model_data|=SKELETAL_TEXTURE;
    if(D_8012C948[6]!=model+*(volatile u32*)(model+12u)) model_data|=SKELETAL_LIST;
    /* @recomp The staged list is never an identity key. Every skeletal label is vertex and
     * texcoord SKIP, and RT64 matches labels by ID only. The list must still be the model's
     * own (staged, or DE498's default) or one of this actor's own copies. This holds under
     * either qualified callback: 801017D0's event handler reaches the same 801015D0
     * producer. C948[11] is the actor's packed appearance record, cleared by DE318 and read
     * only through D_80123030 rows; mode 4 stays refused. It is not a predicate. */
    vertices=model+*(volatile u32*)(model+16u);
    if(D_8012C948[7]!=vertices && !skeletal_owned_copy(vertices,D_8012C948[7]))
        model_data|=SKELETAL_VERTEX;
    /* Row 16 has attributes 0: no pose section exists, so model+0 is never read as one. */
    if(row!=SKELETAL_ROW_ROOT_ONLY && *(volatile s16*)(model+*(volatile u32*)(model+0x18u)+4u)!=(s32)poses)
        model_data|=SKELETAL_POSES;
    if(model_data&0xFF00u) REFUSE(model_data);
    table=D_8012CF8C;
    if(table!=*(volatile u32*)0x80123198u || !silo_range(table,25u*4u,3u) ||
       *(volatile u32*)(table+2u*4u)!=0x800DCF48u || *(volatile u32*)(table+10u*4u)!=0x800DD410u ||
       *(volatile u32*)(table+12u*4u)!=0x800DD56Cu || *(volatile u32*)(table+17u*4u)!=0x800DD65Cu ||
       *(volatile u32*)(table+24u*4u)!=0x800DD998u ||
       (row>=SKELETAL_ROW_TYPE3_FIRST && *(volatile u32*)(table+3u*4u)!=0x800DD024u))
        REFUSE(model_data|SKELETAL_TABLE);
    /* OVERRIDE low byte stays 4; bits8..16 name every failed predicate in this order.
     * Bits0..8: C820, unqualified C838 appearance, CEC4, C948[14/26/28/56/25/24].
     * CEC4 must be zero here: DE498 zeroes it at exit and installs the appearance worker
     * only after DEFA0. The callback stays one of the two traced callbacks.
     * Former bits9..12 (actor +0x70/+0x7B/+0x78/+0x74 inputs) are retired. Only the
     * pre-DEFA0 producers read them. No original code after DEFA0 reads them, and
     * the staged renderer state they can produce is checked here. Row 15 satisfies bit0 only
     * with its exact original setup call; every other row keeps C820==0. */
    overrides=(row==SKELETAL_ROW_HEADER0100 ? !skeletal_header0100_setup(base,current) : D_8012C820!=0u) | ((!skeletal_appearance_qualified())<<1) | ((D_8012CEC4!=0u)<<2) |
        ((D_8012C948[14]!=0u)<<3) | ((D_8012C948[26]!=0u)<<4) | ((D_8012C948[28]!=0u)<<5) |
        ((D_8012C948[56]>3u)<<6) | ((D_8012C948[25]!=AC.actor)<<7) |
        ((D_8012C948[24]!=0x801015D0u && D_8012C948[24]!=0x801017D0u)<<8);
    if(overrides) REFUSE(TOOIE_BRIDGE_OVERRIDE|(overrides<<8));
    pose=(u32)D_8012C968;
    /* Row 16 (attributes 0): DE498 DEDB4..DEDF8 cleared any staged C968 and ran no pose producer
     * before DEFA0, so a nonzero C968 is unqualified staging. No pose block is read; the fixed row
     * marker is the key, and birth, pair and model still renew the entry. */
    if(row==SKELETAL_ROW_ROOT_ONLY) {
        if(pose || !silo_finite((u32)D_8012CE58,16u)) REFUSE(TOOIE_BRIDGE_ANIMATION);
        key=SKELETAL_ROOT_ONLY;
    }
    else if(!silo_range(pose,0x50u+poses*64u,3u) || *(volatile u32*)pose!=poses ||
       *(volatile u32*)(pose+4u)<poses || !silo_finite(pose+0x50u,poses*16u) ||
       !silo_finite((u32)D_8012CE58,16u) || !(key=actor_pose_key(AC.descriptor,pose)))
        REFUSE(TOOIE_BRIDGE_ANIMATION);
    paid=32u+24u*((limits>>8)&255u)+32u*(limits>>16);
    mtxbase=D_8007BCD8[D_8007BEDA]; mtx=(u32)b->mtx;
    /* 1640 is the witnessed NPC665 original-list maximum and bounds all three
     * witnesses (944/584/952), every census row's post-boundary original bytes
     * (240/160/40/184/136), row 9's whole original graph (264), rows 10..14's
     * (520/488/192/88/144) and row 15's 264 plus its 8-byte reset call (its setup call is already
     * below the cursor and is not counted again), and row 16's 16 (two DD024 list calls; its only
     * Mtx is the root slot, 64 below). Paid bytes cover the complete public envelope. */
    if(!silo_range(mtxbase,0x12C00u,7u) || mtx<mtxbase || mtx>mtxbase+0x12C00u ||
       ((mtx-mtxbase)&63u) || mtxbase+0x12C00u-mtx<64u*(1u+((limits>>8)&255u)+(limits>>16)) ||
       base+MAIN_BYTES-current<1640u+paid || SS.extra_bytes>6400u-paid)
        REFUSE(TOOIE_BRIDGE_CAPACITY);
    e=&SS.npc665[AC.slot]; seen=1u<<(AC.view*7u+AC.phase); view_bit=1u<<AC.view;
    /* A culled gap keeps identity: the consumer matches only the immediately prior frame. */
    renew=!(e->block_and_valid&TOOIE_BRIDGE_BLOCK_VALID) || e->flags!=TOOIE_SKELETAL_ROUTE ||
        e->birth!=AC.birth || e->pair!=AC.pair || e->model!=model || e->pose_handle!=key;
    if(!renew) {
        if(e->stamp!=SS.epoch) { e->stamp=SS.epoch; e->seen_mask=0; e->camera_cut_views=0; }
        if(e->camera_id[AC.view] && e->camera_id[AC.view]!=camera) e->camera_cut_views|=view_bit;
        e->camera_id[AC.view]=camera;
        if(e->camera_cut_views&view_bit) REFUSE(TOOIE_BRIDGE_VIEW);
        if(e->seen_mask&seen) REFUSE(TOOIE_BRIDGE_DUPLICATE);
    }
    if(renew) {
        if(SS.next_block>TOOIE_BRIDGE_BLOCK_COUNT-28u) REFUSE(TOOIE_BRIDGE_BLOCKS);
        silo_retire(&SS.silo[AC.slot]);
        volatile u32* words=(volatile u32*)e;
        for(u32 i=0;i<18u;++i) words[i]=0;
        e->birth=AC.birth; e->pair=AC.pair; e->model=model; e->flags=TOOIE_SKELETAL_ROUTE;
        e->pose_handle=key; e->stamp=SS.epoch; e->camera_id[AC.view]=camera;
        e->block_and_valid=SS.next_block|TOOIE_BRIDGE_BLOCK_VALID;
        SS.next_block+=28u; SS.silo_valid_count+=1u;
    }
    e->skeletal_limits=limits; e->seen_mask|=seen;
    AC.idbase=TOOIE_BRIDGE_ID_BASE+(e->block_and_valid&0x7FFFFu)*512u+(AC.view*7u+AC.phase)*512u;
    AC.gfx_base=base; AC.pool_index=(u32)D_8007BEDA; AC.gfx_begin=current; AC.open_group=1;
    AC.admitted=TOOIE_NPC665_ADMITTED; SS.active=1;
    NC.phase.admitted.emitted_bones=0; NC.phase.admitted.current_bone_role=INVALID_ROLE;
    NC.phase.admitted.emitted_hybrid_calls=0; NC.control=TOOIE_NPC665_IDLE|TOOIE_NPC665_ACTIVE;
    SS.extra_bytes+=paid;
    if(SS.extra_bytes>SS.extra_highwater) SS.extra_highwater=SS.extra_bytes;
    GfxCommand* p=(GfxCommand*)current; gEXEnable(p); npc_group(p+1,AC.idbase+129u);
    *local_cursor=(u32)(p+3); silo_count(&SS.silo_admissions);
    return 1;
}
#if !TOOIE_BK_PIPELINE
NPC u32 tooie_npc665_admit(SiloBuffers* b,u32* local_cursor,u32 model) {
    u32 current,base,mtxbase,mtx,data,table,pose,controller,pool,entry,seen,renew,view_bit,camera;
    s32 count,index,offset,pose_handle,controller_handle,stride,capacity;
    volatile struct TooieNpc665Slot* e;
    /* DEFA0 has one native transport. Pending 665 refusals never try the skeletal route. */
    if(!(NC.control&TOOIE_NPC665_PENDING)) return skeletal_admit(b,local_cursor,model);
    if(!(NC.control&TOOIE_NPC665_PENDING) || !AC.actor || AC.admitted ||
       SS.silo_depth!=1u || AC.buffers!=(u32)b) return 0;
    silo_count(&SS.silo_calls); silo_epoch();
    if(!SS.epoch || SS.epoch==0xFFFFFFFFu || AC.epoch!=SS.epoch) REFUSE(TOOIE_BRIDGE_EPOCH);
    if(SS.poisoned || SS.active) REFUSE(TOOIE_BRIDGE_POISONED);
    if(!actor_binding(AC.actor,AC.descriptor,AC.slot,AC.birth,AC.pair) ||
       (AC.pair>>16)!=0x665u) REFUSE(TOOIE_BRIDGE_BIRTH);
    if(AC.view>3u || (u32)func_800A89F8()!=AC.view) REFUSE(TOOIE_BRIDGE_VIEW);
    if(!(AC.phase==0u || AC.phase==1u || AC.phase==2u || AC.phase==5u || AC.phase==6u) ||
       (u32)func_800EB854()!=AC.phase) REFUSE(TOOIE_BRIDGE_PHASE);
    if(NC.control&TOOIE_NPC665_BAD_CAPTURE || NC.phase.pending.capture_count!=1u ||
       NC.phase.pending.event!=0x1Fu) REFUSE(TOOIE_BRIDGE_BIRTH);
    data=NC.phase.pending.actor_data;
    if(!silo_range(data,0x44u,3u) || data<0x80001B14u ||
       *(volatile u32*)(AC.actor+0x10u)!=0x80086980u ||
       *(volatile u16*)(data+4u)!=0x665u || *(volatile u32*)(data+0x38u)!=0x80107C2Cu ||
       *(volatile u32*)(data+0x40u)!=data-0x1B14u+0x4E8u ||
       *(volatile u32*)(data+0x14u)!=data-0x1B14u+0x7FCu) REFUSE(TOOIE_BRIDGE_BIRTH);
    if((u32)b!=0x80127658u || D_8007BEDA<0 || D_8007BEDA>1 ||
       D_8007BCE8[0] || D_8007BCE8[1] || D_8007BCF0[0] || D_8007BCF0[1] ||
       !silo_range((u32)local_cursor,4u,3u)) REFUSE(TOOIE_BRIDGE_POOL);
    base=D_8007BCD0[D_8007BEDA]; current=*local_cursor;
    if(!silo_range(base,MAIN_BYTES,7u) || current<base || current>base+MAIN_BYTES ||
       (current&7u) || current<SS.last_end) REFUSE(TOOIE_BRIDGE_RANGE);
    camera=cam_actor_camera_id(current);
    if(!camera) REFUSE(TOOIE_BRIDGE_VIEW);
    /* Original postcallback D674C publication, not a second cache lookup. */
    count=D_8012BE62; index=D_8011B910;
    if(count<1 || count>135 || index<0 || index>=count ||
       D_8012BE64!=0x665 || D_8012CA18!=0x665u) REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    entry=(u32)D_8012B800+(u32)index*12u;
    if(!silo_range(entry,12u,3u) || (*(volatile u32*)(entry+8u)>>18)!=0x665u ||
       *(volatile u32*)(entry+4u)!=model || !silo_range(model,48516u,7u)) REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    offset=*(volatile s16*)(model+8u);
    if(*(volatile u32*)(model+4u)!=44976u || *(volatile u32*)(model+12u)!=12144u ||
       *(volatile u32*)(model+16u)!=26248u || *(volatile u16*)(model+10u) ||
       *(volatile u32*)(model+0x28u) || offset<0 || (u32)offset>48516u-8u ||
       D_8012C948[5]!=model+(u32)offset || D_8012C948[6]!=model+12144u ||
       D_8012C948[7]!=model+26248u) REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    table=D_8012CF8C;
    if(table!=*(volatile u32*)0x80123198u || !silo_range(table,25u*4u,3u) ||
       *(volatile u32*)(table+2u*4u)!=0x800DCF48u || *(volatile u32*)(table+10u*4u)!=0x800DD410u ||
       *(volatile u32*)(table+12u*4u)!=0x800DD56Cu || *(volatile u32*)(table+17u*4u)!=0x800DD65Cu ||
       *(volatile u32*)(table+24u*4u)!=0x800DD998u) REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    if(D_8012C820 || D_8012C838 || D_8012CEC4 || D_8012C948[14] || D_8012C948[26] ||
       D_8012C948[28] || D_8012C948[24]!=0x801017D0u || D_8012C948[25]!=AC.actor ||
       (*(volatile u32*)(AC.actor+0x70u)&0x01200000u) ||
       (*(volatile u8*)(AC.actor+0x7Bu)&0xC0u) || (*(volatile u8*)(AC.actor+0x78u)&2u) ||
       (*(volatile u8*)(AC.actor+0x74u)&15u)) REFUSE(TOOIE_BRIDGE_OVERRIDE);
    if(D_8012C850[1]<1u || D_8012C850[1]>4u || D_8012C850[4]>1u ||
       D_8012C850[5]>1u) REFUSE(TOOIE_BRIDGE_EXPRESSION);
    pose_handle=*(volatile s16*)(AC.descriptor+0x10u);
    if(pose_handle<=0 || pose_handle>=250) REFUSE(TOOIE_BRIDGE_ANIMATION);
    pose=(u32)func_800AE080((s16)pose_handle);
    if(pose!=(u32)D_8012C968 || !silo_range(pose,0x50u+41u*64u,3u) ||
       *(volatile u32*)pose!=41u || *(volatile u32*)(pose+4u)<41u ||
       !silo_finite(pose+0x50u,41u*16u) ||
       !silo_finite((u32)D_8012CE58,16u)) REFUSE(TOOIE_BRIDGE_ANIMATION);
    /* At DEFA0 the current root/pose are complete; C948[0..3] are
     * published only at DF0C0..DF0D0 and still belong to the prior draw. */
    controller_handle=*(volatile s16*)(AC.actor+0x8Cu); pool=(u32)D_80136E70;
    if(!silo_range(pool,4u,3u)) REFUSE(TOOIE_BRIDGE_ANIMATION);
    stride=*(volatile s16*)pool; capacity=*(volatile s16*)(pool+2u);
    if(stride!=0x3C || capacity<=1 || controller_handle<=0 || controller_handle>=capacity ||
       !silo_range(pool,4u+(u32)stride*(u32)capacity,3u)) REFUSE(TOOIE_BRIDGE_ANIMATION);
    controller=(u32)func_80104200((s16)controller_handle);
    if(controller!=pool+4u+(u32)controller_handle*0x3Cu) REFUSE(TOOIE_BRIDGE_ANIMATION);
    mtxbase=D_8007BCD8[D_8007BEDA]; mtx=(u32)b->mtx;
    if(!silo_range(mtxbase,0x12C00u,7u) || mtx<mtxbase || mtx>mtxbase+0x12C00u ||
       ((mtx-mtxbase)&63u) || mtxbase+0x12C00u-mtx<5120u ||
       base+MAIN_BYTES-current<3872u || SS.extra_bytes>4168u) REFUSE(TOOIE_BRIDGE_CAPACITY);
    e=&SS.npc665[AC.slot]; seen=1u<<(AC.view*7u+AC.phase); view_bit=1u<<AC.view;
    renew=!(e->block_and_valid&TOOIE_BRIDGE_BLOCK_VALID) || e->flags!=TOOIE_NPC665_ROUTE ||
        e->birth!=AC.birth || e->pair!=AC.pair || e->model!=model || e->controller!=controller ||
        e->pose_handle!=(u32)pose_handle || e->controller_handle!=(u32)controller_handle ||
        (e->stamp!=SS.epoch && e->stamp+1u!=SS.epoch);
    if(!renew) {
        if(e->stamp!=SS.epoch) { e->stamp=SS.epoch; e->seen_mask=0; e->camera_cut_views=0; }
        if(e->camera_id[AC.view] && e->camera_id[AC.view]!=camera) e->camera_cut_views|=view_bit;
        e->camera_id[AC.view]=camera;
        if(e->camera_cut_views&view_bit) REFUSE(TOOIE_BRIDGE_VIEW);
        if(e->seen_mask&seen) REFUSE(TOOIE_BRIDGE_DUPLICATE);
    }
    if(renew) {
        if(SS.next_block>TOOIE_BRIDGE_BLOCK_COUNT-28u) REFUSE(TOOIE_BRIDGE_BLOCKS);
        silo_retire(&SS.silo[AC.slot]);
        volatile u32* words=(volatile u32*)e;
        for(u32 i=0;i<18u;++i) words[i]=0;
        e->birth=AC.birth; e->pair=AC.pair; e->model=model; e->controller=controller;
        e->flags=TOOIE_NPC665_ROUTE; e->pose_handle=(u32)pose_handle;
        e->controller_handle=(u32)controller_handle; e->stamp=SS.epoch;
        e->camera_id[AC.view]=camera; e->block_and_valid=SS.next_block|TOOIE_BRIDGE_BLOCK_VALID;
        SS.next_block+=28u; SS.silo_valid_count+=1u;
    }
    e->seen_mask|=seen;
    AC.idbase=TOOIE_BRIDGE_ID_BASE+(e->block_and_valid&0x7FFFFu)*512u+(AC.view*7u+AC.phase)*512u;
    AC.gfx_base=base; AC.pool_index=(u32)D_8007BEDA; AC.gfx_begin=current; AC.open_group=1;
    AC.admitted=TOOIE_NPC665_ADMITTED; SS.active=1;
    NC.phase.admitted.emitted_bones=0; NC.phase.admitted.current_bone_role=INVALID_ROLE;
    NC.phase.admitted.emitted_hybrid_calls=0; NC.control=TOOIE_NPC665_IDLE|TOOIE_NPC665_ACTIVE;
    SS.extra_bytes+=2232u;
    if(SS.extra_bytes>SS.extra_highwater) SS.extra_highwater=SS.extra_bytes;
    GfxCommand* p=(GfxCommand*)current; gEXEnable(p); npc_group(p+1,AC.idbase+129u);
    *local_cursor=(u32)(p+3); silo_count(&SS.silo_admissions);
    return 1;
}
NPC u32 tooie_npc665_bone_push(SiloBuffers* b,s32 index) {
    if(!npc_live(b,24u)) return 0;
    u32 limits=npc_limits();
    if(index<0 || index>=(s32)(limits&255u) ||
       NC.phase.admitted.emitted_bones>=((limits>>8)&255u)) { npc_failed(); return 0; }
    u32 prior=NC.phase.admitted.current_bone_role;
    u32 token=prior==INVALID_ROLE ? 1u : prior+2u;
    GfxCommand* p=(GfxCommand*)b->gfx; npc_group(p,AC.idbase+(u32)index+1u);
    b->gfx=(Gfx*)(p+2); NC.phase.admitted.emitted_bones+=1u;
    NC.phase.admitted.current_bone_role=(u32)index+1u;
    return token;
}
NPC void tooie_npc665_bone_pop(SiloBuffers* b,u32 token) {
    NC.phase.admitted.current_bone_role=token==1u ? INVALID_ROLE : token-2u;
    if(!npc_cursor(b,8u)) { npc_failed(); return; }
    GfxCommand* p=(GfxCommand*)b->gfx; gEXPopMatrixGroup(p,G_MTX_MODELVIEW); b->gfx=(Gfx*)(p+1);
}
NPC u32 tooie_npc665_leaf_begin(SiloBuffers* b) {
    if(!npc_live(b,32u)) return 0;
    u32 role=NC.phase.admitted.current_bone_role,limits=npc_limits();
    if(role<1u || role>(limits&255u) ||
       NC.phase.admitted.emitted_hybrid_calls>=(limits>>16)) { npc_failed(); return 0; }
    GfxCommand* p=(GfxCommand*)b->gfx; npc_group(p,AC.idbase+256u+role);
    gSPMatrix((Gfx*)(p+2),b->mtx,G_MTX_MODELVIEW|G_MTX_LOAD|G_MTX_NOPUSH);
    b->gfx=(Gfx*)(p+3); NC.phase.admitted.emitted_hybrid_calls+=1u;
    return 1;
}
NPC void tooie_npc665_leaf_end(SiloBuffers* b) {
    if(!npc_cursor(b,8u)) { npc_failed(); return; }
    GfxCommand* p=(GfxCommand*)b->gfx; gEXPopMatrixGroup(p,G_MTX_MODELVIEW); b->gfx=(Gfx*)(p+1);
}
#endif
/* END closes the BK-OFF NPC665 occurrence through the 0x8F000034 import (L06b-1b); under BK the whole
 * owner, END and sidecar included, is not built (L06c-2, file gate above). */
NPC u32 tooie_npc665_end(SiloBuffers* b) {
    u32 failed=(NC.control&TOOIE_NPC665_FAILED)!=0 || SS.poisoned ||
        NC.phase.admitted.current_bone_role!=INVALID_ROLE;
    if(!npc_cursor(b,8u)) failed=1;
    else {
        GfxCommand* p=(GfxCommand*)b->gfx; gEXPopMatrixGroup(p,G_MTX_MODELVIEW); b->gfx=(Gfx*)(p+1);
        if((u32)b->gfx>SS.last_end) SS.last_end=(u32)b->gfx;
    }
    AC.open_group=0; AC.admitted=0; SS.active=0; npc665_reset();
    return failed;
}
#endif
