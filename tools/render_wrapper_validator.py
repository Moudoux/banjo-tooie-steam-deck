"""Strict validation of the canonical finite rendering replacement image."""

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection
from render_wrapper_contract import CALLEES, HEAP_DATA, HUD_LITERALS, HUD_LINKED_LITERALS

def validate_heap_object(path, data=HEAP_DATA, functions=('heap_realloc',), callees=CALLEES,
                         linked=False, storage=None, native=None, literals=False,
                         wrapper_literals=False, aliases=None, extra_file_symbols=(), aspect_literals=False,
                         raw_wrapper_literal=False):
    # LLD always creates two unused MIPS GOT header entries. Discard them only
    # after proving only the two named objects add data, with no GOT/GP dependency.
    with path.open('rb') as stream:
        elf = ELFFile(stream)
        if elf.elfclass != 32 or elf.little_endian or elf['e_machine'] != 'EM_MIPS' or not elf['e_flags'] & 0x1000:
            raise ValueError('patch object is not big-endian MIPS32')
        symbols = elf.get_section_by_name('.symtab')
        named = {symbol.name: symbol for symbol in symbols.iter_symbols() if symbol.name}
        storage, native, aliases = storage or {}, native or {}, aliases or {}
        stored = {row[0] for row in storage.values()}
        defined_objects = {name for name, symbol in named.items()
                           if symbol['st_info']['type'] == 'STT_OBJECT' and symbol['st_shndx'] != 'SHN_UNDEF'}
        if defined_objects != stored:
            raise ValueError(f'patch object definition drift: {sorted(defined_objects)}')
        imports = {symbol.name for symbol in symbols.iter_symbols()
                   if symbol.name and symbol['st_shndx'] == 'SHN_UNDEF'}
        allowed = set(callees) | set(data) | set(functions)
        expected_imports = allowed - stored - set(functions) - (set(native) | set(aliases) if linked else set())
        if imports != expected_imports:
            raise ValueError(f'patch import drift: missing={sorted(expected_imports-imports)}, extra={sorted(imports-expected_imports)}')
        definitions = [symbol.name for symbol in symbols.iter_symbols()
                       if symbol['st_info']['type'] == 'STT_FUNC' and symbol['st_shndx'] not in {'SHN_UNDEF', 'SHN_ABS'}]
        if set(definitions) != set(functions) or len(definitions) != len(functions):
            raise ValueError(f'patch function drift: {definitions}')
        code_index = next(index for index, section in enumerate(elf.iter_sections())
                          if section.name == '.recomp_patch')
        code = elf.get_section(code_index).data()
        origin = elf.get_section(code_index)['sh_addr'] if linked else 0
        references = set()
        allowed_sections = {'.recomp_patch', *storage} | (set() if linked else {'.reginfo', '.MIPS.abiflags'})
        literal_names, literal_refs = set(), []
        wrapper_names, wrapper_refs = set(), []
        aspect_names, aspect_refs = set(), []
        aspect_query_refs = 0
        aspect_index = None
        if wrapper_literals:
            wrapper_name = '.tooie_wrapper_literals' if linked else '.rodata.cst8'
            wrapper = elf.get_section_by_name(wrapper_name)
            if (wrapper is None or wrapper['sh_type'] != 'SHT_PROGBITS' or wrapper['sh_flags'] != 18 or
                    wrapper['sh_size'] != 8 or wrapper['sh_addralign'] != 8 or wrapper['sh_entsize'] != 8 or
                    wrapper.data() != bytes.fromhex('40f0000000000000')):
                raise ValueError('wrapper literal section drift')
            allowed_sections.add(wrapper_name)
            if raw_wrapper_literal and linked:
                raise ValueError('raw wrapper literal is only an object-stage contract')
            wrapper_symbol = '$CPI0_0' if raw_wrapper_literal else '$WRP0_0'
            wrapper_names = {wrapper_symbol}
            symbol = named.get(wrapper_symbol)
            if (symbol is None or (symbol['st_info']['bind'], symbol['st_info']['type'],
                    symbol['st_other']['visibility'], symbol['st_size']) != ('STB_LOCAL','STT_NOTYPE','STV_DEFAULT',0) or
                    not isinstance(symbol['st_shndx'], int) or elf.get_section(symbol['st_shndx']).name != wrapper_name or
                    symbol['st_value'] != wrapper['sh_addr']):
                raise ValueError('wrapper literal symbol drift')
            if {symbol.name for symbol in symbols.iter_symbols() if isinstance(symbol['st_shndx'], int) and
                    elf.get_section(symbol['st_shndx']).name == wrapper_name and symbol['st_info']['type'] != 'STT_SECTION'} != wrapper_names:
                raise ValueError('wrapper literal definition closure drift')
            if linked and not 0x80801000 <= wrapper['sh_addr'] < wrapper['sh_addr'] + 8 <= 0x81000000:
                raise ValueError('wrapper literal outside reserved image')
        if aspect_literals:
            aspect_name = '.tooie_aspect_literals' if linked else '.rodata.cst8'
            aspect = elf.get_section_by_name(aspect_name)
            if (aspect is None or aspect['sh_type'] != 'SHT_PROGBITS' or aspect['sh_flags'] != 18 or
                    aspect['sh_size'] != 24 or aspect['sh_addralign'] != 8 or aspect['sh_entsize'] != 8 or
                    aspect.data() != bytes.fromhex('3ff50000000000013ff58000000000004000000000000000')):
                raise ValueError('aspect literal section drift')
            allowed_sections.add(aspect_name)
            aspect_index = next(i for i,section in enumerate(elf.iter_sections()) if section.name == aspect_name)
            aspect_symbols = {symbol.name:symbol for symbol in symbols.iter_symbols()
                              if symbol['st_shndx'] == aspect_index and symbol['st_info']['type'] != 'STT_SECTION'}
            aspect_names = {f'$CPI0_{i}' for i in range(3)}
            if set(aspect_symbols) != aspect_names:
                raise ValueError('aspect literal definition closure drift')
            for i,name in enumerate(sorted(aspect_names)):
                symbol = aspect_symbols[name]
                if ((symbol['st_info']['bind'],symbol['st_info']['type'],symbol['st_other']['visibility'],symbol['st_size']) !=
                        ('STB_LOCAL','STT_NOTYPE','STV_DEFAULT',0) or symbol['st_value'] != aspect['sh_addr'] + 8*i):
                    raise ValueError('aspect literal symbol drift')
            if linked and not 0x80801000 <= aspect['sh_addr'] < aspect['sh_addr'] + 24 <= 0x81000000:
                raise ValueError('aspect literals outside reserved image')
        if literals:
            literal_name = '.tooie_hud_literals' if linked else '.rodata.cst4'
            literal = elf.get_section_by_name(literal_name)
            if (literal is None or literal['sh_type'] != 'SHT_PROGBITS' or literal['sh_flags'] != 18 or
                    literal['sh_size'] != 16 or literal['sh_addralign'] != 4 or literal['sh_entsize'] != 4 or
                    literal.data() != (HUD_LINKED_LITERALS if linked else HUD_LITERALS)):
                raise ValueError('HUD literal section drift')
            allowed_sections.add(literal_name)
            literal_names = {f'$CPI0_{i}' for i in range(4)}
            for i, name in enumerate(sorted(literal_names)):
                symbol = next((v for v in symbols.iter_symbols() if v.name == name and isinstance(v['st_shndx'],int) and elf.get_section(v['st_shndx']).name == literal_name), None)
                if (symbol is None or (symbol['st_info']['bind'], symbol['st_info']['type'],
                        symbol['st_other']['visibility'], symbol['st_size']) != ('STB_LOCAL','STT_NOTYPE','STV_DEFAULT',0) or
                        not isinstance(symbol['st_shndx'], int) or elf.get_section(symbol['st_shndx']).name != literal_name or
                        symbol['st_value'] != literal['sh_addr'] + ((0,12,8,4) if linked else (0,4,8,12))[i]):
                    raise ValueError(f'HUD literal symbol drift: {name}')
            if {symbol.name for symbol in symbols.iter_symbols() if symbol['st_shndx'] ==
                    next(i for i,s in enumerate(elf.iter_sections()) if s.name == literal_name) and
                    symbol['st_info']['type'] != 'STT_SECTION'} != literal_names:
                raise ValueError('HUD literal definition closure drift')
            if linked and not 0x80801000 <= literal['sh_addr'] < literal['sh_addr'] + 16 <= 0x81000000:
                raise ValueError('HUD literals outside reserved image')
        for section_name, (name, size, alignment) in storage.items():
            section, symbol = elf.get_section_by_name(section_name), named[name]
            if (section is None or section['sh_type'] != 'SHT_PROGBITS' or section['sh_flags'] != 3 or section['sh_size'] != size or
                    section['sh_addralign'] < alignment or symbol['st_size'] != size or
                    elf.get_section(symbol['st_shndx']).name != section_name or symbol['st_value'] != section['sh_addr']):
                raise ValueError(f'serialized object drift: {name}')
            expected = bytes(size) if name == 'tooie_gfx_pools' else bytes(800) + b'\0\0\0\1'
            if name == 'tooie_family_birth_state' and size == 1168:
                suffix = bytearray(364)
                suffix[288:296] = bytes.fromhex('0000000100000001')
                expected += suffix
            if section.data() != expected:
                raise ValueError(f'serialized initializer drift: {name}')
            if linked and not 0x80801000 <= section['sh_addr'] < section['sh_addr'] + size <= 0x81000000:
                raise ValueError(f'serialized object outside reserved range: {name}')
        if linked:
            for symbol in symbols.iter_symbols():
                if symbol['st_shndx'] != 'SHN_ABS':
                    continue
                identity = (symbol['st_info']['bind'], symbol['st_info']['type'], symbol['st_other']['visibility'], symbol['st_value'], symbol['st_size'])
                if symbol.name in native:
                    expected = ('STB_GLOBAL', 'STT_NOTYPE', 'STV_DEFAULT', native[symbol.name], 0)
                elif symbol.name in aliases:
                    expected = ('STB_GLOBAL', 'STT_NOTYPE', 'STV_DEFAULT', aliases[symbol.name], 0)
                elif symbol.name == '_gp':
                    expected = ('STB_LOCAL', 'STT_NOTYPE', 'STV_HIDDEN', 0, 0)
                elif symbol.name in {'heap.c', 'initialize.c', 'graphics_buffers.c', 'family.c', 'hud_projection.c', *extra_file_symbols}:
                    expected = ('STB_LOCAL', 'STT_FILE', 'STV_DEFAULT', 0, 0)
                else:
                    raise ValueError(f'unexpected absolute symbol: {symbol.name}')
                if identity != expected:
                    raise ValueError(f'absolute symbol drift: {symbol.name}')
            if any(name not in named or named[name]['st_shndx'] != 'SHN_ABS' for name in native):
                raise ValueError('missing absolute native import')
            if any(name not in named or named[name]['st_shndx'] != 'SHN_ABS' for name in aliases):
                raise ValueError('missing absolute object alias')
        for section in elf.iter_sections():
            if section.name.startswith('.got') or (section['sh_flags'] & 2 and section['sh_size'] and
                    section.name not in allowed_sections):
                raise ValueError(f'patch defines runtime data: {section.name}')
            if isinstance(section, RelocationSection) and section['sh_info'] != code_index:
                target = elf.get_section(section['sh_info']).name
                table = elf.get_section(section['sh_link'])
                if linked or target != '.pdr' or any(relocation['r_info_type'] != 2 or
                        table.get_symbol(relocation['r_info_sym']).name not in functions
                        for relocation in section.iter_relocations()):
                    raise ValueError(f'unexpected non-code relocation: {section.name}')
            if isinstance(section, RelocationSection) and section['sh_info'] == code_index:
                table = elf.get_section(section['sh_link'])
                for relocation in section.iter_relocations():
                    symbol = table.get_symbol(relocation['r_info_sym'])
                    name, kind = symbol.name, relocation['r_info_type']
                    references.add((name, kind))
                    if aspect_literals and name == 'tooie_get_latched_widescreen_settings' and kind == 4:
                        aspect_query_refs += 1
                    if linked and name in native:
                        offset = relocation['r_offset'] - origin
                        target = ((origin + offset + 4) & 0xF0000000) | ((int.from_bytes(code[offset:offset+4], 'big') & 0x03FFFFFF) << 2)
                        if target != native[name]:
                            raise ValueError(f'native jump target drift: {name}')
                    if aspect_literals and symbol['st_shndx'] == aspect_index:
                        if name not in aspect_names or kind not in {5,6}:
                            raise ValueError('aspect literal relocation drift')
                        aspect_refs.append((name,kind))
                    elif name in literal_names:
                        if kind not in {5,6}:
                            raise ValueError('HUD literal relocation type drift')
                        literal_refs.append((name,kind))
                    elif name in wrapper_names:
                        if kind not in {5,6}:
                            raise ValueError('wrapper literal relocation type drift')
                        wrapper_refs.append((name,kind))
                    elif kind == 4 and symbol['st_info']['type'] == 'STT_SECTION' and symbol['st_shndx'] == code_index:
                        offset = relocation['r_offset'] - origin
                        target = (int.from_bytes(code[offset:offset+4], 'big') & 0x03FFFFFF) << 2
                        if linked:
                            target |= (origin + offset + 4) & 0xF0000000
                        if not origin <= target < origin + len(code):
                            raise ValueError('local jump leaves the patch image')
                    elif (name not in allowed | stored or kind not in {4,5,6} or
                          (kind == 4 and name not in set(callees) | set(functions)) or (name in native and kind != 4)):
                        raise ValueError(f'unsupported patch relocation: {name} type {kind}')
        if any((name, kind) not in references for name in stored for kind in (5, 6)) or any((name, 4) not in references for name in native):
            raise ValueError('missing serialized-object/native code relocation')
        if literals and sorted(literal_refs) != sorted((name,kind) for name in literal_names for kind in (5,6)):
            raise ValueError('HUD literal relocation multiplicity drift')
        if wrapper_literals and sorted(wrapper_refs) != sorted((name,kind) for name in wrapper_names for kind in (5,6)):
            raise ValueError('wrapper literal relocation multiplicity drift')
        if aspect_literals and sorted(aspect_refs) != sorted((name,kind) for name in aspect_names for kind in (5,6)):
            raise ValueError('aspect literal relocation multiplicity drift')
        if aspect_literals:
            if aspect_query_refs != 1:
                raise ValueError('aspect pure query call multiplicity drift')
            projection = named['func_80015D14']
            begin = projection['st_value'] - origin
            end = begin + projection['st_size']
            if begin < 0 or begin % 4 or end > len(code) or end <= begin or end % 4:
                raise ValueError('aspect function extent drift')
            for offset in range(begin, end, 4):
                word = int.from_bytes(code[offset:offset+4], 'big')
                opcode, function = word >> 26, word & 63
                if (opcode == 0 and (function == 9 or
                        (function == 8 and ((word >> 21) & 31) != 31))) or (
                        opcode == 1 and ((word >> 16) & 31) in {16,17,18,19}):
                    raise ValueError('aspect defines an indirect/branch-link call')
        for offset in range(0, len(code), 4):
            word = int.from_bytes(code[offset:offset+4], 'big')
            if word >> 26 not in {2,3} and (28 in {(word >> 21) & 31, (word >> 16) & 31} or
                    (word >> 26 == 0 and (word >> 11) & 31 == 28)):
                raise ValueError(f'patch uses GP at code offset {offset:04X}')
        return {name: (named[name]['st_value'], size) for name, size, _ in storage.values()}
