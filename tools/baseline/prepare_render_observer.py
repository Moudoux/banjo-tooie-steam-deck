"""Materialize the exact reviewed, bounded baseline CPU observer; no fuzzy patching."""
import argparse
import hashlib
import re
from pathlib import Path


def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()


def materialize(source, patch, observer, output):
    assert sha(source) == '6e53030e8899c42347b51e67879fcb9679ad487ac067c192612c432d85bca4a4', 'queue input pin'
    assert sha(patch) == 'aa062e613fdd373c494a393d8c893477dae40d75de174740b3a6c41067010b1f', 'patch pin'
    assert sha(observer) == 'b39d0fc819f6b33f3e054938c6360c7bf4306601c52288df0cf9dc6be3fb6051', 'observer pin'
    lines=source.read_text().splitlines(keepends=True)
    delta=patch.read_text().splitlines(keepends=True)
    result=[]; cursor=0; hunks=0; i=2
    while i<len(delta):
        match=re.fullmatch(r'@@ -(\d+),\d+ \+\d+,\d+ @@\n',delta[i])
        assert match, delta[i]
        start=int(match[1])-1; result.extend(lines[cursor:start]); cursor=start; i+=1; hunks+=1
        while i<len(delta) and not delta[i].startswith('@@'):
            row=delta[i]; i+=1
            if row[0] in ' -':
                assert lines[cursor] == row[1:], 'exact hunk mismatch'
                cursor+=1
            if row[0] in ' +': result.append(row[1:])
    result.extend(lines[cursor:]); data=''.join(result).encode()
    assert hunks == 3
    assert hashlib.sha256(data).hexdigest() == '3e0aefd9c1a9ba117c0af76f2d95b8f3735a3b72ceee2acdba38560406515a54'
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_bytes(data)
    (output.parent/'frame_match_observer.inl').write_bytes(observer.read_bytes())


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('source','patch','observer','output'): parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    materialize(args.source,args.patch,args.observer,args.output)
