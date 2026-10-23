#pragma once

#include <cstdint>
#include "../patches/game/widescreen_settings_abi.h"

namespace tooie::widescreen {

enum class NativeAspect : std::uint32_t {
    Ratio16x9 = 0,
    Ratio21x9 = 1,
    Ratio32x9 = 2,
    Ratio43x18 = 3,
};

// Legacy profile stays startup-latched. Baseline publishes a live wide request;
// the guest halfword owns application and presentation acknowledges it.
// The "latched_" names are historical: enabled/override are latched at game
// start, but aspect/fit bits are seeded there and then follow
// set_live_aspect_bits (live publication). Names are kept for existing users.
void configure_profile(bool enabled) noexcept;
void configure_native_aspect(NativeAspect aspect) noexcept;
void latch_for_game_start() noexcept;
bool latched_enabled() noexcept;
NativeAspect configured_native_aspect() noexcept;
NativeAspect latched_native_aspect() noexcept;
double latched_aspect_ratio() noexcept;
bool profile_override_active() noexcept;
void set_live_aspect_bits(std::uint32_t bits) noexcept;
void set_live_widescreen_request(int request) noexcept;
int live_widescreen_request() noexcept;
// Positive drawable dimensions; scale clamps to [1,2] (16:9..32:9), or [0.9,2]
// (16:10..32:9) outside Windows.
std::uint32_t fit_aspect_bits(int width, int height) noexcept;
// Canonical transport: inactive0; startup bits0..1; atomic live aspect bits2..31.
uint32_t latched_settings_word() noexcept;

} // namespace tooie::widescreen
