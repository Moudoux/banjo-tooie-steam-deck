"""Steam library artwork for Banjo-Tooie: Steam Deck Port, N64-meets-modern.

A low-poly, flat-shaded floating isle (lowpoly.py) hovers in a beam of light
from a Steam Deck's screen, on a dark minimal backdrop; extruded 3D title,
Jiggy and notes. Needs rsvg-convert and the Lilita One and Fredoka fonts
(Google Fonts, OFL) visible to fontconfig.
Usage: python3 make_art_n64.py OUTPUT_DIR
"""
import math, random, subprocess, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lowpoly

OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)

ADVANCE = {'B': .58, 'a': .49, 'n': .54, 'j': .40, 'o': .53, '-': .46, 'T': .52, 'i': .29, 'e': .49}
JIGSAW = ('M0,0 L36,0 C36,-6 31,-9 31,-15 C31,-24 40,-27 50,-27 C60,-27 69,-24 69,-15 '
          'C69,-9 64,-6 64,0 L100,0 L100,36 C106,36 109,31 115,31 C124,31 127,40 127,50 '
          'C127,60 124,69 115,69 C109,69 106,64 100,64 L100,100 L64,100 C64,94 69,91 69,85 '
          'C69,77 60,74 50,74 C40,74 31,77 31,85 C31,91 36,94 36,100 L0,100 L0,64 '
          'C6,64 9,69 15,69 C23,69 26,60 26,50 C26,40 23,31 15,31 C9,31 6,36 0,36 Z')
NOTE = ('M-22,30 C-40,30 -46,44 -38,52 C-28,62 -6,54 -4,38 L-4,-30 L36,-46 L36,18 '
        'C18,14 8,30 16,38 C26,48 48,40 48,24 L48,-62 L-16,-38 L-16,30 Z')

DEFS = '''
<defs>
  <linearGradient id="bg" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#070a1f"/><stop offset="0.55" stop-color="#151545"/>
    <stop offset="1" stop-color="#2b1d5c"/>
  </linearGradient>
  <radialGradient id="warm" cx="0.5" cy="0.5" r="0.5">
    <stop offset="0" stop-color="#ff9b54" stop-opacity="0.55"/><stop offset="0.45" stop-color="#d9487a" stop-opacity="0.22"/>
    <stop offset="1" stop-color="#6a2fa0" stop-opacity="0"/>
  </radialGradient>
  <radialGradient id="cool" cx="0.5" cy="0.5" r="0.5">
    <stop offset="0" stop-color="#1a9fff" stop-opacity="0.35"/><stop offset="1" stop-color="#1a9fff" stop-opacity="0"/>
  </radialGradient>
  <linearGradient id="beam" x1="0" y1="1" x2="0" y2="0">
    <stop offset="0" stop-color="#8fe3ff" stop-opacity="0.55"/><stop offset="0.6" stop-color="#4fb8ff" stop-opacity="0.16"/>
    <stop offset="1" stop-color="#4fb8ff" stop-opacity="0"/>
  </linearGradient>
  <linearGradient id="face" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#fffbd0"/><stop offset="0.3" stop-color="#ffe23d"/>
    <stop offset="0.62" stop-color="#ffb000"/><stop offset="1" stop-color="#ff7a00"/>
  </linearGradient>
  <linearGradient id="side" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#b95300"/><stop offset="1" stop-color="#5a2200"/>
  </linearGradient>
  <linearGradient id="jiggyFace" x1="0" y1="0" x2="1" y2="1">
    <stop offset="0" stop-color="#fff6b8"/><stop offset="0.45" stop-color="#ffc61a"/><stop offset="1" stop-color="#e08a00"/>
  </linearGradient>
  <linearGradient id="deckBody" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#3a3e47"/><stop offset="0.2" stop-color="#25282f"/><stop offset="1" stop-color="#121418"/>
  </linearGradient>
  <linearGradient id="screenBg" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#0b1030"/><stop offset="1" stop-color="#3a2470"/>
  </linearGradient>
  <linearGradient id="badge" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#1c2230" stop-opacity="0.92"/><stop offset="1" stop-color="#0c0f16" stop-opacity="0.92"/>
  </linearGradient>
  <filter id="glow" x="-60%" y="-60%" width="220%" height="220%"><feGaussianBlur stdDeviation="12"/></filter>
  <filter id="soft" x="-30%" y="-30%" width="160%" height="160%"><feGaussianBlur stdDeviation="22"/></filter>
</defs>'''


