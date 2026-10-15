#include "player_startup.hpp"
#include <windows.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

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
template <class Action> std::string failure_text(Action&& action) {
    try { action(); } catch (const Failure& failure) { return failure.what(); }
    return {};
}
// Every file below a folder with its bytes and write time, to prove a folder unchanged.
std::map<fs::path, std::pair<std::string, fs::file_time_type>> snapshot(const fs::path& folder) {
    std::map<fs::path, std::pair<std::string, fs::file_time_type>> files;
    for (const auto& entry : fs::recursive_directory_iterator(folder))
        if (entry.is_regular_file()) files[entry.path()] = {read(entry.path()), entry.last_write_time()};
    return files;
}
// Migration staging folders left next to a destination.
std::vector<fs::path> stages(const fs::path& destination) {
    std::vector<fs::path> found;
    const auto prefix = destination.filename().wstring() + L".migrating-";
    for (const auto& entry : fs::directory_iterator(destination.parent_path()))
        if (entry.path().filename().wstring().rfind(prefix, 0) == 0) found.push_back(entry.path());
    return found;
}
// A stand-in previous default profile: every allowlisted file plus files that stay behind.
void write_previous(const fs::path& old) {
    write(old / marker_name, marker_text);
    write(old / L"saves" / save_name, pattern(save_bytes, 21));
    write(old / L"saves" / (std::wstring(save_name) + L".bak"), pattern(save_bytes, 22));
    write(old / L"tooie_imgui.json", "{\"main_volume\":41}");
    write(old / L"bt.n64.us.1.0.z64", pattern(4096, 23)); // Stand-in bytes, never a ROM.
    write(old / import_record_name, "declined: C:\\earlier.bin\n");
    write(old / L"saves" / (std::wstring(save_name) + L".temp"), "unfinished write");
    write(old / L"tooie_imgui.json.tmp", "{}");
    write(old / L"encore.log", "log");
    write(old / L"encore-previous.log", "older log");
    write(old / L"rt64" / L"cache.bin", "cache");
    write(old / L"general.json", "{}");
}
}

