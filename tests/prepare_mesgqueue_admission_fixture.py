#!/usr/bin/env python3
"""Extract authenticated selected admission bodies for the focused MQ01 fixture."""
import hashlib
import importlib.util
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CHILD = 'baseline/lifecycle/mesgqueue.cpp'
STRUCT = 'struct QueuedMessage {\n'
BODIES = (
    'void ultramodern::enqueue_external_message(PTR(OSMesgQueue) mq, OSMesg msg, bool jam, bool requeue_if_blocked) {\n',
    'void dequeue_external_messages(RDRAM_ARG1) {\n',
)
PERSISTENT = '\n#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT\nextern "C" bool tooie_persistent_external_export('
# Check every ordinary retry, including the two wait paths not run by this fixture.
ROUTES = {
    'external_messages.enqueue(': 1,
    'ultramodern::enqueue_external_message(cur_mesg.mq, cur_mesg.mesg, cur_mesg.jam, cur_mesg.requeue_if_blocked);': 1,
    'ultramodern::enqueue_external_message(to_send.mq, to_send.mesg, to_send.jam, to_send.requeue_if_blocked);': 2,
    'ultramodern::enqueue_external_message(mq_, msg, jam, false);': 2,
}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def block(text, anchor):
    require(text.count(anchor) == 1, 'MQ01 fixture anchor drift: ' + anchor.strip())
    start = text.index(anchor)
    depth = 0
    for at in range(start + len(anchor) - 2, len(text)):
        depth += {'{': 1, '}': -1}.get(text[at], 0)
        if depth == 0:
            end = at + 1
            if text[end:end + 1] == ';':
                end += 1
            return text[start:end] + '\n'
    raise RuntimeError('Unterminated MQ01 fixture block')


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: prepare_mesgqueue_admission_fixture.py CHILD OUTPUT_DIRECTORY')
    child, output = Path(sys.argv[1]), Path(sys.argv[2])
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location('selected', ROOT / 'tools/prepare_runtime_baseline_selected.py')
    selected = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(selected)
    data = child.read_bytes()
    require(hashlib.sha256(data).hexdigest() == selected.EXPECTED[CHILD], 'Selected mesgqueue child SHA differs')
    text = data.decode('utf-8')
    require(text.count(PERSISTENT) == 1, 'Persistent export boundary drift')
    ordinary = text[:text.index(PERSISTENT)]
    for route, count in ROUTES.items():
        require(ordinary.count(route) == count, 'MQ01 admission routing drift: ' + route)
    structure = block(text, STRUCT)
    bodies = ''.join(block(text, anchor) for anchor in BODIES)
    output.mkdir(parents=True, exist_ok=True)
    (output / 'mesgqueue_admission_struct.inl').write_bytes(structure.encode('utf-8'))
    (output / 'mesgqueue_admission_bodies.inl').write_bytes(bodies.encode('utf-8'))


if __name__ == '__main__':
    main()
