#pragma once
#include <atomic>
#include <cstdint>

// F6 baseline player: the selected timer translation unit owns the applied guest
// clock on steady_clock. Where high_resolution_clock is steady_clock (pinned
// MSVC), VI stays on the raw upstream path until the first accelerated rate is
// applied; elsewhere the guest clock owns VI from the start.
namespace tooie::guest_clock {
bool owns_vi() noexcept;
struct Applied { std::uint32_t rate, revision; };
Applied applied() noexcept;
// Sleeps to the applied map's steady deadline for total_vis and returns the
// next VI count in the upstream form.
std::uint64_t wait_vi(std::uint64_t total_vis, const std::atomic_bool& exited);
}
