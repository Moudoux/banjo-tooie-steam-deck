#pragma once
#include <filesystem>
#include "ultramodern/ultramodern.hpp"

// Device session for the baseline engine; host/player builds also drive the minimum host
// and texture-pack registration.
namespace baseline {
enum class RenderMode { Original, Display };
ultramodern::renderer::WindowHandle open_devices(const std::filesystem::path& root,
    ultramodern::renderer::GraphicsApi api, unsigned seconds, RenderMode render_mode);
void close_devices();
#ifdef BASELINE_PLAYER
void register_texture_packs();
#endif
void emit_audio_summary();
void load_input_script(const std::filesystem::path& path);
ultramodern::renderer::callbacks_t renderer_callbacks();
ultramodern::audio_callbacks_t audio_callbacks();
ultramodern::input::callbacks_t input_callbacks();
void update_gfx(void*);
void vi_complete();
void expect_first_graphics(uint32_t start, uint32_t size);
bool first_graphics_processed();
bool quit_was_requested();
#ifdef BASELINE_TERMINAL_FENCE
// The runtime error owner admitted a first report whose handler has not returned.
bool runtime_error_report_pending() noexcept;
#endif
}
