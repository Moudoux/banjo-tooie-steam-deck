#pragma once

#include <cstdint>
#include "recomp.h"

namespace tooie::camera {
void set_analog_enabled(bool enabled) noexcept;
bool analog_enabled() noexcept;
void set_horizontal_inverted(bool inverted) noexcept;
bool horizontal_inverted() noexcept;
void set_vertical_inverted(bool inverted) noexcept;
bool vertical_inverted() noexcept;
// First-person mouse aim (and Steam Input gyro-as-mouse): degrees per count, Y inversion.
void set_mouse_aim_sensitivity(float degrees_per_count) noexcept;
void set_mouse_aim_inverted(bool inverted) noexcept;
// Where else the mouse aims: first-person flight, swimming and submarine, and
// the Saucer of Peril ride's crosshair.
void set_mouse_aim_flying(bool enabled) noexcept;
void set_mouse_aim_swimming(bool enabled) noexcept;
void set_mouse_aim_saucer(bool enabled) noexcept;
void set_mouse_aim_submarine(bool enabled) noexcept;
// Right-stick orbit distance as a share of the game's own (far) camera distance.
void set_orbit_distance_scale(float scale) noexcept;
}

// Called only from the pinned, already-gated camera path in func_800A4878.
// It may propagate the runtime's cooperative thread-termination exception.
// Returns -1 when the original C-left/C-right path must run. Otherwise returns
// the original downstream command result after consuming this analog frame.
extern "C" int tooie_camera_analog_apply(std::uint8_t* rdram, recomp_context* ctx);

// Called in func_80110588 after the camera mode's update and before the pose
// is copied for the view (0x8011061C): while the right-stick orbit is active,
// it places the camera, with the game's own collision.
extern "C" void tooie_camera_orbit_apply(std::uint8_t* rdram, recomp_context* ctx);

// Called in func_808002A4_bafpctrl before the pitch clamp (0x80800368), only while
// first-person control is active: adds the mouse turn to pitch (f12) and yaw (f6).
extern "C" void tooie_first_person_aim(std::uint8_t* rdram, recomp_context* ctx);

// First-person egg aim (func_80800264_baeggaim): the reticle follows the stick
// and the view turns when it nears the edge. The pitch hook (before 0x808002E8,
// only when pitch is unlocked, before the clamp) adds the mouse pitch to sp+0x24;
// the yaw hook (L_80800300, every egg-aim frame) adds the mouse yaw to sp+0x28.
extern "C" void tooie_egg_aim_pitch(std::uint8_t* rdram, recomp_context* ctx);
extern "C" void tooie_egg_aim_yaw(std::uint8_t* rdram, recomp_context* ctx);

// First-person flight and swimming (bafpctrl body mode, state +0x11): the view
// follows the player's own pitch and yaw, which the steering sets from the
// stick (bafly_entrypoint_9/10, baswim_entrypoint_11/12, the submarine's
// func_80800634/func_80800768_bssub). The yaw hook sits
// before yaw_setIdeal (a1 = ideal yaw) and adds the mouse turn; the pitch hook
// sits before func_8009BF5C (a1 = ideal pitch, 0..360) and adds a held mouse
// pitch, kept inside the steering's range [low, high] (signed degrees).
// `context` picks the setting (0 flight, 1 swimming, 2 submarine); `player`
// is the PlayerState.
extern "C" void tooie_body_aim_yaw(std::uint8_t* rdram, recomp_context* ctx, int context, gpr player);
extern "C" void tooie_body_aim_pitch(std::uint8_t* rdram, recomp_context* ctx, int context, gpr player,
    float low, float high);
// The first-person camera eases its shown rotation (+0xC) to the target
// (+0x24) with turn rates capped at 120 (pitch) and 300 (yaw) degrees per
// second unless a state changes them (func_808003DC_ncba1p, before
// func_80111410 at 0x8080042C; camera data in s0). This hook first moves it
// by the mouse's share from every mouse-aim hook (egg aim, first-person look,
// body mode), so mouse pitch is as direct as mouse yaw.
extern "C" void tooie_body_aim_view(std::uint8_t* rdram, recomp_context* ctx);

// Saucer of Peril (map 0x124): the ride holds Banjo in egg aim with the camera
// on rails, so the mouse moves the crosshair. Before func_808000B4_baeggaim
// eases the cursor (s0) towards the stick's target (x in f14, y at sp+0x20,
// 0x8080012C), a held mouse offset moves both the target and the cursor.
extern "C" void tooie_saucer_aim(std::uint8_t* rdram, recomp_context* ctx);

// Called after Tooie's original bastick_getX/Y calls, inside bafpctrl's own
// active-mode gate. A zero/disabled right axis preserves the original value.
extern "C" float tooie_camera_first_person_axis(float original_value, int vertical) noexcept;
