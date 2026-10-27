#!/usr/bin/env python3
"""L11 phase 2 focused test: restoration rows are generated from selected.syms-shaped rows with the maintained
owners' names, and map to exactly the numbers the retained literal branches in adapters.cpp use.

The synthetic sections carry the U2-confirmed shape (section identity, one row per name, extent, overlay ID,
function count); this proves the generator's mapping and the source contract, not the local selected.syms
values (U2) or registration at runtime. No generation, build or game. Needs the .venv-codegen packages.

    .venv-codegen/bin/python -I tests/registry_restoration_rows_test.py <repo>
"""
import argparse
import importlib.util
import itertools
import re
import sys
from pathlib import Path


def need(ok, why):
    if not ok:
        raise SystemExit('FAIL: ' + why)


def functions(base, rows, total):
    named = [dict(name=n, vram=base + o, size=s) for n, o, s in rows]
    filler = [dict(name='func_%08X' % (base + 0x2000 + 0x10 * i), vram=base + 0x2000 + 0x10 * i, size=4)
              for i in range(total - len(named))]
    return named + filler


def sections():
    return {
        '.boot': dict(name='.boot', rom=0x1000, vram=0x80000400, size=0x6000,
                      functions=functions(0x80000400, [('boot_osInitialize', 0x12F0, 0x2A8)], 4)),
        '.core1': dict(name='.core1', rom=0x01E29B60, vram=0x80012030, size=0x31350,
                       functions=functions(0x80012030, [('tooie_core1_osInitialize', 0x1B780, 0x2A8),
                                                        ('rom_read_word', 0xC1E0, 0x8C), ('heap_realloc', 0x91D8, 0x180)], 8)),
        '.chhoneycarrier': dict(name='.chhoneycarrier', rom=0x02063630, vram=0x80800000, size=0x440,
                                functions=functions(0x80800000, [('func_80800380_chhoneycarrier', 0x380, 0x3C)], 6)),
        '.bsbtrot': dict(name='.bsbtrot', rom=0x01F14F50, vram=0x80800000, size=0x2F30,
                         functions=functions(0x80800000, [('func_808000DC_bsbtrot', 0xDC, 0x44),
                                                          ('func_80800710_bsbtrot', 0x710, 0x4C)], 83)),
    }


