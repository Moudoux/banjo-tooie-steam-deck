"""Authored exact-pin recipe, not executed by author. Default OFF writes nothing."""
import argparse
import hashlib
import json
import re
from pathlib import Path

PINS = {
    'gbi/rt64_gbi_rdp.cpp': '9eab7d0b8ba70f816c4cd873a535a50d01b4a0285d2726edf7256809299bae43',
    'hle/rt64_rsp.cpp': 'dc2c8b8d4c4a273e321a80d508c55064280b940472082eaeffc73409ce814c0e',
}
INCLUDE = '#include "camera_runtime_observer.hpp"\n'
def sha(data): return hashlib.sha256(data).hexdigest()
def once(text, old, new):
    assert text.count(old) == 1, ('exact observer hunk', old)
    return text.replace(old, new, 1)

def materialize(root, out, enabled, camera_enabled):
    if not enabled:
        # Integrator must retain original owners; no output/include/receipt is produced.
        return []
    assert camera_enabled, 'G6 observer requires camera candidate ON'
    here = Path(__file__).resolve().parent
    rows = []
    for name, expected in PINS.items():
        source = root / 'deps/rt64/src' / name
        original = source.read_bytes(); assert sha(original) == expected, (name, 'source pin')
        text = original.decode('utf-8'); normalized = text.replace('\r\n', '\n')
        text = normalized; inverse = []
        if name.startswith('gbi/'):
            old = '            state->fullSync();\n            state->dpInterrupt();'
            new = '            tooie_g6::sync_entry(state);\n            state->fullSync();\n            tooie_g6::sync_return(state);\n            state->dpInterrupt();'
            text = once(text, old, new); inverse.append((new, old))
        else:
            old = '                viewProjMatrix = hlslpp::mul(floatMatrix, viewProjMatrix);'
            new = '                tooie_g6::shadow_before_multiply(state, address, floatMatrix, viewProjMatrix);\n' + old
            text = once(text, old, new); inverse.append((new, old))
            old = '            projectionMatrixSegmentedAddress = address;\n            projectionMatrixPhysicalAddress = fromSegmentedMasked(address);\n            projectionMatrixChanged = true;\n            projectionMatrixInversed = false;'
            new = old + '\n            tooie_g6::projection(state, address, projectionMatrixPhysicalAddress, (params & loadMask) != 0, (params & loadMask) == 0);'
            text = once(text, old, new); inverse.append((new, old))
            old = '            stackChanged = true;\n        }\n    }\n\n    void RSP::popMatrixId'
            new = '            stackChanged = true;\n        }\n        tooie_g6::assignment(state, id, push, proj, idIsAddress, editGroup);\n    }\n\n    void RSP::popMatrixId'
            text = once(text, old, new); inverse.append((new, old))
        # Move-only materialization needs equivalent include resolution. Record exact inverse.
        include_inverses = []
        def absolute_include(match):
            old = match.group(0); path = (source.parent / match.group(1)).resolve()
            # Only relative includes existing next to the source. Search-root includes stay unchanged.
            if not path.is_file(): return old
            new = '#include "' + path.as_posix() + '"'
            include_inverses.append((new, old)); return new
        text = re.sub(r'#include "([^"]+)"', absolute_include, text)
        text = INCLUDE + text
        restored = text.removeprefix(INCLUDE)
        for new, old in reversed(include_inverses): restored = once(restored, new, old)
        for new, old in reversed(inverse): restored = once(restored, new, old)
        assert restored == normalized, 'Exact normalized inverse; no original call/control-flow change'
        restored_bytes = restored.replace('\n', '\r\n').encode() if b'\r\n' in original else restored.encode()
        assert restored_bytes == original, 'Exact byte inverse including original newline convention'
        out.mkdir(parents=True, exist_ok=True); target = out / source.name; target.write_bytes(text.encode())
        rows.append({'source': str(source), 'source_sha256': expected, 'output': str(target), 'output_sha256': sha(target.read_bytes()),
                     'normalized_inverse_equal': True, 'exact_byte_inverse_equal': True, 'relative_include_relocations': include_inverses})
    for name in ('camera_runtime_observer.hpp', 'camera_guest_summary.hpp', 'shadow_witness.hpp'):
        (out / name).write_bytes((here / name).read_bytes())
    (out / 'G6_MATERIALIZATION.json').write_text(json.dumps({'owners': rows, 'header_sha256': sha((here / 'camera_runtime_observer.hpp').read_bytes()),'shadow_header_sha256':sha((here/'shadow_witness.hpp').read_bytes()),
        'guest_summary_sha256': sha((here / 'camera_guest_summary.hpp').read_bytes()), 'scope': 'CPU parser observations only; no GPU completion'}, indent=2) + '\n')
    return rows

def materialize_guest_adapter(root, output, enabled, camera_enabled):
    """Optional integrator recipe: same pinned provider, one include and one read-only call."""
    if not enabled: return None
    assert camera_enabled, 'G6 guest summary requires camera candidate ON'
    source = root / 'tools/baseline/adapters.cpp'
    original = source.read_bytes()
    assert sha(original) == 'a159e6bc4845556ecc4ec138ec21f613c6e29a6d09353dc10b38ce17d1bfd736', 'adapter source pin'
    text = original.decode('utf-8').replace('\r\n', '\n'); before = text
    marker = '// BEGIN BASELINE_RENDER_PROVIDER -- extracted unchanged by the focused fixture.'
    include = '#if TOOIE_BASELINE_CAMERA_RUNTIME_OBSERVER\n#if !BASELINE_CAMERA_IDENTITY\n#error G6 observer requires camera candidate ON\n#endif\n#include "camera_guest_summary.hpp"\n#endif\n'
    text = once(text, marker, include + marker)
    old = '    }\n#endif\n}\n// END BASELINE_RENDER_PROVIDER'
    call = '    }\n#endif\n#if TOOIE_BASELINE_CAMERA_RUNTIME_OBSERVER\n    tooie_g6_guest::next_begin(rdram);\n#endif\n}\n// END BASELINE_RENDER_PROVIDER'
    text = once(text, old, call)
    restored = once(once(text, call, old), include + marker, marker)
    assert restored == before
    restored_bytes = restored.replace('\n', '\r\n').encode() if b'\r\n' in original else restored.encode()
    assert restored_bytes == original
    # OFF preprocessing removes both additions; original S1 provider bytes and epoch work remain.
    output.parent.mkdir(parents=True, exist_ok=True); output.write_bytes(text.encode())
    return {'source_sha256': sha(original), 'output_sha256': sha(output.read_bytes()), 'normalized_inverse_equal': True, 'exact_byte_inverse_equal': True,
            'call_boundary': 'after unchanged S1 receipt/epoch work, before caller separate camera begin thunk'}

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__); p.add_argument('--root', type=Path, required=True); p.add_argument('--output', type=Path, required=True)
    p.add_argument('--enabled', action='store_true'); p.add_argument('--camera-enabled', action='store_true'); a = p.parse_args()
    materialize(a.root.resolve(), a.output.resolve(), a.enabled, a.camera_enabled)

    guest = materialize_guest_adapter(a.root.resolve(), a.output.resolve() / "adapters.cpp", a.enabled, a.camera_enabled)
    if guest is not None:
        (a.output.resolve() / "G6_ADAPTER_MATERIALIZATION.json").write_text(json.dumps(guest, indent=2) + "\n", encoding="utf-8")
