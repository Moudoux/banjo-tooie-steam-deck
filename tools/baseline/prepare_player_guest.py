"""Maintained player guest recipe: baseline raw, boot/core1/heap, Silo, then the
selected 621 composition with the existing shadow policy, qualified hand labels
and a public refresh-rate command using the intended game/demo cadence.

Run under the existing WSL codegen environment after tools/prepare_codegen.py has
produced config/, generated/heap-patch-products/ and generated/core2-expected.bin
from the user's validated ROM (--rom) and decomp build. Every step writes only
fresh build/windows-dev/baseline outputs.

The Silo folder's legacy standalone image, data symbols and receipt are not
regenerated: they are imported byte-exact from --standalone-reference (see
STANDALONE_REFERENCE). This is a preserved-reference build, not a fresh reproduction.
"""
import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

BASELINE = 'build/windows-dev/baseline'
CAMERA = BASELINE + '/render-bridge-camera-candidate/composition-v2'
OUTPUTS = [BASELINE + '/raw', BASELINE + '/heap-patch', BASELINE + '/render-bridge-silo-candidate', CAMERA,
           BASELINE + '/core1-publication', BASELINE + '/core2-publication', BASELINE + '/ipl3-scatter']
# Explicit render-bridge flags; shadow_observer stays OFF. Quiet/Toggled Talon
# Trot follow backpack621 + shadow-coefficients + minimum-host in camera preparation.
# The receipt and independent image pins below identify the complete selection.
SELECTION = ['--actor-palette', '--demo-timing', '--shadow-fix', '--pre-background', '--cpu625',
             '--minimum-host', '--npc665', '--backpack621', '--shadow-coefficients', '--first-person-aim', '--flap-flip-landing']
# Historical quiet-only image: 3ab2614823ac33e1d6f271aeb0ba6a34f8457efcbf5b75da6d2c4b952f7c7e7e
PRE_MUSIC_IMAGE_SHA256 = '290755f1ecd95da325877c5dab99ed807c8bc373db04ec1c8a3a85fc633f6aaf'
PRE_MUSIC_IMAGE_BYTES = 320976
PRE_MUSIC_LAYOUT_SHA256 = 'b726ebea67ddb48241d9fe590a41048ca7d89c576158ac36297098526746b8c7'
# Unqualified music composition: root installs only measured, independently checked pins.
IMAGE_SHA256 = '6845c8ced20a737a46324e7d388cf9c73e07d6f32d8dda8a71467bb8661f5bc8'
IMAGE_BYTES = 1769616
LAYOUT_SHA256 = '528e3e1e2a368b93855ed07f496058d6410e585b636f77e2344ee2cfe0d908a9'
ROM_SHA256 = '9ec37fba6890362eba86fb855697a9cff1519275531b172083a1a6a045483583'
DECOMPRESSED_SHA256 = '8c9d316b2edca686ec8393ddf95d480a7dfd9879ef434dbcfc4ea3000f2ffe89'
# Native publication references that Baseline.cmake authenticates at configure.
CORE1_OBSERVATION = BASELINE + '/core1-publication/core1-observation.bin'
CORE1_RANGE = (0x01E29B60, 0x01E5AEB0)  # Decompressed ROM core1 slice, 201552 bytes.
CORE1_SHA256 = '965ccdb935542a113c1d58e12fccdb3ff326f3c02abebdda95cc7657403d7ad0'
CORE2_OBSERVATION = BASELINE + '/core2-publication/core2-observation.bin'
CORE2_SHA256 = '6522eefee489b670fb24e594642c5bfb693e639e2c6d82c40dc9fe2eaa12adc7'
IPL3_SHAPE = BASELINE + '/ipl3-scatter/ipl3_shape.hpp'
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
# G0 explicit standalone reference. prepare_camera_bridge.py checks the Silo folder's boot.bin as
# its fixed legacy premise and reads data.syms.toml and receipt.json from that folder as the inputs
# the selected image was qualified with. prepare_silo_bridge.py writes the current layout-5 image
# and cannot regenerate them, so the preserved files are imported unchanged and never replaced.
SILO = BASELINE + '/render-bridge-silo-candidate'
STANDALONE_REFERENCE = {
    'boot.bin': ('f4a5f108e0f342ed4512fab7e014d5f10b03ec1a2c26882eccc2c281b9c308ed', 54156),
    'data.syms.toml': ('79373a7a74b00a029547ff770db4208f0b85fca4d975a8a328d1bcec618a4c40', 2503),
    'receipt.json': ('c5252dc4b1a05ef1fa5af11db8d6e3370817ba1dc6d693ec1aea4d7cae61e5be', 10182),
}


