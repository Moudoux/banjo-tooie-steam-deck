"""In-game features, checked in guest RAM, the log, F4 reports and window pixels."""
import json
import math
import time

import numpy as np

from harness import evidence, flows, guest
from harness.framework import test, check, Failure, Skip
from harness.player import Player

# Window size for pixel checks: the Deck panel's 16:10.
DECK = (1280, 800)


def region(image, x0, y0, x1, y1):
    h, w = image.shape[:2]
    return image[int(y0 * h):int(y1 * h), int(x0 * w):int(x1 * w)].astype(np.int16)


def change(a, b, box):
    return float(np.abs(region(a, *box) - region(b, *box)).mean())


MINIMAP_BOX = (0.01, 0.62, 0.20, 0.98)   # Bottom-left corner, area name included.
CONTROL_BOX = (0.70, 0.05, 0.98, 0.35)   # Sky / scenery, away from the HUD.


def still(p, seconds=1.5):
    for axis in ('lx', 'ly', 'rx', 'ry'):
        p.axis(axis, 0)
    time.sleep(seconds)


@test('quick', 'Spiral Mountain session: features reachable from a new game', parts=(
    ('minimap', 'Minimap: drawn bottom left, F5 toggles it and saves the setting'),
    ('f4_report', 'F4 reports carry a guest RAM snapshot and game_state row'),
    ('analog_camera', 'Analog camera: right stick orbits at Lighthouse\'s rate, stops on release'),
    ('levitate', 'Hold L to levitate: rises while held, gravity restored on release'),
    ('mouse_aim', 'First-person mouse aim (gyro as mouse) turns the view in egg aim'),
    ('frustum_padding', 'Frustum padding: culling planes 5 degrees wider than the view'),
    ('minimap_zoom', 'Minimap zoom: hold F5 and press A / B; the A press is kept from the game'),
    ('minimap_explored', 'Minimap: shade explored ground (orange cells where Banjo walked)'),
    ('pause_full_map', 'Full-screen map while paused (F5)'),
))
def spiral_mountain(ctx):
    settings = {'baseline_levitate': True, 'baseline_minimap_explored': True}
    with Player(ctx, 'run', settings=settings) as p:
        flows.new_game(p)
        p.resize(*DECK)
        still(p, 2.0)

        with ctx.part('minimap', 'Minimap'):
            on = p.screenshot('minimap-on')
            p.key('F5', after=1.0)
            off = p.screenshot('minimap-off')
            in_map, elsewhere = change(on, off, MINIMAP_BOX), change(on, off, CONTROL_BOX)
            ctx.note('corner change %.1f, elsewhere %.1f' % (in_map, elsewhere))
            check(in_map > 8 and in_map > 4 * elsewhere, 'F5 did not toggle the corner minimap')
            saved = json.loads((p.profile / 'tooie_imgui.json').read_text())
            check(saved.get('baseline_minimap') is False, 'F5 did not save baseline_minimap=false: %s' % saved.get('baseline_minimap'))
            p.key('F5', after=1.0)
            back = p.screenshot('minimap-on-again')
            check(change(on, back, MINIMAP_BOX) < in_map / 3, 'second F5 did not bring the minimap back')

        with ctx.part('minimap_zoom', 'Minimap zoom'):
            before = json.loads((p.profile / 'tooie_imgui.json').read_text()).get('baseline_minimap_zoom', 2)
            y0 = p.position()[1]
            p.send('key F5 down')
            time.sleep(0.2)
            p.press('a', after=0.3)
            p.send('key F5 up')
            time.sleep(1.0)
            after = json.loads((p.profile / 'tooie_imgui.json').read_text()).get('baseline_minimap_zoom')
            ctx.note('zoom %s -> %s' % (before, after))
            check(after == before + 1, 'F5 + A did not zoom in (%s -> %s)' % (before, after))
            check(abs(p.position()[1] - y0) < 20, 'the A press reached the game (Banjo jumped)')
            # F5 acts on release only when it did not zoom: the minimap must still be on.
            check(json.loads((p.profile / 'tooie_imgui.json').read_text()).get('baseline_minimap', True) is not False,
                  'releasing F5 after zooming toggled the minimap off')
            p.send('key F5 down')
            time.sleep(0.2)
            p.press('b', after=0.3)
            p.send('key F5 up')
            time.sleep(0.5)

        with ctx.part('minimap_explored', 'Explored ground'):
            # The shading is orange blended over the floor colour: count corner pixels that
            # turn clearly redder while Banjo walks (the map rotates with the camera, which
            # stays put here).
            before = region(p.screenshot('explored-before'), *MINIMAP_BOX)
            p.axis('ly', -1.0)  # Walk forward.
            time.sleep(2.0)
            still(p, 1.0)
            after = region(p.screenshot('explored'), *MINIMAP_BOX)
            warmth = lambda img: img[..., 0] - img[..., 1]
            shaded = int(((warmth(after) - warmth(before) > 30) & (after[..., 0] > 120)).sum())
            ctx.note('%d newly shaded pixels' % shaded)
            check(shaded > 40, 'no explored-ground shading appeared on the minimap while walking')

        with ctx.part('pause_full_map', 'Full map while paused'):
            flows.pause(p)
            menu = p.screenshot('pause')
            p.key('F5', after=1.0)
            full = p.screenshot('pause-map')
            moved = change(menu, full, (0.25, 0.2, 0.75, 0.8))
            ctx.note('centre change %.1f' % moved)
            check(moved > 15, 'F5 did not open the full map in the pause menu')
            p.key('F5', after=1.0)
            flows.unpause(p)
            still(p, 1.0)

        with ctx.part('f4_report', 'F4 reports'):
            report = evidence.capture(p)
            snapshot = report.with_name(report.name[:-len('.jsonl')] + '-rdram.bin')
            check(snapshot.exists() and snapshot.stat().st_size == 0x800000, 'no 8 MB RDRAM snapshot next to the report')
            state = evidence.rows(report, 'game_state')
            check(state and state[0]['map'] == guest.SPIRAL_MOUNTAIN, 'game_state row missing or wrong map: %s' % state[:1])
            reported, live = state[0].get('player_position'), p.position()
            check(reported and math.dist(reported, live) < 50, 'reported position %s, live %s' % (reported, live))

        with ctx.part('frustum_padding', 'Frustum padding',
                      trigger='no slack: widescreen edges showed background while the camera turned'):
            # Viewport: fov +0xE4 (vertical, degrees); the culling normals start at +0xF8, the second
            # is the top plane in view space, (0, cos h, sin h) for half-angle h.
            vp = guest.viewport(p)
            fov = p.f32(vp + 0xE4)
            normal = [p.f32(vp + 0xF8 + 12 + 4 * i) for i in range(3)]
            half = math.degrees(math.asin(min(1.0, abs(normal[2]))))
            ctx.note('vertical half-angle %.2f deg for fov %.1f' % (half, fov))
            check(abs(normal[0]) < 1e-3 and abs(math.hypot(*normal) - 1) < 1e-3, 'unexpected plane layout %s' % normal)
            check(abs(half - (fov / 2 + 5)) < 0.3, 'culling half-angle %.2f, expected fov/2 + 5 = %.2f' % (half, fov / 2 + 5))

        with ctx.part('analog_camera', 'Analog camera'):
            still(p)
            yaw0, t0 = guest.camera_yaw(p), time.time()
            p.axis('rx', 1.0)
            time.sleep(2.0)
            yaw1, t1 = guest.camera_yaw(p), time.time()
            p.axis('rx', 0)
            time.sleep(2.5)  # The game eases the view onto the orbit's pose for about 1.8 s.
            yaw2 = guest.camera_yaw(p)
            time.sleep(1.0)
            yaw3 = guest.camera_yaw(p)
            # Full tilt: libultraship shaping + /127 + 0.15 deadzone gives 98 deg/s (PORT_NOTES).
            rate = abs(guest.angle_delta(yaw0, yaw1)) / (t1 - t0)
            ctx.note('%.0f deg/s at full tilt' % rate)
            check(rate > 5, 'right stick did not turn the camera (the first Deck build\'s dead stick)')
            check(60 < rate < 135, 'turn rate %.0f deg/s, expected about 98 (the first builds turned 1.6x faster)' % rate)
            check(abs(guest.angle_delta(yaw2, yaw3)) < 1.0, 'camera kept turning after release')

        with ctx.part('levitate', 'Hold L to levitate'):
            still(p)
            y0 = p.position()[1]
            p.hold('lb', True)
            time.sleep(2.0)
            y1 = p.position()[1]
            held_gravity = p.f32(p.u32(p.player() + guest.PHYSICS) + 0x34)
            p.hold('lb', False)
            time.sleep(0.8)
            y2 = p.position()[1]
            default = p.f32(0x80117EF0)
            gravity = p.f32(p.u32(p.player() + guest.PHYSICS) + 0x34)
            ctx.note('rose %.0f in 2 s, then fell %.0f' % (y1 - y0, y1 - y2))
            check(y1 - y0 > 600, 'held L raised Banjo only %.0f units' % (y1 - y0))
            check(held_gravity == 0.0, 'gravity %.1f while L was held' % held_gravity)
            check(y2 < y1 - 50, 'Banjo did not fall after L was released')
            check(abs(gravity - default) < 1e-3, 'gravity %.1f after release, default %.1f' % (gravity, default))
            time.sleep(3.0)  # Land (a long fall hurts, which is fine).

        with ctx.part('mouse_aim', 'First-person mouse aim'):
            still(p)
            p.press('y', after=1.5)  # C-up: egg-aim first person.
            yaw0, pitch0 = guest.camera_yaw(p), guest.camera_pitch(p)
            for _ in range(10):
                p.mouse(40, 0)
                time.sleep(0.05)
            time.sleep(0.5)
            turned = abs(guest.angle_delta(yaw0, guest.camera_yaw(p)))
            ctx.note('400 counts turned the view %.1f deg' % turned)
            # Default sensitivity 0.06 deg per count: 24 deg, less where the reticle takes some.
            check(turned > 5, 'mouse motion did not turn the first-person view')
            p.press('y', after=1.0)
