#!/usr/bin/env python3
"""Linux / Steam Deck port test suite: see linux/tests/README.md.

    python3 linux/tests/run.py                 # static + quick tier
    python3 linux/tests/run.py --tier full     # everything (after a rebase)
    python3 linux/tests/run.py -k minimap -k levitate
    python3 linux/tests/run.py --list
"""
import argparse
import importlib
import json
import sys
import time
import traceback
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from harness.framework import REGISTRY, Context, Skip, Failure  # noqa: E402
from harness.player import ROOT, DEFAULT_BINARY, RunError  # noqa: E402

TIERS = {'static': ['static'], 'quick': ['static', 'quick'], 'full': ['static', 'quick', 'full']}


def load_cases():
    for path in sorted((Path(__file__).resolve().parent / 'cases').glob('*.py')):
        importlib.import_module('cases.' + path.stem)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--tier', choices=TIERS, default='quick')
    parser.add_argument('-k', dest='only', action='append', default=[], help='run tests whose name contains this')
    parser.add_argument('--binary', type=Path, default=DEFAULT_BINARY)
    parser.add_argument('--list', action='store_true')
    parser.add_argument('--out', type=Path, help='run folder (default local/tests/runs/<time>)')
    args = parser.parse_args()
    load_cases()
    tests = [t for t in REGISTRY if t.tier in TIERS[args.tier] and (not args.only or any(k in t.name for k in args.only))]
    tests.sort(key=lambda t: TIERS['full'].index(t.tier))  # Cheap checks first; stable within a tier.
    if args.list:
        for t in tests:
            print('%-6s %-34s %s%s' % (t.tier, t.name, t.feature, ' | trigger: ' + t.trigger if t.trigger else ''))
            for name, feature in getattr(t.function, 'parts', ()):
                print('       %-34s %s' % ('  ' + name, feature))
        return 0
    if not args.binary.exists():
        print('binary missing: %s (build first: docker run ... linux/build.sh)' % args.binary)
        return 2
    run_dir = (args.out or ROOT / 'local/tests/runs' / time.strftime('%Y%m%d-%H%M%S')).resolve()
    run_dir.mkdir(parents=True, exist_ok=True)
    print('run folder: %s\nbinary: %s\n%d tests (%s tier)\n' % (run_dir, args.binary, len(tests), args.tier))
    results = []
    for t in tests:
        ctx = Context(run_dir=run_dir / t.name, binary=args.binary.resolve())
        ctx.run_dir.mkdir(parents=True, exist_ok=True)
        print('%-6s %-34s ' % (t.tier, t.name), end='', flush=True)
        start = time.time()
        try:
            t.function(ctx)
            outcome, detail = 'PASS', ''
        except Skip as error:
            outcome, detail = 'SKIP', str(error)
        except (Failure, RunError) as error:
            outcome, detail = 'FAIL', str(error)
        except Exception:
            outcome, detail = 'ERROR', traceback.format_exc()
        seconds = time.time() - start
        failed_parts = [r for r in ctx.parts if r['outcome'] in ('FAIL', 'ERROR')]
        if outcome == 'PASS' and failed_parts:
            outcome = 'FAIL'
        notes = '; '.join(ctx.notes)
        print('%-5s %5.0fs  %s' % (outcome, seconds, notes))
        if detail:
            print('       ' + detail.strip().replace('\n', '\n       '))
        for r in ctx.parts:
            print('         %-5s %-26s %s' % (r['outcome'], r['name'], '; '.join(r['notes'])))
            if r['detail']:
                print('               ' + r['detail'].strip().replace('\n', '\n               '))
        (ctx.run_dir / 'result.txt').write_text('%s\n%s\n%s\n' % (outcome, notes, detail))
        results.append(dict(name=t.name, tier=t.tier, feature=t.feature, trigger=t.trigger, outcome=outcome,
                            seconds=round(seconds, 1), notes=ctx.notes, detail=detail, parts=ctx.parts))
    (run_dir / 'results.json').write_text(json.dumps(results, indent=1) + '\n')
    counts = {o: sum(r['outcome'] == o for r in results) for o in ('PASS', 'FAIL', 'ERROR', 'SKIP')}
    print('\n' + '  '.join('%s %d' % kv for kv in counts.items()) + '   (%s)' % (run_dir / 'results.json'))
    return 0 if not counts['FAIL'] and not counts['ERROR'] else 1


if __name__ == '__main__':
    sys.exit(main())
