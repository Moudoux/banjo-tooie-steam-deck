"""Component-qualified four Trot replacements and two guest owner helpers."""
import hashlib
from pathlib import Path
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection
from prepare_cpu625 import facts
BODIES = ["bs_setState", "bainput_update", "func_80800710_bsbtrot", "func_800F86D8"]
HELPERS = ["tooie_trot_toggle_retire", "tooie_trot_toggle_move"]
EXPORTS = set(BODIES + HELPERS)
IMPORTS = {"tooie_baseline_trot_toggle", "func_8009CAAC", "func_8009CAF8",
    "func_8009CA70", "func_8008E39C", "bakey_released", "bakey_pressed",
    "func_800D8FF8", "func_800F0E00", "bainput_enable", "func_800F46D8", "D_80135490"}

def inspect_object(path):
    with path.open("rb") as stream:
        elf = ELFFile(stream)
        assert elf.elfclass == 32 and not elf.little_endian and elf["e_machine"] == "EM_MIPS"
        flags = elf["e_flags"]
        assert flags & 0xF0000000 == 0x10000000 and flags & 0xF000 == 0x1000
        assert not flags & 0x20
        symbols = list(elf.get_section_by_name(".symtab").iter_symbols())
        definitions = {s.name for s in symbols if s["st_info"]["type"] == "STT_FUNC"
                       and s["st_shndx"] != "SHN_UNDEF"}
        imports = {s.name for s in symbols if s.name and s["st_shndx"] == "SHN_UNDEF"}
        assert definitions == EXPORTS, definitions
        assert imports == IMPORTS, imports
        allocated = {s.name for s in elf.iter_sections() if s["sh_flags"] & 2 and s["sh_size"]}
        assert allocated == {".recomp_patch", ".tooie_trot_helpers", ".tooie_trot_state", ".reginfo", ".MIPS.abiflags"}, allocated
        state = elf.get_section_by_name(".tooie_trot_state")
        assert state["sh_type"] == "SHT_PROGBITS" and state.data() == bytes(12)
        functions = {}

        for symbol in symbols:
            if symbol.name not in EXPORTS:
                continue
            expected_section = ".recomp_patch" if symbol.name in BODIES else ".tooie_trot_helpers"
            assert elf.get_section(symbol["st_shndx"]).name == expected_section, symbol.name
            code = elf.get_section(symbol["st_shndx"]).data()
            start, size = symbol["st_value"], symbol["st_size"]
            words = [int.from_bytes(code[i:i+4], "big") for i in range(start, start+size, 4)]
            frames = [0x10000 - (w & 0xFFFF) for w in words
                      if w >> 16 == 0x27BD and w & 0x8000]
            assert len(frames) <= 1 and all(0 < f <= 512 and f % 8 == 0 for f in frames)
            functions[symbol.name] = {"offset": start, "size": size, "frame_bytes": frames[0] if frames else 0, "section": elf.get_section(symbol["st_shndx"]).name}
        relocations = []
        for section in elf.iter_sections():
            if not isinstance(section, RelocationSection):
                continue
            table = elf.get_section(section["sh_link"])
            for relocation in section.iter_relocations():
                target = table.get_symbol(relocation["r_info_sym"])
                relocations.append({"section": section.name, "offset": relocation["r_offset"],
                    "kind": relocation["r_info_type"], "symbol": target.name})
        return {"elf_flags": flags, "definitions": sorted(definitions), "imports": sorted(imports),
                "allocated_sections": sorted(allocated), "functions": functions, "relocations": relocations}


def compile_owner(root, out, run, flags, win):
    source = root / "tools/baseline/guest/minimum-host/talon_trot_toggle.c"
    obj = out / "talon_trot_toggle.o"
    command = flags + ["-I" + win(root / "deps/banjo-tooie/src"), "-c", win(source), "-o", win(obj)]
    run(command, "talon_trot_toggle-compile.log")
    record = facts(obj)
    record["object_contract"] = inspect_object(obj)
    assert record["frames"] == {"bs_setState": 40, "bainput_update": 40,
        "func_80800710_bsbtrot": 32, "func_800F86D8": 40,
        "tooie_trot_toggle_retire": 8, "tooie_trot_toggle_move": 8}, "Component-qualified frames"
    record["compile_argv"] = [str(x) for x in command]
    record["inputs"] = {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
        for p in (source, Path(__file__))}
    return record


def linked(elf, prior_end):
    symbols = {s.name: s for s in elf.get_section_by_name(".symtab").iter_symbols() if s.name}
    helpers = elf.get_section_by_name(".tooie_trot_helpers")
    state = elf.get_section_by_name(".tooie_trot_state")
    patch_index = elf.get_section_index(".recomp_patch")
    assert helpers and state and state["sh_type"] == "SHT_PROGBITS"
    assert helpers["sh_addr"] == ((prior_end + 3) & ~3), "Exact appended Trot helper placement"
    assert state.data() == bytes(12) and state["sh_size"] == 12
    assert state["sh_addr"] == helpers["sh_addr"] + helpers["sh_size"]
    slot = symbols["tooie_trot_toggle_slot"]
    assert slot["st_size"] == 12 and slot["st_value"] == state["sh_addr"]
    assert all(symbols[n]["st_shndx"] == patch_index and symbols[n]["st_size"] for n in BODIES)
    assert all(symbols[n]["st_shndx"] == elf.get_section_index(helpers.name)
               and symbols[n]["st_size"] for n in HELPERS)
    assert symbols["tooie_baseline_trot_toggle"]["st_value"] == 0x8F000030
    assert state["sh_addr"] + 12 <= 0x81000000
    return dict(rom=helpers["sh_addr"] - 0x80801000, ram=helpers["sh_addr"],
        size=helpers["sh_size"], index=elf.get_section_index(helpers.name),
        slot_address=state["sh_addr"], slot_size=12, image_end=state["sh_addr"] + 12)