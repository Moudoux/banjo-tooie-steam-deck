"""One game-side Flap Flip replacement; no native imports or mutable storage."""
import hashlib
import json
from elftools.elf.elffile import ELFFile
from prepare_cpu625 import facts

BODY = 'bsbflip_update'
IMPORTS = set('baanim_getAnimCtrlPtr anctrl_isAt anctrl_getPlaybackType anctrl_getIndex '
    'func_8009DF94 func_80800064_bsbflip func_80800000_bsbflip anctrl_isStopped '
    'bainput_should_beak_bust func_8008DA24 func_8008E260 bakey_released '
    'player_isStable func_800A01F8 player_inWater bs_setState'.split())
FRAME = 40  # Pinned MIPS-II object; translated fixture checks this body.
IMAGE_SHA256 = '290755f1ecd95da325877c5dab99ed807c8bc373db04ec1c8a3a85fc633f6aaf'


def compile_owner(root, out, run, flags, win):
    source = root / 'tools/baseline/guest/minimum-host/flap_flip_landing.c'
    obj = out / 'flap_flip_landing.o'
    command = flags + ['-fno-jump-tables', '-I'+win(root/'deps/banjo-tooie/src'), '-c', win(source), '-o', win(obj)]
    run(command, 'flap-flip-compile.log')
    record = facts(obj)
    (out / 'FLAP_FLIP_OBJECT_UNQUALIFIED.json').write_text(json.dumps(record, indent=2)+'\n')
    with obj.open('rb') as stream:
        elf = ELFFile(stream)
        assert elf.elfclass == 32 and not elf.little_endian and elf['e_machine'] == 'EM_MIPS'
        assert elf['e_flags'] & 0xF000F020 == 0x10001000, 'MIPS-II o32 ABI'
    assert record['definitions'] == [BODY]
    assert set(record['imports']) == IMPORTS
    assert set(record['allocated_sections']) == {'.recomp_patch', '.reginfo', '.MIPS.abiflags'}
    assert record['frames'] == {BODY: FRAME}, record['frames']
    assert all(r['kind'] in ((2,) if r['section'] == '.pdr' else (4,5,6)) for r in record['relocations'])
    record['compile_argv'] = [str(x) for x in command]
    record['inputs'] = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in (source, root/'tools/baseline/prepare_flap_flip.py')}
    return record
