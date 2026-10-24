"""Selected raw transport hooks; typed guest owns cutscene policy."""
GLOBAL_FUNCTION='func_80800034_glglobalsettings'
CUTSCENE_FUNCTION='func_80800018_glcutDll'
PROJECTION_FUNCTION='func_80015D14'
# Exact existing instrument_continuous.WIDESCREEN_PROFILE_QUERY_HOOK law.
GLOBAL_TEXT=('uint32_t tooie_widescreen_settings = tooie_widescreen_latched_settings_word();\n'
             '    if (tooie_widescreen_settings & 1u)\n'
             '        ctx->r2 = (gpr)(int32_t)((tooie_widescreen_settings >> 1) & 1u);')
CUTSCENE_TEXT='baseline_cutscene_command(rdram, ctx);'
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
# Orbit pose: after func_80110588's mode update, before it copies the position
# and rotation for the view (both of its paths meet at L_8011061C), so the
# game's shake and roll are applied on top.
ORBIT_CAMERA_TEXT=('{ extern void tooie_camera_orbit_apply(uint8_t*, recomp_context*); '
    'tooie_camera_orbit_apply(rdram, ctx); }')
# Instant intro skip: the startup intro ticker (func_808005AC_chintroticker)
# only honours Start once global flag 0xD56 is set, which is not true at the
# start of the intro. Treat it as set; the transition check and frontend exit
# still run.
INTRO_SKIP_TEXT='ctx->r2 = 1;'
FRUSTUM_HOOKS=(('func_808005AC_chintroticker',0x80800644,INTRO_SKIP_TEXT),
               ('func_800A4878',0x800A4908,ANALOG_CAMERA_TEXT),
               ('func_80110588',0x8011061C,ORBIT_CAMERA_TEXT),
               ('func_800CA218',0x800CA218,_pad('r5')),
               ('func_800CA2A8',0x800CA2A8,_pad('r5')),
               ('func_800CA1B0',0x800CA200,_pad('r6')))
# Model draw distance and LOD (Lighthouse's Extended Draw Distance / Disable LOD,
# GraphicsPatches.cpp). func_800DE498 (model render) culls a model when its focus
# distance (sp+0xDC) reaches the draw distance at 0x8012C82C; func_800DF428 sets
# that per actor and func_800DE318 resets it after every model. With LOD off the
# small-and-far shrink (4000 units, size x 50) is skipped, as in Lighthouse; at
# L_800DE68C, which every path reaches once per model, the distance is scaled.
# The three LOD geo commands (type 8 of the 0x80123198 tables) draw their branch
# when the squared distance lies in (min^2, max^2]; Lighthouse forces distance 1
# so the nearest, full-detail branch is chosen.
LOD_SKIP_TEXT=('{ extern int tooie_model_lod_disabled(void); '
    'if (tooie_model_lod_disabled()) goto L_800DE68C; }')
DRAW_DISTANCE_TEXT=('{ extern float tooie_model_draw_distance_scale(void); '
    'const float tooie_scale = tooie_model_draw_distance_scale(); '
    'if (tooie_scale != 1.0f) { union { uint32_t u; float f; } tooie_distance = '
    '{ (uint32_t)MEM_W(-0x37D4, (gpr)(int32_t)0x80130000) }; tooie_distance.f *= tooie_scale; '
    'MEM_W(-0x37D4, (gpr)(int32_t)0x80130000) = (int32_t)tooie_distance.u; } }')
LOD_GEO_TEXT=('{ extern int tooie_model_lod_disabled(void); '
    'if (tooie_model_lod_disabled()) ctx->f0.fl = 1.0f; }')
# First-person mouse aim: func_808002A4_bafpctrl (first-person control) subtracts
# stick x speed x dt from pitch and yaw, then clamps pitch at 0x80800368 (f12 =
# pitch, f6 = yaw, stored by the call's delay slot). Only reached while
# first-person control is active.
MOUSE_AIM_TEXT=('{ extern void tooie_first_person_aim(uint8_t*, recomp_context*); '
    'tooie_first_person_aim(rdram, ctx); }')
# First-person egg aim, func_80800264_baeggaim: the reticle follows the stick and
# func_80800170 turns the view near the screen edge; the turn is added to yaw
# (sp+0x28) and, unless pitch is locked (func_80095738 flag 0x3A), to pitch
# (sp+0x24) before the clamp at 0x808002F4. Pitch goes in before 0x808002E8 (the
# unlocked path), yaw at L_80800300 (both paths), before the rotation is set.
EGG_AIM_PITCH_TEXT=('{ extern void tooie_egg_aim_pitch(uint8_t*, recomp_context*); '
    'tooie_egg_aim_pitch(rdram, ctx); }')
EGG_AIM_YAW_TEXT=('{ extern void tooie_egg_aim_yaw(uint8_t*, recomp_context*); '
    'tooie_egg_aim_yaw(rdram, ctx); }')
GRAPHICS_HOOKS=(('func_808002A4_bafpctrl',0x80800368,MOUSE_AIM_TEXT),
                ('func_80800264_baeggaim',0x808002E8,EGG_AIM_PITCH_TEXT),
                ('func_80800264_baeggaim',0x80800300,EGG_AIM_YAW_TEXT),
                ('func_800DE498',0x800DE628,LOD_SKIP_TEXT),
                ('func_800DE498',0x800DE68C,DRAW_DISTANCE_TEXT),
                ('func_800DD214',0x800DD298,LOD_GEO_TEXT),
                ('func_800DD2E8',0x800DD32C,LOD_GEO_TEXT),
                ('func_800DD37C',0x800DD3C0,LOD_GEO_TEXT))

def add_hooks(hooks,selected):
    assert type(selected) is bool
    if not selected:return
    for func,pc,text in ((GLOBAL_FUNCTION,0x80800044,GLOBAL_TEXT),(CUTSCENE_FUNCTION,0x80800018,CUTSCENE_TEXT))+FRUSTUM_HOOKS+GRAPHICS_HOOKS:
        assert not any(h['func']==func and h.get('before_vram')==pc for h in hooks),(func,hex(pc))
        hooks.append(dict(func=func,before_vram=pc,text=text))

def strip_and_verify(bodies,selected):
    """Return mechanical inverse for the existing camera/raw proof, not a writer."""
    result=dict(bodies)
    for func,pc,text in ((GLOBAL_FUNCTION,0x80800044,GLOBAL_TEXT),(CUTSCENE_FUNCTION,0x80800018,CUTSCENE_TEXT))+FRUSTUM_HOOKS+GRAPHICS_HOOKS:
        call='    '+text+'\n'
        assert result[func].count(call)==int(selected),(func,hex(pc))
        if selected:
            assert call+'    // 0x%08X:'%pc in result[func],(func,hex(pc))
            result[func]=result[func].replace(call,'')
    return result
