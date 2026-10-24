#include "levitate.hpp"

#include <atomic>
#include <bit>

#include "funcs.h"

namespace {
// Lighthouse's LEVITATE_VELOCITY, in the same units (Tooie keeps BK's physics).
constexpr float k_velocity = 500.0f;
constexpr std::int32_t k_button_l = 2; // ButtonId BUTTON_L.
// baphysics_set_gravity / baphysics_set_vertical_velocity write these fields
// of the physics struct at PlayerState +0xC8; baphysics_reset_gravity sets
// the default gravity stored at D_80117EF0.
constexpr std::uint32_t k_physics = 0xC8U, k_vertical_velocity = 0x14U, k_gravity = 0x34U;
constexpr std::uint32_t k_default_gravity = 0x80117EF0U;
// Controlled character: index and PlayerState table (src/minimap.cpp).
constexpr std::uint32_t k_controlled = 0x801354DCU, k_players = 0x80135490U;

std::atomic<bool> on{false};
// Guest thread only: the player whose gravity levitation turned off.
gpr active = 0;

gpr guest(std::uint32_t address) noexcept { return static_cast<gpr>(static_cast<std::int32_t>(address)); }
void store_float(std::uint8_t* rdram, gpr address, float value) {
    MEM_W(0, address) = std::bit_cast<std::int32_t>(value);
}
// As camera_analog.cpp: callee frames go below the hook's stack pointer and
// the whole context is restored, so the hooked function continues unchanged.
template <typename Call>
void guest_call(recomp_context* ctx, Call&& call) {
    const auto saved = *ctx;
    try {
        ctx->r29 -= 0x60;
        call();
        *ctx = saved;
    } catch (...) {
        *ctx = saved;
        throw;
    }
}
void restore_gravity(std::uint8_t* rdram) {
    const gpr physics = static_cast<gpr>(MEM_W(k_physics, active));
    MEM_W(k_gravity, physics) = MEM_W(0, guest(k_default_gravity));
    active = 0;
}
}

namespace tooie::levitate {

void set_enabled(bool enabled) noexcept { on.store(enabled, std::memory_order_release); }
bool enabled() noexcept { return on.load(std::memory_order_acquire); }

} // namespace tooie::levitate

extern "C" void tooie_levitate_update(std::uint8_t* rdram, recomp_context* ctx) {
    // bainput_update can run once per character; only the controlled one levitates.
    const auto controlled = static_cast<std::uint32_t>(MEM_W(0, guest(k_controlled)));
    if (controlled >= 2U) return;
    const gpr player = ctx->r4;
    if (player != static_cast<gpr>(MEM_W(0, guest(k_players + controlled * 4U)))) return;
    if (active && active != player) active = 0; // A new PlayerState starts with its own gravity.
    int held = 0;
    if (tooie::levitate::enabled()) guest_call(ctx, [&] {
        // bakey_held honours the game's per-button suppression latch.
        ctx->r4 = player; ctx->r5 = k_button_l;
        bakey_held(rdram, ctx);
        held = static_cast<int>(ctx->r2);
    });
    if (held) {
        const gpr physics = static_cast<gpr>(MEM_W(k_physics, player));
        store_float(rdram, physics + k_gravity, 0.0f);
        store_float(rdram, physics + k_vertical_velocity, k_velocity);
        // bafalldamage_start (as Flap Flip each frame): the fall is measured
        // from here, so climbing does not add fall damage.
        guest_call(ctx, [&] { ctx->r4 = player; func_800956B8(rdram, ctx); });
        active = player;
    } else if (active) {
        // Only once on release (or when turned off), as Lighthouse.
        restore_gravity(rdram);
    }
}
