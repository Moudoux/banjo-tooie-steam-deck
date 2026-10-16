"""Exercise only the authored-header writer, without importing image generators."""
import ast
import os
from pathlib import Path
import tempfile

root = Path(__file__).resolve().parents[1]
source = root / "tools" / "baseline" / "prepare_camera_bridge.py"
tree = ast.parse(source.read_text(encoding="utf-8"))
helper = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "write_if_changed")
namespace = {}
exec(compile(ast.Module(body=[helper], type_ignores=[]), str(source), "exec"), namespace)
write_if_changed = namespace["write_if_changed"]

with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / "layout.h"
    text = "#pragma once\n#define VALUE 1\n"
    write_if_changed(path, text)
    assert path.read_bytes() == text.encode("utf-8")

    timestamp = 1_600_000_000_000_000_000
    os.utime(path, ns=(timestamp, timestamp))
    write_if_changed(path, text)
    assert path.stat().st_mtime_ns == timestamp

    changed = "#pragma once\n#define VALUE 2\n"
    write_if_changed(path, changed)
    assert path.read_bytes() == changed.encode("utf-8")

    path.write_bytes(text.replace("\n", "\r\n").encode("utf-8"))
    write_if_changed(path, text)
    assert path.read_bytes() == text.encode("utf-8")

print("PASS baseline layout header writer: absent, identical mtime, changed, CRLF")
