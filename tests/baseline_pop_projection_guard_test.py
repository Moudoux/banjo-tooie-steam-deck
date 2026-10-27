#!/usr/bin/env python3
"""L03 focused gate: the selected BK menu text needs the RT64 pop_matrix_group_projection repair.

Executes the guard block extracted verbatim from cmake/Baseline.cmake under `cmake -P`: it must accept
the selected repair list and refuse generic fixes OFF or a list without the repair. The other checks are
source facts about the delta, the emitter words and (optionally) the materialized decoder; none of them
executes RT64 or observes runtime behaviour.

    python -I tests/baseline_pop_projection_guard_test.py <repo> [--cmake PATH]
        [--decoder <build>/baseline/rt64_generic/rt64_gbi_extended.cpp]
"""
import argparse
import hashlib
import json
import re
import subprocess
import tempfile
from pathlib import Path

BEGIN = '# BEGIN tooie-pop-projection-guard'
END = '# END tooie-pop-projection-guard'
REPAIR = 'pop_matrix_group_projection'
RT64_PIN = '6f1c2d99a4ea571c139f449c326fd176ba8f3496'
SELECTED_READ = '(*dl)->p1(8, 1)'
UPSTREAM_READ = '(*dl)->p0(8, 1)'


def need(ok, why):
    if not ok:
        raise SystemExit('FAIL: ' + why)


def guard_and_repairs(text):
    begins = [m.start() for m in re.finditer(re.escape(BEGIN), text)]
    ends = [m.start() for m in re.finditer(re.escape(END), text)]
    need(len(begins) == 1 and len(ends) == 1 and begins[0] < ends[0], 'one ordered guard block')
    preceding = text[:begins[0]].rstrip().splitlines()[-1].strip()
    need(preceding == 'set(_baseline_bk_pipeline ON)', 'guard runs only inside the selected BK pipeline branch')
    lists = list(re.finditer(r'^\s*set\(_baseline_generic_repairs ([^)\n]*)\)\s*$', text, re.M))
    need(len(lists) == 1 and lists[0].start() < begins[0], 'one generic repair list, defined before the guard')
    need(len(re.findall(r'foreach\(_repair IN LISTS _baseline_generic_repairs\)', text)) == 1,
         'the repair materialization loop consumes that list')
    return text[begins[0]:ends[0]], lists[0].group(1).split()


def run_guard(cmake, block, fixes, repairs):
    lines = ['cmake_minimum_required(VERSION 3.20)',
             'set(TOOIE_BASELINE_RT64_GENERIC_FIXES %s)' % ('ON' if fixes else 'OFF')]
    if repairs is not None:
        lines.append('set(_baseline_generic_repairs %s)' % ' '.join(repairs))
    with tempfile.TemporaryDirectory() as tmp:
        script = Path(tmp) / 'guard.cmake'
        script.write_text('\n'.join(lines) + '\n' + block + '\n')
        result = subprocess.run([cmake, '-P', str(script)], capture_output=True, text=True)
    return result.returncode, result.stdout + result.stderr


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('repo', type=Path)
    parser.add_argument('--cmake', default='cmake')
    parser.add_argument('--decoder', type=Path)
    args = parser.parse_args()
    repo = args.repo.resolve()

    # 1. Executed gate: the actual guard text under cmake -P.
    block, repairs = guard_and_repairs((repo / 'cmake/Baseline.cmake').read_text())
    need(REPAIR in repairs, 'selected generic repair list contains ' + REPAIR)
    code, output = run_guard(args.cmake, block, True, repairs)
    need(code == 0, 'selected configuration accepted:\n' + output)
    for label, fixes, listed in (('generic fixes OFF, list undefined', False, None),
                                 ('generic fixes OFF, list present', False, repairs),
                                 ('generic fixes ON, repair removed', True, [r for r in repairs if r != REPAIR])):
        code, output = run_guard(args.cmake, block, fixes, listed)
        need(code != 0 and REPAIR in output, label + ' refused:\n' + output)

    # 2. Source facts (not RT64 execution): the delta reads word 1 bit 8 and menu text needs exactly that.
    delta = json.loads((repo / 'patches/rt64' / (REPAIR + '.delta.json')).read_text())
    need(delta['upstream']['commit'] == RT64_PIN and delta['upstream']['path'] == 'src/gbi/rt64_gbi_extended.cpp',
         'delta targets the pinned RT64 decoder')
    operations = delta['operations']
    need(len(operations) == 1 and operations[0]['replacement_lines'] == ['            const uint8_t proj = %s;\n' % SELECTED_READ],
         'delta replaces only the selector read, with word 1 bit 8')
    menu = (repo / 'tools/baseline/menu_text.hpp').read_text()
    pops = re.findall(r'w\[n\+\+\] = 0x6400000Du; w\[n\+\+\] = ([^;]+);', menu)
    need(pops.count('1u | (1u << 8)') == 1, 'menu text close() pops one projection group')
    need('w[n++] = 0x6400000Cu; w[n++] = projection_id; w[n++] = projection_adjust;' in menu and
         'projection_adjust == 0x00200003u' in menu, 'menu text open() pushes that projection group')
    word0, word1 = 0x6400000D, 1 | (1 << 8)
    need(((word0 >> 8) & 1) == 0 and ((word1 >> 8) & 1) == 1 and (word1 & 0xFF) == 1,
         'the selector exists only in word 1 bit 8; the unrepaired word 0 read yields MODELVIEW')

    # 3. Optional: the materialized decoder in the selected build is that recipe's exact output.
    if args.decoder:
        data = args.decoder.read_bytes()
        need(hashlib.sha256(data).hexdigest() == delta['output']['sha256'], 'materialized decoder hash')
        text = data.decode('utf-8')
        start = text.index('void popMatrixGroupV1(')
        body = text[start:text.index('}', start)]
        need(SELECTED_READ in body and UPSTREAM_READ not in body, 'materialized popMatrixGroupV1 reads word 1 bit 8')

    print('baseline_pop_projection_guard: ok (4 executed guard cases; source facts only, no RT64 execution)'
          + (' + materialized decoder' if args.decoder else ''))


if __name__ == '__main__':
    main()