def background(w, h, glow_x, glow_y, glow_r, seed):
    rnd = random.Random(seed)
    stars = ''.join(f'<circle cx="{rnd.uniform(0, w):.0f}" cy="{rnd.uniform(0, h*0.75):.0f}" r="{rnd.uniform(0.5, 1.6)*max(1, w/1000):.1f}" '
                    f'fill="#ffffff" opacity="{rnd.uniform(0.15, 0.6):.2f}"/>' for _ in range(min(70, int(w * h / 12000))))
    shards = []
    for _ in range(6 if w > 1000 else 4):
        x, y, s = rnd.uniform(0, w), rnd.uniform(0, h * 0.8), rnd.uniform(0.02, 0.05) * max(w, h)
        a = rnd.uniform(0, 360)
        pts = ' '.join(f'{x + s*math.cos(math.radians(a + k*120 + rnd.uniform(-15, 15))):.0f},'
                       f'{y + s*math.sin(math.radians(a + k*120 + rnd.uniform(-15, 15))):.0f}' for k in range(3))
        shards.append(f'<polygon points="{pts}" fill="#8f7cff" opacity="{rnd.uniform(0.05, 0.12):.2f}"/>')
    return (f'<rect width="{w}" height="{h}" fill="url(#bg)"/>' + stars + ''.join(shards)
            + f'<circle cx="{glow_x:.0f}" cy="{glow_y:.0f}" r="{glow_r:.0f}" fill="url(#warm)"/>')


def extrude(shape, steps, dx, dy, side_fill, face_fill, stroke, stroke_w):
    """shape(offset_x, offset_y, attrs) -> svg; draws a stepped extrusion then the face."""
    body = ''.join(shape(dx * k, dy * k, f'fill="{side_fill}" stroke="{side_fill}" stroke-width="{stroke_w*1.2:.1f}" stroke-linejoin="round"')
                   for k in range(steps, 0, -1))
    return body + shape(0, 0, f'fill="{face_fill}" stroke="{stroke}" stroke-width="{stroke_w:.1f}" stroke-linejoin="round"')


def jiggy(cx, cy, scale, angle=-14):
    def shape(ox, oy, attrs):
        return (f'<path transform="translate({cx + ox:.1f},{cy + oy:.1f}) rotate({angle}) scale({scale:.3f}) translate(-56,-50)" '
                f'd="{JIGSAW}" {attrs} vector-effect="non-scaling-stroke"/>')
    depth = max(3, int(scale * 6))
    return (f'<path transform="translate({cx:.1f},{cy:.1f}) rotate({angle}) scale({scale:.3f}) translate(-56,-50)" d="{JIGSAW}" '
            f'fill="#ffd84a" filter="url(#glow)" opacity="0.8"/>'
            + extrude(shape, depth, scale * 1.4, scale * 1.9, '#9a4a00', 'url(#jiggyFace)', '#6b3200', max(1.0, scale * 1.2))
            + f'<path transform="translate({cx:.1f},{cy:.1f}) rotate({angle}) scale({scale:.3f})" '
              f'd="M-44,-38 L-16,-38 M-44,-26 L-36,-26" stroke="#fffbe0" stroke-width="7" stroke-linecap="round" opacity="0.8"/>')


def note(cx, cy, size, angle):
    s = size / 100

    def shape(ox, oy, attrs):
        return (f'<path transform="translate({cx + ox:.1f},{cy + oy:.1f}) rotate({angle:.0f}) scale({s:.3f})" d="{NOTE}" {attrs} '
                'vector-effect="non-scaling-stroke"/>')
    return extrude(shape, 4, size * 0.02, size * 0.03, '#8a3d00', 'url(#face)', '#5a2200', max(0.8, size * 0.03))


def deck(cx, cy, width, angle=0, isle_seed=5):
    s = width / 1000
    sx, sy, sw, sh = 235, 40, 530, 320
    parts = [
        '<rect x="0" y="0" width="1000" height="400" rx="150" fill="url(#deckBody)"/>',
        '<rect x="0" y="0" width="1000" height="400" rx="150" fill="none" stroke="#5a606c" stroke-width="3" opacity="0.6"/>',
        f'<rect x="{sx-14}" y="{sy-14}" width="{sw+28}" height="{sh+28}" rx="16" fill="#07080b"/>',
        f'<svg x="{sx}" y="{sy}" width="{sw}" height="{sh}" viewBox="0 0 530 320">'
        '<rect width="530" height="320" fill="url(#screenBg)"/>'
        '<circle cx="265" cy="170" r="210" fill="url(#warm)"/>'
        + lowpoly.isle_svg(265, 150, 300, isle_seed, fog='#2b1d5c', yaw=-0.5) + '</svg>',
        f'<rect x="{sx}" y="{sy}" width="{sw}" height="{sh*0.45:.0f}" fill="#ffffff" opacity="0.05"/>',
    ]

    def stick(x, y):
        return (f'<circle cx="{x}" cy="{y}" r="50" fill="#0c0d10"/><circle cx="{x}" cy="{y}" r="40" fill="#24262c"/>'
                f'<circle cx="{x}" cy="{y}" r="30" fill="#17181c"/>')

    def pad(x, y):
        return f'<rect x="{x}" y="{y}" width="120" height="120" rx="20" fill="#2b2e35"/>'
    parts += ['<g fill="#0e0f12"><rect x="58" y="92" width="34" height="96" rx="8"/><rect x="27" y="123" width="96" height="34" rx="8"/></g>',
              stick(170, 120), pad(55, 225), stick(830, 120), pad(825, 225),
              '<rect x="185" y="200" width="26" height="14" rx="7" fill="#0e0f12"/>',
              '<rect x="789" y="200" width="26" height="14" rx="7" fill="#0e0f12"/>']
    parts += [f'<circle cx="{925 + dx}" cy="{140 + dy}" r="17" fill="#0e0f12"/>' for dx, dy in ((0, -34), (0, 34), (-34, 0), (34, 0))]
    return (f'<g transform="translate({cx:.0f},{cy:.0f}) rotate({angle}) scale({s:.4f}) translate(-500,-200)">'
            + ''.join(parts) + '</g>')


