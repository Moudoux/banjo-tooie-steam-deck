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
# Start skips story cutscenes: in the cutscene chains (intro, Jingaling,
# Bottles, ending; the ticker's map table) the ticker skips to the chain's end
# when func_80016B30(0, BUTTON_Z) reports a fresh press. Count a fresh Start
# (BUTTON_START, index 0 of controller 0's table at 0x800799E0) as well; the
# ticker's own table lookup and exit still decide what happens.
START_SKIP_TEXT=('{ extern int tooie_cutscene_start_skip(void); '
    'if (ctx->r2 != 1 && MEM_W(0x9E0, (gpr)(int32_t)0x80079000) == 1 && tooie_cutscene_start_skip()) ctx->r2 = 1; }')
FRUSTUM_HOOKS=(('func_808005AC_chintroticker',0x80800644,INTRO_SKIP_TEXT),
               ('func_808005AC_chintroticker',0x80800684,START_SKIP_TEXT),
               ('func_800A4878',0x800A4908,ANALOG_CAMERA_TEXT),
               ('func_80110588',0x8011061C,ORBIT_CAMERA_TEXT),
               ('func_800CA218',0x800CA218,_pad('r5')),
               ('func_800CA2A8',0x800CA2A8,_pad('r5')),
               ('func_800CA1B0',0x800CA200,_pad('r6')))
# Draw distance and LOD (Lighthouse's Extended Draw Distance / Disable LOD,
# GraphicsPatches.cpp). func_800E42C0 returns the viewport's draw-distance scale
# (+0x164), and every distance cull multiplies by it: func_800DE498 (model render:
# focus distance sp+0xDC against 0x8012C82C, set per actor by func_800DF428),
# func_800D3104 (cubes within camera cube +-4 x scale) with func_800D2F44 (cube
# distance), and func_800E1C00 (level geometry, fading 500 units before the
# limit). Scaling its result after the call at 0x800E42CC extends all of them, as
# Lighthouse scales the model cull, cube width and cube distance. With LOD off the
# small-and-far model shrink (4000 units, size x 50) is skipped, as in Lighthouse.
# The three LOD geo commands (type 8 of the 0x80123198 tables) draw their branch
# when the squared distance lies in (min^2, max^2]; Lighthouse forces distance 1
# so the nearest, full-detail branch is chosen.
LOD_SKIP_TEXT=('{ extern int tooie_model_lod_disabled(void); '
    'if (tooie_model_lod_disabled()) goto L_800DE68C; }')
DRAW_DISTANCE_TEXT=('{ extern float tooie_model_draw_distance_scale(void); '
    'ctx->f0.fl *= tooie_model_draw_distance_scale(); }')
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
# Breegull Blaster (bsfirstp) right stick, driven like the orbit camera rather
# than through the Blaster's own smoothed stick inputs (func_80804848 ->
# func_808045FC, with a ramp and, in control type 1, a position-based look).
# Turn: func_80809AF8 adds its yaw delta (sp+0x28) to the facing after the call
# at 0x80809BB0. Look: func_8080682C steps +0x20C, then clamps it from
# 0x80806890. Type 1's look target (f2) is stored at 0x80806998 in func_80806928.
BLASTER_TURN_TEXT=('{ extern void tooie_blaster_turn(uint8_t*, recomp_context*); '
    'tooie_blaster_turn(rdram, ctx); }')
BLASTER_LOOK_TEXT=('{ extern void tooie_blaster_look(uint8_t*, recomp_context*); '
    'tooie_blaster_look(rdram, ctx); }')
# Type 0 eases to its level angle (f0, stored at 0x808069D8 in func_808069BC)
# while +0x254 is set. With the analog camera on, func_80804848's left stick X
# strafes (added to a1, the C strafe axis, before 0x80804938 / L_8080499C) and
# its turn input (a1 before 0x80804960 / 0x808049C4) is zeroed.
def _native_call(name):
    return '{ extern void %s(uint8_t*, recomp_context*); %s(rdram, ctx); }' % (name, name)
