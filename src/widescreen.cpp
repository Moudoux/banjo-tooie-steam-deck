#include "widescreen.hpp"

#include "boot_hooks.h"

#include <atomic>
#include <algorithm>
#include <cstring>

namespace {
std::atomic_bool widescreen_profile_override_latched{false};
std::atomic_bool configured_widescreen_enabled{false};
std::atomic_bool latched_widescreen_enabled{false};
std::atomic<tooie::widescreen::NativeAspect> configured_native_aspect_value{
    tooie::widescreen::NativeAspect::Ratio16x9};
std::atomic<std::uint32_t> current_aspect_bits{0};
std::atomic<int> wide_request{-1}; // Baseline SDL owner publishes -1/0/1 only.

constexpr tooie::widescreen::NativeAspect normalize_aspect(
    tooie::widescreen::NativeAspect aspect) noexcept {
    switch (aspect) {
        case tooie::widescreen::NativeAspect::Ratio16x9:
        case tooie::widescreen::NativeAspect::Ratio21x9:
        case tooie::widescreen::NativeAspect::Ratio32x9:
        case tooie::widescreen::NativeAspect::Ratio43x18:
            return aspect;
        default:
            return tooie::widescreen::NativeAspect::Ratio16x9;
    }
}

constexpr double aspect_ratio(tooie::widescreen::NativeAspect aspect) noexcept {
    switch (normalize_aspect(aspect)) {
        case tooie::widescreen::NativeAspect::Ratio21x9: return 21.0 / 9.0;
        case tooie::widescreen::NativeAspect::Ratio32x9: return 32.0 / 9.0;
        case tooie::widescreen::NativeAspect::Ratio43x18: return 43.0 / 18.0;
        default: return 16.0 / 9.0;
    }
}
}

namespace tooie::widescreen {

void configure_profile(bool enabled) noexcept {
    configured_widescreen_enabled.store(enabled, std::memory_order_release);
}

void configure_native_aspect(NativeAspect aspect) noexcept {
    configured_native_aspect_value.store(normalize_aspect(aspect), std::memory_order_release);
}

// Latches enabled/override only; the aspect bits are a seed that
// set_live_aspect_bits later replaces.
void latch_for_game_start() noexcept {
    latched_widescreen_enabled.store(
        configured_widescreen_enabled.load(std::memory_order_acquire),
        std::memory_order_release);
    current_aspect_bits.store(
        static_cast<uint32_t>(configured_native_aspect_value.load(std::memory_order_acquire)) <<
            TOOIE_WIDESCREEN_SETTINGS_ASPECT_SHIFT,
        std::memory_order_release);
    widescreen_profile_override_latched.store(true, std::memory_order_release);
}

NativeAspect configured_native_aspect() noexcept {
    return configured_native_aspect_value.load(std::memory_order_acquire);
}

// Live aspect, despite the name (see widescreen.hpp).
NativeAspect latched_native_aspect() noexcept {
    const auto bits = current_aspect_bits.load(std::memory_order_acquire);
    if (bits & TOOIE_WIDESCREEN_SETTINGS_FIT) return NativeAspect::Ratio16x9;
    return NativeAspect((bits >> TOOIE_WIDESCREEN_SETTINGS_ASPECT_SHIFT) &
        TOOIE_WIDESCREEN_SETTINGS_ASPECT_MASK);
}

double latched_aspect_ratio() noexcept {
    const auto bits = current_aspect_bits.load(std::memory_order_acquire);
    if (bits & TOOIE_WIDESCREEN_SETTINGS_FIT) {
        float scale;
        const uint32_t encoded = bits & TOOIE_WIDESCREEN_SETTINGS_FIT_SCALE_MASK;
        std::memcpy(&scale, &encoded, sizeof scale);
        return double(scale) * (16.0 / 9.0);
    }
    return aspect_ratio(NativeAspect((bits >> TOOIE_WIDESCREEN_SETTINGS_ASPECT_SHIFT) &
        TOOIE_WIDESCREEN_SETTINGS_ASPECT_MASK));
}

void set_live_aspect_bits(std::uint32_t bits) noexcept {
    current_aspect_bits.store(bits & ~(TOOIE_WIDESCREEN_SETTINGS_ACTIVE |
        TOOIE_WIDESCREEN_SETTINGS_ENABLED), std::memory_order_release);
}

void set_live_widescreen_request(int request) noexcept {
    if (request >= -1 && request <= 1) wide_request.store(request, std::memory_order_release);
}

int live_widescreen_request() noexcept {
    return wide_request.load(std::memory_order_acquire);
}

uint32_t fit_aspect_bits(int width, int height) noexcept {
    if (width <= 0 || height <= 0) return 0;
    const float scale = std::clamp(float(double(width) / height * (9.0 / 16.0)), 1.0f, 2.0f);
    uint32_t bits;
    static_assert(sizeof bits == sizeof scale);
    std::memcpy(&bits, &scale, sizeof bits);
    return (bits & TOOIE_WIDESCREEN_SETTINGS_FIT_SCALE_MASK) | TOOIE_WIDESCREEN_SETTINGS_FIT;
}

bool latched_enabled() noexcept {
    return latched_widescreen_enabled.load(std::memory_order_acquire);
}

bool profile_override_active() noexcept {
    return widescreen_profile_override_latched.load(std::memory_order_acquire);
}

// Latched enabled/override bits plus the live aspect bits.
uint32_t latched_settings_word() noexcept {
    if (!profile_override_active()) return 0;
    return TOOIE_WIDESCREEN_SETTINGS_ACTIVE |
           (latched_enabled() ? TOOIE_WIDESCREEN_SETTINGS_ENABLED : 0U) |
           current_aspect_bits.load(std::memory_order_acquire);
}

} // namespace tooie::widescreen

extern "C" uint32_t tooie_widescreen_latched_settings_word(void) {
    return tooie::widescreen::latched_settings_word();
}

extern "C" void tooie_get_latched_widescreen_settings(
    uint8_t*, recomp_context* ctx) {
    ctx->r2 = static_cast<gpr>(static_cast<int32_t>(
        tooie::widescreen::latched_settings_word()));
}

