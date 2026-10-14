#ifndef TOOIE_CAMERA_ACTOR_ADMISSION_H
#define TOOIE_CAMERA_ACTOR_ADMISSION_H

#include "camera_projection_internal_04.h"
#include "../silo-candidate/actor_palette.h"

#if !defined(TOOIE_ACTOR_PALETTE) || (TOOIE_ACTOR_PALETTE != 0 && TOOIE_ACTOR_PALETTE != 1)
#error Explicit boolean TOOIE_ACTOR_PALETTE selection required
#endif

#if TOOIE_ACTOR_PALETTE || TOOIE_NPC665
#if !defined(TOOIE_PRE_BACKGROUND) || (TOOIE_PRE_BACKGROUND != 0 && TOOIE_PRE_BACKGROUND != 1)
#error Explicit boolean TOOIE_PRE_BACKGROUND selection required with actor palette
#endif
_Static_assert(TOOIE_CAMERA_ID_FIRST > 0u, "actor camera ID zero remains refusal");

extern volatile u32 D_8012D500, D_8012D558;

/* Preserve the existing actor route's enclosing WORLD-camera admission. */
CAM_INLINE u32 cam_actor_camera_id(u32 cursor) {
    struct TooieCameraControl* c = &CAM_STATE.control;
    struct TooieCameraProduction* p;
    struct TooieCameraRecord* record;

#if TOOIE_PRE_BACKGROUND
    if (tooie_pre_background.pending.depth)
        return 0;
#endif

    if (D_8012D508 || c->latch || (cursor & 7u) || cursor < c->main_begin ||
        cursor > c->main_end - 96u || c->main_extra > TOOIE_CAMERA_EXTRA_BYTES - 48u)
        return 0;

    p = &CAM_STATE.saved;
    record = cam_find(p->camera);
    if (c->epoch != tooie_render_bridge_state.epoch || !p->valid || p->epoch != c->epoch ||
        p->matrix != D_8012D558 || p->camera != D_8012D500 || p->view >= 4u ||
        p->view != (u32)D_80127674 ||
        p->view != tooie_render_bridge_state.palette_context.view ||
        p->id < TOOIE_CAMERA_ID_FIRST || p->id >= TOOIE_CAMERA_ID_END ||
        p->policy != CAM_POLICY_WORLD || p->role != TOOIE_CAMERA_WORLD_INITIAL ||
        !record || record->birth != p->birth || record->revision != p->revision ||
        record->view_id[p->view] != p->id || record->view_scale[p->view] != p->scale)
        return 0;

    return p->id;
}
#endif

#endif
