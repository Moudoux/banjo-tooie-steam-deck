#ifndef TOOIE_BACKPACK621_OWNER_H
#define TOOIE_BACKPACK621_OWNER_H
#include "../silo-candidate/render_bridge.h"
#if !defined(TOOIE_BACKPACK621) || (TOOIE_BACKPACK621 != 0 && TOOIE_BACKPACK621 != 1)
#error Explicit boolean TOOIE_BACKPACK621 selection required
#endif
#define TOOIE_BACKPACK621_MAGIC 0x36323131u
#define TOOIE_BACKPACK621_LAYOUT 1u
#define TOOIE_BACKPACK621_STATE_BYTES 612u
#define TOOIE_BACKPACK621_ID_FIRST 0xC0000000u
#define TOOIE_BACKPACK621_ID_END 0xD0000000u
#define TOOIE_BACKPACK621_BLOCKS 0x80000u
#define TOOIE_BACKPACK621_LIFE_BLOCKS 20u
#define TOOIE_BACKPACK621_VALID 1u
struct TooieBackpack621Owner {
    TooieBridgeWord owner,backpack,model,handle,idbase,stamp,seen,flags;
    TooieBridgeWord camera_id[4];
    TooieBridgeWord copy,display_list,animation,vector_cache;
};
struct TooieBackpack621Context {
    TooieBridgeWord owner,model,view,phase,epoch,buffers,group_id,pool_index;
    TooieBridgeWord gfx_base,gfx_begin,depth,paid,slot;
};
struct TooieBackpack621Emission {
    TooieBridgeWord bones,current_role,hybrids,failed;
};
struct TooieBackpack621State {
    TooieBridgeWord magic,layout,next_block,epoch;
    struct TooieBackpack621Owner owner[8];
    struct TooieBackpack621Context context;
    struct TooieBackpack621Emission emission;
    TooieBridgeWord refusals,tripwires,drifts,last_reason;
};
typedef char b621_owner_size[(sizeof(struct TooieBackpack621Owner)==64)?1:-1];
typedef char b621_context_size[(sizeof(struct TooieBackpack621Context)==52)?1:-1];
typedef char b621_emission_size[(sizeof(struct TooieBackpack621Emission)==16)?1:-1];
typedef char b621_state_size[(sizeof(struct TooieBackpack621State)==612)?1:-1];
typedef char b621_domain_size[(0xC0000000ULL+512ULL*TOOIE_BACKPACK621_BLOCKS==0xD0000000ULL)?1:-1];
typedef char b621_model_disjoint[(0x90000000ULL+512ULL*TOOIE_BRIDGE_BLOCK_COUNT<=TOOIE_BACKPACK621_ID_FIRST)?1:-1];
typedef char b621_camera_cpu_disjoint[(0xB0000000ULL<=TOOIE_BACKPACK621_ID_FIRST && 0xB1100000ULL<=TOOIE_BACKPACK621_ID_FIRST)?1:-1];
typedef char b621_reserved_disjoint[(TOOIE_BACKPACK621_ID_FIRST>0u && TOOIE_BACKPACK621_ID_END<0xFFFFFFFFu)?1:-1];
#if TOOIE_BACKPACK621
extern volatile struct TooieBackpack621State tooie_backpack621_state;
#ifdef TOOIE_BRIDGE_GUEST
struct TooieRenderBuffers;
typedef char b621_owner_offset[(__builtin_offsetof(struct TooieBackpack621State,owner)==16)?1:-1];
typedef char b621_context_offset[(__builtin_offsetof(struct TooieBackpack621State,context)==528)?1:-1];
typedef char b621_emission_offset[(__builtin_offsetof(struct TooieBackpack621State,emission)==580)?1:-1];
typedef char b621_diagnostics_offset[(__builtin_offsetof(struct TooieBackpack621State,refusals)==596)?1:-1];
typedef char b621_private_counter_offset[(__builtin_offsetof(struct TooieBackpack621State,next_block)==8)?1:-1];
static inline __attribute__((always_inline)) u32 backpack621_paid(void) {
    return tooie_backpack621_state.context.paid;
}
u32 tooie_backpack621_begin(u32 self,u32 model);
u32 tooie_backpack621_admit(u32 buffers,u32 cursor_slot,u32 model);
u32 tooie_backpack621_end(u32 self);
u32 tooie_backpack621_retire(u32 self,u32 requested,u32 reason);
u32 tooie_backpack621_bone_push(struct TooieRenderBuffers*,s32 index);
void tooie_backpack621_bone_pop(struct TooieRenderBuffers*,u32 token);
u32 tooie_backpack621_leaf_begin(struct TooieRenderBuffers*);
void tooie_backpack621_leaf_end(struct TooieRenderBuffers*);
#endif
#endif
#endif
