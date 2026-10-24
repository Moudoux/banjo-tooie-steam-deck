"""Tiny flat-shaded low-poly renderer that emits SVG polygons (painter's order, N64-style fog)."""
import math, random


def sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
def norm(a):
    length = math.sqrt(dot(a, a)) or 1.0
    return (a[0] / length, a[1] / length, a[2] / length)


def hex_rgb(h): return tuple(int(h[i:i + 2], 16) for i in (1, 3, 5))
def rgb_hex(c): return '#%02x%02x%02x' % tuple(max(0, min(255, int(round(v)))) for v in c)
def mix(a, b, t): return tuple(a[i] + (b[i] - a[i]) * t for i in range(3))


class Mesh:
    def __init__(self):
        self.tris = []  # (p0, p1, p2, base_rgb, outward_hint) ; hint: 'up' or axis point

    def add(self, a, b, c, color, hint):
        self.tris.append((a, b, c, color, hint))


def island(seed=5, rings=7, segments=20):
    rnd = random.Random(seed)
    m = Mesh()

    def height(r, t):
        return 0.16 * (1 - r * r) + 0.05 * math.sin(3 * t + 1.3) * r * (1 - r) * 4 + rnd.uniform(-0.012, 0.012)

    pts = [[(0.0, height(0, 0), 0.0)]]
    for i in range(1, rings + 1):
        ring = []
        for j in range(segments):
            t = 2 * math.pi * j / segments + rnd.uniform(-0.08, 0.08)
            r = i / rings + (rnd.uniform(-0.03, 0.03) if i < rings else rnd.uniform(-0.05, 0.04))
            y = height(min(r, 1), t) if i < rings else rnd.uniform(-0.01, 0.03)
            ring.append((r * math.cos(t), y, r * math.sin(t)))
        pts.append(ring)
    grass = [hex_rgb(c) for c in ('#6cc644', '#7fd14a', '#5fb33a', '#8bdc55')]
    for j in range(segments):
        k = (j + 1) % segments
        m.add(pts[0][0], pts[1][j], pts[1][k], rnd.choice(grass), 'up')
    for i in range(1, rings):
        for j in range(segments):
            k = (j + 1) % segments
            a, b, c, d = pts[i][j], pts[i][k], pts[i + 1][j], pts[i + 1][k]
            m.add(a, c, d, rnd.choice(grass), 'up')
            m.add(a, d, b, rnd.choice(grass), 'up')
    # Cliff band and rocky underside down to a point.
    profile = [(-0.14, 0.97, '#a8693a'), (-0.4, 0.8, '#8f5a2e'), (-0.7, 0.55, '#7a4a28'),
               (-0.95, 0.28, '#5e3a22'), (-1.2, 0.0, '#4a2e1c')]
    prev = pts[rings]
    for y, scale, col in profile:
        ring = []
        for j in range(segments):
            x0, _, z0 = pts[rings][j]
            s = scale * rnd.uniform(0.9, 1.06) if scale else 0
            ring.append((x0 * s, y + rnd.uniform(-0.04, 0.04) if scale else y, z0 * s))
        base = hex_rgb(col)
        for j in range(segments):
            k = (j + 1) % segments
            a, b, c, d = prev[j], prev[k], ring[j], ring[k]
            shade = mix(base, (0, 0, 0), rnd.uniform(0, 0.12))
            m.add(a, c, d, shade, 'axis')
            m.add(a, d, b, shade, 'axis')
        prev = ring
    return m, height


def mountain(mesh, cx, cz, base_y, height, radius, seed=9, rings=9, segments=12):
    rnd = random.Random(seed)
    rock, path = hex_rgb('#d39a5e'), hex_rgb('#7fd14a')
    pts = []
    for i in range(rings + 1):
        t = i / rings
        r = radius * (1 - t) ** 0.8 + radius * 0.16 * t
        ring = []
        for j in range(segments):
            a = 2 * math.pi * j / segments + t * 0.6
            jitter = rnd.uniform(0.93, 1.05) if 0 < i < rings else 1
            ring.append((cx + r * jitter * math.cos(a), base_y + height * t, cz + r * jitter * math.sin(a)))
        pts.append(ring)
    for i in range(rings):
        for j in range(segments):
            k = (j + 1) % segments
            a, b, c, d = pts[i][j], pts[i][k], pts[i + 1][j], pts[i + 1][k]
            theta = (j + 0.5) / segments
            phase = (theta + (i + 0.5) / rings * 2.2) % 1.0
            col = path if phase < 0.3 else rock
            m_axis = ('axis_at', cx, cz)
            mesh.add(a, c, d, col, m_axis)
            mesh.add(a, d, b, col, m_axis)
    top = pts[rings]
    centre = (cx, base_y + height + 0.02, cz)
    for j in range(segments):
        mesh.add(top[j], centre, top[(j + 1) % segments], path, 'up')


