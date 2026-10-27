"""overlay-ids.txt is read as N64Recomp reads it: ASCII whitespace-separated tokens, not lines.

    .venv-codegen/bin/python -I tests/overlay_id_tokens_test.py <repo> [<pinned N64Recomp src/main.cpp>]
Synthetic files in a temporary directory only; no generated or ROM-derived input is read.
"""
import argparse
import importlib.util
import sys
import tempfile
from pathlib import Path


def need(ok, why):
    if not ok:
        raise SystemExit('FAIL: ' + why)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('repo', type=Path)
    parser.add_argument('recompiler_main', type=Path, nargs='?')
    args = parser.parse_args()
    sys.dont_write_bytecode = True
    tools = args.repo.resolve() / 'tools/baseline'
    sys.path.insert(0, str(tools))
    spec = importlib.util.spec_from_file_location('prepare_camera_bridge_under_test', tools / 'prepare_camera_bridge.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)

    ids = ['*'] * 885
    ids[29], ids[438] = '.bsbtrot', '.chhoneycarrier'
    canonical = ('\n'.join(ids) + '\n').encode()
    with tempfile.TemporaryDirectory() as directory:
        def tokens(data):
            path = Path(directory) / 'overlay-ids.txt'
            path.write_bytes(data)
            return m.overlay_id_tokens(path)
        # 1. The maintained writer's form (prepare_codegen.py): tokens equal lines, so generated rows are unchanged.
        need(tokens(canonical) == ids == canonical.decode().splitlines(), 'canonical file: tokens equal lines')
        # 2. Forms the recompiler reads identically, but which line splitting shifts or renames.
        variants = {
            'leading blank line': b'\n' + canonical,
            'blank line inside': canonical.replace(b'\n.bsbtrot\n', b'\n\n.bsbtrot\n'),
            'CRLF': canonical.replace(b'\n', b'\r\n'),
            'trailing spaces and tabs': canonical.replace(b'\n', b' \t\n'),
            'vertical tab and form feed': canonical.replace(b'\n', b'\x0b\x0c\n'),
            'no final newline': canonical.rstrip(b'\n'),
        }
        for label, data in variants.items():
            got = tokens(data)
            need(got == ids and got.index('.chhoneycarrier') == 438 and got.index('.bsbtrot') == 29,
                 label + ': overlay IDs equal the recompiler order')
        need(variants['leading blank line'].decode().splitlines().index('.chhoneycarrier') == 439,
             'line splitting would shift the Honey overlay ID (the corrected mismatch)')
        need(tokens(b'.a .b\n*\n') == ['.a', '.b', '*'], 'several tokens on one line are separate IDs, as with >>')
    # 3. Source contract: the registry header reads tokens.
    source = (tools / 'prepare_camera_bridge.py').read_text(encoding='utf-8')
    need("restoration_lines(sections,overlay_id_tokens(root/'config/overlay-ids.txt'))" in source and
         "overlay-ids.txt').read_text().splitlines()" not in source, 'registry header reads overlay IDs as tokens')
    if args.recompiler_main:
        recompiler = args.recompiler_main.read_text(encoding='utf-8')
        need('while (input_file >> entry)' in recompiler and
             'read_list_file(config.relocatable_sections_path' in recompiler,
             'pinned N64Recomp reads the overlay list as whitespace-separated tokens')
    print('overlay_id_tokens: ok (synthetic files; local header identity is a separate check)')


if __name__ == '__main__':
    main()
