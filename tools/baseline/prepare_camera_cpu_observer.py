"""Exact camera/combined queue materialization; Honey-only uses unchanged legacy tool."""
import argparse
import hashlib
import json
import re
from pathlib import Path

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source', 'patch', 'camera', 'pins', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--honey', type=Path)
    parser.add_argument('--mode', choices=('camera', 'combined'), required=True)
    args = parser.parse_args()
    pins = json.loads(args.pins.read_text())
    selected = pins[args.mode]
    assert sha(args.source) == pins['input_sha256'], 'queue pin'
    assert sha(args.patch) == selected['patch_sha256'], 'patch pin'
    assert sha(args.camera) == pins['camera_inl_sha256'], 'camera pin'
    shadow = args.camera.parent / 'shadow_witness.hpp'
    assert sha(shadow) == pins['shadow_header_sha256'], 'shadow arm pin'
    if args.mode == 'combined':
        assert args.honey and sha(args.honey) == pins['honey_inl_sha256'], 'Honey pin'
    lines = args.source.read_text().splitlines(keepends=True)
    delta = args.patch.read_text().splitlines(keepends=True)
    result = []; cursor = 0; hunks = 0; i = 2
    while i < len(delta):
        match = re.fullmatch(r'@@ -(\d+),\d+ \+\d+,\d+ @@\n', delta[i])
        assert match, 'hunk header'
        start = int(match[1]) - 1
        assert start >= cursor, 'overlap'
        result.extend(lines[cursor:start]); cursor = start; i += 1; hunks += 1
        while i < len(delta) and not delta[i].startswith('@@'):
            row = delta[i]; i += 1
            if row[0] in ' -':
                assert lines[cursor] == row[1:], 'exact hunk mismatch'
                cursor += 1
            if row[0] in ' +': result.append(row[1:])
    result.extend(lines[cursor:]); data = ''.join(result).encode()
    assert hunks == selected['hunks'], 'hunk count'
    assert hashlib.sha256(data).hexdigest() == selected['output_sha256'], 'output pin'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    (args.output.parent / 'camera_projection_observer.inl').write_bytes(args.camera.read_bytes())
    (args.output.parent / 'shadow_witness.hpp').write_bytes(shadow.read_bytes())
    if args.mode == 'combined':
        (args.output.parent / 'frame_match_observer.inl').write_bytes(args.honey.read_bytes())

if __name__ == '__main__': main()
