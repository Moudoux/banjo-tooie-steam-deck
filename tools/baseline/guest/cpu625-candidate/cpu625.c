/* CPU625 public AUTO owner; selected by the player preset. */
#include "../silo-candidate/silo_bridge.h"
#include "../camera-candidate/camera_projection_internal_04.h"
#include "cpu625_layout.h"
#if !defined(TOOIE_CPU625) || (TOOIE_CPU625 != 0 && TOOIE_CPU625 != 1)
#error Explicit boolean TOOIE_CPU625 selection required
#endif
#if !defined(TOOIE_CKCOPY) || (TOOIE_CKCOPY != 0 && TOOIE_CKCOPY != 1)
#error Explicit boolean TOOIE_CKCOPY selection required
#endif
#if TOOIE_CKCOPY && !TOOIE_CPU625
#error P04a rows join the selected CPU625 owner
#endif
#if TOOIE_CPU625
#define CPU625 __attribute__((section(".tooie_cpu625_helpers"),noinline))
#define CS tooie_cpu625_state
#define CX CS.context
/* Actor CPU-root tuple (actor_palette.c tooie_actor_cpu_root_begin): differs from
 * TOOIE_BRIDGE_PALETTE_LEAF_POLICY only by vertex AUTO. */
#define C625_ROOT_POLICY CAM_GROUP_FLAGS(G_EX_INTERPOLATE_SIMPLE,G_EX_PUSH,G_MTX_MODELVIEW, \
    G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE, \
    G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_AUTO,G_EX_COMPONENT_AUTO, \
    G_EX_ORDER_LINEAR,G_EX_EDIT_NONE,G_EX_ASPECT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO)
_Static_assert(C625_ROOT_POLICY==0x02014AA9u,"public CPU625 root gEXMatrixGroup word2");
extern u32 D_80135490[8],D_801354DC;
extern u8 D_801354B0[8];
extern volatile u32 D_8012D500,D_8012D558;
extern s16 D_80132E9C;
s32 func_80100A74(s16,u8);
s32 func_80100AC4(s16);
struct TooieCpu625State tooie_cpu625_state
    __attribute__((section(".tooie_cpu625_state"),aligned(4)))=
    {.magic=TOOIE_CPU625_MAGIC,.layout=TOOIE_CPU625_LAYOUT};

/* Both original lookup owners allocate12-byte inline FreeList elements.
 * Validate header before reading it and the full computed pool before getters. */
SI u32 c625_pool_entry(u32 pool,u32 index) {
    s32 capacity;
    if(!silo_range(pool,4u,3u) || *(volatile s16*)pool!=12)return 0;
    capacity=*(volatile s16*)(pool+2u);
    if(capacity<=1 || !index || index>=(u32)capacity ||
        !silo_range(pool,4u+12u*(u32)capacity,3u))return 0;
    return pool+4u+12u*index;
}
SI u32 c625_lookup_safe(u32 handle) {
    u32 e=c625_pool_entry(*(volatile u32*)0x80135A60u,handle),second;
    if(!e)return 0;
    second=*(volatile u8*)(e+8u);
    if(second && !c625_pool_entry(*(volatile u32*)0x80135A64u,second))return 0;
    return 1;
}
SI u32 c625_phase(u32 p) { return p==0u || p==1u || p==2u || p==5u || p==6u; }
SI u32 c625_slot(u32 self) {
    u32 slot;
    if(!silo_range(self,0x188u,3u))return 8u;
    slot=*(volatile u32*)(self+0x184u);
    return slot<8u && D_801354DC==slot && D_80135490[slot]==self &&
        !D_801354B0[slot] ? slot : 8u;
}
SI void c625_clear(void) {
    volatile u32* p=(volatile u32*)&CX;u32 i;for(i=0;i<12u;i++)p[i]=0;
}
SI u32 c625_trip(u32 reason) {
    silo_count(&CS.tripwires);CS.last_reason=reason;c625_clear();return 1u;
}
/* Destruction, replacement and refused premises end the owner's identity. */
SI void c625_forget(u32 owner) {
    u32 i;
    for (i=0; i<8u; i++) {
        if (CS.owner[i].owner==owner) CS.owner[i].idbase=0;
    }
}
SI u32 c625_refuse(u32 reason) {
    CS.last_reason=reason;
    silo_count(&CS.refusals);
    c625_forget(CX.owner);
    return 0;
}
/* @recomp A repeat stays unlabelled. Keep its first admission's identity and
 * seen mask so later repeats cannot renew and enter the same frame again. */
