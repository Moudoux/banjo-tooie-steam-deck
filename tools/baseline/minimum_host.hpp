#pragma once
#include "frontend_input_preference.hpp"
#include "input.hpp"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace baseline::host {
// Pillarbox capability is latched before the guest starts. Widescreen, cutscene
// aspect and other preferences apply at their owner's next safe boundary and
// persist for the next process.
inline constexpr int fit_aspect = 4;
struct Preferences {
    int resolution = 1, output_rate = 0, manual_hz = 60;
    int texture_filter = 0, output_filter = 0; // Current RT64 defaults: three-point and pixel scaling.
    int wide = -1, native_aspect = 0, cutscene_aspect = 0;
    int vsync = -1; // Missing preserves the backend's current value.
    int fullscreen = 0;
    int quiet_talon_trot = 0;
    int talon_trot_toggle = 0;
    int first_person_aim = 0;
    int first_person_aim_sensitivity = 100;
    int first_person_aim_invert = 0; // Bit0 X, bit1 Y.
    int master_volume = 100;
    int music_volume = 100;
    int rumble_strength = 100;
    int analog_camera = 0, camera_invert_x = 0, camera_invert_y = 0;
    int camera_distance = 100; // Orbit distance, percent of the game's camera distance (50..150).
    int msaa = 0, supersampling = 1; // MSAA samples 0/2/4/8; supersampling 1/2/4.
    int draw_distance = 1, disable_lod = 0; // Model draw-distance multiplier 1..12.
    int mouse_aim = 1, mouse_sensitivity = 6, mouse_invert_y = 1; // Hundredths of a degree per count.
    int mouse_aim_flying = 1, mouse_aim_swimming = 1, mouse_aim_saucer = 1, mouse_aim_submarine = 1; // Also in flight, underwater, the Saucer of Peril, the submarine.
    int start_skips_cutscenes = 1;
    // RT64 picture options. Color format (0 Standard, 1 High, 2 Automatic) applies at
    // the next launch; the others apply live. Defaults keep the earlier output.
    int color_format = 0;
    int upscale_2d = 1; // 0 Original, 1 scaled rects only, 2 all.
    int texture_lod = 1;
    int dither_noise = 100; // Percent of RT64's high-resolution dither noise.
    int minimap = 1; // Level map in the bottom-left corner.
    int levitate = 0; // Hold N64 L to rise (Lighthouse cheat).
    int autosave_minutes = 5; // Minutes of play between autosaves; 0 turns autosave off.
};
enum class Preference { Resolution, OutputRate, ManualHz, Wide, NativeAspect, CutsceneAspect, VSync, Fullscreen, QuietTalonTrot, TalonTrotToggle, FirstPersonAim, FirstPersonAimSensitivity, FirstPersonAimInvert, TextureFilter, OutputFilter, MusicVolume, RumbleStrength, MasterVolume, AnalogCamera, CameraInvertX, CameraInvertY, Msaa, Supersampling, DrawDistance, DisableLod, MouseAim, MouseSensitivity, MouseInvertY, CameraDistance, StartSkipsCutscenes, ColorFormat, Upscale2D, TextureLod, DitherNoise, Minimap, Levitate, AutosaveMinutes, MouseAimFlying, MouseAimSwimming, MouseAimSaucer, MouseAimSubmarine };
struct OutputRate { int option = 0, manual_hz = 60; };
inline uint32_t pack_output_rate(OutputRate rate) noexcept {
    return (uint32_t(rate.option) << 16) | uint32_t(rate.manual_hz);
}
inline OutputRate unpack_output_rate(uint32_t rate) noexcept {
    return {int(rate >> 16), int(rate & 0xffffu)};
}
// Types whose legends match SDL's normalized controls here. Other keeps generic SDL names.
inline bool offered_controller_labels(tooie::input::ControllerType type) noexcept {
    using T = tooie::input::ControllerType;
    return type == T::Other || type == T::Xbox || type == T::SteamDeck ||
        type == T::DualSense || type == T::DualShock || type == T::Steam || type == T::NintendoPro;
}
inline std::string controller_binding_label(tooie::input::Binding binding, tooie::input::ControllerType type) {
    return offered_controller_labels(type) && type != tooie::input::ControllerType::Other
        ? tooie::input::binding_label_for_type(binding, type) : tooie::input::binding_label(binding);
}
struct Picture { int upscale_2d = 1, texture_lod = 1, dither_noise = 100; };
inline uint32_t pack_picture(Picture picture) noexcept {
    return uint32_t(picture.upscale_2d) | (uint32_t(picture.texture_lod) << 3) | (uint32_t(picture.dither_noise) << 8);
}
inline Picture unpack_picture(uint32_t picture) noexcept {
    return {int(picture & 3u), int((picture >> 3) & 1u), int(picture >> 8)};
}
enum class RequestKind { Controller, Profile, ResetBindings, Capture, CancelCapture, Preference, ClearBinding, Modifier, DuplicateProfile, ControllerLabels, CreateDirectCProfile };
struct Request {
    RequestKind kind{};
    int value = 0;
    tooie::input::Action action = tooie::input::Action::A;
    tooie::input::Device device = tooie::input::Device::Keyboard;
    unsigned slot = 0;
    tooie::input::BindingLayer layer = tooie::input::BindingLayer::Base;
    Preference preference{};
    std::string name;
    std::optional<SDL_JoystickID> controller; // Selected pad shown when this controller-scoped request was queued.
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
    bool controller_auto = false;
    tooie::input::ControllerType controller_type = tooie::input::ControllerType::Other;
    std::vector<std::string> profiles;
    std::string active_profile, notice, toast;
    std::array<BindingRow, unsigned(tooie::input::Action::Menu) + 1> bindings;
    BindingRow fast_forward; // Explicit utility row; hidden actions are not exposed.
    BindingRow held_menu; // Modified-layer Menu; controller cells only.
    tooie::input::Binding modifier{tooie::input::Device::Button, -1};
    tooie::input::ModifierSoloBehavior modifier_solo = tooie::input::ModifierSoloBehavior::PassThrough;
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
    bool physical_admitted = false; // Mapper admission, distinct from actual window focus.
    uint32_t aim_sample = 0, aim_policy = 0; // One admitted publication; zero when unavailable.
};
// SDL owner only, except the explicitly identified render-thread methods.
void initialize(SDL_Window*, const std::filesystem::path& profile, bool allow_background_controller = false);
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
int requested_texture_filter() noexcept; // Graphics owner; 0 three-point, 1 bilinear.
int requested_output_filter() noexcept; // Graphics owner; 0 pixel scaling, 1 nearest, 2 linear.
int requested_cutscene_aspect() noexcept; // Guest frame/cutscene boundary.
int requested_msaa() noexcept; // Graphics owner; MSAA samples 0, 2, 4 or 8.
int requested_supersampling() noexcept; // Graphics owner; 1, 2 or 4.
bool requested_quiet_talon_trot() noexcept; // Guest sound-call boundary.
bool requested_talon_trot_toggle() noexcept; // Guest input/state boundary.
int requested_music_volume() noexcept; // Requested percent; guest music-update boundary.
int requested_master_volume() noexcept; // Requested percent; audio sink chunk boundary.
bool requested_start_skips_cutscenes() noexcept; // Intro ticker raw hook.
// Graphics owner only: consume the SDL owner's atomic requested pair.
OutputRate requested_output_rate() noexcept;
Picture requested_picture() noexcept; // Graphics owner.
// Audio sink only: in-place master gain for one queued S16 chunk. 100 or more is
// exact identity, so the default leaves samples bit-identical; lower values
// truncate toward zero and never exceed the original magnitude.
inline void apply_master_volume(int16_t* samples, size_t count, int percent) noexcept {
    if (percent >= 100) return;
    const int32_t gain = percent > 0 ? percent : 0;
    for (size_t i = 0; i < count; ++i)
        samples[i] = int16_t(int32_t(samples[i]) * gain / 100);
}
}
