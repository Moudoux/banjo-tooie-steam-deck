/* Exact Max hook amendment: integer-only guest-owned leaves, compile-first. */
#if TOOIE_ACTOR_PALETTE
#define F3DEX_GBI_2
#endif
#include "camera_projection_internal_04.h"
#define TOOIE_BRIDGE_GUEST 1
#include "../silo-candidate/render_bridge.h"
#include "camera_actor_admission.h"
#if TOOIE_ACTOR_PALETTE
#include "rt64_extended_gbi.h"
#endif
#if !defined(TOOIE_BK_PIPELINE) || (TOOIE_BK_PIPELINE != 0 && TOOIE_BK_PIPELINE != 1)
#error Explicit boolean TOOIE_BK_PIPELINE selection required
#endif
#if TOOIE_BK_PIPELINE
u32 tooie_bk_list_begin(Gfx**, u32, u32);
void tooie_bk_list_end(Gfx**, u32);
#endif
extern volatile s16 D_8007BEDA;
extern volatile u32 D_8007BCD0[2],D_80127658,D_8012D500,D_8012D558;
_Static_assert(sizeof(struct TooieCameraState)==1380,"layout2 required");
__attribute__((section(".tooie_camera_helpers"),noinline)) void tooie_camera_construction_begin(u32 owner_frame,u32 saved_root){
    struct TooieCameraControl* c=&CAM_STATE.control;u32 epoch,index,base;
    epoch=tooie_render_bridge_state.epoch;   /* frame-construction epoch; same PC/order as former native write */
    if(epoch!=0xFFFFFFFFu)tooie_render_bridge_state.epoch=epoch+1u;
    if(!owner_frame){CAM_STATE.diagnostics.refusals++;return;}
    if(CAM_STATE.owner_frame){c->latch=1;CAM_STATE.diagnostics.reentry++;return;}
    CAM_STATE.owner_frame=owner_frame;epoch=tooie_render_bridge_state.epoch;
    c->epoch=epoch;c->active=0;c->main_base=0;c->main_begin=0;c->main_end=0;
    c->main_extra=0;c->main_reserve=0;c->latch=0;c->role=0;
    CAM_STATE.saved.valid=0;CAM_STATE.fill[0].valid=0;CAM_STATE.fill[1].valid=0;
    CAM_STATE.token.paid=0;CAM_STATE.token.slot_base=0;CAM_STATE.token.expected_main=0;CAM_STATE.token.epoch=0;
    CAM_STATE.diagnostics.first_reason=0;
    if(c->backup_depth){c->latch=1;CAM_STATE.backup.valid=0;return;}
    CAM_STATE.backup.valid=0;
    if(!epoch || epoch==0xFFFFFFFFu){c->latch=1;CAM_STATE.diagnostics.refusals++;return;}
    index=(u32)(s32)D_8007BEDA;
    if(index>=2u)return;
    base=(u32)&tooie_camera_main[index][0];
    if(D_8007BCD0[index]!=base || saved_root!=base || D_80127658!=saved_root)return;
    c->main_base=base;c->main_begin=base;c->main_end=base+TOOIE_CAMERA_M_BYTES;c->active=1;
}
__attribute__((section(".tooie_camera_helpers"),noinline)) void tooie_camera_construction_end(u32 owner_frame){
    struct TooieCameraControl* c=&CAM_STATE.control;
    if(!owner_frame || !CAM_STATE.owner_frame || CAM_STATE.owner_frame!=owner_frame){
        if(CAM_STATE.owner_frame)c->latch=1;CAM_STATE.diagnostics.refusals++;return;
    }
    c->active=0;c->role=0;CAM_STATE.owner_frame=0;
    if(!CAM_STATE.token.paid){CAM_STATE.token.slot_base=0;CAM_STATE.token.expected_main=0;CAM_STATE.token.epoch=0;}
}
__attribute__((section(".tooie_camera_helpers"),noinline)) void tooie_camera_de498_ignore(u32 graphics_buffers,u32 cursor_slot,u32 effective_model){
    struct TooieCameraControl* c=&CAM_STATE.control;u32 cursor,enabled=0,actor_id=0;
    if(!CAM_STATE.owner_frame || !c->active)return;
    cursor=*(volatile u32*)cursor_slot;
    if(D_8012D508){u32 i;for(i=0;i<2;i++)if(CAM_STATE.fill[i].valid && CAM_STATE.fill[i].epoch==c->epoch &&
        cursor>=CAM_STATE.fill[i].base && cursor<=CAM_STATE.fill[i].end && CAM_STATE.fill[i].extra>=8u){enabled=1;break;}}
    else if(cursor>=c->main_base && cursor<=c->main_end && c->main_extra>=8u)enabled=1;
    if(!enabled){c->latch=1;CAM_STATE.diagnostics.refusals++;
        if(!CAM_STATE.diagnostics.first_reason)CAM_STATE.diagnostics.first_reason=0x102u;return;}
#if TOOIE_ACTOR_PALETTE
    u32 camera_id = cam_actor_camera_id(cursor);
    if (camera_id)
        actor_id = tooie_actor_palette_begin((struct TooieRenderBuffers*)graphics_buffers,
            (u32*)cursor_slot, effective_model, camera_id);
#endif
    cursor=*(volatile u32*)cursor_slot;
    *(volatile u32*)cursor_slot=cam_projection_policy(cursor,actor_id,actor_id?CAM_POLICY_WORLD:CAM_POLICY_IGNORE);
}

