#pragma once

#include <cstdint>

namespace tooie::widescreen {

enum class NativeAspect : std::uint32_t {
    Ratio16x9 = 0,
    Ratio21x9 = 1,
    Ratio32x9 = 2,
    Ratio43x18 = 3,
    Ratio16x10 = 4,
};

// The frontend profile owns this optional preference. The value is latched when
// Start Game enters native Tooie; later UI saves take effect on the next game
// start. Diagnostic entry points retain the game's stored flag.
void configure_profile(bool enabled) noexcept;
void configure_native_aspect(NativeAspect aspect) noexcept;
void latch_for_game_start() noexcept;
bool latched_enabled() noexcept;
NativeAspect configured_native_aspect() noexcept;
NativeAspect latched_native_aspect() noexcept;
double latched_aspect_ratio() noexcept;
bool profile_override_active() noexcept;
// Canonical transport: inactive0; active bit0, enabled bit1, enum bits2..3.
// Reads session-latched preferences only; no guest access or projection math.
uint32_t latched_settings_word() noexcept;

} // namespace tooie::widescreen
