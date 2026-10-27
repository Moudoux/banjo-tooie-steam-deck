#!/usr/bin/env python3
"""Extract the actual startup-thread lambda and event join from pinned children, or check a parent oracle."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
GAME_START = '    std::thread game_thread{[](ultramodern::renderer::WindowHandle window_handle, uint8_t* rdram) {\n'
GAME_END = '    }, window_handle, rdram};\n'
JOIN_START = 'void ultramodern::join_event_threads() {\n'
NEGATIVE = {'recomp': (73, 'STARTUP_PARENT_TERMINATE_VERIFIED\n', 'recomp-renderer-throw'),
            'events': (74, 'EVENTS_PARENT_JOIN_REFUSED\n', 'events-unstarted-vi')}

def sha(data):
    return hashlib.sha256(data).hexdigest()

def require(condition, message):
    if not condition:
        raise RuntimeError(message)

def composer():
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location('selected', ROOT / 'tools/prepare_runtime_baseline_selected.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

def extract(text, start, end):
    require(text.count(start) == 1, 'Start anchor differs')
    begin = text.index(start)
    finish = text.index(end, begin) + len(end)
    return text[begin:finish]

def main():
    if sys.argv[1] == '--check-negative':
        code, marker, mode = NEGATIVE[sys.argv[2]]
        result = subprocess.run([sys.argv[3], mode], capture_output=True, text=True, encoding='utf-8', timeout=8)
        require(result.returncode == code and result.stdout == '' and result.stderr == marker,
                'Parent negative oracle differs: ' + repr((result.returncode, result.stdout, result.stderr)))
        return
    mode, source, output = sys.argv[1:]
    selected = composer()
    pins = {'recomp-selected': selected.EXPECTED['baseline/lifecycle/recomp.cpp'],
            'recomp-parent': selected.INPUTS['librecomp/src/recomp.cpp'],
            'events-player': selected.EXPECTED['baseline/guest-clock/events.cpp'],
            'events-parent': selected.EXPECTED['baseline/lifecycle/events.cpp']}
    data = Path(source).read_bytes()
    require(sha(data) == pins[mode], 'Startup terminal source SHA differs: ' + mode)
    text = data.decode('utf-8')
    if mode.startswith('recomp-'):
        body = extract(text, GAME_START, GAME_END)
        routed = body.count('tooie::lifecycle::fail(std::current_exception());')
        caught = body.count('catch (const ultramodern::thread_terminated&)')
        require((routed, caught) == ((1, 1) if mode == 'recomp-selected' else (0, 0)), 'Game start lambda boundary differs')
        require(body.count('ultramodern::preinit(rdram, window_handle);') == 1, 'Game start preinit differs')
        guarded = body.count('while (!exited.load() && !wait_for_game_started(rdram, &context)) {}')
        plain = body.count('while (!wait_for_game_started(rdram, &context)) {}')
        require((guarded, plain) == ((1, 0) if mode == 'recomp-selected' else (0, 1)), 'Game start loop guard differs')
    else:
        body = extract(text, JOIN_START, '\n}\n')
        guarded = body.count('if (events_context.vi.thread.joinable())')
        require(guarded == (1 if mode == 'events-player' else 0), 'Event join VI guard differs')
        require(body.count('events_context.vi.thread.join();') == 1 and
                body.count('events_context.sp_task_queue.enqueue(nullptr);') == 1, 'Event join boundary differs')
    target = Path(output)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(body.encode())
    target.with_suffix('.json').write_text(json.dumps(dict(mode=mode, source_sha256=sha(data), body_sha256=sha(body.encode()),
        extractor_sha256=sha(Path(__file__).read_bytes())), indent=2) + '\n', encoding='utf-8')

if __name__ == '__main__':
    main()
