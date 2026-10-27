"""Checks on the tree and the build output; no game run."""
import hashlib
import json
import re
import subprocess
import sys

from harness.framework import test, check, Skip
from harness.player import ROOT

BUILD = ROOT / 'build/linux-player'
GUEST = BUILD / 'baseline/render-bridge-camera-candidate/composition-v2'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


@test('static', 'Repository check (AGENTS.md "Checks")')
def repository_check(ctx):
    out = subprocess.run([sys.executable, 'tools/check_repository.py'], cwd=ROOT, capture_output=True, text=True)
    check(out.returncode == 0, out.stdout + out.stderr)


@test('static', 'Guest image is upstream\'s pinned image (no guest patches in this fork)')
def guest_image_pin(ctx):
    record = GUEST / 'PLAYER_GUEST.json'
    if not record.exists():
        raise Skip('no guest build at %s' % record.relative_to(ROOT))
    source = (ROOT / 'tools/baseline/prepare_player_guest.py').read_text()
    pinned = re.search(r"^IMAGE_SHA256 = '([0-9a-f]{64})'", source, re.M).group(1)
    image = json.loads(record.read_text())['image_sha256']
    check(image == pinned == sha(GUEST / 'boot.bin'), 'guest image %s, pin %s' % (image, pinned))
    ctx.note('image %s...' % image[:8])


@test('static', 'Every fork raw hook is in the generated game C at its address')
def raw_hooks_landed(ctx):
    raw = GUEST / 'raw'
    if not raw.exists():
        raise Skip('no raw corpus at %s' % raw.relative_to(ROOT))
    sys.path.insert(0, str(ROOT / 'tools/baseline'))
    import minimum_host_hooks as hooks
    text = '\n'.join(p.read_text() for p in raw.glob('*.c'))

    def body(func):
        start = text.find('RECOMP_FUNC void %s(uint8_t* rdram, recomp_context* ctx) {\n' % func)
        if start < 0:
            return ''
        end = text.find('\nRECOMP_FUNC ', start + 1)
        return text[start:end if end > 0 else len(text)]
    missing = []
    selected = hooks.HOOKS + hooks.FRUSTUM_HOOKS + hooks.GRAPHICS_HOOKS
    for func, pc, call in selected:
        anchor = '    %s\n    // 0x%08X:' % (call, pc)
        if anchor not in body(func):
            missing.append('%s @ 0x%08X' % (func, pc))
    check(not missing, 'hooks not found: ' + ', '.join(missing))
    ctx.note('%d hooks' % len(selected))


@test('static', 'Linux RT64 recipes are built byte for byte (scene IDs + matching speed, tile copy edges)')
def rt64_recipes_built(ctx):
    generic = BUILD / 'baseline/rt64_generic'
    if not generic.exists():
        raise Skip('no RT64 generic outputs at %s' % generic.relative_to(ROOT))
    for name in ('owner_index_guard_scene_ids', 'tile_copy_edges', 'transform_upload_guard',
                 'rotation_interpolation', 'pop_matrix_group_projection', 'projection_residual'):
        recipe = json.loads((ROOT / 'patches/rt64' / (name + '.delta.json')).read_text())
        output = generic / recipe['output']['filename']
        check(output.exists(), '%s: %s not built' % (name, output.name))
        check(sha(output) == recipe['output']['sha256'], '%s: %s differs from the recipe output' % (name, output.name))


@test('static', 'Name: Banjo-Tooie: Steam Deck Port (executable, window title)')
def player_name(ctx):
    check(ctx.binary.name == 'BanjoTooieSteamDeckPort', 'executable is %s' % ctx.binary.name)
    check(b'Banjo-Tooie: Steam Deck Port' in ctx.binary.read_bytes(), 'product name missing from the binary')
