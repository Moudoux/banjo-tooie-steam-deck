#!/usr/bin/env python3
"""Opt-in continuous ownership overlay; never writes the pinned checkout.

threads input MUST be the approved cleanup materialization. Each replacement
matches exact pinned source text, with source and resulting hashes in manifest.
"""
import argparse
import difflib
import hashlib
import json
from pathlib import Path
import re
import subprocess

PIN = 'ca568b6ad79b9029d14077f0c3ffa757727c5559'
CLEANUP_SHA = '70582b56f42f0ad6935cb9821f7582a788a12da9f3dff5aa48a67109a9501f74'
BASELINE_THREADS_SHA = '64ddcfdd413bd1a893ab00bae1b7b2ad08f251af446687abd605fa0ae09dabde'
BASELINE_COMPOSED_SHA = '1dd1f42bd82ff50226e5ba2d0824ae1d3f343e08f518d0e208a8feb2adb9c48e'
CLEANUP_PATCH_SHA = '9b4261af055d9677c061065c1d7588e79fcec74f679dac6cf12d3c069e2319fb'
PATCH_ROOT = Path(__file__).resolve().parents[1] / 'patches/runtime'
# Tracked edits for recipes with one behaviour over one pinned original; every mode
# applies the same bytes. This module is itself pinned by the selected composer.
RECIPE_PATCHES = {
    'mesgqueue-lifecycle-wait': ('2e4d6eaa8a609424d04b304f14844265aee7095bc4d355b75d1afef2e362eced', 'mesgqueue.cpp', 3),
    'pi-save-persistence': ('bd015254e4ee9a5c0ff13f3ac3ddda780e9e22e9a7691a7d9daa9fd29ed1685e', 'pi.cpp', 10),
}
def sha(data): return hashlib.sha256(data).hexdigest()
def replace(text, old, new, count=1):
    if text.count(old) != count:
        raise RuntimeError(f'Expected {count} exact contexts, got {text.count(old)}: {old[:120]!r}')
    return text.replace(old,new)

def apply_patch(text, patch, filename, expected_hunks, name):
    # Fixed file headers/counts and unique full contexts; no fuzzy offsets.
    def require(condition, message):
        if not condition:
            raise RuntimeError(message)
    lines = patch.splitlines(keepends=True)
    require([x for x in lines if x.startswith(('--- ', '+++ '))] == ['--- a/' + filename + '\n', '+++ b/' + filename + '\n'], 'Patch headers drift')
    starts = [i for i, line in enumerate(lines) if line.startswith('@@ ')]
    require(len(starts) == expected_hunks, 'Patch hunk count drift: ' + name)
    for n, at in enumerate(starts):
        header = re.fullmatch(r'@@ -\d+,(\d+) \+\d+,(\d+) @@[^\n]*\n', lines[at])
        require(header is not None, 'Unexpected pinned hunk header')
        hunk = lines[at + 1:starts[n + 1] if n + 1 < len(starts) else len(lines)]
        require(all(x[:1] in (' ', '-', '+') for x in hunk), 'Unexpected pinned hunk line')
        before = ''.join(x[1:] for x in hunk if x[:1] in (' ', '-'))
        after = ''.join(x[1:] for x in hunk if x[:1] in (' ', '+'))
        require(len(before.splitlines()) == int(header[1]) and len(after.splitlines()) == int(header[2]), 'Pinned hunk lengths drift')
        text = replace(text, before, after)
    return text

def recipe_patch(text, name):
    expected, filename, hunks = RECIPE_PATCHES[name]
    data = (PATCH_ROOT / (name + '.patch')).read_bytes()
    if sha(data) != expected:
        raise RuntimeError('Pinned recipe patch drift: ' + name)
    return apply_patch(text, data.decode('utf-8'), filename, hunks, name)

