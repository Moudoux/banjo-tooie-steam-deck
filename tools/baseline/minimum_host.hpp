#pragma once
#include "frontend_input_preference.hpp"
#include "input.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace baseline::host {
// Aspect is latched before the guest starts. Other player preferences apply
// at their owner's next safe boundary and persist for the next process.
struct Preferences {
    int resolution = 1, output_rate = 0, manual_hz = 60;
    int wide = -1, native_aspect = 0, cutscene_aspect = 0;
    int vsync = -1; // Missing preserves the backend's current value.
    int fullscreen = 0;
    int quiet_talon_trot = 0;
    int analog_camera = 0, camera_invert_x = 0, camera_invert_y = 0;
    int camera_distance = 100; // Orbit distance, percent of the game's camera distance (50..150).
    int msaa = 0, supersampling = 1; // MSAA samples 0/2/4/8; supersampling 1/2/4.
    int draw_distance = 1, disable_lod = 0; // Model draw-distance multiplier 1..12.
    int mouse_aim = 1, mouse_sensitivity = 6, mouse_invert_y = 1; // Hundredths of a degree per count.
    int rumble_strength = 100; // Percent; 0 turns rumble off.
    int start_skips_cutscenes = 1;
    // RT64 picture options. Color format (0 Standard, 1 High, 2 Automatic) applies at
    // the next launch; the others apply live. Defaults keep the earlier output.
    int color_format = 0;
    int upscale_2d = 1; // 0 Original, 1 scaled rects only, 2 all.
    int three_point_filtering = 1, texture_lod = 1;
    int dither_noise = 100; // Percent of RT64's high-resolution dither noise.
    int object_interpolation = 1; // Matrix groups for every actor and the player.
};
enum class Preference { Resolution, OutputRate, ManualHz, Wide, NativeAspect, CutsceneAspect, VSync, Fullscreen, QuietTalonTrot, AnalogCamera, CameraInvertX, CameraInvertY, CameraDistance, Msaa, Supersampling, DrawDistance, DisableLod, MouseAim, MouseSensitivity, MouseInvertY, RumbleStrength, StartSkipsCutscenes, ColorFormat, Upscale2D, ThreePointFiltering, TextureLod, DitherNoise, ObjectInterpolation };
struct OutputRate { int option = 0, manual_hz = 60; };
inline uint32_t pack_output_rate(OutputRate rate) noexcept {
    return (uint32_t(rate.option) << 16) | uint32_t(rate.manual_hz);
}
inline OutputRate unpack_output_rate(uint32_t rate) noexcept {
    return {int(rate >> 16), int(rate & 0xffffu)};
}
struct Picture { int upscale_2d = 1, three_point_filtering = 1, texture_lod = 1, dither_noise = 100; };
inline uint32_t pack_picture(Picture picture) noexcept {
    return uint32_t(picture.upscale_2d) | (uint32_t(picture.three_point_filtering) << 2) |
        (uint32_t(picture.texture_lod) << 3) | (uint32_t(picture.dither_noise) << 8);
}
inline Picture unpack_picture(uint32_t picture) noexcept {
    return {int(picture & 3u), int((picture >> 2) & 1u), int((picture >> 3) & 1u), int(picture >> 8)};
}
enum class RequestKind { Controller, Profile, ResetBindings, Capture, CancelCapture, Preference };
struct Request {
    RequestKind kind{};
    int value = 0;
    tooie::input::Action action = tooie::input::Action::A;
    tooie::input::Device device = tooie::input::Device::Keyboard;
    unsigned slot = 0;
    Preference preference{};
    std::string name;
};
struct BindingRow {
    std::string action;
    std::array<std::string, 2> keyboard;
    std::array<std::string, 6> controller; // Button, positive axis, negative axis; two slots each.
};
struct Snapshot {
    int width = 0, height = 0, pixel_width = 0, pixel_height = 0;
    bool focused = false, minimized = false, menu = false, suppressed = false;
    tooie::input::MenuGamepadState gamepad;
    tooie::input::CaptureStatus capture;
    std::vector<tooie::input::ControllerInfo> controllers;
    std::vector<std::string> profiles;
    std::string active_profile, notice, toast;
    std::array<BindingRow, 19> bindings;
    Preferences preferences;
    int latched_color_format = 0; // The renderer's color format until the next launch.
};
struct RenderInput {
    std::shared_ptr<const Snapshot> snapshot;
    std::deque<SDL_Event> events;
    bool reset_input = false;
};
struct InputPublication {
    input::State physical;
    bool activity = false, focused = false, quit = false;
};
// SDL owner only, except the explicitly identified render-thread methods.
void initialize(SDL_Window*, const std::filesystem::path& profile);
void shutdown() noexcept;
void handle_event(const SDL_Event&);
InputPublication tick(bool gameplay);
const Preferences& latched_preferences() noexcept;
void note_game_started() noexcept;
bool menu_open() noexcept;
// Render thread: queue mutations; sticky actions survive a full queue.
bool enqueue(Request);
void request_close() noexcept;
void request_quit() noexcept;
void publish_ready(bool) noexcept;
RenderInput take_render_input();
int requested_vsync() noexcept;
int requested_resolution() noexcept; // Graphics owner; 0 fits the window, else 1..8.
int requested_msaa() noexcept; // Graphics owner; MSAA samples 0, 2, 4 or 8.
int requested_supersampling() noexcept; // Graphics owner; 1, 2 or 4.
bool requested_quiet_talon_trot() noexcept; // Guest sound-call boundary.
bool requested_start_skips_cutscenes() noexcept; // Intro ticker raw hook.
// Graphics owner only: consume the SDL owner's atomic requested pair.
OutputRate requested_output_rate() noexcept;
Picture requested_picture() noexcept; // Graphics owner.
}
