"""In-graph sphere-predicate seams: exact rows, seam contract and multi-use inverse (synthetic text only).

    python -I tests/test_bk_graph_predicate_seams.py <repo>/tools/baseline
"""
import importlib
import sys

GRAPH = [('func_800DDDC4', 0x800DDE38), ('func_800DDDC4', 0x800DDEE0),
         ('func_800DDF14', 0x800DDF6C), ('func_800DDFA0', 0x800DE008)]
HOOK = '    baseline_bk_bbox_visible(rdram, ctx);\n'


def seam(pc, target=0x800E3E8C, hook=HOOK, label=''):
    # Recompiler shape: JAL comment, delay comment and statement, call, then the hook before the branch.
    return ('    // 0x%08X: jal         0x%08X\n' % (pc - 8, target) +
            '    // 0x%08X: synthetic delay\n    ctx->r5 = ctx->r16;\n' % (pc - 4) +
            '    func_%08X(rdram, ctx);\n' % target + hook + label +
            '    // 0x%08X: beql        $v0, $zero, L_80000000\n' % pc)


def rejects(check, *args):
    try:
        check(*args)
    except (AssertionError, ValueError):
        return
    raise SystemExit('accepted: ' + check.__name__)


def main(tools):
    sys.path.insert(0, tools)
    hooks = importlib.import_module('bk_pipeline_hooks')
    # Exactly the four proven seams at predicate call + 8, sharing the bbox thunk; no new DE498 (model-level) seam.
    graph = {f for f, _ in GRAPH}
    assert [(f, pc) for f, pc, _ in hooks.HOOKS if f in graph] == GRAPH
    assert all(n == 'baseline_bk_bbox_visible' for f, _, n in hooks.HOOKS if f in graph)
    assert [(f, pc, t) for f, pc, t, _ in hooks.PREDICATE_SEAMS if f in graph] == [(f, pc, 0x800E3E8C) for f, pc in GRAPH]
    assert [pc for f, pc, _ in hooks.HOOKS if f == 'func_800DE498'] == [0x800DE498, 0x800DF0B0, 0x800DF234]

    call = '    func_800E3E8C(rdram, ctx);\n'
    for func, pc in GRAPH:
        hooks.verify_predicate_seam(seam(pc), func, pc, 0x800E3E8C, None)
        rejects(hooks.verify_predicate_seam, seam(pc).replace('$v0,', '$a0,'), func, pc, 0x800E3E8C, None)
        before_call = seam(pc, hook='').replace(call, HOOK + call)
        rejects(hooks.verify_predicate_seam, before_call, func, pc, 0x800E3E8C, None)
        rejects(hooks.verify_predicate_seam, seam(pc, 0x800E3D0C), func, pc, 0x800E3E8C, None)
        rejects(hooks.verify_predicate_seam, seam(pc, label='L_%08X:\n' % pc), func, pc, 0x800E3E8C, None)
        slot = '    // 0x%08X: synthetic delay' % (pc - 4)
        before_slot = seam(pc).replace(call, '').replace(slot, call + slot)
        rejects(hooks.verify_predicate_seam, before_slot, func, pc, 0x800E3E8C, None)
    # Pinned delay text on the existing bbox seams is still required.
    f, pc, t, d = hooks.PREDICATE_SEAMS[0]
    hooks.verify_predicate_seam(seam(pc, t).replace('ctx->r5 = ctx->r16;', d), f, pc, t, d)
    rejects(hooks.verify_predicate_seam, seam(pc, t), f, pc, t, d)

    # Two uses of one hook in DDDC4: each PC needs its own call; the original predicate calls survive.
    rows = [(f, pc, 'baseline_bk_bbox_visible') for f, pc in GRAPH]
    bodies = {}
    for f, pc in GRAPH:
        bodies[f] = bodies.get(f, '') + seam(pc)
    stripped = hooks.strip_calls(dict(bodies), rows, True)
    assert all(stripped[f] == bodies[f].replace(HOOK, '') for f in bodies)
    assert stripped['func_800DDDC4'].count(call) == 2
    stacked = dict(bodies, func_800DDDC4=seam(0x800DDE38, hook=HOOK * 2) + seam(0x800DDEE0, hook=''))
    rejects(hooks.strip_calls, stacked, rows, True)
    rejects(hooks.strip_calls, dict(bodies, func_800DDF14=seam(0x800DDF6C, hook='')), rows, True)
    rejects(hooks.strip_calls, dict(bodies), rows, False)
    print('test_bk_graph_predicate_seams: ok')


if __name__ == '__main__':
    main(sys.argv[1])
