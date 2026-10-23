#include "camera_analog.hpp"

#include <bit>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>

#include "recomp.h"
#include "funcs.h"
#ifdef TOOIE_NATIVE_HOST
#include "frontend_input_preference.hpp"
#endif

// Right-stick orbit camera with the feel of Lighthouse's free-look
// (src/port/Enhancements/Camera/FreeLookCamera.cpp): entered past a 0.30
// stick, yaw changes linearly at 160 degrees per second, the camera keeps its
// captured distance and elevation around the focus, and it stays where it was
// left until the game takes the camera back. Tooie conventions, measured from
// the camera component: position at +0x74, rotation (pitch, yaw, roll in
// degrees) at +0x80, yaw = atan2(dx, dz) of position minus focus, and pitch =
// 360 - elevation. The yaw controller at +0x60 keeps target +0x10, current
// +0x14 and spring velocity +0x18; its +0x0C is nonzero while the game runs
// a yaw transition of its own.
namespace {
constexpr float k_yaw_speed_degrees = 160.0f; // Lighthouse kYawSpeed
constexpr float k_enter_threshold = 0.30f;    // Lighthouse kEnterThreshold
constexpr float k_aim_height_rate = 8.0f;     // Lighthouse kAimHeightRate
constexpr float k_degrees = 57.29577951308232f;
constexpr std::uint32_t k_camera_mode_rule_table = 0x801247ACu;
std::atomic_bool configured_analog_enabled{false};
std::atomic_bool configured_horizontal_inverted{false};
std::atomic_bool configured_vertical_inverted{false};

struct Orbit {
    bool active = false, entering = false;
    gpr component = 0;
    std::int32_t mode = 0;
    float yaw = 0.0f, elevation = 0.0f, distance = 0.0f, aim_y = 0.0f, pending_turn = 0.0f;
    // The input hook runs once per camera frame while manual rotation is allowed.
    std::uint32_t input_frames = 0, seen_input_frames = 0;
    std::chrono::steady_clock::time_point last_input{};
} orbit;

float wrap_degrees(float angle) {
    angle = std::fmod(angle, 360.0f);
    return angle < 0.0f ? angle + 360.0f : angle;
}
float load_float(std::uint8_t* rdram, gpr address) {
    return std::bit_cast<float>(static_cast<std::uint32_t>(MEM_W(0, address)));
}
void store_float(std::uint8_t* rdram, gpr address, float value) {
    MEM_W(0, address) = std::bit_cast<std::int32_t>(value);
}
void store_vector(std::uint8_t* rdram, gpr address, const float (&value)[3]) {
    for (int i = 0; i < 3; ++i) store_float(rdram, address + 4 * i, value[i]);
}

// Guest calls may scratch any caller-saved register; scratch vectors live in a
// frame below the hook's stack pointer, and the full context is restored.
template <typename Call>
void guest_call(recomp_context* ctx, Call&& call) {
    const auto saved = *ctx;
    try {
        ctx->r29 -= 0x60;
        call(ctx->r29 + 0x20);
        *ctx = saved;
    } catch (...) {
        *ctx = saved;
        throw;
    }
}

void exit_orbit() {
    orbit.active = orbit.entering = false;
    orbit.pending_turn = 0.0f;
}
}

void tooie::camera::set_analog_enabled(bool enabled) noexcept {
    configured_analog_enabled.store(enabled, std::memory_order_release);
}

bool tooie::camera::analog_enabled() noexcept {
    return configured_analog_enabled.load(std::memory_order_acquire);
}

void tooie::camera::set_horizontal_inverted(bool inverted) noexcept {
    configured_horizontal_inverted.store(inverted, std::memory_order_release);
}

bool tooie::camera::horizontal_inverted() noexcept {
    return configured_horizontal_inverted.load(std::memory_order_acquire);
}

void tooie::camera::set_vertical_inverted(bool inverted) noexcept {
    configured_vertical_inverted.store(inverted, std::memory_order_release);
}

bool tooie::camera::vertical_inverted() noexcept {
    return configured_vertical_inverted.load(std::memory_order_acquire);
}

