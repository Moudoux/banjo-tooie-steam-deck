#include "frontend_input_preference.hpp"
#include "frontend_input_host.hpp"
#include "frontend_input_policy.hpp"
#include "frontend_config.hpp"
#include "json/json.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <climits>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {
using namespace tooie::input;
#if defined(_WIN32)
std::string hid_product_name(const char* path) {
    if (!path || !*path) return {};
    const int path_length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, nullptr, 0);
    if (path_length <= 0) return {};
    std::wstring wide_path(static_cast<size_t>(path_length), L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1,
            wide_path.data(), path_length)) return {};

    // SDL's HID path identifies this exact controller, including duplicate VID/PID devices.
    // Request metadata only; no input or output report is sent to the device.
    HANDLE device = CreateFileW(wide_path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, 0, nullptr);
    if (device == INVALID_HANDLE_VALUE) return {};
    HMODULE hid = LoadLibraryExW(L"hid.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    using GetProductString = BOOLEAN (WINAPI*)(HANDLE, PVOID, ULONG);
    auto get_product_string = hid ? reinterpret_cast<GetProductString>(
        GetProcAddress(hid, "HidD_GetProductString")) : nullptr;
    wchar_t product[128]{};
    const bool found = get_product_string &&
        get_product_string(device, product, sizeof(product)) && product[0] != L'\0';
    if (hid) FreeLibrary(hid);
    CloseHandle(device);
    if (!found) return {};
    product[sizeof(product) / sizeof(product[0]) - 1] = L'\0';
    const int name_length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        product, -1, nullptr, 0, nullptr, nullptr);
    if (name_length <= 0) return {};
    std::string name(static_cast<size_t>(name_length), '\0');
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, product, -1,
            name.data(), name_length, nullptr, nullptr)) return {};
    name.pop_back(); // Omit the converted terminator.
    return name;
}
#endif
constexpr int baseline_digital_axis_threshold = 16384;
constexpr size_t count = static_cast<size_t>(Action::Count);
// Saved controller rows are positional: row i of every stored base and
// "modified" table holds Action(i), and decode_rows requires exactly `count`
// rows. Keyboard keys (names) and legacy controls.json names (import_legacy)
// use the same order. Inserting, removing, reordering or appending an Action
// changes what existing saves mean and needs a versioned migration first.
constexpr std::array<Action, 31> persisted_rows{{
    Action::A, Action::B, Action::Z, Action::Start, Action::L, Action::R,
    Action::CUp, Action::CDown, Action::CLeft, Action::CRight,
    Action::DUp, Action::DDown, Action::DLeft, Action::DRight,
    Action::MoveUp, Action::MoveDown, Action::MoveLeft, Action::MoveRight,
    Action::Menu, Action::SaveProgress, Action::Diagnostics, Action::IssueMarker,
    Action::FastForward, Action::SkipCutscene, Action::FreeCameraReset,
    Action::FreeCameraLeft, Action::FreeCameraRight, Action::FreeCameraUp,
    Action::FreeCameraDown, Action::FreeCameraForward, Action::FreeCameraBack}};
static_assert(count == persisted_rows.size() && [] {
    for (size_t row = 0; row < persisted_rows.size(); ++row)
        if (static_cast<size_t>(persisted_rows[row]) != row) return false;
    return true;
}(), "Action indices are persisted; change them only with a versioned profile migration");
constexpr std::array<std::pair<Action, std::uint16_t>, 14> n64_buttons{{
    {Action::A,0x8000},{Action::B,0x4000},{Action::Z,0x2000},{Action::Start,0x1000},
    {Action::DUp,0x0800},{Action::DDown,0x0400},{Action::DLeft,0x0200},{Action::DRight,0x0100},
    {Action::L,0x0020},{Action::R,0x0010},{Action::CUp,0x0008},{Action::CDown,0x0004},
    {Action::CLeft,0x0002},{Action::CRight,0x0001}}};
constexpr std::array<const char*, count> names{{
    "a","b","z","start","l","r","c_up","c_down","c_left","c_right",
    "d_up","d_down","d_left","d_right","move_up","move_down","move_left","move_right",
    "menu","save_progress","diagnostics","issue_marker","fast_forward","skip_cutscene",
    "free_camera_reset","free_camera_left","free_camera_right","free_camera_up",
    "free_camera_down","free_camera_forward","free_camera_back"
}};
constexpr std::array<const char*, count> labels{{
    "A", "B", "Z", "Start", "L", "R", "C Up", "C Down", "C Left", "C Right",
    "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right",
    "Move Up", "Move Down", "Move Left", "Move Right",
    "Open Menu", "Save Progress", "Toggle Diagnostics", "Mark Graphics Issue",
    "Fast Forward (Hold)", "Skip Cutscene", "Reset Free Camera",
    "Free Camera Left", "Free Camera Right", "Free Camera Up", "Free Camera Down",
    "Free Camera Forward", "Free Camera Back"
}};
constexpr unsigned index(Device device, unsigned slot) { return unsigned(device) * 2 + slot; }
using BindingTable = std::array<std::array<Binding, 8>, count>;
std::mutex binding_mutex;
BindingTable bindings{};
BindingTable modified_bindings{};
Binding modifier_binding{Device::Button, -1};
ModifierSoloBehavior modifier_solo = ModifierSoloBehavior::PassThrough;
int trigger_press_percent = 50, trigger_release_percent = 45;
BindingLayer scanned_layer = BindingLayer::Base;
std::array<std::array<std::array<bool, 8>, 2>, count> axis_latches{};
bool modifier_axis_latch = false;
std::array<bool, 8> z_chord_latches{};
std::map<std::pair<unsigned, int>, bool> chord_axis_latches;
std::vector<Binding> chord_suppressed_base;
std::array<bool, SDL_CONTROLLER_BUTTON_MAX> event_buttons_down{};
std::array<Sint16, SDL_CONTROLLER_AXIS_MAX> event_axes_values{};
std::map<std::pair<unsigned, int>, bool> event_axis_latches;
bool event_modifier_axis_latch = false;
SDL_JoystickID event_buttons_device = -1, event_axes_device = -1;
std::atomic_bool scanning{false};
std::atomic_bool suppress_input{false};
std::mutex capture_mutex;
Action scanned_action = Action::A;
Device scanned_device = Device::Keyboard;
unsigned scanned_slot = 0;
bool capture_armed = false;
bool capture_releasing = false;
std::atomic<SDL_JoystickID> capture_instance{-1};
std::chrono::steady_clock::time_point capture_deadline{}, release_neutral_since{};
constexpr auto capture_timeout = std::chrono::seconds(8);
constexpr auto release_cooldown = std::chrono::milliseconds(250);
struct OpenController { SDL_GameController* handle = nullptr; ControllerInfo info; };
std::mutex controller_mutex;
std::vector<OpenController> opened;
std::string wanted_guid;
std::string wanted_device_id;
SDL_JoystickID manual_instance = -1;
bool manual_required = false;
bool wanted_auto = false;
std::atomic<SDL_JoystickID> chosen{-1};
// SDL owner samples once per initialization; formatters on any thread read.
std::atomic_bool nintendo_button_labels{true};
struct DeviceRecord {
    std::string friendly_name;
    ControllerType type = ControllerType::Other;
    std::string active = "Default";
    std::map<std::string, BindingTable> profiles;
    struct ProfileExtras {
        BindingTable modified{};
        Binding modifier{Device::Button, -1};
        ModifierSoloBehavior solo = ModifierSoloBehavior::PassThrough;
        int trigger_press = 50, trigger_release = 45;
        // Stored fields this build did not read; written back until an edit owns the field.
        // A stored entry that is not an object is kept whole until an extras edit replaces it.
        nlohmann::json retained = nlohmann::json::object();
        // Stored legacy {"base", "modified"} profile whose modified layer could not be read, or
        // could not move to an opaque extras entry; written back with only "base" updated
        // until a modified-layer edit or reset owns it.
        nlohmann::json legacy_base;
    };
    std::map<std::string, ProfileExtras> extras;
    // Stored profiles this build cannot decode, by name: never listed or
    // edited, written back unchanged, and their names stay reserved.
    nlohmann::json unreadable_profiles = nlohmann::json::object();
    nlohmann::json unreadable_extras = nlohmann::json::object();
};
std::map<std::string, DeviceRecord> device_records;
// Stored device entries this build cannot save faithfully: {base, extras if stored}.
// A non-object base, or a readable base whose extras entry is not an object. Written
// back unchanged; a connected device with such an entry edits for this session only.
std::map<std::string, std::pair<nlohmann::json, std::optional<nlohmann::json>>> unreadable_devices;
// A stored string that is not a profile document is never overwritten this session.
bool profile_store_unreadable = false;
std::string active_device_id;
std::string active_profile_key;
BindingTable legacy_template{};
std::string legacy_preferred_guid;
std::map<std::string, BindingTable> legacy_profile_tables;
std::map<std::string, std::string> legacy_assignments;
std::atomic_uint64_t published_buttons{0};
std::atomic_uint32_t transient_buttons{0};
std::atomic_bool mapped_activity{false};
std::atomic<float> baseline_x{0}, baseline_y{0};
std::atomic_bool mouse_aim_wanted{true};
std::atomic<std::int64_t> first_person_until{0}; // steady_clock nanoseconds.
std::mutex mouse_aim_mutex;
float mouse_aim_x = 0, mouse_aim_y = 0; // Counts since the guest last took them.
bool mouse_aim_relative = false; // SDL owner only.
int cursor_shown = -1; // SDL owner only; -1 before the first tick.
std::int64_t steady_nanoseconds() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
std::atomic_int published_x{0}, published_y{0}, published_rx{0}, published_ry{0};
std::atomic_bool published_enabled{false}, published_physical{false}, published_rumble{false};
std::atomic_bool desired_rumble{false};
bool applied_rumble = false;
SDL_JoystickID applied_rumble_device = -1;
std::chrono::steady_clock::time_point last_rumble_update{};
MenuGamepadState published_menu_pad{};
bool last_fast_forward = false;
bool last_free_camera_reset = false;
bool last_menu_binding = false;
bool last_focused = false;
bool focus_transition_pending = false;
SDL_Window* frontend_window = nullptr; // SDL/UI thread only.
bool background_hint_initialized = false;
bool last_background_hint = false;
bool baseline_background_controller = false; // Immutable session policy, SDL owner only.
bool was_menu_open = true;
std::uint16_t suppressed_buttons = 0;
std::uint16_t suppressed_keyboard_buttons = 0;
bool suppress_move = false, suppress_camera = false;
bool suppress_keyboard_move = false;
bool suppress_fast_forward = false, suppress_free_camera_reset = false;
std::array<bool, count> last_utility_binding{};
#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT
std::atomic_bool persistent_input_reset_pending{false};
bool persistent_input_quarantine = false; // SDL/UI thread only.
#endif
std::atomic_bool diagnostics_toggle_pending{false};
std::atomic_uint32_t issue_marker_pending{0};
std::atomic_uint64_t total_keydowns{0}, relevant_keydowns{0}, escape_keydowns{0}, return_keydowns{0},
    function_keydowns{0}, accepted_save_progress{0}, accepted_diagnostics{0}, accepted_issue_markers{0},
    accepted_fast_forward{0}, accepted_cutscene_skips{0};
std::atomic_uint32_t last_scancode{0};
std::atomic_bool last_repeat{false}, last_game_started{false}, last_keyboard_focus{false},
    last_all_input_disabled{false}, last_context_capture{false}, last_binding_scan{false}, last_skip_events{false};

bool background_input_enabled() {
    return host().baseline ? baseline_background_controller :
        tooie::frontend::settings::get_bool("background_input_mode", true);
}
bool analog_camera_input_enabled() {
#ifdef TOOIE_DECK_DEFAULTS
    constexpr bool player_default = true;
#else
    constexpr bool player_default = false;
#endif
    return tooie::frontend::settings::get_bool("tooie_analog_camera_enabled", host().baseline && player_default);
}
void sync_background_joystick_events(bool enabled) {
    if (!background_hint_initialized || last_background_hint != enabled) {
        // SDL's default is off. This must be set before polling/updating the
        // controller; publishing its cached state alone cannot restore input.
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, enabled ? "1" : "0");
        last_background_hint = enabled;
        background_hint_initialized = true;
    }
}

