#!/usr/bin/env python3
"""L06a focused test: the camera-composition provider and helper counts come from the generated registry
table (camera_registry_data.hpp), and no consumer restates them.

Executes the Baseline.cmake block between the tooie-registry-counts markers under `cmake -P` against
synthetic tables rendered with the generator's own row formats (prepare_camera_bridge.py
write_registry_header). Everything else is a source fact. No generation, configure or build.

    python -I tests/baseline_registry_counts_test.py <repo> [--cmake PATH]
"""
import argparse
import re
import subprocess
import tempfile
from pathlib import Path

BEGIN = '# BEGIN tooie-registry-counts'
END = '# END tooie-registry-counts'


def need(ok, why):
    if not ok:
        raise SystemExit('FAIL: ' + why)


def marked(text):
    begins = [m.start() for m in re.finditer(re.escape(BEGIN), text)]
    ends = [m.start() for m in re.finditer(re.escape(END), text)]
    need(len(begins) == 1 and len(ends) == 1 and begins[0] < ends[0], 'one ordered registry-count block')
    return text[begins[0]:ends[0]]


def run(cmake, block, table):
    with tempfile.TemporaryDirectory() as tmp:
        folder = Path(tmp)
        (folder / 'camera_registry_data.hpp').write_bytes(table.encode())
        script = folder / 'counts.cmake'
        script.write_text('cmake_minimum_required(VERSION 3.20)\nset(_baseline_bridge_dir "%s")\n%s\n'
                          'message(STATUS "counts=${_baseline_provider_expected},${_baseline_helper_expected}")\n'
                          % (folder.as_posix(), block))
        result = subprocess.run([cmake, '-P', str(script)], capture_output=True, text=True)
    output = result.stdout + result.stderr
    found = re.search(r'counts=(\d+),(\d+)', output)
    return result.returncode, (int(found[1]), int(found[2])) if found else None, output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('repo', type=Path)
    parser.add_argument('--cmake', default='cmake')
    args = parser.parse_args()
    repo = args.repo.resolve()
    tools = repo / 'tools/baseline'
    text = lambda p: p.read_text(encoding='utf-8')
    cmake_text = text(repo / 'cmake/Baseline.cmake')
    camera = text(tools / 'prepare_camera_bridge.py')

    # 1. The generator's own row formats: one expected row per provider (deduplicated), and alias rows.
    expected = re.findall(r"lines\.append\('(    \{baseline_boot_patch_registry::%s,%d\},)'%\(name,section\)\)", camera)
    alias = re.findall(r"'(    \{baseline_boot_patch_registry::%s,0x%Xu,0x%Xu,0x%Xu,0x%Xu,0x%Xu\},)'", camera)
    need(len(expected) == 1 and alias, 'generator expected/alias row formats')
    need('for name in dict.fromkeys(expected_names):' in camera, 'one expected row per provider')
    row, alias_row = expected[0], alias[0]

    # 2. Executed: the CMake block counts expected rows only, and helpers by nonzero section.
    providers = [('tooie_silo_preflight', 1), ('func_800DCF48', 0), ('tooie_player_begin', 2),
                 ('tooie_bk_frame_begin', 2), ('func_800A7840', 0), ('tooie_shadow_reset', 6),
                 ('tooie_camera_construction_begin', 3), ('_sufade_entrypoint_1', 0), ('tooie_ckcopy_root_begin', 14)]

    def table(rows):
        return '\n'.join(
            ['// Generated from the exact frozen camera roster and selected section identities.',
             'static constexpr uint32_t camera_actor_palette_selected = 1u;',
             'static CameraAlias camera_aliases[] = {', alias_row % ('func_800DE448', 0x1000, 0x80000400, 0x200, 0x10, 0x20), '};',
             'static CameraAlias camera_core2_aliases[] = {', alias_row % ('func_800EBED4', 0x2000, 0x80010000, 0x400, 0x0, 0x40),
             '#if BASELINE_RENDER_SILO', alias_row % ('func_800DD998', 0x2000, 0x80010000, 0x400, 0x40, 0x40), '#endif', '};',
             'static CameraPatchExpected camera_expected_patches[] = {'] + [row % r for r in rows] +
            ['};', 'static constexpr uint32_t npc665_shared_sections[5][6] = {',
             '    {0x0u,0x80801000u,0x96C8u,0x1u,0x27u,0xBBu},', '};']) + '\n'

    block = marked(cmake_text)
    code, counts, output = run(args.cmake, block, table(providers))
    need(code == 0 and counts == (9, 6), 'registry counts (9 providers, 6 helpers): %r\n%s' % (counts, output))
    code, counts, output = run(args.cmake, block, table([]))
    need(code != 0 and 'lists no providers' in output, 'an empty registry table is refused:\n' + output)

    # 3. Source facts: the final gate compares receipt and boot.c with these counts; no ladder remains.
    need(not re.search(r'math\(EXPR _baseline_(provider|helper)_expected', cmake_text), 'no CMake count ladder')
    gate = re.search(r'NOT _baseline_selected_count EQUAL _baseline_provider_expected OR\s+NOT _baseline_actual_body_count EQUAL '
                     r'_baseline_provider_expected OR NOT _baseline_helper_count EQUAL _baseline_helper_expected', cmake_text)
    need(gate and cmake_text.index(END) < gate.start(),
         'the final gate compares receipt definitions, boot.c bodies and receipt helpers with the table counts')
    registry = text(tools / 'camera_registry.hpp')
    include = registry.index('#include "camera_registry_data.hpp"')
    owner = registry.find('constexpr size_t camera_patch_count=std::size(camera_expected_patches);')
    need(owner > include and '53+camera_shadow_helper_count' not in registry, 'native provider count is the generated table size')
    need('constexpr size_t camera_pre_replacement_count' in registry, 'the independent section0 replacement expectation stays')
    for path, restated in ((tools / 'camera_registry.hpp', r'TOOIE_BK_HELPERS==\d'),
                           (tools / 'bk_graph_thunks.hpp', r'TOOIE_BK_HELPERS==\d'),
                           (tools / 'prepare_camera_bridge.py', r'len\(held\.BK_HELPERS\)==\d')):
        need(not re.search(restated, text(path)), '%s restates the BK helper count' % path.name)
    need(re.search(r'assert len\(BK_HELPERS\) == len\(set\(BK_HELPERS\)\) == \d+', text(tools / 'prepare_silo_bridge.py')),
         'the owner keeps its exact BK helper count')
    print('baseline_registry_counts: ok (executed table counting; source facts only otherwise)')


if __name__ == '__main__':
    main()
