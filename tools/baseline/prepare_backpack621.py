"""Finite621 owner proof; use the existing CPU object reader and registry flow."""
import hashlib
from prepare_cpu625 import facts
from elftools.elf.relocation import RelocationSection

HELPERS = ['tooie_backpack621_begin','tooie_backpack621_admit',
           'tooie_backpack621_bone_push','tooie_backpack621_bone_pop',
           'tooie_backpack621_leaf_begin','tooie_backpack621_leaf_end',
           'tooie_backpack621_end','tooie_backpack621_retire']
# L06b-2/L06c-3: under BK no transport, lifecycle call or label body reaches any of the eight (the set_state/
# init hooks and lifecycle calls are BK-OFF), so the BK image links no 621 section or state; 621 stays selected.
BK_RETIRED = HELPERS
assert HELPERS[7:]==['tooie_backpack621_retire']

def section_present(bk_pipeline):
    """L06c-3 section-presence contract: the 621 section and state exist only in the BK-OFF image."""
    assert type(bk_pipeline) is bool
    return not bk_pipeline

def helpers(bk_pipeline):
    assert type(bk_pipeline) is bool
    return HELPERS if section_present(bk_pipeline) else []
PRE_IMAGE_SHA256 = "e34920d5b259d17cfcf67e86b3f4f130462c3c11a5aaddf24afdd982e8977eb8"  # Accepted exact original621/NPC/HOST/CPU composition.
# Selected composition: shadow_projection.o built with TOOIE_SHADOW_COEFFICIENTS=1 and actor.o with TOOIE_HAND862=1.
SHADOW_COEFFICIENTS_IMAGE_SHA256 = "3ab2614823ac33e1d6f271aeb0ba6a34f8457efcbf5b75da6d2c4b952f7c7e7e"  # Historical quiet-only; unchanged.
TROT_TOGGLE_IMAGE_SHA256 = "2fb1ebec41abebdcdb3b3b6168e3940048c4fc090533c2b0a8ac47f8c7af79ce"
INITIAL = bytes.fromhex('3632313100000001') + bytes(604)
IMPORTS = set('''D_8007BCD0 D_8007BCD8 D_8007BCE8 D_8007BCF0 D_8007BEDA
 D_80127674 D_8012C838 D_8012C948 D_8012C968 D_8012CE58 D_8012CEC4
 D_8012CF8C D_8012D500 D_8012D508 D_8012D558 D_80132E9C
 D_80135490 D_801354B0 D_801354DC func_80100A74 func_80100AC4
 tooie_camera_main tooie_camera_state tooie_pre_background tooie_render_bridge_state'''.split())

def compile_owner(root,out,run,flags,win,selected,bk_pipeline=False):
    assert type(selected) is bool and type(bk_pipeline) is bool
    assert not (selected and bk_pipeline), 'L06c-3: the BK image links no backpack621 object (section_present)'
    source=root/'tools/baseline/guest/backpack621-candidate/backpack621_owner.c'
    obj=out/'backpack621.o'
    command=flags+['-DTOOIE_BK_PIPELINE='+str(int(bk_pipeline)),'-c',win(source),'-o',win(obj)]
    run(command,'backpack621-compile.log')
    record=facts(obj)
    record['compile_argv']=[str(x) for x in command]
    record['inputs']={str(p):hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in (source,source.parent/'backpack621_owner.h')}
    if not selected:
        assert not record['definitions'] and not record['imports']
        assert not any(name.startswith('.tooie_backpack621') for name in record['allocated_sections'])
        return record  # Never linked in the OFF image.
    names=helpers(bk_pipeline)
    assert record['definitions']==names,record['definitions']
    assert set(record['imports'])==IMPORTS,record['imports']
    assert set(record['allocated_sections'])=={
        '.tooie_backpack621_helpers','.tooie_backpack621_state','.reginfo','.MIPS.abiflags'}
    assert record['allocated_sections']['.tooie_backpack621_state']['type']=='SHT_PROGBITS'
    assert bytes.fromhex(record['section_bytes']['.tooie_backpack621_state'])==INITIAL
    assert [x for x in record['relocations'] if x['section']=='.pdr']==[
        dict(section='.pdr',offset=32*i,kind=2,target=name) for i,name in enumerate(names)]
    assert all(x['section']=='.tooie_backpack621_helpers' and x['kind'] in (4,5,6)
               for x in record['relocations'] if x['section']!='.pdr')
    return record

def linked(elf,previous_end,bk_pipeline=False):
    assert bk_pipeline is False, 'L06c-3: the BK image links no backpack621 section (see absent)'
    symbols={s.name:s for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
    code=elf.get_section_by_name('.tooie_backpack621_helpers')
    state=elf.get_section_by_name('.tooie_backpack621_state')
    assert code and state and state['sh_type']=='SHT_PROGBITS'
    assert code['sh_addr']==previous_end and state['sh_addr']==code['sh_addr']+code['sh_size']
    assert state.data()==INITIAL and state['sh_addr']+612<=0x81000000
    assert symbols['tooie_backpack621_state']['st_value']==state['sh_addr']
    assert symbols['tooie_backpack621_state']['st_size']==612
    assert all(symbols[n]['st_shndx']==elf.get_section_index(code.name) and symbols[n]['st_size']
               for n in helpers(bk_pipeline))
    return dict(rom=code['sh_addr']-0x80801000,ram=code['sh_addr'],size=code['sh_size'],
                index=elf.get_section_index(code.name),state=state['sh_addr'],state_bytes=612,
                prior_end=previous_end,image_end=state['sh_addr']+612)

def absent(elf):
    """L06c-3 BK link: no 621 section, and no helper or state symbol defined, imported or relocated
    (the link uses --unresolved-symbols=ignore-all, so undefined references must be refused here)."""
    forbidden=set(HELPERS)|{'tooie_backpack621_state'}
    assert not elf.get_section_by_name('.tooie_backpack621_helpers') and not elf.get_section_by_name('.tooie_backpack621_state'), 'L06c-3: BK links no backpack621 section'
    names={s.name for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
    assert not forbidden&names, ('L06c-3: BK symbol table names a 621 helper or the state',sorted(forbidden&names))
    for sec in elf.iter_sections():
        if isinstance(sec,RelocationSection):
            table=elf.get_section(sec['sh_link'])
            assert not any(table.get_symbol(r['r_info_sym']).name in forbidden for r in sec.iter_relocations()), ('L06c-3: BK relocation to a 621 symbol',sec.name)
    return dict(selected=True,section_present=False)