def threads(text, baseline_threads_only=False):
    text = '#include "runtime_lifecycle.hpp"\n'+text
    text = '#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT\n#include "persistent_state_runtime.hpp"\n#include "persistent_state_scheduler.hpp"\n#endif\n'+text
    text = replace(text,'    thread_context->running.wait();',
        '    thread_context->running.wait();\n'
        '#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT\n    tooie::persistent_state::scheduler::after_scheduler_wait();\n#endif\n'
        '    tooie::lifecycle::poll();')
    for call in ['    run_next_thread(PASS_RDRAM1);\n    wait_for_resumed(PASS_RDRAM cur_context);',
                 '    resume_thread(t);\n    wait_for_resumed(PASS_RDRAM cur_context);']:
        schedule,wait=call.split('\n')
        text=replace(text,call,schedule+'\n#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT\n    tooie::persistent_state::scheduler::before_scheduler_wait();\n#endif\n'+wait)
    for target in ['t->context','to_run->context','cur_context']:
        signal=f'    {target}->running.signal();'
        text=replace(text,signal,'#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT\n    tooie::persistent_state::scheduler::before_scheduler_signal('+target+');\n#endif\n'+signal)
    at='static void _thread_func(RDRAM_ARG PTR(OSThread) self_, PTR(thread_func_t) entrypoint, PTR(void) arg, UltraThreadContext* thread_context) {'
    custom='''static void continuous_thread_func(RDRAM_ARG PTR(OSThread) self_, PTR(thread_func_t) entrypoint, PTR(void) arg, UltraThreadContext* thread_context) {
    OSThread* self = TO_PTR(OSThread, self_);
    thread_self = self_;
    is_game_thread = true;
    // osCreateThread must finish assigning host_thread before the cleaner can
    // receive this context; initialized and running form the two-way handshake.
    bool initialization_notified = false;
    try {
#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT
        tooie::persistent_state::scheduler::worker_started(rdram,static_cast<uint32_t>(self_),entrypoint,self->sp,arg,thread_context);
#endif
        thread_context->initialized.signal();
        initialization_notified = true;
        wait_for_resumed(PASS_RDRAM thread_context);
        ultramodern::set_native_thread_name(ultramodern::threads::get_game_thread_name(self));
#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT
        tooie::persistent_state::runtime::run_worker(rdram,entrypoint,self->sp,arg);
#else
        run_thread_function(PASS_RDRAM entrypoint, self->sp, arg);
#endif
    } catch (const ultramodern::thread_terminated&) {
    } catch (...) {
        if (!initialization_notified) thread_context->initialized.signal();
        tooie::lifecycle::fail(std::current_exception());
    }
#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT
    tooie::persistent_state::runtime::detach_worker();
#endif
    // During global stop no worker may read or mutate guest queues: multiple
    // native workers wake only to unwind. A normal game destroy still uses the
    // ordinary single-running-worker scheduling contract.
    tooie::lifecycle::enqueue_completed(thread_context, [&] {
            if (self->context == thread_context) {
                self->context = nullptr;
                self->state = OSThreadState::STOPPED;
                run_next_thread(PASS_RDRAM1);
            }
    });
#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT
    // The retiring worker was excluded before its final scheduler callback.
    // Admission opens only after all of that callback's guest writes finish.
    tooie::persistent_state::scheduler::worker_retirement_complete();
#endif
}

'''+at+'''
    if (tooie::lifecycle::enabled()) {
        continuous_thread_func(PASS_RDRAM self_, entrypoint, arg, thread_context);
        return;
    }'''
    text=replace(text,at,custom)
    at='    context->host_thread = std::thread{_thread_func, PASS_RDRAM t_, entrypoint, arg, t->context};'
    text=replace(text,at,'''    if (tooie::lifecycle::enabled()) {
        try {
            tooie::lifecycle::created(context);
            context->host_thread = std::thread{_thread_func, PASS_RDRAM t_, entrypoint, arg, context};
        } catch (...) {
            tooie::lifecycle::creation_failed(context);
            t->context = nullptr;
            delete context;
            throw;
        }
    } else {
'''+at+'''
    }''')
    text=replace(text,'    context->initialized.wait();','    context->initialized.wait();\n    if (tooie::lifecycle::enabled()) tooie::lifecycle::launched(context);')
    at='extern "C" void osStopThread(RDRAM_ARG PTR(OSThread) t_) {'
    if not baseline_threads_only:
        text=replace(text,at,at+'''
    if (tooie::lifecycle::enabled()) {
        if (t_ == NULLPTR) t_ = thread_self;
        if (t_ == NULLPTR) throw std::runtime_error("osStopThread self requires a running guest worker");
        OSThread* t = TO_PTR(OSThread, t_);
        if (t_ == thread_self) {
            t->state = OSThreadState::STOPPED;
            ultramodern::run_next_thread_and_wait(PASS_RDRAM1);
            return;
        }
        if (t->state == OSThreadState::STOPPED) return;
        // Tooie's func_80016810 stops the joy worker in its message receive
        // queue. The runtime represents both runnable and waiting workers as
        // QUEUED; the actual queue link identifies which list to remove from.
        // Original libultra osStopThread dequeues RUNNABLE/WAITING and leaves
        // the native execution context suspended for a later osStartThread.
        if ((t->state != OSThreadState::QUEUED && t->state != OSThreadState::BLOCKED)
            || t->queue == NULLPTR || t->context == nullptr
            || !ultramodern::thread_queue_remove(PASS_RDRAM t->queue, t_))
            throw std::runtime_error("osStopThread target is not in a supported suspended queue");
        t->state = OSThreadState::STOPPED;
        return;
    }
''')
    # Two deletion sites: ordinary cleaner and approved post-join drain.
    # A worker already inside a scheduler primitive when stop was broadcast may
    # still hold a pointer to another context. Reclaim none of the unwinding
    # contexts until every worker has finished its last native scheduler access.
    text=replace(text,'to_delete->host_thread.join();',
        'if (tooie::lifecycle::enabled() && tooie::lifecycle::stopping()) {\n'
        '                while (!tooie::lifecycle::wait_for_producers(std::chrono::milliseconds{100})) {}\n'
        '            }\n            to_delete->host_thread.join();',2)
    text=replace(text,'delete to_delete;', 'tooie::lifecycle::deleted(to_delete);\n            delete to_delete;',2)
    text=replace(text,'    thread_cleaner_thread.join();','    if (thread_cleaner_thread.joinable()) thread_cleaner_thread.join();')
    return text

def messages(text):
    return recipe_patch(text, 'mesgqueue-lifecycle-wait')

BASELINE_TIMER_STOP = '''
void tooie::lifecycle::stop_and_join_timer() {
    if (timer_context.thread.joinable()) {
        timer_context.action_queue.enqueue(ContinuousTimerStop{});
        timer_context.thread.join();
    }
}
'''

def baseline_timer(text):
    # Keep the upstream epoch and arithmetic; the shared worker uses steady_clock.
    text = '#include "runtime_lifecycle.hpp"\n#include <type_traits>\n' + text
    text = replace(text, '// Start time for the program',
        'static_assert(std::is_same_v<std::chrono::high_resolution_clock, std::chrono::steady_clock>, "Baseline timer requires the pinned MSVC clock alias");\n// Start time for the program')
    text = replace(text, 'using Action = std::variant<AddTimerAction, RemoveTimerAction>;',
        'struct ContinuousTimerStop {};\nusing Action = std::variant<AddTimerAction, RemoveTimerAction, ContinuousTimerStop>;')
    text = replace(text, '    timer_context.thread.detach();', '')
    from owned_timer import ordinary
    return ordinary(text + BASELINE_TIMER_STOP)


