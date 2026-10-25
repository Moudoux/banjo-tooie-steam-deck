/* BK-recomp owner IDs, generic model matrix groups and the DE498 root envelope.
 * Adapted from BanjoRecomp dbd52154bc1e00e56867b1fac94a8f907052f41b patches/transform_tagging.c
 * (set_model_matrix_group 198-240, modelRender_draw 891-934) and
 * patches/marker_transform_tagging.c (__marker_draw 40-84, player_draw 20-37).
 * Tooie differences are marked @tooie; ORDER_AND_BINDINGS.md section 3 justifies each. */
#define F3DEX_GBI_2
#include "silo_bridge.h"
#include "rt64_extended_gbi.h"
#include "bk_pipeline.h"
#define BK_HELPER __attribute__((section(".text.player_boundary"), noinline))
#define BK tooie_bk_state
_Static_assert(sizeof(void*) == 4 && sizeof(Gfx) == 8 && sizeof(GfxCommand) == 8 &&
    sizeof(SiloBuffers) == 8 && __builtin_offsetof(SiloBuffers, mtx) == 4, "BK guest command ABI");

volatile struct TooieBkState tooie_bk_state __attribute__((section(".tooie_bk_state"), aligned(4))) = {
    .magic = TOOIE_BK_MAGIC, .layout = TOOIE_BK_LAYOUT,
};

SI void bk_count(volatile u32* value) { if (*value != 0xFFFFFFFFu) *value += 1u; }
/* B3 producer state and always-inline functions. */
#include "bk_float.inc"
/* Existing WORLD camera-cut adapter (always-inline). */
#include "bk_cut.inc"
/* Stable type-12 selector numbering for PLAYER roots (always-inline; its hooks are in bk_graph.inc). */
#include "bk_select.inc"
/* Shadow producer route, discontinuity and light-identity state (bk_pipeline.h layout2); its
 * helpers are the last ones defined (bk_shadow_attach.inc). */
volatile struct TooieBkShadow tooie_bk_shadow __attribute__((section(".tooie_bk_shadow"), aligned(4))) = {
    .magic = TOOIE_BK_SHADOW_MAGIC, .layout = TOOIE_BK_SHADOW_LAYOUT,
};
#define BKS tooie_bk_shadow

/* Root frame modes (TooieBkRootFrame.pushed_group). */
#define TOOIE_BK_ROOT_NO_GROUP 0u
#define TOOIE_BK_ROOT_GROUP 1u
#define TOOIE_BK_ROOT_BIT40 2u

/* ---- BK set_model_matrix_group / pop_model_matrix_group (non-map branch) ------------- */
BK_HELPER u32 tooie_bk_set_model_matrix_group(Gfx** gfx, u32 node, u32 skip_rotation) {
    GfxCommand* cursor;
    u32 group_id, offset;
    (void)node;
    if (BK.cur_drawn_model_transform_id != 0) {
        offset = BK.cur_model_transform_id_offset;
        /* @tooie BK lets the offset run on into the next owner's IDs; Tooie packs view x phase
         * samples adjacently, so an offset past its 256-ID sample opens no group. */
        if (offset >= TOOIE_BK_TRANSFORM_ID_COUNT) {
            bk_count(&BK.diagnostics.offset_overflow);
            return 0;
        }
        group_id = BK.cur_drawn_model_transform_id + offset;
        cursor = (GfxCommand*)*gfx;
        if (BK.skip_all_interpolation || BK.cur_drawn_model_skip_interpolation) {
            /* BK: skip interpolation if all interpolation is skipped or the transform says so. */
            gEXMatrixGroup(cursor, group_id, G_EX_INTERPOLATE_SIMPLE, G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP,
                G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR,
                G_EX_EDIT_NONE, G_EX_ASPECT_AUTO, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP);
        }
        else {
            /* BK: simple interpolation once bones are in use, decomposed otherwise. Non-map
             * models keep vertex and texcoord interpolation SKIP. */
            gEXMatrixGroup(cursor, group_id,
                BK.cur_model_uses_bones ? G_EX_INTERPOLATE_SIMPLE : G_EX_INTERPOLATE_DECOMPOSE,
                G_EX_PUSH, G_MTX_MODELVIEW, G_EX_COMPONENT_INTERPOLATE,
                skip_rotation ? G_EX_COMPONENT_SKIP : G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_SKIP, G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE,
                G_EX_ASPECT_AUTO, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_AUTO);
        }
        *gfx = (Gfx*)(cursor + 2);
        bk_count(&BK.diagnostics.groups);
        return 1;
    }
    else if (BK.skip_all_interpolation) {
        cursor = (GfxCommand*)*gfx;
        gEXMatrixGroupNoInterpolate(cursor, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);
        *gfx = (Gfx*)(cursor + 2);
        return 1;
    }
    return 0;
}

