/* Shadow producer owner scope, included last by bk_pipeline.c (after bk_attach.inc).
 * BK counterpart: patches/marker_transform_tagging.c __marker_draw 40-84 / player_draw 20-37 give
 * every draw of an owner one stable ID and skinning record before the whole draw and restore the
 * prior owner afterward; specific_actor_patches.c func_802CA7BC draws BK's player shadow as an
 * ordinary model through modelRender_draw. Tooie's player-shadow actor callback A6DAC
 * (execution-facts53 2-3) reaches three kinds of shared DE448/DE498 draws:
 *   blob       A5DB0: DE448 at A5EA8, return A5EB0 (receiver s0)
 *   body       A6250: DE448 at A6664, return A666C (receiver sp+0x250)
 *   attachment A6250: DE448 at A66BC, return/skip join A66C4
 * A6250 runs from A69F0, called at A6D60 (fallback, no light) or at A6CF0 once per accepted
 * local light (at most two), whose one-based light handle is A6B24 s0 (execution-facts54).
 * Every one of those draws gets this owner scope; the draw itself then goes through the shared
 * BK root/float/graph path like any other DE498 (bk_pipeline.c root, bit40 included).
 *
 * Sample identity: owner slot (u16(receiver+0x76) >> 7) x route x ordinal x view x phase.
 *   routes:   fallback, blob, and 64 light route slots (below)
 *   ordinals: body/blob modelview, attachment modelview, and the projection-group IDs of each
 *             (DEFB0, bit40 roots only)
 * Discontinuities use BK's own per-draw skip (cur_drawn_model_skip_interpolation and SkipAll
 * groups, which also withholds explicit velocity), so neither matrices nor vertices interpolate
 * across them: the receiver's validated actor birth (bk_actor_birth) changed for this owner slot
 * during this frame, or the light route slot's identity changed (below).
 * @tooie A third reason, the projected body's own visible-branch change, is decided at its DEFB0
 * (tooie_bk_shadow_project, below), set like BK func_802CA7BC sets cur_drawn_model_skip_interpolation
 * before its shadow model draws and cleared after it (tooie_bk_shadow_end), and followed by the
 * attachment of the same A6250 activation. It only adds to the other two reasons and the world cut.
 * The blob route does not take it.
 *
 * Light identity (execution-facts55). The handle is any positive one-based index; the allocator has
 * no count cap. Its allocation is BK's birth: C8A98 has one return epilogue for reuse and append, and
 * before C8B78 r2 is the returned handle. Vector init, free and the loader reset (C90AC, C90D0, C90F4
 * entries) end every light lifetime. C9074 defragmentation preserves handles and is not an event.
 * A compact table maps actual handles to the 64 light route slots:
 *   - a handle keeps its slot while it holds it; a slot drawn this frame is never reassigned, so two
 *     lights never share an ID within a frame;
 *   - a new handle takes a free slot, else the least recently drawn slot not drawn this frame
 *     (eviction); that frame snaps every sample of the slot;
 *   - an allocation returning a tracked handle renews its birth serial and snaps the slot through
 *     the next frame, unless it already drew this frame: then its old route stays reserved and the
 *     new lifetime acquires a different route (or stays untagged if none is available);
 *   - a vector reset empties the table, so the next draw of any handle is a new assignment.
 * Fallback and blob routes have no light and are not affected by light events. */

#define TOOIE_BK_SHADOW_ROUTE_FALLBACK 0u
#define TOOIE_BK_SHADOW_ROUTE_BLOB 1u
#define TOOIE_BK_SHADOW_ROUTE_LIGHT 2u                    /* + light route slot */
#define TOOIE_BK_SHADOW_ROUTES (TOOIE_BK_SHADOW_ROUTE_LIGHT + TOOIE_BK_SHADOW_LIGHT_SLOTS)
#define TOOIE_BK_SHADOW_PENDING_FALLBACK 1u
#define TOOIE_BK_SHADOW_PENDING_LIGHT 2u
#define TOOIE_BK_SHADOW_ORDINALS 4u                       /* mv 0/1, projection 2/3 */
#define TOOIE_BK_SHADOW_SEAM_BODY 0u
#define TOOIE_BK_SHADOW_SEAM_ATTACHMENT 1u
#define TOOIE_BK_SHADOW_SEAM_BLOB 2u
#define TOOIE_BK_SHADOW_ID_START TOOIE_BK_PLAYER_ID_END
#define TOOIE_BK_SHADOW_ID_END (TOOIE_BK_SHADOW_ID_START + \
    TOOIE_BK_SHADOW_SLOTS * TOOIE_BK_SHADOW_ROUTES * TOOIE_BK_SHADOW_ORDINALS * TOOIE_BK_OWNER_STRIDE)
