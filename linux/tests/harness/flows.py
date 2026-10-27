"""Common routes through the game, driven by the virtual pad and checked in guest RAM."""
import time

from . import guest
from .framework import Failure


def to_game_select(p, timeout=60):
    """Start through the intro until game select is up."""
    p.attach_pad()
    p.ram_ready(60)
    deadline = time.time() + timeout
    while p.map() != guest.GAME_SELECT:
        if time.time() > deadline:
            raise Failure('game select not reached (map 0x%X)' % p.map())
        p.press('start', after=1.2)
    time.sleep(2.5)  # The file select fades in and ignores early presses.


def load_save(p, expect=None, timeout=60):
    """From a fresh launch, load the profile's first save slot into play."""
    to_game_select(p)
    deadline = time.time() + timeout
    while p.map() == guest.GAME_SELECT or p.game_state() != guest.STATE_PLAYING:
        if time.time() > deadline:
            raise Failure('save not loaded (map 0x%X state %d)' % (p.map(), p.game_state()))
        if p.map() == guest.GAME_SELECT:
            p.press('a', after=2.0)
        else:
            time.sleep(0.5)
    if expect is not None:
        p.wait_map(expect, timeout=timeout)
    settle(p)
    return p.map()


def new_game(p, timeout=90):
    """Fresh profile: an empty slot, then Start through the opening story into Spiral Mountain."""
    to_game_select(p)
    deadline = time.time() + timeout
    while p.map() != guest.SPIRAL_MOUNTAIN or p.game_state() != guest.STATE_PLAYING:
        if time.time() > deadline:
            raise Failure('Spiral Mountain not reached (map 0x%X)' % p.map())
        p.press('a' if p.map() == guest.GAME_SELECT else 'start', after=1.5)
    settle(p)


def settle(p, seconds=3.0):
    """Let a freshly loaded map finish its entry (camera, title card)."""
    time.sleep(seconds)


def warp(p, map_id, exit_id=0, timeout=40):
    p.warp(map_id, exit_id)
    p.wait_map(map_id, timeout=timeout)
    settle(p)


def pause(p):
    p.press('start', after=1.5)
    if p.game_state() != guest.STATE_PAUSED:
        raise Failure('pause menu did not open (state %d)' % p.game_state())


def unpause(p):
    p.press('start', after=1.5)