BK_HELPER void tooie_bk_pop_model_matrix_group(Gfx** gfx) {
    GfxCommand* cursor = (GfxCommand*)*gfx;
    gEXPopMatrixGroup(cursor, G_MTX_MODELVIEW);
    *gfx = (Gfx*)(cursor + 1);
}

/* ---- Frame (BK game_draw prologue) ----------------------------------------------------- */
BK_HELPER void tooie_bk_frame_begin(void) {
    u32 i;
    bk_count(&BK.diagnostics.frames);
    BK.cur_drawn_model_transform_id = 0;
    BK.cur_drawn_model_skip_interpolation = 0;
    BK.cur_model_transform_id_offset = 0;
    BK.cur_model_uses_bones = 0;
    BK.root_depth = 0;
    BK.scope_depth = 0;
    BK.draw_depth = 0;
    BK.handler_depth = 0;
    BK.select_roots = 0;
    /* No player body has decided its visible-branch continuity in this construction (bk_select.inc). */
    for (i = 0; i < TOOIE_BK_BRANCH_LATCHES; i++) {
        BK.branch_latches[i].owner = 0;
        BK.branch_latches[i].snap = 0;
    }
    /* No projected-shadow body has decided, and no shadow scope is open, in this construction either. */
    BK.shadow_body_id = 0;
    BK.shadow_body_snap = 0;
    BK.shadow_projected = 0;
    BK.owner.kind = TOOIE_BK_OWNER_NONE;
    BK.owner.own_draw_pending = 0;
    /* No player body draw has run yet this frame; no attachment scope is open. */
    BK.reserved0 = 0;
    BK.handler_reserved = 0;
    /* Shadow producer: no projected route, tagged body or open projection carries over. Light
     * route slots and allocation serials persist across frames (bk_shadow_attach.inc). */
    BKS.route = 0;
    BKS.light = 0;
    BKS.body_frame = 0;
    BKS.projection = 0;
    BKS.projection_open = 0;
    /* BK game_draw 169 clears skip_all after each frame; each root sets its view's cut below. */
    BK.skip_all_interpolation = 0;
    /* BK game_draw: recomp_reset_skinning_stack(). */
    bk_skin_frame_begin();
}

/* ---- Root envelope (BK modelRender_draw 889-934) ---------------------------------------
 * DF0B0: after the original root LOAD/PUSH (DF080-DF0AC), before the transform-context
 * setup, appearance initializer and graph. The live Gfx cursor is DE498's local word at
 * sp+0xCC; b->gfx is not published until DF128.
 * Header bit 0x40 reaches DF0B0 through its own projection/palette path, skips DF234 and
 * rejoins at DF238 (tooie_bk_root_join). It is the same shared draw: the same view cut, the same
 * float rerun of this invocation's executed deform (execution-facts53 3) and the same traversal
 * reset. Only its target differs: the original issues a projection MUL/NOPUSH of the computed
 * object/surface matrix and no modelview LOAD/PUSH, so no modelview root group is pushed; its
 * projection policy was selected at DEFB0 (shadow_projection.c / palette), its vertices get
 * their groups from the generic list node's identity operation (bk_graph.inc), and E4640 at
 * DF208 restores the saved camera projection. */
