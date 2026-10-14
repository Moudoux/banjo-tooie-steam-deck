// Fixture-only composition of the selected ordinary PI, workers and lifecycle.
#ifndef TOOIE_SAVE_RUNTIME_SOURCE
#error Define TOOIE_SAVE_RUNTIME_SOURCE to the pinned ordinary PI source
#endif
#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT
#error This fixture must use the ordinary runtime
#endif
#include TOOIE_SAVE_RUNTIME_SOURCE
#include "librecomp/overlays.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <semaphore>
#include <stdexcept>
#include <thread>

namespace {
constexpr int worker_count = 40;
constexpr uint32_t object_base = 0x80001000, entry = 0x80400000;
constexpr uint32_t save_address = 0x80030000;
constexpr auto fixture_game = u8"continuous.wait-save.test";
std::atomic_int entries{0};
std::atomic_bool timed_out{false}, quiescence_entered{false}, saver_returned{false};
std::atomic_bool wait_returned{false}, quiescence_stop{true};
std::atomic_size_t quiescence_producers{0};
// At most three releases: first timeout, first quiescence entry, saver return.
std::counting_semaphore<4> observations{0};
static_assert(std::atomic_bool::is_always_lock_free &&
              std::atomic_size_t::is_always_lock_free);

void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void await(const std::atomic_bool& event, const char* message) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!event.load(std::memory_order_acquire)) {
        require(std::chrono::steady_clock::now() < deadline, message);
        observations.try_acquire_until(deadline);
    }
}
void guest_entry(uint8_t* rdram, recomp_context*) {
    entries.fetch_add(1);
    if (osGetThreadId(rdram, 0) == worker_count - 1) {
        ultramodern::wait_for_external_message(rdram);
        wait_returned.store(true, std::memory_order_release);
    }
}
}

