// Include the selected materialized saver to exercise its actual loop. The
// linked runtime supplies config_path and exited; this test must not define
// duplicate process globals.
#ifndef TOOIE_SAVE_RUNTIME_SOURCE
#error Define TOOIE_SAVE_RUNTIME_SOURCE to the selected original/materialized pi.cpp
#endif
#include TOOIE_SAVE_RUNTIME_SOURCE
#include "runtime_lifecycle.hpp"
#include "runtime_save_root.hpp"
#include <atomic>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: MODE NEW_OUTPUT_PATH");
        const std::string mode = argv[1];
        const bool owned = mode == "owned-admitted" || mode == "owned-refused" || mode == "startup-held" || mode == "owned-failure";
        const bool path_mode = mode == "save-path-config" || mode == "save-path-profile";
        if (!owned && !path_mode && mode != "final-write" && mode != "wait-producer" && mode != "disabled" && mode != "empty" && mode != "write-failure")
            throw std::runtime_error("unknown mode");
        const std::filesystem::path output = argv[2];
        if (std::filesystem::exists(output)) throw std::runtime_error("test output must be new");
        if (mode != "write-failure") std::filesystem::create_directories(output.parent_path());
        else if (std::filesystem::exists(output.parent_path())) throw std::runtime_error("failure fixture parent must be absent");
        if (path_mode) {
            // Startup save-path derivation only: no saver thread, file or lifecycle use.
            // config: the selected player's runtime config_path/saves contract, ignoring a
            // registered override. profile: the legacy engine's registered save root.
            const auto root = output.parent_path();
            recomp::register_config_path(root / "config");
            tooie::register_save_root(root / "profile" / "saves");
            set_save_file_path(u8"", u8"bt.n64.us.1.0");
            const auto folder = mode == "save-path-config" ? root / "config" / "saves" : root / "profile" / "saves";
            if (ultramodern::get_save_file_path() != folder / "bt.n64.us.1.0.bin")
                throw std::runtime_error("save path derivation differs from the selected runtime contract");
            std::cout << "PASS continuous save drain " << mode << '\n';
            return 0;
        }
        if (mode != "disabled") tooie::lifecycle::enable();
        if (owned) tooie::lifecycle::own_final_drain();
        save_context.save_file_path = output;
        save_context.save_buffer.resize(2048);
        std::array<uint8_t, 2048> expected;
        for (size_t i = 0; i < expected.size(); ++i) expected[i] = uint8_t(i * 37 + 11);
        if (mode != "empty") save_write_ptr(expected.data(), 0, expected.size());
        // The normal path has no producer. The regression path represents an
        // already-admitted guest that is retiring while the saver observes the
        // exit flag; it must drain only after that ownership is gone.
        if (mode == "wait-producer") tooie::lifecycle::created(nullptr);
        else tooie::lifecycle::request_stop();
        if (mode == "owned-failure")
            tooie::lifecycle::fail(std::make_exception_ptr(std::runtime_error("injected worker failure")));
        if (owned && mode != "startup-held") tooie::lifecycle::admit_final_drain(mode != "owned-refused");
        exited.store(true);
        std::thread retire;
        if (mode == "wait-producer") retire=std::thread([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            tooie::lifecycle::request_stop();
            tooie::lifecycle::creation_failed(nullptr);
        });
        std::promise<void> saver_entered;
        auto entered = saver_entered.get_future();
        save_context.saving_thread = std::thread{[&] {
            saver_entered.set_value();
            saving_thread_func(nullptr); // Actual selected PI child; no replacement saver loop.
        }};
        if (mode == "startup-held") {
            entered.wait();
            // Observe a bounded pending-admission interval after native saver thread start. This is
            // deliberately separate from guest-producer quiescence, already true above.
            const bool premature = tooie::lifecycle::wait_for_final_drain(std::chrono::milliseconds{50}) ||
                std::filesystem::exists(output);
            tooie::lifecycle::admit_final_drain(true);
            if (premature) {
                ultramodern::join_saving_thread();
                throw std::runtime_error("startup-held admission allowed an early final write");
            }
        }
        ultramodern::join_saving_thread();
        if (retire.joinable()) retire.join();
        if (mode == "final-write" || mode == "wait-producer" || mode == "owned-admitted" || mode == "startup-held" || mode == "owned-failure") {
            std::ifstream input(output, std::ios::binary);
            std::vector<uint8_t> actual{std::istreambuf_iterator<char>(input), {}};
            if (actual != std::vector<uint8_t>(expected.begin(), expected.end()))
                throw std::runtime_error("final queued EEPROM bytes were not persisted before saver join");
            if (!tooie::save_persistence::complete(tooie::save_persistence::generation()))
                throw std::runtime_error("finalized save did not acknowledge its written generation");
            if (bool(tooie::lifecycle::failure()) != (mode == "owned-failure")) throw std::runtime_error("unexpected lifecycle failure state");
        } else if (mode == "owned-refused") {
            if (std::filesystem::exists(output) || tooie::save_persistence::complete(tooie::save_persistence::generation()))
                throw std::runtime_error("refused final-drain admission wrote or acknowledged a save");
            if (!tooie::lifecycle::failure()) throw std::runtime_error("refused drain was not reported");
        } else if (mode == "write-failure") {
            if (tooie::save_persistence::complete(tooie::save_persistence::generation()) ||
                !tooie::save_persistence::unsuccessful(tooie::save_persistence::generation()))
                throw std::runtime_error("failed persistence produced an incorrect receipt");
            if (!tooie::lifecycle::failure()) throw std::runtime_error("failed final save was reported as success");
        } else if (std::filesystem::exists(output)) throw std::runtime_error("unexpected empty/diagnostic save flush");
        std::cout << "PASS continuous save drain " << mode << '\n';
    } catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
