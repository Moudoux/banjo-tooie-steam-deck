#ifndef TOOIE_CKCOPY_LAYOUT_H
#define TOOIE_CKCOPY_LAYOUT_H
/* P04b. Separate guest section appended after the selected image; no accepted state overlay.
 * Layout 2 (P04a): the most recent S2 redirect and per-copy label registrations for S3. */
#define TOOIE_CKCOPY_MAGIC 0x434B4350u /* CKCP */
#define TOOIE_CKCOPY_LAYOUT 2u
#define TOOIE_CKCOPY_ENTRIES 64u     /* per pool: one disposition per copy address per root */
#define TOOIE_CKCOPY_SLOTS 16u       /* per pool: 8 roster owners x 2 current copies */
#define TOOIE_CKCOPY_REFS 8u         /* tracking capacity per copy; a ninth reverts the copy */
#define TOOIE_CKCOPY_LABELS 4u       /* registered labels per copy; a fifth stays a placeholder */
#define TOOIE_CKCOPY_VERTICES 216u   /* largest two-copy ckpack row (0625) */
#define TOOIE_CKCOPY_LIVE 1u
#define TOOIE_CKCOPY_HELD 2u
/* P04a label contract; cpu625.c asserts each value against its own source definition. A
 * placeholder is the public gEXMatrixGroupNoInterpolate(PUSH,MODELVIEW,EDIT_NONE) with
 * G_EX_ID_IGNORE; S3 promotes it to its registered CPU625 ID and the CPU625 root policy. */
#define TOOIE_CKCOPY_PLACEHOLDER_POLICY 0x00000001u
#define TOOIE_CKCOPY_LABEL_POLICY 0x02014AA9u
#define TOOIE_CKCOPY_LABEL_ID_FIRST 0xB1000000u
#define TOOIE_CKCOPY_LABEL_ID_END 0xB1100000u
struct TooieCkcopyEntry {
    u32 source,count,state,slot,nrefs;
    u32 refs[TOOIE_CKCOPY_REFS];
    u32 nlabels;
    u32 labels[TOOIE_CKCOPY_LABELS][2]; /* {group command address, real CPU625 ID} */
    u32 reserved[2];
};
/* frame: the owning A7840 stack frame of the open construction; fault: persistent containment
 * latch (an unrestorable redirected word or an invalid registered label); nested: diagnostic
 * count of nested begins. last_copy/last_command: the most recent S2 redirect, consumed by the
 * next CPU625 DEFA0 admission. promoted: labels promoted at S3; placeholders: placeholders left
 * unregistered for lack of label room. */
struct TooieCkcopyState {
    u32 magic,layout,active,pool,frame,disqualified;
    u32 exhausted,released,reverted,overflowed,fault,nested;
    u32 used[2],slots[2];
    u32 last_copy,last_command,promoted,placeholders;
    struct TooieCkcopyEntry entry[2][TOOIE_CKCOPY_ENTRIES];
    u8 slot[2][TOOIE_CKCOPY_SLOTS][16u*TOOIE_CKCOPY_VERTICES];
};
_Static_assert(sizeof(struct TooieCkcopyEntry)==96 &&
    __builtin_offsetof(struct TooieCkcopyEntry,refs)==20 &&
    __builtin_offsetof(struct TooieCkcopyEntry,nlabels)==52 &&
    __builtin_offsetof(struct TooieCkcopyEntry,labels)==56,"ckcopy entry ABI");
_Static_assert(__builtin_offsetof(struct TooieCkcopyState,frame)==16 &&
    __builtin_offsetof(struct TooieCkcopyState,fault)==40 &&
    __builtin_offsetof(struct TooieCkcopyState,last_copy)==64 &&
    __builtin_offsetof(struct TooieCkcopyState,promoted)==72 &&
    __builtin_offsetof(struct TooieCkcopyState,entry)==80 &&
    __builtin_offsetof(struct TooieCkcopyState,slot)==80+2*64*96,"ckcopy state ABI");
_Static_assert(sizeof(struct TooieCkcopyState)==122960,"ckcopy separate state extent");
extern struct TooieCkcopyState tooie_ckcopy_state;
u32 tooie_ckcopy_root_begin(u32 frame);
u32 tooie_ckcopy_segment(u32 command);
u32 tooie_ckcopy_root_end(u32 frame);
u32 tooie_ckcopy_release(u32 record);
#endif
