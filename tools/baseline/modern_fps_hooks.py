"""One declared N64Recomp before_vram hook; never rewrites generated code."""
HOOK = ('func_800991B0', 0x80099220, 'baseline_modern_fps_capture')

def add_hooks(hooks, selected):
    assert type(selected) is bool
    if not selected:
        return
    function, pc, name = HOOK
    assert not any(h['func'] == function and h.get('before_vram') == pc for h in hooks)
    hooks.append(dict(func=function, before_vram=pc, text=name+'(rdram, ctx);'))

def strip_and_verify(bodies, selected):
    """Read-only mechanical inverse for existing raw qualification, not a writer."""
    assert type(selected) is bool
    restored = dict(bodies)
    function, pc, name = HOOK
    call = '    '+name+'(rdram, ctx);\n'
    assert restored[function].count(call) == int(selected)
    if selected:
        assert call+'    // 0x%08X:' % pc in restored[function]
        restored[function] = restored[function].replace(call, '')
    return restored
