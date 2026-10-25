#include "frontend_config.hpp"

#include <json/json.hpp>
#include <climits>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <type_traits>
#include <array>
#include <utility>
#include <chrono>

namespace {
std::mutex mutex;
std::filesystem::path path;
nlohmann::json values = nlohmann::json::object();
bool dirty = false;
bool write_blocked = false;
bool write_failed = false;
std::chrono::steady_clock::time_point retry_after{};

void migrate_label(const char* id, std::initializer_list<std::pair<const char*,int>> labels) {
    auto it=values.find(id);
    if (it==values.end() || !it->is_string()) return;
    const auto label=it->get<std::string>();
    for (const auto& [text,number]:labels) if (label==text) { *it=number; return; }
}

void migrate_legacy_labels() {
    migrate_label("res_option",{{"Auto (match output)",0},{"Auto",0},{"1x",1},{"2x",2},{"4x",4},{"8x",8}});
    migrate_label("msaa_option",{{"None",0},{"2x",2},{"4x",4},{"8x",8}});
    migrate_label("tooie_downsample_quality",{{"Off",1},{"2x internal",2},{"4x internal",4}});
    migrate_label("tooie_output_mode",{{"Windowed",0},{"Borderless desktop",1},{"Exclusive fullscreen",2}});
    migrate_label("tooie_output_display",{{"Current display",0},{"Display 1",1},{"Display 2",2},{"Display 3",3},{"Display 4",4}});
    migrate_label("tooie_output_resolution",{{"Desktop native",0},{"1280 x 720",1},{"1920 x 1080",2},
        {"2560 x 1440",3},{"3440 x 1440",4},{"3840 x 2160",5},{"Default window (960 x 720)",6}});
    migrate_label("tooie_game_widescreen",{{"Original (4:3)",0},{"On (16:9)",1},{"On (selected aspect)",1}});
    migrate_label("tooie_native_aspect",{{"16:9 (Default)",0},{"21:9 (2.33:1)",1},{"32:9",2},{"43:18 (3440x1440)",3}});
    migrate_label("tooie_actor_draw_distance",{{"Original",0},{"Extended actors (2x)",1}});
    migrate_label("tooie_counter_layout",{{"Centered (default)",0},{"Expanded",1}});
    migrate_label("tooie_cutscene_aspect",{{"Original 4:3",0},{"Follow game",1}});
    migrate_label("tooie_cutscene_motion",{{"Interpolated",0},{"Original motion",1}});
    migrate_label("tooie_pacing_preset",{{"Custom pacing",0},{"Baseline: VSync, Original, Console",1},
        {"Observed smooth: Display + Present Early (experimental)",2},{"Isolation: Display + Console",3}});
    migrate_label("tooie_output_rate_mode",{{"Original",0},{"Display",1},{"Custom (experimental)",2}});
    migrate_label("tooie_presentation_mode",{{"Console",0},{"Present Early (experimental)",1}});
    migrate_label("tooie_fast_forward_rate",{{"2x",2},{"4x",4},{"6x",6},{"8x",8}});
    migrate_label("tooie_free_camera_speed",{{"Slow",150},{"Normal",600},{"Fast",1200}});
    migrate_label("tooie_diagnostics_overlay",{{"Off",0},{"FPS",1},{"Detailed",2},{"Practice",3}});
    migrate_label("tooie_diagnostics_corner",{{"Top Left",0},{"Top Right",1},{"Bottom Left",2},{"Bottom Right",3}});
    migrate_label("tooie_minimap_mode",{{"Off",0},{"Trail",1}});
    migrate_label("tooie_minimap_corner",{{"Bottom Left",0},{"Bottom Right",1}});
    migrate_label("tooie_minimap_heading",{{"Camera Up",0},{"Character Up",1}});
    migrate_label("background_input_mode",{{"Off",0},{"On",1}});
    migrate_label("tooie_general_category",{{"Input",0},{"Camera",1},{"Diagnostics",3},{"Exploration",2}});
    migrate_label("tooie_graphics_category",{{"Quality",0},{"Framing",1},{"Pacing",2},{"Display",3}});
    migrate_label("tooie_tools_category",{{"Cheats",0},{"Travel",1},{"Moves",2},
        {"World Entrances",3},{"Collectibles",4},{"Player",5},{"Bosses",6},{"Practice",7}});
}

void establish_pacing_defaults() {
    if (values.contains("tooie_pacing_preset")) return;
    const bool has_explicit_pacing = values.contains("tooie_vsync") ||
        values.contains("tooie_output_rate_mode") ||
        values.contains("tooie_custom_output_rate") ||
        values.contains("tooie_presentation_mode");
    // Retained frontend pacing defaults remain for shared-profile compatibility.
    // Older explicit choices become Custom; new retained profiles use Display +
    // Present Early without imposing a custom Hz. Selected baseline player pacing
    // uses separate baseline_output_rate, baseline_manual_hz and baseline_vsync keys.
    values["tooie_pacing_preset"] = 0;
    if (!has_explicit_pacing) {
        values["tooie_vsync"] = false;
        values["tooie_output_rate_mode"] = 1;
        values["tooie_custom_output_rate"] = 60;
        values["tooie_presentation_mode"] = 1;
    }
    dirty = true;
}

bool merge_legacy(const std::filesystem::path& file, bool override_existing = false) {
    std::ifstream input(file);
    if (!input) {
        // Absence is normal for a new profile; an existing but unreadable
        // primary must not be replaced by defaults on the next flush.
        std::error_code error;
        return !std::filesystem::exists(file, error) && !error;
    }
    try {
        nlohmann::json old;
        input >> old;
        if (old.is_object()) {
            if (old.contains("storage") && old["storage"].is_object()) old = old["storage"];
            for (auto it = old.begin(); it != old.end(); ++it) {
                if (override_existing || !values.contains(it.key())) values[it.key()] = it.value();
            }
            return true;
        }
    } catch (...) { /* A damaged legacy file does not prevent launch. */ }
    return false;
}

template <typename T> T read(std::string_view id, T fallback) {
    std::lock_guard lock(mutex);
    auto it = values.find(std::string(id));
    if (it == values.end()) return fallback;
    try {
        // The JSON library narrows with static_cast, so ranges are checked first;
        // an out-of-range number falls back. In-range floats keep their truncation.
        if constexpr (std::is_same_v<T, int>) {
            if (it->is_number_unsigned()) {
                if (const auto value = it->get<std::uint64_t>(); value <= std::uint64_t(INT_MAX)) return int(value);
            } else if (it->is_number_integer()) {
                if (const auto value = it->get<std::int64_t>(); value >= INT_MIN && value <= INT_MAX) return int(value);
            } else if (it->is_number_float()) {
                if (const auto value = it->get<double>(); value >= INT_MIN && value <= INT_MAX) return int(value);
            }
        } else if constexpr (std::is_same_v<T, double>) {
            if (it->is_number()) return it->get<double>();
        } else if constexpr (std::is_same_v<T, bool>) {
            if (it->is_boolean()) return it->get<bool>();
            if (it->is_number_unsigned()) return it->get<std::uint64_t>() != 0;
            if (it->is_number_integer()) return it->get<std::int64_t>() != 0;
        } else if constexpr (std::is_same_v<T, std::string>) {
            if (it->is_string()) return it->get<std::string>();
        }
    } catch (...) {}
    return fallback;
}

template <typename T> void write(std::string_view id, T value) {
    std::lock_guard lock(mutex);
    const std::string key(id);
    if (values.contains(key) && values[key] == value) return;
    values[key] = std::move(value);
    dirty = true;
}
}

