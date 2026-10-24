#pragma once

#include <cstdint>

namespace tooie::draw_distance {

enum class ActorDistance : std::uint32_t {
    Original = 0,
    Extended2x = 1,
};

// Restart-latched with other game graphics features. This affects only the
// common actor model fade/cull distance selected by func_80101970.
void configure_actor_distance(ActorDistance value) noexcept;
void latch_for_game_start() noexcept;
ActorDistance configured_actor_distance() noexcept;
ActorDistance latched_actor_distance() noexcept;
float adjust_actor_distance(float original_distance) noexcept;

// Player: live model draw-distance multiplier (1 = original, up to 12; Lighthouse
// stops at 6) and level-of-detail override, read by raw hooks in the model
// renderer (func_800DE498) and its LOD geo commands.
void set_model_distance_scale(int multiplier) noexcept;
int model_distance_scale() noexcept;
void set_lod_disabled(bool disabled) noexcept;
bool lod_disabled() noexcept;

} // namespace tooie::draw_distance

extern "C" float tooie_actor_draw_distance_adjust(float original_distance);
extern "C" float tooie_model_draw_distance_scale(void) noexcept;
extern "C" int tooie_model_lod_disabled(void) noexcept;
