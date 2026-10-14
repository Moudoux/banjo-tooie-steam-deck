"""Semantic verification subordinate to the canonical codegen provenance receipt."""
from pathlib import Path
import argparse
import json
import re
from elftools.elf.elffile import ELFFile
from codegen_provenance import sha
from render_wrapper_contract import (
    OBJECT_CONTRACTS, SOURCES, REPLACEMENTS, NATIVE_IMPORTS, RENDER_STORAGE,
    LITERAL_SECTIONS, ORIGINAL_ELF_SHA256, object_parameters, linked_parameters)
from render_wrapper_validator import validate_heap_object


def verify(directory, receipt=None, require_receipt=True):
    generated = Path(directory).resolve()
    raw = generated / 'heap-patch-products'
    facts_path = raw / 'module-semantic.json'
    facts = json.loads(facts_path.read_text())
    if (facts.get('schema') != 1 or facts.get('definitions') != sorted(REPLACEMENTS) or
            facts.get('replacements') != {name:list(row) for name,row in REPLACEMENTS.items()} or
            facts.get('native_imports') != NATIVE_IMPORTS or
            facts.get('original_elf_sha256') != ORIGINAL_ELF_SHA256):
        raise ValueError('canonical module semantic contract changed')
    records = facts.get('objects', [])
    if [row.get('stem') for row in records] != [row['stem'] for row in OBJECT_CONTRACTS]:
        raise ValueError('canonical object order/closure changed')
    required_outputs = {'heap-patch-products/module-semantic.json', 'heap_patch.c',
                        'render_layout.h', 'render_patch_layout.h',
                        'heap-patch-products/heap.elf', 'heap-patch-products/heap.bin'}
    for contract, record in zip(OBJECT_CONTRACTS, records):
        stem = contract['stem']
        if (record.get('source') != SOURCES[stem] or record.get('functions') != contract['functions'] or
                record.get('imports') != contract['imports']):
            raise ValueError(f'object semantic identity changed: {stem}')
        path = raw / (stem + '.o')
        original = raw / (stem + '.raw.o') if stem == 'model_nodes' else path
        if sha(path) != record['sha256'] or sha(original) != record['raw_sha256']:
            raise ValueError(f'compiled object identity changed: {stem}')
        local_data, functions, calls, storage, native = object_parameters(contract)
        kwargs = dict(storage=storage, native=native, literals=stem == 'hud_projection',
                      wrapper_literals=stem == 'model_nodes',
                      aspect_literals=stem == 'native_aspect_projection')
        if stem == 'model_nodes':
            validate_heap_object(original, local_data, functions, calls, raw_wrapper_literal=True, **kwargs)
            required_outputs.add('heap-patch-products/model_nodes.raw.o')
        validate_heap_object(path, local_data, functions, calls, **kwargs)
        required_outputs.add('heap-patch-products/' + stem + '.o')

    with (raw / 'heap.elf').open('rb') as stream:
        elf = ELFFile(stream)
        symbol = elf.get_section_by_name('.symtab').get_symbol_by_name('tooie_family_birth_state')[0]
        alias = symbol['st_value'] + 804
        data, definitions, calls = linked_parameters()
        layout = validate_heap_object(raw / 'heap.elf', data, definitions, calls, linked=True,
            storage=RENDER_STORAGE, native=NATIVE_IMPORTS, literals=True, wrapper_literals=True,
            aspect_literals=True, aliases={'tooie_wrapper_state': alias},
            extra_file_symbols=('model_nodes.c', 'player_release.c', 'native_aspect_projection.c'))
        sections = [elf.get_section_by_name(name) for name in
                    ('.recomp_patch', *LITERAL_SECTIONS, *RENDER_STORAGE)]
        if any(section is None for section in sections):
            raise ValueError('canonical linked image section absent')
        if any(segment['p_paddr'] != segment['p_vaddr'] for segment in elf.iter_segments()
               if segment['p_type'] == 'PT_LOAD'):
            raise ValueError('linked load address differs from virtual address')
        image = (raw / 'heap.bin').read_bytes()
        base = min(section['sh_addr'] for section in sections)
        end = max(section['sh_addr'] + section['sh_size'] for section in sections)
        if base != 0x80801000 or len(image) != end - base:
            raise ValueError('canonical serialized image extent changed')
        # N64Recomp elf.cpp resolves non-relocatable reference HI16/LO16
        # instructions in context.rom before writing heap.bin. Require exact
        # ELF/image equality for data; the receipt binds both complete files.
        for section in sections[1:]:
            offset = section['sh_addr'] - base
            if image[offset:offset + section['sh_size']] != section.data():
                raise ValueError(f'serialized guest data changed: {section.name}')
    if facts.get('layout') != {name:list(value) for name,value in layout.items()} or facts.get('wrapper_alias') != alias:
        raise ValueError('fresh linked layout differs from semantic record')
    gfx, gfx_size = layout['tooie_gfx_pools']
    family, family_size = layout['tooie_family_birth_state']
    expected_layout = '#pragma once\n' + (
        f'#define TOOIE_GFX_POOLS_VRAM 0x{gfx:08X}u\n'
        f'#define TOOIE_GFX_POOLS_END_VRAM 0x{gfx + gfx_size:08X}u\n'
        f'#define TOOIE_RENDER_BIRTH_VRAM 0x{family:08X}u\n')
    expected_patch_layout = '#pragma once\n' + (
        f'#define TOOIE_RENDER_FAMILY_VRAM 0x{family:08X}u\n'
        f'#define TOOIE_RENDER_FAMILY_BYTES {family_size}u\n'
        f'#define TOOIE_RENDER_GFX_VRAM 0x{gfx:08X}u\n'
        f'#define TOOIE_RENDER_GFX_BYTES {gfx_size}u\n')
    if ((generated / 'render_layout.h').read_text() != expected_layout or
            (generated / 'render_patch_layout.h').read_text() != expected_patch_layout):
        raise ValueError('generated layout headers differ from strict ELF')
    source = (generated / 'heap_patch.c').read_text()
    emitted = re.findall(r'^RECOMP_FUNC void (\w+)\(', source, re.M)
    if len(emitted) != len(REPLACEMENTS) or set(emitted) != set(REPLACEMENTS):
        raise ValueError('canonical replacement source definition closure changed')
    if source != (raw / 'heap.c').read_text():
        raise ValueError('ordinary replacement source differs from strict codegen output')

    if require_receipt:
        receipt = receipt if receipt is not None else json.loads((generated / 'codegen-receipt.json').read_text())
        if receipt.get('schema') != 1 or not isinstance(receipt.get('outputs'), dict):
            raise ValueError('canonical codegen receipt missing or unsupported')
        for name in required_outputs:
            if receipt['outputs'].get(name) != sha(generated / name):
                raise ValueError(f'canonical receipt lacks/does not bind module output: {name}')
    # Whole-corpus/program/continuation bindings remain the canonical full/replay
    # entrypoint's responsibility, after the real complete lifter.
    return dict(definitions=15, objects=8, native_imports=9,
                family_bytes=family_size, gfx_bytes=gfx_size, wrapper_alias=alias)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--directory', required=True)
    print(json.dumps(verify(parser.parse_args().directory), sort_keys=True))