SI u32 c625_refuse_repeat(void) {
    CS.last_reason=12u;
    silo_count(&CS.refusals);
    return 0;
}
/* Same accepted camera law, with the lexical625 view instead of actor context. */
SI u32 c625_camera(u32 cursor) {
    struct TooieCameraControl* c=&CAM_STATE.control;
    struct TooieCameraProduction* p=&CAM_STATE.saved;
    struct TooieCameraRecord* r;
#if TOOIE_PRE_BACKGROUND
    if(tooie_pre_background.pending.depth)return 0;
#endif
    if(!CAM_STATE.owner_frame || !c->active || D_8012D508 || c->latch ||
        (cursor&7u) || cursor<c->main_begin || cursor>c->main_end-144u ||
        c->main_extra>TOOIE_CAMERA_EXTRA_BYTES-32u-48u)return 0;
    r=cam_find(p->camera);
    if(c->epoch!=SS.epoch || !p->valid || p->epoch!=c->epoch ||
        p->matrix!=D_8012D558 || p->camera!=D_8012D500 || p->view>=4u ||
        p->view!=CX.view || p->view!=(u32)D_80127674 ||
        p->id<TOOIE_CAMERA_ID_FIRST || p->id>=TOOIE_CAMERA_ID_END ||
        p->policy!=CAM_POLICY_WORLD || p->role!=TOOIE_CAMERA_WORLD_INITIAL ||
        !r || r->birth!=p->birth || r->revision!=p->revision ||
        r->view_id[p->view]!=p->id || r->view_scale[p->view]!=p->scale)return 0;
    return p->id;
}
/* Fixed13-node source class, not a new list decoder. Original parse validity
 * is inherited as for065E; ordered per-WORLD roles are a fixture obligation. */
SI u32 c625_graph(u32 model) {
    u32 node=model+9776u,i,type,step,n10=0,n17=0,size,table=D_8012CF8C;
    if(!silo_range(model,9992u,7u) || *(volatile u32*)(model+4u)!=9776u ||
        *(volatile u16*)(model+10u)!=0u || *(volatile u32*)(model+16u)!=5432u ||
        *(volatile u32*)(model+24u)!=5152u || *(volatile u32*)(model+40u)!=3056u ||
        *(volatile s16*)(model+5156u)!=17 ||
        !silo_range(table,18u*4u,3u) || table!=*(volatile u32*)0x80123198u ||
        *(volatile u32*)(table+40u)!=0x800DD410u ||
        *(volatile u32*)(table+68u)!=0x800DD65Cu)return 0;
    for(i=0;i<13u;i++) {
        if(node<model+9776u || node>model+9992u-16u)return 0;
        type=*(volatile u32*)node;step=*(volatile u32*)(node+4u);
        if(type==10u) {size=24u;n10++;}
        else if(type==17u) {
            size=16u;n17++;
            if(*(volatile s16*)(node+8u)<0 || (*(volatile u16*)(node+14u)&1u))return 0;
        } else return 0;
        if(node>model+9992u-size || (i<12u ? step!=size : step!=0u))return 0;
        node+=size;
    }
    return n10==1u && n17==12u && node==model+9992u;
}
#if TOOIE_CKCOPY
/* P04a: the CPU-deform attachment rows of the same shared ckpack draw3 producer, selected only
 * with the P04b copy storage. A new row is labelled only for storage this root publishes. */
#include "ckcopy_layout.h"
#include "ckpack_rows.h"
#include "../camera-candidate/shadow_state.h"
#define CK tooie_ckcopy_state
/* Public gEXMatrixGroupNoInterpolate(PUSH,MODELVIEW,EDIT_NONE) word2. RT64 files G_EX_ID_IGNORE
 * transforms apart and never matches them (buildTransformIdMap, the AUTO/hash guards), so a
 * placeholder neither takes nor leaves interpolation history for any real ID. */
#define C625_PLACEHOLDER_POLICY CAM_GROUP_FLAGS(G_EX_INTERPOLATE_SIMPLE,G_EX_PUSH,G_MTX_MODELVIEW, \
    G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP, \
    G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP, \
    G_EX_ORDER_LINEAR,G_EX_EDIT_NONE,G_EX_ASPECT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_SKIP)
_Static_assert(C625_PLACEHOLDER_POLICY==TOOIE_CKCOPY_PLACEHOLDER_POLICY && G_EX_ID_IGNORE==0u &&
    C625_ROOT_POLICY==TOOIE_CKCOPY_LABEL_POLICY && TOOIE_CKCOPY_LABEL_ID_FIRST==TOOIE_CPU625_ID_FIRST &&
    TOOIE_CKCOPY_LABEL_ID_END==TOOIE_CPU625_ID_FIRST+512u*TOOIE_CPU625_BLOCKS,"P04a label contract");
/* N3 shadow-attachment role inside each CPU625 (view, phase) cell: main uses +1, so +2 is free
 * in every cell; the largest offset 3*128+6*16+2 = 482 stays inside the 512-ID block. */
#define C625_ATTACH_ROLE 2u
#define C625_SHADOW_ROLE 480u /* shadow_projection.c SHADOW_ROLE_FIRST: the silhouette projection ID */
_Static_assert(3u*128u+6u*16u+C625_ATTACH_ROLE<512u,"N3 role stays in its block");
/* Lifetime-contract row facts as if-chains (no .rodata): model bytes, geometry (+4), vertex
 * section (+0x10), bone section (+0x18, poses at bone+4), copy vertices (bytes 24+16n) and
 * physical main-list bytes (24 prefix + remaining original 112/136 + 8 paid close). */
