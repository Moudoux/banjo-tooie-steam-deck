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
// (pushing up raises the camera, as there), the distance eases to the far
// end of the mode's own range (which follows the game's view toggle and the
// statue's larger range) times the distance setting, and the camera stays
// where it was left until the game takes the camera back. Flight (ncbafly), where the game allows no manual rotation, also
// orbits; there the game's chase camera takes over again soon after the
// stick is let go, since Banjo keeps turning. Collision follows
// Lighthouse's free-look update: the pose is swept and slid from last frame's
// position, a slide that crossed over is undone, a camera that keeps
// colliding while Banjo is hidden is moved to a clear spot, and the orbit
// yaw is re-read from wherever the camera ended up. Tooie has a counterpart
// for each of those Banjo-Kazooie routines. Tooie conventions, measured from
// the camera component: position at +0x74, rotation (pitch, yaw, roll in
// degrees) at +0x80, yaw = atan2(dx, dz) of position minus focus, and pitch =
// 360 - elevation. The yaw controller at +0x60 keeps target +0x10, current
// +0x14 and spring velocity +0x18; +0x0C is the kind of yaw transition the
// game is running (func_80112D90) and +0x0E its priority: 2 for the player's
// C-left/C-right turns and R centring, 1 for the follow camera's own wall
// avoidance (func_80113978), which the orbit cancels.
namespace {
constexpr float k_yaw_speed_degrees = 160.0f; // Lighthouse kYawSpeed
constexpr float k_pitch_speed_degrees = 100.0f; // Lighthouse kPitchSpeed
// Lighthouse kMinPitch/kMaxPitch (-85, 40), as elevation above the focus.
constexpr float k_min_elevation = -40.0f;
constexpr float k_max_elevation = 85.0f;
constexpr float k_enter_threshold = 0.30f;    // Lighthouse kEnterThreshold
constexpr float k_aim_height_rate = 8.0f;     // Lighthouse kAimHeightRate
constexpr float k_degrees = 57.29577951308232f;
constexpr float k_distance_rate = 4.0f;       // Easing towards the orbit distance.
constexpr float k_min_distance = 100.0f;
constexpr float k_fly_release_seconds = 0.6f; // Released stick in flight: back to the chase camera.
// The input hook is skipped on some frames (a move's camera lock, a camera
// zone, flight); the orbit only ends after this long without it.
constexpr float k_input_gap_seconds = 0.5f;
constexpr std::int16_t k_player_turn_priority = 2;
// D_8012C760, the game's frame delta (time_getDelta, as Lighthouse's free-look
// uses). Wall-clock time between hook calls is uneven, since the recompiled
// game runs a frame in a burst and then waits for the display, and turned
// the camera in uneven steps.
constexpr std::uint32_t k_game_delta = 0x8012C760u;
constexpr std::uint32_t k_camera_mode_rule_table = 0x801247ACu;
constexpr std::int32_t k_fly_mode = 3;        // ncbafly; its rule entry is zero.
std::atomic_bool configured_analog_enabled{false};
std::atomic_bool configured_horizontal_inverted{false};
std::atomic_bool configured_vertical_inverted{false};
std::atomic<float> configured_mouse_sensitivity{0.06f};
std::atomic_bool configured_mouse_inverted{true};
std::atomic<float> configured_distance_scale{1.0f};

struct Orbit {
    bool active = false, entering = false;
    // The player whose camera this is: with Mumbo's statue or split-up, more
    // than one camera component is updated each frame.
    gpr component = 0, player = 0;
    std::int32_t mode = 0;
    float yaw = 0.0f, elevation = 0.0f, distance = 0.0f, aim_y = 0.0f, released_seconds = 0.0f;
    // Lighthouse's crossover guard (func_802C03BC) and occlusion probe
    // (func_802BC84C) state; both only advance on frames that collided.
    float crossover_dot = 0.0f;
    int occlusion_probe = 0;
    // The input hook runs once per player frame while manual rotation is allowed.
    std::uint32_t input_frames = 0, seen_input_frames = 0;
    float input_gap_seconds = 0.0f;
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
float game_delta(std::uint8_t* rdram) {
    const float dt = load_float(rdram, static_cast<gpr>(static_cast<std::int32_t>(k_game_delta)));
    return std::isfinite(dt) && dt > 0.0f ? std::min(dt, 0.1f) : 0.0f;
}
// The right stick as the orbit uses it, shaped like Lighthouse's ReadStick
// (radial 0.15 deadzone, rescaled) by the input module, with the inversions.
void read_stick(float& horizontal, float& vertical) {
    horizontal = vertical = 0.0f;
#ifdef TOOIE_NATIVE_HOST
    if (tooie::input::game_input_disabled()) return;
    float right_x = 0.0f, right_y = 0.0f;
    tooie::input::get_right_analog(0, &right_x, &right_y);
    if (std::isfinite(right_x)) horizontal = configured_horizontal_inverted.load(std::memory_order_acquire) ? -right_x : right_x;
    if (std::isfinite(right_y)) vertical = configured_vertical_inverted.load(std::memory_order_acquire) ? -right_y : right_y;
#endif
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

void tooie::camera::set_mouse_aim_sensitivity(float degrees_per_count) noexcept {
    configured_mouse_sensitivity.store(std::clamp(degrees_per_count, 0.01f, 2.0f), std::memory_order_release);
}

void tooie::camera::set_mouse_aim_inverted(bool inverted) noexcept {
    configured_mouse_inverted.store(inverted, std::memory_order_release);
}

void tooie::camera::set_orbit_distance_scale(float scale) noexcept {
    configured_distance_scale.store(std::clamp(scale, 0.5f, 1.5f), std::memory_order_release);
}

#ifdef TOOIE_NATIVE_HOST
namespace {
// This frame's mouse turn in Tooie's first-person rotation units (degrees; pitch
// falls when looking up, yaw falls when turning right, like the stick math).
// Keeps relative mouse mode on while first-person control runs; motion from
// more than 150 ms before (before entering first person) is dropped.
bool take_mouse_turn(float& pitch, float& yaw) {
    tooie::input::note_first_person_aim();
    static std::chrono::steady_clock::time_point last_call{};
    const auto now = std::chrono::steady_clock::now();
    const bool continuing = last_call != std::chrono::steady_clock::time_point{} &&
        now - last_call < std::chrono::milliseconds(150);
    last_call = now;
    float x = 0.0f, y = 0.0f;
    tooie::input::take_mouse_aim(&x, &y);
    if (!continuing || tooie::input::game_input_disabled() || !std::isfinite(x) || !std::isfinite(y)) return false;
    const float sensitivity = configured_mouse_sensitivity.load(std::memory_order_acquire);
    if (configured_mouse_inverted.load(std::memory_order_acquire)) y = -y;
    pitch = y * sensitivity; // Moving up (negative y) looks up.
    yaw = -x * sensitivity;  // Moving right turns right.
    return true;
}
// Egg aim takes the turn in its pitch hook when pitch is unlocked; the yaw hook
// (reached on every egg-aim frame) then uses the yaw taken there.
bool egg_yaw_pending = false;
float egg_yaw = 0.0f;
}
#endif

extern "C" void tooie_first_person_aim(std::uint8_t* rdram, recomp_context* ctx) {
#ifndef TOOIE_NATIVE_HOST
    (void)rdram;
    (void)ctx;
#else
    (void)rdram;
    float pitch = 0.0f, yaw = 0.0f;
    if (!take_mouse_turn(pitch, yaw)) return;
    ctx->f12.fl += pitch;
    ctx->f6.fl += yaw;
#endif
}

extern "C" void tooie_egg_aim_pitch(std::uint8_t* rdram, recomp_context* ctx) {
#ifndef TOOIE_NATIVE_HOST
    (void)rdram;
    (void)ctx;
#else
    float pitch = 0.0f, yaw = 0.0f;
    const bool taken = take_mouse_turn(pitch, yaw);
    egg_yaw_pending = true;
    egg_yaw = taken ? yaw : 0.0f;
    if (taken) store_float(rdram, ctx->r29 + 0x24, load_float(rdram, ctx->r29 + 0x24) + pitch);
#endif
}

extern "C" void tooie_egg_aim_yaw(std::uint8_t* rdram, recomp_context* ctx) {
#ifndef TOOIE_NATIVE_HOST
    (void)rdram;
    (void)ctx;
#else
    float yaw = egg_yaw;
    bool taken = egg_yaw_pending;
    egg_yaw_pending = false;
    if (!taken) {
        float pitch = 0.0f; // Pitch is locked on this frame.
        taken = take_mouse_turn(pitch, yaw);
    }
    if (taken && yaw != 0.0f) store_float(rdram, ctx->r29 + 0x28, load_float(rdram, ctx->r29 + 0x28) + yaw);
#endif
}

namespace {
// The Blaster eases its look angle back to a target: in control type 1 one set
// by the left stick's position, in type 0 the level angle while +0x254 is set.
// The right stick's accumulated look is added to both targets, so the view
// stays where the right stick left it. Reset when the Blaster is re-entered.
float blaster_look_offset = 0.0f;
std::chrono::steady_clock::time_point blaster_last_look{};
// Turn and look at the same rate, as the user asked for the Blaster.
constexpr float k_blaster_look_speed_degrees = k_yaw_speed_degrees;

float register_float(gpr value) { return std::bit_cast<float>(static_cast<std::uint32_t>(value)); }
gpr float_register(float value) { return static_cast<gpr>(std::bit_cast<std::int32_t>(value)); }
}

// Twin-stick Blaster while the analog camera is on: the right stick turns, so
// the left stick's X strafes (added to the C-left/C-right axis, -1 left) and
// no longer turns. control_type only tells the two hook sites apart.
extern "C" void tooie_blaster_strafe(std::uint8_t* rdram, recomp_context* ctx, int control_type) {
    (void)control_type;
    if (!tooie::camera::analog_enabled()) return;
    const gpr player = static_cast<gpr>(MEM_W(0x30, ctx->r29)); // func_80804848's a0.
    float stick_x = 0.0f;
    guest_call(ctx, [&](gpr) {
        ctx->r4 = player;
        bastick_getX(rdram, ctx);
        stick_x = ctx->f0.fl;
    });
    if (!std::isfinite(stick_x) || stick_x == 0.0f) return;
    ctx->r5 = float_register(std::clamp(register_float(ctx->r5) + stick_x, -1.0f, 1.0f));
}

extern "C" void tooie_blaster_left_turn(std::uint8_t* rdram, recomp_context* ctx, int control_type) {
    (void)rdram;
    (void)control_type;
    if (tooie::camera::analog_enabled()) ctx->r5 = float_register(0.0f);
}

extern "C" void tooie_blaster_turn(std::uint8_t* rdram, recomp_context* ctx) {
    if (!tooie::camera::analog_enabled()) return;
    float horizontal = 0.0f, vertical = 0.0f;
    read_stick(horizontal, vertical);
    if (horizontal == 0.0f) return;
    // Facing yaw turns the same way as the orbit's camera yaw (the view is
    // facing + 180), at the orbit's rate.
    const gpr delta = ctx->r29 + 0x28;
    store_float(rdram, delta, load_float(rdram, delta) + horizontal * k_yaw_speed_degrees * game_delta(rdram));
}

extern "C" void tooie_blaster_look(std::uint8_t* rdram, recomp_context* ctx) {
    const gpr state = ctx->r16; // The Blaster state at player +0x188.
    const auto now = std::chrono::steady_clock::now();
    if (now - blaster_last_look > std::chrono::milliseconds(500)) blaster_look_offset = 0.0f;
    blaster_last_look = now;
    if (!tooie::camera::analog_enabled()) { blaster_look_offset = 0.0f; return; }
    float horizontal = 0.0f, vertical = 0.0f;
    read_stick(horizontal, vertical);
    if (vertical == 0.0f) return;
    // +0x20C rises as the view tilts down. Pushing up looks up (unless Invert
    // camera Y is on), the opposite of the orbit, where up raises the camera.
    const float step = -vertical * k_blaster_look_speed_degrees * game_delta(rdram);
    store_float(rdram, state + 0x20C, load_float(rdram, state + 0x20C) + step);
    const float limit = std::max(std::fabs(load_float(rdram, state + 0x1E8)), std::fabs(load_float(rdram, state + 0x1EC)));
    if (std::isfinite(limit)) blaster_look_offset = std::clamp(blaster_look_offset + step, -limit, limit);
}

extern "C" void tooie_blaster_look_target(std::uint8_t* rdram, recomp_context* ctx, int control_type) {
    (void)rdram;
    if (blaster_look_offset == 0.0f || !tooie::camera::analog_enabled()) return;
    // Type 1's target is in f2 (func_80806928); type 0's level angle in f0 (func_808069BC).
    if (control_type == 1) ctx->f2.fl += blaster_look_offset;
    else ctx->f0.fl += blaster_look_offset;
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
    // Flight has none (C-left/C-right do nothing there) but orbits anyway.
    const auto mode = static_cast<std::uint32_t>(MEM_W(component, 0x6C));
    const auto mode_rule_address = static_cast<gpr>(static_cast<std::int32_t>(
        k_camera_mode_rule_table + mode * 16u));
    if (MEM_H(0, mode_rule_address) == 0 && static_cast<std::int32_t>(mode) != k_fly_mode) {
        return -1;
    }
    const gpr player = ctx->r16; // func_800A4878's arg0, kept in s0.

    // Manual rotation is allowed this frame. The stick itself is read by the
    // orbit hook, once per camera frame.
    ++orbit.input_frames;

    // Tooie's heap relocates the camera component; the same player in the
    // same mode means the same camera.
    if ((orbit.active || orbit.entering) && orbit.player != player) exit_orbit();
    if (orbit.active || orbit.entering) orbit.component = component;
    if (orbit.active && orbit.mode != static_cast<std::int32_t>(mode)) {
        exit_orbit();
    }
    float horizontal = 0.0f, vertical = 0.0f;
    read_stick(horizontal, vertical);
    if (!orbit.active && !orbit.entering && std::hypot(horizontal, vertical) >= k_enter_threshold) {
        orbit.entering = true;
        orbit.component = component;
        orbit.player = player;
        orbit.mode = static_cast<std::int32_t>(mode);
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
    if (!(orbit.active || orbit.entering)) {
        orbit.seen_input_frames = orbit.input_frames;
        return;
    }
    // Another player's camera (Mumbo beside his statue, the other half of a
    // split-up), or this one relocated before the input hook has seen it:
    // only the input hook decides which component is the orbit's.
    if (component != orbit.component) return;
    const bool input_this_frame = orbit.input_frames != orbit.seen_input_frames;
    orbit.seen_input_frames = orbit.input_frames;

    const gpr yaw_controller = static_cast<gpr>(MEM_W(component, 0x60));
    if (!rdram_pointer(yaw_controller)) { exit_orbit(); return; }
    // Only the player's own turns (C-left/C-right, R) hand the camera back.
    const bool player_turn = MEM_H(0x0C, yaw_controller) != 0 &&
        MEM_H(0x0E, yaw_controller) >= k_player_turn_priority;
    const float dt = game_delta(rdram);
    orbit.input_gap_seconds = input_this_frame ? 0.0f : orbit.input_gap_seconds + dt;
    const bool input_lost = orbit.entering ? !input_this_frame : orbit.input_gap_seconds > k_input_gap_seconds;
    if (input_lost || player_turn || MEM_W(component, 0x6C) != orbit.mode ||
        !tooie::camera::analog_enabled()) {
        // The game owns the camera again (C buttons, R, view toggle, mode
        // change, cutscene or dialogue). Its state already holds the orbit's
        // last pose.
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

    float horizontal = 0.0f, vertical = 0.0f;
    read_stick(horizontal, vertical);
    if (orbit.mode == k_fly_mode && !orbit.entering) {
        const bool held = horizontal != 0.0f || vertical != 0.0f;
        orbit.released_seconds = held ? 0.0f : orbit.released_seconds + dt;
        if (orbit.released_seconds >= k_fly_release_seconds) { exit_orbit(); return; }
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
        orbit.released_seconds = orbit.input_gap_seconds = 0.0f;
        orbit.entering = false;
        orbit.active = true;
    } else {
        // Lighthouse SmoothTowards on the aim height keeps Banjo's bob out of the view.
        orbit.aim_y += (focus[1] - orbit.aim_y) * (1.0f - std::exp(-k_aim_height_rate * dt));
    }

    // Lighthouse orbits at the game's zoom distance, not the live one. Tooie's
    // follow camera keeps anywhere inside the mode's range (func_80115358
    // clamps to +0xC..+0x10 of the block at +0x14, set per view by
    // func_800A3BA8 and scaled up for the statue) and trails at its far end,
    // so that is the orbit's distance, times the distance setting. The live
    // distance it was entered at (often the near end, or pulled in by a wall)
    // eases out to it.
    const gpr range = static_cast<gpr>(MEM_W(component, 0x14));
    if (rdram_pointer(range)) {
        const float near_limit = load_float(rdram, range + 0xC), far_limit = load_float(rdram, range + 0x10);
        if (std::isfinite(near_limit) && std::isfinite(far_limit) && near_limit > 1.0f && near_limit <= far_limit) {
            const float wanted = std::max(k_min_distance,
                far_limit * configured_distance_scale.load(std::memory_order_acquire));
            orbit.distance += (wanted - orbit.distance) * (1.0f - std::exp(-k_distance_rate * dt));
        }
    }

    orbit.yaw = wrap_degrees(orbit.yaw + horizontal * k_yaw_speed_degrees * dt);
    // Captured outside the limits (the game's own angle): only move inwards.
    orbit.elevation = std::clamp(orbit.elevation + vertical * k_pitch_speed_degrees * dt,
        std::min(k_min_elevation, orbit.elevation), std::max(k_max_elevation, orbit.elevation));

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
    // The follow camera's wall avoidance would keep swinging the yaw, and its
    // transition would outlive the orbit; the orbit has its own collision.
    // Clearing kind and priority ends it, as func_80112A84 does on arrival.
    MEM_H(0x0C, yaw_controller) = 0;
    MEM_H(0x0E, yaw_controller) = 0;
}
