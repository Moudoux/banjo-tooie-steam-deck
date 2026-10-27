# Release notes

## 0.1.0-alpha.4.5 — Banjo-Tooie: Scurvy Dog Port

- Added Master volume above Music volume. Both are saved with your
  settings.
- Fixed low-rate save-select animation.
- Changed renderer ownership and shared interpolation handling. Player testing has accepted ordinary
  models, split play, camera movement and widescreen zone transitions.
- Texture pack support remains in the source but is untested in this release.

Known issues: the backpack shadow can appear detached in some situations, and the warp
menu can jitter while it opens. A shadow issue on the tent slope and the
Clockwork Kazooie view are deferred. F6 fast-forward is not available, and the
full game has not been verified start to finish.

Upgrade: extract the full ZIP into a new folder. Existing Scurvy Dog Port saves
and settings carry over.

## 0.1.0-alpha.4.4 — Banjo-Tooie: Scurvy Dog Port

- Changed character interpolation to share consistent object, model-part and
  view identities across frames.
- Added floating-point position and motion output for CPU-deformed animation.
- Fixed split-player animation artifacts caused by Banjo and Kazooie using the
  wrong interpolation history.
- Improved camera/model correspondence and initial model-detail selection.
- Changed shadow cuts to preserve identity while skipping invalid history.
- Improved F4 evidence for character, shadow, menu and transition investigation.
- Added stable projection identities for animated menu models.
- Corrected a view-index bound that excluded the fifth perspective view from
  widescreen raster compensation.

Known issues under investigation: backpack shadows and a lower-priority shadow
issue while sliding on a slope. The menu-projection and transition-edge
corrections have passed focused tests and await player retesting. Remaining issues are carried forward for follow-up.

Upgrade: extract the full ZIP into a new folder. Existing Scurvy Dog Port saves
and settings carry over.

## 0.1.0-alpha.4.3 — Banjo-Tooie: Scurvy Dog Port

- Fixed a reported distant-geometry flicker.
- Fixed audio clicking caused by insufficient queue reserve.
- Changed output rate and resolution scale to apply during play.
- Added borderless fullscreen and stabilized the controls menu layout.
- Added an optional Talon Trot movement-sound mute that applies during play.
- Added experimental support for local RT64 `.rtz` texture packs.
- Added F4 issue markers with timestamps and settings in `player.log`.
- Changed texture detail scaling to follow render resolution.

Known issues at release: widescreen edges and aspect sizing, Mumbo demo shadows,
low-rate save-select animation, and controller input while unfocused. F6
fast-forward remained unavailable; reported player-model artifacts needed retesting.

Upgrade: Extract the full ZIP into a new folder. Existing Alpha 4.2 saves and
settings carry over. See the [published 4.3 notes](https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port/releases/tag/v0.1.0-alpha.4.3).

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
