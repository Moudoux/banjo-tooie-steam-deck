// Actual selected startup-thread lambda and event join, extracted from pinned children.
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <thread>

static void require(bool value) { if (!value) std::_Exit(90); }
enum class Inject { none, renderer, boot, terminated, quit_race, quit_before };
static Inject inject = Inject::none;
static int names = 0, preinits = 0, waits = 0, failures = 0, task_exits = 0;
static std::exception_ptr retained;
// Runtime globals the selected lambda reads (recomp.cpp:474, 551).
std::atomic_bool exited{false};
enum class Status { None, Running, Quit };
static std::atomic<Status> status{Status::None};
static bool cas_missed = false;
// Same ordering as the selected fence quit: exited first, then the None->Quit CAS.
static void host_quit() {
    exited.store(true);
    Status desired = Status::None;
    cas_missed = !status.compare_exchange_strong(desired, Status::Quit);
}
static bool message(std::exception_ptr error, const char* text) {
    if (!error) return false;
    try { std::rethrow_exception(error); }
    catch (const std::runtime_error& value) { return std::strcmp(value.what(), text) == 0; }
    catch (...) { return false; }
}
[[maybe_unused]] static void parent_terminate() {
    const auto current = std::current_exception();
    if (inject != Inject::renderer || names != 1 || preinits != 1 || waits != 0 || failures != 0 ||
        (current && !message(current, "Failed to initialize the renderer"))) std::_Exit(92);
    std::fputs("STARTUP_PARENT_TERMINATE_VERIFIED\n", stderr);
    std::fflush(stderr);
    std::_Exit(73);
}

#define debug_printf(...) ((void)0)
struct recomp_context {};
struct OSTask;
namespace ultramodern {
struct thread_terminated {};
namespace renderer { struct WindowHandle { int value = 0; }; }
void set_native_thread_name(const char* name) {
    require(std::strcmp(name, "Game Start Thread") == 0);
    ++names;
#ifdef STARTUP_RECOMP_PARENT
    std::set_terminate(parent_terminate); // UCRT terminate handlers belong to the installing thread.
#endif
}
// Same exception init_events raises after its renderer message (events.cpp:614).
void preinit(uint8_t*, renderer::WindowHandle handle) {
    require(names == 1 && handle.value == 7);
    ++preinits;
    if (inject == Inject::renderer) throw std::runtime_error("Failed to initialize the renderer");
    if (inject == Inject::terminated) throw thread_terminated{};
}
void join_event_threads();
}
bool wait_for_game_started(uint8_t*, recomp_context*) {
    require(preinits == 1);
    ++waits;
    if (inject == Inject::boot) throw std::runtime_error("startup boot injected");
    if (inject == Inject::quit_race) {
        // recomp.cpp:704-747 Running branch with a mod-load error: message_box retains it and
        // the SDL owner's quit lands before game_status.store(None). A second entry here
        // stands for the unfixed loop re-entering game_status.wait(None) with no further wake.
        if (waits != 1) std::_Exit(94);
        status.store(Status::Running);
        host_quit();
        require(cas_missed);
        status.store(Status::None);
        return false;
    }
    return true;
}
namespace tooie::lifecycle { void fail(std::exception_ptr error) noexcept { ++failures; retained = error; } }

static void run_game_thread() {
    const ultramodern::renderer::WindowHandle window_handle{7};
    uint8_t* rdram = nullptr;
#ifdef STARTUP_RECOMP_PARENT
#include "startup_game_thread_parent.inc"
#else
#include "startup_game_thread_selected.inc"
#endif
    game_thread.join();
}

struct TaskQueue {
    std::mutex mutex;
    std::condition_variable changed;
    std::deque<OSTask*> items;
    int enqueues = 0;
    void enqueue(OSTask* task) {
        { std::lock_guard lock(mutex); items.push_back(task); ++enqueues; }
        changed.notify_all();
    }
    OSTask* wait_dequeue() {
        std::unique_lock lock(mutex);
        changed.wait(lock, [&] { return !items.empty(); });
        OSTask* value = items.front();
        items.pop_front();
        return value;
    }
};
struct { struct { std::thread gfx_thread, task_thread; } sp; struct { std::thread thread; } vi; TaskQueue sp_task_queue; } events_context;
#ifdef STARTUP_EVENTS_PARENT
#include "startup_join_events_parent.inc"
#else
#include "startup_join_events_selected.inc"
#endif

int main(int argc, char** argv) {
    require(argc == 2);
    const std::string_view mode = argv[1];
    if (mode.substr(0, 7) == "recomp-") {
        inject = mode == "recomp-renderer-throw" ? Inject::renderer : mode == "recomp-boot-throw" ? Inject::boot :
            mode == "recomp-terminated" ? Inject::terminated : mode == "recomp-mod-error-quit-race" ? Inject::quit_race :
            mode == "recomp-quit-before-start" ? Inject::quit_before : Inject::none;
        require(inject != Inject::none || mode == "recomp-normal");
        if (inject == Inject::quit_before) host_quit(); // Window close before the first wait.
        run_game_thread();
#ifdef STARTUP_RECOMP_PARENT
        std::_Exit(93); // The unpatched lambda must not return from an escaping renderer exception.
#endif
        const bool fails = inject == Inject::renderer || inject == Inject::boot;
        require(names == 1 && preinits == 1);
        require(waits == ((inject == Inject::none || inject == Inject::boot || inject == Inject::quit_race) ? 1 : 0));
        require(exited.load() == (inject == Inject::quit_race || inject == Inject::quit_before));
        require(failures == (fails ? 1 : 0));
        require(fails ? message(retained, inject == Inject::renderer ? "Failed to initialize the renderer" : "startup boot injected") : !retained);
        return 0;
    }
    require(mode == "events-unstarted-vi" || mode == "events-started-vi");
    // gfx_thread_func returns early when the renderer is invalid; the task worker waits for nullptr.
    events_context.sp.gfx_thread = std::thread{[] {}};
    events_context.sp.task_thread = std::thread{[] { while (events_context.sp_task_queue.wait_dequeue() != nullptr) {} ++task_exits; }};
    if (mode == "events-started-vi") events_context.vi.thread = std::thread{[] {}};
#ifdef STARTUP_EVENTS_PARENT
    try {
        ultramodern::join_event_threads();
    } catch (const std::system_error&) {
        require(!events_context.sp.gfx_thread.joinable() && events_context.sp_task_queue.enqueues == 0);
        events_context.sp_task_queue.enqueue(nullptr);
        events_context.sp.task_thread.join();
        std::fputs("EVENTS_PARENT_JOIN_REFUSED\n", stderr);
        std::fflush(stderr);
        std::_Exit(74);
    }
    std::_Exit(93);
#else
    ultramodern::join_event_threads();
    require(!events_context.sp.gfx_thread.joinable() && !events_context.vi.thread.joinable() &&
            !events_context.sp.task_thread.joinable());
    require(events_context.sp_task_queue.enqueues == 1 && task_exits == 1);
    return 0;
#endif
}