def tree(mesh, x, y, z, h, seed):
    rnd = random.Random(seed)
    greens = [hex_rgb(c) for c in ('#2f8f2a', '#3a9e31', '#277a22')]
    seg = 6
    r = h * 0.38
    tip = (x, y + h, z)
    base = [(x + r * math.cos(2 * math.pi * j / seg), y + h * 0.18, z + r * math.sin(2 * math.pi * j / seg)) for j in range(seg)]
    for j in range(seg):
        mesh.add(base[j], tip, base[(j + 1) % seg], rnd.choice(greens), ('axis_at', x, z))
    trunk = hex_rgb('#6b4022')
    tw = h * 0.07
    for j in range(4):
        a0, a1 = math.pi / 2 * j + 0.4, math.pi / 2 * (j + 1) + 0.4
        p0 = (x + tw * math.cos(a0), y, z + tw * math.sin(a0))
        p1 = (x + tw * math.cos(a1), y, z + tw * math.sin(a1))
        mesh.add(p0, (p0[0], y + h * 0.2, p0[2]), (p1[0], y + h * 0.2, p1[2]), trunk, ('axis_at', x, z))
        mesh.add(p0, (p1[0], y + h * 0.2, p1[2]), p1, trunk, ('axis_at', x, z))


def build_isle(seed=5):
    m, height = island(seed)
    mountain(m, 0.26, -0.24, 0.06, 0.78, 0.36, seed + 4)
    rnd = random.Random(seed + 1)
    placed = 0
    while placed < 7:
        r, t = rnd.uniform(0.35, 0.85), rnd.uniform(0, 2 * math.pi)
        x, z = r * math.cos(t), r * math.sin(t)
        if (x - 0.28) ** 2 + (z + 0.22) ** 2 < 0.2:
            continue
        tree(m, x, 0.16 * (1 - r * r) - 0.01, z, rnd.uniform(0.2, 0.3), seed + placed)
        placed += 1
    return m


class Camera:
    def __init__(self, eye, target, focal, cx, cy):
        self.eye, self.focal, self.cx, self.cy = eye, focal, cx, cy
        self.f = norm(sub(target, eye))
        self.r = norm(cross(self.f, (0, 1, 0)))
        self.u = cross(self.r, self.f)

    def view(self, p):
        d = sub(p, self.eye)
        return (dot(d, self.r), dot(d, self.u), dot(d, self.f))

    def project(self, p):
        x, y, z = self.view(p)
        return (self.cx + x / z * self.focal, self.cy - y / z * self.focal, z)


def render(mesh, cam, light=(-0.55, 0.9, 0.55), fog=None, fog_range=(0, 1), ambient=0.5, stroke=0.6):
    light = norm(light)
    items = []
    for a, b, c, col, hint in mesh.tris:
        n = norm(cross(sub(b, a), sub(c, a)))
        centroid = tuple((a[i] + b[i] + c[i]) / 3 for i in range(3))
        if hint == 'up':
            out = (0, 1, 0)
        elif hint == 'axis':
            out = (centroid[0], 0, centroid[2])
        else:
            out = (centroid[0] - hint[1], 0, centroid[2] - hint[2])
        if dot(n, out) < 0:
            n = (-n[0], -n[1], -n[2])
        # Faces turned away from the camera are hidden by nearer ones; skip them.
        if dot(n, sub(cam.eye, centroid)) < 0:
            continue
        lit = ambient + (1 - ambient) * max(0.0, dot(n, light))
        colour = tuple(v * lit for v in col)
        pa, pb, pc = cam.project(a), cam.project(b), cam.project(c)
        depth = (pa[2] + pb[2] + pc[2]) / 3
        if fog:
            t = min(1, max(0, (depth - fog_range[0]) / (fog_range[1] - fog_range[0])))
            colour = mix(colour, hex_rgb(fog), t * 0.3)
        items.append((depth, pa, pb, pc, rgb_hex(colour)))
    items.sort(key=lambda item: -item[0])
    out = []
    for _, pa, pb, pc, colour in items:
        out.append(f'<polygon points="{pa[0]:.1f},{pa[1]:.1f} {pb[0]:.1f},{pb[1]:.1f} {pc[0]:.1f},{pc[1]:.1f}" '
                   f'fill="{colour}" stroke="{colour}" stroke-width="{stroke}" stroke-linejoin="round"/>')
    return ''.join(out)


def isle_svg(cx, cy, width, seed=5, fog='#3b2a6e', yaw=0.0):
    """The floating isle centred on (cx, cy) about `width` pixels across."""
    mesh = build_isle(seed)
    dist = 3.4
    eye = (dist * math.sin(yaw), 1.35, dist * math.cos(yaw))
    focal = width / 2.25 * dist
    cam = Camera(eye, (0, -0.05, 0), focal, cx, cy)
    return render(mesh, cam, fog=fog, fog_range=(2.4, 4.4), stroke=max(0.4, width / 900))
