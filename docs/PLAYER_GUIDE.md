# Banjo-Tooie: Scurvy Dog Port player guide

For Alpha 4.5 on Windows x64. Scurvy Dog Port
is unofficial and the game is not included.

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
- The selected controller can control gameplay while another window has focus.
  Keyboard input, settings-menu navigation and binding capture require focus
  on the game window.
- **Talon Trot: press Z again to stop** is off by default. When enabled, enter
  Talon Trot, release Z, then press Z again to stop. Enabling it during Talon
  Trot applies on the next fresh entry; disabling restores hold-Z behavior at
  the next game input/state check. No restart is needed.

**Video**

- **Fullscreen** checkbox (borderless desktop), **Resolution scale** (1x–8x),
  **Output rate** (Default (Original), Original, Display or Manual, 20–360 Hz),
  **Widescreen**, **Gameplay aspect**, **Cutscene aspect** and **VSync**.
- Resolution, output rate, widescreen, gameplay aspect, VSync and fullscreen apply during
  play. **Gameplay aspect** offers fixed ratios and **Fit window (16:9–32:9)**,
  which follows the drawable window size as it resizes within that range.
  Widescreen **On/Off** overrides the original game's option. **Use game setting**
  follows that option. Gameplay aspect applies when the game is wide.
  **Cutscene aspect** changes take effect the next time you start the game. If fullscreen
  cannot change, the menu shows an error and keeps the actual window state.
- Default (Original) uses the game's original output rate. This label replaces
  Current; it does not change an existing output-rate choice.

- **Audio:** **Master volume** controls all game audio; **Music volume** separately controls music. Both default to 100% and are saved with your settings. Setting Master volume to 0 silences audio without pausing playback.
  **Mute Talon Trot movement sound** is off by default. Changes apply
  on the next movement sound, without restarting. Long-legged and stilt movement
  sounds are unchanged.

### Texture packs (experimental, untested)

- Place RT64 `.rtz` texture packs in
  `%LOCALAPPDATA%\BanjoTooieScurvyDogPort\mods`, then restart the game. There is
  no pack menu yet.
- This experimental support loads texture replacements only. Code mods, ROM
  patches and modified ROMs are unsupported.
- Packs load when the game starts. To stop using a pack, close the game, move
  its file out of the `mods` folder, then start the game again.
- Where two packs replace the same texture, the pack listed first in
  `mod_order` in `mods.json` (in the profile folder) wins. The game writes that
  file after it starts, placing a newly added pack ahead of packs it has
  already seen.
- A pack with its own HD texture setting loads only when that setting is on.
  There is no pack menu, so the pack's saved or default setting applies.
- If a pack file cannot be opened, the game uses the original textures for
  every pack, and `player.log` records this.
- If a mod stops the game from starting, the closing message names it and the
  folder to move it out of. Move it, then start the game again. The same
  message, with the folder, is written to `player.log`.
- Texture pack support is untested in this release; in-game replacement has
  not been checked.

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

- The backpack shadow can appear detached in some situations.
- The warp menu can jitter while it opens.
- Deferred: a shadow issue on the tent slope and the Clockwork view.
- Texture packs are untested.
- F6 fast-forward is not available in this player.
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
| Game closes with a mod message | Move the named mod out of `%LOCALAPPDATA%\BanjoTooieScurvyDogPort\mods`, then start again. If no message appears, check `player.log`. |

## Reporting a problem

Report reproducible problems in the
[issue tracker](https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port/issues).
Include the version, Windows version, GPU and driver, controller, settings,
where it happened and steps to reproduce. Press plain F4 once when the issue
appears to add a numbered marker with the time and current settings to the
existing `player.log`. A message appears for four seconds to confirm the marker
or report that it could not be written. The player also saves bounded recent
CPU-side rendering evidence to `issue-evidence-*.jsonl` in the profile folder.
Press F4 as soon as the issue appears; it does not start a future recording.
F4 does not take a screenshot or video.
If F4 is assigned to an action or a binding capture is active, that input
takes precedence and no marker is added.

To send an F4 report:

1. Press F4 while the problem is visible. Note the marker number. For a visual
   problem, also record a short video; F4 does not record it.
2. Close the game. Before starting it again, press Win+R and open
   `%LOCALAPPDATA%\BanjoTooieScurvyDogPort`.
3. Copy `player.log` and the matching `issue-evidence-*.jsonl` report to a separate
   folder. Check them for usernames and local paths; redact them if needed.
   ZIP **only those reviewed copies**. The next launch
   moves the current log to `player-previous.log`; later launches replace it.
4. Attach the ZIP and video to the GitHub issue. Include the marker number,
   scene, action and settings. If private delivery is preferable, email them to
   **somescurvydog@gmail.com** with the issue number and game version in the subject.

Keep large videos separate from the small log ZIP. Never send the whole profile
folder: it includes your ROM and saves. Do not upload ROMs, saves or generated
game files. The marker identifies the time and requested settings. The separate
evidence report contains selected actor, matrix, vertex and history diagnostics;
it is not a GPU image or proof that every draw was captured.
