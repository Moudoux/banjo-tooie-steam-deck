#include "devices.hpp"
#include "input.hpp"
#include "librecomp/game.hpp"
#include "librecomp/overlays.hpp"
#include "funcs.h"
#include "recomp_overlays.inl"
#include "adapters.h"
#ifdef BASELINE_TERMINAL_FENCE
#include "runtime_lifecycle.hpp"
#endif
#ifdef BASELINE_SAVE_COMPOSITION
#include "save_persistence.hpp"
#endif
#ifdef BASELINE_PLAYER
#include "player_startup.hpp"
#endif
#include <atomic>
#ifdef BASELINE_VI_CONTROLLER_CYCLE
#include "tooie_audio_rsp.hpp"
#include "si_adapter.hpp"
#endif
#include <charconv>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <windows.h>

namespace {
std::atomic<bool> runtime_error_reported = false;
constexpr char profile_marker[] = "TooieBaseline isolated profile v1\n";
class ProfileLock {
    HANDLE handle = INVALID_HANDLE_VALUE;
public:
    void acquire(const std::filesystem::path& root) {
        if (GetFileAttributesW(root.c_str()) & FILE_ATTRIBUTE_REPARSE_POINT)
            throw std::runtime_error("Baseline profile must not be a reparse point");
        handle = CreateFileW((root / ".baseline-profile").c_str(), GENERIC_READ, 0, nullptr,
            OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (handle == INVALID_HANDLE_VALUE) throw std::runtime_error("Baseline marker absent or profile already in use");
        BY_HANDLE_FILE_INFORMATION info{};
        char marker[sizeof(profile_marker)]{};
        DWORD bytes = 0;
        if (!GetFileInformationByHandle(handle, &info) || (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
            !ReadFile(handle, marker, sizeof(marker), &bytes, nullptr) || bytes != sizeof(profile_marker) - 1 ||
            std::string_view(marker, bytes) != std::string_view(profile_marker))
            throw std::runtime_error("Baseline ownership marker does not match");
    }
    ~ProfileLock() { if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
};
struct Options {
    std::filesystem::path rom, profile, input_script;
    std::string condition;
    ultramodern::renderer::GraphicsApi api = ultramodern::renderer::GraphicsApi::Vulkan;
    baseline::RenderMode render_mode = baseline::RenderMode::Original;
    unsigned seconds = 10;
    bool run = false;
    bool manual = false, reuse_profile = false;
};

// Require explicit source, condition and a fresh or marked isolated destination.
Options parse(int argc, char** argv) {
    Options options;
    bool seconds_explicit = false, render_mode_explicit = false;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg == "--run") { options.run = true; continue; }
        if (arg == "--manual") { options.manual = true; continue; }
        if (arg == "--reuse-profile") { options.reuse_profile = true; continue; }
        if (++i == argc) throw std::runtime_error("Missing option value");
        std::string_view value(argv[i]);
        if (arg == "--rom") options.rom = argv[i];
        else if (arg == "--input-script") options.input_script = argv[i];
        else if (arg == "--render-mode") {
            if (render_mode_explicit) throw std::runtime_error("Duplicate --render-mode");
            render_mode_explicit = true;
#if BASELINE_RENDERER_COMPARISON
            throw std::runtime_error("Explicit --render-mode is unavailable in the comparison build");
#endif
            if (value == "original") options.render_mode = baseline::RenderMode::Original;
            else if (value == "display") options.render_mode = baseline::RenderMode::Display;
            else throw std::runtime_error("--render-mode must be original or display");
        }
        else if (arg == "--profile") options.profile = argv[i];
        else if (arg == "--condition") options.condition = value;
        else if (arg == "--api" && value == "vulkan") options.api = ultramodern::renderer::GraphicsApi::Vulkan;
        else if (arg == "--api" && value == "d3d12") options.api = ultramodern::renderer::GraphicsApi::D3D12;
        else if (arg == "--seconds") {
            seconds_explicit = true;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), options.seconds);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || options.seconds < 1 || options.seconds > 900)
                throw std::runtime_error("--seconds must be 1..900");
        } else throw std::runtime_error("Unknown option/value");
    }
    if (options.rom.empty() || options.profile.empty() || options.condition.empty())
        throw std::runtime_error("Usage: TooieBaseline --condition LABEL --rom ORIGINAL --profile ROOT --api vulkan|d3d12 [--run] [--seconds 1..900 | --manual] [--reuse-profile] [--input-script PATH] [--render-mode original|display]");
    if (options.manual && seconds_explicit) throw std::runtime_error("--manual cannot be combined with --seconds");
    baseline::input::require_script_bound(!options.input_script.empty(), seconds_explicit, options.manual);
    options.profile = std::filesystem::absolute(options.profile).lexically_normal();
    if (options.reuse_profile) {
        if (!std::filesystem::is_directory(options.profile)) throw std::runtime_error("Reusable baseline profile must exist");
    } else if (std::filesystem::exists(options.profile)) throw std::runtime_error("Disposable profile must be a new path");
    if (std::filesystem::file_size(options.rom) != 32u * 1024 * 1024)
        throw std::runtime_error("Original input must be a 32 MiB ROM");
    return options;
}

