"""Linux x86_64 ELF primitives shared by the existing baseline binding gates.

Only GNU ar, GNU ld (bfd) maps written with --cref and GNU objdump are accepted;
every unrecognised shape is refused rather than interpreted."""
from collections import Counter
import hashlib
import os
from pathlib import Path
import re
import shlex
import struct
import subprocess

EM_X86_64 = 62
ET_REL, ET_EXEC, ET_DYN = 1, 2, 3
SHT_SYMTAB, SHT_STRTAB, SHT_RELA, SHT_DYNAMIC, SHT_NOBITS, SHT_REL, SHT_RELR = 2, 3, 4, 6, 8, 9, 19
SHF_WRITE, SHF_ALLOC, SHF_EXECINSTR, SHF_COMPRESSED = 0x1, 0x2, 0x4, 0x800
PT_LOAD, PT_INTERP, PF_X = 1, 3, 1
STB_GLOBAL, STT_OBJECT, STT_FUNC = 1, 1, 2
SHN_LORESERVE = 0xff00
DT_NULL, DT_FLAGS_1, DF_1_PIE = 0, 0x6ffffffb, 0x08000000
MAP_FORMAT = 'elf64-x86-64'


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def compile_command(row, build):
    """Normalize one GNU compile DB row; preserve its exact command/arguments."""
    build = Path(build).resolve()
    directory = Path(row['directory'])
    need(directory.is_absolute(), 'Compile command directory must be absolute')
    directory = directory.resolve()
    source = (directory / row['file']).resolve()
    arguments = row.get('arguments')
    if arguments is None:
        need(isinstance(row.get('command'), str), 'Compile command text is missing')
        arguments = shlex.split(row['command'], posix=True)
    need(isinstance(arguments, list) and all(isinstance(x, str) for x in arguments), 'Invalid compile argument list')
    need(not any(x.startswith('@') for x in arguments), 'Response-file compile commands require explicit support')
    positions = [i for i, value in enumerate(arguments) if value == '-o']
    need(len(positions) == 1 and positions[0] + 1 < len(arguments), 'GNU compile command must have exactly one -o output')
    output = (directory / arguments[positions[0] + 1]).resolve()
    need(output.is_relative_to(build) and output.suffix == '.o', 'Selected GNU object outside build root or not .o')
    if 'output' in row:
        need((directory / row['output']).resolve() == output, 'Declared compile output differs from command -o')
    normalized = dict(row)
    normalized['file'] = str(source)
    normalized['output'] = str(output)
    return normalized


def elf_type(data, allowed):
    need(len(data) >= 64 and data[:4] == b'\x7fELF', 'ELF signature')
    need(data[4:7] == b'\x02\x01\x01', 'ELF64 little-endian version 1 required')
    kind, machine, version = struct.unpack_from('<HHI', data, 16)
    need(machine == EM_X86_64 and version == 1, 'x86_64 ELF required')
    need(kind in allowed, 'Unexpected ELF type %d' % kind)
    return kind


