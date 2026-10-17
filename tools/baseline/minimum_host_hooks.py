"""Selected cutscene transport; guest frame owner applies live widescreen."""
GLOBAL_FUNCTION='func_80800034_glglobalsettings'
CUTSCENE_FUNCTION='func_80800018_glcutDll'
PROJECTION_FUNCTION='func_80015D14'
CUTSCENE_TEXT='baseline_cutscene_command(rdram, ctx);'

def add_hooks(hooks,selected):
    assert type(selected) is bool
    if not selected:return
    assert not any(h['func']==GLOBAL_FUNCTION and h.get('before_vram')==0x80800044 for h in hooks), 'Retired G07 query hook'
    for func,pc,text in ((CUTSCENE_FUNCTION,0x80800018,CUTSCENE_TEXT),):
        assert not any(h['func']==func and h.get('before_vram')==pc for h in hooks),(func,hex(pc))
        hooks.append(dict(func=func,before_vram=pc,text=text))

def strip_and_verify(bodies,selected):
    """Return mechanical inverse for the existing camera/raw proof, not a writer."""
    result=dict(bodies)
    assert 'tooie_widescreen_latched_settings_word(' not in result[GLOBAL_FUNCTION], 'Retired G07 query substitution'
    for func,pc,text in ((CUTSCENE_FUNCTION,0x80800018,CUTSCENE_TEXT),):
        call='    '+text+'\n'
        assert result[func].count(call)==int(selected),(func,hex(pc))
        if selected:
            assert call+'    // 0x%08X:'%pc in result[func],(func,hex(pc))
            result[func]=result[func].replace(call,'')
    return result