_Static_assert(TOOIE_BK_SHADOW_ID_START == 0x2001C000u && TOOIE_BK_SHADOW_ID_END == 0x20E8C000u &&
    TOOIE_BK_SHADOW_ID_END <= 0x40000000u, "Shadow domain follows the player domain, below the live product labels");

SI u32 bk_shadow_id(u32 slot, u32 route, u32 ordinal, u32 view, u32 phase) {
    if (slot >= TOOIE_BK_SHADOW_SLOTS || route >= TOOIE_BK_SHADOW_ROUTES || ordinal >= TOOIE_BK_SHADOW_ORDINALS ||
        view >= TOOIE_BK_VIEW_COUNT || phase >= TOOIE_BK_PHASE_COUNT) return 0;
    return TOOIE_BK_SHADOW_ID_START +
        ((slot * TOOIE_BK_SHADOW_ROUTES + route) * TOOIE_BK_SHADOW_ORDINALS + ordinal) * TOOIE_BK_OWNER_STRIDE +
        (view * TOOIE_BK_PHASE_COUNT + phase) * TOOIE_BK_TRANSFORM_ID_COUNT;
}

/* The light route slot drawing this actual handle now, or TOOIE_BK_SHADOW_LIGHT_SLOTS when every slot
 * is already drawn this frame (the draw then stays untagged). *skip reports a snapped identity. */
SI u32 bk_shadow_light_slot(u32 handle, u32* skip) {
    volatile struct TooieBkLightSlot* s;
    u32 i, pick = TOOIE_BK_SHADOW_LIGHT_SLOTS, oldest = 0xFFFFFFFFu;
    for (i = 0; i < TOOIE_BK_SHADOW_LIGHT_SLOTS; i++) if (BKS.lights[i].handle == handle) break;
    if (i == TOOIE_BK_SHADOW_LIGHT_SLOTS) {
        for (i = 0; i < TOOIE_BK_SHADOW_LIGHT_SLOTS; i++) {
            s = &BKS.lights[i];
            if (s->drawn_frame == BKF.frame) continue;
            if (!s->handle) { pick = i; break; }
            if (s->drawn_frame < oldest) { oldest = s->drawn_frame; pick = i; }
        }
        if (pick == TOOIE_BK_SHADOW_LIGHT_SLOTS) return pick;
        s = &BKS.lights[pick];
        if (s->handle) bk_count(&BKS.evictions);
        /* A new identity in this route slot: its allocation serial is not known here (0). */
        s->handle = handle; s->birth = 0; s->skip_until = BKF.frame;
        i = pick;
    }
    s = &BKS.lights[i];
    s->drawn_frame = BKF.frame;
    *skip = BKF.frame <= s->skip_until;
    return i;
}

/* A6CF0 (local light, handle = A6B24 s0) or A6D60 (fallback), before the A69F0 call: the route
 * of the A6250 body/attachment draws that follow. Each A6250 entry is preceded by one of these. */
BK_HELPER void tooie_bk_shadow_route(u32 local, u32 handle) {
    BKS.route = local ? TOOIE_BK_SHADOW_PENDING_LIGHT : TOOIE_BK_SHADOW_PENDING_FALLBACK;
    BKS.light = local ? handle : 0u;
}

/* Light lifetime events. handle > 0: C8A98 returned this handle (before C8B78); renew its birth
 * serial. handle 0: C90AC (vector init), C90D0 (vector free) or C90F4 (loader free+init) entry. */
BK_HELPER void tooie_bk_shadow_light_event(u32 handle) {
    u32 i;
    bk_count(&BKS.light_events);
    if (!handle) {
        /* Every light lifetime ends; drawn_frame stays so no slot is reassigned within this frame. */
        for (i = 0; i < TOOIE_BK_SHADOW_LIGHT_SLOTS; i++) { BKS.lights[i].handle = 0; BKS.lights[i].birth = 0; }
        bk_count(&BKS.resets);
        return;
    }
    BKS.serial += 1u;
    for (i = 0; i < TOOIE_BK_SHADOW_LIGHT_SLOTS; i++) {
        if (BKS.lights[i].handle != handle) continue;
        if (BKS.lights[i].drawn_frame == BKF.frame) {
            /* The old lifetime already emitted this route's IDs. Keep its drawn_frame reserved;
             * a later draw of the reborn handle must acquire another slot or stay untagged. */
            BKS.lights[i].handle = 0;
            BKS.lights[i].birth = 0;
        }
        else {
            BKS.lights[i].birth = BKS.serial;
            BKS.lights[i].skip_until = BKF.frame + 1u;
        }
    }
}

