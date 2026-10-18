#!/usr/bin/env python3
"""Extract the pinned native SP worker, or check its parent negative control."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

PINS = {
    'selected': '585ba081a6cdc99b2fcf90cb74f94ea33c01b2f2a92ede5324efa506a6a7089e',
    'parent': '96f362c7a774c4629a2e9523ecea748a1865bc9cb54de177207c486a6e7ccf4c',
}

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    if sys.argv[1] == '--check-negative':
        result = subprocess.run([sys.argv[2], 'runtime-error'], capture_output=True, text=True, encoding='utf-8', timeout=8)
        if result.returncode != 73 or result.stdout != '' or result.stderr != 'SP_PARENT_TERMINATE_VERIFIED\n':
            raise RuntimeError('Parent negative oracle differs: ' + repr((result.returncode, result.stdout, result.stderr)))
        return
    mode, source, output = sys.argv[1:]
    data = Path(source).read_bytes()
    if sha(data) != PINS[mode]:
        raise RuntimeError('SP source SHA differs')
    text = data.decode('utf-8')
    start = 'void task_thread_func(uint8_t* rdram, moodycamel::LightweightSemaphore* thread_ready) {'
    end = '\nstd::atomic_uint32_t display_refresh_rate = 60;'
    if text.count(start) != 1 or text.count(end) != 1:
        raise RuntimeError('SP worker anchors differ')
    body = text[text.index(start):text.index(end)]
    if not body.endswith('}\n') or body.count('ultramodern::rsp::run_task(PASS_RDRAM task)') != 1:
        raise RuntimeError('SP worker boundary differs')
    target = Path(output)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(body.encode())
    target.with_suffix('.json').write_text(json.dumps(dict(mode=mode, source_sha256=sha(data), body_sha256=sha(body.encode()), extractor_sha256=sha(Path(__file__).read_bytes())), indent=2) + '\n', encoding='utf-8')

if __name__ == '__main__':
    main()