class Elf:
    """Use the project's pyelftools0.33 decoder; keep selected qualification guards here."""
    def __init__(self, path):
        import io
        try:
            import elftools
            need(elftools.__version__ == '0.33', 'Selected ELF qualification requires pyelftools==0.33')
            from elftools.common.exceptions import ELFError
            from elftools.elf.elffile import ELFFile
            from elftools.elf.relocation import RelocationSection, RelrRelocationSection
        except ImportError as error:
            raise ValueError('Linux ELF qualification requires existing pyelftools==0.33 environment') from error
        self.data = Path(path).read_bytes()
        self.kind = elf_type(self.data, (ET_EXEC, ET_DYN))
        try:
            elf = ELFFile(io.BytesIO(self.data))
            need(elf.elfclass == 64 and elf.little_endian and elf['e_machine'] == 'EM_X86_64', 'ELF64 LE x86_64 required')
            need(elf['e_ehsize'] == 64 and elf['e_phentsize'] == 56 and elf['e_shentsize'] == 64, 'ELF64 header entry sizes')
            need(0 < elf['e_phnum'] < 0xffff and 0 < elf['e_shstrndx'] < elf['e_shnum'], 'Extended ELF numbering is refused')
            self.take(elf['e_phoff'], 56 * elf['e_phnum']); self.take(elf['e_shoff'], 64 * elf['e_shnum'])
            self.segments = []
            for segment in elf.iter_segments():
                self.take(segment['p_offset'], segment['p_filesz'])
                need(segment['p_memsz'] >= segment['p_filesz'], 'ELF segment file size exceeds memory span')
                self.segments.append(dict(type={'PT_LOAD': PT_LOAD, 'PT_INTERP': PT_INTERP}.get(segment['p_type'], segment['p_type']),
                    flags=segment['p_flags'], offset=segment['p_offset'], vaddr=segment['p_vaddr'], filesz=segment['p_filesz']))
            need(any(x['type'] == PT_LOAD for x in self.segments), 'ELF has no LOAD segment')
            sections = list(elf.iter_sections())
            kinds = {'SHT_SYMTAB':SHT_SYMTAB,'SHT_STRTAB':SHT_STRTAB,'SHT_RELA':SHT_RELA,'SHT_DYNAMIC':SHT_DYNAMIC,
                     'SHT_NOBITS':SHT_NOBITS,'SHT_REL':SHT_REL,'SHT_RELR':SHT_RELR,'SHT_PROGBITS':1}
            self.sections = []
            for section in sections:
                kind = kinds.get(section['sh_type'], section['sh_type'])
                if kind != SHT_NOBITS: self.take(section['sh_offset'], section['sh_size'])
                self.sections.append(dict(name=section.name,type=kind,flags=section['sh_flags'],addr=section['sh_addr'],
                    offset=section['sh_offset'],size=section['sh_size'],link=section['sh_link'],entsize=section['sh_entsize']))
            tables = [x for x in sections if x['sh_type'] == 'SHT_SYMTAB']
            need(len(tables) == 1 and tables[0]['sh_entsize'] == 24 and tables[0]['sh_size'] % 24 == 0, 'Exactly one unstripped .symtab required')
            self.symbols = {}
            for symbol in tables[0].iter_symbols():
                if not symbol.name: continue
                info=symbol['st_info']
                self.symbols.setdefault(symbol.name,[]).append(dict(bind=STB_GLOBAL if info['bind']=='STB_GLOBAL' else info['bind'],
                    type={'STT_OBJECT':STT_OBJECT,'STT_FUNC':STT_FUNC}.get(info['type'],info['type']),
                    shndx=symbol['st_shndx'],value=symbol['st_value'],size=symbol['st_size']))
            dynamic=[x for x in sections if x['sh_type']=='SHT_DYNAMIC']
            if self.kind == ET_DYN:
                need(len(dynamic)==1 and dynamic[0]['sh_entsize']==16 and dynamic[0]['sh_size']%16==0, 'One .dynamic table required')
                flags=[tag['d_val'] for tag in dynamic[0].iter_tags() if tag['d_tag']=='DT_FLAGS_1']
                need(len(flags)==1 and flags[0]&DF_1_PIE and sum(x['type']==PT_INTERP for x in self.segments)==1, 'ET_DYN must be a PIE executable')
            self.writes=[]
            for section in sections:
                if section['sh_flags']&SHF_ALLOC and isinstance(section,(RelocationSection,RelrRelocationSection)):
                    width={'SHT_RELA':24,'SHT_REL':16,'SHT_RELR':8}[section['sh_type']]
                    need(section['sh_entsize']==width and section['sh_size']%width==0,'Dynamic relocation table shape')
                    self.writes.extend(relocation['r_offset'] for relocation in section.iter_relocations())
        except (ELFError, AssertionError, IndexError, TypeError, KeyError, struct.error, UnicodeError) as error:
            raise ValueError('Malformed selected ELF: '+str(error)) from error

    def take(self, offset, size):
        need(0 <= offset <= len(self.data) and 0 <= size <= len(self.data)-offset, 'ELF file span outside bounds')
        return self.data[offset:offset+size]

    def symbol(self, name, kind):
        rows=self.symbols.get(name,[])
        need(len(rows)==1,'ELF symbol must be unique: '+name)
        row=rows[0]
        # Linux fork: clang's RECOMP_FUNC is weak by design (N64Recomp recomp.h). Uniqueness here and the
        # single GNU map definition and cref definer (GnuMap.owner_of) still bind the one linked copy.
        need(row['bind'] in (STB_GLOBAL,'STB_WEAK') and row['type']==kind,'ELF symbol must be global or weak and correctly typed: '+name)
        need(isinstance(row['shndx'],int) and 0<row['shndx']<min(SHN_LORESERVE,len(self.sections)) and row['size']>0,
             'ELF symbol must be sized and section-defined: '+name)
        return row

    def span(self, row, executable):
        start,size=row['value'],row['size'];section=self.sections[row['shndx']]
        need(section['type']==1 and section['flags']&SHF_ALLOC and not section['flags']&SHF_COMPRESSED,'Symbol requires uncompressed allocated PROGBITS')
        need(bool(section['flags']&SHF_EXECINSTR)==executable and not section['flags']&SHF_WRITE,'Symbol section permissions differ')
        matches=[s for s in self.sections if s['type']==1 and s['flags']&SHF_ALLOC and s['addr']<=start and size<=s['size'] and start-s['addr']<=s['size']-size]
        need(len(matches)==1 and matches[0] is section,'Symbol span must belong to exactly one file-backed section')
        loads=[x for x in self.segments if x['type']==PT_LOAD and x['vaddr']<=start and size<=x['filesz'] and start-x['vaddr']<=x['filesz']-size]
        need(len(loads)==1 and (not executable or loads[0]['flags']&PF_X),'Symbol span requires one compatible file-backed LOAD')
        offset=section['offset']+start-section['addr']
        need(offset==loads[0]['offset']+start-loads[0]['vaddr'],'Section/LOAD file offsets disagree')
        return self.take(offset,size)

    def unrelocated(self, row):
        need(not any(row['value']<target+8 and target<row['value']+row['size'] for target in self.writes),'Dynamic relocation writes into a verified span')


