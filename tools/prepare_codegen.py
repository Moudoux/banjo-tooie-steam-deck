"""Regenerate CPU translations from an explicit user-owned NTSC-U ROM."""
from pathlib import Path
from elftools.elf.elffile import ELFFile
import argparse, hashlib, json, os, re, subprocess, sys, tempfile, toml
try:
    import tomllib
except ModuleNotFoundError:
    import tomli as tomllib
from prepare_core1_metadata import prepare_core1_metadata
from prepare_main_metadata import prepare_main_metadata
from instrument_continuous import instrument, WIDESCREEN_PROFILE_QUERY_HOOK
from instrument_audio_pacing import extract_audio_pacing_fixture
from prepare_core2_reference import prepare_core2_reference
from prepare_overlay_validation import prepare as prepare_overlay_validation
from prepare_boot_reference import prepare as prepare_boot_reference
from prepare_native_fixture import prepare as prepare_native_fixture
from prepare_ipl3_shape import prepare as prepare_ipl3_shape
from codegen_io import capture_output_times, restore_unchanged_times
from codegen_provenance import REQUIRED_INPUTS, write_receipt, generated_directory, configuration_directory, validate
from prepare_heap_patch import PATCH_INPUTS, REPLACEMENTS, prepare_heap_patch

ROM_SHA256='9ec37fba6890362eba86fb855697a9cff1519275531b172083a1a6a045483583'
DECOMP_SHA256='8c9d316b2edca686ec8393ddf95d480a7dfd9879ef434dbcfc4ea3000f2ffe89'

# @recomp: use the host return for saved-RA wrappers and cached RDRAM aliases
# for the two integrity loads. Delay slots and load consumers stay original.
INSTRUCTION_PATCHES = [
    ('func_800137D4', 0x800137EC, 0x00A00008, 0x03E00008),
    ('func_800137F4', 0x80013810, 0x00A00008, 0x03E00008),
    ('func_80013A5C', 0x80013A74, 0x00E00008, 0x03E00008),
    ('func_80013A7C', 0x80013A98, 0x00E00008, 0x03E00008),
    ('func_8001DE64', 0x8001DE70, 0x3C0EA030, 0x3C0E8030),
    ('func_8001DE64', 0x8001DE80, 0x3C18A030, 0x3C188030),
    ('func_80082088', 0x80082138, 0x8D2A0000, 0x340A0000),
    ('func_80082088', 0x80082144, 0x8D2A0000, 0x340A0000),
]

def validate_pinned_hashes():
    for label,value in [('ROM_SHA256',ROM_SHA256),('DECOMP_SHA256',DECOMP_SHA256)]:
        if len(value)!=64 or any(character not in '0123456789abcdef' for character in value):
            raise RuntimeError(f'invalid pinned SHA-256 literal {label}: {value!r}')

def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def require_hash(path, expected, label):
    actual=sha(path)
    if actual!=expected: raise RuntimeError(f'{label} identity mismatch: {path} ({actual})')
def relative(path,base): return Path(os.path.relpath(Path(path).resolve(),Path(base).resolve())).as_posix()
def run(command,cwd,log):
    log.parent.mkdir(parents=True,exist_ok=True)
    with log.open('w') as output:
        result=subprocess.run([str(x) for x in command],cwd=cwd,stdout=output,stderr=subprocess.STDOUT)
    if result.returncode: raise RuntimeError(f'command failed ({result.returncode}); see {log}')

