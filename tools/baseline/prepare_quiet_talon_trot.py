"""Exact component-qualified quiet Talon Trot object; one provider, no helpers."""
import hashlib
from pathlib import Path
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection
from prepare_cpu625 import facts

BODY = "func_808000DC_bsbtrot"
IMPORTS = {"func_8009DE38", "func_8009DCCC", "tooie_baseline_quiet_talon_trot"}

def inspect_object(path):
    with path.open("rb") as stream:
        elf = ELFFile(stream)
        assert elf.elfclass == 32 and not elf.little_endian and elf["e_machine"] == "EM_MIPS"
        elf_flags = elf["e_flags"]
        assert elf_flags & 0xF0000000 == 0x10000000, "EF_MIPS_ARCH_2 required"
        assert elf_flags & 0x0000F000 == 0x00001000, "EF_MIPS_ABI_O32 required"
        assert not elf_flags & 0x20, "EF_MIPS_ABI2 forbidden"
        symbols = list(elf.get_section_by_name(".symtab").iter_symbols())
        defined = {s.name for s in symbols if s["st_info"]["type"] == "STT_FUNC"
                   and s["st_shndx"] != "SHN_UNDEF"}
        imports = {s.name for s in symbols if s.name and s["st_shndx"] == "SHN_UNDEF"}
        allocated = {s.name for s in elf.iter_sections() if s["sh_flags"] & 2 and s["sh_size"]}
        assert defined == {BODY}, defined
        assert imports == IMPORTS, imports
        assert allocated == {".recomp_patch", ".reginfo", ".MIPS.abiflags"}, allocated
        section = elf.get_section_by_name(".recomp_patch")
        data = section.data()
        assert len(data) % 4 == 0
        words = [int.from_bytes(data[i:i+4], "big") for i in range(0, len(data), 4)]
        assert words[0] >> 16 == 0x27BD, "expected addiu sp,sp,-frame prologue"
        frame = 0x10000 - (words[0] & 0xFFFF)
        assert 0 < frame <= 512 and frame % 8 == 0
        indirect = [i * 4 for i, word in enumerate(words)
                    if word >> 26 == 0 and (word & 63) in (8, 9)
                    and word != 0x03E00008]
        assert not indirect, indirect  # Normal jr ra is the sole permitted indirect control transfer.
        relocations = []
        for section in elf.iter_sections():
            if isinstance(section, RelocationSection):
                table = elf.get_section(section["sh_link"])
                for relocation in section.iter_relocations():
                    target = table.get_symbol(relocation["r_info_sym"])
                    target_index = target["st_shndx"]
                    relocations.append({"section": section.name, "offset": relocation["r_offset"],
                                        "kind": relocation["r_info_type"],
                                        "symbol": target.name,
                                        "target_section": elf.get_section(target_index).name
                                            if isinstance(target_index, int) else target_index})
        code_relocs = [r for r in relocations if r["section"] == ".rel.recomp_patch"]
        # Pinned clang duplicates the getter into mutually exclusive pitch branches.
        # Root + independent review of leaf-disassembly.txt established this exact CFG.
        # The unnamed section relocation is a local J to the epilogue, not a callee.
        expected = {0x28: "tooie_baseline_quiet_talon_trot", 0x4C: "func_8009DCCC",
                    0x54: "", 0x60: "tooie_baseline_quiet_talon_trot", 0x78: "func_8009DE38"}
        assert len(code_relocs) == len(expected), code_relocs
        assert {r["offset"] for r in code_relocs} == set(expected), code_relocs
        assert all(r["kind"] == 4 and r["offset"] in expected
                   and r["symbol"] == expected[r["offset"]] for r in code_relocs), code_relocs
        assert next(r for r in code_relocs if r["offset"] == 0x54)["target_section"] == ".recomp_patch"
        assert len(data) == 0x9C and frame == 40, "reviewed pinned leaf shape changed"
        controls = {i * 4: word for i, word in enumerate(words)
                    if word >> 26 in (1, 2, 3, 4, 5, 6, 7, 20, 21, 22, 23)
                    or (word >> 26 == 0 and word & 63 in (8, 9))}
        assert controls == {0x1C: 0x1420000F, 0x28: 0x0C000000, 0x30: 0x1040000F,
                            0x4C: 0x0C000000, 0x54: 0x08000020, 0x60: 0x0C000000,
                            0x68: 0x1440FFF3, 0x78: 0x0C000000, 0x94: 0x03E00008}, controls
        for offset, word in {0x18: 0x2CA10001, 0x20: 0x00808025,
                0x24: 0x3C013F75, 0x2C: 0x3431C28F, 0x38: 0x8E010104,
                0x3C: 0x90250002, 0x40: 0xAFA00010, 0x44: 0x02002025,
                0x48: 0x24064433, 0x50: 0x02203825, 0x58: 0,
                0x5C: 0x3C013F85, 0x64: 0x34311EB8, 0x70: 0x02002025,
                0x74: 0x24054433, 0x7C: 0x02203025, 0x98: 0x27BD0028}.items():
            assert words[offset // 4] == word, (offset, hex(words[offset // 4]))
        cfg_paths = {"nonzero_on": [0x28, 0x4C], "nonzero_off": [0x28, 0x78],
                     "zero_on": [0x60, 0x4C], "zero_off": [0x60, 0x78]}
        assert all(sum(expected[offset] == "tooie_baseline_quiet_talon_trot" for offset in path) == 1
                   for path in cfg_paths.values())
        return {"elf_machine": elf["e_machine"], "elf_class": elf.elfclass,
                "big_endian": not elf.little_endian, "elf_flags": elf_flags,
                "definitions": sorted(defined), "imports": sorted(imports),
                "allocated_sections": sorted(allocated), "frame_bytes": frame,
                "indirect_nonreturn_offsets": indirect, "relocations": relocations,
                "reviewed_cfg_call_offsets": cfg_paths,
                "static_getter_sites": 2, "getters_per_cfg_path": 1,
                "local_epilogue_jump": {"offset": 0x54, "target": 0x80}}


def compile_owner(root, out, run, flags, win):
    source = root / "tools/baseline/guest/minimum-host/quiet_talon_trot.c"
    obj = out / "quiet_talon_trot.o"
    command = flags + ["-c", win(source), "-o", win(obj)]
    run(command, "quiet_talon_trot-compile.log")
    record = facts(obj)
    record["object_contract"] = inspect_object(obj)
    assert record["frames"] == {BODY: 40}
    record["compile_argv"] = [str(x) for x in command]
    record["inputs"] = {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
                        for p in (source, Path(__file__))}
    return record
