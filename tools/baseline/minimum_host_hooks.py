"""Two selected raw transport hooks; typed guest owns cutscene policy."""
GLOBAL_FUNCTION='func_80800034_glglobalsettings'
CUTSCENE_FUNCTION='func_80800018_glcutDll'
PROJECTION_FUNCTION='func_80015D14'
# Exact existing instrument_continuous.WIDESCREEN_PROFILE_QUERY_HOOK law.
GLOBAL_TEXT=('uint32_t tooie_widescreen_settings = tooie_widescreen_latched_settings_word();\n'
             '    if (tooie_widescreen_settings & 1u)\n'
             '        ctx->r2 = (gpr)(int32_t)((tooie_widescreen_settings >> 1) & 1u);')
CUTSCENE_TEXT='baseline_cutscene_command(rdram, ctx);'

def add_hooks(hooks,selected):
    assert type(selected) is bool
    if not selected:return
    for func,pc,text in ((GLOBAL_FUNCTION,0x80800044,GLOBAL_TEXT),(CUTSCENE_FUNCTION,0x80800018,CUTSCENE_TEXT)):
        assert not any(h['func']==func and h.get('before_vram')==pc for h in hooks),(func,hex(pc))
        hooks.append(dict(func=func,before_vram=pc,text=text))

def strip_and_verify(bodies,selected):
    """Return mechanical inverse for the existing camera/raw proof, not a writer."""
    result=dict(bodies)
    for func,pc,text in ((GLOBAL_FUNCTION,0x80800044,GLOBAL_TEXT),(CUTSCENE_FUNCTION,0x80800018,CUTSCENE_TEXT)):
        call='    '+text+'\n'
        assert result[func].count(call)==int(selected),(func,hex(pc))
        if selected:
            assert call+'    // 0x%08X:'%pc in result[func],(func,hex(pc))
            result[func]=result[func].replace(call,'')
    return result
