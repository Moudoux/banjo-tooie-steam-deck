"""BK pipeline graph hooks: B3 producer capture, bit40 root join, billboard groups, type-12 selectors.

Selected with the BK pipeline. The P04b ckpack copy storage (and its DE8C8 segment redirect)
is never selected together with BK, so every PC here is new; none composes with another hook.
"""
import re

HOOKS = [
    # DE498 after DE8C4 stores the segment-1 word (sp+0xB8 holds the command): root begin re-reads
    # that word and publishes floats only for the original header+24 stream.
    ('func_800DE498', 0x800DE8C8, 'baseline_bk_skin_segment'),
    # DE498 executed dbanim calls (BT_DISPATCH DEE8C-DEEF8); bypass paths reach neither PC.
    ('func_800DE498', 0x800DEEE4, 'baseline_bk_skin_capture_a'),
    ('func_800DE498', 0x800DEEF8, 'baseline_bk_skin_capture_b'),
    # DE498 DF238: join of the normal path (after DF234) and the header-bit40 path (skips DF234).
    ('func_800DE498', 0x800DF238, 'baseline_bk_root_join'),
    # Billboard table0/1/2: entry (BK func_803384A8 offset++), after push+Mtx advance, after pop.
    ('func_800DC634', 0x800DC634, 'baseline_bk_billboard_entry'),
    ('func_800DC634', 0x800DC6F8, 'baseline_bk_billboard_begin'),
    ('func_800DC634', 0x800DC72C, 'baseline_bk_billboard_end'),
    ('func_800DC740', 0x800DC740, 'baseline_bk_billboard_entry'),
    ('func_800DC740', 0x800DC804, 'baseline_bk_billboard_begin'),
    ('func_800DC740', 0x800DC838, 'baseline_bk_billboard_end'),
    ('func_800DC84C', 0x800DC84C, 'baseline_bk_billboard_entry'),
    ('func_800DC84C', 0x800DC910, 'baseline_bk_billboard_begin'),
    ('func_800DC84C', 0x800DC944, 'baseline_bk_billboard_end'),
    # Type-12 selector: after the prologue saves, before each executed child DE2A4 JAL (single DD5DC,
    # bitmask DD614; their delay-slot ADDUs have not run) and the universal join DD63C of every path.
    ('func_800DD56C', 0x800DD58C, 'baseline_bk_selector_begin'),
    ('func_800DD56C', 0x800DD5DC, 'baseline_bk_selector_single'),
    ('func_800DD56C', 0x800DD614, 'baseline_bk_selector_mask'),
    ('func_800DD56C', 0x800DD63C, 'baseline_bk_selector_end'),
]


def add_hooks(hooks, selected):
    assert type(selected) is bool
    if not selected:
        return
    for func, pc, name in HOOKS:
        assert not any(h['func'] == func and h.get('before_vram') == pc for h in hooks), (func, hex(pc))
        hooks.append(dict(func=func, before_vram=pc, text=name + '(rdram, ctx);'))


def strip_and_verify(after, selected):
    """Exact inverse: each call once in its function, directly at its PC, then removed."""
    assert type(selected) is bool
    for func, pc, name in HOOKS:
        call = '    ' + name + '(rdram, ctx);\n'
        assert after[func].count(call) == int(selected), (func, name)
        if selected:
            assert re.search(re.escape(call) + r'(?:    \w+\(rdram, ctx\);\n)*' + re.escape('    // 0x%08X:' % pc), after[func]), (func, hex(pc))
            after[func] = after[func].replace(call, '')
    return after
