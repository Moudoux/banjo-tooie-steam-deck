"""Finite621 owner proof; use the existing CPU object reader and registry flow."""
import hashlib
from prepare_cpu625 import facts

HELPERS = ['tooie_backpack621_begin','tooie_backpack621_admit',
           'tooie_backpack621_bone_push','tooie_backpack621_bone_pop',
           'tooie_backpack621_leaf_begin','tooie_backpack621_leaf_end',
           'tooie_backpack621_end','tooie_backpack621_retire']
PRE_IMAGE_SHA256 = "ec6d5739e438a87421ff751253d007ecfe2697ff65e4e9fddee87b799412c93c"  # Accepted exact original621/NPC/HOST/CPU composition.
# Selected composition: shadow_projection.o built with TOOIE_SHADOW_COEFFICIENTS=1 and actor.o with TOOIE_HAND862=1.
SHADOW_COEFFICIENTS_IMAGE_SHA256 = "c78e92102a8e20e67db4ea5e3faf8eb09c2261f2da120a63470d3ce2846fa6b9"
INITIAL = bytes.fromhex('3632313100000001') + bytes(604)
IMPORTS = set('''D_8007BCD0 D_8007BCD8 D_8007BCE8 D_8007BCF0 D_8007BEDA
 D_80127674 D_8012C838 D_8012C948 D_8012C968 D_8012CE58 D_8012CEC4
 D_8012CF8C D_8012D500 D_8012D508 D_8012D558 D_80132E9C
 D_80135490 D_801354B0 D_801354DC func_80100A74 func_80100AC4
 tooie_camera_main tooie_camera_state tooie_pre_background tooie_render_bridge_state'''.split())

def compile_owner(root,out,run,flags,win,selected):
    assert type(selected) is bool
    source=root/'tools/baseline/guest/backpack621-candidate/backpack621_owner.c'
    obj=out/'backpack621.o'
    command=flags+['-c',win(source),'-o',win(obj)]
    run(command,'backpack621-compile.log')
    record=facts(obj)
    record['compile_argv']=[str(x) for x in command]
    record['inputs']={str(p):hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in (source,source.parent/'backpack621_owner.h')}
    if not selected:
        assert not record['definitions'] and not record['imports']
        assert not any(name.startswith('.tooie_backpack621') for name in record['allocated_sections'])
        return record  # Never linked in the OFF image.
    assert record['definitions']==HELPERS,record['definitions']
    assert set(record['imports'])==IMPORTS,record['imports']
    assert set(record['allocated_sections'])=={
        '.tooie_backpack621_helpers','.tooie_backpack621_state','.reginfo','.MIPS.abiflags'}
    assert record['allocated_sections']['.tooie_backpack621_state']['type']=='SHT_PROGBITS'
    assert bytes.fromhex(record['section_bytes']['.tooie_backpack621_state'])==INITIAL
    assert [x for x in record['relocations'] if x['section']=='.pdr']==[
        dict(section='.pdr',offset=32*i,kind=2,target=name) for i,name in enumerate(HELPERS)]
    assert all(x['section']=='.tooie_backpack621_helpers' and x['kind'] in (4,5,6)
               for x in record['relocations'] if x['section']!='.pdr')
    return record

def linked(elf,previous_end):
    symbols={s.name:s for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
    code=elf.get_section_by_name('.tooie_backpack621_helpers')
    state=elf.get_section_by_name('.tooie_backpack621_state')
    assert code and state and state['sh_type']=='SHT_PROGBITS'
    assert code['sh_addr']==previous_end and state['sh_addr']==code['sh_addr']+code['sh_size']
    assert state.data()==INITIAL and state['sh_addr']+612<=0x81000000
    assert symbols['tooie_backpack621_state']['st_value']==state['sh_addr']
    assert symbols['tooie_backpack621_state']['st_size']==612
    assert all(symbols[n]['st_shndx']==elf.get_section_index(code.name) and symbols[n]['st_size']
               for n in HELPERS)
    return dict(rom=code['sh_addr']-0x80801000,ram=code['sh_addr'],size=code['sh_size'],
                index=elf.get_section_index(code.name),state=state['sh_addr'],state_bytes=612,
                prior_end=previous_end,image_end=state['sh_addr']+612)
