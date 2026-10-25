#include "pacing_present_probe.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace {
constexpr std::size_t interval_capacity = 120;
std::array<std::atomic<std::uint64_t>, interval_capacity> intervals_ns{};
std::atomic<std::uint64_t> successful_presents{0};
std::atomic<std::uint64_t> last_timestamp_ns{0};
std::atomic<std::uint32_t> latest_source_hz{0};
std::atomic<std::uint32_t> latest_target_hz{0};
std::atomic<bool> requested_vsync{true};
std::atomic<bool> latest_vsync_actual{true};
std::atomic<std::uint64_t> probe_started_ns{0};
std::atomic<std::uint64_t> session_max_interval_ns{0};
std::atomic<std::uint64_t> session_intervals_over_50ms{0};
std::atomic<std::uint64_t> session_last_long_interval_ns{0};
std::atomic<std::uint64_t> session_last_long_elapsed_ms{0};

// Strict diagnostic-only bounds; malformed values retain the legacy fallback.
std::uint32_t trace_milliseconds(const char* name, std::uint32_t maximum) noexcept {
    const char* value = std::getenv(name);
    if (!value || !*value) return 0;
    std::uint32_t parsed = 0;
    for (const char* digit = value; *digit; ++digit) {
        if (*digit < '0' || *digit > '9' ||
            parsed > (maximum - std::uint32_t(*digit - '0')) / 10u) return 0;
        parsed = parsed * 10u + std::uint32_t(*digit - '0');
    }
    return parsed;
}

std::uint64_t now_ns() noexcept {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

void update_max(std::uint64_t value) noexcept {
    auto observed = session_max_interval_ns.load(std::memory_order_relaxed);
    while (observed < value && !session_max_interval_ns.compare_exchange_weak(
        observed, value, std::memory_order_release, std::memory_order_relaxed)) {}
}
}

extern "C" bool tooie_rt64_vsync_requested() noexcept {
    return requested_vsync.load(std::memory_order_acquire);
}

extern "C" void tooie_rt64_present_observed(bool successful, bool vsync_actual,
    std::uint32_t source_hz, std::uint32_t target_hz) noexcept {
    latest_source_hz.store(source_hz, std::memory_order_relaxed);
    latest_target_hz.store(target_hz, std::memory_order_relaxed);
    latest_vsync_actual.store(vsync_actual, std::memory_order_release);
    if (!successful) return;
    const auto now = now_ns();
    const auto previous = last_timestamp_ns.exchange(now, std::memory_order_relaxed);
    const auto count = successful_presents.fetch_add(1, std::memory_order_acq_rel) + 1;
    if (previous != 0 && now > previous) {
        const auto interval = now - previous;
        intervals_ns[(count - 2) % interval_capacity].store(interval, std::memory_order_release);
        update_max(interval);
        if (interval > 50'000'000ull) {
            session_intervals_over_50ms.fetch_add(1, std::memory_order_relaxed);
            session_last_long_interval_ns.store(interval, std::memory_order_release);
            const auto started = probe_started_ns.load(std::memory_order_acquire);
            session_last_long_elapsed_ms.store(started != 0 && now >= started
                ? (now - started) / 1'000'000ull : 0, std::memory_order_release);
        }
    }
}

extern "C" bool tooie_rt64_present_trace_enabled() noexcept {
    static const bool enabled = [] {
        const char* value = std::getenv("TOOIE_PRESENT_TRACE");
        return value && std::string_view(value) == "1";
    }();
    return enabled;
}
extern "C" std::uint64_t tooie_rt64_present_trace_clock_ns() noexcept { return now_ns(); }

namespace {
void trace_present(std::uint64_t present_id,
    std::uint64_t workload_id, std::uint32_t mode, std::uint32_t vi_origin,
    std::uint32_t vi_framebuffer, std::uint32_t color_framebuffer,
    std::uint32_t vi_x, std::uint32_t vi_y, std::uint32_t vi_h,
    std::uint32_t vi_v, std::uint32_t index, std::uint32_t count,
    std::uint32_t source_hz, std::uint32_t target_hz, bool successful,
    bool vsync_actual, const tooie::pacing::PresentStageTimes* stages) noexcept {
    if (!tooie_rt64_present_trace_enabled()) return;
    // Called exclusively on RT64's present thread. Delay is relative to its
    // first present callback; the existing row cap also bounds duration=0.
    static const auto delay_ns = std::uint64_t(trace_milliseconds(
        "TOOIE_PRESENT_TRACE_DELAY_MS", 900000)) * 1'000'000ull;
    static const auto duration_ns = std::uint64_t(trace_milliseconds(
        "TOOIE_PRESENT_TRACE_DURATION_MS", 60000)) * 1'000'000ull;
    static std::uint32_t calls = 0;
    if (calls >= 4096) return;
    const auto now = now_ns();
    static const auto started = now;
    const auto elapsed = now - started;
    if (elapsed < delay_ns || (duration_ns != 0 && elapsed - delay_ns >= duration_ns)) return;
    // The first captured interval is unknown, never the preceding delay.
    static auto previous = now;
    const tooie::pacing::PresentStageTimes absent{};
    const auto& t = stages ? *stages : absent;
    std::fprintf(stderr,
        "TOOIE_PRESENT_TRACE n=%u us=%llu dt_us=%llu present=%llu workload=%llu "
        "mode=%u origin=%08x vi_fb=%08x color_fb=%08x x=%08x y=%08x h=%08x v=%08x "
        "index=%u count=%u source=%u target=%u success=%u vsync=%u "
        "stage_mask=%u phase_ns=%llu interp_ns=%llu acquire_ns=%llu worker_ns=%llu "
        "sleep_ns=%llu present_wait_ns=%llu present_ns=%llu\n",
        calls++, static_cast<unsigned long long>((now - started) / 1000),
        static_cast<unsigned long long>((now - previous) / 1000),
        static_cast<unsigned long long>(present_id),
        static_cast<unsigned long long>(workload_id), mode, vi_origin,
        vi_framebuffer, color_framebuffer, vi_x, vi_y, vi_h, vi_v,
        index, count, source_hz, target_hz, unsigned(successful), unsigned(vsync_actual),
        t.measured, static_cast<unsigned long long>(t.phase_ns),
        static_cast<unsigned long long>(t.interp_ns), static_cast<unsigned long long>(t.acquire_ns),
        static_cast<unsigned long long>(t.worker_ns), static_cast<unsigned long long>(t.sleep_ns),
        static_cast<unsigned long long>(t.present_wait_ns), static_cast<unsigned long long>(t.present_ns));
    previous = now;
}
} // namespace

