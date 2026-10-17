"""Explicit default-OFF camera image preparation; accepted OFF outputs stay separate."""
import json
import re
from pathlib import Path
import toml
import prepare_render_bridge as accepted
import prepare_silo_bridge as held
import prepare_cpu625 as cpu_owner
import prepare_minimum_host as host_owner
import minimum_host_hooks as host_hooks
import prepare_npc665 as npc_owner
import prepare_backpack621 as backpack_owner
import prepare_graphics_rate as rate_owner
import prepare_quiet_talon_trot as quiet_owner
from npc665_hooks import add_hooks as add_npc_hooks, strip_hooks as strip_npc_hooks, HOOKS as NPC_HOOKS
from cpu625_hooks import add_cpu625_hooks, CPU625_HOOKS
sha,win=accepted.sha,accepted.win
SOURCE='tools/baseline/guest/camera-candidate'
OUTPUT='build/windows-dev/baseline/render-bridge-camera-candidate/composition-v2'
DEMO_HELPERS=['tooie_demo_begin','tooie_demo_record','tooie_demo_wait']
PRE_HELPERS=['tooie_pre_background_reset','tooie_pre_background_begin','tooie_pre_background_end']
SHADOW_HELPERS=['tooie_shadow_reset','tooie_shadow_source','tooie_shadow_prepare','tooie_shadow_de498','tooie_shadow_rectangle_args','tooie_shadow_mode_begin','tooie_shadow_mode_end','tooie_shadow_complete']
CPU_ROOT_HELPERS=['tooie_actor_cpu_root_begin','tooie_camera_cpu_root_begin']
FEATHER_LEAF_HELPER='tooie_actor_feather_leaf_id'
FEATHER_LEAF_PATCH='func_800DD024'

def write_if_changed(path,text):
    data=text.encode('utf-8')
    if not path.exists() or path.read_bytes()!=data:path.write_bytes(data)

def prepare_honey_containment(root, output, run, npc665=False, backpack621=False):
    """Exact camera-only insertion derivative; the accepted OFF bridge stays untouched."""
    from elftools.elf.elffile import ELFFile
    from elftools.elf.relocation import RelocationSection
    original=root/'tools/baseline/guest/render_bridge.c'
    assert sha(original)=='d0041b2ddda17415a61f44975be125520d9ab7b65c69b7743a4faf2208155969'
    text=original.read_bytes().decode('utf-8'); insertions=[]
    def insert(anchor, addition, after=True):
        nonlocal text
        assert text.count(anchor)==1, ('Honey exact insertion anchor',anchor)
        replacement=anchor+addition if after else addition+anchor
        text=text.replace(anchor,replacement,1);insertions.append((anchor,addition,after))
    insert('#include "rt64_extended_gbi.h"\n', '#define TOOIE_CAMERA_GUEST 1\n#include "camera_identity.h"\n')
    insert('    int admitted = 0, owns_active = 0;\n',
           '    u32 camera_owner = 0, camera_base = 0, camera_returned = 0;\n'
           '    volatile struct TooieCameraState* camera_state = &tooie_camera_state;\n')
    insert('                else if ((current & 7u) != 0 || (base & 7u) != 0) reason = TOOIE_BRIDGE_ALIGN;\n',
           '                else if (base != (camera_base = (u32)&tooie_camera_main[index][0]) ||\n'
           '                         !(camera_owner = camera_state->owner_frame) || camera_state->control.active != 1u ||\n'
           '                         camera_state->control.epoch != epoch || camera_state->control.main_base != camera_base ||\n'
           '                         camera_state->control.main_end != camera_base + TOOIE_CAMERA_M_BYTES ||\n'
           '                         current < camera_base || current > camera_base + TOOIE_CAMERA_M_BYTES ||\n'
           '                         camera_base + TOOIE_CAMERA_M_BYTES - current < 24u) reason = TOOIE_BRIDGE_RANGE;\n')
    pop='        cursor = (GfxCommand*)(u32)*store;\n        gEXPopMatrixGroup(cursor, 0);\n        *store = (s32)(u32)(cursor + 1);\n'
    insert(pop,
           '        camera_returned = (u32)*store;\n'
           '        if (camera_state->owner_frame == camera_owner && tooie_render_bridge_state.epoch == epoch &&\n'
           '            camera_state->control.epoch == epoch && D_8007BCD0[index] == camera_base &&\n'
           '            !(camera_returned & 7u) && camera_returned >= camera_base &&\n'
           '            camera_returned <= camera_base + TOOIE_CAMERA_M_BYTES &&\n'
           '            camera_base + TOOIE_CAMERA_M_BYTES - camera_returned >= 8u) {\n',after=False)
    insert(pop,
           '        } else {\n'
           '            tooie_render_bridge_state.poisoned = 1;\n'
           '            count(&tooie_render_bridge_state.poison_count);\n'
           '            camera_state->control.latch = 1;camera_state->diagnostics.refusals++;\n'
           '            if (!camera_state->diagnostics.first_reason) {\n'
           '                camera_state->diagnostics.first_reason = 0x101u;\n'
           '                camera_state->diagnostics.raw0 = camera_returned;\n'
           '                camera_state->diagnostics.raw1 = camera_base;\n'
           '                camera_state->diagnostics.raw2 = camera_base + TOOIE_CAMERA_M_BYTES;\n'
           '                camera_state->diagnostics.raw3 = 8u;\n'
           '            }\n'
           '        }\n')
    inverse=text
    for anchor,addition,after in reversed(insertions):
        replacement=anchor+addition if after else addition+anchor
        assert inverse.count(replacement)==1
        inverse=inverse.replace(replacement,anchor,1)
    assert inverse.encode('utf-8')==original.read_bytes()
    # The existing replay span diagnostic defines the exact 0x101/raw order.
    replay=(root/SOURCE/'camera_replay.c').read_text()
    assert '#define CAM_REFUSE_REPLAY_SPAN 0x101u' in replay
    assert 'raw0=cursor;' in replay and 'raw1=c->main_base;' in replay and 'raw2=c->main_end;' in replay and 'raw3=bytes;' in replay
    derivative=output/'camera_honey_bridge.c';derivative.write_bytes(text.encode('utf-8'))
    compiler=root/'deps/LLVM-19.1.3-Windows-X64/bin/clang.exe'
    includes=[root/'tools/baseline/guest/silo-candidate',root/'tools/baseline/guest',root/'deps/banjo-tooie/include',root/'deps/banjo-tooie/lib/ultralib/include',root/'deps/rt64/include']
    flags=[compiler,'-target','mips','-EB','-mips2','-mabi=32','-O2','-G0','-mno-abicalls','-mno-odd-spreg','-nostdinc','-D_LANGUAGE_C','-DMIPS','-fno-builtin']+['-I'+win(p) for p in includes]+['-I'+win(root/SOURCE)]
    flags+=['-DTOOIE_NPC665='+str(int(npc665)),'-DTOOIE_BACKPACK621='+str(int(backpack621))]
    object_path=output/'camera_honey_bridge.o'
    run(flags+['-c',win(derivative),'-o',win(object_path)],'camera-honey-compile.log')
    prior=root/'build/windows-dev/baseline/render-bridge-silo-candidate/bridge.o'
    def facts(path):
        with path.open('rb') as stream:
            elf=ELFFile(stream);table=elf.get_section_by_name('.symtab');symbols={s.name:s for s in table.iter_symbols() if s.name}
            fn=symbols['func_800EBED4'];section=elf.get_section(fn['st_shndx']);begin=fn['st_value'];end=begin+fn['st_size'];relocs=[]
            for relsec in elf.iter_sections():
                if isinstance(relsec,RelocationSection) and relsec['sh_info']==fn['st_shndx']:
                    linked=elf.get_section(relsec['sh_link'])
                    for rel in relsec.iter_relocations():
                        if begin<=rel['r_offset']<end:
                            relocs.append((rel['r_offset']-begin,rel['r_info_type'],linked.get_symbol(rel['r_info_sym']).name,rel.entry.get('r_addend',None)))
            return dict(definitions=[s.name for s in table.iter_symbols() if s['st_info']['type']=='STT_FUNC' and s['st_shndx']!='SHN_UNDEF'],imports={s.name for s in table.iter_symbols() if s.name and s['st_shndx']=='SHN_UNDEF'},ebed4=section.data()[begin:end].hex(),ebed4_relocations=relocs,state=elf.get_section_by_name('.tooie_render_bridge_state').data().hex())
    before,after=facts(prior),facts(object_path)
    assert after['definitions']==before['definitions']==['func_800EBED4','func_80800380_chhoneycarrier']
    assert after['imports']==before['imports']|{'tooie_camera_state','tooie_camera_main'}
    for name in ('ebed4','ebed4_relocations','state'):assert after[name]==before[name],('Honey preserved object fact',name)
    record=dict(source_sha256=sha(original),derivative_sha256=sha(derivative),object_sha256=sha(object_path),prior_object_sha256=sha(prior),pure_insertion_byte_inverse=True,insertions=insertions,definitions=after['definitions'],imports=sorted(after['imports']),ebed4_bytes_relocations_equal=True,bridge_state_equal=True,explicit_suffix_upper_bound=True)
    (output/'HONEY_CONTAINMENT_BINDING.json').write_text(json.dumps(record,indent=2)+'\n')
    return object_path

def roster(root):
    rows=json.loads((root/SOURCE/'camera_roster.json').read_text())['owners']
    assert len(rows)==26 and len({r['name'] for r in rows})==26
    assert {'func_8001A3A0','func_8001A324','func_800DE498','ncpod_entrypoint_12','ncpod_entrypoint_14'} <= {r['name'] for r in rows}
    return rows