def prepare_lift_bindings(app, generated, evidence):
    """Bind admission IDs to the actual composed corpus, without enabling admission."""
    lifted=evidence/'lifted'
    lifted.mkdir(parents=True,exist_ok=True)
    run([sys.executable,app/'tools/lift_persistent_continuations.py',
         '--source-dir',generated,'--output-dir',lifted],app,evidence/'logs/lift.log')
    coverage=json.loads((lifted/'persistent_continuation_coverage.json').read_text())
    names={record['name']:record['id'] for record in coverage['function_records']}
    graphics=['func_800A7840','func_8001A270','func_8001A1E0','func_80013D10',
              'func_80013DE4','func_80014148','func_800141D0','func_800140D8',
              'func_80013EF0','func_80013FE4','func_800142F0','func_800146D8']
    release=['func_800F3880','func_800F482C','func_800F759C','func_800A91F4',
             'func_80098E64','babackpack_entrypoint_3']
    header='#pragma once\n#include <array>\nnamespace tooie::render_wrapper_ids {\n'
    for key,group in [('graphics',graphics),('release_attachment',release)]:
        values=[names[name] for name in group]
        if len(values)!=len(set(values)) or not all(values):
            raise ValueError('Fresh graphics/release function identity collision')
        header+=f'inline constexpr std::array<unsigned,{len(values)}> {key}={{'+','.join(map(str,values))+'};\n'
    (generated/'render_wrapper_ids.h').write_text(header+'}\n')
    result={'program_sha256':coverage['program_sha256'],
            'functions':len(coverage['function_records']),
            'coverage_sha256':sha(lifted/'persistent_continuation_coverage.json'),
            'checkpoint_qualified':False}
    (generated/'render_wrapper_program.json').write_text(json.dumps(result,indent=2)+'\n')
    return result
def comparison_sha(path):
    data=Path(path).read_bytes()
    if Path(path).suffix.lower() not in {'.c','.cpp','.h','.hpp','.inl','.json','.toml','.txt'}:
        return hashlib.sha256(data).hexdigest()
    text=data.decode('utf-8')
    return hashlib.sha256(text.replace('\r\n','\n').encode()).hexdigest()
def generated_registry_count(path,expected_sections):
    text=Path(path).read_text()
    entry_pattern=(r'^    \{ \.func = [A-Za-z_]\w*, \.offset = 0x[0-9A-F]{8}, '
                   r'\.rom_size = 0x[0-9A-F]{8} \},$')
    entries=re.findall(entry_pattern,text,re.M)
    arrays=re.findall(r'^static FuncEntry .+_funcs\[\] = \{$',text,re.M)
    if len(entries)!=text.count('{ .func = '):
        raise RuntimeError(f'generated FuncEntry format drift: {path}')
    if len(arrays)!=expected_sections:
        raise RuntimeError(f'generated section registry mismatch: expected {expected_sections}, found {len(arrays)}')
    if not entries:raise RuntimeError(f'generated registry is empty: {path}')
    # register_overlays.cpp restores every sole-image replacement at its original address.
    return len(entries)+len(REPLACEMENTS)
def compare_reference(generated,reference):
    manifest=lambda root:{p.name:comparison_sha(p) for p in sorted(root.iterdir()) if p.is_file()}
    actual,expected=manifest(generated),manifest(reference)
    normalized_line_endings=[]
    for name in sorted(set(actual)&set(expected)):
        current,retained=generated/name,reference/name
        if actual[name]==expected[name] and sha(current)!=sha(retained):
            normalized_line_endings.append(name)
    # Fresh decomp checkouts can encode a different absolute build path in DWARF.
    # These fields document the ELF container. Generated text otherwise compares
    # exact content after CRLF-to-LF normalization; binary evidence stays byte exact.
    normalized_metadata=[]
    for name,field in [('core2-reference.json','elf_sha256'),
                       ('overlay-validation.json','source_elf_sha256')]:
        current,retained=generated/name,reference/name
        if current.is_file() and retained.is_file() and actual[name]!=expected[name]:
            current_data=json.loads(current.read_text());retained_data=json.loads(retained.read_text())
            current_value=current_data.pop(field,None);retained_value=retained_data.pop(field,None)
            if current_data==retained_data:
                actual[name]=expected[name]
                normalized_metadata.append(dict(file=name,field=field,
                    generated=current_value,reference=retained_value,
                    reason='ELF container hash may differ because debug paths are checkout-specific'))
    approved_new={'boot-reference.json','core1-expected.bin','native_fixture.h','ipl3_shape.hpp'}
    result=dict(reference=str(reference),expected_files=len(expected),generated_files=len(actual),
        missing=sorted(set(expected)-set(actual)),extra=sorted(set(actual)-set(expected)-approved_new),
        changed=sorted(n for n in set(actual)&set(expected) if actual[n]!=expected[n]),
        normalized_metadata=normalized_metadata,normalized_line_endings=normalized_line_endings)
    if result['missing'] or result['extra'] or result['changed']:
        raise RuntimeError(f'generated corpus differs from retained reference: {result}')
    return result