def prepare_native_references(root, rom):
    """Core1/core2 observation bytes and the IPL3 shape from the validated ROM inputs."""
    if sha(rom) != ROM_SHA256:
        raise ValueError('NTSC-U ROM identity mismatch')
    decompressed = (root / 'deps/banjo-tooie/decompressed.us.z64').read_bytes()
    if hashlib.sha256(decompressed).hexdigest() != DECOMPRESSED_SHA256:
        raise ValueError('decompressed ROM identity mismatch')
    # Core2 is prepare_codegen's ELF-checked inflation of the ROM's compressed core2 streams.
    for name, data, digest in [(CORE1_OBSERVATION, decompressed[CORE1_RANGE[0]:CORE1_RANGE[1]], CORE1_SHA256),
                               (CORE2_OBSERVATION, (root / 'generated/core2-expected.bin').read_bytes(), CORE2_SHA256)]:
        if hashlib.sha256(data).hexdigest() != digest:
            raise ValueError(f'native reference differs from its pin: {name}')
        (root / name).parent.mkdir(parents=True)
        (root / name).write_bytes(data)
    sys.path.insert(0, str(root / 'tools'))
    from prepare_ipl3_shape import prepare as prepare_ipl3_shape
    (root / IPL3_SHAPE).parent.mkdir(parents=True)
    prepare_ipl3_shape(rom, root / IPL3_SHAPE)


def read_standalone_reference(reference):
    """Authenticate the preserved legacy Silo files in memory; nothing is written."""
    files = {}
    for name, (digest, size) in STANDALONE_REFERENCE.items():
        path = reference / name
        if path.is_symlink() or not path.is_file():
            raise ValueError(f'standalone reference {name} is missing or not a regular file: {path}')
        data = path.read_bytes()
        if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
            raise ValueError(f'standalone reference {name} is not the pinned legacy file: {path}')
        files[name] = data
    return files


def install_standalone_reference(root, reference, files):
    """Add the authenticated bytes to the fresh Silo folder; exclusive create, never replace."""
    silo = root / SILO
    occupied = [name for name in [*files, 'STANDALONE_REFERENCE.json']
                if (silo / name).exists() or (silo / name).is_symlink()]
    if occupied:
        raise FileExistsError(f'the Silo folder already holds {occupied}; the import never replaces')
    for name, data in files.items():
        with (silo / name).open('xb') as output:
            output.write(data)
    record = dict(scope='Imported preserved legacy standalone reference; not regenerated, not a fresh reproduction',
                  reference=str(reference), regenerated=False,
                  files={name: dict(sha256=digest, bytes=size) for name, (digest, size) in STANDALONE_REFERENCE.items()},
                  fresh_objects={name: sha(silo / name) for name in ('bridge.o', 'OBJECT_BINDING.json')})
    with (silo / 'STANDALONE_REFERENCE.json').open('x') as output:
        output.write(json.dumps(record, indent=2) + '\n')
    return record


