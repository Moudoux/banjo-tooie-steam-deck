#include <thread>
#include <exception>
#include <stdexcept>
#include <new>
#include <cstdint>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct OSTask { struct { uint32_t type = 2; } t; } task;
static int ready = 0, pops = 0, runs = 0, completions = 0, failures = 0;
static std::exception_ptr injected, retained;
static bool throws = false, bad_alloc = false, escaping = false;
static bool expected(std::exception_ptr error) {
    if (!error) return false;
    try { std::rethrow_exception(error); }
    catch (const std::bad_alloc&) { return bad_alloc; }
    catch (const std::runtime_error& value) { return !bad_alloc && std::strcmp(value.what(), "SP injected") == 0; }
    catch (...) { return false; }
}
static void require(bool value) { if (!value) std::_Exit(90); }
namespace moodycamel { struct LightweightSemaphore { void signal() { ++ready; } }; }
struct { struct { void wait_dequeue(OSTask*& out) { out = ++pops == 1 ? &task : nullptr; } } sp_task_queue; } events_context;
namespace ultramodern {
enum class ThreadPriority { Normal };
void set_native_thread_name(const char*) {}
void set_native_thread_priority(ThreadPriority) {}
namespace rsp {
bool run_task(uint8_t*, OSTask* value) {
    require(ready == 1 && pops == 1 && value == &task);
    ++runs;
    if (throws) { escaping = true; std::rethrow_exception(injected); }
    return true;
}
}
}
namespace tooie::lifecycle { void fail(std::exception_ptr error) noexcept { ++failures; retained = error; } }
void sp_complete() { ++completions; }
#define PASS_RDRAM rdram,
#define ULTRAMODERN_QUICK_EXIT() std::_Exit(91)
#ifdef SP_TASK_PARENT
#include "sp_task_parent.inc"
#else
#include "sp_task_selected.inc"
#endif

int main(int argc, char** argv) {
    require(argc == 2);
    const bool runtime_error = std::strcmp(argv[1], "runtime-error") == 0;
    bad_alloc = std::strcmp(argv[1], "bad-alloc") == 0;
    require(runtime_error || bad_alloc || std::strcmp(argv[1], "success") == 0);
    throws = runtime_error || bad_alloc;
    if (throws) {
        try { if (bad_alloc) throw std::bad_alloc{}; throw std::runtime_error("SP injected"); }
        catch (...) { injected = std::current_exception(); }
    }
#ifdef SP_TASK_PARENT
    require(runtime_error && expected(injected));
#endif
    moodycamel::LightweightSemaphore semaphore;
    std::thread worker([&semaphore] {
#ifdef SP_TASK_PARENT
        // UCRT terminate handlers belong to the installing thread.
        std::set_terminate([] {
            const auto current = std::current_exception();
            if (!escaping || ready != 1 || pops != 1 || runs != 1 || completions != 0 || failures != 0 || (current && !expected(current))) std::_Exit(92);
            std::fputs("SP_PARENT_TERMINATE_VERIFIED\n", stderr);
            std::fflush(stderr);
            std::_Exit(73);
        });
#endif
        task_thread_func(nullptr, &semaphore);
    });
    worker.join();
    require(ready == 1 && runs == 1);
    require(pops == (throws ? 1 : 2) && completions == (throws ? 0 : 1));
    require(failures == (throws ? 1 : 0));
    require(throws ? expected(retained) : !retained);
    return 0;
}