def beam(cx, deck_top, top_y, bottom_w, top_w):
    return (f'<polygon points="{cx - bottom_w/2:.0f},{deck_top:.0f} {cx + bottom_w/2:.0f},{deck_top:.0f} '
            f'{cx + top_w/2:.0f},{top_y:.0f} {cx - top_w/2:.0f},{top_y:.0f}" fill="url(#beam)"/>'
            f'<ellipse cx="{cx:.0f}" cy="{deck_top:.0f}" rx="{bottom_w*0.55:.0f}" ry="{bottom_w*0.08:.0f}" fill="url(#cool)"/>')


def orbit(cx, cy, rx, ry, notes_at, size):
    ring = (f'<ellipse cx="{cx:.0f}" cy="{cy:.0f}" rx="{rx:.0f}" ry="{ry:.0f}" fill="none" stroke="#ffd84a" '
            f'stroke-width="{max(1.2, size*0.04):.1f}" opacity="0.35" stroke-dasharray="{size*0.3:.0f} {size*0.22:.0f}"/>')
    notes = ''.join(note(cx + rx * math.cos(math.radians(a)), cy + ry * math.sin(math.radians(a)), size * (0.8 + 0.25 * math.sin(math.radians(a))), -12 + a % 25)
                    for a in notes_at)
    return ring, notes


def title(text, x, y, size, anchor='middle'):
    width = sum(ADVANCE.get(c, .5) for c in text) * size
    start = x - width / 2 if anchor == 'middle' else x
    rot = [-4, 3, -2, 4, -3, 0, -3, 3, -2, 3, -2]
    lift = [0, -0.03, 0.015, -0.025, 0.01, 0, -0.02, 0.025, -0.015, 0.02, -0.01]
    glyphs, cursor = [], start
    for k, c in enumerate(text):
        adv = ADVANCE.get(c, .5) * size
        glyphs.append((c, cursor + adv / 2, y + lift[k % len(lift)] * size, 0 if c == '-' else rot[k % len(rot)]))
        cursor += adv

    def layer(ox, oy, attrs):
        return ''.join(f'<text x="{gx + ox:.1f}" y="{gy + oy:.1f}" text-anchor="middle" font-family="Lilita One" font-size="{size}" '
                       f'transform="rotate({r} {gx + ox:.1f} {gy + oy - size*0.35:.1f})" {attrs}>{c}</text>' for c, gx, gy, r in glyphs)
    steps = max(4, int(size / 14))
    dx, dy = size * 0.006, size * 0.011
    shadow = (f'<g filter="url(#soft)" opacity="0.6">'
              + layer(size * 0.04, size * 0.12, f'fill="#000000" stroke="#000000" stroke-width="{size*0.12:.1f}"') + '</g>')
    rim = layer(0, 0, f'fill="#2a0e00" stroke="#2a0e00" stroke-width="{size*0.1:.1f}" stroke-linejoin="round"')
    extr = ''.join(layer(dx * k, dy * k, f'fill="url(#side)" stroke="#4a1a00" stroke-width="{size*0.1:.1f}" stroke-linejoin="round"')
                   for k in range(steps, 0, -1))
    face = layer(0, 0, 'fill="url(#face)"')
    shine = layer(0, 0, f'fill="none" stroke="#fffde6" stroke-width="{size*0.014:.1f}" opacity="0.9"')
    return shadow + extr + rim + face + shine


