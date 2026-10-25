"""BK pipeline shadow producer owner scope (execution-facts53 2-3, execution-facts54, execution-facts55).

Seams (each before the instruction at that PC; none is a delay slot):
  A6B24 A6CF0  local-light route, handle in s0, before the A69F0 JAL
  A6B24 A6D60  fallback route, before the A69F0 JAL
  A6250 A6664  body begin (direct DE448 JAL; A6668 delay stores the model)
  A6250 A666C  body end (return continuation)
  A6250 A66BC  attachment begin (direct DE448 JAL; A66C0 delay stores the model)
  A6250 A66C4  attachment end: return continuation AND the A6674 skip target, so the generated end
               must follow the L_800A66C4 label (both predecessors execute it)
  A5DB0 A5EA8  blob begin (direct DE448 JAL; A5EAC delay stores the model), receiver s0
  A5DB0 A5EB0  blob end (return continuation)
  C8A98 C8B78  light allocation return: C8A98's one epilogue (reuse and append), r2 = returned handle
               (loaded at C8B74), original sp not yet restored
  C90AC, C90D0, C90F4 at function entry: light vector init, free, and the loader's free+init
               (overlay entry2) that bypasses the two wrappers

Occupancy in the BK selection: the shadow-fix hooks that used A6CF0/A6D60/A6664/A666C (source,
prepare, MODELVIEW close) are not installed with BK (prepare_camera_bridge SHADOW_BK_RETIRED_HOOKS);
these seams have no other occupant. Named occupants that may coexist are composed, BK call first,
so they keep their own PC-adjacent placement and checks: the diagnostic observer's route markers
at A6CF0/A6D60 and the P04b/N3 attachment hooks at A66BC/A66C4 (never selected with BK). Any other
occupant, including the retired shadow-fix hooks, fails the recipe. BK scope calls emit no
commands, so their order next to a composed hook changes no display list.
"""
import re

ROUTE, BLOB, BODY_ATTACH = 'func_800A6B24', 'func_800A5DB0', 'func_800A6250'
END_LABEL = 0x800A66C4
HOOKS = [
    (ROUTE, 0x800A6CF0, 'baseline_bk_shadow_route_local'),
    (ROUTE, 0x800A6D60, 'baseline_bk_shadow_route_fallback'),
    (BODY_ATTACH, 0x800A6664, 'baseline_bk_shadow_body_begin'),
    (BODY_ATTACH, 0x800A666C, 'baseline_bk_shadow_body_end'),
    (BODY_ATTACH, 0x800A66BC, 'baseline_bk_shadow_attachment_begin'),
    (BODY_ATTACH, 0x800A66C4, 'baseline_bk_shadow_attachment_end'),
    (BLOB, 0x800A5EA8, 'baseline_bk_shadow_blob_begin'),
    (BLOB, 0x800A5EB0, 'baseline_bk_shadow_blob_end'),
    ('func_800C8A98', 0x800C8B78, 'baseline_bk_shadow_light_born'),
    ('func_800C90AC', 0x800C90AC, 'baseline_bk_shadow_light_reset'),
    ('func_800C90D0', 0x800C90D0, 'baseline_bk_shadow_light_reset'),
    ('func_800C90F4', 0x800C90F4, 'baseline_bk_shadow_light_reset'),
]
FUNCTION_ENTRIES = {0x800C90AC, 0x800C90D0, 0x800C90F4}
# The allocator's only return epilogue: the hook must sit after the r2 reload marker (C8B74) and
# before the stack restore marker (C8B78), each present exactly once.
RETURN_SEAM = (0x800C8B74, 0x800C8B78)
COMPOSABLE = {
    0x800A6CF0: 'baseline_shadow_local(rdram, ctx);',
    0x800A6D60: 'baseline_shadow_fallback(rdram, ctx);',
    0x800A66BC: 'baseline_cpu625_attach_begin(rdram, ctx);',
    0x800A66C4: 'baseline_cpu625_attach_end(rdram, ctx);',
}


def add_hooks(hooks, selected):
    assert type(selected) is bool
    if not selected:
        return
    for func, pc, name in HOOKS:
        call = name + '(rdram, ctx);'
        same = [h for h in hooks if h['func'] == func and h.get('before_vram') == pc]
        assert len(same) <= 1, (func, hex(pc))
        if same:
            assert pc in COMPOSABLE and same[0]['text'] == COMPOSABLE[pc], (func, hex(pc), same[0]['text'])
            same[0]['text'] = call + '\n    ' + same[0]['text']
        else:
            hooks.append(dict(func=func, before_vram=pc, text=call))


def strip_and_verify(after, selected):
    """Exact inverse: each call once, directly at its PC (end after the A66C4 label, resets as the
    first statement of their function, the allocation return between C8B74 and C8B78), then removed."""
    assert type(selected) is bool
    for func, pc, name in HOOKS:
        call = '    ' + name + '(rdram, ctx);\n'
        assert after[func].count(call) == int(selected), (func, name)
        if selected:
            marker = '    // 0x%08X:' % pc
            tail = r'(?:    \w+\(rdram, ctx\);\n)*' + re.escape(marker)
            assert re.search(re.escape(call) + tail, after[func]), (func, hex(pc))
            if pc == END_LABEL:
                # Both predecessors, the DE448 return and the A6674 skip branch, pass this label.
                assert re.search(r'^[ \t]*L_800A66C4:\n' + re.escape(call) + tail, after[func], re.M), \
                    (func, 'end must follow L_800A66C4')
            if pc in FUNCTION_ENTRIES:
                assert after[func].index(marker) == after[func].index('    // 0x'), (func, 'function entry')
            if pc == RETURN_SEAM[1]:
                load, restore = ('    // 0x%08X:' % p for p in RETURN_SEAM)
                assert after[func].count(load) == after[func].count(restore) == 1, (func, 'single return epilogue')
                assert after[func].index(load) < after[func].index(call) < after[func].index(restore), \
                    (func, 'handle loaded, stack not yet restored')
            after[func] = after[func].replace(call, '')
    return after
