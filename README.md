<p align="center">
  <img src="linux/steam-artwork/logo.png" alt="Banjo-Tooie: Steam Deck Port logo" width="640">
</p>

# Banjo-Tooie: Steam Deck Port

An unofficial native Linux port of Banjo-Tooie, tuned for the Steam Deck
OLED. It is a fork of
[Banjo-Tooie: Scurvy Dog Port](https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port),
a Windows PC port that statically recompiles Banjo-Tooie with
[N64Recomp](https://github.com/N64Recomp/N64Recomp) and renders it with
[RT64](https://github.com/rt64/rt64), and has since grown its own Deck
features. It is an alpha, built from source only. The
game is not included: you need your own NTSC-U 1.0 ROM.

Not affiliated with the upstream project, BanjoRecomp, Rare or Nintendo.
Report Linux problems here, not upstream.

## What this fork adds

- Native Linux player (Vulkan through RT64, SDL2).
- Steam Deck build: Zen 2 code generation, and defaults for new profiles of
  3x resolution, output following the display rate, widescreen fitted to the
  16:10 screen, right-stick first-person aiming and fullscreen.
- "Fit window" gameplay aspect down to 16:10 (upstream stops at 16:9), and an
  "Auto (fit window)" resolution setting.
- Xbox 360 (XBLA) button layout as the controller default.
- The ROM is picked up automatically when it sits beside the game.
- `recompcontrollerdb.txt` fills in controllers that SDL and Steam don't
  already know.

The game-code guest image is upstream's verified one; this fork's additions
hook the recompiled code without changing it.

## Build

You need Docker and your ROM. Its SHA-256 must be
`9ec37fba6890362eba86fb855697a9cff1519275531b172083a1a6a045483583`.

```sh
git clone git@github.com:Moudoux/banjo-tooie.git
cd banjo-tooie
git switch linux
cp /path/to/your/rom.z64 baserom.us.z64
docker build -t tooie-toolchain:latest .
docker run --rm -it -v "$(pwd)":/workspace tooie-toolchain:latest linux/build.sh
sudo chown -R "$USER": .
```

The first build downloads about 10 GB of pinned tools and takes a while.
The game ends up in `build/linux-player/baseline/player-bin/`.

Build options (pass with `-e NAME=value` to `docker run`):

| Option | Meaning |
| --- | --- |
| `TARGET=deck` | Default. Tuned for the Steam Deck and other Zen 2 CPUs, with Deck defaults and XBLA controls. |
| `TARGET=generic` | Any x86-64 CPU, upstream defaults and controls. |
| `CLEAN=1` | Start from fresh build folders. Needed after pulling changes to the game patches. |

Nothing ROM-derived may be shared: not the ROM, not `generated/`, `deps/`,
`build/`, nor the built game.

## Play

Copy `BanjoTooieSteamDeckPort`, `recompcontrollerdb.txt` and your ROM (as
`baserom.us.z64`) into one folder and run the game. The ROM is copied into
your profile on first launch, after which the folder copy is no longer needed.

Your profile, saves, settings and `player.log` are in
`~/.local/share/BanjoTooieSteamDeckPort` (or `$XDG_DATA_HOME`). A profile
from before the rename, `~/.local/share/BanjoTooieScurvyDogPort`, is moved
there on first launch with everything in it. If you added the game to Steam
before the rename, point the shortcut at the new executable name.

### Steam Deck

1. Check `ldd --version` in Desktop Mode: SteamOS needs glibc 2.38 or newer.
2. Put the game folder somewhere in your home directory and add the
   executable to Steam as a non-Steam game.
3. Launch it from Game Mode. Steam Input's default gamepad template works as
   is; cap the frame rate in the quick access menu to save battery.

### Controls (Deck build)

| Button | N64 | Action |
| --- | --- | --- |
| Left stick | Stick | Move; look around in first person |
| A | A | Jump |
| X | B | Attack |
| B | B | Talk, read signs, cancel text |
| LT / RT | Z | Crouch |
| Hold LT or RT, press the other | Z + C-left | Talon Trot |
| LB / RB | R | Centre camera behind Banjo |
| Y | C-up | First-person view |
| Right stick | | Orbit the camera (Lighthouse-style, inverted X; up raises it) |
| Right stick click | C-down | Toggle camera views |
| D-pad | C-buttons | C-up / C-down / C-left / C-right (hold LT or RT + right = Wonderwing) |
| Left stick click | L | |
| Start | Start | Pause |
| View | | Settings menu |

The settings menu has **Analog camera**, **Invert camera X**, **Invert
camera Y**, **Camera distance** (orbit distance, 50-150% of the game's) and **Mouse / gyro aiming** (first person, with separate switches for
flying, underwater and the Saucer of Peril crosshair; for the Deck's gyro, set
it to output as mouse in the Steam controller layout). With the analog camera off, the right stick acts as C-left /
C-right in steps and up / down toggles camera views.
Everything can be rebound in the settings menu. Profiles made before this
layout keep their bindings until you choose "Reset bindings".

## Updating

```sh
git fetch upstream
git rebase upstream/main
docker run --rm -it -e CLEAN=1 -v "$(pwd)":/workspace tooie-toolchain:latest linux/build.sh
```

Add the upstream remote once with
`git remote add upstream https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port.git`.

## For developers and AI agents

Read [linux/PORT_NOTES.md](linux/PORT_NOTES.md) first: how the build works,
every change made for Linux and the Deck, what has and hasn't been tested,
and how to handle upstream updates. Upstream's rules in
[CONTRIBUTING.md](CONTRIBUTING.md) and
[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) still apply, and
`python tools/check_repository.py` must pass.

## AI disclosure

AI tools generated substantial portions of the upstream project's code. This
fork's Linux and Steam Deck changes were written with an AI agent and
reviewed by the fork's owner.

## Credits and license

Upstream port by its authors; see the
[upstream project](https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port).
It builds on N64Recomp, N64ModernRuntime, RT64, Dear ImGui, SDL2 and the
others listed in [third-party notices](THIRD_PARTY_NOTICES.md), and on the
[Banjo-Tooie decompilation](https://github.com/Mr-Wiseguy/banjo-tooie).

Project material is under [GPL-3.0-only](LICENSE). Dependencies and
game-derived material keep their own terms. Banjo-Tooie belongs to its
rights holders.
