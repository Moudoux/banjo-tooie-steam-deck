"""F4 issue reports (tools/baseline/issue_evidence.cpp): trigger one and read it back."""
import json
import math
import struct
import time
from pathlib import Path

from .framework import Failure


def capture(p, timeout=40):
    """Press F4 and wait for the report (and its RDRAM snapshot) to be written."""
    before = set(p.profile.glob('issue-evidence-*.jsonl'))
    p.key('F4')
    deadline = time.time() + timeout
    while True:
        new = sorted(set(p.profile.glob('issue-evidence-*.jsonl')) - before)
        if new:
            report = new[0]
            # The worker writes the report in one go and the completion row last.
            text = report.read_text(errors='replace')
            if '"type":"completion"' in text:
                return report
        if time.time() > deadline:
            raise Failure('F4 report not written within %ss' % timeout)
        time.sleep(0.3)


def rows(report, kind=None):
    out = []
    for line in Path(report).read_text(errors='replace').splitlines():
        row = json.loads(line)
        if kind is None or row.get('type') == kind:
            out.append(row)
    return out


def matrix(words):
    return [struct.unpack('>f', bytes.fromhex(w))[0] for w in words.split()]


def aspect(words):
    """y scale over x scale (column norms of the view-projection's 3x3 part)."""
    m = matrix(words)
    sx = math.sqrt(sum(m[r * 4] ** 2 for r in range(3)))
    sy = math.sqrt(sum(m[r * 4 + 1] ** 2 for r in range(3)))
    return sy / sx if sx else 0.0