// Strict S0 has no Tooie RSP task provider; the stock worker owns failure/completion.
RspUcodeFunc* reject_rsp(const OSTask* task) {
    std::fprintf(stderr, "baseline unsupported RSP task type=%u ucode=%08x; no completion supplied\n",
        task ? task->t.type : 0, task ? uint32_t(task->t.ucode) : 0);
    std::fflush(stderr);
    return nullptr;
}
void console_error(const char* message) {
#ifdef BASELINE_PLAYER
    // Stock runtime message boxes arrive here; a player has no console.
    const bool first = !runtime_error_reported.exchange(true, std::memory_order_relaxed);
#endif
    runtime_error_reported.store(true, std::memory_order_relaxed);
    std::fprintf(stderr, "baseline runtime error: %s\n", message);
    std::fflush(stderr);
#ifdef BASELINE_PLAYER
    if (first) baseline::player::show_error(std::string("The game hit a problem and may close.\n\n") + message);
#endif
}
#ifdef BASELINE_PLAYER
Options player_options(const baseline::player::Arguments& arguments, const std::filesystem::path& root) {
    Options options;
    options.profile = root;
    options.condition = "player";
    options.run = true;
    options.reuse_profile = true; // Created, marked and locked by the player startup.
    options.manual = arguments.seconds == 0;
    if (!options.manual) options.seconds = arguments.seconds;
    options.input_script = arguments.input_script;
    // Same rule as the engineering command line: a script needs explicit seconds, never manual.
    baseline::input::require_script_bound(!options.input_script.empty(), !options.manual, options.manual);
    return options;
}
std::string rom_problem(const std::filesystem::path& rom, std::u8string& game_id) {
    std::error_code error;
    const auto size = std::filesystem::file_size(rom, error);
    if (error) return "The ROM file could not be read.";
    // Checked first: the stock selection reads the whole file into memory.
    if (size != 32u * 1024 * 1024) return "This file is not the 32 MB Banjo-Tooie (USA) ROM.";
    switch (recomp::select_rom(rom, game_id)) {
    case recomp::RomValidationError::Good: return {};
    case recomp::RomValidationError::FailedToOpen: return "The ROM file could not be read.";
    case recomp::RomValidationError::NotARom: return "This file is not an N64 ROM.";
    case recomp::RomValidationError::IncorrectVersion: return "This is a different version of Banjo-Tooie; Encore needs the USA ROM.";
    case recomp::RomValidationError::IncorrectRom: return "This is not a Banjo-Tooie ROM.";
    default: return "This ROM is not supported.";
    }
}
// False only when the player cancels; the marked profile stays reusable.
bool select_player_rom(const baseline::player::Arguments& arguments, std::u8string& game_id) {
    recomp::check_all_stored_roms();
    if (arguments.rom.empty() && recomp::is_rom_valid(game_id)) return true;
    for (auto candidate = arguments.rom;; candidate.clear()) {
        if (candidate.empty()) {
            // Unattended QA never opens the picker.
            if (arguments.qa) throw baseline::player::Failure("QA run has no valid stored ROM; pass --rom FILE.");
            const auto picked = baseline::player::pick_rom();
            if (!picked) return false;
            candidate = *picked;
        }
        auto problem = rom_problem(candidate, game_id);
        if (problem.empty()) {
            recomp::check_all_stored_roms();
            if (recomp::is_rom_valid(game_id)) {
                std::fprintf(stderr, "player ROM accepted: %s\n", baseline::player::utf8(candidate).c_str());
                return true;
            }
            problem = "The ROM was accepted but could not be copied into the profile folder. "
                "Check free disk space and folder permissions.";
        }
        std::fprintf(stderr, "player ROM rejected: %s\n", problem.c_str());
        if (!arguments.rom.empty()) throw baseline::player::Failure(problem);
        if (!baseline::player::ask_retry(problem + "\n\nChoose another file?")) return false;
    }
}
#endif
}

