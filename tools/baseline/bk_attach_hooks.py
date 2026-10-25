"""BK pipeline sequential player attachment owner scope (execution-facts24 Q-C1).

func_80092444 draws the player body (DE448 call at 926D8) and then the backpack through ckpack
entry3: JAL at 92710 (delay slot 92714), return continuation 92718. Selected with the BK pipeline;
neither PC carries another hook in the selected raw configuration.
"""
import re

HOOKS = [
    ('func_80092444', 0x80092710, 'baseline_bk_attachment_begin'),
    ('func_80092444', 0x80092718, 'baseline_bk_attachment_end'),
]


def add_hooks(hooks, selected):
    assert type(selected) is bool
    if not selected:
        return
    for func, pc, name in HOOKS:
        assert not any(h['func'] == func and h.get('before_vram') == pc for h in hooks), (func, hex(pc))
        hooks.append(dict(func=func, before_vram=pc, text=name + '(rdram, ctx);'))


def strip_and_verify(after, selected):
    """Exact inverse: each call once, directly at its PC, then removed."""
    assert type(selected) is bool
    for func, pc, name in HOOKS:
        call = '    ' + name + '(rdram, ctx);\n'
        assert after[func].count(call) == int(selected), (func, name)
        if selected:
            assert re.search(re.escape(call) + r'(?:    \w+\(rdram, ctx\);\n)*' + re.escape('    // 0x%08X:' % pc), after[func]), (func, hex(pc))
            after[func] = after[func].replace(call, '')
    return after
