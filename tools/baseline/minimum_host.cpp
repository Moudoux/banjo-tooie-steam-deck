#include "minimum_host.hpp"
#include "frontend_input_host.hpp"
#include "frontend_config.hpp"
#include "platform_support.hpp"
#include "camera_analog.hpp"
#include "draw_distance.hpp"
#if BASELINE_PLAYER
#include "player_version.hpp"
#endif
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <mutex>
#include <stdexcept>

namespace baseline::host {
namespace {
// recompcontrollerdb.txt beside the executable only fills gaps: a controller
// already mapped by SDL, or by Steam through SDL_GAMECONTROLLERCONFIG, keeps
// that mapping.
void add_missing_controller_mappings() {
    std::filesystem::path path;
    try { path = tooie::platform::executable_path().parent_path() / "recompcontrollerdb.txt"; }
    catch (const std::exception&) { return; }
    std::ifstream input(path);
    if (!input) return;
    const std::string platform = std::string("platform:") + SDL_GetPlatform() + ",";
    int added = 0;
    for (std::string line; std::getline(input, line);) {
        if (line.size() < 33 || line[0] == '#' || line[32] != ',') continue;
        if (line.back() != ',') line += ',';
        if (line.find("platform:") != std::string::npos && line.find(platform) == std::string::npos) continue;
        char* existing = SDL_GameControllerMappingForGUID(SDL_JoystickGetGUIDFromString(line.substr(0, 32).c_str()));
        if (existing) { SDL_free(existing); continue; }
        if (SDL_GameControllerAddMapping(line.c_str()) == 1) ++added;
    }
    std::fprintf(stderr, "baseline controller mappings added=%d from %s\n", added, path.string().c_str());
}
SDL_Window* window = nullptr; // SDL owner, never dereferenced by Draw.
Uint32 window_id = 0;
bool focused = false, previously_ready = false, quarantine = false;
bool reported_unready = false, initialized = false;
std::atomic<bool> menu{false}, ready{false}, started{false};
std::atomic<bool> sticky_close{false}, sticky_quit{false}, overflow{false};
std::atomic<int> vsync{-1}, resolution{1}, msaa{0}, supersampling{1};
std::atomic<bool> quiet_talon_trot{false};
std::atomic<uint32_t> output_rate{pack_output_rate({})};
Preferences latched, selected;
std::string notice;
std::mutex request_mutex, render_mutex;
std::deque<Request> requests;
RenderInput render_input;
constexpr size_t request_limit = 64, event_limit = 1024;
#if BASELINE_PLAYER
unsigned marker_seq = 0;
bool marker_conflict_reported = false;
std::string marker_toast;
std::chrono::steady_clock::time_point marker_until{};

const char* f4_owner() {
    using namespace tooie::input;
    for (unsigned action = 0; action <= unsigned(Action::Menu); ++action)
        for (unsigned slot = 0; slot < 2; ++slot)
            if (get_binding(Action(action), Device::Keyboard, BindingLayer::Base, slot).code == SDL_SCANCODE_F4)
                return action_name(Action(action));
    return nullptr;
}
void mark_issue() {
    if (const auto* owner = f4_owner()) {
        notice = std::string("F4 is bound to ") + owner + "; issue marker disabled.";
        if (!marker_conflict_reported) {
            std::fprintf(stderr, "baseline issue-marker disabled: F4 is bound to %s\n", owner);
            std::fflush(stderr);
            marker_conflict_reported = true;
        }
        return;
    }
    const auto now = std::time(nullptr);
    std::tm local{}, utc{};
#ifdef _WIN32
    localtime_s(&local, &now); gmtime_s(&utc, &now);
#else
    localtime_r(&now, &local); gmtime_r(&now, &utc);
#endif
    char local_stamp[32]{}, utc_stamp[32]{}, clock_stamp[16]{};
    std::strftime(local_stamp, sizeof(local_stamp), "%Y-%m-%d %H:%M:%S", &local);
    std::strftime(utc_stamp, sizeof(utc_stamp), "%Y-%m-%dT%H:%M:%SZ", &utc);
    std::strftime(clock_stamp, sizeof(clock_stamp), "%H:%M:%S", &local);
    std::string controller = "none";
    for (const auto& pad : tooie::input::controllers()) if (pad.selected) {
        controller = pad.friendly_name.empty() ? pad.name : pad.friendly_name;
        break;
    }
    // Keep a hardware-supplied name inside this single quoted log field/line.
    for (auto& ch : controller) if (ch == '\r' || ch == '\n' || ch == '"' || ch == '\\') ch = '?';
    ++marker_seq;
    const bool saved = std::fprintf(stderr, "baseline issue-marker seq=%u local=%s utc=%s version=%s game_started=%d menu=%d "
        "resolution=%d wide=%d native_aspect=%d cutscene_aspect=%d output_rate=%d manual_hz=%d vsync=%d fullscreen=%d controller=\"%s\"\n",
        marker_seq, local_stamp, utc_stamp, baseline::player::build::version,
        int(started.load(std::memory_order_acquire)), int(menu.load(std::memory_order_acquire)),
        selected.resolution, latched.wide, latched.native_aspect, latched.cutscene_aspect,
        selected.output_rate, selected.manual_hz, selected.vsync, selected.fullscreen, controller.c_str()) > 0 && std::fflush(stderr) == 0;
    marker_toast = "Issue marker #" + std::to_string(marker_seq) + (saved
        ? std::string(" saved at ") + clock_stamp + ". Send player.log."
        : std::string(" at ") + clock_stamp + " could not be written to player.log.");
    marker_until = std::chrono::steady_clock::now() + std::chrono::seconds(4);
    if (notice.rfind("F4 is bound to ", 0) == 0) notice.clear();
}
#endif

void set_menu(bool value) noexcept {
    if (value && !ready.load(std::memory_order_acquire)) {
        if (!reported_unready) {
            std::fputs("baseline menu unavailable until the renderer is ready\n", stderr);
            reported_unready = true;
        }
        return;
    }
    menu.store(value, std::memory_order_release);
}
void close_menu() {
    menu.store(false, std::memory_order_release);
    tooie::input::abandon_baseline_capture();
}
int bounded(int value, int lo, int hi, int fallback) {
    return value < lo || value > hi ? fallback : value;
}
#if BASELINE_PLAYER
void set_fullscreen(bool enabled) {
    const int result = SDL_SetWindowFullscreen(window, enabled ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    const std::string error = result != 0 ? SDL_GetError() : "Window flags did not match the request";
    selected.fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (result == 0 && selected.fullscreen == int(enabled)) {
        tooie::frontend::settings::set_int("baseline_fullscreen", selected.fullscreen);
        if (notice.rfind("Fullscreen could not be changed:", 0) == 0) notice.clear();
        std::fprintf(stderr, "baseline fullscreen applied=%d owner=SDL\n", selected.fullscreen);
    } else {
        notice = "Fullscreen could not be changed: " + error;
        std::fprintf(stderr, "baseline fullscreen requested=%d actual=%d error=%s\n", int(enabled), selected.fullscreen, error.c_str());
    }
}
#endif
void update_preference(Preference key, int value) {
    using namespace tooie::frontend;
    switch (key) {
    case Preference::Resolution:
        if (value < 0 || value > 8) return;
        selected.resolution = value; settings::set_int("baseline_resolution", value);
        resolution.store(value, std::memory_order_release); break;
    case Preference::OutputRate:
        if (value < 0 || value > 3) return;
        selected.output_rate = value; settings::set_int("baseline_output_rate", value);
        output_rate.store(pack_output_rate({selected.output_rate, selected.manual_hz}), std::memory_order_release); break;
    case Preference::ManualHz:
        if (value < 20 || value > 360) return;
        selected.manual_hz = value; settings::set_int("baseline_manual_hz", value);
        output_rate.store(pack_output_rate({selected.output_rate, selected.manual_hz}), std::memory_order_release); break;
    case Preference::Wide:
        if (value < -1 || value > 1) return;
        selected.wide = value; settings::set_int("tooie_game_widescreen", value); break;
    case Preference::NativeAspect:
        if (value < 0 || value > 4) return;
        selected.native_aspect = value; settings::set_int("tooie_native_aspect", value); break;
    case Preference::CutsceneAspect:
        if (value < 0 || value > 1) return;
        selected.cutscene_aspect = value; settings::set_int("tooie_cutscene_aspect", value); break;
    case Preference::AnalogCamera:
        if (value < 0 || value > 1) return;
        selected.analog_camera = value; settings::set_bool("tooie_analog_camera_enabled", value != 0);
        tooie::camera::set_analog_enabled(value != 0); break;
    case Preference::CameraInvertX:
        if (value < 0 || value > 1) return;
        selected.camera_invert_x = value; settings::set_bool("tooie_camera_invert_x", value != 0);
        tooie::camera::set_horizontal_inverted(value != 0); break;
    case Preference::Msaa:
        if (value != 0 && value != 2 && value != 4 && value != 8) return;
        selected.msaa = value; settings::set_int("baseline_msaa", value);
        msaa.store(value, std::memory_order_release); break;
    case Preference::Supersampling:
        if (value != 1 && value != 2 && value != 4) return;
        selected.supersampling = value; settings::set_int("baseline_supersampling", value);
        supersampling.store(value, std::memory_order_release); break;
    case Preference::DrawDistance:
        if (value < 1 || value > 6) return;
        selected.draw_distance = value; settings::set_int("tooie_draw_distance", value);
        tooie::draw_distance::set_model_distance_scale(value); break;
    case Preference::DisableLod:
        if (value < 0 || value > 1) return;
        selected.disable_lod = value; settings::set_bool("tooie_disable_lod", value != 0);
        tooie::draw_distance::set_lod_disabled(value != 0); break;
    case Preference::MouseAim:
        if (value < 0 || value > 1) return;
        selected.mouse_aim = value; settings::set_bool("tooie_mouse_aim", value != 0);
        tooie::input::set_mouse_aim_enabled(value != 0); break;
    case Preference::MouseSensitivity:
        if (value < 1 || value > 100) return;
        selected.mouse_sensitivity = value; settings::set_int("tooie_mouse_sensitivity", value);
        tooie::camera::set_mouse_aim_sensitivity(value / 100.0f); break;
    case Preference::MouseInvertY:
        if (value < 0 || value > 1) return;
        selected.mouse_invert_y = value; settings::set_bool("tooie_mouse_invert_y", value != 0);
        tooie::camera::set_mouse_aim_inverted(value != 0); break;
    case Preference::CameraInvertY:
        if (value < 0 || value > 1) return;
        selected.camera_invert_y = value; settings::set_bool("tooie_camera_invert_y", value != 0);
        tooie::camera::set_vertical_inverted(value != 0); break;
    case Preference::QuietTalonTrot:
        if (value < 0 || value > 1) return;
        selected.quiet_talon_trot = value;
        settings::set_int("tooie_quiet_talon_trot", value);
#if BASELINE_PLAYER
        quiet_talon_trot.store(value != 0, std::memory_order_release);
#endif
        break;
    case Preference::VSync:
        if (value < 0 || value > 1) return;
        selected.vsync = value; settings::set_int("baseline_vsync", value);
        vsync.store(value, std::memory_order_release); break;
    case Preference::Fullscreen:
#if BASELINE_PLAYER
        if (value < 0 || value > 1) return;
        set_fullscreen(value != 0);
#endif
        break;
    }
}
void apply(Request& request) {
    using namespace tooie::input;
    switch (request.kind) {
    case RequestKind::Controller: select_controller(request.value); break;
    case RequestKind::Profile: select_controller_profile(request.name); break;
    case RequestKind::ResetBindings: reset_bindings(); break;
    case RequestKind::Capture:
        if (menu_open() && request.action <= Action::Menu && request.slot < 2 &&
            request.device <= Device::AxisNegative)
            begin_capture(request.action, request.device, request.slot);
        break;
    case RequestKind::CancelCapture: cancel_capture(); break;
    case RequestKind::Preference: update_preference(request.preference, request.value); break;
    }
}
bool belongs(const SDL_Event& event) {
    switch (event.type) {
    case SDL_WINDOWEVENT: return event.window.windowID == window_id;
    case SDL_KEYDOWN: case SDL_KEYUP: return event.key.windowID == window_id;
    case SDL_TEXTINPUT: return event.text.windowID == window_id;
    case SDL_MOUSEMOTION: return event.motion.windowID == window_id;
    case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP: return event.button.windowID == window_id;
    case SDL_MOUSEWHEEL: return event.wheel.windowID == window_id;
    default: return true;
    }
}
void queue_event(const SDL_Event& event) {
    switch (event.type) {
    case SDL_WINDOWEVENT: case SDL_KEYDOWN: case SDL_KEYUP: case SDL_TEXTINPUT:
    case SDL_MOUSEMOTION: case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP: case SDL_MOUSEWHEEL: break;
    default: return;
    }
    std::lock_guard lock(render_mutex);
    if (render_input.events.size() == event_limit) {
        render_input.events.clear(); render_input.reset_input = true;
        overflow.store(true, std::memory_order_release);
    }
    render_input.events.push_back(event);
}
}
#ifdef TOOIE_DECK_DEFAULTS
// Steam Deck defaults for settings a profile has not saved yet: 3x of 240 lines
// suits the 800-line panel, output follows the display rate, and 16:10
// widescreen fills the screen.
constexpr Preferences fresh{.resolution = 3, .output_rate = 2, .wide = 1, .native_aspect = 4, .fullscreen = 1,
    .analog_camera = 1, .camera_invert_x = 1};
#else
constexpr Preferences fresh{};
#endif
void initialize(SDL_Window* target, const std::filesystem::path& profile) {
    if (initialized || !target) throw std::runtime_error("Invalid baseline host initialization");
    window = target; window_id = SDL_GetWindowID(target);
    focused = (SDL_GetWindowFlags(target) & SDL_WINDOW_INPUT_FOCUS) != 0;
    tooie::frontend::settings::initialize(profile);
    using namespace tooie::frontend;
    selected.resolution = bounded(settings::get_int("baseline_resolution", fresh.resolution), 0, 8, fresh.resolution);
    selected.output_rate = bounded(settings::get_int("baseline_output_rate", fresh.output_rate), 0, 3, fresh.output_rate);
    selected.manual_hz = bounded(settings::get_int("baseline_manual_hz", 60), 20, 360, 60);
    selected.wide = bounded(settings::get_int("tooie_game_widescreen", fresh.wide), -1, 1, fresh.wide);
    selected.native_aspect = bounded(settings::get_int("tooie_native_aspect", fresh.native_aspect), 0, 4, fresh.native_aspect);
    selected.cutscene_aspect = bounded(settings::get_int("tooie_cutscene_aspect", 0), 0, 1, 0);
    selected.vsync = bounded(settings::get_int("baseline_vsync", -1), -1, 1, -1);
    selected.fullscreen = bounded(settings::get_int("baseline_fullscreen", fresh.fullscreen), 0, 1, fresh.fullscreen);
#if BASELINE_PLAYER
    selected.quiet_talon_trot = bounded(settings::get_int("tooie_quiet_talon_trot", 0), 0, 1, 0);
#else
    selected.quiet_talon_trot = 0;
#endif
    selected.analog_camera = settings::get_bool("tooie_analog_camera_enabled", fresh.analog_camera != 0);
    selected.camera_invert_x = settings::get_bool("tooie_camera_invert_x", fresh.camera_invert_x != 0);
    selected.camera_invert_y = settings::get_bool("tooie_camera_invert_y", fresh.camera_invert_y != 0);
    tooie::camera::set_analog_enabled(selected.analog_camera != 0);
    tooie::camera::set_horizontal_inverted(selected.camera_invert_x != 0);
    tooie::camera::set_vertical_inverted(selected.camera_invert_y != 0);
    {
        const int samples = settings::get_int("baseline_msaa", 0);
        const int factor = settings::get_int("baseline_supersampling", 1);
        selected.msaa = samples == 2 || samples == 4 || samples == 8 ? samples : 0;
        selected.supersampling = factor == 2 || factor == 4 ? factor : 1;
        msaa.store(selected.msaa, std::memory_order_release);
        supersampling.store(selected.supersampling, std::memory_order_release);
    }
    selected.draw_distance = bounded(settings::get_int("tooie_draw_distance", 1), 1, 6, 1);
    selected.disable_lod = settings::get_bool("tooie_disable_lod", false) ? 1 : 0;
    tooie::draw_distance::set_model_distance_scale(selected.draw_distance);
    tooie::draw_distance::set_lod_disabled(selected.disable_lod != 0);
    selected.mouse_aim = settings::get_bool("tooie_mouse_aim", true) ? 1 : 0;
    selected.mouse_sensitivity = bounded(settings::get_int("tooie_mouse_sensitivity", 6), 1, 100, 6);
    selected.mouse_invert_y = settings::get_bool("tooie_mouse_invert_y", true) ? 1 : 0;
    tooie::input::set_mouse_aim_enabled(selected.mouse_aim != 0);
    tooie::camera::set_mouse_aim_sensitivity(selected.mouse_sensitivity / 100.0f);
    tooie::camera::set_mouse_aim_inverted(selected.mouse_invert_y != 0);
    latched = selected; vsync.store(selected.vsync, std::memory_order_release);
    resolution.store(selected.resolution, std::memory_order_release);
    quiet_talon_trot.store(selected.quiet_talon_trot != 0, std::memory_order_release);
    output_rate.store(pack_output_rate({latched.output_rate, latched.manual_hz}), std::memory_order_release);
#if BASELINE_PLAYER
    if (selected.fullscreen) set_fullscreen(true);
#endif
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
        std::fprintf(stderr, "baseline mapper keyboard-only: %s\n", SDL_GetError());
    else add_missing_controller_mappings();
    tooie::input::initialize_settings(profile);
    initialized = true;
}
void shutdown() noexcept {
    if (initialized) { tooie::frontend::settings::flush(); tooie::input::shutdown(); }
    initialized = false; window = nullptr; window_id = 0;
    ready = menu = started = sticky_close = sticky_quit = overflow = false;
    previously_ready = quarantine = reported_unready = false; vsync = -1;
    resolution.store(1, std::memory_order_release);
    msaa.store(0, std::memory_order_release); supersampling.store(1, std::memory_order_release);
    quiet_talon_trot.store(false, std::memory_order_release);
    output_rate.store(pack_output_rate({}), std::memory_order_release);
#if BASELINE_PLAYER
    marker_seq = 0; marker_conflict_reported = false;
    marker_toast.clear(); marker_until = {};
    if (notice.rfind("F4 is bound to ", 0) == 0) notice.clear();
#endif
    { std::lock_guard lock(request_mutex); requests.clear(); }
    { std::lock_guard lock(render_mutex); render_input = {}; }
}
void handle_event(const SDL_Event& event) {
    if (!initialized || !belongs(event)) return;
    if (event.type == SDL_QUIT) request_quit();
    if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_CLOSE) request_quit();
        if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) focused = false;
        if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) focused = true;
    }
