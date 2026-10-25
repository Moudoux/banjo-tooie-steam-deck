#!/usr/bin/env python3
"""Extract exact canonical guest bodies for focused fixtures and metadata."""

import re
import hashlib, json
import sys
from pathlib import Path


DEFAULT_ALLOWED = ("bs_setState", "bainput_update", "func_80800710_bsbtrot", "func_800F86D8",
                        "tooie_trot_toggle_retire", "tooie_trot_toggle_move", "bs_getCurrentState", "bs_getPreviousState", "bs_getNextState", "func_808000DC_bsbtrot", "func_8009DE38", "func_800E7EF4", "heap_realloc", "func_80800380_chhoneycarrier",
                        "boot_osInitialize", "tooie_core1_osInitialize", "boot___osSetFpcCsr",
                        "func_800EBED4", "func_8010A79C", "func_800A7840", "func_800E42F0", "func_800E44FC", "func_800E4640", "func_800E4628", "func_800DCF48", "func_800DD998",
                        "func_80101808", "func_800DE448", "func_800DE498", "func_80092234", "func_80092898", "func_800F3880",
                        "tooie_actor_palette_begin", "tooie_actor_palette_end",
                        "tooie_shadow_mode_begin", "tooie_shadow_mode_end", "tooie_shadow_complete",
                        "tooie_shadow_reset", "tooie_shadow_de498", "tooie_shadow_rectangle_args", "tooie_shadow_mv_end",
                        "tooie_npc665_admit", "tooie_npc665_bone_push", "tooie_npc665_bone_pop",
                        "tooie_actor_cpu_root_begin", "tooie_actor_feather_leaf_id",
                        "tooie_npc665_end", "tooie_cpu625_retire", "tooie_backpack621_retire",
                        "tooie_backpack621_leaf_begin", "tooie_backpack621_leaf_end",
                        "tooie_npc665_leaf_begin", "tooie_npc665_leaf_end",
                        "tooie_camera_construction_begin", "tooie_camera_construction_end", "tooie_camera_de498_ignore",
                        "func_8008B10C", "anctrl_setSmoothTransition", "tooie_silo_preflight", "tooie_player_begin", "tooie_player_end", "tooie_player_bone_push", "tooie_player_bone_pop", "tooie_player_leaf_begin", "tooie_player_leaf_end",
                        "tooie_ckcopy_root_begin", "tooie_ckcopy_segment", "tooie_ckcopy_root_end", "tooie_ckcopy_release",
                        "tooie_cpu625_begin", "tooie_cpu625_admit", "tooie_cpu625_end",
                        "tooie_cpu625_attach_begin", "tooie_cpu625_attach_end", "tooie_shadow_prepare",
                        "func_800DD024",
                        "tooie_secondary_route_begin", "tooie_secondary_route_end", "tooie_secondary_route_arm", "tooie_secondary_route_disarm", "tooie_player_secondary_begin",
                        "tooie_bk_set_model_matrix_group", "tooie_bk_pop_model_matrix_group", "tooie_bk_frame_begin",
                        "tooie_bk_root_begin", "tooie_bk_root_end", "tooie_bk_scope_end",
                        "tooie_bk_menu_model_begin", "tooie_bk_menu_model_end",
                        "tooie_bk_actor_draw_begin", "tooie_bk_de448_begin", "tooie_bk_de448_end",
                        "tooie_bk_list_begin", "tooie_bk_list_end", "tooie_bk_handler_begin", "tooie_bk_handler_end",
                        "tooie_bk_skin_segment", "tooie_bk_skin_capture", "tooie_bk_root_join",
                        "tooie_bk_selector_begin", "tooie_bk_selector_child", "tooie_bk_selector_end",
                        "tooie_bk_sort_children", "tooie_bk_attachment_begin", "tooie_bk_attachment_end",
                        "tooie_bk_shadow_route", "tooie_bk_shadow_light_event", "tooie_bk_shadow_begin",
                        "tooie_bk_shadow_end", "tooie_bk_shadow_project")


def extract(source: str, function: str = "heap_realloc", *, allowed=DEFAULT_ALLOWED) -> str:
    if function not in allowed:
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


def _locate(directory: Path, function: str):
    """Return the one funcs_*.c path and raw bytes that define function."""
    if function not in DEFAULT_ALLOWED:
        raise RuntimeError("unsupported focused fixture function")
    # Keep this canonical definition pattern identical to extract() above.
    pattern = re.compile(
        r"(?m)^RECOMP_FUNC void " + function + r"\(uint8_t\* rdram, recomp_context\* ctx\) \{"
    )
    found = []
    for path in sorted(directory.glob("funcs_*.c")):
        data = path.read_bytes()
        text = data.decode("utf-8").replace("\r\n", "\n").replace("\r", "\n")
        found += [(path, data)] * len(pattern.findall(text))
    if len(found) != 1:
        raise RuntimeError(f"expected one {function} definition in {directory.name}/funcs_*.c, found {len(found)}")
    return found[0]


def main() -> int:
    args = sys.argv[1:]
    expected_sha256 = None
    if args[:1] == ["--source-sha256"]:
        if len(args) < 2 or not re.fullmatch(r"[0-9a-f]{64}", args[1]):
            raise SystemExit("--source-sha256 requires a lowercase 64-digit hex digest")
        expected_sha256, args = args[1], args[2:]
    if len(args) < 2 or "--source-sha256" in args:
        raise SystemExit("usage: extract_heap_realloc.py [--source-sha256 HEX] SOURCE OUTPUT [FUNCTION ...]")
    source_path = Path(args[0])
    if source_path.is_dir():
        # Directory lookup retains the located whole-TU byte pin.
        if expected_sha256 is None or len(args) != 3:
            raise SystemExit("directory SOURCE requires --source-sha256 and exactly one FUNCTION")
        source_path, source_bytes = _locate(source_path, args[2])
    else:
        source_bytes = source_path.read_bytes()
    if expected_sha256 is not None:
        actual_sha256 = hashlib.sha256(source_bytes).hexdigest()
        if actual_sha256 != expected_sha256:
            raise RuntimeError(f"{source_path.name} source SHA256 {actual_sha256} differs from pinned {expected_sha256}")
    # Same universal-newline decoding as the former read_text(encoding="utf-8").
    source = source_bytes.decode("utf-8").replace("\r\n", "\n").replace("\r", "\n")
    output = Path(args[1])
    output.parent.mkdir(parents=True, exist_ok=True)
    functions = args[2:] or ["heap_realloc"]
    if len(functions) != len(set(functions)):
        raise RuntimeError("duplicate focused fixture function")
    bodies = "\n".join(extract(source, function) for function in functions)
    if source_path.name == "persistent_heap_patch.c" and functions == ["func_800E42F0"]:
        metadata = json.loads(source_path.with_name("persistent_continuation_coverage.json").read_text())
        rows = [r for r in metadata["function_records"] if r["name"] == functions[0]]
        if metadata["schema"] != 1 or len(rows) != 1 or rows[0]["translation_unit"] != "heap_patch.c":
            raise RuntimeError("HUD lifted owner metadata drift")
        row = rows[0]
        outputs = [r for r in metadata["outputs"] if r["name"] == source_path.name]
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
