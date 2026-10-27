#!/usr/bin/env python3
"""G0 focused test: prepare_player_guest.py imports the pinned legacy standalone Silo reference.

The module's three pins are replaced by synthetic bytes for this test, and every preparation
step is recorded instead of run. It needs no ROM, toolchain, WSL or build folder and writes only
below a temporary folder. It proves refusal order and exclusive installation, not that the real
preserved files or a real preparation pass.

    python -I tests/player_guest_standalone_reference_test.py <repo>
"""
import argparse
import contextlib
import hashlib
import importlib.util
import io
import json
import os
import re
import sys
import tempfile
import types
from pathlib import Path

SYNTHETIC = {'boot.bin': b'legacy image\x00\x01', 'data.syms.toml': b'[[section]]\n',
             'receipt.json': b'{"condition": "legacy"}\n'}
PINS = {name: (hashlib.sha256(data).hexdigest(), len(data)) for name, data in SYNTHETIC.items()}
OBJECTS = ('bridge.o', 'OBJECT_BINDING.json')


def need(ok, why):
    if not ok:
        raise SystemExit('FAIL: ' + why)


@contextlib.contextmanager
def patched(module, **values):
    saved = {name: getattr(module, name) for name in values}
    module.__dict__.update(values)
    try:
        yield
    finally:
        module.__dict__.update(saved)


def forbidden(*args, **kwargs):
    raise AssertionError('called before the expected refusal')


def reference_folder(path):
    path.mkdir(parents=True)
    for name, data in SYNTHETIC.items():
        (path / name).write_bytes(data)
    return path


def fresh_objects(silo):
    silo.mkdir(parents=True, exist_ok=True)
    for name in OBJECTS:
        (silo / name).write_bytes(name.encode())


def refused(module, reference):
    try:
        module.read_standalone_reference(reference)
    except ValueError:
        return True
    return False


