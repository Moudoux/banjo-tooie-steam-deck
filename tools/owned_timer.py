"""Exact source recipes for copied requests in the composed timer runtime."""

def once(text, old, new):
    if text.count(old) != 1:
        raise ValueError(f"owned timer anchor changed: {old[:80]!r}")
    return text.replace(old, new, 1)

def ordinary(text):
    text = once(text, '#include <set>', '#include <vector>\n#include <mutex>\n#include <optional>\n#include <algorithm>\n#include <stdexcept>\n#include <cstdio>\n#include <cstdlib>')
    text = once(text, '#include \"blockingconcurrentqueue.h\"', '#include \"blockingconcurrentqueue.h\"\n#ifndef TOOIE_TIMER_QUEUE_TRAITS\n#define TOOIE_TIMER_QUEUE_TRAITS moodycamel::ConcurrentQueueDefaultTraits\n#endif')
    text = once(text, 'struct AddTimerAction {\n    PTR(OSTimer) timer;\n};', '''struct OwnedTimer {
    PTR(OSTimer) address;
    OSTime deadline;
    OSTime interval;
    PTR(OSMesgQueue) queue;
    OSMesg message;
};
struct AddTimerAction { OwnedTimer value; };''')
    text = once(text, '''    moodycamel::BlockingConcurrentQueue<Action> action_queue{};
} timer_context;''', '''    // Fixture builds may supply an allocator trait; production uses pinned defaults.
    moodycamel::BlockingConcurrentQueue<Action, TOOIE_TIMER_QUEUE_TRAITS> action_queue{};
    // One mutation stream across host threads, inside the queue lifetime.
    moodycamel::ProducerToken mutation_producer{action_queue};
    std::mutex request_mutex;
    std::vector<OwnedTimer> active;
    uint8_t* rdram = nullptr;
} timer_context;

static bool timer_device_checkpoint() { return true; }
static bool timer_device_enabled() { return false; }
static void publish_timer_action(const Action& action) {
    if (!timer_context.action_queue.enqueue(timer_context.mutation_producer, action))
        throw std::runtime_error("Timer mutation publication failed");
}''')
    start = text.index('void timer_thread(RDRAM_ARG1) {')
    end = text.index('\nvoid ultramodern::init_timers', start)
    text = text[:start] + WORKER + text[end:]
    start = text.index('extern "C" int osSetTimer(')
    end = text.index('\n#ifdef _WIN32', start)
    text = text[:start] + SETTERS + text[end:]
    text = once(text, '''        timer_context.action_queue.enqueue(ContinuousTimerStop{});
        timer_context.thread.join();''', '''        try {
            std::lock_guard lock{timer_context.request_mutex};
            publish_timer_action(ContinuousTimerStop{});
        } catch (...) {
            // New fatal-retention branch: no join, unwind or resource cleanup.
            std::fputs("Terminal timer publication failed; retaining runtime resources\\n", stderr);
            std::_Exit(70);
        }
        timer_context.thread.join();''')
    text = once(text, 'extern "C" OSTime osGetTime() {',
        'extern "C" OSTime osGetTime() {\n    std::lock_guard lock{timer_context.request_mutex};')
    text = once(text, 'extern "C" void osSetTime(OSTime t) {',
        'extern "C" void osSetTime(OSTime t) {\n    std::lock_guard lock{timer_context.request_mutex};')
    return text

WORKER = '''void timer_thread(RDRAM_ARG1) {
    ultramodern::set_native_thread_name("Timer Thread");
    ultramodern::set_native_thread_priority(ultramodern::ThreadPriority::VeryHigh);
    timer_context.rdram = rdram;
    bool stop_requested = false;
    std::optional<Action> returned_action;
    auto process = [&](const Action& action) {
        if (std::holds_alternative<ContinuousTimerStop>(action)) {
            stop_requested = true;
        } else if (const auto* add = std::get_if<AddTimerAction>(&action)) {
            auto it = std::find_if(timer_context.active.begin(), timer_context.active.end(),
                [&](const OwnedTimer& value) { return value.address == add->value.address; });
            if (it == timer_context.active.end()) timer_context.active.push_back(add->value);
            else *it = add->value;
        } else if (const auto* remove = std::get_if<RemoveTimerAction>(&action)) {
            std::erase_if(timer_context.active,
                [&](const OwnedTimer& value) { return value.address == remove->timer; });
        }
        // ClockRateChanged only wakes; selection reads the current clock.
    };
    auto drain = [&]() {
        if (returned_action) {
            process(*returned_action);
            returned_action.reset();
        }
        Action action;
        while (timer_context.action_queue.try_dequeue(action)) process(action);
    };
    try {
        while (true) {
            {
                std::lock_guard lock{timer_context.request_mutex};
                drain();
                if (stop_requested) return;
            }
            // No lock, timeout, selected pointer or unprocessed action crosses a park.
            if (!timer_device_checkpoint()) return;
            std::optional<std::chrono::steady_clock::duration> wait_duration;
            {
                std::lock_guard lock{timer_context.request_mutex};
                drain();
                if (stop_requested) return;
                if (!timer_context.active.empty()) {
                    auto selected = std::min_element(timer_context.active.begin(), timer_context.active.end(),
                        [](const OwnedTimer& a, const OwnedTimer& b) {
                            return a.deadline == b.deadline ? a.address < b.address : a.deadline < b.deadline;
                        });
                    // Sequence the two samples: an expired virtual deadline maps
                    // to now, so sampling the subtraction operand first can make
                    // an already due timer appear to have a positive wait.
                    const auto wait_deadline = ticks_to_timepoint(selected->deadline);
                    wait_duration = wait_deadline - std::chrono::steady_clock::now();
                    if (*wait_duration <= std::chrono::steady_clock::duration::zero()) {
                        // Pinned void API hides enqueue false; retain that inherited limit.
                        // Thrown allocation remains visible to the failure owner below.
                        ultramodern::enqueue_external_message(selected->queue, selected->message, false, true);
                        if (selected->interval != 0) {
                            selected->deadline = selected->interval + time_now();
                            TO_PTR(OSTimer, selected->address)->timestamp = selected->deadline;
                        } else timer_context.active.erase(selected);
                        continue;
                    }
                }
            }
            Action action;
            bool received;
            if (timer_device_enabled()) {
                const auto slice = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::milliseconds{2});
                const auto duration = wait_duration ? std::min(*wait_duration, slice) : slice;
                received = timer_context.action_queue.wait_dequeue_timed(action, duration);
            } else if (wait_duration) {
                received = timer_context.action_queue.wait_dequeue_timed(action, *wait_duration);
            } else {
                timer_context.action_queue.wait_dequeue(action);
                received = true;
            }
            if (received) returned_action = std::move(action);
        }
    } catch (...) {
        // Request locks have unwound before reporting or stop signals.
        if (!tooie::lifecycle::enabled()) throw;
        tooie::lifecycle::fail(std::current_exception());
    }
}
'''

