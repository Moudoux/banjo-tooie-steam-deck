#ifndef TOOIE_NPC665_OWNER_H
#define TOOIE_NPC665_OWNER_H
#include "render_bridge.h"
#if !defined(TOOIE_NPC665) || (TOOIE_NPC665 != 0 && TOOIE_NPC665 != 1)
#error Explicit boolean TOOIE_NPC665 selection required
#endif
#define TOOIE_NPC665_ROUTE 0x80000003u
#define TOOIE_SKELETAL_ROUTE 0x80000004u
#define TOOIE_NPC665_ADMITTED 5u
#define TOOIE_NPC665_IDLE 0x66500000u
#define TOOIE_NPC665_PENDING 1u
#define TOOIE_NPC665_ACTIVE 2u
#define TOOIE_NPC665_FAILED 4u
#define TOOIE_NPC665_BAD_CAPTURE 8u
struct TooieRenderBuffers;
#if TOOIE_NPC665
extern volatile struct TooieNpc665Sidecar tooie_npc665_state;
/* Resources of the three offline witnesses in npc665_owner.c. This bounds capture;
 * the model witness at admission remains the authority. Keep both lists together. */
static inline __attribute__((always_inline)) int skeletal_resource(u32 resource) {
    return resource==0x707u || resource==0x671u || resource==0x90Cu;
}
static inline __attribute__((always_inline)) void npc665_reset(void) {
    tooie_npc665_state.phase.pending.actor_data=0;
    tooie_npc665_state.phase.pending.capture_count=0;
    tooie_npc665_state.phase.pending.event=0;
    tooie_npc665_state.control=TOOIE_NPC665_IDLE;
}
static inline __attribute__((always_inline)) void npc665_pending(u32 callback) {
    npc665_reset();
    if(callback==0x801017D0u && (tooie_render_bridge_state.silo_context.pair>>16)==0x665u)
        tooie_npc665_state.control|=TOOIE_NPC665_PENDING;
}
static inline __attribute__((always_inline)) void npc665_invalidated(u32 slot) {
    if(tooie_render_bridge_state.silo_context.admitted==TOOIE_NPC665_ADMITTED &&
       tooie_render_bridge_state.silo_context.slot==slot &&
       (tooie_npc665_state.control&TOOIE_NPC665_ACTIVE))
        tooie_npc665_state.control|=TOOIE_NPC665_FAILED;
}
void tooie_npc665_capture(u32 actor,u32 data,u32 event);
void tooie_npc665_retire(u32 actor,u32 descriptor,u32 pose_handle);
u32 tooie_npc665_admit(struct TooieRenderBuffers*,u32* local_cursor,u32 model);
u32 tooie_npc665_bone_push(struct TooieRenderBuffers*,s32 index);
void tooie_npc665_bone_pop(struct TooieRenderBuffers*,u32 token);
u32 tooie_npc665_leaf_begin(struct TooieRenderBuffers*);
void tooie_npc665_leaf_end(struct TooieRenderBuffers*);
u32 tooie_npc665_end(struct TooieRenderBuffers*);
/* Native absolute import: runs tooie_npc665_end on the caller's context, then stops on nonzero. */
u32 baseline_npc665_end_checked(struct TooieRenderBuffers*);
#endif
#endif
