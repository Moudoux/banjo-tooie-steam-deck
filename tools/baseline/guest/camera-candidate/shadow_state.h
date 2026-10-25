#ifndef TOOIE_SHADOW_STATE_H
#define TOOIE_SHADOW_STATE_H
/* Moved verbatim from shadow_projection.c (N3) so the CPU625 shadow-attachment owner reads the
 * same 56-byte prefix (id +8, seen_epoch +16, group +20). A TU without TOOIE_SHADOW_COEFFICIENTS
 * sees only that prefix; the definition stays in shadow_projection.c. */
struct ShadowState {u32 magic,epoch,id,model,seen_epoch,group,composite,mode_open,fault,args[4],source_pc;
#if TOOIE_SHADOW_COEFFICIENTS && !TOOIE_BK_PIPELINE
 /* Appended after the unchanged 56-byte prefix: open push, then last emitted history. */
 u32 mv_group,mv_epoch,mv_id,mv_camera[3],mv_selectors[5];
#endif
};
#if TOOIE_SHADOW_COEFFICIENTS && !TOOIE_BK_PIPELINE
_Static_assert(sizeof(struct ShadowState)==100 &&
    __builtin_offsetof(struct ShadowState,source_pc)==52,"shadow state 56-byte prefix and tail extent");
#else
_Static_assert(sizeof(struct ShadowState)==56 &&
    __builtin_offsetof(struct ShadowState,source_pc)==52,"shadow state 56-byte prefix and tail extent");
#endif
_Static_assert(__builtin_offsetof(struct ShadowState,id)==8 && __builtin_offsetof(struct ShadowState,seen_epoch)==16 &&
    __builtin_offsetof(struct ShadowState,group)==20,"N3 reads the shadow prefix");
extern struct ShadowState shadow_pending;
#endif
