/* Existing WORLD camera discontinuity as BK's skip_all_interpolation input.
 * Included by bk_pipeline.c after bk_float.inc; always-inline only.
 * BK sets skip_all_interpolation for a camera cut and clears it after the frame (game_draw 169);
 * every group then takes SkipAll with its unchanged ID. Tooie has no cut Boolean: the source
 * boundary (execution-facts48) is a change of the valid normal WORLD saved-production camera ID
 * for the same view. A changed ID latches a per-view, per-construction-epoch sticky cut that
 * every model, phase and attachment of that view's construction consumes, and that also
 * withholds explicit velocity (bk_apply_skinning). The first ID a view ever reports is
 * initialization, not a cut. No scale, matrix, distance or angle test. */
#define TOOIE_CAMERA_GUEST 1
#include "../camera-candidate/camera_identity.h"
extern volatile s32 D_80127674;
extern volatile u32 D_8012D500, D_8012D558;
/* Existing camera_projection_internal_04.h CAM_POLICY_WORLD word (CAM_GROUP_FLAGS layout). */
#define BK_CAM_POLICY_WORLD \
    (PARAM(G_EX_NOPUSH,1,0)|PARAM((G_MTX_PROJECTION)!=0,1,1)|PARAM(G_EX_INTERPOLATE_SIMPLE,1,2)| \
     PARAM(G_EX_COMPONENT_INTERPOLATE,2,3)|PARAM(G_EX_COMPONENT_INTERPOLATE,2,5)|PARAM(G_EX_COMPONENT_INTERPOLATE,2,7)| \
     PARAM(G_EX_COMPONENT_INTERPOLATE,2,9)|PARAM(G_EX_COMPONENT_INTERPOLATE,2,11)|PARAM(G_EX_COMPONENT_SKIP,2,13)| \
     PARAM(G_EX_COMPONENT_INTERPOLATE,2,15)|PARAM(G_EX_ORDER_LINEAR,2,17)|PARAM(G_EX_EDIT_NONE,1,19)| \
     PARAM(G_EX_ASPECT_AUTO,2,20)|PARAM(G_EX_COMPONENT_SKIP,2,22)|PARAM(G_EX_COMPONENT_AUTO,2,24))
_Static_assert(BK_CAM_POLICY_WORLD == 0x02008AAAu, "CAM_POLICY_WORLD: BK gameplay projection group");

struct TooieBkCut {
    u32 magic, layout, cuts, initializations;
    u32 last_id[TOOIE_BK_VIEW_COUNT];       /* retained WORLD production ID per view */
    u32 epoch[TOOIE_BK_VIEW_COUNT];         /* construction epoch the sticky bit belongs to */
    u32 sticky[TOOIE_BK_VIEW_COUNT];        /* cut latched for that view in that epoch */
};
_Static_assert(sizeof(struct TooieBkCut) == 64u, "BK cut adapter layout1");
volatile struct TooieBkCut tooie_bk_cut __attribute__((section(".tooie_bk_cut"), aligned(4))) = {
    .magic = 0x424B4331u, .layout = 1u,     /* 'BKC1' */
};
#define BKC tooie_bk_cut

/* The existing normal WORLD join (facts48): saved valid in the current construction epoch, for
 * the current view, matching the live matrix/camera words, WORLD policy and WORLD_INITIAL role,
 * and a live camera record with the same birth, revision and per-view ID. Returns 0 otherwise;
 * IGNORE, orthographic and PRE productions are not a cut and add no evidence. */
SI u32 bk_world_id(u32 view) {
    volatile struct TooieCameraProduction* p = &tooie_camera_state.saved;
    volatile struct TooieCameraControl* c = &tooie_camera_state.control;
    u32 i;
    if (c->epoch != SS.epoch || !p->valid || p->epoch != c->epoch || p->view != view ||
        p->matrix != D_8012D558 || p->camera != D_8012D500 ||
        p->policy != BK_CAM_POLICY_WORLD || p->role != TOOIE_CAMERA_WORLD_INITIAL ||
        p->id < TOOIE_CAMERA_ID_FIRST || p->id >= TOOIE_CAMERA_ID_END) return 0;
    for (i = 0; i < TOOIE_CAMERA_RECORDS; i++) {
        volatile struct TooieCameraRecord* r = &tooie_camera_state.camera[i];
        if (r->valid && r->pointer == p->camera)
            return r->birth == p->birth && r->revision == p->revision && r->view_id[view] == p->id ? p->id : 0;
    }
    return 0;
}

/* Cut decision for the root being opened, in the current construction view. */
SI u32 bk_world_cut(void) {
    u32 view = (u32)D_80127674, epoch = SS.epoch, id;
    if (view >= TOOIE_BK_VIEW_COUNT) return 0;
    if (BKC.epoch[view] != epoch) { BKC.epoch[view] = epoch; BKC.sticky[view] = 0; }
    id = bk_world_id(view);
    if (id) {
        if (!BKC.last_id[view]) { BKC.last_id[view] = id; bk_count(&BKC.initializations); }
        else if (BKC.last_id[view] != id) {
            /* A second production change in the same epoch keeps the view sticky. */
            BKC.last_id[view] = id;
            if (!BKC.sticky[view]) bk_count(&BKC.cuts);
            BKC.sticky[view] = 1;
        }
    }
    return BKC.sticky[view];
}
