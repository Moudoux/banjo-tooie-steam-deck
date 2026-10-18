#include "minimum_host.hpp"
#include "frontend_input_host.hpp"
#include "frontend_config.hpp"
#include "widescreen.hpp"
#include "minimum_render.hpp"
#if BASELINE_PLAYER
#include "player_version.hpp"
#endif
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <stdexcept>

namespace baseline::host {
namespace {
SDL_Window* window = nullptr; // SDL owner, never dereferenced by Draw.
Uint32 window_id = 0;
bool focused = false, previously_ready = false, quarantine = false;
bool reported_unready = false, initialized = false;
// SDL-computed request bits; UINT32_MAX means no publication yet. The callee masks flags.
uint32_t published_live_aspect = UINT32_MAX;
std::atomic<bool> menu{false}, ready{false}, started{false};
std::atomic<bool> sticky_close{false}, sticky_quit{false}, overflow{false};
std::atomic<int> vsync{-1}, resolution{1};
std::atomic<bool> quiet_talon_trot{false}, talon_trot_toggle{false};
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
    int pixel_width = 0, pixel_height = 0;
    SDL_GetWindowSizeInPixels(window, &pixel_width, &pixel_height);
    ++marker_seq;
    // Guest publication is sampled independently; these fields do not attest displayed pixels.
    const bool saved = std::fprintf(stderr, "baseline issue-marker seq=%u local=%s utc=%s version=%s game_started=%d menu=%d "
        "requested_resolution=%d startup_wide=%d startup_aspect=%d startup_cutscene_aspect=%d requested_output_rate=%d requested_manual_hz=%d requested_vsync=%d sdl_fullscreen=%d controller=\"%s\" published_aspect_bits=0x%08x requested_aspect=%d drawable=%dx%d requested_wide=%d requested_quiet_trot=%d requested_trot_toggle=%d guest_publication=0x%08x\n",
        marker_seq, local_stamp, utc_stamp, baseline::player::build::version,
        int(started.load(std::memory_order_acquire)), int(menu.load(std::memory_order_acquire)),
        selected.resolution, latched.wide, latched.native_aspect, latched.cutscene_aspect,
        selected.output_rate, selected.manual_hz, selected.vsync,
        int((SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0), controller.c_str(),
        published_live_aspect, selected.native_aspect, pixel_width, pixel_height,
        selected.wide, selected.quiet_talon_trot, selected.talon_trot_toggle,
        render::publication_word()) > 0 && std::fflush(stderr) == 0;
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
        if (value < 1 || value > 8) return;
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
        selected.wide = value; settings::set_int("tooie_game_widescreen", value);
        tooie::widescreen::set_live_widescreen_request(value); break;
    case Preference::NativeAspect:
        if (value < 0 || value > fit_aspect) return;
        selected.native_aspect = value; settings::set_int("tooie_native_aspect", value); break;
    case Preference::CutsceneAspect:
        if (value < 0 || value > 1) return;
        selected.cutscene_aspect = value; settings::set_int("tooie_cutscene_aspect", value); break;
    case Preference::QuietTalonTrot:
        if (value < 0 || value > 1) return;
        selected.quiet_talon_trot = value;
        settings::set_int("tooie_quiet_talon_trot", value);
#if BASELINE_PLAYER
        quiet_talon_trot.store(value != 0, std::memory_order_release);
#endif
        break;
    case Preference::TalonTrotToggle:
        if (value < 0 || value > 1) return;
        selected.talon_trot_toggle = value;
        settings::set_int("tooie_talon_trot_toggle", value);
#if BASELINE_PLAYER
        talon_trot_toggle.store(value != 0, std::memory_order_release);
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
void initialize(SDL_Window* target, const std::filesystem::path& profile, bool allow_background_controller) {
    if (initialized || !target) throw std::runtime_error("Invalid baseline host initialization");
    window = target; window_id = SDL_GetWindowID(target);
    focused = (SDL_GetWindowFlags(target) & SDL_WINDOW_INPUT_FOCUS) != 0;
    tooie::frontend::settings::initialize(profile);
    using namespace tooie::frontend;
    selected.resolution = bounded(settings::get_int("baseline_resolution", 1), 1, 8, 1);
    selected.output_rate = bounded(settings::get_int("baseline_output_rate", 0), 0, 3, 0);
    selected.manual_hz = bounded(settings::get_int("baseline_manual_hz", 60), 20, 360, 60);
    selected.wide = bounded(settings::get_int("tooie_game_widescreen", -1), -1, 1, -1);
    tooie::widescreen::set_live_widescreen_request(selected.wide);
    selected.native_aspect = bounded(settings::get_int("tooie_native_aspect", 0), 0, fit_aspect, 0);
    selected.cutscene_aspect = bounded(settings::get_int("tooie_cutscene_aspect", 0), 0, 1, 0);
    selected.vsync = bounded(settings::get_int("baseline_vsync", -1), -1, 1, -1);
    selected.fullscreen = bounded(settings::get_int("baseline_fullscreen", 0), 0, 1, 0);
#if BASELINE_PLAYER
    selected.quiet_talon_trot = bounded(settings::get_int("tooie_quiet_talon_trot", 0), 0, 1, 0);
    selected.talon_trot_toggle = bounded(settings::get_int("tooie_talon_trot_toggle", 0), 0, 1, 0);
#else
    selected.quiet_talon_trot = 0;
    selected.talon_trot_toggle = 0;
#endif
    latched = selected; vsync.store(selected.vsync, std::memory_order_release);
    resolution.store(selected.resolution, std::memory_order_release);
    quiet_talon_trot.store(selected.quiet_talon_trot != 0, std::memory_order_release);
    talon_trot_toggle.store(selected.talon_trot_toggle != 0, std::memory_order_release);
    output_rate.store(pack_output_rate({latched.output_rate, latched.manual_hz}), std::memory_order_release);
#if BASELINE_PLAYER
    if (selected.fullscreen) set_fullscreen(true);
#endif
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
        std::fprintf(stderr, "baseline mapper keyboard-only: %s\n", SDL_GetError());
    tooie::input::initialize_settings(profile, allow_background_controller);
    initialized = true;
}
void shutdown() noexcept {
    if (initialized) {
        if (!tooie::frontend::settings::flush()) {
            std::fputs("baseline settings could not be saved during shutdown\n", stderr);
            std::fflush(stderr);
        }
        tooie::input::shutdown();
    }
    initialized = false; window = nullptr; window_id = 0;
    ready = menu = started = sticky_close = sticky_quit = overflow = false;
    previously_ready = quarantine = reported_unready = false; vsync = -1;
    published_live_aspect = UINT32_MAX;
    tooie::widescreen::set_live_widescreen_request(-1);
    resolution.store(1, std::memory_order_release);
    quiet_talon_trot.store(false, std::memory_order_release);
    talon_trot_toggle.store(false, std::memory_order_release);
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
    if (!focused) switch (event.type) {
    case SDL_KEYDOWN: case SDL_KEYUP: case SDL_TEXTINPUT:
    case SDL_MOUSEMOTION: case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP: case SDL_MOUSEWHEEL:
        return; // Other-app activity must not reach mapper shortcuts or menu UI.
    default: break;
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
    if (started.load(std::memory_order_acquire) && !(SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) &&
        snapshot->pixel_width > 0 && snapshot->pixel_height > 0) {
        const uint32_t live = selected.native_aspect == fit_aspect ?
            tooie::widescreen::fit_aspect_bits(snapshot->pixel_width, snapshot->pixel_height) :
            uint32_t(selected.native_aspect) << TOOIE_WIDESCREEN_SETTINGS_ASPECT_SHIFT;
        if (live != published_live_aspect) {
            published_live_aspect = live;
            tooie::widescreen::set_live_aspect_bits(live);
        }
    }
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
    tooie::frontend::settings::flush(true);
    return {physical, activity, focused, quit, mapped.game_enabled};
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
bool requested_quiet_talon_trot() noexcept { return quiet_talon_trot.load(std::memory_order_acquire); }
bool requested_talon_trot_toggle() noexcept { return talon_trot_toggle.load(std::memory_order_acquire); }
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
