# Building Banjo-Tooie: Scurvy Dog Port on Windows

This is the maintained way to build the Scurvy Dog Port player (`BanjoTooieScurvyDogPort.exe`)
from source and, for maintainers, to package it. Players don't need any of it;
see the [player guide](PLAYER_GUIDE.md).

Status: these commands follow the current scripts, CMake presets and checks in
this repository. The step 5 evidence collector was run against the existing
Alpha 4 build and reproduced its archive hash. The full sequence has not been
re-run end to end on a fresh machine. Report gaps in the issue tracker.

## How the build fits together

1. **Windows:** fetch the pinned dependencies and Windows toolchain.
2. **WSL:** generate ROM-derived source from your own ROM, then prepare the
   selected player guest image and the native reference files.
3. **Windows:** configure, compile the raw game-code archive, qualify it once,
   then build the gated player executable.
4. **Maintainers:** create a build receipt, package and verify.

The raw archive holds the 417 C translation units containing the recompiled game functions. Qualification records
exactly which compile commands, sources, headers and objects produced it.
Every later build and the packager refuse to continue if any of them change. On
a fresh tree, `cmake --build --preset windows-encore` therefore fails until
step 5 has been done.

Everything generated stays in ignored folders: `deps/`, `generated/`,
`.venv-codegen/`, `.local-evidence/`, `build/` and `dist/`. Never commit or
share their contents or your ROM.

## Requirements

Windows:

- Windows 10 or 11 x64.
- Git for Windows with long paths enabled, Python 3.10+ on `PATH`, CMake 3.25+
  and Ninja.
- Visual Studio 2022 (or Build Tools) with **Desktop development with C++** and
  a Windows SDK. Run all Windows commands below in an **x64 Developer
  PowerShell for VS 2022** at the checkout root.

WSL:

- WSL 2 with Ubuntu. Keep the checkout on the Windows drive and reach it
  through `/mnt/c/...`. Guest preparation runs the pinned Windows LLVM tools
  through WSL interop, so leave interop enabled.
- Network access for the first run.

```bash
sudo apt-get update
sudo apt-get install -y build-essential binutils-mips-linux-gnu gcc-mips-linux-gnu \
  cmake ninja-build git python3 python3-venv python3-pip \
  libfmt-dev libtoml11-dev zlib1g-dev wget tar
```

You also need your own NTSC-U 1.0 big-endian Banjo-Tooie ROM.

## 1. Get the source and Windows dependencies

```powershell
git clone https://github.com/some-scurvy-dog/banjo-tooie-scurvy-dog-port.git
Set-Location .\banjo-tooie-scurvy-dog-port
python .\tools\bootstrap_dependencies.py --with-windows-toolchain
python .\tools\bootstrap_dependencies.py --with-windows-toolchain --verify
```

Dependencies are pinned in `dependencies.lock.json` and installed under `deps/`.
Do not edit those checkouts: packaging requires `deps/N64ModernRuntime` and
`deps/rt64` to be unmodified. The `recomp_frontend` entry is reference-only and
is not used by the build.

## 2. Supply and check your ROM

Copy your ROM to `baserom.us.z64` in the checkout root (this path is ignored)
and check it:

```powershell
(Get-FileHash -Algorithm SHA256 .\baserom.us.z64).Hash.ToLowerInvariant()
```

The result must be
`9ec37fba6890362eba86fb855697a9cff1519275531b172083a1a6a045483583`. Stop if it
differs.

## 3. Generate local source in WSL

In Ubuntu, from the same checkout:

```bash
cd /mnt/c/path/to/banjo-tooie-scurvy-dog-port
python3 tools/generate_local.py --rom baserom.us.z64 --jobs "$(nproc)"
.venv-codegen/bin/python tools/baseline/prepare_player_guest.py --rom baserom.us.z64
```

`generate_local.py` does the following:

1. Checks the ROM and copies it into `deps/banjo-tooie/`.
2. Fetches the Linux-side pinned tools and the decompilation.
3. Builds the decompilation and checks that the rebuilt ROM matches.
4. Builds the pinned N64Recomp and RSPRecomp.
5. Writes CPU and audio source to `generated/` and its receipts to
   `.local-evidence/`.

On later runs it reuses decompilation outputs that still verify.

`prepare_player_guest.py` writes fresh outputs under
`build/windows-dev/baseline/`:

- the raw game-code corpus;
- the boot and heap guest image;
- the selected player guest image and its layout;
- the core1 and core2 reference files;
- the IPL3 shape header.

It reads the ROM but never copies it. It refuses to run if any of its output
folders already exist, and stops if the resulting guest image, layout or shadow
policy differs from the selected player build.

## 4. Configure

