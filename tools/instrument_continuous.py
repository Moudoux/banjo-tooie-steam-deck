"""Apply bounded application adaptations to generated code.

Add cooperative cancellation, observations and native hardware reads. This is
an application post-generation pass, not a change to N64Recomp or the ROM.
"""
import hashlib
import re
from pathlib import Path

_GPRS = 'zero at v0 v1 a0 a1 a2 a3 t0 t1 t2 t3 t4 t5 t6 t7 s0 s1 s2 s3 s4 s5 s6 s7 t8 t9 k0 k1 gp sp fp ra'.split()

def callsite_observations(lines, stubs, source='<generated>'):
    """Map executable calls to their own instruction plus delay-slot comments.

    This only reads generated text. A previous JAL is never used as a fallback.
    JR returns and jump-table switches have no executable lookup call and are
    deliberately excluded. Pin the LOOKUP_FUNC register to the original operand.
    """
    observations = {}
    recent = []
    function = ''
    for index, line in enumerate(lines):
        start = re.match(r'RECOMP_FUNC void (\w+)\(', line)
        if start:
            function = start[1]
            recent = []
        instruction = re.match(r'\s*// 0x([0-9A-Fa-f]+):\s+(\w+)\s*(.*)', line)
        if instruction:
            recent = (recent + [(int(instruction[1],16), instruction[2], instruction[3].strip())])[-2:]
        direct = re.fullmatch(r'\s*(\w+)\(rdram, ctx\);\s*', line)
        lookup = re.fullmatch(r'\s*LOOKUP_FUNC\((ctx->r\d+|0x[0-9A-Fa-f]+)\)\(rdram, ctx\);\s*', line)
        if 'LOOKUP_FUNC' in line and not line.lstrip().startswith('//'):
            if not lookup:
                raise ValueError(f'{source}:{index+1} {function}: unmatched executable lookup: {line.strip()}')
        if lookup or (direct and direct[1] in stubs):
            where = f'{source}:{index+1} {function}'
            if len(recent) != 2 or recent[1][0] != recent[0][0] + 4:
                raise ValueError(f'{where}: call lacks adjacent original instruction/delay-slot PCs: {recent}')
            pc, opcode, operand = recent[0]
            if lookup and lookup[1].startswith('ctx->r'):
                register = int(lookup[1][6:])
                original_register = operand.split(',')[-1].strip().removeprefix('$')
                if opcode not in ('jalr','jr') or not 0 <= register < 32 or original_register != _GPRS[register] or (opcode == 'jr' and register == 31):
                    raise ValueError(f'{where}: lookup register/control transfer mismatch: {recent[0]}, {lookup[1]}')
                kind = 'indirect_jalr' if opcode == 'jalr' else 'indirect_tail_jr'
            else:
                if opcode not in ('jal','j','bal','b'):
                    raise ValueError(f'{where}: named/constant call lacks a direct transfer: {recent[0]}')
                target = re.fullmatch(r'(?:0x|L_)([0-9A-Fa-f]+)', operand)
                expected_target = stubs[direct[1]] if direct else int(lookup[1],16)
                if not target or int(target[1],16) != expected_target:
                    raise ValueError(f'{where}: generated call target disagrees with original transfer: {recent[0]}')
                kind = 'direct_stub' if direct else 'direct_lookup'
            observations[index] = (pc, kind)
        # Even an unobserved ordinary generated call consumes its origin.
        # Authored tooie_* helpers may sit between a generated transfer's two
        # comments and its generated C call, so they preserve that provenance.
        # Labels/control boundaries still prevent inheritance by a later call.
        if ((direct and not direct[1].startswith('tooie_')) or lookup or
                re.match(r'^\s*(?:L_[0-9A-Fa-f]+:|after_\d+:|return;|;}|goto\s)', line)):
            recent = []
    return observations


WIDESCREEN_PROFILE_QUERY_HOOK = (
    'uint32_t tooie_widescreen_settings = tooie_widescreen_latched_settings_word();\n'
    '    if (tooie_widescreen_settings & 1u)\n'
    '        ctx->r2 = (gpr)(int32_t)((tooie_widescreen_settings >> 1) & 1u);')


def verify_widescreen_profile_source(text, source='<generated>'):
    """Verify the supported v0 producer hook; never rewrite its body."""
    if 'tooie_widescreen_apply_profile(' in text:
        raise ValueError(f'{source}: retired native profile writer')
    matches = list(re.finditer(
        r'RECOMP_FUNC void func_80800034_glglobalsettings\([^\n]*\n.*?^;}\n',
        text, re.S | re.M))
    if not matches:
        if 'tooie_widescreen_latched_settings_word(' in text:
            raise ValueError(f'{source}: settings query outside profile producer')
        return 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate global-settings import function')
    body = matches[0][0]
    expected = ('    ' + WIDESCREEN_PROFILE_QUERY_HOOK + '\n'
                '    // 0x80800044: jal         0x808003C8\n'
                '    // 0x80800048: or          $a0, $v0, $zero\n'
                '    ctx->r4 = ctx->r2 | 0;\n'
                '    glglobalsettings_entrypoint_4(rdram, ctx);\n')
    if (body.count(expected) != 1 or
            text.count('tooie_widescreen_latched_settings_word(') != 1):
        raise ValueError(f'{source}: profile source hook/one-adjuster drift')
    return 1


def verify_guest_native_aspect_source(text, source='<generated>'):
    """The compiled guest replacement owns projection arithmetic."""
    if (re.search(r'RECOMP_FUNC void func_80015D14\(', text) or
            'tooie_widescreen_adjust_projection_aspect(' in text):
        raise ValueError(f'{source}: retired projection owner in generated corpus')
    return 0


def observe_graphics(text, source='<generated>'):
    """Five const observations, scoped to original core1 call+delay pairs."""
    targets={
        'func_80014A88':(
            '    // 0x80014AA4: jal         0x8002E1F0\n'
            '    // 0x80014AA8: addiu       $a2, $zero, 0x1\n'
            '    ctx->r6 = ADD32(0, 0X1);\n',
            '    osRecvMesg_recomp(rdram, ctx);\n',0x80014AAC,True),
        'func_80013E34':(
            '    // 0x80013ECC: jal         0x8002F1F4\n'
            '    // 0x80013ED0: or          $a0, $s0, $zero\n'
            '    ctx->r4 = ctx->r16 | 0;\n',
            '    osSpTaskStartGo_recomp(rdram, ctx);\n',0x80013ECC,False),
        'func_80013EF0':(
            '    // 0x80013F7C: jal         0x8002F1F4\n'
            '    // 0x80013F80: or          $a0, $s0, $zero\n'
            '    ctx->r4 = ctx->r16 | 0;\n',
            '    osSpTaskStartGo_recomp(rdram, ctx);\n',0x80013F7C,False),
        'func_80013FE4':(
            '    // 0x80014070: jal         0x8002F1F4\n'
            '    // 0x80014074: or          $a0, $s0, $zero\n'
            '    ctx->r4 = ctx->r16 | 0;\n',
            '    osSpTaskStartGo_recomp(rdram, ctx);\n',0x80014070,False),
        'func_800146D8':(
            '    // 0x800147A8: jal         0x8002F1F4\n'
            '    // 0x800147AC: addiu       $a0, $a0, -0xC10\n'
            '    ctx->r4 = ADD32(ctx->r4, -0XC10);\n',
            '    osSpTaskStartGo_recomp(rdram, ctx);\n',0x800147A8,False),
    }
    found={}
    for match in re.finditer(r'RECOMP_FUNC void (\w+)\([^\n]*\n.*?^;}\n',text,re.S|re.M):
        if match[1] in targets:
            if match[1] in found:raise ValueError(f'{source}: duplicate graphics observation function {match[1]}')
            found[match[1]]=match[0]
    for name,body in found.items():
        if 'tooie_observe_graphics' in body:raise ValueError(f'{source}: graphics function already instrumented: {name}')
        prefix,call,pc,after=targets[name];anchor=prefix+call
        if body.count(anchor)!=1:raise ValueError(f'{source}: changed original graphics call/delay anchor in {name}')
        hook=f'    tooie_observe_graphics(rdram, ctx, 0x{pc:08X}u);\n'
        replacement=prefix+(call+hook if after else hook+call)
        text=text.replace(body,body.replace(anchor,replacement,1),1)
    return text,len(found)


def observe_title(text, source='<generated>'):
    """Nine bounded title/attract timing observations at original instruction anchors."""
    targets={
        'func_808005AC_chintroticker':[
            ('    // 0x80800614: lbu         $v0, 0x79($s3)\n    ctx->r2 = MEM_BU(ctx->r19, 0X79);\n',0x80800614,'before'),
            ('    // 0x80800644: beq         $v0, $zero, L_80800694\n    if (ctx->r2 == 0) {\n',0x80800644,'before'),
            ('    // 0x80800654: bne         $v0, $zero, L_80800694\n    if (ctx->r2 != 0) {\n',0x80800654,'before'),
            ('    // 0x80800664: bne         $v0, $s0, L_80800694\n    if (ctx->r2 != ctx->r16) {\n',0x80800664,'before'),
            ('    // 0x8080066C: jal         0x80088180\n    // 0x80800670: nop\n\n    _gcfrontend_entrypoint_10(rdram, ctx);\n',0x8080066C,'call'),
            ('    // 0x808006C4: swc1        $f10, 0x2C($s3)\n    MEM_W(0X2C, ctx->r19) = ctx->f10.u32l;\n',0x808006C4,'after')],
        'func_80800738_gcfrontend':[
            ('    // 0x808007AC: jal         0x800A791C\n    // 0x808007B0: addiu       $a2, $zero, 0x1\n    ctx->r6 = ADD32(0, 0X1);\n    func_800A791C(rdram, ctx);\n',0x808007AC,'call')],
        'gsworldDll_entrypoint_2':[
            ('    // 0x808002C4: sh          $a1, 0x2($a0)\n    MEM_H(0X2, ctx->r4) = ctx->r5;\n',0x808002C4,'after')],
        'gsattract_entrypoint_1':[
            ('    // 0x80800050: swc1        $f8, 0x4($a1)\n    MEM_W(0X4, ctx->r5) = ctx->f8.u32l;\n',0x80800050,'after')],
    }
    found={};count=0
    for match in re.finditer(r'RECOMP_FUNC void (\w+)\([^\n]*\n.*?^;}\n',text,re.S|re.M):
        if match[1] in targets:
            if match[1] in found:raise ValueError(f'{source}: duplicate title observation function {match[1]}')
            found[match[1]]=match[0]
    for name,original in found.items():
        if 'tooie_observe_title' in original:raise ValueError(f'{source}: title function already instrumented: {name}')
        body=original
        if name=='func_808005AC_chintroticker':
            for guard in [
                '    // 0x808005C4: jal         0x800EA05C\n    // 0x808005C8: sw          $s0, 0x14($sp)\n    MEM_W(0X14, ctx->r29) = ctx->r16;\n    func_800EA05C(rdram, ctx);\n',
                '    // 0x8080063C: jal         0x800DA298\n    // 0x80800640: addiu       $a0, $zero, 0xD56\n    ctx->r4 = ADD32(0, 0XD56);\n    func_800DA298(rdram, ctx);\n',
                '    // 0x8080064C: jal         0x800A8264\n    // 0x80800650: nop\n\n    func_800A8264(rdram, ctx);\n',
                '    // 0x8080065C: jal         0x80015FA0\n    // 0x80800660: or          $a0, $zero, $zero\n    ctx->r4 = 0 | 0;\n    func_80015FA0(rdram, ctx);\n']:
                if body.count(guard)!=1:raise ValueError(f'{source}: changed title getter call/delay anchor in {name}')
        for anchor,pc,where in targets[name]:
            if body.count(anchor)!=1:raise ValueError(f'{source}: changed original title anchor in {name} at {pc:08X}')
            hook=f'    tooie_observe_title(rdram, ctx, 0x{pc:08X}u);\n'
            if where=='before':replacement=hook+anchor
            elif where=='after':replacement=anchor+hook
            else:
                lines=anchor.splitlines(keepends=True);replacement=''.join(lines[:-1])+hook+lines[-1]
            body=body.replace(anchor,replacement,1);count+=1
        text=text.replace(original,body,1)
    return text,count


