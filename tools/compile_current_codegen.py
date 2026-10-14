"""Replay prepared codegen after hook-only edits, preserving verified ancestry."""
from pathlib import Path
import argparse
import json
import subprocess
try:
    import tomllib
except ModuleNotFoundError:
    import tomli as tomllib
from instrument_continuous import instrument
from instrument_audio_pacing import extract_audio_pacing_fixture
from codegen_io import capture_output_times, restore_unchanged_times
from codegen_provenance import validate, write_receipt, sha, generated_directory, configuration_directory
from prepare_heap_patch import prepare_heap_patch
from prepare_codegen import prepare_lift_bindings


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--generated-dir', type=Path)
    args = parser.parse_args()
    app = Path(__file__).resolve().parents[1]
    generated = generated_directory(app, args.generated_dir)
    config_dir = configuration_directory(app, generated)
    # No stamp-only migration: missing preparation ancestry requires full codegen.
    parent = validate(app, replay=True, generated_dir=generated)
    input_hashes = {app / name: sha(app / name) for name in parent['inputs']}
    command = parent['evidence']['command']
    if len(command) != 2:
        raise ValueError('Invalid ancestor generation command')
    config_path = Path(command[1]).resolve()
    if config_path != (config_dir / 'tooie.us.toml').resolve():
        raise ValueError('Replay ancestor configuration does not belong to the selected output')
    config = tomllib.loads(config_path.read_text())
    if (config_dir / config['input']['output_func_path']).resolve() != generated:
        raise ValueError('Replay configuration output differs from the selected generated directory')
    previous = capture_output_times(app, generated, config_dir)
    evidence = args.evidence.resolve()
    evidence.mkdir(parents=True, exist_ok=True)
    sections = tomllib.loads((config_dir / 'selected.syms.toml').read_text())['section']
    ancestor_hash = sha(generated / 'codegen-receipt.json')
    (generated / 'codegen-receipt.json').unlink()
    (generated / 'heap_patch.c').unlink(missing_ok=True)
    for source in generated.glob('funcs_*.c'):
        source.unlink()
    with (evidence / 'codegen.log').open('w') as log:
        subprocess.run(command, cwd=config_dir, stdout=log,
                       stderr=subprocess.STDOUT, check=True)
    sources = sorted(generated.glob('*.c'))
    (generated / 'test-fixtures/audio_pacing_functions.inc').write_text(extract_audio_pacing_fixture(sources))
    instrumentation = instrument(generated, sections, guest_native_aspect=True)
    elf_path = app / 'deps/banjo-tooie/build/us/banjotooie_decompressed.elf'
    patch_inputs = prepare_heap_patch(app, Path(command[0]), sections, elf_path, evidence,
                                      generated_dir=generated)
    lift_binding = prepare_lift_bindings(app, generated, evidence)
    receipt = {'generation': 'replay', 'command': command,
               'ancestor_receipt_sha256': ancestor_hash,
               'configuration_dir': str(config_dir.resolve()),
               'guest_native_aspect': True,
               'lift_binding': lift_binding,
               'instrumentation': instrumentation,
               'byte_identical_timestamps_preserved': restore_unchanged_times(previous)}
    if any(sha(path) != digest for path, digest in input_hashes.items()):
        raise ValueError('Codegen inputs changed during replay; rerun after edits freeze')
    write_receipt(app, [app / name for name in parent['inputs']] + patch_inputs, receipt,
                  generated_dir=generated)
    validate(app, generated_dir=generated)
    (evidence / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')


if __name__ == '__main__':
    main()
