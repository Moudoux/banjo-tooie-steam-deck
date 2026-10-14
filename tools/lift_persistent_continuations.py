#!/usr/bin/env python3
"""Lift canonical generated C into explicit continuations without replacing it.

The default action materializes all translation units into an EXISTING build
directory, plus a coverage/identity manifest. Native callbacks are fail-closed
dependencies, not implicitly serializable. Output is local derived game code:
never commit/package/upload it. This generator is the tracked source of truth.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import re

BODY = re.compile(r"^RECOMP_FUNC void (\w+)\([^\n]*\n.*?^;}\n", re.M | re.S)
PREAMBLE = "    uint64_t hi = 0, lo = 0, result = 0;\n    int c1cs = 0;\n"
DECL = re.compile(r"^(?P<indent>\s*)(?:const )?(?P<type>gpr|int|int32_t|uint32_t|uint64_t)\s+"
                  r"(?P<name>\w+)\s*=\s*(?P<value>[^;]+);", re.M)
CALL = re.compile(r"^(?P<indent>\s*)(?P<name>\w+)\(rdram,\s*ctx\);\s*$")
LOOKUP = re.compile(r"^(?P<indent>\s*)LOOKUP_FUNC\((?P<target>[^\n]+)\)\(rdram,\s*ctx\);\s*$")
NATIVE_TOKEN = re.compile(r"\b((?:tooie_|boot_|os\w*_|__\w*_)\w*|recomp_syscall_handler|pause_self|yield_self(?:_1ms)?)\s*\(")
RUNTIME_ADAPTERS = frozenset({"osRecvMesg_recomp", "osSendMesg_recomp", "osJamMesg_recomp",
    "osStartThread_recomp", "osStopThread_recomp", "osSetThreadPri_recomp", "recomp_syscall_handler"})
CAMERA_SYNC_EDGES = {
    "baseline_camera_construction_begin": "tooie_camera_construction_begin",
    "baseline_camera_construction_end": "tooie_camera_construction_end",
    "baseline_camera_de498_ignore": "tooie_camera_de498_ignore",
}


def camera_sync_adapters(bodies):
    """Exact reviewed clone edges: selected leaves must remain call-free.

    Bounded source loops and the 16+8-byte ABI are reviewed separately. No
    suspension or capture is admitted while the native clone is live.
    """
    semantic = {name: re.sub(r"//[^\n]*|/\*.*?\*/", "", body, flags=re.S)
                for name, body in bodies.items()}
    used = {edge for edge in CAMERA_SYNC_EDGES if any(
        re.search(r"\b" + edge + r"\s*\(", body) for body in semantic.values())}
    if not used:
        return frozenset()
    if used != CAMERA_SYNC_EDGES.keys():
        raise ValueError("incomplete selected camera thunk contract")
    for leaf in CAMERA_SYNC_EDGES.values():
        if leaf not in semantic:
            raise ValueError("missing selected camera leaf: " + leaf)
        declaration = re.match(r"\s*RECOMP_FUNC void " + leaf +
            r"\(uint8_t\* rdram, recomp_context\* ctx\)\s*\{", semantic[leaf])
        if not declaration:
            raise ValueError("unexpected selected camera declaration: " + leaf)
        calls = set(re.findall(r"\b([A-Za-z_]\w*)\s*\(", semantic[leaf][declaration.end():]))
        calls -= {"if", "switch", "ADD32", "MEM_HU", "MEM_W", "S32", "S64"}
        if calls:
            raise ValueError("camera clone leaf is not call-free: " + leaf)
    return frozenset(used)

SYSCALL = re.compile(r"^\s*recomp_syscall_handler\(rdram,\s*ctx,\s*[^;]+\);\s*$")
BACKPACK_BEGIN = '    tooie_model_backpack_draw_begin((uint32_t)ctx->r16);\n'
BACKPACK_END = '    tooie_model_backpack_draw_end();\n'
BACKPACK_BODY_BEGIN = '    tooie_model_backpack_body_draw_begin(rdram, (uint32_t)ctx->r16);\n'
BACKPACK_BODY_END = '    tooie_model_backpack_body_draw_end();\n'
GPU_NODE_BEGIN = '    tooie_model_gpu_node_begin(rdram, ctx);\n'
GPU_NODE_END = '    tooie_model_gpu_node_end(rdram, ctx);\n'
NEST_BEGIN = re.compile(r'    tooie_model_nest_child_begin\(\(uint32_t\)ctx->r16, 0x80800E(?:30|90|BC)u\);\n')
NEST_END = '    tooie_model_nest_child_end();\n'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def write_if_changed(path, content):
    if not path.exists() or path.read_text(encoding="utf-8") != content:
        path.write_text(content, encoding="utf-8", newline="\n")


def lift_body(body, function_id, generated_names, adapters=frozenset(), checkpoint=None):
    """Preserve instructions; hoist every observed scalar and stage call returns.

    Native admission guards are conservative per function. Capturing an unknown
    native reentry is never treated as an atomic call. A caller's native adapter
    set is a compiled implementation decision, never a state-file option.
    """
    if body.count(PREAMBLE) != 1:
        raise ValueError("generated frame preamble changed")
    name_match = re.match(r"RECOMP_FUNC void (\w+)\([^\n]*\n", body)
    if not name_match:
        raise ValueError("unsupported generated function signature")
    name = name_match[1]
    if "recomp_syscall_handler" in adapters:
        body=body.replace("    pause_self(rdram);", "    tooie_persistent_pause(rdram, ctx);")
        adapters=adapters|{"tooie_persistent_pause"}
    signature = name_match[0]
    content = body[len(signature):]
    content = content.replace(PREAMBLE, "", 1)
    # The authored wrapper's mechanical bracket is compiled guest C. Close
    # its native capture before yielding from DE448, while retaining all guest
    # instructions between that return and the original END import.
    wrapper_begin = '    tooie_wrapper_attachment_begin(rdram, ctx);\n'
    wrapper_end = '    tooie_wrapper_attachment_end(rdram, ctx);\n'
    wrapper_call = '    func_800DE448(rdram, ctx);\n'
    compiled_wrapper = wrapper_begin in content or wrapper_end in content
    if compiled_wrapper:
        if (name != 'babackpack_entrypoint_3' or content.count(wrapper_begin) != 1 or
                content.count(wrapper_end) != 1 or content.count(wrapper_call) != 1):
            raise ValueError(f'{name}: compiled attachment bracket identity drift')
        begin_at, call_at, end_at = (content.index(token) for token in
                                    (wrapper_begin, wrapper_call, wrapper_end))
        if not begin_at < call_at < end_at:
            raise ValueError(f'{name}: compiled attachment bracket order drift')
        region = content[begin_at + len(wrapper_begin):end_at]
        if [m['name'] for line in region.splitlines() if (m := CALL.fullmatch(line))] != ['func_800DE448']:
            raise ValueError(f'{name}: compiled attachment has an intervening call')
        if not content[:end_at].rstrip().endswith('ctx->r4 = ctx->r16 | 0;'):
            raise ValueError(f'{name}: compiled attachment END owner ABI drift')
        content = content.replace(wrapper_end,
            '    // Native attachment END was performed before the DE448 yield test.\n', 1)
    # Strip comments before classifying native tokens; names in instruction
    # comments do not establish executable callback edges.
    semantic = re.sub(r"//[^\n]*", "", content)
    native = sorted(symbol for symbol in set(NATIVE_TOKEN.findall(semantic))
                    if symbol not in generated_names and symbol not in adapters)
    locals_schema = [("uint64_t", "hi"), ("uint64_t", "lo"), ("uint64_t", "result"), ("int", "c1cs")]
    extra = []

    def declaration(match):
        variable = match["name"]
        if variable in {name for _, name in locals_schema + extra}:
            raise ValueError(f"{name}: duplicate/scoped local {variable}")
        extra.append((match["type"], variable))
        return f'{match["indent"]}{variable} = {match["value"]};'

    content = DECL.sub(declaration, content)
    content = content.replace("tooie_camera_result = (ctx->r2 == 0)\n        ?", "tooie_camera_result = (ctx->r2 == 0) ?")
    locals_schema += extra
    # Fail rather than lose a native local if the recompiler changes its output.
    leftover_decl = re.search(r"^\s*(?:(?:const|static|volatile|unsigned|signed)\s+)*"
                              r"(?:int|float|double|[ui]int\d+_t|gpr|fpr|recomp_context)\s+\w+", content, re.M)
    if leftover_decl:
        raise ValueError(f"{name}: unrepresented native declaration {leftover_decl[0]!r}")
    if len(locals_schema) > 128:
        raise ValueError(f"{name}: native local limit exceeded")
    # One canonical generated replacement contains a native call and return on
    # the same line. Split only this standalone tail spelling, preserving order.
    content = re.sub(r"(\);) return;", r"\1\n    return;", content)
    points = []
    next_pc = 1
    direct_calls = indirect_calls = 0

    def stage(pc):
        return "".join(f"    persistent_locals[{i}] = (uint64_t){var};\n"
                       for i, (_, var) in enumerate(locals_schema)) + \
            f"    tooie_persist_store({pc}u, persistent_locals, {len(locals_schema)}u);\n"

    lines = []
    checkpoint_seen = False
    draw_bracket_open = None
    draw_end_after_resume = None
    for line in content.splitlines(keepends=True):
        if draw_end_after_resume:
            if line != draw_end_after_resume:
                raise ValueError(f"{name}: draw bracket lost adjacent end")
            draw_end_after_resume = None
            continue
        if line in (BACKPACK_BEGIN, BACKPACK_BODY_BEGIN, GPU_NODE_BEGIN) or NEST_BEGIN.fullmatch(line):
            if draw_bracket_open:
                raise ValueError(f"{name}: nested draw bracket")
            if line == BACKPACK_BEGIN:
                draw_bracket_open = ("func_800DE448", BACKPACK_END)
            elif line == BACKPACK_BODY_BEGIN:
                draw_bracket_open = ("func_800DE448", BACKPACK_BODY_END)
            elif line == GPU_NODE_BEGIN:
                draw_bracket_open = ("LOOKUP:ctx->r25", GPU_NODE_END)
            else:
                draw_bracket_open = ("func_801018A4", NEST_END)
            lines.append(line)
            continue
        if checkpoint is not None and line.startswith(f"    // 0x{checkpoint:08X}:"):
            if checkpoint_seen:
                raise ValueError(f"{name}: duplicate checkpoint instruction")
            checkpoint_seen = True
            points.append({"pc": checkpoint, "kind": "instruction_checkpoint"})
            lines.append(stage(checkpoint))
            lines.append(f"    if (tooie_persist_checkpoint({checkpoint}u)) return;\n")
            lines.append(f"persistent_resume_{checkpoint}:\n")
        call = CALL.fullmatch(line.rstrip("\n"))
        lookup = LOOKUP.fullmatch(line.rstrip("\n"))
        syscall = "recomp_syscall_handler" in adapters and SYSCALL.fullmatch(line.rstrip("\n"))
        if lookup or syscall or (call and (call["name"] in generated_names or call["name"] in adapters)):
            close_draw = draw_bracket_open
            bracket_callee = ("LOOKUP:" + lookup["target"]) if lookup else call["name"] if call else None
            if close_draw and bracket_callee != close_draw[0]:
                raise ValueError(f"{name}: draw bracket has unexpected guest call")
            pc = next_pc
            next_pc += 1
            if pc == checkpoint:
                raise ValueError("checkpoint PC collides with generated return label")
            points.append({"pc": pc, "kind": "indirect_return" if lookup else "direct_return",
                           "callee": lookup["target"] if lookup else "recomp_syscall_handler" if syscall else call["name"]})
            lines.append(stage(pc))
            if lookup:
                indirect_calls += 1
                lines.append(f'    tooie_persist_resolve(LOOKUP_FUNC({lookup["target"]}))(rdram, ctx);\n')
            else:
                direct_calls += 1
                lines.append(line)
            if compiled_wrapper and call and call['name'] == 'func_800DE448':
                # No guest registers are passed or changed: the typed BEGIN
                # already captured the owner and the native worker identity.
                lines.append('    tooie_wrapper_attachment_native_end();\n')
            if close_draw:
                lines.append(close_draw[1])
                draw_bracket_open = None
                draw_end_after_resume = close_draw[1]
            lines.append("    if (tooie_persist_yielded()) return;\n")
            lines.append(f"persistent_resume_{pc}:\n")
        elif line.strip() == "return;":
            lines.append(f"    tooie_persist_leave({function_id}u);\n")
            lines.append(line)
        elif line == ";}\n":
            lines.append(f"    tooie_persist_leave({function_id}u);\n")
            lines.append(line)
        else:
            if "LOOKUP_FUNC(" in line and not line.lstrip().startswith("//"):
                raise ValueError(f"{name}: unsupported indirect call syntax {line.strip()}")
            if not line.lstrip().startswith("//"):
                for callee in re.findall(r"\b(\w+)\(rdram,\s*ctx\)",line):
                    if callee in generated_names or callee in adapters:
                        raise ValueError(f"{name}: unsupported generated/adapter call syntax {line.strip()}")
            if "recomp_syscall_handler" in adapters and not line.lstrip().startswith("//") and NATIVE_TOKEN.search(line):
                # An opaque native callback may reenter guest code or block.
                # Invalidate its caller's resume label for that entire call.
                # Never mistake an earlier generated call label for a valid
                # continuation through an unrepresented native stack frame.
                if line.lstrip().startswith(("?", ":")):
                    raise ValueError(f"{name}: native callback in multiline expression needs an explicit contract")
                lines.append("    tooie_persist_invalidate();\n")
            lines.append(line)
    if draw_bracket_open or draw_end_after_resume:
        raise ValueError(f"{name}: unterminated draw bracket")
    if checkpoint is not None and not checkpoint_seen:
        raise ValueError(f"{name}: checkpoint instruction not present")
    declarations = PREAMBLE + "".join(f"    {kind} {var} = 0;\n" for kind, var in extra)
    entry = f"    uint64_t persistent_locals[{len(locals_schema)}] = {{0}};\n"
    entry += f"    uint32_t persistent_pc = tooie_persist_enter({function_id}u, persistent_locals, {len(locals_schema)}u);\n"
    entry += "".join(f"    {var} = ({kind})persistent_locals[{i}];\n"
                     for i, (kind, var) in enumerate(locals_schema))
    entry += "".join(f'    tooie_persist_native_guard("{symbol}");\n' for symbol in native)
    entry += "    switch (persistent_pc) {\n    case 0: break;\n"
    entry += "".join(f'    case {point["pc"]}u: goto persistent_resume_{point["pc"]};\n' for point in points)
    entry += '    default: tooie_persist_native_guard("INVALID_CONTINUATION_PC"); return;\n    }\n'
    metadata = {"id": function_id, "name": name, "locals": locals_schema,
                "points": points, "native_contracts_required": native,
                "direct_calls": direct_calls, "indirect_calls": indirect_calls}
    return signature + declarations + entry + "".join(lines), metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    if not args.output_dir.is_dir():
        raise ValueError("Use an existing build directory")
    files = sorted(args.source_dir.glob("*.c"))
    texts = {path: path.read_text(encoding="utf-8") for path in files}
    bodies = [(path, match) for path, text in texts.items() for match in BODY.finditer(text)]
    names = {match[1] for _, match in bodies}
    if len(names) != len(bodies):
        raise ValueError("Duplicate generated function identity")
    camera_adapters = camera_sync_adapters({match[1]: match[0] for _, match in bodies})
    ids = {name: i + 1 for i, name in enumerate(sorted(names))}
    metadata = []
    output_files = []
    inputs = {path.name: digest(text.encode()) for path, text in texts.items()}
    for path, text in texts.items():
        def replace(match):
            transformed, record = lift_body(match[0], ids[match[1]], names, RUNTIME_ADAPTERS | camera_adapters)
            record["translation_unit"] = path.name
            metadata.append(record)
            return transformed
        out = '#include "persistent_state_hooks.h"\n' + BODY.sub(replace, text)
        destination = args.output_dir / ("persistent_" + path.name)
        write_if_changed(destination, out)
        output_files.append({"name": destination.name, "sha256": digest(out.encode())})
    native = collections.Counter(symbol for item in metadata for symbol in item["native_contracts_required"])
    manifest = {
        "schema": 1, "kind": "generated-continuation-coverage-NOT-gameplay-savestate",
        "camera_synchronous_adapters": sorted(camera_adapters),
        "generator_sha256": digest(Path(__file__).read_bytes()), "inputs": inputs,
        "translation_units": len(files), "functions": len(metadata),
        "direct_continuation_calls": sum(item["direct_calls"] for item in metadata),
        "indirect_continuation_calls": sum(item["indirect_calls"] for item in metadata),
        "native_dependencies_by_function_count": dict(sorted(native.items())),
        "max_local_words": max(len(item["locals"]) for item in metadata),
        "outputs": output_files, "function_records": metadata,
    }
    program_identity = digest(json.dumps({"generator": manifest["generator_sha256"], "inputs": inputs},
                                        sort_keys=True).encode())
    registry_header = ('#pragma once\n#include "persistent_state_continuation.hpp"\n'
                       'namespace tooie::continuation::generated {\n'
                       'std::span<const Function> functions();\n'
                       'const char* program_sha256();\n}\n')
    registry_source = ['#include "persistent_continuation_registry.hpp"\nextern "C" {\n']
    for record in metadata:
        registry_source.append(f'void {record["name"]}(uint8_t*,recomp_context*);\n')
    registry_source.append('}\nnamespace tooie::continuation::generated {\nnamespace {\n')
    for record in metadata:
        if record["points"]:
            registry_source.append(f'constexpr uint32_t pcs_{record["id"]}[]={{' +
                ','.join(str(point["pc"]) + 'u' for point in record["points"]) + '};\n')
    registry_source.append('const Function registry[]={\n')
    for record in metadata:
        pcs = f'pcs_{record["id"]}' if record["points"] else 'std::span<const uint32_t>{}'
        registry_source.append(f'{{{record["id"]}u,{record["name"]},{pcs},{len(record["locals"])}u}},\n')
    registry_source.append('};\n}\nstd::span<const Function> functions(){return registry;}\n'
                           f'const char* program_sha256(){{return "{program_identity}";}}\n' + '}\n')
    write_if_changed(args.output_dir / 'persistent_continuation_registry.hpp', registry_header)
    write_if_changed(args.output_dir / 'persistent_continuation_registry.cpp', ''.join(registry_source))
    manifest["program_sha256"] = program_identity
    destination = args.output_dir / "persistent_continuation_coverage.json"
    destination.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: value for key, value in manifest.items()
                      if key not in ("inputs", "outputs", "function_records")}, indent=2))


if __name__ == "__main__":
    main()
