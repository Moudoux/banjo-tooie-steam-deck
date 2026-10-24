#include "draw_distance.hpp"

#include <atomic>
#include <cmath>

namespace {
std::atomic<tooie::draw_distance::ActorDistance> configured{
    tooie::draw_distance::ActorDistance::Original};
std::atomic<tooie::draw_distance::ActorDistance> latched{
    tooie::draw_distance::ActorDistance::Original};
std::atomic<int> model_scale{1};
std::atomic<bool> lod_off{false};

constexpr tooie::draw_distance::ActorDistance normalize(
    tooie::draw_distance::ActorDistance value) noexcept {
    return value == tooie::draw_distance::ActorDistance::Extended2x
        ? value : tooie::draw_distance::ActorDistance::Original;
}
}

namespace tooie::draw_distance {

void configure_actor_distance(ActorDistance value) noexcept {
    configured.store(normalize(value), std::memory_order_release);
}

void latch_for_game_start() noexcept {
    latched.store(configured.load(std::memory_order_acquire), std::memory_order_release);
}

ActorDistance configured_actor_distance() noexcept {
    return configured.load(std::memory_order_acquire);
}

ActorDistance latched_actor_distance() noexcept {
    return latched.load(std::memory_order_acquire);
}

float adjust_actor_distance(float original_distance) noexcept {
    if (latched_actor_distance() != ActorDistance::Extended2x ||
        !std::isfinite(original_distance) || original_distance <= 0.0f)
        return original_distance;
    return original_distance * 2.0f;
}

void set_model_distance_scale(int multiplier) noexcept {
    model_scale.store(multiplier < 1 ? 1 : multiplier > 6 ? 6 : multiplier, std::memory_order_release);
}

int model_distance_scale() noexcept {
    return model_scale.load(std::memory_order_acquire);
}

void set_lod_disabled(bool disabled) noexcept {
    lod_off.store(disabled, std::memory_order_release);
}

bool lod_disabled() noexcept {
    return lod_off.load(std::memory_order_acquire);
}

} // namespace tooie::draw_distance

extern "C" float tooie_actor_draw_distance_adjust(float original_distance) {
    return tooie::draw_distance::adjust_actor_distance(original_distance);
}

extern "C" float tooie_model_draw_distance_scale(void) noexcept {
    const int multiplier = tooie::draw_distance::model_distance_scale();
    // Lighthouse's port_drawDistanceMul: a small nudge past whole multiples.
    return multiplier <= 1 ? 1.0f : float(multiplier) + 0.1f;
}

extern "C" int tooie_model_lod_disabled(void) noexcept {
    return tooie::draw_distance::lod_disabled() ? 1 : 0;
}