def verify_map_actor_list_source(text, source='<generated>'):
    """Preserve the declared snapshot's original whole-body obligation."""
    matches=list(re.finditer(r'RECOMP_FUNC void gspropsDll_entrypoint_1\([^\n]*\n.*?^;}\n',text,re.S|re.M))
    if not matches:return 0
    if len(matches)!=1:raise ValueError(f'{source}: duplicate map actor list function')
    body=matches[0][0]
    call='    tooie_observe_map_actor_list(rdram, ctx, 0x808001E0u);\n'
    if body.count(call)!=1:raise ValueError(f'{source}: map actor list call changed')
    if hashlib.sha256(body.replace(call,'',1).encode()).hexdigest()!='5ef5d033b99e044c71923141ec0dcece947cb97b782a5b8e77f5b2c169badea4':
        raise ValueError(f'{source}: changed original map actor list body')
    return 1


def verify_map_actor_list_hook(text, source='<generated>'):
    """Require the declared owner and final poll/snapshot/branch order."""
    bodies=re.findall(r'RECOMP_FUNC void gspropsDll_entrypoint_1\([^\n]*\n.*?^;}\n',text,re.S|re.M)
    expected=int(bool(bodies))
    if len(bodies)>1 or len(re.findall(r'\btooie_observe_map_actor_list\s*\(',text))!=expected:
        raise ValueError(f'{source}: map actor list hook count/owner changed')
    seam=('L_808001E0:\n    tooie_continuous_poll(rdram, ctx, 0x808001E0u);\n'
          '    tooie_observe_map_actor_list(rdram, ctx, 0x808001E0u);\n'
          '    // 0x808001E0: blez        $s4, L_80800284\n')
    if bodies and bodies[0].count(seam)!=1:
        raise ValueError(f'{source}: map actor list poll/hook order changed')


