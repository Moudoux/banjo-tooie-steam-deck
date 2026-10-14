#pragma once
#include <filesystem>
#include "ultramodern/ultramodern.hpp"

// Private carrier boundary: one fixed device session, no product host linkage.
namespace baseline {
enum class RenderMode { Original, Display };
ultramodern::renderer::WindowHandle open_devices(const std::filesystem::path& root,
    ultramodern::renderer::GraphicsApi api, unsigned seconds, RenderMode render_mode);
void close_devices();
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
}
