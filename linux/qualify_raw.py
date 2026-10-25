#!/usr/bin/env python3
"""Linux counterpart of docs/WINDOWS.md step 5: compile the raw game-code archive,
collect its evidence and qualify it with tools/baseline/verify_baseline_binding.py.

Run from linux/build.sh after CMake has configured BUILD (with compile commands
exported) and only when BUILD has no raw qualification yet; the gate refuses to
replace one. The raw objects are removed first so the compile log shows every
raw source compiled exactly once, as the gate requires."""
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1]).resolve()
raw = (build / 'baseline/render-bridge-camera-candidate/composition-v2/raw').resolve()
archive = build / 'libBaselineRecompiledFuncs.a'
inputs = build / 'raw-qualification-input'
snapshot = build / 'baseline/raw-qualification/qualified.json'
sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()

if snapshot.exists():
    sys.exit(f'{snapshot} exists; move the build aside to qualify again')
shutil.rmtree(build / 'CMakeFiles/BaselineRecompiledFuncs.dir', ignore_errors=True)
archive.unlink(missing_ok=True)
shutil.rmtree(inputs, ignore_errors=True)
inputs.mkdir()
with (inputs / 'compile.log').open('w') as log:
    subprocess.run(['cmake', '--build', str(build), '--target', 'BaselineRecompiledFuncs'],
                   stdout=log, stderr=subprocess.STDOUT, check=True)

count = len(list(raw.glob('funcs_*.c')))
rows = [row for row in json.loads((build / 'compile_commands.json').read_text())
        if Path(row['directory'], row['file']).resolve().parent == raw]
assert len(rows) == count, f'{len(rows)} raw compile commands for {count} sources'
objects = {str(Path(row['directory'], row.get('output') or re.search(r'-o (\S+)', row['command'])[1]).resolve())
           for row in rows}

files = {Path(row['directory'], row['file']).resolve() for row in rows} | {raw.parent / 'RAW_BINDING.json'}
current, valid = False, 0
for line in subprocess.check_output(['ninja', '-C', str(build), '-t', 'deps'], text=True).splitlines():
    if line and not line[0].isspace():
        current = str((build / line.rsplit(': #deps', 1)[0]).resolve()) in objects
        if current:
            assert '(VALID)' in line, line
            valid += 1
    elif current and line.strip():
        path = Path(line.strip())
        files.add((path if path.is_absolute() else build / path).resolve())
assert valid == count, f'{valid} valid dependency records for {count} objects'

record = {'commands': rows, 'files': [{'path': str(p), 'sha256': sha(p)} for p in sorted(files)]}
(inputs / 'evidence.json').write_text(json.dumps(record, indent=2) + '\n')
subprocess.run([sys.executable, str(ROOT / 'tools/baseline/verify_baseline_binding.py'), 'qualify', '--format', 'elf',
                '--build', str(build), '--raw', str(raw), '--archive', str(archive), '--tools', '/usr/bin',
                '--snapshot', str(snapshot), '--expected-count', str(count),
                '--evidence', str(inputs / 'evidence.json'), '--compile-log', str(inputs / 'compile.log'),
                '--accepted-archive-sha', sha(archive)], check=True)