#if TOOIE_ACTOR_PALETTE
__attribute__((section(".tooie_camera_helpers"), noinline))
void tooie_camera_cpu_root_begin(u32 buffers, u32 cursor_slot, u32 model) {
    if (!CAM_STATE.owner_frame || !CAM_STATE.control.active) return;
    u32 camera_id = cam_actor_camera_id(*(volatile u32*)cursor_slot);
    if (camera_id)
        tooie_actor_cpu_root_begin((struct TooieRenderBuffers*)buffers,
            (u32*)cursor_slot, model, camera_id);
}

/* Original type3 owns one DE. Its optional public bracket is lexical. */
struct TooieRenderBuffers {
    Gfx* volatile gfx;
    Mtx* volatile mtx;
};
extern u8* volatile D_8012C960;
extern u32 D_8012CA18;
_Static_assert(sizeof(struct TooieRenderBuffers) == 8 && sizeof(Gfx) == 8 &&
    __builtin_offsetof(struct TooieRenderBuffers, mtx) == 4, "original type3 buffer ABI");

__attribute__((section(".recomp_patch"), noinline))
void func_800DD024(struct TooieRenderBuffers* buffers, u8* node) {
#if TOOIE_BK_PIPELINE
    /* @recomp BK func_80338904: identity MUL/PUSH and group before the original list call;
       original-matrix pop and group pop after it. Supersedes the feather bracket here. */
    u32 bkList = tooie_bk_list_begin((Gfx**)&buffers->gfx, (u32)node, 1u);
#else
    u32 current = (u32)buffers->gfx;
    /* @recomp Optional actor feather admission and prepaid public push around the
       original type-three leaf command below. */
    u32 id = 0;
    if (tooie_render_bridge_state.palette_context.actor && D_8012CA18 == 0x6EAu &&
        CAM_STATE.owner_frame && CAM_STATE.control.active)
        id = tooie_actor_feather_leaf_id(buffers,
            node, cam_actor_camera_id(current));
    if (id) {
        GfxCommand* group = (GfxCommand*)current;
        /* @recomp A nonzero id validated this slot and view. After a camera-ID change in
           this view and epoch, keep the same explicit ID but skip every component, so the
           camera-facing billboard is not paired across the cut. The public NoInterpolate
           macro would force ID_IGNORE instead. */
        u32 slot = tooie_render_bridge_state.palette_context.slot;
        u32 view_bit = 1u << tooie_render_bridge_state.palette_context.view;
        gEXEnable(group);
        if (tooie_render_bridge_state.palette[slot].camera_cut_views & view_bit) {
            gEXMatrixGroup(group + 1, id, G_EX_INTERPOLATE_SIMPLE,
                G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_SKIP,
                G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP,
                G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP,
                G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR,
                G_EX_EDIT_NONE, G_EX_ASPECT_AUTO, G_EX_COMPONENT_SKIP,
                G_EX_COMPONENT_SKIP);
        } else {
            gEXMatrixGroup(group + 1, id, G_EX_INTERPOLATE_SIMPLE,
                G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_SKIP, G_EX_COMPONENT_AUTO, G_EX_ORDER_LINEAR,
                G_EX_EDIT_NONE, G_EX_ASPECT_AUTO, G_EX_COMPONENT_SKIP,
                G_EX_COMPONENT_AUTO);
        }
        buffers->gfx = (Gfx*)(current + 24u);
    }
#endif

    Gfx* cursor = buffers->gfx;
    s32 index = *(volatile s16*)(node + 8u);
    u32 list_base = (u32)D_8012C960;
    buffers->gfx = cursor + 1;
    ((volatile u32*)cursor)[0] = 0xDE000000u;
    u32 list = list_base + (u32)(index * 8) + 8u;
    ((volatile u32*)cursor)[1] = osVirtualToPhysical((void*)list);

#if TOOIE_BK_PIPELINE
    tooie_bk_list_end((Gfx**)&buffers->gfx, bkList);
#else
    if (id) {
        /* @recomp The successful helper prepaid this pop; no second admission gate. */
        GfxCommand* tail = (GfxCommand*)buffers->gfx;
        gEXPopMatrixGroup(tail, G_MTX_MODELVIEW);
        buffers->gfx = (Gfx*)(tail + 1);
        if ((u32)buffers->gfx > tooie_render_bridge_state.last_end)
            tooie_render_bridge_state.last_end = (u32)buffers->gfx;
    }
#endif
}
#endif
