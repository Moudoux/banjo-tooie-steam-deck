"""Generate the selected baseline raw corpus from the maintained raw condition.

Maintained successor of the S0-G generator: one N64Recomp run over the canonical
selected symbols/overlay ids and the validated decompressed ROM, with the reviewed
baseline patch set (ignored providers, hooks, instruction edits). Run under the
existing WSL codegen environment after tools/prepare_codegen.py.
"""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

import toml

CONDITION = 'tools/baseline/baseline_raw_condition.toml'
CONFIG = 'build/windows-dev/baseline/raw-condition.toml'
RAW = 'build/windows-dev/baseline/raw'
GENERATOR = 'deps/N64Recomp-codegen/build/N64Recomp'
PATHS = {'symbols_file_path': 'config/selected.syms.toml',
         'rom_file_path': 'deps/banjo-tooie/decompressed.us.z64',
         'relocatable_sections_path': 'config/overlay-ids.txt'}
PINS = {GENERATOR: '34958d6ae8c047adb772090c75fdcc051036646a1db58595694f814c3a4a09b6',
        'config/selected.syms.toml': '37998003f59c74676667c898ef960641f195bfdefb4b47181665e75a8aada395',
        'deps/banjo-tooie/decompressed.us.z64': '8c9d316b2edca686ec8393ddf95d480a7dfd9879ef434dbcfc4ea3000f2ffe89'}
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def condition(root):
    """The maintained path-free condition bound to this checkout's inputs and raw output."""
    config = toml.loads((root / CONDITION).read_text())
    if set(config) != {'input', 'patches'}:
        raise ValueError('raw condition holds only generator settings and patches')
    if set(config['input']) != {'entrypoint', 'functions_per_output_file', 'recomp_include'}:
        raise ValueError('raw condition paths are bound per checkout')
    config['input'].update({key: str(root / name) for key, name in PATHS.items()})
    config['input']['output_func_path'] = str(root / RAW)
    return config


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path.cwd())
    root = parser.parse_args().root.resolve()
    raw, config_path = root / RAW, root / CONFIG
    if raw.exists() and any(raw.iterdir()):
        raise ValueError('raw generation requires a fresh output; preserve prior corpora')
    for name, digest in PINS.items():
        if sha(root / name) != digest:
            raise ValueError(f'pinned raw input changed: {name}')
    inputs = [root / CONDITION, root / PATHS['relocatable_sections_path'], Path(__file__).resolve()]
    inputs += [root / name for name in PINS]
    initial = {path: sha(path) for path in inputs}
    config = condition(root)
    raw.mkdir(parents=True, exist_ok=True)
    config_path.write_text(toml.dumps(config))
    if toml.loads(config_path.read_text()) != config:
        raise ValueError('raw condition does not round-trip')
    command = [str(root / GENERATOR), str(config_path)]
    with (config_path.parent / 'raw-codegen.log').open('w') as log:
        subprocess.run(command, cwd=config_path.parent, stdout=log, stderr=subprocess.STDOUT, check=True)
    if any(sha(path) != digest for path, digest in initial.items()):
        raise ValueError('raw inputs changed during generation')
    if not all((raw / name).is_file() for name in ('funcs.h', 'recomp_overlays.inl')) or not any(raw.glob('funcs_*.c')):
        raise ValueError('raw registry/guest corpus missing')
    receipt = dict(condition=CONDITION, config_sha256=sha(config_path), command=command,
                   inputs={str(path): digest for path, digest in initial.items()},
                   outputs={path.name: sha(path) for path in sorted(raw.iterdir()) if path.is_file()})
    (config_path.parent / 'raw-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'raw_files': len(receipt['outputs'])}))


if __name__ == '__main__':
    main()