Back in the Developer PowerShell:

```powershell
cmake --preset windows-encore
```

Configuration does the following:

- checks the step 3 outputs and the pinned runtime and source hashes;
- prepares the patched runtime sources inside the build folder;
- fails if step 3 is incomplete.

`windows-encore` uses `build/windows-dev`, the same folder as the older
`windows-dev` preset for the retired Recompiled frontend. Do not configure both
presets in one checkout.

## 5. Compile and qualify the raw archive

Compile only the raw archive, keeping its log. The log must come from the
build that actually compiled all 417 sources:

```powershell
New-Item -ItemType Directory build\windows-dev\raw-qualification-input | Out-Null
cmd /c "cmake --build build\windows-dev --target BaselineRecompiledFuncs > build\windows-dev\raw-qualification-input\compile.log 2>&1"
if ($LASTEXITCODE) { throw "Raw archive build failed; see compile.log" }
```

Next, collect the qualification evidence. This script takes the 417 raw compile
commands from `compile_commands.json`, checks the archive's members separately
with `llvm-ar`, and reads every header each object used from Ninja's
dependency records. It writes `evidence.json` and prints the archive's
SHA-256.

```powershell
$archiveSha = @'
import hashlib, json, re, subprocess
from pathlib import Path

build = Path("build/windows-dev").resolve()
raw = (build / "baseline/render-bridge-camera-candidate/composition-v2/raw").resolve()
archive = build / "BaselineRecompiledFuncs.lib"
llvm_ar = Path("deps/LLVM-19.1.3-Windows-X64/bin/llvm-ar.exe")
sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()

rows = [row for row in json.loads((build / "compile_commands.json").read_text())
        if Path(row["file"]).resolve().parent == raw]
assert len(rows) == 417, f"{len(rows)} raw compile commands"
objects = {row["output"].replace("\\", "/") for row in rows}
members = subprocess.check_output([str(llvm_ar), "t", str(archive)], text=True).splitlines()
assert len(members) == 417 and {m.replace("\\", "/") for m in members} == objects, "archive membership"

ninja = re.search(r"^CMAKE_MAKE_PROGRAM:\w+=(.*)$", (build / "CMakeCache.txt").read_text(), re.M)[1]
files = {Path(row["file"]).resolve() for row in rows} | {raw.parent / "RAW_BINDING.json"}
current, valid = False, 0
for line in subprocess.check_output([ninja, "-C", str(build), "-t", "deps"], text=True).splitlines():
    if line and not line[0].isspace():
        current = line.rsplit(": #deps", 1)[0].replace("\\", "/") in objects
        if current:
            assert "(VALID)" in line, line
            valid += 1
    elif current and line.strip():
        path = Path(line.strip())
        files.add((path if path.is_absolute() else build / path).resolve())
assert valid == 417, f"{valid} valid dependency records"

record = {"commands": rows, "files": [{"path": str(p), "sha256": sha(p)} for p in sorted(files)]}
with open(build / "raw-qualification-input/evidence.json", "x") as out:
    json.dump(record, out, indent=2)
print(sha(archive))
'@ | python -
if ($LASTEXITCODE -or -not $archiveSha) { throw "Raw evidence collection failed" }
```

Then qualify with the existing gate:

```powershell
python .\tools\baseline\verify_baseline_binding.py qualify `
  --build build\windows-dev `
  --raw build\windows-dev\baseline\render-bridge-camera-candidate\composition-v2\raw `
  --archive build\windows-dev\BaselineRecompiledFuncs.lib `
  --tools deps\LLVM-19.1.3-Windows-X64\bin `
  --snapshot build\windows-dev\baseline\raw-qualification\qualified.json `
  --expected-count 417 `
  --evidence build\windows-dev\raw-qualification-input\evidence.json `
  --compile-log build\windows-dev\raw-qualification-input\compile.log `
  --accepted-archive-sha $archiveSha
