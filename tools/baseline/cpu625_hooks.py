"""Authored insertion helper for prepare_camera_bridge.prepare_raw; never executed by W2.
Call before config serialization; selected flag must propagate through compile/link,
registry/providers/receipts and native adapters. No original omissions are added.
"""
CPU625_HELPERS = ['tooie_cpu625_begin','tooie_cpu625_admit',
                  'tooie_cpu625_end','tooie_cpu625_retire']
CPU625_HOOKS = [
    ('babackpack_entrypoint_3',0x808003FC,'baseline_cpu625_begin'),
    ('babackpack_entrypoint_3',0x80800404,'baseline_cpu625_end'),
    ('babackpack_set_state',0x8080053C,'baseline_cpu625_set_state'),
    ('babackpack_entrypoint_9',0x808004D4,'baseline_cpu625_init'),
    ('func_800DE498',0x800DEFA0,'baseline_cpu625_admit'),
]
# N3, selected only with ckpack copy storage: the fallback-shadow attachment's direct DE448 in A6250.
# A66BC is the jal itself (not its A66C0 delay slot); A66C4 is reached from the DE448 return and
# from the A6674 skip branch, so its call must follow the generated label (verify_attach_placement).
ATTACH_HOOKS = [
    ('func_800A6250',0x800A66BC,'baseline_cpu625_attach_begin'),
    ('func_800A6250',0x800A66C4,'baseline_cpu625_attach_end'),
]
def add_cpu625_hooks(hooks, selected, attachment=False):
    assert type(selected) is bool and type(attachment) is bool and (selected or not attachment)
    if not selected:return
    for func,pc,name in CPU625_HOOKS+(ATTACH_HOOKS if attachment else []):
        same=[h for h in hooks if h['func']==func and h.get('before_vram')==pc]
        assert len(same)<=1,(func,hex(pc))
        call=name+'(rdram, ctx);'
        if same:
            # Only DEFA0 may compose with the existing065E/observer transport.
            assert pc==0x800DEFA0 and call not in same[0]['text']
            same[0]['text']+='\n    '+call
        else:hooks.append(dict(func=func,before_vram=pc,text=call))

def verify_attach_placement(after, attachment):
    """Generated A6250: each N3 call exactly once, directly before its PC comment; the A66C4 call
    after the generated L_800A66C4 label (executed on the return and the skip-branch legs)."""
    assert type(attachment) is bool
    body=after['func_800A6250']
    for func,pc,name in ATTACH_HOOKS:
        call='    '+name+'(rdram, ctx);\n'
        assert body.count(call)==int(attachment),(func,name)
        if attachment:assert call+'    // 0x%08X:'%pc in body,(func,hex(pc))
    if attachment:
        assert 'L_800A66C4:\n    baseline_cpu625_attach_end(rdram, ctx);\n' in body, 'A66C4 end must follow its branch label'

def prove_cpu625_inverse(before,after,selected):
    names={f for f,_,_ in CPU625_HOOKS}
    for func in names:
        restored=after[func]
        for owner,pc,name in CPU625_HOOKS:
            if owner!=func:continue
            call='    '+name+'(rdram, ctx);\n'
            assert restored.count(call)==int(selected),(func,name)
            if selected:
                # PC adjacency after all same-PC composed hooks must be checked by
                # existing generator regex, in addition to this exact inverse.
                restored=restored.replace(call,'')
        # Compare against the otherwise-identical selected current camera parent,
        # not pristine raw DE498 which lacks accepted camera/065E/observer hooks.
        assert restored==before[func],func
