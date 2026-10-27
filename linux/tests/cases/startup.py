"""Launch, profile, ROM, controller database and the startup fixes."""
import fcntl
import os
import re
import shutil
import time

from harness import flows, guest
from harness.framework import test, check, Failure
from harness.player import Player, ROM, PROFILE_MARKER

STORED_ROM = 'bt.n64.us.1.0.z64'

FRESH = {}  # No saved settings: the Deck defaults apply (fullscreen is always forced off).


def marker_fields(p):
    """Press F4 and parse the issue-marker line (the player's effective settings)."""
    p.mark()
    p.key('F4')
    line = p.wait_log(r'^baseline issue-marker seq=.*$', 15, since_mark=True).group(0)
    return dict(re.findall(r'(\w+)=("[^"]*"|\S+)', line))


@test('quick', 'Boot, intro, game select, clean shutdown with the save persisted; heap pool extended')
def boot_and_shutdown(ctx):
    with Player(ctx, 'run', settings=FRESH) as p:
        flows.to_game_select(p)
        log = p.log()
        check('test-control active' in log, 'test control channel not active')
        check(re.search(r'^baseline heap-extended start=80137800 size=006c8800 end=80800000$', log, re.M),
              'heap pool not extended (Heap: bigger pool)')
    log = p.log()
    check(re.search(r'^baseline terminal-fence success ', log, re.M), 'no clean terminal fence:\n' + p.tail())
    check(re.search(r'^baseline saver receipt: .* failed=0 confirmed=1$', log, re.M), 'save not confirmed on exit')
    check(p.proc.returncode == 0, 'exit code %s' % p.proc.returncode)


@test('quick', 'Deck defaults: 3x, output rate Display, widescreen Fit window, XBLA layout, right-stick aim')
def deck_defaults(ctx):
    with Player(ctx, 'run', settings=FRESH) as p:
        flows.to_game_select(p)
        fields = marker_fields(p)
    expected = dict(requested_resolution='3', requested_output_rate='2', requested_wide='1', requested_aspect='4')
    wrong = {k: (fields.get(k), v) for k, v in expected.items() if fields.get(k) != v}
    check(not wrong, 'defaults differ (got, expected): %s' % wrong)
    check(fields.get('controller', '').strip('"') == 'Tooie test pad', 'virtual pad not selected: %s' % fields.get('controller'))


@test('quick', 'XBLA button layout on the pad (A=A, X/B=B, LT/RT=Z, LB=L, RB=R, Y=C-up, Start)')
def xbla_layout(ctx):
    # D_80079AF8[0] +0: the N64 buttons the game read this frame (libultra bits).
    expected = {'a': 0x8000, 'b': 0x4000, 'x': 0x4000, 'y': 0x0008, 'lb': 0x0020, 'rb': 0x0010,
                'start': 0x1000, 'ls': 0x0020, 'rs': 0x0004}
    with Player(ctx, 'run', settings=FRESH) as p:
        p.attach_pad()
        p.ram_ready(60)
        p.wait_map(guest.STARTUP_INTRO, playing=False)
        wrong = {}
        for button, bits in expected.items():
            p.hold(button, True)
            time.sleep(0.25)
            got = p.u16(0x80079AF8)
            p.hold(button, False)
            time.sleep(0.25)
            if got != bits:
                wrong[button] = '%04X (expected %04X)' % (got, bits)
        for trigger in ('lt', 'rt'):
            p.axis(trigger, 1.0)
            time.sleep(0.25)
            got = p.u16(0x80079AF8)
            p.axis(trigger, -1.0)
            time.sleep(0.25)
            if got != 0x2000:
                wrong[trigger] = '%04X (expected 2000)' % got
        # Talon Trot chord: LT + RT adds C-left once Z was already sent on the previous poll.
        p.axis('lt', 1.0)
        time.sleep(0.2)
        p.axis('rt', 1.0)
        time.sleep(0.3)
        chord = p.u16(0x80079AF8)
        p.axis('lt', -1.0)
        p.axis('rt', -1.0)
        if chord != 0x2002:
            wrong['lt+rt'] = '%04X (expected 2002, Z + C-left)' % chord
        # Left stick reaches the game's stick value (D_80079AF8 +8, f32).
        p.axis('lx', 1.0)
        time.sleep(0.25)
        stick = p.f32(0x80079B00)
        p.axis('lx', 0)
        if stick < 0.95:
            wrong['lx'] = 'stick x %.2f' % stick
    check(not wrong, 'pad to N64 mapping differs: %s' % wrong)


