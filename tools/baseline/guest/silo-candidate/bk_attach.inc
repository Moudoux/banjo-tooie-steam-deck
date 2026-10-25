/* Sequential player attachment owner scope, included last by bk_pipeline.c.
 * BK counterpart: patches/marker_transform_tagging.c player_draw 20-37, which sets Banjo's
 * transform ID and skinning record around baModel_draw, so everything the player draws shares
 * one owner. Tooie's player body renderer func_80092444 draws the body (DE448 call at 926D8,
 * closed back at depth 0) and then the backpack through ckpack entry3 (JAL 92710, return
 * 92718) as a separate depth-1 DE448 (execution-facts24 Q-C1). This scope gives that draw the
 * player owner as attachment ordinal 1 of the same view x phase sample: a creation-stable ID
 * and a velocity record keyed by it, under the same previous-frame/model/cut rules. */

/* JAL 92710: after the body draw has returned and its player scope has closed. owner = func_80092444's s0,
 * the PlayerState it is drawing (kept in s0 through 9271C): the authentic producer of this backpack. It
 * must be the bound roster owner the body draw recorded, so the backpack keys its IDs by that player. */
BK_HELPER void tooie_bk_attachment_begin(u32 owner) {
    u32 view, phase, crumb = BK.reserved0, base, slot, id, token;
    /* Consume the body route: each 92710 pairs only with the body draw just before it. */
    BK.reserved0 = 0;
    BK.handler_reserved = 0;
    /* Only right after a player body draw of this frame, at top level and with no open owner. */
    if (!(crumb & 0x100u) || BK.draw_depth || BK.owner.kind != TOOIE_BK_OWNER_NONE) return;
    base = crumb & 0xFFu;
    /* @tooie Split players: the producer's roster identity must be the body's. */
    slot = bk_player_slot(owner);
    if (!slot || slot != crumb >> 16) { bk_count(&BK.diagnostics.player_untagged); return; }
    view = (u32)func_800A89F8(); phase = (u32)func_800EB854();
    id = bk_player_id(slot - 1u, base + 1u, view, phase);
    if (!id || !(token = bk_scope_open(0))) return;
    BK.owner.kind = TOOIE_BK_OWNER_PLAYER; BK.owner.birth = slot;
    BK.owner.view = view; BK.owner.phase = phase; BK.owner.base_attachment = base;
    /* Ordinal 1 is taken by this draw; anything nested inside it continues at 2. */
    BK.owner.attachments = 1; BK.owner.own_draw_pending = 1;
    BK.owner.id = bk_player_id(slot - 1u, base, view, phase);
    BK.cur_drawn_model_transform_id = id;
    BK.handler_reserved = token;
}

/* 92718: entry3 has returned (its DE448 and all of its commands are complete). */
BK_HELPER void tooie_bk_attachment_end(void) {
    u32 token = BK.handler_reserved;
    BK.handler_reserved = 0;
    if (token) tooie_bk_scope_end(token);
}
