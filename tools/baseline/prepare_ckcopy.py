"""P04b ckpack copy-storage owner (layout 2, P04a labels): exact object facts and appended-image placement."""
import hashlib
from prepare_cpu625 import facts

HELPERS=['tooie_ckcopy_root_begin','tooie_ckcopy_segment','tooie_ckcopy_root_end','tooie_ckcopy_release']
STATE_BYTES=122960
INITIAL=bytes.fromhex('434b435000000002')+bytes(STATE_BYTES-8)
IMPORTS={'D_8007BCD0','D_8007BEDA','D_8012C948','D_80135490','tooie_camera_main'}

def compile_owner(root,out,run,flags,win):
    source=root/'tools/baseline/guest/cpu625-candidate/ckcopy.c'
    obj=out/'ckcopy.o'
    command=flags+['-fno-jump-tables','-c',win(source),'-o',win(obj)]
    run(command,'ckcopy-compile.log')
    record=facts(obj)
    assert record['definitions']==HELPERS,record['definitions']
    assert set(record['allocated_sections'])=={'.tooie_ckcopy_helpers','.tooie_ckcopy_state','.reginfo','.MIPS.abiflags'},record['allocated_sections']
    state=record['allocated_sections']['.tooie_ckcopy_state']
    assert state['type']=='SHT_PROGBITS' and state['bytes']==STATE_BYTES and state['alignment']>=8
    assert bytes.fromhex(record['section_bytes']['.tooie_ckcopy_state'])==INITIAL
    assert set(record['imports'])==IMPORTS,record['imports']
    assert all(x['section']=='.tooie_ckcopy_helpers' and x['kind'] in (4,5,6)
        for x in record['relocations'] if x['section']!='.pdr'),'Allocated ckcopy relocation closure'
    record['compile_argv']=[str(x) for x in command]
    record['inputs']={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in
        (source,source.parent/'ckcopy_layout.h',root/'tools/baseline/prepare_ckcopy.py',root/'tools/baseline/ckcopy_hooks.py')}
    return record

def linked(elf,prior_end):
    symbols={s.name:s for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
    code=elf.get_section_by_name('.tooie_ckcopy_helpers');state=elf.get_section_by_name('.tooie_ckcopy_state')
    assert code and state and code['sh_addr']==((prior_end+3)&~3)
    assert state['sh_type']=='SHT_PROGBITS' and state.data()==INITIAL
    assert state['sh_addr']==((code['sh_addr']+code['sh_size']+7)&~7)
    assert symbols['tooie_ckcopy_state']['st_value']==state['sh_addr'] and symbols['tooie_ckcopy_state']['st_size']==STATE_BYTES
    assert all(symbols[n]['st_shndx']==elf.get_section_index(code.name) and symbols[n]['st_size'] for n in HELPERS)
    end=state['sh_addr']+STATE_BYTES
    assert end<=0x81000000
    return dict(rom=code['sh_addr']-0x80801000,ram=code['sh_addr'],size=code['sh_size'],
        index=elf.get_section_index(code.name),state=state['sh_addr'],state_bytes=STATE_BYTES,image_end=end)