// Called only inside the original false-dequeue loop, before its unchanged poll.
extern "C" void tooie_wait_save_fixture_timeout(int worker) noexcept {
    if (worker == worker_count - 1 && !timed_out.exchange(true))
        observations.release();
}
// Only the real saver invokes the observed quiescence function in this fixture.
extern "C" void tooie_wait_save_fixture_quiescence(bool stop, std::size_t producers) noexcept {
    if (!quiescence_entered.load(std::memory_order_relaxed)) {
        quiescence_stop.store(stop, std::memory_order_relaxed);
        quiescence_producers.store(producers, std::memory_order_relaxed);
        quiescence_entered.store(true, std::memory_order_release);
        observations.release();
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: continuous_wait_save_composition_test NEW_RUN_DIRECTORY\n";
        return 2;
    }
    // This storage must outlive every native context, including failure cleanup.
    std::vector<uint8_t> memory(8 * 1024 * 1024);
    auto* rdram = memory.data();
    std::array<uint8_t, 2048> expected{};
    std::filesystem::path output;
    bool lifecycle_enabled = false, cleaner_started = false, game_started = false;
    bool saver_started = false;
    uint64_t before_generation = 0, written_generation = 0;
    std::exception_ptr failure;
    try {
        const auto run_root = std::filesystem::absolute(argv[1]);
        require(!std::filesystem::exists(run_root), "Fixture run directory must be new");
        std::filesystem::create_directories(run_root);
        output = run_root / "save.bin";
        recomp::register_config_path(run_root / "config");
        require(recomp::register_game({.rom_hash=0, .internal_name="TEST",
            .display_name="Ordinary wait and late save", .game_id=fixture_game}),
            "Fixture game registration failed");
        recomp::start_game(fixture_game, "");
        game_started = true;
        recomp::overlays::add_loaded_function(int32_t(entry), guest_entry);
        save_context.save_file_path = output;
        save_context.save_buffer.resize(expected.size());
        for (size_t i = 0; i < expected.size(); ++i) {
            expected[i] = uint8_t(i * 37 + 11);
            MEM_B(i, int32_t(save_address)) = expected[i];
        }
        require(tooie::save_persistence::generation() == 0, "Unexpected prior save write");
        require(!exited.load(), "Fixture runtime already exited");
        tooie::lifecycle::enable();
        lifecycle_enabled = true;
        ultramodern::init_thread_cleanup();
        cleaner_started = true;
        for (int i = 0; i < worker_count; ++i)
            osCreateThread(rdram, int32_t(object_base + i * 0x80), i, int32_t(entry),
                           0, int32_t(0x80010000 + i * 0x100), 1);
        for (int i = 1; i < worker_count; ++i)
            ultramodern::schedule_running_thread(rdram, int32_t(object_base + i * 0x80));
        osStartThread(rdram, int32_t(object_base));
        await(timed_out, "Worker 39 did not reach a false timed dequeue");
        require(entries.load() == worker_count, "Not all real workers entered");
        require(!wait_returned.load(), "Empty external wait returned normally");
        require(!tooie::lifecycle::stopping(), "Stop preceded the late-save boundary");

        exited.store(true);
        save_context.saving_thread = std::thread([rdram] {
            saving_thread_func(rdram);
            saver_returned.store(true, std::memory_order_release);
            observations.release();
        });
        saver_started = true;
        await(quiescence_entered, "Saver did not enter final quiescence");
        require(!quiescence_stop.load() && quiescence_producers.load() >= 1,
                "Saver did not enter before stop with an owned real producer");
        before_generation = tooie::save_persistence::generation();
        require(before_generation == 0, "A write preceded quiescence entry");
        save_write(rdram, int32_t(save_address), 0, uint32_t(expected.size()));
        written_generation = tooie::save_persistence::generation();
        require(written_generation > before_generation, "Late write did not advance generation");
        require(!tooie::lifecycle::stopping() && tooie::lifecycle::counts().producers >= 1,
                "Real producer ownership ended before the explicit stop");
        require(!std::filesystem::exists(output) && !saver_returned.load() &&
                !tooie::save_persistence::complete(written_generation) &&
                !tooie::save_persistence::unsuccessful(written_generation),
                "Late write finalized or saver returned before stop/quiescence");
        std::cout << "observed false_dequeue_worker=39 quiescence_stop=0 producers="
                  << quiescence_producers.load() << " late_generation=" << written_generation << '\n';
    } catch (...) {
        failure = std::current_exception();
    }

    // Never let an assertion release memory still owned by real native workers.
    if (lifecycle_enabled) {
        tooie::lifecycle::request_stop();
        if (!tooie::lifecycle::wait_for_producers(std::chrono::seconds(3))) {
            std::cerr << "FAIL: producers did not quiesce; retaining memory until process exit\n";
            std::_Exit(70);
        }
    }
    exited.store(true);
    if (cleaner_started) ultramodern::join_thread_cleaner_thread();
    if (saver_started) ultramodern::join_saving_thread();
    const bool game_retained = game_started && recomp::current_game_id() == fixture_game;
    if (game_started) ultramodern::quit();
    try {
        if (failure) std::rethrow_exception(failure);
        const auto counts = tooie::lifecycle::counts();
        require(counts.created == worker_count && counts.enqueued == worker_count &&
                counts.deleted == worker_count && counts.producers == 0,
                "Real worker cleanup counts disagree");
        require(!tooie::lifecycle::failure(), "Lifecycle or saver reported a failure");
        require(timed_out.load() && quiescence_entered.load() && saver_returned.load() &&
                !wait_returned.load() && game_retained,
                "Causal wait/save/join or retained-game requirement failed");
        std::ifstream input(output, std::ios::binary);
        require(input.is_open(), "Finalized save file is absent");
        const std::vector<uint8_t> actual{std::istreambuf_iterator<char>(input), {}};
        require(actual == std::vector<uint8_t>(expected.begin(), expected.end()),
                "Finalized bytes differ from independent expected guest bytes");
        require(tooie::save_persistence::complete(written_generation) &&
                !tooie::save_persistence::unsuccessful(written_generation),
                "Captured late-write generation lacks a successful persistence receipt");
        std::cout << "PASS ordinary wait/late-save: created=" << counts.created
                  << " enqueued=" << counts.enqueued << " deleted=" << counts.deleted
                  << " producers=" << counts.producers << " bytes=" << actual.size()
                  << " generation=" << written_generation << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after cleanup: " << error.what() << '\n';
        return 1;
    }
}
