/* BK stable-order sorted-branch traversal for the three Tooie sort variants
 * (DC958 / DCB58 / DCD10). Included at the end of bk_pipeline.c after bk_graph.inc.
 * Adapted from BanjoRecomp dbd52154 patches/transform_tagging.c func_803385BC 300-404.
 *
 * The original handlers keep their own scalar preparation (table0 optional pose transform,
 * DC580, DC5BC), ordered comparisons and distance publication (CEC8 / CEFC / CF2C); the recipe
 * only turns their four both-child DE2A4 JALs into NOPs. At the common epilogue this helper
 * constructs child A (signed16 node+0x22) then child B (32-bit node+0x24) in CPU order, with
 * one offset advance at each logical boundary even for a zero child. When the original chose
 * the reversed path (B then A, including NaN), three nonreturning branch-list commands keep
 * the RSP execution order B then A: first -> B, middle -> after-all, last -> A. One-sided
 * mode keeps its original single child call; its one advance is made at the bit0 branch point,
 * before that call (tooie_bk_sort_onesided), and this epilogue returns for it. */
void func_800DE2A4(SiloBuffers*, u8*);

SI void bk_branch_list(GfxCommand* at, u32 target) {
    /* F3DEX2 gSPBranchList: G_DL with G_DL_NOPUSH, as BK emits. */
    ((volatile u32*)at)[0] = 0xDE010000u;
    ((volatile u32*)at)[1] = target;
}

BK_HELPER void tooie_bk_sort_children(u32 buffers, u32 node, u32 reversed) {
    SiloBuffers* b = (SiloBuffers*)buffers;
    s32 a, second;
    GfxCommand *before_cmds, *before_a, *between_cmds, *before_b, *after_b, *after_cmds;
    if (tooie_bk_sort_one_sided(*(volatile u8*)(node + 0x21u))) return;
    a = *(volatile s16*)(node + 0x22u);
    second = *(volatile s32*)(node + 0x24u);
    if (!reversed) {
        /* BK: increment before the first child, then before the second. */
        BK.cur_model_transform_id_offset += 1u;
        if (a) func_800DE2A4(b, (u8*)(node + (u32)a));
        BK.cur_model_transform_id_offset += 1u;
        if (second) func_800DE2A4(b, (u8*)(node + (u32)second));
        return;
    }
    /* BK: reserve one command for the branch list to the second node. */
    before_cmds = (GfxCommand*)b->gfx;
    b->gfx = (Gfx*)(before_cmds + 1);
    before_a = (GfxCommand*)b->gfx;
    BK.cur_model_transform_id_offset += 1u;
    if (a) func_800DE2A4(b, (u8*)(node + (u32)a));
    /* BK: reserve the branch past the second node's commands, after the first node's. */
    between_cmds = (GfxCommand*)b->gfx;
    b->gfx = (Gfx*)(between_cmds + 1);
    before_b = (GfxCommand*)b->gfx;
    BK.cur_model_transform_id_offset += 1u;
    if (second) func_800DE2A4(b, (u8*)(node + (u32)second));
    /* BK: reserve the third branch, back to the first node. */
    after_b = (GfxCommand*)b->gfx;
    b->gfx = (Gfx*)(after_b + 1);
    after_cmds = (GfxCommand*)b->gfx;
    bk_branch_list(before_cmds, (u32)before_b);
    bk_branch_list(between_cmds, (u32)after_cmds);
    bk_branch_list(after_b, (u32)before_a);
}
