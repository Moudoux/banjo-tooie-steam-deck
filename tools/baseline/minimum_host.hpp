#pragma once
#include "frontend_input_preference.hpp"
#include "input.hpp"
#include <array>
#include <atomic>
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
    int music_volume = 100;
    int rumble_strength = 100;
};
enum class Preference { Resolution, OutputRate, ManualHz, Wide, NativeAspect, CutsceneAspect, VSync, Fullscreen, QuietTalonTrot, TalonTrotToggle, FirstPersonAim, FirstPersonAimSensitivity, FirstPersonAimInvert, TextureFilter, OutputFilter, MusicVolume, RumbleStrength };
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
int requested_resolution() noexcept; // Graphics owner; range 1..8.
int requested_texture_filter() noexcept; // Graphics owner; 0 three-point, 1 bilinear.
int requested_output_filter() noexcept; // Graphics owner; 0 pixel scaling, 1 nearest, 2 linear.
int requested_cutscene_aspect() noexcept; // Guest frame/cutscene boundary.
bool requested_quiet_talon_trot() noexcept; // Guest sound-call boundary.
bool requested_talon_trot_toggle() noexcept; // Guest input/state boundary.
int requested_music_volume() noexcept; // Requested percent; guest music-update boundary.
// Graphics owner only: consume the SDL owner's atomic requested pair.
OutputRate requested_output_rate() noexcept;
}
