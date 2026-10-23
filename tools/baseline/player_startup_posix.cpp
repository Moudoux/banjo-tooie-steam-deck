// Linux counterpart of player_startup.cpp; Baseline.cmake selects one by host.
// Same profile marker, messages and never-replace rules. flock on the marker
// stands in for exclusive Windows handles, and renameat2(RENAME_NOREPLACE) for
// MoveFileExW without replace.
#include "player_startup.hpp"
#include "player_version.hpp"
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string_view>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include "SDL.h"
#include "nfd.h"

namespace fs = std::filesystem;

namespace baseline::player {
namespace {
constexpr char product_folder[] = "BanjoTooieScurvyDogPort";
constexpr char previous_folder[] = "BanjoTooieEncore"; // Alpha 4 default; only ever read.
constexpr char legacy_folder[] = "BanjoTooieRecompiled";
constexpr char title[] = "Banjo-Tooie: Scurvy Dog Port";
// The runtime's stored ROM, <profile>/<game_id>.z64; the largest file a migration copies.
constexpr char rom_name[] = "bt.n64.us.1.0.z64";
constexpr off_t copy_limit = 64 << 20;
using SaveBytes = std::array<char, save_bytes>;
fs::path log_file;
std::atomic<bool> log_only_errors{false};

[[noreturn]] void fail(const std::string& text) { throw Failure(text); }
std::string system_error(int error) { return " (" + std::string(std::strerror(error)) + ")"; }
std::string in_use(const fs::path& root) {
    return "Another copy of Banjo-Tooie: Scurvy Dog Port is already using " + utf8(root) +
        ". Close it, then start the game again.";
}

fs::path data_home() {
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && *xdg == '/') return xdg;
    const char* home = std::getenv("HOME");
    if (!home || *home != '/') fail("HOME is not set, so no profile could be chosen.");
    return fs::path(home) / ".local" / "share";
}

struct File {
    int fd;
    explicit File(int value) : fd(value) {}
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    ~File() { if (fd >= 0) ::close(fd); }
    explicit operator bool() const { return fd >= 0; }
};

// Exactly size bytes, then end of file.
bool read_exact(int fd, char* data, size_t size) {
    for (size_t total = 0; total < size;) {
        const ssize_t bytes = ::read(fd, data + total, size - total);
        if (bytes < 0 && errno == EINTR) continue;
        if (bytes <= 0) return false;
        total += size_t(bytes);
    }
    char extra = 0;
    return ::read(fd, &extra, 1) == 0;
}

bool file_equals(const fs::path& path, std::string_view expected) {
    File file(::open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    std::string actual(expected.size(), '\0');
    return file && read_exact(file.fd, actual.data(), actual.size()) && actual == expected;
}

bool write_new(const fs::path& path, std::string_view bytes) {
    File file(::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0644));
    if (!file) return false;
    for (size_t total = 0; total < bytes.size();) {
        const ssize_t written = ::write(file.fd, bytes.data() + total, bytes.size() - total);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) return false;
        total += size_t(written);
    }
    return ::fsync(file.fd) == 0;
}

// Fails with EEXIST instead of replacing an existing destination.
bool rename_new(const fs::path& from, const fs::path& to) {
    return ::renameat2(AT_FDCWD, from.c_str(), AT_FDCWD, to.c_str(), RENAME_NOREPLACE) == 0;
}

// An absent or empty saves folder holds no progress. Links are never followed.
bool saves_empty(const fs::path& saves) {
    struct stat info{};
    if (::lstat(saves.c_str(), &info) != 0) {
        const int error = errno;
        if (error == ENOENT || error == ENOTDIR) return true;
        fail("The saves folder " + utf8(saves) + " could not be inspected" + system_error(error) + ".");
    }
    if (!S_ISDIR(info.st_mode))
        fail("The saves location " + utf8(saves) + " is not an ordinary folder. Nothing was copied.");
    std::error_code error;
    const bool empty = fs::is_empty(saves, error);
    if (error) fail("The saves folder " + utf8(saves) + " could not be read (" + error.message() + ").");
    return empty;
}

