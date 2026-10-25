#!/usr/bin/env python3
"""Fixed baseline runtime source composition; the F6 player guest-clock timer/VI children are the only clock recipe."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys

PIN = 'ca568b6ad79b9029d14077f0c3ffa757727c5559'
INPUTS = {
    'ultramodern/src/events.cpp': '474621d28868daa3ea82ce137a043b560472dd45f4942d361f02935091d4aa6c',
    'ultramodern/src/threads.cpp': 'e3ba32a1e06ba1643adc10ccc29ef5e3429e8809270f787a181314fc5704e653',
    'ultramodern/src/threadqueue.cpp': 'b9bb55edeb9cb99a153d00d80a13c5a09140113403499af7656fd9489d38f305',
    'ultramodern/src/mesgqueue.cpp': '720db0e01ed2d874710b180292bd3ee9480c742a99bc574c4ce87e8f42949d21',
    'librecomp/src/dp.cpp': '21d8601c2207137b296d4211f8fd5d45ae2284d41913298d66916580c74d2094',
    'librecomp/src/recomp.cpp': 'af9c6f983fb1620ff5b971836d7792ef0b83a47e77010ab89ddf2fa1fb87e143',
    'librecomp/src/pi.cpp': '30189d04fe830f26ee69ec0f3aae8a4e37ffedc74cfee3294d5ac135038f90ca',
    'ultramodern/src/timer.cpp': '5919412bafcde237d911d1110059c39863b2a09baedba782d366d4804a32f72d',
}
# Only these named maintained patches are accepted; this is not a patch CLI.
PATCHES = {
    'events-q-vi-lock': ('a2cee8eafb01f4e6a5779174e6d9faa2e471fa855f621552c05c73f53b05321b', 'events.cpp', 3),
    'timer-guest-clock': ('3f1db54412c4df52048c0607c47ac49a725629a1b55e156bceceaa06e4641cbb', 'timer.cpp', 8),
    'events-guest-clock': ('594a6c424ac803ed54feb31c4c53e7fc5db8d4db97434324a8a552874fb4bd22', 'events.cpp', 3),
    'events-sp-exception': ('73511703d287ce3fcf76eecafe16d97c8806a0dc448dca7f86529627dde2614f', 'events.cpp', 2),
    'events-next-framebuffer': ('6e17ed72a30d942af19ed4242c9e8a3f8f947bad9d709cc264498b1084b5ab27', 'events.cpp', 1),
    'events-dp-freeze': ('7f287aec255a5dcb9eaa2ae53082403ff9a4eaa67c22b0f56d65e4f5d5864cdf', 'events.cpp', 3),
    'threads-suspended-stop': ('9438dd3c67452b673f4e5bfe494b2c54fa88574e6a11c73f0758a7df64401322', 'ultramodern/src/threads.cpp', 2),
    'producer-quiescent-cleanup': ('9b4261af055d9677c061065c1d7588e79fcec74f679dac6cf12d3c069e2319fb', 'ultramodern/src/threads.cpp', 1),
    'threadqueue-removal': ('713659775b22eeac5c1e6f23322af35f68d66eec580819e6c0e41ee5a67b56da', 'ultramodern/src/threadqueue.cpp', 2),
    'dp-freeze': ('4006e77ef9591ac7584e84ad348eb237b09799b56962b0ed70d45100d53cfa23', 'dp.cpp', 2),
    'recomp-terminal-fence': ('eb157ab9876972d73b610edc5280cfd6475930591d119b3eb3877429cd12aae2', 'recomp.cpp', 4),
    'mesgqueue-required-admission': ('912b8c1c6cd7e658ab63bcfbb6d4320c536c9104008e5512633abdd0919bf644', 'mesgqueue.cpp', 4),
}
EXPECTED = {
    'baseline/guest-clock/events.cpp': '720195ce6c8d2a77dc6c6b256e4ec10f0f6d744337d3dfe8fbb1221ce100298c',
    'baseline/guest-clock/timer.cpp': '910d020a22203cc6d0c91f2c7aed0c0856835ab8bed2c6dffd948e35483de37c',
    'baseline/lifecycle/events.cpp': 'c680d7ba54a66010eabaad23dfa26d1c7dea35030ed0ae75cec4e5306470e9b0',
    'baseline/scheduler-event/events.cpp': '7ad9ad3def0f061125db69ed608078558141aecfa1ed715588b807fdcdf5dee2',
    'baseline/first-frame-sync/events.cpp': 'aed57c028028a756d507a4c4adfb915171fac167cf543d40447c935e47d3bf88',
    'baseline/dp-freeze/events.cpp': '96f362c7a774c4629a2e9523ecea748a1865bc9cb54de177207c486a6e7ccf4c',
    'baseline/thread-refusals/threads.cpp': '64ddcfdd413bd1a893ab00bae1b7b2ad08f251af446687abd605fa0ae09dabde',
    'baseline/lifecycle/threads.cpp': 'a1227291c933fa003706575e407ad8b0a469f04fe44125b7de03ad16faff8469',
    'baseline/controller-cycle/threadqueue.cpp': '67f48b19bc92f2cd8abba882b53209d5bbba1d297c0ad0c159108eb4faa8e621',
    'baseline/lifecycle/mesgqueue.cpp': '0889f02f33c2da072eebdccaccc81bcdeba09993eb7ea3e020200feaeabfcf8d',
    'baseline/dp-freeze/dp.cpp': 'ac866e51f0cb90c07ee330bfd7579ef4ead8c9c30bfc0cb285d09e515c3df4e4',
    'baseline/lifecycle/recomp.cpp': '53f3f6480924483fcc3e19a7f90632abf2e6b3997bb6b85c6614ad66501bcc89',
    'runtime-continuous/pi.cpp': 'e44d154c455cbd1d4d9a1cf61dc9d4c2ca74274b1515f6c0f23e735ce7a868ba',
}
RECIPES = {
    'owned_timer.py': '16566e4fdf843d5094eab27e05e72a98e5d4a06f9d9971a149bd40a40b3a6545',
    'prepare_runtime_continuous.py': '404e24fe8646359715145f6ba81ab7d71e61317db80918a273f30ecffb80f7fb',
    'prepare_runtime_threadqueue.py': 'ffcdb4d858342572348608f7fd2114f729637b4686d3c14d3e3a6171174f7143',
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def load_recipe(path):
    # Import guarded function definitions only, never either existing writer.
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location(path.stem, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def preflight(root, build, runtime, inputs, destinations):
    allowed = (root / 'build').resolve()
    require(allowed.is_relative_to(root) and build.is_relative_to(allowed), 'Output must stay in this repository build workspace')
    protected = [runtime, *(root / n for n in ('deps', 'tools', 'patches', 'config', 'cmake', 'src', 'generated', 'local', '.git'))]
    resolved = [p.resolve() for p in destinations]
    require(len(set(resolved)) == len(resolved), 'Duplicate resolved destinations')
    for destination, target in zip(destinations, resolved):
        require(target.is_relative_to(build), 'Output symlink escapes selected build directory')
        require(not any(target == p.resolve() or target.is_relative_to(p.resolve()) for p in protected), 'Protected output overlap')
        require(target not in inputs, 'Output aliases a source/patch/recipe input')
        require(not destination.exists() or destination.is_file(), 'Output is not a regular file')
        require(not destination.exists() or destination.stat().st_nlink == 1, 'Output has hardlink aliases')
        for source in inputs:
            require(not destination.exists() or not os.path.samefile(destination, source), 'Output hardlink aliases an input')
    for i, destination in enumerate(destinations):
        for other in destinations[:i]:
            require(not (destination.exists() and other.exists() and os.path.samefile(destination, other)), 'Output aliases another destination')


def materialize(root, runtime, build):
    root, runtime, build = root.resolve(), runtime.resolve(), build.resolve()
    require(subprocess.check_output(['git', '-C', str(runtime), 'rev-parse', 'HEAD'], text=True).strip() == PIN, 'Runtime revision drift')
    inputs = {}

    def read(path, expected):
        path = path.resolve()
        data = path.read_bytes()
        require(sha(data) == expected, 'Pinned input drift: ' + str(path))
        inputs[path] = data
        return data.decode('utf-8')

    originals = {name: read(runtime / name, expected) for name, expected in INPUTS.items()}
    patches = {name: read(root / 'patches/runtime' / (name + '.patch'), item[0]) for name, item in PATCHES.items()}
    for name, expected in RECIPES.items():
        read(root / 'tools' / name, expected)
    read(root / 'patches/runtime/dp-freeze.hpp', '32cad60bc4a471733db1be985ebbe0eb6ac3c0f44a382cadf056e5f8dce31639')
    recipe = Path(__file__).resolve()
    inputs[recipe] = recipe.read_bytes()
    continuous = load_recipe(root / 'tools/prepare_runtime_continuous.py')
    queue = load_recipe(root / 'tools/prepare_runtime_threadqueue.py')
    # messages()/saving() apply these; pinned in the recipe module, recorded here for the publication recheck.
    for name, item in continuous.RECIPE_PATCHES.items():
        read(root / 'patches/runtime' / (name + '.patch'), item[0])

    def patch(text, name):
        _, filename, expected_hunks = PATCHES[name]
        return continuous.apply_patch(text, patches[name], filename, expected_hunks, name)

    outputs = {}

    def emit(relative, text):
        data = text.encode('utf-8')
        require(sha(data) == EXPECTED[relative], 'Computed child drift: ' + relative + ' ' + sha(data))
        outputs[build / relative] = data

    events = patch(originals['ultramodern/src/events.cpp'], 'events-q-vi-lock')
    emit('baseline/scheduler-event/events.cpp', events)
    events = patch(events, 'events-next-framebuffer')
    emit('baseline/first-frame-sync/events.cpp', events)
    events = patch(events, 'events-dp-freeze')
    emit('baseline/dp-freeze/events.cpp', events)
    events = patch(events, 'events-sp-exception')
    emit('baseline/lifecycle/events.cpp', events)
    # F6 player arm: the consumed SP-exception child plus only the guest-clock VI hunks.
    emit('baseline/guest-clock/events.cpp', patch(events, 'events-guest-clock'))
    # F6 player child of the unchanged copied 1x timer (baseline_timer + ordinary).
    emit('baseline/guest-clock/timer.cpp',
        patch(continuous.baseline_timer(originals['ultramodern/src/timer.cpp']), 'timer-guest-clock'))
    threads = patch(originals['ultramodern/src/threads.cpp'], 'threads-suspended-stop')
    emit('baseline/thread-refusals/threads.cpp', threads)
    threads = patch(threads, 'producer-quiescent-cleanup')
    require(sha(threads.encode()) == continuous.BASELINE_COMPOSED_SHA, 'Cleanup composed parent drift')
    emit('baseline/lifecycle/threads.cpp', continuous.threads(threads, True))
    queue_text = queue.normalize_lf(originals['ultramodern/src/threadqueue.cpp'].encode()).decode()
    emit('baseline/controller-cycle/threadqueue.cpp', patch(queue_text, 'threadqueue-removal'))
    emit('baseline/lifecycle/mesgqueue.cpp',
        patch(continuous.messages(originals['ultramodern/src/mesgqueue.cpp']), 'mesgqueue-required-admission'))
    emit('baseline/dp-freeze/dp.cpp', patch(originals['librecomp/src/dp.cpp'], 'dp-freeze'))
    emit('baseline/lifecycle/recomp.cpp', patch(originals['librecomp/src/recomp.cpp'], 'recomp-terminal-fence'))
    emit('runtime-continuous/pi.cpp', continuous.saving(originals['librecomp/src/pi.cpp']))
    require(len(outputs) == len(EXPECTED), 'Incomplete fixed output set')
    manifest = dict(scope='Fixed selected baseline runtime sources; timer separate; F6 player guest-clock timer/VI children; no historical raw acquisition', runtime_pin=PIN,
        inputs=[dict(path=str(p), sha256=sha(data)) for p, data in sorted(inputs.items())],
        outputs=[dict(path=str(p), sha256=sha(data)) for p, data in sorted(outputs.items())])
    outputs[build / 'baseline/selected-runtime/manifest.json'] = (json.dumps(manifest, indent=2, sort_keys=True) + '\n').encode()
    # All children, manifest and aliases have been computed/authenticated before
    # any mkdir/write. Recheck captured inputs immediately before publication.
    preflight(root, build, runtime, inputs, list(outputs))
    require(all(p.read_bytes() == data for p, data in inputs.items()), 'Inputs changed during computation')
    for destination, data in outputs.items():
        if not destination.exists() or destination.read_bytes() != data:
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
