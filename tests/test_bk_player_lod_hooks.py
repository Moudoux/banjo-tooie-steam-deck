"""Player LOD hook registration (proposal 1a, optional 1b): exact PCs, one call each, exact inverse.

Run with the repository's tools/baseline on sys.path, after applying 1a (and 1b to cover it):
    python -I tests/test_bk_player_lod_hooks.py <repo>/tools/baseline
Synthetic recompiler text only; no generated game code is read.
"""
import importlib
import sys


def main(tools):
    sys.path.insert(0, tools)
    hooks = importlib.import_module('bk_pipeline_hooks')
    names = {(f, pc): n for f, pc, n in hooks.HOOKS}
    # 1a: after EEAD4 returns (its JAL delay slot already executed), before the tier walk.
    assert names[('func_80094070', 0x800940B8)] == 'baseline_bk_player_lod_distance'
    optional = ('func_80093F30', 0x80093F30) in names
    if optional:
        # 1b: 93F30 entry, so the 94038 JAL's delay slot 9403C has loaded a0 before the thunk runs.
        assert names[('func_80093F30', 0x80093F30)] == 'baseline_bk_player_lod_initial'
    # Graph LOD seams unchanged; exactly one player-tier seam per function.
    assert sum(1 for f, _, _ in hooks.HOOKS if f == 'func_80094070') == 1
    assert [pc for f, pc, n in hooks.HOOKS if n == 'baseline_bk_lod_distance'] == [0x800DD294, 0x800DD328, 0x800DD3BC]

    # add_hooks composes after the maintained hooks and inserts the new PCs once.
    existing = [dict(func='func_800A7840', before_vram=pc, text=text) for pc, text in hooks.COMPOSED.items()]
    hooks.add_hooks(existing, True)
    tier = [h for h in existing if h['func'] == 'func_80094070']
    assert len(tier) == 1 and tier[0]['before_vram'] == 0x800940B8 and \
        tier[0]['text'] == 'baseline_bk_player_lod_distance(rdram, ctx);'
    try:
        hooks.add_hooks(existing, True)
        raise AssertionError('duplicate registration must fail')
    except AssertionError as error:
        assert 'duplicate' not in str(error)

    # strip_and_verify: each call directly before its PC comment, then removed exactly.
    after = {}
    for func, pc, name in hooks.HOOKS:
        body = after.get(func, '')
        body += '    %s(rdram, ctx);\n    // 0x%08X: original\n' % (name, pc)
        after[func] = body
    # Include the exact structural contracts required by the production inverse.
    for func, pc, reg in [('func_800BE7F8',0x800BE90C,13),('func_800BFAD0',0x800BFB40,14),('func_800BFD6C',0x800BFE70,11)]:
        after[func] = after[func].replace('// 0x%08X: original'%pc,
            '// 0x%08X: jal         0x800DE448\n    MEM_W(0X10, ctx->r29) = ctx->r%d;\n    func_800DE448(rdram, ctx);'%(pc,reg))
    if optional:
        assert names[('func_80093FD4',0x80094038)] == 'baseline_bk_player_lod_initial_begin'
        assert names[('func_80093FD4',0x80094040)] == 'baseline_bk_player_lod_initial_end'
        after['func_80093FD4'] = after['func_80093FD4'].replace('// 0x80094038: original',
            '// 0x80094038: jal         0x80093F30\n    // 0x8009403C: tier load\n    ctx->r4 = MEM_H(ctx->r25, 0X2);\n    func_80093F30(rdram, ctx);')
    for f, pc, target, delay in hooks.PREDICATE_SEAMS:
        marker = '    baseline_bk_bbox_visible(rdram, ctx);\n    // 0x%08X: original\n' % pc
        segment = ('    // 0x%08X: jal         0x%08X\n' % (pc-8, target) +
                   '    // 0x%08X: synthetic delay\n' % (pc-4) +
                   ('    ' + delay + '\n' if delay else '') +
                   '    func_%08X(rdram, ctx);\n' % target +
                   '    baseline_bk_bbox_visible(rdram, ctx);\n    // 0x%08X: beql        $v0, $zero, L_80000000\n' % pc)
        after[f] = after[f].replace(marker, segment)
    after['func_800DE498'] = after['func_800DE498'].replace('// 0x800DE498: original',
        '// 0x800DE498: original\n    ctx->r29 = ADD32(ctx->r29, -0XF8);')
    stripped = hooks.strip_and_verify(dict(after), True)
    for func, pc, name in hooks.HOOKS:
        assert name + '(rdram, ctx);' not in stripped[func]
        assert '    // 0x%08X:' % pc in stripped[func]
    # A call placed after its PC (e.g. after the delay slot moved) is rejected.
    misplaced = dict(after)
    misplaced['func_80094070'] = '    // 0x800940B8: original\n    baseline_bk_player_lod_distance(rdram, ctx);\n'
    try:
        hooks.strip_and_verify(misplaced, True)
        raise AssertionError('misplaced call accepted')
    except AssertionError as error:
        assert 'misplaced' not in str(error)
    print('test_bk_player_lod_hooks: ok' + (' (1b included)' if optional else ''))


if __name__ == '__main__':
    main(sys.argv[1])
