"""Extract pinned pristine audio bodies for local inverse tests only.
P06I guest observation insertion is retired; no generated transform.
"""
import hashlib
import re
TARGETS={
 'func_800128C0':'568516b89c471deb6de5bc32b6457e05456652077e301f0c0968031091025be1',
 'func_800129FC':'bd969df88a0deb42afb5b487d66c0938d243b63e869d69b72308363a31b66abd',
}
FUNCTION_PATTERN=r'RECOMP_FUNC void (\w+)\([^\n]*\n.*?^;}\n'

def extract_audio_pacing_fixture(paths):
    """Extract the pristine target bodies for local pristine/inverse regression tests."""
    found={}
    for path in paths:
        text=path.read_text()
        for match in re.finditer(FUNCTION_PATTERN,text,re.S|re.M):
            name=match[1]
            if name not in TARGETS:continue
            if name in found:raise ValueError(f'duplicate audio pacing function {name}: {path}')
            found[name]=match[0]
    if set(found)!=set(TARGETS):raise ValueError(f'missing audio pacing fixture functions: {sorted(set(TARGETS)-set(found))}')
    for name,pin in TARGETS.items():
        if hashlib.sha256(found[name].encode()).hexdigest()!=pin:
            raise ValueError(f'changed audio pacing fixture body {name}')
    return ''.join(found[name] for name in TARGETS)
