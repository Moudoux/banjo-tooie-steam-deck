"""Finite CPU625 object/layout proof for the maintained camera composition."""
import hashlib
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

HELPERS=['tooie_cpu625_begin','tooie_cpu625_admit','tooie_cpu625_end','tooie_cpu625_retire']
# N3 (selected with ckpack copy storage): the fallback-shadow attachment owner, same section.
ATTACH_HELPERS=['tooie_cpu625_attach_begin','tooie_cpu625_attach_end']
# Installed only after independent actual source/ELF/registry/byte acceptance.
PRE_IMAGE_SHA256='63a564aa2af9a69424ef9154965da92488a81aa3d405671fbb633074dbf4761c'

def helpers(ckcopy):
    assert type(ckcopy) is bool
    return HELPERS+(ATTACH_HELPERS if ckcopy else [])

def facts(path):
    with path.open('rb') as stream:
        elf=ELFFile(stream);table=elf.get_section_by_name('.symtab');symbols={s.name:s for s in table.iter_symbols() if s.name}
        definitions=[s.name for s in table.iter_symbols() if s['st_info']['type']=='STT_FUNC' and s['st_shndx']!='SHN_UNDEF']
        imports={s.name for s in table.iter_symbols() if s.name and s['st_shndx']=='SHN_UNDEF'}
        sections={s.name:s for s in elf.iter_sections() if s['sh_flags']&2 and s['sh_size']}
        frames={}
        for name in definitions:
            symbol=symbols[name];section=elf.get_section(symbol['st_shndx']);offset=symbol['st_value']-section['sh_addr'];code=section.data()[offset:offset+symbol['st_size']]
            words=[int.from_bytes(code[i:i+4],'big') for i in range(0,len(code),4)]
            frames[name]=max((65536-(w&65535) for w in words if w>>16==0x27bd and w&0x8000),default=0)
        relocations=[]
        for sec in elf.iter_sections():
            if not isinstance(sec,RelocationSection):continue
            linked=elf.get_section(sec['sh_link'])
            for item in sec.iter_relocations():
                relocations.append(dict(section=elf.get_section(sec['sh_info']).name,offset=item['r_offset'],kind=item['r_info_type'],target=linked.get_symbol(item['r_info_sym']).name))
        return dict(sha256=hashlib.sha256(path.read_bytes()).hexdigest(),definitions=definitions,imports=sorted(imports),frames=frames,allocated_sections={name:dict(type=s['sh_type'],bytes=s['sh_size'],alignment=s['sh_addralign']) for name,s in sections.items()},relocations=relocations,section_bytes={name:s.data().hex() for name,s in sections.items()})

def compile_owner(root,out,run,flags,win,ckcopy=False):
    assert type(ckcopy) is bool
    obj=out/'cpu625.o';source=root/'tools/baseline/guest/cpu625-candidate/cpu625.c'
    # P04a rows join the selected ckpack copy storage; their if-chains must stay free of .rodata.
    selection=['-DTOOIE_CKCOPY='+str(int(ckcopy))]+(['-fno-jump-tables'] if ckcopy else [])
    command=flags+selection+['-c',win(source),'-o',win(obj)];run(command,'cpu625-compile.log')
    record=facts(obj)
    assert record['definitions']==helpers(ckcopy),record['definitions']
    assert set(record['allocated_sections'])=={'.tooie_cpu625_helpers','.tooie_cpu625_state','.reginfo','.MIPS.abiflags'}
    assert record['allocated_sections']['.tooie_cpu625_state']['type']=='SHT_PROGBITS'
    assert bytes.fromhex(record['section_bytes']['.tooie_cpu625_state'])==bytes.fromhex('3632353100000001')+bytes(456)
    # N3 reads the shadow prefix (shadow_state.h) only in the selected composition.
    assert set(record['imports'])==set('D_8007BCD0 D_8007BCE8 D_8007BCF0 D_8007BEDA D_80127674 D_8012C838 D_8012C948 D_8012C968 D_8012CEC4 D_8012CF8C D_8012D500 D_8012D508 D_8012D558 D_80132E9C D_80135490 D_801354B0 D_801354DC func_80100A74 func_80100AC4 tooie_camera_main tooie_camera_state tooie_pre_background tooie_render_bridge_state'.split())|({'tooie_ckcopy_state','shadow_pending'} if ckcopy else set()),record['imports']
    metadata=[x for x in record['relocations'] if x['section']=='.pdr']
    assert metadata==[dict(section='.pdr',offset=32*i,kind=2,target=name) for i,name in enumerate(helpers(ckcopy))], 'Exact discarded compiler procedure metadata'
    assert all(x['section']=='.tooie_cpu625_helpers' and x['kind'] in (4,5,6) for x in record['relocations'] if x['section']!='.pdr'), 'Allocated CPU relocation closure'
    record['compile_argv']=[str(x) for x in command]
    record['inputs']={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (source,source.parent/'cpu625_layout.h')+((source.parent/'ckcopy_layout.h',source.parent/'ckpack_rows.h',source.parent.parent/'camera-candidate/shadow_state.h') if ckcopy else ())}
    return record

def linked(elf):
    table=elf.get_section_by_name('.symtab');symbols={s.name:s for s in table.iter_symbols() if s.name}
    section=elf.get_section_by_name('.tooie_cpu625_helpers');state=elf.get_section_by_name('.tooie_cpu625_state')
    assert section and state and state['sh_type']=='SHT_PROGBITS'
    assert state.data()==bytes.fromhex('3632353100000001')+bytes(456)
    assert state['sh_addr']==section['sh_addr']+section['sh_size'] and state['sh_addr']+464<=0x81000000
    assert all(symbols[n]['st_shndx']==elf.get_section_index(section.name) and symbols[n]['st_size'] for n in HELPERS)
    return dict(rom=section['sh_addr']-0x80801000,ram=section['sh_addr'],size=section['sh_size'],index=elf.get_section_index(section.name),state=state['sh_addr'],state_bytes=464)
