#include "minimum_host.hpp"
#include "guest/minimum-host/first_person_aim_abi.h"
#include <cmath>
#include "frontend_input_host.hpp"
#include "frontend_config.hpp"
#include "widescreen.hpp"
#include "minimum_render.hpp"
#if BASELINE_PLAYER
#include "virtual_clock.hpp"
#include "player_version.hpp"
#include "sound_trace.hpp"
#include "issue_evidence.hpp"
#include "json/json.hpp"
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
std::atomic<int> vsync{-1}, resolution{1}, cutscene_aspect{0}, texture_filter{0}, output_filter{0};
std::atomic<bool> quiet_talon_trot{false}, talon_trot_toggle{false};
std::atomic<int> master_volume{100}, music_volume{100};
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
std::string loaded_profiles; // RAM copy of the string passed to the mapper, before migration.
constexpr uint64_t fingerprint_basis = 1469598103934665603ULL;
uint64_t fingerprint_step(uint64_t hash, uint64_t value) { return (hash ^ value) * 1099511628211ULL; }

// Base six-code rows only. Fingerprints correlate sessions; they are not anonymous or proof.
std::string saved_base_rows(const std::string& source, const std::string& id, std::string& active) {
    if (source.empty()) return "absent";
    try {
        const auto doc = nlohmann::json::parse(source);
        if (!doc.is_object()) return "invalid";
        if (!doc.contains("devices")) return "absent";
        const auto& devices = doc["devices"];
        if (!devices.is_object()) return "invalid";
        if (!devices.contains(id)) return "absent";
        const auto& record = devices[id];
        active = record.value("active", std::string{});
        const auto& rows = record.at("profiles").at(active);
        if (!rows.is_array() || rows.size() != size_t(tooie::input::Action::Count)) return "invalid";
        uint64_t hash = fingerprint_basis;
        for (const auto& row : rows) {
            if (!row.is_array() || row.size() != 6) return "invalid";
            for (const auto& code : row) {
                if (!code.is_number_integer() || (code.is_number_unsigned() && code.get<uint64_t>() > 255)) return "invalid";
                const auto value = code.get<int64_t>();
                if (value < -1 || value > 255) return "invalid";
                hash = fingerprint_step(hash, uint64_t(value + 1));
            }
        }
        char text[17];
        std::snprintf(text, sizeof(text), "%016llx", static_cast<unsigned long long>(hash));
        return text;
    } catch (...) { return "invalid"; }
}

