"""Finite HOST projection/live-wide object proof; no native command policy."""
import hashlib
import json
import sys
from elftools.elf.elffile import ELFFile
from prepare_cpu625 import facts  # Reuse ELF facts only; this selects no CPU owner.

HELPER='tooie_baseline_cutscene_command'
FRAME='tooie_baseline_widescreen_frame'
TICKER_CLAIM='tooie_baseline_cutscene_ticker_claim'
TICKER_RETIRE='tooie_baseline_cutscene_ticker_retire'
HELPERS=[FRAME,HELPER,TICKER_CLAIM,TICKER_RETIRE]
# generation, glcut cause, ticker owner.
HOST_STATE_BYTES=12
# Unmeasured pins are None; the exact assertions below fail until root measures.
ROOT_PENDING=None
# Measured in the strict -fno-jump-tables live Wide object with both causes.
HOST_FRAMES={FRAME:40,HELPER:40,TICKER_CLAIM:40,TICKER_RETIRE:8}
PROJECTION='func_80015D14'
MUSIC_BODIES=['func_80025ED0','func_800123BC']
MUSIC_IMPORTS={'D_80079FA0','func_80026500','func_800A7D84','func_800A7840',
               'func_800A5D1C','tooie_baseline_music_volume'}
MUSIC_INITIAL=bytes(16)+(100).to_bytes(4,'big')
MUSIC_ORIGINAL_ELF_SHA256='331fe74764f14ef480688da128bb0c75ee388c416e6af7de38beff7defb8373f'
# Unqualified: root must install independently inspected object frames/image evidence.
MUSIC_FRAMES={'func_80025ED0':56,'func_800123BC':48}
# Pre-Q15/Q16/Q30 selected image was 7917a5cdbdc6abb2a9b6eafa96265e3a729501b4e7f4cad32d5c15ef012a8592.
MUSIC_SELECTED_IMAGE_SHA256='6845c8ced20a737a46324e7d388cf9c73e07d6f32d8dda8a71467bb8661f5bc8'
NATIVE_IMPORTS={'tooie_get_latched_widescreen_settings':0x8F000020,
                'tooie_baseline_presentation_settings':0x8F000024,
                'tooie_baseline_presentation_publish':0x8F000028,
                'tooie_baseline_quiet_talon_trot':0x8F00002C,
                'tooie_baseline_trot_toggle':0x8F000030,
                'tooie_baseline_first_person_aim':0x8F000038,
                'tooie_baseline_music_volume':0x8F000040}
# Independently accepted image hashes are installed only after linked evidence.
PRE_CPU_OFF_SHA256=None
PRE_CPU_ON_SHA256='896ddc57ba43db4f5ef368d27eb9e9fc55015854ec12282cb2664d59f53c07e1'

def music_data_identity(root):
    # Named original identity, not a guest literal or an invented object extent.
    path=root/'deps/banjo-tooie/build/us/banjotooie_decompressed.elf'
    assert hashlib.sha256(path.read_bytes()).hexdigest()==MUSIC_ORIGINAL_ELF_SHA256
    with path.open('rb') as stream:
        elf=ELFFile(stream);rows=elf.get_section_by_name('.symtab').get_symbol_by_name('D_80079FA0')
        assert rows and len(rows)==1 and rows[0]['st_value']==0x80079FA0
        assert rows[0]['st_info']['bind']=='STB_GLOBAL' and rows[0]['st_info']['type']=='STT_NOTYPE' and rows[0]['st_size']==0
        section=elf.get_section(rows[0]['st_shndx'])
        assert section.name=='.core1_bss' and section['sh_type']=='SHT_NOBITS'
    return path


