/* Shared ckpack two-copy vertex storage (P04b, layout 2). Within one open construction, owned
 * by one A7840 frame, each segment-1 reference to a protected copy reads a slot owned by that
 * construction's Gfx pool; the slot receives the copy's final bytes when that frame's
 * construction ends. The first disposition of a copy address is sticky for the root: a release,
 * extent change, reference overflow, foreign command storage or nested construction restores
 * every redirected word of that copy (all or nothing) and keeps the address original.
 * P04a: CPU625 registers each new-row placeholder against the LIVE copy whose redirect it
 * consumed; the owning end validates every registered label, then publishes, then promotes.
 * Finite contract: original manager-mediated allocation/free and append-only main-list
 * construction. A redirected word that no longer holds its command, or a registered label that
 * is no longer its placeholder, faults the construction. */
#include "../silo-candidate/silo_bridge.h"
#include "../camera-candidate/camera_projection_internal_04.h"
#include "ckcopy_layout.h"
#if !defined(TOOIE_CPU625) || TOOIE_CPU625 != 1
#error ckpack copy storage joins the selected CPU625 composition
#endif
#define CKCOPY __attribute__((section(".tooie_ckcopy_helpers"),noinline))
#define CKI static inline __attribute__((always_inline))
#define CK tooie_ckcopy_state
extern u32 D_80135490[8];
struct TooieCkcopyState tooie_ckcopy_state
    __attribute__((section(".tooie_ckcopy_state"),aligned(8)))=
    {.magic=TOOIE_CKCOPY_MAGIC,.layout=TOOIE_CKCOPY_LAYOUT};

CKI u32 ckcopy_phys(u32 address) { return address&0x1FFFFFFFu; }
CKI u32 ckcopy_slot(u32 p,u32 s) { return (u32)&CK.slot[p][s][0]; }
CKI u32 ckcopy_used(u32 p) { return CK.used[p]>TOOIE_CKCOPY_ENTRIES ? TOOIE_CKCOPY_ENTRIES : CK.used[p]; }
/* The open pool's accepted construction storage: its main Gfx list, 8-aligned, room for one command. */
CKI u32 ckcopy_owned(u32 p,u32 at) {
    u32 base=D_8007BCD0[p];
    return base==(u32)&tooie_camera_main[p][0] && at>=base && at<=base+TOOIE_CAMERA_M_BYTES-8u && !(at&7u);
}
/* Current copy of a roster owner's two-copy ckpack part: owner+20 part, part+8 handle,
 * part+10 two-copy flag; copy-manager FreeList 80135A60 record +0/+4 copies, +9 active. */
CKI u32 ckcopy_protected(u32 copy,u32* count) {
    u32 i,owner,part,pool,record,handle,n;
    s32 capacity;
    pool=*(volatile u32*)0x80135A60u;
    if(!silo_range(pool,4u,3u) || *(volatile s16*)pool!=12)return 0;
    capacity=*(volatile s16*)(pool+2u);
    if(capacity<=1 || !silo_range(pool,4u+12u*(u32)capacity,3u))return 0;
    for(i=0;i<8u;i++) {
        owner=D_80135490[i];
        if(!silo_range(owner,0x188u,3u))continue;
        part=*(volatile u32*)(owner+20u);
        if(!silo_range(part,24u,3u) || *(volatile u8*)(part+10u)!=1u)continue;
        handle=*(volatile u8*)(part+8u);
        if(!handle || handle>=(u32)capacity)continue;
        record=pool+4u+12u*handle;
        if(!*(volatile u8*)(record+9u) ||
           (*(volatile u32*)record!=copy && *(volatile u32*)(record+4u)!=copy))continue;
        n=*(volatile u16*)(copy+22u)>>1;
        if(!n || n>TOOIE_CKCOPY_VERTICES || !silo_range(copy,24u+16u*n,7u))return 0;
        *count=n;return 1;
    }
    return 0;
}
/* Sticky original disposition for an address not yet in this root's table (room permitting;
 * a full table admits no new enrollment for the rest of the root). */
CKI void ckcopy_hold(u32 p,u32 source) {
    u32 i,used=ckcopy_used(p);
    struct TooieCkcopyEntry* e;
    for(i=0;i<used;i++)if(CK.entry[p][i].source==source)return;
    if(used>=TOOIE_CKCOPY_ENTRIES)return;
    e=&CK.entry[p][used];CK.used[p]=used+1u;
    e->source=source;e->count=0;e->slot=0;e->nrefs=0;e->nlabels=0;e->state=TOOIE_CKCOPY_HELD;
}
/* All or nothing: every recorded word must still be this construction's redirected command in
 * the open pool's main list before any is restored. Otherwise latch the fault (fatal stop).
 * Registered labels stay placeholders: a HELD entry is never promoted. */
