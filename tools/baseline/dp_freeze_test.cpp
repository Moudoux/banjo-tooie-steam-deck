#include "dp-freeze.hpp"
#include "recomp.h"
#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>

std::atomic_bool exited{false};
extern "C" void osDpGetStatus_recomp(uint8_t*, recomp_context*);
extern "C" void osDpSetStatus_recomp(uint8_t*, recomp_context*);
using namespace std::chrono_literals;
namespace gate = ultramodern::detail::dp_freeze;

void require(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::_Exit(EXIT_FAILURE); // A broken waiter must not hang test teardown.
    }
}
void set_status(uint32_t flags) {
    recomp_context ctx{};
    ctx.r4 = flags;
    osDpSetStatus_recomp(nullptr, &ctx);
}
uint32_t get_status() {
    recomp_context ctx{};
    osDpGetStatus_recomp(nullptr, &ctx);
    return uint32_t(ctx.r2);
}

int main() {
    require(get_status() == 0x80, "initial BufferReady status");
    set_status(2 | 8 | 32);
    require(get_status() == 0x87, "set Xbus/Freeze/Flush");
    set_status(4 | 8);
    require(get_status() == 0x87, "paired Freeze clear/set must preserve status");
    set_status(4);
    require(get_status() == 0x85, "Freeze clear must preserve unrelated bits");
    set_status(1 | 16);
    require(get_status() == 0x80, "clear Xbus/Flush");

    std::atomic<unsigned> actions{0};
    for (bool stop : {false, true}) {
        set_status(8);
        std::promise<void> started;
        auto ready = started.get_future();
        auto done = std::async(std::launch::async, [&] {
            started.set_value();
            return gate::wait_and_run([&] { ++actions; });
        });
        require(ready.wait_for(2s) == std::future_status::ready, "waiter did not start");
        require(done.wait_for(50ms) == std::future_status::timeout && actions.load() == 0,
            "frozen action must remain blocked");
        if (stop) {
            exited.store(true);
            gate::notify_stop();
        } else {
            set_status(4);
        }
        require(done.wait_for(2s) == std::future_status::ready, "clear/stop wake was lost");
        require(done.get() == !stop && actions.load() == (stop ? 0u : 1u),
            "clear must run once; observed stop must run no action");
        actions.store(0);
    }
    std::cout << "PASS status pairs, clear wake, frozen stop\n";
}
