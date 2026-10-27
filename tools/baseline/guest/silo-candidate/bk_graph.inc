/* Generic graph helpers, included near the end of bk_pipeline.c (definition order is the
 * BK_HELPERS order). BK counterparts: transform_tagging.c func_80338904 (display-list node),
 * func_803384A8 (billboard), modelRender_draw root close, and the B3 producer argument capture. */

/* Display-list node, BK func_80338904. mode bit0: BK identity MUL/PUSH + later pop (type3).
 * mode clear: identity MUL/NOPUSH, so the type-24 leaf keeps its original list + LOAD/PUSH
 * reload pair adjacent with its CPU/RSP stack depth unchanged (facts47); top x I renders
 * exactly as before and gives RT64 the matrix change that binds the new group.
 * mode bit1: the list's vertices are rewritten by the CPU during traversal (F070/leaf
 * callback), after the root's float rerun, so any enclosing float segments are suspended.
 * @tooie bit0 is dropped inside a header-bit40 root (bk_list_keeps_stack).
 * Returns the flags list_end needs. */
#define TOOIE_BK_LIST_PUSH 1u
#define TOOIE_BK_LIST_CPU_WRITTEN 2u
/* @tooie The innermost recorded root is a header-bit40 draw. Its original root issues no modelview
 * matrix (bk_pipeline.c root envelope), and its type3 lists own the modelview stack across siblings:
 * a list may LOAD/PUSH a matrix that later sibling lists draw under and a later list pops (save-select
 * actors 80C/80D/80E: list0 push, selector-branch vertices, list69 pop). BK's lists are stack-neutral
 * (its bones push at graph level, func_803387F8), so its per-list PUSH/pop would here drop that matrix
 * at list0's end. These lists take identity MUL/NOPUSH and a group-only close instead, which leaves
 * every stack value and depth as the original. A nested non-bit40 root is its own innermost frame and
 * keeps BK's bracket; an overflow root records no frame and keeps it too. */
SI u32 bk_list_keeps_stack(void) {
    return BK.root_depth && BK.root_depth <= TOOIE_BK_ROOT_DEPTH &&
        BK.roots[BK.root_depth - 1u].pushed_group == TOOIE_BK_ROOT_BIT40;
}
BK_HELPER u32 tooie_bk_list_begin(Gfx** gfx, u32 node, u32 mode) {
    GfxCommand* cursor;
    u32 flags = 0;
    if (bk_list_keeps_stack()) mode &= ~TOOIE_BK_LIST_PUSH;
    if ((mode & TOOIE_BK_LIST_CPU_WRITTEN) && BK.root_depth && BK.root_depth <= TOOIE_BK_ROOT_DEPTH &&
        bk_segments_effective((s32)BK.root_depth - 1)) {
        *gfx = (Gfx*)bk_segments_off((GfxCommand*)*gfx);
        bk_count(&BKF.suspended);
        flags |= 4u;
    }
    if (tooie_bk_set_model_matrix_group(gfx, node, 0)) {
        flags |= 2u;
        cursor = (GfxCommand*)*gfx;
        /* @recomp BK: create a new matrix by multiplying in the identity matrix. */
        ((volatile u32*)cursor)[0] = (mode & TOOIE_BK_LIST_PUSH) ? 0xDA380000u : 0xDA380001u;
        ((volatile u32*)cursor)[1] = (u32)&BKF.identity[0];
        *gfx = (Gfx*)(cursor + 1);
        if (mode & TOOIE_BK_LIST_PUSH) flags |= 1u;
    }
    return flags;
}

/* After the list (type3) or after the original reload (type-24 leaf): pop the identity matrix
 * and the group if they were opened, then restore any suspended float segments. */
