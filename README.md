# Banjo-Tooie: Scurvy Dog Port

Banjo-Tooie: Scurvy Dog Port is an unofficial PC port of Banjo-Tooie. It
statically recompiles the original game code with
[N64Recomp](https://github.com/N64Recomp/N64Recomp) and renders it with
[RT64](https://github.com/rt64/rt64). It is an alpha. The game is not
included: you need your own NTSC-U 1.0 ROM.

This project is a work in progress. The design is not final, and improvements are continually evaluated.

The current release is for Windows x64.

**This project is not affiliated with BanjoRecomp.**

## AI disclosure

AI tools generated substantial portions of this project's code and assisted
with engineering and review. The project owner directs development, tests the
game and checks its behavior. Some upstream projects do not accept AI-generated
contributions. Respect their policies when you contribute to them.

## Play

Download the latest ZIP from
[Releases](https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port/releases).
Extract all of it into one folder and run `BanjoTooieScurvyDogPort.exe`. On
first launch, choose your ROM when asked.

- [Player guide](docs/PLAYER_GUIDE.md): supported ROM, saves, bringing over an
  Alpha 3 save, controls, settings and troubleshooting.
- [Release notes](docs/RELEASE_NOTES.md): changes and known issues.

## Renderer direction

The port draws through the original game's recompiled code. Smoother output
comes from labelling that output for RT64, not from running the game faster.
The renderer work follows three principles:

- **Stable history.** Each object, model part and view keeps a consistent
  identity across game updates, so interpolation pairs the right transforms.
- **Shared handling.** Deformation, attachments and split play follow common
  rules rather than fixes for individual characters.
- **Correct ownership.** Game behavior stays in patches to the game's own
  code; the runtime and renderer provide services, not game policy.

Player testing has accepted ordinary models, split play, camera movement,
save-select animation and widescreen zone transitions. Known issues are in the
[release notes](docs/RELEASE_NOTES.md), and [Rendering](docs/RENDERING.md)
describes the current design and its limits. Texture pack support is in the
source but untested; code mods and ROM patches are not supported.

## Looking ahead to Alpha 5

These are general aims, not promises or a schedule:

- Make maintained game patches, runtime integration and build tools easier to
  understand and change.
- Simplify duplicated handling while preserving established game behavior and
  renderer lifetimes.
- Prove improvements on small, independently reviewed examples before expanding
  them across the port.

## Build and contribute

- [Windows build guide](docs/WINDOWS.md): the maintained way to build and
  package the port from source.
- [Contributing](CONTRIBUTING.md): reporting problems and proposing changes.
- [Architecture](docs/ARCHITECTURE.md): what is built, who owns each
  behavior, and the rules for changes.

`python tools/check_repository.py` runs the ROM-free source checks.

### Agents: start here

If you are an AI agent or automated tool working in this repository:

1. Read [CONTRIBUTING.md](CONTRIBUTING.md). Its rules on checks, private
   files and AI disclosure apply to you.
2. Read [Rules for changes](docs/ARCHITECTURE.md#rules-for-changes) before
   you edit code.
3. Use
   [How to find what is selected](docs/ARCHITECTURE.md#how-to-find-what-is-selected)
   to confirm that a file is in the player before you change it.

## Report a problem

See [how to report a problem](CONTRIBUTING.md#report-a-problem).

## Credits and license

The port uses N64Recomp, N64ModernRuntime, RT64, Dear ImGui, SDL2 and other
projects listed in [third-party notices](THIRD_PARTY_NOTICES.md). The
[Banjo-Tooie decompilation](https://github.com/Mr-Wiseguy/banjo-tooie) provides
symbols and reverse-engineering groundwork. Credit for that work stays with
their contributors.

Project material that its authors can license is under
[GPL-3.0-only](LICENSE). Dependencies and game-derived material keep their own
terms. Banjo-Tooie belongs to its rights holders.
