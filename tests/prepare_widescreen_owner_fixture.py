"""Extract the actual translated widescreen/cutscene owners and one precise mutant."""
import argparse
import hashlib
import json
import re
from pathlib import Path

from elftools.elf.elffile import ELFFile

HELPERS = ('tooie_baseline_widescreen_frame', 'tooie_baseline_cutscene_command',
           'tooie_baseline_cutscene_ticker_claim', 'tooie_baseline_cutscene_ticker_retire')
STATE_BYTES = 12  # generation, glcut cause, ticker owner
IMPORTS = ('set_widescreen', 'func_800DA298',
           'tooie_baseline_presentation_settings', 'tooie_baseline_presentation_publish')


def extract(text, name):
    # Same balanced-body boundary as extract_heap_realloc, limited to this pair.
    assert name in HELPERS
    matches = list(re.finditer(r'^RECOMP_FUNC void ' + name + r'\([^\n]*\) \{', text, re.M))
    assert len(matches) == 1, (name, 'absent/duplicate translated helper')
    start = matches[0].start()
    depth = 0
    for pos in range(matches[0].end() - 1, len(text)):
        depth += (text[pos] == '{') - (text[pos] == '}')
        if depth == 0:
            return text[start:pos + 1] + '\n'
    raise AssertionError('unterminated translated helper')


def cause_predicate(elf, symbol, glcut_address, owner_address):
    """Authenticate LW glcut, LW owner -> OR -> SLTU boolean -> AND suppression chain."""
    section = elf.get_section(symbol['st_shndx'])
    start = symbol['st_value'] - section['sh_addr']
    code = section.data()[start:start + symbol['st_size']]
    words = [int.from_bytes(code[i:i + 4], 'big') for i in range(0, len(code), 4)]
    fields = {glcut_address: 'glcut', owner_address: 'owner'}
    saved = (0, *range(16, 24), 28, 29, 30)
    constants = {0: 0}
    loaded = {}
    loads = []
    found = []
    for index, word in enumerate(words):
        op, rs, rt, rd = word >> 26, (word >> 21) & 31, (word >> 16) & 31, (word >> 11) & 31
        imm = word & 0xffff
        signed = imm - 0x10000 if imm & 0x8000 else imm
        field = None
        if op == 35 and rs in constants:
            field = fields.get((constants[rs] + signed) & 0xffffffff)
            if field:
                loads.append(field)
        if op == 0 and word & 63 == 0x25 and {loaded.get(rs), loaded.get(rt)} == {'glcut', 'owner'}:
            assert index + 2 < len(words), 'cause predicate truncated'
            boolean, combine = words[index + 1:index + 3]
            # SLTU dest,zero,or converts the combined cause to a boolean.
            assert boolean >> 26 == 0 and boolean & 63 == 0x2b and (boolean >> 21) & 31 == 0 and \
                (boolean >> 16) & 31 == rd, 'exact cause boolean conversion required'
            dest = (boolean >> 11) & 31
            # The immediately following AND combines it with the policy boolean.
            assert combine >> 26 == 0 and combine & 63 == 0x24
            sources = ((combine >> 21) & 31, (combine >> 16) & 31)
            assert dest in sources and sources[0] != sources[1]
            found.append((symbol['st_value'] + (index + 1) * 4, rd, dest))
        if op == 15:
            constants[rt] = imm << 16
        elif op in (9, 13) and rs in constants:
            constants[rt] = ((constants[rs] + signed) if op == 9 else (constants[rs] | imm)) & 0xffffffff
        elif op == 3:
            constants = {k: v for k, v in constants.items() if k in saved}
            loaded = {k: v for k, v in loaded.items() if k in saved}
        elif op not in (4, 5, 6, 7, 40, 41, 43, 57, 61):
            dest = rd if op == 0 else rt
            constants.pop(dest, None)
            loaded.pop(dest, None)
        if field:
            loaded[rt] = field
        constants[0] = 0
    assert sorted(loads) == ['glcut', 'owner'], ('each cause field is read once', loads)
    assert len(found) == 1, ('expected one actual cause suppression predicate', found)
    return found[0]