std::string setting_key(Action action, Device device, unsigned slot) {
    return std::string("input.") + names[static_cast<size_t>(action)] + "." +
        std::to_string(unsigned(device)) + "." + std::to_string(slot);
}
void put(BindingTable& table, Action action, Device device, int code, unsigned slot = 0) {
    table[static_cast<size_t>(action)][index(device, slot)] = {device, code};
}
BindingTable defaults() {
    BindingTable table{};
    for (auto& row : table) for (unsigned i = 0; i < row.size(); ++i) row[i] = {Device(i / 2), -1};
    put(table, Action::A, Device::Keyboard, SDL_SCANCODE_J);
    put(table, Action::B, Device::Keyboard, SDL_SCANCODE_K);
    put(table, Action::Z, Device::Keyboard, SDL_SCANCODE_SPACE);
    put(table, Action::Start, Device::Keyboard, SDL_SCANCODE_RETURN);
    put(table, Action::L, Device::Keyboard, SDL_SCANCODE_Q);
    put(table, Action::R, Device::Keyboard, SDL_SCANCODE_E);
    put(table, Action::CUp, Device::Keyboard, SDL_SCANCODE_UP);
    put(table, Action::CDown, Device::Keyboard, SDL_SCANCODE_DOWN);
    put(table, Action::CLeft, Device::Keyboard, SDL_SCANCODE_LEFT);
    put(table, Action::CRight, Device::Keyboard, SDL_SCANCODE_RIGHT);
    put(table, Action::DUp, Device::Keyboard, SDL_SCANCODE_T);
    put(table, Action::DDown, Device::Keyboard, SDL_SCANCODE_G);
    put(table, Action::DLeft, Device::Keyboard, SDL_SCANCODE_F);
    put(table, Action::DRight, Device::Keyboard, SDL_SCANCODE_H);
    put(table, Action::MoveUp, Device::Keyboard, SDL_SCANCODE_W);
    put(table, Action::MoveDown, Device::Keyboard, SDL_SCANCODE_S);
    put(table, Action::MoveLeft, Device::Keyboard, SDL_SCANCODE_A);
    put(table, Action::MoveRight, Device::Keyboard, SDL_SCANCODE_D);
    put(table, Action::Menu, Device::Keyboard, SDL_SCANCODE_ESCAPE);
#ifdef TOOIE_DECK_DEFAULTS
    if (host().baseline) {
        // Xbox 360 (XBLA) layout: X attacks and B talks/cancels (both N64 B),
        // either trigger crouches, the right bumper centres the camera (the
        // left bumper is N64 L, for Hold L to levitate), Y looks
        // first-person, the right stick turns the camera (C-left/right, X axis
        // inverted) and either vertical push toggles camera views (C-down).
        // With the analog camera on, the right stick orbits on both axes and
        // the view toggle is on the right stick click instead. The D-pad is
        // the four C-buttons, so every C action (Z + C-right Wonderwing, egg
        // aiming, C-left/right turns) stays reachable with the stick orbiting;
        // Tooie reads no N64 D-pad input through its button API.
        // L, unused by XBLA, is also on LS.
        put(table, Action::A, Device::Button, SDL_CONTROLLER_BUTTON_A);
        put(table, Action::B, Device::Button, SDL_CONTROLLER_BUTTON_X);
        put(table, Action::B, Device::Button, SDL_CONTROLLER_BUTTON_B, 1);
        put(table, Action::Start, Device::Button, SDL_CONTROLLER_BUTTON_START);
        put(table, Action::Z, Device::AxisPositive, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        put(table, Action::Z, Device::AxisPositive, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 1);
        put(table, Action::L, Device::Button, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        put(table, Action::L, Device::Button, SDL_CONTROLLER_BUTTON_LEFTSTICK, 1);
        put(table, Action::R, Device::Button, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
        put(table, Action::CUp, Device::Button, SDL_CONTROLLER_BUTTON_Y);
        put(table, Action::CDown, Device::AxisPositive, SDL_CONTROLLER_AXIS_RIGHTY);
        put(table, Action::CDown, Device::AxisNegative, SDL_CONTROLLER_AXIS_RIGHTY);
        put(table, Action::CDown, Device::Button, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
        put(table, Action::CLeft, Device::AxisPositive, SDL_CONTROLLER_AXIS_RIGHTX);
        put(table, Action::CRight, Device::AxisNegative, SDL_CONTROLLER_AXIS_RIGHTX);
        put(table, Action::CUp, Device::Button, SDL_CONTROLLER_BUTTON_DPAD_UP, 1);
        put(table, Action::CDown, Device::Button, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 1);
        put(table, Action::CLeft, Device::Button, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        put(table, Action::CRight, Device::Button, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        put(table, Action::Menu, Device::Button, SDL_CONTROLLER_BUTTON_BACK);
        put(table, Action::MoveUp, Device::AxisNegative, SDL_CONTROLLER_AXIS_LEFTY);
        put(table, Action::MoveDown, Device::AxisPositive, SDL_CONTROLLER_AXIS_LEFTY);
        put(table, Action::MoveLeft, Device::AxisNegative, SDL_CONTROLLER_AXIS_LEFTX);
        put(table, Action::MoveRight, Device::AxisPositive, SDL_CONTROLLER_AXIS_LEFTX);
        if (const auto* product = host().product; product && product->set_fast_forward_held)
            put(table, Action::FastForward, Device::Keyboard, SDL_SCANCODE_F6);
        return table;
    }
#endif
    if (host().baseline) {
        put(table, Action::A, Device::Button, SDL_CONTROLLER_BUTTON_A);
        put(table, Action::B, Device::Button, SDL_CONTROLLER_BUTTON_B);
        put(table, Action::Start, Device::Button, SDL_CONTROLLER_BUTTON_START);
        put(table, Action::L, Device::Button, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        put(table, Action::R, Device::Button, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
        put(table, Action::Menu, Device::Button, SDL_CONTROLLER_BUTTON_BACK);
        put(table, Action::DUp, Device::Button, SDL_CONTROLLER_BUTTON_DPAD_UP);
        put(table, Action::DDown, Device::Button, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        put(table, Action::DLeft, Device::Button, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        put(table, Action::DRight, Device::Button, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        put(table, Action::MoveUp, Device::AxisNegative, SDL_CONTROLLER_AXIS_LEFTY);
        put(table, Action::MoveDown, Device::AxisPositive, SDL_CONTROLLER_AXIS_LEFTY);
        put(table, Action::MoveLeft, Device::AxisNegative, SDL_CONTROLLER_AXIS_LEFTX);
        put(table, Action::MoveRight, Device::AxisPositive, SDL_CONTROLLER_AXIS_LEFTX);
        put(table, Action::Z, Device::AxisPositive, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        put(table, Action::CLeft, Device::AxisNegative, SDL_CONTROLLER_AXIS_RIGHTX);
        put(table, Action::CRight, Device::AxisPositive, SDL_CONTROLLER_AXIS_RIGHTX);
        put(table, Action::CUp, Device::AxisNegative, SDL_CONTROLLER_AXIS_RIGHTY);
        put(table, Action::CDown, Device::AxisPositive, SDL_CONTROLLER_AXIS_RIGHTY);
        // Player F6 hold; inert unless the host offers set_fast_forward_held.
        if (const auto* product = host().product; product && product->set_fast_forward_held)
            put(table, Action::FastForward, Device::Keyboard, SDL_SCANCODE_F6);
        return table;
    }
    put(table, Action::Diagnostics, Device::Keyboard, SDL_SCANCODE_F3);
    put(table, Action::IssueMarker, Device::Keyboard, SDL_SCANCODE_F4);
    put(table, Action::SaveProgress, Device::Keyboard, SDL_SCANCODE_F5);
    put(table, Action::FastForward, Device::Keyboard, SDL_SCANCODE_F6);
    put(table, Action::SkipCutscene, Device::Keyboard, SDL_SCANCODE_F7);
    put(table, Action::FreeCameraReset, Device::Keyboard, SDL_SCANCODE_KP_5);
    put(table, Action::FreeCameraLeft, Device::Keyboard, SDL_SCANCODE_KP_4);
    put(table, Action::FreeCameraRight, Device::Keyboard, SDL_SCANCODE_KP_6);
    put(table, Action::FreeCameraUp, Device::Keyboard, SDL_SCANCODE_KP_9);
    put(table, Action::FreeCameraDown, Device::Keyboard, SDL_SCANCODE_KP_3);
    put(table, Action::FreeCameraForward, Device::Keyboard, SDL_SCANCODE_KP_8);
    put(table, Action::FreeCameraBack, Device::Keyboard, SDL_SCANCODE_KP_2);
    put(table, Action::A, Device::Button, SDL_CONTROLLER_BUTTON_A);
    put(table, Action::B, Device::Button, SDL_CONTROLLER_BUTTON_X);
    put(table, Action::Start, Device::Button, SDL_CONTROLLER_BUTTON_START);
    put(table, Action::L, Device::Button, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    put(table, Action::CLeft, Device::Button, SDL_CONTROLLER_BUTTON_Y);
    put(table, Action::CRight, Device::Button, SDL_CONTROLLER_BUTTON_B);
    put(table, Action::CUp, Device::Button, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    put(table, Action::CDown, Device::Button, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    put(table, Action::DUp, Device::Button, SDL_CONTROLLER_BUTTON_DPAD_UP);
    put(table, Action::DDown, Device::Button, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    put(table, Action::DLeft, Device::Button, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    put(table, Action::DRight, Device::Button, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    put(table, Action::Menu, Device::Button, SDL_CONTROLLER_BUTTON_BACK);
    put(table, Action::MoveUp, Device::AxisNegative, SDL_CONTROLLER_AXIS_LEFTY);
    put(table, Action::MoveDown, Device::AxisPositive, SDL_CONTROLLER_AXIS_LEFTY);
    put(table, Action::MoveLeft, Device::AxisNegative, SDL_CONTROLLER_AXIS_LEFTX);
    put(table, Action::MoveRight, Device::AxisPositive, SDL_CONTROLLER_AXIS_LEFTX);
    put(table, Action::Z, Device::AxisPositive, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    put(table, Action::R, Device::AxisPositive, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    put(table, Action::CLeft, Device::AxisNegative, SDL_CONTROLLER_AXIS_RIGHTX);
    put(table, Action::CRight, Device::AxisPositive, SDL_CONTROLLER_AXIS_RIGHTX);
    put(table, Action::CUp, Device::AxisNegative, SDL_CONTROLLER_AXIS_RIGHTY);
    put(table, Action::CDown, Device::AxisPositive, SDL_CONTROLLER_AXIS_RIGHTY);
    return table;
}
void reset_input_latches() {
    axis_latches = {}; z_chord_latches = {}; modifier_axis_latch = false; chord_axis_latches.clear(); chord_suppressed_base.clear();
    event_buttons_down.fill(false); event_axes_values.fill(0); event_axis_latches.clear();
    event_modifier_axis_latch = false; event_buttons_device = event_axes_device = -1;
}
void copy_controller_bindings(BindingTable& to, const BindingTable& from) {
    for (size_t a = 0; a < count; ++a)
        for (unsigned i = 2; i < 8; ++i) to[a][i] = from[a][i];
}
// Caller holds binding_mutex; profile and latch ownership stay with the caller.
void apply_profile_extras(const DeviceRecord::ProfileExtras& extra) {
    modified_bindings = extra.modified;
    modifier_binding = extra.modifier;
    modifier_solo = extra.solo;
    trigger_press_percent = extra.trigger_press;
    trigger_release_percent = extra.trigger_release;
}
bool profile_name_taken(const DeviceRecord& record, const std::string& name) {
    return record.profiles.contains(name) || record.unreadable_profiles.contains(name);
}
// An explicit edit owns these stored extras fields. A stored entry that is not
// an object cannot hold them, so the edit replaces that entry whole.
void own_fields(DeviceRecord::ProfileExtras& extra, std::initializer_list<const char*> fields) {
    if (!extra.retained.is_object()) extra.retained = nlohmann::json::object();
    for (const char* field : fields) extra.retained.erase(field);
}
DeviceRecord& record_for(const std::string& id) {
    auto& record = device_records[id];
    if (record.profiles.empty()) {
        std::string name = "Default";
        for (int n = 2; profile_name_taken(record, name); ++n) name = "Default " + std::to_string(n);
        record.profiles.emplace(std::move(name), legacy_template);
    }
    if (!record.profiles.contains(record.active)) record.active = record.profiles.begin()->first;
    if (!record.extras.contains(record.active)) {
        DeviceRecord::ProfileExtras extra;
        for (size_t a = 0; a < count; ++a) for (unsigned i = 2; i < 8; ++i)
            extra.modified[a][i] = {Device((i - 2) / 2 + 1), -1};
        record.extras.emplace(record.active, std::move(extra));
    }
    return record;
}
void save_profiles() {
    if (profile_store_unreadable) return; // Both stored strings stay exactly as loaded.
    nlohmann::json devices = nlohmann::json::object();
    nlohmann::json extra_devices = nlohmann::json::object();
    for (auto& [id, record] : device_records) {
        if (id.find("|live-instance:") != std::string::npos || unreadable_devices.contains(id)) continue;
        if (id == active_profile_key && record.profiles.contains(record.active)) {
            auto& extra = record.extras[record.active];
            extra.modified = modified_bindings;
            extra.modifier = modifier_binding;
            extra.solo = modifier_solo;
            extra.trigger_press = trigger_press_percent;
            extra.trigger_release = trigger_release_percent;
        }
        nlohmann::json profiles = nlohmann::json::object();
        nlohmann::json extras = nlohmann::json::object();
        auto encode_rows = [](const BindingTable& table) {
            nlohmann::json rows = nlohmann::json::array();
            for (const auto& row : table) {
                nlohmann::json codes = nlohmann::json::array();
                for (unsigned i = 2; i < 8; ++i)
                    codes.push_back(row[i].code);
                rows.push_back(std::move(codes));
            }
            return rows;
        };
        for (const auto& [name, table] : record.profiles) {
            const auto extra_it = record.extras.find(name);
            const DeviceRecord::ProfileExtras extra = extra_it == record.extras.end()
                ? DeviceRecord::ProfileExtras{}
                : extra_it->second;
            // Alpha 3 and earlier read these arrays directly; keep this schema stable.
            if (extra.legacy_base.is_null())
                profiles[name] = encode_rows(table);
            else
                (profiles[name] = extra.legacy_base)["base"] = encode_rows(table);
            if (!extra.retained.is_object()) {
                extras[name] = extra.retained;
                continue;
            }
            extras[name] = {
                {"modified", encode_rows(extra.modified)},
                {"modifier", {unsigned(extra.modifier.device), extra.modifier.code}},
                {"solo", unsigned(extra.solo)},
                {"trigger_press", extra.trigger_press},
                {"trigger_release", extra.trigger_release}
            };
            extras[name].update(extra.retained);
        }
        profiles.update(record.unreadable_profiles);
        extras.update(record.unreadable_extras);
        devices[id] = {
            {"friendly_name", record.friendly_name},
            {"type", unsigned(record.type)},
            {"active", record.active},
            {"profiles", std::move(profiles)}
        };
        extra_devices[id] = std::move(extras);
    }
    for (const auto& [id, stored] : unreadable_devices) {
        devices[id] = stored.first;
        if (stored.second) extra_devices[id] = *stored.second;
    }
    tooie::frontend::settings::set_string("tooie_input_profiles_v1",
        nlohmann::json({{"devices", devices}}).dump());
    tooie::frontend::settings::set_string("tooie_input_profile_extras_v1",
        nlohmann::json({{"devices", extra_devices}}).dump());
}
// Stored-schema integer read: values outside [lo, hi] never wrap into a valid code. Requires hi >= 0.
bool stored_int(const nlohmann::json& value, int lo, int hi, int& out) {
    if (!value.is_number_integer()) return false;
    if (value.is_number_unsigned() && value.get<std::uint64_t>() > std::uint64_t(hi)) return false;
    const auto checked = value.get<std::int64_t>();
    if (checked < lo || checked > hi) return false;
    out = int(checked);
    return true;
}
void load_profiles() {
    device_records.clear();
    unreadable_devices.clear();
    profile_store_unreadable = false;
    try {
        // Both strings are checked even when the base is empty: either one that
        // cannot be read stays unchanged, including through startup legacy import.
        const auto source = tooie::frontend::settings::get_string("tooie_input_profiles_v1", "");
        const auto extra_source = tooie::frontend::settings::get_string("tooie_input_profile_extras_v1", "");
        const auto doc = source.empty() ? nlohmann::json::object() : nlohmann::json::parse(source, nullptr, false);
        const auto extra_doc = extra_source.empty() ? nlohmann::json::object() :
            nlohmann::json::parse(extra_source, nullptr, false);
        const auto unreadable = [](const nlohmann::json& stored) {
            return !stored.is_object() || (stored.contains("devices") && !stored["devices"].is_object());
        };
        profile_store_unreadable = unreadable(doc) || unreadable(extra_doc);
        if (profile_store_unreadable || !doc.contains("devices")) return;
        const auto extra_devices = extra_doc.value("devices", nlohmann::json::object());
        auto decode_rows = [](const nlohmann::json& rows, BindingTable& table) {
            if (!rows.is_array() || rows.size() != count) return false;
            BindingTable decoded = table;
            for (size_t a = 0; a < count; ++a) {
                const auto& row = rows[a];
                if (!row.is_array() || row.size() != 6) return false;
                for (unsigned i = 2; i < 8; ++i) {
                    int code;
                    if (!stored_int(row[i - 2], -1, 255, code)) return false;
                    decoded[a][i].code = code;
                }
            }
            table = std::move(decoded);
            return true;
        };
        for (auto it = doc["devices"].begin(); it != doc["devices"].end(); ++it) {
            const auto& entry = it.value();
            const auto stored_extras = extra_devices.find(it.key());
            const auto profiles = entry.find("profiles");
            const auto keep_stored = [&] {
                unreadable_devices.emplace(it.key(), std::pair{entry, stored_extras == extra_devices.end()
                    ? std::nullopt : std::optional<nlohmann::json>(*stored_extras)});
            };
            if (!entry.is_object() || (profiles != entry.end() && !profiles->is_object())) {
                keep_stored();
                continue;
            }
            // Readable mappings stay usable; the device's opaque extras entry is never overwritten.
            if (stored_extras != extra_devices.end() && !stored_extras->is_object())
                keep_stored();
            DeviceRecord record;
            if (const auto name = entry.find("friendly_name"); name != entry.end() && name->is_string())
                record.friendly_name = name->get<std::string>();
            int type;
            if (entry.contains("type") && stored_int(entry["type"], 0, int(ControllerType::SteamDeck), type))
                record.type = ControllerType(type);
            if (const auto active = entry.find("active"); active != entry.end() && active->is_string())
                record.active = active->get<std::string>();
            const auto device_extras = stored_extras != extra_devices.end() && stored_extras->is_object()
                ? *stored_extras : nlohmann::json::object();
            if (profiles != entry.end())
                for (auto profile = profiles->begin(); profile != profiles->end(); ++profile) {
                    BindingTable base = legacy_template;
                    DeviceRecord::ProfileExtras extra;
                    for (size_t a = 0; a < count; ++a)
                        for (unsigned i = 2; i < 8; ++i)
                            extra.modified[a][i] = {Device((i - 2) / 2 + 1), -1};
                    const auto stored = device_extras.find(profile.key());
                    const bool opaque_extras = stored != device_extras.end() && !stored->is_object();
                    bool valid = false;
                    if (profile.value().is_array()) {
                        // Schema 1 stored only the base layer as rows of six codes.
                        valid = decode_rows(profile.value(), base);
                    } else if (profile.value().is_object() && profile.value().contains("base")) {
                        // Read the short-lived development shape defensively, then write Alpha 3 compatible arrays.
                        // Embedded modified data that cannot move to the extras entry (unreadable here,
                        // or that entry is opaque and written back whole) keeps this stored form instead.
                        valid = decode_rows(profile.value()["base"], base);
                        if (valid && profile.value().contains("modified") &&
                            (!decode_rows(profile.value()["modified"], extra.modified) || opaque_extras))
                            extra.legacy_base = profile.value();
                    }
                    if (!valid || profile.key().empty()) {
                        record.unreadable_profiles[profile.key()] = profile.value();
                        if (stored != device_extras.end()) record.unreadable_extras[profile.key()] = *stored;
                        continue;
                    }
                    if (opaque_extras) {
                        extra.retained = *stored; // Opaque: base mappings stay usable, defaults apply.
                    } else if (stored != device_extras.end()) {
                        // Each field read here moves into this build's model; the rest stays as stored.
                        auto& fields = extra.retained = *stored;
                        if (fields.contains("modified") && decode_rows(fields["modified"], extra.modified))
                            fields.erase("modified");
                        int d, code, solo, press, release;
                        if (const auto modifier = fields.find("modifier"); modifier != fields.end() &&
                            modifier->is_array() && modifier->size() == 2 &&
                            stored_int((*modifier)[0], int(Device::Button), int(Device::AxisNegative), d) &&
                            stored_int((*modifier)[1], -1, 255, code)) {
                            extra.modifier = {Device(d), code};
                            fields.erase(modifier);
                        }
                        if (fields.contains("solo") &&
                            stored_int(fields["solo"], 0, int(ModifierSoloBehavior::ModifierOnly), solo)) {
                            extra.solo = ModifierSoloBehavior(solo);
                            fields.erase("solo");
                        }
                        // Checked before narrowing; stored integers keep the existing clamps.
                        if (fields.contains("trigger_press") &&
                            stored_int(fields["trigger_press"], INT_MIN, INT_MAX, press)) {
                            extra.trigger_press = std::clamp(press, 1, 100);
                            fields.erase("trigger_press");
                        }
                        if (fields.contains("trigger_release") &&
                            stored_int(fields["trigger_release"], INT_MIN, INT_MAX, release)) {
                            extra.trigger_release = release;
                            fields.erase("trigger_release");
                        }
                        extra.trigger_release = std::clamp(extra.trigger_release, 0, extra.trigger_press - 1);
                    }
                    record.profiles.emplace(profile.key(), std::move(base));
                    record.extras.emplace(profile.key(), std::move(extra));
                }
            if (!record.profiles.empty() || !record.unreadable_profiles.empty())
                device_records.emplace(it.key(), std::move(record));
        }
    } catch (const std::exception&) {
        // Fail closed: nothing from this load is written over the stored strings.
        device_records.clear();
        unreadable_devices.clear();
        profile_store_unreadable = true;
    }
}
void activate_device(const ControllerInfo* info) {
    std::string next_profile_key = info ? info->stable_id : "";
    if (info && info->ambiguous)
        next_profile_key += "|live-instance:" + std::to_string(info->instance);

    if (next_profile_key != active_profile_key)
        reset_input_latches();

    std::lock_guard lock(binding_mutex);
    active_device_id = info ? info->stable_id : "";
    active_profile_key = next_profile_key;

    if (info && info->ambiguous && !device_records.contains(active_profile_key))
        device_records.emplace(active_profile_key, record_for(active_device_id));

    if (!active_profile_key.empty()) {
        auto& record = record_for(active_profile_key);
        copy_controller_bindings(bindings, record.profiles.at(record.active));

        const auto& extra = record.extras.at(record.active);
        apply_profile_extras(extra);
    } else {
        copy_controller_bindings(bindings, legacy_template);
        modified_bindings = {};
        modifier_binding = {Device::Button, -1};
        modifier_solo = ModifierSoloBehavior::PassThrough;
        trigger_press_percent = 50;
        trigger_release_percent = 45;
    }
}

// SDL owner, with controller_mutex held: stop the old handle before it can close.
void stop_rumble() {
    const auto it = std::find_if(opened.begin(), opened.end(), [](const auto& entry) {
        return entry.info.instance == applied_rumble_device;
    });
    if (applied_rumble && it != opened.end())
        SDL_GameControllerRumble(it->handle, 0, 0, 0);
    applied_rumble = false;
    applied_rumble_device = -1;
    last_rumble_update = {};
    desired_rumble.store(false, std::memory_order_release);
}
void choose_controller() {
    std::sort(opened.begin(), opened.end(), [](const auto& a, const auto& b) {
        return std::tie(a.info.guid, a.info.instance) < std::tie(b.info.guid, b.info.instance);
    });

    for (auto& entry : opened)
        entry.info.ambiguous = std::count_if(opened.begin(), opened.end(), [&](const auto& other) {
            return other.info.stable_id == entry.info.stable_id;
        }) > 1;

    auto it = manual_instance < 0 ? opened.end() :
        std::find_if(opened.begin(), opened.end(), [](const auto& entry) {
            return entry.info.instance == manual_instance;
        });

    if (it == opened.end() && !manual_required && !wanted_device_id.empty()) {
        const auto matches = std::count_if(opened.begin(), opened.end(), [](const auto& entry) {
            return entry.info.stable_id == wanted_device_id;
        });
        if (matches == 1)
            it = std::find_if(opened.begin(), opened.end(), [](const auto& entry) {
                return entry.info.stable_id == wanted_device_id;
            });
    }

    if (it == opened.end() && !manual_required && wanted_device_id.empty() && !wanted_guid.empty()) {
        const auto matches = std::count_if(opened.begin(), opened.end(), [](const auto& entry) {
            return entry.info.guid == wanted_guid;
        });
        if (matches == 1)
            it = std::find_if(opened.begin(), opened.end(), [](const auto& entry) {
                return entry.info.guid == wanted_guid;
            });
    }

    if (it == opened.end() && !manual_required && wanted_device_id.empty() &&
        wanted_guid.empty() && opened.size() == 1)
        it = opened.begin();

    if (it == opened.end() && !manual_required && wanted_auto) {
        const auto previous = chosen.load(std::memory_order_acquire);
        it = std::find_if(opened.begin(), opened.end(), [previous](const auto& entry) {
            return entry.info.instance == previous;
        });
        if (it == opened.end() && !opened.empty())
            it = opened.begin();
    }

    const auto selected = it == opened.end() ? -1 : it->info.instance;
    if (selected != chosen.load(std::memory_order_acquire)) stop_rumble();
    chosen.store(selected, std::memory_order_release);
    for (auto& entry : opened)
        entry.info.selected = entry.info.instance == selected;

    published_physical.store(selected >= 0, std::memory_order_release);
    published_rumble.store(it != opened.end() && it->info.rumble, std::memory_order_release);
    activate_device(it == opened.end() ? nullptr : &it->info);
}
void connect(int device_index) {
    if (!SDL_IsGameController(device_index)) return;
    auto* handle = SDL_GameControllerOpen(device_index);
    if (!handle) return;
    auto* joystick = SDL_GameControllerGetJoystick(handle);
    if (!joystick) { SDL_GameControllerClose(handle); return; }
    const auto instance = SDL_JoystickInstanceID(joystick);
    if (std::any_of(opened.begin(), opened.end(), [instance](const auto& e) { return e.info.instance == instance; })) {
        SDL_GameControllerClose(handle); return;
    }
    Uint16 vendor = 0, product = 0, version = 0, crc16 = 0;
    SDL_GetJoystickGUIDInfo(SDL_JoystickGetGUID(joystick), &vendor, &product, &version, &crc16);
    const char* serial = SDL_JoystickGetSerial(joystick);
    const char* path = SDL_JoystickPath(joystick);
    char guid_text[33]{};
    SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joystick), guid_text, sizeof(guid_text));
    // Keep the saved preferred-controller identity format used by older profiles.
    const std::string guid = "SERIAL_" + std::string(serial ? serial : "") +
        "_VID_" + std::to_string(vendor) + "_PID_" + std::to_string(product) +
        "_VERSION_" + std::to_string(version) + "_CRC16_" + std::to_string(crc16);
    const char* hardware_name = SDL_JoystickName(joystick);
    const char* mapping_name = SDL_GameControllerName(handle);
    const std::string serial_text = serial ? serial : "";
    const std::string path_text = path ? path : "";
    std::string device_name;
#if defined(_WIN32)
    device_name = hid_product_name(path);
#endif
    if (device_name.empty()) device_name = hardware_name ? hardware_name : "Controller";
    const std::string stable = std::string(guid_text) + ":" + std::to_string(vendor) + ":" +
        std::to_string(product) + (serial_text.empty() ? (path_text.empty() ? "" : ":path:" + path_text)
            : ":serial:" + serial_text);
    ControllerInfo info{instance, guid, std::move(device_name), false,
        SDL_JoystickHasRumble(joystick) == SDL_TRUE};
    info.stable_id = stable; info.serial = serial_text; info.path = path_text;
    info.mapping_name = mapping_name ? mapping_name : "";
    info.sdl_guid = guid_text;
    info.vendor = vendor; info.product = product;
    {
        std::lock_guard lock(binding_mutex);
        if (!device_records.contains(stable) && !legacy_profile_tables.empty()) {
            DeviceRecord migrated;
            BindingTable initial=legacy_template;
            // Only the old preferred device receives the canonical input.*
            // overlay. Other known devices retain their own old assignment (or
            // the shared mapping), rather than inheriting the preferred pad.
            if (!legacy_preferred_guid.empty() && guid!=legacy_preferred_guid) {
                const auto assignment=legacy_assignments.find(guid);
                const auto selected=assignment==legacy_assignments.end()
                    ? legacy_profile_tables.end():legacy_profile_tables.find(assignment->second);
                const auto shared=legacy_profile_tables.find("controller_sp");
                initial=selected!=legacy_profile_tables.end()?selected->second:
                    shared!=legacy_profile_tables.end()?shared->second:defaults();
            }
            migrated.profiles.emplace("Default",std::move(initial));
            migrated.extras.emplace("Default", DeviceRecord::ProfileExtras{});
            for (const auto& [key, table] : legacy_profile_tables) {
                if (key == "controller_sp") continue;
                const auto profile_name = key == "Default" ? "Default (legacy)" : key;
                migrated.profiles.emplace(profile_name, table);
                migrated.extras.emplace(profile_name, DeviceRecord::ProfileExtras{});
            }
            device_records.emplace(stable, std::move(migrated));
            save_profiles();
        }
        const auto& configured_name = record_for(stable).friendly_name;
        info.friendly_name = configured_name.empty() ? info.name : configured_name;
    }
    opened.push_back({handle, std::move(info)});
    choose_controller();
}
SDL_GameController* selected_controller() {
    const auto wanted = chosen.load(std::memory_order_acquire);
    const auto it = std::find_if(opened.begin(), opened.end(), [wanted](const auto& e) { return e.info.instance == wanted; });
    return it == opened.end() ? nullptr : it->handle;
}
bool same_binding(Binding a, Binding b) { return a.device == b.device && a.code == b.code; }
bool digital_axis_down(int magnitude, bool was_latched) {
    const int press = trigger_press_percent * 32767 / 100;
    const int release = trigger_release_percent * 32767 / 100;
    if (host().baseline) {
        return magnitude > baseline_digital_axis_threshold;
    }
    if (was_latched) {
        return magnitude > release;
    }
    return magnitude >= press;
}
bool physical_binding_down(Binding binding, SDL_GameController* controller, bool& latched) {
    if (!controller || binding.code < 0) return false;
    if (binding.device == Device::Button && binding.code < SDL_CONTROLLER_BUTTON_MAX)
        return SDL_GameControllerGetButton(controller, SDL_GameControllerButton(binding.code)) != 0;
    if ((binding.device == Device::AxisPositive || binding.device == Device::AxisNegative) &&
        binding.code < SDL_CONTROLLER_AXIS_MAX) {
        const int value = SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis(binding.code));
        const int magnitude = binding.device == Device::AxisPositive ? value : -value;
        latched = digital_axis_down(magnitude, latched);
        return latched;
    }
    return false;
}
bool down(const BindingTable& table, Action action, const Uint8* keys, SDL_GameController* controller,
    BindingLayer layer = BindingLayer::Base, const std::vector<Binding>* suppressed = nullptr,
    bool analog_camera_enabled = false) {
    const auto& row = table[static_cast<size_t>(action)];
    for (unsigned slot = 0; slot < row.size(); ++slot) {
        const Binding binding = row[slot];
        if (binding.code < 0) continue;
        if (suppressed && std::any_of(suppressed->begin(), suppressed->end(),
                [&](Binding blocked) { return same_binding(binding, blocked); })) continue;
        if (analog_camera_enabled &&
            (action == Action::CUp || action == Action::CDown || action == Action::CLeft || action == Action::CRight) &&
            (binding.device == Device::AxisPositive || binding.device == Device::AxisNegative) &&
            (binding.code == SDL_CONTROLLER_AXIS_RIGHTX || binding.code == SDL_CONTROLLER_AXIS_RIGHTY)) continue;
        switch (binding.device) {
        case Device::Keyboard:
            if (keys && binding.code < SDL_NUM_SCANCODES && keys[binding.code]) return true;
            break;
        case Device::Button:
            if (controller && binding.code < SDL_CONTROLLER_BUTTON_MAX &&
                SDL_GameControllerGetButton(controller, SDL_GameControllerButton(binding.code))) return true;
            break;
        case Device::AxisPositive:
        case Device::AxisNegative:
            if (controller && binding.code < SDL_CONTROLLER_AXIS_MAX) {
                const auto value = SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis(binding.code));
                bool& latched = axis_latches[static_cast<size_t>(action)][static_cast<size_t>(layer)][slot];
                const int magnitude = binding.device == Device::AxisPositive ? value : -value;
                latched = digital_axis_down(magnitude, latched);
                if (latched) return true;
            }
            break;
        }
    }
    return false;
}
// Baseline precedence for a shared input: gameplay keeps it and fast-forward
// is withheld, as the F4 marker is. Bindings themselves are never changed.
bool fast_forward_shares_gameplay_binding(const BindingTable& table) {
    const auto& fast = table[static_cast<size_t>(Action::FastForward)];
    for (size_t action = 0; action <= static_cast<size_t>(Action::Menu); ++action)
        for (const Binding binding : table[action])
            if (binding.code >= 0 && std::any_of(fast.begin(), fast.end(),
                    [&](Binding held) { return same_binding(held, binding); })) return true;
    return false;
}
// XBLA talon trot: with Z on two controller inputs (LT and RT), holding one
// and pressing the other is the N64's Z + C-left.
bool z_chord_down(const BindingTable& table, SDL_GameController* controller, const std::vector<Binding>& suppressed) {
    const auto& row = table[static_cast<size_t>(Action::Z)];
    unsigned held = 0;
    for (unsigned slot = 0; slot < row.size(); ++slot) {
        const Binding binding = row[slot];
        if (binding.device == Device::Keyboard || std::any_of(suppressed.begin(), suppressed.end(),
                [&](Binding blocked) { return same_binding(binding, blocked); }) ||
            std::any_of(row.begin(), row.begin() + slot, [&](Binding prior) { return same_binding(prior, binding); })) {
            z_chord_latches[slot] = false;
            continue;
        }
        if (physical_binding_down(binding, controller, z_chord_latches[slot])) ++held;
    }
    return held >= 2;
}
float axis(SDL_GameController* controller, SDL_GameControllerAxis which, float deadzone) {
    if (!controller) return 0;
    const int raw = SDL_GameControllerGetAxis(controller, which);
    if (host().baseline) return baseline_axis(raw);
    const float value = std::clamp(float(raw) / 32767.0f, -1.0f, 1.0f);
    return std::abs(value) < deadzone ? 0.0f : value;
}
float direction(const BindingTable& table, Action action, const Uint8* keys,
    SDL_GameController* controller, float deadzone, const std::vector<Binding>* suppressed = nullptr) {
    float amount = 0;
    for (const auto binding : table[static_cast<size_t>(action)]) {
        if (binding.code < 0 || (suppressed && std::any_of(suppressed->begin(), suppressed->end(),
                [&](Binding blocked) { return same_binding(binding, blocked); }))) continue;
        if (binding.device == Device::Keyboard && keys && binding.code < SDL_NUM_SCANCODES && keys[binding.code])
            amount = 1;
        else if (binding.device == Device::Button && controller && binding.code < SDL_CONTROLLER_BUTTON_MAX &&
            SDL_GameControllerGetButton(controller, SDL_GameControllerButton(binding.code))) amount = 1;
        else if (controller && (binding.device == Device::AxisPositive || binding.device == Device::AxisNegative) &&
            binding.code < SDL_CONTROLLER_AXIS_MAX) {
            const float value = axis(controller, SDL_GameControllerAxis(binding.code), deadzone);
            if (binding.device == Device::AxisPositive) amount = std::max(amount, value);
            else amount = std::max(amount, -value);
        }
    }
    return amount;
}
bool movement_held(const BindingTable& table, const Uint8* keys, SDL_GameController* controller,
    float deadzone, const std::vector<Binding>* suppressed = nullptr) {
    return direction(table, Action::MoveRight, keys, controller, deadzone, suppressed) > 0 ||
        direction(table, Action::MoveLeft, keys, controller, deadzone, suppressed) > 0 ||
        direction(table, Action::MoveUp, keys, controller, deadzone, suppressed) > 0 ||
        direction(table, Action::MoveDown, keys, controller, deadzone, suppressed) > 0;
}
void import_legacy(const std::filesystem::path& root, const std::string& preferred, BindingTable& table) {
    std::ifstream source(root / "controls.json");
    if (!source) return;
    try {
        nlohmann::json doc; source >> doc;
        constexpr std::array<const char*, count> legacy_names{{
            "A","B","Z","START","L","R","C_UP","C_DOWN","C_LEFT","C_RIGHT",
            "DPAD_UP","DPAD_DOWN","DPAD_LEFT","DPAD_RIGHT",
            "Y_AXIS_POS","Y_AXIS_NEG","X_AXIS_NEG","X_AXIS_POS",
            "TOGGLE_MENU","SAVE_PROGRESS","TOGGLE_DIAGNOSTICS","MARK_ISSUE","FAST_FORWARD","SKIP_CUTSCENE",
            "RESET_FREE_CAMERA","FREE_CAMERA_LEFT","FREE_CAMERA_RIGHT","FREE_CAMERA_UP",
            "FREE_CAMERA_DOWN","FREE_CAMERA_FORWARD","FREE_CAMERA_BACK"}};
        auto apply_profile = [&](const nlohmann::json& mappings, bool keyboard, BindingTable& target) {
            if (!mappings.is_object()) return;
            for (size_t a = 0; a < count; ++a) {
                auto it = mappings.find(legacy_names[a]);
                if (it == mappings.end() || !it->is_array()) continue;
                for (unsigned d = keyboard ? 0 : 1; d < (keyboard ? 1U : 4U); ++d)
                    for (unsigned s = 0; s < 2; ++s) put(target, Action(a), Device(d), -1, s);
                for (unsigned s = 0; s < std::min<size_t>(2, it->size()); ++s) {
                    const auto& field = (*it)[s];
                    if (!field.is_object()) continue;
                    int type = 0, id = -1;
                    if (!field.contains("input_type") || !field.contains("input_id") ||
                        !stored_int(field["input_type"], 1, 4, type) ||
                        !stored_int(field["input_id"], -int(SDL_CONTROLLER_AXIS_MAX), int(SDL_NUM_SCANCODES) - 1, id))
                        continue;
                    if (keyboard && type == 1 && id >= 0) put(target, Action(a), Device::Keyboard, id, s);
                    else if (!keyboard && type == 3 && id >= 0 && id < SDL_CONTROLLER_BUTTON_MAX)
                        put(target, Action(a), Device::Button, id, s);
                    else if (!keyboard && type == 4 && id != 0 && id <= SDL_CONTROLLER_AXIS_MAX) put(target, Action(a),
                        id > 0 ? Device::AxisPositive : Device::AxisNegative, std::abs(id) - 1, s);
                }
            }
        };
        if (doc.contains("profiles") && doc["profiles"].is_array()) {
            for (const auto& profile : doc["profiles"]) {
                const auto key = profile.value("key", std::string{});
                if (key == "keyboard_sp") apply_profile(profile.value("mappings", nlohmann::json::object()), true, table);
                if (key == "controller_sp") apply_profile(profile.value("mappings", nlohmann::json::object()), false, table);
                if (key != "keyboard_sp" && !key.empty()) {
                    BindingTable migrated = defaults();
                    apply_profile(profile.value("mappings", nlohmann::json::object()), false, migrated);
                    legacy_profile_tables.emplace(key, std::move(migrated));
                }
            }
            // A device assigned its own profile in the old menu overrides the
            // shared single-player controller map for the saved preferred GUID.
            if (doc.contains("controllers") && doc["controllers"].is_array()) {
                std::string assigned_profile;
                for (const auto& controller : doc["controllers"]) {
                    if (!controller.is_object() || !controller.contains("guid") ||
                        !controller["guid"].is_object()) continue;
                    const auto& guid = controller["guid"];
                    // SDL supplies Uint16 identity parts; omitted fields retain their zero defaults.
                    int vendor = 0, product = 0, version = 0, crc16 = 0;
                    const auto identity_part = [&](const char* name, int& out) {
                        const auto field = guid.find(name);
                        return field == guid.end() || stored_int(*field, 0, 0xFFFF, out);
                    };
                    if (!identity_part("vendor", vendor) || !identity_part("product", product) ||
                        !identity_part("version", version) || !identity_part("crc16", crc16)) continue;
                    const std::string identity = "SERIAL_" + guid.value("serial", std::string{}) +
                        "_VID_" + std::to_string(vendor) +
                        "_PID_" + std::to_string(product) +
                        "_VERSION_" + std::to_string(version) +
                        "_CRC16_" + std::to_string(crc16);
                    const auto profile_key = controller.value("profile", std::string{});
                    if (!profile_key.empty()) legacy_assignments.emplace(identity, profile_key);
                    if (identity == preferred) assigned_profile = profile_key;
                }
                if (!preferred.empty() && !assigned_profile.empty())
                    for (const auto& profile : doc["profiles"])
                        if (profile.is_object() && profile.value("key", std::string{}) == assigned_profile)
                            apply_profile(profile.value("mappings", nlohmann::json::object()), false, table);
            }
        } else {
            if (doc.contains("keyboard")) apply_profile(doc["keyboard"], true, table);
            if (doc.contains("controller")) apply_profile(doc["controller"], false, table);
        }
    } catch (const std::exception&) {
        // The old file is untouched; a later error can leave earlier mappings applied.
    }
}
void dispatch(Action action) {
    const auto* product = host().product;
    if (product && product->dispatch) product->dispatch(action);
}
void set_fast_forward_rate(std::uint32_t rate) noexcept {
    const auto* product = host().product;
    if (product && product->set_fast_forward_rate) product->set_fast_forward_rate(rate);
}
void set_fast_forward_held(bool held) noexcept {
    const auto* product = host().product;
    if (product && product->set_fast_forward_held) product->set_fast_forward_held(held);
}
bool free_camera_enabled() noexcept {
    const auto* product = host().product;
    return product && product->free_camera_enabled && product->free_camera_enabled();
}
void set_free_camera_input_active(bool active) noexcept {
    const auto* product = host().product;
    if (product && product->set_free_camera_input_active) product->set_free_camera_input_active(active);
}
void set_free_camera_motion(float x, float y, float z) noexcept {
    const auto* product = host().product;
    if (product && product->set_free_camera_motion) product->set_free_camera_motion(x, y, z);
}
void request_free_camera_reset() noexcept {
    const auto* product = host().product;
    if (product && product->request_free_camera_reset) product->request_free_camera_reset();
}

#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT
void apply_persistent_input_reset() noexcept {
    // This is the SDL/UI thread. Cross-thread restore only publishes the
    // request and zeroes atomic gameplay output; it never touches SDL state.
    if (!persistent_input_reset_pending.exchange(false, std::memory_order_acq_rel))
        return;
    persistent_input_quarantine = true;
    {
        std::lock_guard lock(capture_mutex);
        scanning.store(false, std::memory_order_release);
        capture_releasing = capture_armed = false;
        release_neutral_since = {};
        suppress_input.store(false, std::memory_order_release);
    }
    transient_buttons.store(0, std::memory_order_release);
    published_buttons.store(0, std::memory_order_release);
    published_x.store(0, std::memory_order_release);
    published_y.store(0, std::memory_order_release);
    published_rx.store(0, std::memory_order_release);
    published_ry.store(0, std::memory_order_release);
    published_enabled.store(false, std::memory_order_release);
    published_menu_pad = {};
    last_menu_binding = false;
    last_focused = false;
    focus_transition_pending = true;
    was_menu_open = true;
    suppressed_buttons = suppressed_keyboard_buttons = 0;
    suppress_move = suppress_camera = suppress_keyboard_move = false;
    suppress_fast_forward = suppress_free_camera_reset = true;
    last_utility_binding.fill(false);
    last_fast_forward = last_free_camera_reset = false;
    set_fast_forward_held(false);
    set_free_camera_input_active(false);
    set_free_camera_motion(0, 0, 0);
}
#endif
}

namespace tooie::input {
const char* action_name(Action action) noexcept {
    const auto i = static_cast<size_t>(action);
    return i < labels.size() ? labels[i] : "Unknown";
}
Binding get_binding(Action action, Device device, unsigned slot) {
    return get_binding(action, device, BindingLayer::Base, slot);
}
void set_binding(Action action, Binding binding, unsigned slot) {
    set_binding(action, binding, BindingLayer::Base, slot);
}
Binding get_binding(Action action, Device device, BindingLayer layer, unsigned slot) {
    if (action >= Action::Count || slot > 1) return {device, -1};
    std::lock_guard lock(binding_mutex);
    const auto& table = layer == BindingLayer::Modified ? modified_bindings : bindings;
    return table[static_cast<size_t>(action)][index(device, slot)];
}
void set_binding(Action action, Binding binding, BindingLayer layer, unsigned slot) {
    if (action >= Action::Count || slot > 1 || binding.device == Device::Keyboard && layer == BindingLayer::Modified) return;
    {
        std::lock_guard lock(binding_mutex);
        auto& table = layer == BindingLayer::Modified ? modified_bindings : bindings;
        table[static_cast<size_t>(action)][index(binding.device, slot)] = binding;
        reset_input_latches();
        if (binding.device != Device::Keyboard && !active_profile_key.empty()) {
            auto& record = record_for(active_profile_key);
            auto& saved = record.profiles.at(record.active);
            auto& extra = record.extras.at(record.active);
            (layer == BindingLayer::Modified ? extra.modified : saved)[static_cast<size_t>(action)][index(binding.device, slot)] = binding;
            if (layer == BindingLayer::Modified) {
                own_fields(extra, {"modified"});
                extra.legacy_base = nullptr;
            }
            save_profiles();
        }
    }
    if (binding.device == Device::Keyboard && layer == BindingLayer::Base)
        tooie::frontend::settings::set_int(setting_key(action, binding.device, slot), binding.code);
}
void reset_bindings() {
    auto next = defaults();
    {
        std::lock_guard lock(binding_mutex);
        bindings = next;
        reset_input_latches();
        if (!active_profile_key.empty()) {
            auto& record = record_for(active_profile_key);
            record.profiles.at(record.active) = next;
            auto& extra = record.extras.at(record.active);
            extra.modified = {}; extra.modifier = {Device::Button, -1};
            extra.solo = ModifierSoloBehavior::PassThrough; extra.trigger_press = 50; extra.trigger_release = 45;
            extra.retained = nlohmann::json::object(); extra.legacy_base = nullptr;
            modified_bindings = extra.modified; modifier_binding = extra.modifier;
            modifier_solo = extra.solo; trigger_press_percent = 50; trigger_release_percent = 45;
            save_profiles();
        }
    }
    for (size_t a = 0; a < count; ++a)
        for (unsigned s = 0; s < 2; ++s)
            tooie::frontend::settings::set_int(setting_key(Action(a), Device::Keyboard, s),
                next[a][index(Device::Keyboard, s)].code);
}
std::string binding_label(Binding binding) {
    if (binding.code < 0) return "Unbound";
    if (binding.device == Device::Keyboard) {
        // UI may ask from the render thread, so do not call SDL name helpers here.
        switch (binding.code) {
        case SDL_SCANCODE_ESCAPE: return "Escape";
        case SDL_SCANCODE_RETURN: return "Enter";
        case SDL_SCANCODE_SPACE: return "Space";
        case SDL_SCANCODE_LSHIFT: return "Left Shift";
        case SDL_SCANCODE_RSHIFT: return "Right Shift";
        case SDL_SCANCODE_UP: return "Up Arrow";
        case SDL_SCANCODE_DOWN: return "Down Arrow";
        case SDL_SCANCODE_LEFT: return "Left Arrow";
        case SDL_SCANCODE_RIGHT: return "Right Arrow";
        case SDL_SCANCODE_F3: return "F3";
        case SDL_SCANCODE_F4: return "F4";
        case SDL_SCANCODE_F5: return "F5";
        case SDL_SCANCODE_F6: return "F6";
        case SDL_SCANCODE_F7: return "F7";
        default: break;
        }
        if (binding.code >= SDL_SCANCODE_A && binding.code <= SDL_SCANCODE_Z)
            return std::string(1, char('A' + binding.code - SDL_SCANCODE_A));
        return "Key " + std::to_string(binding.code);
    }
    if (binding.device == Device::Button) {
        constexpr std::array<const char*, 21> labels{{
            "A", "B", "X", "Y", "Back", "Guide", "Start", "Left Stick", "Right Stick",
            "Left Shoulder", "Right Shoulder", "D-pad Up", "D-pad Down", "D-pad Left",
            "D-pad Right", "Misc", "Paddle 1", "Paddle 2", "Paddle 3", "Paddle 4", "Touchpad"}};
        return binding.code < int(labels.size()) ? labels[binding.code] : "Button " + std::to_string(binding.code);
    }
    constexpr std::array<const char*, 6> axes{{"Left X", "Left Y", "Right X", "Right Y", "Left Trigger", "Right Trigger"}};
    return (binding.code < int(axes.size()) ? std::string(axes[binding.code]) : "Axis " + std::to_string(binding.code)) +
        (binding.device == Device::AxisPositive ? " +" : " -");
}
const char* controller_type_name(ControllerType type) noexcept {
    switch (type) {
    case ControllerType::N64: return "N64";
    case ControllerType::DualSense: return "DualSense";
    case ControllerType::Xbox: return "Xbox";
    case ControllerType::NintendoPro: return "Nintendo Pro Controller";
    case ControllerType::Steam: return "Steam Controller";
    case ControllerType::DualShock: return "DualShock";
    case ControllerType::GameCube: return "GameCube Controller";
    case ControllerType::SteamDeck: return "Steam Deck Controller";
    default: return "Other";
    }
}
std::string binding_label_for_type(Binding binding, ControllerType type) {
    if (binding.code < 0 || binding.device == Device::Keyboard) return binding_label(binding);
    const bool raw = type == ControllerType::N64 || type == ControllerType::Other;
    if (binding.device != Device::Button) {
        const auto sign = binding.device == Device::AxisPositive ? " +" : " -";
        if (raw) return "SDL Axis " + std::to_string(binding.code) + sign;
        if (binding.code == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
            switch (type) {
            case ControllerType::DualSense: case ControllerType::DualShock: return std::string("L2") + sign;
            case ControllerType::NintendoPro: return std::string("ZL") + sign;
            default: return std::string("LT") + sign;
            }
        }
        if (binding.code == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
            switch (type) {
            case ControllerType::DualSense: case ControllerType::DualShock: return std::string("R2") + sign;
            case ControllerType::NintendoPro: return std::string("ZR") + sign;
            default: return std::string("RT") + sign;
            }
        }
        return binding_label(binding);
    }
    if (raw) return "SDL Button " + std::to_string(binding.code);
    constexpr std::array<const char*, 4> dualsense{{"Cross", "Circle", "Square", "Triangle"}};
    constexpr std::array<const char*, 4> xbox{{"A", "B", "X", "Y"}};
    constexpr std::array<const char*, 4> nintendo{{"B (south)", "A (east)", "Y (west)", "X (north)"}};
    constexpr std::array<const char*, 4> steam{{"A (south)", "B (east)", "X (west)", "Y (north)"}};
    if (binding.code <= SDL_CONTROLLER_BUTTON_Y) {
        const auto i = size_t(binding.code);
        switch (type) {
        case ControllerType::DualSense: case ControllerType::DualShock: return dualsense[i];
        case ControllerType::GameCube: return std::array<const char*,4>{{"A", "B", "X", "Y"}}[i];
        case ControllerType::SteamDeck: return xbox[i];
        case ControllerType::Xbox: return xbox[i];
        case ControllerType::NintendoPro:
            return nintendo_button_labels.load(std::memory_order_relaxed) ? xbox[i] : nintendo[i];
        case ControllerType::Steam: return steam[i];
        default: break;
        }
    }
    switch (binding.code) {
    case SDL_CONTROLLER_BUTTON_BACK:
        switch (type) {
        case ControllerType::DualSense: case ControllerType::DualShock: return "Create / Share";
        case ControllerType::Xbox: return "View";
        case ControllerType::NintendoPro: return "Minus";
        case ControllerType::Steam: return "Back";
        default: break;
        }
        break;
    case SDL_CONTROLLER_BUTTON_GUIDE:
        switch (type) {
        case ControllerType::DualSense: case ControllerType::DualShock: return "PS";
        case ControllerType::Xbox: return "Xbox";
        case ControllerType::NintendoPro: return "Home";
        case ControllerType::Steam: return "Steam";
        default: break;
        }
        break;
    case SDL_CONTROLLER_BUTTON_START:
        switch (type) {
        case ControllerType::DualSense: return "Options";
        case ControllerType::Xbox: return "Menu";
        case ControllerType::NintendoPro: return "Plus";
        case ControllerType::Steam: return "Start";
        default: break;
        }
        break;
    case SDL_CONTROLLER_BUTTON_LEFTSTICK:
        switch (type) {
        case ControllerType::DualSense: case ControllerType::DualShock: return "L3";
        case ControllerType::Xbox: case ControllerType::SteamDeck: return "LS";
        default: return "Left Stick Click";
        }
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
        switch (type) {
        case ControllerType::DualSense: case ControllerType::DualShock: return "R3";
        case ControllerType::Xbox: case ControllerType::SteamDeck: return "RS";
        default: return "Right Stick Click";
        }
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
        switch (type) {
        case ControllerType::DualSense: case ControllerType::DualShock: return "L1";
        case ControllerType::Xbox: return "LB";
        case ControllerType::NintendoPro: return "L";
        case ControllerType::Steam: return "Left Bumper";
        default: break;
        }
        break;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
        switch (type) {
        case ControllerType::DualSense: case ControllerType::DualShock: return "R1";
        case ControllerType::Xbox: return "RB";
        case ControllerType::NintendoPro: return "R";
        case ControllerType::Steam: return "Right Bumper";
        default: break;
        }
        break;
    default: break;
    }
    return binding_label(binding);
}
void begin_capture(Action action, Device device, unsigned slot) {
    begin_capture(action, device, slot, BindingLayer::Base);
}
void begin_capture(Action action, Device device, unsigned slot, BindingLayer layer) {
    if (action >= Action::Count || slot > 1 || (layer == BindingLayer::Modified && device == Device::Keyboard)) return;
    const auto instance = chosen.load(std::memory_order_acquire);
    if (device != Device::Keyboard && instance < 0) return;
    std::lock_guard lock(capture_mutex);
    scanned_action = action; scanned_device = device; scanned_slot = slot; scanned_layer = layer;
    capture_instance = instance;
    capture_armed = false;
    capture_releasing = false;
    capture_deadline = std::chrono::steady_clock::now() + capture_timeout;
    release_neutral_since = {};
    suppress_input.store(true, std::memory_order_release);
    scanning.store(true, std::memory_order_release);
}
void cancel_capture() noexcept {
    std::lock_guard lock(capture_mutex);
    scanning.store(false, std::memory_order_release);
    capture_releasing = true;
    release_neutral_since = {};
}
void abandon_baseline_capture() noexcept {
    if (!host().baseline) return;
    std::lock_guard lock(capture_mutex);
    scanning.store(false, std::memory_order_release);
    capture_releasing = capture_armed = false;
    release_neutral_since = {};
    suppress_input.store(false, std::memory_order_release);
}
bool capture_active() noexcept { return scanning.load(std::memory_order_acquire); }
CaptureStatus capture_status() noexcept {
    std::lock_guard lock(capture_mutex);
    const auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
        capture_deadline - std::chrono::steady_clock::now()).count();
    return {scanning.load(std::memory_order_acquire), !capture_armed,
        scanning.load(std::memory_order_acquire) ? int(std::max<std::int64_t>(0, remaining + 1)) : 0,
        scanned_action, scanned_device, scanned_slot};
}
bool input_suppressed() noexcept { return suppress_input.load(std::memory_order_acquire); }
std::vector<ControllerInfo> controllers() {
    std::vector<ControllerInfo> out;
    std::lock_guard lock(controller_mutex);
    for (const auto& e : opened) out.push_back(e.info);
    return out;
}
SDL_JoystickID selected_controller_instance() noexcept { return chosen.load(std::memory_order_acquire); }
bool select_controller(SDL_JoystickID instance) {
    if (instance < 0) { set_preferred_guid(""); return true; }
    std::lock_guard lock(controller_mutex);
    const auto it = std::find_if(opened.begin(), opened.end(), [instance](const auto& item) {
        return item.info.instance == instance;
    });
    if (it == opened.end()) return false;
    manual_instance = instance;
    manual_required = false;
    wanted_auto = false;
    wanted_device_id = it->info.stable_id;
    wanted_guid = it->info.guid;
    choose_controller();
    // The live instance is only a routing hint. An indistinguishable pair needs
    // another explicit selection after restart rather than a guessed restore.
    tooie::frontend::settings::set_string("tooie_preferred_controller_id", it->info.ambiguous ? "" : wanted_device_id);
    tooie::frontend::settings::set_string("tooie_preferred_controller_guid", it->info.ambiguous ? "" : wanted_guid);
    tooie::frontend::settings::set_bool("tooie_controller_auto", false);
    return true;
}
std::string selected_device_id() {
    std::lock_guard lock(binding_mutex);
    return active_device_id;
}
std::string preferred_device_id() {
    std::lock_guard lock(controller_mutex);
    return wanted_device_id;
}
bool controller_auto_selection() noexcept {
    std::lock_guard lock(controller_mutex);
    return wanted_auto;
}
std::string selected_friendly_name() {
    std::lock_guard lock(controller_mutex);
    const auto selected = chosen.load(std::memory_order_acquire);
    const auto it = std::find_if(opened.begin(), opened.end(), [selected](const auto& entry) {
        return entry.info.instance == selected;
    });
    return it == opened.end() ? "" : it->info.friendly_name;
}
void set_selected_friendly_name(std::string name) {
    if (name.size() > 64) name.resize(64);
    std::string id;
    {
        std::lock_guard lock(binding_mutex);
        if (active_device_id.empty()) return;
        id = active_profile_key;
        record_for(id).friendly_name = name;
        save_profiles();
    }
    std::lock_guard controller_lock(controller_mutex);
    for (auto& entry : opened)
        if ((id.find("|live-instance:") == std::string::npos && entry.info.stable_id == id) ||
            id == entry.info.stable_id + "|live-instance:" + std::to_string(entry.info.instance))
            entry.info.friendly_name = name.empty() ? entry.info.name : name;
}
ControllerType selected_controller_type() {
    std::lock_guard lock(binding_mutex);
    return active_profile_key.empty() ? ControllerType::Other : record_for(active_profile_key).type;
}
void set_selected_controller_type(ControllerType type) {
    if (type > ControllerType::SteamDeck) return;
    std::lock_guard lock(binding_mutex);
    if (active_device_id.empty()) return;
    record_for(active_profile_key).type = type;
    save_profiles();
}
std::vector<std::string> controller_profiles() {
    std::lock_guard lock(binding_mutex);
    std::vector<std::string> out;
    if (!active_profile_key.empty()) for (const auto& [name, table] : record_for(active_profile_key).profiles)
        out.push_back(name);
    return out;
}
std::string active_controller_profile() {
    std::lock_guard lock(binding_mutex);
    return active_profile_key.empty() ? "" : record_for(active_profile_key).active;
}
bool saved_profiles_unreadable() {
    std::lock_guard lock(binding_mutex);
    return profile_store_unreadable ||
        (!active_profile_key.empty() && unreadable_devices.contains(active_profile_key));
}
bool create_controller_profile(std::string name) {
    if (name.empty() || name.size() > 48) return false;
    std::lock_guard lock(binding_mutex);
    if (active_device_id.empty()) return false;
    auto& record = record_for(active_profile_key);
    if (profile_name_taken(record, name)) return false;
    record.profiles.emplace(name, bindings);
    auto& copy = record.extras[name] = record.extras.at(record.active);
    copy.retained = nlohmann::json::object(); // The copy holds only settings in effect.
    copy.legacy_base = nullptr;
    record.active = name;
    save_profiles();
    return true;
}
bool select_controller_profile(std::string name) {
    std::lock_guard lock(binding_mutex);
    if (active_device_id.empty()) return false;
    auto& record = record_for(active_profile_key);
    const auto it = record.profiles.find(name);
    if (it == record.profiles.end()) return false;
    record.active = name;
    reset_input_latches();
    copy_controller_bindings(bindings, it->second);
    auto& extra = record.extras[name];
    apply_profile_extras(extra);
    save_profiles();
    return true;
}
bool rename_controller_profile(std::string name) {
    if (name.empty() || name.size() > 48) return false;
    std::lock_guard lock(binding_mutex);
    if (active_device_id.empty()) return false;
    auto& record = record_for(active_profile_key);
    if (profile_name_taken(record, name)) return false;
    auto node = record.profiles.extract(record.active);
    node.key() = name;
    record.profiles.insert(std::move(node));
    auto extra_node = record.extras.extract(record.active);
    extra_node.key() = name;
    record.extras.insert(std::move(extra_node));
    record.active = name;
    save_profiles();
    return true;
}
void reset_active_controller_profile() {
    std::lock_guard lock(binding_mutex);
    if (active_device_id.empty()) return;
    auto& record = record_for(active_profile_key);
    record.profiles.at(record.active) = defaults();
    auto& extra = record.extras.at(record.active);
    extra = DeviceRecord::ProfileExtras{};
    for (size_t a = 0; a < count; ++a) for (unsigned i = 2; i < 8; ++i)
        extra.modified[a][i] = {Device((i - 2) / 2 + 1), -1};
    copy_controller_bindings(bindings, record.profiles.at(record.active));
    apply_profile_extras(extra);
    save_profiles();
}
bool delete_controller_profile(std::string name) {
    std::lock_guard lock(binding_mutex);
    if (active_device_id.empty()) return false;
    auto& record = record_for(active_profile_key);
    if (!record.profiles.contains(name) || record.profiles.size() <= 1) return false;
    if (record.active == name) {
        auto fallback = record.profiles.find("Default");
        if (fallback == record.profiles.end() || fallback->first == name) {
            fallback = std::find_if(record.profiles.begin(), record.profiles.end(),
                [&](const auto& entry) { return entry.first != name; });
        }
        if (fallback == record.profiles.end()) return false;
        record.active = fallback->first;
        reset_input_latches();
        copy_controller_bindings(bindings, fallback->second);
        const auto& extra = record.extras.at(record.active);
        apply_profile_extras(extra);
    }
    record.profiles.erase(name);
    record.extras.erase(name);
    save_profiles();
    return true;
}
Binding get_modifier_binding() { std::lock_guard lock(binding_mutex); return modifier_binding; }
void set_modifier_binding(Binding binding) {
    if (binding.device == Device::Keyboard || binding.code < -1 || binding.code > 255) return;
    std::lock_guard lock(binding_mutex);
    modifier_binding = binding;
    modifier_axis_latch = false; chord_suppressed_base.clear(); chord_axis_latches.clear();
    if (!active_profile_key.empty()) {
        auto& record = record_for(active_profile_key);
        auto& extra = record.extras.at(record.active);
        extra.modifier = binding;
        own_fields(extra, {"modifier"});
    }
    save_profiles();
}
ModifierSoloBehavior get_modifier_solo_behavior() {
    std::lock_guard lock(binding_mutex); return modifier_solo;
}
void set_modifier_solo_behavior(ModifierSoloBehavior behavior) {
    if (behavior > ModifierSoloBehavior::ModifierOnly) return;
    std::lock_guard lock(binding_mutex); modifier_solo = behavior;
    if (!active_profile_key.empty()) {
        auto& record = record_for(active_profile_key);
        auto& extra = record.extras.at(record.active);
        extra.solo = behavior;
        own_fields(extra, {"solo"});
    }
    save_profiles();
}
bool set_trigger_thresholds(int press_percent, int release_percent) {
    if (press_percent < 1 || press_percent > 100 || release_percent < 0 || release_percent >= press_percent) return false;
    std::lock_guard lock(binding_mutex);
    reset_input_latches();
    trigger_press_percent = press_percent; trigger_release_percent = release_percent;
    if (!active_profile_key.empty()) {
        auto& record = record_for(active_profile_key);
        auto& extra = record.extras.at(record.active);
        extra.trigger_press = press_percent; extra.trigger_release = release_percent;
        own_fields(extra, {"trigger_press", "trigger_release"});
    }
    save_profiles(); return true;
}
std::pair<int, int> get_trigger_thresholds() {
    std::lock_guard lock(binding_mutex); return {trigger_press_percent, trigger_release_percent};
}
std::string preferred_guid() {
    std::lock_guard lock(controller_mutex);
    return wanted_guid;
}
void set_preferred_guid(std::string guid) {
    {
        std::lock_guard lock(controller_mutex);
        wanted_guid = guid;
        wanted_device_id.clear();
        manual_instance = -1;
        manual_required = false;
        wanted_auto = guid.empty();
        choose_controller();
    }
    tooie::frontend::settings::set_string("tooie_preferred_controller_guid", guid);
    tooie::frontend::settings::set_string("tooie_preferred_controller_id", "");
    tooie::frontend::settings::set_bool("tooie_controller_auto", guid.empty());
}
void initialize_settings(const std::filesystem::path& config_root, bool allow_background_controller) {
    baseline_background_controller = allow_background_controller;
    sync_background_joystick_events(background_input_enabled());
    nintendo_button_labels.store(
        SDL_GetHintBoolean(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, SDL_TRUE), std::memory_order_relaxed);
    auto next = defaults();
    const auto saved_guid = tooie::frontend::settings::get_string("tooie_preferred_controller_guid", "");
    legacy_preferred_guid=saved_guid;
    const bool profiles_present = !tooie::frontend::settings::get_string("tooie_input_profiles_v1", "").empty();
    legacy_profile_tables.clear();
    legacy_assignments.clear();
    import_legacy(config_root, saved_guid, next);
    for (size_t a = 0; a < count; ++a)
        for (unsigned d = 0; d < 4; ++d)
            for (unsigned s = 0; s < 2; ++s) {
                if (d != unsigned(Device::Keyboard) && profiles_present) continue;
                auto& b = next[a][index(Device(d), s)];
                b.code = tooie::frontend::settings::get_int(setting_key(Action(a), Device(d), s), b.code);
            }
    {
        std::lock_guard lock(binding_mutex);
        bindings = next;
        modified_bindings = {};
        modifier_binding = {Device::Button, -1}; modifier_solo = ModifierSoloBehavior::PassThrough;
        trigger_press_percent = 50; trigger_release_percent = 45;
        reset_input_latches();
        legacy_template = next;
        load_profiles();
    }
    {
        std::lock_guard lock(controller_mutex);
        wanted_guid = saved_guid;
        wanted_device_id = tooie::frontend::settings::get_string("tooie_preferred_controller_id", "");
        wanted_auto = tooie::frontend::settings::get_bool("tooie_controller_auto",
            wanted_device_id.empty() && wanted_guid.empty());
        manual_required = !wanted_auto && wanted_device_id.empty() && wanted_guid.empty();
        manual_instance = -1;
        for (int i = 0, n = SDL_NumJoysticks(); i < n; ++i) connect(i);
        choose_controller();
    }
    if (const auto* product = host().product; product && product->set_fast_forward_rate)
        set_fast_forward_rate(tooie::frontend::settings::get_int("tooie_fast_forward_rate", 2));
}
void shutdown() noexcept {
    mapped_activity.store(false, std::memory_order_release);
    if (mouse_aim_relative) { SDL_SetRelativeMouseMode(SDL_FALSE); mouse_aim_relative = false; }
    if (cursor_shown == 0) SDL_ShowCursor(SDL_ENABLE);
    cursor_shown = -1;
    baseline_x.store(0, std::memory_order_release); baseline_y.store(0, std::memory_order_release);
    std::lock_guard lock(controller_mutex);
    stop_rumble();
    for (auto& e : opened) SDL_GameControllerClose(e.handle);
    opened.clear(); chosen = -1;
    published_enabled.store(false); published_physical.store(false); published_rumble.store(false);
    transient_buttons.store(0); scanning.store(false); suppress_input.store(false);
    capture_releasing = capture_armed = false;
    last_menu_binding = false; last_focused = false; focus_transition_pending = false;
    frontend_window = nullptr;
    background_hint_initialized = false;
    baseline_background_controller = false;
    was_menu_open = true; suppressed_buttons = suppressed_keyboard_buttons = 0;
    suppress_move = suppress_camera = suppress_keyboard_move = false;
    suppress_fast_forward = suppress_free_camera_reset = false;
    last_utility_binding.fill(false); last_fast_forward = last_free_camera_reset = false;
    set_fast_forward_held(false);
}
void handle_event(const SDL_Event& event) {
    if (event.type == SDL_MOUSEMOTION && mouse_aim_relative) {
        std::lock_guard lock(mouse_aim_mutex);
        mouse_aim_x += float(event.motion.xrel);
        mouse_aim_y += float(event.motion.yrel);
    }
#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT
    apply_persistent_input_reset();
    if (persistent_input_quarantine && event.type != SDL_CONTROLLERDEVICEADDED &&
        event.type != SDL_CONTROLLERDEVICEREMOVED && event.type != SDL_WINDOWEVENT)
        return;
#endif
    if (event.type == SDL_WINDOWEVENT &&
        (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ||
         event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)) {
        focus_transition_pending = true;
        // Loss discards old foreground edges. A selected-controller gameplay
        // edge received while unfocused must survive a subsequent gain in the
        // same poll batch; keyboard/capture/utility events never enter that path.
        if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST ||
            !background_input_enabled())
            transient_buttons.store(0, std::memory_order_release);
        return;
    }
    if (event.type == SDL_CONTROLLERDEVICEADDED) {
        std::lock_guard lock(controller_mutex);
        connect(event.cdevice.which);
        return;
    }
    if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        if (event.cdevice.which == capture_instance.load(std::memory_order_acquire) && capture_active()) cancel_capture();
        std::lock_guard lock(controller_mutex);
        if (event.cdevice.which == chosen.load(std::memory_order_acquire)) {
            stop_rumble();
            const auto removed = std::find_if(opened.begin(), opened.end(), [&](const auto& entry) {
                return entry.info.instance == event.cdevice.which;
            });
            manual_required = removed != opened.end() && removed->info.ambiguous;
            manual_instance = -1;
        }
        std::erase_if(opened, [&](auto& e) {
            if (e.info.instance != event.cdevice.which) return false;
            SDL_GameControllerClose(e.handle);
            return true;
        });
        choose_controller();
        return;
    }
    const bool key = event.type == SDL_KEYDOWN && !event.key.repeat;
    const bool button_event = event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP;
    const bool button = event.type == SDL_CONTROLLERBUTTONDOWN;
    const bool axis_motion = event.type == SDL_CONTROLLERAXISMOTION;
    const bool axis_event = axis_motion && (host().baseline ? baseline_axis(event.caxis.value) != 0 :
        std::abs(int(event.caxis.value)) >= trigger_press_percent * 32767 / 100);
    const bool capture_axis_event = axis_motion && std::abs(int(event.caxis.value)) > 22000;
    // Temporary capture-only probe: distinguish SDL delivery from capture filtering.
    if (scanning.load(std::memory_order_acquire) &&
        (button_event || axis_motion || event.type == SDL_JOYBUTTONDOWN ||
         event.type == SDL_JOYAXISMOTION || event.type == SDL_JOYHATMOTION)) {
        std::lock_guard capture_lock(capture_mutex);
        std::fprintf(stderr,
            "binding-capture event=%u instance=%d control=%u value=%d selected=%d capture=%d device=%u armed=%u focused=%u transition=%u suppressed=%u\n",
            event.type, event.cbutton.which, unsigned(event.cbutton.button),
            axis_motion || event.type == SDL_JOYAXISMOTION ? int(event.caxis.value) : int(event.cbutton.state),
            chosen.load(), capture_instance.load(), unsigned(scanned_device), unsigned(capture_armed),
            unsigned(frontend_window && SDL_GetKeyboardFocus() == frontend_window),
            unsigned(focus_transition_pending), unsigned(input_suppressed()));
    }
    if (button_event && event.cbutton.which == chosen.load() && event.cbutton.button < SDL_CONTROLLER_BUTTON_MAX) {
        if (event_buttons_device != event.cbutton.which) {
            event_buttons_down.fill(false);
            event_buttons_device = event.cbutton.which;
        }
        event_buttons_down[event.cbutton.button] = button;
    }
    if (axis_motion && event.caxis.which == chosen.load() && event.caxis.axis < SDL_CONTROLLER_AXIS_MAX) {
        if (event_axes_device != event.caxis.which) {
            event_axes_values.fill(0);
            event_axis_latches.clear();
            event_modifier_axis_latch = false;
            event_axes_device = event.caxis.which;
        }
        event_axes_values[event.caxis.axis] = Sint16(event.caxis.value);
    }
    if (button_event && event.cbutton.which == chosen.load() && !button) {
        const Binding released{Device::Button, event.cbutton.button};
        std::erase_if(chord_suppressed_base, [&](Binding binding) { return same_binding(binding, released); });
    }
    if (axis_motion && event.caxis.which == chosen.load()) {
        const int value = event.caxis.value;
        const int release = host().baseline ? baseline_digital_axis_threshold : trigger_release_percent * 32767 / 100;
        std::erase_if(chord_suppressed_base, [&](Binding binding) {
            if (binding.code != event.caxis.axis ||
                (binding.device != Device::AxisPositive && binding.device != Device::AxisNegative)) return false;
            const int magnitude = binding.device == Device::AxisPositive ? value : -value;
            if (magnitude > release) return false;
            const auto key = std::pair<unsigned, int>{unsigned(binding.device), binding.code};
            event_axis_latches[key] = false;
            chord_axis_latches[key] = false;
            return true;
        });
    }
    const bool analog_camera_enabled = analog_camera_input_enabled();
    if (!key && !button && !axis_event) return;
    const bool focused = frontend_window && SDL_GetKeyboardFocus() == frontend_window;
    const bool selected_event = key || (button_event ? event.cbutton.which : event.caxis.which) == chosen.load();
    const bool background_gameplay = !focused && selected_event && (button || axis_event) &&
        background_input_enabled() && host().game_started() &&
        !host().menu_open() && !input_suppressed();
    if (!focused && !background_gameplay) return;
    if (focused && focus_transition_pending) return;
    if (focused && input_suppressed()) {
        if (key && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            cancel_capture();
            return;
        }
        std::lock_guard capture_lock(capture_mutex);
        if (!scanning.load(std::memory_order_acquire) || !capture_armed) return;
        Binding found{scanned_device, -1};
        if (key && scanned_device == Device::Keyboard) found.code = event.key.keysym.scancode;
        if (button && scanned_device != Device::Keyboard && event.cbutton.which == capture_instance.load()) found.code = event.cbutton.button;
        if (capture_axis_event && event.caxis.which == capture_instance.load() && scanned_device != Device::Keyboard) {
            found.device = event.caxis.value > 0 ? Device::AxisPositive : Device::AxisNegative;
            found.code = event.caxis.axis;
        }
        if (found.code >= 0) {
            if (scanned_device != Device::Keyboard)
                for (Device device : {Device::Button, Device::AxisPositive, Device::AxisNegative})
                    set_binding(scanned_action, {device, -1}, scanned_layer, scanned_slot);
            set_binding(scanned_action, found, scanned_layer, scanned_slot);
            scanning.store(false, std::memory_order_release);
            capture_releasing = true;
            release_neutral_since = {};
        }
        return;
    }
    BindingTable table, modified_table;
    Binding active_modifier;
    ModifierSoloBehavior solo_behavior;
    {
        std::lock_guard lock(binding_mutex);
        table = bindings;
        modified_table = modified_bindings;
        active_modifier = modifier_binding;
        solo_behavior = modifier_solo;
    }
    SDL_GameController* controller = nullptr;
    {
        std::lock_guard lock(controller_mutex);
        controller = selected_controller();
    }
    const Binding event_binding = key ? Binding{Device::Keyboard, event.key.keysym.scancode} :
        button ? Binding{Device::Button, event.cbutton.button} :
        Binding{event.caxis.value > 0 ? Device::AxisPositive : Device::AxisNegative, event.caxis.axis};
    auto event_hits = [&](Binding binding) {
        if (!selected_event) return false;
        if (key) return binding.device == Device::Keyboard && binding.code == event_binding.code;
        return same_binding(binding, event_binding) && (!host().baseline || !axis_event ||
            std::abs(int(event.caxis.value)) > baseline_digital_axis_threshold);
    };
    const bool modifier_event = event_hits(active_modifier);
    const bool modifier_button_down = active_modifier.device == Device::Button && active_modifier.code >= 0 &&
        active_modifier.code < SDL_CONTROLLER_BUTTON_MAX && active_modifier.code < int(event_buttons_down.size()) &&
        event_buttons_device == chosen.load() && event_buttons_down[active_modifier.code];
    auto event_axis_down = [&](Binding binding, bool& latched) {
        if ((binding.device != Device::AxisPositive && binding.device != Device::AxisNegative) ||
            binding.code < 0 || binding.code >= SDL_CONTROLLER_AXIS_MAX || event_axes_device != chosen.load()) return false;
        const int value = event_axes_values[binding.code];
        const int magnitude = binding.device == Device::AxisPositive ? value : -value;
        latched = digital_axis_down(magnitude, latched);
        return latched;
    };
    const bool modifier_axis_down = event_axis_down(active_modifier, event_modifier_axis_latch);
    const bool modifier_down = modifier_event || modifier_button_down || modifier_axis_down ||
        physical_binding_down(active_modifier, controller, modifier_axis_latch);
    std::vector<Binding> active_chords;
    if (modifier_down) for (const auto& row : modified_table) for (const auto binding : row) {
        if (binding.code < 0 || binding.device == Device::Keyboard) continue;
        auto& latch = chord_axis_latches[{unsigned(binding.device), binding.code}];
        const bool event_button_down = binding.device == Device::Button && binding.code >= 0 &&
            binding.code < int(event_buttons_down.size()) && event_buttons_device == chosen.load() && event_buttons_down[binding.code];
        auto& event_latch = event_axis_latches[{unsigned(binding.device), binding.code}];
        const bool event_axis_held = event_axis_down(binding, event_latch);
        if ((event_hits(binding) || event_button_down || event_axis_held || physical_binding_down(binding, controller, latch)) &&
            std::none_of(active_chords.begin(), active_chords.end(),
                [&](Binding prior) { return same_binding(prior, binding); })) active_chords.push_back(binding);
    }
    for (const auto binding : active_chords)
        if (std::none_of(chord_suppressed_base.begin(), chord_suppressed_base.end(),
                [&](Binding prior) { return same_binding(prior, binding); })) chord_suppressed_base.push_back(binding);
    const auto consumed_chord = [&](Binding binding) {
        return std::any_of(active_chords.begin(), active_chords.end(),
                [&](Binding active) { return same_binding(binding, active); }) ||
            std::any_of(chord_suppressed_base.begin(), chord_suppressed_base.end(),
                [&](Binding active) { return same_binding(binding, active); });
    };
    auto matches_layer = [&](const BindingTable& source, Action action, BindingLayer layer) {
        if (layer == BindingLayer::Modified && !modifier_down) return false;
        for (const auto binding : source[static_cast<size_t>(action)]) {
            bool hit = event_hits(binding);
            if (host().baseline && axis_event && action >= Action::MoveUp && action <= Action::MoveRight)
                hit = selected_event && same_binding(binding, event_binding) && baseline_axis(event.caxis.value) != 0;
            if (!hit) continue;
            if (layer == BindingLayer::Base && solo_behavior == ModifierSoloBehavior::ModifierOnly &&
                modifier_down && same_binding(binding, active_modifier)) continue;
            if (layer == BindingLayer::Base && consumed_chord(binding)) continue;
            if (analog_camera_enabled && (binding.device == Device::AxisPositive || binding.device == Device::AxisNegative) &&
                (action == Action::CUp || action == Action::CDown || action == Action::CLeft || action == Action::CRight) &&
                (binding.code == SDL_CONTROLLER_AXIS_RIGHTX || binding.code == SDL_CONTROLLER_AXIS_RIGHTY)) continue;
            return true;
        }
        if (layer == BindingLayer::Modified && modifier_event &&
            std::any_of(source[static_cast<size_t>(action)].begin(), source[static_cast<size_t>(action)].end(),
                [&](Binding binding) { return binding.code >= 0 && consumed_chord(binding); })) return true;
        return false;
    };
    const auto matches = [&](Action action) {
        return matches_layer(table, action, BindingLayer::Base) ||
            matches_layer(modified_table, action, BindingLayer::Modified);
    };
    if (host().baseline) {
        for (unsigned a = 0; a <= unsigned(Action::Menu); ++a)
            if (matches(Action(a))) mapped_activity.store(true, std::memory_order_release);
    }
    // The baseline/player reserves Escape even when every Menu cell is cleared.
    // Capture cancellation above keeps ownership of Escape while recording.
    const bool escape_menu = host().baseline && key && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE;
    if (!axis_event && focused && (escape_menu || matches(Action::Menu))) {
        last_menu_binding = true; transient_buttons.store(0, std::memory_order_release);
        host().set_menu_open(!host().menu_open()); return;
    }
    if (!host().game_started() || host().menu_open()) return;
    for (const auto [action, mask] : n64_buttons) if (matches(action))
        transient_buttons.fetch_or(mask, std::memory_order_release);
    if (modifier_event) for (const auto [action, mask] : n64_buttons) {
        const auto& base_row = table[static_cast<size_t>(action)];
        if (std::any_of(base_row.begin(), base_row.end(),
                [&](Binding binding) { return binding.code >= 0 && consumed_chord(binding); }))
            transient_buttons.fetch_and(~std::uint32_t(mask), std::memory_order_acq_rel);
    }
    for (Action action : {Action::SaveProgress, Action::Diagnostics, Action::IssueMarker,
                          Action::SkipCutscene, Action::FreeCameraReset})
        if (focused && !axis_event && matches(action)) {
            last_utility_binding[static_cast<size_t>(action)] = true;
            dispatch(action);
        }
}
void tick(SDL_Window* window, bool gameplay, bool menu_open) {
#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT
    apply_persistent_input_reset();
#endif
    frontend_window = window;
    const bool background_controller = background_input_enabled();
    sync_background_joystick_events(background_controller);
    SDL_GameControllerUpdate();
    const bool focused = window && SDL_GetKeyboardFocus() == window;
    BindingTable table, modified_table;
    Binding active_modifier;
    ModifierSoloBehavior solo_behavior;
    {
        std::lock_guard lock(binding_mutex);
        table = bindings;
        modified_table = modified_bindings;
        active_modifier = modifier_binding;
        solo_behavior = modifier_solo;
    }
    SDL_GameController* controller;
    {
        std::lock_guard lock(controller_mutex);
        controller = selected_controller();
        const double raw_strength = tooie::frontend::settings::get_number("rumble_strength", 100.0);
        const double strength = std::isfinite(raw_strength) ? std::clamp(raw_strength, 0.0, 100.0) : 100.0;
        const bool capable = controller && published_rumble.load(std::memory_order_acquire);
        const bool allowed = capable && focused && gameplay && !menu_open &&
            !input_suppressed() && strength > 0;
        if (!allowed) desired_rumble.store(false, std::memory_order_release);
        const bool wanted = allowed && desired_rumble.load(std::memory_order_acquire);
        const auto intensity = static_cast<Uint16>(std::lround(strength * 655.35));
        const auto now = std::chrono::steady_clock::now();
        if (capable && (wanted != applied_rumble || chosen.load() != applied_rumble_device ||
            (wanted && now - last_rumble_update > std::chrono::milliseconds(500)))) {
            SDL_GameControllerRumble(controller, wanted ? intensity : 0, wanted ? intensity : 0, 1000);
            applied_rumble = wanted;
            applied_rumble_device = chosen.load();
            last_rumble_update = now;
        }
        published_menu_pad = {};
        if (focused && controller) {
            published_menu_pad.connected = true;
            for (unsigned b = 0; b < SDL_CONTROLLER_BUTTON_MAX && b < 32; ++b)
                if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton(b)))
                    published_menu_pad.buttons |= 1U << b;
            published_menu_pad.left_x = float(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX)) / 32767.0f;
            published_menu_pad.left_y = float(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY)) / 32767.0f;
        }
    }
    const Uint8* keys = focused ? SDL_GetKeyboardState(nullptr) : nullptr;
    {
        bool neutral = true;
        if (keys) for (int i = 0; i < SDL_NUM_SCANCODES; ++i)
            if (keys[i]) { neutral = false; break; }
        if (controller) {
            for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i)
                if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton(i))) neutral = false;
            for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i)
                if (std::abs(int(SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis(i)))) > 16000)
                    neutral = false;
        }
        const auto now = std::chrono::steady_clock::now();
        std::lock_guard lock(capture_mutex);
        if (scanning.load(std::memory_order_acquire)) {
            if (now >= capture_deadline ||
                (scanned_device != Device::Keyboard && capture_instance.load() != chosen.load(std::memory_order_acquire))) {
                scanning.store(false, std::memory_order_release);
                capture_releasing = true;
                release_neutral_since = {};
            } else if (neutral) capture_armed = true;
        }
        if (capture_releasing) {
            if (!neutral) release_neutral_since = {};
            else if (release_neutral_since == std::chrono::steady_clock::time_point{})
                release_neutral_since = now;
            else if (now - release_neutral_since >= release_cooldown) {
                capture_releasing = false;
                suppress_input.store(false, std::memory_order_release);
            }
        }
    }
    if (input_suppressed()) published_menu_pad = {};
    const bool modifier_down = physical_binding_down(active_modifier, controller, modifier_axis_latch);
    std::vector<Binding> currently_chorded;
    if (modifier_down) for (const auto& row : modified_table) for (const auto binding : row) {
        if (binding.code < 0 || binding.device == Device::Keyboard) continue;
        auto& latch = chord_axis_latches[{unsigned(binding.device), binding.code}];
        if (physical_binding_down(binding, controller, latch) &&
            std::none_of(currently_chorded.begin(), currently_chorded.end(),
                [&](Binding prior) { return same_binding(prior, binding); })) currently_chorded.push_back(binding);
    }
    std::vector<Binding> retained_chords;
    for (const auto binding : chord_suppressed_base) {
        if (std::any_of(currently_chorded.begin(), currently_chorded.end(),
                [&](Binding active) { return same_binding(active, binding); })) continue;
        auto& latch = chord_axis_latches[{unsigned(binding.device), binding.code}];
        if (physical_binding_down(binding, controller, latch)) retained_chords.push_back(binding);
    }
    for (const auto binding : currently_chorded)
        if (std::none_of(retained_chords.begin(), retained_chords.end(),
                [&](Binding prior) { return same_binding(prior, binding); })) retained_chords.push_back(binding);
    chord_suppressed_base = std::move(retained_chords);
    std::vector<Binding> blocked_base = chord_suppressed_base;
    if (solo_behavior == ModifierSoloBehavior::ModifierOnly && modifier_down && active_modifier.code >= 0)
        blocked_base.push_back(active_modifier);
    const bool analog_camera_enabled = analog_camera_input_enabled();
    auto is_down = [&](Action action) {
        return down(table, action, keys, nullptr, BindingLayer::Base) ||
            down(table, action, nullptr, controller, BindingLayer::Base, &blocked_base, analog_camera_enabled) ||
            (modifier_down && down(modified_table, action, nullptr, controller, BindingLayer::Modified));
    };
    const bool focus_regained = focused && (!last_focused || focus_transition_pending);
    last_focused = focused;
    focus_transition_pending = false;
    if (!focused && background_controller) {
        // Foreground menu/focus suppression must not interrupt a controller
        // that was deliberately left active while this window was unfocused.
        suppressed_buttons = 0;
        suppress_move = suppress_camera = false;
    }
    const bool menu_pressed = is_down(Action::Menu);
    if (focused && !focus_regained && menu_pressed && !last_menu_binding && !input_suppressed()) {
        transient_buttons.store(0, std::memory_order_release);
        host().set_menu_open(!host().menu_open());
    }
    last_menu_binding = menu_pressed;
    const bool enabled = gameplay && !host().menu_open() && !input_suppressed() &&
        (focused || background_controller);
    const bool utility_allowed = gameplay && focused && !focus_regained &&
        !host().menu_open() && !input_suppressed();
    for (Action action : {Action::SaveProgress, Action::Diagnostics, Action::IssueMarker,
                          Action::SkipCutscene, Action::FreeCameraReset}) {
        const bool pressed = is_down(action);
        auto& previous = last_utility_binding[static_cast<size_t>(action)];
        if (utility_allowed && pressed && !previous) dispatch(action);
        previous = pressed;
    }
    // Baseline axis() applies baseline_axis and never reads this value.
    const float deadzone = host().baseline ? 0.0f :
        std::clamp(float(tooie::frontend::settings::get_number("joystick_deadzone", 5.0)) / 100.0f, 0.0f, 0.9f);
    const bool current_menu_open = host().menu_open();
    const bool menu_closed = was_menu_open && !current_menu_open;
    if (menu_closed || (focus_regained && !background_controller)) {
        for (const auto [action, mask] : n64_buttons)
            if (is_down(action)) suppressed_buttons |= mask;
        suppress_move = movement_held(table, keys, controller, deadzone);
        suppress_camera = std::abs(axis(controller, SDL_CONTROLLER_AXIS_RIGHTX, deadzone)) > 0 ||
            std::abs(axis(controller, SDL_CONTROLLER_AXIS_RIGHTY, deadzone)) > 0;
    } else if (focus_regained) {
        // Suppress keys held while typing in another app, but do not suppress
        // the simultaneously held physical controller source for that action.
        for (const auto [action, mask] : n64_buttons)
            if (down(table, action, keys, nullptr)) suppressed_keyboard_buttons |= mask;
        suppress_keyboard_move = movement_held(table, keys, nullptr, deadzone);
    }
    if (focus_regained) {
        suppress_fast_forward = is_down(Action::FastForward);
        suppress_free_camera_reset = is_down(Action::FreeCameraReset);
    }
    was_menu_open = current_menu_open;
    std::uint16_t bits = 0;
    float x = 0, y = 0, rx = 0, ry = 0;
    if (enabled) {
        std::uint16_t keyboard_bits = 0, controller_bits = 0;
        for (const auto [action, mask] : n64_buttons) {
            if (down(table, action, keys, nullptr, BindingLayer::Base)) keyboard_bits |= mask;
            if (down(table, action, nullptr, controller, BindingLayer::Base, &blocked_base, analog_camera_enabled) ||
                (modifier_down && down(modified_table, action, nullptr, controller, BindingLayer::Modified))) controller_bits |= mask;
        }
        // C-left only once last poll already sent Z, so a two-trigger press
        // in the same frame still crouches before it trots.
        if (z_chord_down(table, controller, blocked_base) &&
            (published_buttons.load(std::memory_order_acquire) & 0x2000)) controller_bits |= 0x0002;
        suppressed_keyboard_buttons &= keyboard_bits;
        bits = (keyboard_bits & ~suppressed_keyboard_buttons) | controller_bits;
        suppressed_buttons &= bits;
        bits &= ~suppressed_buttons;
        const bool keyboard_move_held =
            movement_held(table, keys, nullptr, deadzone);
        if (!keyboard_move_held) suppress_keyboard_move = false;
        const Uint8* move_keys = suppress_keyboard_move ? nullptr : keys;
        if (host().baseline) {
            const float kx = direction(table, Action::MoveRight, move_keys, nullptr, deadzone) -
                direction(table, Action::MoveLeft, move_keys, nullptr, deadzone);
            const float ky = direction(table, Action::MoveUp, move_keys, nullptr, deadzone) -
                direction(table, Action::MoveDown, move_keys, nullptr, deadzone);
            float px = direction(table, Action::MoveRight, nullptr, controller, deadzone, &blocked_base) -
                direction(table, Action::MoveLeft, nullptr, controller, deadzone, &blocked_base);
            float py = direction(table, Action::MoveUp, nullptr, controller, deadzone, &blocked_base) -
                direction(table, Action::MoveDown, nullptr, controller, deadzone, &blocked_base);
            if (modifier_down) {
                px += direction(modified_table, Action::MoveRight, nullptr, controller, deadzone) -
                    direction(modified_table, Action::MoveLeft, nullptr, controller, deadzone);
                py += direction(modified_table, Action::MoveUp, nullptr, controller, deadzone) -
                    direction(modified_table, Action::MoveDown, nullptr, controller, deadzone);
            }
            x = baseline_keyboard_override(kx, std::clamp(px, -1.0f, 1.0f));
            y = baseline_keyboard_override(ky, std::clamp(py, -1.0f, 1.0f));
        } else {
            x = direction(table, Action::MoveRight, move_keys, controller, deadzone, &blocked_base) -
                direction(table, Action::MoveLeft, move_keys, controller, deadzone, &blocked_base);
            y = direction(table, Action::MoveUp, move_keys, controller, deadzone, &blocked_base) -
                direction(table, Action::MoveDown, move_keys, controller, deadzone, &blocked_base);
            if (modifier_down) {
                x += direction(modified_table, Action::MoveRight, nullptr, controller, deadzone) -
                    direction(modified_table, Action::MoveLeft, nullptr, controller, deadzone);
                y += direction(modified_table, Action::MoveUp, nullptr, controller, deadzone) -
                    direction(modified_table, Action::MoveDown, nullptr, controller, deadzone);
            }
            const float length = std::hypot(x, y);
            if (length > 1.0f) { x /= length; y /= length; }
        }
        if (suppress_move) {
            if (x == 0 && y == 0) suppress_move = false;
            else x = y = 0;
        }
        rx = axis(controller, SDL_CONTROLLER_AXIS_RIGHTX, deadzone);
        ry = -axis(controller, SDL_CONTROLLER_AXIS_RIGHTY, deadzone);
        if (host().baseline && controller) {
            // Lighthouse's right stick as its free-look sees it. libultraship
            // (ControllerStick::Process, default settings) first applies a 20%
            // circular deadzone on its +-85 scale and bounds diagonals to the
            // N64 octagon; controller_getRightStick then divides by 127, so a
            // full push reads 0.67. ReadStick (FreeLookCamera.cpp) adds a radial
            // 0.15 deadzone, rescaled. Only the analog camera reads rx/ry.
            constexpr double axis_range = 85.0, lus_deadzone = 17.0;
            const double sx = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX) * axis_range / 32767.0;
            const double sy = -SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY) * axis_range / 32767.0;
            double ux = std::fabs(sx), uy = std::fabs(sy);
            double len = std::hypot(ux, uy);
            if (len < lus_deadzone) len = 0;
            else if (len > axis_range) len = axis_range / len;
            else len = (len - lus_deadzone) * axis_range / (axis_range - lus_deadzone) / len;
            ux *= len; uy *= len;
            if (ux != 0.0 && uy != 0.0) {
                const double slope = uy / ux;
                double edge_x = axis_range / (slope + 16.0 / 69.0);
                const double edge_y = std::min(edge_x * slope, axis_range / (1.0 / slope + 16.0 / 69.0));
                edge_x = edge_y / slope;
                const double scale = std::hypot(edge_x, edge_y) / axis_range;
                ux *= scale; uy *= scale;
            }
            // libultraship stores the stick as int8_t (truncated).
            const float raw_x = std::clamp(float(std::copysign(std::trunc(ux), sx)) / 127.0f, -1.0f, 1.0f);
            const float raw_y = std::clamp(float(std::copysign(std::trunc(uy), sy)) / 127.0f, -1.0f, 1.0f);
            const float magnitude = std::hypot(raw_x, raw_y);
            constexpr float right_deadzone = 0.15f;
            if (magnitude < right_deadzone) rx = ry = 0;
            else {
                const float scale = (std::min(magnitude, 1.0f) - right_deadzone) / (1.0f - right_deadzone) / magnitude;
                rx = raw_x * scale; ry = raw_y * scale;
            }
        }
        if (suppress_camera) {
            if (rx == 0 && ry == 0) suppress_camera = false;
            else rx = ry = 0;
        }
    }
    {
        // The cursor is only shown while the settings menu is open (the player
        // sets ImGuiConfigFlags_NoMouseCursorChange, so ImGui leaves it alone).
        const int show = host().menu_open() ? 1 : 0;
        if (show != cursor_shown) {
            SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
            cursor_shown = show;
        }
    }
    {
        // First-person mouse aim: relative mode only while first-person control ran
        // in the last 150 ms, so the cursor is never captured elsewhere.
        const bool want = enabled && focused && mouse_aim_wanted.load(std::memory_order_acquire) &&
            steady_nanoseconds() < first_person_until.load(std::memory_order_acquire);
        if (want != mouse_aim_relative) {
            mouse_aim_relative = want && SDL_SetRelativeMouseMode(SDL_TRUE) == 0;
            if (!mouse_aim_relative) SDL_SetRelativeMouseMode(SDL_FALSE);
            static int logged = -1; // relative + 2 x wanted, logged on change only.
            const int state = int(mouse_aim_relative) + 2 * int(want);
            if (state != logged) {
                logged = state;
                std::fprintf(stderr, "input mouse-aim relative=%d wanted=%d%s%s\n", int(mouse_aim_relative), int(want),
                    want && !mouse_aim_relative ? " error=" : "", want && !mouse_aim_relative ? SDL_GetError() : "");
            }
            std::lock_guard lock(mouse_aim_mutex);
            mouse_aim_x = mouse_aim_y = 0;
        }
    }
    if (host().baseline) {
        // Held opposing keys still count as activity, even when their output cancels.
        bool mapped_button_held = false;
        for (unsigned i = 0; i <= unsigned(Action::Menu); ++i)
            mapped_button_held |= is_down(Action(i));
        if (focused && (mapped_button_held || movement_held(table, keys, controller, deadzone, &blocked_base)))
            mapped_activity.store(true, std::memory_order_release);
        baseline_x.store(x, std::memory_order_release);
        baseline_y.store(y, std::memory_order_release);
    }
    published_buttons.store(bits, std::memory_order_release);
    if (!enabled) transient_buttons.store(0, std::memory_order_release);
    published_x.store(int(std::lround(x * 32767)), std::memory_order_release);
    published_y.store(int(std::lround(y * 32767)), std::memory_order_release);
    published_rx.store(int(std::lround(rx * 32767)), std::memory_order_release);
    published_ry.store(int(std::lround(ry * 32767)), std::memory_order_release);
    published_enabled.store(enabled, std::memory_order_release);
    const bool fast_pressed = is_down(Action::FastForward);
    if (!fast_pressed) suppress_fast_forward = false;
    // Baseline: release is required after menu close, after a modified keyboard
    // press, or while a gameplay action shares the input.
    if (host().baseline && fast_pressed && !suppress_fast_forward &&
        (menu_closed || fast_forward_shares_gameplay_binding(table) ||
         (down(table, Action::FastForward, keys, nullptr) &&
          (SDL_GetModState() & (KMOD_SHIFT | KMOD_ALT | KMOD_CTRL | KMOD_GUI)))))
        suppress_fast_forward = true;
    const bool fast = utility_allowed && fast_pressed && !suppress_fast_forward;
    if (fast && !last_fast_forward) note_discrete_action(DiscreteInputAction::FastForward);
    last_fast_forward = fast;
    if (const auto* product = host().product; product && product->set_fast_forward_rate)
        set_fast_forward_rate(tooie::frontend::settings::get_int("tooie_fast_forward_rate", 2));
    set_fast_forward_held(fast);
    const bool camera_enabled = enabled && focused && !focus_regained &&
        free_camera_enabled();
    set_free_camera_input_active(camera_enabled);
    set_free_camera_motion(
        camera_enabled ? float(is_down(Action::FreeCameraRight)) - float(is_down(Action::FreeCameraLeft)) : 0,
        camera_enabled ? float(is_down(Action::FreeCameraUp)) - float(is_down(Action::FreeCameraDown)) : 0,
        camera_enabled ? float(is_down(Action::FreeCameraForward)) - float(is_down(Action::FreeCameraBack)) : 0);
    const bool reset_pressed = is_down(Action::FreeCameraReset);
    if (!reset_pressed) suppress_free_camera_reset = false;
    const bool reset = camera_enabled && reset_pressed && !suppress_free_camera_reset;
    if (reset && !last_free_camera_reset) request_free_camera_reset();
    last_free_camera_reset = reset;
#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT
    persistent_input_quarantine = false;
    // A restore may have arrived while this poll was publishing old physical
    // state. Re-apply its atomic neutral gate before returning to the guest.
    apply_persistent_input_reset();
#endif
}
Snapshot snapshot() noexcept {
    // The right stick is published for the player too; the analog camera reads it.
    if (host().baseline) return {static_cast<std::uint16_t>(published_buttons.load(std::memory_order_acquire)),
        baseline_x.load(std::memory_order_acquire), baseline_y.load(std::memory_order_acquire),
        published_rx.load(std::memory_order_acquire) / 32767.0f,
        published_ry.load(std::memory_order_acquire) / 32767.0f,
        published_enabled.load(std::memory_order_acquire)};
    return {static_cast<std::uint16_t>(published_buttons.load(std::memory_order_acquire)),
        published_x.load(std::memory_order_acquire) / 32767.0f,
        published_y.load(std::memory_order_acquire) / 32767.0f,
        published_rx.load(std::memory_order_acquire) / 32767.0f,
        published_ry.load(std::memory_order_acquire) / 32767.0f,
        published_enabled.load(std::memory_order_acquire)};
}
std::uint16_t consume_transient_buttons() noexcept {
    return static_cast<std::uint16_t>(transient_buttons.exchange(0, std::memory_order_acq_rel));
}
bool consume_mapped_activity() noexcept { return mapped_activity.exchange(false, std::memory_order_acq_rel); }
MenuGamepadState menu_gamepad_state() noexcept { return published_menu_pad; }
bool game_input_disabled() noexcept { return !published_enabled.load(std::memory_order_acquire); }
#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT
void persistent_reset_transients() noexcept {
    persistent_input_reset_pending.store(true, std::memory_order_release);
    // The UI tick will observe this and send a zero-strength update to stop
    // any pre-restore pulse; the next guest rumble command may enable it again.
    desired_rumble.store(false, std::memory_order_release);
    transient_buttons.store(0, std::memory_order_release);
    published_buttons.store(0, std::memory_order_release);
    published_x.store(0, std::memory_order_release);
    published_y.store(0, std::memory_order_release);
    published_rx.store(0, std::memory_order_release);
    published_ry.store(0, std::memory_order_release);
    published_enabled.store(false, std::memory_order_release);
}
#endif
void get_right_analog(int port, float* x, float* y) noexcept {
    // Baseline snapshot keeps N64 right axes at zero. This separate sample
    // reads the existing gated mapper publication; no new controller poll.
    const bool enabled = port == 0 && published_enabled.load(std::memory_order_acquire);
    if (x) *x = enabled ? published_rx.load(std::memory_order_acquire) / 32767.0f : 0;
    if (y) *y = enabled ? published_ry.load(std::memory_order_acquire) / 32767.0f : 0;
}
void set_mouse_aim_enabled(bool enabled) noexcept { mouse_aim_wanted.store(enabled, std::memory_order_release); }
void note_first_person_aim() noexcept {
    first_person_until.store(steady_nanoseconds() + 150'000'000, std::memory_order_release);
}
void take_mouse_aim(float* x, float* y) noexcept {
    std::lock_guard lock(mouse_aim_mutex);
    if (x) *x = mouse_aim_x;
    if (y) *y = mouse_aim_y;
    mouse_aim_x = mouse_aim_y = 0;
}
void set_rumble(int port, bool enabled) noexcept {
    if (port == 0) desired_rumble.store(enabled, std::memory_order_release);
}
bool physical_controller() noexcept { return published_physical.load(std::memory_order_acquire); }
bool rumble_capable() noexcept { return published_rumble.load(std::memory_order_acquire); }
void post_diagnostics_toggle() noexcept { diagnostics_toggle_pending.store(true, std::memory_order_release); }
bool consume_diagnostics_toggle() noexcept { return diagnostics_toggle_pending.exchange(false, std::memory_order_acq_rel); }
void post_issue_marker() noexcept {
    auto pending = issue_marker_pending.load(std::memory_order_relaxed);
    while (pending != UINT32_MAX && !issue_marker_pending.compare_exchange_weak(
        pending, pending + 1U, std::memory_order_release, std::memory_order_relaxed)) {}
}
bool consume_issue_marker() noexcept {
    auto pending = issue_marker_pending.load(std::memory_order_acquire);
    while (pending != 0U) {
        if (issue_marker_pending.compare_exchange_weak(
            pending, pending - 1U, std::memory_order_acq_rel, std::memory_order_acquire)) return true;
    }
    return false;
}
void note_relevant_keydown(std::uint32_t scancode, bool repeat, bool game_started,
    bool keyboard_focus, bool all_input_disabled, bool context_capture, bool binding_scan, bool skip_events) noexcept {
    total_keydowns.fetch_add(1, std::memory_order_relaxed);
    const bool escape = scancode == SDL_SCANCODE_ESCAPE;
    const bool enter = scancode == SDL_SCANCODE_RETURN;
    const bool function = scancode >= SDL_SCANCODE_F3 && scancode <= SDL_SCANCODE_F7;
    if (!escape && !enter && !function) return;
    relevant_keydowns.fetch_add(1); if (escape) escape_keydowns.fetch_add(1);
    if (enter) return_keydowns.fetch_add(1); if (function) function_keydowns.fetch_add(1);
    last_scancode.store(scancode); last_repeat.store(repeat); last_game_started.store(game_started);
    last_keyboard_focus.store(keyboard_focus); last_all_input_disabled.store(all_input_disabled);
    last_context_capture.store(context_capture); last_binding_scan.store(binding_scan);
    last_skip_events.store(skip_events);
}
void note_discrete_action(DiscreteInputAction action) noexcept {
    switch (action) {
    case DiscreteInputAction::SaveProgress: accepted_save_progress.fetch_add(1); break;
    case DiscreteInputAction::ToggleDiagnostics: accepted_diagnostics.fetch_add(1); break;
    case DiscreteInputAction::MarkIssue: accepted_issue_markers.fetch_add(1); break;
    case DiscreteInputAction::FastForward: accepted_fast_forward.fetch_add(1); break;
    case DiscreteInputAction::SkipCutscene: accepted_cutscene_skips.fetch_add(1); break;
    }
}
InputDebugSnapshot input_debug_snapshot() noexcept {
    return {total_keydowns.load(), relevant_keydowns.load(), escape_keydowns.load(),
        return_keydowns.load(), function_keydowns.load(), accepted_save_progress.load(),
        accepted_diagnostics.load(), accepted_issue_markers.load(), accepted_fast_forward.load(),
        accepted_cutscene_skips.load(), last_scancode.load(), last_repeat.load(),
        last_game_started.load(), last_keyboard_focus.load(), last_all_input_disabled.load(),
        last_context_capture.load(), last_binding_scan.load(), last_skip_events.load()};
}
} // namespace tooie::input