def object_bijection(rows):
    """Basename -> full compile output; GNU ar keeps basenames, so duplicates are refused."""
    out = {}
    for row in rows:
        output = row['output'].replace('\\', '/')
        base = output.rsplit('/', 1)[-1]
        need(base.endswith('.o'), 'ELF object output must end in .o: ' + output)
        need(base not in out, 'Duplicate raw object basename: ' + base)
        out[base] = output
    return out


# Linux fork: libc++ (linked statically) adds the dotless output section __lcxx_override.
SECTION_NAME = r'(?:\.\S+|__lcxx_override)'
SECTION_OUT = re.compile(r'(' + SECTION_NAME + r')\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)(?: load address 0x[0-9a-f]+)?')
OUT_WRAPPED = re.compile(r'\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)(?: load address 0x[0-9a-f]+)?')
SECTION_IN = re.compile(r' ([^\s*]\S*)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S.*)')
IN_WRAPPED = re.compile(r'\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S.*)')
SYMBOL = re.compile(r'\s{16,}0x([0-9a-f]+)\s+([A-Za-z_.$][\w.$@]*)')


class GnuMap:
    """GNU ld (bfd) -Map with --cref: output layout, input-section owners and cref definers."""

    def __init__(self, path, build):
        self.build = Path(build).resolve()
        text = Path(path).read_text()
        start, cref = text.find('\nLinker script and memory map\n'), text.find('\nCross Reference Table\n')
        need(start >= 0 and text.count('\nLinker script and memory map\n') == 1, 'GNU ld (bfd) map required')
        need(cref > start and text.count('\nCross Reference Table\n') == 1, 'GNU ld --cref table required after the memory map')
        memory, table = text[start:cref], text[cref:]
        outputs = re.findall(r'^OUTPUT\((.+) (\S+)\)$', memory, re.M)
        need(len(outputs) == 1 and outputs[0][1] == MAP_FORMAT, 'GNU map must name one elf64-x86-64 output')
        self.output = self.resolve(outputs[0][0])
        self.out_sections, self.inputs, self.defined = [], [], {}
        pending_out = pending_in = current = None
        for line in memory.splitlines():
            line = line.rstrip()
            if pending_in is not None:
                match = IN_WRAPPED.fullmatch(line)
                need(match, 'Unrecognised wrapped input section: ' + pending_in)
                current, pending_in = self.add_input(pending_in, *match.groups()), None
                continue
            if pending_out is not None:
                name, pending_out, current = pending_out, None, None
                match = OUT_WRAPPED.fullmatch(line)
                if match:
                    self.out_sections.append((name, int(match[1], 16), int(match[2], 16)))
                    continue
                # A name without an address is an output statement ld removed; process this line normally.
            if re.fullmatch(SECTION_NAME, line):
                pending_out = line
            elif re.fullmatch(' ' + SECTION_NAME, line):
                pending_in = line[1:]
            elif (match := SECTION_OUT.fullmatch(line)):
                self.out_sections.append((match[1], int(match[2], 16), int(match[3], 16)))
                current = None
            elif (match := SECTION_IN.fullmatch(line)):
                current = self.add_input(*match.groups())
            elif (match := SYMBOL.fullmatch(line)) and current is not None:
                self.defined.setdefault(match[2], []).append((int(match[1], 16), current))
        need(pending_in is None, 'GNU map ends inside a wrapped input section')
        rows = [x.rstrip() for x in table.splitlines()[2:] if x.strip()]
        need(rows and re.fullmatch(r'Symbol\s+File', rows[0]), 'GNU cref header')
        self.cref, symbol = {}, None
        for row in rows[1:]:
            if row[0] != ' ':
                parts = row.split(None, 1)
                need(len(parts) == 2, 'Cross reference row without a file: ' + row)
                symbol = parts[0]
                need(symbol not in self.cref, 'Duplicate cross reference symbol: ' + symbol)
                self.cref[symbol] = [parts[1].strip()]
            else:
                need(symbol is not None, 'Cross reference continuation without a symbol')
                self.cref[symbol].append(row.strip())

    def resolve(self, text):
        path = Path(text)
        return (path if path.is_absolute() else self.build / path).resolve()

    def file(self, text):
        match = re.fullmatch(r'(.+)\(([^()/]+)\)', text)
        return (self.resolve(match[1]), match[2]) if match else (self.resolve(text), None)

    def add_input(self, name, address, size, owner):
        self.inputs.append(dict(name=name, addr=int(address, 16), size=int(size, 16), owner=self.file(owner)))
        return len(self.inputs) - 1

    def bind(self, elf, exe):
        """No GNU map timestamp exists: bind OUTPUT path plus the whole allocated section layout."""
        need(self.output == Path(exe).resolve(), 'GNU map OUTPUT names a different executable')
        actual = {(x['name'], x['addr'], x['size']) for x in elf.sections if x['flags'] & SHF_ALLOC and x['size']}
        names = {x[0] for x in actual}
        need(len(names) == len(actual), 'Duplicate allocated ELF section names')
        mapped = [x for x in self.out_sections if x[0] in names]
        need(len(mapped) == len(actual) and set(mapped) == actual, 'GNU map output sections differ from the ELF allocation layout')

    def owner_of(self, name, size):
        rows = self.defined.get(name, [])
        need(len(rows) == 1, 'GNU map must list one definition: ' + name)
        address, index = rows[0]
        section = self.inputs[index]
        need(section['addr'] <= address and address + size <= section['addr'] + section['size'],
             'GNU map symbol outside its input section: ' + name)
        files = self.cref.get(name)
        need(files and self.file(files[0]) == section['owner'], 'GNU cref definer differs from memory-map owner: ' + name)
        return address, section['owner']