SI u32 c625_model_bytes(u32 row) { return row==0u ? 9992u : row==1u ? 9432u : row==2u ? 9408u : 9384u; }
SI u32 c625_geometry(u32 row) { return row==0u ? 9776u : row==1u ? 9192u : row==2u ? 9168u : 9144u; }
SI u32 c625_vertex_section(u32 row) { return row==0u ? 5432u : row==1u ? 4928u : row==2u ? 4904u : 4880u; }
SI u32 c625_bone_section(u32 row) { return row==0u ? 5152u : row==1u ? 4648u : row==2u ? 4640u : 4632u; }
SI u32 c625_poses(u32 row) { return row<=1u ? 17u : row==2u ? 16u : 15u; }
SI u32 c625_vertices(u32 row) { return row ? 212u : 216u; }
SI u32 c625_physical(u32 row) { return row ? 168u : 144u; }
/* Row 0 keeps the accepted 0625 walk. A new row has exactly 15 contiguous type17 nodes (type10
 * refused) ending at the model's end, deform section 2520 and the same default-table
 * identities. */
SI u32 c625_graph_row(u32 model,u32 row) {
    u32 bytes=c625_model_bytes(row),bone=c625_bone_section(row),node=model+c625_geometry(row);
    u32 i,step,table=D_8012CF8C;
    if(!row)return c625_graph(model);
    if(!silo_range(model,bytes,7u) || *(volatile u32*)(model+4u)!=c625_geometry(row) ||
        *(volatile u16*)(model+10u)!=0u || *(volatile u32*)(model+16u)!=c625_vertex_section(row) ||
        *(volatile u32*)(model+24u)!=bone || *(volatile u32*)(model+40u)!=2520u ||
        *(volatile s16*)(model+bone+4u)!=(s32)c625_poses(row) ||
        !silo_range(table,18u*4u,3u) || table!=*(volatile u32*)0x80123198u ||
        *(volatile u32*)(table+40u)!=0x800DD410u ||
        *(volatile u32*)(table+68u)!=0x800DD65Cu)return 0;
    for(i=0;i<15u;i++,node+=16u) {
        step=*(volatile u32*)(node+4u);
        if(*(volatile u32*)node!=17u || (i<14u ? step!=16u : step!=0u) ||
            *(volatile s16*)(node+8u)<0 || (*(volatile u16*)(node+14u)&1u))return 0;
    }
    return node==model+bytes;
}
/* P04a storage admission. This DEFA0 consumed the most recent S2 redirect: it must be this
 * DE498's own segment command (sp+0xB8, transported by the native admit), name this copy, lie
 * in the open pool's main list before this occurrence, and be the last recorded reference of the
 * copy's LIVE entry in the open construction. Returns that entry, else ENTRIES. */
SI u32 c625_storage(u32 copy,u32 base,u32 current,u32 last_copy,u32 last_command,u32 draw_command) {
    u32 p=CK.pool,i,used,n;
    struct TooieCkcopyEntry* e;
    if(CK.magic!=TOOIE_CKCOPY_MAGIC || CK.layout!=TOOIE_CKCOPY_LAYOUT || CK.fault || !CK.active ||
        CK.disqualified || p!=(u32)D_8007BEDA || !last_copy || last_copy!=copy || last_command!=draw_command ||
        last_command<base || last_command>current-8u || (last_command&7u))return TOOIE_CKCOPY_ENTRIES;
    used=CK.used[p]>TOOIE_CKCOPY_ENTRIES ? TOOIE_CKCOPY_ENTRIES : CK.used[p];
    for(i=0;i<used;i++) {
        e=&CK.entry[p][i];
        if(e->source!=copy)continue;
        n=e->nrefs;
        return e->state==TOOIE_CKCOPY_LIVE && n && n<=TOOIE_CKCOPY_REFS &&
            e->refs[n-1u]==last_command ? i : TOOIE_CKCOPY_ENTRIES;
    }
    return TOOIE_CKCOPY_ENTRIES;
}
/* Emit the placeholder in the paid root slot and register {label, real ID} for the owning S3
 * while the copy has label room; a fifth occurrence of one copy stays a counted placeholder. */
SI void c625_label(u32 at,u32 live,u32 id) {
    struct TooieCkcopyEntry* e=&CK.entry[CK.pool][live];
    cam_group_words(at,G_EX_ID_IGNORE,C625_PLACEHOLDER_POLICY);
    if(e->nlabels<TOOIE_CKCOPY_LABELS) {
        e->labels[e->nlabels][0]=at;e->labels[e->nlabels][1]=id;e->nlabels++;
    } else CK.placeholders++;
}
/* Shared identity decision for both roles. F4: the phase-5 silhouette attachment precedes draw3
 * in the same view, so either role may be first in an epoch. Renewal resets both masks once; an
 * epoch advance resets both (F6: the attachment mask lives in flags bits 1..28, bit 0 keeps its
 * renewal value); each role tests and sets only its own bit. Returns 0, 12 (same-epoch repeat of
 * this role and cell) or 13 (ID blocks exhausted). */
