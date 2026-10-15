# Contributing

Bug reports, focused fixes and documentation corrections are welcome. For a
larger change, open an issue first so its goal and scope can be agreed.

## Report a problem

Use the
[issue tracker](https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port/issues).
Include the Scurvy Dog Port version, Windows version, GPU and driver, controller,
relevant settings, where in the game it happened and steps to reproduce.
Describe what you expected, what happened and whether it repeats.

The game writes `encore.log` and `encore-previous.log` in its profile folder
(see the [player guide](docs/PLAYER_GUIDE.md)). Logs can contain usernames and
local paths: review and redact them before attaching. Never attach a ROM, save
file, generated game code or data, credentials or a whole profile. If a private
file seems necessary, ask a maintainer about a safe way to share it first.

## Propose a change

A pull request should explain the player-facing or developer-facing result,
list the areas changed, and state the exact checks you ran and what you
observed. Do not describe a build, test or gameplay check as passing unless you
ran it. A successful build does not show that every area, controller, setting
or GPU works.

Keep the public tree to player, contributor, build and licensing material.
Working notes, plans, investigations and status logs belong in your own ignored
files. `docs/` only holds the files listed in `tools/check_repository.py`.

## Build and check

Follow the [Windows build guide](docs/WINDOWS.md). Run
`python tools/check_repository.py` for the ROM-free source checks that also run
in CI. For code changes, build the player and test the affected area. For
documentation changes, check commands, labels and links against the current
source.

Local generation uses your own supported ROM. Do not commit or attach the ROM,
`generated/`, `deps/`, build output, profiles, saves, logs or captures.

Release packages are prepared by maintainers with `tools/package_encore.py`
from the exact file list in `release/encore-windows-files.json`. That list is
not permission to share other generated files or a build folder.

## Code, assets and attribution

Contribute only material you have the right to submit. Keep copyright notices
and attribution, and explain the source and license of any copied or adapted
code or assets. Do not submit ROM data, extracted game assets or other
copyrighted game material. See [LICENSE](LICENSE) and
[third-party notices](THIRD_PARTY_NOTICES.md).

This project builds on N64Recomp, N64ModernRuntime, RT64, Dear ImGui and other
projects whose licenses and contribution rules remain their own. It is
independent and has no upstream endorsement.

## AI assistance

AI tools generated substantial portions of this project's code and assisted
with engineering and review; the project owner directs development and tests
the results. Contributors are responsible for the provenance, correctness,
licensing and testing of their own submissions. Disclose material AI assistance
in your pull request, and do not send AI-generated work to an upstream project
whose policy prohibits it.

Attribute commits truthfully: name an AI agent as author only for material it
actually wrote, and keep the person who reviewed and integrated the work as
committer. For work shared between a person and an agent, keep the person as
author and credit the agent with a `Co-authored-by:` trailer.