def prepare_raw(root,output,run,shadow_observer=False,demo_timing=False,shadow_fix=False,pre_background=False,actor_palette=False,cpu625=False,minimum_host=False,npc665=False,backpack621=False,quiet_talon_trot=False):
    assert type(quiet_talon_trot) is bool and (not quiet_talon_trot or (backpack621 and minimum_host))
    assert type(backpack621) is bool and (not backpack621 or cpu625), "621 requires existing selected CPU625 transports"
    parent=root/'build/windows-dev/baseline/render-bridge-silo-candidate/raw.toml'
    parent_receipt=json.loads((parent.parent/'RAW_BINDING.json').read_text())
    for name,digest in parent_receipt['outputs'].items():
        assert sha(parent.parent/'raw'/name)==digest, ('current helper raw input',name)
    before=accepted.bodies(root/'build/windows-dev/baseline/raw')
    rows=roster(root);contract=json.loads((root/SOURCE/'camera_hook_contract.json').read_text());branch=contract['branches']['hook']
    functions=list(dict.fromkeys(held.GENERATED_FUNCTIONS+branch['providers']));omitted=list(dict.fromkeys(held.FUNCTIONS[3:]+branch['omissions']))
    if actor_palette:functions.append(FEATHER_LEAF_PATCH);omitted.append(FEATHER_LEAF_PATCH)
    if minimum_host:functions.append(host_owner.PROJECTION);omitted.append(host_owner.PROJECTION)
    if backpack621:functions.append(rate_owner.BODY);omitted.append(rate_owner.BODY)
    if quiet_talon_trot:functions.append(quiet_owner.BODY);omitted.append(quiet_owner.BODY)
    assert set(held.GENERATED_FUNCTIONS)<=set(functions) and len(set(omitted))==len(omitted)
    assert 'func_800DE498' not in omitted
    config=toml.loads(parent.read_text());patches=config['patches'];hooks=patches['hook']
    edits=patches.get('instruction',[])
    allocation=[e for e in edits if e.get('func')=='func_8001A3A0']
    assert allocation==[dict(func='func_8001A3A0',vram=pc,value=0x34048FC0) for pc in (0x8001A3BC,0x8001A3CC)]
    patches['instruction']=[e for e in edits if e not in allocation]
    assert not any(e.get('func') in omitted for kind in ('hook','instruction') for e in patches.get(kind,[])), 'omitted camera body already has hook/instruction'
    for pc,name in [(0x800A78B0,'baseline_first_sync_returned'),(0x800A78E8,'baseline_first_graphics_submit')]:
        assert [h['text'] for h in hooks if h['func']=='func_800A7840' and h.get('before_vram')==pc]==[name+'(rdram, ctx);']
    epoch_hooks=[h for h in hooks if h['func']=='func_800A7840' and h.get('before_vram')==0x800A7888]
    assert len(epoch_hooks)==1 and epoch_hooks[0]['text']=='baseline_render_construction_begin(rdram, ctx);'
    epoch_hooks[0]['text']+='\n    baseline_camera_construction_begin(rdram, ctx);'
    for func,pc,text in [('func_800A7840',0x800A7900,'baseline_camera_construction_end(rdram, ctx);'),('func_800DE498',0x800DEFB0,'baseline_camera_de498_ignore(rdram, ctx);')]:
        assert not any(h['func']==func and h.get('before_vram')==pc for h in hooks)
        hooks.append(dict(func=func,before_vram=pc,text=text))
    shadow_hooks=[('func_800A6DAC',0x800A6EC0,'baseline_shadow_branch'),
                  ('func_800A6DAC',0x800A6EE4,'baseline_shadow_end'),
                  ('func_800DE498',0x800DEFE0,'baseline_shadow_projection'),
                  ('func_800A6250',0x800A69D4,'baseline_shadow_rectangle'),
                  ('func_800A6B24',0x800A6CF0,'baseline_shadow_local'),
                  ('func_800A6B24',0x800A6D60,'baseline_shadow_fallback'),
                  ('func_800BFD6C',0x800BFE70,'baseline_pre_slot')]
    shadow_hooks += [('func_80800D8C_chnests',0x80800EBC,'baseline_feather_stable_begin'),('func_80800D8C_chnests',0x80800EC4,'baseline_feather_stable_end'),('func_800DE498',0x800DEFA0,'baseline_actor_model_observer')]
    if shadow_observer:
        for func,pc,name in shadow_hooks:
            assert not any(h['func']==func and h.get('before_vram')==pc for h in hooks)
            hooks.append(dict(func=func,before_vram=pc,text=name+'(rdram, ctx);'))
    cpu_call='baseline_actor_cpu_root_begin(rdram, ctx);'
    if actor_palette:
        same=[h for h in hooks if h['func']=='func_800DE498' and h.get('before_vram')==0x800DEFA0]
        if same:
            assert len(same)==1 and same[0]['text']=='baseline_actor_model_observer(rdram, ctx);'
            same[0]['text']=cpu_call+'\n    '+same[0]['text']
        else:hooks.append(dict(func='func_800DE498',before_vram=0x800DEFA0,text=cpu_call))
    shadow_fix_hooks=[('func_800A6B24',0x800A6CF0,'baseline_shadow_source_local'),('func_800A6B24',0x800A6D60,'baseline_shadow_source_fallback'),
                      ('func_800A6250',0x800A6664,'baseline_shadow_prepare'),('func_800A6250',0x800A69D4,'baseline_shadow_fullframe'),
                      ('func_800A5EC4',0x800A5F84,'baseline_shadow_mode_begin'),('func_800A5EC4',0x800A6180,'baseline_shadow_mode_end'),
                      ('func_800A6250',0x800A69DC,'baseline_shadow_complete')]
    if shadow_fix:
        for func,pc,name in shadow_fix_hooks:
            existing=[h for h in hooks if h['func']==func and h.get('before_vram')==pc];assert len(existing)<=1
            if existing:existing[0]['text']=name+'(rdram, ctx);\n    '+existing[0]['text']
            else:hooks.append(dict(func=func,before_vram=pc,text=name+'(rdram, ctx);'))
    pre_hooks=[('func_800BFD6C',0x800BFE6C,'baseline_pre_background_begin'),('func_800BFD6C',0x800BFE78,'baseline_pre_background_end'),('func_800BFF28',0x800BFF28,'baseline_pre_background_reset'),('func_800BFF4C',0x800BFF4C,'baseline_pre_background_reset')]
    if pre_background:
        for func,pc,name in pre_hooks:
            assert not any(h['func']==func and h.get('before_vram')==pc for h in hooks)
            hooks.append(dict(func=func,before_vram=pc,text=name+'(rdram, ctx);'))
    demo_hooks=[('func_8001608C',0x8001608C,'baseline_demo_begin'),('func_8001608C',0x800160F8,'baseline_demo_record'),('func_800151BC',0x8001520C,'baseline_demo_first'),('func_800151BC',0x8001524C,'baseline_demo_loop')]
    if demo_timing:
        for func,pc,name in demo_hooks:
            assert not any(h['func']==func and h.get('before_vram')==pc for h in hooks)
            hooks.append(dict(func=func,before_vram=pc,text=name+'(rdram, ctx);'))
    add_cpu625_hooks(hooks,cpu625)
    host_hooks.add_hooks(hooks,minimum_host)
    add_npc_hooks(hooks,npc665)
    for name in omitted:
        assert name in before
        if name not in patches['ignored']:patches['ignored'].append(name)
    # 1A3A0 is a candidate C owner: never apply the accepted raw allocation edits.
    assert not any(e.get('vram') in (0x8001A3BC,0x8001A3CC) for e in patches.get('instruction',[]))
    destination=output/'raw';existing=destination.exists();destination.mkdir(exist_ok=True)
    config['input']['output_func_path']=str(destination);serialized=toml.dumps(config);config_path=output/'raw.toml'
    config_path.write_text(serialized);run([root/'deps/N64Recomp-codegen/build/N64Recomp',config_path],'raw-codegen.log')
    actual_after=accepted.bodies(destination)
    after=host_hooks.strip_and_verify(actual_after,minimum_host)
    after=strip_npc_hooks(before,after,npc665)
    for func,pc,name in CPU625_HOOKS:
        call="    "+name+"(rdram, ctx);\n"
        assert after[func].count(call)==int(cpu625),(func,name)
        if cpu625:
            assert call+"    // 0x%08X:"%pc in after[func],(func,hex(pc))
            after[func]=after[func].replace(call,"")
    assert set(before)-set(after)==set(omitted) and not set(after)-set(before)
    assert not set(functions).intersection(after)
    changed=sorted(n for n in after if after[n]!=before[n]);expected_changed=['func_800A7840','func_800DE498']+(['func_800A6250','func_800A6B24','func_800A5EC4'] if shadow_fix else [])+(['func_800A6250','func_800A6DAC','func_800A6B24','func_800BFD6C','func_80800D8C_chnests'] if shadow_observer else [])+(['func_8001608C','func_800151BC'] if demo_timing else [])+(['func_800BFD6C','func_800BFF28','func_800BFF4C'] if pre_background else []);assert changed==sorted(set(expected_changed)),changed
    if demo_timing:
        for func in ['func_8001608C','func_800151BC']:
            restored=after[func]
            for owner,pc,name in demo_hooks:
                if owner==func:
                    call='    '+name+'(rdram, ctx);\n';assert restored.count(call)==1 and call+'    // 0x%08X:'%pc in restored
                    restored=restored.replace(call,'')
            assert restored==before[func]
    hook='    baseline_render_construction_begin(rdram, ctx);\n';begin='    baseline_camera_construction_begin(rdram, ctx);\n';end='    baseline_camera_construction_end(rdram, ctx);\n';ignore='    baseline_camera_de498_ignore(rdram, ctx);\n'
    body=after['func_800A7840'];de=after['func_800DE498']
    assert body.count(hook)==body.count(begin)==body.count(end)==de.count(ignore)==1
    assert body.replace(hook,'').replace(begin,'').replace(end,'')==before['func_800A7840']
    de_original=de
    if actor_palette:
        call='    '+cpu_call+'\n';assert de.count(call)==1 and re.search(re.escape(call)+r'(?:    baseline_actor_model_observer\(rdram, ctx\);\n)?    // 0x800DEFA0:',de)
        de_original=de_original.replace(call,'')
    else:assert cpu_call not in de
    if shadow_observer or shadow_fix or pre_background:
        selected_shadow_hooks=(shadow_hooks if shadow_observer else [])+(shadow_fix_hooks if shadow_fix else [])+(pre_hooks if pre_background else [])
        for func,pc,name in selected_shadow_hooks:
            call='    '+name+'(rdram, ctx);\n'
            assert after[func].count(call)==1 and re.search(re.escape(call)+r'(?:    baseline_shadow_\w+\(rdram, ctx\);\n)*'+re.escape('    // 0x%08X:'%pc),after[func])
        for func in sorted({f for f,_,_ in selected_shadow_hooks}-{'func_800DE498'}):
            restored=after[func]
            for owner,_,name in selected_shadow_hooks:
                if owner==func:restored=restored.replace('    '+name+'(rdram, ctx);\n','')
            assert restored==before[func]
        de_original=de_original.replace('    baseline_shadow_projection(rdram, ctx);\n','').replace('    baseline_actor_model_observer(rdram, ctx);\n','')
    assert de_original.replace(ignore,'')==before['func_800DE498']
    assert hook+begin+'    // 0x800A7888: jal' in body
    assert body.rindex('func_8001A4B8(rdram, ctx);')<body.index(hook)
    assert '    after_10:\n'+end+'    // 0x800A7900:' in body
    assert 'L_800A7900:' not in body
    assert '    skip_19:\n'+ignore+'    // 0x800DEFB0:' in de
    assert 'L_800DEFB0:' not in de
    assert {'_ncpod_entrypoint_12','_ncpod_entrypoint_14'} <= set(after)
    return dict(quiet_talon_trot=quiet_talon_trot,backpack621=backpack621,condition='BASELINE-CAMERA-DEFAULT-OFF-CANDIDATE',parent_sha256=sha(parent),definitions=functions,omitted=omitted,changed=changed,changed_scope='After exact CPU625/HOST/NPC665 insertion inverses; legacy camera changes only',actual_changed=sorted(n for n in actual_after if actual_after[n]!=before[n]),actual_unchanged_body_count=sum(actual_after[n]==before[n] for n in actual_after),original_body_count=len(before),child_body_count=len(after),unchanged_body_count=len(after)-len(changed),npc665=npc665,npc665_hook_pcs=[hex(pc) for _,pc,_ in NPC_HOOKS] if npc665 else [],npc665_changed=sorted({f for f,_,_ in NPC_HOOKS}) if npc665 else [],minimum_host=minimum_host,host_hook_functions=[host_hooks.GLOBAL_FUNCTION,host_hooks.CUTSCENE_FUNCTION] if minimum_host else [],cpu625=cpu625,cpu625_hook_pcs=[hex(pc) for _,pc,_ in CPU625_HOOKS] if cpu625 else [],cpu625_changed=sorted({f for f,_,_ in CPU625_HOOKS}) if cpu625 else [],actor_palette=actor_palette,demo_timing=demo_timing,demo_hook_pcs=[hex(pc) for _,pc,_ in demo_hooks] if demo_timing else [],shadow_observer=shadow_observer,shadow_hook_pcs=[hex(pc) for _,pc,_ in shadow_hooks] if shadow_observer else [],pre_background=pre_background,pre_hook_pcs=[hex(pc) for _,pc,_ in pre_hooks] if pre_background else [],shadow_fix=shadow_fix,shadow_fix_hook_pcs=[hex(pc) for _,pc,_ in shadow_fix_hooks] if shadow_fix else [],hook_pcs=['800A7888','800A7900','800DEFB0'],helper_lookup_contract=branch['helper_lookup'],contract_sha256=sha(root/SOURCE/'camera_hook_contract.json'),allocation_instruction_edits=False,roster_sha256=sha(root/SOURCE/'camera_roster.json'),original_raw={p.name:sha(p) for p in (root/'build/windows-dev/baseline/raw').iterdir() if p.is_file()},outputs={p.name:sha(p) for p in destination.iterdir() if p.is_file()})

