#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>

// Existing runtime exit flag; the selected host notifies after every quit call.
extern std::atomic_bool exited;

namespace ultramodern::detail::dp_freeze {
inline uint32_t status = 0x80;
inline std::mutex admission_mutex;
inline std::condition_variable changed;

inline uint32_t get_status() {
    std::lock_guard lock(admission_mutex);
    return status;
}

template<class Action> bool wait_and_run(Action&& action) {
    std::unique_lock lock(admission_mutex);
    changed.wait(lock, [] { return ::exited.load() || !(status & 2u); });
    if (::exited.load()) return false;
    // Quit may publish exited concurrently after this admission.
    // The owner joins this worker before releasing its borrowed RDRAM.
    action();
    return true;
}

inline void notify_stop() {
    {
        std::lock_guard lock(admission_mutex);
    }
    changed.notify_all();
}
}
