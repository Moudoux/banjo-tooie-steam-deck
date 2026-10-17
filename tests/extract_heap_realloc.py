#!/usr/bin/env python3
"""Extract exact canonical guest bodies for focused fixtures and metadata."""

import re
import hashlib, json
import sys
from pathlib import Path


def extract(source: str, function: str = "heap_realloc") -> str:
    if function not in ("func_808000DC_bsbtrot", "func_8009DE38", "func_800E7EF4", "heap_realloc", "func_80800380_chhoneycarrier",
                        "boot_osInitialize", "tooie_core1_osInitialize", "boot___osSetFpcCsr",
                        "func_800EBED4", "func_8010A79C", "func_800A7840", "func_800E42F0", "func_800DCF48", "func_800DD998",
                        "func_80101808", "func_800DE448", "func_800DE498", "func_80092234", "func_80092898", "func_800F3880",
                        "tooie_actor_palette_begin", "tooie_actor_palette_end",
                        "tooie_npc665_end_body", "tooie_cpu625_retire", "tooie_backpack621_retire",
                        "tooie_backpack621_leaf_begin", "tooie_backpack621_leaf_end",
                        "tooie_npc665_leaf_begin", "tooie_npc665_leaf_end",
                        "tooie_camera_construction_begin", "tooie_camera_construction_end", "tooie_camera_de498_ignore",
                        "func_8008B10C", "anctrl_setSmoothTransition", "tooie_silo_preflight", "tooie_player_begin", "tooie_player_end", "tooie_player_bone_push", "tooie_player_bone_pop", "tooie_player_leaf_begin", "tooie_player_leaf_end"):
        raise RuntimeError("unsupported focused fixture function")
    pattern = re.compile(
        r"(?m)^RECOMP_FUNC void " + function + r"\(uint8_t\* rdram, recomp_context\* ctx\) \{"
    )
    matches = list(pattern.finditer(source))
    if len(matches) != 1:
        raise RuntimeError(f"expected one {function} definition, found {len(matches)}")

    start = matches[0].start()
    cursor = matches[0].end() - 1
    depth = 0
    while cursor < len(source):
        if source[cursor] == "{":
            depth += 1
        elif source[cursor] == "}":
            depth -= 1
            if depth == 0:
                return source[start : cursor + 1] + "\n"
        cursor += 1
    raise RuntimeError(f"unterminated {function} definition")


def main() -> int:
    if len(sys.argv) < 3:
        raise SystemExit("usage: extract_heap_realloc.py SOURCE OUTPUT [FUNCTION ...]")
    source = Path(sys.argv[1]).read_text(encoding="utf-8").replace("\r\n", "\n")
    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)
    functions = sys.argv[3:] or ["heap_realloc"]
    if len(functions) != len(set(functions)):
        raise RuntimeError("duplicate focused fixture function")
    bodies = "\n".join(extract(source, function) for function in functions)
    if Path(sys.argv[1]).name == "persistent_heap_patch.c" and functions == ["func_800E42F0"]:
        metadata = json.loads(Path(sys.argv[1]).with_name("persistent_continuation_coverage.json").read_text())
        rows = [r for r in metadata["function_records"] if r["name"] == functions[0]]
        if metadata["schema"] != 1 or len(rows) != 1 or rows[0]["translation_unit"] != "heap_patch.c":
            raise RuntimeError("HUD lifted owner metadata drift")
        row = rows[0]
        outputs = [r for r in metadata["outputs"] if r["name"] == Path(sys.argv[1]).name]
        if (len(outputs) != 1 or outputs[0]["sha256"] != hashlib.sha256(source.encode()).hexdigest() or
                metadata["generator_sha256"] != hashlib.sha256(Path(__file__).resolve().parents[1].joinpath(
                    "tools/lift_persistent_continuations.py").read_bytes()).hexdigest()):
            raise RuntimeError("HUD lifted output/lifter identity drift")
        entry = re.findall(r"tooie_persist_enter\((\d+)u, persistent_locals, (\d+)u\)", bodies)
        pcs = [p["pc"] for p in row["points"]]
        guards = re.findall(r'tooie_persist_native_guard\("(\w+)"\)', bodies)
        if (entry != [(str(row["id"]), str(len(row["locals"])))] or not pcs or
                sorted(pcs) != sorted(map(int,re.findall(r"^persistent_resume_(\d+):", bodies,re.M))) or
                row["native_contracts_required"] != ["tooie_get_hud_half_width_bits"] or
                sorted(guards) != ["INVALID_CONTINUATION_PC", "tooie_get_hud_half_width_bits"]):
            raise RuntimeError("HUD lifted entry/schema/PC/native contract drift")
        bodies = (f'constexpr uint32_t HUD_LIFTED_FUNCTION_ID={row["id"]}u;\n'
                  f'constexpr uint32_t HUD_LIFTED_LOCAL_WORDS={len(row["locals"])}u;\n'
                  'constexpr uint32_t HUD_LIFTED_PCS[]={' + ','.join(str(p)+'u' for p in pcs) + '};\n'
                  f'constexpr uint32_t HUD_LIFTED_NATIVE_GUARDS={len(row["native_contracts_required"])}u;\n' + bodies)
    output.write_text(bodies, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
