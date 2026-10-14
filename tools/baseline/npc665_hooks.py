"""Three argument transports; no original instruction/body omission."""
import re

HOOKS = [('func_8010108C',0x801010E8,'baseline_npc665_capture'),
         ('func_8010381C',0x801038A0,'baseline_npc665_retire'),
         ('func_800DE498',0x800DEFA0,'baseline_npc665_admit')]

def add_hooks(hooks, selected):
    assert type(selected) is bool
    if not selected:return
    for func,pc,name in HOOKS:
        same=[h for h in hooks if h['func']==func and h.get('before_vram')==pc]
        assert len(same)<=1,(func,hex(pc))
        call=name+'(rdram, ctx);'
        if same:
            assert pc==0x800DEFA0 and call not in same[0]['text']
            # Prefix preserves accepted CPU625's last-call/PC adjacency.
            same[0]['text']=call+'\n    '+same[0]['text']
        else:hooks.append(dict(func=func,before_vram=pc,text=call))

def strip_hooks(before, after, selected):
    assert type(selected) is bool
    restored=dict(after)
    for func,pc,name in HOOKS:
        call='    '+name+'(rdram, ctx);\n'
        assert restored[func].count(call)==int(selected),(func,name)
        if selected:
            tail=(r'(?:    baseline_(?:actor_cpu_root_begin|actor_model_observer|cpu625_admit)\(rdram, ctx\);\n)*'
                  if pc==0x800DEFA0 else '')
            assert re.search(re.escape(call)+tail+re.escape('    // 0x%08X:'%pc),restored[func])
            restored[func]=restored[func].replace(call,'')
        if pc!=0x800DEFA0:assert restored[func]==before[func],func
    return restored
