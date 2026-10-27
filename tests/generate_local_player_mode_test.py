#!/usr/bin/env python3
"""L12 focused test: generate_local.py --player routes only CPU generation to --player-inputs.

Loads tools/generate_local.py and replaces each subprocess, hash and toolchain step with a
recorder. It needs no ROM, dependency, network or WSL, and writes only below a temporary folder.
It proves command construction and refusal order, not that either mode generates correct source.

    python -I tests/generate_local_player_mode_test.py <repo>
"""
import argparse
import contextlib
import importlib.util
import io
import json
import os
import sys
import tempfile
import types
from pathlib import Path


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


def refused(module, argv):
    """main(argv) with hashing and every command forbidden; return (exception, stderr)."""
    stderr = io.StringIO()
    with patched(module, run=forbidden, validate_pinned_hashes=forbidden), contextlib.redirect_stderr(stderr):
        try:
            module.main(argv)
        except (SystemExit, RuntimeError) as error:
            return error, stderr.getvalue()
    raise SystemExit('FAIL: main() did not refuse ' + ' '.join(argv))


def occupied(module, root):
    try:
        module.player_cpu_evidence(root)
    except RuntimeError:
        return True
    return False


class Pipeline:
    """main() through the audio step, with every external effect recorded."""

    def __init__(self, module, tmp):
        self.module, self.deps = module, tmp / 'deps'
        self.decomp = self.deps / 'banjo-tooie'
        for name in ('baserom.us.z64', 'decompressed.us.z64', 'build/us/banjotooie_decompressed.z64',
                     'build/us/banjotooie_decompressed.elf', 'tools/requirements.txt',
                     'tools/splat/requirements.txt'):
            (self.decomp / name).parent.mkdir(parents=True, exist_ok=True)
            (self.decomp / name).write_bytes(b'stub')
        self.rom = tmp / 'rom.z64'
        self.rom.write_bytes(b'stub')

    def __call__(self, *extra):
        m, commands = self.module, []

        def record(command, *, cwd=None):
            command = [str(value) for value in command]
            commands.append(command)
            if '--evidence' in command:  # both preparers create their evidence folder
                Path(command[command.index('--evidence') + 1]).mkdir(parents=True, exist_ok=True)

        with patched(m, run=record, require_hash=lambda path, expected, label: expected,
                     sha256=lambda path: m.DECOMP_SHA256,
                     materialize_codegen_tools=lambda deps, decomp: None,
                     executable=lambda build, name: build / name,
                     os=types.SimpleNamespace(name='posix', cpu_count=lambda: 1)), \
                contextlib.redirect_stdout(io.StringIO()):
            code = m.main(['--rom', str(self.rom), '--deps-dir', str(self.deps), '--jobs', '1', *extra])
        need(code == 0, 'recorded pipeline completes')
        cpu = [c for c in commands if c[1].endswith('prepare_codegen.py')]
        audio = [c for c in commands if c[1].endswith('prepare_audio_rsp.py')]
        need(len(cpu) == 1 and len(audio) == 1, 'one CPU and one audio command')
        need(commands.index(cpu[0]) < commands.index(audio[0]), 'audio runs after CPU')
        return cpu[0], audio[0], [c for c in commands if c not in (cpu[0], audio[0])]

    def python_command(self, script, *tail):
        m = self.module
        return [str(v) for v in (m.ROOT / '.venv-codegen/bin/python', m.ROOT / 'tools' / script,
                                 '--rom', self.decomp / 'baserom.us.z64', *tail)]

    def expected_cpu(self, cpu_evidence, *tail):
        return self.python_command('prepare_codegen.py', '--decomp-root', self.decomp,
                                   '--n64recomp', self.deps / 'N64Recomp-codegen/build/N64Recomp',
                                   '--evidence', cpu_evidence, *tail)

    def expected_audio(self, evidence, *tail):
        return self.python_command('prepare_audio_rsp.py', '--decompressed-rom', self.decomp / 'decompressed.us.z64',
                                   '--rsp-recomp', self.deps / 'N64Recomp-codegen/build/RSPRecomp',
                                   '--n64recomp-source', self.deps / 'N64Recomp-codegen',
                                   '--evidence', evidence / 'audio', *tail)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('repo', type=Path)
    repo = parser.parse_args().repo.resolve()
    need((repo / 'tools/requirements-codegen.txt').is_file(), 'run against the checkout root')
    sys.dont_write_bytecode = True  # no __pycache__ in the checkout
    spec = importlib.util.spec_from_file_location('generate_local_under_test', repo / 'tools/generate_local.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    notes = []

    with tempfile.TemporaryDirectory() as name:
        tmp = Path(name).resolve()
        ref = tmp / 'reference-generated'
        absent_rom = str(tmp / 'absent.z64')

        # 1. The exact CLI conflict is refused by argparse, before hashing, the platform check or any write.
        for order in (['--player', '--reference-generated', str(ref)], ['--reference-generated', str(ref), '--player']):
            error, text = refused(m, ['--rom', absent_rom, '--evidence', str(tmp / 'conflict'), *order])
            need(isinstance(error, SystemExit) and error.code == 2 and '--reference-generated' in text,
                 '--player with --reference-generated refused: ' + text)
        need(not (tmp / 'conflict').exists(), 'conflict refusal wrote nothing')

        # 2. CPU evidence: absent or empty is fresh; contents, a file or a symlink are refused untouched.
        root = tmp / 'evidence-check'
        target = root / m.PLAYER_CPU_EVIDENCE
        need(m.player_cpu_evidence(root) == target and not root.exists(), 'absent evidence accepted without writes')
        target.mkdir(parents=True)
        need(m.player_cpu_evidence(root) == target, 'empty CPU evidence accepted')
        (target / 'startup-coverage.json').write_text('kept\n')
        error, _ = refused(m, ['--rom', absent_rom, '--evidence', str(root), '--player'])
        need(isinstance(error, RuntimeError) and 'occupied' in str(error), 'occupied CPU evidence refused by main')
        need((target / 'startup-coverage.json').read_text() == 'kept\n', 'occupied evidence untouched')
        file_root = tmp / 'evidence-file'
        file_root.mkdir()
        (file_root / m.PLAYER_CPU_EVIDENCE).write_text('')
        need(occupied(m, file_root), 'a file in place of CPU evidence refused')
        link_root = tmp / 'evidence-link'
        link_root.mkdir()
        (tmp / 'elsewhere').mkdir()
        try:
            os.symlink(tmp / 'elsewhere', link_root / m.PLAYER_CPU_EVIDENCE, target_is_directory=True)
        except OSError:
            notes.append('symlink case skipped: no symlink privilege')
        else:
            need(occupied(m, link_root), 'symlinked CPU evidence refused even when empty')

        # 3. The platform and ROM checks keep their order in both modes; no command runs.
        for extra in ([], ['--player']):
            caught = None
            with patched(m, run=forbidden):
                try:
                    m.main(['--rom', absent_rom, '--evidence', str(tmp / 'platform'), *extra])
                except (RuntimeError, FileNotFoundError) as error:
                    caught = error
            if os.name == 'nt':
                need(isinstance(caught, RuntimeError) and 'Linux/WSL' in str(caught), 'Windows refusal ' + str(extra))
            else:
                need(isinstance(caught, FileNotFoundError), 'ROM checked before any command ' + str(extra))
        need(not (tmp / 'platform').exists(), 'platform/ROM refusals wrote nothing')

        # 4. Recorded pipeline: legacy commands unchanged; --player changes only the CPU command.
        pipe = Pipeline(m, tmp)
        evidence = tmp / 'evidence'
        legacy_cpu, legacy_audio, legacy_rest = pipe('--evidence', str(evidence))
        need(legacy_cpu == pipe.expected_cpu(evidence / 'codegen'), 'legacy CPU command unchanged')
        need(legacy_audio == pipe.expected_audio(evidence), 'legacy audio command unchanged')
        legacy_receipt = (evidence / 'generate-local.json').read_bytes()
        need('cpu_mode' not in json.loads(legacy_receipt), 'legacy receipt fields unchanged')
        ref_evidence = tmp / 'evidence-ref'
        ref_cpu, ref_audio, _ = pipe('--evidence', str(ref_evidence), '--reference-generated', str(ref))
        need(ref_cpu == pipe.expected_cpu(ref_evidence / 'codegen', '--reference-generated', ref),
             'legacy CPU reference comparison unchanged')
        need(ref_audio == pipe.expected_audio(ref_evidence, '--reference-generated', ref / 'audio_mission01'),
             'legacy audio reference comparison unchanged')

        player_cpu, player_audio, player_rest = pipe('--evidence', str(evidence), '--player')
        need(player_cpu == pipe.expected_cpu(evidence / m.PLAYER_CPU_EVIDENCE, '--player-inputs'),
             'player CPU command adds only --player-inputs and its own evidence folder')
        need(player_audio == legacy_audio, 'player audio command identical to legacy')
        need(player_rest == legacy_rest, 'bootstrap, venv, pip and tool-build commands identical')
        need((evidence / 'generate-local.json').read_bytes() == legacy_receipt, 'legacy receipt not overwritten')
        receipt = json.loads((evidence / m.PLAYER_CPU_EVIDENCE / 'generate-local.json').read_text())
        need(receipt.get('cpu_mode') == 'player-inputs', 'player run receipt in its CPU evidence')
        error, _ = refused(m, ['--rom', str(pipe.rom), '--deps-dir', str(pipe.deps), '--evidence', str(evidence), '--player'])
        need(isinstance(error, RuntimeError) and 'occupied' in str(error), 'rerun into the same evidence root refused')

    print('generate_local_player_mode: ok (recorded commands and refusals; no generation executed)'
          + ''.join('; ' + note for note in notes))


if __name__ == '__main__':
    main()
