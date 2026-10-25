"""BK pipeline stable-order sort traversal for DC958 / DCB58 / DCD10 (execution-facts48).

Each original handler keeps its scalar preparation, ordered comparisons and distance publication.
Only its four both-child DE2A4 JALs become NOPs (their delay-slot preparation still executes);
the BK construction then runs at the common epilogue. One-sided calls stay original.
"""
import re

# (function, bit0 branch point, reversed-path entry, common epilogue, four both-child JALs)
SORTS = [
    ('func_800DC958', 0x800DCA48, 0x800DCB18, 0x800DCB48, (0x800DCAEC, 0x800DCB04, 0x800DCB24, 0x800DCB3C)),
    ('func_800DCB58', 0x800DCC00, 0x800DCCD0, 0x800DCD00, (0x800DCCA4, 0x800DCCBC, 0x800DCCDC, 0x800DCCF4)),
    ('func_800DCD10', 0x800DCDB8, 0x800DCE88, 0x800DCEB8, (0x800DCE5C, 0x800DCE74, 0x800DCE94, 0x800DCEAC)),
]
HOOKS = [h for func, branch, reverse, epilogue, _ in SORTS for h in (
    (func, branch, 'baseline_bk_sort_onesided'),     # one advance before the one-sided child
    (func, reverse, 'baseline_bk_sort_reversed'),    # original chose B then A (incl. NaN)
    (func, epilogue, 'baseline_bk_sort_children'))]  # BK construction, A then B
NOPS = [(func, pc) for func, _, _, _, jals in SORTS for pc in jals]
MIPS_NOP = 0x00000000


def add_hooks(hooks, instructions, selected):
    assert type(selected) is bool
    if not selected:
        return
    for func, pc, name in HOOKS:
        assert not any(h['func'] == func and h.get('before_vram') == pc for h in hooks), (func, hex(pc))
        hooks.append(dict(func=func, before_vram=pc, text=name + '(rdram, ctx);'))
    for func, pc in NOPS:
        assert not any(e.get('func') == func and e.get('vram') == pc for e in instructions), (func, hex(pc))
        instructions.append(dict(func=func, vram=pc, value=MIPS_NOP))


def strip_and_verify(before, after, selected):
    """Hook calls are removed exactly. The NOP edits are verified, not inverted: the original
    body issues six DE2A4 calls; the edited body issues only the two one-sided calls."""
    assert type(selected) is bool
    for func, _, _, _, jals in SORTS:
        for owner, pc, name in HOOKS:
            if owner != func:
                continue
            call = '    ' + name + '(rdram, ctx);\n'
            assert after[func].count(call) == int(selected), (func, name)
            if selected:
                assert call + '    // 0x%08X:' % pc in after[func], (func, hex(pc))
                after[func] = after[func].replace(call, '')
        calls = len(re.findall(r'\bfunc_800DE2A4\(rdram, ctx\);', after[func]))
        original = len(re.findall(r'\bfunc_800DE2A4\(rdram, ctx\);', before[func]))
        assert original == 6, (func, original)
        if selected:
            assert calls == 2, ('sort both-child JALs not removed', func, calls)
            for pc in jals:
                assert re.search(r'// 0x%08X: nop\b' % pc, after[func]), (func, hex(pc))
        else:
            assert after[func] == before[func], func
    return after