def bound_symbol(elf, gmap, name, kind, executable):
    row = elf.symbol(name, kind)
    data = elf.span(row, executable)
    address, owner = gmap.owner_of(name, row['size'])
    need(address == row['value'], 'GNU map and ELF symbol addresses differ: ' + name)
    return row, owner, data


def owner_text(owner, build):
    path, member = owner
    build = Path(build).resolve()
    text = path.relative_to(build).as_posix() if path.is_relative_to(build) else path.as_posix()
    return text + ('(' + member + ')' if member else '')


def run(command):
    return subprocess.run([str(x) for x in command], check=True, capture_output=True, text=True,
                          env=dict(os.environ, LC_ALL='C')).stdout


def objdump_identity(tools):
    tools = Path(tools)
    tool = tools / 'objdump'
    need(tools.is_absolute() and tool.is_file(), 'Explicit absolute GNU objdump required: ' + str(tool))
    first = (run([tool, '--version']).splitlines() or [''])[0]
    need(first.startswith('GNU objdump '), 'GNU objdump required: ' + first)
    return dict(path=str(tool), sha256=sha(tool.resolve()), version=first)


RELOCATION = re.compile(r'([0-9a-f]{16})\s+(R_X86_64_\w+)\s+(\S+?)([+-]0x[0-9a-f]+)?')


