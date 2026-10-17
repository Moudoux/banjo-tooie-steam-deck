"""Explicit held Silo guest candidate; accepted eight-body preparer is unchanged."""
import argparse
import importlib.util
import json
import re
import subprocess
from pathlib import Path

import toml
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection
import prepare_render_bridge as accepted

sha, win = accepted.sha, accepted.win
PAIR = ['func_80101808', 'func_800DE448']
SETTERS = ['func_8008B10C', 'anctrl_setSmoothTransition']
LEAF_FUNCTIONS = ['func_800DD998']
LEAF_CALLEES = {'mlMtx_get_stack_pointer', 'func_80018BB4', 'mlMtxApply',
                'func_8010F070', 'osVirtualToPhysical'}
LIFECYCLE_FUNCTIONS = ['func_80092898', 'func_800F3880']
LIFECYCLE_CALLEES = set('_babackpack_entrypoint_9 func_800936E8 func_80093700 func_800AE020 func_800AE080 func_800E09B8 func_800AE6FC func_800DBFF8 func_800EFD24 func_80093370 func_8009337C func_80093230 func_800931AC func_80092778 func_80092864 func_80098E64 heap_free'.split())
FUNCTIONS = accepted.FUNCTIONS + SETTERS + LEAF_FUNCTIONS + LIFECYCLE_FUNCTIONS
HELPER = 'tooie_silo_preflight'
PLAYER_HELPERS = ['tooie_player_begin', 'tooie_player_bone_push', 'tooie_player_bone_pop', 'tooie_player_leaf_begin', 'tooie_player_leaf_end', 'tooie_player_end']
ACTOR_HELPERS = ['tooie_actor_palette_begin', 'tooie_actor_palette_end']
HELPERS = [HELPER] + PLAYER_HELPERS + ACTOR_HELPERS
GENERATED_FUNCTIONS = FUNCTIONS[:5] + PLAYER_HELPERS + ACTOR_HELPERS + FUNCTIONS[5:6] + [HELPER] + FUNCTIONS[6:8] + SETTERS + LEAF_FUNCTIONS + LIFECYCLE_FUNCTIONS
EXTRA_FUNCTIONS = {'func_80104248', 'func_800A89F8', 'func_800EB854', 'func_80100368', 'func_800AE080','func_80104148','func_80104200'}
RENDER_DATA = {'D_80132ED0','D_8007BCD0','D_8007BCD8','D_8007BCE8','D_8007BCF0',
               'D_8007BEDA','D_8012C948','D_8012C838','D_8012CF8C','D_8012CEC4',
               'D_8012CA11','D_8012C968','D_8012CE58','D_80135450','D_80136EE0',
               'D_80135490','D_801354B0','D_801354DC','D_8012C960',
               'D_80124740','D_8012C820','D_8012C850','D_8012CA18','D_80136E70'}
# Accepted draw-boundaries image; the maintained player recipe has none and binds the camera image pin instead.
HONEY_REFERENCE = 'build/windows-dev/baseline/render-bridge-draw-boundaries'


def symbols(elf):
    return {s.name: s for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}


def relocations(elf):
    syms = elf.get_section_by_name('.symtab')
    return [(r['r_offset'], r['r_info_type'], syms.get_symbol(r['r_info_sym']).name)
            for sec in elf.iter_sections() if isinstance(sec, RelocationSection)
            and sec.name in {'.rel.recomp_patch', '.rel.text.silo_preflight', '.rel.silo_helper', '.rel.text.player_boundary', '.rel.player_helper', '.rel.text.actor_palette', '.rel.actor_helper'} for r in sec.iter_relocations()]


