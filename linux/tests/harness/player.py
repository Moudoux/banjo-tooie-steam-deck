"""One player run: a scratch profile, the test control channel, guest RAM and the log.

The player is started as a child process (Yama ptrace_scope 1 lets a parent read
/proc/PID/mem). It runs as an ordinary background window (SDL_VIDEODRIVER=x11 so it can
be captured by window id); TOOIE_TEST_CONTROL stands in for focus, so tests never take
the desktop's keyboard, mouse or focus.
"""
import json
import os
import re
import shutil
import signal
import stat
import struct
import subprocess
import time
from pathlib import Path

import numpy as np

from . import guest

ROOT = Path(__file__).resolve().parents[3]
DEFAULT_BINARY = ROOT / 'build/linux-player/baseline/player-bin/BanjoTooieSteamDeckPort'
ROM = ROOT / 'baserom.us.z64'
WINDOW_NAME = 'Banjo-Tooie: Steam Deck Port'
PROFILE_MARKER = 'Banjo-Tooie: Encore profile v1\n'
SAVE = 'saves/bt.n64.us.1.0.bin'


class RunError(AssertionError):
    pass


class Player:
    """Use as a context manager: `with Player(ctx, 'name', settings={...}) as p:`."""

    def __init__(self, ctx, name, settings=None, save=None, seconds=120, env=None, args=(),
                 profile_files=None, rom=True, profile=True, cwd=None, binary=None, control=True):
        self.ctx, self.name = ctx, name
        self.dir = (Path(ctx.run_dir) / name).resolve()
        if self.dir.exists():
            shutil.rmtree(self.dir)
        self.dir.mkdir(parents=True)
        self.profile = self.dir / 'profile'
        self.binary = Path(binary or ctx.binary)
        self.settings = {'baseline_fullscreen': 0, **(settings or {})}
        self.save, self.seconds, self.extra_env, self.args = save, seconds, env or {}, list(args)
        self.profile_files, self.use_rom, self.use_profile, self.cwd = profile_files or {}, rom, profile, cwd
        self.control = control
        self.proc = None
        self.base = None
        self.fifo = None
        self.window = None
        self._log_offset = 0

    # Lifecycle ---------------------------------------------------------------------------
    def __enter__(self):
        self.start()
        return self

    def __exit__(self, kind, value, tb):
        self.stop()
        return False

    def seed_profile(self):
        self.profile.mkdir(parents=True, exist_ok=True)
        (self.profile / '.encore-profile').write_text(PROFILE_MARKER)
        (self.profile / 'tooie_imgui.json').write_text(json.dumps(self.settings, indent=1) + '\n')
        if self.save:
            (self.profile / 'saves').mkdir(exist_ok=True)
            shutil.copyfile(self.save, self.profile / SAVE)
        for relative, data in self.profile_files.items():
            path = self.profile / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)

    def start(self):
        if self.use_profile:
            self.seed_profile()
        control = self.dir / 'control.fifo'
        env = dict(os.environ, SDL_VIDEODRIVER='x11')
        # Set even when control is off: the player then only logs errors, never opens a dialog.
        env['TOOIE_TEST_CONTROL'] = ''
        if self.control:
            os.mkfifo(control)
            env['TOOIE_TEST_CONTROL'] = str(control)
        env.update(self.extra_env)
        command = [str(self.binary)]
        if self.use_profile:
            command += ['--profile-dir', str(self.profile)]
        if self.use_rom:
            command += ['--rom', str(ROM)]
        if self.seconds:
            command += ['--seconds', str(self.seconds)]
        command += self.args
        self.stdout = open(self.dir / 'stdout.txt', 'wb')
        self.proc = subprocess.Popen(command, stdout=self.stdout, stderr=subprocess.STDOUT, env=env,
                                     cwd=self.cwd or self.dir)
        self.started_at = time.time()
        if not self.control:
            return
        # The player opens the FIFO for reading on its first SDL update.
        deadline = time.time() + 30
        while True:
            try:
                self.fifo = os.open(control, os.O_WRONLY | os.O_NONBLOCK)
                break
            except OSError:
                if self.proc.poll() is not None:
                    raise RunError('player exited before opening the test control channel (%s)' % self.tail())
                if time.time() > deadline:
                    raise RunError('test control channel never opened (is TOOIE_TEST_CONTROL in this build?)')
                time.sleep(0.05)
        os.set_blocking(self.fifo, True)

    def stop(self, timeout=20):
        if not self.proc:
            return None
        if self.proc.poll() is None:
            try:
                if self.fifo is None:
                    raise OSError
                self.send('quit')
            except OSError:
                self.proc.send_signal(signal.SIGTERM)
            try:
                self.proc.wait(timeout)
            except subprocess.TimeoutExpired:
                self.proc.send_signal(signal.SIGKILL)
                self.proc.wait()
        if self.fifo is not None:
            os.close(self.fifo)
            self.fifo = None
        self.stdout.close()
        return self.proc.returncode

    def wait_exit(self, timeout):
        try:
            return self.proc.wait(timeout)
        except subprocess.TimeoutExpired:
            raise RunError('player still running after %ss' % timeout)

    @property
    def alive(self):
        return self.proc is not None and self.proc.poll() is None

    # Log ---------------------------------------------------------------------------------
    @property
    def log_path(self):
        return self.profile / 'player.log'

    def log(self):
        try:
            return self.log_path.read_text(errors='replace')
        except FileNotFoundError:
            return ''

    def tail(self, lines=15):
        text = self.log() or (self.dir / 'stdout.txt').read_text(errors='replace')
        return '\n'.join(text.splitlines()[-lines:])

    def wait_log(self, pattern, timeout=60, since_mark=False):
        """First match of a regex in player.log (after the last mark() if since_mark)."""
        regex = re.compile(pattern, re.M)
        deadline = time.time() + timeout
        while True:
            text = self.log()
            match = regex.search(text, self._log_offset if since_mark else 0)
            if match:
                return match
            if not self.alive:
                raise RunError('player exited (%s) waiting for /%s/:\n%s' % (self.proc.returncode, pattern, self.tail()))
            if time.time() > deadline:
                raise RunError('no /%s/ in player.log within %ss:\n%s' % (pattern, timeout, self.tail()))
            time.sleep(0.1)

    def mark(self):
        self._log_offset = len(self.log())

    def log_since_mark(self):
        return self.log()[self._log_offset:]

    # Test control ------------------------------------------------------------------------
    def send(self, *commands):
        os.write(self.fifo, ''.join(c + '\n' for c in commands).encode())

    def sync(self, timeout=10):
        """Returns once the player applied every command sent before."""
        token = '%x' % time.monotonic_ns()
        self.send('echo ' + token)
        self.wait_log(r'^test-control echo %s$' % token, timeout)

    def attach_pad(self):
        self.send('pad attach')
        self.wait_log(r'controller|gamepad|input', 10)
        self.sync()

    def press(self, button, ms=120, after=0.25):
        self.send('pad button %s 1' % button)
        time.sleep(ms / 1000)
        self.send('pad button %s 0' % button)
        time.sleep(after)

    def hold(self, button, down=True):
        self.send('pad button %s %d' % (button, int(down)))

    def axis(self, name, value):
        self.send('pad axis %s %d' % (name, round(max(-1.0, min(1.0, value)) * 32767)))

    def key(self, name, ms=120, after=0.25):
        self.send('key %s down' % name)
        time.sleep(ms / 1000)
        self.send('key %s up' % name)
        time.sleep(after)

    def mouse(self, dx, dy):
        self.send('mouse %d %d' % (dx, dy))

    # Guest RAM ---------------------------------------------------------------------------
    def _find_base(self):
        pid = self.proc.pid
        with open('/proc/%d/maps' % pid) as maps, open('/proc/%d/mem' % pid, 'rb', 0) as mem:
            for line in maps:
                m = re.match(r'([0-9a-f]+)-([0-9a-f]+) rw', line)
                if not m:
                    continue
                lo, hi = int(m.group(1), 16), int(m.group(2), 16)
                if hi - lo < 0x800000:
                    continue
                # RDRAM may start anywhere page-aligned inside a larger reservation.
                for base in range(lo, hi - 0x800000 + 1, 0x1000):
                    try:
                        mem.seek(base + 0x136EE0)
                        vector = struct.unpack('<I', mem.read(4))[0]
                    except OSError:
                        break
                    if not 0x80100000 <= vector < 0x80800000:
                        continue
                    mem.seek(base + (0x132DC2 ^ 2))
                    level = struct.unpack('<H', mem.read(2))[0]
                    mem.seek(base + (vector - 0x80000000))
                    if 0xA0 <= level < 0x1B0 and struct.unpack('<I', mem.read(4))[0] == 0x9C:
                        return base
        return None

    def ram_ready(self, timeout=90):
        deadline = time.time() + timeout
        while self.base is None:
            if not self.alive:
                raise RunError('player exited before guest RAM was found:\n' + self.tail())
            self.base = self._find_base()
            if self.base is None:
                if time.time() > deadline:
                    raise RunError('guest RAM not found within %ss' % timeout)
                time.sleep(0.5)
        return self.base

    def read(self, address, size):
        """Raw bytes in the recomp layout (little-endian words)."""
        self.ram_ready()
        with open('/proc/%d/mem' % self.proc.pid, 'rb', 0) as mem:
            mem.seek(self.base + (address - 0x80000000))
            return mem.read(size)

    def u32(self, address):
        return struct.unpack('<I', self.read(address, 4))[0]

    def s32(self, address):
        return struct.unpack('<i', self.read(address, 4))[0]

    def f32(self, address):
        return struct.unpack('<f', self.read(address, 4))[0]

    def u16(self, address):
        return struct.unpack('<H', self.read(address ^ 2, 2))[0]

    def s16(self, address):
        return struct.unpack('<h', self.read(address ^ 2, 2))[0]

    def u8(self, address):
        return self.read(address ^ 3, 1)[0]

    def write(self, address, data, unit):
        """Write a byte (1), halfword (2) or word (4) in guest order."""
        self.ram_ready()
        with open('/proc/%d/mem' % self.proc.pid, 'r+b', 0) as mem:
            mem.seek(self.base + ((address - 0x80000000) ^ (4 - unit if unit < 4 else 0)))
            mem.write(data)

    def w32(self, address, value):
        self.write(address, struct.pack('<I', value & 0xFFFFFFFF), 4)

    def wf32(self, address, value):
        self.write(address, struct.pack('<f', value), 4)

    def dump(self, path):
        Path(path).write_bytes(self.read(0x80000000, 0x800000))

    # Game state shortcuts (see guest.py) -------------------------------------------------
    def map(self):
        return self.u16(guest.MAP)

    def game_state(self):
        return self.s16(guest.GAME_STATE)

    def wait_map(self, *maps, timeout=60, playing=True):
        deadline = time.time() + timeout
        while True:
            if self.base is not None or self._try_base():
                current = self.map()
                if current in maps and (not playing or self.game_state() == guest.STATE_PLAYING):
                    return current
            if not self.alive:
                raise RunError('player exited waiting for map %s:\n%s' % (
                    '/'.join('0x%X' % m for m in maps), self.tail()))
            if time.time() > deadline:
                now = self.map() if self.base is not None else None
                raise RunError('map %s not reached within %ss (map now %s)' % (
                    '/'.join('0x%X' % m for m in maps), timeout, None if now is None else '0x%X' % now))
            time.sleep(0.2)

    def _try_base(self):
        if self.alive:
            self.base = self._find_base()
        return self.base is not None

    def player(self):
        return guest.player(self)

    def position(self):
        return guest.position(self)

    def warp(self, map_id, exit_id=0):
        guest.warp(self, map_id, exit_id)

    # Window ------------------------------------------------------------------------------
    def window_id(self, timeout=30):
        deadline = time.time() + timeout
        while self.window is None:
            out = subprocess.run(['xdotool', 'search', '--pid', str(self.proc.pid), '--name', WINDOW_NAME],
                                 capture_output=True, text=True).stdout.split()
            if out:
                self.window = out[-1]
            elif time.time() > deadline:
                raise RunError('game window not found')
            else:
                time.sleep(0.3)
        return self.window

    def resize(self, width, height):
        subprocess.run(['xdotool', 'windowsize', self.window_id(), str(width), str(height)], check=True)
        time.sleep(1.0)

    def screenshot(self, label=None):
        """The window's pixels as an HxWx3 uint8 array; also saved as <label>.ppm."""
        data = subprocess.run(['import', '-silent', '-window', self.window_id(), '-depth', '8', 'ppm:-'],
                              capture_output=True, check=True).stdout
        image = read_ppm(data)
        if label:
            (self.dir / (label + '.ppm')).write_bytes(data)
        return image

    def record(self, seconds, label, width=480):
        """Gray raw video of the window at 60 fps (linux/frame_motion.py input)."""
        path = self.dir / (label + '.gray')
        geometry = subprocess.run(['xdotool', 'getwindowgeometry', self.window_id()],
                                  capture_output=True, text=True).stdout
        w, h = map(int, re.search(r'Geometry: (\d+)x(\d+)', geometry).groups())
        height = (h * width // w) // 2 * 2
        subprocess.run(['ffmpeg', '-loglevel', 'error', '-y', '-f', 'x11grab', '-window_id', self.window_id(),
                        '-framerate', '60', '-i', os.environ.get('DISPLAY', ':0'), '-t', str(seconds),
                        '-vf', 'scale=%d:%d,format=gray' % (width, height), '-f', 'rawvideo', str(path)],
                       check=True)
        return path, width, height

    # CPU ---------------------------------------------------------------------------------
    def thread_cpu(self, seconds):
        """Percent of one core per thread name over an interval."""
        def sample():
            out = {}
            for task in Path('/proc/%d/task' % self.proc.pid).iterdir():
                try:
                    text = (task / 'stat').read_text()
                except OSError:
                    continue
                name = text[text.index('(') + 1:text.rindex(')')]
                fields = text[text.rindex(')') + 2:].split()
                out[(task.name, name)] = int(fields[11]) + int(fields[12])
            return out
        hz = os.sysconf('SC_CLK_TCK')
        before = sample()
        time.sleep(seconds)
        after = sample()
        totals = {}
        for key, ticks in after.items():
            delta = ticks - before.get(key, ticks)
            totals[key[1]] = totals.get(key[1], 0) + delta * 100.0 / hz / seconds
        return totals


def read_ppm(data):
    # P6 header: magic, width, height, maxval, then one whitespace byte.
    parts, index = [], 0
    while len(parts) < 4:
        while data[index:index + 1].isspace():
            index += 1
        end = index
        while not data[end:end + 1].isspace():
            end += 1
        parts.append(data[index:end])
        index = end
    index += 1
    assert parts[0] == b'P6' and parts[3] == b'255', parts
    width, height = int(parts[1]), int(parts[2])
    return np.frombuffer(data, np.uint8, width * height * 3, index).reshape(height, width, 3)


def fifo_ok(path):
    return stat.S_ISFIFO(os.stat(path).st_mode)