/* Before the seam's direct DE448 JAL (not its delay slot). receiver = the shadow actor, frame =
 * the producer's original sp (the lexical key of this activation). */
BK_HELPER void tooie_bk_shadow_begin(u32 receiver, u32 frame, u32 seam) {
    u32 birth, slot, route, ordinal, view, phase, id, projection, skip = 0, light;
    if (seam == TOOIE_BK_SHADOW_SEAM_BODY) {
        BKS.body_frame = 0;
        /* No branch decision of an earlier A6250 activation reaches this one or its attachment. */
        BK.shadow_body_id = 0; BK.shadow_body_snap = 0;
    }
    /* An independent top-level draw: the producer is a draw callback, not a nested model draw. */
    if (BK.draw_depth || BK.owner.kind != TOOIE_BK_OWNER_NONE) return;
    birth = bk_actor_birth(receiver);
    if (!birth || !silo_range(receiver + 0x76u, 2u, 1u)) { bk_count(&BKS.untagged); return; }
    slot = (u32)*(volatile u16*)(receiver + 0x76u) >> 7;
    if (slot >= TOOIE_BK_SHADOW_SLOTS) { bk_count(&BKS.untagged); return; }
    if (seam == TOOIE_BK_SHADOW_SEAM_BLOB) route = TOOIE_BK_SHADOW_ROUTE_BLOB;
    else if (BKS.route == TOOIE_BK_SHADOW_PENDING_FALLBACK) route = TOOIE_BK_SHADOW_ROUTE_FALLBACK;
    else if (BKS.route == TOOIE_BK_SHADOW_PENDING_LIGHT && BKS.light &&
             (light = bk_shadow_light_slot(BKS.light, &skip)) < TOOIE_BK_SHADOW_LIGHT_SLOTS)
        route = TOOIE_BK_SHADOW_ROUTE_LIGHT + light;
    else { bk_count(&BKS.untagged); return; }
    ordinal = seam == TOOIE_BK_SHADOW_SEAM_ATTACHMENT ? 1u : 0u;
    view = (u32)func_800A89F8(); phase = (u32)func_800EB854();
    id = bk_shadow_id(slot, route, ordinal, view, phase);
    projection = bk_shadow_id(slot, route, ordinal + 2u, view, phase);
    if (!id || !bk_scope_open(0)) { bk_count(&BKS.untagged); return; }
    /* A changed receiver birth skips every sample of that owner slot for the rest of this frame. */
    if (BKS.birth[slot] != birth) { BKS.birth[slot] = birth; BKS.birth_frame[slot] = BKF.frame; }
    if (BKS.birth_frame[slot] == BKF.frame) skip = 1u;
    if (skip) bk_count(&BKS.discontinuities);
    /* @tooie Visible-branch continuity (tooie_bk_shadow_project): the body decides at its own DEFB0; the
     * attachment of the same A6250 activation (same frame key and body sample) follows that decision.
     * Never the player body's latch. Only A6250 draws take the type-12 reservation (bk_select.inc). */
    if (seam == TOOIE_BK_SHADOW_SEAM_BODY) BK.shadow_body_id = id;
    else if (seam == TOOIE_BK_SHADOW_SEAM_ATTACHMENT && BKS.body_frame == frame && BK.shadow_body_id &&
             BK.shadow_body_id == bk_shadow_id(slot, route, 0u, view, phase)) skip |= BK.shadow_body_snap;
    BK.shadow_projected = seam != TOOIE_BK_SHADOW_SEAM_BLOB;
    BK.owner.kind = TOOIE_BK_OWNER_SHADOW; BK.owner.birth = 0;
    BK.owner.view = view; BK.owner.phase = phase;
    /* Only the player kind reads base_attachment (nested ordinals); for this kind it holds the
     * frame key. birth 0 gives any nested DE448 ID 0: no ordinal is borrowed from another domain.
     * attachments records this seam's ordinal (attachment 1), as the player attachment scope does. */
    BK.owner.base_attachment = frame; BK.owner.attachments = ordinal;
    /* The seam's direct DE448 is this scope's own draw. */
    BK.owner.own_draw_pending = 1; BK.owner.id = id;
    BK.cur_drawn_model_transform_id = id;
    /* This draw's skip: a discontinuity above, or this view's camera cut (the same bk_world_cut every root
     * of the draw applies as skip_all at DF0B0, so folding it changes no group or float decision). */
    BK.cur_drawn_model_skip_interpolation = skip || bk_world_cut();
    /* Projection-group ID for a bit40 root of this draw (DEFB0, shadow_projection.c), kept on a skip:
     * BK skips a group by keeping its ID with every component SKIP (transform_tagging.c 211-213,
     * camera_transform_tagging.c 94-95), so the next workload maps it again. RT64 maps a projection only
     * when its ID equals the previous workload's, never ID 0 (rt64_game_frame.cpp 470-481). The DEFB0
     * policy follows cur_drawn_model_skip_interpolation. */
    BKS.projection = projection;
    BKS.projection_open = 1u;
    /* The compositor of this A6250 activation copies the full target (shadow_projection.c). */
    if (seam == TOOIE_BK_SHADOW_SEAM_BODY) BKS.body_frame = frame;
    bk_count(&BKS.scopes);
}

