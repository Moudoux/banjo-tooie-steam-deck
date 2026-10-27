#!/usr/bin/env python3
"""G0 focused test: prepare_silo_bridge.py --bridge-object compiles only the Honey comparison object.

Loads tools/baseline/prepare_silo_bridge.py with the compiler, path conversion and ELF reader
replaced by recorders, so no toolchain runs and nothing is written outside a temporary folder.
It proves owner selection and option handling; it does not prove that any real object passes
its gates. Needs toml and pyelftools (the existing .venv-codegen).

    .venv-codegen/bin/python -I tests/silo_bridge_object_mode_test.py <repo>
"""
import argparse
import contextlib
import importlib.util
import io
import json
import re
import sys
import tempfile
import types
from pathlib import Path

OWNERS = ('bridge', 'bone', 'pair', 'silo', 'player', 'actor', 'leaf', 'lifecycle')


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


class Section:
    def data(self):
        return b''


class Elf:
    elfclass = 32
    little_endian = False

    def __init__(self, owner):
        self.owner = owner

    def __getitem__(self, key):
        return {'e_machine': 'EM_MIPS'}[key]

    def iter_sections(self):
        return iter(())

    def get_section_by_name(self, name):
        return Section()

    def get_section(self, index):
        return Section()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('repo', type=Path)
    repo = parser.parse_args().repo.resolve()
    sys.dont_write_bytecode = True  # no __pycache__ in the checkout
    sys.path.insert(0, str(repo / 'tools/baseline'))
    path = repo / 'tools/baseline/prepare_silo_bridge.py'
    spec = importlib.util.spec_from_file_location('prepare_silo_bridge_under_test', path)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)

    # 1. Source facts: the historical OFF bone gates are unchanged.
    source = path.read_text(encoding='utf-8')
    need("if name == 'bone' and not npc665 and not backpack621:" in source, 'bone gate condition unchanged')
    need("assert sha(obj)=='7356a8f35224811c6fc8f5b730988fac05a003ca1d94c852dd174b0ef4dcf8a3', 'measured bone object changed'"
         in source, 'bone object pin unchanged')
    need("'eb5c7f47930974014df369080ef90cd7b731aa9d06e9724d520ff0c3fa216fd9', 'retained Silo helper body changed'"
         in source, 'helper body pin unchanged')
    compiler_pin = re.search(r"assert sha\(compiler\) == '([0-9a-f]{64})'", source)[1]
    generator_pin = re.search(r"build/N64Recomp'\)=='([0-9a-f]{64})'", source)[1]
    definitions = {'bridge': m.accepted.BRIDGE_FUNCTIONS, 'bone': ['func_800DCF48', m.HELPER]}

    def symbols(elf):
        return {n: {'st_info': {'type': 'STT_FUNC', 'bind': 'STB_GLOBAL'}, 'st_shndx': 1, 'st_value': 0, 'st_size': 0}
                for n in definitions.get(elf.owner, [])}

    def fake_sha(target):
        name = Path(target).name
        return compiler_pin if name == 'clang.exe' else generator_pin if name == 'N64Recomp' else 'same'

    def run_main(tmp, *flags):
        output = tmp / 'build/windows-dev/baseline/render-bridge-silo-candidate'
        output.mkdir(parents=True)
        for owner in OWNERS:
            (output / (owner + '.o')).write_bytes(b'')
        compiled, stderr, error = [], io.StringIO(), None

        def recorder(command, **kwargs):
            compiled.append(Path(command[command.index('-c') + 1]).name)

        saved_argv, sys.argv = sys.argv, ['prepare_silo_bridge.py', '--root', str(tmp), *flags]
        try:
            with patched(m, sha=fake_sha, win=str, ELFFile=lambda stream: Elf(Path(stream.name).stem),
                         symbols=symbols, relocations=lambda elf: [],
                         subprocess=types.SimpleNamespace(run=recorder, STDOUT=-2)), \
                    contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(stderr):
                try:
                    m.main()
                except (AssertionError, SystemExit) as caught:
                    error = caught
        finally:
            sys.argv = saved_argv
        return compiled, error, stderr.getvalue(), output

    with tempfile.TemporaryDirectory() as name:
        tmp = Path(name).resolve()

        # 2. --bridge-object compiles render_bridge.c alone and binds only that object.
        compiled, error, _, output = run_main(tmp / 'bridge', '--bridge-object', '--no-prior-honey')
        need(error is None and compiled == ['render_bridge.c'], 'bridge-only compile: %s %r' % (compiled, error))
        need(list(json.loads((output / 'OBJECT_BINDING.json').read_text())) == ['bridge'], 'binding names bridge only')

        # 3. Full --objects is unchanged: it still compiles bone after bridge and gates it.
        compiled, error, _, _ = run_main(tmp / 'objects', '--objects', '--no-prior-honey')
        need(isinstance(error, AssertionError) and compiled[:2] == ['render_bridge.c', 'bone_handler.c'],
             'full objects still reach the bone gates: %s %r' % (compiled, error))

        # 4. --bridge-object is an alternative to --objects and --image, refused before any work.
        for other in ('--objects', '--image'):
            compiled, error, text, output = run_main(tmp / ('both' + other), '--bridge-object', other)
            need(isinstance(error, SystemExit) and error.code == 2 and '--bridge-object' in text and not compiled
                 and not (output / 'OBJECT_BINDING.json').exists(), '--bridge-object with ' + other + ' refused')

        # 5. The library flag refuses the shared/actor/lifecycle selections.
        for selection in ({'shared': True}, {'cpu_root': True}, {'lifecycle_only': True}):
            try:
                m.compile_objects(tmp, tmp, None, bridge_only=True, **selection)
            except AssertionError:
                continue
            need(False, 'bridge_only with %s refused' % selection)

    print('silo_bridge_object_mode: ok (recorded compiles; no toolchain or real object gate executed)')


if __name__ == '__main__':
    main()