def run_main(module, argv, **values):
    """main() with sys.platform 'linux' and the given replacements; return (exception, stderr)."""
    fake_sys = types.SimpleNamespace(platform='linux', executable=sys.executable, path=[])
    saved_argv, sys.argv = sys.argv, ['prepare_player_guest.py', *argv]
    stderr = io.StringIO()
    try:
        with patched(module, sys=fake_sys, **values), contextlib.redirect_stderr(stderr), \
                contextlib.redirect_stdout(io.StringIO()):
            try:
                module.main()
            except (SystemExit, ValueError) as error:
                return error, stderr.getvalue()
        return None, stderr.getvalue()
    finally:
        sys.argv = saved_argv


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('repo', type=Path)
    repo = parser.parse_args().repo.resolve()
    sys.dont_write_bytecode = True  # no __pycache__ in the checkout
    spec = importlib.util.spec_from_file_location('prepare_player_guest_under_test',
                                                  repo / 'tools/baseline/prepare_player_guest.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    notes = []

    # 1. Source facts: the boot.bin pin is the camera stage's unchanged legacy premise.
    camera = (repo / 'tools/baseline/prepare_camera_bridge.py').read_text(encoding='utf-8')
    premise = (re.search(r"^LEGACY_STANDALONE_IMAGE_SHA256='([0-9a-f]{64})'", camera, re.M)[1],
               int(re.search(r'^LEGACY_STANDALONE_IMAGE_BYTES=(\d+)', camera, re.M)[1]))
    need(m.STANDALONE_REFERENCE['boot.bin'] == premise, 'boot.bin pin equals the camera legacy premise')
    need("assert sha(prior/'boot.bin')==LEGACY_STANDALONE_IMAGE_SHA256" in camera, 'camera legacy premise check present')
    need(set(m.STANDALONE_REFERENCE) == set(SYNTHETIC), 'exactly boot.bin, data.syms.toml and receipt.json')
    need(m.SILO in m.OUTPUTS, 'the Silo folder must be a fresh output')

    with tempfile.TemporaryDirectory() as name, patched(m, STANDALONE_REFERENCE=PINS):
        tmp = Path(name).resolve()
        rom = str(tmp / 'absent.z64')

        # 2. Authentication only reads, and refuses any deviation from the pins.
        good = reference_folder(tmp / 'reference')
        need(m.read_standalone_reference(good) == SYNTHETIC, 'exact reference accepted')
        cases = {
            'missing file': lambda d: (d / 'receipt.json').unlink(),
            'changed byte': lambda d: (d / 'boot.bin').write_bytes(b'legacy image\x00\x02'),
            'extra byte': lambda d: (d / 'data.syms.toml').write_bytes(SYNTHETIC['data.syms.toml'] + b'\n'),
            'directory in place of a file': lambda d: ((d / 'boot.bin').unlink(), (d / 'boot.bin').mkdir()),
        }
        for label, mutate in cases.items():
            case = reference_folder(tmp / label.replace(' ', '-'))
            mutate(case)
            need(refused(m, case), label + ' refused')
        linked = reference_folder(tmp / 'symlinked-file')
        (linked / 'receipt.json').unlink()
        try:
            os.symlink(good / 'receipt.json', linked / 'receipt.json')
        except OSError:
            notes.append('symlink case skipped: no symlink privilege')
        else:
            need(refused(m, linked), 'symlinked file refused even with pinned bytes')

        # 3. Installation: exclusive create of the authenticated bytes plus an explicit record.
        root = tmp / 'install'
        silo = root / m.SILO
        fresh_objects(silo)
        record = m.install_standalone_reference(root, good, SYNTHETIC)
        need(all((silo / n).read_bytes() == data for n, data in SYNTHETIC.items()),
             'installed bytes are the authenticated bytes')
        saved = json.loads((silo / 'STANDALONE_REFERENCE.json').read_text())
        need(saved == record and saved['regenerated'] is False and 'not a fresh reproduction' in saved['scope'],
             'record states an unregenerated import')
        need(saved['files'] == {n: dict(sha256=d, bytes=s) for n, (d, s) in PINS.items()}, 'record names the pins')
        need(saved['fresh_objects'] == {n: hashlib.sha256(n.encode()).hexdigest() for n in OBJECTS},
             'record names the fresh objects beside the import')
        clash_root = tmp / 'clash'
        clash = clash_root / m.SILO
        fresh_objects(clash)
        (clash / 'data.syms.toml').write_bytes(b'current')
        try:
            m.install_standalone_reference(clash_root, good, SYNTHETIC)
        except FileExistsError:
            pass
        else:
            need(False, 'an occupied Silo file is refused')
        need((clash / 'data.syms.toml').read_bytes() == b'current', 'an occupied Silo file is never replaced')
        need(not (clash / 'STANDALONE_REFERENCE.json').exists(), 'no record after a refused import')
        need(not (clash / 'boot.bin').exists(), 'nothing is installed while any target is occupied')

        # 4. main(): the reference is required and authenticated before any write.
        fresh = tmp / 'fresh'
        before_writes = dict(prepare_native_references=forbidden, subprocess=types.SimpleNamespace(run=forbidden))
        error, text = run_main(m, ['--root', str(fresh), '--rom', rom], **before_writes)
        need(isinstance(error, SystemExit) and error.code == 2 and '--standalone-reference' in text,
             'reference required')
        error, _ = run_main(m, ['--root', str(fresh), '--rom', rom, '--standalone-reference', str(tmp / 'changed-byte')],
                            **before_writes)
        need(isinstance(error, ValueError) and 'boot.bin' in str(error), 'unpinned reference refused')
        error, _ = run_main(m, ['--root', str(fresh), '--rom', rom, '--standalone-reference', str(tmp / 'absent')],
                            **before_writes)
        need(isinstance(error, ValueError), 'absent reference refused')
        need(not fresh.exists(), 'refusals wrote nothing')

        # 5. Recorded order: the Silo bridge object only, the import, then camera preparation;
        #    the selected-image gate still decides.
        root = tmp / 'recorded'
        events = []
        carrier_selection = {}

        def record_step(command, **kwargs):
            script = Path(command[1]).name
            events.append([script, *[str(a) for a in command[4:]]])
            if script == 'prepare_silo_bridge.py':
                fresh_objects(root / m.SILO)
            if script == 'prepare_render_bridge.py':
                camera_dir = root / m.CAMERA
                (camera_dir / 'boot.bin').write_bytes(b'not the selected image')
                (camera_dir / 'render_bridge_layout.h').write_text('')
                (camera_dir / 'receipt.json').write_text('{}')

        def carrier(*args, **kwargs):
            events.append(['carrier', (root / m.SILO / 'STANDALONE_REFERENCE.json').is_file()])
            carrier_selection.update(kwargs)

        sys.modules['prepare_camera_bridge'] = types.SimpleNamespace(compile_backpack621_carrier=carrier)
        try:
            error, _ = run_main(m, ['--root', str(root), '--rom', rom, '--standalone-reference', str(good)],
                                prepare_native_references=lambda root_, rom_: events.append(['native']),
                                subprocess=types.SimpleNamespace(run=record_step))
        finally:
            del sys.modules['prepare_camera_bridge']
        need(isinstance(error, ValueError) and 'selected player guest image' in str(error),
             'final selected-image gate still applies')
        need(events == [['native'], ['prepare_baseline_raw.py'], ['prepare_boot_heap.py'],
                        ['prepare_silo_bridge.py', '--raw', '--bridge-object', '--no-prior-honey'], ['carrier', True],
                        ['prepare_render_bridge.py', '--camera', '--raw', '--image', *m.SELECTION]],
             'step order: ' + json.dumps(events))
        need(all((root / m.SILO / n).read_bytes() == data for n, data in SYNTHETIC.items()),
             'imported bytes in the Silo folder')
        need(not (root / m.CAMERA / 'PLAYER_GUEST.json').exists(), 'no player record after a failed image gate')

        # The carrier's CPU625 selection is the one prepare_camera_bridge.prepare derives for the
        # flags prepare_player_guest passes (prepare_render_bridge passes no bk_pipeline).
        derived = {'cpu625': '--cpu625' in m.SELECTION, 'flap_flip_landing': '--flap-flip-landing' in m.SELECTION,
                   'bk_pipeline': None}
        for statement in (r'bk_pipeline=bool\(cpu625 and flap_flip_landing\) if bk_pipeline is None else bk_pipeline',
                          r'ckpack_copy_storage=bool\(cpu625 and flap_flip_landing and not bk_pipeline\)'):
            exec(re.search(r'^\s*(' + statement + r')\s*$', camera, re.M)[1], {}, derived)
        render = (repo / 'tools/baseline/prepare_render_bridge.py').read_text(encoding='utf-8')
        need('bk_pipeline' not in re.search(r'prepare_camera_bridge\.prepare\((.*)\)', render)[1],
             'prepare_render_bridge leaves the BK selection to its default')
        need(carrier_selection.get('ckpack_copy_storage') is derived['ckpack_copy_storage'] is False,
             'carrier CPU625 copy-storage selection equals the image derivation')

    print('player_guest_standalone_reference: ok (synthetic pins and recorded steps; no preparation executed)'
          + ''.join('; ' + note for note in notes))


if __name__ == '__main__':
    main()
