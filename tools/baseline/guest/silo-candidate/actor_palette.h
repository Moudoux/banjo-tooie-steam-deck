#ifndef TOOIE_ACTOR_PALETTE_H
#define TOOIE_ACTOR_PALETTE_H
#include "common.h"

struct TooieRenderBuffers;
u32 tooie_actor_palette_begin(struct TooieRenderBuffers* buffers,
    u32* local_gfx_cursor, u32 effective_model, u32 saved_camera_id);
#if TOOIE_ACTOR_PALETTE
u32 tooie_actor_cpu_root_begin(struct TooieRenderBuffers* buffers,
    u32* local_gfx_cursor, u32 effective_model, u32 saved_camera_id);
u32 tooie_actor_feather_leaf_id(struct TooieRenderBuffers* buffers,
    u8* node, u32 saved_camera_id);
#endif
void tooie_actor_palette_end(struct TooieRenderBuffers* buffers);

#endif
