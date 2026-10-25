"""Authored insertion helper for prepare_camera_bridge.prepare_raw, beside cpu625_hooks.py.
PC[8] secondary route provenance: six before_vram hooks with context-preserving native thunks
(secondary_thunks.hpp). The selected flag must propagate through compile/link, registry,
providers, receipts and native adapters. No original omissions are added.
"""
import re
SECONDARY_HELPERS = ['tooie_secondary_route_begin','tooie_secondary_route_end',
                     'tooie_secondary_route_arm','tooie_secondary_route_disarm',
                     'tooie_player_secondary_begin']
# execution-facts26/28. Begins precede their 93864 jal (the a2=1 delay slot has not run: the
# route comes from the site). Ends and the disarm follow their generated return labels.
SECONDARY_HOOKS = [
    ('func_80093AB8',0x80093D40,'baseline_secondary_route_begin_main'),
    ('func_80093AB8',0x80093D48,'baseline_secondary_route_end_main'),
    ('func_80093E18',0x80093E9C,'baseline_secondary_route_begin_late'),
    ('func_80093E18',0x80093EA4,'baseline_secondary_route_end_late'),
    ('func_80093864',0x80093A9C,'baseline_secondary_route_arm'),
    ('func_80093864',0x80093AA4,'baseline_secondary_route_disarm'),
]
# No original branch targets 93D48/93EA4. 93AA4 is also the target of the 93A74 null-model
# branch, so the disarm must follow both labels.
END_LABELS = {0x80093D48:('after_16',),0x80093EA4:('after_7',),0x80093AA4:('after_31','L_80093AA4')}
# The three mode0 primary calls of 93864 in 93AB8 receive nothing.
MODE0_SITES = [('func_80093AB8',0x80093CE8),('func_80093AB8',0x80093D8C),('func_80093AB8',0x80093DD8)]

def add_secondary_hooks(hooks, selected):
    assert type(selected) is bool
    if not selected:return
    for func,pc,name in SECONDARY_HOOKS:
        # None of these PCs composes with another hook.
        assert not [h for h in hooks if h['func']==func and h.get('before_vram')==pc],(func,hex(pc))
        hooks.append(dict(func=func,before_vram=pc,text=name+'(rdram, ctx);'))

def verify_secondary_placement(after, selected):
    """Each call exactly once, directly before its PC comment; ends/disarm directly after their
    generated labels; nothing at the mode0 sites."""
    assert type(selected) is bool
    for func,pc,name in SECONDARY_HOOKS:
        body=after[func]; call='    '+name+'(rdram, ctx);\n'
        assert body.count(call)==int(selected),(func,name)
        if not selected:continue
        assert call+'    // 0x%08X:'%pc in body,(func,hex(pc))
        if pc in END_LABELS:
            labels=''.join(r'[ \t]*%s:\n'%re.escape(label) for label in END_LABELS[pc])
            assert re.search(labels+re.escape(call),body),(func,hex(pc),END_LABELS[pc])
    for func,pc in MODE0_SITES:
        assert not re.search(r'baseline_secondary_\w+\(rdram, ctx\);\n    // 0x%08X:'%pc,after[func]),(func,hex(pc))

def prove_secondary_inverse(before,after,selected):
    """Removing exactly the six calls restores the otherwise-identical selected parent bodies."""
    for func in sorted({f for f,_,_ in SECONDARY_HOOKS}):
        restored=after[func]
        for owner,pc,name in SECONDARY_HOOKS:
            if owner!=func:continue
            call='    '+name+'(rdram, ctx);\n'
            assert restored.count(call)==int(selected),(func,name)
            restored=restored.replace(call,'')
        assert restored==before[func],func
