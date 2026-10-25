"""Source-authenticated four transition fade return witnesses; no rendering changes."""
import re
FUNCTION='gctransitionDll_entrypoint_0'
SITES=(0x80800734,0x80800778,0x808007F0,0x80800834)
def add_hooks(hooks,selected):
    assert type(selected) is bool
    if not selected:return
    for pc in SITES:
        assert not any(h['func']==FUNCTION and h.get('before_vram')==pc for h in hooks),(FUNCTION,hex(pc))
        hooks.append(dict(func=FUNCTION,before_vram=pc,text='baseline_transition_fade_%08x(rdram, ctx);'%pc))
def strip_and_verify(after,selected):
    assert type(selected) is bool
    body=after[FUNCTION]
    for pc in SITES:
        call='    baseline_transition_fade_%08x(rdram, ctx);\n'%pc
        assert body.count(call)==int(selected),(FUNCTION,hex(pc))
        if selected:
            marker='    // 0x%08X:'%pc
            assert call+marker in body,(FUNCTION,hex(pc),'placement')
            begin=body.index('    // 0x%08X:'%(pc-8));end=body.index(call,begin)
            segment=body[begin:end]
            assert 'jal         0x800B99CC' in segment and 'func_800B99CC(rdram, ctx);' in segment
            assert '    // 0x%08X: nop'%(pc-4) in segment,(FUNCTION,hex(pc),'delay')
            if pc==0x80800834:
                assert '    // 0x808007F8: addiu       $at, $zero, 0x9' in body
                assert '    // 0x808007FC: bne         $v0, $at, L_80800834' in body
                assert body.count('L_80800834:')==1
            else:
                assert not re.search(r'^L_%08X:'%pc,body,re.M),(FUNCTION,hex(pc),'return-only')
            body=body.replace(call,'')
    after[FUNCTION]=body
    return after