def hook_relocations(objdump, obj, pattern=r'baseline_\w+'):
    """Hook references must be PC32/PLT32 rel32 branches (addend -4) in text; debug records are ignored."""
    names, forms, section, headers = Counter(), Counter(), None, 0
    for line in run([objdump, '-r', '-w', obj]).splitlines():
        line = line.rstrip()
        if not line:
            continue
        if (match := re.fullmatch(r'.+:\s+file format (\S+)', line)):
            need(match[1] == MAP_FORMAT and not headers, 'objdump must read one elf64-x86-64 object')
            headers += 1
        elif (match := re.fullmatch(r'RELOCATION RECORDS FOR \[(.+)\]:', line)):
            section = match[1]
        elif re.fullmatch(r'OFFSET\s+TYPE\s+VALUE', line):
            need(section is not None, 'Relocation header outside a record block')
        elif (match := RELOCATION.fullmatch(line)):
            need(section is not None, 'Relocation outside a record block')
            symbol, addend = match[3], int(match[4] or '0', 16)
            if not re.fullmatch(pattern, symbol) or section.startswith('.debug'):
                continue
            need(section == '.text' or section.startswith('.text.'), 'Hook relocation outside executable text: ' + section)
            need(match[2] in ('R_X86_64_PLT32', 'R_X86_64_PC32') and addend == -4, 'Hook relocation is not a direct rel32 branch: ' + line)
            names[symbol] += 1
            forms[match[2]] += 1
        else:
            need(False, 'Unrecognised objdump relocation line: ' + line)
    need(headers == 1, 'objdump produced no object header')
    return names, forms


INSTRUCTION = re.compile(r'\s*([0-9a-f]+):\s+(?:bnd\s+)?(\S+)(?:\s+(.*))?')


def direct_branches(objdump, exe, name, start, size):
    text = run([objdump, '-d', '-w', '--no-show-raw-insn', '--start-address=0x%x' % start,
                '--stop-address=0x%x' % (start + size), exe])
    need(re.search(r'^0*%x <%s>:$' % (start, re.escape(name)), text, re.M), 'Disassembly does not start at linked caller: ' + name)
    branches = []
    for line in text.splitlines():
        match = INSTRUCTION.fullmatch(line.rstrip())
        if match and match[2] in ('call', 'callq', 'jmp', 'jmpq'):
            target = re.fullmatch(r'(?:0x)?([0-9a-f]+) <([^>]+)>', (match[3] or '').strip())
            if target:
                branches.append((match[2].rstrip('q'), int(target[1], 16), target[2]))
    return branches
