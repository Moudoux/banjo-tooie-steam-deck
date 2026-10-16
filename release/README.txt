Banjo-Tooie: Scurvy Dog Port 0.1.0-alpha.4.2 - Windows x64
================================================

Unofficial alpha. The game is not included; you need your own copy.
There is no installer.

START
1. Extract the whole ZIP to a folder and keep its files together.
2. Run BanjoTooieScurvyDogPort.exe. If Windows reports a missing DLL, install the
   Microsoft Visual C++ x64 Redistributable:
   https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist
3. When asked, choose your NTSC-U 1.0 big-endian .z64 ROM.
   Supported ROM SHA-256:
   9ec37fba6890362eba86fb855697a9cff1519275531b172083a1a6a045483583

SAVES AND SETTINGS
Stored in %LOCALAPPDATA%\BanjoTooieScurvyDogPort
The first time this version starts and that folder does not exist yet,
your Alpha 4/4.1 save, settings, controls and stored ROM are copied from
%LOCALAPPDATA%\BanjoTooieEncore, which is only read and stays unchanged.
This is a one-time copy; later progress in Alpha 4/4.1 is not synchronized.
Close Alpha 4/4.1 first. An existing BanjoTooieScurvyDogPort folder, even an
empty one, is always used as it is. A failed or interrupted copy can leave
an unused BanjoTooieScurvyDogPort.migrating-... folder; you can delete it.
If your profile has no game save yet and an Alpha 3 save is found in
%LOCALAPPDATA%\BanjoTooieRecompiled\saves, you are asked once whether to
copy it. The Alpha 3 save is never changed.
Alpha 3 settings and practice states are not carried over.
Back up your saves before switching versions.

CONTROLS
Settings: Escape (keyboard) or Back (controller).
Keyboard:   WASD move, J = A, K = B, Space = Z, Enter = Start,
            arrow keys = C buttons, Q = L, E = R.
Controller: left stick moves, A = A, B = B, Start = Start,
            left trigger = Z, left/right shoulder = L/R,
            right stick = C buttons.

VIDEO
Output rate defaults to Original and changes apply during play.
Default (Original) is the former Current choice.

TEXTURE PACKS (EXPERIMENTAL)
Place RT64 .rtz packs in %LOCALAPPDATA%\BanjoTooieScurvyDogPort\mods and restart.
Only texture replacements are supported; code mods and ROM patches are not.
In-game texture appearance has not yet been verified.

KNOWN ISSUES
- Mayahem Temple can render black in the distance.
- Audio may clip at the Original output rate.
- Menus and some animations are limited.
- The full game has not been verified start to finish.

Report problems:
https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port/issues
Never attach ROMs, saves or unreviewed logs.

License: LICENSE. Third-party notices: THIRD_PARTY_NOTICES.md and
third_party/notices. Unofficial; not affiliated with Nintendo, Rare or
BanjoRecomp. AI tools generated substantial portions of the code.
