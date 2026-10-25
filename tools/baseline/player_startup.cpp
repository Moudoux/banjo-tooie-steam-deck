// Windows only: Baseline.cmake rejects other hosts.
#define _CRT_SECURE_NO_WARNINGS
#include "player_startup.hpp"
#include "player_version.hpp"
#include <array>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <string_view>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>

namespace fs = std::filesystem;

namespace baseline::player {
namespace {
constexpr wchar_t product_folder[] = L"BanjoTooieScurvyDogPort";
constexpr wchar_t previous_folder[] = L"BanjoTooieEncore"; // Alpha 4 default; only ever read.
constexpr wchar_t legacy_folder[] = L"BanjoTooieRecompiled";
constexpr wchar_t title[] = L"Banjo-Tooie: Scurvy Dog Port";
// The runtime's stored ROM, <profile>\<game_id>.z64; the largest file a migration copies.
constexpr wchar_t rom_name[] = L"bt.n64.us.1.0.z64";
constexpr DWORD copy_limit = 64u << 20;
using SaveBytes = std::array<char, save_bytes>;
fs::path log_file;
std::atomic<bool> log_only_errors{false};

[[noreturn]] void fail(const std::string& text) { throw Failure(text); }
std::string windows_error(DWORD error) { return " (Windows error " + std::to_string(error) + ")"; }
std::string in_use(const fs::path& root) {
    return "Another copy of Banjo-Tooie: Scurvy Dog Port is already using " + utf8(root) +
        ". Close it, then start the game again.";
}

std::wstring widen(const std::string& text) {
    if (text.empty()) return {};
    // Player text is UTF-8; a runtime message in the ANSI code page still displays.
    for (const UINT page : {UINT(CP_UTF8), UINT(CP_ACP)}) {
        const DWORD flags = page == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0;
        const int length = MultiByteToWideChar(page, flags, text.data(), int(text.size()), nullptr, 0);
        if (length <= 0) continue;
        std::wstring result(size_t(length), L'\0');
        MultiByteToWideChar(page, flags, text.data(), int(text.size()), result.data(), length);
        return result;
    }
    return L"(The message could not be displayed.)";
}

fs::path local_app_data() {
    PWSTR folder = nullptr;
    fs::path path;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &folder)) && folder) path = folder;
    CoTaskMemFree(folder);
    if (path.empty()) fail("Windows did not report your local application data folder, so no profile could be chosen.");
    return path;
}