namespace tooie::frontend::settings {
std::string_view recovery_notice() {
    std::lock_guard lock(mutex);
    if (write_blocked)
        return "The primary settings file could not be read. Automatic saving is paused; tooie_imgui.json and its .bak are unchanged. Recover or replace the primary file outside the game, then restart.";
    return write_failed
        ? "Settings could not be saved. Automatic saving will retry. Changes may be lost when you close the game."
        : "";
}
#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT
std::string persistent_identity_payload() {
    std::lock_guard lock(mutex);
    auto compatible_values = values;
    // Navigation, diagnostic presentation and one-shot UI receipts do not
    // alter the captured guest. Visiting the state controls must not make a
    // checkpoint incompatible. Keep gameplay, rendering and unknown keys
    // fail-closed until their cross-setting restore semantics are established.
    for (const char* key : {
        "tooie_general_category", "tooie_graphics_category", "tooie_tools_category",
        "tooie_diagnostics_corner", "tooie_diagnostics_overlay", "tooie_mapped_prompt_legend",
        "tooie_minimap_corner", "tooie_minimap_heading", "tooie_minimap_mode",
        "tooie_mark_graphics_issue", "tooie_reset_free_camera_now",
        "tooie_show_cursor_when_opening_settings", "tooie_cheat_status",
        "tooie_progression_status", "tooie_pacing_default_version"}) {
        compatible_values.erase(key);
    }
    return compatible_values.dump();
}
#endif
void initialize(const std::filesystem::path& config_root) {
    std::lock_guard lock(mutex);
    path = config_root / "tooie_imgui.json";
    values = nlohmann::json::object();
    write_blocked = false;
    write_failed = false;
    retry_after = {};
    merge_legacy(config_root / "general.json");
    merge_legacy(config_root / "graphics.json");
    merge_legacy(config_root / "sound.json");
    merge_legacy(config_root / "tooie_tools.json");
    write_blocked = !merge_legacy(path, true);
    migrate_legacy_labels();
    dirty = false;
    establish_pacing_defaults();
}
bool flush(bool periodic) {
    std::lock_guard lock(mutex);
    if (!dirty) return true;
    if (path.empty() || write_blocked) return false;
    if (periodic && write_failed && std::chrono::steady_clock::now() < retry_after) return false;
    const auto failed = [] {
        write_failed = true;
        retry_after = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        return false;
    };
    try {
        std::filesystem::create_directories(path.parent_path());
        const auto temp = path.string() + ".tmp";
        {
            std::ofstream output(temp, std::ios::trunc);
            output << values.dump(2) << '\n';
            output.close();
            if (!output) return failed();
        }
        std::error_code error;
        std::filesystem::rename(temp, path, error);
        if (error) {
            const auto backup = path.string() + ".bak";
            std::filesystem::remove(backup, error);
            error.clear();
            std::filesystem::rename(path, backup, error);
            if (error) return failed();
            std::filesystem::rename(temp, path, error);
            if (error) {
                std::error_code restore_error;
                std::filesystem::rename(backup, path, restore_error);
                return failed();
            }
        }
        dirty = false;
        write_failed = false;
        retry_after = {};
        return true;
    } catch (...) { return failed(); }
}
bool get_bool(std::string_view id, bool fallback) { return read(id, fallback); }
int get_int(std::string_view id, int fallback) { return read(id, fallback); }
double get_number(std::string_view id, double fallback) { return read(id, fallback); }
std::string get_string(std::string_view id, std::string_view fallback) { return read(id, std::string(fallback)); }
void set_bool(std::string_view id, bool value) { write(id, value); }
void set_int(std::string_view id, int value) { write(id, value); }
void set_number(std::string_view id, double value) { write(id, value); }
void set_string(std::string_view id, std::string value) { write(id, std::move(value)); }
}