SETTERS = '''extern "C" int osSetTimer(RDRAM_ARG PTR(OSTimer) t_, OSTime countdown, OSTime interval, PTR(OSMesgQueue) mq, OSMesg msg) {
    std::lock_guard lock{timer_context.request_mutex};
    const OwnedTimer value{t_, (countdown == 0 ? interval : countdown) + time_now(), interval, mq, msg};
    // Publication precedes guest mirror stores; the worker shares this mutex.
    publish_timer_action(AddTimerAction{value});
    OSTimer* t = TO_PTR(OSTimer, t_);
    t->timestamp = value.deadline;
    t->interval = interval;
    t->mq = mq;
    t->msg = msg;
    return 0;
}

extern "C" int osStopTimer(RDRAM_ARG PTR(OSTimer) t_) {
    std::lock_guard lock{timer_context.request_mutex};
    publish_timer_action(RemoveTimerAction{t_});
    // Asynchronous Stop cannot retract a message already in the external FIFO.
    return 0;
}
'''

def persistent(text):
    if 'tooie_persistent_timer_export' in text:
        raise ValueError('owned timer device hooks already present')
    text = '#include "persistent_state_devices.hpp"\n' + text
    text = once(text, 'static bool timer_device_checkpoint() { return true; }', '''static bool timer_device_checkpoint() {
    if (tooie::persistent_state::devices::terminal_abort_requested()) return false;
    return tooie::persistent_state::devices::worker_checkpoint(
        tooie::persistent_state::devices::Worker::Timer);
}''')
    text = once(text, 'static bool timer_device_enabled() { return false; }', '''static bool timer_device_enabled() {
    return tooie::persistent_state::devices::experiment_enabled();
}''')
    return text + DEVICE_HOOKS

DEVICE_HOOKS = '''
extern "C" bool tooie_persistent_timer_export(
    tooie::persistent_state::devices::Snapshot* out) noexcept {
    using namespace tooie::persistent_state::devices;
    if (!experiment_enabled() || !out || !timer_context.rdram) return false;
    try {
        std::lock_guard lock{timer_context.request_mutex};
        if (timer_context.action_queue.size_approx() != 0) return false;
        const auto now = time_now();
        std::vector<Timer> values;
        values.reserve(timer_context.active.size());
        for (const auto& value : timer_context.active)
            values.push_back(Timer{value.address,
                value.deadline > now ? value.deadline - now : 0,
                value.interval, {value.queue, value.message}});
        out->timers.swap(values);
        out->virtual_ticks = now;
        out->os_time_offset = ostime_offset;
        return true;
    } catch (...) { return false; }
}

extern "C" bool tooie_persistent_timer_import(
    const tooie::persistent_state::devices::Snapshot* in) noexcept {
    using namespace tooie::persistent_state::devices;
    if (!experiment_enabled() || !in || !timer_context.rdram || !valid_snapshot(*in)) return false;
    try {
        std::lock_guard lock{timer_context.request_mutex};
        if (timer_context.action_queue.size_approx() != 0) return false;
        const auto now = time_now();
        std::vector<OwnedTimer> values;
        values.reserve(in->timers.size());
        for (const auto& item : in->timers) {
            const auto physical = static_cast<std::uint32_t>(item.guest_address) & 0x7FFFFFFFU;
            if (physical > 0x800000U - sizeof(OSTimer) || physical % alignof(OSTimer)) return false;
            if (std::any_of(values.begin(), values.end(), [&](const OwnedTimer& value) {
                    return value.address == item.guest_address; })) return false;
            values.push_back({item.guest_address, rebase_timer_deadline(now, item.remaining_ticks),
                item.interval_ticks, item.route.queue, item.route.message});
        }
        auto* rdram = timer_context.rdram;
        // Validation/allocation precedes owned replacement and all mirror writes.
        timer_context.active.swap(values);
        for (const auto& value : timer_context.active) {
            auto* target = TO_PTR(OSTimer, value.address);
            target->timestamp = value.deadline;
            target->interval = value.interval;
            target->mq = value.queue;
            target->msg = value.message;
        }
        ostime_offset = in->os_time_offset;
        return true;
    } catch (...) { return false; }
}
'''
