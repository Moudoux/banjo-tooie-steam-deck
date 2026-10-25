#!/usr/bin/env python3
"""Supervise MQ01 cases; required refusal must reach the real quick-exit sink."""
import subprocess
import sys

FATAL = "Exiting with exit status '1'. Function enqueue_external_message"
CASES = {'admitted': 0, 'droppable-refused': 0, 'retry': 0, 'required-refused': 1, 'retry-refused': 1}


def main():
    if len(sys.argv) != 2:
        raise SystemExit('usage: run_mesgqueue_admission_fixture.py FIXTURE_EXECUTABLE')
    for case, code in CASES.items():
        run = subprocess.run([sys.argv[1], case], capture_output=True, text=True, encoding='utf-8', timeout=8)
        if (run.returncode != code or run.stdout != '' or
                run.stderr.count(FATAL) != (1 if code else 0) or 'returned' in run.stderr):
            raise RuntimeError(f'{case}: exit {run.returncode}, stdout {run.stdout!r}, stderr {run.stderr!r}')
    print('mesgqueue required admission: 5 cases')


if __name__ == '__main__':
    main()