def deck_glyph(x, y, h):
    w = h * 2.5
    return (f'<rect x="{x:.1f}" y="{y - h/2:.1f}" width="{w:.1f}" height="{h:.1f}" rx="{h*0.38:.1f}" fill="none" stroke="#ffffff" stroke-width="{h*0.1:.1f}"/>'
            f'<rect x="{x + w*0.26:.1f}" y="{y - h*0.3:.1f}" width="{w*0.48:.1f}" height="{h*0.6:.1f}" rx="{h*0.06:.1f}" fill="#1a9fff"/>'
            f'<circle cx="{x + w*0.13:.1f}" cy="{y - h*0.12:.1f}" r="{h*0.1:.1f}" fill="#ffffff"/>'
            f'<circle cx="{x + w*0.87:.1f}" cy="{y - h*0.12:.1f}" r="{h*0.1:.1f}" fill="#ffffff"/>')


def badge(text, cx, cy, size):
    glyph_h = size * 0.9
    text_w = size * 0.74 * len(text)
    width = glyph_h * 2.5 + size * 0.7 + text_w + size * 1.6
    height = size * 1.8
    left = cx - width / 2
    return (f'<rect x="{left:.0f}" y="{cy - height/2:.0f}" width="{width:.0f}" height="{height:.0f}" rx="{height/2:.0f}" '
            f'fill="url(#badge)" stroke="#1a9fff" stroke-width="{max(1.5, size*0.08):.1f}"/>'
            + deck_glyph(left + size * 0.8, cy, glyph_h)
            + f'<text x="{left + size*0.8 + glyph_h*2.5 + size*0.7 + text_w/2:.0f}" y="{cy + size*0.36:.0f}" font-family="Fredoka" '
              f'font-weight="600" font-size="{size}" letter-spacing="{size*0.16:.1f}" text-anchor="middle" fill="#ffffff">{text}</text>')


def stage(cx, isle_cy, isle_w, deck_cy, deck_w, note_size, jiggy_scale, seed=5, deck_angle=0):
    """Deck, beam, isle with orbiting notes and a Jiggy; returns svg."""
    deck_top = deck_cy - deck_w * 0.4 * 0.42
    ring_back, notes = orbit(cx, isle_cy + isle_w * 0.05, isle_w * 0.62, isle_w * 0.14, (200, 250, 300, 340), note_size)
    back_notes = ''.join(note(cx + isle_w * 0.62 * math.cos(math.radians(a)), isle_cy + isle_w * 0.05 + isle_w * 0.14 * math.sin(math.radians(a)),
                              note_size * 0.75, -10) for a in (215, 300))
    front_notes = ''.join(note(cx + isle_w * 0.62 * math.cos(math.radians(a)), isle_cy + isle_w * 0.05 + isle_w * 0.14 * math.sin(math.radians(a)),
                               note_size * 1.05, 8) for a in (25, 150))
    return (beam(cx, deck_top, isle_cy + isle_w * 0.1, deck_w * 0.5, isle_w * 0.95)
            + ring_back + back_notes
            + lowpoly.isle_svg(cx, isle_cy, isle_w, seed, yaw=-0.5)
            + jiggy(cx + isle_w * 0.4, isle_cy - isle_w * 0.33, isle_w * 0.0015)
            + front_notes
            + deck(cx, deck_cy, deck_w, deck_angle, seed))


def svg(w, h, body):
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">' + DEFS + body + '</svg>'


def render(name, w, h, body):
    path = os.path.join(OUT, name)
    with open(path + '.svg', 'w') as f:
        f.write(svg(w, h, body))
    subprocess.run(['rsvg-convert', '-w', str(w), '-h', str(h), '-o', path + '.png', path + '.svg'], check=True)
    os.remove(path + '.svg')


render('capsule_portrait', 600, 900,
       background(600, 900, 300, 520, 380, 1)
       + title('Banjo-', 300, 150, 128) + title('Tooie', 300, 272, 128)
       + stage(300, 505, 380, 745, 460, 42, 0)
       + badge('STEAM DECK PORT', 300, 866, 20))

render('capsule_wide', 920, 430,
       background(920, 430, 690, 200, 300, 2)
       + title('Banjo-Tooie', 290, 185, 88)
       + badge('STEAM DECK PORT', 290, 262, 18)
       + stage(700, 180, 260, 385, 320, 28, 0))

render('hero', 3840, 1240,
       background(3840, 1240, 2650, 560, 1100, 3)
       + stage(2650, 540, 1050, 1190, 1250, 105, 0))

render('logo', 1600, 560,
       title('Banjo-Tooie', 800, 300, 250) + badge('STEAM DECK PORT', 800, 470, 40))

render('icon', 512, 512,
       '<clipPath id="tile"><rect width="512" height="512" rx="96"/></clipPath><g clip-path="url(#tile)">'
       + background(512, 512, 256, 230, 300, 4)
       + stage(256, 215, 330, 470, 430, 34, 0.6) + '</g>')
print('done')