// SDL owner only; no raw identifier, serial, path, profile name or JSON is logged.
bool log_controller_admission(const char* phase, unsigned seq) {
    using namespace tooie::input;
    const auto pads = controllers();
    const auto pad = std::find_if(pads.begin(), pads.end(), [](const auto& item) { return item.selected; });
    const auto id = selected_device_id();
    uint64_t live = fingerprint_basis, id_fp = fingerprint_basis;
    for (unsigned action = 0; action < unsigned(Action::Count); ++action)
        for (unsigned i = 2; i < 8; ++i)
            live = fingerprint_step(live, uint64_t(get_binding(Action(action), Device(i / 2), BindingLayer::Base, i % 2).code + 1));
    for (const unsigned char ch : id) id_fp = fingerprint_step(id_fp, ch);
    std::string loaded_active, current_active;
    const auto loaded = id.empty() ? std::string("none") : saved_base_rows(loaded_profiles, id, loaded_active);
    const auto current = id.empty() ? std::string("none") : saved_base_rows(
        tooie::frontend::settings::get_string("tooie_input_profiles_v1", ""), id, current_active);
    const auto profiles = controller_profiles();
    const auto active = active_controller_profile();
    const auto profile_index = std::find(profiles.begin(), profiles.end(), active) - profiles.begin();
    return std::fprintf(stderr, "baseline controller-admission phase=%s seq=%u controllers=%zu selected=%d auto=%d "
        "preferred_match=%d ambiguous=%d vid=%04x pid=%04x id_fp=%016llx profile=%td/%zu loaded_active_match=%d "
        "live_base=%016llx loaded_base=%s current_base=%s settings_notice=%d profiles_unreadable=%d\n",
        phase, seq, pads.size(), int(pad != pads.end()), int(controller_auto_selection()),
        int(!id.empty() && preferred_device_id() == id), int(pad != pads.end() && pad->ambiguous),
        pad != pads.end() ? unsigned(pad->vendor) : 0u, pad != pads.end() ? unsigned(pad->product) : 0u,
        id.empty() ? 0ULL : static_cast<unsigned long long>(id_fp), profile_index, profiles.size(),
        int(!loaded_active.empty() && loaded_active == active), static_cast<unsigned long long>(live),
        loaded.c_str(), current.c_str(), int(!tooie::frontend::settings::recovery_notice().empty()),
        int(saved_profiles_unreadable())) > 0 &&
        std::fflush(stderr) == 0;
}

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
    bool saved = std::fprintf(stderr, "baseline issue-marker seq=%u local=%s utc=%s version=%s game_started=%d menu=%d "
        "requested_resolution=%d startup_wide=%d startup_aspect=%d startup_cutscene_aspect=%d requested_cutscene_aspect=%d requested_output_rate=%d requested_manual_hz=%d requested_vsync=%d sdl_fullscreen=%d controller=\"%s\" published_aspect_bits=0x%08x requested_aspect=%d drawable=%dx%d requested_wide=%d requested_quiet_trot=%d requested_trot_toggle=%d requested_texture_filter=%d requested_output_filter=%d requested_master_volume=%d requested_music_volume=%d guest_publication=0x%08x\n",
        marker_seq, local_stamp, utc_stamp, baseline::player::build::version,
        int(started.load(std::memory_order_acquire)), int(menu.load(std::memory_order_acquire)),
        selected.resolution, latched.wide, latched.native_aspect, latched.cutscene_aspect, selected.cutscene_aspect,
        selected.output_rate, selected.manual_hz, selected.vsync,
        int((SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0), controller.c_str(),
        published_live_aspect, selected.native_aspect, pixel_width, pixel_height,
        selected.wide, selected.quiet_talon_trot, selected.talon_trot_toggle,
        selected.texture_filter, selected.output_filter, selected.master_volume, selected.music_volume,
        render::publication_word()) > 0 && std::fflush(stderr) == 0;
    // Accepted captures retain one audio snapshot for both the report and log.
    const bool capturing = issue_evidence::capture(marker_seq);
    // L05 F4 run: only an accepted evidence report arms the single renderer run.
    if (capturing) RT64::tooieL05BridgeArm.fetch_add(1, std::memory_order_release);
    if (!capturing) {
        try { saved = baseline::sound_trace::dump(stderr, marker_seq) && saved; }
        catch (...) { saved = false; }
    }
    const bool admission_saved = log_controller_admission("marker", marker_seq);
    saved = saved && admission_saved;
    marker_toast = "Issue marker #" + std::to_string(marker_seq) + (capturing
        ? ": saving diagnostic report..."
        : saved ? std::string(" saved at ") + clock_stamp + ". Report busy/unavailable; send player.log."
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
uint32_t pack_first_person_aim(const Preferences& preferences) noexcept {
    return TOOIE_FP_AIM_ENABLED | (uint32_t(preferences.first_person_aim_invert) & 3u) << 1 |
        uint32_t(preferences.first_person_aim_sensitivity) << TOOIE_FP_AIM_SENSITIVITY_SHIFT;
}
uint32_t pack_aim_sample(float x, float y) noexcept {
    const auto axis = [](float value) {
        if (!std::isfinite(value)) value = 0;
        const int16_t quantized = int16_t(std::lround(std::clamp(value, -1.0f, 1.0f) * 32767.0f));
        const uint16_t bits = uint16_t(quantized);
        return uint32_t(bits);
    };
    return axis(x) << 16 | axis(y);
}
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
    case Preference::TextureFilter:
        if (value < 0 || value > 1) return;
        selected.texture_filter = value; settings::set_int("baseline_texture_filter", value);
        texture_filter.store(value, std::memory_order_release); break;
    case Preference::OutputFilter:
        if (value < 0 || value > 2) return;
        selected.output_filter = value; settings::set_int("baseline_output_filter", value);
        output_filter.store(value, std::memory_order_release); break;
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
        selected.cutscene_aspect = value; settings::set_int("tooie_cutscene_aspect", value);
        cutscene_aspect.store(value, std::memory_order_release); break;
    case Preference::RumbleStrength:
        if (value < 0 || value > 100) return;
        selected.rumble_strength = value;
        settings::set_number("rumble_strength", double(value));
        break;
    case Preference::MasterVolume:
        if (value < 0 || value > 100) return;
        selected.master_volume = value;
        settings::set_int("tooie_master_volume_percent", value);
#if BASELINE_PLAYER
        master_volume.store(value, std::memory_order_release);
#endif
        break;
    case Preference::MusicVolume:
        if (value < 0 || value > 100) return;
        selected.music_volume = value;
        settings::set_int("tooie_music_volume_percent", value);
#if BASELINE_PLAYER
        music_volume.store(value, std::memory_order_release);
#endif
        break;
    case Preference::QuietTalonTrot:
        if (value < 0 || value > 1) return;
        selected.quiet_talon_trot = value;
        settings::set_int("tooie_quiet_talon_trot", value);
#if BASELINE_PLAYER
        quiet_talon_trot.store(value != 0, std::memory_order_release);
#endif
        break;
    case Preference::FirstPersonAim:
        if (value < 0 || value > 1) return;
        selected.first_person_aim = value; settings::set_int("tooie_first_person_aim", value); break;
    case Preference::FirstPersonAimSensitivity:
        if (value < TOOIE_FP_AIM_SENSITIVITY_MIN || value > TOOIE_FP_AIM_SENSITIVITY_MAX) return;
        selected.first_person_aim_sensitivity = value; settings::set_int("tooie_first_person_aim_sensitivity", value); break;
    case Preference::FirstPersonAimInvert:
        if (value < 0 || value > 3) return;
        selected.first_person_aim_invert = value; settings::set_int("tooie_first_person_aim_invert", value); break;
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
bool bindable(const Request& request) {
    using namespace tooie::input;
    if (!menu_open() || request.slot >= 2 || request.device > Device::AxisNegative) return false;
    if (request.layer == BindingLayer::Modified)
        return request.action == Action::Menu && request.device != Device::Keyboard;
    return request.layer == BindingLayer::Base && (request.action <= Action::Menu
#if BASELINE_PLAYER
        || request.action == Action::FastForward
#endif
        );
}
constexpr const char* duplicate_failed = "Could not duplicate the controller profile.";
constexpr const char* profiles_unreadable =
    "Saved controller profiles could not be read; controller changes this session will not be saved.";
// Existing mapper API: copies the active base bindings/extras, activates and saves the copy.
bool duplicate_active_profile() {
    const auto active = tooie::input::active_controller_profile();
    if (active.empty()) return false;
    for (int n = 1; n <= 99; ++n) {
        const std::string suffix = n == 1 ? " copy" : " copy " + std::to_string(n);
        std::string base = active.substr(0, 48 - suffix.size());
        // Never split a UTF-8 sequence: the profile JSON writer rejects invalid UTF-8.
        while (!base.empty() && base.size() < active.size() &&
               (static_cast<unsigned char>(active[base.size()]) & 0xC0) == 0x80)
            base.pop_back();
        if (tooie::input::create_controller_profile(base + suffix)) return true;
    }
    return false;
}
constexpr const char* direct_c_capability_failed = "Direct-C profile needs a controller with the required buttons, left stick and triggers.";
constexpr const char* direct_c_create_failed = "Could not create a direct-C controller profile.";
void create_direct_c_profile() {
    using namespace tooie::input;
    auto* controller = SDL_GameControllerFromInstanceID(selected_controller_instance());
    if (!controller || !SDL_GameControllerGetAttached(controller)) {
        notice = direct_c_capability_failed;
        return;
    }
    for (auto button : {SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X,
            SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
            SDL_CONTROLLER_BUTTON_LEFTSTICK, SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_BACK,
            SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
            SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT}) {
        if (!SDL_GameControllerHasButton(controller, button)) {
            notice = direct_c_capability_failed;
            return;
        }
    }
    for (auto axis : {SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY,
            SDL_CONTROLLER_AXIS_TRIGGERLEFT, SDL_CONTROLLER_AXIS_TRIGGERRIGHT}) {
        if (!SDL_GameControllerHasAxis(controller, axis)) {
            notice = direct_c_capability_failed;
            return;
        }
    }
    bool created = false;
    for (int n = 1; n <= 99; ++n) {
        const auto name = n == 1 ? std::string("Direct C") : "Direct C " + std::to_string(n);
        if (create_controller_profile(name)) {
            created = true;
            break;
        }
    }
    if (!created) {
        notice = direct_c_create_failed;
        return;
    }
    // Only the new profile changes; keyboard columns stay intact.
    for (unsigned action = 0; action < unsigned(Action::Count); ++action)
        for (auto layer : {BindingLayer::Base, BindingLayer::Modified})
            for (auto device : {Device::Button, Device::AxisPositive, Device::AxisNegative})
                for (unsigned slot = 0; slot < 2; ++slot)
                    if (get_binding(Action(action), device, layer, slot).code >= 0)
                        set_binding(Action(action), {device, -1}, layer, slot);
    constexpr std::array<std::pair<Action, Binding>, 19> layout{{
        {Action::A, {Device::Button, SDL_CONTROLLER_BUTTON_A}},
        {Action::B, {Device::Button, SDL_CONTROLLER_BUTTON_X}},
        {Action::CUp, {Device::Button, SDL_CONTROLLER_BUTTON_Y}},
        {Action::CDown, {Device::Button, SDL_CONTROLLER_BUTTON_B}},
        {Action::CLeft, {Device::AxisPositive, SDL_CONTROLLER_AXIS_TRIGGERRIGHT}},
        {Action::CRight, {Device::Button, SDL_CONTROLLER_BUTTON_LEFTSHOULDER}},
        {Action::Z, {Device::AxisPositive, SDL_CONTROLLER_AXIS_TRIGGERLEFT}},
        {Action::R, {Device::Button, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER}},
        {Action::L, {Device::Button, SDL_CONTROLLER_BUTTON_LEFTSTICK}},
        {Action::Start, {Device::Button, SDL_CONTROLLER_BUTTON_START}},
        {Action::Menu, {Device::Button, SDL_CONTROLLER_BUTTON_BACK}},
        {Action::DUp, {Device::Button, SDL_CONTROLLER_BUTTON_DPAD_UP}},
        {Action::DDown, {Device::Button, SDL_CONTROLLER_BUTTON_DPAD_DOWN}},
        {Action::DLeft, {Device::Button, SDL_CONTROLLER_BUTTON_DPAD_LEFT}},
        {Action::DRight, {Device::Button, SDL_CONTROLLER_BUTTON_DPAD_RIGHT}},
        {Action::MoveUp, {Device::AxisNegative, SDL_CONTROLLER_AXIS_LEFTY}},
        {Action::MoveDown, {Device::AxisPositive, SDL_CONTROLLER_AXIS_LEFTY}},
        {Action::MoveLeft, {Device::AxisNegative, SDL_CONTROLLER_AXIS_LEFTX}},
        {Action::MoveRight, {Device::AxisPositive, SDL_CONTROLLER_AXIS_LEFTX}},
    }};
    for (const auto& [action, binding] : layout)
        set_binding(action, binding, BindingLayer::Base, 0);
    set_modifier_binding({Device::Button, -1});
    if (notice == direct_c_capability_failed || notice == direct_c_create_failed)
        notice.clear();
}
void apply(Request& request) {
    using namespace tooie::input;
    // A request made for one pad never applies after selection moved to another.
    if (request.controller && *request.controller != selected_controller_instance())
        return;
    switch (request.kind) {
    case RequestKind::Controller:
        select_controller(request.value);
        break;
    case RequestKind::Profile:
        if (capture_active()) // A pending capture writes into the active profile.
            break;
        select_controller_profile(request.name);
        break;
    case RequestKind::ControllerLabels:
        if (!menu_open() || capture_active() || selected_controller_instance() < 0 ||
            request.value < 0 || request.value > int(ControllerType::SteamDeck) ||
            !offered_controller_labels(ControllerType(request.value)))
            break;
        set_selected_controller_type(ControllerType(request.value));
        break;
    case RequestKind::DuplicateProfile:
        if (!menu_open() || capture_active() || selected_controller_instance() < 0)
            break;
        if (duplicate_active_profile()) {
            if (notice == duplicate_failed)
                notice.clear();
        } else
            notice = duplicate_failed;
        break;
    case RequestKind::CreateDirectCProfile:
        if (!menu_open() || capture_active() || selected_controller_instance() < 0)
            break;
        create_direct_c_profile();
        break;
    case RequestKind::ResetBindings:
        reset_bindings();
        break;
    case RequestKind::Capture:
        if (bindable(request))
            begin_capture(request.action, request.device, request.slot, request.layer);
        break;
    case RequestKind::ClearBinding:
        if (!bindable(request) || capture_active() ||
            (request.device != Device::Keyboard && selected_controller_instance() < 0))
            break;
        if (request.device == Device::Keyboard) {
            set_binding(request.action, {Device::Keyboard, -1}, request.slot);
        } else {
            // One controller cell represents button and both axis directions.
            for (Device device : {Device::Button, Device::AxisPositive, Device::AxisNegative})
                set_binding(request.action, {device, -1}, request.layer, request.slot);
        }
        break;
    case RequestKind::Modifier:
        if (!menu_open() || capture_active() || selected_controller_instance() < 0)
            break;
        if ((request.device == Device::Button && request.value >= -1 && request.value < SDL_CONTROLLER_BUTTON_MAX) ||
            (request.device == Device::AxisPositive &&
                (request.value == SDL_CONTROLLER_AXIS_TRIGGERLEFT || request.value == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)))
            set_modifier_binding({request.device, request.value});
        break;
    case RequestKind::CancelCapture:
        cancel_capture();
        break;
    case RequestKind::Preference:
        update_preference(request.preference, request.value);
        break;
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
// suits the 800-line panel, output follows the display rate, and widescreen
// fitted to the window fills the 16:10 screen.
constexpr Preferences fresh{.resolution = 3, .output_rate = 2, .wide = 1, .native_aspect = fit_aspect, .fullscreen = 1};
#else
constexpr Preferences fresh{};
#endif
void initialize(SDL_Window* target, const std::filesystem::path& profile, bool allow_background_controller) {
    if (initialized || !target) throw std::runtime_error("Invalid baseline host initialization");
    window = target; window_id = SDL_GetWindowID(target);
    focused = (SDL_GetWindowFlags(target) & SDL_WINDOW_INPUT_FOCUS) != 0;
    tooie::frontend::settings::initialize(profile);
    using namespace tooie::frontend;
    const double rumble_strength = settings::get_number("rumble_strength", 100.0);
    selected.rumble_strength = std::isfinite(rumble_strength) ? int(std::clamp(rumble_strength, 0.0, 100.0)) : 100;
    selected.resolution = bounded(settings::get_int("baseline_resolution", fresh.resolution), 0, 8, fresh.resolution);
    selected.texture_filter = bounded(settings::get_int("baseline_texture_filter", 0), 0, 1, 0);
    selected.output_filter = bounded(settings::get_int("baseline_output_filter", 0), 0, 2, 0);
    selected.output_rate = bounded(settings::get_int("baseline_output_rate", fresh.output_rate), 0, 3, fresh.output_rate);
    selected.manual_hz = bounded(settings::get_int("baseline_manual_hz", 60), 20, 360, 60);
    selected.wide = bounded(settings::get_int("tooie_game_widescreen", fresh.wide), -1, 1, fresh.wide);
    tooie::widescreen::set_live_widescreen_request(selected.wide);
    selected.native_aspect = bounded(settings::get_int("tooie_native_aspect", fresh.native_aspect), 0, fit_aspect, fresh.native_aspect);
    selected.cutscene_aspect = bounded(settings::get_int("tooie_cutscene_aspect", 0), 0, 1, 0);
    selected.vsync = bounded(settings::get_int("baseline_vsync", -1), -1, 1, -1);
    selected.fullscreen = bounded(settings::get_int("baseline_fullscreen", fresh.fullscreen), 0, 1, fresh.fullscreen);
#if BASELINE_PLAYER
    selected.master_volume = std::clamp(settings::get_int("tooie_master_volume_percent", 100), 0, 100);
    selected.music_volume = std::clamp(settings::get_int("tooie_music_volume_percent", 100), 0, 100);
    selected.quiet_talon_trot = bounded(settings::get_int("tooie_quiet_talon_trot", 0), 0, 1, 0);
    selected.talon_trot_toggle = bounded(settings::get_int("tooie_talon_trot_toggle", 0), 0, 1, 0);
    selected.first_person_aim = bounded(settings::get_int("tooie_first_person_aim", 0), 0, 1, 0);
    selected.first_person_aim_sensitivity = bounded(
        settings::get_int("tooie_first_person_aim_sensitivity", TOOIE_FP_AIM_SENSITIVITY_DEFAULT),
        TOOIE_FP_AIM_SENSITIVITY_MIN, TOOIE_FP_AIM_SENSITIVITY_MAX, TOOIE_FP_AIM_SENSITIVITY_DEFAULT);
    selected.first_person_aim_invert = bounded(settings::get_int("tooie_first_person_aim_invert", 0), 0, 3, 0);
#else
    selected.master_volume = 100;
    selected.music_volume = 100;
    selected.quiet_talon_trot = 0;
    selected.talon_trot_toggle = 0;
    selected.first_person_aim = 0;
#endif
    latched = selected; vsync.store(selected.vsync, std::memory_order_release);
    resolution.store(selected.resolution, std::memory_order_release);
    texture_filter.store(selected.texture_filter, std::memory_order_release);
    output_filter.store(selected.output_filter, std::memory_order_release);
    cutscene_aspect.store(selected.cutscene_aspect, std::memory_order_release);
    master_volume.store(selected.master_volume, std::memory_order_release);
    music_volume.store(selected.music_volume, std::memory_order_release);
    quiet_talon_trot.store(selected.quiet_talon_trot != 0, std::memory_order_release);
    talon_trot_toggle.store(selected.talon_trot_toggle != 0, std::memory_order_release);
    output_rate.store(pack_output_rate({latched.output_rate, latched.manual_hz}), std::memory_order_release);
#if BASELINE_PLAYER
    if (selected.fullscreen) set_fullscreen(true);
#endif
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
        std::fprintf(stderr, "baseline mapper keyboard-only: %s\n", SDL_GetError());
#if BASELINE_PLAYER
    loaded_profiles = settings::get_string("tooie_input_profiles_v1", "");
#endif
    tooie::input::initialize_settings(profile, allow_background_controller);
#if BASELINE_PLAYER
    log_controller_admission("startup", 0);
#endif
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
    texture_filter.store(0, std::memory_order_release);
    output_filter.store(0, std::memory_order_release);
    cutscene_aspect.store(0, std::memory_order_release);
    master_volume.store(100, std::memory_order_release);
    music_volume.store(100, std::memory_order_release);
    quiet_talon_trot.store(false, std::memory_order_release);
    talon_trot_toggle.store(false, std::memory_order_release);
    output_rate.store(pack_output_rate({}), std::memory_order_release);
#if BASELINE_PLAYER
    loaded_profiles.clear();
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
#if BASELINE_PLAYER
    if (event.type == SDL_CONTROLLERDEVICEADDED || event.type == SDL_CONTROLLERDEVICEREMOVED)
        log_controller_admission(event.type == SDL_CONTROLLERDEVICEADDED ? "added" : "removed", 0);
#endif
    queue_event(event);
}
InputPublication tick(bool gameplay) {
    if (!initialized) throw std::runtime_error("Baseline host tick before initialization");
#if BASELINE_PLAYER
    if (auto completed = issue_evidence::poll()) {
        marker_toast = "Report #" + std::to_string(completed->marker) + (completed->success
            ? " saved: " + completed->filename + " (beside player.log)"
            : " failed: " + completed->error + ". Keep player.log.");
        marker_until = std::chrono::steady_clock::now() + std::chrono::seconds(8);
        std::fprintf(stderr, "baseline issue-report marker=%u success=%u file=%s bytes=%llu error=%s\n",
            completed->marker, unsigned(completed->success), completed->filename.c_str(),
            static_cast<unsigned long long>(completed->bytes), completed->error.c_str());
    }
#endif
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
    bool activity = tooie::input::consume_mapped_activity();
    input::State physical{mapped.buttons, mapped.x, mapped.y};
    physical.buttons |= tooie::input::consume_transient_buttons();
    float aim_x = 0, aim_y = 0;
    tooie::input::get_right_analog(0, &aim_x, &aim_y);
    // Enhanced axes participate in the existing release/script owner even
    // when a custom binding profile maps neither axis to N64 C buttons.
    activity |= selected.first_person_aim && mapped.game_enabled &&
        tooie::input::physical_controller() && (aim_x != 0 || aim_y != 0);
    if (quarantine && !activity && !physical.nonneutral()) quarantine = false;
    const bool quit = sticky_quit.load(std::memory_order_acquire);
    const bool withheld = quarantine || menu_open() || quit;
    if (withheld) physical = {};
#if BASELINE_PLAYER
    const bool aim_admitted = selected.first_person_aim && mapped.game_enabled &&
        tooie::input::physical_controller() && !withheld;
#else
    constexpr bool aim_admitted = false;
#endif
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
    snapshot->controller_auto = tooie::input::controller_auto_selection();
    snapshot->profiles = tooie::input::controller_profiles(); snapshot->active_profile = tooie::input::active_controller_profile();
    snapshot->preferences = selected; snapshot->notice = notice;
#if BASELINE_PLAYER
    issue_evidence::HostRecord evidence{};
    evidence.width = snapshot->pixel_width; evidence.height = snapshot->pixel_height;
    evidence.wide = selected.wide; evidence.aspect = selected.native_aspect;
    evidence.cutscene_aspect = selected.cutscene_aspect; evidence.resolution = selected.resolution;
    evidence.output_rate = selected.output_rate; evidence.manual_hz = selected.manual_hz;
    evidence.vsync = selected.vsync; evidence.texture_filter = selected.texture_filter;
    evidence.output_filter = selected.output_filter; evidence.published_aspect = published_live_aspect;
    evidence.guest_publication = render::publication_word(); evidence.buttons = physical.buttons;
    const uint32_t axes = pack_aim_sample(physical.x, physical.y);
    evidence.stick_x = int16_t(axes >> 16); evidence.stick_y = int16_t(axes & 0xFFFFu); // Signed Q15.
    evidence.flags = uint32_t(focused) | uint32_t(menu_open()) << 1 |
        uint32_t(snapshot->suppressed) << 2 | uint32_t(mapped.game_enabled) << 3 |
        uint32_t(withheld) << 4 | uint32_t((SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) << 5;
    issue_evidence::record(evidence); // Existing mapped state; no extra device poll.
    if (std::chrono::steady_clock::now() < marker_until) snapshot->toast = marker_toast;
#endif
    if (const auto recovery = tooie::frontend::settings::recovery_notice(); !recovery.empty()) snapshot->notice = recovery;
    if (tooie::input::saved_profiles_unreadable())
        snapshot->notice += (snapshot->notice.empty() ? "" : "\n") + std::string(profiles_unreadable);
    snapshot->controller_type = tooie::input::selected_controller_type();
    const auto fill_binding = [type = snapshot->controller_type](BindingRow& row, tooie::input::Action action,
            tooie::input::BindingLayer layer = tooie::input::BindingLayer::Base) {
        row.action = tooie::input::action_name(action);
        for (unsigned slot = 0; slot < 2; ++slot) {
            row.keyboard[slot] = tooie::input::binding_label(tooie::input::get_binding(action, tooie::input::Device::Keyboard, slot));
            for (unsigned device = 1; device <= 3; ++device) {
                const auto binding = tooie::input::get_binding(action, tooie::input::Device(device), layer, slot);
                row.controller[(device - 1) * 2 + slot] = binding.code < 0 ? std::string{} : controller_binding_label(binding, type);
            }
        }
    };
    for (unsigned i = 0; i < snapshot->bindings.size(); ++i)
        fill_binding(snapshot->bindings[i], tooie::input::Action(i));
#if BASELINE_PLAYER
    fill_binding(snapshot->fast_forward, tooie::input::Action::FastForward);
#endif
    fill_binding(snapshot->held_menu, tooie::input::Action::Menu, tooie::input::BindingLayer::Modified);
    snapshot->modifier = tooie::input::get_modifier_binding();
    snapshot->modifier_solo = tooie::input::get_modifier_solo_behavior();
    { std::lock_guard lock(render_mutex); render_input.snapshot = std::move(snapshot); }
    tooie::frontend::settings::flush(true);
    return {physical, activity, focused, quit, mapped.game_enabled,
        aim_admitted ? pack_aim_sample(aim_x, aim_y) : 0u,
        aim_admitted ? pack_first_person_aim(selected) : 0u};
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
int requested_texture_filter() noexcept { return texture_filter.load(std::memory_order_acquire); }
int requested_output_filter() noexcept { return output_filter.load(std::memory_order_acquire); }
int requested_cutscene_aspect() noexcept { return cutscene_aspect.load(std::memory_order_acquire); }
bool requested_quiet_talon_trot() noexcept { return quiet_talon_trot.load(std::memory_order_acquire); }
bool requested_talon_trot_toggle() noexcept { return talon_trot_toggle.load(std::memory_order_acquire); }
int requested_music_volume() noexcept { return music_volume.load(std::memory_order_acquire); }
int requested_master_volume() noexcept { return master_volume.load(std::memory_order_acquire); }
OutputRate requested_output_rate() noexcept { return unpack_output_rate(output_rate.load(std::memory_order_acquire)); }
}
namespace tooie::input {
const Host& host() noexcept {
#if BASELINE_PLAYER
    // The player offers only the supported 2x hold; every other utility stays
    // absent, so the mapper's null-safe wrappers cannot unlock it.
    static const ProductActions player_actions{nullptr, nullptr,
        [](bool held) noexcept {
            tooie::timing::set_fast_forward_rate(2U); // tooie_fast_forward_rate is never read here
            tooie::timing::set_fast_forward_held(held);
        }, nullptr, nullptr, nullptr, nullptr};
    static const Host value{true, baseline::host::menu_open,
        [](bool open) noexcept { baseline::host::set_menu(open); },
        []() { return baseline::host::started.load(std::memory_order_acquire); }, &player_actions};
#else
    static const Host value{true, baseline::host::menu_open,
        [](bool open) noexcept { baseline::host::set_menu(open); },
        []() { return baseline::host::started.load(std::memory_order_acquire); }, nullptr};
#endif
    return value;
}
}