BK_HELPER void tooie_bk_list_end(Gfx** gfx, u32 flags) {
    GfxCommand* cursor = (GfxCommand*)*gfx;
    volatile struct TooieBkSegment* outer;
    if (flags & 1u) {
        /* Original gSPPopMatrixN(...,1) encoding. */
        ((volatile u32*)cursor)[0] = 0xD8380002u;
        ((volatile u32*)cursor)[1] = 0x40u;
        cursor += 1;
    }
    if (flags & 2u) {
        gEXPopMatrixGroup(cursor, G_MTX_MODELVIEW);
        cursor += 1;
    }
    if ((flags & 4u) && (outer = bk_segments_effective((s32)BK.root_depth - 1))) cursor = bk_segments_on(cursor, outer);
    *gfx = (Gfx*)cursor;
}

/* Billboards DC634/DC740/DC84C, at DC6F8/DC804/DC910 after the original LOAD/PUSH and output
 * Mtx advance (buffers s0, node s1). BK func_803384A8 skips rotation interpolation when the
 * camera skips: skip_all_interpolation carries this root's view cut. The offset advance is
 * made at handler entry (tooie_bk_billboard_entry), as BK does at function entry. */
BK_HELPER void tooie_bk_handler_begin(u32 buffers, u32 node, u32 handler_sp) {
    u32 depth = BK.handler_depth, opened, offset;
    if (depth >= TOOIE_BK_HANDLER_DEPTH) { bk_count(&BK.diagnostics.root_overflow); return; }
    if (!node) {
        /* @tooie L14-VR01b: no graph node = an owner component draw (the option-box sprite; BK func_803164B0's
         * fixed portrait ID). Its group is exactly the owner's component ID: ordinal 0 for this group only, and
         * the traversal offset is left as it was. Billboards always pass their graph node. */
        offset = BK.cur_model_transform_id_offset;
        BK.cur_model_transform_id_offset = 0;
        opened = tooie_bk_set_model_matrix_group((Gfx**)&((SiloBuffers*)buffers)->gfx, node, BK.skip_all_interpolation);
        BK.cur_model_transform_id_offset = offset;
    }
    else opened = tooie_bk_set_model_matrix_group((Gfx**)&((SiloBuffers*)buffers)->gfx, node, BK.skip_all_interpolation);
    BK.handlers[depth].sp = handler_sp;
    BK.handlers[depth].opened = opened;
    BK.handler_depth = depth + 1u;
}

/* At DC72C/DC838/DC944 after the original RSP pop. Zero-child branches join this epilogue
 * without the begin seam: only the invocation whose own SP opened an entry pops it. */
BK_HELPER void tooie_bk_handler_end(u32 buffers, u32 handler_sp) {
    u32 depth = BK.handler_depth;
    if (!depth || BK.handlers[depth - 1u].sp != handler_sp) return;
    BK.handler_depth = depth - 1u;
    if (BK.handlers[depth - 1u].opened)
        tooie_bk_pop_model_matrix_group((Gfx**)&((SiloBuffers*)buffers)->gfx);
}

/* DE498 at DE8C8, after the segment-1 command word is stored: remember this draw's own
 * segment command so root begin can re-read the final word. */
BK_HELPER void tooie_bk_skin_segment(u32 key, u32 command) {
    BKF.capture.key = key;
    BKF.capture.valid = 0;
    BKF.capture.command = command;
}

/* DE498 before the executed dbanim call at DEEE4 or DEEF8 (the two bypass paths reach neither).
 * a0 = effective model + model.u32(+0x28); a1 = header C964; a2 = pose C968. Entry1 exactly
 * when C978 != 0, C838 == 1 and CA11 is neither 2 nor 3; otherwise entry0. */
BK_HELPER void tooie_bk_skin_capture(u32 buffers, u32 key, u32 model) {
    volatile struct TooieBkCapture* c = &BKF.capture;
    (void)buffers;
    if (c->key != key) c->command = 0;
    c->key = key;
    c->model = model;
    c->valid = 0;
    if (!silo_range(model, 0x2Cu, 3u)) return;
    c->deform = model + *(volatile u32*)(model + 0x28u);
    c->header = D_8012C964;
    c->pose = (u32)D_8012C968;
    c->entry = D_8012C978 != 0u && D_8012C838 == 1u && D_8012CA11 != 2u && D_8012CA11 != 3u;
    c->valid = 1;
}

