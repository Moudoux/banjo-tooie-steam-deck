# Banjo-Tooie: Scurvy Dog Port player guide

For the Windows x64 alpha. Scurvy Dog Port is unofficial and the game is not
included.

## Install and first launch

1. Download the ZIP from
   [Releases](https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port/releases)
   and extract all of it into one folder. Keep the files together; there is no
   installer.
2. Run `BanjoTooieScurvyDogPort.exe`. If Windows reports a missing DLL, install the
   [Microsoft Visual C++ x64 Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist).
   Do not download individual DLLs from other sites.
3. When asked, choose your Banjo-Tooie ROM. Only the NTSC-U 1.0 big-endian
   `.z64` image with this SHA-256 is supported:

```text
9ec37fba6890362eba86fb855697a9cff1519275531b172083a1a6a045483583
```

Other regions, byte orders and modified ROMs are not supported, and renaming a
file does not convert it. To check a file in PowerShell:

```powershell
(Get-FileHash -Algorithm SHA256 'C:\path\to\your.z64').Hash.ToLowerInvariant()
```

## Profile, saves and logs

Saves, settings, logs and a copy of your ROM are kept in
`%LOCALAPPDATA%\BanjoTooieScurvyDogPort`. Ordinary game saves are in its
`saves` folder. Progress is kept only through the game's own saves. Back up
the `saves` folder before switching versions.

Only one copy of the game can use the profile at a time. If the folder already
contains files that are not a Scurvy Dog Port profile, the game explains this and
changes nothing; move those files elsewhere and start again.

Each run writes `player.log` in the profile folder and keeps the previous run
as `player-previous.log`. Startup error messages say where details were
written.

### Moving from Alpha 4

Alpha 4 and 4.1 kept their profile in `%LOCALAPPDATA%\BanjoTooieEncore`. The
first time this version starts and `BanjoTooieScurvyDogPort` does not exist
yet, it copies your save and its backup, settings and controls, stored ROM copy
and Alpha 3 import choice from there. Logs and the RT64 cache are not copied.
The Alpha 4 folder is only read and stays as it is. This is a one-time copy;
progress made later in Alpha 4 or 4.1 is not synchronized with this profile.

- Close Alpha 4 or 4.1 first. While it is running, the game says so and copies
  nothing.
- An existing `BanjoTooieScurvyDogPort` folder, even an empty one, is always
  used as it is and nothing is copied into it.
- If copying fails, the message says why and names a folder such as
  `BanjoTooieScurvyDogPort.migrating-1234-1` holding what was copied so far.
  The game never uses it; you can delete it. Starting again retries with a new
  folder. An interrupted copy can also leave such a folder behind.
  If copying keeps failing, report the error. To start a fresh profile instead,
  create an empty `%LOCALAPPDATA%\BanjoTooieScurvyDogPort` folder; old progress
  will not be copied into it, and `BanjoTooieEncore` stays unchanged.

### Bringing over an Alpha 3 save

If your Scurvy Dog Port profile has no save yet and an Alpha 3 save exists in
`%LOCALAPPDATA%\BanjoTooieRecompiled\saves`, Scurvy Dog Port asks once whether to copy
it. **Yes** copies and checks the save; the Alpha 3 file is never changed.
**No** leaves it alone and you are not asked again. Close Alpha 3 first.
Settings, controls and Private Practice states are not carried over.

## Settings

Press Escape on the keyboard or Back on a controller to open **Banjo-Tooie
settings**. **Resume** returns to the game and **Quit** exits.

**Controls**

- **Controller**: **Automatic** or a specific connected controller.
- **Controller profile** and **Binding type** (keyboard or controller). The
  controller view lists buttons and stick/trigger directions together.
- To rebind, select an action's **Primary** or **Secondary** binding, release
  all controls, then press the new input before the countdown ends. Press
  Escape or click **Cancel binding** to stop a capture and keep the old binding.
  A controller cannot cancel: during a controller capture any press becomes the
  new binding. With only a controller, wait for the countdown to end.
  **Reset bindings** restores the defaults.