@test('quick', 'Instant intro skip', trigger='Start did nothing while flag 0xD56 was unset (first part of the intro)')
def intro_skip(ctx):
    with Player(ctx, 'run', settings=FRESH) as p:
        p.attach_pad()
        p.ram_ready(60)
        p.wait_map(guest.STARTUP_INTRO, playing=False, timeout=60)
        time.sleep(1.0)
        flag = guest.flag(p, 0xD56)  # The ticker reads Start only once this is set.
        check(flag == 0, 'flag 0xD56 already set; the trigger condition was not reproduced')
        p.press('start')
        start = time.time()
        while p.map() == guest.STARTUP_INTRO:
            if time.time() - start > 6:
                raise Failure('still in the intro 6 s after Start with flag 0xD56 unset')
            time.sleep(0.1)
        ctx.note('left the intro %.1fs after Start' % (time.time() - start))


@test('quick', 'Start skips story cutscenes (new game opening)')
def start_skips_cutscenes(ctx):
    with Player(ctx, 'run', settings=FRESH) as p:
        flows.to_game_select(p)
        while p.map() == guest.GAME_SELECT:
            p.press('a', after=1.5)
        cutscene = p.map()
        start = time.time()
        while p.map() != guest.SPIRAL_MOUNTAIN:
            if time.time() - start > 25:
                raise Failure('opening cutscene (map 0x%X) not skipped by Start' % p.map())
            p.press('start', after=1.0)
        ctx.note('opening 0x%X skipped in %.1fs' % (cutscene, time.time() - start))


@test('full', 'Start skips story cutscenes: off leaves Start alone')
def start_skips_cutscenes_off(ctx):
    with Player(ctx, 'run', settings={'tooie_start_skips_cutscenes': False}) as p:
        flows.to_game_select(p)
        while p.map() == guest.GAME_SELECT:
            p.press('a', after=1.5)
        cutscene = p.map()
        for _ in range(6):
            p.press('start', after=1.0)
        check(p.map() != guest.SPIRAL_MOUNTAIN, 'Start skipped the opening with the setting off')
        ctx.note('still in 0x%X' % cutscene)


def normal_play(ctx, data, **kwargs):
    """Zero-argument play (the Deck's Steam launch): profile under XDG_DATA_HOME, quit by control."""
    return Player(ctx, 'run', profile=False, rom=False, seconds=None,
                  env={'XDG_DATA_HOME': str(data)}, **kwargs)


def profile_folder(path, rom=True):
    path.mkdir(parents=True)
    (path / '.encore-profile').write_text(PROFILE_MARKER)
    (path / 'tooie_imgui.json').write_text('{"baseline_fullscreen": 0}\n')
    if rom:  # The stored ROM copy, so no file picker opens.
        shutil.copyfile(ROM, path / STORED_ROM)
    return path


@test('quick', 'First launch uses baserom.us.z64 beside the executable')
def adjacent_rom(ctx):
    folder = ctx.run_dir / 'game'
    folder.mkdir(exist_ok=True)
    shutil.copy2(ctx.binary, folder / ctx.binary.name)
    os.symlink(ROM, folder / 'baserom.us.z64')
    data = ctx.run_dir / 'xdg'
    profile_folder(data / 'BanjoTooieSteamDeckPort', rom=False)
    with normal_play(ctx, data, binary=folder / ctx.binary.name) as p:
        p.profile = data / 'BanjoTooieSteamDeckPort'
        p.wait_log(r'^player using ROM beside the executable: .*baserom\.us\.z64$', 30)
        p.wait_log(r'^baseline start_game requested', 60)


@test('quick', 'Profile rename from BanjoTooieScurvyDogPort (renameat2, no replace)')
def profile_rename(ctx):
    data = ctx.run_dir / 'xdg'
    old = profile_folder(data / 'BanjoTooieScurvyDogPort')
    (old / 'sentinel').write_text('moved')
    with normal_play(ctx, data) as p:
        p.profile = data / 'BanjoTooieSteamDeckPort'
        p.wait_log(r'^player profile rename: moved ', 30)
    new = data / 'BanjoTooieSteamDeckPort'
    check(not old.exists() and (new / 'sentinel').read_text() == 'moved', 'profile folder not moved whole')


