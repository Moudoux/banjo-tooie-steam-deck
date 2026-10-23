"""Selected cutscene transport; guest frame owner applies live widescreen."""
GLOBAL_FUNCTION='func_80800034_glglobalsettings'
CUTSCENE_FUNCTION='func_80800018_glcutDll'
PROJECTION_FUNCTION='func_80015D14'
CUTSCENE_TEXT='baseline_cutscene_command(rdram, ctx);'
# chintroticker descriptor +0x0C update at its original actor+0x79 consumer
# (lbu v0,0x79(s3)), and +0x28 destruction entry dispatched once by FF3EC.
TICKER_UPDATE_FUNCTION='func_808005AC_chintroticker'
TICKER_DESTROY_FUNCTION='func_808003BC_chintroticker'
HOOKS=((CUTSCENE_FUNCTION,0x80800018,CUTSCENE_TEXT),
       (TICKER_UPDATE_FUNCTION,0x80800614,'baseline_cutscene_ticker_update(rdram, ctx);'),
       (TICKER_DESTROY_FUNCTION,0x808003BC,'baseline_cutscene_ticker_destroy(rdram, ctx);'))
# The thunk passes s3 as the actor; require that exact original consumer.
TICKER_ACTOR_ANCHOR='    // 0x80800614: lbu         $v0, 0x79($s3)\n'
# Frustum slack: the camera planes are built from exact half-angles (in
# degrees), so widescreen edges and interpolated frames showed culled geometry
# while the camera turned. Each hook widens one builder's angle argument
# before use: a1 at entry of func_800CA218/func_800CA2A8, and the side-plane
# angle in a2 just before func_800CA1B0 rotates it. Projection is unchanged.
FRUSTUM_PAD_DEGREES='5.0f'
def _pad(register):
    return ('{ union { uint32_t u; float f; } tooie_pad = { (uint32_t)ctx->%s }; '
            'tooie_pad.f += %s; ctx->%s = (gpr)(int32_t)tooie_pad.u; }' % (register, FRUSTUM_PAD_DEGREES, register))
# Analog orbit camera, the retired frontend's func_800A4878 integration: after
# the original input gate (v0 == 0 selects the C-left/C-right path), a nonzero
# right-stick X feeds the original rotate routine proportionally and resumes at
# the shared exit; otherwise the original instructions run unchanged.
ANALOG_CAMERA_TEXT=('{ extern int tooie_camera_analog_apply(uint8_t*, recomp_context*); '
    'const int tooie_camera_result = (ctx->r2 == 0) ? tooie_camera_analog_apply(rdram, ctx) : -1; '
    'if (tooie_camera_result >= 0) { ctx->r2 = tooie_camera_result; goto L_800A4964; } }')
# Orbit pose: before func_80110588 hands the position/rotation copies to the view.
ORBIT_CAMERA_TEXT=('{ extern void tooie_camera_orbit_apply(uint8_t*, recomp_context*); '
    'tooie_camera_orbit_apply(rdram, ctx); }')
FRUSTUM_HOOKS=(('func_800A4878',0x800A4908,ANALOG_CAMERA_TEXT),
               ('func_80110588',0x8011066C,ORBIT_CAMERA_TEXT),
               ('func_800CA218',0x800CA218,_pad('r5')),
               ('func_800CA2A8',0x800CA2A8,_pad('r5')),
               ('func_800CA1B0',0x800CA200,_pad('r6')))

def add_hooks(hooks,selected):
    assert type(selected) is bool
    if not selected:return
    assert not any(h['func']==GLOBAL_FUNCTION and h.get('before_vram')==0x80800044 for h in hooks), 'Retired G07 query hook'
    for func,pc,text in HOOKS+FRUSTUM_HOOKS:
        assert not any(h['func']==func and h.get('before_vram')==pc for h in hooks),(func,hex(pc))
        hooks.append(dict(func=func,before_vram=pc,text=text))

def strip_and_verify(bodies,selected):
    """Return mechanical inverse for the existing camera/raw proof, not a writer."""
    result=dict(bodies)
    assert 'tooie_widescreen_latched_settings_word(' not in result[GLOBAL_FUNCTION], 'Retired G07 query substitution'
    for func,pc,text in HOOKS+FRUSTUM_HOOKS:
        call='    '+text+'\n'
        assert result[func].count(call)==int(selected),(func,hex(pc))
        if selected:
            assert call+'    // 0x%08X:'%pc in result[func],(func,hex(pc))
            assert func!=TICKER_UPDATE_FUNCTION or result[func].count(call+TICKER_ACTOR_ANCHOR)==1,(func,hex(pc))
            result[func]=result[func].replace(call,'')
    return result
