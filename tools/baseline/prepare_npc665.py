"""Finite NPC665 ELF/codegen checks, reusing the maintained CPU object reader."""
from prepare_cpu625 import facts

HELPERS = ['tooie_npc665_capture','tooie_npc665_retire','tooie_npc665_admit',
           'tooie_npc665_bone_push','tooie_npc665_bone_pop',
           'tooie_npc665_leaf_begin','tooie_npc665_leaf_end','tooie_npc665_end']
END = HELPERS[-1]
CHECKED = 'baseline_npc665_end_checked'  # native absolute import; sole caller of END
CHECKED_VRAM = 0x8F000034                # render_bridge.ld, manual patch-symbol range
# Installed only after actual composed source/ELF/provider/results acceptance.
PRE_IMAGE_SHA256 = '8a0b86b7b8f6a1ef8672d1c1d6a0b3698a50c0faff347e19f3a54104dd4fcbbb'
IMPORTS = set('''D_8007BCD0 D_8007BCD8 D_8007BCE8 D_8007BCF0 D_8007BEDA
 D_8011B910 D_80127674 D_8012B800 D_8012BE62 D_8012BE64 D_8012C820 D_8012C824
 D_8012C838 D_8012C840 D_8012C850 D_8012C948 D_8012C968 D_8012CA18 D_8012CE58
 D_8012CEC4 D_8012CF8C D_8012D500 D_8012D508 D_8012D558 D_80132ED0
 D_80135450 D_80136E70 D_80136EE0 func_800AE080 func_80104200
 func_800A89F8 func_800EB854 tooie_camera_state tooie_pre_background
 tooie_render_bridge_state'''.split())

def compile_owner(root,out,run,flags,win):
    source=root/'tools/baseline/guest/silo-candidate/npc665_owner.c'
    obj=out/'npc665.o'
    command=flags+['-c',win(source),'-o',win(obj)]
    run(command,'npc665-compile.log')
    record=facts(obj)
    assert record['definitions']==HELPERS,record['definitions']
    assert set(record['imports'])==IMPORTS,record['imports']
    assert set(record['allocated_sections'])=={'.tooie_npc665_helpers','.tooie_npc665_state','.reginfo','.MIPS.abiflags'}
    assert record['allocated_sections']['.tooie_npc665_state']['type']=='SHT_PROGBITS'
    assert bytes.fromhex(record['section_bytes']['.tooie_npc665_state'])==bytes(12)+bytes.fromhex('66500000')
    assert [x for x in record['relocations'] if x['section']=='.pdr']==[
        dict(section='.pdr',offset=32*i,kind=2,target=name) for i,name in enumerate(HELPERS)]
    assert all(x['section']=='.tooie_npc665_helpers' and x['kind'] in (4,5,6)
               for x in record['relocations'] if x['section']!='.pdr')
    record['compile_argv']=[str(x) for x in command]
    record['inputs']={str(p):__import__('hashlib').sha256(p.read_bytes()).hexdigest()
                      for p in (source,source.parent/'npc665_owner.h')}
    return record

def linked(elf,previous_end):
    symbols={s.name:s for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
    code=elf.get_section_by_name('.tooie_npc665_helpers')
    state=elf.get_section_by_name('.tooie_npc665_state')
    assert code and state and state['sh_type']=='SHT_PROGBITS'
    assert code['sh_addr']==previous_end and state['sh_addr']==code['sh_addr']+code['sh_size']
    assert state.data()==bytes(12)+bytes.fromhex('66500000') and state['sh_addr']+16<=0x81000000
    assert all(symbols[n]['st_shndx']==elf.get_section_index(code.name) and symbols[n]['st_size'] for n in HELPERS)
    checked=symbols[CHECKED]
    assert checked['st_shndx']=='SHN_ABS' and checked['st_value']==CHECKED_VRAM and checked['st_size']==0
    return dict(rom=code['sh_addr']-0x80801000,ram=code['sh_addr'],size=code['sh_size'],
                index=elf.get_section_index(code.name),state=state['sh_addr'],state_bytes=16,
                end=symbols[END]['st_value'])

def checked_end(out,pair_object):
    """Read-only: the pair reaches END only through the native import; generated text is not edited."""
    # The compiled pair's actual JAL relocations, not a prose/guessed call count.
    calls=[x for x in facts(pair_object)['relocations'] if x['target'] in (END,CHECKED)]
    assert calls and all(x['target']==CHECKED and x['kind']==4 and x['section']=='.recomp_patch' for x in calls),calls
    linked=facts(out/'boot.elf')['relocations']
    assert not any(x['target']==END for x in linked), 'END has no static caller or address use'
    imports=[x for x in linked if x['target']==CHECKED]
    assert len(imports)==len(calls) and all(x['kind']==4 and x['section']=='.recomp_patch' for x in imports),imports
    text=(out/'boot.c').read_text()
    assert text.count('    '+CHECKED+'(rdram, ctx);')==len(calls) and END+'(rdram, ctx);' not in text
    assert text.count('RECOMP_FUNC void '+END+'(')==1 and 'RECOMP_FUNC void '+CHECKED+'(' not in text
    header=(out/'funcs.h').read_text()
    assert header.count('void '+END+'(uint8_t* rdram, recomp_context* ctx);')==1 and CHECKED not in header
    overlay=(out/'recomp_overlays.inl').read_text()
    assert overlay.count('.func = '+END+',')==1 and CHECKED not in overlay
    # The import VRAM appears only as R_MIPS_26 rows into the absolute section, never as a FuncEntry.
    row='.target_section_offset = 0x%08X, .target_section = 65534, .type = R_MIPS_26 }'%CHECKED_VRAM
    assert overlay.count('0x%08X'%CHECKED_VRAM)==overlay.count(row)==len(calls)
    return dict(pair_jal_count=len(calls),pair_jal_relocations=calls,import_vram=CHECKED_VRAM,
                checked_calls=len(calls),guest_end=END,provider=END)
