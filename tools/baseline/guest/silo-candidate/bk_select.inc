/* Stable type-12 selector numbering, included by bk_pipeline.c after bk_cut.inc (always-inline).
 * BK numbers model nodes by traversal ordinal and keeps that numbering stable across branch choices:
 * its sorted branches advance before each child even when that child draws nothing, "to make matrix
 * group IDs consistent between frames" (transform_tagging.c 334-353). Tooie's type-12 selector
 * 800DD56C chooses child lists from a runtime control with no such reservation, so a skipped child
 * shifts every later ordinal (TROT_BRANCH_FACTS: 62A bones 60..67 move from 37..44 to 20..27).
 * @tooie For PLAYER roots, every selector child keeps a fixed span: child i starts at the selector's
 * base plus the spans of children 0..i-1, and the selector ends at base plus every child's span. The
 * original selector body keeps every control read, count/value/bitmask decision and child call; the
 * native hooks only report its entry, each executed child and its universal join (bk_graph.inc).
 *
 * One whole-root preflight at DF0B0 decides before any numbering changes: the canonical dispatcher, a
 * graph of only the five authenticated default node types, readable nodes, positive selector counts
 * with distinct child pointers, every alternative within the walk budget and below 256 advances (the
 * group window is offset < 256), and one free handler slot per selector occurrence. Anything else
 * keeps the WHOLE root on the original numbering, counted in select_declined; no unsupported node is
 * ever counted as zero and nothing a root draws is suppressed. */
#define TOOIE_BK_SELECT_TABLE 0x8012306Cu       /* D_8012CF8C: canonical graph handler table0 */
#define TOOIE_BK_SELECT_STACK 64u               /* pending resume points (sibling continuations, children) */
#define TOOIE_BK_SELECT_VISITS 1024u            /* node visits over all alternatives (62A: 253) */

/* @tooie The owned A6250 silhouettes (853/854) are type-3/type-12 graphs (execution-facts17; the maintained
 * non-BK shadow_projection.c checks table entries 3 and 12): 800DD024 reads only its s16 list index +8 and
 * emits one returning display-list call through C960, with no child, matrix, ordinal advance, callback or
 * control write (execution-facts37, GRAMMAR_FEATURES). Only an owned projected-shadow scope admits type 3;
 * PLAYER and every other root keep exactly the five-type grammar below. */
SI u32 bk_grammar_leaf3(void) {
    return BK.owner.kind == TOOIE_BK_OWNER_SHADOW && BK.shadow_projected;
}

/* The default handlers whose grammar the counter below is written against. */
SI u32 bk_select_table(void) {
    const u32 t = TOOIE_BK_SELECT_TABLE;
    return D_8012CF8C == t && silo_range(t, 25u * 4u, 3u) &&
        *(volatile u32*)(t + 2u * 4u) == 0x800DCF48u && *(volatile u32*)(t + 10u * 4u) == 0x800DD410u &&
        *(volatile u32*)(t + 12u * 4u) == 0x800DD56Cu && *(volatile u32*)(t + 17u * 4u) == 0x800DD65Cu &&
        *(volatile u32*)(t + 24u * 4u) == 0x800DD998u &&
        (!bk_grammar_leaf3() || *(volatile u32*)(t + 3u * 4u) == 0x800DD024u);
}

/* Selector child i: the node plus its u32 relative offset, the original's own address arithmetic. */
SI u32 bk_select_child(u32 node, u32 i) { return node + *(volatile u32*)(node + 0xCu + 4u * i); }

/* Bounded read-only expansion of the sibling list `list` over EVERY selector alternative: the bone
 * advances it can make and its selector occurrences. Depth-first with resume points, so the pending
 * stack grows with depth and selector fan-out, not with list length. 0: unsupported grammar, schema
 * or budget (the caller then keeps the original numbering). */