def materialize_baseline_timer(runtime_root, output_root):
    source = runtime_root / 'ultramodern/src/timer.cpp'
    destinations = [output_root / 'timer.cpp', output_root / 'manifest.json']
    for destination in destinations:
        if destination.resolve().is_relative_to(runtime_root.resolve()):
            raise RuntimeError('Baseline timer output overlaps pinned runtime')
    revision = subprocess.check_output(['git', '-C', str(runtime_root), 'rev-parse', 'HEAD'], text=True).strip()
    original = source.read_bytes()
    if revision != PIN or sha(original) != '5919412bafcde237d911d1110059c39863b2a09baedba782d366d4804a32f72d':
        raise RuntimeError('Baseline timer requires exact pinned upstream input')
    data = baseline_timer(source.read_text()).encode()
    output_root.mkdir(parents=True, exist_ok=True)
    output = destinations[0]
    if not output.exists() or output.read_bytes() != data:
        output.write_bytes(data)
    manifest = dict(runtime_pin=PIN, scope='Baseline ordinary 1x copied timer; no product clock policy',
        materializer_sha256=sha(Path(__file__).read_bytes()), timer_recipe_sha256=sha(Path(__file__).with_name('owned_timer.py').read_bytes()),
        sources=[dict(original=str(source), original_sha256=sha(original), output=str(output), output_sha256=sha(data))])
    destinations[1].write_text(json.dumps(manifest, indent=2) + '\n')

def timer(text):
    text='#include "runtime_lifecycle.hpp"\n#include "virtual_clock.hpp"\n'+text
    text=replace(text,'using Action = std::variant<AddTimerAction, RemoveTimerAction>;',
        'struct ContinuousTimerStop {};\nstruct ClockRateChanged {};\nusing Action = std::variant<AddTimerAction, RemoveTimerAction, ContinuousTimerStop, ClockRateChanged>;')
    text=replace(text,'''// Game speed multiplier (1 means no speedup)
constexpr uint32_t speed_multiplier = 1;
// N64 CPU counter ticks per millisecond
constexpr uint32_t counter_per_ms = 46'875 * speed_multiplier;''',
        '''// Virtual elapsed time already includes the selected rate.
constexpr uint32_t counter_per_ms = 46'875;''')
    text=replace(text,'uint64_t duration_to_ticks(std::chrono::high_resolution_clock::duration duration) {',
        'uint64_t duration_to_ticks(std::chrono::nanoseconds duration) {')
    text=replace(text,'std::chrono::microseconds ticks_to_duration(uint64_t ticks) {',
        'uint64_t time_now();\n\nstd::chrono::microseconds ticks_to_duration(uint64_t ticks) {')
    text=replace(text,'    // Lambda comparator function to keep the set ordered',
        '    bool stop_requested=false;\n\n    // Lambda comparator function to keep the set ordered')
    text=replace(text,'        // Determine the action type and act on it',
        '        if (std::holds_alternative<ContinuousTimerStop>(action)) { stop_requested=true; return; }\n        if (std::holds_alternative<ClockRateChanged>(action)) return;\n        // Determine the action type and act on it')
    text=replace(text,'    while (true) {','    while (!stop_requested) {')
    text=replace(text,'        while (active_timers.empty()) {','        while (active_timers.empty() && !stop_requested) {')
    text=replace(text,"        // Get the timer that's closest to running out", "        if (stop_requested) return;\n\n        // Get the timer that's closest to running out")
    text=replace(text,'''std::chrono::high_resolution_clock::time_point ticks_to_timepoint(uint64_t ticks) {
    return start_time + ticks_to_duration(ticks);
}

uint64_t time_now() {
    return duration_to_ticks(std::chrono::high_resolution_clock::now() - start_time);
}''', '''std::chrono::steady_clock::time_point ticks_to_timepoint(uint64_t ticks) {
    if (tooie::lifecycle::enabled()) return tooie::timing::host_deadline_for_virtual(
        std::chrono::duration_cast<std::chrono::nanoseconds>(ticks_to_duration(ticks)));
    const auto now = time_now();
    return std::chrono::steady_clock::now() + (ticks > now ? ticks_to_duration(ticks - now) : std::chrono::steady_clock::duration::zero());
}

uint64_t time_now() {
    return tooie::lifecycle::enabled()
        ? duration_to_ticks(tooie::timing::virtual_elapsed())
        : duration_to_ticks(std::chrono::high_resolution_clock::now() - start_time);
}''')
    text=replace(text,'        auto wait_duration = ticks_to_timepoint(cur_timer->timestamp) - std::chrono::high_resolution_clock::now();',
        '        auto wait_duration = ticks_to_timepoint(cur_timer->timestamp) - std::chrono::steady_clock::now();')
    text=replace(text,'void ultramodern::init_timers(RDRAM_ARG1) {',
        '''void ultramodern::init_timers(RDRAM_ARG1) {
    if (tooie::lifecycle::enabled()) {
        tooie::timing::set_rate_change_notifier([]() noexcept {
            timer_context.action_queue.enqueue(ClockRateChanged{});
        });
    }''')
    text=replace(text,'uint32_t ultramodern::get_speed_multiplier() {\n    return speed_multiplier;\n}',
        'uint32_t ultramodern::get_speed_multiplier() {\n    return tooie::lifecycle::enabled() ? tooie::timing::rate() : 1U;\n}')
    text=replace(text,'std::chrono::high_resolution_clock::duration ultramodern::time_since_start() {\n    return std::chrono::high_resolution_clock::now() - start_time;\n}',
        '''std::chrono::high_resolution_clock::duration ultramodern::time_since_start() {
    return tooie::lifecycle::enabled()
        ? std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(tooie::timing::virtual_elapsed())
        : std::chrono::high_resolution_clock::now() - start_time;
}''')
    text=replace(text,'    timer_context.thread.detach();',
        '    if (!tooie::lifecycle::enabled()) timer_context.thread.detach();')
    text += BASELINE_TIMER_STOP
    from owned_timer import ordinary
    return ordinary(text)

