#!/usr/bin/env python3
"""L01/L02 source contract: one BK helper set across the guest, the preparers, the native asserts, CMake
and the fixture extractor; and no native thunk still deciding the moved game policy.

Source facts only. The image gates and BaselineBkPolicyTest (translated helpers vs the former native
decisions) prove behaviour.

    python -I tests/bk_helper_set_contract_test.py <repo>
"""
import argparse
import ast
import re
from pathlib import Path

GUEST = 'tools/baseline/guest/silo-candidate'
POLICY = ['tooie_bk_lod_distance', 'tooie_bk_player_lod_distance', 'tooie_bk_player_lod_tier',
          'tooie_bk_bbox_visible', 'tooie_bk_billboard_entry', 'tooie_bk_sort_onesided',
          'tooie_bk_branch_submitted', 'tooie_bk_transition_role_begin', 'tooie_bk_transition_role_end']
# Thunk file: guest helpers it must call through a clone; former native decisions it must not keep.
THUNKS = {
    'bk_pipeline_thunks.hpp': (POLICY[:3] + ['tooie_bk_branch_submitted'],
                               ['branch_commit_table', 'tooie_bk_branch_commit(', '2500.0f', '50.0f', 'lod_alias_body', '0x607']),
    'bk_graph_thunks.hpp': (['tooie_bk_bbox_visible', 'tooie_bk_billboard_entry'], ['ctx->r2 = 1', 'TOOIE_BK_OFFSET_WORD_OFFSET']),
    'bk_sort_thunks.hpp': (['tooie_bk_sort_onesided', 'tooie_bk_sort_children'], ['TOOIE_BK_OFFSET_WORD_OFFSET', 'MEM_BU(0x21']),
    'bk_menu_thunks.hpp': (['tooie_bk_transition_role_begin', 'tooie_bk_transition_role_end'],
                           ['role_address', 'TOOIE_CAMERA_TRANSITION_ROLE_FIRST', 'TOOIE_CAMERA_TRANSITION_ROLE_SKIP']),
}


def need(ok, why):
    if not ok:
        raise SystemExit('FAIL: ' + why)


def module_list(path, name):
    """A module-level list built only from `name = [...]` and `name += [...]` literals."""
    values = []
    for node in ast.parse(path.read_text(encoding='utf-8')).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == name for t in node.targets):
            values = list(ast.literal_eval(node.value))
        elif isinstance(node, ast.AugAssign) and isinstance(node.target, ast.Name) and node.target.id == name:
            values += list(ast.literal_eval(node.value))
    return values


def expanded(path):
    """A guest source with its local bk_*.inc includes expanded in place."""
    return re.sub(r'^#include "(bk_\w+\.inc)"$', lambda m: expanded(path.parent / m.group(1)),
                  path.read_text(encoding='utf-8'), flags=re.M)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('repo', type=Path)
    repo = parser.parse_args().repo.resolve()
    tools = repo / 'tools/baseline'
    text = lambda p: p.read_text(encoding='utf-8')

    helpers = module_list(tools / 'prepare_silo_bridge.py', 'BK_HELPERS')
    need(len(helpers) == len(set(helpers)) == 46 and helpers[29:38] == POLICY,
         'BK_HELPERS: the 29 existing helpers, then the 9 policy helpers and 8 text helpers')
    defined = re.findall(r'^BK_HELPER\s+\w+\s+(tooie_bk_\w+)\s*\(', expanded(repo / GUEST / 'bk_pipeline.c'), re.M)
    need(defined == helpers, 'guest definition order equals BK_HELPERS')

    count = str(len(helpers))
    # L06a: only the owner and the fixture test state the count; every consumer takes it from the generated
    # registry table (tests/baseline_registry_counts_test.py checks that none restates it).
    sites = {
        'prepare_silo_bridge.py': re.findall(r'assert len\(BK_HELPERS\) == len\(set\(BK_HELPERS\)\) == (\d+)', text(tools / 'prepare_silo_bridge.py')),
        'baseline_bk_policy_test.cpp': re.findall(r'TOOIE_BK_HELPERS==(\d+)u', text(repo / 'tests/baseline_bk_policy_test.cpp')),
    }
    for site, found in sites.items():
        need(found and set(found) == {count}, '%s helper count %s, expected %s' % (site, found, count))

    allowed = None
    for node in ast.parse(text(repo / 'tests/extract_heap_realloc.py')).body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'DEFAULT_ALLOWED' for t in node.targets):
            allowed = set(ast.literal_eval(node.value))
    need(allowed is not None and set(helpers) <= allowed, 'the fixture extractor admits every BK helper')

    for name, (calls, removed) in THUNKS.items():
        source = text(tools / name)
        for helper in calls:
            need(re.search(re.escape(helper) + r'\(rdram,&clone\);', source), '%s calls %s through a clone' % (name, helper))
        for former in removed:
            need(former not in source, '%s still contains the former native decision %r' % (name, former))
    print('bk_helper_set_contract: ok (%d helpers; source facts only)' % len(helpers))


if __name__ == '__main__':
    main()