int main(int argc, char** argv) {
    // Retain stock fatal diagnostics when stdout is redirected to a run receipt.
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    try {
#ifdef BASELINE_PLAYER
        // Zero-argument play or an explicit QA folder; the profile stays owned until exit.
        const auto player_arguments = baseline::player::parse_arguments(baseline::player::command_line());
        // Unattended QA reports every later failure in the log and exit code, never a dialog.
        if (player_arguments.qa) baseline::player::set_log_only_errors();
        const auto player_profile = baseline::player::Profile::acquire(player_arguments.profile.empty() ?
            baseline::player::default_profile_root() : player_arguments.profile);
        baseline::player::open_log(player_profile.root());
        const auto options = player_options(player_arguments, player_profile.root());
#else
        const auto options = parse(argc, argv);
#endif
        baseline::load_input_script(options.input_script);
        std::fprintf(stderr, "baseline condition=%s api=%d run=%d bounded_seconds=%u manual=%d reuse_profile=%d render_mode=%s\n",
            options.condition.c_str(), int(options.api), int(options.run), options.manual ? 0 : options.seconds,
            int(options.manual), int(options.reuse_profile),
            options.render_mode == baseline::RenderMode::Display ? "display" : "original");
#ifndef BASELINE_PLAYER
        ProfileLock profile_lock;
        if (options.reuse_profile) profile_lock.acquire(options.profile);
        if (!options.reuse_profile && !std::filesystem::create_directories(options.profile))
            throw std::runtime_error("Disposable profile appeared before creation");
#endif
        recomp::register_config_path(options.profile);
        // Game identity and the unmodified raw entry/registry are supported startup inputs.
        const recomp::GameEntry game{
            .rom_hash = 0x00F70DE5F2D70EA2,
            .internal_name = "BANJO TOOIE",
#ifdef BASELINE_PLAYER
            .display_name = "Banjo-Tooie: Encore",
#else
            .display_name = "Banjo-Tooie baseline",
#endif
            .game_id = u8"bt.n64.us.1.0", .mod_game_id = "",
            .save_type = recomp::SaveType::Eep16k, .thumbnail_bytes = {}, .is_enabled = true,
            .decompression_routine = nullptr, .has_compressed_code = true,
            .entrypoint_address = gpr(int32_t(0x80000400)), .entrypoint = recomp_entrypoint,
#ifdef BASELINE_VI_CONTROLLER_CYCLE
            .thread_create_callback = baseline::seed_thread_status,
#else
            .thread_create_callback = nullptr,
#endif
#ifdef BASELINE_CIC_OBSERVER
            .on_init_callback = baseline::scatter_then_observe_boot,
#elif defined(BASELINE_BOOT_OBSERVER)
            .on_init_callback = baseline::observe_boot_before,
#else
            .on_init_callback = nullptr,
#endif
        };
        if (!recomp::register_game(game)) throw std::runtime_error("Game registration failed");
#ifdef BASELINE_BOOT_PATCH
        baseline::register_boot_patch(section_table, ARRLEN(section_table));
#endif
#ifdef BASELINE_IMPORT_recomp_syscall_handler
        baseline::register_overlay_metadata(section_table, overlay_sections_by_index, num_sections);
#endif
        recomp::overlays::register_overlays(
            {section_table, ARRLEN(section_table), num_sections},
            {overlay_sections_by_index, ARRLEN(overlay_sections_by_index)});
        auto game_id = game.game_id;
#ifdef BASELINE_PLAYER
        if (!select_player_rom(player_arguments, game_id)) {
            std::fprintf(stderr, "player ROM selection cancelled; nothing started\n");
            return 0;
        }
#else
        const auto validation = recomp::select_rom(options.rom, game_id);
        std::fprintf(stderr, "baseline ROM selection=%d stored=%s\n", int(validation),
            (options.profile / game.stored_filename()).string().c_str());
        if (validation != recomp::RomValidationError::Good || game_id != game.game_id)
            throw std::runtime_error("ROM selection rejected");
#endif
        recomp::check_all_stored_roms();
        if (!recomp::is_rom_valid(game_id)) throw std::runtime_error("Stored ROM did not validate");
#ifdef BASELINE_PLAYER
        // QA --import-save, or the one-time offer of an older default save.
        baseline::player::prepare_saves(player_arguments, options.profile);
#else
        if (!options.reuse_profile) {
            std::ofstream marker(options.profile / ".baseline-profile", std::ios::binary);
            marker << profile_marker;
            marker.close();
            if (!marker) throw std::runtime_error("Could not create baseline ownership marker");
            profile_lock.acquire(options.profile);
        }
#endif
        if (!options.run) {
            std::fprintf(stderr, "baseline admission complete; no runtime started\n");
            return 0;
        }
        const auto handle = baseline::open_devices(options.profile, options.api, options.manual ? 0 : options.seconds, options.render_mode);
        // A stuck guest/join ends as process failure; this is not a graceful-quit claim.
        std::mutex deadline_mutex;
        std::condition_variable deadline_cv;
        bool returned = false;
        std::thread watchdog([&] {
            std::unique_lock lock(deadline_mutex);
            if (options.manual) {
                while (!returned && !baseline::quit_was_requested())
                    deadline_cv.wait_for(lock, std::chrono::milliseconds(20));
            }
            const auto grace = std::chrono::seconds(options.manual ? 5 : options.seconds + 5);
            if (!deadline_cv.wait_for(lock, grace, [&] { return returned; })) {
                std::fprintf(stderr, "baseline hard deadline: standard start/quit did not return; process failure, saves/joins unaccepted\n");
                std::fflush(stderr);
                std::quick_exit(124);
            }
        });
        try {
            recomp::rsp::callbacks_t rsp_callbacks{reject_rsp};
#ifdef BASELINE_VI_CONTROLLER_CYCLE
            rsp_callbacks = tooie::audio_rsp::callbacks();
            tooie::audio_rsp::configure([](const char* event, const nlohmann::json& data) {
                std::fprintf(stderr, "baseline %s %s\n", event, data.dump().c_str()); std::fflush(stderr);
            }, [](const std::string& reason) { console_error(reason.c_str()); }, false);
            tooie::si::reset();
#endif
            recomp::start(recomp::Version(0, 1, 0, "-baseline"), handle,
                rsp_callbacks, baseline::renderer_callbacks(), baseline::audio_callbacks(),
                baseline::input_callbacks(), {nullptr, nullptr, baseline::update_gfx},
                {baseline::vi_complete, nullptr}, {console_error}, {});
        } catch (const std::exception& error) {
            std::fprintf(stderr, "baseline standard start threw: %s; process failure, ownership unaccepted\n", error.what());
            std::fflush(stderr);
#ifdef BASELINE_PLAYER
            baseline::player::show_error(std::string("The game could not start.\n\n") + error.what());
#endif
            std::quick_exit(EXIT_FAILURE);
        }
#ifdef BASELINE_VI_CONTROLLER_CYCLE
#ifdef BASELINE_SAVE_COMPOSITION
        std::fprintf(stderr, "baseline saver receipt: returned=1 written=%llu persisted=%llu failed=%llu\n",
            static_cast<unsigned long long>(tooie::save_persistence::written.load(std::memory_order_acquire)),
            static_cast<unsigned long long>(tooie::save_persistence::persisted.load(std::memory_order_acquire)),
            static_cast<unsigned long long>(tooie::save_persistence::failed.load(std::memory_order_acquire)));
#endif
        std::fprintf(stderr, "baseline terminal: standard-start allocation retained; process ending via _Exit; all-worker joins and save completion unclaimed\n");
        baseline::emit_audio_summary();
#if TOOIE_BASELINE_HOST
#ifdef BASELINE_TERMINAL_FENCE
        // A failed fence can return before joins/Deinit; retain until _Exit.
        if(!tooie::lifecycle::failure()) baseline::close_devices();
#else
        baseline::close_devices();
#endif
#endif
        std::fflush(nullptr);
#ifdef BASELINE_TERMINAL_FENCE
        std::_Exit((runtime_error_reported.load(std::memory_order_relaxed) || tooie::lifecycle::failure()) ? 1 : 0);
#else
        std::_Exit(runtime_error_reported.load(std::memory_order_relaxed) ? 1 : 0);
#endif
#else
        {
            std::lock_guard lock(deadline_mutex);
            returned = true;
        }
        deadline_cv.notify_one();
        watchdog.join();
        baseline::close_devices();
        std::fprintf(stderr, "baseline standard start returned; joins are stock-runtime operations, gameplay/save acceptance unresolved\n");
        return runtime_error_reported.load(std::memory_order_relaxed) ? 1 : 0;
#endif
    } catch (const std::exception& error) {
        std::fprintf(stderr, "baseline admission failed: %s\n", error.what());
#ifdef BASELINE_PLAYER
        baseline::player::show_error(std::string("Banjo-Tooie: Encore could not start.\n\n") + error.what());
#endif
        baseline::close_devices();
        return 1;
    }
}