def events(text):
    text='#include "runtime_lifecycle.hpp"\n#include "runtime_dp.hpp"\n#include "virtual_clock.hpp"\n#include "camera_interpolation.hpp"\n#include "model_interpolation.hpp"\n'+text
    # Keep early SP exactly where the pinned runtime sends it. Tooie can submit
    # the next RSP task while RDP is frozen for a framebuffer boundary. Admit
    # parsing only after unfreeze, and recheck after an already-admitted parser
    # returns (FREEZE may have been raised during parsing).
    text=replace(text,'                ultramodern::measure_input_latency();',
        '                if (!tooie::dp::admit(tooie::dp::Stage::parse,\n'
        '                    [] { return exited.load() || tooie::lifecycle::stopping(); }, [] {})) break;\n'
        '                ultramodern::measure_input_latency();')
    text=replace(text,'                dp_complete();',
        '                if (!tooie::dp::admit(tooie::dp::Stage::completion,\n'
        '                    [] { return exited.load() || tooie::lifecycle::stopping(); }, [] { dp_complete(); })) break;')
    text=replace(text,'void task_thread_func(uint8_t* rdram, moodycamel::LightweightSemaphore* thread_ready) {',
        'moodycamel::LightweightSemaphore continuous_events_producers_joined;\n\n'
        'void task_thread_func(uint8_t* rdram, moodycamel::LightweightSemaphore* thread_ready) {')
    # Keep the renderer alive outside the failure boundary until the controller
    # has joined guest/device producers and releases graphics shutdown.
    text=replace(text,'    auto renderer_context = ultramodern::renderer::create_render_context(rdram, window_handle, ultramodern::renderer::get_graphics_config().developer_mode);',
        '    renderer_context = ultramodern::renderer::create_render_context(rdram, window_handle, ultramodern::renderer::get_graphics_config().developer_mode);\n'
        '    if (tooie::lifecycle::enabled() && !renderer_context) throw std::runtime_error("Renderer construction returned null");')
    for signature in [
        'void task_thread_func(uint8_t* rdram, moodycamel::LightweightSemaphore* thread_ready) {',
        'void gfx_thread_func(uint8_t* rdram, moodycamel::LightweightSemaphore* thread_ready, ultramodern::renderer::WindowHandle window_handle) {']:
        prefix='    std::unique_ptr<ultramodern::renderer::RendererContext> renderer_context;\n' if signature.startswith('void gfx') else ''
        text=replace(text,signature,signature+'\n'+prefix+'''    bool ready_sent = false;
    auto notify_ready = [&] { if (!ready_sent) { ready_sent = true; thread_ready->signal(); } };
    try {''')
    # Exactly three original signals: RSP startup, renderer invalid, gfx ready.
    text=replace(text,'    thread_ready->signal();','    notify_ready();',3)
    text=replace(text,'            ULTRAMODERN_QUICK_EXIT();',
        '            if (tooie::lifecycle::enabled()) throw std::runtime_error("RSP task execution failed");\n            ULTRAMODERN_QUICK_EXIT();')
    text=replace(text,'        if (!ultramodern::rsp::run_task(PASS_RDRAM task)) {',
        '        if (tooie::lifecycle::enabled() && tooie::lifecycle::stopping()) return;\n\n'
        '        if (!ultramodern::rsp::run_task(PASS_RDRAM task)) {')
    text=replace(text,'''        sp_complete();
    }
}

std::atomic_uint32_t display_refresh_rate''','''        sp_complete();
    }
    } catch (...) {
        if (!tooie::lifecycle::enabled()) throw;
        tooie::lifecycle::fail(std::current_exception());
        notify_ready();
    }
}

std::atomic_uint32_t display_refresh_rate''')
    text=replace(text,'        renderer_setup_result.store(renderer_context->get_setup_result());',
        '        renderer_setup_result.store(renderer_context->get_setup_result());\n'
        '        if (tooie::lifecycle::enabled()) throw std::runtime_error("Renderer initialization failed");')
    text=replace(text,'''    graphics_shutdown_ready.wait();
    renderer_context->shutdown();''','''    } catch (...) {
        if (!tooie::lifecycle::enabled()) throw;
        if (!ready_sent && renderer_setup_result.load() == ultramodern::renderer::SetupResult::Success)
            renderer_setup_result.store(ultramodern::renderer::SetupResult::GraphicsDeviceNotFound);
        tooie::lifecycle::fail(std::current_exception());
        notify_ready();
    }
    graphics_shutdown_ready.wait();
    if (tooie::lifecycle::enabled()) continuous_events_producers_joined.wait();
    try {
        if (renderer_context) renderer_context->shutdown();
    } catch (...) {
        if (!tooie::lifecycle::enabled()) throw;
        tooie::lifecycle::fail(std::current_exception());
    }''')
    text=replace(text,'void vi_thread_func() {','void vi_thread_func() {\n    try {')
    # A rate change is represented by the virtual clock, which accrues the old
    # rate first. Thus this thread never reinterprets a fixed host epoch.
    text=replace(text,'    int remaining_retraces = 1;', '    int remaining_retraces = 1;')
    text=replace(text,'        auto next = ultramodern::get_start() + (total_vis * 1000000us) / (60 * ultramodern::get_speed_multiplier());', '')
    text=replace(text,'''        // Detect if there's more than a second to wait and wait a fixed amount instead for the next VI if so, as that usually means the system clock went back in time.
        if (std::chrono::floor<std::chrono::seconds>(next - std::chrono::high_resolution_clock::now()) > 1s) {
            // printf("Skipping the next VI wait\\n");
            next = std::chrono::high_resolution_clock::now();
        }
''', '')
    text=replace(text,'        ultramodern::sleep_until(next);', '''        if (tooie::lifecycle::enabled()) {
            while (!exited.load() && !tooie::lifecycle::stopping()) {
                const auto deadline = tooie::timing::host_deadline_for_virtual(
                    std::chrono::microseconds{(total_vis * 1000000ull) / 60ull});
                // Convert the unsigned counter's deadline to a signed time point
                // before subtracting, so a past deadline cannot wrap positive.
                const std::chrono::steady_clock::time_point signed_deadline = deadline;
                const auto remaining = signed_deadline - std::chrono::steady_clock::now();
                if (remaining <= decltype(remaining)::zero()) break;
                const auto slice = std::min(remaining,
                    std::chrono::duration_cast<std::chrono::steady_clock::duration>(2ms));
                // Relative native sleep cannot turn a backwards wall-clock
                // correction into a minute-long absolute wait.
                std::this_thread::sleep_for(slice);
            }
            if (exited.load() || tooie::lifecycle::stopping()) return;
        } else {
            auto next = ultramodern::get_start() +
                (total_vis * 1000000us) / (60 * ultramodern::get_speed_multiplier());
            if (std::chrono::floor<std::chrono::seconds>(next - std::chrono::high_resolution_clock::now()) > 1s)
                next = std::chrono::high_resolution_clock::now();
            ultramodern::sleep_until(next);
        }''')
    text=replace(text,'        auto time_now = ultramodern::time_since_start();', '''        auto time_now = tooie::lifecycle::enabled()
            ? std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(
                tooie::timing::virtual_elapsed())
            : ultramodern::time_since_start();''')
    text=replace(text,'        uint64_t new_total_vis = (time_now * (60 * ultramodern::get_speed_multiplier()) / 1000ms) + 1;',
        '        uint64_t new_total_vis = (time_now * 60 / 1000ms) + 1;')
    text=replace(text,'''        if (events_callbacks.vi_callback != nullptr) {
            events_callbacks.vi_callback();
        }
    }
}''','''        if (events_callbacks.vi_callback != nullptr) {
            events_callbacks.vi_callback();
        }
    }
    } catch (...) {
        if (!tooie::lifecycle::enabled()) throw;
        tooie::lifecycle::fail(std::current_exception());
    }
}''')
    text=replace(text,'''    events_context.sp.gfx_thread = std::thread{ gfx_thread_func, rdram, &gfx_thread_ready, window_handle };
    events_context.sp.task_thread = std::thread{ task_thread_func, rdram, &task_thread_ready };''','''    try {
        events_context.sp.gfx_thread = std::thread{ gfx_thread_func, rdram, &gfx_thread_ready, window_handle };
        events_context.sp.task_thread = std::thread{ task_thread_func, rdram, &task_thread_ready };
    } catch (...) {
        if (tooie::lifecycle::enabled()) {
            tooie::lifecycle::fail(std::current_exception());
            // Started workers hold these stack semaphore pointers until ready.
            // Do not unwind init_events before each worker has released them.
            if (events_context.sp.gfx_thread.joinable()) gfx_thread_ready.wait();
            if (events_context.sp.task_thread.joinable()) task_thread_ready.wait();
        }
        throw;
    }''')
    text=replace(text,'    task_thread_ready.wait();','    task_thread_ready.wait();\n'
        '    if (tooie::lifecycle::enabled()) {\n'
        '        if (auto failure = tooie::lifecycle::failure()) std::rethrow_exception(failure);\n'
        '    }')
    text=replace(text,'void ultramodern::submit_rsp_task(RDRAM_ARG PTR(OSTask) task_) {',
        'void ultramodern::submit_rsp_task(RDRAM_ARG PTR(OSTask) task_) {\n    tooie::lifecycle::reject_task(TO_PTR(OSTask, task_)->t.type);')
    text=replace(text,'''    // Send gfx tasks to the graphics action queue
    if (task->t.type == M_GFXTASK) {
        events_context.action_queue.enqueue(SpTaskAction{ *task });
    }
    // Set all other tasks as the RSP task''', '''    // Bind a camera-cut request to this exact copied display list. The
    // renderer consumes it only when the same task reaches send_dl.
    if (task->t.type == M_GFXTASK) {
        // Bind construction policy from the exact root, before queueing it.
        const auto model_binding = tooie::model_interpolation::bind_task_submission(
            static_cast<uint32_t>(task->t.data_ptr), static_cast<uint32_t>(task->t.data_size));
        if (model_binding.status == tooie::model_interpolation::SubmissionStatus::PublicationLost)
            throw tooie::model_interpolation::PublicationLoss{};
        (void)tooie_camera_interpolation_bind_task(rdram, static_cast<uint32_t>(task->t.data_ptr),
            static_cast<uint32_t>(task->t.data_size));
        bool published = false;
        try {
            published = events_context.action_queue.enqueue(SpTaskAction{ *task });
        } catch (...) {
            tooie::model_interpolation::publication_failed(model_binding);
            throw;
        }
        if (!published) {
            tooie::model_interpolation::publication_failed(model_binding);
            throw tooie::model_interpolation::PublicationLoss{};
        }
    }
    // Set all other tasks as the RSP task''')
    # Immediate startup can beat VI's first iteration. Seed the same baseline
    # startup mode before starting VI, rather than relying on frontend delay.
    text=replace(text,'    events_context.vi.thread = std::thread{ vi_thread_func };',
        '    if (tooie::lifecycle::enabled()) set_dummy_vi(false);\n    events_context.vi.thread = std::thread{ vi_thread_func };')
    # Guest osViSetMode/SwapBuffer/SetEvent already take message_mutex. The VI
    # consumer must use that same lock while swapping and copying their state.
    text=replace(text,"        // If the game hasn't started yet, set a dummy VI mode and origin.",
        '        {\n        std::unique_lock<std::mutex> continuous_vi_lock(events_context.message_mutex, std::defer_lock);\n'
        '        if (tooie::lifecycle::enabled()) continuous_vi_lock.lock();\n'
        "        // If the game hasn't started yet, set a dummy VI mode and origin.")
    text=replace(text,'        events_context.vi.update_vi();',
        '        events_context.vi.update_vi();\n        }')
    # Both scalar framebuffer readers share the VI writer's state mutex.
    for getter in ['Next', 'Current']:
        anchor = f'extern "C" PTR(void) osViGet{getter}Framebuffer() {{\n'
        text = replace(text, anchor,
            anchor + '    std::lock_guard lock{ events_context.message_mutex };\n')
    # Partial preinit failures must still allow all started event threads to join.
    for name in ['events_context.sp.gfx_thread','events_context.vi.thread','events_context.sp.task_thread']:
        text=replace(text,f'    {name}.join();',f'    if ({name}.joinable()) {name}.join();')
    text=replace(text,'void ultramodern::join_event_threads() {','''void ultramodern::join_event_threads() {
    if (tooie::lifecycle::enabled()) {
        if (events_context.vi.thread.joinable()) events_context.vi.thread.join();
        events_context.sp_task_queue.enqueue(nullptr);
        if (events_context.sp.task_thread.joinable()) events_context.sp.task_thread.join();
        // Renderer resources outlive both device producers, even when the
        // controller has already released graphics_shutdown_ready.
        continuous_events_producers_joined.signal();
        if (events_context.sp.gfx_thread.joinable()) events_context.sp.gfx_thread.join();
        return;
    }''')
    return text