def write_registry_header(root,output,actor_palette,demo,shadow,pre,cpu=None,host=None,npc=None,backpack=None,quiet_talon_trot=False):
    assert type(actor_palette) is bool, 'Explicit registry actor selection'
    rows=roster(root)
    rows=[row for row in rows if row['name']!='func_800DE498']
    assert len(rows)==25
    sections={sec['name']:sec for sec in toml.loads((root/'config/selected.syms.toml').read_text())['section']}
    for name in held.LIFECYCLE_FUNCTIONS:
        matches=[(sec,fn) for sec in sections.values() for fn in sec.get('functions',[]) if fn['name']==name]
        assert len(matches)==1, ('selected lifecycle owner',name,matches)
        sec,fn=matches[0]
        rows.append(dict(name=name,section=sec['name'],vram=hex(fn['vram']),section_offset=hex(fn['vram']-sec['vram']),extent=hex(fn['size'])))
    if host:
        matches=[(sec,fn) for sec in sections.values() for fn in sec.get('functions',[]) if fn['name']==host_owner.PROJECTION]
        assert len(matches)==1
        sec,fn=matches[0];rows.append(dict(name=fn['name'],section=sec['name'],vram=hex(fn['vram']),section_offset=hex(fn['vram']-sec['vram']),extent=hex(fn['size'])))
    if backpack:
        matches=[(sec,fn) for sec in sections.values() for fn in sec.get('functions',[]) if fn['name']==rate_owner.BODY]
        assert len(matches)==1
        sec,fn=matches[0];rows.append(dict(name=fn['name'],section=sec['name'],vram=hex(fn['vram']),section_offset=hex(fn['vram']-sec['vram']),extent=hex(fn['size'])))
    assert len(rows)==27+int(bool(host))+int(bool(backpack)) and len({r['name'] for r in rows})==len(rows)
    contract=json.loads((root/SOURCE/'camera_hook_contract.json').read_text())['branches']['hook']
    base_functions=list(held.GENERATED_FUNCTIONS)
    if actor_palette:base_functions.insert(base_functions.index('tooie_actor_palette_end'),CPU_ROOT_HELPERS[0]);base_functions.insert(base_functions.index('tooie_actor_palette_end'),FEATHER_LEAF_HELPER)
    helpers={held.HELPER:1,'tooie_player_begin':2,'tooie_player_end':2,'tooie_player_bone_push':2,'tooie_player_bone_pop':2,'tooie_player_leaf_begin':2,'tooie_player_leaf_end':2,'tooie_camera_construction_begin':3,'tooie_camera_construction_end':3,'tooie_camera_de498_ignore':3,'tooie_actor_palette_begin':4,'tooie_actor_palette_end':4}
    if actor_palette:helpers.update({CPU_ROOT_HELPERS[0]:4,CPU_ROOT_HELPERS[1]:3,FEATHER_LEAF_HELPER:4})
    lines=['// Generated from the exact frozen camera roster and selected section identities.',
           'static constexpr uint32_t camera_actor_palette_selected = %du;'%int(actor_palette)]
    lines+=['static CameraAlias camera_aliases[] = {']
    for row in rows:
        sec=sections[row['section']];offset=int(row['section_offset'],16);extent=int(row['extent'],16)
        assert int(row['vram'],16)==sec['vram']+offset and offset+extent<=sec['size']
        lines.append('    {baseline_boot_patch_registry::%s,0x%Xu,0x%Xu,0x%Xu,0x%Xu,0x%Xu},'%(row['name'],sec['rom'],sec['vram'],sec['size'],offset,extent))
    lines+=['};','static CameraPatchExpected camera_expected_patches[] = {']
    expected_names=base_functions+contract['providers']+([CPU_ROOT_HELPERS[1],FEATHER_LEAF_PATCH] if actor_palette else [])+(DEMO_HELPERS if demo else [])+(SHADOW_HELPERS if shadow else [])+(PRE_HELPERS if pre else [])+(cpu_owner.HELPERS if cpu else [])+([host_owner.PROJECTION,host_owner.HELPER] if host else [])+([npc_owner.CHECKED if n==npc_owner.END else n for n in npc_owner.HELPERS] if npc else [])
    expected_names+=(backpack_owner.HELPERS+[rate_owner.BODY]) if backpack else []
    expected_names+=[quiet_owner.BODY] if quiet_talon_trot else []
    for name in dict.fromkeys(expected_names):
        section=(5+int(bool(demo))+int(bool(shadow))+int(bool(pre))+int(bool(cpu))+int(bool(host))) if name in npc_owner.HELPERS or name==npc_owner.CHECKED else (5+int(bool(demo))+int(bool(shadow))+int(bool(pre))+int(bool(cpu))) if name==host_owner.HELPER else (5+int(bool(demo))+int(bool(shadow))+int(bool(pre))) if name in cpu_owner.HELPERS else (5+int(bool(demo))+int(bool(shadow))) if name in PRE_HELPERS else 6 if name in SHADOW_HELPERS else 5 if name in DEMO_HELPERS else helpers.get(name,0)
        if name in backpack_owner.HELPERS:section=5+int(bool(demo))+int(bool(shadow))+int(bool(pre))+int(bool(cpu))+int(bool(host))+int(bool(npc))
        lines.append('    {baseline_boot_patch_registry::%s,%d},'%(name,section))
    lines+=['};']
    layout='#pragma once\n#define TOOIE_DEMO_TIMING_SELECTED %du\n'%int(bool(demo))
    if demo:
        for key,value in demo.items():layout+='#define TOOIE_DEMO_%s 0x%Xu\n'%(key.upper(),value)
    write_if_changed(output/'demo_timing_layout.h',layout)
    layout='#pragma once\n#define TOOIE_SHADOW_SELECTED %du\n'%int(bool(shadow))
    if shadow:
        for key,value in shadow.items():layout+='#define TOOIE_SHADOW_%s 0x%Xu\n'%(key.upper(),value)
    write_if_changed(output/'shadow_projection_layout.h',layout)
    layout='#pragma once\n#define TOOIE_ACTOR_PALETTE_SELECTED %du\n#define TOOIE_PRE_SELECTED %du\n'%(int(actor_palette),int(bool(pre)))
    if pre:
        for key,value in pre.items():layout+='#define TOOIE_PRE_%s 0x%Xu\n'%(key.upper(),value)
        layout+='#define TOOIE_PRE_SECTION %du\n'%(5+int(bool(demo))+int(bool(shadow)))
        overlay=(output/'recomp_overlays.inl').read_text();rows=re.findall(r'\{ \.rom_addr = (0x\w+), \.ram_addr = (0x\w+), \.size = (0x\w+), \.funcs = (\w+),.*?\.relocs = (\w+),.*?\.index = (\d+) \}',overlay)
        assert len(rows)==5+int(bool(demo))+int(bool(shadow))+1+int(bool(cpu))+int(bool(host))+int(bool(npc))+int(bool(backpack))
        tuples=[]
        for rom,ram,size,funcs,relocs,index in rows:
            f=re.search(r'static FuncEntry '+funcs+r'\[\] = \{(.*?)\n\};',overlay,re.S);q=re.search(r'static RelocEntry '+relocs+r'\[\] = \{(.*?)\n\};',overlay,re.S);assert f and q
            tuples.append((int(rom,16),int(ram,16),int(size,16),int(index),f[1].count('.func'),q[1].count('.offset')))
        first=tuples[0];assert (first[0],first[1],first[3],first[4])==(0,0x80801000,1,(39 if actor_palette else 38)+int(bool(host))+int(bool(backpack))+int(quiet_talon_trot))
        shift=first[2]-0x96C8
        fixed=[(0x96C8,0x8080A6C8,0x2E38,2,1,2),(0xC500,0x8080D500,0x15FC,3,6,3),(0xDAFC,0x8080EAFC,0xD10 if actor_palette else 0x68C,4,4 if actor_palette else 3,0),(0xE820 if actor_palette else 0xE19C,0x8080F820 if actor_palette else 0x8080F19C,0x3DB4 if actor_palette else 0x20F0,8,4 if actor_palette else 2,6 if actor_palette else 4)]
        if not npc and not backpack:assert all(actual==(old[0]+shift,old[1]+shift,*old[2:]) for actual,old in zip(tuples[1:5],fixed)), 'PRE fixed section identity and uniform shift'
        elif npc:
            assert [x[3:5] for x in tuples[:5]]==[(1,39+int(bool(host))+int(bool(backpack))+int(quiet_talon_trot)),(2,1),(3,6),(4,4),(8,4)], 'NPC changed shared sections preserve original memberships'
            row=tuples[-2] if backpack else tuples[-1]
            assert row[:4]==(npc['rom'],npc['ram'],npc['size'],npc['index']) and row[4]==8
            npc['sections']=tuples[:5];npc['relocs']=row[5]
        if backpack:
            assert [x[3:5] for x in tuples[:5]]==[(1,39+int(bool(host))+int(bool(backpack))+int(quiet_talon_trot)),(2,1),(3,6),(4,4),(8,4)]
            assert tuples[-1][:4]==(backpack['rom'],backpack['ram'],backpack['size'],backpack['index']) and tuples[-1][4]==8
            backpack['sections']=tuples[:5];backpack['relocs']=tuples[-1][5]
        layout+='#define TOOIE_PRE_TEXT_SIZE 0x%Xu\n#define TOOIE_PRE_TEXT_RELOCS %du\n#define TOOIE_PRE_SHIFT 0x%Xu\n'%(first[2],first[5],shift)
    write_if_changed(output/'pre_background_layout.h',layout)
    layout='#pragma once\n#define TOOIE_CPU625_SELECTED %du\n'%int(bool(cpu))
    if cpu:
        for key,value in cpu.items():layout+='#define TOOIE_CPU625_%s 0x%Xu\n'%(key.upper(),value)
        layout+='#define TOOIE_CPU625_SECTION %du\n'%(5+int(bool(demo))+int(bool(shadow))+int(bool(pre)))
    write_if_changed(output/'cpu625_selection_layout.h',layout)
    layout='#pragma once\n#define TOOIE_MINIMUM_HOST_SELECTED %du\n'%int(bool(host))
    if host:
        for key,value in host.items():layout+='#define TOOIE_MINIMUM_HOST_%s 0x%Xu\n'%(key.upper(),value)
        layout+='#define TOOIE_MINIMUM_HOST_SECTION %du\n'%(5+int(bool(demo))+int(bool(shadow))+int(bool(pre))+int(bool(cpu)))
    write_if_changed(output/'minimum_host_selection_layout.h',layout)
    layout='#pragma once\n#define TOOIE_NPC665_SELECTED %du\n'%int(bool(npc))
    if npc:
        for key in ('rom','ram','size','index','state','state_bytes','end','prior_end','relocs'):
            layout+='#define TOOIE_NPC665_%s 0x%Xu\n'%(key.upper(),npc[key])
        layout+='#define TOOIE_NPC665_SECTION %du\n'%(5+int(bool(demo))+int(bool(shadow))+int(bool(pre))+int(bool(cpu))+int(bool(host)))
        lines+=['static constexpr uint32_t npc665_shared_sections[5][6] = {']
        lines+=['    {'+','.join('0x%Xu'%v for v in row)+'},' for row in npc['sections'][:5]]
        lines+=['};']
    write_if_changed(output/'npc665_selection_layout.h',layout)
    layout='#pragma once\n#define TOOIE_BACKPACK621_SELECTED %du\n'%int(bool(backpack))
    if backpack:
        for key in ('rom','ram','size','index','state','state_bytes','prior_end','relocs'):
            layout+='#define TOOIE_BACKPACK621_%s 0x%Xu\n'%(key.upper(),backpack[key])
        layout+='#define TOOIE_BACKPACK621_SECTION %du\n'%(5+int(bool(demo))+int(bool(shadow))+int(bool(pre))+int(bool(cpu))+int(bool(host))+int(bool(npc)))
        lines+=['static constexpr uint32_t backpack621_shared_sections[5][6] = {']
        lines+=['    {'+','.join('0x%Xu'%v for v in row)+'},' for row in backpack['sections']]
        lines+=['};']
    write_if_changed(output/'backpack621_selection_layout.h',layout)
    write_if_changed(output/'camera_registry_data.hpp','\n'.join(lines)+'\n')