SI u32 bk_select_walk(u32 list, u32* advances, u32* selectors) {
    u32 pending[TOOIE_BK_SELECT_STACK];
    u32 top = 0, visits = 0, n = 0, s = 0, node, type, next, resume, descend, count, i, j;
    const u32 leaf3 = bk_grammar_leaf3();
    pending[top++] = list;
    while (top) {
        node = pending[--top];
        for (;;) {
            /* DE2A4: the first node unconditionally, then node+4 relative, zero terminating. */
            if (++visits > TOOIE_BK_SELECT_VISITS || !silo_range(node, 8u, 3u)) return 0;
            type = *(volatile u32*)node;
            next = *(volatile u32*)(node + 4u);
            resume = next ? node + next : 0u;
            descend = 0;
            if (type == 2u) {
                /* 800DCF48: exactly one advance even without animation; child list at u8 +8. */
                if (!silo_range(node, 10u, 1u)) return 0;
                n += 1u;
                if (*(volatile u8*)(node + 8u)) descend = node + *(volatile u8*)(node + 8u);
            }
            else if (type == 12u) {
                /* 800DD56C: s16 count +8 (a nonpositive count is never reserved), u32 offsets +0x0C. */
                s32 c;
                if (!silo_range(node, 12u, 3u)) return 0;
                c = *(volatile s16*)(node + 8u);
                if (c <= 0 || (u32)c > TOOIE_BK_SELECT_STACK - top ||
                    !silo_range(node + 0xCu, 4u * (u32)c, 3u)) return 0;
                count = (u32)c;
                /* Bound the fan-out before the pairwise distinct-pointer check. */
                /* A repeated child pointer would make an executed occurrence ambiguous. */
                for (i = 1; i < count; i++)
                    for (j = 0; j < i; j++) if (bk_select_child(node, i) == bk_select_child(node, j)) return 0;
                s += 1u;
                for (i = 1; i < count; i++) pending[top++] = bk_select_child(node, i);
                descend = bk_select_child(node, 0);
            }
            /* 800DD410 point, 800DD65C plain list, 800DD998 leaf, and for an owned silhouette the 800DD024
             * display list: no graph child and no advance. */
            else if (type != 10u && type != 17u && type != 24u && !(type == 3u && leaf3)) return 0;
            if (descend) {
                if (resume) {
                    if (top >= TOOIE_BK_SELECT_STACK) return 0;
                    pending[top++] = resume;
                }
                node = descend;
                continue;
            }
            if (!resume) break;
            node = resume;
        }
    }
    *advances = n;
    *selectors = s;
    return 1;
}

/* The whole-model preflight above for `model`: canonical table0, every alternative of the admitted grammar
 * within budget, below 256 advances and one free handler slot per selector occurrence. */
SI u32 bk_select_admit(u32 model, u32* selectors) {
    u32 advances;
    return bk_select_table() && silo_range(model, 8u, 3u) &&
        bk_select_walk(model + *(volatile u32*)(model + 4u), &advances, selectors) &&
        advances < TOOIE_BK_TRANSFORM_ID_COUNT && BK.handler_depth <= TOOIE_BK_HANDLER_DEPTH &&
        *selectors <= TOOIE_BK_HANDLER_DEPTH - BK.handler_depth;
}

/* @tooie The top-level root of an owned A6250 shadow draw (body or attachment, bk_shadow_attach.inc), its
 * own sample ID. Its silhouette has type-12 selectors of its own, so a vanished alternative would shift
 * its later IDs exactly as a player's; it takes the same reservation, validated on its own graph. */
SI u32 bk_shadow_projected_root(u32 index, u32 id) {
    return BK.owner.kind == TOOIE_BK_OWNER_SHADOW && BK.shadow_projected && !index && BK.draw_depth == 1u &&
        id == BK.owner.id;
}

/* DF0B0, after the root frame is set up and before its graph runs: this root's numbering decision.
 * `index` is the root's frame index; only a PLAYER root with an owner ID, or an owned projected-shadow
 * root, is ever reserved. */
SI void bk_select_root(u32 index, u32 id, u32 model) {
    u32 selectors;
    BK.select_roots &= ~(1u << index);
    if (!id || (BK.owner.kind != TOOIE_BK_OWNER_PLAYER && !bk_shadow_projected_root(index, id))) return;
    if (!bk_select_admit(model, &selectors)) {
        bk_count(&BK.select_declined);
        return;
    }
    /* A root without selectors has nothing to reserve. */
    if (selectors) BK.select_roots |= 1u << index;
}

/* ---- Visible-branch continuity (@tooie; BK has no counterpart) ------------------------------------
 * RT64 draws a newly visible branch's unmatched groups at the current pose while the matched groups keep
 * interpolating, so connected parts of one player can separate for one workload (reports 7/11 idle and
 * talon batches). The PRIMARY BODY root of a player sample decides, at DF0B0 before any of its groups or
 * float history: when its executed selector choices (or model) differ from the committed baseline, the
 * body root takes BK's existing skip_all_interpolation, and the decision is latched for the owner sample
 * so every later root of that player in the construction (nested roots, the 92710 backpack) follows it:
 * a one-workload whole-player snap instead of mixed-time separation. Nested roots and attachments never
 * decide. The rules are the pure functions in bk_pipeline.h; numbering above is untouched. */
#define TOOIE_BK_APPEARANCE_TABLE 0x80123030u  /* D_80123030: rows 0 and 1, 24 bytes */
#define TOOIE_BK_APPEARANCE_ARGUMENT 0x8012CA0Cu  /* D_8012CA0C: DE498's initializer a2 */
#define TOOIE_BK_INSTALLED_LEAF 0x8012CEC4u    /* D_8012CEC4 */
#define TOOIE_BK_SELECT_CONTROL_BASE 0x8012C850u  /* D_8012C850[60] */

