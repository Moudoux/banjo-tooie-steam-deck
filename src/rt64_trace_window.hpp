#pragma once

#include <cstdint>

namespace tooie::rt64_trace {

// Each nonzero, newer request gets its own bounded capture file. A new press
// ends the preceding window without reusing or truncating its file name.
struct Window {
    static constexpr uint32_t duration = 420;
    uint32_t seen = 0;
    uint32_t calls = duration;
    uint32_t generation = 0;

    uint32_t request(uint32_t generation) noexcept {
        if (generation == 0 || generation <= seen) return 0;
        seen = generation;
        this->generation = generation;
        calls = 0;
        return generation;
    }
};

// L05 F4 run: one bounded run per session. The first armed presentation starts it; arms while it runs or after it
// completes are ignored, so repeated presses never restart, extend or queue another run.
struct OneShot {
    static constexpr uint32_t duration = 3300;
    static constexpr uint32_t idle = UINT32_MAX;
    uint32_t calls = 0;
    bool started = false;
    // This presentation's index within the run, or idle outside it.
    uint32_t next(bool armed) noexcept {
        if (!started) {
            if (!armed) return idle;
            started = true;
        }
        return calls < duration ? calls++ : idle;
    }
    bool completed() const noexcept { return started && calls == duration; }
};

} // namespace tooie::rt64_trace