// Links, folders and missing entries have no ordinary size.
std::optional<std::uintmax_t> ordinary_size(const fs::path& path) {
    struct stat info{};
    if (::lstat(path.c_str(), &info) != 0 || !S_ISREG(info.st_mode)) return std::nullopt;
    return std::uintmax_t(info.st_size);
}

// Held with an exclusive lock for the profile's lifetime; a link is never followed.
int open_marker(const fs::path& root, int flags = 0) {
    const int fd = ::open((root / marker_name).c_str(), O_RDWR | O_NOFOLLOW | O_CLOEXEC | flags, 0644);
    if (fd < 0) return -1;
    if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
        const int error = errno;
        ::close(fd);
        errno = error == EWOULDBLOCK ? EBUSY : error;
        return -1;
    }
    return fd;
}

bool marker_matches(int marker, const fs::path& root) {
    char text[sizeof(marker_text)]{};
    const ssize_t bytes = ::pread(marker, text, sizeof(text), 0);
    if (bytes < 0) {
        const int error = errno;
        fail("The profile marker in " + utf8(root) + " could not be read" + system_error(error) +
            ". The original profile is unchanged.");
    }
    return std::string_view(text, size_t(bytes)) == std::string_view(marker_text, sizeof(marker_text) - 1);
}

bool ordinary_folder(const fs::path& path) {
    struct stat info{};
    return ::lstat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

// One allowlisted profile file into a new target; an absent source is skipped.
// The source is never written.
void copy_ordinary(const fs::path& source, const fs::path& target) {
    File input(::open(source.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    if (!input) {
        const int error = errno;
        if (error == ENOENT) return;
        fail(utf8(source) + " could not be opened" + system_error(error));
    }
    struct stat info{};
    if (::fstat(input.fd, &info) != 0 || !S_ISREG(info.st_mode)) fail(utf8(source) + " is not an ordinary file");
    if (info.st_size > copy_limit) fail(utf8(source) + " is unexpectedly large");
    std::string bytes(size_t(info.st_size), '\0');
    if (!read_exact(input.fd, bytes.data(), bytes.size()))
        fail(utf8(source) + " could not be read completely");
    if (!write_new(target, bytes) || !file_equals(target, bytes))
        fail(utf8(target) + " could not be written and verified");
}

void write_record(const fs::path& path, const std::string& text) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << text << '\n';
    output.flush();
    if (!output) std::fprintf(stderr, "player could not write %s\n", utf8(path).c_str());
}

// SDL message boxes need no SDL_Init and fall back to the log when no display is available.
int ask(const std::string& text, Uint32 flags, std::initializer_list<SDL_MessageBoxButtonData> buttons) {
    const std::vector<SDL_MessageBoxButtonData> list(buttons);
    const SDL_MessageBoxData data{flags, nullptr, title, text.c_str(), int(list.size()), list.data(), nullptr};
    int chosen = -1;
    if (SDL_ShowMessageBox(&data, &chosen) != 0) {
        std::fprintf(stderr, "player dialog unavailable: %s\n", SDL_GetError());
        return -1;
    }
    return chosen;
}

bool ask_import(const fs::path& source) {
    const auto text = "Saved progress from an earlier Banjo-Tooie: Recompiled alpha was found:\n" + utf8(source) +
        "\n\nCopy it into your new Banjo-Tooie: Scurvy Dog Port profile?\n\nThe original file is left unchanged. "
        "Settings and practice states are not copied. You will not be asked again.";
    return ask(text, SDL_MESSAGEBOX_INFORMATION, {
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "No"},
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes"}}) == 1;
}
}

std::string utf8(const fs::path& path) {
    const auto text = path.u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}

std::vector<std::wstring> command_line() {
    std::ifstream input("/proc/self/cmdline", std::ios::binary);
    if (!input) fail("The command line could not be read.");
    std::vector<std::wstring> result;
    bool first = true;
    for (std::string value; std::getline(input, value, '\0'); first = false)
        if (!first) result.push_back(fs::path(value).wstring());
    return result;
}

Arguments parse_arguments(const std::vector<std::wstring>& arguments) {
    Arguments result;
    if (arguments.empty()) return result;
    const auto usage = [] {
        return Failure("Usage: BanjoTooieScurvyDogPort [--profile-dir FOLDER [--rom FILE] [--import-save FILE] "
            "[--seconds 1..900 [--input-script FILE]]]");
    };
    bool seconds_seen = false;
    for (size_t i = 0; i < arguments.size(); i += 2) {
        if (i + 1 == arguments.size()) throw usage();
        const auto& name = arguments[i];
        const auto& value = arguments[i + 1];
        const auto take = [&](fs::path& slot) {
            if (!slot.empty() || value.empty()) throw usage();
            slot = value;
        };
        if (name == L"--profile-dir") take(result.profile);
        else if (name == L"--rom") take(result.rom);
        else if (name == L"--import-save") take(result.import_save);
        else if (name == L"--input-script") take(result.input_script);
        else if (name == L"--seconds") {
            if (seconds_seen) throw usage();
            seconds_seen = true;
            unsigned seconds = 0;
            bool valid = !value.empty() && value.size() <= 3;
            for (const wchar_t c : value) {
                if (c < L'0' || c > L'9') valid = false;
                else seconds = seconds * 10 + unsigned(c - L'0');
            }
            if (!valid || seconds < 1 || seconds > 900) throw Failure("--seconds must be 1..900");
            result.seconds = seconds;
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

fs::path default_profile_root() { return data_home() / product_folder; }
fs::path previous_profile_root() { return data_home() / previous_folder; }
fs::path legacy_save_path() { return data_home() / legacy_folder / "saves" / save_name; }

Profile::Profile(Profile&& other) noexcept
    : root_(std::move(other.root_)), handle_(std::exchange(other.handle_, nullptr)) {}
Profile::~Profile() { if (handle_) ::close(int(reinterpret_cast<intptr_t>(handle_)) - 1); }

Profile Profile::acquire(const fs::path& requested) {
    const auto root = fs::absolute(requested).lexically_normal();
    // The descriptor is stored off by one so that descriptor 0 is not the empty handle.
    const auto owned = [&](int fd) { return Profile(root, reinterpret_cast<void*>(intptr_t(fd) + 1)); };
    struct stat info{};
    if (::lstat(root.c_str(), &info) != 0) {
        const int error = errno;
        if (error != ENOENT)
            fail("The profile folder " + utf8(root) + " could not be inspected" + system_error(error) + ".");
        if (::mkdir(root.c_str(), 0755) != 0 && errno != EEXIST)
            fail("The profile folder " + utf8(root) + " could not be created" + system_error(errno) +
                ". Check that its parent folder exists and can be written.");
    }
    if (!ordinary_folder(root))
        fail("The profile location " + utf8(root) + " is not an ordinary folder. Rename or remove it, "
            "then start the game again. Nothing was changed.");
    const auto marker = root / marker_name;
    int fd = open_marker(root);
    if (fd >= 0) {
        Profile profile = owned(fd);
        if (!marker_matches(fd, root))
            fail("The folder " + utf8(root) + " has a profile marker this version does not recognize. "
                "Nothing was changed.");
        return profile;
    }
    int error = errno;
    if (error == EBUSY) fail(in_use(root));
    if (error != ENOENT)
        fail("The profile folder " + utf8(root) + " could not be opened" + system_error(error) + ".");
    std::error_code listing;
    const bool empty = fs::is_empty(root, listing);
    if (listing) fail("The profile folder " + utf8(root) + " could not be read (" + listing.message() + ").");
    if (!empty)
        fail("The folder " + utf8(root) + " already contains files that are not a Banjo-Tooie: Scurvy Dog Port profile. "
            "Move them elsewhere or choose another folder. Nothing was changed.");
    // O_EXCL: of two simultaneous first launches exactly one owns the folder.
    fd = open_marker(root, O_CREAT | O_EXCL);
    if (fd < 0) {
        error = errno;
        if (error == EEXIST || error == EBUSY) fail(in_use(root));
        fail("The profile folder " + utf8(root) + " could not be marked" + system_error(error) +
            ". Check that the folder can be written.");
    }
    const ssize_t length = sizeof(marker_text) - 1;
    if (::write(fd, marker_text, length) != length || ::fsync(fd) != 0) {
        error = errno;
        ::close(fd);
        ::unlink(marker.c_str()); // Back to the empty folder that was found.
        fail("The profile folder " + utf8(root) + " could not be marked" + system_error(error) + ".");
    }
    return owned(fd);
}

Migration migrate_profile(const fs::path& previous, const fs::path& destination) {
    const auto normal = [](const fs::path& path) {
        const auto result = fs::absolute(path).lexically_normal();
        return result.has_filename() ? result : result.parent_path();
    };
    const auto old_root = normal(previous), new_root = normal(destination);
    const auto kept = "Nothing was copied, and your progress in " + utf8(old_root) + " is unchanged.";
    struct stat info{};
    // Any existing destination wins, even an empty one; nothing is merged into it.
    if (::lstat(new_root.c_str(), &info) == 0)
        return {Migration::DestinationExists, "player profile migration: none; " + utf8(new_root) + " exists"};
    if (errno != ENOENT)
        fail("The profile folder " + utf8(new_root) + " could not be inspected" + system_error(errno) + ". " + kept);
    if (::lstat(old_root.c_str(), &info) != 0) {
        if (errno == ENOENT) return {Migration::SourceMissing, "player profile migration: none; no " + utf8(old_root)};
        fail("The earlier profile " + utf8(old_root) + " could not be opened" + system_error(errno) + ". " + kept);
    }
    // Only a folder the earlier version owned is copied; that version refused anything else too.
    const Migration unrecognized{Migration::SourceUnrecognized,
        "player profile migration: none; " + utf8(old_root) + " is not a recognized profile and was left unchanged"};
    if (!S_ISDIR(info.st_mode)) return unrecognized;
    File marker(open_marker(old_root));
    if (!marker) {
        const int error = errno;
        if (error == EBUSY) fail(in_use(old_root) + " " + kept);
        if (error != ENOENT)
            fail("The earlier profile " + utf8(old_root) + " could not be opened" + system_error(error) + ". " + kept);
        return unrecognized;
    }
    if (!marker_matches(marker.fd, old_root)) return unrecognized;
    const auto old_saves = old_root / "saves";
    const bool saves = ::lstat(old_saves.c_str(), &info) == 0;
    if (!saves && errno != ENOENT)
        fail("The earlier saves folder " + utf8(old_saves) + " could not be opened" + system_error(errno) + ". " + kept);
    if (saves && !S_ISDIR(info.st_mode))
        fail("The earlier saves location " + utf8(old_saves) + " is not an ordinary folder. " + kept);
    // A new sibling owned by this attempt alone, never the profile folder itself.
    // Marked only once complete; a failed attempt leaves it in place for the player.
    fs::path stage;
    for (unsigned attempt = 1;; ++attempt) {
        stage = new_root;
        stage += ".migrating-" + std::to_string(::getpid()) + "-" + std::to_string(attempt);
        if (::mkdir(stage.c_str(), 0755) == 0) break;
        const int error = errno;
        if (error != EEXIST || attempt == 100)
            fail("No folder for the copy could be created next to " + utf8(new_root) + system_error(error) + ". " + kept);
    }
    try {
        if (saves) {
            if (::mkdir((stage / "saves").c_str(), 0755) != 0)
                fail("the saves folder could not be created" + system_error(errno));
            // The runtime reads <save>.bak when the save itself cannot be opened.
            for (const auto& name : {std::string(fs::path(save_name).string()), fs::path(save_name).string() + ".bak"})
                copy_ordinary(old_saves / name, stage / "saves" / name);
        }
        for (const fs::path name : {fs::path("tooie_imgui.json"), fs::path(rom_name), fs::path(import_record_name)})
            copy_ordinary(old_root / name, stage / name);
        if (!write_new(stage / marker_name, {marker_text, sizeof(marker_text) - 1}))
            fail("the profile marker could not be written");
        // No replace: a destination that appeared meanwhile wins.
        if (!rename_new(stage, new_root)) {
            const int error = errno;
            if (error == EEXIST || error == ENOTEMPTY)
                fail(utf8(new_root) + " appeared meanwhile and was left as it is");
            fail("the copy could not be renamed to " + utf8(new_root) + system_error(error));
        }
    } catch (const std::exception& failure) {
        fail(std::string("Your earlier profile could not be copied: ") + failure.what() + ".\n\nYour original " +
            utf8(old_root) + " is unchanged and still holds your progress. This attempt's copy is kept, unused, in " +
            utf8(stage) + "; you can delete it. Start the game again to retry.");
    }
    return {Migration::Migrated,
        "player profile migration: copied " + utf8(old_root) + " to " + utf8(new_root) + "; the original is unchanged"};
}

ImportResult import_save(const fs::path& source, const fs::path& root) {
    const auto saves = root / "saves";
    if (!saves_empty(saves)) return ImportResult::DestinationNotEmpty;
    struct stat info{};
    if (::lstat(source.c_str(), &info) != 0) return ImportResult::SourceMissing;
    if (!S_ISREG(info.st_mode))
        fail("The save to import, " + utf8(source) + ", is not an ordinary file. Nothing was copied.");
    File input(::open(source.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    if (!input) {
        const int error = errno;
        if (error == ENOENT) return ImportResult::SourceMissing;
        fail("The earlier save " + utf8(source) + " could not be opened" + system_error(error) +
            ". Nothing was copied.");
    }
    if (::fstat(input.fd, &info) != 0)
        fail("The earlier save " + utf8(source) + " could not be read. Nothing was copied.");
    if (std::uintmax_t(info.st_size) != save_bytes) return ImportResult::WrongSize;
    SaveBytes bytes{};
    if (!read_exact(input.fd, bytes.data(), bytes.size()))
        fail("The earlier save " + utf8(source) + " could not be read completely. Nothing was copied.");
    if (::mkdir(saves.c_str(), 0755) != 0 && errno != EEXIST)
        fail("The saves folder " + utf8(saves) + " could not be created" + system_error(errno) + ".");
    if (!saves_empty(saves)) return ImportResult::DestinationNotEmpty;
    const auto staging = root / "save-import.partial";
    ::unlink(staging.c_str()); // An interrupted earlier copy; never a save.
    const std::string_view data(bytes.data(), bytes.size());
    if (!write_new(staging, data) || !file_equals(staging, data)) {
        ::unlink(staging.c_str());
        fail("Your earlier save could not be copied into " + utf8(root) +
            ". Check free disk space and folder permissions. The original is unchanged.");
    }
    const auto target = saves / save_name;
    // No replace: an existing save is never replaced.
    if (!rename_new(staging, target)) {
        const int error = errno;
        ::unlink(staging.c_str());
        if (error == EEXIST) return ImportResult::DestinationNotEmpty;
        fail("Your earlier save could not be placed in " + utf8(saves) + system_error(error) +
            ". The original is unchanged.");
    }
    if (!file_equals(target, data)) {
        ::unlink(target.c_str());
        fail("The copied save did not verify and was removed. The original is unchanged.");
    }
    return ImportResult::Imported;
}

bool offer_import(const fs::path& source, const fs::path& root,
    const std::function<bool(const fs::path&)>& ask) {
    const auto record = root / import_record_name;
    std::error_code error;
    if (fs::exists(fs::symlink_status(record, error)) || !saves_empty(root / "saves")) return false;
    const auto size = ordinary_size(source);
    if (!size) return false;
    if (*size != save_bytes) {
        std::fprintf(stderr, "player ignored older save %s: %llu bytes\n", utf8(source).c_str(),
            static_cast<unsigned long long>(*size));
        return false;
    }
    if (!ask(source)) {
        write_record(record, "declined: " + utf8(source));
        return false;
    }
    if (import_save(source, root) != ImportResult::Imported)
        fail("Your earlier save changed while it was being copied and was not imported. "
            "Start the game again to retry.");
    write_record(record, "imported: " + utf8(source));
    std::fprintf(stderr, "player imported older save %s\n", utf8(source).c_str());
    return true;
}

void prepare_saves(const Arguments& arguments, const fs::path& root) {
    if (arguments.import_save.empty()) {
        if (!arguments.qa) offer_import(legacy_save_path(), root, ask_import);
        return;
    }
    switch (import_save(arguments.import_save, root)) {
    case ImportResult::Imported:
        std::fprintf(stderr, "player imported save %s\n", utf8(arguments.import_save).c_str());
        return;
    case ImportResult::DestinationNotEmpty:
        std::fprintf(stderr, "player kept the existing profile save; --import-save ignored\n");
        return;
    case ImportResult::SourceMissing:
        fail("The save to import was not found: " + utf8(arguments.import_save));
    case ImportResult::WrongSize:
        fail("The save to import is not a " + std::to_string(save_bytes) + "-byte Banjo-Tooie save: " +
            utf8(arguments.import_save));
    }
}

void open_log(const fs::path& root) {
    const auto path = root / "player.log";
    ::rename(path.c_str(), (root / "player-previous.log").c_str());
    // Both streams append to one shareable file; a failure leaves startup unlogged, not blocked.
    if (!std::freopen(path.c_str(), "a", stderr)) return;
    std::setvbuf(stderr, nullptr, _IONBF, 0);
    if (std::freopen(path.c_str(), "a", stdout)) std::setvbuf(stdout, nullptr, _IONBF, 0);
    log_file = path;
    std::fprintf(stderr, "%s %s\nprofile=%s\n", build::product, build::version, utf8(root).c_str());
}

// A ROM beside the executable, so first launch in Steam Game Mode needs no
// file dialog: baserom.us.z64, otherwise the folder's only N64 ROM file.
std::optional<fs::path> adjacent_rom() {
    std::error_code error;
    const auto folder = fs::read_symlink("/proc/self/exe", error).parent_path();
    if (error || folder.empty()) return std::nullopt;
    if (fs::is_regular_file(folder / "baserom.us.z64", error)) return folder / "baserom.us.z64";
    std::optional<fs::path> found;
    for (const auto& entry : fs::directory_iterator(folder, error)) {
        const auto extension = entry.path().extension();
        if (!entry.is_regular_file(error) || (extension != ".z64" && extension != ".n64" && extension != ".v64")) continue;
        if (found) return std::nullopt;
        found = entry.path();
    }
    return found;
}

std::optional<fs::path> pick_rom() {
    // Offered once; a rejected adjacent ROM falls through to the picker on retry.
    static bool adjacent_offered = false;
    if (!std::exchange(adjacent_offered, true)) {
        if (auto rom = adjacent_rom()) {
            std::fprintf(stderr, "player using ROM beside the executable: %s\n", utf8(*rom).c_str());
            return rom;
        }
    }
    if (NFD_Init() != NFD_OKAY) fail(std::string("The file picker could not open: ") + NFD_GetError());
    const nfdu8filteritem_t filters[] = {{"N64 ROM", "z64,n64,v64"}};
    nfdu8char_t* chosen = nullptr;
    const nfdresult_t result = NFD_OpenDialogU8(&chosen, filters, 1, nullptr);
    std::optional<fs::path> path;
    if (result == NFD_OKAY) {
        path = fs::path(chosen);
        NFD_FreePathU8(chosen);
    }
    const std::string error = result == NFD_ERROR ? NFD_GetError() : "";
    NFD_Quit();
    if (result == NFD_ERROR) fail("The file picker could not open: " + error);
    return path;
}

bool ask_retry(const std::string& text) {
    return ask(text, SDL_MESSAGEBOX_WARNING, {
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Cancel"},
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Retry"}}) == 1;
}

void set_log_only_errors() noexcept { log_only_errors.store(true, std::memory_order_release); }

bool show_error(const std::string& text) noexcept {
    try {
        std::fprintf(stderr, "player error: %s\n", text.c_str());
        if (log_only_errors.load(std::memory_order_acquire)) return false;
        auto message = text;
        if (!log_file.empty()) message += "\n\nDetails were written to:\n" + utf8(log_file);
        return SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message.c_str(), nullptr) == 0;
    } catch (...) {
        return false;
    }
}
}