extern "C" float tooie_camera_first_person_axis(float original_value, int vertical) noexcept {
#ifndef TOOIE_NATIVE_HOST
    (void)vertical;
    return original_value;
#else
    if (!tooie::camera::analog_enabled() || tooie::input::game_input_disabled()) {
        return original_value;
    }

    float right_x = 0.0f;
    float right_y = 0.0f;
    tooie::input::get_right_analog(0, &right_x, &right_y);
    auto value = vertical != 0 ? right_y : right_x;
    if (!std::isfinite(value) || value == 0.0f) return original_value;
    const bool inverted = vertical != 0 ? tooie::camera::vertical_inverted()
                                        : tooie::camera::horizontal_inverted();
    return inverted ? -value : value;
#endif
}

extern "C" int tooie_camera_analog_apply(std::uint8_t* rdram, recomp_context* ctx) {
#ifndef TOOIE_NATIVE_HOST
    (void)rdram;
    (void)ctx;
    return -1;
#else
    if (!tooie::camera::analog_enabled() || tooie::input::game_input_disabled()) {
        return -1;
    }

    // This hook is placed after func_800A4878's original eligibility gates.
    // Its 0x24($sp) slot holds the active camera component saved at 0x800A48BC.
    const auto component = MEM_W(ctx->r29, 0x24);
    if (component == 0) {
        return -1;
    }

    // func_80110EFC, used by the original C-left/C-right paths, first admits
    // only components whose mode's table entry has a nonzero +0xC field.
    const auto mode = static_cast<std::uint32_t>(MEM_W(component, 0x6C));
    const auto mode_rule_address = static_cast<gpr>(static_cast<std::int32_t>(
        k_camera_mode_rule_table + mode * 16u));
    if (MEM_H(0, mode_rule_address) == 0) {
        return -1;
    }

    // Manual rotation is allowed this camera frame.
    ++orbit.input_frames;
    const auto now = std::chrono::steady_clock::now();
    const float dt = orbit.last_input == std::chrono::steady_clock::time_point{} ? 0.0f
        : std::min(std::chrono::duration<float>(now - orbit.last_input).count(), 0.1f);
    orbit.last_input = now;

    float right_x = 0.0f;
    float right_y = 0.0f;
    // Shaped like Lighthouse's ReadStick (radial 0.15 deadzone, rescaled).
    tooie::input::get_right_analog(0, &right_x, &right_y);
    float horizontal = std::isfinite(right_x) ? right_x : 0.0f;
    if (tooie::camera::horizontal_inverted()) horizontal = -horizontal;

    // Tooie's heap relocates the camera component; same mode means the same camera.
    if (orbit.active || orbit.entering) orbit.component = component;
    if (orbit.active && orbit.mode != static_cast<std::int32_t>(mode)) {
        exit_orbit();
    }
    if (!orbit.active && !orbit.entering && std::hypot(right_x, right_y) >= k_enter_threshold) {
        orbit.entering = true;
        orbit.component = component;
        orbit.mode = static_cast<std::int32_t>(mode);
    }
    if (orbit.active || orbit.entering) {
        orbit.pending_turn += horizontal * k_yaw_speed_degrees * dt;
    }
    // The original C-left/C-right handling still runs; a C turn ends the orbit.
    return -1;
#endif
}