def image_flags(root,pre_background,cpu625,npc665,backpack621):
    """Exact guest compile flags shared by the image and its same-selection 621OFF carrier."""
    compiler=root/'deps/LLVM-19.1.3-Windows-X64/bin/clang.exe';flags=[compiler,'-target','mips','-EB','-mips2','-mabi=32','-O2','-G0','-mno-abicalls','-mno-odd-spreg','-nostdinc','-D_LANGUAGE_C','-DMIPS','-fno-builtin']+(['-DTOOIE_PRE_BACKGROUND=1'] if pre_background else [])+['-I'+win(p) for p in [root/SOURCE,root/'tools/baseline/guest',root/'deps/banjo-tooie/include',root/'deps/banjo-tooie/lib/ultralib/include',root/'deps/rt64/include']]
    return compiler,flags+['-DTOOIE_CPU625='+str(int(cpu625)),'-DTOOIE_NPC665='+str(int(npc665)),'-DTOOIE_BACKPACK621='+str(int(backpack621))]

def compile_backpack621_carrier(root,output,run,actor_palette,pre_background,cpu625,npc665):
    """Fresh same-selection 621OFF owner objects; prepare_image(backpack621=True) proves them unchanged."""
    assert cpu625 and actor_palette and pre_background, "621 existing transport/carrier prerequisite"
    assert not any((output/(name+'.o')).exists() for name in ('player','cpu625','npc665')), "Never replace existing carrier custody"
    _,flags=image_flags(root,pre_background,cpu625,npc665,False)
    held.compile_objects(root,output,run,cpu_root=actor_palette,cpu625=cpu625,npc665=npc665,shared=True,backpack621=False)
    cpu_owner.compile_owner(root,output,run,flags,win)
    if npc665:npc_owner.compile_owner(root,output,run,flags+['-DTOOIE_ACTOR_PALETTE=1'],win)

