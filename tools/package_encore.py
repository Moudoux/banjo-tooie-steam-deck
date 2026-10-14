#!/usr/bin/env python3
"""Bind, assemble and verify the Banjo-Tooie: Encore Windows ZIP; never publishes it.

  receipt  After a clean build of this committed checkout, run every gate and
           write the path-free build receipt (default <build>/encore-build-receipt.json).
  package  Recompute the receipt, require it to equal the saved one, then write
           the exact-allowlist ZIP and <zip>.sha256.
  verify   Recompute the receipt and re-check an existing ZIP and checksum.

The executable embeds no source revision. The binding is a clean committed
checkout, a build configured from that checkout and brought current by a real
build of the gated target (source and selection rechecked after it), the
existing raw-archive/native gate, the embedded-guest gate and the pinned guest.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import stat
import struct
import subprocess
import sys
import time
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "baseline"))
import verify_baseline_binding as raw_gate  # noqa: E402
import verify_guest_binding as guest_gate  # noqa: E402

ALLOWLIST = ROOT / "release" / "encore-windows-files.json"
RECEIPT_NAME = "encore-build-receipt.json"
RECEIPT_SCHEMA = "banjo-tooie-encore-build-receipt/1"
MANIFEST = "package-manifest.json"
MANIFEST_SCHEMA = "banjo-tooie-encore-package/1"
TRUE = {"ON", "TRUE", "YES", "Y", "1"}
FALSE = {"OFF", "FALSE", "NO", "N", "0", ""}
# Player startup and the functional boot-initialization chain (BOOT_OBSERVER is part of it).
REQUIRED_ON = ("TOOIE_BASELINE", "TOOIE_BASELINE_PLAYER", "TOOIE_BASELINE_BOOT_PI",
               "TOOIE_BASELINE_BOOT_OBSERVER", "TOOIE_BASELINE_CORE1_PUBLICATION")
# Never shipped: ROMs, saves, Python, build/debug artifacts, logs, source config.
FORBIDDEN_SUFFIXES = {".z64", ".n64", ".v64", ".rom", ".bin", ".eep", ".sav", ".srm",
                      ".py", ".pdb", ".map", ".lib", ".obj", ".log", ".toml", ".ini"}
# A current build finishes in seconds; a genuine corpus rebuild stays bounded.
BUILD_TIMEOUT = 4 * 60 * 60


def need(ok: object, why: str) -> None:
    if not ok:
        raise ValueError(why)


def digest(path: Path) -> str:
    result = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            result.update(chunk)
    return result.hexdigest()


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def encode(value: dict) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")


def safe_relative(text: str) -> PurePosixPath:
    path = PurePosixPath(text)
    if (not text or "\\" in text or ":" in text or "//" in text or str(path) != text or path.is_absolute()
            or any(part in ("", ".", "..") for part in path.parts)):
        raise ValueError(f"Unsafe relative path: {text!r}")
    return path


def is_reparse(path: Path) -> bool:
    if not os.path.lexists(path):
        return False
    details = os.lstat(path)
    return path.is_symlink() or bool(getattr(details, "st_file_attributes", 0)
                                     & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400))


def git(*args: str, cwd: Path = ROOT) -> str:
    return subprocess.run(["git", *args], cwd=cwd, capture_output=True, encoding="utf-8",
                          check=True, timeout=300).stdout


def load_allowlist() -> dict:
    allow = read_json(ALLOWLIST)
    need(allow.get("schema") == 1, "Unrecognized Encore package allowlist")
    need(allow["tag"] == "v" + allow["version"], "Release tag must be v<version>")
    need(re.fullmatch(r"[A-Za-z0-9._-]+\.zip", allow["archive"]), "Unsafe archive name")
    rules = allow["cmake"]
    need(all(name in rules["on"] for name in REQUIRED_ON),
         "Allowlist must require player startup and boot initialization ON")
    named = [*rules["on"], *rules["off"], *rules["exact"]]
    need(len(named) == len(set(named)), "CMake selection names a flag twice")
    seen: set[str] = set()
    for row in allow["files"]:
        path = safe_relative(row["path"])
        key = str(path).lower()
        need(key not in seen and key != MANIFEST.lower(), f"Duplicate or reserved package path: {path}")
        seen.add(key)
        need(path.suffix.lower() not in FORBIDDEN_SUFFIXES, f"Package path type is never shipped: {path}")
        need(row["from"] in ("build", "source"), f"Unknown package origin: {path}")
        need(row["from"] == "source" or "source" in row, f"Build row needs a source: {path}")
    executable = [row for row in allow["files"] if row["path"] == allow["executable"]]
    need(len(executable) == 1 and executable[0]["from"] == "build"
         and executable[0]["source"] == allow["build"]["executable"],
         "Allowlisted executable must be the gated build executable")
    return allow


def tracked_sources() -> dict[str, str]:
    rows: dict[str, str] = {}
    for entry in git("ls-files", "-s", "-z").split("\0"):
        if not entry:
            continue
        meta, name = entry.split("\t", 1)
        mode, object_id, stage = meta.split()
        need(stage == "0", f"Unmerged source path: {name}")
        if mode == "160000":
            rows[name] = "gitlink:" + object_id
            continue
        need(mode in ("100644", "100755"), f"Unsupported tracked source mode {mode}: {name}")
        path = ROOT.joinpath(*safe_relative(name).parts)
        need(path.is_file() and not is_reparse(path), f"Missing or unsafe tracked source: {name}")
        rows[name] = digest(path)
    return rows


def source_identity() -> dict:
    need(not git("status", "--porcelain=v1", "--untracked-files=all").strip(),
         "Source checkout is not clean; commit the selected release source and rebuild")
    files = tracked_sources()
    listing = "".join(f"{files[name]}  {name}\n" for name in sorted(files))
    return {"commit": git("rev-parse", "HEAD").strip(), "tree": git("rev-parse", "HEAD^{tree}").strip(),
            "trackedCount": len(files), "trackedSha256": hashlib.sha256(listing.encode("utf-8")).hexdigest(),
            "tracked": files}


def cmake_cache(build: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in (build / "CMakeCache.txt").read_text(encoding="utf-8").splitlines():
        match = re.fullmatch(r"([A-Za-z0-9_.+-]+):[A-Z]+=(.*)", line)
        if match:
            values[match[1]] = match[2]
    return values


def selected_options(cache: dict[str, str], rules: dict) -> dict[str, str]:
    actual: dict[str, str] = {}
    for name in rules["on"]:
        need(cache.get(name, "").upper() in TRUE, f"Player build requires {name}=ON")
        actual[name] = "ON"
    for name in rules["off"]:
        # Absent is OFF in CMake; retired selections are not cache entries in a fresh tree.
        need(cache.get(name, "").upper() in FALSE, f"Player build requires {name}=OFF")
        actual[name] = "OFF"
    for name, wanted in rules["exact"].items():
        need(cache.get(name) == wanted, f"Player build requires the declared {name}")
        actual[name] = wanted
    return actual


def build_target(cache: dict[str, str], build: Path, target: str) -> None:
    # A Ninja dry run cannot execute CMake's CONFIGURE_DEPENDS glob check, so it always
    # plans a regeneration. A real build runs that check, rebuilds anything genuinely
    # stale from this clean checkout and reruns the always-on archive gate.
    result = subprocess.run([cache["CMAKE_COMMAND"], "--build", str(build), "--target", target],
                            capture_output=True, encoding="utf-8", errors="replace", timeout=BUILD_TIMEOUT)
    need(result.returncode == 0,
         "Build of the gated target failed: " + (result.stdout + result.stderr).strip()[-2000:])


def checkout_commit(path: Path) -> str:
    need(path.is_dir(), f"Missing dependency checkout: {path.name}")
    top = Path(git("rev-parse", "--show-toplevel", cwd=path).strip()).resolve()
    need(top == path.resolve(), f"Dependency is not its own Git checkout: {path.name}")
    need(not git("status", "--porcelain=v1", "--untracked-files=no", cwd=path).strip(),
         f"Dependency checkout has local changes: {path.name}")
    return git("rev-parse", "HEAD", cwd=path).strip()


def in_build(build: Path, relative: str) -> Path:
    path = build.joinpath(*safe_relative(relative).parts)
    need(path.is_file() and not is_reparse(path) and path.resolve().is_relative_to(build),
         f"Missing or unsafe build input: {relative}")
    return path


def build_selection(build: Path, allow: dict) -> tuple[dict[str, str], dict[str, str]]:
    cache = cmake_cache(build)
    need(Path(cache.get("CMAKE_HOME_DIRECTORY", "")).resolve() == ROOT,
         "Build directory was configured from a different source checkout")
    need(Path(cache.get("CMAKE_CACHEFILE_DIR", "")).resolve() == build, "CMake cache belongs to another build directory")
    need(cache.get("CMAKE_GENERATOR") == "Ninja", "Receipt build requires the single-config Ninja generator")
    return cache, selected_options(cache, allow["cmake"])


def compute_receipt(build: Path, allow: dict) -> tuple[dict, dict[str, Path]]:
    source = source_identity()
    cache, options = build_selection(build, allow)
    spec = allow["build"]
    build_target(cache, build, spec["target"])
    # The build may regenerate; it must not change the source or the selected configuration.
    need(source_identity() == source, "Source changed during the build; commit, rebuild and rerun")
    cache, rebuilt_options = build_selection(build, allow)
    need(rebuilt_options == options, "CMake selection changed during the build")
    deps = Path(cache["TOOIE_DEPS_ROOT"]).resolve()
    dependencies = {name: checkout_commit(deps.joinpath(*safe_relative(relative).parts))
                    for name, relative in sorted(allow["dependencyCheckouts"].items())}

    exe, map_path, archive, snapshot_path, native_path, guest, layout = (
        in_build(build, spec[key]) for key in ("executable", "map", "archive", "snapshot",
                                               "nativeReceipt", "guest", "layout"))
    raw = build.joinpath(*safe_relative(spec["raw"]).parts).resolve()
    need(raw.is_dir() and raw.is_relative_to(build), "Selected raw corpus is missing or outside the build")
    count = allow["rawFunctionCount"]
    gate = argparse.Namespace(build=build, raw=raw, archive=archive, exe=exe, map=map_path,
                              snapshot=snapshot_path, expected_count=count,
                              tools=deps.joinpath(*safe_relative(spec["llvmTools"]).parts))
    snapshot = read_json(snapshot_path)
    need(len(snapshot["objects"]) == count, "Qualified raw object count differs from the selected corpus")
    raw_gate.check_archive(gate, snapshot)
    native = raw_gate.check_native(gate, snapshot)
    need(native == read_json(native_path), "Native/archive/qualification receipt is stale; rebuild")
    bound = guest_gate.check(exe, map_path, guest, layout)
    need(bound["guest_sha256"] == allow["guest"]["sha256"] and bound["embedded_bytes"] == allow["guest"]["bytes"],
         "Embedded guest differs from the selected release guest")
    need(bound["exe_sha256"] == native["exe_sha256"] and bound["map_sha256"] == native["map_sha256"],
         "Guest and native gates inspected different files")

    payload: dict[str, str] = {}
    origins: dict[str, Path] = {}
    for row in allow["files"]:
        name = row["path"]
        if row["from"] == "build":
            path = in_build(build, row["source"])
            if "sameAsDependency" in row:
                original = deps.joinpath(*safe_relative(row["sameAsDependency"]).parts)
                need(original.is_file() and digest(original) == digest(path),
                     f"{name} differs from its pinned dependency copy")
        else:
            relative = str(safe_relative(row.get("source", name)))
            need(relative in source["tracked"] and not source["tracked"][relative].startswith("gitlink:"),
                 f"Package source is not a tracked file: {relative}")
            path = ROOT.joinpath(*PurePosixPath(relative).parts)
        payload[name] = digest(path)
        origins[name] = path
    need(payload[allow["executable"]] == native["exe_sha256"], "Packaged executable is not the gated executable")

    receipt = {
        "schema": RECEIPT_SCHEMA,
        "version": allow["version"],
        "source": source,
        "dependencies": dependencies,
        "build": {"generator": "Ninja", "buildType": cache.get("CMAKE_BUILD_TYPE", ""),
                  "target": spec["target"], "options": options},
        "native": native,
        "rawFunctionCount": count,
        "guest": {key: bound[key] for key in ("guest_sha256", "embedded_bytes", "layout_sha256",
                                              "state_offset", "state_vram")},
        "payload": payload,
    }
    return receipt, origins


def require_same(saved: dict, current: dict) -> None:
    if saved != current:
        keys = sorted(key for key in set(saved) | set(current) if saved.get(key) != current.get(key))
        raise ValueError(f"Build receipt is stale or from another build ({', '.join(keys)}); "
                         "rebuild this checkout and rerun receipt")


def manifest(receipt: dict, allow: dict) -> bytes:
    return encode({
        "schema": MANIFEST_SCHEMA,
        "product": allow["product"],
        "version": allow["version"],
        "tag": allow["tag"],
        "sourceCommit": receipt["source"]["commit"],
        "sourceTree": receipt["source"]["tree"],
        "trackedSourceCount": receipt["source"]["trackedCount"],
        "trackedSourceSha256": receipt["source"]["trackedSha256"],
        "dependencyCommits": receipt["dependencies"],
        "rawFunctionCount": receipt["rawFunctionCount"],
        "selectedGuest": {"sha256": receipt["guest"]["guest_sha256"], "bytes": receipt["guest"]["embedded_bytes"]},
        "nativeGate": receipt["native"],
        "buildReceiptSha256": hashlib.sha256(encode(receipt)).hexdigest(),
        "files": receipt["payload"],
    })


def output_paths(output_dir: Path, allow: dict, build: Path) -> tuple[Path, Path]:
    need(not any(is_reparse(path) for path in (output_dir, *output_dir.parents)), "Output path crosses a reparse point")
    output = output_dir.resolve()
    need(output != ROOT and not output.is_relative_to(build), "Output must be separate from source and build")
    need(not output.is_relative_to(ROOT) or output.is_relative_to(ROOT / "dist"),
         "A repository-local output must be under dist/")
    return output / allow["archive"], output / (allow["archive"] + ".sha256")


def zip_entry(name: str, when: tuple) -> zipfile.ZipInfo:
    info = zipfile.ZipInfo(name, date_time=when)
    info.compress_type = zipfile.ZIP_DEFLATED
    info.create_system = 0
    info.external_attr = 0
    return info


def write_zip(target: Path, origins: dict[str, Path], payload: dict[str, str], manifest_bytes: bytes,
              when: tuple) -> None:
    with zipfile.ZipFile(target, "x") as archive:
        for name in sorted(set(origins) | {MANIFEST}):
            if name == MANIFEST:
                archive.writestr(zip_entry(name, when), manifest_bytes)
                continue
            # Hash exactly the bytes written so a concurrently changed input cannot slip in.
            result = hashlib.sha256()
            with origins[name].open("rb") as source, archive.open(zip_entry(name, when), "w") as sink:
                for chunk in iter(lambda: source.read(1024 * 1024), b""):
                    result.update(chunk)
                    sink.write(chunk)
            need(result.hexdigest() == payload[name], f"Input changed while packaging: {name}")


def verify_zip(path: Path, payload: dict[str, str], manifest_bytes: bytes) -> None:
    expected = dict(payload, **{MANIFEST: hashlib.sha256(manifest_bytes).hexdigest()})
    with zipfile.ZipFile(path) as archive:
        infos = archive.infolist()
        names = [info.filename for info in infos]
        need(len(names) == len(set(names)) and set(names) == set(expected),
             f"ZIP entries differ: missing={sorted(set(expected) - set(names))}, "
             f"extra={sorted(set(names) - set(expected))}")
        for info in infos:
            safe_relative(info.filename)
            need(not info.is_dir() and not info.flag_bits & 0x1, f"Unexpected ZIP entry form: {info.filename}")
            result = hashlib.sha256()
            with archive.open(info) as data:  # Reading checks CRC.
                for chunk in iter(lambda: data.read(1024 * 1024), b""):
                    result.update(chunk)
            need(result.hexdigest() == expected[info.filename], f"ZIP entry hash mismatch: {info.filename}")


def run(args: argparse.Namespace) -> None:
    allow = load_allowlist()
    build = args.build_dir.resolve()
    receipt_path = args.receipt if args.receipt else build / RECEIPT_NAME
    current, origins = compute_receipt(build, allow)
    summary = {"status": "PASS", "mode": args.mode, "version": allow["version"],
               "sourceCommit": current["source"]["commit"], "guestSha256": current["guest"]["guest_sha256"],
               "exeSha256": current["native"]["exe_sha256"]}
    if args.mode == "receipt":
        receipt_path.parent.mkdir(parents=True, exist_ok=True)
        with receipt_path.open("wb" if args.replace_receipt else "xb") as result:
            result.write(encode(current))
        print(json.dumps(summary))
        return
    need(receipt_path.is_file(), "Build receipt missing; run receipt after a clean build")
    require_same(read_json(receipt_path), current)
    manifest_bytes = manifest(current, allow)
    zip_path, sum_path = output_paths(args.output_dir, allow, build)
    if args.mode == "package":
        partial = zip_path.with_name(zip_path.name + ".partial")
        need(not any(os.path.lexists(path) for path in (zip_path, sum_path, partial)),
             "Refusing to replace an existing package; choose a new --output-dir")
        zip_path.parent.mkdir(parents=True, exist_ok=True)
        stamp = int(git("show", "-s", "--format=%ct", current["source"]["commit"]).strip())
        when = max(time.gmtime(stamp)[:6], (1980, 1, 1, 0, 0, 0))
        try:
            write_zip(partial, origins, current["payload"], manifest_bytes, when)
            verify_zip(partial, current["payload"], manifest_bytes)
        except BaseException:
            partial.unlink(missing_ok=True)
            raise
        os.replace(partial, zip_path)
        archive_sha = digest(zip_path)
        with sum_path.open("x", encoding="ascii", newline="\n") as result:
            result.write(f"{archive_sha}  {zip_path.name}\n")
    else:
        need(zip_path.is_file() and sum_path.is_file(), "Package or checksum file missing")
        verify_zip(zip_path, current["payload"], manifest_bytes)
        archive_sha = digest(zip_path)
        need(sum_path.read_text(encoding="ascii") == f"{archive_sha}  {zip_path.name}\n",
             "Checksum file differs from the archive")
    print(json.dumps(dict(summary, archive=zip_path.name, archiveSha256=archive_sha,
                          files=len(current["payload"]) + 1)))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("mode", choices=("receipt", "package", "verify"))
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build" / "windows-dev")
    parser.add_argument("--receipt", type=Path, help="Default: <build-dir>/" + RECEIPT_NAME)
    parser.add_argument("--output-dir", type=Path, default=ROOT / "dist" / "encore")
    parser.add_argument("--replace-receipt", action="store_true", help="Only after a fresh clean build")
    args = parser.parse_args()
    try:
        run(args)
    except (ValueError, KeyError, OSError, struct.error, subprocess.SubprocessError) as error:
        detail = (getattr(error, "stderr", None) or "").strip()
        print(json.dumps({"status": "FAIL", "error": f"{type(error).__name__}: {error} {detail}".strip()}))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
