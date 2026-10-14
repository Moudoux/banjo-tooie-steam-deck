#pragma once
#include "frontend_input_preference.hpp"

namespace tooie::input {
// Each executable links one real host adapter. SDL devices remain in the mapper.
struct ProductActions {
    void (*dispatch)(Action);
    void (*set_fast_forward_rate)(std::uint32_t) noexcept;
    void (*set_fast_forward_held)(bool) noexcept;
    bool (*free_camera_enabled)() noexcept;
    void (*set_free_camera_input_active)(bool) noexcept;
    void (*set_free_camera_motion)(float, float, float) noexcept;
    void (*request_free_camera_reset)() noexcept;
};
struct Host {
    bool baseline;
    bool (*menu_open)() noexcept;
    void (*set_menu_open)(bool) noexcept;
    bool (*game_started)();
    const ProductActions* product; // Absent when the host does not offer product utilities.
};
const Host& host() noexcept;
}