def prepare_image(root,output,run,actor_palette=False,demo_timing=False,shadow_fix=False,pre_background=False,cpu625=False,minimum_host=False,npc665=False,backpack621=False,shadow_coefficients=False,quiet_talon_trot=False):
    raw_binding=json.loads((output/"RAW_BINDING.json").read_text())
    assert type(quiet_talon_trot) is bool
    raw_quiet=raw_binding.get("quiet_talon_trot",False)
    assert type(raw_quiet) is bool and raw_quiet==quiet_talon_trot, "Raw/image quiet Trot selection mismatch"
    assert not quiet_talon_trot or (backpack621 and shadow_coefficients and minimum_host)
    assert type(backpack621) is bool and raw_binding.get('backpack621',False)==backpack621
    assert not backpack621 or (cpu625 and actor_palette and pre_background), "621 existing transport/carrier prerequisite"
    protected_before={}
    if backpack621:
        # Reuse current output custody; a fresh accepted OFF carrier precedes firstON.
        for name in ('player','cpu625')+('npc665',)*int(npc665):
            obj=output/(name+'.o')
            assert obj.is_file(), "Generate same-selection current621OFF carrier before firstON"
            protected_before[name]=cpu_owner.facts(obj)

    assert type(raw_binding.get("shadow_fix")) is bool and raw_binding["shadow_fix"]==shadow_fix, "Raw/image shadow selection mismatch"
    assert type(raw_binding.get("actor_palette")) is bool and raw_binding["actor_palette"]==actor_palette, "Raw/image actor selection mismatch"
    assert type(raw_binding.get("pre_background")) is bool and raw_binding["pre_background"]==pre_background, "Raw/image PRE selection mismatch"
    assert type(cpu625) is bool and raw_binding.get("cpu625",False)==cpu625
    assert type(minimum_host) is bool and raw_binding.get("minimum_host",False)==minimum_host
    assert not minimum_host or (actor_palette and pre_background), "HOST requires accepted actor/PRE carrier"
    assert type(npc665) is bool and raw_binding.get("npc665",False)==npc665
    assert not npc665 or (actor_palette and pre_background), "NPC665 requires accepted actor/PRE carrier"
    assert not cpu625 or (actor_palette and pre_background), "CPU625 requires accepted actor/PRE carrier"
    assert not pre_background or (demo_timing and shadow_fix), "PRE requires accepted demo+shadow"
    assert not (shadow_fix and actor_palette) or pre_background, "Combined actor/shadow requires reviewed PRE condition"
    assert type(actor_palette) is bool, "Explicit actor selection must be boolean"
    assert not shadow_fix or demo_timing, "Shadow requires accepted demo"
    assert type(shadow_coefficients) is bool and (not shadow_coefficients or (shadow_fix and backpack621)), "Shadow coefficient policy is qualified only on the 621 composition"
    from elftools.elf.elffile import ELFFile
    r=root;out=output;source=root/SOURCE;h=sha
    compiler,flags=image_flags(root,pre_background,cpu625,npc665,backpack621)
    objects={};defs=[]
    for name in ['camera_lifecycle','camera_ncpod','camera_pools','camera_emitters','camera_replay','camera_hook_leaves','camera_state']+(['demo_timing'] if demo_timing else [])+(['shadow_projection'] if shadow_fix else [])+(['camera_pre_background_state','camera_pre_background_slots'] if pre_background else []):
     command=flags+(['-DTOOIE_ACTOR_PALETTE='+str(int(actor_palette)),'-DTOOIE_PRE_BACKGROUND='+str(int(pre_background))] if name in ('camera_hook_leaves','shadow_projection') else [])+(['-DTOOIE_SHADOW_COEFFICIENTS='+str(int(shadow_coefficients))] if name=='shadow_projection' else [])+['-c',win(source/(name+'.c')),'-o',win(out/(name+'.o'))]
     run(command,name+'-compile.log')
     with (out/(name+'.o')).open('rb') as stream:
      elf=ELFFile(stream);syms=list(elf.get_section_by_name('.symtab').iter_symbols());defined=[s.name for s in syms if s['st_info']['type']=='STT_FUNC' and s['st_shndx']!='SHN_UNDEF'];imports={s.name for s in syms if s.name and s['st_shndx']=='SHN_UNDEF'}
      if name=='camera_pre_background_slots':
       assert defined==PRE_HELPERS and imports==set('tooie_camera_state tooie_pre_background D_80127658 D_80128780 D_8012D508'.split()), (defined,imports)
       assert {s.name for s in elf.iter_sections() if s['sh_flags']&2 and s['sh_size']}=={'.tooie_pre_background_helpers','.reginfo','.MIPS.abiflags'}
      elif name=='camera_pre_background_state':
       assert not defined and not imports
       storage=elf.get_section_by_name('.tooie_pre_background_state');assert storage['sh_type']=='SHT_PROGBITS' and storage.data()==bytes(124)
      elif name=='shadow_projection':
       expected_imports=set('tooie_camera_state tooie_render_bridge_state D_80132E9C D_801354B0 D_80135490 D_8012D508 D_80127674 D_8007BEDA D_8007BCE8 D_8007BCF0 D_8007BCD0 D_80127658 D_8012C838 D_8012C9B0 D_8012CF8C'.split())
       if actor_palette:expected_imports|={'D_8012D500','D_8012D558','tooie_actor_palette_begin'}|({'tooie_pre_background'} if pre_background else set())
       assert defined==SHADOW_HELPERS and imports==expected_imports, (defined,imports)
       assert {s.name for s in elf.iter_sections() if s['sh_flags']&2 and s['sh_size']}=={'.tooie_shadow_helpers','.tooie_shadow_state','.reginfo','.MIPS.abiflags'}
       state=elf.get_section_by_name('.tooie_shadow_state');assert state['sh_type']=='SHT_PROGBITS' and state.data()==bytes.fromhex('53484403')+bytes(52)
      elif name=='demo_timing':
       assert defined==DEMO_HELPERS and not imports, (defined,imports)
       assert {s.name for s in elf.iter_sections() if s['sh_flags']&2 and s['sh_size']}=={'.tooie_demo_timing_helpers','.tooie_demo_timing_state','.reginfo','.MIPS.abiflags'}
       for metadata in ['.reginfo','.MIPS.abiflags']:assert elf.get_section_by_name(metadata)['sh_size']==24
       state=elf.get_section_by_name('.tooie_demo_timing_state');assert state['sh_type']=='SHT_PROGBITS' and state.data()==bytes(8)
      elif name=='camera_pools':
       assert set(defined)==set(['func_8001A3A0', 'func_8001A324', 'func_800BC690', 'func_800BC8A0'])
       assert imports==set(['heap_alloc', 'heap_free', 'func_8001A570', 'func_800A9800', 'func_8001A63C', 'func_800EA05C', 'func_800B5680', 'func_800EF410', 'func_800EEAA4', 'func_800EFA20', 'guMtxF2L', 'func_800EE7F8', 'D_8007BCD0', 'D_8007BCD4', 'D_8007BCD8', 'D_8007BCDC', 'D_8007BCE0', 'D_8007BCE4', 'D_8007BEDA', 'D_801282E0', 'D_8011A760', 'D_8011A7C0', 'tooie_camera_state', 'tooie_camera_main', 'tooie_camera_redirected'])
      elif name in ('camera_emitters','camera_hook_leaves'):
       prior_object=source/(name+'_object_contract.json')  # Frozen emitter-seven-object-04/hook-leaves-object-02 definitions/imports.
       prior_record=json.loads(prior_object.read_text());assert set(defined)==set(prior_record['definitions'])|({CPU_ROOT_HELPERS[1],FEATHER_LEAF_PATCH} if name=='camera_hook_leaves' and actor_palette else set());assert imports==set(prior_record['imports'])|({'D_80127674','D_8012D500','D_8012D558','tooie_actor_palette_begin','tooie_actor_cpu_root_begin',FEATHER_LEAF_HELPER,'D_8012C960','D_8012CA18','osVirtualToPhysical'}|({'tooie_pre_background'} if pre_background else set()) if name=='camera_hook_leaves' and actor_palette else set())|({'tooie_pre_background','D_80128780','D_80127658'} if name=='camera_emitters' and pre_background else set())
      elif name=='camera_replay':
       assert set(defined)=={'func_800BC8F8','func_800BC948'}
       assert imports==set('func_800E42E4 func_800E4640 func_800BCC90 osWritebackDCache func_800E42A8 func_800CAA24 func_800E46E0 func_800E7FF8 func_800D674C func_800B2840 func_800B21CC func_800DF440 func_800DF830 func_800DF410 func_800DE498 D_80128390 D_8012839C D_8012D558 D_8012D508 D_801282E0 tooie_camera_state tooie_camera_redirected'.split()),imports
      else:
       assert set(defined)==set(['func_800CA314', 'func_800CA334', 'func_800CA364', 'func_800CA384', 'func_800CA440', 'func_8010F9C0', 'func_8010FB34', 'func_8010FBD4', 'func_80110068', 'func_8010FD08'] if name=='camera_lifecycle' else (['ncpod_entrypoint_12','ncpod_entrypoint_14'] if name=='camera_ncpod' else []))
       assert imports==({'func_800CA6C0', 'func_800CA6F0', 'func_801150F8', 'func_800CA218', 'func_800CA2A8', '_ncpod_entrypoint_1', '_ncfixposrot_entrypoint_0', 'heap_alloc', '_ncfixposrot_entrypoint_1', 'func_80114C7C', 'tooie_camera_state', 'heap_free', 'func_80115324', 'func_801104F4', 'func_800139F8', 'defrag', 'func_800CA688', 'func_80110B68', 'func_80110FCC', 'D_80125970', 'func_800CAF34', 'func_800CA5F8', 'func_80110424', '_ncpod_entrypoint_0', 'func_80114BD4', 'aligned4_memcpy', 'func_80114CD4', 'func_800DA298', 'func_80115118', 'func_8010FC38'} if name=='camera_lifecycle' else (set(['_ncpoddialog_entrypoint_3', '_ncpoducam_entrypoint_6', '_ncpodfixpos_entrypoint_1', '_ncpoddialog_entrypoint_2', '_ncpoducam_entrypoint_0', '_ncpodfixpos_entrypoint_0', 'ncpod_entrypoint_2', '_ncpoddialog_entrypoint_1', '_ncpoducam_entrypoint_7', '_ncpodfixpos_entrypoint_6', 'func_800A940C', 'func_8010FE14', 'func_800C5148', 'func_800C51A4', 'D_801276D0', 'D_8007BF10', 'tooie_camera_state']) if name=='camera_ncpod' else set())),imports
      sections={s.name:{'type':s['sh_type'],'bytes':s['sh_size'],'alignment':s['sh_addralign']} for s in elf.iter_sections() if s['sh_flags']&2 and s['sh_size']}
      if name in ('shadow_projection','camera_hook_leaves'):
       assert '.text' not in sections, 'Camera owners must not outline stray text'
       if name=='camera_hook_leaves':assert set(sections)=={'.tooie_camera_helpers','.reginfo','.MIPS.abiflags'}|({'.recomp_patch'} if actor_palette else set()), 'Exact DD024 patch placement'
       calls=[]
       for sec in elf.iter_sections():
        if sec['sh_type']!='SHT_REL':continue
        table=elf.get_section(sec['sh_link'])
        code=elf.get_section(sec['sh_info']).data()
        for item in sec.iter_relocations():
         if item['r_info_type']!=4:continue
         offset=item['r_offset'];word=int.from_bytes(code[offset:offset+4],'big');target=table.get_symbol(item['r_info_sym'])
         if word>>26==3:calls.append(target.name)
         else:
          owner=next(s for s in syms if s['st_shndx']==sec['sh_info'] and s['st_info']['type']=='STT_FUNC' and s['st_value']<=offset<s['st_value']+s['st_size'])
          destination=target['st_value']+((word&0x3FFFFFF)<<2)
          assert word>>26==2 and target['st_shndx']==sec['sh_info'] and owner['st_value']<=destination<owner['st_value']+owner['st_size'], 'Only same-function local J control flow permitted'
       assert calls==(['tooie_actor_palette_begin']+([CPU_ROOT_HELPERS[0],FEATHER_LEAF_HELPER,'osVirtualToPhysical'] if name=='camera_hook_leaves' else []) if actor_palette else []), 'Exact separate actor begin JALs per selected owner'
      if name=='camera_state':
       for sec in ['.tooie_camera_state','.tooie_camera_pools']:assert elf.get_section_by_name(sec)['sh_type']=='SHT_PROGBITS'
      frames={}
      if name in ('camera_hook_leaves','demo_timing','shadow_projection','camera_pre_background_slots'):
       for symbol in syms:
        if symbol.name not in defined:continue
        code=elf.get_section(symbol['st_shndx']).data()[symbol['st_value']:symbol['st_value']+symbol['st_size']]
        words=[int.from_bytes(code[i:i+4],'big') for i in range(0,len(code),4)]
        frames[symbol.name]=max((0x10000-(w&0xffff) for w in words if w>>16==0x27bd and w&0x8000),default=0)
        if name=='camera_hook_leaves':assert not any(w>>26==0 and w&63==9 for w in words), 'Camera owner must not add indirect JALR'
        if name in ('demo_timing','shadow_projection','camera_pre_background_slots'):
         combined_owner=name=='shadow_projection' and actor_palette and symbol.name=='tooie_shadow_de498'
         assert frames[symbol.name]<=(32 if name in ('shadow_projection','camera_pre_background_slots') else 16), 'Compiled helper frame bound'
         assert sum(w>>26==3 for w in words)==int(combined_owner) and not any(w>>26==0 and w&63==9 for w in words), 'Exact selected helper JAL/leaf bound'
         assert not any((w>>26 in [15,32,33,35,36,37,40,41,43]) and ((w>>16&31)==28 if w>>26==15 else (w>>21&31)==28) for w in words), 'Compiled demo helper GP dependency'
      defs+=defined;objects[name]={'sha256':h(out/(name+'.o')),'definitions':defined,'imports':sorted(imports),'allocated_sections':sections,'compile_argv':[str(x) for x in command],'frames':frames}
    demo=None;shadow=None;pre=None;cpu=None;host=None;npc=None;backpack=None;graphics_rate=None
    selected_shared=held.compile_objects(root,out,run,cpu_root=actor_palette,cpu625=cpu625,npc665=npc665,shared=True,backpack621=backpack621,hand862=backpack621 and shadow_coefficients)
    lifecycle=selected_shared['lifecycle']
    if not npc665:
        for name in ('player','silo')+(() if backpack621 else ('bone','leaf')):
            fresh=cpu_owner.facts(out/(name+'.o'))
            old=cpu_owner.facts(root/'build/windows-dev/baseline/render-bridge-silo-candidate'/(name+'.o'))
            assert fresh['section_bytes']==old['section_bytes'] and fresh['relocations']==old['relocations'], ('NPC OFF exact shared object',name)
    if npc665:
        objects['npc665']=npc_owner.compile_owner(root,out,run,flags+['-DTOOIE_ACTOR_PALETTE=1'],win)
        defs+=npc_owner.HELPERS
    fresh_lifecycle=cpu_owner.facts(out/'lifecycle.o')
    fresh_lifecycle['compile_argv']=lifecycle['compile_argv']
    fresh_lifecycle['inputs']={str(root/'tools/baseline/guest/silo-candidate/player_lifecycle.c'):sha(root/'tools/baseline/guest/silo-candidate/player_lifecycle.c')}
    if not cpu625:
        previous=cpu_owner.facts(root/'build/windows-dev/baseline/render-bridge-silo-candidate/lifecycle.o')
        assert fresh_lifecycle['section_bytes']['.recomp_patch']==previous['section_bytes']['.recomp_patch'] and fresh_lifecycle['relocations']==previous['relocations'], "OFF lifecycle exact text/relocations"
    else:
        calls=[x for x in fresh_lifecycle['relocations'] if x['target']=='tooie_cpu625_retire']
        assert len(calls)==2 and all(x['kind']==4 for x in calls), "Exactly two lifecycle retire call relocations"
        objects['cpu625']=cpu_owner.compile_owner(root,out,run,flags,win);defs+=cpu_owner.HELPERS
    b621_object=backpack_owner.compile_owner(root,out,run,flags,win,backpack621)
    if backpack621:
        objects['backpack621']=b621_object;defs+=backpack_owner.HELPERS
        for name,before in protected_before.items():
            assert cpu_owner.facts(out/(name+'.o'))==before, ('621 unchanged existing owner object',name)
    if minimum_host:
        objects.update(host_owner.compile_objects(root,out,run,flags,win));defs += [host_owner.HELPER,host_owner.PROJECTION]
    if backpack621:
        objects['graphics_rate']=rate_owner.compile_owner(root,out,run,flags,win);defs.append(rate_owner.BODY)
    if quiet_talon_trot:
        objects['quiet_talon_trot']=quiet_owner.compile_owner(root,out,run,flags,win);defs.append(quiet_owner.BODY)
    heap=r/'build/windows-dev/baseline/heap-patch';prior=r/'build/windows-dev/baseline/render-bridge-silo-candidate'
    honey=prepare_honey_containment(root,output,run,npc665,backpack621)
    selected_actor=selected_shared if actor_palette else None
    base_functions=list(held.GENERATED_FUNCTIONS)
    if actor_palette:base_functions.insert(base_functions.index('tooie_actor_palette_end'),CPU_ROOT_HELPERS[0]);base_functions.insert(base_functions.index('tooie_actor_palette_end'),FEATHER_LEAF_HELPER)
    inputs=[heap/'boot.o',heap/'core1.o',r/'generated/heap-patch-products/heap.o',honey]+[out/(n+'.o') for n in ['player','actor','bone','pair','silo','leaf','lifecycle']]+[out/(n+'.o') for n in objects]
    run([compiler.parent/'ld.lld.exe','-m','elf32btsmip','-T',win(source/'render_bridge.ld'),'--unresolved-symbols=ignore-all','--emit-relocs','--orphan-handling=error']+[win(p) for p in inputs]+['-o',win(out/'boot.elf')],'link.log')
    with (out/'boot.elf').open('rb') as stream:
     elf=ELFFile(stream);symbols={s.name:s for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name};state=elf.get_section_by_name('.tooie_camera_state');pools=elf.get_section_by_name('.tooie_camera_pools');oldstate=elf.get_section_by_name('.tooie_render_bridge_state')
     if demo_timing:
      section=elf.get_section_by_name('.tooie_demo_timing_helpers');storage=elf.get_section_by_name('.tooie_demo_timing_state')
      reloc=elf.get_section_by_name('.rel.tooie_demo_timing_helpers');kinds=[x['r_info_type'] for x in reloc.iter_relocations()] if reloc else []
      assert all(x in (4,5,6) for x in kinds), 'Demo helpers permit only local jumps and own-state HI16/LO16 relocations'
      assert storage['sh_type']=='SHT_PROGBITS' and storage.data()==bytes(8) and storage['sh_addr']==section['sh_addr']+section['sh_size']
      assert all(symbols[n]['st_shndx']==elf.get_section_index('.tooie_demo_timing_helpers') and symbols[n]['st_size']>0 for n in DEMO_HELPERS)
      if reloc:
       table=elf.get_section(reloc['sh_link'])
       for item in reloc.iter_relocations():
        target=table.get_symbol(item['r_info_sym']);assert (target['st_shndx']==elf.get_section_index('.tooie_demo_timing_helpers')) if item['r_info_type']==4 else target.name=='tooie_demo_timing_state'
      demo=dict(rom=section['sh_addr']-0x80801000,ram=section['sh_addr'],size=section['sh_size'],index=elf.get_section_index('.tooie_demo_timing_helpers'),state=storage['sh_addr'])
     if shadow_fix:
      section=elf.get_section_by_name('.tooie_shadow_helpers');storage=elf.get_section_by_name('.tooie_shadow_state')
      assert storage['sh_type']=='SHT_PROGBITS' and storage.data()==bytes.fromhex('53484403')+bytes(52)
      assert section['sh_addr']==demo['state']+8 and storage['sh_addr']==section['sh_addr']+section['sh_size']
      assert all(symbols[n]['st_shndx']==elf.get_section_index('.tooie_shadow_helpers') and symbols[n]['st_size']>0 for n in SHADOW_HELPERS)
      reloc=elf.get_section_by_name('.rel.tooie_shadow_helpers');assert reloc
      table=elf.get_section(reloc['sh_link'])
      for item in reloc.iter_relocations():
       target=table.get_symbol(item['r_info_sym']);assert item['r_info_type'] in (4,5,6)
       if item['r_info_type']==4:
        assert target['st_shndx']==elf.get_section_index('.tooie_shadow_helpers') or (actor_palette and target.name=='tooie_actor_palette_begin' and target['st_value']==symbols['tooie_actor_palette_begin']['st_value'])
       else:assert target.name in objects['shadow_projection']['imports']+['shadow_pending']
      shadow=dict(rom=section['sh_addr']-0x80801000,ram=section['sh_addr'],size=section['sh_size'],index=elf.get_section_index('.tooie_shadow_helpers'),state=storage['sh_addr'],args=storage['sh_addr']+36)
     if pre_background:
      section=elf.get_section_by_name('.tooie_pre_background_helpers');storage=elf.get_section_by_name('.tooie_pre_background_state')
      assert storage['sh_type']=='SHT_PROGBITS' and storage.data()==bytes(124) and storage['sh_addr']==section['sh_addr']+section['sh_size']
      assert all(symbols[n]['st_shndx']==elf.get_section_index('.tooie_pre_background_helpers') and symbols[n]['st_size']>0 for n in PRE_HELPERS)
      reloc=elf.get_section_by_name('.rel.tooie_pre_background_helpers');assert reloc;table=elf.get_section(reloc['sh_link'])
      for item in reloc.iter_relocations():
       target=table.get_symbol(item['r_info_sym']);assert item['r_info_type'] in (4,5,6)
       if item['r_info_type']==4:assert target['st_shndx']==elf.get_section_index('.tooie_pre_background_helpers')
       else:assert target.name in objects['camera_pre_background_slots']['imports']
      pre=dict(rom=section['sh_addr']-0x80801000,ram=section['sh_addr'],size=section['sh_size'],index=elf.get_section_index('.tooie_pre_background_helpers'),state=storage['sh_addr'])
     if cpu625:cpu=cpu_owner.linked(elf)
     if minimum_host:host=host_owner.linked(elf)
     if npc665:
      prior_end=host['image_end'] if host else cpu['state']+464 if cpu else pre['state']+124
      npc=npc_owner.linked(elf,prior_end);npc['prior_end']=prior_end
     if backpack621:
      prior_end=npc['state']+16 if npc else host['image_end'] if host else cpu['state']+464
      backpack=backpack_owner.linked(elf,prior_end)
      graphics_rate=rate_owner.linked(elf,backpack['image_end'])
     expected=bytearray(1380)
     for offset,value in {1024:0x43414D32,1028:2,1064:0xA0000000,1068:1,1072:1,1076:1}.items():expected[offset:offset+4]=value.to_bytes(4,'big')
     assert state.data()==expected and state['sh_size']==1380
     assert elf.get_section_by_name('.tooie_camera_constants').data()==bytes.fromhex('41a000003f800000')
     assert elf.get_section_by_name('.tooie_camera_ncpod_constants').data()==bytes.fromhex('422000003f800000')
     assert elf.get_section_by_name('.tooie_camera_pool_constants').data()==bytes.fromhex('3f800000')
     assert pools['sh_type']=='SHT_PROGBITS' and len(pools.data())==147200 and not any(pools.data())
     assert oldstate.data()==(prior/'boot.bin').read_bytes()[-18844:] and oldstate['sh_size']==18844
     addresses={n:symbols[n]['st_value'] for n in ['tooie_camera_state','tooie_camera_main','tooie_camera_redirected','tooie_render_bridge_state']}
     assert addresses['tooie_camera_main']%16==0 and addresses['tooie_camera_redirected']%16==0
     assert symbols['tooie_camera_main']['st_size']==86400 and symbols['tooie_camera_redirected']['st_size']==60800
     ranges=sorted([(addresses['tooie_camera_main'],addresses['tooie_camera_main']+86400),(addresses['tooie_camera_redirected'],addresses['tooie_camera_redirected']+60800)]);assert ranges[0][1]==ranges[1][0] and ranges[1][1]<=0x81000000
     data=toml.loads((prior/'data.syms.toml').read_text());existing=[s for sec in data['section'] for s in sec.get('symbols',[])];assert not any(s['name']=='D_80125970' or s['vram']==0x80125970 for s in existing);data['section'][0]['symbols'].append(dict(name='D_80125970',vram=0x80125970));
     for name,address in [('D_801276D0',0x801276D0),('D_8007BF10',0x8007BF10)]:
      assert not any(s['name']==name or s['vram']==address for s in existing);data['section'][0]['symbols'].append(dict(name=name,vram=address))
     for name in ['D_8007BCD0', 'D_8007BCD4', 'D_8007BCD8', 'D_8007BCDC', 'D_8007BCE0', 'D_8007BCE4', 'D_8007BEDA', 'D_801282E0', 'D_8011A760', 'D_8011A7C0','D_8012D500','D_8012D504','D_8012D554','D_8012D558','D_8012D508','D_80127674','D_80132DCC','D_80127EF4','D_80127658','D_80128390','D_8012839C']+(['D_80132E9C','D_80135490','D_801354B0','D_8012C9B0'] if shadow_fix else [])+(['D_80128780'] if pre_background else []):
      address=int(name[2:],16);owners=[x for sec in data['section'] for x in sec.get('symbols',[]) if x['name']==name or x['vram']==address]
      if owners:assert owners==[dict(name=name,vram=address)],owners
      else:data['section'][0]['symbols'].append(dict(name=name,vram=address))
     if npc665:
      for name in sorted(npc_owner.IMPORTS):
       if not name.startswith('D_'):continue
       address=int(name[2:],16);owners=[x for sec in data['section'] for x in sec.get('symbols',[]) if x['name']==name or x['vram']==address]
       if owners:assert owners==[dict(name=name,vram=address)],owners
       else:data['section'][0]['symbols'].append(dict(name=name,vram=address))
     assert not any(x['name']=='widescreen_enabled' or x['vram']==0x800799B8 for sec in data['section'] for x in sec.get('symbols',[]))
     data['section'][0]['symbols'].append(dict(name='widescreen_enabled',vram=0x800799B8))
     if minimum_host:
      assert not any(x['name']=='D_80041710' or x['vram']==0x80041710 for sec in data['section'] for x in sec.get('symbols',[]))
      data['section'][0]['symbols'].append(dict(name='D_80041710',vram=0x80041710))
     (out/'data.syms.toml').write_text(toml.dumps(data))
     config={'input':{'elf_path':str(out/'boot.elf'),'output_func_path':str(out),'single_file_output':True,'func_reference_syms_file':str(r/'config/selected.syms.toml'),'data_reference_syms_files':[str(out/'data.syms.toml')],'output_binary_path':str(out/'boot.bin'),'strict_patch_mode':True,'unpaired_lo16_warnings':False,'recomp_include':'#include "recomp.h"'}}
    if minimum_host:
        config['input']['use_absolute_symbols']=True
        config['input']['recomp_include']+='\n#include \"minimum_host_imports.h\"'
    (out/'boot.toml').write_text(toml.dumps(config));run([r/'deps/N64Recomp-codegen/build/N64Recomp',out/'boot.toml'],'codegen.log')
    cpu_table=accepted.authenticate_function_table(out/'boot.elf',out/'recomp_overlays.inl','.tooie_cpu625_helpers',cpu_owner.HELPERS) if cpu else None
    host_table=accepted.authenticate_function_table(out/'boot.elf',out/'recomp_overlays.inl','.baseline_host_helpers',[host_owner.HELPER]) if host else None
    npc_table=accepted.authenticate_function_table(out/'boot.elf',out/'recomp_overlays.inl','.tooie_npc665_helpers',npc_owner.HELPERS) if npc else None
    backpack_table=accepted.authenticate_function_table(out/'boot.elf',out/'recomp_overlays.inl','.tooie_backpack621_helpers',backpack_owner.HELPERS) if backpack else None
    if demo:
     overlay=(out/'recomp_overlays.inl').read_text();match=re.search(r'static RelocEntry section_%d_tooie_demo_timing_helpers_relocs\[\] = \{(.*?)\n\};'%demo['index'],overlay,re.S);assert match
     demo['relocs']=match[1].count('.offset');assert demo['relocs']==0, 'Internal guest jumps must remain translated local control flow'
    if shadow:
     overlay=(out/'recomp_overlays.inl').read_text();match=re.search(r'static RelocEntry section_%d_tooie_shadow_helpers_relocs\[\] = \{(.*?)\n\};'%shadow['index'],overlay,re.S);assert match
     shadow['relocs']=match[1].count('.offset');assert shadow['relocs']==0, 'Shadow code has only fixed data and local translated control flow'
    if pre:
     overlay=(out/'recomp_overlays.inl').read_text();match=re.search(r'static RelocEntry section_%d_tooie_pre_background_helpers_relocs\[\] = \{(.*?)\n\};'%pre['index'],overlay,re.S);assert match
     pre['relocs']=match[1].count('.offset');assert pre['relocs']==0
    binary=(out/'boot.bin').read_bytes();assert binary[:2760]==(heap/'boot.bin').read_bytes();assert binary[addresses['tooie_camera_state']-0x80801000:addresses['tooie_camera_state']-0x80801000+1380]==expected;assert len(binary)==(graphics_rate['image_end'] if graphics_rate else backpack['image_end'] if backpack else npc['state']+16 if npc else host['image_end'] if host else cpu['state']+464 if cpu else pre['state']+124 if pre else shadow['state']+56 if shadow else demo['state']+8 if demo else ranges[1][1])-0x80801000
    if demo:assert demo['ram']==ranges[1][1] and binary[demo['state']-0x80801000:demo['state']-0x80801000+8]==bytes(8)
    if pre:
     assert demo_timing and shadow_fix
     expected_image=(backpack_owner.SHADOW_COEFFICIENTS_IMAGE_SHA256 if shadow_coefficients else backpack_owner.PRE_IMAGE_SHA256) if backpack621 else npc_owner.PRE_IMAGE_SHA256 if npc665 else (host_owner.PRE_CPU_ON_SHA256 if cpu625 else host_owner.PRE_CPU_OFF_SHA256) if minimum_host else cpu_owner.PRE_IMAGE_SHA256 if cpu625 else '9b52d782634dbd33c91588acf3804ffd51180fd9f7a8b6f6946b7cdb462d9718' if actor_palette else '9f55304f37a88c83579c0c4e5ac02deab9baec9bf278d3a705c4d3a0fe49dae7'
    if shadow and not pre:
     import hashlib
     assert hashlib.sha256(binary[:233884]).hexdigest()=='033f7b3ae4c987ef03398ad874eb92cf3a9982a81f00c1bcb6ea030eeb0b9122', 'Exact requalified playable665 demo prefix'
     assert sha(out/'boot.bin')=='1831cc65f28e6f3acdde02a40ce5ea4f660f9cefad8b0932cbc4ed4faabb0e7e', 'Exact requalified playable665 paletteOFF shadow/demo image'
    text=(out/'boot.c').read_text();assert text.count('\n    tooie_actor_palette_begin(rdram, ctx);')==int(actor_palette)*(1+int(shadow_fix)), 'Exact compiled actor hook call selection'
    expected_order=base_functions+defs
    if cpu625:
        # Codegen emits this same-image dependency at its first lifecycle use.
        expected_order.remove('tooie_cpu625_retire')
        expected_order.insert(expected_order.index('func_80092898')+1,'tooie_cpu625_retire')
    actual_order=re.findall(r'^RECOMP_FUNC void (\w+)\(',text,re.M)
    if npc665 or backpack621:
        new_helpers=(npc_owner.HELPERS if npc665 else [])+((backpack_owner.HELPERS+[rate_owner.BODY]) if backpack621 else [])+([quiet_owner.BODY] if quiet_talon_trot else [])
        original_order=[n for n in actual_order if n not in new_helpers]
        assert len(actual_order)==len(set(actual_order)) and set(actual_order)==set(expected_order)
        assert original_order==[n for n in expected_order if n not in new_helpers], 'Existing provider order preserved'
    else:assert len(actual_order)==len(set(actual_order)) and actual_order==expected_order, 'Exact ordered generated providers'
    assert set(base_functions+defs)==set(base_functions)|set(json.loads((source/'camera_hook_contract.json').read_text())['branches']['hook']['providers'])|set(DEMO_HELPERS if demo_timing else [])|set(SHADOW_HELPERS if shadow_fix else [])|set(PRE_HELPERS if pre_background else [])|set(cpu_owner.HELPERS if cpu625 else [])|set(npc_owner.HELPERS if npc665 else [])|set(backpack_owner.HELPERS if backpack621 else [])|set([host_owner.PROJECTION,host_owner.HELPER] if minimum_host else [])|set([rate_owner.BODY] if backpack621 else [])|set([quiet_owner.BODY] if quiet_talon_trot else [])|set([CPU_ROOT_HELPERS[1],FEATHER_LEAF_PATCH] if actor_palette else [])
    npc_end=None
    if npc665:
        npc_end=npc_owner.checked_end(out,out/'pair.o')
        text=(out/'boot.c').read_text()
        defs=[npc_owner.BODY if n==npc_owner.END else n for n in defs]
        expected_order=[npc_owner.BODY if n==npc_owner.END else n for n in actual_order]
    if backpack621 and not npc665:expected_order=actual_order
    shared=r/'tests/extract_heap_realloc.py';s=shared.read_text();old='"tooie_player_begin", "tooie_player_end", "tooie_player_bone_push", "tooie_player_bone_pop", "tooie_player_leaf_begin", "tooie_player_leaf_end"):';scope={'__name__':'local_extractor'};exec(compile(s.replace(old,'"tooie_player_begin", "tooie_player_end", '+','.join(repr(n) for n in defs)+'):'),str(shared),'exec'),scope)
    (out/'candidate_camera_lifecycle_ncpod_slice.inc').write_text(''.join(scope['extract'](text,n) for n in defs));write_if_changed(out/'camera_identity_layout.h','#pragma once\n#define TOOIE_CAMERA_STATE_VRAM 0x%08Xu\n#define TOOIE_CAMERA_MAIN_VRAM 0x%08Xu\n#define TOOIE_CAMERA_REDIRECTED_VRAM 0x%08Xu\n#define TOOIE_BRIDGE_STATE_VRAM 0x%08Xu\n'%(addresses['tooie_camera_state'],addresses['tooie_camera_main'],addresses['tooie_camera_redirected'],addresses['tooie_render_bridge_state']))
    write_if_changed(out/'render_bridge_layout.h','#pragma once\n#define TOOIE_BRIDGE_STATE_VRAM 0x%08Xu\n#define TOOIE_BRIDGE_IMAGE_BYTES %du\n#define TOOIE_BRIDGE_STATE_IMAGE_OFFSET %du\n'%(addresses['tooie_render_bridge_state'],len(binary),addresses['tooie_render_bridge_state']-0x80801000))
    assert text.count("\n    tooie_cpu625_retire(rdram, ctx);")==2*int(cpu625), "Exact generated lifecycle call selection"
    if cpu:
        overlay=(out/'recomp_overlays.inl').read_text();match=re.search(r'static RelocEntry section_%d_tooie_cpu625_helpers_relocs\[\] = \{(.*?)\n\};'%cpu['index'],overlay,re.S);assert match
        cpu['relocs']=match[1].count('.offset')
    if host:
        overlay=(out/'recomp_overlays.inl').read_text();match=re.search(r'static RelocEntry section_%d_baseline_host_helpers_relocs\[\] = \{(.*?)\n\};'%host['index'],overlay,re.S);assert match
        host['relocs']=match[1].count('.offset')
    assert text.count('\n    tooie_backpack621_retire(rdram, ctx);')==2*int(backpack621), 'Exact621 lifecycle call selection'
    write_registry_header(root,output,actor_palette,demo,shadow,pre,cpu,host,npc,backpack,quiet_talon_trot)
    if pre:
        if minimum_host:
            names=['boot.c','boot.bin','boot.elf','recomp_overlays.inl','funcs.h','camera_identity_layout.h','render_bridge_layout.h','camera_registry_data.hpp','pre_background_layout.h','cpu625_selection_layout.h','minimum_host_selection_layout.h']
            candidate=dict(scope='UNQUALIFIED mechanically validated HOST candidate; no native/readiness authority',cpu625=cpu625,minimum_host=host,host_table=host_table,image_sha256=sha(out/'boot.bin'),image_bytes=len(binary),addresses=addresses,raw_binding_sha256=sha(out/'RAW_BINDING.json'),generated={name:sha(out/name) for name in names})
            (out/'HOST_UNQUALIFIED_LINK.json').write_text(json.dumps(candidate,indent=2)+'\n')
        elif cpu625:
            candidate=dict(scope='UNQUALIFIED mechanically validated linked candidate; no native/readiness authority',image_sha256=sha(out/'boot.bin'),image_bytes=len(binary),cpu625=cpu,cpu625_table=cpu_table,lifecycle=fresh_lifecycle,addresses=addresses,shared_state_bytes=18844,camera_state_bytes=1380,pool_bytes=147200,raw_binding_sha256=sha(out/'RAW_BINDING.json'),generated={name:sha(out/name) for name in ['boot.c','boot.bin','boot.elf','recomp_overlays.inl','funcs.h','camera_identity_layout.h','render_bridge_layout.h','camera_registry_data.hpp','pre_background_layout.h','cpu625_selection_layout.h']})
            (out/'CPU625_UNQUALIFIED_LINK.json').write_text(json.dumps(candidate,indent=2)+'\n')
        if npc665:
            candidate=dict(scope='UNQUALIFIED linked NPC665 candidate; composed qualification still required',image_sha256=sha(out/'boot.bin'),npc665=npc,object=objects['npc665'],shared_objects=selected_shared,table=npc_table,end=npc_end,raw_binding_sha256=sha(out/'RAW_BINDING.json'))
            (out/'NPC665_UNQUALIFIED_LINK.json').write_text(json.dumps(candidate,indent=2)+'\n')
        if backpack:
            candidate=dict(scope='UNQUALIFIED linked621 candidate; original/public composed qualification still owed',image_sha256=sha(out/'boot.bin'),backpack621=backpack,object=b621_object,shared_objects=selected_shared,table=backpack_table,protected_owner_objects={name:row['sha256'] for name,row in protected_before.items()},raw_binding_sha256=sha(out/'RAW_BINDING.json'))
            (out/'BACKPACK621_UNQUALIFIED_LINK.json').write_text(json.dumps(candidate,indent=2)+'\n')
            (out/'GRAPHICS_RATE_UNQUALIFIED_LINK.json').write_text(json.dumps(dict(scope='UNQUALIFIED exact public-rate replacement; stop before image pin acceptance',graphics_rate=graphics_rate,object=objects['graphics_rate'],image_sha256=sha(out/'boot.bin'),image_bytes=len(binary),raw_binding_sha256=sha(out/'RAW_BINDING.json')),indent=2)+'\n')
        assert sha(out/'boot.bin')==expected_image, 'Exact separately qualified PREON actorON/OFF/NPC image'
    receipt={'scope':'Full frozen emitter/replay layout2 composition; unselected, no parser compatibility or enabling claim','objects':objects,'addresses':addresses,'camera_state_bytes':1380,'state_bytes':18844,'pool_bytes':147200,'pool_alignment':16,'image_bytes':len(binary),'image_final_extent':hex(ranges[1][1]),'initialized_PROGBITS':True,'prefix_bytes':2760,'old_state_bytes':18844,'state_vram':addresses['tooie_render_bridge_state'],'bridge_state_bytes':18844,'helper_definitions':held.HELPERS+['tooie_camera_construction_begin','tooie_camera_construction_end','tooie_camera_de498_ignore'],'bone_frame_bytes':json.loads((prior/'receipt.json').read_text())['bone_frame_bytes'],'leaf_frame_bytes':json.loads((prior/'receipt.json').read_text())['leaf_frame_bytes'],'pair_frames':json.loads((prior/'receipt.json').read_text())['pair_frames'],'player_frames':json.loads((prior/'receipt.json').read_text())['player_frames'],'actor_frames':json.loads((prior/'receipt.json').read_text())['actor_frames'],'lifecycle_frames':lifecycle['frames'],'definitions':base_functions+defs,'inputs':{str(p):h(p) for p in inputs+list(source.glob('camera*')) if p.is_file()},'outputs':{p.name:h(p) for p in sorted(out.iterdir(),key=lambda p:p.name) if p.is_file() and p.name not in ('receipt.json','PLAYER_GUEST.json')}}
    receipt['backpack621']=backpack;receipt['backpack621_table']=backpack_table
    receipt['backpack621_object']=b621_object;receipt['backpack621_protected_objects']={name:row['sha256'] for name,row in protected_before.items()}
    if backpack:
        receipt['inputs'].update(b621_object['inputs']);receipt['helper_definitions']+=backpack_owner.HELPERS
    receipt['minimum_host']=host;receipt['host_table']=host_table
    if host:
        receipt['helper_definitions'] += [host_owner.HELPER]
        for name in ('minimum_cutscene','native_aspect_projection'):receipt['inputs'].update(objects[name]['inputs'])
    receipt['npc665']=npc;receipt['npc665_table']=npc_table;receipt['npc665_end']=npc_end;receipt['shared_objects']=selected_shared
    if npc:receipt['inputs'].update(objects['npc665']['inputs']);receipt['helper_definitions']+=npc_owner.HELPERS
    receipt['cpu625']=cpu;receipt['lifecycle_object']=fresh_lifecycle
    receipt['cpu625_table']=cpu_table
    receipt['quiet_talon_trot']=quiet_talon_trot
    if quiet_talon_trot:receipt['inputs'].update(objects['quiet_talon_trot']['inputs'])
    receipt['graphics_rate']=graphics_rate
    if graphics_rate:receipt['inputs'].update(objects['graphics_rate']['inputs'])
    receipt['definitions']=expected_order
    receipt['bone_frame_bytes']=selected_shared['bone']['frames']['func_800DCF48']
    receipt['leaf_frame_bytes']=selected_shared['leaf']['frames']['func_800DD998']
    receipt['player_frames']=selected_shared['player']['frames']
    receipt['pair_frames']=selected_shared['pair']['frames']
    receipt['actor_frames']=selected_shared['actor']['frames']
    receipt['inputs'].update(fresh_lifecycle['inputs'])
    if cpu:receipt['inputs'].update(objects['cpu625']['inputs'])
    receipt['helper_definitions']+=cpu_owner.HELPERS if cpu else []
    receipt['image_final_extent']=hex(0x80801000+len(binary));receipt['pool_final_extent']=hex(ranges[1][1])
    receipt['demo_timing']=demo;receipt['helper_definitions']+=DEMO_HELPERS if demo_timing else []
    receipt['pre_background']=pre;receipt['helper_definitions']+=PRE_HELPERS if pre_background else []
    receipt['shadow_projection']=shadow;receipt['helper_definitions']+=SHADOW_HELPERS if shadow_fix else []
    receipt['shadow_coefficients']=shadow_coefficients
    if shadow_fix:receipt['inputs'][str(source/'shadow_projection.c')]=h(source/'shadow_projection.c')
    receipt['inputs'][str(source/'camera_actor_admission.h')]=h(source/'camera_actor_admission.h')
    if demo_timing:receipt['inputs'][str(source/'demo_timing.c')]=h(source/'demo_timing.c')
    receipt['helper_definitions']+=CPU_ROOT_HELPERS+[FEATHER_LEAF_HELPER] if actor_palette else []
    if selected_actor:receipt['actor_frames']=selected_actor['actor']['frames'];receipt['pair_frames']=selected_actor['pair']['frames'];receipt['actor_root_objects']=selected_actor
    receipt['actor_palette_enabled']=actor_palette;receipt['actor_palette_compile_define']='TOOIE_ACTOR_PALETTE='+str(int(actor_palette))
    # Exclude this receipt and the downstream PLAYER_GUEST receipt that hashes it.
    receipt['inputs']=dict(sorted(receipt['inputs'].items()))
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');return receipt

def prepare(root,output,run,raw=False,image=False,actor_palette=False,shadow_observer=False,demo_timing=False,shadow_fix=False,pre_background=False,cpu625=False,minimum_host=False,npc665=False,backpack621=False,shadow_coefficients=False):
    output.mkdir(exist_ok=True)
    quiet_talon_trot=bool(backpack621 and shadow_coefficients and minimum_host)
    if raw:
        receipt=prepare_raw(root,output,run,shadow_observer,demo_timing,shadow_fix,pre_background,actor_palette,cpu625,minimum_host,npc665,backpack621,quiet_talon_trot);(output/'RAW_BINDING.json').write_text(json.dumps(receipt,indent=2)+'\n');print('camera raw',sha(output/'RAW_BINDING.json'))
    if image:
        receipt=prepare_image(root,output,run,actor_palette,demo_timing,shadow_fix,pre_background,cpu625,minimum_host,npc665,backpack621,shadow_coefficients,quiet_talon_trot);print('camera image',sha(output/'receipt.json'))