```

It checks all of the following:

- the evidence names exactly the current raw compile commands and sources,
  plus the raw binding file;
- every recorded file still has its recorded hash;
- the log shows each raw source compiled exactly once;
- the archive's bytes and members match;
- the compiled hook calls match the source.

If everything passes, it writes
`build\windows-dev\baseline\raw-qualification\qualified.json`. It refuses to
replace an existing qualification.

## 6. Build the player

```powershell
cmake --build --preset windows-encore
```

This builds the `TooieBaseline` target, whose output is named
`BanjoTooieScurvyDogPort.exe`:

- **Before linking:** it re-checks the archive against the qualification.
- **After linking:** it checks the executable's map and hook calls and writes
  `baseline\raw-qualification\native.json`.

The player and its `SDL2.dll`, `dxcompiler.dll` and `dxil.dll` are in
`build\windows-dev\baseline\player-bin\`. Running the game requires the
Microsoft Visual C++ x64 runtime.

## 7. Run a local test

Started without arguments, the player uses the real profile in
`%LOCALAPPDATA%\BanjoTooieEncore` and may offer to import an Alpha 3 save. For
testing, use a separate profile instead:

```powershell
.\build\windows-dev\baseline\player-bin\BanjoTooieScurvyDogPort.exe --profile-dir build\encore-test-profile --rom baserom.us.z64
```

The profile folder must be new, empty or an existing Scurvy Dog Port profile. Its
`encore.log` records startup details. To test a save import into that profile,
add `--import-save` with the path to a copy of a save; the import offer is not
shown with `--profile-dir`.

## Reusing or regenerating a build

- **Reuse:** after editing native source, run `cmake --build --preset
  windows-encore` again. The gates re-check the qualified archive on every
  build. If the raw sources, their headers or the archive change, the gate
  fails, and you need a fresh build folder.
- **Guest C edits:** changes under `tools/baseline/guest/` need steps 3–6
  in a fresh build folder. The selected image is pinned in
  `prepare_player_guest.py` (`IMAGE_SHA256`, `LAYOUT_SHA256`),
  `prepare_backpack621.py` (`SHADOW_COEFFICIENTS_IMAGE_SHA256`, checked by
  `prepare_camera_bridge.py`), `prepare_camera_bridge.py` (`render_bridge.c` source hash),
  `cmake/Baseline.cmake` (`_baseline_expected_image`), and
  `release/encore-windows-files.json` (`guest`). Preparation refuses a changed
  image until those pins are deliberately updated after reviewing the change.
  Do not replace a failing pin merely to make a build pass.
- **Regenerate on purpose:** guest preparation and qualification never
  overwrite earlier results. Move the old build folder aside instead of
  deleting it, then repeat steps 3 to 6:

```powershell
Rename-Item build\windows-dev windows-dev.previous
```

Do not use `--replace` to qualify an archive again unless you have rebuilt it
from scratch. The compile log has to come from the build that produced the
archive.

## Packaging (maintainers)

`release/encore-windows-files.json` declares the following:

- the release version and archive name;
- the expected build options;
- the selected guest image;
- the exact list of packaged files.

`tools/package_encore.py` never publishes anything. It requires all of the
following:

- a clean, committed checkout with no untracked files;
- a build folder configured from that checkout with the `windows-encore`
  options;
- unmodified dependency checkouts.

```powershell
git status --short --untracked-files=all
python .\tools\package_encore.py receipt
python .\tools\package_encore.py package --output-dir dist\scurvy-dog-alpha4.1
python .\tools\package_encore.py verify --output-dir dist\scurvy-dog-alpha4.1
```

The `git status` command must print nothing.

- **`receipt`** runs a real build of the player target, which does nothing if
  the build is already current. It confirms that the source and build options
  did not change during that build, then reruns the archive, native and
  guest-image checks. Finally it writes `encore-build-receipt.json` in the
  build folder. It will not replace an existing receipt unless you pass
  `--replace-receipt`; do that only after a fresh clean build.
- **`package`** recomputes the receipt and requires it to match the saved one.
  It then writes the allowlisted ZIP, with its manifest, and a `.sha256` file.
  It refuses to overwrite an existing package; use a new output folder.
- **`verify`** re-checks an existing ZIP and checksum against the current build.
  Run it again just before uploading.

An output folder inside the repository must be under `dist/`. For a new
version, update the following together:

- `release/encore-windows-files.json`;
- the `TOOIE_PLAYER_VERSION` value in the top-level CMake configuration;
- `release/README.encore.txt` (packaged as `README.txt`).

### Comparing hashes

Package hashes cover the exact bytes in the ZIP. Depending on Git's line-ending
settings, a Windows working copy can use CRLF where a Git source archive uses
LF, so text-file hashes can differ between them. Compare package files against
the package manifest, and source archives against their named commit. A
line-ending difference does not excuse any other difference.

## Troubleshooting

| Symptom | Likely cause |
| --- | --- |
| Configure says the IPL3, core1 or core2 reference is missing | Step 3 did not finish; check `prepare_player_guest.py` output. |
| `fresh build tree required` from guest preparation | Earlier outputs exist; reuse them or move the build folder aside. |
| `Missing/duplicate successful compile evidence` | The compile log is not from the build that compiled the 417 sources. Start from a fresh build folder. |
| `Archive member identity/size/hash mismatch` during the build | The raw archive or its inputs changed after qualification. |
| Packager says the source checkout is not clean | Commit or remove the changes; build outputs must stay in ignored folders. |