extern "C" void tooie_camera_orbit_apply(std::uint8_t* rdram, recomp_context* ctx) {
    // Called in func_80110588 just before the camera position (sp+0x34) and
    // rotation (sp+0x28) copies are handed to the view at 0x8011066C.
    const gpr component = ctx->r16;
    const bool input_this_frame = orbit.input_frames != orbit.seen_input_frames;
    orbit.seen_input_frames = orbit.input_frames;
    if (!(orbit.active || orbit.entering)) return;
    if (component != orbit.component) {
        // Relocated camera component (the input hook may run later this frame).
        if (MEM_W(0x6C, component) != orbit.mode) return;
        orbit.component = component;
    }

    const gpr yaw_controller = static_cast<gpr>(MEM_W(component, 0x60));
    const bool game_transition = MEM_H(0x0C, yaw_controller) != 0;
    if (!input_this_frame || game_transition || !tooie::camera::analog_enabled()) {
        // The game owns the camera again (C buttons, view toggle, mode change,
        // cutscene or menu). Its state already holds the orbit's last pose.
        exit_orbit();
        return;
    }

    float focus[3]{};
    guest_call(ctx, [&](gpr out) {
        ctx->r4 = component; ctx->r5 = out;
        func_80112550(rdram, ctx);
        for (int i = 0; i < 3; ++i) focus[i] = load_float(rdram, out + 4 * i);
    });

    float dt = 0.0f;
    {
        static std::chrono::steady_clock::time_point last{};
        const auto now = std::chrono::steady_clock::now();
        dt = last == std::chrono::steady_clock::time_point{} ? 0.0f
            : std::min(std::chrono::duration<float>(now - last).count(), 0.1f);
        last = now;
    }

    if (orbit.entering) {
        const float dx = load_float(rdram, component + 0x74) - focus[0];
        const float dy = load_float(rdram, component + 0x78) - focus[1];
        const float dz = load_float(rdram, component + 0x7C) - focus[2];
        const float horizontal = std::hypot(dx, dz);
        orbit.distance = std::hypot(horizontal, dy);
        if (orbit.distance < 1.0f) { exit_orbit(); return; }
        orbit.yaw = wrap_degrees(std::atan2(dx, dz) * k_degrees);
        orbit.elevation = std::atan2(dy, horizontal) * k_degrees;
        orbit.aim_y = focus[1];
        orbit.entering = false;
        orbit.active = true;
    } else {
        // Lighthouse SmoothTowards on the aim height keeps Banjo's bob out of the view.
        orbit.aim_y += (focus[1] - orbit.aim_y) * (1.0f - std::exp(-k_aim_height_rate * dt));
    }

    auto pose = [&](float yaw, float (&position)[3]) {
        const float elevation = orbit.elevation / k_degrees, heading = yaw / k_degrees;
        const float horizontal = orbit.distance * std::cos(elevation);
        position[0] = focus[0] + horizontal * std::sin(heading);
        position[1] = focus[1] + orbit.distance * std::sin(elevation);
        position[2] = focus[2] + horizontal * std::cos(heading);
    };

    // Take this frame's turn unless the game's own wall check (func_8011349C,
    // as used by the C-button turn) reports the camera's path blocked.
    float position[3];
    if (orbit.pending_turn != 0.0f) {
        const float turned = wrap_degrees(orbit.yaw + orbit.pending_turn);
        float from[3], to[3];
        pose(orbit.yaw, from);
        pose(turned, to);
        bool clear = true;
        guest_call(ctx, [&](gpr out) {
            store_vector(rdram, out, from);
            store_vector(rdram, out + 0x10, to);
            ctx->r4 = out; ctx->r5 = out + 0x10;
            func_8011349C(rdram, ctx);
            clear = ctx->r2 != 0;
        });
        if (clear) orbit.yaw = turned;
        orbit.pending_turn = 0.0f;
    }
    pose(orbit.yaw, position);

    const float dx = position[0] - focus[0], dz = position[2] - focus[2];
    const float dy = position[1] - orbit.aim_y; // Look at the smoothed aim height.
    const float rotation[3] = {wrap_degrees(-std::atan2(dy, std::hypot(dx, dz)) * k_degrees),
                               wrap_degrees(std::atan2(dx, dz) * k_degrees), 0.0f};

    // The view copies, and the camera's own state so the game resumes from here.
    store_vector(rdram, ctx->r29 + 0x34, position);
    store_vector(rdram, ctx->r29 + 0x28, rotation);
    store_vector(rdram, component + 0x74, position);
    store_vector(rdram, component + 0x80, rotation);
    store_float(rdram, yaw_controller + 0x10, orbit.yaw);
    store_float(rdram, yaw_controller + 0x14, orbit.yaw);
    store_float(rdram, yaw_controller + 0x18, 0.0f);
}
