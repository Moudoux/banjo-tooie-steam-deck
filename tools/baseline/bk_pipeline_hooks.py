"""BK pipeline packet1 hooks (B1 PARSED acknowledgement, B2 frame/root tags).

Selected only with the current player composition. PCs are execution-facts47 bindings.
Two PCs already carry a maintained hook; the BK call is appended after it, never replacing it.
"""
import re

HOOKS = [
    # Explicit native origin only: original model argument is in t5/t6/t3 before delay store.
    ('func_800BE7F8',0x800BE90C,'baseline_bk_origin_slot_begin'),
    ('func_800BE7F8',0x800BE914,'baseline_bk_origin_slot_end'),
    ('func_800BFAD0',0x800BFB40,'baseline_bk_origin_resolved_begin'),
    ('func_800BFAD0',0x800BFB48,'baseline_bk_origin_resolved_end'),
    ('func_800BFD6C',0x800BFE70,'baseline_bk_origin_background_begin'),
    ('func_800BFD6C',0x800BFE78,'baseline_bk_origin_background_end'),

    # A7840: after both pool-toggle paths and the render/camera construction begin (BK game_draw
    # resets its per-frame tagging state before construction).
    ('func_800A7840', 0x800A7888, 'baseline_bk_frame_begin'),
    # A7840 before the 1A270 JAL: a0 was loaded from sp+0x1C at 78E4 (exact submitted start);
    # a1 is still loaded by the 78EC delay slot. Reaching here proves 789C chose submission.
    ('func_800A7840', 0x800A78E8, 'baseline_bk_submit_register'),
    # A7840 first instruction after 1A270 returns; also the 789C suppression destination.
    ('func_800A7840', 0x800A78F0, 'baseline_bk_submit_wait'),
    # DE498 first instruction, before its prologue: the F4 caller witness reads only a1 and the caller's sp.
    ('func_800DE498', 0x800DE498, 'baseline_bk_de498_entry'),
    # DE498 after the original root LOAD/PUSH (DF080-DF0AC), before transform-context setup.
    ('func_800DE498', 0x800DF0B0, 'baseline_bk_root_begin'),
    # DE498 after the original root pop's word0 store (DF230), before the DF238 tail join.
    ('func_800DE498', 0x800DF234, 'baseline_bk_root_end'),
    # BK GeoCmdD: preserve the original bbox call and zero-child guard, then visit child.
    ('func_800DDC28', 0x800DDCEC, 'baseline_bk_bbox_visible'),
    ('func_800DDD14', 0x800DDD98, 'baseline_bk_bbox_visible'),
    # BK transform_tagging disables every frustum check for the whole geo list (demos included), so its
    # in-graph sphere test passes. Same seam: after the original E3E8C call and its delay slot, at the
    # zero-result branch; each handler's separate child-offset null guard still runs.
    ('func_800DDDC4', 0x800DDE38, 'baseline_bk_bbox_visible'),
    ('func_800DDDC4', 0x800DDEE0, 'baseline_bk_bbox_visible'),
    ('func_800DDF14', 0x800DDF6C, 'baseline_bk_bbox_visible'),
    ('func_800DDFA0', 0x800DE008, 'baseline_bk_bbox_visible'),
    # BK lod_patches GeoCmd8 fixes distance at50; Tooie compares squared distance.
    # These post-distance-return seams preserve null children and DD214 ordinal override.
    ('func_800DD214', 0x800DD294, 'baseline_bk_lod_distance'),
    ('func_800DD2E8', 0x800DD328, 'baseline_bk_lod_distance'),
    ('func_800DD37C', 0x800DD3BC, 'baseline_bk_lod_distance'),
    # @tooie Whole-player resource tier (94070): BK has no such table; the same fixed-detail intent
    # replaces the linear camera distance EEAD4 just returned in f0 (first instruction after the call's
    # delay slot), so the original tier walk, resolver, heap gate, pending and install paths all run.
    ('func_80094070', 0x800940B8, 'baseline_bk_player_lod_distance'),
    # Exact caller scope; callee consumes after the original 9403C tier load.
    ('func_80093FD4', 0x80094038, 'baseline_bk_player_lod_initial_begin'),
    ('func_80093F30', 0x80093F30, 'baseline_bk_player_lod_initial'),
    ('func_80093FD4', 0x80094040, 'baseline_bk_player_lod_initial_end'),
]
# Maintained hooks the BK call composes after (execution-facts47 B1/B2 bindings).
COMPOSED = {0x800A7888: 'baseline_render_construction_begin(rdram, ctx);\n    baseline_camera_construction_begin(rdram, ctx);',
            0x800A78E8: 'baseline_first_graphics_submit(rdram, ctx);'}
# Original visibility-predicate seams: (function, result-branch PC, predicate, exact delay statement or None).
# The JAL is at PC-8 and its delay slot at PC-4, so the original call and its side effects complete before
# the hook; only v0 at the branch changes. None: the delay statement text is not pinned, only its slot.
PREDICATE_SEAMS = [
    ('func_800DDC28', 0x800DDCEC, 0x800E3D0C, 'MEM_W(0X4C, ctx->r29) = ctx->r6;'),
    ('func_800DDD14', 0x800DDD98, 0x800E3D0C, 'ctx->r5 = ADD32(ctx->r29, 0X28);'),
    ('func_800DDDC4', 0x800DDE38, 0x800E3E8C, None),
    ('func_800DDDC4', 0x800DDEE0, 0x800E3E8C, None),
    ('func_800DDF14', 0x800DDF6C, 0x800E3E8C, None),
    ('func_800DDFA0', 0x800DE008, 0x800E3E8C, None),
]
assert [(f, pc) for f, pc, _, _ in PREDICATE_SEAMS] == [(f, pc) for f, pc, n in HOOKS if n == 'baseline_bk_bbox_visible']


