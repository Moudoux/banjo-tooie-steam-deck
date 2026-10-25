"""Finite production FB04 fixture: actual linked candidate vs frozen base originals.

Reuses the established exact extractor. No guest compilation, application,
pin promotion or qualification. The six candidate functions are never oracles.
"""
import argparse, hashlib, importlib.util, json, re
from pathlib import Path
from elftools.elf.elffile import ELFFile

# Frozen normalized baseline/raw originals.
BASE_PINS = {
    'bs_setState': 'ea7756eb08bb625bd101af92d7c0a5fba185e037af477554df518a7794697d47',
    'bainput_update': '608793194391fc36dcdb024e2499f68b98d0a77485d6dc399e49a211f12d2e14',
    'func_80800710_bsbtrot': '62555d36baa12e0c5085937b9276f7d79501b28bad0ff8e324c365d1f7804749',
    'func_800F86D8': 'db6b83b9211df3567ddaf12d0579e02c59e64ca0cc909287e9389bcb516abcfd',
    'bs_getCurrentState': '83e2509fe6bb5a3c04d65af07c9ca905ff00f5b7dd04b7a5daddf3a3326d9ab7',
    'bs_getPreviousState': 'bfd29162a8fbe420ee4efa60b6b7acffd9549a7dae569092ada2010c4f8abdf8',
    'bs_getNextState': 'dcfd4d2e9b800f643122b774488bef582c68cdf38836f95c32bb01e378637f41',
}
BODIES = (
    'bs_setState', 'bainput_update', 'func_80800710_bsbtrot', 'func_800F86D8',
)
HELPERS = ('tooie_trot_toggle_retire', 'tooie_trot_toggle_move')
GETTERS = ('bs_getCurrentState', 'bs_getPreviousState', 'bs_getNextState')
IMPORTS = (
    'func_8009CAAC', 'func_8009CAF8', 'func_8009CA70', 'func_8008E39C',
    'bakey_released', 'bakey_pressed', 'bakey_held', 'func_800D8FF8', 'func_800F0E00',
    'bainput_enable', 'func_800F46D8', 'tooie_baseline_trot_toggle', *GETTERS,
)


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    if not __debug__:  # also false when PYTHONOPTIMIZE is set
        raise SystemExit('prepare_trot_toggle_fixture.py: required validation uses assert; '
                         'run without -O and with PYTHONOPTIMIZE unset')
    p = argparse.ArgumentParser()
    p.add_argument('--candidate', type=Path, required=True)
    p.add_argument('--elf', type=Path, required=True)
    p.add_argument('--originals', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    base = a.originals.resolve(strict=True)
    out = a.output.resolve()
    assert (
        base.name == 'raw' and base.parent.name == 'baseline'
    ), 'originals must be protected baseline/raw'
    assert set(BASE_PINS) == set((*BODIES, *GETTERS)), 'frozen original body pins required'
    spec = importlib.util.spec_from_file_location(
        'focused_extract', Path(__file__).with_name('extract_heap_realloc.py')
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    found = {name: [] for name in BASE_PINS}
    texts = {}
    for path in sorted(base.glob('funcs_*.c')):
        text = path.read_text(encoding='utf-8')
        texts[path] = text
        for name in re.findall(r'^RECOMP_FUNC void (\w+)\(', text, re.M):
            if name in found:
                found[name].append(path)
    originals = []
    custody = {}
    for name, paths in found.items():
        assert len(paths) == 1, (name, 'original absent/duplicate', paths)
        path = paths[0]
        body = module.extract(texts[path], name)
        digest = hashlib.sha256(body.encode()).hexdigest()
        assert digest == BASE_PINS[name], (name, 'original body drift', digest)
        originals.append(
            body.replace(
                'RECOMP_FUNC void ' + name + '(',
                'RECOMP_FUNC void ' + name + '_original(',
                1,
            )
        )
        custody[name] = {
            'source': str(path), 'source_sha256': sha(path), 'body_sha256': digest,
        }
    text = a.candidate.read_text(encoding='utf-8')
    names = (*BODIES, *HELPERS)
    candidate = '\n'.join(module.extract(text, name) for name in names)
    # Mechanical removal of only the actual710 current/last guard; current addresses retained.
    body = module.extract(text, 'func_80800710_bsbtrot')
    condition = 'if (ctx->r3 != ctx->r1) {'
    assert body.count(condition) == 1, '710 reviewed guard shape drift'
    offset = body.index(condition)
    prefix = body[max(0, offset - 700):offset]
    loads = (
        'ctx->r1 = MEM_W(ctx->r2, 0X4);',
        'ctx->r3 = MEM_W(ctx->r16, 0X120);',
        'ctx->r3 = MEM_W(ctx->r3, 0X4);',
    )
    assert all(prefix.count(load) == 1 for load in loads), 'unique710 current/last loads required'
    positions = [prefix.index(load) for load in loads]
    assert positions == sorted(positions)
    branch = re.search(
        r'// (0x[0-9A-Fa-f]+): bne\s+\$v1, \$at, (L_[0-9A-F]+)\s*$', prefix
    )
    assert branch, 'exact710 current/last instruction branch metadata required'
    mutant = candidate.replace(
        body,
        body.replace(condition, 'if (false) { // actual candidate guard mutation', 1),
    )

    def wrap(s):
        old = 'RECOMP_FUNC void tooie_trot_toggle_move('
        assert s.count(old) == 1
        return s.replace(old, 'RECOMP_FUNC void tooie_trot_toggle_move_body(', 1)

    with a.elf.open('rb') as stream:
        elf = ELFFile(stream)
        assert (
            elf.elfclass == 32 and not elf.little_endian and elf['e_machine'] == 'EM_MIPS'
        )
        symbols = list(elf.get_section_by_name('.symtab').iter_symbols())
        slots = [s for s in symbols if s.name == 'tooie_trot_toggle_slot']
        assert len(slots) == 1
        slot = slots[0]
        assert slot['st_size'] == 12
        section = elf.get_section(slot['st_shndx'])
        off = slot['st_value'] - section['sh_addr']
        assert (
            section['sh_type'] == 'SHT_PROGBITS'
            and section.data()[off:off + 12] == bytes(12)
        )
        slot_addr = slot['st_value']
    out.mkdir(parents=True, exist_ok=True)

    def write(name, value):
        (out / name).write_text(value, encoding='utf-8', newline='\n')

    write('candidate_fixture.inc', wrap(candidate))
    write('candidate_guard_mutant.inc', wrap(mutant))
    write('original_fixture.inc', '\n'.join(originals))
    write(
        'fixture_functions.h',
        '#pragma once\nextern "C" {\n' + ''.join(
            'void ' + name + '(uint8_t*, recomp_context*);\n'
            for name in sorted(set((*names, *IMPORTS, 'tooie_trot_toggle_move_body')))
        ) + '}\n',
    )
    write(
        'fixture_facts.h',
        f'#pragma once\n#define FB04_SLOT_ADDR 0x{slot_addr:08X}u\n'
        '#define FB04_SLOT_SIZE 12u\n#define FB04_HAS_MOVE 1\n',
    )
    outputs = [
        'candidate_fixture.inc', 'candidate_guard_mutant.inc', 'original_fixture.inc',
        'fixture_functions.h', 'fixture_facts.h',
    ]
    write(
        'fixture_custody.json',
        json.dumps(
            {
                'scope': 'PRODUCTION_FIXTURE_PREPARED_NOT_EXECUTED',
                'originals': custody,
                'candidate_sha256': sha(a.candidate),
                'elf_sha256': sha(a.elf),
                'extractor_sha256': sha(Path(__file__).with_name('extract_heap_realloc.py')),
                'slot': slot_addr,
                'negative': (
                    'actual710 guard branch ' + branch[1]
                    + ' forced not taken; oracle unchanged'
                ),
                'outputs': {name: sha(out / name) for name in outputs},
            },
            indent=2,
        ) + '\n',
    )


if __name__ == '__main__':
    main()
