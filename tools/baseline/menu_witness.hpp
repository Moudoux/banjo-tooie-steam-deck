#ifndef TOOIE_MENU_WITNESS_HPP
#define TOOIE_MENU_WITNESS_HPP
#include <array>
#include <cstdint>
namespace tooie_menu_witness {
// CPU guest-thread cumulative counters. The F4 frame ring copies these values; render thread never reads them.
// New counters are appended before Count so the serialized order of the earlier ones is unchanged.
enum Counter : uint32_t { Reset, Init, InitRefused, Free, Enter, EnterUnregistered,
    EnterInvocationLimit, EnterInvalid, Box, BoxInvalid, ModelScope, ModelGateRefused,
    ModelCall, IdentityRefused, ResourceAbsent, TokenRefused, TokenAccepted,
    ModelEnd, RootGroupDelta,
    ModelOutsideOption,   // 80801424 box model outside any option box occurrence (other zoomboxes)
    TransitionModel, TransitionTagged, TransitionCut, TransitionRefused,
    Count };
struct Snapshot {
    std::array<uint32_t,Count> counts{};
    uint32_t bk_frame{}, owner{}, gfx{}, box{}, slot{}, role{}, id{}, resource{}, token{},
        live_slots{}, active_valid{}, gate{}, group_delta{}, root_delta{}, graph_group_delta{}, init_owner{}, free_owner{};
    // The two pointers each gate compares: 80801424's s0/a0 at 8080148C and fxdialog3's a0 at 80800174.
    uint32_t model_box{}, model_gfx{}, dialog_gfx{};
    std::array<uint32_t,3> registered{};
};
inline Snapshot current{};
inline void hit(Counter c) { if(current.counts[c]!=UINT32_MAX)++current.counts[c]; }
}
#endif
