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
// stick, yaw changes linearly at 160 and elevation at 100 degrees per second
// (pushing up raises the camera, as there), the distance is the one captured
// on entry kept inside the mode's own range, and the camera stays where it
// was left until the game takes the camera back. Collision follows
// Lighthouse's free-look update: the pose is swept and slid from last frame's
// position, a slide that crossed over is undone, a camera that keeps
// colliding while Banjo is hidden is moved to a clear spot, and the orbit
// yaw is re-read from wherever the camera ended up. Tooie has a counterpart
// for each of those Banjo-Kazooie routines. Tooie conventions, measured from
// the camera component: position at +0x74, rotation (pitch, yaw, roll in
// degrees) at +0x80, yaw = atan2(dx, dz) of position minus focus, and pitch =
// 360 - elevation. The yaw controller at +0x60 keeps target +0x10, current
// +0x14 and spring velocity +0x18; its +0x0C is nonzero while the game runs
// a yaw transition of its own.
namespace {
constexpr float k_yaw_speed_degrees = 160.0f; // Lighthouse kYawSpeed
constexpr float k_pitch_speed_degrees = 100.0f; // Lighthouse kPitchSpeed
// Lighthouse kMinPitch/kMaxPitch (-85, 40), as elevation above the focus.
constexpr float k_min_elevation = -40.0f;
constexpr float k_max_elevation = 85.0f;
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
    float yaw = 0.0f, elevation = 0.0f, distance = 0.0f, aim_y = 0.0f, pending_turn = 0.0f,
        pending_pitch = 0.0f;
    // Lighthouse's crossover guard (func_802C03BC) and occlusion probe
    // (func_802BC84C) state; both only advance on frames that collided.
    float crossover_dot = 0.0f;
    int occlusion_probe = 0;
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
void load_vector(std::uint8_t* rdram, gpr address, float (&value)[3]) {
    for (int i = 0; i < 3; ++i) value[i] = load_float(rdram, address + 4 * i);
}
bool rdram_pointer(gpr address) {
    const auto value = static_cast<std::uint32_t>(address);
    return value >= 0x80000000u && value < 0x80800000u && (value & 3u) == 0;
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
    orbit.pending_turn = orbit.pending_pitch = 0.0f;
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
    float vertical = std::isfinite(right_y) ? right_y : 0.0f;
    if (tooie::camera::vertical_inverted()) vertical = -vertical;

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
        orbit.pending_pitch += vertical * k_pitch_speed_degrees * dt;
    }
    // The original C-left/C-right handling still runs; a C turn ends the orbit.
    return -1;
#endif
}