def dp(text):
    text='#include "runtime_dp.hpp"\n'+text
    text=replace(text,'uint32_t rdp_state = 1 << (int)RDPStatusBit::BufferReady;',
        '// Status is shared with the opt-in graphics admission gate.\n'
        'auto& rdp_state = tooie::dp::status;')
    text=replace(text,'    ctx->r2 = rdp_state;',
        '    ctx->r2 = rdp_state.load(std::memory_order_acquire);')
    text=replace(text,'''    update_bit(rdp_state, ctx->r4, RDPStatusBit::XbusDmem);
    update_bit(rdp_state, ctx->r4, RDPStatusBit::Freeze);
    update_bit(rdp_state, ctx->r4, RDPStatusBit::Flush);''','''    std::lock_guard lock(tooie::dp::admission_mutex);
    const uint32_t before = rdp_state.load(std::memory_order_relaxed);
    uint32_t after = before;
    update_bit(after, ctx->r4, RDPStatusBit::XbusDmem);
    update_bit(after, ctx->r4, RDPStatusBit::Freeze);
    update_bit(after, ctx->r4, RDPStatusBit::Flush);
    rdp_state.store(after, std::memory_order_release);
    if ((before & 2u) && !(after & 2u)) tooie::dp::changed.notify_all();
    if (tooie::lifecycle::enabled() && ((before ^ after) & 2u)) {
        if (after & 2u) ++tooie::dp::freeze_sets;
        else ++tooie::dp::freeze_clears;
    }''')
    return text