BK_HELPER void tooie_bk_root_begin(u32 buffers, u32 cursor_slot, u32 model) {
    volatile struct TooieBkRootFrame* frame;
    GfxCommand* cursor;
    u32 id, bit40;
    (void)buffers;
    bk_count(&BK.diagnostics.roots);
    bit40 = silo_range(model, 0x2Cu, 3u) && (*(volatile u16*)(model + 0xAu) & 0x40u);
    if (BK.root_depth >= TOOIE_BK_ROOT_DEPTH) {
        /* First overflow root suspends the enclosing float stream without using another
         * frame slot. Deeper overflow roots inherit disabled streams. Original draws remain. */
        if (BK.root_depth == TOOIE_BK_ROOT_DEPTH &&
            bk_segments_effective((s32)TOOIE_BK_ROOT_DEPTH - 1)) {
            cursor = (GfxCommand*)*(volatile u32*)cursor_slot;
            *(volatile u32*)cursor_slot = (u32)bk_segments_off(cursor);
        }
        bk_count(&BK.diagnostics.root_overflow);
        BK.root_depth += 1u;
        return;
    }
    frame = &BK.roots[BK.root_depth];
    BK.root_depth += 1u;
    frame->outer_offset = BK.cur_model_transform_id_offset;
    frame->outer_uses_bones = BK.cur_model_uses_bones;
    frame->reserved = BK.skip_all_interpolation;
    cursor = (GfxCommand*)*(volatile u32*)cursor_slot;
    /* @tooie BK's skip_all_interpolation is set per camera cut; Tooie's cut is per view, so each
     * root takes its own view's cut and restores the enclosing root's context when it closes.
     * The same decision drives SkipAll groups and suppresses explicit velocity below. */
    BK.skip_all_interpolation = bk_world_cut();
    id = BK.cur_drawn_model_transform_id;
    /* @tooie This PLAYER root's type-12 numbering, then its visible-branch continuity (bk_select.inc),
     * both decided before the float rerun and the first group, so every group and the float velocity
     * of this draw follow one policy. root_end restores the incoming skip_all (frame->reserved). */
    bk_select_root(BK.root_depth - 1u, id, model);
    if (bk_branch_root(BK.root_depth - 1u, id, model)) BK.skip_all_interpolation = 1u;
    /* BK modelRender_draw 844-867: float rerun and vertex segments for this draw. */
    cursor = bk_skin_root_begin(BK.root_depth - 1u, cursor_slot, model, cursor);
    if (bit40) {
        /* @tooie No modelview root group: the original emits no modelview matrix here. */
        frame->pushed_group = TOOIE_BK_ROOT_BIT40;
    }
    else {
        frame->pushed_group = TOOIE_BK_ROOT_NO_GROUP;
        /* BK: Create a matrix group if a transform id is set. */
        if (BK.skip_all_interpolation || BK.cur_drawn_model_skip_interpolation) {
            gEXMatrixGroup(cursor, id, G_EX_INTERPOLATE_SIMPLE, G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP,
                G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR,
                G_EX_EDIT_NONE, G_EX_ASPECT_AUTO, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP);
            frame->pushed_group = TOOIE_BK_ROOT_GROUP;
        }
        else if (id != 0) {
            /* BK gEXMatrixGroupDecomposedNormalTcs: decomposed; vertices SKIP; texcoords INTERPOLATE.
             * @tooie A66BC: the shadow attachment's own root carries A6250's singular flattening projector
             * (DF540 mode3) through MODELVIEW, so it takes coefficient interpolation, as SHADOW_POLICY gives
             * its bit40 body (shadow_projection.c). Every other component is unchanged. */
            u32 mode = (BK.owner.kind == TOOIE_BK_OWNER_SHADOW && BK.owner.attachments == 1u &&
                BK.draw_depth == 1u && BK.root_depth == 1u) ? G_EX_INTERPOLATE_SIMPLE : G_EX_INTERPOLATE_DECOMPOSE;
            gEXMatrixGroup(cursor, id, mode, G_EX_PUSH, G_MTX_MODELVIEW,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP,
                G_EX_COMPONENT_INTERPOLATE, G_EX_ORDER_LINEAR, G_EX_EDIT_NONE, G_EX_ASPECT_AUTO,
                G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_AUTO);
            frame->pushed_group = TOOIE_BK_ROOT_GROUP;
        }
    }
    if (frame->pushed_group == TOOIE_BK_ROOT_GROUP) {
        cursor += 2;
        bk_count(&BK.diagnostics.root_groups);
    }
    *(volatile u32*)cursor_slot = (u32)cursor;
    /* BK: Reset the transform ID offset and the bones flag. */
    BK.cur_model_transform_id_offset = 0;
    BK.cur_model_uses_bones = 0;
}

