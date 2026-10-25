// Deterministic promotion/custody boundaries; each run creates a new directory under PARENT.
#include "runtime_save_root.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;
namespace {
int failures = 0;
unsigned long process_id() {
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return static_cast<unsigned long>(::getpid());
#endif
}
void check(bool ok, const char* what) {
    if (!ok) { std::fprintf(stderr, "save_publication FAIL: %s\n", what); ++failures; }
}
std::vector<char> bytes(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
void put(const fs::path& path, const std::vector<char>& data) {
    std::ofstream(path, std::ios::binary).write(data.data(), std::streamsize(data.size()));
}
fs::path with(fs::path path, const char* suffix) { path += suffix; return path; }
}

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    const std::string mode = argv[1];
    const auto root = fs::path(argv[2]) /
        (mode + "-" + std::to_string(process_id()) + "-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directories(root)) return 2;
    const auto save = root / "save.bin", temp = with(save, ".temp"), bak = with(save, ".bak"),
        bak_stage = with(save, ".bak.temp");
    const std::vector<char> a(0x800, 'A'), b(0x800, 'B'), c(0x800, 'C'), partial(0x100, 'S');
    put(temp, b); // The saver's closed temporary.
    if (mode == "main-absent") {
        check(tooie::promote_save_file(save, b.size()), "promotes");
        check(bytes(save) == b && !fs::exists(bak) && !fs::exists(temp), "main new, no backup, temporary consumed");
    } else if (mode == "malformed-main") {
        put(save, partial); put(bak, c);
        check(tooie::promote_save_file(save, b.size()), "promotes");
        check(bytes(save) == b && bytes(bak) == c, "complete backup kept, not overwritten by a short main");
    } else if (mode == "both-valid") {
        put(save, a); put(bak, c);
        check(tooie::promote_save_file(save, b.size()), "promotes");
        check(bytes(save) == b && bytes(bak) == a, "backup refreshed from the complete previous main");
    } else if (mode == "temporary-incomplete") {
        put(save, a); put(temp, partial);
        check(!tooie::promote_save_file(save, b.size()), "refuses an incomplete temporary");
        check(bytes(save) == a && !fs::exists(bak), "main and backup untouched");
    } else if (mode == "backup-refused") {
        put(save, a); put(bak, c); fs::create_directory(bak_stage);
        check(!tooie::promote_save_file(save, b.size()), "refuses when the backup copy fails");
        check(bytes(save) == a && bytes(bak) == c && bytes(temp) == b, "main, backup and new temporary kept");
    } else if (mode == "promotion-refused") {
        // Both native rename paths refuse promotion of a regular temporary onto a directory.
        fs::create_directory(save); put(bak, c);
        check(!tooie::promote_save_file(save, b.size()), "refuses promotion onto a directory");
        check(fs::is_directory(save) && bytes(bak) == c && bytes(temp) == b, "nothing replaced");
    } else {
        return 2;
    }
    std::fprintf(stderr, "save_publication %s %s\n", mode.c_str(), failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