def translation(text):
    # The pinned wrapper asserts and becomes an empty return under NDEBUG. In
    # continuous mode, yield the current guest thread through the existing
    # priority-ready queue instead. Keep the disabled runtime's exact contract.
    text = '#include "runtime_lifecycle.hpp"\n#include <stdexcept>\n' + text
    text = '#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT\n#include "persistent_state_hle.hpp"\n#endif\n' + text
    for symbol, adapter in [
        ('osRecvMesg_recomp', 'recv'), ('osSendMesg_recomp', 'send'), ('osJamMesg_recomp', 'jam'),
        ('osStartThread_recomp', 'start_thread'), ('osStopThread_recomp', 'stop_thread'),
        ('osSetThreadPri_recomp', 'set_priority'),
    ]:
        signature = 'extern "C" void ' + symbol + '('
        if text.count(signature) != 1:
            raise RuntimeError('Persistent HLE wrapper signature changed: ' + symbol)
        at = text.index('{\n', text.index(signature)) + 2
        hook = ('#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT\n'
                '    if (tooie::continuation::Machine::current_if_bound()) {\n'
                f'        tooie_persistent_{adapter}(rdram, ctx); return;\n'
                '    }\n#endif\n')
        text = text[:at] + hook + text[at:]
    return replace(text, '''extern "C" void osYieldThread_recomp(uint8_t * rdram, recomp_context * ctx) {
    assert(false);''', '''extern "C" void osYieldThread_recomp(uint8_t * rdram, recomp_context * ctx) {
    if (tooie::lifecycle::enabled()) {
        const auto self = ultramodern::this_thread();
        if (!ultramodern::thread_queue_empty(PASS_RDRAM ultramodern::running_queue)) {
            const auto candidate = ultramodern::thread_queue_peek(PASS_RDRAM ultramodern::running_queue);
            // The selected queue inserts after existing equal-priority peers.
            // Pop the ready head before requeueing self: this preserves the
            // original enqueue-self/dispatch FIFO order. A lower-priority
            // candidate cannot preempt this caller.
            if (TO_PTR(OSThread, candidate)->priority >= TO_PTR(OSThread, self)->priority) {
                const auto next = ultramodern::thread_queue_pop(PASS_RDRAM ultramodern::running_queue);
                ultramodern::schedule_running_thread(PASS_RDRAM self);
                ultramodern::resume_thread_and_wait(PASS_RDRAM TO_PTR(OSThread, next));
            }
        }
        tooie::lifecycle::poll();
        return;
    }
    assert(false);''')