def compile_objects(root, output, run, cpu_root=False, cpu625=False, lifecycle_only=False, npc665=False, shared=False, backpack621=False, hand862=False, talon_trot_toggle=False, honey_reference=HONEY_REFERENCE):
    assert all(type(x) is bool for x in (cpu_root,cpu625,lifecycle_only,npc665,shared,backpack621,hand862,talon_trot_toggle))
    assert not hand862 or (cpu_root and backpack621), 'Hand862 is compiled only into the selected actor-palette 621 composition'
    assert honey_reference is None or type(honey_reference) is str
    source = root/'tools/baseline/guest/silo-candidate'
    compiler = root/'deps/LLVM-19.1.3-Windows-X64/bin/clang.exe'
    assert sha(compiler) == '6ac72dac3eb54eb7f807b0c4428c8a61cf73fe77d00be8aec8bc07ab94cdcf23'
    assert sha(source/'render_bridge.c') == sha(root/'tools/baseline/guest/render_bridge.c')
    includes = [source, root/'deps/banjo-tooie/include', root/'deps/banjo-tooie/lib/ultralib/include', root/'deps/rt64/include']
    flags = [compiler, '-target', 'mips', '-EB', '-mips2', '-mabi=32', '-O2', '-G0',
             '-mno-abicalls', '-mno-odd-spreg', '-nostdinc', '-D_LANGUAGE_C', '-DMIPS', '-fno-builtin']
    flags += ['-I'+win(p) for p in includes]+['-DTOOIE_CPU625='+str(int(cpu625)),'-DTOOIE_NPC665='+str(int(npc665)),'-DTOOIE_BACKPACK621='+str(int(backpack621)),'-DTOOIE_TROT_TOGGLE='+str(int(talon_trot_toggle))]
    owners = {'bridge': ('render_bridge.c', accepted.BRIDGE_FUNCTIONS),
              'bone': ('bone_handler.c', ['func_800DCF48', HELPER]),
              'pair': ('draw_boundaries.c', PAIR), 'silo': ('silo_nodes.c', SETTERS),
              'player': ('player_boundary.c', PLAYER_HELPERS),
              'actor': ('actor_palette.c', ACTOR_HELPERS),
              'leaf': ('leaf_handler.c', LEAF_FUNCTIONS),
              'lifecycle': ('player_lifecycle.c', LIFECYCLE_FUNCTIONS)}
    if cpu_root and not shared:
        owners={name:owners[name] for name in ('actor','pair')}
    if shared:owners.pop('bridge')
    if cpu_root:
        owners['actor']=('actor_palette.c',ACTOR_HELPERS[:1]+['tooie_actor_cpu_root_begin','tooie_actor_feather_leaf_id']+ACTOR_HELPERS[1:])
    if lifecycle_only:owners={'lifecycle':owners['lifecycle']}
    receipt = {}
    state={'tooie_render_bridge_state'}
    owner_data={'D_80132ED0','D_80135450','D_80136EE0'}
    expected_imports={'pair':accepted.PAIR_CALLEES|{'func_800A89F8','func_800EB854','tooie_player_begin','tooie_player_end','tooie_actor_palette_end'}|{'D_8012C948'}|owner_data|state,
                      'bone':accepted.BONE_CALLEES|{'func_80104248','func_80100368','tooie_player_bone_push','tooie_player_bone_pop'}|
                          (RENDER_DATA-{'D_80135490','D_801354B0','D_801354DC','D_8012C960','D_80124740','D_8012C820','D_8012C850','D_8012CA18','D_80136E70'})|state,
                      'silo':{'D_80135450'}|state,
                      'player': {'func_800AE080','func_800A89F8','func_800EB854',
                          'D_80135490','D_801354B0','D_801354DC','D_8012C948',
                          'D_8012C838','D_8012C968','D_8012CF8C','D_8007BCD0',
                          'D_8007BEDA','D_8007BCE8','D_8007BCF0'}|state,
                      'leaf': LEAF_CALLEES|{'D_8012C960','D_8012CEC4','tooie_player_leaf_begin','tooie_player_leaf_end'},
                      'lifecycle': LIFECYCLE_CALLEES|state|({'tooie_cpu625_retire'} if cpu625 else set()),
                      'actor': set('D_8007BCD0 D_8007BCD8 D_8007BCE8 D_8007BCF0 D_8007BEDA D_80124740 D_8012C820 D_8012C838 D_8012C850 D_8012C948 D_8012C968 D_8012CA18 D_8012CE58 D_8012CEC4 D_8012CF8C D_80132ED0 D_80135450 D_80136E70 D_80136EE0 func_800AE080 func_80100368 func_80104148 func_80104200'.split())|state}
    if cpu_root:expected_imports['actor'].add('D_8012C960')
    if npc665:
        for name in ('pair','bone','silo','actor'):expected_imports[name].add('tooie_npc665_state')
        expected_imports['pair'].add('tooie_npc665_end')
        expected_imports['bone']|={'tooie_npc665_bone_push','tooie_npc665_bone_pop'}
        expected_imports['leaf']|={'tooie_render_bridge_state','tooie_npc665_leaf_begin','tooie_npc665_leaf_end'}
    if backpack621:
        expected_imports['bone']|={'tooie_backpack621_state','tooie_backpack621_bone_push','tooie_backpack621_bone_pop'}
        expected_imports['leaf']|={'tooie_backpack621_state','tooie_backpack621_leaf_begin','tooie_backpack621_leaf_end'}
        expected_imports['lifecycle'].add('tooie_backpack621_retire')
    if talon_trot_toggle:expected_imports['lifecycle'].add('tooie_trot_toggle_retire')
    for name, (filename, definitions) in owners.items():
        obj = output/(name+'.o')
        command=flags+(['-DTOOIE_ACTOR_PALETTE='+str(int(cpu_root))] if name in ('actor','pair') else [])+(['-fno-jump-tables'] if name=='actor' else [])+(['-DTOOIE_HAND862=1'] if name=='actor' and hand862 else [])+['-c',win(source/filename),'-o',win(obj)]
        run(command, name+'-compile.log')
        with obj.open('rb') as stream:
            elf = ELFFile(stream); syms = symbols(elf)
            assert elf.elfclass == 32 and not elf.little_endian and elf['e_machine'] == 'EM_MIPS'
            actual = [n for n,s in syms.items() if s['st_info']['type']=='STT_FUNC' and s['st_shndx']!='SHN_UNDEF']
            assert actual == definitions, (name,actual)
            imports = {n for n,s in syms.items() if s['st_shndx']=='SHN_UNDEF'}
            if name in expected_imports: assert imports==expected_imports[name], ('closed imports',name,imports,expected_imports[name])
            if name == 'silo': assert imports <= {'tooie_render_bridge_state','D_80135450'}|({'tooie_npc665_state'} if npc665 else set()), imports
            for sec in elf.iter_sections():
                assert not (sec['sh_flags']&2 and sec['sh_size'] and sec.name not in
                            {'.recomp_patch','.tooie_render_bridge_state','.reginfo','.MIPS.abiflags'} | ({'.text.silo_preflight'} if name == 'bone' else {'.text.player_boundary'} if name == 'player' else {'.text.actor_palette'} if name == 'actor' else set())), (name,sec.name)
            text = elf.get_section_by_name('.text.player_boundary' if name == 'player' else '.text.actor_palette' if name == 'actor' else '.recomp_patch').data()
            for sec in elf.iter_sections():
                if not (sec['sh_flags'] & 4 and sec['sh_size']): continue
                assert sec.name in ({'.recomp_patch', '.text.silo_preflight'} if name == 'bone' else {'.text.player_boundary'} if name == 'player' else {'.text.actor_palette'} if name == 'actor' else {'.recomp_patch'})
                for i in range(0,sec['sh_size'],4):
                    word=int.from_bytes(sec.data()[i:i+4],'big')
                    assert not (word>>26==0 and word&63 in {0x1a,0x1b,0x34,0x0d}), ('division/trap opcode',name,sec.name,i,hex(word))
            assert all(r[1] in (2,4,5,6) for r in relocations(elf)), ('executable relocation closure',name)
            if name == 'silo':
                assert all(int.from_bytes(text[i:i+4],'big')>>26 != 17 for i in range(0,len(text),4)), 'setter COP1'
            if name == 'pair':
                calls = [r for r in relocations(elf) if r[2]=='func_800DE448']
                assert calls and all(c[1]==4 for c in calls)
                assert all(int.from_bytes(text[c[0]:c[0]+4],'big')>>26==3 for c in calls), 'every pair boundary must use JAL'
            if name == 'bridge' and honey_reference:
                with (root/honey_reference/'bridge.o').open('rb') as old_stream:
                    old=ELFFile(old_stream)
                    assert text==old.get_section_by_name('.recomp_patch').data(), 'Honey object text changed'
                    assert relocations(elf)==relocations(old), 'Honey object relocations changed'
            frames = {}
            for function in definitions:
                symbol = syms[function]
                section=elf.get_section(symbol['st_shndx'])
                instructions = section.data()[symbol['st_value']:symbol['st_value']+symbol['st_size']]
                adjustments = [int.from_bytes(instructions[i:i+4],'big') & 0xffff
                               for i in range(0,len(instructions),4)
                               if int.from_bytes(instructions[i:i+4],'big') >> 16 == 0x27bd]
                frames[function] = max((0x10000-v for v in adjustments if v & 0x8000), default=0)
            if name == 'bone' and not npc665 and not backpack621:
                assert frames=={'func_800DCF48':48, HELPER:56}, ('exact measured object frames',frames)
                assert sha(obj)=='7356a8f35224811c6fc8f5b730988fac05a003ca1d94c852dd174b0ef4dcf8a3', 'measured bone object changed'
                helper=syms[HELPER]; assert helper['st_info']['bind']=='STB_GLOBAL'
                code=elf.get_section(helper['st_shndx']).data()[helper['st_value']:helper['st_value']+helper['st_size']]
                assert __import__('hashlib').sha256(code).hexdigest()=='eb5c7f47930974014df369080ef90cd7b731aa9d06e9724d520ff0c3fa216fd9', 'retained Silo helper body changed'
                prologues=[i for i in range(0,len(code),4) if int.from_bytes(code[i:i+4],'big')==0x27BDFFC8]
                assert prologues==[0x1C0], ('helper shrinkwrapped frame',prologues)
                calls=[r for r in relocations(elf) if r[2]==HELPER]
                assert len(calls)==1 and calls[0][1:]==(4,HELPER), ('sole direct helper JAL',calls)
            receipt[name] = dict(sha256=sha(obj), definitions=actual, imports=sorted(imports),text_bytes=len(text))
            receipt[name]['frames'] = frames
            receipt[name]['compile_argv']=[str(x) for x in command]
            if name == 'pair': receipt[name]['pair_boundary_JAL_sites'] = [c[0] for c in calls]
    (output/'OBJECT_BINDING.json').write_text(json.dumps(receipt,indent=2)+'\n')
    return receipt


