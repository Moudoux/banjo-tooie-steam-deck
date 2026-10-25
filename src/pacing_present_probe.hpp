#pragma once

#include <cstdint>

namespace tooie::pacing {

struct PresentSnapshot {
    std::uint64_t successful_presents = 0;
    std::uint64_t interval_samples = 0;
    double mean_interval_ms = 0.0;
    double max_interval_ms = 0.0;
    std::uint64_t intervals_over_50ms = 0;
    // Lifetime session values; the rolling fields above remain UI-oriented.
    double session_max_interval_ms = 0.0;
    std::uint64_t session_intervals_over_50ms = 0;
    double session_last_long_interval_ms = 0.0;
    std::uint64_t session_last_long_elapsed_ms = 0;
    std::uint32_t source_hz = 0;
    std::uint32_t target_hz = 0;
    bool vsync_actual = true;
};

void reset_present_probe() noexcept;
void request_vsync(bool enabled) noexcept;
PresentSnapshot present_snapshot() noexcept;

} // namespace tooie::pacing

extern "C" bool tooie_rt64_vsync_requested() noexcept;
extern "C" void tooie_rt64_present_observed(bool successful, bool vsync_actual,
    std::uint32_t source_hz, std::uint32_t target_hz) noexcept;

// Opt-in bounded diagnostics at the actual swapchain submission boundary.
// Framebuffer identity matters: a high presentation rate alone cannot show
// whether Console is displaying the workload that generated the extra frames.
extern "C" void tooie_rt64_present_trace(std::uint64_t present_id,
    std::uint64_t workload_id, std::uint32_t mode, std::uint32_t vi_origin,
    std::uint32_t vi_framebuffer, std::uint32_t color_framebuffer,
    std::uint32_t vi_x, std::uint32_t vi_y, std::uint32_t vi_h,
    std::uint32_t vi_v, std::uint32_t index, std::uint32_t count,
    std::uint32_t source_hz, std::uint32_t target_hz, bool successful,
    bool vsync_actual) noexcept;

namespace tooie::pacing {
// Present-owner stack values, in steady-clock nanoseconds. Mask bits mark
// executed measurements: interp1/acquire2/worker4/sleep8/presentWait16/call32/phase64.
struct PresentStageTimes {
    std::uint64_t interp_ns = 0, acquire_ns = 0, worker_ns = 0;
    std::uint64_t sleep_ns = 0, present_wait_ns = 0, present_ns = 0, phase_ns = 0;
    std::uint32_t measured = 0;
};
}
extern "C" bool tooie_rt64_present_trace_enabled() noexcept;
extern "C" std::uint64_t tooie_rt64_present_trace_clock_ns() noexcept;
extern "C" void tooie_rt64_present_trace_staged(std::uint64_t present_id,
    std::uint64_t workload_id, std::uint32_t mode, std::uint32_t vi_origin,
    std::uint32_t vi_framebuffer, std::uint32_t color_framebuffer,
    std::uint32_t vi_x, std::uint32_t vi_y, std::uint32_t vi_h,
    std::uint32_t vi_v, std::uint32_t index, std::uint32_t count,
    std::uint32_t source_hz, std::uint32_t target_hz, bool successful,
    bool vsync_actual, const tooie::pacing::PresentStageTimes* stages) noexcept;