SI u32 c625_identity(struct TooieCpu625Owner* e,u32 backpack,u32 model,u32 handle,u32 camera,u32 attachment) {
    u32 cell=CX.view*7u+CX.phase,bit=attachment ? 2u<<cell : 1u<<cell,renew,i;
    renew=!e->idbase || e->owner!=CX.owner || e->backpack!=backpack ||
        e->model!=model || e->handle!=handle ||
        (e->stamp!=SS.epoch && e->stamp+1u!=SS.epoch) ||
        (e->camera_id[CX.view] && e->camera_id[CX.view]!=camera);
    if(!renew && e->stamp==SS.epoch && ((attachment ? e->flags : e->seen)&bit))return 12u;
    if(renew) {
        if(CS.next_block>=TOOIE_CPU625_BLOCKS)return 13u;
        e->owner=CX.owner;e->backpack=backpack;e->model=model;e->handle=handle;
        e->idbase=TOOIE_CPU625_ID_FIRST+512u*CS.next_block++;
        e->stamp=SS.epoch;e->seen=0;e->flags=1u;
        for(i=0;i<4u;i++)e->camera_id[i]=0;
    } else if(e->stamp!=SS.epoch) {e->stamp=SS.epoch;e->seen=0;e->flags&=1u;}
    if(attachment)e->flags|=bit;
    else e->seen|=bit;
    e->camera_id[CX.view]=camera;
    return 0;
}
/* N3 camera law: the accepted c625_camera checks without its main-draw capacity terms (the
 * attachment's bytes were prepaid by the provisional shadow prepare). */
SI u32 c625_attach_camera(void) {
    struct TooieCameraControl* c=&CAM_STATE.control;
    struct TooieCameraProduction* p=&CAM_STATE.saved;
    struct TooieCameraRecord* r;
#if TOOIE_PRE_BACKGROUND
    if(tooie_pre_background.pending.depth)return 0;
#endif
    if(!CAM_STATE.owner_frame || !c->active || D_8012D508 || c->latch)return 0;
    r=cam_find(p->camera);
    if(c->epoch!=SS.epoch || !p->valid || p->epoch!=c->epoch ||
        p->matrix!=D_8012D558 || p->camera!=D_8012D500 || p->view>=4u ||
        p->view!=CX.view || p->view!=(u32)D_80127674 ||
        p->id<TOOIE_CAMERA_ID_FIRST || p->id>=TOOIE_CAMERA_ID_END ||
        p->policy!=CAM_POLICY_WORLD || p->role!=TOOIE_CAMERA_WORLD_INITIAL ||
        !r || r->birth!=p->birth || r->revision!=p->revision ||
        r->view_id[p->view]!=p->id || r->view_scale[p->view]!=p->scale)return 0;
    return p->id;
}
/* N3 staging: the main draw3 premises except the root mode, which the shadow route stages as
 * exactly 3 (DF540, shadow matrix in C9C8); pre/post callbacks stay zero. Graph proved range. */
SI u32 c625_attach_staged(u32 model,u32 row) {
    u32 bytes=c625_model_bytes(row),poses=c625_poses(row);
    s32 texture_offset=*(volatile s16*)(model+8u);
    return !(texture_offset<0 || (u32)texture_offset>bytes-8u || D_8012C838 || D_8012CEC4 ||
        D_8012C948[24] || D_8012C948[26] || D_8012C948[28]!=3u ||
        D_8012C948[5]!=model+(u32)texture_offset ||
        D_8012C948[6]!=model+*(volatile u32*)(model+12u) ||
        D_8012C948[6]<model || D_8012C948[6]>model+bytes-8u ||
        *(volatile u32*)0x8012C848u || !D_8012C968 ||
        !silo_range((u32)D_8012C968,0x50u+poses*64u,3u) ||
        *(volatile u32*)D_8012C968!=poses || *((volatile u32*)D_8012C968+1)<poses);
}
/* N3 DEFA0 for an open attachment owner (exclusive dispatch from tooie_cpu625_admit, after its
 * single snapshot/clear of the redirect register). The target is recognized by the borrowed
 * context plus model/buffers equality at render depth 1; the A6250 frame is checked only at the
 * paid end. Premise: the original default-handler/no-callback route, where no reentrant draw
 * occurs in the bracket. A DEFA0 failing that recognition is ignored (its redirect was already
 * consumed and is never lent); a same-model depth-1 reentrant draw is NOT excluded by frame
 * evidence and is outside the guarantee. The first recognized DEFA0 emits one 16-byte group into the bytes the
 * provisional prepare paid: the public placeholder registered for S3 promotion to role +2, or
 * on any later failure the same placeholder unregistered (explicit no-interpolation). Only a
 * broken MAIN premise (F2) emits nothing (counted drift); no guarantee is claimed past it. */