struct File {
    HANDLE handle;
    explicit File(HANDLE value) : handle(value) {}
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    ~File() { if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
    explicit operator bool() const { return handle != INVALID_HANDLE_VALUE; }
};

// Exactly size bytes, then end of file.
bool read_exact(HANDLE handle, char* data, DWORD size) {
    for (DWORD total = 0; total < size;) {
        DWORD bytes = 0;
        if (!ReadFile(handle, data + total, size - total, &bytes, nullptr) || bytes == 0) return false;
        total += bytes;
    }
    char extra = 0;
    DWORD bytes = 0;
    return ReadFile(handle, &extra, 1, &bytes, nullptr) && bytes == 0;
}

bool file_equals(const fs::path& path, std::string_view expected) {
    File file(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    std::string actual(expected.size(), '\0');
    return file && read_exact(file.handle, actual.data(), DWORD(actual.size())) && actual == expected;
}

bool write_new(const fs::path& path, std::string_view bytes) {
    File file(CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
    DWORD written = 0;
    return file && WriteFile(file.handle, bytes.data(), DWORD(bytes.size()), &written, nullptr) &&
        written == bytes.size() && FlushFileBuffers(file.handle);
}

// An absent or empty saves folder holds no progress. Links are never followed.
bool saves_empty(const fs::path& saves) {
    const DWORD attributes = GetFileAttributesW(saves.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) return true;
        fail("The saves folder " + utf8(saves) + " could not be inspected" + windows_error(error) + ".");
    }
    if (!(attributes & FILE_ATTRIBUTE_DIRECTORY) || (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        fail("The saves location " + utf8(saves) + " is not an ordinary folder. Nothing was copied.");
    std::error_code error;
    const bool empty = fs::is_empty(saves, error);
    if (error) fail("The saves folder " + utf8(saves) + " could not be read (" + error.message() + ").");
    return empty;
}

// Links, folders and missing entries have no ordinary size.
std::optional<std::uintmax_t> ordinary_size(const fs::path& path) {
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data) ||
        (data.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
        return std::nullopt;
    return (std::uintmax_t(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
}

// Exclusive like the engineering ProfileLock: no sharing while the handle is held.
HANDLE open_marker(const fs::path& root) {
    return CreateFileW((root / marker_name).c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
}

bool marker_matches(HANDLE marker, const fs::path& root) {
    BY_HANDLE_FILE_INFORMATION info{};
    char text[sizeof(marker_text)]{};
    DWORD bytes = 0;
    if (!GetFileInformationByHandle(marker, &info)) {
        const DWORD error = GetLastError();
        fail("The profile marker in " + utf8(root) + " could not be inspected" + windows_error(error) +
            ". The original profile is unchanged.");
    }
    if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) return false;
    if (!ReadFile(marker, text, sizeof(text), &bytes, nullptr)) {
        const DWORD error = GetLastError();
        fail("The profile marker in " + utf8(root) + " could not be read" + windows_error(error) +
            ". The original profile is unchanged.");
    }
    return std::string_view(text, bytes) == std::string_view(marker_text, sizeof(marker_text) - 1);
}

// Held without delete sharing, so the folder cannot be renamed or replaced
// meanwhile. A link is opened as itself, never followed.
HANDLE open_folder(const fs::path& path) {
    return CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
}

bool ordinary_folder(HANDLE folder, const fs::path& path) {
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(folder, &info)) {
        const DWORD error = GetLastError();
        fail("The profile folder " + utf8(path) + " could not be inspected" + windows_error(error) +
            ". The original profile is unchanged.");
    }
    return (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        !(info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
}

// One allowlisted profile file into a new target; an absent source is skipped.
// Shared read only: a file still open for writing is refused, and none can start
// writing until the copy has verified. The source is never written.
void copy_ordinary(const fs::path& source, const fs::path& target) {
    File input(CreateFileW(source.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (!input) {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND) return;
        if (error == ERROR_SHARING_VIOLATION) fail(utf8(source) + " is in use by another program");
        fail(utf8(source) + " could not be opened" + windows_error(error));
    }
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(input.handle, &info) ||
        (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
        fail(utf8(source) + " is not an ordinary file");
    if (info.nFileSizeHigh || info.nFileSizeLow > copy_limit) fail(utf8(source) + " is unexpectedly large");
    std::string bytes(info.nFileSizeLow, '\0');
    if (!read_exact(input.handle, bytes.data(), DWORD(bytes.size())))
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

bool ask_import(const fs::path& source) {
    const auto text = "Saved progress from an earlier Banjo-Tooie: Recompiled alpha was found:\n" + utf8(source) +
        "\n\nCopy it into your new Banjo-Tooie: Scurvy Dog Port profile?\n\nThe original file is left unchanged. "
        "Settings and practice states are not copied. You will not be asked again.";
    return MessageBoxW(nullptr, widen(text).c_str(), title,
        MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL | MB_SETFOREGROUND) == IDYES;
}
}

std::string utf8(const fs::path& path) {
    const auto text = path.u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}

std::vector<std::wstring> command_line() {
    int count = 0;
    LPWSTR* values = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!values) fail("The command line could not be read.");
    std::vector<std::wstring> result;
    for (int i = 1; i < count; ++i) result.emplace_back(values[i]);
    LocalFree(values);
    return result;
}

fs::path default_profile_root() { return local_app_data() / product_folder; }
fs::path previous_profile_root() { return local_app_data() / previous_folder; }
fs::path legacy_save_path() { return local_app_data() / legacy_folder / L"saves" / save_name; }

Profile::Profile(Profile&& other) noexcept
    : root_(std::move(other.root_)), handle_(std::exchange(other.handle_, nullptr)) {}
Profile::~Profile() { if (handle_) CloseHandle(handle_); }

Profile Profile::acquire(const fs::path& requested) {
    const auto root = fs::absolute(requested).lexically_normal();
    DWORD attributes = GetFileAttributesW(root.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
            fail("The profile folder " + utf8(root) + " could not be inspected" + windows_error(error) + ".");
        if (!CreateDirectoryW(root.c_str(), nullptr)) {
            const DWORD create_error = GetLastError();
            if (create_error != ERROR_ALREADY_EXISTS)
                fail("The profile folder " + utf8(root) + " could not be created" + windows_error(create_error) +
                    ". Check that its parent folder exists and can be written.");
        }
        attributes = GetFileAttributesW(root.c_str());
    }
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY) ||
        (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        fail("The profile location " + utf8(root) + " is not an ordinary folder. Rename or remove it, "
            "then start the game again. Nothing was changed.");
    const auto marker = root / marker_name;
    HANDLE handle = open_marker(root);
    if (handle != INVALID_HANDLE_VALUE) {
        Profile profile(root, handle);
        if (!marker_matches(handle, root))
            fail("The folder " + utf8(root) + " has a profile marker this version does not recognize. "
                "Nothing was changed.");
        return profile;
    }
    DWORD error = GetLastError();
    if (error == ERROR_SHARING_VIOLATION) fail(in_use(root));
    if (error != ERROR_FILE_NOT_FOUND)
        fail("The profile folder " + utf8(root) + " could not be opened" + windows_error(error) + ".");
    std::error_code listing;
    const bool empty = fs::is_empty(root, listing);
    if (listing) fail("The profile folder " + utf8(root) + " could not be read (" + listing.message() + ").");
    if (!empty)
        fail("The folder " + utf8(root) + " already contains files that are not a Banjo-Tooie: Scurvy Dog Port profile. "
            "Move them elsewhere or choose another folder. Nothing was changed.");
    // CREATE_NEW: of two simultaneous first launches exactly one owns the folder.
    handle = CreateFileW(marker.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        if (error == ERROR_FILE_EXISTS || error == ERROR_SHARING_VIOLATION) fail(in_use(root));
        fail("The profile folder " + utf8(root) + " could not be marked" + windows_error(error) +
            ". Check that the folder can be written.");
    }
    DWORD written = 0;
    if (!WriteFile(handle, marker_text, DWORD(sizeof(marker_text) - 1), &written, nullptr) ||
        written != sizeof(marker_text) - 1 || !FlushFileBuffers(handle)) {
        error = GetLastError();
        CloseHandle(handle);
        DeleteFileW(marker.c_str()); // Back to the empty folder that was found.
        fail("The profile folder " + utf8(root) + " could not be marked" + windows_error(error) + ".");
    }
    return Profile(root, handle);
}

Migration migrate_profile(const fs::path& previous, const fs::path& destination) {
    const auto normal = [](const fs::path& path) {
        const auto result = fs::absolute(path).lexically_normal();
        return result.has_filename() ? result : result.parent_path();
    };
    const auto old_root = normal(previous), new_root = normal(destination);
    const auto kept = "Nothing was copied, and your progress in " + utf8(old_root) + " is unchanged.";
    // Any existing destination wins, even an empty one; nothing is merged into it.
    if (GetFileAttributesW(new_root.c_str()) != INVALID_FILE_ATTRIBUTES)
        return {Migration::DestinationExists, "player profile migration: none; " + utf8(new_root) + " exists"};
    if (const DWORD error = GetLastError(); error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
        fail("The profile folder " + utf8(new_root) + " could not be inspected" + windows_error(error) + ". " + kept);
    File old_folder(open_folder(old_root));
    if (!old_folder) {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
            return {Migration::SourceMissing, "player profile migration: none; no " + utf8(old_root)};
        fail("The earlier profile " + utf8(old_root) + " could not be opened" + windows_error(error) + ". " + kept);
    }
    // Only a folder the earlier version owned is copied; that version refused anything else too.
    const Migration unrecognized{Migration::SourceUnrecognized,
        "player profile migration: none; " + utf8(old_root) + " is not a recognized profile and was left unchanged"};
    if (!ordinary_folder(old_folder.handle, old_root)) return unrecognized;
    File marker(open_marker(old_root));
    if (!marker) {
        const DWORD error = GetLastError();
        if (error == ERROR_SHARING_VIOLATION) fail(in_use(old_root) + " " + kept);
        if (error != ERROR_FILE_NOT_FOUND)
            fail("The earlier profile " + utf8(old_root) + " could not be opened" + windows_error(error) + ". " + kept);
        return unrecognized;
    }
    if (!marker_matches(marker.handle, old_root)) return unrecognized;
    const auto old_saves = old_root / L"saves";
    File saves(open_folder(old_saves));
    if (!saves) {
        const DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND)
            fail("The earlier saves folder " + utf8(old_saves) + " could not be opened" + windows_error(error) + ". " + kept);
    } else if (!ordinary_folder(saves.handle, old_saves)) {
        fail("The earlier saves location " + utf8(old_saves) + " is not an ordinary folder. " + kept);
    }
    // A new sibling owned by this attempt alone, never the profile folder itself.
    // Marked only once complete; a failed attempt leaves it in place for the player.
    fs::path stage;
    for (unsigned attempt = 1;; ++attempt) {
        stage = new_root;
        stage += L".migrating-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(attempt);
        if (CreateDirectoryW(stage.c_str(), nullptr)) break;
        const DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS || attempt == 100)
            fail("No folder for the copy could be created next to " + utf8(new_root) + windows_error(error) + ". " + kept);
    }
    try {
        if (saves) {
            const auto stage_saves = stage / L"saves";
            if (!CreateDirectoryW(stage_saves.c_str(), nullptr)) {
                const DWORD error = GetLastError();
                fail("the saves folder could not be created" + windows_error(error));
            }
            // The runtime reads <save>.bak when the save itself cannot be opened.
            for (const auto& name : {std::wstring(save_name), std::wstring(save_name) + L".bak"})
                copy_ordinary(old_saves / name, stage / L"saves" / name);
        }
        for (const wchar_t* name : {L"tooie_imgui.json", rom_name, import_record_name})
            copy_ordinary(old_root / name, stage / name);
        if (!write_new(stage / marker_name, {marker_text, sizeof(marker_text) - 1}))
            fail("the profile marker could not be written");
        // No replace flag: a destination that appeared meanwhile wins.
        if (!MoveFileExW(stage.c_str(), new_root.c_str(), 0)) {
            const DWORD error = GetLastError();
            if (error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS)
                fail(utf8(new_root) + " appeared meanwhile and was left as it is");
            fail("the copy could not be renamed to " + utf8(new_root) + windows_error(error));
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
    const auto saves = root / L"saves";
    if (!saves_empty(saves)) return ImportResult::DestinationNotEmpty;
    const DWORD attributes = GetFileAttributesW(source.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) return ImportResult::SourceMissing;
    if (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
        fail("The save to import, " + utf8(source) + ", is not an ordinary file. Nothing was copied.");
    // Shared read only: a program still writing it is refused, and none can
    // start writing until the copy has verified. The source is never written.
    File input(CreateFileW(source.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (!input) {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) return ImportResult::SourceMissing;
        if (error == ERROR_SHARING_VIOLATION)
            fail("The earlier save " + utf8(source) + " is in use. Close Banjo-Tooie: Recompiled, "
                "then start Scurvy Dog Port again. Nothing was copied.");
        fail("The earlier save " + utf8(source) + " could not be opened" + windows_error(error) +
            ". Nothing was copied.");
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(input.handle, &size))
        fail("The earlier save " + utf8(source) + " could not be read. Nothing was copied.");
    if (std::uintmax_t(size.QuadPart) != save_bytes) return ImportResult::WrongSize;
    SaveBytes bytes{};
    if (!read_exact(input.handle, bytes.data(), DWORD(bytes.size())))
        fail("The earlier save " + utf8(source) + " could not be read completely. Nothing was copied.");
    if (!CreateDirectoryW(saves.c_str(), nullptr)) {
        const DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS)
            fail("The saves folder " + utf8(saves) + " could not be created" + windows_error(error) + ".");
    }
    if (!saves_empty(saves)) return ImportResult::DestinationNotEmpty;
    const auto staging = root / L"save-import.partial";
    DeleteFileW(staging.c_str()); // An interrupted earlier copy; never a save.
    const std::string_view data(bytes.data(), bytes.size());
    if (!write_new(staging, data) || !file_equals(staging, data)) {
        DeleteFileW(staging.c_str());
        fail("Your earlier save could not be copied into " + utf8(root) +
            ". Check free disk space and folder permissions. The original is unchanged.");
    }
    const auto target = saves / save_name;
    // No MOVEFILE_REPLACE_EXISTING: an existing save is never replaced.
    if (!MoveFileExW(staging.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH)) {
        const DWORD error = GetLastError();
        DeleteFileW(staging.c_str());
        if (error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS) return ImportResult::DestinationNotEmpty;
        fail("Your earlier save could not be placed in " + utf8(saves) + windows_error(error) +
            ". The original is unchanged.");
    }
    if (!file_equals(target, data)) {
        DeleteFileW(target.c_str());
        fail("The copied save did not verify and was removed. The original is unchanged.");
    }
    return ImportResult::Imported;
}

bool offer_import(const fs::path& source, const fs::path& root,
    const std::function<bool(const fs::path&)>& ask) {
    const auto record = root / import_record_name;
    std::error_code error;
    if (fs::exists(fs::symlink_status(record, error)) || !saves_empty(root / L"saves")) return false;
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
    const auto path = root / L"player.log";
    MoveFileExW(path.c_str(), (root / L"player-previous.log").c_str(), MOVEFILE_REPLACE_EXISTING);
    // Both streams append to one shareable file; a failure leaves startup unlogged, not blocked.
    if (!_wfreopen(path.c_str(), L"a", stderr)) return;
    std::setvbuf(stderr, nullptr, _IONBF, 0);
    if (_wfreopen(path.c_str(), L"a", stdout)) std::setvbuf(stdout, nullptr, _IONBF, 0);
    log_file = path;
    std::fprintf(stderr, "%s %s\nprofile=%s\n", build::product, build::version, utf8(root).c_str());
}

std::optional<fs::path> pick_rom() {
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    std::vector<wchar_t> buffer(32768, L'\0');
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFilter = L"N64 ROM (*.z64;*.n64;*.v64)\0*.z64;*.n64;*.v64\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = buffer.data();
    dialog.nMaxFile = DWORD(buffer.size());
    dialog.lpstrTitle = L"Banjo-Tooie: Scurvy Dog Port - choose your Banjo-Tooie (USA) ROM";
    dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR |
        OFN_DONTADDTORECENT | OFN_HIDEREADONLY;
    const BOOL chosen = GetOpenFileNameW(&dialog);
    const DWORD error = chosen ? 0 : CommDlgExtendedError();
    if (SUCCEEDED(com)) CoUninitialize();
    if (chosen) return fs::path(buffer.data());
    if (error) fail("The file picker could not open (error " + std::to_string(error) + ").");
    return std::nullopt;
}

bool ask_retry(const std::string& text) {
    return MessageBoxW(nullptr, widen(text).c_str(), title,
        MB_RETRYCANCEL | MB_ICONWARNING | MB_TASKMODAL | MB_SETFOREGROUND) == IDRETRY;
}

void set_log_only_errors() noexcept { log_only_errors.store(true, std::memory_order_release); }

bool show_error(const std::string& text) noexcept {
    try {
        std::fprintf(stderr, "player error: %s\n", text.c_str());
        if (log_only_errors.load(std::memory_order_acquire)) return false;
        auto message = text;
        if (!log_file.empty()) message += "\n\nDetails were written to:\n" + utf8(log_file);
        MessageBoxW(nullptr, widen(message).c_str(), title,
            MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_SETFOREGROUND | MB_TOPMOST);
        return true;
    } catch (...) {
        return false;
    }
}
}