def main():
    # The camera/render/Silo steps check identity with assert; never skip them.
    if not __debug__:  # also false when PYTHONOPTIMIZE is set
        raise SystemExit('prepare_player_guest.py: required validation uses assert; '
                         'run without -O and with PYTHONOPTIMIZE unset')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path.cwd())
    parser.add_argument('--rom', type=Path, required=True, help='User-supplied NTSC-U ROM; never copied')
    parser.add_argument('--standalone-reference', type=Path, required=sys.platform != 'linux',
                        help='Preserved Silo folder with the pinned legacy boot.bin, data.syms.toml and '
                             'receipt.json; read only. Linux fork: optional, omitted to regenerate them')
    args = parser.parse_args()
    root, rom = args.root.resolve(), args.rom.resolve()
    if sys.platform != 'linux':
        raise RuntimeError('Use the existing WSL codegen environment')
    for name in OUTPUTS:
        if (root / name).exists():
            raise ValueError(f'fresh build tree required; preserve existing {name}')
    # Authenticate the preserved legacy files before any output is written.
    # Linux fork: without a preserved folder the Silo step regenerates the standalone image, data symbols
    # and receipt from source, as Alpha 4.4 did (prepare_camera_bridge.CLEAN_STANDALONE_IMAGE_SHA256).
    # The gate below is unchanged: the selected image must still equal IMAGE_SHA256.
    reference = args.standalone_reference.resolve() if args.standalone_reference else None
    legacy = read_standalone_reference(reference) if reference else None
    prepare_native_references(root, rom)
    tools = root / 'tools/baseline'
    def step(script, *args):
        subprocess.run([sys.executable, str(tools / script), '--root', str(root), *args], cwd=root, check=True)
    step('prepare_baseline_raw.py')
    step('prepare_boot_heap.py')
    # Fresh raw and only the bridge.o that camera Honey containment compares. The selected camera
    # path compiles its own owner objects (prepare_camera_bridge.py) and never reads the standalone
    # bone/pair/silo/player/actor/leaf/lifecycle objects, whose historical OFF pins stay with --objects;
    # the standalone image is the imported legacy reference.
    if reference:
        step('prepare_silo_bridge.py', '--raw', '--bridge-object', '--no-prior-honey')
        standalone = install_standalone_reference(root, reference, legacy)
    else:
        step('prepare_silo_bridge.py', '--raw', '--image', '--no-prior-honey')
        standalone = dict(regenerated=True, files={name: dict(sha256=sha(root / SILO / name))
                                                   for name in STANDALONE_REFERENCE})
    # The 621 image proves its existing owner objects unchanged against a same-selection OFF carrier.
    sys.path.insert(0, str(tools))
    import prepare_camera_bridge as camera_owner
    camera = root / CAMERA
    camera.mkdir(parents=True)
    def run(command, label):
        with (camera / label).open('w') as log:
            subprocess.run(list(map(str, command)), cwd=root, stdout=log, stderr=subprocess.STDOUT, check=True)
    # P04a/BK: the carrier's CPU625 object must use the image's ckpack copy-storage selection, which
    # prepare_camera_bridge.prepare derives: the default BK pipeline (CPU625 + Flap Flip) retires P04b,
    # so the selected image compiles CPU625 without copy storage. The 621 custody check
    # (prepare_image, "621 unchanged existing owner object") then compares like objects.
    # L06c-2/L06c-4: under that BK selection neither the carrier nor the image builds an NPC665 or CPU625
    # object, so the custody check compares the player object only; BK-OFF carriers keep both custodies.
    camera_owner.compile_backpack621_carrier(root, camera, run, actor_palette=True, pre_background=True,
                                             cpu625=True, npc665=True, ckpack_copy_storage=False,
                                             bk_pipeline=True)
    step('prepare_render_bridge.py', '--camera', '--raw', '--image', *SELECTION)
    image, receipt = camera / 'boot.bin', json.loads((camera / 'receipt.json').read_text())
    if (sha(image) != IMAGE_SHA256 or image.stat().st_size != IMAGE_BYTES or
            sha(camera / 'render_bridge_layout.h') != LAYOUT_SHA256 or receipt.get('shadow_coefficients') is not True or receipt.get('talon_trot_toggle') is not True or receipt.get('first_person_aim') is not True or receipt.get('flap_flip_landing') is not True):
        raise ValueError('selected player guest image/layout/policy differs')
    record = dict(selection=SELECTION, image_sha256=IMAGE_SHA256, image_bytes=IMAGE_BYTES,
                  layout_sha256=LAYOUT_SHA256, raw_binding_sha256=sha(camera / 'RAW_BINDING.json'),
                  receipt_sha256=sha(camera / 'receipt.json'),
                  baseline_raw=json.loads((root / BASELINE / 'raw-receipt.json').read_text())['outputs'],
                  native_references={name: sha(root / name) for name in (CORE1_OBSERVATION, CORE2_OBSERVATION, IPL3_SHAPE)})
    record['standalone_reference'] = standalone
    (camera / 'PLAYER_GUEST.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps({k: record[k] for k in ('image_sha256', 'image_bytes', 'layout_sha256')}))


if __name__ == '__main__':
    main()
