"""Features and fixes that need the owner's save (worlds, warp pads) or long runs: full tier."""
import math
import time

import numpy as np

from harness import evidence, flows, guest, vulkan
from harness.framework import test, check, Failure, Skip
from harness.player import Player
from cases.gameplay import DECK, still

SAVE = 'owner-save.bin'
WITCHYWORLD = 0xD6
# The owner's Deck settings from the Jinjo Village investigation (local/deck-perf/stats.sh).
JV_SETTINGS = {'baseline_output_rate': 2, 'tooie_game_widescreen': 1, 'tooie_native_aspect': 0,
               'tooie_draw_distance': 6, 'tooie_disable_lod': True, 'baseline_color_format': 1,
               'baseline_resolution': 8}


@test('full', 'Jinjo Village session (owner\'s save)', parts=(
    ('bbox_culling', 'Bounding-box culling kept with the BK pipeline'),
    ('camera_scene_pairing', 'Camera scenes paired by their IDs (no unmapped camera while turning)'),
))
def jinjo_village(ctx):
    save = ctx.fixture(SAVE)
    stats = ctx.run_dir / 'gpu.stats'
    with Player(ctx, 'run', settings=JV_SETTINGS, save=save, env=vulkan.env(stats), seconds=240) as p:
        flows.load_save(p, expect=guest.JINJO_VILLAGE)
        still(p, 4.0)

        with ctx.part('bbox_culling', 'Bounding-box culling',
                      trigger='upstream forced every culled subtree visible: 2,650 draws per present (fixed: 1,140)'):
            work = vulkan.measure(stats, 8)
            per_present = work['draws'] / max(work['presents'], 1)
            ctx.note('%.0f draws, %.1fk vertices per present' % (per_present, work['kverts'] / max(work['presents'], 1)))
            check(per_present < 1700, '%.0f draws per present: culled subtrees are being drawn again' % per_present)

        with ctx.part('camera_scene_pairing', 'Camera scenes paired by their IDs',
                      trigger='turning: world and sky scenes cross-paired, camera unmapped for a game frame'):
            p.axis('rx', 0.8)
            time.sleep(5.0)
            report = evidence.capture(p)
            p.axis('rx', 0)
            rows = [r for r in evidence.rows(report, 'player_projection')]
            check(rows, 'no player_projection rows in the report')
            unmapped = [r for r in rows if not r['mapped']]
            ctx.note('%d of %d camera projections unmapped' % (len(unmapped), len(rows)))
            # A map load or camera cut may leave a few; the bug was steady pairs every few frames.
            check(len(unmapped) <= max(3, len(rows) // 200), '%d unmapped camera projections while turning' % len(unmapped))


@test('full', 'Draw distance 12x draws far more than 1x (Witchyworld entrance, owner\'s save)',
      trigger='only the model cull was scaled: entities past about 4 cubes still vanished')
def draw_distance(ctx):
    save = ctx.fixture(SAVE)
    counts = {}
    for scale in (1, 12):
        stats = ctx.run_dir / ('gpu-%d.stats' % scale)
        with Player(ctx, 'run%d' % scale, settings={'tooie_draw_distance': scale}, save=save,
                    env=vulkan.env(stats)) as p:
            flows.load_save(p)
            flows.warp(p, WITCHYWORLD, 0)  # A long view over the park's cubes.
            still(p, 2.0)
            work = vulkan.measure(stats, 4)
            counts[scale] = work['draws'] / max(work['presents'], 1)
            p.screenshot('view')
    ctx.note('draws per present: 1x %.0f, 12x %.0f' % (counts[1], counts[12]))
    # Measured 2026-10-27: 1,354 -> 2,350 (+74%).
    check(counts[12] > counts[1] * 1.3, '12x draws less than 30%% more than 1x')


@test('full', 'Draw-call matching without the quadratic pair loop (Hailfire Peaks ice cave)',
      trigger='HFP icy side looking out of the ice cave: RT64_Workload 39% of a core (fixed: 8%)')
def hfp_ice_cave(ctx):
    save, dump = ctx.fixture(SAVE), ctx.fixture('hfp-icecave-rdram.bin')
    settings = dict(baseline_output_rate=2, baseline_manual_hz=60, tooie_game_widescreen=1, tooie_native_aspect=4,
                    baseline_minimap=False, tooie_camera_distance=150, tooie_draw_distance=6, tooie_disable_lod=True,
                    baseline_resolution=3)
    with Player(ctx, 'run', settings=settings, save=save, seconds=240) as p:
        flows.load_save(p)
        p.resize(*DECK)
        flows.warp(p, guest.HFP_ICY_SIDE, 1)
        # The Deck report's camera: its viewport's eye and rotation (pitch, yaw, roll at +0x18),
        # held every frame by the test control "camera" pose on the game thread.
        source = dump.read_bytes()
        word = lambda a: int.from_bytes(source[a - 0x80000000:a - 0x80000000 + 4], 'little')
        real = lambda a: float(np.frombuffer(source[a - 0x80000000:][:4], '<f4')[0])
        view = word(guest.VIEWPORT)
        eye = [real(view + 4 * i) for i in range(3)]
        rotation = [real(view + 0x18 + 4 * i) for i in range(3)]
        p.send('camera ' + ' '.join('%.4f' % v for v in eye + rotation))
        time.sleep(3)
        # Held on every frame, not raced: 20 samples over a second all at the report's eye.
        for _ in range(20):
            live = guest.camera_eye(p)
            check(math.dist(live, eye) < 5, 'camera not held at the report\'s eye %s (live %s)' % (eye, live))
            time.sleep(0.05)
        p.screenshot('view')
        cpu = p.thread_cpu(10)
        p.send('camera off')
    workload = max((v for k, v in cpu.items() if k.startswith('RT64') and 'orkload' in k), default=None)
    if workload is None:
        raise Skip('no RT64 workload thread found: %s' % sorted(cpu))
    ctx.note('RT64 workload %.0f%% of a core at the Deck report\'s view' % workload)
    check(workload < 20, 'RT64 workload %.0f%% of a core (the quadratic loop took 39%%, fixed 8%%)' % workload)


@test('full', 'Pause backdrop without a shifted strip (1280x800)',
      trigger='paused in Jinjo Village at 1280x800: column 3 drawn from a 1 px narrower tile copy, a full-height strip')
def pause_backdrop(ctx):
    save = ctx.fixture(SAVE)
    # Dither noise is random per frame; off, static scenery is identical from frame to frame.
    with Player(ctx, 'run', settings={'baseline_minimap': False, 'baseline_dither_noise': 0}, save=save) as p:
        flows.load_save(p, expect=guest.JINJO_VILLAGE)
        p.resize(*DECK)
        still(p, 3.0)
        live = p.screenshot('live')
        flows.pause(p)
        time.sleep(1.0)
        paused = p.screenshot('paused')
        flows.unpause(p)
    shifted = shifted_columns(live, paused)
    ctx.note('%d windows match better shifted' % len(shifted))
    check(not shifted, 'backdrop content shifted against the live frame at (x, px): %s' % shifted[:8])


def shifted_columns(live, paused):
    """The bug drew one column of 32x32 tiles (about 34-43% across at 1280x800) from a 1 px narrower
    copy, so its content sat about 2 px off. Below the pause list and above the bottom counters,
    find runs of 3+ neighbouring 32 px windows that match the live frame better shifted sideways
    than in place. Banjo's idle animation in the middle is skipped. Checked against a synthetic
    copy of the bug (that column moved 2 px): 7 windows found; the fixed build: none."""
    h, w = live.shape[:2]
    band = slice(int(h * 0.76), int(h * 0.85))
    a, b = live[band].astype(np.int16), paused[band].astype(np.int16)
    hits = []
    for x in range(8, w - 40, 16):
        if 0.42 * w < x < 0.58 * w:
            continue
        cost = {d: float(np.abs(a[:, x:x + 32] - b[:, x + d:x + d + 32]).mean()) for d in range(-3, 4)}
        best = min(cost, key=cost.get)
        if best and cost[0] - cost[best] > 0.2:
            hits.append((x, best))
    runs = [h for h in hits if sum(1 for o in hits if abs(o[0] - h[0]) <= 16) >= 2]
    return runs if len(runs) >= 3 else []


@test('full', 'Pause-menu warp to an activated pad (owner\'s save: Mayahem Temple pad 1)')
def pause_warp(ctx):
    save = ctx.fixture(SAVE)
    pad1 = (-4695, -41, 4346)  # Where exit 0x1E (pad 1) places Banjo.
    with Player(ctx, 'run', save=save) as p:
        flows.load_save(p)
        flows.warp(p, guest.MAYAHEM_TEMPLE, 0x1E)  # On pad 1; the entrance spot fails the row's footing test.
        still(p, 6.0)  # Let the area title pass.
        p.axis('ly', -1.0)  # Walk off the pad.
        time.sleep(1.5)
        still(p, 2.0)
        start = p.position()
        check(math.dist(start, pad1) > 300, 'did not walk away from pad 1 (at %s)' % (start,))
        for _ in range(4):
            p.press('start', after=1.5)
            if p.game_state() == guest.STATE_PAUSED:
                break
        check(p.game_state() == guest.STATE_PAUSED, 'pause did not open')
        down(p, 6)                # WARP is the last row; the list stops at its end.
        p.screenshot('pause-list')
        p.press('a', after=3.0)   # The pad list (its title types itself in first).
        p.screenshot('pad-list')
        p.mark()
        p.press('a', after=1.0)   # The first activated pad.
        line = p.wait_log(r'^player pause warp: world 0 pad 1 -> map 0xb8 exit 0x1e$', 20, since_mark=True)
        deadline = time.time() + 40
        while math.dist(p.position(), pad1) > 300:
            if time.time() > deadline:
                raise Failure('Banjo did not arrive on pad 1 (at %s)' % (p.position(),))
            time.sleep(0.5)
        ctx.note(line.group(0).split(': ', 1)[1])


def down(p, count):
    for _ in range(count):
        p.axis('ly', 1.0)
        time.sleep(0.15)
        p.axis('ly', 0)
        time.sleep(0.3)


@test('full', 'Heap: bigger pool and low-memory cleanup (Witchyworld, 6x, LOD off)',
      trigger='the Deck ran out of heap in map 0xD6 at 6x with LOD off (heap-exhaustion)')
def heap_dense_level(ctx):
    save = ctx.fixture(SAVE)
    with Player(ctx, 'run', settings={'tooie_draw_distance': 6, 'tooie_disable_lod': True}, save=save, seconds=240) as p:
        flows.load_save(p)
        flows.warp(p, WITCHYWORLD, 0)
        p.axis('rx', 0.7)  # Turn so every direction's props load.
        time.sleep(40)
        p.axis('rx', 0)
        check(p.alive, 'player quit in the dense level')
        log = p.log()
    check('heap-exhaustion' not in log, 'heap ran out:\n' + '\n'.join(l for l in log.splitlines() if 'heap' in l)[-800:])
    ctx.note('%d low-memory cleanups' % log.count('heap-pressure-cleanup'))


@test('full', 'Atlantis session (owner\'s save, 16:10)', parts=(
    ('underwater_aspect', 'Underwater sway at the right width'),
    ('mouse_aim_swimming', 'Mouse aim does not steer outside first person while swimming'),
))
def atlantis(ctx):
    save = ctx.fixture(SAVE)
    with Player(ctx, 'run', save=save) as p:
        flows.load_save(p)
        p.resize(*DECK)
        flows.warp(p, 0x1A8, 0)
        still(p, 4.0)
        p.screenshot('underwater')

        with ctx.part('underwater_aspect', 'Underwater sway',
                      trigger='underwater: camera aspect jumped to exactly 4:3 for one game frame every few seconds'):
            report = evidence.capture(p)
            groups = {}
            for r in evidence.rows(report, 'player_projection'):
                if r.get('raw'):
                    groups.setdefault(r['group_id'], []).append(evidence.aspect(r['raw']))
            # The world camera has the most rows; look for one-frame jumps in its aspect.
            series = max(groups.values(), key=len) if groups else []
            check(len(series) > 20, 'too few camera rows in the report (%d)' % len(series))
            jumps = [abs(b / a - 1) for a, b in zip(series, series[1:]) if a]
            ctx.note('camera aspect %.3f..%.3f, largest step %.1f%%' % (min(series), max(series), 100 * max(jumps)))
            check(max(jumps) < 0.10, 'camera aspect jumped %.0f%% between frames' % (100 * max(jumps)))

        with ctx.part('mouse_aim_swimming', 'Mouse aim while swimming',
                      trigger='bafpctrl state 2 accepted: the gyro steered Banjo while swimming in third person'):
            facing = lambda: p.f32(p.player() + 0x3F4)  # Banjo's facing (F4 notes).
            before = facing()
            for _ in range(10):
                p.mouse(60, 0)
                time.sleep(0.05)
            time.sleep(0.6)
            turned = abs(guest.angle_delta(before, facing()))
            ctx.note('facing moved %.1f deg' % turned)
            check(turned < 2.0, 'mouse motion turned Banjo %.1f deg outside first person' % turned)


@test('full', 'Autosave on a timer with a backup first')
def autosave(ctx):
    with Player(ctx, 'run', settings={'baseline_autosave_minutes': 1}, seconds=200) as p:
        flows.new_game(p)
        line = p.wait_log(r'^player autosave slot=\d+ generation=\d+$', 90)
        time.sleep(3)
    backup = p.profile / 'saves/bt.n64.us.1.0.bin.autosave.bak'
    check(backup.exists(), 'no autosave backup written')
    ctx.note(line.group(0))