CKI u32 ckcopy_revert(u32 p,u32 i) {
    struct TooieCkcopyEntry* e=&CK.entry[p][i];
    u32 r,at,n=e->nrefs,slot,original=ckcopy_phys(e->source+24u);
    if(n>TOOIE_CKCOPY_REFS || e->slot>=TOOIE_CKCOPY_SLOTS){CK.fault=1;return 1;}
    slot=ckcopy_phys(ckcopy_slot(p,e->slot));
    for(r=0;r<n;r++) {
        at=e->refs[r];
        if(!ckcopy_owned(p,at) || *(volatile u32*)at!=0xDB060004u || *(volatile u32*)(at+4u)!=slot) {
            CK.fault=1;return 1;
        }
    }
    for(r=0;r<n;r++)*(volatile u32*)(e->refs[r]+4u)=original;
    if(e->state==TOOIE_CKCOPY_LIVE)CK.reverted++;
    e->state=TOOIE_CKCOPY_HELD;e->nrefs=0;
    return 0;
}
/* Disqualify the captured open table only (never a table chosen from the current selector). */
CKI u32 ckcopy_disqualify(void) {
    u32 p=CK.pool,i,used,bad=0;
    CK.disqualified=1;
    if(p>1u)return 0;
    used=ckcopy_used(p);
    for(i=0;i<used;i++)if(CK.entry[p][i].state==TOOIE_CKCOPY_LIVE)bad|=ckcopy_revert(p,i);
    return bad;
}
/* P04a: every registered label must still be its public placeholder, all 16 bytes inside the
 * open pool's main list, naming an ID inside the CPU625 blocks. */
CKI u32 ckcopy_labels_valid(u32 p,struct TooieCkcopyEntry* e) {
    u32 k,at,id;
    volatile u32* group;
    if(e->nlabels>TOOIE_CKCOPY_LABELS)return 0;
    for(k=0;k<e->nlabels;k++) {
        at=e->labels[k][0];id=e->labels[k][1];
        if(!ckcopy_owned(p,at) || !ckcopy_owned(p,at+8u) ||
           id<TOOIE_CKCOPY_LABEL_ID_FIRST || id>=TOOIE_CKCOPY_LABEL_ID_END)return 0;
        group=(volatile u32*)at;
        if(group[0]!=CAM_GROUP_OPCODE || group[1]!=0u ||
           group[2]!=TOOIE_CKCOPY_PLACEHOLDER_POLICY || group[3]!=0u)return 0;
    }
    return 1;
}
/* S1 (A7840 before 0x800A7884, after both pool toggles; frame = A7840's sp). Opens restoration
 * authority for this frame's construction, or, if a construction is already open, disqualifies
 * the open outer table only and leaves its owner, pool and every completed pool untouched. */
CKCOPY u32 tooie_ckcopy_root_begin(u32 frame) {
    u32 p=(u32)D_8007BEDA;
    if(CK.magic!=TOOIE_CKCOPY_MAGIC || CK.layout!=TOOIE_CKCOPY_LAYOUT || CK.fault)return 1;
    CK.last_copy=0;CK.last_command=0;
    if(CK.active){CK.nested++;return ckcopy_disqualify();}
    if(p>1u)return 0;
    CK.active=1;CK.frame=frame;CK.pool=p;CK.used[p]=0;CK.slots[p]=0;
    CK.disqualified=D_8007BCD0[p]!=(u32)&tooie_camera_main[p][0];
    return 0;
}
/* S2 (DE498 before 0x800DE8C8): DE8C4 stored phys(C948[7]+24) at command+4; DE498 keeps the
 * command pointer at sp+0xB8. The first encounter of an address fixes its disposition. Only a
 * successful redirect becomes the most recent redirect; every other outcome clears it. */