extern "C" void tooie_rt64_present_trace(std::uint64_t present_id,
    std::uint64_t workload_id, std::uint32_t mode, std::uint32_t vi_origin,
    std::uint32_t vi_framebuffer, std::uint32_t color_framebuffer,
    std::uint32_t vi_x, std::uint32_t vi_y, std::uint32_t vi_h,
    std::uint32_t vi_v, std::uint32_t index, std::uint32_t count,
    std::uint32_t source_hz, std::uint32_t target_hz, bool successful,
    bool vsync_actual) noexcept {
    trace_present(present_id, workload_id, mode, vi_origin, vi_framebuffer, color_framebuffer,
        vi_x, vi_y, vi_h, vi_v, index, count, source_hz, target_hz, successful, vsync_actual, nullptr);
}

extern "C" void tooie_rt64_present_trace_staged(std::uint64_t present_id,
    std::uint64_t workload_id, std::uint32_t mode, std::uint32_t vi_origin,
    std::uint32_t vi_framebuffer, std::uint32_t color_framebuffer,
    std::uint32_t vi_x, std::uint32_t vi_y, std::uint32_t vi_h,
    std::uint32_t vi_v, std::uint32_t index, std::uint32_t count,
    std::uint32_t source_hz, std::uint32_t target_hz, bool successful,
    bool vsync_actual,
    const tooie::pacing::PresentStageTimes* stages) noexcept {
    trace_present(present_id, workload_id, mode, vi_origin, vi_framebuffer, color_framebuffer,
        vi_x, vi_y, vi_h, vi_v, index, count, source_hz, target_hz, successful, vsync_actual, stages);
}

namespace tooie::pacing {

void reset_present_probe() noexcept {
    successful_presents.store(0, std::memory_order_release);
    last_timestamp_ns.store(0, std::memory_order_release);
    probe_started_ns.store(now_ns(), std::memory_order_release);
    session_max_interval_ns.store(0, std::memory_order_release);
    session_intervals_over_50ms.store(0, std::memory_order_release);
    session_last_long_interval_ns.store(0, std::memory_order_release);
    session_last_long_elapsed_ms.store(0, std::memory_order_release);
    latest_source_hz.store(0, std::memory_order_release);
    latest_target_hz.store(0, std::memory_order_release);
    requested_vsync.store(true, std::memory_order_release);
    latest_vsync_actual.store(true, std::memory_order_release);
    for (auto& interval : intervals_ns) interval.store(0, std::memory_order_release);
}

void request_vsync(bool enabled) noexcept {
    requested_vsync.store(enabled, std::memory_order_release);
}

PresentSnapshot present_snapshot() noexcept {
    PresentSnapshot result{};
    result.successful_presents = successful_presents.load(std::memory_order_acquire);
    result.source_hz = latest_source_hz.load(std::memory_order_acquire);
    result.target_hz = latest_target_hz.load(std::memory_order_acquire);
    result.vsync_actual = latest_vsync_actual.load(std::memory_order_acquire);
    result.session_max_interval_ms = static_cast<double>(session_max_interval_ns.load(std::memory_order_acquire)) / 1'000'000.0;
    result.session_intervals_over_50ms = session_intervals_over_50ms.load(std::memory_order_acquire);
    result.session_last_long_interval_ms = static_cast<double>(session_last_long_interval_ns.load(std::memory_order_acquire)) / 1'000'000.0;
    result.session_last_long_elapsed_ms = session_last_long_elapsed_ms.load(std::memory_order_acquire);
    const auto available = result.successful_presents > 1
        ? std::min<std::uint64_t>(result.successful_presents - 1, interval_capacity) : 0;
    long double total = 0;
    for (std::uint64_t i = 0; i < available; ++i) {
        const auto value = intervals_ns[i].load(std::memory_order_acquire);
        if (value != 0) {
            total += value;
            ++result.interval_samples;
            result.max_interval_ms = std::max(result.max_interval_ms,
                static_cast<double>(value) / 1'000'000.0);
            if (value > 50'000'000ull) ++result.intervals_over_50ms;
        }
    }
    if (result.interval_samples != 0) {
        result.mean_interval_ms = static_cast<double>(total / result.interval_samples / 1'000'000.0L);
    }
    return result;
}

} // namespace tooie::pacing
