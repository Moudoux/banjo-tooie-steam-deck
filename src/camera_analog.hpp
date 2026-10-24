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

// Called after Tooie's original bastick_getX/Y calls, inside bafpctrl's own
// active-mode gate. A zero/disabled right axis preserves the original value.
extern "C" float tooie_camera_first_person_axis(float original_value, int vertical) noexcept;
