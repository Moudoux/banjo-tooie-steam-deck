#include "runtime_save_root.hpp"

#include <mutex>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {
std::mutex save_root_mutex;
std::filesystem::path save_root;
bool refuse(const char* step, unsigned long error) {
    std::fprintf(stderr, "save publication refused step=%s error=%lu\n", step, error);
    std::fflush(stderr);
    return false;
}
}

void tooie::register_save_root(std::filesystem::path root) {
    std::lock_guard lock(save_root_mutex);
    save_root = std::move(root);
}

std::filesystem::path tooie::runtime_save_folder(const std::filesystem::path& config_root) {
    std::lock_guard lock(save_root_mutex);
    return save_root.empty() ? config_root / "saves" : save_root;
}

bool tooie::promote_save_file(const std::filesystem::path& save, std::uintmax_t expected_size) noexcept {
    try {
        auto temporary = save;
        temporary += u8".temp";
        auto backup = save;
        backup += u8".bak";
        auto backup_staging = save;
        backup_staging += u8".bak.temp";
#ifdef _WIN32
        // 1. The closed temporary must be complete; flush its cached data through the OS.
        const HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return refuse("open-temporary", GetLastError());
        LARGE_INTEGER size{};
        if (!GetFileSizeEx(file, &size)) {
            const DWORD error = GetLastError(); CloseHandle(file); return refuse("temporary-size", error);
        }
        if (static_cast<std::uintmax_t>(size.QuadPart) != expected_size) {
            CloseHandle(file); return refuse("temporary-size", ERROR_BAD_LENGTH);
        }
        const bool flushed = FlushFileBuffers(file) != 0;
        const DWORD error = flushed ? ERROR_SUCCESS : GetLastError();
        const bool closed = CloseHandle(file) != 0;
        if (!flushed) return refuse("flush-temporary", error);
        if (!closed) return refuse("close-temporary", GetLastError());
        // 2. Backup custody: only an expected-size main replaces the backup, by its own rename.
        WIN32_FILE_ATTRIBUTE_DATA main{};
        if (!GetFileAttributesExW(save.c_str(), GetFileExInfoStandard, &main)) {
            const DWORD missing = GetLastError();
            if (missing != ERROR_FILE_NOT_FOUND && missing != ERROR_PATH_NOT_FOUND) return refuse("inspect-main", missing);
        } else if ((main.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                ((static_cast<std::uintmax_t>(main.nFileSizeHigh) << 32) | main.nFileSizeLow) != expected_size) {
            std::fputs("save backup kept: current main is not an expected-size image\n", stderr);
        } else if (!CopyFileW(save.c_str(), backup_staging.c_str(), FALSE)) {
            return refuse("copy-backup", GetLastError());
        } else if (!MoveFileExW(backup_staging.c_str(), backup.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            return refuse("replace-backup", GetLastError());
        }
        // 3. One replace-rename; no MOVEFILE_COPY_ALLOWED, so never a copy into main.
        if (!MoveFileExW(temporary.c_str(), save.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            return refuse("promote", GetLastError());
        return true;
#else
        const int file = ::open(temporary.c_str(), O_WRONLY | O_CLOEXEC);
        if (file < 0) return refuse("open-temporary", static_cast<unsigned long>(errno));
        struct stat info{};
        if (::fstat(file, &info) != 0) {
            const int error = errno; ::close(file); return refuse("temporary-size", static_cast<unsigned long>(error));
        }
        if (static_cast<std::uintmax_t>(info.st_size) != expected_size) {
            ::close(file); return refuse("temporary-size", static_cast<unsigned long>(EINVAL));
        }
        const bool flushed = ::fsync(file) == 0;
        const int error = flushed ? 0 : errno;
        const bool closed = ::close(file) == 0;
        if (!flushed) return refuse("flush-temporary", static_cast<unsigned long>(error));
        if (!closed) return refuse("close-temporary", static_cast<unsigned long>(errno));
        std::error_code status_error;
        const auto status = std::filesystem::status(save, status_error);
        if (status.type() != std::filesystem::file_type::not_found) {
            if (status_error) return refuse("inspect-main", static_cast<unsigned long>(status_error.value()));
            const bool regular = std::filesystem::is_regular_file(status);
            const auto main_size = regular ? std::filesystem::file_size(save, status_error) : 0;
            if (status_error) return refuse("inspect-main-size", static_cast<unsigned long>(status_error.value()));
            if (!regular || main_size != expected_size) {
                std::fputs("save backup kept: current main is not an expected-size image\n", stderr);
            } else {
                std::filesystem::copy_file(save, backup_staging, std::filesystem::copy_options::overwrite_existing, status_error);
                if (status_error) return refuse("copy-backup", static_cast<unsigned long>(status_error.value()));
                if (::rename(backup_staging.c_str(), backup.c_str()) != 0)
                    return refuse("replace-backup", static_cast<unsigned long>(errno));
            }
        }
        if (::rename(temporary.c_str(), save.c_str()) != 0) return refuse("promote", static_cast<unsigned long>(errno));
        return true;
#endif
    } catch (...) {
        return refuse("exception", 0);
    }
}