def apply_save_progress_pause_hook(text, source='<generated>'):
    """Add one pause-only call to the original pause-menu update entry.

    The native helper only consumes a short-lived frontend request after it
    rechecks the original PauseState and save-manager admission fields. This
    transformation is intentionally scoped to the unique update entry rather
    than broad ``continuous_poll`` locations, which also run at title, overlay
    and worker contexts.
    """
    matches = list(re.finditer(
        r'RECOMP_FUNC void gcnewpause_entrypoint_2\(uint8_t\* rdram, recomp_context\* ctx\) \{\n.*?^;}\n',
        text, re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate gcnewpause update entry')
    body = matches[0][0]
    if 'tooie_save_progress_pause_tick' in body:
        raise ValueError(f'{source}: save-progress pause hook already present')
    anchor = ('RECOMP_FUNC void gcnewpause_entrypoint_2(uint8_t* rdram, recomp_context* ctx) {\n'
              '    uint64_t hi = 0, lo = 0, result = 0;\n')
    if body.count(anchor) != 1:
        raise ValueError(f'{source}: changed gcnewpause update-entry anchor')
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected gcnewpause include layout')
    text = text.replace(include, include +
        'extern void tooie_save_progress_pause_tick(uint8_t*, recomp_context*, uint32_t);\n', 1)
    hook = ('RECOMP_FUNC void gcnewpause_entrypoint_2(uint8_t* rdram, recomp_context* ctx) {\n'
            '    tooie_save_progress_pause_tick(rdram, ctx, (uint32_t)ctx->r4);\n'
            '    uint64_t hi = 0, lo = 0, result = 0;\n')
    return text.replace(body, body.replace(anchor, hook, 1), 1), 1


def verify_pause_snapshot_capture_hook(text, source='<generated>'):
    """Verify the declared entry and original snapshot source/copy ownership."""
    bodies = re.findall(r'RECOMP_FUNC void func_800C0F8C\([^\n]*\n.*?^;}\n', text, re.S | re.M)
    call = '    tooie_pause_snapshot_capture();\n'
    expected = int(bool(bodies))
    if (len(bodies) > 1 or text.count(call) != expected or
            text.count('tooie_pause_snapshot_capture(') != expected or (bodies and bodies[0].count(call) != 1)):
        raise ValueError(f'{source}: pause snapshot hook count/owner changed')
    if 'extern void tooie_pause_snapshot_capture(void);' in text:
        raise ValueError(f'{source}: obsolete pause snapshot extern injection')
    if not bodies:
        return 0
    body = bodies[0]
    entry = ('RECOMP_FUNC void func_800C0F8C(uint8_t* rdram, recomp_context* ctx) {\n'
             '    tooie_continuous_poll(rdram, ctx, 0x800C0F8Cu);\n'
             '    uint64_t hi = 0, lo = 0, result = 0;\n    int c1cs = 0;\n' + call + '    // 0x800C0F8C:')
    if body.count(entry) != 1:
        raise ValueError(f'{source}: pause snapshot entry order changed')
    ownership = ('    // 0x800C0FA8: jal         0x8001A0A8\n',
                 '    func_8001A0A8(rdram, ctx);\n',
                 '    // 0x800C0FB0: jal         0x80014F00\n',
                 '    func_80014F00(rdram, ctx);\n',
                 '    // 0x800C0FC0: jal         0x800C0BC0\n',
                 '    func_800C0BC0(rdram, ctx);\n')
    if any(body.count(seam) != 1 for seam in ownership):
        raise ValueError(f'{source}: changed pause snapshot source/copy ownership')
    if [body.index(seam) for seam in ownership] != sorted(body.index(seam) for seam in ownership):
        raise ValueError(f'{source}: changed pause snapshot source/copy order')
    return 1


def apply_game_feature_hooks(text, source='<generated>'):
    """Pin guest-thread hooks to original input and cutscene state entries."""
    targets = {
        'bainput_update': ('    tooie_cheats_tick(rdram, ctx);\n'
                           '    tooie_minimap_tick(rdram, ctx);\n'),
        'func_80800018_glcutDll':
            '    tooie_cutscene_state_command(rdram, ctx, (uint32_t)ctx->r4);\n',
    }
    count = 0
    declarations = []
    for match in list(re.finditer(r'RECOMP_FUNC void (\w+)\([^\n]*\n.*?^;}\n', text, re.S | re.M)):
        name, body = match[1], match[0]
        if name not in targets:
            continue
        hook = targets[name]
        if hook in body:
            raise ValueError(f'{source}: game feature hook already present: {name}')
        anchor = ('    uint64_t hi = 0, lo = 0, result = 0;\n'
                  '    int c1cs = 0;\n')
        if body.count(anchor) != 1:
            raise ValueError(f'{source}: changed game feature entry anchor: {name}')
        text = text.replace(body, body.replace(anchor, anchor + hook, 1), 1)
        declarations.append(
            ('extern void tooie_cheats_tick(uint8_t*, recomp_context*);\n'
             'extern void tooie_minimap_tick(uint8_t*, recomp_context*);\n')
            if name == 'bainput_update' else
            'extern void tooie_cutscene_state_command(uint8_t*, recomp_context*, uint32_t);\n')
        count += 1
    if declarations:
        include = '#include "funcs.h"\n'
        if text.count(include) != 1:
            raise ValueError(f'{source}: unexpected generated include layout for game features')
        text = text.replace(include, include + ''.join(declarations), 1)
    text, followup_count = apply_game_feature_followup_hooks(text, source)
    return text, count + followup_count


def apply_practice_form_hooks(text, source='<generated>'):
    """Scope form overrides to the original saved-record and basetup selectors.

    These run only during an explicitly reserved same-map Practice reload. The
    original guest functions still own model, behavior, and scene setup.
    """
    hooks = {
        'func_800F8B94': [(
            '    // 0x800F8C20: lbu         $a1, 0x0($s0)\n'
            '    ctx->r5 = MEM_BU(ctx->r16, 0X0);\n'
            '    func_800F7E64(rdram, ctx);\n',
            '    // 0x800F8C20: lbu         $a1, 0x0($s0)\n'
            '    ctx->r5 = MEM_BU(ctx->r16, 0X0);\n'
            '    tooie_practice_form_restore_override(rdram, ctx);\n'
            '    func_800F7E64(rdram, ctx);\n',
            'extern void tooie_practice_form_restore_override(uint8_t*, recomp_context*);\n')],
        'basetup_entrypoint_2': [(
            '    after_9:\n'
            '    // 0x80800908: addiu       $at, $zero, 0x9\n',
            '    after_9:\n'
            '    tooie_practice_form_basetup_override(rdram, ctx);\n'
            '    // 0x80800908: addiu       $at, $zero, 0x9\n',
            'extern void tooie_practice_form_basetup_override(uint8_t*, recomp_context*);\n'),
            ('L_80800980:\n'
             '    // 0x80800980: or          $a0, $s0, $zero\n',
             'L_80800980:\n'
             '    tooie_practice_form_basetup_final_override(rdram, ctx);\n'
             '    // 0x80800980: or          $a0, $s0, $zero\n',
             'extern void tooie_practice_form_basetup_final_override(uint8_t*, recomp_context*);\n')],
    }
    count = 0
    declarations = []
    for match in list(re.finditer(r'RECOMP_FUNC void (\w+)\([^\n]*\n.*?^;}\n', text, re.S | re.M)):
        name, body = match[1], match[0]
        if name not in hooks:
            continue
        if 'tooie_practice_form_' in body:
            raise ValueError(f'{source}: practice form hook already present: {name}')
        updated = body
        for before, after, declaration in hooks[name]:
            if updated.count(before) != 1:
                raise ValueError(f'{source}: changed practice form selector anchor: {name}')
            updated = updated.replace(before, after, 1)
            declarations.append(declaration)
            count += 1
        text = text.replace(body, updated, 1)
    if declarations:
        include = '#include "funcs.h"\n'
        if text.count(include) != 1:
            raise ValueError(f'{source}: unexpected generated include layout for practice forms')
        text = text.replace(include, include + ''.join(declarations), 1)
    return text, count


def apply_game_feature_followup_hooks(text, source='<generated>', *, lifecycle=True, restore=True, camera=True):
    """Pin save-generation invalidation and horizontal analog camera hooks."""
    count = 0
    declarations = set()
    lifecycle_names = []
    if lifecycle:
        lifecycle_names.extend(('func_800DA0B4', 'func_800DA188'))
    if restore:
        lifecycle_names.append('func_800DAC10')
    lifecycle_pattern = '|'.join(lifecycle_names)
    lifecycle_matches = list(re.finditer(
        rf'RECOMP_FUNC void ({lifecycle_pattern})\([^\n]*\n.*?^;}}\n', text, re.S | re.M)) if lifecycle_names else []
    for match in lifecycle_matches:
        name, body = match[1], match[0]
        hook = '    tooie_cheats_invalidate();\n'
        if hook in body:
            raise ValueError(f'{source}: cheat lifecycle hook already present: {name}')
        anchor = ('    uint64_t hi = 0, lo = 0, result = 0;\n'
                  '    int c1cs = 0;\n')
        if body.count(anchor) != 1:
            raise ValueError(f'{source}: changed cheat lifecycle entry anchor: {name}')
        text = text.replace(body, body.replace(anchor, anchor + hook, 1), 1)
        declarations.add('extern void tooie_cheats_invalidate(void);\n')
        count += 1

    matches = list(re.finditer(
        r'RECOMP_FUNC void func_800A4878\([^\n]*\n.*?^;}\n', text, re.S | re.M)) if camera else []
    if len(matches) > 1:
        raise ValueError(f'{source}: duplicate horizontal camera integration function')
    if matches:
        body = matches[0][0]
        hook = ('    const int tooie_camera_result = (ctx->r2 == 0)\n'
                '        ? tooie_camera_analog_apply(rdram, ctx) : -1;\n'
                '    if (tooie_camera_result >= 0) {\n'
                '        ctx->r2 = tooie_camera_result;\n'
                '        goto L_800A4964;\n'
                '    }\n')
        if 'tooie_camera_analog_apply(rdram, ctx)' in body:
            raise ValueError(f'{source}: horizontal camera hook already present')
        anchor = ('    after_5:\n'
                  '    // 0x800A4908: beq         $v0, $zero, L_800A4920\n'
                  '    if (ctx->r2 == 0) {\n')
        if body.count(anchor) != 1:
            raise ValueError(f'{source}: changed horizontal camera call/delay anchor')
        replacement = anchor.replace(
            '    // 0x800A4908: beq         $v0, $zero, L_800A4920\n',
            hook + '    // 0x800A4908: beq         $v0, $zero, L_800A4920\n')
        text = text.replace(body, body.replace(anchor, replacement, 1), 1)
        declarations.add('extern int tooie_camera_analog_apply(uint8_t*, recomp_context*);\n')
        count += 1

    declarations = {declaration for declaration in declarations if declaration not in text}
    if declarations:
        include = '#include "funcs.h"\n'
        if text.count(include) != 1:
            raise ValueError(f'{source}: unexpected generated include layout for feature followups')
        text = text.replace(include, include + ''.join(sorted(declarations)), 1)
    return text, count


def apply_camera_early_update_hook(text, source='<generated>'):
    """Apply admitted manual yaw after the early native mode-2 update path."""
    matches = list(re.finditer(
        r'RECOMP_FUNC void func_800A4878\([^\n]*\n.*?^;}\n', text, re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate camera update')
    body = matches[0][0]
    if 'tooie_camera_component' in body:
        raise ValueError(f'{source}: early camera hook already present')
    capture = '    after_0:\n    // 0x800A488C: lw          $t6, 0x38($s0)\n'
    apply = '    after_1:\n    // 0x800A48B0: b           L_800A4968\n'
    if body.count(capture) != 1 or body.count(apply) != 1:
        raise ValueError(f'{source}: changed early camera update anchors')
    updated = body.replace(capture, capture.replace('    // 0x800A488C:',
        '    uint32_t tooie_camera_component = (uint32_t)ctx->r2;\n    // 0x800A488C:'), 1)
    updated = updated.replace(apply, apply.replace('    // 0x800A48B0:',
        '    tooie_camera_analog_apply_after_native_update(rdram, ctx, tooie_camera_component);\n    // 0x800A48B0:'), 1)
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected camera include layout')
    text = text.replace(include, include +
        'extern void tooie_camera_analog_apply_after_native_update(uint8_t*, recomp_context*, uint32_t);\n', 1)
    return text.replace(body, updated, 1), 1


def apply_first_person_analog_hooks(text, source='<generated>'):
    """Route right-stick axes through Tooie's original first-person math.

    func_808002A4_bafpctrl admits only the active ncba1p controller state,
    applies the game's sensitivity and delta, clamps pitch, and writes the
    resulting aim vector. Replace only the two original bastick return values;
    zero/disabled right axes retain the original left-stick values.
    """
    matches = list(re.finditer(
        r'RECOMP_FUNC void func_808002A4_bafpctrl\([^\n]*\n.*?^;}\n', text, re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate first-person camera controller')
    body = matches[0][0]
    if 'tooie_camera_first_person_axis' in body:
        raise ValueError(f'{source}: first-person analog hooks already present')
    anchors = (
        ('    after_7:\n'
         '    // 0x80800320: lwc1        $f8, 0x2C($sp)\n',
         '    after_7:\n'
         '    ctx->f0.fl = tooie_camera_first_person_axis(ctx->f0.fl, 1);\n'
         '    // 0x80800320: lwc1        $f8, 0x2C($sp)\n'),
        ('    after_8:\n'
         '    // 0x80800344: lwc1        $f8, 0x2C($sp)\n',
         '    after_8:\n'
         '    ctx->f0.fl = tooie_camera_first_person_axis(ctx->f0.fl, 0);\n'
         '    // 0x80800344: lwc1        $f8, 0x2C($sp)\n'),
    )
    updated = body
    for anchor, replacement in anchors:
        if updated.count(anchor) != 1:
            raise ValueError(f'{source}: changed first-person stick return anchor')
        updated = updated.replace(anchor, replacement, 1)
    include = '#include "funcs.h"\n'
    declaration = 'extern float tooie_camera_first_person_axis(float, int);\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected generated include layout for first-person camera')
    text = text.replace(include, include + declaration, 1)
    return text.replace(body, updated, 1), 2


def apply_free_camera_hooks(text, source='<generated>'):
    """Temporarily translate Tooie's camera for only the active world draw.

    The begin/end pair brackets every draw call after the original camera prep.
    The native helper restores the exact position words at end, so camera and
    player simulation never inherit the detached rendering offset.
    """
    matches = list(re.finditer(
        r'RECOMP_FUNC void func_800E9F20\([^\n]*\n.*?^;}\n', text, re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate active frame composer')
    body = matches[0][0]
    if 'tooie_free_camera_begin' in body or 'tooie_free_camera_end' in body:
        raise ValueError(f'{source}: free-camera hooks already present')
    raw_begin_anchor = ('L_800E9F5C:\n'
                        '    // 0x800E9F5C: jal         0x800BE9B0\n')
    polled_begin_anchor = ('L_800E9F5C:\n'
                           '    tooie_continuous_poll(rdram, ctx, 0x800E9F5Cu);\n'
                           '    // 0x800E9F5C: jal         0x800BE9B0\n')
    end_anchor = ('    after_24:\n'
                  '    // 0x800EA048: lw          $ra, 0x1C($sp)\n')
    begin_matches = body.count(raw_begin_anchor) + body.count(polled_begin_anchor)
    if begin_matches != 1 or body.count(end_anchor) != 1:
        raise ValueError(f'{source}: changed active frame begin/end anchors')
    begin_anchor = polled_begin_anchor if polled_begin_anchor in body else raw_begin_anchor
    updated = body.replace(begin_anchor,
        begin_anchor.replace('    // 0x800E9F5C: jal         0x800BE9B0\n',
                             '    tooie_free_camera_begin(rdram, ctx);\n'
                             '    // 0x800E9F5C: jal         0x800BE9B0\n'), 1)
    updated = updated.replace(end_anchor,
        '    after_24:\n    tooie_free_camera_end(rdram, ctx);\n'
        '    // 0x800EA048: lw          $ra, 0x1C($sp)\n', 1)
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected generated include layout for free camera')
    text = text.replace(include, include +
        'extern void tooie_free_camera_begin(uint8_t*, recomp_context*);\n'
        'extern void tooie_free_camera_end(uint8_t*, recomp_context*);\n', 1)
    return text.replace(body, updated, 1), 2


def apply_camera_pod_recreation_hook(text, source='<generated>'):
    """Reset projection history after ncpod has replaced a camera allocation.

    ncpod_entrypoint_12 first tears down the active dialog/ucam/fixed-position
    camera, then initializes the selected replacement. L_80800684 is reached
    only after one of those three replacement handles was stored. It is not the
    ordinary active-camera setter, which legitimately changes during a frame.
    """
    matches = list(re.finditer(
        r'RECOMP_FUNC void ncpod_entrypoint_12\([^\n]*\n.*?^;}\n', text,
        re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate ncpod camera recreation function')
    body = matches[0][0]
    if 'tooie_camera_interpolation_request_skip' in body:
        raise ValueError(f'{source}: ncpod camera recreation hook already present')
    matches = list(re.finditer(
        r'(L_80800684:\n(?:(?!L_80800688:).)*?'
        r'ctx->r31 = MEM_W\(ctx->r29, 0X1C\);\n)', body, re.S))
    if len(matches) != 1:
        raise ValueError(f'{source}: changed ncpod post-creation convergence')
    updated = body[:matches[0].end()] + \
        '    tooie_camera_interpolation_request_skip();\n' + body[matches[0].end():]
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected generated include layout for ncpod camera')
    text = text.replace(include, include +
        'extern void tooie_camera_interpolation_request_skip(void);\n', 1)
    return text.replace(body, updated, 1), 1


def apply_model_interpolation_hooks(text, source='<generated>'):
    """Observe one original model draw and its actual CPU-skinning calls.

    Only func_800DE498 owns the complete GraphicsBuffers matrix allocation
    interval. The two dbanim calls are reached after both the model skinning
    section and resolved animation list were checked by original code.
    """
    matches = list(re.finditer(
        r'RECOMP_FUNC void func_800DE498\([^\n]*\n.*?^;}\n', text, re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate model renderer')
    body = matches[0][0]
    if 'tooie_model_draw_' in body:
        raise ValueError(f'{source}: model interpolation hooks already present')
    entry = 'RECOMP_FUNC void func_800DE498(uint8_t* rdram, recomp_context* ctx) {\n'
    exit_label = 'L_800DF310:\n'
    if body.count(entry) != 1 or body.count(exit_label) != 1:
        raise ValueError(f'{source}: changed model renderer entry/exit')
    updated = body.replace(entry, entry +
        '    tooie_model_draw_begin(rdram, ctx);\n', 1)
    for pc, target, call in (
        (0x800DEEE4, 0x800879B8, '_dbanim_entrypoint_1'),
        (0x800DEEF8, 0x800879B0, '_dbanim_entrypoint_0'),
    ):
        anchor = (f'    // 0x{pc:08X}: jal         0x{target:08X}\n'
                  f'    // 0x{pc + 4:08X}: lw          $a1, 0x1C($s0)\n'
                  '    ctx->r5 = MEM_W(ctx->r16, 0X1C);\n'
                  f'    {call}(rdram, ctx);\n')
        if updated.count(anchor) != 1:
            raise ValueError(f'{source}: changed CPU-skinning call {pc:08X}')
        updated = updated.replace(anchor, anchor.replace(
            f'    {call}(rdram, ctx);\n',
            f'    tooie_model_draw_cpu_skinning(rdram, ctx);\n    {call}(rdram, ctx);\n'), 1)
    frustum_anchor = (
        '    // 0x800DE6E8: mfc1        $a1, $f4\n'
        '    ctx->r5 = (int32_t)ctx->f4.u32l;\n'
        '    // 0x800DE6EC: jal         0x800E3E8C\n')
    if updated.count(frustum_anchor) != 1:
        raise ValueError(f'{source}: changed model frustum-sphere call')
    updated = updated.replace(frustum_anchor, frustum_anchor +
        '    tooie_nest_visibility_expand(ctx);\n', 1)
    updated = updated.replace(exit_label, exit_label +
        '    tooie_model_draw_end(rdram, ctx);\n', 1)
    projection_anchor = ('    // 0x800DEFE8: sw          $t9, 0x4($a2)\n'
                         '    MEM_W(0X4, ctx->r6) = ctx->r25;\n')
    if updated.count(projection_anchor) != 1:
        raise ValueError(f'{source}: changed nest projection allocation anchor')
    updated = updated.replace(projection_anchor, projection_anchor +
        '    tooie_model_nest_projection_emitted(rdram, ctx);\n', 1)
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected generated include layout for model interpolation')
    text = text.replace(include, include +
        'extern void tooie_model_draw_begin(uint8_t*, recomp_context*);\n'
        'extern void tooie_model_draw_cpu_skinning(uint8_t*, recomp_context*);\n'
        'extern void tooie_model_draw_end(uint8_t*, recomp_context*);\n', 1)
    text = text.replace('extern void tooie_model_draw_end(uint8_t*, recomp_context*);\n',
        'extern void tooie_model_draw_end(uint8_t*, recomp_context*);\n'
        'extern void tooie_model_nest_projection_emitted(uint8_t*, recomp_context*);\n'
        'extern void tooie_nest_visibility_expand(recomp_context*);\n', 1)
    return text.replace(body, updated, 1), 6


def apply_nest_visibility_hooks(text, source='<generated>'):
    """Pass the initialized nest child radius to its immediate body draw.

    The source wrapper func_800DE448 always enters func_800DE498.  A one-shot
    marker therefore needs no cross-call scope or yield-sensitive cleanup.
    """
    matches = list(re.finditer(
        r'RECOMP_FUNC void func_80800898_chnests\([^\n]*\n.*?^;}\n', text,
        re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate nest body renderer')
    body = matches[0][0]
    if 'tooie_nest_body_draw_begin' in body:
        raise ValueError(f'{source}: nest visibility hook already present')
    anchor = '    // 0x80800920: jal         0x800DE448\n'
    if body.count(anchor) != 1:
        raise ValueError(f'{source}: changed nest body model-call anchor')
    call = '    func_800DE448(rdram, ctx);\n'
    anchor_pos = body.index(anchor)
    call_pos = body.find(call, anchor_pos)
    if call_pos < 0 or call_pos - anchor_pos > 512:
        raise ValueError(f'{source}: changed nest body model-call body')
    updated = body[:call_pos] + '    tooie_nest_body_draw_begin(rdram, ctx);\n' + body[call_pos:]
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected generated include layout for nest body')
    text = text.replace(include, include +
        'extern void tooie_nest_body_draw_begin(uint8_t*, recomp_context*);\n', 1)
    return text.replace(body, updated, 1), 1


def apply_nest_projection_hooks(text, source='<generated>'):
    """Associate each nest child projection with its actor and draw role."""
    matches = list(re.finditer(
        r'RECOMP_FUNC void func_80800D8C_chnests\([^\n]*\n.*?^;}\n', text,
        re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate nest child renderer')
    body = matches[0][0]
    if 'tooie_model_nest_child_' in body:
        raise ValueError(f'{source}: nest projection ownership already present')
    updated = body
    call = '    func_801018A4(rdram, ctx);\n'
    for pc in (0x80800E30, 0x80800E90, 0x80800EBC):
        anchor = f'    // 0x{pc:08X}: jal         0x801018A4\n'
        if updated.count(anchor) != 1:
            raise ValueError(f'{source}: changed nest child call {pc:08X}')
        start = updated.index(anchor)
        pos = updated.find(call, start)
        if pos < 0 or pos - start > 512:
            raise ValueError(f'{source}: changed nest child call body {pc:08X}')
        updated = updated[:pos] + (
            f'    tooie_model_nest_child_begin((uint32_t)ctx->r16, 0x{pc:08X}u);\n'
            + call + '    tooie_model_nest_child_end();\n') + updated[pos + len(call):]
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected nest include layout')
    text = text.replace(include, include +
        'extern void tooie_model_nest_child_begin(uint32_t, uint32_t);\n'
        'extern void tooie_model_nest_child_end(void);\n', 1)
    return text.replace(body, updated, 1), 6


def apply_intro_model_draw_hooks(text, source='<generated>'):
    """Keep the measured title-character CPU poses on their paired guest camera.

    The title renderer can execute with an existing save slot, so a slot/level
    gameplay gate alone is insufficient to distinguish its camera-follow draws.
    Mark only its three direct func_800DE448 calls and restore the thread-local
    marker immediately afterward; the ordinary gameplay actor path is untouched.
    """
    matches = list(re.finditer(
        r'RECOMP_FUNC void func_80803A78_chintrochar\([^\n]*\n.*?^;}\n', text, re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate title-character renderer')
    body = matches[0][0]
    updated = body
    for pc in (0x80803C20, 0x80803C48, 0x80803CD4):
        anchor = (f'    // 0x{pc:08X}: jal         0x800DE448\n')
        if updated.count(anchor) != 1:
            raise ValueError(f'{source}: changed title model call {pc:08X}')
        call_start = updated.index(anchor)
        call = '    func_800DE448(rdram, ctx);\n'
        call_pos = updated.find(call, call_start)
        if call_pos < 0 or call_pos - call_start > 250:
            raise ValueError(f'{source}: changed title model call body {pc:08X}')
        updated = (updated[:call_pos] +
            '    tooie_model_intro_draw_begin();\n' +
            '    func_800DE448(rdram, ctx);\n' +
            '    tooie_model_intro_draw_end();\n' +
            updated[call_pos + len(call):])
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected generated include layout for title model')
    text = text.replace(include, include +
        'extern void tooie_model_intro_draw_begin(void);\n'
        'extern void tooie_model_intro_draw_end(void);\n', 1)
    return text.replace(body, updated, 1), 6


def apply_backpack_model_draw_hooks(text, source='<generated>'):
    """Bracket the one PlayerState-owned backpack renderer call.

    babackpack_entrypoint_3 supplies its PlayerState in s0 and invokes the
    generic model renderer exactly once. The persistent-continuation lifter
    recognizes this adjacent bracket and closes it before its generated yield
    check, so it cannot leak into a resumed guest continuation. This includes
    the selected backpack contents model.
    """
    matches = list(re.finditer(
        r'RECOMP_FUNC void babackpack_entrypoint_3\([^\n]*\n.*?^;}\n', text,
        re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate backpack renderer')
    body = matches[0][0]
    if 'tooie_model_backpack_draw_' in body:
        raise ValueError(f'{source}: backpack model hooks already present')
    anchor = '    // 0x808003FC: jal         0x800DE448\n'
    if body.count(anchor) != 1:
        raise ValueError(f'{source}: changed backpack model call anchor')
    call = '    func_800DE448(rdram, ctx);\n'
    anchor_pos = body.index(anchor)
    call_pos = body.find(call, anchor_pos)
    if call_pos < 0 or call_pos - anchor_pos > 512:
        raise ValueError(f'{source}: changed backpack model call body')
    updated = body[:call_pos] + (
        '    tooie_model_backpack_draw_begin((uint32_t)ctx->r16);\n'
        '    func_800DE448(rdram, ctx);\n'
        '    tooie_model_backpack_draw_end();\n') + body[call_pos + len(call):]
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected generated include layout for backpack model')
    text = text.replace(include, include +
        'extern void tooie_model_backpack_draw_begin(uint32_t);\n'
        'extern void tooie_model_backpack_draw_end(void);\n', 1)
    return text.replace(body, updated, 1), 2


def apply_backpack_body_draw_hooks(text, source='<generated>'):
    """Tag only the source player-body draw; native code gates backpack states."""
    matches = list(re.finditer(
        r'RECOMP_FUNC void func_80092444\([^\n]*\n.*?^;}\n', text, re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate player body renderer')
    body = matches[0][0]
    if 'tooie_model_backpack_body_draw_' in body:
        raise ValueError(f'{source}: backpack body hooks already present')
    anchor = '    // 0x800926D8: jal         0x800DE448\n'
    call = '    func_800DE448(rdram, ctx);\n'
    if body.count(anchor) != 1:
        raise ValueError(f'{source}: changed backpack body call anchor')
    anchor_pos = body.index(anchor)
    call_pos = body.find(call, anchor_pos)
    if call_pos < 0 or call_pos - anchor_pos > 512:
        raise ValueError(f'{source}: changed backpack body call body')
    updated = body[:call_pos] + (
        '    tooie_model_backpack_body_draw_begin(rdram, (uint32_t)ctx->r16);\n'
        + call + '    tooie_model_backpack_body_draw_end();\n') + body[call_pos + len(call):]
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected generated include layout for backpack body')
    text = text.replace(include, include +
        'extern void tooie_model_backpack_body_draw_begin(uint8_t*, uint32_t);\n'
        'extern void tooie_model_backpack_body_draw_end(void);\n', 1)
    return text.replace(body, updated, 1), 2


def apply_gpu_node_draw_hooks(text, source='<generated>'):
    """Identify matrix emissions by immutable model node, including recursion."""
    matches = list(re.finditer(
        r'RECOMP_FUNC void func_800DE2A4\([^\n]*\n.*?^;}\n', text, re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate model node dispatcher')
    body = matches[0][0]
    if 'tooie_model_gpu_node_' in body:
        raise ValueError(f'{source}: GPU node hooks already present')
    anchor = '    // 0x800DE2E4: jalr        $t9\n'
    call = '    LOOKUP_FUNC(ctx->r25)(rdram, ctx);\n'
    if body.count(anchor) != 1 or body.count(call) != 1:
        raise ValueError(f'{source}: changed GPU node dispatch anchor')
    pos = body.find(call, body.index(anchor))
    if pos < 0 or pos - body.index(anchor) > 512:
        raise ValueError(f'{source}: changed GPU node dispatch body')
    updated = body[:pos] + ('    tooie_model_gpu_node_begin(rdram, ctx);\n'
        + call + '    tooie_model_gpu_node_end(rdram, ctx);\n') + body[pos + len(call):]
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected GPU node include layout')
    text = text.replace(include, include +
        'extern void tooie_model_gpu_node_begin(uint8_t*, recomp_context*);\n'
        'extern void tooie_model_gpu_node_end(uint8_t*, recomp_context*);\n', 1)
    return text.replace(body, updated, 1), 2


def apply_scene_observer_hooks(text, source='<generated>'):
    """Observe Tooie's current map and bounded ordered activation calls."""
    count = 0
    declarations = []
    map_matches = list(re.finditer(
        r'RECOMP_FUNC void func_800EA05C\([^\n]*\n.*?^;}\n', text, re.S | re.M))
    if len(map_matches) > 1:
        raise ValueError(f'{source}: duplicate current-map getter')
    if map_matches:
        body = map_matches[0][0]
        if 'tooie_scene_observe_map' in body:
            raise ValueError(f'{source}: map observer already present')
        anchor = ('    // 0x800EA064: lhu         $v0, 0x2DC2($v0)\n'
                  '    ctx->r2 = MEM_HU(ctx->r2, 0X2DC2);\n'
                  '    return;\n')
        if body.count(anchor) != 1:
            raise ValueError(f'{source}: changed current-map load/return anchor')
        updated = body.replace(anchor,
            '    // 0x800EA064: lhu         $v0, 0x2DC2($v0)\n'
            '    ctx->r2 = MEM_HU(ctx->r2, 0X2DC2);\n'
            '    tooie_scene_observe_map((uint16_t)ctx->r2);\n'
            '    return;\n', 1)
        text = text.replace(body, updated, 1)
        declarations.append('extern void tooie_scene_observe_map(uint16_t);\n')
        count += 1

    activation_matches = list(re.finditer(
        r'RECOMP_FUNC void func_800A72A4\([^\n]*\n.*?^;}\n', text, re.S | re.M))
    if len(activation_matches) > 1:
        raise ValueError(f'{source}: duplicate scene-activation routine')
    if activation_matches:
        body = activation_matches[0][0]
        if 'tooie_scene_activation_started' in body or 'tooie_scene_activation_phase_started' in body:
            raise ValueError(f'{source}: scene activation observers already present')
        entry = 'RECOMP_FUNC void func_800A72A4(uint8_t* rdram, recomp_context* ctx) {\n'
        variables = '    uint64_t hi = 0, lo = 0, result = 0;\n'
        poll = '    tooie_continuous_poll(rdram, ctx, 0x800A72A4u);\n'
        start_variants = ((entry + variables,
                           entry + '    tooie_scene_activation_started();\n' + variables),
                          (entry + poll + variables,
                           entry + poll + '    tooie_scene_activation_started();\n' + variables))
        finish_anchor = ('    after_12:\n'
                         '    // 0x800A7370: lw          $ra, 0x14($sp)\n')
        phases = (
            (0, '    MEM_W(0X1C, ctx->r29) = ctx->r14;\n', '_gcsectionDll_entrypoint_4', 'after_0', 0x800A72C4),
            (1, '', '_gclevel_entrypoint_1', 'after_3', 0x800A7300),
            (2, '', '_gclevel_entrypoint_0', 'after_6', 0x800A7328),
            (3, '', '_gcsectionDll_entrypoint_0', 'after_7', 0x800A7334),
            (4, '    ctx->r6 = 0 | 0;\n', 'func_800EA0CC', 'after_11', None),
        )
        start_matches = [pair for pair in start_variants if body.count(pair[0]) == 1]
        if len(start_matches) != 1 or body.count(finish_anchor) != 1:
            raise ValueError(f'{source}: changed scene-activation boundary anchors')
        updated = body.replace(*start_matches[0], 1)
        for phase, prefix, callee, label, call_pc in phases:
            call = f'    {callee}(rdram, ctx);\n'
            suffix = f'        goto {label};\n'
            raw = prefix + call + suffix
            variants = [(raw, prefix +
                f'    tooie_scene_activation_phase_started({phase});\n' + call +
                f'    tooie_scene_activation_phase_finished({phase});\n' + suffix)]
            if call_pc is not None:
                push = f'    tooie_overlay_callsite_push(0x{call_pc:08X}u);\n'
                pop = '    tooie_overlay_callsite_pop();\n'
                observed = prefix + push + call + pop + suffix
                variants.append((observed, prefix +
                    f'    tooie_scene_activation_phase_started({phase});\n' + push + call + pop +
                    f'    tooie_scene_activation_phase_finished({phase});\n' + suffix))
            matches = [(anchor, replacement) for anchor, replacement in variants
                       if updated.count(anchor) == 1]
            if len(matches) != 1:
                raise ValueError(f'{source}: changed scene phase {phase} anchors')
            updated = updated.replace(*matches[0], 1)
        updated = updated.replace(finish_anchor,
            '    after_12:\n'
            '    tooie_scene_activation_finished();\n'
            '    // 0x800A7370: lw          $ra, 0x14($sp)\n', 1)
        text = text.replace(body, updated, 1)
        declarations.extend((
            'extern void tooie_scene_activation_phase_started(uint32_t);\n',
            'extern void tooie_scene_activation_phase_finished(uint32_t);\n',
            'extern void tooie_scene_activation_started(void);\n',
            'extern void tooie_scene_activation_finished(void);\n'))
        count += 12

    if declarations:
        include = '#include "funcs.h"\n'
        if text.count(include) != 1:
            raise ValueError(f'{source}: unexpected generated include layout for scene observer')
        text = text.replace(include, include + ''.join(declarations), 1)
    return text, count

def verify_guest_update_observer(text, source='<generated>'):
    """Validate the declared guest-update entry diagnostic without rewriting it."""
    bodies = re.findall(r'RECOMP_FUNC void func_800A73F4\([^\n]*\n.*?^;}\n', text, re.S | re.M)
    call = '    tooie_guest_update_observed(MEM_H(0, (gpr)(int32_t)0x80127634U) != 0);\n'
    expected = int(bool(bodies))
    if (len(bodies) > 1 or text.count(call) != expected or
            text.count('tooie_guest_update_observed(') != expected or (bodies and bodies[0].count(call) != 1)):
        raise ValueError(f'{source}: guest-update hook count/owner changed')
    if 'extern void tooie_guest_update_observed(int wait_bypassed);' in text:
        raise ValueError(f'{source}: obsolete guest-update extern injection')
    seam = ('RECOMP_FUNC void func_800A73F4(uint8_t* rdram, recomp_context* ctx) {\n'
            '    tooie_continuous_poll(rdram, ctx, 0x800A73F4u);\n'
            '    uint64_t hi = 0, lo = 0, result = 0;\n    int c1cs = 0;\n' + call + '    // 0x800A73F4:')
    if bodies and bodies[0].count(seam) != 1:
        raise ValueError(f'{source}: guest-update entry order changed')
    return expected


def verify_game_delta_observer(text, source='<generated>'):
    """Check the declared post-replay diagnostic without modifying output."""
    bodies = re.findall(r'RECOMP_FUNC void func_800123F4\([^\n]*\n.*?^;}\n', text, re.S | re.M)
    call = '    tooie_game_delta_observed((uint32_t)MEM_W(0, (gpr)(int32_t)0x8012C760U));\n'
    expected = int(bool(bodies))
    if (len(bodies) > 1 or text.count(call) != expected or
            text.count('tooie_game_delta_observed(') != expected or (bodies and bodies[0].count(call) != 1)):
        raise ValueError(f'{source}: game-delta hook count/owner changed')
    if 'extern void tooie_game_delta_observed(uint32_t);' in text:
        raise ValueError(f'{source}: obsolete game-delta extern injection')
    seam = ('    func_8001608C(rdram, ctx);\n'
            '        goto after_4;\n    // 0x8001245C: nop\n\n'
            '    after_4:\n' + call + '    // 0x80012460:')
    if bodies and bodies[0].count(seam) != 1:
        raise ValueError(f'{source}: game-delta post-replay continuation changed')
    return expected


_REPLAY_TIMING_HOOKS = (
    ('func_8001608C', 0x8001608C, 'tooie_replay_timing_begin_record();'),
    ('func_8001608C', 0x800160F8, 'tooie_replay_timing_record_divisor((uint32_t)MEM_W(0X50, ctx->r29), ctx->r2 != 0);'),
    ('func_800151BC', 0x8001520C, 'ctx->r2 = (gpr)(int32_t)tooie_replay_timing_scheduler_divisor((uint32_t)ctx->r2, 1);'),
    ('func_800151BC', 0x8001524C, 'ctx->r2 = (gpr)(int32_t)tooie_replay_timing_scheduler_divisor((uint32_t)ctx->r2, 0);'),
)


def verify_replay_timing_hooks(text, source='<generated>'):
    """Verify four declared hooks without changing generated text."""
    count = 0
    for name, pc, hook in _REPLAY_TIMING_HOOKS:
        bodies = re.findall(r'RECOMP_FUNC void ' + name + r'\([^\n]*\n.*?^;}\n', text, re.S | re.M)
        call = '    ' + hook + '\n'
        expected = int(bool(bodies))
        if len(bodies) > 1 or text.count(call) != expected:
            raise ValueError(f'{source}: replay hook count/owner changed: {name}:{pc:08X}')
        if bodies and (bodies[0].count(call + f'    // 0x{pc:08X}:') != 1 or
                       (pc == 0x800160F8 and bodies[0].count('    after_3:\n' + call) != 1)):
            raise ValueError(f'{source}: replay hook PC/continuation changed: {name}:{pc:08X}')
        count += expected
    return count


def apply_actor_draw_distance_hooks(text, source='<generated>'):
    """Scale only the common actor model fade/cull distances.

    func_80101970 selects either the actor descriptor's +0x1A distance or the
    original map-derived fallback, then sends it through func_800DF428 to the
    model renderer. Camera planes, terrain, and special-camera paths are not
    part of these two pinned call sites.
    """
    matches = list(re.finditer(
        r'RECOMP_FUNC void func_80101970\([^\n]*\n.*?^;}\n', text, re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate common actor setup owner')
    body = matches[0][0]
    if 'tooie_actor_draw_distance_adjust' in body:
        raise ValueError(f'{source}: actor-distance hooks already present')
    call = '    func_800DF428(rdram, ctx);\n'
    if body.count(call) != 2 or body.count('// 0x801019CC: jal         0x800DF428') != 1 or \
            body.count('// 0x80101A1C: jal         0x800DF428') != 1:
        raise ValueError(f'{source}: changed actor-distance owner/calls')
    updated = body.replace(call,
        '    ctx->f12.fl = tooie_actor_draw_distance_adjust(ctx->f12.fl);\n' + call)
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected generated include layout for actor distance')
    text = text.replace(include, include +
        'extern float tooie_actor_draw_distance_adjust(float);\n', 1)
    return text.replace(body, updated, 1), 2


def apply_hud_counter_layout_hook(text, source='<generated>'):
    """Adjust source-identified ordinary counters at the scinfobar draw seam.

    func_800FA508 walks the 48 original 0x1c-byte slots. r16 is the current
    slot, r6/r7 are its animated horizontal/vertical origins, and D_8012B2C0
    retains the counter ID written by func_800D22E0. The helper changes r6
    only for the exact ordinary-counter allowlist in hud_layout.cpp.
    """
    matches = list(re.finditer(
        r'RECOMP_FUNC void func_800FA508\([^\n]*\n.*?^;}\n', text, re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate scinfobar renderer')
    body = matches[0][0]
    if 'tooie_hud_counter_adjust' in body:
        raise ValueError(f'{source}: HUD counter-layout hook already present')
    anchor = ('    // 0x800FA5CC: jal         0x8008A640\n'
              '    // 0x800FA5D0: lh          $a3, 0x8($s0)\n'
              '    ctx->r7 = MEM_H(ctx->r16, 0X8);\n')
    if body.count(anchor) != 1 or body.count('// 0x800FA5C8: lh          $a2, 0x6($s0)') != 1:
        raise ValueError(f'{source}: changed scinfobar slot/call anchor')
    updated = body.replace(anchor,
        '    // 0x800FA5CC: jal         0x8008A640\n'
        '    // 0x800FA5D0: lh          $a3, 0x8($s0)\n'
        '    ctx->r7 = MEM_H(ctx->r16, 0X8);\n'
        '    tooie_hud_counter_adjust(rdram, ctx);\n', 1)
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected generated include layout for HUD counters')
    text = text.replace(include, include +
        'extern void tooie_hud_counter_adjust(uint8_t*, recomp_context*);\n', 1)
    return text.replace(body, updated, 1), 1


def apply_hud_counter_rect_hook(text, source='<generated>'):
    """Scope RT64 rectangle/scissor alignment to the numeric callback."""
    matches = list(re.finditer(
        r'RECOMP_FUNC void scinfobar_entrypoint_24\([^\n]*\n.*?^;}\n',
        text, re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate scinfobar callback owner')
    body = matches[0][0]
    if 'tooie_hud_counter_rect_begin' in body or 'tooie_hud_counter_rect_end' in body:
        raise ValueError(f'{source}: HUD counter rectangle hooks already present')
    if re.search(r'MEM_[A-Z]+\(0X30, ctx->r29\)|MEM_[A-Z]+\(ctx->r29, 0X30\)|0x30\(\$sp\)',
            body, re.I):
        raise ValueError(f'{source}: scinfobar guest-frame marker is no longer free')
    # This hook runs before the generic observed-callsite pass adds the
    # push/pop wrapper; anchor the original generated jalr itself.
    anchor = '    LOOKUP_FUNC(ctx->r2)(rdram, ctx);\n'
    if body.count(anchor) != 1 or body.count(
            '// 0x808007D0: jalr        $v0') != 1:
        raise ValueError(f'{source}: changed scinfobar callback anchor')
    updated = body.replace(anchor,
        '    tooie_hud_counter_rect_begin(rdram, ctx);\n' +
        anchor +
        '    tooie_hud_counter_rect_end(rdram, ctx);\n', 1)
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected generated include layout for HUD rectangles')
    text = text.replace(include, include +
        'extern void tooie_hud_counter_rect_begin(uint8_t*, recomp_context*);\n'
        'extern void tooie_hud_counter_rect_end(uint8_t*, recomp_context*);\n', 1)
    return text.replace(body, updated, 1), 2


def apply_egg_reticle_ortho_hook(text, source='<generated>'):
    """Restore only the egg cursor sprite's original X matrix coefficient.

    The cursor sprite applies its own inverse-aspect X scale before its draw.
    Hook after that draw, while D_8012D554 still names its ortho matrix, so
    no caller-wide state or world projection is changed.
    """
    matches = list(re.finditer(
        r'RECOMP_FUNC void baeggcursor_entrypoint_1\([^\n]*\n.*?^;}\n',
        text, re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate egg cursor draw owner')
    body = matches[0][0]
    if 'tooie_hud_egg_reticle_ortho_restore' in body:
        raise ValueError(f'{source}: egg reticle ortho hook already present')
    anchor = ('    func_800E30E0(rdram, ctx);\n'
              '        goto after_9;\n'
              '    // 0x808000F8: addiu       $a3, $sp, 0x30\n'
              '    ctx->r7 = ADD32(ctx->r29, 0X30);\n'
              '    after_9:\n')
    if body.count(anchor) != 1:
        raise ValueError(f'{source}: changed egg cursor sprite draw anchor')
    body = body.replace(anchor, anchor +
        '    tooie_hud_egg_reticle_ortho_restore(rdram, ctx);\n', 1)
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected generated include layout for egg reticle')
    text = text.replace(include, include +
        'extern void tooie_hud_egg_reticle_ortho_restore(uint8_t*, recomp_context*);\n', 1)
    return text.replace(matches[0][0], body, 1), 1


def apply_cutscene_skip_input_hook(text, source='<generated>'):
    """Merge a queued host request into the intro tick's original Start test.

    This preserves func_80800530_chintroticker as the sole transition owner,
    including its original map-specific skip table and state changes.
    """
    matches = list(re.finditer(
        r'RECOMP_FUNC void func_808005AC_chintroticker\([^\n]*\n.*?^;}\n',
        text, re.S | re.M))
    if not matches:
        return text, 0
    if len(matches) != 1:
        raise ValueError(f'{source}: duplicate intro ticker')
    body = matches[0][0]
    if 'tooie_cutscene_skip_input' in body:
        raise ValueError(f'{source}: cutscene-skip input hook already present')
    anchor = ('    func_80016B30(rdram, ctx);\n'
              '        goto after_7;\n'
              '    // 0x80800680: or          $a1, $s0, $zero\n'
              '    ctx->r5 = ctx->r16 | 0;\n'
              '    after_7:\n')
    if body.count(anchor) != 1 or body.count('// 0x80800684: bne         $v0, $s0, L_80800694') != 1:
        raise ValueError(f'{source}: changed original intro Start-test anchor')
    updated = body.replace(anchor, anchor +
        '    ctx->r2 = tooie_cutscene_skip_input((int)ctx->r2);\n', 1)
    include = '#include "funcs.h"\n'
    if text.count(include) != 1:
        raise ValueError(f'{source}: unexpected generated include layout for cutscene skip')
    text = text.replace(include, include +
        'extern int tooie_cutscene_skip_input(int);\n', 1)
    return text.replace(body, updated, 1), 1


def apply_music_volume_hooks(text, source='<generated>'):
    """Pin volume hooks to Tooie's private music manager and setter.

    func_800FB968 owns the six 0x50-byte music slots and consumes the original
    D_801359B0 refresh byte. func_80017404 is their master-volume setter. Its
    complete caller set is confined to that manager and its player housekeeping;
    the separate func_800C4xxx spatial sound-effect service does not call it.
    """
    targets = {
        'func_800FB968': (
            ('    uint64_t hi = 0, lo = 0, result = 0;\n'
             '    int c1cs = 0;\n'),
            '    tooie_music_volume_tick(rdram, ctx);\n',
            'extern void tooie_music_volume_tick(uint8_t*, recomp_context*);\n'),
        'func_80017404': (
            ('    // 0x8001744C: sh          $a1, 0x26($sp)\n'
             '    MEM_H(0X26, ctx->r29) = ctx->r5;\n'
             '    // 0x80017450: addiu       $a0, $v0, 0x1E0\n'),
            '    tooie_music_volume_apply(rdram, ctx);\n',
            'extern void tooie_music_volume_apply(uint8_t*, recomp_context*);\n'),
    }
    count = 0
    declarations = []
    for match in list(re.finditer(r'RECOMP_FUNC void (\w+)\([^\n]*\n.*?^;}\n', text, re.S | re.M)):
        name, body = match[1], match[0]
        if name not in targets:
            continue
        anchor, hook, declaration = targets[name]
        if hook in body:
            raise ValueError(f'{source}: music-volume hook already present: {name}')
        if body.count(anchor) != 1:
            raise ValueError(f'{source}: changed music-volume entry anchor: {name}')
        if name == 'func_80017404':
            # The original 0x80017448 and 0x8001744C stores have already
            # preserved the unscaled low-level cache and stack copy here.
            # Original a0 still identifies the six-player slot for Jukebox
            # classification. Scale only a1 for the event-posting call;
            # 0x80017460 reloads the original before housekeeping paths run.
            replacement = anchor.replace(
                '    // 0x80017450: addiu       $a0, $v0, 0x1E0\n',
                hook + '    // 0x80017450: addiu       $a0, $v0, 0x1E0\n')
        else:
            replacement = anchor + hook
        text = text.replace(body, body.replace(anchor, replacement, 1), 1)
        declarations.append(declaration)
        count += 1
    if declarations:
        include = '#include "funcs.h"\n'
        if text.count(include) != 1:
            raise ValueError(f'{source}: unexpected generated include layout for music volume')
        text = text.replace(include, include + ''.join(declarations), 1)
    return text, count


_MENU_BODIES={
    'func_808006A0_chgameselect':('aa44ebe3bfc0bd58ac4fed2d5a4cd2059c1ce371bd7901d40aa6b704a414436e',(0x808006A0,0x8080074C,0x8080085C)),
    'func_8080086C_chgameselect':('b875c2f72f1293da5a13a86921896b3b0a037a65f0642a4c0ba1ff0481d47c21',(0x8080086C,0x808008B0)),
    'func_80800D3C_chgameselect':('6ee500c84c2b2cafea8898d28f8ba8a907b61b9c12cf049670677bbb1a97b3a6',(0x80800D4C,)),
    'func_80800E24_chgameselect':('0770131ff58f15389e8329577905f08ab30cac25380b3b21c91a406c9d0b58c7',(0x80800EB0,0x80801000,0x80801014,0x80801048)),
    'func_808013C8_chgameselect':('78ac57f94c00f4d2d8cb19d8a356e2eb23a08e39720b2d8d2503937d84a6eff8',(0x8080140C,)),
}

def verify_menu_source(text,source='<generated>',section_index=58):
    """Validate declared snapshots and original metadata-bound complete bodies."""
    found=set();count=0
    for match in list(re.finditer(r'RECOMP_FUNC void (\w+)\([^\n]*\n.*?^;}\n',text,re.S|re.M)):
        name,body=match[1],match[0]
        if name not in _MENU_BODIES:continue
        if name in found:raise ValueError(f'{source}: duplicate menu function {name}')
        found.add(name);expected,sites=_MENU_BODIES[name]
        if section_index is None:raise ValueError(f'{source}: menu function lacks selected section metadata')
        if len(re.findall(r'\btooie_observe_menu\s*\(',body))!=len(sites):
            raise ValueError(f'{source}: menu hook identifier count changed in {name}')
        for pc in sites:
            call=f'    tooie_observe_menu(rdram, ctx, 0x{pc:08X}u);\n'
            if body.count(call)!=1:raise ValueError(f'{source}: menu call changed in {name}:{pc:08X}')
            body=body.replace(call,'',1);count+=1
        # Only the compiler's selected-section ordinal is variable. Bind every
        # own-section relocation to the supplied .chgameselect metadata index,
        # then canonicalize that integer for hashing; emit original bytes.
        def canonical_reloc(match):
            if int(match[2])!=section_index:raise ValueError(f'{source}: wrong menu relocation section in {name}')
            return match[1]+'58'+match[3]
        canonical=re.sub(r'(RELOC_(?:HI16|LO16)\()(\d+)(,)',canonical_reloc,body)
        if hashlib.sha256(canonical.encode()).hexdigest()!=expected:raise ValueError(f'{source}: changed original menu body {name}')
    return count


def verify_menu_hooks(text,source='<generated>'):
    """Require owner-local instruction order and the final label poll seam."""
    found=set();expected=0
    for match in re.finditer(r'RECOMP_FUNC void (\w+)\([^\n]*\n.*?^;}\n',text,re.S|re.M):
        name,body=match[1],match[0]
        if name not in _MENU_BODIES:continue
        if name in found:raise ValueError(f'{source}: duplicate menu function {name}')
        found.add(name);sites=_MENU_BODIES[name][1];expected+=len(sites)
        for pc in sites:
            call=f'    tooie_observe_menu(rdram, ctx, 0x{pc:08X}u);\n'
            if body.count(call+f'    // 0x{pc:08X}:')!=1:
                raise ValueError(f'{source}: menu hook instruction order changed in {name}:{pc:08X}')
        if name=='func_808006A0_chgameselect' and body.count(
                'L_8080085C:\n    tooie_continuous_poll(rdram, ctx, 0x8080085Cu);\n'
                '    tooie_observe_menu(rdram, ctx, 0x8080085Cu);\n')!=1:
            raise ValueError(f'{source}: menu label poll order changed')
    if len(re.findall(r'\btooie_observe_menu\s*\(',text))!=expected:
        raise ValueError(f'{source}: menu hook global identifier count changed')


def instrument(directory, sections, *, guest_native_aspect=True):
    if guest_native_aspect is not True:
        raise ValueError('guest-native aspect composition is required; legacy writers are retired')
    addresses={f['name']: f['vram'] for s in sections for f in s['functions']}
    menu_indices=[i for i,s in enumerate(sections) if s.get('name')=='.chgameselect']
    if len(menu_indices)>1:raise ValueError('duplicate selected chgameselect section')
    menu_section_index=menu_indices[0] if menu_indices else None
    stubs={f['name']:f['vram'] for s in sections for f in s['functions'] if f['name'].startswith('_') and '_entrypoint_' in f['name'] and f['size']==8}
    counts={'entries':0,'labels':0,'observed_syscall_callsites':0,'si_completion_observations':0,
            'observed_indirect_jalr':0,'observed_indirect_tail_jr':0,'observed_direct_lookup':0,
            'widescreen_profile_hooks':0,'graphics_observations':0,'title_observations':0,'map_actor_list_observations':0,'menu_observations':0,'save_progress_pause_hooks':0,'pause_snapshot_capture_hooks':0,'game_feature_hooks':0,'first_person_analog_hooks':0,'free_camera_hooks':0,'scene_observer_hooks':0,'guest_update_observers':0,'game_delta_observers':0,'replay_timing_hooks':0,'native_aspect_hooks':0,'actor_draw_distance_hooks':0,'hud_counter_layout_hooks':0,'hud_counter_rect_hooks':0,'egg_reticle_ortho_hooks':0,'cutscene_skip_input_hooks':0,'music_volume_hooks':0}
    counts['practice_form_hooks']=0
    counts['model_interpolation_hooks']=0
    counts['nest_visibility_hooks']=0
    counts['nest_projection_hooks']=0
    counts['intro_model_draw_hooks']=0
    counts['backpack_model_draw_hooks']=0
    counts['backpack_body_draw_hooks']=0
    counts['gpu_node_draw_hooks']=0
    counts['camera_early_update_hooks']=0
    counts['camera_pod_recreation_hooks']=0
    overlay_counts={'capture':0,'publish':0,'free':0,'pi_status':0,'rom_key':0}
    overlay_companion_counts={'delta':0,'validate':0}
    for path in sorted(Path(directory).glob('*.c')):
        text=path.read_text()
        if re.search(r'\btooie_observe_global_settings\b', text):
            raise ValueError(f'{path}: retired global settings observer remains')
        widescreen_count=verify_widescreen_profile_source(text,str(path))
        counts['widescreen_profile_hooks']+=widescreen_count
        text,graphics_count=observe_graphics(text,str(path))
        counts['graphics_observations']+=graphics_count
        text,title_count=observe_title(text,str(path))
        counts['title_observations']+=title_count
        counts['map_actor_list_observations']+=verify_map_actor_list_source(text,str(path))
        counts['menu_observations']+=verify_menu_source(text,str(path),menu_section_index)
        text,save_progress_count=apply_save_progress_pause_hook(text,str(path))
        counts['save_progress_pause_hooks']+=save_progress_count
        text,game_feature_count=apply_game_feature_hooks(text,str(path))
        counts['game_feature_hooks']+=game_feature_count
        text,camera_early_count=apply_camera_early_update_hook(text,str(path))
        counts['camera_early_update_hooks']+=camera_early_count
        text,practice_form_count=apply_practice_form_hooks(text,str(path))
        counts['practice_form_hooks']+=practice_form_count
        text,first_person_analog_count=apply_first_person_analog_hooks(text,str(path))
        counts['first_person_analog_hooks']+=first_person_analog_count
        text,free_camera_count=apply_free_camera_hooks(text,str(path))
        counts['free_camera_hooks']+=free_camera_count
        text,camera_pod_recreation_count=apply_camera_pod_recreation_hook(text,str(path))
        counts['camera_pod_recreation_hooks']+=camera_pod_recreation_count
        text,model_interpolation_count=apply_model_interpolation_hooks(text,str(path))
        counts['model_interpolation_hooks']+=model_interpolation_count
        text,nest_visibility_count=apply_nest_visibility_hooks(text,str(path))
        counts['nest_visibility_hooks']+=nest_visibility_count
        text,nest_projection_count=apply_nest_projection_hooks(text,str(path))
        counts['nest_projection_hooks']+=nest_projection_count
        text,intro_model_draw_count=apply_intro_model_draw_hooks(text,str(path))
        counts['intro_model_draw_hooks']+=intro_model_draw_count
        text,backpack_model_draw_count=apply_backpack_model_draw_hooks(text,str(path))
        counts['backpack_model_draw_hooks']+=backpack_model_draw_count
        text,backpack_body_draw_count=apply_backpack_body_draw_hooks(text,str(path))
        counts['backpack_body_draw_hooks']+=backpack_body_draw_count
        text,gpu_node_draw_count=apply_gpu_node_draw_hooks(text,str(path))
        counts['gpu_node_draw_hooks']+=gpu_node_draw_count
        text,scene_observer_count=apply_scene_observer_hooks(text,str(path))
        counts['scene_observer_hooks']+=scene_observer_count
        game_delta_count=verify_game_delta_observer(text,str(path))
        counts['game_delta_observers']+=game_delta_count
        native_aspect_count=verify_guest_native_aspect_source(text,str(path))
        counts['native_aspect_hooks']+=native_aspect_count
        counts['replay_timing_hooks']+=verify_replay_timing_hooks(text,str(path))
        text,actor_distance_count=apply_actor_draw_distance_hooks(text,str(path))
        counts['actor_draw_distance_hooks']+=actor_distance_count
        text,hud_counter_layout_count=apply_hud_counter_layout_hook(text,str(path))
        counts['hud_counter_layout_hooks']+=hud_counter_layout_count
        text,hud_counter_rect_count=apply_hud_counter_rect_hook(text,str(path))
        counts['hud_counter_rect_hooks']+=hud_counter_rect_count
        text,egg_reticle_ortho_count=apply_egg_reticle_ortho_hook(text,str(path))
        counts['egg_reticle_ortho_hooks']+=egg_reticle_ortho_count
        text,cutscene_skip_count=apply_cutscene_skip_input_hook(text,str(path))
        counts['cutscene_skip_input_hooks']+=cutscene_skip_count
        text,music_volume_count=apply_music_volume_hooks(text,str(path))
        counts['music_volume_hooks']+=music_volume_count
        lines=text.splitlines(keepends=True)
        observations=callsite_observations(lines, stubs, str(path))
        starts={}
        ends={}
        for call_index, observation in observations.items():
            start=call_index-1 if call_index and lines[call_index-1] == '    tooie_model_gpu_node_begin(rdram, ctx);\n' else call_index
            end=call_index+1 if call_index+1<len(lines) and lines[call_index+1] == '    tooie_model_gpu_node_end(rdram, ctx);\n' else call_index
            if start in starts or end in ends:raise ValueError(f'{path}: overlapping observed call wrappers')
            starts[start]=observation;ends[end]=observation
        result=[]
        current=0
        current_name=''
        for index, line in enumerate(lines):
            match=re.match(r'RECOMP_FUNC void (\w+)\(uint8_t\* rdram, recomp_context\* ctx\) \{',line)
            if match: current_name=match[1]
            if index in starts:
                call_pc, kind=starts[index]
                result.append(f'    tooie_overlay_callsite_push(0x{call_pc:08X}u);\n')
                counts['observed_syscall_callsites' if kind=='direct_stub' else 'observed_'+kind]+=1
            if current_name=='func_80082088':
                if line.strip()=='ctx->r10 = MEM_W(ctx->r9, 0X0);':
                    overlay_counts['pi_status']+=1
                if line.strip()=='ctx->r21 = MEM_W(ctx->r21, 0X40);':
                    line='    ctx->r21 = tooie_overlay_hardware_word((uint32_t)ADD32(ctx->r21,0X40));\n';overlay_counts['rom_key']+=1
                if line.strip()=='uint32_t tooie_overlay_header=(uint32_t)ctx->r4;':
                    overlay_counts['capture']+=1
                if line.strip()=='int32_t tooie_overlay_delta=(int32_t)ctx->r5;':
                    overlay_companion_counts['delta']+=1
                if line.strip()=='tooie_validate_overlay(rdram, tooie_overlay_header, tooie_overlay_delta);':
                    overlay_companion_counts['validate']+=1
                if line.strip()=='tooie_overlay_after_relocate(rdram, tooie_overlay_header, tooie_overlay_delta);':
                    overlay_counts['publish']+=1
            if current_name=='ovl_unload' and line.strip()=='tooie_overlay_before_free(rdram, (uint32_t)ctx->r4);':
                overlay_counts['free']+=1
            si_pc=re.match(r'\s*// 0x(8001DFB0|8001DFC8|8001E1B0):',line)
            if si_pc:
                pc=int(si_pc[1],16)
                assert current_name==('func_8001E170' if pc==0x8001E1B0 else 'func_8001DEE8'),current_name
                result.append(f'    tooie_continuous_poll(rdram, ctx, 0x{pc:08X}u);\n')
                counts['si_completion_observations']+=1
            result.append(line)
            if index in ends:result.append('    tooie_overlay_callsite_pop();\n')
            match=re.match(r'RECOMP_FUNC void (\w+)\(uint8_t\* rdram, recomp_context\* ctx\) \{',line)
            if match:
                name=match[1]
                current=addresses.get(name,0x80000400 if name=='recomp_entrypoint' else 0)
                result.append(f'    tooie_continuous_poll(rdram, ctx, 0x{current:08X}u);\n')
                counts['entries']+=1
            elif re.match(r'^L_[0-9A-Fa-f]+:',line):
                pc=int(line.split(':')[0][2:],16)
                result.append(f'    tooie_continuous_poll(rdram, ctx, 0x{pc:08X}u);\n')
                counts['labels']+=1
        text = ''.join(result)
        counts['guest_update_observers'] += verify_guest_update_observer(text, str(path))
        counts['pause_snapshot_capture_hooks'] += verify_pause_snapshot_capture_hook(text, str(path))
        verify_map_actor_list_hook(text, str(path))
        verify_menu_hooks(text, str(path))
        path.write_text(text)
    expected_si=2*int('func_8001DEE8' in addresses)+int('func_8001E170' in addresses)
    assert counts['si_completion_observations']==expected_si,counts
    # Canonical instructions replace both PI loads, including delay-slot copies.
    # Any remaining raw MMIO expression must fail generation.
    assert overlay_counts=={'capture':1,'publish':1,'free':1,'pi_status':0,'rom_key':1},overlay_counts
    assert overlay_companion_counts=={'delta':1,'validate':1},overlay_companion_counts
    assert counts['widescreen_profile_hooks']==int('func_80800034_glglobalsettings' in addresses),counts
    expected_graphics=len(set(addresses)&{'func_80014A88','func_80013E34','func_80013EF0','func_80013FE4','func_800146D8'})
    assert counts['graphics_observations']==expected_graphics,counts
    expected_title=6*int('func_808005AC_chintroticker' in addresses)+int('func_80800738_gcfrontend' in addresses)+int('gsworldDll_entrypoint_2' in addresses)+int('gsattract_entrypoint_1' in addresses)
    assert counts['title_observations']==expected_title,counts
    assert counts['map_actor_list_observations']==int('gspropsDll_entrypoint_1' in addresses),counts
    assert counts['menu_observations']==sum(len(value[1]) for name,value in _MENU_BODIES.items() if name in addresses),counts
    assert counts['save_progress_pause_hooks']==int('gcnewpause_entrypoint_2' in addresses),counts
    assert counts['pause_snapshot_capture_hooks']==int('func_800C0F8C' in addresses),counts
    counts['expected_game_feature_hooks']=sum(int(name in addresses) for name in (
        'bainput_update','func_80800018_glcutDll','func_800DA0B4','func_800DA188','func_800DAC10','func_800A4878'))
    assert counts['game_feature_hooks']==counts['expected_game_feature_hooks'],counts
    counts['expected_practice_form_hooks']=int('func_800F8B94' in addresses)+2*int('basetup_entrypoint_2' in addresses)
    assert counts['practice_form_hooks']==counts['expected_practice_form_hooks'],counts
    counts['expected_first_person_analog_hooks']=2*int('func_808002A4_bafpctrl' in addresses)
    assert counts['first_person_analog_hooks']==counts['expected_first_person_analog_hooks'],counts
    counts['expected_free_camera_hooks']=2*int('func_800E9F20' in addresses)
    assert counts['free_camera_hooks']==counts['expected_free_camera_hooks'],counts
    counts['expected_model_interpolation_hooks']=6*int('func_800DE498' in addresses)
    assert counts['model_interpolation_hooks']==counts['expected_model_interpolation_hooks'],counts
    counts['expected_nest_visibility_hooks']=int('func_80800898_chnests' in addresses)
    assert counts['nest_visibility_hooks']==counts['expected_nest_visibility_hooks'],counts
    counts['expected_nest_projection_hooks']=6*int('func_80800D8C_chnests' in addresses)
    assert counts['nest_projection_hooks']==counts['expected_nest_projection_hooks'],counts
    counts['expected_backpack_body_draw_hooks']=2*int('func_80092444' in addresses)
    assert counts['backpack_body_draw_hooks']==counts['expected_backpack_body_draw_hooks'],counts
    counts['expected_gpu_node_draw_hooks']=2*int('func_800DE2A4' in addresses)
    assert counts['gpu_node_draw_hooks']==counts['expected_gpu_node_draw_hooks'],counts
    counts['expected_camera_early_update_hooks']=int('func_800A4878' in addresses)
    assert counts['camera_early_update_hooks']==counts['expected_camera_early_update_hooks'],counts
    counts['expected_intro_model_draw_hooks']=6*int('func_80803A78_chintrochar' in addresses)
    assert counts['intro_model_draw_hooks']==counts['expected_intro_model_draw_hooks'],counts
    counts['expected_scene_observer_hooks']=int('func_800EA05C' in addresses)+12*int('func_800A72A4' in addresses)
    assert counts['scene_observer_hooks']==counts['expected_scene_observer_hooks'],counts
    counts['expected_guest_update_observers']=int('func_800A73F4' in addresses)
    assert counts['guest_update_observers']==counts['expected_guest_update_observers'],counts
    counts['expected_game_delta_observers']=int('func_800123F4' in addresses)
    assert counts['game_delta_observers']==counts['expected_game_delta_observers'],counts
    counts['expected_replay_timing_hooks']=(
        2*int('func_8001608C' in addresses)+2*int('func_800151BC' in addresses))
    assert counts['replay_timing_hooks']==counts['expected_replay_timing_hooks'],counts
    counts['guest_native_aspect']=True
    counts['guest_native_aspect_replacement_required']=int('func_80015D14' in addresses)
    counts['expected_native_aspect_hooks']=0
    assert counts['native_aspect_hooks']==counts['expected_native_aspect_hooks'],counts
    counts['expected_actor_draw_distance_hooks']=2*int('func_80101970' in addresses)
    assert counts['actor_draw_distance_hooks']==counts['expected_actor_draw_distance_hooks'],counts
    counts['expected_hud_counter_layout_hooks']=int('func_800FA508' in addresses)
    assert counts['hud_counter_layout_hooks']==counts['expected_hud_counter_layout_hooks'],counts
    counts['expected_hud_counter_rect_hooks']=2*int('scinfobar_entrypoint_24' in addresses)
    assert counts['hud_counter_rect_hooks']==counts['expected_hud_counter_rect_hooks'],counts
    counts['expected_egg_reticle_ortho_hooks']=int('baeggcursor_entrypoint_1' in addresses)
    assert counts['egg_reticle_ortho_hooks']==counts['expected_egg_reticle_ortho_hooks'],counts
    counts['expected_cutscene_skip_input_hooks']=int('func_808005AC_chintroticker' in addresses)
    assert counts['cutscene_skip_input_hooks']==counts['expected_cutscene_skip_input_hooks'],counts
    counts['expected_music_volume_hooks']=sum(int(name in addresses) for name in (
        'func_800FB968','func_80017404'))
    assert counts['music_volume_hooks']==counts['expected_music_volume_hooks'],counts
    counts['overlay_hooks']=overlay_counts
    return counts
