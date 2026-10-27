"""The GPU work counter (linux/tests/vulkan/stats_layer.c) as an explicit Vulkan layer."""
import json
import subprocess
import time

from .framework import Skip
from .player import ROOT

SOURCE = ROOT / 'linux/tests/vulkan/stats_layer.c'
HEADERS = ROOT / 'deps/rt64/src/contrib/plume/contrib/Vulkan-Headers/include'
BUILD = ROOT / 'local/tests/build/vulkan'
NAME = 'VK_LAYER_TOOIE_stats'
COLUMNS = ('presents', 'submits', 'renderpasses', 'rp_mpix', 'draws', 'kverts', 'draw_mpix', 'dispatches',
           'kgroups', 'copies', 'copy_mpix', 'resolves', 'resolve_mpix', 'copybuf_mb', 'barriers')


def layer_dir():
    library = BUILD / 'libstats_layer.so'
    if not library.exists() or library.stat().st_mtime < SOURCE.stat().st_mtime:
        if not HEADERS.exists():
            raise Skip('Vulkan headers missing at %s (deps not bootstrapped)' % HEADERS)
        BUILD.mkdir(parents=True, exist_ok=True)
        result = subprocess.run(['cc', '-O2', '-shared', '-fPIC', '-I', str(HEADERS), '-o', str(library), str(SOURCE)],
                                capture_output=True, text=True)
        if result.returncode:
            raise Skip('stats layer did not build: ' + result.stderr[-500:])
    manifest = {'file_format_version': '1.1.2', 'layer': {
        'name': NAME, 'type': 'GLOBAL', 'library_path': str(library), 'api_version': '1.3.0',
        'implementation_version': '1', 'description': 'Tooie test suite GPU work counter',
        'functions': {'vkGetInstanceProcAddr': 'stats_layer_GetInstanceProcAddr',
                      'vkGetDeviceProcAddr': 'stats_layer_GetDeviceProcAddr'}}}
    (BUILD / 'stats_layer.json').write_text(json.dumps(manifest) + '\n')
    return BUILD


def env(output):
    """Environment for a Player that records per-second GPU work into `output`."""
    return {'VK_ADD_LAYER_PATH': str(layer_dir()), 'VK_INSTANCE_LAYERS': NAME, 'STATS_LAYER_OUT': str(output)}


def rows(output, since=0.0, until=None):
    out = []
    try:
        lines = output.read_text().splitlines()
    except FileNotFoundError:
        return out
    for line in lines:
        values = line.split()
        stamp = float(values[0])
        if stamp >= since and (until is None or stamp <= until):
            out.append(dict(zip(COLUMNS, map(float, values[1:]))))
    return out


def measure(output, seconds):
    """Mean of each column over the next `seconds`."""
    start = time.time()
    time.sleep(seconds + 1.2)
    window = rows(output, start + 0.5, time.time())
    if not window:
        raise Skip('the stats layer recorded nothing (layer not loaded?)')
    return {k: sum(r[k] for r in window) / len(window) for k in COLUMNS}
