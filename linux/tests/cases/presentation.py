"""Video settings, aspect and interpolation; autosave recovery at startup."""
import re
import time

import numpy as np

from harness import flows, guest, vulkan
from harness.framework import test, check, Failure
from harness.player import Player, SAVE
from cases.gameplay import DECK, still


@test('quick', 'Picture, anti-aliasing and auto resolution settings reach RT64; intro at Original rate', parts=(
    ('picture_options', 'Picture options (2D upscaling, texture LOD, dither noise) applied'),
    ('anti_aliasing', 'MSAA applied live (within the device\'s supported levels)'),
    ('menu_rate', 'Startup intro presented at Original rate, game select at the chosen rate'),
))
def video_settings(ctx):
    settings = {'baseline_upscale_2d': 2, 'baseline_texture_lod': False, 'baseline_dither_noise': 50,
                'baseline_msaa': 4, 'baseline_resolution': 0}
    with Player(ctx, 'run', settings=settings) as p:
        flows.to_game_select(p)
        log = p.log()
    with ctx.part('picture_options', 'Picture options'):
        check(re.search(r'^baseline picture config-applied upscale_2d=2 texture_lod=0 dither_noise=50$', log, re.M),
              'picture settings not applied:\n' + '\n'.join(l for l in log.splitlines() if 'picture' in l))
    with ctx.part('anti_aliasing', 'Anti-aliasing'):
        line = re.search(r'^baseline msaa config-applied requested=(\d+) supported_max=(\d+)$', log, re.M)
        check(line, 'no MSAA line')
        requested, supported = map(int, line.groups())
        ctx.note('MSAA %d (device max %d)' % (requested, supported))
        check(requested == min(4, supported), 'MSAA %d applied, wanted 4 (device max %d)' % (requested, supported))
    with ctx.part('menu_rate', 'Interpolation at Display output: intro rate'):
        check(re.search(r'^baseline map=0x18B menu_rate_original=1$', log, re.M), 'intro not forced to Original rate')
        check(re.search(r'^baseline map=0x158 menu_rate_original=0$', log, re.M), 'game select not at the chosen rate')


@test('quick', 'Auto resolution: RT64 WindowIntegerScale follows the window size')
def auto_resolution(ctx):
    stats = ctx.run_dir / 'gpu.stats'
    with Player(ctx, 'run', settings={'baseline_resolution': 0}, env=vulkan.env(stats)) as p:
        flows.to_game_select(p)
        p.resize(1280, 800)
        large = vulkan.measure(stats, 3)
        p.resize(480, 300)
        time.sleep(1.0)
        small = vulkan.measure(stats, 3)
    # Render-pass pixels per present, 1280x800 against 480x300 (3x against 1x internally). Measured
    # 2026-10-27: 4.6 with auto resolution, 1.9 with a fixed 3x (only the output pass shrinks).
    ratio = (large['rp_mpix'] / large['presents']) / (small['rp_mpix'] / small['presents'])
    ctx.note('render-pass pixels 1280x800 / 480x300 = %.1f' % ratio)
    check(ratio > 3.2, 'internal resolution did not follow the window (ratio %.1f; a fixed scale gives 1.9)' % ratio)


@test('quick', 'Autosave: a truncated save is replaced by the autosave backup at startup',
      trigger='the runtime copies over the save in place, so a crash there can truncate it')
def autosave_recovery(ctx):
    whole = bytes(range(256)) * 8
    files = {SAVE: b'\0' * 100, SAVE + '.autosave.bak': whole}
    with Player(ctx, 'run', profile_files=files) as p:
        p.wait_log(r'^player autosave: .* was 100 bytes; restored the autosave backup$', 30)
    check((p.profile / SAVE).read_bytes() == whole, 'save not restored from the backup')


@test('quick', '16:10 via Fit window: gameplay fills a 1280x800 window (no pillarbox)')
def fit_window(ctx):
    with Player(ctx, 'run') as p:
        flows.new_game(p)
        p.resize(*DECK)
        still(p, 2.0)
        image = p.screenshot('gameplay')
    h, w = image.shape[:2]
    edges = [float(image[h // 8:h // 2, x0:x1].mean()) for x0, x1 in ((0, w // 50), (w - w // 50, w))]
    ctx.note('%dx%d, edge brightness %.0f / %.0f' % (w, h, *edges))
    check((w, h) == DECK, 'window is %dx%d' % (w, h))
    check(min(edges) > 12, 'black bars at the window edges (brightness %.0f / %.0f)' % tuple(edges))


@test('full', 'Interpolation at Display output: game select actors have matrix groups (frame_motion)',
      trigger='without the groups, game select jittered: max 2.59 / p95 0.92 (with: 0.478 / 0.392)')
def game_select_motion(ctx):
    with Player(ctx, 'run') as p:
        flows.to_game_select(p)
        time.sleep(2.0)
        path, width, height = p.record(4, 'game-select')
    data = np.fromfile(path, dtype=np.uint8)
    count = data.size // (width * height)
    frames = data[:count * width * height].reshape(count, height, width).astype(np.int16)
    # linux/frame_motion.py's measure: the upper middle of the image.
    region = frames[:, height // 8:height // 2, width // 6:width * 5 // 6]
    diff = np.abs(np.diff(region, axis=0)).mean(axis=(1, 2))
    moving = diff[diff > 0.05]
    check(len(moving) > count // 4, 'game select barely moved (%d of %d frames)' % (len(moving), count))
    ctx.note('frames %d, max %.3f, p95 %.3f' % (count, moving.max(), np.percentile(moving, 95)))
    check(moving.max() < 1.2, 'frame-to-frame spikes up to %.2f (jitter)' % moving.max())
