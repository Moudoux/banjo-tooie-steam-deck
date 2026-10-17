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
};
enum class Preference { Resolution, OutputRate, ManualHz, Wide, NativeAspect, CutsceneAspect, VSync, Fullscreen, QuietTalonTrot };
struct OutputRate { int option = 0, manual_hz = 60; };
inline uint32_t pack_output_rate(OutputRate rate) noexcept {
    return (uint32_t(rate.option) << 16) | uint32_t(rate.manual_hz);
}
inline OutputRate unpack_output_rate(uint32_t rate) noexcept {
    return {int(rate >> 16), int(rate & 0xffffu)};
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
int requested_resolution() noexcept; // Graphics owner; range 1..8.
bool requested_quiet_talon_trot() noexcept; // Guest sound-call boundary.
// Graphics owner only: consume the SDL owner's atomic requested pair.
OutputRate requested_output_rate() noexcept;
}
