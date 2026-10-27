#!/usr/bin/env python3
"""L11 focused test: generated function tables stay as N64Recomp emitted them, and the registry
validators find providers by identity rather than row position.

Runs prepare_render_bridge.authenticate_function_table with its ELF reader replaced by a synthetic
section and symbol table, over a synthetic recomp_overlays.inl in traversal (not address) order. Source
facts cover the native validators. No generation, build or game. Needs the .venv-codegen packages.

    .venv-codegen/bin/python -I tests/registry_table_order_test.py <repo>
"""
import argparse
import contextlib
import hashlib
import importlib.util
import re
import sys
import tempfile
from pathlib import Path

BASE = 0x80801000
ROWS = [('tooie_b', 0x100, 0x40), ('boot_osInitialize', 0x0, 0x80), ('tooie_a', 0x80, 0x80)]   # traversal order


def need(ok, why):
    if not ok:
        raise SystemExit('FAIL: ' + why)


@contextlib.contextmanager
def patched(module, **values):
    saved = {name: getattr(module, name) for name in values}
    module.__dict__.update(values)
    try:
        yield
    finally:
        module.__dict__.update(saved)


class Symbol(dict):
    def __init__(self, name, offset, size):
        super().__init__(st_value=BASE + offset, st_size=size, st_shndx=1, st_info={'type': 'STT_FUNC'})
        self.name = name


class Symtab:
    def __init__(self, symbols):
        self.symbols = symbols

    def iter_symbols(self):
        return iter(self.symbols)


class Elf:
    def __init__(self, symbols):
        self.symtab = Symtab(symbols)

    def get_section_index(self, name):
        return 1 if name == '.recomp_patch' else None

    def get_section(self, index):
        return {'sh_addr': BASE, 'sh_size': 0x1000}

    def get_section_by_name(self, name):
        return self.symtab if name == '.symtab' else None


def overlay(rows):
    lines = ['%s{ .func = %s, .offset = 0x%X, .rom_size = 0x%X },' % ('    ', n, o, s) for n, o, s in rows]
    return ('// generated\nstatic FuncEntry section_1_recomp_patch_funcs[] = {\n' + '\n'.join(lines) +
            '\n};\nstatic SectionTableEntry section_table[] = {};\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('repo', type=Path)
    repo = parser.parse_args().repo.resolve()
    sys.dont_write_bytecode = True
    tools = repo / 'tools/baseline'
    sys.path.insert(0, str(tools))
    spec = importlib.util.spec_from_file_location('prepare_render_bridge_under_test', tools / 'prepare_render_bridge.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    names = [n for n, _, _ in ROWS]

    def authenticate(rows, symbols, expected=names):
        with tempfile.TemporaryDirectory() as tmp:
            elf_path, inl = Path(tmp) / 'boot.elf', Path(tmp) / 'recomp_overlays.inl'
            elf_path.write_bytes(b'synthetic')
            inl.write_bytes(overlay(rows).encode())
            before = inl.read_bytes()
            with patched(m, ELFFile=lambda stream: Elf(symbols)):
                try:
                    result = m.authenticate_function_table(elf_path, inl, '.recomp_patch', expected)
                except AssertionError:
                    result = None
            need(inl.read_bytes() == before, 'the generated table text is never rewritten')
            return result, hashlib.sha256(before).hexdigest()

    exact = [Symbol(n, o, s) for n, o, s in ROWS]
    result, digest = authenticate(ROWS, exact)
    need(result and result['generated_order'] == names and result['rows_rewritten'] is False and
         result['overlay_sha256'] == digest, 'emitted order kept and reported')
    need([r['name'] for r in result['members']] == ['boot_osInitialize', 'tooie_a', 'tooie_b'], 'members by ELF offset')
    wrong = [('tooie_b', 0x104, 0x40)] + ROWS[1:]
    need(authenticate(wrong, exact)[0] is None, 'a row offset differing from the ELF is refused')
    need(authenticate(ROWS[1:], exact)[0] is None, 'a missing row is refused')
    need(authenticate(ROWS, exact + [Symbol('tooie_extra', 0x200, 0x10)])[0] is None, 'an undeclared section function is refused')
    overlap = [Symbol('tooie_b', 0x100, 0x40), Symbol('boot_osInitialize', 0x0, 0x90), Symbol('tooie_a', 0x80, 0x80)]
    need(authenticate([('tooie_b', 0x100, 0x40), ('boot_osInitialize', 0x0, 0x90), ('tooie_a', 0x80, 0x80)], overlap)[0] is None,
         'overlapping ELF extents are refused')

    text = lambda p: p.read_text(encoding='utf-8')
    source = text(tools / 'prepare_render_bridge.py')
    body = source[source.index('def authenticate_function_table'):source.index('PAIR_FUNCTIONS = ')]
    need('write_bytes' not in body and 'only_declared_rows_reordered' not in body, 'authenticate_function_table is read-only')
    registry = text(tools / 'camera_registry.hpp')
    need('section.funcs[i-1]' not in registry and 'std::sort(extents.begin(),extents.end());' in registry,
         'camera overlap check is order-independent')
    adapters = text(tools / 'adapters.cpp')
    need('section_table[0].funcs[' not in adapters, 'no positional selected-initializer check')
    for provider in ('boot_osInitialize', 'tooie_core1_osInitialize', 'heap_realloc'):
        need('initializer_provider(%s)' % provider in adapters, provider + ' found by identity')
    need('entries.push_back({.func = boot_osInitialize' in adapters, 'J2 re-injection retained')
    print('registry_table_order: ok (synthetic authentication; source facts for native validators)')


if __name__ == '__main__':
    main()
