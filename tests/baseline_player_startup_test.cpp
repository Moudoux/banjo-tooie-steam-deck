#include "player_startup.hpp"
#include <windows.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

namespace fs = std::filesystem;
using namespace baseline::player;

namespace {
int failures = 0;
void check(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}
template <class Action> bool refused(Action&& action) {
    try { action(); } catch (const Failure&) { return true; }
    return false;
}
std::string read(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
void write(const fs::path& path, const std::string& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << bytes;
}
std::string pattern(std::size_t size, int seed) {
    std::string bytes(size, '\0');
    for (std::size_t i = 0; i < size; ++i) bytes[i] = char((seed + i * 7) & 0xff);
    return bytes;
}
std::function<bool(const fs::path&)> answer(bool value, int& asked) {
    return [value, &asked](const fs::path&) { ++asked; return value; };
}
}

int main(int argc, char** argv) try {
    const fs::path base = argc > 1 ? fs::path(argv[1]) : fs::temp_directory_path() / "encore-player-test";
    const auto run = base / (L"run-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
    // Non-ASCII components exercise the wide Windows paths end to end.
    const auto root = run / L"Encore \u00e9t\u00e9 \u65e5\u672c";
    fs::create_directories(root);

    // Zero-argument play, explicit QA, nothing engineering-shaped.
    const auto play = parse_arguments({});
    check(!play.qa && play.profile.empty() && play.rom.empty() && play.seconds == 0, "zero arguments select default play");
    const auto qa = parse_arguments({L"--profile-dir", root.wstring(), L"--rom", L"x.z64", L"--seconds", L"5"});
    check(qa.qa && qa.profile == root && qa.rom == L"x.z64" && qa.seconds == 5, "QA arguments keep wide paths");
    check(refused([] { parse_arguments({L"--rom", L"x.z64"}); }), "QA ROM requires a profile");
    check(refused([] { parse_arguments({L"--condition", L"x"}); }), "engineering options are not player options");
    check(refused([] { parse_arguments({L"--profile-dir", L"a", L"--profile-dir", L"b"}); }), "duplicate profile refused");
    check(refused([] { parse_arguments({L"--profile-dir", L"a", L"--seconds", L"901"}); }), "seconds bounded");
    check(refused([] { parse_arguments({L"--profile-dir"}); }), "missing value refused");
    const auto scripted = parse_arguments({L"--profile-dir", root.wstring(), L"--seconds", L"900",
        L"--input-script", (root / L"save-reload \u00e9.txt").wstring()});
    check(scripted.qa && scripted.seconds == 900 && scripted.input_script == root / L"save-reload \u00e9.txt",
        "bounded QA input script accepted with a wide path");
    check(play.input_script.empty(), "zero-argument play never loads an input script");
    check(refused([&] { parse_arguments({L"--profile-dir", root.wstring(), L"--input-script", L"s.txt"}); }),
        "input script without an explicit duration refused");
    check(refused([] { parse_arguments({L"--input-script", L"s.txt", L"--seconds", L"5"}); }),
        "input script requires a QA profile");
    check(refused([] { parse_arguments({L"--profile-dir", L"a", L"--seconds", L"5", L"--input-script", L"s.txt",
        L"--input-script", L"t.txt"}); }), "duplicate input script refused");
    check(refused([] { parse_arguments({L"--profile-dir", L"a", L"--seconds", L"0", L"--input-script", L"s.txt"}); }),
        "input script duration must be within bounds");

    // Ownership: marked before any ROM choice, exclusive, reusable after a cancelled first launch.
    const auto fresh = root / L"fresh";
    {
        const auto profile = Profile::acquire(fresh);
        check(fs::is_regular_file(fresh / marker_name), "new profile is marked first");
        check(refused([&] { Profile::acquire(fresh); }), "second instance refused while held");
    }
    {
        // The exclusive live handle deliberately denies a second reader.
        check(read(fresh / marker_name) == marker_text, "released marker has exact ownership text");
        check(fs::is_regular_file(fresh / marker_name), "cancelled first launch leaves a marked profile");
        const auto again = Profile::acquire(fresh);
        check(again.root() == fs::absolute(fresh).lexically_normal(), "marked profile reused");
    }
    const auto empty = root / L"empty";
    fs::create_directories(empty);
    { const auto profile = Profile::acquire(empty); check(fs::is_regular_file(empty / marker_name), "empty folder adopted"); }
    const auto unknown = root / L"unknown";
    write(unknown / L"personal.txt", "keep");
    check(refused([&] { Profile::acquire(unknown); }), "unmarked non-empty folder refused");
    check(read(unknown / L"personal.txt") == "keep" && !fs::exists(unknown / marker_name), "unknown folder unchanged");
    const auto foreign = root / L"foreign";
    write(foreign / marker_name, "TooieBaseline isolated profile v1\n");
    check(refused([&] { Profile::acquire(foreign); }), "unrecognized marker refused");
    check(read(foreign / marker_name) == "TooieBaseline isolated profile v1\n", "unrecognized marker unchanged");
    write(root / L"plain-file", "x");
    check(refused([&] { Profile::acquire(root / L"plain-file"); }), "file is not a profile");
    const auto link = root / L"link";
    if (CreateSymbolicLinkW(link.c_str(), empty.c_str(), SYMBOLIC_LINK_FLAG_DIRECTORY | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
        check(refused([&] { Profile::acquire(link); }), "linked profile refused");
    else
        std::fprintf(stderr, "note: symbolic links unavailable; link refusal not exercised\n");

    // Import: byte copy into an empty destination; source and existing data preserved.
    const auto source = root / L"old \u00e9" / L"saves" / save_name;
    const auto original = pattern(save_bytes, 3);
    write(source, original);
    {
        const auto profile = Profile::acquire(root / L"import");
        const auto target = profile.root() / L"saves" / save_name;
        const auto stamp = fs::last_write_time(source);
        check(import_save(source, profile.root()) == ImportResult::Imported, "ordinary save imported");
        check(read(target) == original, "copy is byte-identical");
        check(read(source) == original && fs::last_write_time(source) == stamp, "source unchanged");
        check(!fs::exists(profile.root() / L"save-import.partial"), "no staging left");
        write(source, pattern(save_bytes, 9));
        check(import_save(source, profile.root()) == ImportResult::DestinationNotEmpty, "repeat import refused");
        check(read(target) == original, "existing destination never overwritten");
        write(source, original);
    }
    for (const std::uintmax_t size : {save_bytes - 1, save_bytes * 2}) {
        const auto profile = Profile::acquire(root / (L"size-" + std::to_wstring(size)));
        const auto wrong = root / (L"wrong-" + std::to_wstring(size) + L".bin");
        write(wrong, pattern(size_t(size), 5));
        check(import_save(wrong, profile.root()) == ImportResult::WrongSize, "wrong-size save rejected");
        check(!fs::exists(profile.root() / L"saves"), "rejected import creates nothing");
        check(read(wrong) == pattern(size_t(size), 5), "rejected source unchanged");
    }
    {
        const auto profile = Profile::acquire(root / L"missing");
        check(import_save(root / L"absent.bin", profile.root()) == ImportResult::SourceMissing, "missing source reported");
    }
    {
        const auto profile = Profile::acquire(root / L"backup");
        const auto older = profile.root() / L"saves" / (std::wstring(save_name) + L".bak");
        write(older, "older personal data");
        check(import_save(source, profile.root()) == ImportResult::DestinationNotEmpty, "non-empty saves folder refused");
        check(read(older) == "older personal data" && !fs::exists(profile.root() / L"saves" / save_name),
            "existing personal data untouched");
    }
    {
        const auto profile = Profile::acquire(root / L"busy");
        HANDLE writer = CreateFileW(source.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        check(writer != INVALID_HANDLE_VALUE, "fixture writer opened");
        check(refused([&] { import_save(source, profile.root()); }), "save still open for writing is not copied");
        if (writer != INVALID_HANDLE_VALUE) CloseHandle(writer);
        check(!fs::exists(profile.root() / L"saves" / save_name) && read(source) == original,
            "busy source created nothing and stayed unchanged");
    }

    // One explicit consent per profile, never over existing progress.
    {
        const auto profile = Profile::acquire(root / L"offer-no");
        int asked = 0;
        check(!offer_import(source, profile.root(), answer(false, asked)) && asked == 1, "consent asked");
        check(!offer_import(source, profile.root(), answer(false, asked)) && asked == 1, "declined offer not repeated");
        check(!fs::exists(profile.root() / L"saves"), "declined offer copies nothing");
    }
    {
        const auto profile = Profile::acquire(root / L"offer-yes");
        int asked = 0;
        check(offer_import(source, profile.root(), answer(true, asked)) && asked == 1, "accepted offer imports");
        check(read(profile.root() / L"saves" / save_name) == original, "accepted copy is byte-identical");
        check(!offer_import(source, profile.root(), answer(true, asked)) && asked == 1, "imported offer not repeated");
    }
    {
        const auto profile = Profile::acquire(root / L"offer-progress");
        write(profile.root() / L"saves" / save_name, pattern(save_bytes, 11));
        int asked = 0;
        check(!offer_import(source, profile.root(), answer(true, asked)) && asked == 0, "existing progress never offered over");
        check(read(profile.root() / L"saves" / save_name) == pattern(save_bytes, 11), "existing progress unchanged");
    }
    {
        const auto profile = Profile::acquire(root / L"offer-absent");
        int asked = 0;
        check(!offer_import(root / L"absent.bin", profile.root(), answer(true, asked)) && asked == 0, "no source, no question");
        check(!offer_import(root / L"wrong-2047.bin", profile.root(), answer(true, asked)) && asked == 0, "wrong size, no question");
        check(!fs::exists(profile.root() / import_record_name), "nothing recorded without a question");
    }

    // QA errors are log-only: the call returns without a dialog or waiting for one.
    set_log_only_errors();
    const auto started = GetTickCount64();
    check(!show_error("fixture: log-only QA error"), "log-only error shows no dialog");
    check(GetTickCount64() - started < 5000, "log-only error does not wait");

    std::error_code ignored;
    if (!failures) fs::remove_all(run, ignored);
    std::fprintf(failures ? stderr : stdout, "baseline player startup: %d failure(s)\n", failures);
    return failures ? 1 : 0;
} catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL: unexpected exception: %s\n", error.what());
    return 1;
}