SI u32 c625_attach_admit(u32 buffers,u32 cursor_slot,u32 model,u32 draw_command,u32 last_copy,u32 last_command) {
    struct TooieCameraControl* c=&CAM_STATE.control;
    struct TooieCpu625Owner* e;
    u32 slot,backpack,row,handle,copy,camera,base,current,p,live=TOOIE_CKCOPY_ENTRIES,reason=0,id=0;
    if(CX.paid)return 0;
    if(SS.silo_depth!=1u || model!=CX.model || buffers!=0x80127658u || !silo_range(cursor_slot,4u,3u))return 0;
    backpack=*(volatile u32*)(CX.owner+20u);
    row=silo_range(backpack,24u,3u) ? ckpack_row(backpack) : 4u;
    p=(u32)(s32)D_8007BEDA;base=p<=1u ? D_8007BCD0[p] : 0u;current=*(volatile u32*)cursor_slot;
    if(p>1u || D_8007BCE8[0] || D_8007BCE8[1] || D_8007BCF0[0] || D_8007BCF0[1] ||
        base!=(u32)&tooie_camera_main[p][0] || c->main_base!=base || c->main_end!=base+TOOIE_CAMERA_M_BYTES ||
        !c->active || c->latch || D_8012D508 || (current&7u) || current<c->main_begin ||
        current>base+TOOIE_CAMERA_M_BYTES-(row<=3u ? c625_physical(row)-8u : 160u)) {
        CX.paid=2u;silo_count(&CS.drifts);CS.last_reason=40u;return 0;
    }
    slot=c625_slot(CX.owner);
    if(CX.epoch!=SS.epoch || CX.view>=4u || !c625_phase(CX.phase) || CX.view!=(u32)D_80127674 ||
        CX.phase!=(u32)(s32)D_80132E9C ||
        (slot<8u && SS.primary[slot][CX.view].view_phase!=(CX.view|(CX.phase<<2))))reason=41u;
    else if(slot>=8u)reason=43u;
    else if(row>3u || !*(volatile u8*)(backpack+8u) || *(volatile u8*)(backpack+9u)>1u ||
        *(volatile u8*)(backpack+10u)!=1u)reason=44u;
    else {
        handle=*(volatile u8*)(backpack+8u);
        if(!c625_lookup_safe(handle))reason=45u;
        else {
            copy=(u32)func_80100A74((s16)handle,*(volatile u8*)(backpack+9u));
            e=&CS.owner[slot];
            if(!silo_range(copy,24u+16u*c625_vertices(row),7u) || D_8012C948[7]!=copy ||
                D_8012C948[11]!=(u32)func_80100AC4((s16)handle) || !c625_graph_row(model,row) ||
                *(volatile u16*)(copy+22u)>>1!=c625_vertices(row))reason=46u;
            else if(!c625_attach_staged(model,row))reason=47u;
            else if(!(camera=c625_attach_camera()))reason=48u;
            /* All four rows, including 0625, need this draw's own LIVE redirect (no main-0625 bypass). */
            else if((live=c625_storage(copy,base,current,last_copy,last_command,draw_command))>=TOOIE_CKCOPY_ENTRIES)reason=49u;
            /* A live identity whose model differs is not proven to alias this model: never renew
             * the shared block on it (avoids per-frame main/attachment renewal alternation). */
            else if(e->idbase && e->owner==CX.owner && e->backpack==backpack && e->handle==handle &&
                e->model!=model && (e->stamp==SS.epoch || e->stamp+1u==SS.epoch))reason=50u;
            else if((reason=c625_identity(e,backpack,model,handle,camera,1u))==0u)
                id=e->idbase+CX.view*128u+CX.phase*16u+C625_ATTACH_ROLE;
        }
    }
    /* Attachment refusals never forget or renew identity: the main role re-decides it. */
    if(reason) {silo_count(&CS.refusals);cam_group_words(current,G_EX_ID_IGNORE,C625_PLACEHOLDER_POLICY);}
    else c625_label(current,live,id);
    CS.last_reason=reason;
    CX.pool_index=p;CX.gfx_base=base;CX.gfx_begin=current;CX.group_id=id;CX.paid=1u;
    *(volatile u32*)cursor_slot=current+16u;
    if(c->main_begin<current+16u)c->main_begin=current+16u;
    return 0;
}
#endif
CPU625 u32 tooie_cpu625_begin(u32 self,u32 model) {
    u32 slot,backpack;
    if(CS.magic!=TOOIE_CPU625_MAGIC || CS.layout!=TOOIE_CPU625_LAYOUT)return c625_trip(1u);
#if TOOIE_CKCOPY
    /* N3: an attachment owner whose A6250 route never reached A66C4 but emitted nothing
     * (reserved, open or spent) is released (counted drift). Only a paid group, whose pop is
     * owed, trips below. */
    if(CX.owner && CX.depth==TOOIE_CPU625_ATTACH && CX.paid!=1u) {
        c625_clear();silo_count(&CS.drifts);
    }
#endif
    if(CX.owner)return c625_trip(2u);
    if(CS.tripwires)return 1u;
    slot=c625_slot(self);
    if(slot>=8u) {
        c625_forget(self);
        return 0;
    }
    backpack=*(volatile u32*)(self+20u);
#if TOOIE_CKCOPY
    if(!silo_range(backpack,24u,3u) || ckpack_row(backpack)>3u)return 0;
#else
    if(!silo_range(backpack,24u,3u) || *(volatile u32*)(backpack+12u)!=3u ||
        *(volatile u32*)backpack!=0x625u || *(volatile u32*)(backpack+16u)!=0x3DCCCCCDu)return 0;
#endif
    CX.owner=self;CX.model=model;CX.view=(u32)D_80127674;
    CX.phase=(u32)(s32)D_80132E9C;CX.epoch=SS.epoch;CX.depth=SS.silo_depth;
    if(CX.depth || SS.active || SC.actor || SC.admitted || D_8012C948[24] ||
        !SS.epoch || SS.epoch==0xFFFFFFFFu || CX.view>=4u || !c625_phase(CX.phase)) {
        c625_refuse(3u);
        /* Failed lexical begin never creates a paid token. */
        CX.depth=0xFFFFFFFFu;
    }
    return 0;
}
#if TOOIE_CKCOPY
CPU625 u32 tooie_cpu625_admit(u32 buffers,u32 cursor_slot,u32 model,u32 draw_command) {
#else
CPU625 u32 tooie_cpu625_admit(u32 buffers,u32 cursor_slot,u32 model) {
#endif
    u32 slot,backpack,handle,copy,camera,base,current,seen,renew,i;
#if TOOIE_CKCOPY
    u32 row,bytes,poses,live=TOOIE_CKCOPY_ENTRIES;
    u32 last_copy=CK.last_copy,last_command=CK.last_command;
#endif
    s32 texture_offset;
    struct TooieCpu625Owner* e;
    struct TooieCameraControl* c=&CAM_STATE.control;
#if TOOIE_CKCOPY
    /* Every DEFA0 consumes the most recent S2 redirect, admitted or not: no later or nested draw
     * can reuse another draw's redirect. */
    CK.last_copy=0;CK.last_command=0;
    /* N3: one consumer; the open owner's kind selects exactly one path. */
    if(CX.owner && CX.depth==TOOIE_CPU625_ATTACH)
        return c625_attach_admit(buffers,cursor_slot,model,draw_command,last_copy,last_command);
#endif
    if(!CX.owner || CX.paid)return 0;
    if(CS.tripwires || CX.depth || SS.silo_depth!=1u || SS.active || SC.actor || SC.admitted)
        return c625_refuse(4u);
    if(CX.model!=model || CX.epoch!=SS.epoch || CX.view!=(u32)D_80127674 ||
        CX.phase!=(u32)(s32)D_80132E9C)return c625_refuse(5u);
    slot=c625_slot(CX.owner);if(slot>=8u)return c625_refuse(6u);
    backpack=*(volatile u32*)(CX.owner+20u);
#if TOOIE_CKCOPY
    row=silo_range(backpack,24u,3u) ? ckpack_row(backpack) : 4u;
    if(row>3u || !*(volatile u8*)(backpack+8u) || *(volatile u8*)(backpack+9u)>1u ||
        *(volatile u8*)(backpack+10u)!=1u)return c625_refuse(7u);
#else
    if(!silo_range(backpack,24u,3u) || *(volatile u32*)(backpack+12u)!=3u ||
        *(volatile u32*)backpack!=0x625u || *(volatile u32*)(backpack+16u)!=0x3DCCCCCDu ||
        !*(volatile u8*)(backpack+8u) || *(volatile u8*)(backpack+9u)>1u ||
        *(volatile u8*)(backpack+10u)!=1u)return c625_refuse(7u);
#endif
    handle=*(volatile u8*)(backpack+8u);
    if(!c625_lookup_safe(handle))return c625_refuse(16u);
    copy=(u32)func_80100A74((s16)handle,*(volatile u8*)(backpack+9u));
#if TOOIE_CKCOPY
    if(!silo_range(copy,24u+16u*c625_vertices(row),7u) || D_8012C948[7]!=copy ||
        D_8012C948[11]!=(u32)func_80100AC4((s16)handle) || !c625_graph_row(model,row) ||
        *(volatile u16*)(copy+22u)>>1!=c625_vertices(row))return c625_refuse(8u);
    bytes=c625_model_bytes(row);poses=c625_poses(row);
#else
    if(!silo_range(copy,3480u,7u) || D_8012C948[7]!=copy ||
        D_8012C948[11]!=(u32)func_80100AC4((s16)handle) || !c625_graph(model) ||
        *(volatile u16*)(copy+22u)>>1!=216u)return c625_refuse(8u);
#endif
    /* graph above proves the full model range before this original signed field. */
    texture_offset=*(volatile s16*)(model+8u);
#if TOOIE_CKCOPY
    if(texture_offset<0 || (u32)texture_offset>bytes-8u || D_8012C838 || D_8012CEC4 ||
        D_8012C948[24] || D_8012C948[26] || D_8012C948[28] ||
        D_8012C948[5]!=model+(u32)texture_offset ||
        D_8012C948[6]!=model+*(volatile u32*)(model+12u) ||
        D_8012C948[6]<model || D_8012C948[6]>model+bytes-8u ||
        *(volatile u32*)0x8012C848u || !D_8012C968 ||
        !silo_range((u32)D_8012C968,0x50u+poses*64u,3u) ||
        *(volatile u32*)D_8012C968!=poses || *((volatile u32*)D_8012C968+1)<poses)
        return c625_refuse(9u);
#else
    if(texture_offset<0 || (u32)texture_offset>9992u-8u || D_8012C838 || D_8012CEC4 ||
        D_8012C948[24] || D_8012C948[26] || D_8012C948[28] ||
        D_8012C948[5]!=model+(u32)texture_offset ||
        D_8012C948[6]!=model+*(volatile u32*)(model+12u) ||
        D_8012C948[6]<model || D_8012C948[6]>model+9992u-8u ||
        *(volatile u32*)0x8012C848u || !D_8012C968 ||
        !silo_range((u32)D_8012C968,0x50u+17u*64u,3u) ||
        *(volatile u32*)D_8012C968!=17u || *((volatile u32*)D_8012C968+1)<17u)
        return c625_refuse(9u);
#endif
    if(buffers!=0x80127658u || !silo_range(cursor_slot,4u,3u) ||
        D_8007BEDA<0 || D_8007BEDA>1 || D_8007BCE8[0] || D_8007BCE8[1] ||
        D_8007BCF0[0] || D_8007BCF0[1])return c625_refuse(10u);
    base=D_8007BCD0[D_8007BEDA];current=*(volatile u32*)cursor_slot;
    camera=c625_camera(current);
    if(!camera || base!=(u32)&tooie_camera_main[D_8007BEDA][0] ||
        c->main_base!=base || c->main_end!=base+TOOIE_CAMERA_M_BYTES ||
        !silo_range(base,TOOIE_CAMERA_M_BYTES,7u) || current<base ||
        current>base+TOOIE_CAMERA_M_BYTES-144u || (current&7u) || SS.extra_bytes>6400u)
        return c625_refuse(11u);
#if TOOIE_CKCOPY
    /* Row physical room; the camera ledger above is unchanged (32 debit + 48 margin). */
    if(current>base+TOOIE_CAMERA_M_BYTES-c625_physical(row))return c625_refuse(11u);
    if(row) {
        live=c625_storage(copy,base,current,last_copy,last_command,draw_command);
        if(live>=TOOIE_CKCOPY_ENTRIES)return c625_refuse(17u);
    }
    /* Shared with the N3 attachment role (c625_identity); main keeps its own seen bit. */
    e=&CS.owner[slot];
    renew=c625_identity(e,backpack,model,handle,camera,0u);
    if(renew==12u)return c625_refuse_repeat();
    if(renew)return c625_refuse(renew);
    (void)seen;(void)i;
#else
    e=&CS.owner[slot];seen=1u<<(CX.view*7u+CX.phase);
    renew=!e->idbase || e->owner!=CX.owner || e->backpack!=backpack ||
        e->model!=model || e->handle!=handle ||
        (e->stamp!=SS.epoch && e->stamp+1u!=SS.epoch) ||
        (e->camera_id[CX.view] && e->camera_id[CX.view]!=camera);
    if(!renew && e->stamp==SS.epoch && (e->seen&seen))return c625_refuse_repeat();
    if(renew) {
        if(CS.next_block>=TOOIE_CPU625_BLOCKS)return c625_refuse(13u);
        e->owner=CX.owner;e->backpack=backpack;e->model=model;e->handle=handle;
        e->idbase=TOOIE_CPU625_ID_FIRST+512u*CS.next_block++;
        e->stamp=SS.epoch;e->seen=0;e->flags=1u;
        for(i=0;i<4u;i++)e->camera_id[i]=0;
    } else if(e->stamp!=SS.epoch) {e->stamp=SS.epoch;e->seen=0;}
    e->seen|=seen;e->camera_id[CX.view]=camera;
#endif
    CX.buffers=buffers;CX.pool_index=(u32)D_8007BEDA;CX.gfx_base=base;CX.gfx_begin=current;
    CX.group_id=e->idbase+CX.view*128u+CX.phase*16u+1u;CX.paid=1u;CS.epoch=SS.epoch;
    /* 24 prefix + remaining original (0625 112, P04a rows 136) + 8 paid close = 144/168 bytes. */
    c->main_extra+=32u;c->main_begin=current+24u;
    cam_enable_words(current);
#if TOOIE_CKCOPY
    if(row)c625_label(current+8u,live,CX.group_id);
    else cam_group_words(current+8u,CX.group_id,C625_ROOT_POLICY);
#else
    cam_group_words(current+8u,CX.group_id,C625_ROOT_POLICY);
#endif
    *(volatile u32*)cursor_slot=current+24u;CS.last_reason=0;return 0;
}
CPU625 u32 tooie_cpu625_end(u32 self) {
    u32 current,base,pool;
    if(!CX.owner)return CS.tripwires?1u:0u;
    if(!CX.paid){c625_clear();return CS.tripwires?1u:0u;}
    pool=CX.pool_index;base=CX.gfx_base;
    current=CX.buffers==0x80127658u ? *(volatile u32*)CX.buffers : 0u;
    if(self!=CX.owner || CX.epoch!=SS.epoch || CX.epoch!=CAM_STATE.control.epoch || pool>=2u || D_8007BEDA!=(s32)pool ||
        D_8007BCD0[pool]!=base || base!=(u32)&tooie_camera_main[pool][0] ||
        CX.buffers!=0x80127658u || !silo_range(base,TOOIE_CAMERA_M_BYTES,7u) ||
        CX.gfx_begin<base || CX.gfx_begin>base+TOOIE_CAMERA_M_BYTES-144u ||
        current<CX.gfx_begin+24u || current>base+TOOIE_CAMERA_M_BYTES-8u || (current&7u))
        return c625_trip(14u);
    if(CX.view!=(u32)D_80127674 || CX.phase!=(u32)(s32)D_80132E9C)silo_count(&CS.drifts);
    /* Public pop word0=64|POP opcode, word1=count1/MODELVIEW. Exact macro fixture binds it. */
    {volatile u32* words=(volatile u32*)current;words[0]=0x6400000Du;words[1]=1u;}
    *(volatile u32*)CX.buffers=current+8u;
    if(CAM_STATE.control.main_begin<current+8u)CAM_STATE.control.main_begin=current+8u;
    c625_clear();return CS.tripwires?1u:0u;
}
CPU625 u32 tooie_cpu625_retire(u32 self,u32 requested,u32 reason) {
    u32 backpack,old=0;
    if(reason==1u) {
        if(!silo_range(self,24u,3u))return 0;
        backpack=*(volatile u32*)(self+20u);
        if(!silo_range(backpack,24u,3u))return 0;
        old=*(volatile u32*)(backpack+12u);
#if TOOIE_CKCOPY
        /* P04a: entering or leaving any CPU-deform row state (3..6) ends the identity. */
        if((old<3u || old>6u) && (requested<3u || requested>6u))return 0;
#else
        if(old!=3u && requested!=3u)return 0;
#endif
    }
    c625_forget(self);
    /* Retained identity may retire during a paid bracket; its saved token survives. */
    if(CX.owner==self) {silo_count(&CS.tripwires);CS.last_reason=15u;}
    return 0;
}
#if TOOIE_CKCOPY
/* N3 A66BC: before the direct DE448 jal and its A66C0 delay-slot store. Native passes the A6250
 * frame's owner word (sp+0x1CC), the untouched original r2 model and that frame. State only.
 * Opens only the reservation the provisional shadow prepare made for this owner and epoch, and
 * only for a silhouette that emitted its 480 group; a silhouette left unlabelled leaves the
 * attachment unlabelled too (composite unchanged from today). */
CPU625 u32 tooie_cpu625_attach_begin(u32 owner,u32 model,u32 frame) {
    u32 component,slot,view;
    if(CS.magic!=TOOIE_CPU625_MAGIC || CS.layout!=TOOIE_CPU625_LAYOUT)return c625_trip(1u);
    if(CX.owner && CX.depth!=TOOIE_CPU625_ATTACH)return c625_trip(2u); /* draw3 owner open */
    if(CS.tripwires)return 1u;
    if(!CX.owner || CX.paid!=TOOIE_CPU625_ATTACH_RESERVED)return 0;      /* nothing reserved */
    component=silo_range(owner,0x188u,3u) ? *(volatile u32*)(owner+0x50u) : 0u;
    /* PC[8]: the same retained primary entry tooie_shadow_prepare selected. CPU625 owner
     * qualification (c625_slot) is unchanged: controlled-only. */
    slot=c625_slot(owner);view=(u32)D_80127674;
    if(CX.owner!=owner || CX.epoch!=SS.epoch || CAM_STATE.control.epoch!=SS.epoch || SS.silo_depth ||
        !silo_range(component,0x118u,3u) || !*(volatile u8*)(component+0x11u) ||
        slot>=8u || view>=4u || !SS.primary[slot][view].idbase || SS.primary[slot][view].owner!=owner ||
        shadow_pending.id!=SS.primary[slot][view].idbase+C625_SHADOW_ROLE || !shadow_pending.group ||
        shadow_pending.seen_epoch!=SS.epoch) {
        CX.paid=2u;CS.last_reason=39u;return 0;  /* spent: the end only releases it */
    }
    CX.model=model;CX.view=(u32)D_80127674;CX.phase=(u32)(s32)D_80132E9C;CX.buffers=frame;CX.paid=0;
    return 0;
}
/* N3 A66C4: reached both from the DE448 return and from the A6674 skip branch (generator-checked
 * placement after the label). r2 is not the model here. Closes only a scope this owner paid:
 * one public pop after the attachment's original pop, before the A7180 getter. */
CPU625 u32 tooie_cpu625_attach_end(u32 owner,u32 frame) {
    u32 current,g;
    if(!CX.owner || CX.depth!=TOOIE_CPU625_ATTACH)return CS.tripwires?1u:0u;
    if(CX.owner!=owner)return c625_trip(18u);
    if(CX.paid==1u) {
        current=*(volatile u32*)0x80127658u;g=CX.gfx_begin;
        if(CX.buffers!=frame || CX.epoch!=SS.epoch || CX.pool_index>1u || D_8007BEDA!=(s32)CX.pool_index ||
            D_8007BCD0[CX.pool_index]!=CX.gfx_base || (current&7u) || current<g+16u ||
            current>CX.gfx_base+TOOIE_CAMERA_M_BYTES-8u)return c625_trip(19u);
        {volatile u32* words=(volatile u32*)current;words[0]=0x6400000Du;words[1]=1u;}
        *(volatile u32*)0x80127658u=current+8u;
        if(CAM_STATE.control.main_begin<current+8u)CAM_STATE.control.main_begin=current+8u;
    }
    c625_clear();return CS.tripwires?1u:0u;
}
#endif
#endif