def add_hooks(hooks, selected):
    assert type(selected) is bool
    if not selected:
        return
    for func, pc, name in HOOKS:
        same = [h for h in hooks if h['func'] == func and h.get('before_vram') == pc]
        call = name + '(rdram, ctx);'
        if pc in COMPOSED:
            assert len(same) == 1 and same[0]['text'].startswith(COMPOSED[pc]) and call not in same[0]['text'], (func, hex(pc), same)
            same[0]['text'] = same[0]['text'] + '\n    ' + call
        elif pc in (0x800BFE70,0x800BFE78):
            # Exact optional predecessor from prepare_camera_bridge; no arbitrary composition.
            expected={0x800BFE70:'baseline_pre_slot(rdram, ctx);',
                      0x800BFE78:'baseline_pre_background_end(rdram, ctx);'}[pc]
            assert len(same)<=1, (func,hex(pc),same)
            if same:
                assert same[0]['text']==expected, (func,hex(pc),same)
                same[0]['text'] += '\n    '+call
            else:
                hooks.append(dict(func=func,before_vram=pc,text=call))
        else:
            assert not same, (func, hex(pc))
            hooks.append(dict(func=func, before_vram=pc, text=call))


def verify_predicate_seam(body, func, pc, target, delay):
    """The hook follows the original predicate call and its delay slot, directly before the result branch."""
    start = body.index('    // 0x%08X:' % (pc - 8))
    end = body.index('    // 0x%08X:' % pc, start)
    segment, call, hook = body[start:end], 'func_%08X(rdram, ctx);' % target, 'baseline_bk_bbox_visible(rdram, ctx);'
    assert segment.startswith('    // 0x%08X: jal         0x%08X\n' % (pc - 8, target)) and segment.count(call) == 1, (func, hex(pc), 'predicate target')
    slot = segment.index('    // 0x%08X:' % (pc - 4))
    assert slot < segment.index(call) and (delay is None or slot < segment.index(delay) < segment.index(call)), (func, hex(pc), 'completed delay')
    assert segment.count(hook) == 1 and segment.index(call) < segment.index(hook), (func, hex(pc), 'post-call seam')
    assert re.search(r'^    // 0x%08X: (?:(?:beq|bne)l?\s+\$v0, \$zero|(?:beqz|bnez)l?\s+\$v0),' % pc, body, re.M), (func, hex(pc), 'v0/zero branch')
    assert not re.search(r'^\s*L_%08X:' % pc, body, re.M), (func, hex(pc), 'post-return seam')


def strip_and_verify(after, selected):
    """Exact inverse: each call once per registered PC (only later composed calls between), then removed."""
    assert type(selected) is bool
    if selected:
        for func, pc, target, delay in PREDICATE_SEAMS:
            verify_predicate_seam(after[func], func, pc, target, delay)
    if selected:
        body = after['func_80093FD4']
        segment = body[body.index('    // 0x80094038:'):body.index('    // 0x80094040:')]
        assert 'jal         0x80093F30' in segment
        assert segment.count('func_80093F30(rdram, ctx);') == 1
        load = 'ctx->r4 = MEM_H(ctx->r25, 0X2);'
        assert load in segment and segment.index(load) < segment.index('func_80093F30(rdram, ctx);')
        assert not re.search(r'^\s*L_80094040:', body, re.M), 'caller end must be return-only'
    if selected:
        for func, pc, reg in [('func_800BE7F8',0x800BE90C,13),('func_800BFAD0',0x800BFB40,14),('func_800BFD6C',0x800BFE70,11)]:
            body=after[func]
            start=body.index('    // 0x%08X:'%pc)
            end=body.index('    // 0x%08X:'%(pc+8),start)
            segment=body[start:end]
            assert 'jal         0x800DE448' in segment and 'func_800DE448(rdram, ctx);' in segment,(func,'original target')
            assert ('MEM_W(0X10, ctx->r29) = ctx->r%d;'%reg) in segment,(func,'original model delay store')
            assert ('ctx->r%d ='%reg) not in segment,(func,'model must precede delay slot')
            assert not re.search(r'^[ \t]*L_%08X:'%(pc+8),body,re.M),(func,'return-only end')
    if selected:
        # DE498 entry: the hook precedes the first instruction (a1 and the caller's sp intact), and the native
        # caller witness subtracts exactly this prologue frame (issue_evidence_guest.hpp de498_frame).
        body = after['func_800DE498']
        assert body.index('    // 0x800DE498:') == body.index('    // 0x'), 'DE498 entry hook must precede its first instruction'
        frames = re.findall(r'ctx->r29 = ADD32\(ctx->r29, -0X([0-9A-F]+)\);', body)
        assert frames and int(frames[0], 16) == 0xF8, ('DE498 prologue frame', frames)
    return strip_calls(after, HOOKS, selected)


def strip_calls(after, hooks, selected):
    """Each (function, hook) call occurs once per registered PC, directly before it; all are then removed."""
    for func, name in dict.fromkeys((f, n) for f, _, n in hooks):
        call = '    ' + name + '(rdram, ctx);\n'
        pcs = [p for f, p, n in hooks if (f, n) == (func, name)]
        assert after[func].count(call) == len(pcs) * int(selected), (func, name)
        if selected:
            for pc in pcs:
                assert re.search(re.escape(call) + r'(?:    \w+\(rdram, ctx\);\n)*' + re.escape('    // 0x%08X:' % pc), after[func]), (func, hex(pc))
            after[func] = after[func].replace(call, '')
    return after