def overlay_ids():
    ids = ['*'] * 885
    ids[29], ids[438] = '.bsbtrot', '.chhoneycarrier'
    return ids


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('repo', type=Path)
    repo = parser.parse_args().repo.resolve()
    sys.dont_write_bytecode = True
    tools = repo / 'tools/baseline'
    sys.path.insert(0, str(tools))
    spec = importlib.util.spec_from_file_location('prepare_camera_bridge_under_test', tools / 'prepare_camera_bridge.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)

    # 1. Owner names: the boot row is boot_osInitialize, as prepare_boot_heap and the raw condition say.
    need(m.boot_owner.FUNCTIONS == ['boot_osInitialize', 'tooie_core1_osInitialize', 'heap_realloc'], 'boot/heap owner names')
    raw_ignored = (tools / 'baseline_raw_condition.toml').read_text(encoding='utf-8')
    need(all('"%s"' % n in raw_ignored for n in m.boot_owner.FUNCTIONS + [m.ROM_PIO_PROVIDER]), 'restored providers are raw-ignored')

    # 2. Generated rows, exactly.
    lines = m.restoration_lines(sections(), overlay_ids())
    expected = [
        '#define TOOIE_REGISTRY_GENERATED_RESTORATIONS 1', 'static CameraAlias camera_base_restored[] = {',
        '    {baseline_boot_patch_registry::boot_osInitialize,0x1000u,0x80000400u,0x6000u,0x12F0u,0x2A8u},',
        '    {baseline_boot_patch_registry::tooie_core1_osInitialize,0x1E29B60u,0x80012030u,0x31350u,0x1B780u,0x2A8u},',
        '#ifdef BASELINE_ROM_PIO_BRIDGE', '    {::rom_read_word,0x1E29B60u,0x80012030u,0x31350u,0xC1E0u,0x8Cu},', '#endif',
        '#if BASELINE_HEAP_PATCH', '    {baseline_boot_patch_registry::heap_realloc,0x1E29B60u,0x80012030u,0x31350u,0x91D8u,0x180u},', '#endif',
        '};', 'static CameraOverlayAlias camera_overlay_restored[] = {',
        '#if BASELINE_HONEY_PATCH || BASELINE_RENDER_BRIDGE',
        '    {baseline_boot_patch_registry::func_80800380_chhoneycarrier,438u,0x2063630u,0x80800000u,0x440u,0x380u,0x3Cu,6u},', '#endif',
        '#if BASELINE_QUIET_TROT',
        '    {baseline_boot_patch_registry::func_808000DC_bsbtrot,29u,0x1F14F50u,0x80800000u,0x2F30u,0xDCu,0x44u,83u},', '#endif',
        '#if BASELINE_TROT_TOGGLE',
        '    {baseline_boot_patch_registry::func_80800710_bsbtrot,29u,0x1F14F50u,0x80800000u,0x2F30u,0x710u,0x4Cu,83u},', '#endif']
    need(lines == expected, 'generated restoration rows:\n' + '\n'.join(lines))

    # 3. The same numbers as the retained literal branches (legacy custody in adapters.cpp).
    adapters = (tools / 'adapters.cpp').read_text(encoding='utf-8')
    for literal in ('0x800016F0 - 0x80000400', '.func = boot_osInitialize, .offset = offset, .rom_size = 0x2A8',
                    '.func = tooie_core1_osInitialize, .offset = 0x8002D7B0 - 0x80012030, .rom_size = 0x2A8',
                    '.func = rom_read_word, .offset = 0xC1E0, .rom_size = 0x8C', '.func = heap_realloc, .offset = 0x91D8, .rom_size = 0x180',
                    'ids[438] != 433', 'honey.rom_addr != 0x02063630 || honey.ram_addr != 0x80800000', 'honey.size != 0x440',
                    'honey.num_funcs != 5', '.offset = 0x380, .rom_size = 0x3C', 'ids[29] != 30', 'trot.rom_addr != 0x01F14F50',
                    'trot.size != 0x2F30', '(BASELINE_TROT_TOGGLE ? 81 : 82)', 'trot_entries.size() != 83',
                    '.offset = 0xDC, .rom_size = 0x44', '.offset=0x710, .rom_size=0x4C'):
        need(literal in adapters, 'legacy literal retained: ' + literal)
    need(0x800016F0 - 0x80000400 == 0x12F0 and 0x8002D7B0 - 0x80012030 == 0x1B780, 'literal offsets equal generated offsets')
    need(6 - 1 == 5 and 83 - 1 == 82 and 83 - 2 == 81, 'selected.syms counts minus omitted equal the literal raw counts')

    # 4. Refusals: duplicate name across sections, absent overlay ID, extent past the section.
    duplicate = sections(); duplicate['.boot']['functions'].append(dict(name='heap_realloc', vram=0x80003000, size=4))
    missing = overlay_ids(); missing[438] = '*'
    past = sections(); past['.core1']['functions'][0]['size'] = 0x31350
    for label, args in (('duplicate provider row', (duplicate, overlay_ids())), ('absent overlay ID', (sections(), missing)),
                        ('extent past section', (past, overlay_ids()))):
        try:
            m.restoration_lines(*args)
        except AssertionError:
            continue
        need(False, label + ' refused')

    # 5. Source contract: the camera path consumes the rows; L06a counting ignores them.
    registry = (tools / 'camera_registry.hpp').read_text(encoding='utf-8')
    need('#if defined(TOOIE_REGISTRY_GENERATED_RESTORATIONS)' in registry and 'void camera_restore_base(' in registry and
         'void camera_restore_overlays(' in registry and 'section.index!=ordinal' in registry, 'native restoration consumers')
    for call in ('camera_restore_base(entries, base[0], baseline_boot_patch_registry::boot_osInitialize);',
                 'camera_restore_base(core1_entries, base[1], rom_read_word);',
                 'camera_restore_base(core1_entries, base[1], baseline_boot_patch_registry::heap_realloc);',
                 'camera_restore_base(core1_entries, base[1], baseline_boot_patch_registry::tooie_core1_osInitialize);',
                 'camera_restore_overlays(sections, ids);'):
        need(call in adapters, 'camera path: ' + call)
    need(adapters.count('#if BASELINE_CAMERA_IDENTITY && defined(TOOIE_REGISTRY_GENERATED_RESTORATIONS)') == 5, 'five guarded sites')
    l06a = re.compile(r'^    \{baseline_boot_patch_registry::[A-Za-z_][A-Za-z0-9_]*,[0-9]+\},$')
    need(not any(l06a.match(line) for line in lines), 'restoration rows are not counted as providers (L06a)')
    # 6. L11-R4/R5: the native compile-time row counts equal the generated guards for every selection.
    def kept(rows, values, defined):
        count, active = 0, True
        for line in rows:
            if line.startswith('#ifdef '):
                active = line.split()[1] in defined
            elif line.startswith('#if '):
                active = bool(eval(line[4:].replace('||', ' or '), {'__builtins__': {}}, values))
            elif line == '#endif':
                active = True
            elif line.startswith('    {'):
                count += active
        return count
    evaluate = lambda expr, values: int(eval(expr.replace('||', ' or '), {'__builtins__': {}}, values))
    base_expr = re.search(r'#ifdef BASELINE_ROM_PIO_BRIDGE\nconstexpr size_t camera_base_restored_selected=([^;]+);\n'
                          r'#else\nconstexpr size_t camera_base_restored_selected=([^;]+);\n#endif\n'
                          r'static_assert\(std::size\(camera_base_restored\)==camera_base_restored_selected,', registry)
    overlay_expr = re.search(r'constexpr size_t camera_overlay_restored_selected=([^;]+);\n'
                             r'static_assert\(std::size\(camera_overlay_restored\)==camera_overlay_restored_selected,', registry)
    need(base_expr and overlay_expr, 'R4/R5 compile-time row-count checks present')
    need('generated overlay restoration selection' not in registry, 'one overlay count gate, at compile time')
    base_rows = lines[2:lines.index('};')]
    overlay_rows = lines[lines.index('static CameraOverlayAlias camera_overlay_restored[] = {') + 1:]
    names = ('BASELINE_HEAP_PATCH', 'BASELINE_HONEY_PATCH', 'BASELINE_RENDER_BRIDGE', 'BASELINE_QUIET_TROT', 'BASELINE_TROT_TOGGLE')
    for rom_pio in (False, True):
        defined = {'BASELINE_ROM_PIO_BRIDGE'} if rom_pio else set()
        for bits in itertools.product((0, 1), repeat=len(names)):
            values = dict(zip(names, bits))
            need(kept(base_rows, values, defined) == evaluate(base_expr[1 if rom_pio else 2], values),
                 'base row count %r' % ((rom_pio, values),))
            need(kept(overlay_rows, values, defined) == evaluate(overlay_expr[1], values), 'overlay row count %r' % (values,))
    values = dict.fromkeys(names, 1)
    need(kept(base_rows + [base_rows[0]], values, {'BASELINE_ROM_PIO_BRIDGE'}) != evaluate(base_expr[1], values),
         'an extra unguarded base row is what the compile-time check refuses')
    print('registry_restoration_rows: ok (generator mapping and source contract; no runtime registration claim)')


if __name__ == '__main__':
    main()
