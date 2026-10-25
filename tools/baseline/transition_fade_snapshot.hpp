#pragma once
#include <array>
#include <cstdint>
namespace tooie_transition_fade {
// Last exact transition fade attempt per BK construction; CPU evidence, never consumed/pixel proof.
struct Snapshot {
    uint32_t valid{},frame{},view{},site{},object{},kind{},flags{},elapsed_word{},duration_word{},
        gfx{},begin{},end{},attempts{},refused{},mixed_views{};
    std::array<uint32_t,10> commands{};
};
struct Native { uint8_t* ram{}; Snapshot value{}; };
inline Native current{};
}
