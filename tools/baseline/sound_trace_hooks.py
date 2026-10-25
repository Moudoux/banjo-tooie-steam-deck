"""F4 audio-domain retrospective ring hooks; they copy argument bits only (read-only)."""
HOOKS = [
    ('bsbflip_init', 0x808003D4, 'baseline_trace_flip_init'),
    ('func_80800064_bsbflip', 0x80800064, 'baseline_trace_flip_substate'),
    ('func_8009FE58', 0x8009FE58, 'baseline_trace_landing'),
    ('func_800C301C', 0x800C301C, 'baseline_trace_sound_id'),
    ('func_800C3BDC', 0x800C3BDC, 'baseline_trace_sound_start'),
    ('func_800C1D80', 0x800C1D80, 'baseline_trace_sound_submit'),
    ('func_800C3CE8', 0x800C3CE8, 'baseline_trace_sound_stop'),
]


def add_hooks(hooks, selected):
    if not selected:
        return
    for function, pc, name in HOOKS:
        assert not any(h['func'] == function and h.get('before_vram') == pc for h in hooks)
        hooks.append(dict(func=function, before_vram=pc, text=name+'(rdram, ctx);'))


def strip_and_verify(bodies, selected):
    """Read-only inverse: the original game body must remain identical."""
    restored = dict(bodies)
    for function, pc, name in HOOKS:
        call = '    '+name+'(rdram, ctx);\n'
        assert restored[function].count(call) == int(selected), function
        if selected:
            assert call+'    // 0x%08X:' % pc in restored[function], function
            restored[function] = restored[function].replace(call, '')
    return restored