def saving(text):
    # Save root, temporary-file promotion, generation tracking and final drain.
    return recipe_patch(text, 'pi-save-persistence')

def standard_start(text):
    # Sole product child of the exact pinned source. Baseline never selects it.
    if sha(text.encode()) != 'af9c6f983fb1620ff5b971836d7792ef0b83a47e77010ab89ddf2fa1fb87e143':
        raise RuntimeError('Product standard-start source pin mismatch')
    text = '#include "continuous_host.hpp"\n#include "runtime_lifecycle.hpp"\n#include "platform_support.hpp"\n#include <atomic>\n#include <exception>\n' + text
    text = replace(text, '    recomp::mods::initialize_mods();\n    recomp::mods::scan_mods();\n', '')
    allocation_begin = '    // Allocate rdram without comitting it.'
    allocation_end = '    recomp::register_heap_exports();'
    if text.count(allocation_begin) != 1 or text.count(allocation_end) != 1:
        raise RuntimeError('Product RDRAM allocation anchors changed')
    allocation = text[text.index(allocation_begin):text.index(allocation_end)]
    text = replace(text, allocation, '''    uint8_t* rdram;
    try {
        rdram = tooie::platform::reserve_rdram(allocation_size, mem_size);
    } catch (...) {
        tooie::lifecycle::fail(std::current_exception());
        ultramodern::error_handling::message_box("Failed to allocate memory!");
        return;
    }

''')
    text = replace(text, '''    std::thread game_thread{[](ultramodern::renderer::WindowHandle window_handle, uint8_t* rdram) {
        debug_printf("[Recomp] Starting\\n");

        ultramodern::set_native_thread_name("Game Start Thread");

        ultramodern::preinit(rdram, window_handle);

        recomp_context context{};

        // Loop until the game starts.
        while (!wait_for_game_started(rdram, &context)) {}
    }, window_handle, rdram};

''',
        '''    std::atomic_bool startup_done{false};
    std::thread game_thread{[&startup_done](ultramodern::renderer::WindowHandle window_handle, uint8_t* rdram) {
        try {
            debug_printf("[Recomp] Starting\\n");
            ultramodern::set_native_thread_name("Game Start Thread");
            ultramodern::preinit(rdram, window_handle);
            recomp_context context{};
            while (!tooie::lifecycle::stopping() && !wait_for_game_started(rdram, &context)) {}
        } catch (const ultramodern::thread_terminated&) {
        } catch (...) {
            tooie::lifecycle::fail(std::current_exception());
        }
        startup_done.store(true, std::memory_order_release);
    }, window_handle, rdram};

''')
    text = replace(text, '''    while (!exited) {
        ultramodern::sleep_milliseconds(1);
        if (gfx_callbacks.update_gfx != nullptr) {
            gfx_callbacks.update_gfx(gfx_data);
        }
    }

''',
        '''    try {
        while (!exited && !tooie::lifecycle::stopping()) {
            ultramodern::sleep_milliseconds(1);
            if (gfx_callbacks.update_gfx != nullptr) {
                gfx_callbacks.update_gfx(gfx_data);
            }
        }
    } catch (...) {
        tooie::lifecycle::fail(std::current_exception());
    }

''')
    return replace(text, '''    graphics_shutdown_ready.signal();

    game_thread.join();
    ultramodern::join_event_threads();
    ultramodern::join_thread_cleaner_thread();
    ultramodern::join_saving_thread();
\x20\x20\x20\x20
    // Free rdram.
    bool free_failed;
#ifdef _WIN32
    // VirtualFree returns zero on failure.
    free_failed = (VirtualFree(rdram, 0, MEM_RELEASE) == 0);
#else
    // munmap returns -1 on failure.
    free_failed = (munmap(rdram, allocation_size) == -1);
#endif

    if (free_failed) {
        printf("Failed to free rdram\\n");
    }''',
        '    tooie::finish_standard_runtime(game_thread, startup_done);\n    // Retain RDRAM to process exit after the shared product terminal owner.\n')

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--runtime',required=True,type=Path)
    p.add_argument('--cleanup-threads',type=Path)
    p.add_argument('--output-directory',required=True,type=Path)
    baseline=p.add_mutually_exclusive_group()
    baseline.add_argument('--baseline-timer',action='store_true')
    baseline.add_argument('--baseline-threads-only',action='store_true')
    baseline.add_argument('--baseline-external-wait',action='store_true')
    baseline.add_argument('--baseline-selected-runtime',action='store_true')
    a=p.parse_args()
    if a.baseline_selected_runtime:
        import sys
        sys.dont_write_bytecode = True
        from prepare_runtime_baseline_selected import materialize
        materialize(Path(__file__).resolve().parents[1], a.runtime, a.output_directory)
        return
    if a.baseline_timer:
        materialize_baseline_timer(a.runtime, a.output_directory)
        return
    if a.cleanup_threads is None:
        p.error('--cleanup-threads is required outside --baseline-timer')
    baseline_mode=a.baseline_threads_only or a.baseline_external_wait
    runtime_root=a.runtime.resolve()
    output_root=a.output_directory.resolve()
    protected_input=a.cleanup_threads.resolve()
    names=['threads'] if baseline_mode else ['threads','mesgqueue','timer','events','pi','ultra_translation','dp','recomp']
    if a.baseline_external_wait: names.append('mesgqueue')
    cleanup_patch=Path(__file__).resolve().parents[1]/'patches/runtime/producer-quiescent-cleanup.patch'
    # Validate every destination before making any change, including existing
    # output-file symlinks. Preserve immutable refs and the approved input layer.
    destinations=[output_root/'manifest.json']
    for name in names:
        destinations.extend([output_root/f'{name}.cpp',output_root/f'{name}.patch'])
    for destination in destinations:
        resolved=destination.resolve()
        if resolved==runtime_root or runtime_root in resolved.parents or resolved==protected_input:
            raise RuntimeError('Continuous output overlaps protected input/reference: '+str(resolved))
        if baseline_mode and resolved==cleanup_patch.resolve():
            raise RuntimeError('Baseline output overlaps pinned cleanup patch: '+str(resolved))
    revision=subprocess.check_output(['git','-C',str(a.runtime),'rev-parse','HEAD'],text=True).strip()
    if revision!=PIN: raise RuntimeError('Continuous runtime pin mismatch: '+revision)
    expected_input=BASELINE_THREADS_SHA if baseline_mode else CLEANUP_SHA
    if sha(a.cleanup_threads.read_bytes())!=expected_input: raise RuntimeError('Continuous overlay requires exact approved cleanup source')
    threads_input=a.cleanup_threads.read_text()
    if baseline_mode:
        if sha(cleanup_patch.read_bytes())!=CLEANUP_PATCH_SHA:
            raise RuntimeError('Baseline cleanup patch pin mismatch')
        patch_lines=cleanup_patch.read_text().splitlines(True)
        hunks=[i for i,line in enumerate(patch_lines) if line.startswith('@@ ')]
        if len(hunks)!=1: raise RuntimeError('Baseline cleanup requires one exact hunk')
        hunk=patch_lines[hunks[0]+1:]
        if sum(line.startswith(' ') for line in hunk)!=4 or any(line[0] not in ' +-' for line in hunk):
            raise RuntimeError('Baseline cleanup requires its unique four-line drain context')
        before=''.join(line[1:] for line in hunk if line[0] in ' -')
        after=''.join(line[1:] for line in hunk if line[0] in ' +')
        threads_input=replace(threads_input,before,after)
        if sha(threads_input.encode())!=BASELINE_COMPOSED_SHA:
            raise RuntimeError('Baseline composed cleanup source pin mismatch')
    outputs=[]
    for name,transform in [('threads',threads),('mesgqueue',messages),('timer',timer),('events',events),('pi',saving),('ultra_translation',translation),('dp',dp),('recomp',standard_start)]:
        if name not in names: continue
        component='librecomp' if name in {'pi','ultra_translation','dp','recomp'} else 'ultramodern'
        source=a.runtime/component/'src'/f'{name}.cpp'
        original=source.read_bytes()
        # Compare worktree bytes to immutable Git object, normalizing only CRLF.
        pinned=subprocess.check_output(['git','-C',str(a.runtime),'show',f'{PIN}:{component}/src/{name}.cpp'])
        if original.replace(b'\r\n',b'\n')!=pinned.replace(b'\r\n',b'\n'):
            raise RuntimeError('Reference worktree drift: '+str(source))
        if a.baseline_external_wait and name=='mesgqueue' and sha(original)!= '720db0e01ed2d874710b180292bd3ee9480c742a99bc574c4ce87e8f42949d21':
            raise RuntimeError('Baseline external message source pin mismatch')
        input_path=a.cleanup_threads if name=='threads' else source
        input_text=threads_input if name=='threads' else input_path.read_text()
        data=(threads(input_text,True) if baseline_mode and name=='threads' else transform(input_text)).encode()
        output=a.output_directory/f'{name}.cpp'
        output.parent.mkdir(parents=True,exist_ok=True)
        if not output.exists() or output.read_bytes()!=data: output.write_bytes(data)
        patch=''.join(difflib.unified_diff(input_path.read_text().splitlines(True),data.decode().splitlines(True),fromfile=str(input_path),tofile=str(output))).encode()
        patch_path=output.with_suffix('.patch')
        if not patch_path.exists() or patch_path.read_bytes()!=patch: patch_path.write_bytes(patch)
        outputs.append(dict(original=str(source),original_sha256=sha(original),input=str(input_path),input_sha256=sha(input_path.read_bytes()),output=str(output),output_sha256=sha(data),patch=str(patch_path),patch_sha256=sha(patch)))
        if baseline_mode and name=='threads':
            outputs[-1].update(cleanup_patch=str(cleanup_patch),cleanup_patch_sha256=CLEANUP_PATCH_SHA,composed_input_sha256=BASELINE_COMPOSED_SHA)
    (a.output_directory/'manifest.json').write_text(json.dumps(dict(runtime_pin=PIN,scope='Opt-in continuous lifecycle; bounded diagnostics retain original paths',materializer_sha256=sha(Path(__file__).read_bytes()),timer_recipe_sha256=sha(Path(__file__).with_name('owned_timer.py').read_bytes()) if not baseline_mode else None,sources=outputs),indent=2)+'\n')

if __name__=='__main__': main()