/* DF234 (normal) / DF238 via tooie_bk_root_join (bit40): after the original root pop, before
 * the reset list (DF258), appearance finalizer (DF2A8), F270 cleanup (DF2CC) and post callback
 * (DF2E8). The live cursor is the active b->gfx. BK pops here: graph, matrix pop, group pop,
 * post_method. */
BK_HELPER void tooie_bk_root_end(u32 buffers) {
    volatile struct TooieBkRootFrame* frame;
    SiloBuffers* b = (SiloBuffers*)buffers;
    GfxCommand* cursor;
    if (!BK.root_depth) return;
    BK.root_depth -= 1u;
    if (BK.root_depth >= TOOIE_BK_ROOT_DEPTH) {
        /* Only the outermost overflow exit returns to the saved bounded segment state. */
        if (BK.root_depth == TOOIE_BK_ROOT_DEPTH) {
            volatile struct TooieBkSegment* outer = bk_segments_effective((s32)TOOIE_BK_ROOT_DEPTH - 1);
            if (outer) b->gfx = (Gfx*)bk_segments_on((GfxCommand*)b->gfx, outer);
        }
        return;
    }
    BK.select_roots &= ~(1u << BK.root_depth);   /* this root's selector decision ends with it */
    frame = &BK.roots[BK.root_depth];
    if (frame->pushed_group == TOOIE_BK_ROOT_GROUP) {
        cursor = (GfxCommand*)b->gfx;
        gEXPopMatrixGroup(cursor, G_MTX_MODELVIEW);
        b->gfx = (Gfx*)(cursor + 1);
    }
    /* BK: Clear use of high precision vertex buffers (restoring an enclosing root's). */
    b->gfx = (Gfx*)bk_skin_root_end(BK.root_depth, (GfxCommand*)b->gfx);
    /* @tooie Nested DE498 roots restore the outer traversal and skip context (BK keeps locals). */
    BK.cur_model_transform_id_offset = frame->outer_offset;
    BK.cur_model_uses_bones = frame->outer_uses_bones;
    BK.skip_all_interpolation = frame->reserved;
}

/* ---- Owner identity -------------------------------------------------------------------
 * BK: MARKER_TRANSFORM_ID_START + spawn_index * 256, spawn_index being the runtime creation
 * counter. Tooie counterpart: the EBED4 birth serial (render_bridge.c), a global creation
 * counter stored per descriptor slot at creation. @tooie IDs reuse after 37449 generations. */
SI u32 bk_actor_id(u32 birth, u32 attachment, u32 view, u32 phase) {
    if (!birth || attachment >= TOOIE_BK_ACTOR_ATTACHMENTS ||
        view >= TOOIE_BK_VIEW_COUNT || phase >= TOOIE_BK_PHASE_COUNT) return 0;
    return TOOIE_BK_ACTOR_ID_START + ((birth - 1u) % TOOIE_BK_ACTOR_GENERATIONS) * TOOIE_BK_ACTOR_OWNER_STRIDE +
        attachment * TOOIE_BK_OWNER_STRIDE + (view * TOOIE_BK_PHASE_COUNT + phase) * TOOIE_BK_TRANSFORM_ID_COUNT;
}
/* @tooie BK player_draw tags its one player with BANJO_TRANSFORM_ID_START. The drawn player's identity is
 * its roster slot (bk_player_slot); slot 0 keeps BK's single player range, slots 1..7 their own. */