int main(int argc, char** argv) try {
    // Integrator end-to-end fixture: migrate one disposable stand-in profile, then stop.
    // A mistake guard, not a boundary: folders directly in %LOCALAPPDATA% are refused.
    if (const auto arguments = command_line(); !arguments.empty() && arguments[0] == L"--migrate-fixture") {
        if (arguments.size() != 3) throw Failure("usage: --migrate-fixture PREVIOUS DESTINATION");
        const auto local = default_profile_root().parent_path();
        for (const auto& argument : {arguments[1], arguments[2]}) {
            auto path = fs::absolute(argument).lexically_normal();
            if (!path.has_filename()) path = path.parent_path();
            if (CompareStringOrdinal(path.parent_path().c_str(), -1, local.c_str(), -1, TRUE) == CSTR_EQUAL)
                throw Failure("--migrate-fixture refuses folders directly in %LOCALAPPDATA%");
        }
        const auto result = migrate_profile(arguments[1], arguments[2]);
        std::printf("%s\n", result.note.c_str());
        return result.outcome == Migration::Migrated ? 0 : 2;
    }
    const fs::path base = argc > 1 ? fs::path(argv[1]) : fs::temp_directory_path() / "encore-player-test";
    const auto run = base / (L"run-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
    // Non-ASCII components exercise the wide Windows paths end to end.
    const auto root = run / L"Encore \u00e9t\u00e9 \u65e5\u672c";
    fs::create_directories(root);

    // Zero-argument play, explicit QA, nothing engineering-shaped.
    const auto play = parse_arguments({});
    check(!play.qa && play.profile.empty() && play.rom.empty() && play.seconds == 0, "zero arguments select default play");
    check(default_profile_root().filename() == L"BanjoTooieScurvyDogPort", "default profile uses the current project name");
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

    // Migration: an existing destination wins, the source is only read, only the
    // allowlist is copied, and a failed attempt keeps its own staging folder.
    {
        const auto folder = root / L"migrate-none";
        fs::create_directories(folder);
        check(migrate_profile(folder / L"old", folder / L"new").outcome == Migration::SourceMissing &&
            !fs::exists(folder / L"new") && stages(folder / L"new").empty(), "no previous profile, no migration");
    }
    for (const bool progress : {false, true}) {
        const auto folder = root / (progress ? L"migrate-progress" : L"migrate-empty");
        write_previous(folder / L"old");
        fs::create_directories(folder / L"new");
        if (progress) write(folder / L"new" / L"saves" / save_name, pattern(save_bytes, 11));
        const auto before = snapshot(folder / L"new");
        check(migrate_profile(folder / L"old", folder / L"new").outcome == Migration::DestinationExists,
            "existing destination wins");
        check(snapshot(folder / L"new") == before && fs::exists(folder / L"new" / L"saves") == progress &&
            stages(folder / L"new").empty(), "existing destination unchanged");
    }
    {
        const auto folder = root / L"migrate-full";
        const auto old = folder / L"old \u00e9", target = folder / L"new \u65e5";
        write_previous(old);
        const auto before = snapshot(old);
        check(migrate_profile(old, target).outcome == Migration::Migrated, "previous profile migrated");
        check(snapshot(old) == before, "previous profile byte- and time-identical");
        check(stages(target).empty(), "successful migration leaves no staging folder");
        std::size_t files = 0;
        for (const auto& entry : fs::recursive_directory_iterator(target))
            if (!entry.is_directory()) ++files;
        check(files == 6, "only the allowlist and a marker are copied");
        for (const auto& name : {fs::path(L"saves") / save_name, fs::path(L"saves") / (std::wstring(save_name) + L".bak"),
                 fs::path(L"tooie_imgui.json"), fs::path(L"bt.n64.us.1.0.z64"), fs::path(import_record_name)})
            check(read(target / name) == read(old / name), "allowlisted file byte-identical");
        check(read(target / marker_name) == marker_text, "migrated profile keeps the existing marker");
        {
            const auto profile = Profile::acquire(target);
            int asked = 0;
            check(!offer_import(source, profile.root(), answer(true, asked)) && asked == 0,
                "migrated progress is never offered over");
        }
        check(migrate_profile(old, target).outcome == Migration::DestinationExists, "migration happens once");
    }
    {
        // The Alpha 3 offer still follows the copied profile's own state.
        const auto folder = root / L"migrate-unplayed";
        write(folder / L"old" / marker_name, marker_text);
        check(migrate_profile(folder / L"old", folder / L"new").outcome == Migration::Migrated, "unplayed profile migrated");
        const auto profile = Profile::acquire(folder / L"new");
        int asked = 0;
        check(!offer_import(source, profile.root(), answer(false, asked)) && asked == 1,
            "Alpha 3 offer still made once after migration");
    }
    {
        const auto folder = root / L"migrate-locked";
        write_previous(folder / L"old");
        const auto before = snapshot(folder / L"old");
        {
            const auto held = Profile::acquire(folder / L"old");
            const auto text = failure_text([&] { migrate_profile(folder / L"old", folder / L"new"); });
            check(text.find("unchanged") != std::string::npos, "previous profile in use refused, original reported intact");
            check(!fs::exists(folder / L"new") && stages(folder / L"new").empty(), "in-use refusal creates nothing");
        }
        check(snapshot(folder / L"old") == before, "in-use source unchanged");
    }
    for (const bool marked : {false, true}) {
        const auto folder = root / (marked ? L"migrate-foreign" : L"migrate-unmarked");
        write_previous(folder / L"old");
        if (marked) write(folder / L"old" / marker_name, "TooieBaseline isolated profile v1\n");
        else fs::remove(folder / L"old" / marker_name);
        const auto before = snapshot(folder / L"old");
        check(migrate_profile(folder / L"old", folder / L"new").outcome == Migration::SourceUnrecognized,
            "unrecognized previous profile skipped");
        check(!fs::exists(folder / L"new") && stages(folder / L"new").empty() && snapshot(folder / L"old") == before,
            "unrecognized previous profile neither copied nor changed");
    }
    {
        const auto folder = root / L"migrate-folder-file";
        write_previous(folder / L"old");
        fs::remove(folder / L"old" / L"tooie_imgui.json");
        fs::create_directories(folder / L"old" / L"tooie_imgui.json");
        check(refused([&] { migrate_profile(folder / L"old", folder / L"new"); }) && !fs::exists(folder / L"new"),
            "folder in an allowlisted position refused");
    }
    {
        // A failed attempt keeps its staging folder; the retry makes a new one and leaves it alone.
        const auto folder = root / L"migrate-retry";
        const auto old = folder / L"old", target = folder / L"new";
        write_previous(old);
        const auto save = old / L"saves" / save_name;
        HANDLE writer = CreateFileW((old / L"tooie_imgui.json").c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        check(writer != INVALID_HANDLE_VALUE, "fixture writer opened");
        const auto text = failure_text([&] { migrate_profile(old, target); });
        if (writer != INVALID_HANDLE_VALUE) CloseHandle(writer);
        const auto kept = stages(target);
        check(!fs::exists(target) && kept.size() == 1, "settings open for writing refused before publishing");
        if (kept.size() == 1) {
            check(text.find(utf8(fs::absolute(kept[0]).lexically_normal())) != std::string::npos && text.find("unchanged") != std::string::npos,
                "failure names the kept staging folder and the intact original");
            const auto staged = snapshot(kept[0]);
            check(staged.size() == 2 && read(kept[0] / L"saves" / save_name) == read(save),
                "failed stage retains the copied save and backup");
            check(!fs::exists(kept[0] / marker_name), "unfinished stage is never a profile");
            check(migrate_profile(old, target).outcome == Migration::Migrated, "retry migrates");
            check(stages(target) == kept && snapshot(kept[0]) == staged, "retry left the earlier staging folder untouched");
            check(read(target / L"saves" / save_name) == read(save), "retry copied the save");
        }
    }
    {
        const auto folder = root / L"migrate-links";
        const auto real = folder / L"real";
        write_previous(real);
        const DWORD flags = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
        if (CreateSymbolicLinkW((folder / L"linked-root").c_str(), real.c_str(), SYMBOLIC_LINK_FLAG_DIRECTORY | flags)) {
            check(migrate_profile(folder / L"linked-root", folder / L"new-root").outcome == Migration::SourceUnrecognized &&
                !fs::exists(folder / L"new-root"), "linked previous profile not copied");
            write(folder / L"linked-saves" / marker_name, marker_text);
            check(CreateSymbolicLinkW((folder / L"linked-saves" / L"saves").c_str(), (real / L"saves").c_str(),
                SYMBOLIC_LINK_FLAG_DIRECTORY | flags), "fixture saves link created");
            check(refused([&] { migrate_profile(folder / L"linked-saves", folder / L"new-saves"); }) &&
                !fs::exists(folder / L"new-saves"), "linked saves folder refused");
            write(folder / L"linked-file" / marker_name, marker_text);
            check(CreateSymbolicLinkW((folder / L"linked-file" / L"tooie_imgui.json").c_str(),
                (real / L"tooie_imgui.json").c_str(), flags), "fixture settings link created");
            check(refused([&] { migrate_profile(folder / L"linked-file", folder / L"new-file"); }) &&
                !fs::exists(folder / L"new-file"), "linked settings file refused");
        } else {
            std::fprintf(stderr, "note: symbolic links unavailable; migration link refusal not exercised\n");
        }
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