/* DE498 at DF238, the join of the normal path (after DF234) and the header-bit40 path (which
 * skips DF234). Only a bit40 invocation closes here: its root frame (opened at DF0B0 like every
 * root, with no modelview group) closes its own float segments (restoring any enclosing ones) and
 * the outer traversal/skip context, after E4640 has restored the saved camera projection. A normal
 * invocation already closed at DF234 and returns. */
BK_HELPER void tooie_bk_root_join(u32 buffers, u32 cursor_slot, u32 model) {
    (void)cursor_slot;
    if (!(silo_range(model, 0x2Cu, 3u) && (*(volatile u16*)(model + 0xAu) & 0x40u))) return;
    if (!BK.root_depth) { bk_count(&BK.diagnostics.late_closes); return; }
    if (BK.root_depth <= TOOIE_BK_ROOT_DEPTH && BK.roots[BK.root_depth - 1u].pushed_group != 2u) {
        /* The top frame belongs to another invocation: a seam defect, never an accepted path. */
        bk_count(&BK.diagnostics.late_closes);
        return;
    }
    tooie_bk_root_end(buffers);
}

/* Type-12 selector 800DD56C (bk_select.inc), native before-PC hooks keyed by the selector activation's
 * own SP: entry before DD58C, before each executed child traversal (DD5DC single, DD614 bitmask) and
 * the universal join DD63C. Only a reserved PLAYER root at the current root depth acts; a nested root
 * has its own decision and never touches the enclosing selector frames. */
SI u32 bk_select_active(void) {
    return BK.root_depth && BK.root_depth <= TOOIE_BK_ROOT_DEPTH && ((BK.select_roots >> (BK.root_depth - 1u)) & 1u);
}
SI u32 bk_select_top(u32 frame) {
    return BK.handler_depth && BK.handler_depth <= TOOIE_BK_HANDLER_DEPTH && BK.handlers[BK.handler_depth - 1u].sp == frame;
}
/* Reserved span of children 0..last-1; the preflight already proved every child supported. */
SI u32 bk_select_prefix(u32 node, u32 last, u32* span) {
    u32 i, advances, selectors;
    *span = 0;
    for (i = 0; i < last; i++) {
        if (!bk_select_walk(bk_select_child(node, i), &advances, &selectors)) return 0;
        *span += advances;
    }
    return 1;
}
BK_HELPER void tooie_bk_selector_begin(u32 node, u32 frame) {
    u32 depth = BK.handler_depth;
    (void)node;
    if (!bk_select_active()) return;
    /* The preflight reserved one handler slot per selector occurrence of this root. */
    if (depth >= TOOIE_BK_HANDLER_DEPTH) { bk_count(&BK.select_declined); return; }
    BK.handlers[depth].sp = frame;
    BK.handlers[depth].opened = BK.cur_model_transform_id_offset;   /* this selector's base ordinal */
    BK.handler_depth = depth + 1u;
}
BK_HELPER void tooie_bk_selector_child(u32 node, u32 child, u32 frame) {
    u32 i, count, span;
    if (!bk_select_active() || !bk_select_top(frame)) return;
    count = (u32)*(volatile s16*)(node + 8u);
    for (i = 0; i < count && bk_select_child(node, i) != child; i++) {}
    /* The original executes only listed children, and the preflight made every pointer distinct. */
    if (i == count || !bk_select_prefix(node, i, &span)) { bk_count(&BK.select_declined); return; }
    BK.cur_model_transform_id_offset = BK.handlers[BK.handler_depth - 1u].opened + span;
}
BK_HELPER void tooie_bk_selector_end(u32 node, u32 frame) {
    u32 base, span;
    if (!bk_select_top(frame)) return;
    base = BK.handlers[BK.handler_depth - 1u].opened;
    BK.handler_depth -= 1u;
    if (!bk_select_active()) return;
    /* Every path, including control 0, value 0 and an out-of-range value, ends past every child. */
    if (!bk_select_prefix(node, (u32)*(volatile s16*)(node + 8u), &span)) { bk_count(&BK.select_declined); return; }
    BK.cur_model_transform_id_offset = base + span;
}