SI u32 bk_player_id(u32 slot, u32 attachment, u32 view, u32 phase) {
    if (slot >= TOOIE_BK_PLAYER_SLOTS || attachment >= TOOIE_BK_PLAYER_ATTACHMENTS ||
        view >= TOOIE_BK_VIEW_COUNT || phase >= TOOIE_BK_PHASE_COUNT) return 0;
    return (slot ? TOOIE_BK_PLAYER_SLOT_ID_START + (slot - 1u) * TOOIE_BK_PLAYER_SLOT_STRIDE : TOOIE_BK_PLAYER_ID_START) +
        attachment * TOOIE_BK_OWNER_STRIDE + (view * TOOIE_BK_PHASE_COUNT + phase) * TOOIE_BK_TRANSFORM_ID_COUNT;
}
/* Original D_80135490[8] roster and PlayerOwner.slot (player_boundary.c: offsetof(PlayerOwner, slot) == 0x184). */
#define TOOIE_BK_PLAYER_ROSTER 0x80135490u
#define TOOIE_BK_PLAYER_SLOT_OFFSET 0x184u
/* The drawn player's identity: roster slot + 1 of a PlayerState bound in the roster at its own slot, or 0.
 * Never the controlled slot (D_801354DC), so a control swap changes no player's IDs. As with BK's one
 * constant player ID, the identity is the slot: a PlayerState later bound there takes its IDs, and the
 * B3 float record keyed by them continues only under its previous-frame/model rule (bk_float.inc). */
SI u32 bk_player_slot(u32 owner) {
    u32 slot;
    if (!silo_range(owner, TOOIE_BK_PLAYER_SLOT_OFFSET + 4u, 3u)) return 0;
    slot = *(volatile u32*)(owner + TOOIE_BK_PLAYER_SLOT_OFFSET);
    if (slot >= TOOIE_BK_PLAYER_SLOTS || *(volatile u32*)(TOOIE_BK_PLAYER_ROSTER + slot * 4u) != owner) return 0;
    return slot + 1u;
}

/* Scope stack: BK saves prev_transform_id in a C local around drawFunc. Tooie's begin/end
 * calls are separate, so the saved owner lives in a token-indexed slot. Token 0 = no scope. */
SI u32 bk_scope_open(u32 nested) {
    volatile struct TooieBkScope* scope;
    if (BK.scope_depth >= TOOIE_BK_SCOPE_DEPTH) { bk_count(&BK.diagnostics.scope_overflow); return 0; }
    scope = &BK.scopes[BK.scope_depth];
    scope->prev_transform_id = BK.cur_drawn_model_transform_id;
    scope->nested = nested;
    scope->prev.kind = BK.owner.kind; scope->prev.birth = BK.owner.birth;
    scope->prev.view = BK.owner.view; scope->prev.phase = BK.owner.phase;
    scope->prev.base_attachment = BK.owner.base_attachment; scope->prev.attachments = BK.owner.attachments;
    scope->prev.own_draw_pending = BK.owner.own_draw_pending; scope->prev.id = BK.owner.id;
    BK.scope_depth += 1u;
    return BK.scope_depth;
}

/* BK: Clear (restore) the current transform ID after drawing. */
BK_HELPER void tooie_bk_scope_end(u32 token) {
    volatile struct TooieBkScope* scope;
    u32 attachments;
    if (!token || token != BK.scope_depth) return;
    scope = &BK.scopes[token - 1u];
    attachments = BK.owner.attachments;
    BK.cur_drawn_model_transform_id = scope->prev_transform_id;
    BK.owner.kind = scope->prev.kind; BK.owner.birth = scope->prev.birth;
    BK.owner.view = scope->prev.view; BK.owner.phase = scope->prev.phase;
    BK.owner.base_attachment = scope->prev.base_attachment; BK.owner.id = scope->prev.id;
    /* An attachment scope keeps its owner's ordinal count so the next attachment advances. */
    BK.owner.attachments = scope->nested ? attachments : scope->prev.attachments;
    BK.owner.own_draw_pending = 0;
    BK.scope_depth = token - 1u;
}

/* Proposal: authenticated fxdialog3 / DF944 menu owner. The existing generic root,
 * graph and float machinery supplies its commands under the existing MAIN bulk
 * provisioning contract. There is no additional per-node menu reservation policy.
 * Native caller authenticates outer option occurrence and same gfx, and passes
 * only a component-aligned ID from the proposal's menu domain. */
