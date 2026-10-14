"""Materialize two pinned, fixture-only observations; never edit their inputs."""
import argparse
import hashlib
import json
from pathlib import Path


def sha(data):
    return hashlib.sha256(data).hexdigest()


def observed(source, expected, prefix, anchor, call):
    original = source.read_bytes()
    if sha(original) != expected:
        raise ValueError(f"Selected input hash differs: {source}")
    if original.count(anchor) != 1 or prefix in original or call in original:
        raise ValueError(f"Observation anchor is not unique/clean: {source}")
    child = prefix + original.replace(anchor, anchor + call, 1)
    inverse = child[len(prefix):].replace(anchor + call, anchor, 1)
    if not child.startswith(prefix) or inverse != original:
        raise ValueError(f"Observation inverse differs: {source}")
    return child, {
        "source": str(source), "source_sha256": sha(original),
        "observed_sha256": sha(child), "inverse_exact": True,
        "prefix_hex": prefix.hex(), "anchor_hex": anchor.hex(),
        "insert_after_anchor_hex": call.hex(),
        "inserted_nonblank_lines": sum(bool(line.strip()) for line in (prefix + call).splitlines()),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--binary-root", type=Path, required=True)
    parser.add_argument("--output-directory", type=Path, required=True)
    args = parser.parse_args()
    root = args.source_root.resolve()
    binary = args.binary_root.resolve()
    output = args.output_directory.resolve()
    if binary != (root / "build/windows-dev").resolve() or not binary.is_dir():
        raise ValueError("Use the existing build/windows-dev tree")
    if output != (binary / "baseline/wait-save-composition").resolve():
        raise ValueError("Fixture children must remain in the nominated build subtree")
    specs = [
        (binary / "baseline/lifecycle/mesgqueue.cpp", "mesgqueue.cpp",
         "ed3f3d6e2ad4894e3648143eb6375aa7ca67cb5df4c1e2bc394033f6915d1cee",
         b'extern "C" void tooie_wait_save_fixture_timeout(int) noexcept;\n',
         b"        while (!external_messages.wait_dequeue_timed(to_send, std::chrono::milliseconds{10})) {\n",
         b"            tooie_wait_save_fixture_timeout(osGetThreadId(rdram, 0));\n"),
        (root / "src/runtime_lifecycle.cpp", "runtime_lifecycle.cpp",
         "d665d74b3bad8e239ed404bf2d9b95b8970fd0a310cecad97dfc3d6d600c0146",
         b'#include <cstddef>\nextern "C" void tooie_wait_save_fixture_quiescence(bool, std::size_t) noexcept;\n',
         b"bool wait_for_shutdown_quiescence(std::chrono::milliseconds timeout) {\n    std::unique_lock lock(owner_mutex);\n",
         b"    tooie_wait_save_fixture_quiescence(stopping_flag.load(std::memory_order_acquire), producers.size());\n"),
    ]
    products, records = {}, []
    for source, name, expected, prefix, anchor, call in specs:
        child, record = observed(source, expected, prefix, anchor, call)
        record["observed"] = str(output / name)
        products[name] = child
        records.append(record)
    receipt = {"status": "FIXTURE_ONLY_EXACT_INVERSES", "production_source_changes": 0,
               "observations": 2, "files": records}
    products["manifest.json"] = (json.dumps(receipt, indent=2) + "\n").encode("utf-8")
    # Validate all destinations before writing; repeat materialization is idempotent.
    for name, data in products.items():
        destination = output / name
        if destination.resolve().parent != output:
            raise ValueError("Fixture output escapes its directory")
        if destination.exists() and destination.read_bytes() != data:
            raise ValueError(f"Refusing to overwrite different fixture output: {destination}")
    output.mkdir(parents=True, exist_ok=True)
    for name, data in products.items():
        destination = output / name
        if not destination.exists():
            destination.write_bytes(data)
        if destination.read_bytes() != data:
            raise ValueError(f"Written output differs: {destination}")
    for record in records:
        if sha(Path(record["source"]).read_bytes()) != record["source_sha256"]:
            raise ValueError("Selected input changed during materialization")
    print(json.dumps({"status": receipt["status"], "manifest": str(output / "manifest.json"),
                      "manifest_sha256": sha(products["manifest.json"])}))


if __name__ == "__main__":
    main()
