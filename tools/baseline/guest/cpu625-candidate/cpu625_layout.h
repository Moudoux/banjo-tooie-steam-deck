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
u32 tooie_cpu625_admit(u32 buffers,u32 cursor_slot,u32 model);
u32 tooie_cpu625_end(u32 owner);
u32 tooie_cpu625_retire(u32 owner,u32 requested,u32 reason);
#endif