BK_HELPER u32 tooie_bk_menu_model_begin(u32 id, u32 cut) {
    u32 token, previous_skip;
    if (id < 0x30000000u || id >= 0x30060500u || (id & 255u) || cut > 3u ||
        BK.cur_drawn_model_skip_interpolation > 1u) return 0;
    previous_skip = BK.cur_drawn_model_skip_interpolation;
    token = bk_scope_open(0);
    if (!token) return 0;
    BK.owner.kind = TOOIE_BK_OWNER_MENU; /* independent of actor/player/shadow ownership. */
    BK.owner.birth = 0; BK.owner.view = 0; BK.owner.phase = 0;
    BK.owner.base_attachment = 0; BK.owner.attachments = 0;
    BK.owner.own_draw_pending = cut >> 1; BK.owner.id = id;
    BK.cur_drawn_model_transform_id = id;
    BK.cur_drawn_model_skip_interpolation = cut & 1u;
    return token | (previous_skip << 8);
}
BK_HELPER void tooie_bk_menu_model_end(u32 token) {
    u32 depth = token & 255u, previous_pending;
    if (!depth || depth != BK.scope_depth) return;
    previous_pending = BK.scopes[depth - 1u].prev.own_draw_pending;
    tooie_bk_scope_end(depth);
    BK.owner.own_draw_pending = previous_pending;
    BK.cur_drawn_model_skip_interpolation = (token >> 8) & 1u;
}
/* The validated creation generation of a live descriptor actor, or 0. Shared by the marker
 * owner below and the shadow receiver (bk_shadow_attach.inc). */
SI u32 bk_actor_birth(u32 actor) {
    u32 descriptor, offset, slot, birth;
    if (!silo_range(actor, 4u, 3u)) return 0;
    descriptor = *(volatile u32*)actor;
    offset = descriptor - (u32)D_80132ED0;
    if (descriptor < (u32)D_80132ED0 || offset >= 200u * 0x30u || offset % 0x30u) return 0;
    slot = offset / 0x30u;
    birth = SS.births[slot];
    /* Creation generation: BK bkrecomp_get_marker_spawn_index(marker). */
    if (!birth || birth > TOOIE_BRIDGE_MAX_BIRTH ||
        !(*(volatile u8*)&D_80135450[slot >> 3] & (0x80u >> (slot & 7u)))) return 0;
    return birth;
}

SI u32 bk_owner_actor(u32 actor, u32 own_draw_pending) {
    u32 birth, view, phase, id, token;
    if (!(birth = bk_actor_birth(actor))) return 0;
    view = (u32)func_800A89F8(); phase = (u32)func_800EB854();
    id = bk_actor_id(birth, 0, view, phase);
    if (!id || !(token = bk_scope_open(0))) return 0;
    BK.owner.kind = TOOIE_BK_OWNER_ACTOR; BK.owner.birth = birth;
    BK.owner.view = view; BK.owner.phase = phase; BK.owner.base_attachment = 0;
    BK.owner.attachments = 0; BK.owner.own_draw_pending = own_draw_pending; BK.owner.id = id;
    BK.cur_drawn_model_transform_id = id;
    return token;
}

/* 80101808 after its original argument resolution: BK __marker_draw sets the drawn marker's
 * transform ID before drawFunc and restores it after. */
BK_HELPER u32 tooie_bk_actor_draw_begin(u32 actor, u32 model) {
    (void)model;
    return bk_owner_actor(actor, 1);
}

