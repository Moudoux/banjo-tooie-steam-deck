# Release notes

## 0.1.0-alpha.4.2 - Banjo-Tooie: Scurvy Dog Port

- Changed player presentation to Present Early for every output-rate option.
- Changed frame pacing to use the game's measured graphics rate.
- Changed the profile folder to BanjoTooieScurvyDogPort, with a one-time copy of existing Alpha 4/4.1 saves and settings.
- Fixed controller binding guidance and the controls menu layout.

Upgrade: Close Alpha 4/4.1 before starting this version. Your previous profile is preserved; later progress in the old version is not synchronized.

## 0.1.0-alpha.4.1 — Banjo-Tooie: Scurvy Dog Port

- Fixed: Project name in the executable, dialogs and package. Run
  `BanjoTooieScurvyDogPort.exe`.

Saves and settings stay in the Alpha 4 profile folder; no migration is needed.
This release changes naming only.

## 0.1.0-alpha.4

- Changed: The player build now uses a new engine based on the recompiled
  original game code with a selected set of patches.
- Added: On first launch, a file picker asks for your ROM.
- Added: One-time offer to copy an existing Alpha 3 save; the original is
  never changed.
- Fixed: Shadow interpolation.
- Removed: The Alpha 3 launcher, Private Practice, cheats and travel tools are
  not part of the current player.

Known issues: Mayahem Temple can render black in the distance; audio may clip
at the Original output rate; menus and some animations are limited.

Upgrading: Alpha 3 settings, controls and practice states do not carry over.

See the [player guide](PLAYER_GUIDE.md) for setup, controls and
troubleshooting.

## Earlier releases

0.1.0-alpha.3 and earlier were Banjo-Tooie: Recompiled builds
(`TooieRecompiled.exe`) with a different launcher and feature set. Nothing in
them describes the current player. Their notes and guides are kept at the
[v0.1.0-alpha.3 tag](https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port/tree/v0.1.0-alpha.3)
and on its
[release page](https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port/releases/tag/v0.1.0-alpha.3).

## Credits and disclosure

This project uses [N64Recomp](https://github.com/N64Recomp/N64Recomp),
[N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime),
[RT64](https://github.com/rt64/rt64), [Dear ImGui](https://github.com/ocornut/imgui)
and other libraries listed in [third-party notices](../THIRD_PARTY_NOTICES.md).
Their contributors keep credit for their work. The project is independent and
has no upstream endorsement.

AI tools generated substantial portions of this project's code and assisted
with engineering and review. The project owner directed development, tested
the game and checked its behavior. See the
[contributor guide](../CONTRIBUTING.md).

Project material that its authors can license is under
[GPL-3.0-only](../LICENSE). Dependencies and other material keep their own
terms. This project is unofficial and is not affiliated with Nintendo or Rare.
