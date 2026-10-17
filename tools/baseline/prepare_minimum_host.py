"""Finite HOST/G07 object and linked-state proof; no native command policy."""
import hashlib
import sys
from elftools.elf.elffile import ELFFile
from prepare_cpu625 import facts  # Reuse ELF facts only; this selects no CPU owner.

HELPER='tooie_baseline_cutscene_command'
PROJECTION='func_80015D14'
NATIVE_IMPORTS={'tooie_get_latched_widescreen_settings':0x8F000020,
                'tooie_baseline_presentation_settings':0x8F000024,
                'tooie_baseline_presentation_publish':0x8F000028,
                'tooie_baseline_quiet_talon_trot':0x8F00002C}
# Independently accepted image hashes are installed only after linked evidence.
PRE_CPU_OFF_SHA256=None
PRE_CPU_ON_SHA256='896ddc57ba43db4f5ef368d27eb9e9fc55015854ec12282cb2664d59f53c07e1'

def compile_objects(root,out,run,flags,win):
    source=root/'tools/baseline/guest/minimum-host'
    commands={
        'minimum_cutscene':flags+['-DTOOIE_BASELINE_HOST=1','-I'+win(source),'-c',win(source/'minimum_cutscene.c'),'-o',win(out/'minimum_cutscene.o')],
        'native_aspect_projection':flags+['-DTOOIE_BASELINE_HOST=1','-I'+win(root/'patches/game'),'-fno-jump-tables','-c',win(root/'patches/game/native_aspect_projection.c'),'-o',win(out/'native_aspect_projection.o')],
    }
    records={}
    for name,command in commands.items():
        run(command,name+'-compile.log');record=facts(out/(name+'.o'));record['compile_argv']=[str(x) for x in command];records[name]=record
    host=records['minimum_cutscene']
    assert host['definitions']==[HELPER]
    assert set(host['imports'])=={'set_widescreen','tooie_baseline_presentation_settings','tooie_baseline_presentation_publish'}
    assert set(host['allocated_sections'])=={'.baseline_host_helpers','.baseline_host_state','.reginfo','.MIPS.abiflags'}
    assert host['allocated_sections']['.baseline_host_state']['type']=='SHT_PROGBITS'
    assert bytes.fromhex(host['section_bytes']['.baseline_host_state'])==bytes(8)
    assert host['frames']=={HELPER:40}
    metadata=[row for row in host['relocations'] if row['section']=='.pdr']
    assert metadata==[dict(section='.pdr',offset=0,kind=2,target=HELPER)]
    assert all(row['section']=='.baseline_host_helpers' and row['kind'] in (4,5,6) for row in host['relocations'] if row['section']!='.pdr')
    # Reuse the maintained G07 object validator and the actual existing contract.
    sys.path.insert(0,str(root/'tools'))
    from render_wrapper_contract import OBJECT_CONTRACTS,object_parameters
    from render_wrapper_validator import validate_heap_object
    contract=next(row for row in OBJECT_CONTRACTS if row['stem']=='native_aspect_projection')
    data,functions,calls,storage,native=object_parameters(contract)
    validate_heap_object(out/'native_aspect_projection.o',data,functions,calls,storage=storage,native=native,aspect_literals=True)
    projection=records['native_aspect_projection']
    assert projection['definitions']==[PROJECTION] and projection['frames']=={PROJECTION:48}
    assert set(projection['imports'])=={'widescreen_enabled','D_80041710','tooie_get_latched_widescreen_settings'}
    for name,paths in {
        'minimum_cutscene':[source/'minimum_cutscene.c',source/'minimum_presentation_abi.h'],
        'native_aspect_projection':[root/'patches/game/native_aspect_projection.c',root/'patches/game/widescreen_settings_abi.h'],
    }.items():
        records[name]['inputs']={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    return records

def linked(elf):
    table=elf.get_section_by_name('.symtab');symbols={s.name:s for s in table.iter_symbols() if s.name}
    section=elf.get_section_by_name('.baseline_host_helpers');state=elf.get_section_by_name('.baseline_host_state');literals=elf.get_section_by_name('.baseline_g07_literals')
    assert section and state and literals and state['sh_type']=='SHT_PROGBITS'
    assert state['sh_size']==8 and state.data()==bytes(8) and state['sh_addr']==section['sh_addr']+section['sh_size']
    assert literals['sh_size']==24 and literals['sh_addr']%8==0 and literals['sh_addr']>=state['sh_addr']+8
    assert literals['sh_addr']+24<=0x81000000
    assert symbols[HELPER]['st_shndx']==elf.get_section_index(section.name)
    assert symbols[PROJECTION]['st_shndx']==elf.get_section_index('.recomp_patch')
    assert {name:symbols[name]['st_value'] for name in NATIVE_IMPORTS}==NATIVE_IMPORTS
    return dict(rom=section['sh_addr']-0x80801000,ram=section['sh_addr'],size=section['sh_size'],index=elf.get_section_index(section.name),
                state=state['sh_addr'],state_bytes=8,literals=literals['sh_addr'],literal_bytes=24,image_end=literals['sh_addr']+24)