def main():
    if not __debug__:  # also false when PYTHONOPTIMIZE is set
        raise SystemExit('prepare_widescreen_owner_fixture.py: required validation uses assert; '
                         'run without -O and with PYTHONOPTIMIZE unset')
    parser = argparse.ArgumentParser()
    parser.add_argument('--candidate', type=Path, required=True)
    parser.add_argument('--elf', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    text = args.candidate.read_text(encoding='utf-8')
    bodies = {name: extract(text, name) for name in HELPERS}
    with args.elf.open('rb') as stream:
        elf = ELFFile(stream)
        assert elf.elfclass == 32 and not elf.little_endian and elf['e_machine'] == 'EM_MIPS'
        symbols = {s.name: s for s in elf.get_section_by_name('.symtab').iter_symbols() if s.name}
        state = symbols['tooie_baseline_cutscene_state']
        section = elf.get_section(state['st_shndx'])
        offset = state['st_value'] - section['sh_addr']
        assert state['st_size'] == STATE_BYTES and section.data()[offset:offset + STATE_BYTES] == bytes(STATE_BYTES)
        assert section.name == '.baseline_host_state'
        for name in HELPERS:
            assert elf.get_section(symbols[name]['st_shndx']).name == '.baseline_host_helpers'
        pc, cause, register = cause_predicate(elf, symbols[HELPERS[0]], state['st_value'] + 4, state['st_value'] + 8)
        # LLD leaves this original import unresolved; codegen applies data.syms.
        data_symbols = args.candidate.with_name('data.syms.toml').read_text()
        addresses = re.findall(r'name = "widescreen_enabled"\s+vram = (\d+)', data_symbols)
        assert len(addresses) == 1
        wide = int(addresses[0])
        assert wide == 0x800799B8, 'original live halfword data binding required'
        state_address = state['st_value']
    frame = bodies[HELPERS[0]]
    pattern = (r'(// 0x' + format(pc, '08X') + r': sltu[^\n]*\n\s*)' +
               re.escape(f'ctx->r{register} = 0 < ctx->r{cause} ? 1 : 0;'))
    assert len(re.findall(pattern, frame, re.I)) == 1, 'exact cause predicate translation required'
    mutant = re.sub(pattern, lambda m: m[1] + f'ctx->r{register} = 0;', frame, count=1, flags=re.I)
    assert mutant != frame
    # Verify codegen applied the original halfword binding to the live compare
    # (same base register; allocation may differ from the pre-Q16 object).
    binding = re.findall(r'lui\s+\$(\w+), 0x8008\n(?:.*\n)*?.*lh\s+\$\w+, -0x6648\(\$(\w+)\)', frame)
    assert any(base == used for base, used in binding), 'original live halfword binding required'
    out = args.output
    out.mkdir(parents=True, exist_ok=True)
    outputs = {
        'widescreen_owner_candidate.inc': ''.join(bodies.values()),
        'widescreen_owner_negative.inc': mutant + ''.join(bodies[name] for name in HELPERS[1:]),
        'widescreen_owner_functions.h': '#pragma once\nextern "C" {\n' + ''.join(
            f'void {name}(uint8_t*, recomp_context*);\n' for name in (*HELPERS, *IMPORTS)) + '}\n',
        'widescreen_owner_facts.h': ('#pragma once\n' +
            f'#define WIDE_OWNER_STATE 0x{state_address:08X}u\n' +
            f'#define WIDE_OWNER_HALFWORD 0x{wide:08X}u\n'),
    }
    for name, value in outputs.items():
        (out / name).write_text(value, encoding='utf-8', newline='\n')
    custody = dict(candidate_sha256=hashlib.sha256(args.candidate.read_bytes()).hexdigest(),
                   elf_sha256=hashlib.sha256(args.elf.read_bytes()).hexdigest(),
                   state_address=state_address, state_bytes=STATE_BYTES, widescreen_halfword=wide,
                   mutant_predicate_pc=pc, mutant_predicate_register=register,
                   mutant_cause_register=cause,
                   data_symbols_sha256=hashlib.sha256(args.candidate.with_name('data.syms.toml').read_bytes()).hexdigest(),
                   outputs={n: hashlib.sha256(v.encode()).hexdigest() for n, v in outputs.items()})
    (out / 'manifest.json').write_text(json.dumps(custody, indent=2) + '\n')


if __name__ == '__main__':
    main()
