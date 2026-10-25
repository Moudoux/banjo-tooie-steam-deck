"""Qualify five FPS replacements and one appended input capture provider."""
import hashlib, json
from elftools.elf.elffile import ELFFile
from prepare_cpu625 import facts

BODIES=['func_80804848_bsfirstp','func_80808E78_bsfirstp','func_808099B4_bsfirstp',
        'func_808069FC_bsfirstp','func_80806928_bsfirstp']
HELPERS=['tooie_modern_fps_capture']
IMPORTS=set('func_800EA05C func_800A9CAC func_80091E80 tooie_baseline_first_person_aim '
    'func_80804488_bsfirstp func_808045FC_bsfirstp _baeggcursor_entrypoint_10 '
    'func_80801928_bsfirstp func_80801DC0_bsfirstp func_808023A0_bsfirstp '
    'func_80808DB0_bsfirstp func_80809D24_bsfirstp func_80800830_bsfirstp '
    'func_80808EF0_bsfirstp func_80802F20_bsfirstp func_80809A10_bsfirstp '
    'func_80809A7C_bsfirstp func_8080682C_bsfirstp func_808068E8_bsfirstp '
    'func_80809D94_bsfirstp func_80013728 func_800F122C func_80806AD0_bsfirstp '
    'bakey_getControllerIndex bastick_getX bastick_getY'.split())
# Measured from the pinned compiler and independently decoded from its ELF.
# The complete image is accepted separately after translated execution checks.
QUALIFIED_FRAMES={
    'tooie_modern_fps_capture':32, 'func_80804848_bsfirstp':56,
    'func_80808E78_bsfirstp':40, 'func_808099B4_bsfirstp':24,
    'func_808069FC_bsfirstp':40, 'func_80806928_bsfirstp':40,
}
QUALIFIED_LITERALS='42c8000046fffe003f800000bf8000003f80000041400000'
# LLD merges the two identical 1.0f entries; relocation targets are qualified.
QUALIFIED_LINKED_LITERALS='42c8000046fffe00bf8000003f80000041400000'
QUALIFIED_IMAGE_SHA256='a0ce3e34290565647df70cb54bc72de2101ca2828c86309162ba91c1df4642c7'

def compile_owner(root,out,run,flags,win):
    source=root/'tools/baseline/guest/minimum-host/modern_fps.c'
    obj=out/'modern_fps.o'
    command=flags+['-fno-jump-tables','-I'+win(root/'deps/banjo-tooie/src'),
        '-I'+win(source.parent),'-c',win(source),'-o',win(obj)]
    run(command,'modern-fps-compile.log')
    record=facts(obj)
    record['compile_argv']=[str(x) for x in command]
    record['inputs']={str(p):hashlib.sha256(p.read_bytes()).hexdigest()
        for p in (source,source.parent/'first_person_aim_abi.h',root/'tools/baseline/prepare_modern_fps.py',
                  root/'tools/baseline/modern_fps_hooks.py')}
    (out/'MODERN_FPS_OBJECT_UNQUALIFIED.json').write_text(json.dumps(record,indent=2)+'\n')
    with obj.open('rb') as stream:
        elf=ELFFile(stream)
        assert elf.elfclass==32 and not elf.little_endian and elf['e_machine']=='EM_MIPS'
        assert elf['e_flags']&0xF0000000==0x10000000 and elf['e_flags']&0xF000==0x1000
        assert not elf['e_flags']&0x20
        symbols=list(elf.get_section_by_name('.symtab').iter_symbols())
        assert set(record['definitions'])==set(BODIES+HELPERS)
        assert set(record['imports'])==IMPORTS,record['imports']
        for symbol in symbols:
            if symbol.name not in BODIES+HELPERS: continue
            section=elf.get_section(symbol['st_shndx'])
            assert section.name==('.recomp_patch' if symbol.name in BODIES else '.tooie_modern_fps_helpers')
            start=symbol['st_value']-section['sh_addr']
            code=section.data()[start:start+symbol['st_size']]
            words=[int.from_bytes(code[n:n+4],'big') for n in range(0,len(code),4)]
            assert not any(w>>26==0 and w&63==9 for w in words),'Unexpected indirect JALR'
            assert not any((w>>26 in [15,32,33,35,36,37,40,41,43]) and
                ((w>>16&31)==28 if w>>26==15 else (w>>21&31)==28) for w in words),'GP dependency'
        allowed={'.recomp_patch','.tooie_modern_fps_helpers','.tooie_modern_fps_state',
                 '.reginfo','.MIPS.abiflags','.rodata.cst4'}
        assert set(record['allocated_sections'])<=allowed
        assert allowed-{'.rodata.cst4'}<=set(record['allocated_sections'])
        state=elf.get_section_by_name('.tooie_modern_fps_state')
        assert state['sh_type']=='SHT_PROGBITS' and state.data()==bytes(28)
        slot=next(s for s in symbols if s.name=='tooie_modern_fps_scope')
        assert slot['st_size']==28 and slot['st_value']==0
    assert QUALIFIED_FRAMES is not None and record['frames']==QUALIFIED_FRAMES,(
        'Install independently measured FPS object frames after fixture',record['frames'])
    assert QUALIFIED_LITERALS is not None and record['section_bytes'].get('.rodata.cst4','')==QUALIFIED_LITERALS,(
        'Install exact independently inspected FPS literals',record['section_bytes'].get('.rodata.cst4',''))
    assert all(r['section'] in ('.pdr','.recomp_patch','.tooie_modern_fps_helpers') and
        r['kind'] in ((2,) if r['section']=='.pdr' else (4,5,6)) for r in record['relocations'])
    return record

def linked(elf,prior_end):
    symbols={s.name:s for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
    code=elf.get_section_by_name('.tooie_modern_fps_helpers')
    state=elf.get_section_by_name('.tooie_modern_fps_state')
    literals=elf.get_section_by_name('.tooie_modern_fps_literals')
    assert code and state and code['sh_addr']==((prior_end+3)&~3)
    assert state['sh_type']=='SHT_PROGBITS' and state.data()==bytes(28)
    assert state['sh_addr']==code['sh_addr']+code['sh_size']
    slot=symbols['tooie_modern_fps_scope']
    assert slot['st_value']==state['sh_addr'] and slot['st_size']==28
    assert all(symbols[n]['st_shndx']==elf.get_section_index('.recomp_patch') for n in BODIES)
    assert all(symbols[n]['st_shndx']==elf.get_section_index(code.name) for n in HELPERS)
    assert symbols['tooie_baseline_first_person_aim']['st_shndx']=='SHN_ABS'
    assert symbols['tooie_baseline_first_person_aim']['st_value']==0x8F000038
    expected=bytes.fromhex(QUALIFIED_LINKED_LITERALS)
    if expected:
        assert literals and literals.data()==expected and literals['sh_addr']==state['sh_addr']+28
        end=literals['sh_addr']+len(expected)
    else:
        assert not literals or not literals['sh_size']
        end=state['sh_addr']+28
    assert end<=0x81000000
    return dict(rom=code['sh_addr']-0x80801000,ram=code['sh_addr'],size=code['sh_size'],
        index=elf.get_section_index(code.name),state=state['sh_addr'],state_bytes=28,image_end=end)