# Actor matrix groups (src/object_interpolation.cpp, src/menu_interpolation.cpp):
# func_8010A79C, the actor draw dispatcher, calls the template's draw at 0x8010A7E8
# with the actor in a0 and returns to 0x8010A7F0, which no branch reaches.
# func_800DE498 (model render) for draws outside any actor (the player): entry,
# and its single return path at L_800DF310.
ACTOR_DRAW_HOOKS=(('func_8010A79C',0x8010A7E8,_native_call('tooie_actor_draw_begin')),
                  ('func_8010A79C',0x8010A7F0,_native_call('tooie_actor_draw_end')),
                  ('func_800DE498',0x800DE498,_native_call('tooie_model_draw_begin')),
                  ('func_800DE498',0x800DF310,_native_call('tooie_model_draw_end')))
# Minimap (src/level_minimap.cpp): bainput_update entry, once per character
# update with the PlayerState in a0, samples the player, camera and level
# collision on the guest thread.
MINIMAP_HOOKS=(('bainput_update',0x8009819C,_native_call('tooie_level_minimap_observe')),)
def _blaster_call(name, control_type):
    return ('{ extern void %s(uint8_t*, recomp_context*, int); %s(rdram, ctx, %d); }'
            % (name, name, control_type))
GRAPHICS_HOOKS=(('func_808002A4_bafpctrl',0x80800368,MOUSE_AIM_TEXT),
                ('func_80809AF8_bsfirstp',0x80809BB8,BLASTER_TURN_TEXT),
                ('func_8080682C_bsfirstp',0x80806890,BLASTER_LOOK_TEXT),
                ('func_80806928_bsfirstp',0x80806998,_blaster_call('tooie_blaster_look_target',1)),
                ('func_808069BC_bsfirstp',0x808069D8,_blaster_call('tooie_blaster_look_target',0)),
                ('func_80804848_bsfirstp',0x80804938,_blaster_call('tooie_blaster_strafe',0)),
                ('func_80804848_bsfirstp',0x8080499C,_blaster_call('tooie_blaster_strafe',1)),
                ('func_80804848_bsfirstp',0x80804960,_blaster_call('tooie_blaster_left_turn',0)),
                ('func_80804848_bsfirstp',0x808049C4,_blaster_call('tooie_blaster_left_turn',1)),
                ('func_80800264_baeggaim',0x808002E8,EGG_AIM_PITCH_TEXT),
                ('func_80800264_baeggaim',0x80800300,EGG_AIM_YAW_TEXT),
                ('func_800DE498',0x800DE628,LOD_SKIP_TEXT),
                ('func_800E42C0',0x800E42D4,DRAW_DISTANCE_TEXT),
                ('func_800DD214',0x800DD298,LOD_GEO_TEXT),
                ('func_800DD2E8',0x800DD32C,LOD_GEO_TEXT),
                ('func_800DD37C',0x800DD3C0,LOD_GEO_TEXT))+ACTOR_DRAW_HOOKS+MINIMAP_HOOKS

def add_hooks(hooks,selected):
    assert type(selected) is bool
    if not selected:return
    assert not any(h['func']==GLOBAL_FUNCTION and h.get('before_vram')==0x80800044 for h in hooks), 'Retired G07 query hook'
    for func,pc,text in HOOKS+FRUSTUM_HOOKS+GRAPHICS_HOOKS:
        assert not any(h['func']==func and h.get('before_vram')==pc for h in hooks),(func,hex(pc))
        hooks.append(dict(func=func,before_vram=pc,text=text))

def strip_and_verify(bodies,selected):
    """Return mechanical inverse for the existing camera/raw proof, not a writer."""
    result=dict(bodies)
    assert 'tooie_widescreen_latched_settings_word(' not in result[GLOBAL_FUNCTION], 'Retired G07 query substitution'
    for func,pc,text in HOOKS+FRUSTUM_HOOKS+GRAPHICS_HOOKS:
        call='    '+text+'\n'
        assert result[func].count(call)==int(selected),(func,hex(pc))
        if selected:
            assert call+'    // 0x%08X:'%pc in result[func],(func,hex(pc))
            assert func!=TICKER_UPDATE_FUNCTION or result[func].count(call+TICKER_ACTOR_ANCHOR)==1,(func,hex(pc))
            result[func]=result[func].replace(call,'')
    return result
