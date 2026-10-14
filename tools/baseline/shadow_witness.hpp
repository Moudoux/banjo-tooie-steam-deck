#pragma once
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <string_view>
// Diagnostic arm only; the guest owner and parser retain their own finite counts.
namespace shadow_witness {
inline std::atomic<bool> armed{false};
inline std::atomic<bool> forward_ready{false};
inline bool forward_mode() {
    static const bool enabled=[] { const char* p=std::getenv("TOOIE_BASELINE_SHADOW_FORWARD");return p && std::string_view(p)=="1"; }();
    return enabled;
}
inline uint32_t lines=0; // Written only by the existing guest construction owner.
}
