#ifndef TOOIE_CPU625_LAYOUT_H
#define TOOIE_CPU625_LAYOUT_H
/* Authored proposal. Separate appended guest section; no accepted state overlay. */
#define TOOIE_CPU625_MAGIC 0x36323531u
#define TOOIE_CPU625_LAYOUT 1u
#define TOOIE_CPU625_ID_FIRST 0xB1000000u
#define TOOIE_CPU625_BLOCKS 2048u
struct TooieCpu625Owner {
    u32 owner,backpack,model,handle,idbase,stamp,seen,flags;
    u32 camera_id[4];
};
struct TooieCpu625Context {
    u32 owner,model,view,phase,epoch,buffers,group_id,pool_index;
    u32 gfx_base,gfx_begin,depth,paid;
};
struct TooieCpu625State {
    u32 magic,layout,next_block,epoch;
    struct TooieCpu625Owner owner[8];
    struct TooieCpu625Context context;
    u32 refusals,tripwires,drifts,last_reason;
};
_Static_assert(sizeof(struct TooieCpu625Owner)==48,"625 owner ABI");
_Static_assert(sizeof(struct TooieCpu625Context)==48,"625 context ABI");
_Static_assert(sizeof(struct TooieCpu625State)==464,"625 separate state ABI");
extern struct TooieCpu625State tooie_cpu625_state;
u32 tooie_cpu625_begin(u32 owner,u32 model);
#if defined(TOOIE_CKCOPY) && TOOIE_CKCOPY
/* P04a: the native DEFA0 transport adds this DE498's own segment command (sp+0xB8). */
u32 tooie_cpu625_admit(u32 buffers,u32 cursor_slot,u32 model,u32 draw_command);
#else
u32 tooie_cpu625_admit(u32 buffers,u32 cursor_slot,u32 model);
#endif
u32 tooie_cpu625_end(u32 owner);
u32 tooie_cpu625_retire(u32 owner,u32 requested,u32 reason);
#if defined(TOOIE_CKCOPY) && TOOIE_CKCOPY
/* N3 shadow attachment (A6250 A66BC..A66C4). No ABI change: between its own brackets, which are
 * disjoint from draw3's, the attachment owns the context. context.depth holds the kind marker;
 * context.paid holds its state: 0 open, 1 paid (pop owed), 2 spent (nothing emitted),
 * 3 reserved by tooie_shadow_prepare (24 camera-ledger bytes prepaid). The attachment's seen mask
 * is owner[].flags bits 1..28 (bit index 1+view*7+phase); bit 0 keeps its renewal value. */
#define TOOIE_CPU625_ATTACH 0x41540000u
#define TOOIE_CPU625_ATTACH_RESERVED 3u
u32 tooie_cpu625_attach_begin(u32 owner,u32 model,u32 frame);
u32 tooie_cpu625_attach_end(u32 owner,u32 frame);
#endif
#endif
