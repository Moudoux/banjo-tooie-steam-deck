#include "minimum_host.hpp"
#include "frontend_input_host.hpp"
#include "frontend_config.hpp"
#include <algorithm>
#include <cstdio>
#include <mutex>
#include <stdexcept>

namespace baseline::host {
namespace {
SDL_Window* window = nullptr; // SDL owner, never dereferenced by Draw.
Uint32 window_id = 0;
bool focused = false, previously_ready = false, quarantine = false;
bool reported_unready = false, initialized = false;
std::atomic<bool> menu{false}, ready{false}, started{false};
std::atomic<bool> sticky_close{false}, sticky_quit{false}, overflow{false};
std::atomic<int> vsync{-1};
Preferences latched, selected;
std::string notice;
std::mutex request_mutex, render_mutex;
std::deque<Request> requests;
RenderInput render_input;
constexpr size_t request_limit = 64, event_limit = 1024;

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
void update_preference(Preference key, int value) {
    using namespace tooie::frontend;
    switch (key) {
    case Preference::Resolution:
        if (value < 1 || value > 8) return;
        selected.resolution = value; settings::set_int("baseline_resolution", value); break;
    case Preference::OutputRate:
        if (value < 0 || value > 3) return;
        selected.output_rate = value; settings::set_int("baseline_output_rate", value); break;
    case Preference::ManualHz:
        if (value < 20 || value > 360) return;
        selected.manual_hz = value; settings::set_int("baseline_manual_hz", value); break;
    case Preference::Wide:
        if (value < -1 || value > 1) return;
        selected.wide = value; settings::set_int("tooie_game_widescreen", value); break;
    case Preference::NativeAspect:
        if (value < 0 || value > 3) return;
        selected.native_aspect = value; settings::set_int("tooie_native_aspect", value); break;
    case Preference::CutsceneAspect:
        if (value < 0 || value > 1) return;
        selected.cutscene_aspect = value; settings::set_int("tooie_cutscene_aspect", value); break;
    case Preference::VSync:
        if (value < 0 || value > 1) return;
        selected.vsync = value; settings::set_int("baseline_vsync", value);
        vsync.store(value, std::memory_order_release); break;
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
void initialize(SDL_Window* target, const std::filesystem::path& profile) {
    if (initialized || !target) throw std::runtime_error("Invalid baseline host initialization");
    window = target; window_id = SDL_GetWindowID(target);
    focused = (SDL_GetWindowFlags(target) & SDL_WINDOW_INPUT_FOCUS) != 0;
    tooie::frontend::settings::initialize(profile);
    using namespace tooie::frontend;
    selected.resolution = bounded(settings::get_int("baseline_resolution", 1), 1, 8, 1);
    selected.output_rate = bounded(settings::get_int("baseline_output_rate", 0), 0, 3, 0);
    selected.manual_hz = bounded(settings::get_int("baseline_manual_hz", 60), 20, 360, 60);
    selected.wide = bounded(settings::get_int("tooie_game_widescreen", -1), -1, 1, -1);
    selected.native_aspect = bounded(settings::get_int("tooie_native_aspect", 0), 0, 3, 0);
    selected.cutscene_aspect = bounded(settings::get_int("tooie_cutscene_aspect", 0), 0, 1, 0);
    selected.vsync = bounded(settings::get_int("baseline_vsync", -1), -1, 1, -1);
    latched = selected; vsync.store(selected.vsync, std::memory_order_release);
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
        std::fprintf(stderr, "baseline mapper keyboard-only: %s\n", SDL_GetError());
    tooie::input::initialize_settings(profile);
    initialized = true;
}
void shutdown() noexcept {
    if (initialized) { tooie::frontend::settings::flush(); tooie::input::shutdown(); }
    initialized = false; window = nullptr; window_id = 0;
    ready = menu = started = sticky_close = sticky_quit = overflow = false;
    previously_ready = quarantine = reported_unready = false; vsync = -1;
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
}
namespace tooie::input {
const Host& host() noexcept {
    static const Host value{true, baseline::host::menu_open,
        [](bool open) noexcept { baseline::host::set_menu(open); },
        []() { return baseline::host::started.load(std::memory_order_acquire); }, nullptr};
    return value;
}
}