- Controller bindings can be changed only while a controller is selected;
  otherwise the menu says **Connect or select a controller to change its bindings.**

**Video**

- **Fullscreen** checkbox (borderless desktop), **Resolution scale** (1x–8x),
  **Output rate** (Default (Original), Original, Display or Manual, 20–360 Hz),
  **Widescreen**, **Gameplay aspect**, **Cutscene aspect** and **VSync**.
- Resolution, output rate, VSync and fullscreen apply during play. Aspect
  changes take effect the next time you start the game. If fullscreen cannot
  change, the menu shows an error and keeps the actual window state.
- Default (Original) uses the game's original output rate. This label replaces
  Current; it does not change an existing output-rate choice.

- **Audio:** **Mute Talon Trot movement sound** is off by default. Changes apply
  on the next movement sound, without restarting. Long-legged and stilt movement
  sounds are unchanged.

### Texture packs (local, unreleased build)

- Place RT64 `.rtz` texture packs in
  `%LOCALAPPDATA%\BanjoTooieScurvyDogPort\mods`, then restart the game. There is
  no pack menu yet.
- This experimental support loads texture replacements only. Code mods, ROM
  patches and modified ROMs are unsupported.
- Pack loading passed startup checks; in-game replacement appearance has not
  yet been verified.

## Default controls

| Game input | Keyboard | Controller |
| --- | --- | --- |
| Move | W / A / S / D | Left stick |
| A / B | J / K | A / B |
| Z | Space | Left trigger |
| Start | Enter | Start |
| L / R | Q / E | Left / right shoulder |
| C buttons | Arrow keys | Right stick |
| Settings | Escape | Back |

## Known issues

- Mayahem Temple can render black in the distance.
- Audio may clip at the Original output rate.
- Menus and some animations are limited.
- The full game has not been verified start to finish.

## Troubleshooting

| Problem | First check |
| --- | --- |
| Missing DLL | Keep the package together and install Microsoft's x64 Visual C++ Redistributable. |
| ROM not accepted | Check the `.z64` format and the SHA-256 above. |
| "Already using" message | Another copy of Scurvy Dog Port is running; close it. |
| Profile folder refused | Move the unrelated files out of `%LOCALAPPDATA%\BanjoTooieScurvyDogPort`. |
| Alpha 3 progress missing | The save is only offered while the Scurvy Dog Port profile has no save. Your Alpha 3 file is still in its original folder. |
| Alpha 4 progress missing | It is copied only while `%LOCALAPPDATA%\BanjoTooieScurvyDogPort` does not exist. Your `BanjoTooieEncore` folder is unchanged. |

## Reporting a problem

Report reproducible problems in the
[issue tracker](https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port/issues).
Include the version, Windows version, GPU and driver, controller, settings,
where it happened and steps to reproduce. Press plain F4 once when the issue
appears to add a numbered marker with the time and current settings to the
existing `player.log`. A message appears for four seconds to confirm the marker
or report that it could not be written. F4 does not take a screenshot or video.
If F4 is assigned to an action or a binding capture is active, that input
takes precedence and no marker is added.

To send an F4 report:

1. Press F4 while the problem is visible. Note the marker number. For a visual
   problem, also record a short video; F4 does not record it.
2. Close the game. Before starting it again, press Win+R and open
   `%LOCALAPPDATA%\BanjoTooieScurvyDogPort`.
3. Copy `player.log` to a separate folder. Check the copy for usernames and local
   paths; redact them if needed. ZIP **only that reviewed copy**. The next launch
   moves the current log to `player-previous.log`; later launches replace it.
4. Attach the ZIP and video to the GitHub issue. Include the marker number,
   scene, action and settings. If private delivery is preferable, email them to
   **somescurvydog@gmail.com** with the issue number and game version in the subject.

Keep large videos separate from the small log ZIP. Never send the whole profile
folder: it includes your ROM and saves. Do not upload ROMs, saves or generated
game files. A marker identifies the time and requested settings; it does not
contain actor, vertex or matrix captures.