SI u32 bk_branch_appearance(void) {
    const u32 rows = TOOIE_BK_APPEARANCE_TABLE;
    if (!silo_range(rows, 24u, 3u)) return 0;
    return tooie_bk_branch_appearance(D_8012C838, (u32)D_8012CA11, *(volatile u32*)TOOIE_BK_APPEARANCE_ARGUMENT,
        *(volatile u32*)TOOIE_BK_INSTALLED_LEAF, (const volatile u32*)rows);
}

/* The executed choices in original order: DE2A4 siblings, a type-2 child list, and for each selector the
 * original choice (tooie_bk_branch_choice) over its s16 count +8 and s16 control index +0x0A. Only executed
 * children are walked, before the selector's siblings as the original returns to DE2A4 after them.
 * 0: outside the bounded grammar or more than TOOIE_BK_BRANCH_SELECTORS selector occurrences. */
SI u32 bk_branch_walk(u32 list, struct TooieBkBranchSig* sig) {
    u32 pending[TOOIE_BK_SELECT_STACK];
    u32 top = 0, visits = 0, node, type, next, resume, mask, i;
    s32 count, index;
    const u32 leaf3 = bk_grammar_leaf3();
    sig->count = 0;
    pending[top++] = list;
    while (top) {
        node = pending[--top];
        for (;;) {
            if (++visits > TOOIE_BK_SELECT_VISITS || !silo_range(node, 8u, 3u)) return 0;
            type = *(volatile u32*)node;
            next = *(volatile u32*)(node + 4u);
            resume = next ? node + next : 0u;
            if (type == 2u) {
                if (!silo_range(node, 10u, 1u)) return 0;
                if (*(volatile u8*)(node + 8u)) {
                    if (resume) {
                        if (top >= TOOIE_BK_SELECT_STACK) return 0;
                        pending[top++] = resume;
                    }
                    node += *(volatile u8*)(node + 8u);
                    continue;
                }
            }
            else if (type == 12u) {
                if (!silo_range(node, 12u, 3u)) return 0;
                count = *(volatile s16*)(node + 8u);
                index = *(volatile s16*)(node + 0xAu);
                if (sig->count >= TOOIE_BK_BRANCH_SELECTORS || count <= 0 ||
                    !silo_range(node + 0xCu, 4u * (u32)count, 3u)) return 0;
                if (!tooie_bk_branch_choice(index, index > 0 && index < (s32)TOOIE_BK_SELECT_CONTROLS ?
                        *(volatile s32*)(TOOIE_BK_SELECT_CONTROL_BASE + 4u * (u32)index) : 0, count, &mask)) return 0;
                sig->masks[sig->count++] = mask;
                if (resume) {
                    if (top >= TOOIE_BK_SELECT_STACK) return 0;
                    pending[top++] = resume;
                }
                for (i = (u32)count; i-- > 0;) {
                    if (!(mask & (1u << i))) continue;
                    if (top >= TOOIE_BK_SELECT_STACK) return 0;
                    pending[top++] = bk_select_child(node, i);
                }
                break;
            }
            else if (type != 10u && type != 17u && type != 24u && !(type == 3u && leaf3)) return 0;
            if (!resume) break;
            node = resume;
        }
    }
    return 1;
}

/* DF0B0, after bk_select_root and before bk_skin_root_begin and any group: 1 when this root's draw takes
 * skip_all_interpolation. The primary body root decides and latches; every other PLAYER root follows the
 * latch of its owner sample (0 when the body has not decided in this construction). An unsupported body
 * still stages "unknown", so a later supported body never compares across a workload it did not see. */
SI u32 bk_branch_root(u32 index, u32 id, u32 model) {
    struct TooieBkBranchSig sig;
    const u32 owner = BK.owner.id;
    u32 supported, snap;
    if (BK.owner.kind != TOOIE_BK_OWNER_PLAYER || !id || !owner) return 0;
    if (!tooie_bk_branch_is_body(BK.owner.kind, id, owner, BK.owner.attachments, index, BK.draw_depth))
        return tooie_bk_branch_latched(BK.branch_latches, owner);
    supported = (BK.select_roots & 1u) && bk_branch_appearance() &&
        bk_branch_walk(model + *(volatile u32*)(model + 4u), &sig);
    if (supported) sig.model = model;
    else bk_count(&BK.branch_declined);
    snap = tooie_bk_branch_body(BK.branches, id, supported ? &sig : 0, BK.diagnostics.frames);
    snap |= tooie_bk_branch_latched(BK.branch_latches, owner);
    if (snap) bk_count(&BK.branch_cuts);
    tooie_bk_branch_latch(BK.branch_latches, owner, snap, &BK.branch_latch_overflow);
    return snap;
}