/* DE448 entry: owner selection for draws that do not pass through 80101808. */
BK_HELPER u32 tooie_bk_de448_begin(u32 position, u32 model) {
    u32 depth, view, phase, id, token, ordinal;
    (void)model;
    depth = BK.draw_depth != 0xFFFFFFFFu ? ++BK.draw_depth : BK.draw_depth;
    if (depth && depth <= TOOIE_BK_DRAW_DEPTH) BK.draw_roots[depth - 1u] = BK.root_depth;
    if (BK.owner.own_draw_pending) {
        /* The 80101808 owner's own DE448 draw. */
        BK.owner.own_draw_pending = 0;
        return 0;
    }
    if (BK.owner.kind != TOOIE_BK_OWNER_NONE && depth > 1u) {
        /* @tooie A DE448 nested inside an owner's draw gets the next ordinal under the same
         * owner and sample. BK draws attachments within one model. */
        if (!(token = bk_scope_open(1))) return 0;
        ordinal = ++BK.owner.attachments;
        id = BK.owner.kind == TOOIE_BK_OWNER_PLAYER ?
            bk_player_id(BK.owner.birth - 1u, BK.owner.base_attachment + ordinal, BK.owner.view, BK.owner.phase) :
            (ordinal < TOOIE_BK_ACTOR_ATTACHMENTS ? bk_actor_id(BK.owner.birth, ordinal, BK.owner.view, BK.owner.phase) : 0);
        if (BK.owner.kind == TOOIE_BK_OWNER_PLAYER && ordinal >= TOOIE_BK_PLAYER_SECONDARY) id = 0;
        BK.cur_drawn_model_transform_id = id;
        return token;
    }
    if (depth != 1u) return 0;
    if (D_8012C948[24] == 0x80092234u || D_8012C948[24] == 0x80092224u) {
        /* BK player_draw: cur_drawn_model_transform_id = BANJO_TRANSFORM_ID_START.
         * @tooie The 92224 secondary route is the player's second top-level draw. DF47C staged this
         * callback together with its PlayerState (C948[24]/[25]); DE318 clears only the callback, so
         * C948[25] is read only under a matching callback and must be bound in the roster. */
        u32 base = D_8012C948[24] == 0x80092224u ? TOOIE_BK_PLAYER_SECONDARY : 0u;
        u32 slot = bk_player_slot(D_8012C948[25]);
        BK.reserved0 = 0;
        if (slot) {
            view = (u32)func_800A89F8(); phase = (u32)func_800EB854();
            id = bk_player_id(slot - 1u, base, view, phase);
            if (!id || !(token = bk_scope_open(0))) return 0;
            BK.owner.kind = TOOIE_BK_OWNER_PLAYER; BK.owner.birth = slot;
            BK.owner.view = view; BK.owner.phase = phase; BK.owner.base_attachment = base;
            /* Remember this body draw's route and roster identity for its sequential backpack. */
            BK.reserved0 = 0x100u | base | (slot << 16);
            BK.owner.attachments = 0; BK.owner.own_draw_pending = 0; BK.owner.id = id;
            BK.cur_drawn_model_transform_id = id;
            return token;
        }
        /* No bound roster owner: no player identity (never a shared slot-less range); unowned rule below. */
        bk_count(&BK.diagnostics.player_untagged);
    }
    if (D_8012C948[24] == 0x801017D0u && position == D_8012C948[25] + 4u) {
        /* Direct-world actor draw: DF47C staged the actor in C948[25]; DE448 is its own draw. */
        return bk_owner_actor(D_8012C948[25], 0);
    }
    if (BK.cur_drawn_model_transform_id != 0 && BK.owner.kind == TOOIE_BK_OWNER_NONE) {
        /* An unowned top-level draw never inherits a stale ID. */
        if (!(token = bk_scope_open(0))) return 0;
        BK.cur_drawn_model_transform_id = 0;
        return token;
    }
    return 0;
}

/* DE448 after the original DE498 returns. */
BK_HELPER void tooie_bk_de448_end(u32 token, u32 buffers) {
    u32 depth = BK.draw_depth;
    /* @tooie A root still open here bypassed DF234/DF238: a seam defect, counted and closed
     * late. Any nonzero late_closes fails the batch; it is never an accepted path. */
    if (depth && depth <= TOOIE_BK_DRAW_DEPTH) {
        while (BK.root_depth > BK.draw_roots[depth - 1u]) {
            bk_count(&BK.diagnostics.late_closes);
            tooie_bk_root_end(buffers);
        }
    }
    tooie_bk_scope_end(token);
    if (depth && depth != 0xFFFFFFFFu) BK.draw_depth = depth - 1u;
}
/* Graph helpers: type3/leaf list node, billboards, B3 argument capture, bit40 root join. */
#include "bk_graph.inc"
/* Stable-order sorted-branch traversal. */
#include "bk_sort.inc"
/* Sequential player attachment owner scope (func_80092444 92710/92718). */
#include "bk_attach.inc"
/* Shadow producer owner scope: blob A5EA8, projected body A6664 and attachment A66BC for the
 * fallback and local-light routes (A6CF0/A6D60), light lifecycle events. */
#include "bk_shadow_attach.inc"
/* Split players: slots 1..7 follow the shadow domain, so player, shadow and actor IDs stay disjoint. */
_Static_assert(TOOIE_BK_PLAYER_SLOT_ID_START == TOOIE_BK_SHADOW_ID_END, "player slots 1-7 follow the shadow domain");

