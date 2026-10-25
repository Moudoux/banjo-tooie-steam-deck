"""Finite public frame-rate command object/state proof; no new runtime selection."""
import hashlib
from prepare_cpu625 import facts
from elftools.elf.elffile import ELFFile
BODY='func_800E7EF4'
CALLS=['func_80015184','func_800A9800'] # Requested-period getter, original completion.
IMPORTS=set(CALLS)|{'D_8007BEDA','tooie_demo_timing_state'}
# LO16 consumers: LH slot index, ADDIU tail address, LW pending demo divisor.
LO16_OPCODES={'D_8007BEDA':33,'tooie_graphics_rate_tail':9,'tooie_demo_timing_state':35}

def compile_owner(root,out,run,flags,win):
    source=root/'tools/baseline/guest/camera-candidate/graphics_rate.c'
    header=root/'tools/baseline/guest/camera-candidate/demo_timing.h'
    obj=out/'graphics_rate.o'
    # Source range1..60 dominates DIVU; suppress only this object's unsupported TEQ guard.
    command=flags+['-mno-check-zero-division','-c',win(source),'-o',win(obj)]
    run(command,'graphics-rate-compile.log')
    record=facts(obj)
    assert record['definitions']==[BODY] and set(record['imports'])==IMPORTS
    assert set(record['allocated_sections'])=={'.recomp_patch','.tooie_graphics_rate_state','.reginfo','.MIPS.abiflags'}
    assert record['allocated_sections']['.tooie_graphics_rate_state']['type']=='SHT_PROGBITS'
    assert bytes.fromhex(record['section_bytes']['.tooie_graphics_rate_state'])==bytes(48)
    assert 0<record['frames'][BODY]<=64
    assert [x for x in record['relocations'] if x['section']=='.pdr']==[dict(section='.pdr',offset=0,kind=2,target=BODY)]
    assert all(x['section']=='.recomp_patch' and x['kind'] in (4,5,6) for x in record['relocations'] if x['section']!='.pdr')
    # R_MIPS_26 carries both JAL calls and absolute local J control flow.
    # Retain ELF symbol section/type here; the generic facts reader omits them.
    with obj.open('rb') as stream:
        elf=ELFFile(stream);code_section=elf.get_section_by_name('.recomp_patch')
        code=code_section.data();code_index=elf.get_section_index('.recomp_patch')
        table=elf.get_section_by_name('.symtab')
        body=next(x for x in table.iter_symbols() if x.name==BODY)
        begin=body['st_value'];end=begin+body['st_size']
        calls=[];local_jumps=[];data_relocations=[]
        for relocation in elf.get_section_by_name('.rel.recomp_patch').iter_relocations():
            offset=relocation['r_offset'];kind=relocation['r_info_type']
            assert begin<=offset<end and not offset%4
            word=int.from_bytes(code[offset:offset+4],'big');opcode=word>>26
            target=table.get_symbol(relocation['r_info_sym'])
            if kind==4:
                if opcode==3:
                    assert target['st_shndx']=='SHN_UNDEF' and target.name in CALLS
                    calls.append(target.name)
                else:
                    assert opcode==2 and target['st_info']['type']=='STT_SECTION' and target['st_shndx']==code_index
                    destination=target['st_value']+((word&0x3FFFFFF)<<2)
                    assert begin<=destination<end and not destination%4
                    local_jumps.append(dict(offset=offset,destination=destination))
            else:
                assert kind in (5,6) and target.name in LO16_OPCODES
                assert opcode==(15 if kind==5 else LO16_OPCODES[target.name])
                data_relocations.append((kind,target.name))
        assert calls==CALLS,calls
        # Measured selected compiler shape; target also proved inside this body above.
        assert len(local_jumps)==1,local_jumps
        # Scheduling may interleave the three HI16/LO16 pairs; their set is exact.
        assert sorted(data_relocations)==sorted((kind,name) for name in LO16_OPCODES for kind in (5,6)),data_relocations
        words=[int.from_bytes(code[i:i+4],'big') for i in range(begin,end,4)]
        assert not any(w>>26==0 and w&63==9 for w in words), 'No indirect JALR added'
        assert not any(w>>26==0 and w&63==0x34 for w in words), 'Selected recompiler does not support TEQ'
        assert sum(w>>26==0 and w&63==0x1B for w in words)==1, 'Exactly one guarded unsigned division'
    record['external_calls']=calls;record['local_jumps']=local_jumps
    record['data_relocation_targets']=[list(x) for x in data_relocations]
    record['compile_argv']=[str(x) for x in command]
    record['inputs']={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in (source,header)}
    return record

def linked(elf,previous_end):
    table=elf.get_section_by_name('.symtab');symbols={s.name:s for s in table.iter_symbols() if s.name}
    state=elf.get_section_by_name('.tooie_graphics_rate_state')
    assert state and state['sh_type']=='SHT_PROGBITS' and state.data()==bytes(48)
    assert state['sh_addr']==(previous_end+7)&~7 and state['sh_addr']+48<=0x81000000
    assert symbols['tooie_graphics_rate_tail']['st_value']==state['sh_addr'] and symbols['tooie_graphics_rate_tail']['st_size']==48
    assert symbols[BODY]['st_shndx']==elf.get_section_index('.recomp_patch') and symbols[BODY]['st_size']>0
    # The link ignores unresolved symbols, so prove the demo divisor resolved to its 8-byte state.
    demo=elf.get_section_by_name('.tooie_demo_timing_state')
    assert demo and demo['sh_size']==8 and 'tooie_demo_timing_state' in symbols
    assert symbols['tooie_demo_timing_state']['st_shndx']==elf.get_section_index('.tooie_demo_timing_state')
    assert symbols['tooie_demo_timing_state']['st_value']==demo['sh_addr']
    return dict(state=state['sh_addr'],state_bytes=48,image_end=state['sh_addr']+48,body=BODY,
        demo_state=demo['sh_addr'])