# The selected player consumes exactly these products; downstream preparers pin them.
PLAYER_INPUTS=('config/selected.syms.toml','config/overlay-ids.txt',
               'generated/core2-expected.bin','generated/heap-patch-products/heap.o')
def install_player_inputs(app,scratch):
    """Install absent player prerequisites; never replace or touch an existing file."""
    # Logical roots only: generated_directory alone would accept an alias into local/.
    for root in (app/'generated',app/'config'):
        if root.resolve()!=root:raise RuntimeError(f'{root} must not be an alias for --player-inputs')
    canonical={'config':configuration_directory(app),'generated':generated_directory(app)}
    rows,pending=[],[]
    # Preflight all four before any install.
    for name in PLAYER_INPUTS:
        area,rest=name.split('/',1)
        data=(scratch/area/rest).read_bytes();target=canonical[area]/rest
        # A symlinked parent (e.g. heap-patch-products) or a dangling link resolves elsewhere.
        if target.resolve()!=target or os.path.islink(target):
            raise RuntimeError(f'{name} destination does not resolve to its canonical path')
        if os.path.lexists(target):
            if not target.is_file() or target.read_bytes()!=data:
                raise RuntimeError(f'existing {name} differs from this preparation; --player-inputs never replaces it')
        else:pending.append((target,data))
        rows.append(dict(path=name,sha256=hashlib.sha256(data).hexdigest(),installed=not os.path.lexists(target)))
    for target,data in pending:
        target.parent.mkdir(parents=True,exist_ok=True)
        if target.parent.resolve()!=target.parent:
            raise RuntimeError(f'{target.parent} changed to an alias during installation')
        with target.open('xb') as output:output.write(data)  # exclusive create
    return dict(scratch=str(scratch),player_inputs=rows)

