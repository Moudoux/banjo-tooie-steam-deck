#pragma once
#include <cmath>
namespace tooie::input {
inline float baseline_axis(int value) noexcept {
    const float v = float(value) / (value < 0 ? 32768.0f : 32767.0f);
    return std::abs(v) <= .20f ? 0.0f : std::copysign((std::abs(v) - .20f) / .80f, v);
}
inline float baseline_keyboard_override(float keyboard, float controller) noexcept {
    return keyboard != 0 ? keyboard : controller;
}
}