#if BASELINE_PLAYER
    if (event.type == SDL_KEYDOWN && !event.key.repeat && event.key.keysym.scancode == SDL_SCANCODE_F4 &&
        !(event.key.keysym.mod & (KMOD_SHIFT | KMOD_ALT | KMOD_CTRL | KMOD_GUI)) && focused &&
        !tooie::input::input_suppressed()) mark_issue();
#endif
    tooie::input::handle_event(event);
    queue_event(event);
}
InputPublication tick(bool gameplay) {
    if (!initialized) throw std::runtime_error("Baseline host tick before initialization");
    const bool usable = ready.load(std::memory_order_acquire);
    if (!usable && (previously_ready || menu_open() || tooie::input::capture_active())) {
        close_menu(); notice = "Menu renderer became unavailable.";
        std::fputs("baseline menu ready lost; capture and menu ownership released\n", stderr);
    }
    previously_ready = usable;
    const bool overloaded = overflow.exchange(false, std::memory_order_acq_rel);
    std::deque<Request> pending;
    { std::lock_guard lock(request_mutex); pending.swap(requests); }
    if (overloaded) {
        pending.clear(); close_menu(); quarantine = true;
        notice = "Menu input queue overflowed; release controls to continue.";
        std::lock_guard lock(render_mutex); render_input.events.clear(); render_input.reset_input = true;
    }
    if (sticky_close.exchange(false, std::memory_order_acq_rel)) close_menu();
    if (!overloaded) for (auto& request : pending) apply(request);
    tooie::input::tick(window, gameplay, menu_open());
    const auto mapped = tooie::input::snapshot();
    const bool activity = tooie::input::consume_mapped_activity();
    input::State physical{mapped.buttons, mapped.x, mapped.y};
    physical.buttons |= tooie::input::consume_transient_buttons();
    if (quarantine && !activity && !physical.nonneutral()) quarantine = false;
    const bool quit = sticky_quit.load(std::memory_order_acquire);
    if (quarantine || menu_open() || quit) physical = {};
    auto snapshot = std::make_shared<Snapshot>();
    SDL_GetWindowSize(window, &snapshot->width, &snapshot->height);
    SDL_GetWindowSizeInPixels(window, &snapshot->pixel_width, &snapshot->pixel_height);
    snapshot->focused = focused;
    snapshot->minimized = (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0;
    snapshot->menu = menu_open(); snapshot->suppressed = tooie::input::input_suppressed();
    snapshot->gamepad = tooie::input::menu_gamepad_state();
    snapshot->capture = tooie::input::capture_status(); snapshot->controllers = tooie::input::controllers();
    snapshot->profiles = tooie::input::controller_profiles(); snapshot->active_profile = tooie::input::active_controller_profile();
    snapshot->preferences = selected; snapshot->notice = notice;
#if BASELINE_PLAYER
    if (std::chrono::steady_clock::now() < marker_until) snapshot->toast = marker_toast;
#endif
    if (const auto recovery = tooie::frontend::settings::recovery_notice(); !recovery.empty()) snapshot->notice = recovery;
    for (unsigned i = 0; i < snapshot->bindings.size(); ++i) {
        auto& row = snapshot->bindings[i]; const auto action = tooie::input::Action(i);
        row.action = tooie::input::action_name(action);
        for (unsigned slot = 0; slot < 2; ++slot) {
            row.keyboard[slot] = tooie::input::binding_label(tooie::input::get_binding(action, tooie::input::Device::Keyboard, slot));
            for (unsigned device = 1; device <= 3; ++device)
                row.controller[(device - 1) * 2 + slot] = tooie::input::binding_label(
                    tooie::input::get_binding(action, tooie::input::Device(device), slot));
        }
    }
    { std::lock_guard lock(render_mutex); render_input.snapshot = std::move(snapshot); }
    tooie::frontend::settings::flush();
    return {physical, activity, focused, quit};
}
const Preferences& latched_preferences() noexcept { return latched; }
void note_game_started() noexcept { started.store(true, std::memory_order_release); }
bool menu_open() noexcept { return menu.load(std::memory_order_acquire); }
bool enqueue(Request request) {
    if (request.name.size() > 128) return false;
    std::lock_guard lock(request_mutex);
    if (requests.size() == request_limit) {
        requests.clear(); overflow.store(true, std::memory_order_release); return false;
    }
    requests.push_back(std::move(request)); return true;
}
void request_close() noexcept { sticky_close.store(true, std::memory_order_release); }
void request_quit() noexcept { sticky_quit.store(true, std::memory_order_release); }
void publish_ready(bool value) noexcept { ready.store(value, std::memory_order_release); }
RenderInput take_render_input() {
    std::lock_guard lock(render_mutex);
    RenderInput out{render_input.snapshot, {}, render_input.reset_input};
    out.events.swap(render_input.events); render_input.reset_input = false; return out;
}
int requested_vsync() noexcept { return vsync.load(std::memory_order_acquire); }
int requested_resolution() noexcept { return resolution.load(std::memory_order_acquire); }
int requested_msaa() noexcept { return msaa.load(std::memory_order_acquire); }
int requested_supersampling() noexcept { return supersampling.load(std::memory_order_acquire); }
bool requested_quiet_talon_trot() noexcept { return quiet_talon_trot.load(std::memory_order_acquire); }
OutputRate requested_output_rate() noexcept { return unpack_output_rate(output_rate.load(std::memory_order_acquire)); }
}
namespace tooie::input {
const Host& host() noexcept {
    static const Host value{true, baseline::host::menu_open,
        [](bool open) noexcept { baseline::host::set_menu(open); },
        []() { return baseline::host::started.load(std::memory_order_acquire); }, nullptr};
    return value;
}
}