def image(root, output, run, honey_reference=HONEY_REFERENCE):
    source=root/'tools/baseline/guest/silo-candidate'
    object_receipt=compile_objects(root,output,run,honey_reference=honey_reference)
    heap=root/'build/windows-dev/baseline/heap-patch'
    objects=[heap/'boot.o',heap/'core1.o',root/'generated/heap-patch-products/heap.o']+[output/(n+'.o') for n in ('bridge','player','actor','bone','pair','silo','leaf','lifecycle')]
    linker=root/'deps/LLVM-19.1.3-Windows-X64/bin/ld.lld.exe'
    assert sha(linker)=='91da6d00511f43b86c8f9800888e6c66939a864e860bfda6594c1c186fd97a77'
    run([linker,'-m','elf32btsmip','-T',win(source/'render_bridge.ld'),'--unresolved-symbols=ignore-all','--emit-relocs','--orphan-handling=error']+
        [win(p) for p in objects]+['-o',win(output/'boot.elf')], 'bridge-link.log')
    with (output/'boot.elf').open('rb') as stream:
        elf=ELFFile(stream); syms=symbols(elf)
        assert [n for n,s in syms.items() if s['st_info']['type']=='STT_FUNC' and s['st_shndx']!='SHN_UNDEF']==GENERATED_FUNCTIONS
        assert {s.name for s in elf.iter_sections() if s['sh_flags']&2 and s['sh_size']}=={'.recomp_patch','.silo_helper','.player_helper','.actor_helper','.tooie_render_bridge_state'}
        expected=bytearray(18844); expected[:4]=bytes.fromhex('42524731'); expected[4:8]=(3).to_bytes(4,'big'); expected[32:36]=(1).to_bytes(4,'big')
        assert elf.get_section_by_name('.tooie_render_bridge_state').data()==expected
        state_vram=syms['tooie_render_bridge_state']['st_value']; assert state_vram%4==0 and state_vram+18844<=0x81000000
        linked_text=elf.get_section_by_name('.recomp_patch'); new_relocs=relocations(elf)
        if honey_reference:
            with (root/honey_reference/'boot.elf').open('rb') as old_stream:
                old=ELFFile(old_stream); old_syms=symbols(old); old_text=old.get_section_by_name('.recomp_patch')
                for name in accepted.BRIDGE_FUNCTIONS:
                    a,b=old_syms[name],syms[name]; assert a['st_size']==b['st_size']
                    left=bytearray(old_text.data()[a['st_value']-old_text['sh_addr']:a['st_value']-old_text['sh_addr']+a['st_size']])
                    right=bytearray(linked_text.data()[b['st_value']-linked_text['sh_addr']:b['st_value']-linked_text['sh_addr']+b['st_size']])
                    old_rs=[(o-a['st_value'],t,n) for o,t,n in relocations(old) if a['st_value']<=o<a['st_value']+a['st_size']]
                    new_rs=[(o-b['st_value'],t,n) for o,t,n in new_relocs if b['st_value']<=o<b['st_value']+b['st_size']]
                    assert old_rs==new_rs
                    for offset,kind,target in old_rs:
                        if target=='tooie_render_bridge_state':
                            assert kind in (5,6); left[offset+2:offset+4]=b'\0\0';right[offset+2:offset+4]=b'\0\0'
                    assert left==right, ('Honey linked text',name)
    data=toml.loads((heap/'data.syms.toml').read_text())
    # Author's explicit data imports are checked against the reviewed selected-symbol addresses.
    required=set().union(*(set(v['imports']) for v in object_receipt.values()))
    function_imports=accepted.BONE_CALLEES|accepted.PAIR_CALLEES|EXTRA_FUNCTIONS|LEAF_CALLEES|LIFECYCLE_CALLEES|set(FUNCTIONS)|set(HELPERS)
    old_imports=set(object_receipt['bridge']['imports'])
    function_imports |= {n for n in old_imports if n.startswith('func_') or n in {'bzero'}}
    function_imports |= old_imports
    data_names={n for n in required if n.startswith('D_')}
    assert data_names <= RENDER_DATA, ('unreviewed renderer data',data_names-RENDER_DATA)
    assert required-data_names-{'tooie_render_bridge_state'} <= function_imports, ('unapproved function imports',required-function_imports-data_names)
    existing_symbols=[s for section in data['section'] for s in section.get('symbols',[])]
    existing={s['name'] for s in existing_symbols}
    for name in data_names:
        address=int(name[2:],16)
        assert not any((s['name']==name and s['vram']!=address) or (s['vram']==address and s['name']!=name)
                       for s in existing_symbols), ('data owner collision',name)
    for name in sorted(data_names-existing):
        data['section'][0]['symbols'].append(dict(name=name,vram=int(name[2:],16)))
    (output/'data.syms.toml').write_text(toml.dumps(data))
    config=dict(input=dict(elf_path=str(output/'boot.elf'),output_func_path=str(output),single_file_output=True,
        func_reference_syms_file=str(root/'config/selected.syms.toml'),data_reference_syms_files=[str(output/'data.syms.toml')],
        output_binary_path=str(output/'boot.bin'),strict_patch_mode=True,unpaired_lo16_warnings=False,recomp_include='#include "recomp.h"'))
    (output/'boot.toml').write_text(toml.dumps(config));run([root/'deps/N64Recomp-codegen/build/N64Recomp',output/'boot.toml'],'bridge-codegen.log')
    binary=(output/'boot.bin').read_bytes();assert binary[:2760]==(heap/'boot.bin').read_bytes()
    assert binary[state_vram-0x80801000:state_vram-0x80801000+18844]==expected
    text=(output/'boot.c').read_text();assert re.findall(r'^RECOMP_FUNC void (\w+)\(',text,re.M)==GENERATED_FUNCTIONS
    spec=importlib.util.spec_from_file_location('extract',root/'tests/extract_heap_realloc.py');extract=importlib.util.module_from_spec(spec);spec.loader.exec_module(extract)
    for name in FUNCTIONS[:3]:assert extract.extract(text,name)==extract.extract((heap/'boot.c').read_text(),name)
    frames={}
    for name in ['func_800DCF48',HELPER]+PAIR+SETTERS+PLAYER_HELPERS+ACTOR_HELPERS+LEAF_FUNCTIONS+LIFECYCLE_FUNCTIONS:
        body=extract.extract(text,name)
        if name in LEAF_FUNCTIONS:
            assert body.count('LOOKUP_FUNC')==2, 'original DD998 callback pair'
        else:
            assert 'LOOKUP_FUNC' not in body
        m=re.search(r'ctx->r29 = ADD32\(ctx->r29, -0X([0-9A-F]+)\);',body)
        frames[name]=int(m[1],16) if m else 0
    assert frames['func_800DCF48']==48 and frames[HELPER]==56, ('exact translated frames',frames)
    bone=extract.extract(text,'func_800DCF48'); helper=extract.extract(text,HELPER)
    for offset,register in [(0x2C,31),(0x28,30)]+[(0x14+4*i,16+i) for i in range(5)]:
        assert f'MEM_W(0X{offset:X}, ctx->r29) = ctx->r{register};' in bone, ('DCF translated saved layout',offset,register)
    assert bone.count(HELPER+'(rdram, ctx);')==1
    assert re.findall(r'ctx->r29 = ADD32\(ctx->r29, -0X([0-9A-F]+)\);',helper)==['38']
    assert (output/'funcs.h').read_text().count('void '+HELPER+'(uint8_t* rdram, recomp_context* ctx);')==1
    assert (output/'recomp_overlays.inl').read_text().count('.func = '+HELPER+',')==1
    header='#pragma once\n#define TOOIE_BRIDGE_STATE_VRAM 0x%08Xu\n#define TOOIE_BRIDGE_IMAGE_BYTES %du\n#define TOOIE_BRIDGE_STATE_IMAGE_OFFSET %du\n'%(state_vram,len(binary),state_vram-0x80801000)
    (output/'render_bridge_layout.h').write_text(header)
    receipt=dict(condition='BASELINE-SILO-GUEST-HELD-CANDIDATE',state_vram=state_vram,state_bytes=18844,prefix_bytes=2760,
        definitions=FUNCTIONS,helper_definitions=HELPERS,raw_omissions=FUNCTIONS[3:],helper_frame_bytes=frames[HELPER],helper_prologue_offset=0x1C0,bone_frame_bytes=frames['func_800DCF48'],leaf_frame_bytes=frames['func_800DD998'],pair_frames={n:frames[n] for n in PAIR},player_frames={n:frames[n] for n in PLAYER_HELPERS},actor_frames={n:frames[n] for n in ACTOR_HELPERS},lifecycle_frames={n:frames[n] for n in LIFECYCLE_FUNCTIONS},frames=frames,
        honey_reference=honey_reference,honey_object_text_relocations_identical=bool(honey_reference),honey_linked_text_only_state_HI_LO=bool(honey_reference),
        object_binding_sha256=sha(output/'OBJECT_BINDING.json'),inputs={str(p):sha(p) for p in objects+list(source.iterdir()) if p.is_file()},
        outputs={p.name:sha(p) for p in output.iterdir() if p.is_file()})
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--root',type=Path,required=True);parser.add_argument('--raw',action='store_true');parser.add_argument('--image',action='store_true');parser.add_argument('--objects',action='store_true')
    parser.add_argument('--no-prior-honey',action='store_true',help='Maintained recipe: no historical draw-boundaries image; the camera image pin binds the result');args=parser.parse_args()
    root=args.root.resolve();output=root/'build/windows-dev/baseline/render-bridge-silo-candidate';output.mkdir(exist_ok=True)
    assert sha(root/'deps/N64Recomp-codegen/build/N64Recomp')=='34958d6ae8c047adb772090c75fdcc051036646a1db58595694f814c3a4a09b6'
    def run(command,label):
        with (output/label).open('w') as log:subprocess.run(list(map(str,command)),cwd=root,stdout=log,stderr=subprocess.STDOUT,check=True)
    if args.raw:
        accepted.FUNCTIONS=FUNCTIONS;accepted.OMISSIONS=FUNCTIONS[3:];accepted.PAIR_FUNCTIONS=PAIR+SETTERS
        receipt=accepted.prepare_raw(root,output,run);(output/'RAW_BINDING.json').write_text(json.dumps(receipt,indent=2)+'\n');print('raw',sha(output/'RAW_BINDING.json'))
    honey_reference=None if args.no_prior_honey else HONEY_REFERENCE
    if args.image:image(root,output,run,honey_reference);print('image',sha(output/'receipt.json'))
    elif args.objects:compile_objects(root,output,run,honey_reference=honey_reference);print('objects',sha(output/'OBJECT_BINDING.json'))


if __name__=='__main__':main()
