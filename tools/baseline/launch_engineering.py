"""Checked engineering launch; retained manual-player launchers remain separate."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

from verify_guest_binding import check


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--tools', type=Path, required=True)
    parser.add_argument('--check-only', action='store_true')
    parser.add_argument('native_arguments', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    build = args.build.resolve()
    snapshot = build / 'baseline/raw-qualification/qualified.json'
    qualified = json.loads(snapshot.read_text())
    raw = Path(qualified['raw_root'])
    candidate = build / 'baseline/renderer-candidate-bin'
    exe, mapping = candidate / 'TooieBaseline.exe', candidate / 'TooieBaseline.map'
    command = [sys.executable, str(Path(__file__).with_name('verify_baseline_binding.py')), 'prelaunch',
               '--build', str(build), '--raw', str(raw), '--archive', str(build / 'BaselineRecompiledFuncs.lib'),
               '--tools', str(args.tools.resolve()), '--snapshot', str(snapshot),
               '--expected-count', str(len(qualified['commands'])), '--exe', str(exe), '--map', str(mapping),
               '--receipt', str(snapshot.with_name('native.json'))]
    gate = subprocess.run(command, capture_output=True, text=True, creationflags=subprocess.CREATE_NO_WINDOW)
    if gate.returncode:
        sys.stdout.write(gate.stdout)
        sys.stderr.write(gate.stderr)
        gate.check_returncode()
    print(json.dumps(check(exe, mapping, raw.parent / 'boot.bin', raw.parent / 'render_bridge_layout.h')))
    native = args.native_arguments
    if native[:1] == ['--']:
        native = native[1:]
    if args.check_only:
        return
    if '--manual' in native or native.count('--profile') != 1 or native.count('--seconds') != 1:
        raise ValueError('Engineering acquisition requires one explicit bounded disposable profile')
    for name in ('--profile', '--rom', '--input-script'):
        if name in native:
            if native.count(name) != 1:
                raise ValueError('Duplicate native path option')
            index = native.index(name) + 1
            native[index] = str(Path(native[index]).resolve())
    profile = Path(native[native.index('--profile') + 1])
    protected = (build / 'baseline/profiles/manual-play').resolve()
    if not profile.is_relative_to(build / 'baseline/profiles') or profile.is_relative_to(protected):
        raise ValueError('Protected or outside engineering profile')
    # Native main owns the existing marker validation, exclusive lock and reuse guard.
    env = dict(os.environ, SDL_WINDOW_NO_ACTIVATION_WHEN_SHOWN='1')
    raise SystemExit(subprocess.run([str(exe)] + native, env=env, stdout=sys.stdout, stderr=sys.stderr,
                                   stdin=subprocess.DEVNULL, creationflags=subprocess.CREATE_NO_WINDOW).returncode)


if __name__ == '__main__':
    main()