def main():
    # Imported metadata/instrumentation preparers check identity with assert.
    if not __debug__:  # also false when PYTHONOPTIMIZE is set
        raise SystemExit('prepare_codegen.py: required validation uses assert; run without -O and with PYTHONOPTIMIZE unset')
    validate_pinned_hashes()
    app=Path(__file__).resolve().parents[1]
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom',required=True,type=Path)
    p.add_argument('--decomp-root',required=True,type=Path)
    p.add_argument('--n64recomp',required=True,type=Path)
    p.add_argument('--evidence',required=True,type=Path)
    p.add_argument('--coverage',type=Path,default=app/'config/startup_coverage.json')
    p.add_argument('--work-dir',type=Path,help='Default .codegen; --player-inputs always uses its scratch root')
    p.add_argument('--reference-generated',type=Path)
    p.add_argument('--generated-dir',type=Path,
                   help='Bounded temporary output root; default is canonical generated')
    p.add_argument('--player-inputs',action='store_true',
                   help='Prepare only the selected player prerequisites in a fresh .codegen scratch root; '
                        'install each only if absent or byte-identical. No corpus, lifter or receipt.')
    a=p.parse_args();rom=a.rom.resolve();decomp=a.decomp_root.resolve();exe=a.n64recomp.resolve()
    evidence=a.evidence.resolve();coverage_file=a.coverage.resolve();work=(a.work_dir or app/'.codegen').resolve()
    if a.player_inputs and (a.generated_dir or a.work_dir or a.reference_generated):
        p.error('--player-inputs uses its own scratch root and has no reference comparison')
    if a.player_inputs and evidence.exists() and any(evidence.iterdir()):
        p.error('--player-inputs requires a fresh --evidence directory')
    generated=generated_directory(app,a.generated_dir)
    config_dir=configuration_directory(app,generated)
    source_config=app/'config'
    stable_inputs = [app / name for name in REQUIRED_INPUTS
                     if name.startswith('tools/') or name in {
                         'config/startup_coverage.json', 'config/boundary_adjustments.json',
                         'config/runtime_exclusions.json'}]
    stable_inputs += [app / name for name in PATCH_INPUTS]
    stable_hashes = {path: sha(path) for path in stable_inputs}
    elf=decomp/'build/us/banjotooie_decompressed.elf';decomp_rom=decomp/'decompressed.us.z64'
    linked_rom=decomp/'build/us/banjotooie_decompressed.z64';overlays_file=decomp/'overlays.us.toml'
    require_hash(rom,ROM_SHA256,'NTSC-U ROM');require_hash(decomp_rom,DECOMP_SHA256,'decompressed ROM')
    require_hash(linked_rom,DECOMP_SHA256,'rebuilt decompressed ROM')
    if not elf.is_file():raise FileNotFoundError(elf)
    with elf.open('rb') as stream:
        elf_file = ELFFile(stream)
        sections = [elf_file.get_section_by_name(name) for name in ('.core1', '.core2')]
        if any(section is None for section in sections):
            raise RuntimeError('Missing original instruction section')
        sections = [(section['sh_addr'], section.data()) for section in sections]
    for function, pc, original, replacement in INSTRUCTION_PATCHES:
        matches = [(base, data) for base, data in sections if base <= pc and pc + 4 <= base + len(data)]
        if len(matches) != 1:
            raise RuntimeError(f'instruction section ambiguity: {function} at {pc:08X}')
        base, data = matches[0]
        actual = int.from_bytes(data[pc-base:pc-base+4], 'big')
        if actual != original:
            raise RuntimeError(f'instruction drift: {function} at {pc:08X}: expected {original:08X}, found {actual:08X}')
    if not decomp_rom.read_bytes()==linked_rom.read_bytes():
        raise RuntimeError(f'rebuilt ROM differs from canonical decompressed ROM: {linked_rom}')
    for path in [exe,overlays_file,coverage_file,source_config/'boundary_adjustments.json',source_config/'runtime_exclusions.json']:
        if not path.is_file():raise FileNotFoundError(path)
    if a.player_inputs:
        # Every step below writes only into this fresh scratch root (the existing
        # bounded --generated-dir route); canonical files are compared at the end.
        if (app/'.codegen').resolve()!=app/'.codegen':
            raise RuntimeError('.codegen must not be an alias for --player-inputs scratch')
        (app/'.codegen').mkdir(exist_ok=True)
        generated=generated_directory(app,Path(tempfile.mkdtemp(prefix='player-inputs-',dir=app/'.codegen'))/'generated')
        config_dir=configuration_directory(app,generated);work=generated.parent/'work'
    evidence.mkdir(parents=True,exist_ok=True);(evidence/'logs').mkdir(exist_ok=True);work.mkdir(parents=True,exist_ok=True);generated.mkdir(parents=True,exist_ok=True);config_dir.mkdir(parents=True,exist_ok=True)
    if not a.player_inputs:
        previous=capture_output_times(app,generated,config_dir)
        (generated/'codegen-receipt.json').unlink(missing_ok=True)

    overlays=tomllib.loads(overlays_file.read_text())['overlay']
    if len(overlays)!=884:raise RuntimeError(f'expected 884 overlay slots, found {len(overlays)}')
    relocatable=work/'relocatable-sections.txt'
    relocatable.write_text(''.join(f'.{row["name"]}\n' for row in overlays if row.get('name') and not row.get('empty')))
    direct=work/'direct-elf.toml'
    direct.write_text(toml.dumps({'input':dict(entrypoint=0x80000400,elf_path=relative(elf,work),
        output_func_path='direct-output',relocatable_sections_path=relocatable.name,bss_section_suffix='_bss')}))
    run([exe,direct,'--dump-context'],work,evidence/'logs/dump-context.log')
    dump=work/'dump.toml'
    if not dump.is_file():raise RuntimeError(f'N64Recomp did not create {dump}')
    sections=tomllib.loads(dump.read_text())['section'];coverage=json.loads(coverage_file.read_text());names=[r['name'] for r in coverage['sections']]
    if len(names)!=len(set(names)):raise RuntimeError('duplicate coverage section')
    selected=[s for s in sections if s['name'] in names]
    if len(selected)!=len(names):raise RuntimeError(f'missing coverage sections: {sorted(set(names)-{s["name"] for s in selected})}')
    for adj in json.loads((source_config/'boundary_adjustments.json').read_text()):
        matching=[f for s in selected for f in s['functions'] if f['name']==adj['function']]
        if matching:
            if len(matching)!=1 or matching[0]['size']!=adj['original_size']:raise RuntimeError(f'boundary drift: {adj["function"]}')
            matching[0]['size']=adj['size']
        else:
            if adj['original_size']!=0:raise RuntimeError(f'missing boundary: {adj["function"]}')
            next(s for s in selected if s['name']==adj['section'])['functions'].append(dict(name=adj['function'],vram=adj['vram'],size=adj['size']))
    for section in selected:
        if section['name'] not in {'.boot','.core1','.core2'}:section.setdefault('relocs',[])
    renames=[]
    for section in selected:
        for function in section['functions']:
            if function['name']=='wait_one_frame':
                if function['vram']!=0x80015778:raise RuntimeError('wait_one_frame address drift')
                renames.append(dict(original='wait_one_frame',native_name='tooie_wait_one_frame',vram=function['vram'],reason='Guest/runtime name collision'));function['name']='tooie_wait_one_frame'
            if function['name']=='osInitialize' and function['vram']==0x8002D7B0:
                renames.append(dict(original='osInitialize',native_name='tooie_core1_osInitialize',vram=function['vram'],reason='Distinct core1 FINALROM/J instance'));function['name']='tooie_core1_osInitialize'
    if len(renames)!=2:raise RuntimeError(f'expected two renames: {renames}')
    selected_path=config_dir/'selected.syms.toml';selected_path.write_text(toml.dumps({'section':selected}))
    (config_dir/'symbol_renames.json').write_text(json.dumps(renames,indent=2)+'\n')
    stable=['*']+['.'+r['name'] if r.get('name') and not r.get('empty') and '.'+r['name'] in names else '*' for r in overlays]
    if stable[350]!='.chweldarbossfireball' or stable[679]!='.gcstatusDll':raise RuntimeError('overlay slot drift')
    overlay_ids=config_dir/'overlay-ids.txt';overlay_ids.write_text('\n'.join(stable)+'\n')
    coverage['selected_ids']={name:i for i,name in enumerate(stable) if name!='*'};(evidence/'startup-coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    if not a.player_inputs:
        prepare_core1_metadata(app,evidence,elf_path=elf,baseline_symbols=selected_path,generated_dir=generated);prepare_main_metadata(app,evidence,elf_path=elf,generated_dir=generated)
    prepare_core2_reference(app,evidence,rom_path=rom,decompressed_rom_path=decomp_rom,elf_path=elf,generated_dir=generated)
    if a.player_inputs:
        # The complete existing heap pipeline and its validators, in scratch.
        prepare_heap_patch(app,exe,selected,elf,evidence,generated_dir=generated)
        if any(sha(path) != digest for path, digest in stable_hashes.items()):
            raise RuntimeError('Codegen inputs changed during generation; rerun after edits freeze')
        print(json.dumps(install_player_inputs(app,generated.parent),indent=2))
        return 0
    prepare_overlay_validation(app.parent,generated/'overlay-validation.json',coverage=coverage_file,source=decomp,rom=rom,elf_path=elf)
    prepare_boot_reference(rom,decomp_rom,elf,generated)
    prepare_native_fixture(decomp,selected_path,generated/'native_fixture.h',elf)
    prepare_ipl3_shape(rom,generated/'ipl3_shape.hpp')

    exclusions=json.loads((source_config/'runtime_exclusions.json').read_text())
    for name in ('func_8001A3A0','func_8001A324','func_800A7840','func_800EBED4','func_8010A79C'):
        if exclusions.count(name)!=1:raise ValueError(f'Rendering replacement exclusion drift: {name}')
    replacement_names=set(REPLACEMENTS)
    ignored=list(dict.fromkeys(exclusions+sorted(replacement_names)))
    config={'input':dict(entrypoint=0x80000400,symbols_file_path=selected_path.name,rom_file_path=relative(decomp_rom,config_dir),
        output_func_path=relative(generated,config_dir),functions_per_output_file=50,relocatable_sections_path=overlay_ids.name,
        recomp_include='#include "recomp.h"\n#include "boot_hooks.h"'),
        'patches':{'ignored':ignored,
                   'instruction':[dict(func=f,vram=pc,value=value)
                                  for f,pc,original,value in INSTRUCTION_PATCHES]}}
    if config['patches']['ignored'].count('func_800E42F0') != 1:
        raise ValueError('HUD replacement exclusion drift')
    if config['patches']['ignored'].count('func_80015D14') != 1:
        raise ValueError('projection replacement exclusion drift')
    hooks=[(f,pc,f'tooie_boot_hook(rdram, ctx, {stage});') for f,pc,stage in [('func_80000450',0x80000450,0),('func_80000560',0x80000560,1),('func_80000560',0x800005E4,2),('func_80000450',0x80000518,3)]]
    hooks += [(f,pc,f'if (tooie_thread_hooks_active()) tooie_thread_hook(rdram, ctx, {stage});') for f,pc,stage in [('func_80012030',0x80012030,0),('func_80012030',0x80012054,1),('func_80012030',0x80012064,2),('func_8001DCB0',0x8001DCE4,3),('func_8001DCB0',0x8001DCEC,4),('func_80013620',0x80013660,5),('func_80013620',0x80013668,6)]]
    hooks += [(f,pc,f'if (tooie_main_hooks_active()) tooie_main_hook(rdram, ctx, {stage});') for f,pc,stage in [('func_80013678',0x80013678,0),('func_800131C0',0x800131E0,1),('func_800131C0',0x800131F8,2),('func_800131C0',0x8001320C,3),('func_800131C0',0x80013214,4),('func_8001DD28',0x8001DD5C,5),('func_8001DD28',0x8001DD64,6),('func_80012548',0x80012588,7),('func_80013678',0x80013698,8),('func_80013678',0x800136A0,9)]]
    hooks += [('rom_read_word',0x8001E210,'tooie_rom_read_word(rdram, ctx); return;')]
    hooks += [('func_80800034_glglobalsettings',0x80800044,
               WIDESCREEN_PROFILE_QUERY_HOOK)]
    hooks += [('func_80019EC0',pc,f'tooie_core2_hook(rdram, ctx, {stage});') for pc,stage in [(0x80019EC0,0),(0x80019F70,1),(0x80019F7C,2),(0x80019F9C,3),(0x80019FF0,4)]]
    hooks += [('func_800C2AB8',0x800C2B50,'if (ctx->r19 != 0) tooie_sfx_sweep_service(rdram, ctx);')]
    hooks += [
        ('func_80082088',0x80082088,'uint32_t tooie_overlay_header=(uint32_t)ctx->r4;\n    int32_t tooie_overlay_delta=(int32_t)ctx->r5;'),
        ('func_80082088',0x800821F4,'tooie_validate_overlay(rdram, tooie_overlay_header, tooie_overlay_delta);\n    tooie_overlay_after_relocate(rdram, tooie_overlay_header, tooie_overlay_delta);'),
        ('ovl_unload',0x80081A74,'tooie_overlay_before_free(rdram, (uint32_t)ctx->r4);'),
    ]
    hooks += [
        ('func_8001608C',0x8001608C,'tooie_replay_timing_begin_record();'),
        ('func_8001608C',0x800160F8,'tooie_replay_timing_record_divisor((uint32_t)MEM_W(0X50, ctx->r29), ctx->r2 != 0);'),
        ('func_800151BC',0x8001520C,'ctx->r2 = (gpr)(int32_t)tooie_replay_timing_scheduler_divisor((uint32_t)ctx->r2, 1);'),
        ('func_800151BC',0x8001524C,'ctx->r2 = (gpr)(int32_t)tooie_replay_timing_scheduler_divisor((uint32_t)ctx->r2, 0);'),
    ]
    hooks.append(('func_800123F4',0x80012460,'tooie_game_delta_observed((uint32_t)MEM_W(0, (gpr)(int32_t)0x8012C760U));'))
    hooks.append(('func_800A73F4',0x800A73F4,'tooie_guest_update_observed(MEM_H(0, (gpr)(int32_t)0x80127634U) != 0);'))
    hooks.append(('func_800C0F8C',0x800C0F8C,'tooie_pause_snapshot_capture();'))
    hooks.append(('gspropsDll_entrypoint_1',0x808001E0,'tooie_observe_map_actor_list(rdram, ctx, 0x808001E0u);'))
    hooks += [(name, pc, f'tooie_observe_menu(rdram, ctx, 0x{pc:08X}u);') for name, sites in [
        ('func_808006A0_chgameselect', (0x808006A0, 0x8080074C, 0x8080085C)),
        ('func_8080086C_chgameselect', (0x8080086C, 0x808008B0)),
        ('func_80800D3C_chgameselect', (0x80800D4C,)),
        ('func_80800E24_chgameselect', (0x80800EB0, 0x80801000, 0x80801014, 0x80801048)),
        ('func_808013C8_chgameselect', (0x8080140C,)),
    ] for pc in sites]
    config['patches']['hook']=[dict(func=f,before_vram=pc,text=text) for f,pc,text in hooks]
    config_path=config_dir/'tooie.us.toml';config_path.write_text(toml.dumps(config))
    (generated/'heap_patch.c').unlink(missing_ok=True)
    for stale in generated.glob('funcs_*.c'):stale.unlink()
    for name in ['funcs.h','lookup.cpp','recomp_overlays.inl']:
        path=generated/name
        if path.exists():path.unlink()
    run([exe,config_path],config_dir,evidence/'logs/codegen.log')
    counts={'expected_registry_entries':generated_registry_count(generated/'recomp_overlays.inl',len(selected)),
            'selected_sections':len(selected)}
    (generated/'coverage_metadata.json').write_text(json.dumps(counts,indent=2)+'\n')
    (generated/'coverage_metadata.hpp').write_text('#pragma once\n#include <cstddef>\nnamespace tooie::coverage_meta {\n'+''.join(f'inline constexpr std::size_t {k}={v};\n' for k,v in counts.items())+'}\n')
    pristine_sources=sorted(generated.glob('*.c'))
    fixture_dir=generated/'test-fixtures';fixture_dir.mkdir(exist_ok=True)
    (fixture_dir/'audio_pacing_functions.inc').write_text(extract_audio_pacing_fixture(pristine_sources))
    instrumentation=instrument(generated,selected,
                               guest_native_aspect=True)
    (evidence/'continuous-instrumentation.json').write_text(json.dumps(instrumentation,indent=2)+'\n')
    patch_inputs=prepare_heap_patch(app,exe,selected,elf,evidence,generated_dir=generated)
    projection_sources=[source.read_text() for source in generated.glob('*.c')]
    replacement_count=sum(len(re.findall(
        r'RECOMP_FUNC void func_80015D14\(', text)) for text in projection_sources)
    if (replacement_count != 1 or any(
            'tooie_widescreen_apply_profile(' in text or
            'tooie_widescreen_adjust_projection_aspect(' in text
            for text in projection_sources)):
        raise RuntimeError('guest projection image/one-adjuster composition drift')
    instrumentation['guest_native_aspect_replacements']=replacement_count
    (evidence/'continuous-instrumentation.json').write_text(
        json.dumps(instrumentation,indent=2)+'\n')
    lift_binding=prepare_lift_bindings(app,generated,evidence)
    comparison=compare_reference(generated,a.reference_generated.resolve()) if a.reference_generated else None
    inputs=[rom,decomp_rom,linked_rom,elf,overlays_file,coverage_file,source_config/'boundary_adjustments.json',source_config/'runtime_exclusions.json',selected_path,config_path,overlay_ids,config_dir/'symbol_renames.json',Path(__file__).resolve(),app/'tools/prepare_core1_metadata.py',app/'tools/prepare_main_metadata.py',app/'tools/instrument_continuous.py',app/'tools/instrument_audio_pacing.py',app/'tools/prepare_core2_reference.py',app/'tools/prepare_overlay_validation.py',app/'tools/prepare_boot_reference.py',app/'tools/prepare_native_fixture.py',app/'tools/prepare_ipl3_shape.py',app/'tools/codegen_io.py']
    receipt=dict(command=[str(exe),str(config_path)],generator_sha256=sha(exe),symbol_renames=renames,coverage=counts,
        instrumentation=instrumentation,inputs={str(x):sha(x) for x in inputs},reference_comparison=comparison,
        byte_identical_timestamps_preserved=restore_unchanged_times(previous))
    (evidence/'codegen.json').write_text(json.dumps(receipt,indent=2)+'\n')
    if any(sha(path) != digest for path, digest in stable_hashes.items()):
        raise RuntimeError('Codegen inputs changed during generation; rerun after edits freeze')
    write_receipt(app, inputs + patch_inputs + [exe] + [app / name for name in REQUIRED_INPUTS],
                  {'generation': 'full', 'command': receipt['command'],
                   'evidence_receipt_sha256': sha(evidence/'codegen.json'),
                   'configuration_dir': str(config_dir.resolve()),
                   'guest_native_aspect': True,
                   'lift_binding':lift_binding},generated_dir=generated)
    validate(app, generated_dir=generated)
    return 0
if __name__=='__main__':raise SystemExit(main())