CKCOPY u32 tooie_ckcopy_segment(u32 command) {
    u32 copy=D_8012C948[7],p,i,n=0,used;
    struct TooieCkcopyEntry* e;
    if(CK.magic!=TOOIE_CKCOPY_MAGIC || CK.fault)return 1;
    CK.last_copy=0;CK.last_command=0;
    if(!CK.active || !copy)return 0;
    p=CK.pool;
    if(p!=(u32)D_8007BEDA)return ckcopy_disqualify();
    used=ckcopy_used(p);
    for(i=0;i<used;i++)if(CK.entry[p][i].source==copy)break;
    if(i==used) {
        if(used>=TOOIE_CKCOPY_ENTRIES){CK.exhausted++;return 0;}
        e=&CK.entry[p][used];CK.used[p]=used+1u;
        e->source=copy;e->count=0;e->slot=0;e->nrefs=0;e->nlabels=0;e->state=TOOIE_CKCOPY_HELD;
        if(!CK.disqualified && ckcopy_protected(copy,&n)) {
            if(CK.slots[p]>=TOOIE_CKCOPY_SLOTS)CK.exhausted++;
            else {e->state=TOOIE_CKCOPY_LIVE;e->count=n;e->slot=CK.slots[p]++;}
        }
    } else e=&CK.entry[p][i];
    if(e->state!=TOOIE_CKCOPY_LIVE)return 0;
    if(e->nrefs>=TOOIE_CKCOPY_REFS)CK.overflowed++;
    if(e->nrefs>=TOOIE_CKCOPY_REFS || !ckcopy_protected(copy,&n) || n!=e->count ||
       !ckcopy_owned(p,command) || *(volatile u32*)command!=0xDB060004u ||
       *(volatile u32*)(command+4u)!=ckcopy_phys(copy+24u))
        return ckcopy_revert(p,i);
    e->refs[e->nrefs++]=command;
    *(volatile u32*)(command+4u)=ckcopy_phys(ckcopy_slot(p,e->slot));
    CK.last_copy=copy;CK.last_command=command;
    return 0;
}
/* S3 (A7840 before 0x800A7890; frame = A7840's sp): only the owning frame finishes. It restores
 * every copy that is no longer live, then validates every registered label of the live copies
 * (an invalid label faults before anything is published), then publishes each live copy's final
 * bytes and promotes its labels, and closes restoration authority before the original 15410
 * wait, submission and update. */
CKCOPY u32 tooie_ckcopy_root_end(u32 frame) {
    u32 p,i,k,w,words,n=0,used,bad=0;
    volatile u32* from;volatile u32* to;
    struct TooieCkcopyEntry* e;
    if(CK.magic!=TOOIE_CKCOPY_MAGIC || CK.fault)return 1;
    if(!CK.active || frame!=CK.frame)return 0;
    CK.last_copy=0;CK.last_command=0;
    p=CK.pool;
    if(p!=(u32)D_8007BEDA)bad=ckcopy_disqualify();
    else {
        used=ckcopy_used(p);
        for(i=0;i<used;i++) {
            e=&CK.entry[p][i];
            if(e->state==TOOIE_CKCOPY_LIVE && (!ckcopy_protected(e->source,&n) || n!=e->count))
                bad|=ckcopy_revert(p,i);
        }
        if(bad)return 1;
        for(i=0;i<used;i++) {
            e=&CK.entry[p][i];
            if(e->state==TOOIE_CKCOPY_LIVE && !ckcopy_labels_valid(p,e)){CK.fault=1;return 1;}
        }
        for(i=0;i<used;i++) {
            e=&CK.entry[p][i];
            if(e->state!=TOOIE_CKCOPY_LIVE)continue;
            from=(volatile u32*)(e->source+24u);to=(volatile u32*)ckcopy_slot(p,e->slot);words=4u*e->count;
            for(w=0;w<words;w++)to[w]=from[w];
            for(k=0;k<e->nlabels;k++) {
                to=(volatile u32*)e->labels[k][0];
                to[1]=e->labels[k][1];to[2]=TOOIE_CKCOPY_LABEL_POLICY;
            }
            CK.promoted+=e->nlabels;
        }
    }
    CK.active=0;
    return bad;
}
/* S4 (801009C4 before 0x801009E0): a0 is the copy-manager record whose two vertex copies are
 * about to be freed. Only inside the open construction: restore every bound reference to them
 * and keep both addresses original for the rest of the root. Outside it nothing is touched. */
CKCOPY u32 tooie_ckcopy_release(u32 record) {
    u32 p=CK.pool,i,a,b,used,bad=0;
    struct TooieCkcopyEntry* e;
    if(CK.magic!=TOOIE_CKCOPY_MAGIC || CK.fault)return 1;
    if(!CK.active || p>1u || !silo_range(record,8u,3u))return 0;
    a=*(volatile u32*)record;b=*(volatile u32*)(record+4u);
    used=ckcopy_used(p);
    for(i=0;i<used;i++) {
        e=&CK.entry[p][i];
        if(e->state==TOOIE_CKCOPY_LIVE && ((a && e->source==a) || (b && e->source==b))) {
            CK.released++;bad|=ckcopy_revert(p,i);
        }
    }
    if(a)ckcopy_hold(p,a);
    if(b)ckcopy_hold(p,b);
    return bad;
}