@test('quick', 'Profile rename refuses while the old folder is locked (a running older build)')
def profile_rename_locked(ctx):
    data = ctx.run_dir / 'xdg'
    old = profile_folder(data / 'BanjoTooieScurvyDogPort')
    (old / 'sentinel').write_text('kept')
    handle = os.open(old, os.O_RDONLY | os.O_DIRECTORY)
    fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
    try:
        p = normal_play(ctx, data, control=False)
        p.start()
        try:
            code = p.wait_exit(30)
        finally:
            p.stop()
    finally:
        os.close(handle)
    check(code != 0, 'player started while the old profile was locked')
    check((old / 'sentinel').exists() and not (data / 'BanjoTooieSteamDeckPort').exists(), 'locked profile was touched')
    check(b'is already using' in (p.dir / 'stdout.txt').read_bytes(), 'no "already using" message')


@test('quick', 'Controller DB beside the executable fills gaps only (Steam\'s SDL_GAMECONTROLLERCONFIG wins)')
def controller_db(ctx):
    folder = ctx.run_dir / 'game'
    folder.mkdir(exist_ok=True)
    shutil.copy2(ctx.binary, folder / ctx.binary.name)
    steam = '03000000feca0000adde000001000000,Steam mapped pad,a:b0,b:b1,platform:Linux,'
    fresh = '03000000adde0000feca000001000000,DB only pad,a:b0,b:b1,platform:Linux,'
    override = steam.replace('Steam mapped pad,a:b0,b:b1', 'DB override,a:b5,b:b6')
    (folder / 'recompcontrollerdb.txt').write_text(override + '\n' + fresh + '\n')
    with Player(ctx, 'run', settings=FRESH, binary=folder / ctx.binary.name,
                env={'SDL_GAMECONTROLLERCONFIG': steam}) as p:
        line = p.wait_log(r'^baseline controller mappings added=(\d+) from .*$', 30)
    check(line.group(1) == '1', 'expected only the DB-only GUID added: %s' % line.group(0))


@test('quick', 'No on-screen keyboard at launch (SDL_ENABLE_SCREEN_KEYBOARD=0)')
def no_screen_keyboard(ctx):
    with Player(ctx, 'run', settings=FRESH) as p:
        p.send('hint SDL_ENABLE_SCREEN_KEYBOARD')
        value = p.wait_log(r'^test-control hint SDL_ENABLE_SCREEN_KEYBOARD=(.*)$', 15).group(1)
    check(value == '0', 'hint is %s' % value)


@test('quick', 'Test control is inert unless TOOIE_TEST_CONTROL names a FIFO')
def test_control_inert(ctx):
    p = Player(ctx, 'run', settings=FRESH, control=False, seconds=12)
    p.start()
    try:
        p.wait_exit(40)
    finally:
        p.stop()
    check('test-control' not in p.log(), 'test control logged without the variable')
    check(re.search(r'^baseline start_game requested', p.log(), re.M), 'did not start')


@test('full', 'No startup crash after a slow first RT64 workload',
      trigger='NVIDIA shader disk cache off: dummy VI workload zeroed the sky DL, crashed 10 of 10 launches')
def startup_dummy_workload(ctx, launches=10):
    deferred = 0
    for i in range(launches):
        p = Player(ctx, 'run%d' % i, settings=FRESH, control=False, seconds=14,
                   env={'__GL_SHADER_DISK_CACHE': '0'})
        p.start()
        try:
            code = p.wait_exit(60)
        finally:
            p.stop()
        log = p.log()
        check(code == 0 and 'terminal-fence success' in log, 'launch %d failed (exit %s):\n%s' % (i, code, p.tail()))
        check('baseline map=0x18B' in log, 'launch %d never reached the intro' % i)
        deferred += 'start_game deferred for a running dummy workload' in log
    ctx.note('%d/%d launches waited for a dummy workload' % (deferred, launches))
    check(deferred, 'no launch hit the trigger (a dummy workload running at start)')