extern "C" void tooie_camera_orbit_apply(std::uint8_t* rdram, recomp_context* ctx) {
    // Called in func_80110588 at 0x8011061C, after the mode update and before
    // the position (+0x74) and rotation (+0x80) are copied for the view, so
    // the game's shake and roll still apply on top of the orbit.
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
    // A nonzero +0x72 skips this frame's mode update, including the copy of
    // the start position (func_8011347C) that the collision sweep starts from.
    if (MEM_BU(component, 0x72) != 0) return;

    float focus[3]{};
    guest_call(ctx, [&](gpr out) {
        ctx->r4 = component; ctx->r5 = out;
        func_80112550(rdram, ctx);
        load_vector(rdram, out, focus);
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
        orbit.crossover_dot = 0.0f;
        orbit.occlusion_probe = 0;
        orbit.entering = false;
        orbit.active = true;
    } else {
        // Lighthouse SmoothTowards on the aim height keeps Banjo's bob out of the view.
        orbit.aim_y += (focus[1] - orbit.aim_y) * (1.0f - std::exp(-k_aim_height_rate * dt));
    }

    // The mode's distance range (func_80115358 clamps to +0xC..+0x10 of the
    // block at +0x14), so an orbit entered while the game had pulled the
    // camera in close returns to a normal distance, as Lighthouse orbits at
    // the game's zoom distance rather than the live one.
    const gpr range = static_cast<gpr>(MEM_W(component, 0x14));
    if (rdram_pointer(range)) {
        const float near_limit = load_float(rdram, range + 0xC), far_limit = load_float(rdram, range + 0x10);
        if (std::isfinite(near_limit) && std::isfinite(far_limit) && near_limit > 1.0f && near_limit <= far_limit)
            orbit.distance = std::clamp(orbit.distance, near_limit, far_limit);
    }

    orbit.yaw = wrap_degrees(orbit.yaw + orbit.pending_turn);
    // Captured outside the limits (the game's own angle): only move inwards.
    orbit.elevation = std::clamp(orbit.elevation + orbit.pending_pitch,
        std::min(k_min_elevation, orbit.elevation), std::max(k_max_elevation, orbit.elevation));
    orbit.pending_turn = orbit.pending_pitch = 0.0f;

    float target[3];
    {
        const float pitch = orbit.elevation / k_degrees, heading = orbit.yaw / k_degrees;
        const float horizontal = orbit.distance * std::cos(pitch);
        target[0] = focus[0] + horizontal * std::sin(heading);
        target[1] = focus[1] + orbit.distance * std::sin(pitch);
        target[2] = focus[2] + horizontal * std::cos(heading);
    }

    // func_8011329C (Lighthouse's func_802BE60C) sweeps a 45-unit sphere from
    // the position func_8011347C saved at the start of this frame (+0x28, last
    // frame's orbit pose) to +0x74 and slides along what it hits. It honours
    // the game's no-collision flag and returns nonzero when it moved the camera.
    const gpr start_address = static_cast<gpr>(MEM_W(component, 0x28));
    if (!rdram_pointer(start_address)) { exit_orbit(); return; }
    float start[3];
    load_vector(rdram, start_address, start);
    store_vector(rdram, component + 0x74, target);
    bool collided = false;
    guest_call(ctx, [&](gpr) {
        ctx->r4 = component;
        func_8011329C(rdram, ctx);
        collided = ctx->r2 != 0;
    });

    if (collided) {
        float camera[3];
        load_vector(rdram, component + 0x74, camera);
        // Lighthouse's func_802BC84C: one probe per colliding frame, from the
        // camera to Banjo's focus, then beside it (100 units either way) and
        // above it. Five blocked probes in a row move the camera to a clear
        // spot with func_801138F4, Tooie's func_802BC640.
        float to_focus[3] = {focus[0] - camera[0], focus[1] - camera[1], focus[2] - camera[2]};
        float length = std::sqrt(to_focus[0] * to_focus[0] + to_focus[1] * to_focus[1] + to_focus[2] * to_focus[2]);
        float probe[3] = {focus[0], focus[1], focus[2]};
        if (length > 0.001f) {
            for (auto& value : to_focus) value /= length;
            if (length > 1500.0f)
                for (int i = 0; i < 3; ++i) probe[i] = camera[i] + to_focus[i] * 1500.0f;
            const float side = orbit.occlusion_probe == 1 ? -100.0f : 100.0f;
            switch (orbit.occlusion_probe) {
            case 1: case 2: // 100 x the view direction, yawed 90 degrees either way.
                probe[0] += -to_focus[2] * side;
                probe[1] += to_focus[1] * 100.0f;
                probe[2] += to_focus[0] * side;
                break;
            case 3: probe[1] += 100.0f; break;
            default: break;
            }
        }
        bool hidden = false;
        guest_call(ctx, [&](gpr out) {
            store_vector(rdram, out, camera);
            store_vector(rdram, out + 0x10, probe);
            ctx->r4 = out; ctx->r5 = out + 0x10;
            func_8011349C(rdram, ctx);
            hidden = ctx->r2 == 0;
        });
        bool recovered = false;
        if (!hidden) {
            orbit.occlusion_probe = 0;
        } else if (++orbit.occlusion_probe >= 5) {
            orbit.occlusion_probe = 0;
            guest_call(ctx, [&](gpr) {
                ctx->r4 = component;
                func_801138F4(rdram, ctx);
                recovered = ctx->r2 != 0;
            });
        }
        if (!recovered) {
            // Lighthouse's func_802C03BC (Tooie's func_80111C04): a slide that
            // turned back against the intended move, this frame or the last
            // colliding one, crossed through something; stay where we were.
            const float wanted[3] = {target[0] - start[0], target[1] - start[1], target[2] - start[2]};
            const float moved[3] = {camera[0] - start[0], camera[1] - start[1], camera[2] - start[2]};
            const float wanted_length = std::sqrt(wanted[0] * wanted[0] + wanted[1] * wanted[1] + wanted[2] * wanted[2]);
            const float moved_length = std::sqrt(moved[0] * moved[0] + moved[1] * moved[1] + moved[2] * moved[2]);
            const float dot = wanted_length > 0.0f && moved_length > 0.0f
                ? (wanted[0] * moved[0] + wanted[1] * moved[1] + wanted[2] * moved[2]) / (wanted_length * moved_length)
                : 0.0f;
            if (dot < 0.0f || orbit.crossover_dot < 0.0f) store_vector(rdram, component + 0x74, start);
            orbit.crossover_dot = dot;
        }
        // Lighthouse's func_802C04B0: the orbit continues from where the
        // camera really is, so pushing into a wall does not wind up the yaw.
        load_vector(rdram, component + 0x74, camera);
        const float dx = camera[0] - focus[0], dz = camera[2] - focus[2];
        if (std::hypot(dx, dz) > 0.001f) orbit.yaw = wrap_degrees(std::atan2(dx, dz) * k_degrees);
    }

    float position[3];
    load_vector(rdram, component + 0x74, position);
    const float dx = position[0] - focus[0], dz = position[2] - focus[2];
    const float dy = position[1] - orbit.aim_y; // Look at the smoothed aim height.
    const float rotation[3] = {wrap_degrees(-std::atan2(dy, std::hypot(dx, dz)) * k_degrees),
                               wrap_degrees(std::atan2(dx, dz) * k_degrees), 0.0f};

    // The camera's own state, which the view copies next and the game resumes from.
    store_vector(rdram, component + 0x80, rotation);
    store_float(rdram, yaw_controller + 0x10, orbit.yaw);
    store_float(rdram, yaw_controller + 0x14, orbit.yaw);
    store_float(rdram, yaw_controller + 0x18, 0.0f);
}
