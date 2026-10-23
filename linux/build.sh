#!/usr/bin/env bash
# Linux build of the Scurvy Dog Port player, run inside the tooie-toolchain image:
#
#   docker build -t tooie-toolchain:latest .
#   docker run --rm -it -v "$(pwd)":/workspace tooie-toolchain:latest linux/build.sh
#
# Follows docs/WINDOWS.md steps 1-4 and 6. Guest preparation runs the pinned
# Windows LLVM under Wine (see linux/shims), so the guest image is checked
# against the same pins as the Windows build. The Windows-only raw archive
# qualification (step 5) and packaging are skipped.
#
# Stages are skipped when their outputs already exist. Set CLEAN=1 to move
# earlier build folders aside and start over (needed after guest patch edits).
#
# TARGET=deck (default) tunes for the Steam Deck's Zen 2 CPU and gives new
# profiles Deck defaults (3x, display rate, 16:10, fullscreen).
# TARGET=generic builds for any x86-64 CPU with the upstream defaults.
# SYMBOLS=1 keeps debug info and symbols (for crash backtraces), and
# BUILD=<dir> builds into another folder, e.g. build/linux-player-debug.
set -euo pipefail
cd "$(dirname "$0")/.."
ROOT=$PWD
ROM=${ROM:-baserom.us.z64}
JOBS=${JOBS:-$(nproc)}
BUILD=${BUILD:-build/linux-player}
TARGET=${TARGET:-deck}
# No FMA contraction: fused multiply-adds would round guest float math
# differently from the N64 and the Windows build.
if [[ ${SYMBOLS:-0} == 1 ]]; then BUILD_TYPE=RelWithDebInfo STRIP=""; else BUILD_TYPE=Release STRIP=" -s"; fi
case $TARGET in
    deck) ARCH_FLAGS="-march=znver2 -mtune=znver2 -ffp-contract=off"; DEFAULTS="-DTOOIE_DECK_DEFAULTS=1" ;;
    generic) ARCH_FLAGS="-ffp-contract=off"; DEFAULTS="" ;;
    *) echo "TARGET must be deck or generic" >&2; exit 2 ;;
esac
GUEST=build/windows-dev/baseline   # Fixed output location of the guest preparation scripts.

export PATH="$ROOT/linux/shims/bin:$PATH"
export PYTHONPATH="$ROOT/linux/shims/python${PYTHONPATH:+:$PYTHONPATH}"

if [[ ${CLEAN:-0} == 1 ]]; then
    stamp=$(date +%Y%m%d-%H%M%S)
    for dir in build/windows-dev "$BUILD"; do
        [[ -e $dir ]] && mv "$dir" "$dir.previous-$stamp"
    done
fi

echo "== 1. Pinned dependencies"
python3 tools/bootstrap_dependencies.py
# The Windows LLVM archive is marked Windows-only, but guest preparation pins
# and runs its clang.exe/ld.lld.exe. Install it with the bootstrap's own checks.
python3 - <<'EOF'
import json, pathlib, sys
sys.path.insert(0, 'tools')
import bootstrap_dependencies as bootstrap
spec = json.loads(pathlib.Path('dependencies.lock.json').read_text())['dependencies']['llvm_windows']
bootstrap.install_archive('llvm_windows', spec, pathlib.Path('deps').resolve(), pathlib.Path('downloads').resolve())
EOF

echo "== 2-3. ROM check and local source generation"
python3 tools/generate_local.py --rom "$ROM" --jobs "$JOBS"
if [[ ! -e $GUEST/render-bridge-camera-candidate/composition-v2/PLAYER_GUEST.json ]]; then
    .venv-codegen/bin/python tools/baseline/prepare_player_guest.py --rom "$ROM"
fi

echo "== 4. Configure"
if [[ ! -e $BUILD/baseline ]]; then
    mkdir -p "$BUILD"
    cp -a "$GUEST" "$BUILD/baseline"
fi
# Reuse the maintained windows-player selection so it follows upstream changes.
mapfile -t CACHE_ARGS < <(python3 - "$ROOT" "$BUILD" <<'EOF'
import json, sys
root, build = sys.argv[1:]
presets = {p['name']: p for p in json.load(open('CMakePresets.json'))['configurePresets']}
def variables(name):
    preset = presets[name]
    result = {}
    parents = preset.get('inherits', [])
    for parent in [parents] if isinstance(parents, str) else parents:
        result.update(variables(parent))
    result.update(preset.get('cacheVariables', {}))
    return result
skip = {'CMAKE_C_COMPILER', 'CMAKE_CXX_COMPILER', 'CMAKE_MT', 'CMAKE_CXX_FLAGS'}
for key, value in variables('windows-player').items():
    if key not in skip:
        value = value.replace('${sourceDir}/build/windows-dev', '${sourceDir}/' + build)
        print(f'-D{key}={value.replace("${sourceDir}", root)}')
EOF
)
# libc++ like MSVC's STL aliases high_resolution_clock to steady_clock, which the
# pinned timer patch requires. It is linked statically so the binary runs
# without libc++ installed on the host.
# Release (-O3, stripped) replaces the preset's RelWithDebInfo unless SYMBOLS=1.
cmake -S . -B "$BUILD" -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_FLAGS="$ARCH_FLAGS" -DCMAKE_CXX_FLAGS="-stdlib=libc++ $ARCH_FLAGS $DEFAULTS" \
    -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++ -static-libstdc++$STRIP" \
    -DPython3_EXECUTABLE="$ROOT/.venv-codegen/bin/python" "${CACHE_ARGS[@]}" -DCMAKE_BUILD_TYPE=$BUILD_TYPE

echo "== 6. Build the player"
cmake --build "$BUILD" --target TooieBaseline -j "$JOBS" ${BUILD_ARGS:-}
# Fallback controller mappings, read from beside the executable.
cp recompcontrollerdb.txt "$BUILD/baseline/player-bin/"
echo "Built $BUILD/baseline/player-bin/BanjoTooieScurvyDogPort (TARGET=$TARGET)"
