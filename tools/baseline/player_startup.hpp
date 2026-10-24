#pragma once
#include <algorithm>
#include <string_view>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Banjo-Tooie: Steam Deck Port player startup (BASELINE_PLAYER only): owned profile,
// ROM picker, one-time copy of an older ordinary save, user log and readable
// failures. The engineering command line is compiled unchanged without it.
namespace baseline::player {
// Player-readable UTF-8 text.
struct Failure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Arguments {
    std::filesystem::path profile, rom, import_save; // Empty profile: the default folder.
    std::filesystem::path input_script;              // QA only, bounded by explicit seconds.
    unsigned seconds = 0;                           // 0 runs until Quit.
    bool qa = false;
    // QA only: an explicit graphics API for a measured control run. Default keeps the player's
    // existing API selection; nothing is stored in the profile.
    enum class Api { Default, Vulkan, D3D12 } api = Api::Default;
};
// No arguments for play, or QA: --profile-dir FOLDER [--rom FILE] [--import-save FILE]
// [--seconds 1..900 [--input-script FILE]] [--api vulkan|d3d12].
namespace detail {
template <class String>
bool is(const String& text, std::string_view ascii) {
    return text.size() == ascii.size() && std::equal(ascii.begin(), ascii.end(), text.begin(),
        [](char a, typename String::value_type b) { return typename String::value_type(a) == b; });
}
// One QA grammar for both native argument encodings; values become paths unconverted.
template <class String>
Arguments parse_arguments(const std::vector<String>& arguments) {
    Arguments result;
    if (arguments.empty()) return result;
    const auto usage = [] {
        return Failure("Usage: BanjoTooieScurvyDogPort [--profile-dir FOLDER [--rom FILE] [--import-save FILE] "
            "[--seconds 1..900 [--input-script FILE]] [--api vulkan|d3d12]]");
    };
    bool seconds_seen = false;
    for (std::size_t i = 0; i < arguments.size(); i += 2) {
        if (i + 1 == arguments.size()) throw usage();
        const auto& name = arguments[i];
        const auto& value = arguments[i + 1];
        const auto take = [&](std::filesystem::path& slot) {
            if (!slot.empty() || value.empty()) throw usage();
            slot = value;
        };
        if (is(name, "--profile-dir")) take(result.profile);
        else if (is(name, "--rom")) take(result.rom);
        else if (is(name, "--import-save")) take(result.import_save);
        else if (is(name, "--input-script")) take(result.input_script);
        else if (is(name, "--seconds")) {
            if (seconds_seen) throw usage();
            seconds_seen = true;
            unsigned seconds = 0;
            bool valid = !value.empty() && value.size() <= 3;
            for (const auto c : value) {
                if (c < '0' || c > '9') valid = false;
                else seconds = seconds * 10 + unsigned(c - '0');
            }
            if (!valid || seconds < 1 || seconds > 900) throw Failure("--seconds must be 1..900");
            result.seconds = seconds;
        } else if (is(name, "--api")) {
            if (result.api != Arguments::Api::Default) throw usage();
            if (is(value, "vulkan")) result.api = Arguments::Api::Vulkan;
            else if (is(value, "d3d12")) result.api = Arguments::Api::D3D12;
            else throw Failure("--api must be vulkan or d3d12");
        } else throw usage();
    }
    if (result.profile.empty())
        throw Failure("QA options require --profile-dir; start without arguments for normal play.");
    // The existing engineering script owns port 0 only for a bounded QA run.
    if (!result.input_script.empty() && result.seconds == 0)
        throw Failure("--input-script requires an explicit --seconds bound.");
    result.qa = true;
    return result;
}
}
#ifdef _WIN32
inline Arguments parse_arguments(const std::vector<std::wstring>& arguments) { return detail::parse_arguments(arguments); }
// Wide command line without the executable; Unicode paths survive.
std::vector<std::wstring> command_line();
#else
// Native argv bytes without the executable; paths are never transcoded.
inline Arguments parse_arguments(const std::vector<std::string>& arguments) { return detail::parse_arguments(arguments); }
std::vector<std::string> command_line(int argc, char** argv);
#endif

// Compatibility identifier: the Alpha 4 marker name and bytes, so existing
// explicit --profile-dir profiles and migrated profiles stay valid.
inline constexpr wchar_t marker_name[] = L".encore-profile";
inline constexpr char marker_text[] = "Banjo-Tooie: Encore profile v1\n";
// Unchanged runtime identity: saves/<game_id>.bin, 16 Kbit EEPROM.
inline constexpr wchar_t save_name[] = L"bt.n64.us.1.0.bin";
inline constexpr std::uintmax_t save_bytes = 2048;
inline constexpr wchar_t import_record_name[] = L"save-import.txt";

std::filesystem::path default_profile_root();  // %LOCALAPPDATA%\BanjoTooieScurvyDogPort; Linux $XDG_DATA_HOME/BanjoTooieSteamDeckPort
std::filesystem::path previous_profile_root(); // %LOCALAPPDATA%\BanjoTooieEncore (Alpha 4); only ever read.
std::filesystem::path legacy_save_path();      // Earlier alpha default save; only ever read.

// Exclusive owner of a marked profile for its lifetime. A missing or empty
// folder is created and marked before anything else is written; a marked
// folder is reused; a link, a file, an unmarked non-empty folder or a folder
// held by another copy is refused unchanged.
class Profile {
public:
    static Profile acquire(const std::filesystem::path& root);
    Profile(Profile&& other) noexcept;
    Profile& operator=(Profile&&) = delete;
    ~Profile();
    const std::filesystem::path& root() const noexcept { return root_; }
#ifdef _WIN32
    const std::filesystem::path& storage() const noexcept { return root_; }
private:
    Profile(std::filesystem::path root, void* handle) : root_(std::move(root)), handle_(handle) {}
    std::filesystem::path root_;
    void* handle_ = nullptr;
#else
    const std::filesystem::path& storage() const noexcept { return storage_; }
private:
    Profile(std::filesystem::path root, int directory, std::filesystem::path storage);
    std::filesystem::path root_, storage_;
    int directory_ = -1;
#endif
};

// Normal play's one-time copy of the previous default profile, made before the
// destination is acquired. Any existing destination wins unchanged, and a
// missing, unmarked or linked source is left alone. Otherwise the source stays
// locked and is only read while its save, save backup, settings, stored ROM
// and import record are verified into a new sibling folder owned by this
// attempt. That folder is marked last and renamed to the absent destination.
// A source in use, or any failure, throws a Failure; a failed attempt keeps
// its folder, unused, and names it.
struct Migration {
    enum Outcome { DestinationExists, SourceMissing, SourceUnrecognized, Migrated } outcome;
    std::string note; // One log line, written once the profile log is open.
};
Migration migrate_profile(const std::filesystem::path& previous, const std::filesystem::path& destination);
#ifndef _WIN32
// Linux: the Scurvy Dog Port profile folder, renamed whole (same format) to
// the Steam Deck Port folder before migrate_profile runs. Any existing
// destination wins; a missing, unmarked, linked or in-use source is left alone.
std::filesystem::path renamed_profile_root(); // $XDG_DATA_HOME/BanjoTooieScurvyDogPort
Migration rename_profile(const std::filesystem::path& previous, const std::filesystem::path& destination);
#endif

enum class ImportResult { Imported, SourceMissing, WrongSize, DestinationNotEmpty };
// Copies one ordinary save into a profile whose saves folder is absent or
// empty. The source is only read and is held against writers until the copy
// verifies; an existing destination is never replaced.
ImportResult import_save(const std::filesystem::path& source, const std::filesystem::path& root);
// One explicit consent per profile, only while it has no saves; the answer is recorded.
bool offer_import(const std::filesystem::path& source, const std::filesystem::path& root,
    const std::function<bool(const std::filesystem::path&)>& ask);
// QA --import-save, otherwise the default profile's one-time offer.
void prepare_saves(const Arguments& arguments, const std::filesystem::path& root);

// Appends stdout/stderr to <profile>\player.log, keeping one previous log.
void open_log(const std::filesystem::path& root);
std::optional<std::filesystem::path> pick_rom(); // Empty when cancelled.
bool ask_retry(const std::string& text);
// QA only, once and before any runtime thread: later errors go to the log, never a dialog.
void set_log_only_errors() noexcept;
// Adds the log location; returns whether a dialog was shown.
bool show_error(const std::string& text) noexcept;
std::string utf8(const std::filesystem::path& path);
}
