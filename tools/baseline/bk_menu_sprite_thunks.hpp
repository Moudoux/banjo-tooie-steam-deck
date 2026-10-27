// Included at the end of bk_menu_thunks.hpp (TOOIE_BK_PIPELINE_SELECTED), after bk_menu, bk_transition and bk_ui.
/* ---- Option-box sprite component (BK projection_transform_tagging.c func_803164B0, partial) -----------
 * zoombox entry2 draws the box model (80801424 -> fxdialog3 -> DF944, owned above) and then its sprite:
 * 808014A8 calls fxdialog entry4 at 80801550 (s0 = box, a0 = gfx), which reaches core E30E0 -> E3208 ->
 * E2E48. E2E48 bypasses DE448/DE498: E305C..E308C emit its own MODELVIEW LOAD|PUSH, E3090 draws (E2180),
 * E3098..E30C0 pop, E30C4 restores the projection (E4640). Untagged, its world transform kept RT64's default
 * AUTO group, whose AUTO position test rejects a first step from rest (rt64_rigid_body.cpp updateLinear), so
 * each box's sprite snapped one slide-in step while its model interpolated (warp opening jitter).
 * BK pushes the sprite's own MODELVIEW group after the sprite matrix load and pops it after the draw. Here the
 * owner is the reserved sprite component of the same box occurrence (menu_scope.hpp), through the existing
 * menu owner scope and the billboard group helpers, under the BK MAIN bulk-provisioning contract of the model's
 * graph groups (16 + 8 bytes). Only a box occurrence of an admitted option, at top level, in the camera MAIN
 * construction and with the same gfx is taken; every other E2E48 (HUD, other sprites) stays original. */
extern "C" void tooie_bk_handler_begin(uint8_t*,recomp_context*);
extern "C" void tooie_bk_handler_end(uint8_t*,recomp_context*);
namespace bk_sprite {
struct Resident { uint32_t sp=0,gfx=0,token=0,draw_sp=0,groups=0,drew=0; };
static Resident live;
static gpr state() { return gpr(int32_t(TOOIE_BK_STATE)); }
}
// gczoombox 808014A8 before its fxdialog entry4 JAL at 80801550 (s0 = box, a0 = gfx; the delay slot writes only sp+18).
extern "C" void baseline_bk_menu_box_sprite(uint8_t* rdram,recomp_context* ctx) {
    auto& S=bk_sprite::live;
    if(S.token)return;   // no nested sprite scope in the original; never replace an open one
    S=bk_sprite::Resident{};
    const auto& m=bk_menu::live;
    if(!m.box || m.box!=uint32_t(ctx->r16) || !m.state.active.valid || m.state.active.gfx!=uint32_t(ctx->r4) ||
       !bk_transition::camera_main(rdram) ||
       bk_ui::bk_word(rdram,offsetof(TooieBkState,owner)+offsetof(TooieBkOwner,kind))!=TOOIE_BK_OWNER_NONE ||
       bk_ui::bk_word(rdram,offsetof(TooieBkState,draw_depth)) || bk_ui::bk_word(rdram,offsetof(TooieBkState,root_depth)))return;
    const uint32_t id=tooie_menu::identity(m.state,uint32_t(ctx->r4),tooie_menu::sprite);
    if(!id)return;
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=id;clone.r5=m.state.active.cut;   // the box's own freshness cut, as for its model component
    tooie_bk_menu_model_begin(rdram,&clone);
    S.token=uint32_t(clone.r2);
    if(S.token){S.sp=uint32_t(ctx->r29);S.gfx=uint32_t(ctx->r4);}
}
// E2E48 before its E2180 JAL at E3090: the original MODELVIEW LOAD|PUSH is emitted; open the sprite group at the
// live cursor of s1 (gfx). The handler key is this E2E48 activation's own sp.
extern "C" void baseline_bk_menu_sprite_draw(uint8_t* rdram,recomp_context* ctx) {
    auto& S=bk_sprite::live;
    if(!S.token || S.draw_sp || uint32_t(ctx->r17)!=S.gfx)return;
    S.groups=uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET+12,bk_sprite::state()));
    // L14-VR01b: node 0 (no graph node) makes the guest group exactly the sprite component ID; the guest keeps the
    // traversal offset. Native code no longer writes BK state here.
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r17;clone.r5=0;clone.r6=ctx->r29;
    tooie_bk_handler_begin(rdram,&clone);
    S.draw_sp=uint32_t(ctx->r29);S.drew=1;
}
// E2E48 before its E4640 JAL at E30C4: the original sprite draw and MODELVIEW pop are complete; close the group.
extern "C" void baseline_bk_menu_sprite_drawn(uint8_t* rdram,recomp_context* ctx) {
    auto& S=bk_sprite::live;
    if(!S.draw_sp || S.draw_sp!=uint32_t(ctx->r29) || uint32_t(ctx->r17)!=S.gfx)return;
    recomp_context clone;camera_clone_context(clone,*ctx);
    clone.r4=ctx->r17;clone.r5=ctx->r29;
    tooie_bk_handler_end(rdram,&clone);   // pops only the entry this activation opened
    S.draw_sp=0;
}
// 80801558, entry4's return continuation: close the owner scope; a group still open (E30C4 not reached) is
// closed first so no ID can outlive its draw.
extern "C" void baseline_bk_menu_box_sprite_end(uint8_t* rdram,recomp_context* ctx) {
    auto& S=bk_sprite::live;
    if(!S.token || S.sp!=uint32_t(ctx->r29))return;
    if(S.draw_sp){
        recomp_context clone;camera_clone_context(clone,*ctx);
        clone.r4=gpr(int32_t(S.gfx));clone.r5=gpr(int32_t(S.draw_sp));
        tooie_bk_handler_end(rdram,&clone);
    }
    recomp_context clone;camera_clone_context(clone,*ctx);clone.r4=S.token;
    tooie_bk_menu_model_end(rdram,&clone);
    // Only an actually emitted group commits the box's history (as the model's graph-group delta does).
    if(S.drew && uint32_t(MEM_W(TOOIE_BK_DIAGNOSTICS_OFFSET+12,bk_sprite::state()))!=S.groups)
        tooie_menu::emitted(bk_menu::live.state);
    S=bk_sprite::Resident{};
}
