"""Guest RAM addresses and decoders (Banjo-Tooie NTSC-U, decomp names in comments).

All reads go through a Player (player.py), which handles the recomp RAM layout.
"""
import math
import struct

MAP = 0x80132DC2            # Current map id (u16).
GAME_STATE = 0x80127632     # D_80127630 +0x02 (s16): 3 in play, 4 paused.
STATE_PLAYING, STATE_PAUSED = 3, 4
PLAYERS, CONTROLLED = 0x80135490, 0x801354DC  # PlayerState table, controlled index.
VIEWPORT = 0x8012D500       # Pointer to the viewport: eye +0, forward +0xC, pitch/yaw +0x18/+0x1C.
CONTROLLERS = 0x800799E0    # Controller button tables (port 0 first).
TRANSITION = 0x80127630     # Pending map transition (capture.py's warp, as func_800A791C).
ACTORS = 0x80136EE0         # Vector of 0x9C-byte actors.

# Map ids (decomp include/maps.h).
SPIRAL_MOUNTAIN = 0xAF
MAYAHEM_TEMPLE = 0xB8
HFP_ICY_SIDE = 0x128
JINJO_VILLAGE = 0x142
GAME_SELECT = 0x158
STARTUP_INTRO = 0x18B
JRL_LAGOON = 0x1A7
GGM_MINE = 0xC7

# Player fields.
POSITION_POINTER = 0xE4     # -> f32 x, y, z.
YAW = 0xF8                  # Facing (minimap's player arrow).
PHYSICS = 0xC8              # -> baphysics struct: vertical velocity +0x14, gravity +0x34.


def flag(p, index):
    """Game flag 0xD49..0xD74 (func_800DA298's static block D_8012C78C, decomp src/core2/1EB3750.c)."""
    assert 0xD48 < index < 0xD75, 'only the static block is decoded'
    i = index - 0xD48
    return p.u8(0x8012C78C + i // 8) >> (i & 7) & 1


def player(p):
    return p.u32(PLAYERS + 4 * p.u32(CONTROLLED))


def position(p):
    pos = p.u32(player(p) + POSITION_POINTER)
    return tuple(p.f32(pos + 4 * i) for i in range(3))


def set_position(p, xyz):
    pos = p.u32(player(p) + POSITION_POINTER)
    for i, value in enumerate(xyz):
        p.wf32(pos + 4 * i, value)


def viewport(p):
    return p.u32(VIEWPORT)


def camera_eye(p):
    vp = viewport(p)
    return tuple(p.f32(vp + 4 * i) for i in range(3))


def camera_yaw(p):
    """Viewport yaw in degrees (+0x1C)."""
    return p.f32(viewport(p) + 0x1C)


def camera_pitch(p):
    return p.f32(viewport(p) + 0x18)


def angle_delta(a, b):
    """Signed b - a in degrees, wrapped to [-180, 180)."""
    return (b - a + 180.0) % 360.0 - 180.0


def warp(p, map_id, exit_id=0):
    """Pending transition, as func_800A791C writes it: map +0x10, exit +0x13, +0x16 = 1,
    +0x15 / +0x14 cleared, then the warp bit +0x12, consumed by func_800A7D84 next frame."""
    p.write(TRANSITION + 0x16, b'\1', 1)
    p.write(TRANSITION + 0x15, b'\0', 1)
    p.write(TRANSITION + 0x14, b'\0', 1)
    p.write(TRANSITION + 0x13, bytes([exit_id]), 1)
    p.write(TRANSITION + 0x10, struct.pack('<H', map_id), 2)
    p.write(TRANSITION + 0x12, b'\1', 1)


def actors(p):
    """(actor id, position) for each actor in the map's actor vector."""
    # include/vector.h: element size, begin, end, capacity end.
    vector = p.u32(ACTORS)
    size, begin, end = p.u32(vector), p.u32(vector + 4), p.u32(vector + 8)
    out = []
    if size != 0x9C:
        return out
    for i in range(min((end - begin) // size, 1024)):
        actor = begin + i * size
        actor_id = (p.u32(actor + 0x6C) >> 9) & 0xFFF
        out.append((actor_id, tuple(p.f32(actor + 4 + 4 * j) for j in range(3))))
    return out


def distance(a, b):
    return math.dist(a, b)
