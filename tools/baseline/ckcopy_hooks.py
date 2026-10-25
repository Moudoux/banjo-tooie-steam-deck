"""P04b ckpack copy-storage hooks (HPC1-4). Selected only with the current player composition."""
CKCOPY_HOOKS = [
    # After both pool-toggle paths merge (A7874); first instruction of the A73F4 argument setup.
    ('func_800A7840',0x800A7884,'baseline_ckcopy_root_begin'),
    # A73F4 returned (label before A7890); before the A789C branch and the A78A8 DP wait.
    ('func_800A7840',0x800A7890,'baseline_ckcopy_root_end'),
    # First instruction after DE8C4 stores the segment-1 address word; sp+0xB8 holds the command.
    ('func_800DE498',0x800DE8C8,'baseline_ckcopy_segment'),
    # After 801009C4 saves a0/a1 (009DC), before any vertex free (009F8); a0 is the record.
    ('func_801009C4',0x801009E0,'baseline_ckcopy_release'),
]

def add_hooks(hooks,selected):
    assert type(selected) is bool
    if not selected:return
    for func,pc,name in CKCOPY_HOOKS:
        # HPC3: none of these PCs carries an existing hook; never compose silently.
        assert not any(h['func']==func and h.get('before_vram')==pc for h in hooks),(func,hex(pc))
        hooks.append(dict(func=func,before_vram=pc,text=name+'(rdram, ctx);'))

def strip_and_verify(after,selected):
    """Exact inverse: each call once, directly before its PC comment, then removed."""
    assert type(selected) is bool
    for func,pc,name in CKCOPY_HOOKS:
        call='    '+name+'(rdram, ctx);\n'
        assert after[func].count(call)==int(selected),(func,name)
        if selected:
            assert call+'    // 0x%08X:'%pc in after[func],(func,hex(pc))
            after[func]=after[func].replace(call,'')
    return after