def compile_objects(root,out,run,flags,win):
    source=root/'tools/baseline/guest/minimum-host'
    data_source=music_data_identity(root)
    commands={
        'music_gain':flags+['-DTOOIE_BASELINE_HOST=1','-I'+win(source),'-fno-jump-tables','-c',win(source/'music_gain.c'),'-o',win(out/'music_gain.o')],
        'minimum_cutscene':flags+['-DTOOIE_BASELINE_HOST=1','-I'+win(source),'-fno-jump-tables','-c',win(source/'minimum_cutscene.c'),'-o',win(out/'minimum_cutscene.o')],
        'native_aspect_projection':flags+['-DTOOIE_BASELINE_HOST=1','-I'+win(root/'patches/game'),'-fno-jump-tables','-c',win(root/'patches/game/native_aspect_projection.c'),'-o',win(out/'native_aspect_projection.o')],
    }
    records={}
    for name,command in commands.items():
        run(command,name+'-compile.log');record=facts(out/(name+'.o'));record['compile_argv']=[str(x) for x in command];records[name]=record
    music=records['music_gain']
    (out/'MUSIC_OBJECT_UNQUALIFIED.json').write_text(json.dumps(music,indent=2)+'\n')
    with (out/'music_gain.o').open('rb') as stream:
        elf=ELFFile(stream);table=elf.get_section_by_name('.symtab')
        assert elf.elfclass==32 and not elf.little_endian and elf['e_machine']=='EM_MIPS'
        assert elf['e_flags']&0xF000F020==0x10001000, 'MIPS-II o32 music ABI'
        state=table.get_symbol_by_name('tooie_music_state')
        assert state and len(state)==1 and state[0]['st_size']==20
        assert state[0]['st_info']['type']=='STT_OBJECT' and state[0]['st_shndx']==elf.get_section_index('.tooie_music_state')
        assert all(s['st_shndx']==elf.get_section_index('.recomp_patch') for name in MUSIC_BODIES for s in table.get_symbol_by_name(name))
        # Like graphics-rate admission, retain typed ELF facts for local J relocations.
        patch_index=elf.get_section_index('.recomp_patch');patch=elf.get_section(patch_index)
        assert elf['e_type']=='ET_REL' and patch['sh_addr']==0
        code=patch.data();bounds=[]
        for name in MUSIC_BODIES:
            matches=table.get_symbol_by_name(name);assert matches and len(matches)==1
            fn=matches[0];lo,hi=fn['st_value'],fn['st_value']+fn['st_size']
            assert fn['st_info']['type']=='STT_FUNC' and 0<=lo<hi<=len(code) and lo%4==hi%4==0
            bounds.append((lo,hi))
        assert sorted(bounds)[0][1]<=sorted(bounds)[1][0]
        rels=elf.get_section_by_name('.rel.recomp_patch')
        assert rels['sh_info']==patch_index and rels['sh_link']==elf.get_section_index('.symtab')
        local_jumps=set()
        for relocation in rels.iter_relocations():
            target=table.get_symbol(relocation['r_info_sym'])
            if relocation['r_info_type']!=4 or target.name:continue
            assert target['st_info']['type']=='STT_SECTION' and target['st_info']['bind']=='STB_LOCAL'
            assert target['st_shndx']==patch_index and target['st_value']==0
            offset=relocation['r_offset'];assert offset%4==0 and offset not in local_jumps
            owners=[(lo,hi) for lo,hi in bounds if lo<=offset and offset+8<=hi]
            assert len(owners)==1
            word=int.from_bytes(code[offset:offset+4],'big');assert word>>26==2
            destination=(word&0x03ffffff)<<2;lo,hi=owners[0]
            assert lo<=destination and destination+4<=hi, 'Local jump crosses its function bound'
            local_jumps.add(offset)
    assert music['definitions']==MUSIC_BODIES and set(music['imports'])==MUSIC_IMPORTS
    assert set(music['allocated_sections'])=={'.recomp_patch','.tooie_music_state','.reginfo','.MIPS.abiflags'}
    assert music['allocated_sections']['.tooie_music_state']==dict(type='SHT_PROGBITS',bytes=20,alignment=4)
    assert bytes.fromhex(music['section_bytes']['.tooie_music_state'])==MUSIC_INITIAL
    metadata=[row for row in music['relocations'] if row['section']=='.pdr']
    assert metadata==[dict(section='.pdr',offset=32*i,kind=2,target=name) for i,name in enumerate(MUSIC_BODIES)]
    for row in music['relocations']:
        if row['section']=='.pdr':continue
        assert row['section']=='.recomp_patch' and row['kind'] in (4,5,6)
        if row['kind']==4 and not row['target']:
            assert row['offset'] in local_jumps
            local_jumps.remove(row['offset']);continue
        targets=MUSIC_IMPORTS-{'D_80079FA0'} if row['kind']==4 else {'D_80079FA0','tooie_music_state'}
        assert row['target'] in targets, 'Exact music call/data relocation targets'
    assert not local_jumps
    assert music['frames']==MUSIC_FRAMES, 'Music object frames remain unqualified; inspect MUSIC_OBJECT_UNQUALIFIED.json'
    host=records['minimum_cutscene']
    (out/'HOST_OBJECT_UNQUALIFIED.json').write_text(json.dumps(host,indent=2)+'\n')
    assert host['definitions']==HELPERS
    assert set(host['imports'])=={'set_widescreen','func_800DA298','widescreen_enabled','tooie_baseline_presentation_settings','tooie_baseline_presentation_publish'}
    assert set(host['allocated_sections'])=={'.baseline_host_helpers','.baseline_host_state','.reginfo','.MIPS.abiflags'}
    assert host['allocated_sections']['.baseline_host_state']['type']=='SHT_PROGBITS'
    assert bytes.fromhex(host['section_bytes']['.baseline_host_state'])==bytes(HOST_STATE_BYTES)
    assert host['frames']==HOST_FRAMES, 'ROOT-PENDING HOST frames; inspect HOST_OBJECT_UNQUALIFIED.json'
    metadata=[row for row in host['relocations'] if row['section']=='.pdr']
    assert metadata==[dict(section='.pdr',offset=32*i,kind=2,target=name) for i,name in enumerate(HELPERS)]
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
        'music_gain':[source/'music_gain.c',source/'minimum_host_imports.h',root/'tools/baseline/prepare_minimum_host.py',data_source],
        'minimum_cutscene':[source/'minimum_cutscene.c',source/'minimum_presentation_abi.h'],
        'native_aspect_projection':[root/'patches/game/native_aspect_projection.c',root/'patches/game/widescreen_settings_abi.h'],
    }.items():
        records[name]['inputs']={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
    return records

def linked(elf,prior_end):
    table=elf.get_section_by_name('.symtab');symbols={s.name:s for s in table.iter_symbols() if s.name}
    section=elf.get_section_by_name('.baseline_host_helpers');state=elf.get_section_by_name('.baseline_host_state');literals=elf.get_section_by_name('.baseline_g07_literals')
    music=elf.get_section_by_name('.tooie_music_state')
    assert section and state and literals and state['sh_type']=='SHT_PROGBITS'
    assert music and music['sh_type']=='SHT_PROGBITS' and music['sh_size']==20 and music.data()==MUSIC_INITIAL
    assert music['sh_addr']==((prior_end+3)&~3) and section['sh_addr']==music['sh_addr']+20
    assert symbols['tooie_music_state']['st_shndx']==elf.get_section_index(music.name) and symbols['tooie_music_state']['st_size']==20
    assert all(symbols[name]['st_shndx']==elf.get_section_index('.recomp_patch') for name in MUSIC_BODIES)
    assert state['sh_size']==HOST_STATE_BYTES and state.data()==bytes(HOST_STATE_BYTES) and state['sh_addr']==section['sh_addr']+section['sh_size']
    assert literals['sh_size']==24 and literals['sh_addr']%8==0 and literals['sh_addr']>=state['sh_addr']+HOST_STATE_BYTES
    assert literals['sh_addr']+24<=0x81000000
    assert all(symbols[name]['st_shndx']==elf.get_section_index(section.name) and symbols[name]['st_size']>0 for name in HELPERS)
    assert symbols[PROJECTION]['st_shndx']==elf.get_section_index('.recomp_patch')
    assert {name:symbols[name]['st_value'] for name in NATIVE_IMPORTS}==NATIVE_IMPORTS
    assert all(symbols[name]['st_shndx']=='SHN_ABS' for name in NATIVE_IMPORTS)
    return dict(rom=section['sh_addr']-0x80801000,ram=section['sh_addr'],size=section['sh_size'],index=elf.get_section_index(section.name),
                state=state['sh_addr'],state_bytes=HOST_STATE_BYTES,music_state=music['sh_addr'],music_bytes=20,literals=literals['sh_addr'],literal_bytes=24,image_end=literals['sh_addr']+24)
