#pragma once
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
};
// No arguments for play, or QA: --profile-dir FOLDER [--rom FILE] [--import-save FILE]
// [--seconds 1..900 [--input-script FILE]].
Arguments parse_arguments(const std::vector<std::wstring>& arguments);
// Wide command line without the executable; Unicode paths survive.
std::vector<std::wstring> command_line();

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
private:
    Profile(std::filesystem::path root, void* handle) : root_(std::move(root)), handle_(handle) {}
    std::filesystem::path root_;
    void* handle_ = nullptr;
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