/* After the seam's DE448 returns (A5EB0, A666C, A66C4; A66C4 is also the A6674 skip target).
 * Closes only the scope this activation opened; the skip leg, another frame or a nested scope
 * still on top leave everything alone. */
BK_HELPER void tooie_bk_shadow_end(u32 frame) {
    if (BK.owner.kind != TOOIE_BK_OWNER_SHADOW || BK.owner.base_attachment != frame ||
        !BK.scope_depth || BK.scope_depth > TOOIE_BK_SCOPE_DEPTH) return;
    /* The begin required no open owner, so this activation's own scope saved kind NONE. */
    if (BK.scopes[BK.scope_depth - 1u].prev.kind != TOOIE_BK_OWNER_NONE) return;
    BK.cur_drawn_model_skip_interpolation = 0;
    BK.shadow_projected = 0;
    BKS.projection = 0;
    BKS.projection_open = 0;
    tooie_bk_scope_end(BK.scope_depth);
}

/* @tooie DEFB0, called by tooie_shadow_de498 (shadow_projection.c) after DE498's DE738 pre-draw callback and
 * before its projection policy, the original projection MUL/NOPUSH, the DF0B0 root, float and graph
 * (SHADOW_EARLY_BINDINGS): the owned A6250 body's own visible-branch continuity. Its effective model must
 * pass the all-alternative grammar preflight that bk_select_root repeats at DF0B0 (bk_select_admit), and
 * its executed choices are read from the current controls only under the certified appearance (row0,
 * row1 with D_8012CA0C nonzero, or mode != 1 with leaf 0/F070/E4C4), whose remaining closure before the
 * graph writes no control. Only a proven change from a valid committed shadow baseline draws this sample
 * at its current pose: SkipAll projection under its kept ID, SkipAll groups, no float velocity, and the same for the
 * attachment of this activation. First sight, no baseline and unsupported choices keep the continuous
 * policy (unsupported: staged unknown). Birth/light/world-cut reasons are only added to. Returns the ID for the
 * policy; every other draw (attachment, blob, nested, unowned) gets BKS.projection unchanged. */
BK_HELPER u32 tooie_bk_shadow_project(u32 model) {
    struct TooieBkBranchSig sig;
    u32 id = BK.owner.id, selectors, supported, snap;
    if (!BKS.projection_open || BK.owner.kind != TOOIE_BK_OWNER_SHADOW || BK.draw_depth != 1u || BK.root_depth != 0u ||
        BK.owner.attachments || !id || id != BK.shadow_body_id || BK.cur_drawn_model_transform_id != id)
        return BKS.projection;
    supported = bk_select_admit(model, &selectors) && bk_branch_appearance() &&
        bk_branch_walk(model + *(volatile u32*)(model + 4u), &sig);
    if (supported) sig.model = model;
    else bk_count(&BK.shadow_declined);
    snap = tooie_bk_shadow_branch(BK.shadow_branches, id, supported ? &sig : 0, BK.diagnostics.frames);
    BK.shadow_body_snap |= snap;
    if (snap) {
        bk_count(&BK.shadow_cuts);
        BK.cur_drawn_model_skip_interpolation = 1u;
    }
    return BKS.projection;
}
