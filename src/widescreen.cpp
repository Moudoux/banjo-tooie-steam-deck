#include "widescreen.hpp"

#include "boot_hooks.h"

#include <atomic>

namespace {
std::atomic_bool widescreen_profile_override_latched{false};
std::atomic_bool configured_widescreen_enabled{false};
std::atomic_bool latched_widescreen_enabled{false};
std::atomic<tooie::widescreen::NativeAspect> configured_native_aspect_value{
    tooie::widescreen::NativeAspect::Ratio16x9};
std::atomic<tooie::widescreen::NativeAspect> latched_native_aspect_value{
    tooie::widescreen::NativeAspect::Ratio16x9};

constexpr tooie::widescreen::NativeAspect normalize_aspect(
    tooie::widescreen::NativeAspect aspect) noexcept {
    switch (aspect) {
        case tooie::widescreen::NativeAspect::Ratio16x9:
        case tooie::widescreen::NativeAspect::Ratio21x9:
        case tooie::widescreen::NativeAspect::Ratio32x9:
        case tooie::widescreen::NativeAspect::Ratio43x18:
        case tooie::widescreen::NativeAspect::Ratio16x10:
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
        case tooie::widescreen::NativeAspect::Ratio16x10: return 16.0 / 10.0;
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

void latch_for_game_start() noexcept {
    latched_widescreen_enabled.store(
        configured_widescreen_enabled.load(std::memory_order_acquire),
        std::memory_order_release);
    latched_native_aspect_value.store(
        configured_native_aspect_value.load(std::memory_order_acquire),
        std::memory_order_release);
    widescreen_profile_override_latched.store(true, std::memory_order_release);
}

NativeAspect configured_native_aspect() noexcept {
    return configured_native_aspect_value.load(std::memory_order_acquire);
}

NativeAspect latched_native_aspect() noexcept {
    return latched_native_aspect_value.load(std::memory_order_acquire);
}

double latched_aspect_ratio() noexcept {
    return aspect_ratio(latched_native_aspect());
}

bool latched_enabled() noexcept {
    return latched_widescreen_enabled.load(std::memory_order_acquire);
}

bool profile_override_active() noexcept {
    return widescreen_profile_override_latched.load(std::memory_order_acquire);
}

uint32_t latched_settings_word() noexcept {
    if (!profile_override_active()) return 0;
    return 1U | (latched_enabled() ? 2U : 0U) |
           (static_cast<uint32_t>(normalize_aspect(latched_native_aspect())) << 2);
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

