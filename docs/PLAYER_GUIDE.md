# Banjo-Tooie: Scurvy Dog Port player guide

For the Windows x64 alpha, version 0.1.0-alpha.4.1. Scurvy Dog Port is unofficial and the
game is not included.

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

Saves, settings, logs and a copy of your ROM are kept in `%LOCALAPPDATA%\BanjoTooieEncore`.
This folder name is retained from Alpha 4; no save migration is needed.
Ordinary game saves are in its `saves` folder. Progress is kept only through
the game's own saves. Back up the `saves` folder before switching versions.

Only one copy of the game can use the profile at a time. If the folder already
contains files that are not a Scurvy Dog Port profile, the game explains this and
changes nothing; move those files elsewhere and start again.

Each run writes `encore.log` in the profile folder and keeps the previous run
as `encore-previous.log`. Startup error messages say where details were
written.

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
- **Controller profile** and **Binding type** (keyboard, controller button or
  axis direction).
- To rebind, select an action's **Primary** or **Secondary** binding, release
  all controls, then press the new input. **Cancel binding** stops a capture;
  **Reset bindings** restores the defaults.

**Video**

- **Resolution scale** (1x–8x), **Output rate** (Current, Original, Display or
  Manual, 20–360 Hz), **Widescreen**, **Gameplay aspect**, **Cutscene aspect**
  and **VSync**.
- Display settings except VSync take effect the next time you start the game.

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
| Profile folder refused | Move the unrelated files out of `%LOCALAPPDATA%\BanjoTooieEncore`. |
| Alpha 3 progress missing | The save is only offered while the Scurvy Dog Port profile has no save. Your Alpha 3 file is still in its original folder. |

## Reporting a problem

Report reproducible problems in the
[issue tracker](https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port/issues).
Include the version, Windows version, GPU and driver, controller, settings,
where it happened and steps to reproduce. Review `encore.log` before sharing:
it can include usernames and local paths. Never upload a ROM, saves, generated
game files, a whole profile or unreviewed logs.
