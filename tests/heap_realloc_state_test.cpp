// Executes the sole canonical patch-origin heap_realloc body, extracted at
// build time. The fixture covers the repaired allocator paths and guest ABI.
#include "recomp.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
void heap_resize_block(uint8_t*, recomp_context*);
void func_8001B864(uint8_t*, recomp_context*);
void heap_alloc(uint8_t*, recomp_context*);
void func_8001BD30(uint8_t*, recomp_context*);
void aligned8_memcpy(uint8_t*, recomp_context*);
void heap_free(uint8_t*, recomp_context*);
}

#include "heap_realloc_fixture.inc"

namespace {
constexpr uint32_t kOld = 0x80010000;
constexpr uint32_t kNew = 0x80020000;
constexpr uint32_t kFreePrev = 0x80030000;
constexpr uint32_t kFreeNext = 0x80030100;
constexpr std::size_t kRamSize = 8 * 1024 * 1024;

enum class Mode { Move, Fail, Shrink, Adjacent };
struct Calls {
    Mode mode = Mode::Move;
    uint32_t allocation_result = kNew;
    unsigned resize = 0, alloc = 0, copies = 0, frees = 0;
    uint32_t resize_header = 0, resize_size = 0;
    uint32_t alloc_size = 0, copy_dst = 0, copy_src = 0, copy_size = 0;
    uint32_t free_ptr = 0;
    std::vector<uint32_t> pins;
    std::vector<std::string> order;
} calls;

uint32_t u32(gpr value) { return static_cast<uint32_t>(value); }
gpr guest(uint32_t value) { return S32(value); }
void require(bool condition, const char* detail) {
    if (!condition) throw std::runtime_error(detail);
}

void put_word(uint8_t* rdram, uint32_t address, uint32_t value) {
    MEM_W(guest(address), 0) = value;
}
uint32_t word(uint8_t* rdram, uint32_t address) {
    return static_cast<uint32_t>(MEM_W(guest(address), 0));
}
void fill(uint8_t* rdram, uint32_t address, std::size_t count, uint8_t value) {
    for (std::size_t i = 0; i < count; ++i) MEM_B(guest(address + static_cast<uint32_t>(i)), 0) = value;
}
uint8_t byte(uint8_t* rdram, uint32_t address) {
    return static_cast<uint8_t>(MEM_BU(guest(address), 0));
}

recomp_context context(uint32_t request) {
    recomp_context ctx{};
    ctx.r4 = guest(kOld);
    ctx.r5 = request;
    ctx.r16 = 0x16161616;
    ctx.r17 = 0x17171717;
    ctx.r18 = 0x18181818; ctx.r19 = 0x19191919; ctx.r20 = 0x20202020;
    ctx.r21 = 0x21212121; ctx.r22 = 0x22222222; ctx.r23 = 0x23232323;
    ctx.r28 = 0x28282828; ctx.r30 = 0x30303030;
    ctx.r29 = guest(0x8007F000);
    ctx.r31 = guest(0x80045678);
    return ctx;
}

void check_abi(const recomp_context& before, const recomp_context& after) {
    require(after.r16 == before.r16, "s0 was not restored");
    require(after.r17 == before.r17, "s1 was not restored");
    require(after.r18 == before.r18 && after.r19 == before.r19 && after.r20 == before.r20 &&
            after.r21 == before.r21 && after.r22 == before.r22 && after.r23 == before.r23 &&
            after.r28 == before.r28 && after.r30 == before.r30, "callee-saved guest registers changed");
    require(after.r29 == before.r29, "stack pointer was not restored");
    require(after.r31 == before.r31, "return address was not restored");
}

void set_old_block(uint8_t* rdram, uint32_t capacity, bool next_free) {
    const uint32_t next = kOld + capacity;
    put_word(rdram, kOld - 12, next);
    MEM_B(guest(next + 15), 0) = next_free ? 0 : 0x40;
}

void move_growth() {
    std::vector<uint8_t> ram(kRamSize);
    auto* rdram = ram.data();
    calls = {}; calls.mode = Mode::Move; calls.allocation_result = kNew;
    constexpr uint32_t capacity = 0x30, request = 0x60;
    set_old_block(ram.data(), capacity, false);
    for (uint32_t i = 0; i < capacity; ++i) MEM_B(guest(kOld + i), 0) = static_cast<uint8_t>(0x20 + i);
    fill(ram.data(), kOld + capacity, 0x20, 0xE7);
    fill(ram.data(), kNew, request, 0xCC);
    put_word(rdram, kOld - 4, 0x001122D8);
    put_word(rdram, kNew - 4, 0x33445547);
    std::array<uint8_t, capacity> expected{};
    for (uint32_t i = 0; i < capacity; ++i) expected[i] = byte(ram.data(), kOld + i);
    auto ctx = context(request), before = ctx;
    heap_realloc(ram.data(), &ctx);
    require(u32(ctx.r2) == kNew, "move growth did not return new allocation");
    require(calls.alloc == 1 && calls.alloc_size == request, "move growth allocation mismatch");
    require(calls.copies == 1 && calls.copy_src == kOld && calls.copy_dst == kNew,
            "move growth copy endpoints mismatch");
    require(calls.copy_size == capacity, "move growth copied beyond old physical payload");
    for (uint32_t i = 0; i < capacity; ++i)
        require(byte(ram.data(), kNew + i) == expected[i], "old payload or padding was not preserved");
    for (uint32_t i = capacity; i < request; ++i)
        require(byte(ram.data(), kNew + i) == 0xCC, "destination extension was overwritten");
    require(calls.frees == 1 && calls.free_ptr == kOld, "old allocation was not freed exactly once");
    require(calls.pins == std::vector<uint32_t>({kOld, 0}), "move growth pin lifecycle mismatch");
    require(word(rdram,kNew - 4)==0x3344554B, "moving growth flags or unrelated new header bits changed");
    require(calls.order==std::vector<std::string>({"pin","alloc","align","copy","free","unpin"}),
            "moving growth call order changed");
    check_abi(before, ctx);
}

void allocation_failure() {
    std::vector<uint8_t> ram(kRamSize);
    auto* rdram = ram.data();
    calls = {}; calls.mode = Mode::Fail; calls.allocation_result = 0;
    constexpr uint32_t capacity = 0x30, request = 0x60;
    set_old_block(ram.data(), capacity, false);
    for (uint32_t i = 0; i < capacity + 0x20; ++i) MEM_B(guest(kOld - 16 + i), 0) = static_cast<uint8_t>(i ^ 0xA5);
    // Restore the next pointer/state after seeding the region.
    set_old_block(ram.data(), capacity, false);
    std::array<uint8_t, capacity + 0x20> snapshot{};
    for (uint32_t i = 0; i < snapshot.size(); ++i) snapshot[i] = byte(ram.data(), kOld - 16 + i);
    auto ctx = context(request), before = ctx;
    heap_realloc(ram.data(), &ctx);
    require(ctx.r2 == 0, "failed growth did not return NULL");
    require(calls.alloc == 1 && calls.copies == 0 && calls.frees == 0,
            "failed growth modified allocation ownership");
    require(calls.pins == std::vector<uint32_t>({kOld, 0}), "failed growth left old allocation pinned");
    for (uint32_t i = 0; i < snapshot.size(); ++i)
        require(byte(ram.data(), kOld - 16 + i) == snapshot[i], "failed growth changed old block");
    require(calls.order==std::vector<std::string>({"pin","alloc","unpin"}), "failure call order changed");
    check_abi(before, ctx);
}

void shrink_in_place() {
    std::vector<uint8_t> ram(kRamSize);
    auto* rdram = ram.data();
    calls = {}; calls.mode = Mode::Shrink;
    set_old_block(ram.data(), 0x60, false);
    auto ctx = context(0x20), before = ctx;
    heap_realloc(ram.data(), &ctx);
    require(u32(ctx.r2) == kOld, "shrink did not retain pointer");
    require(calls.resize == 1 && calls.resize_header == kOld - 16 && calls.resize_size == 0x20,
            "shrink resize mismatch");
    require(calls.pins.empty() && calls.alloc == 0 && calls.copies == 0 && calls.frees == 0,
            "shrink entered growth path");
    require(calls.order==std::vector<std::string>({"resize"}), "shrink call order changed");
    check_abi(before, ctx);
}

void adjacent_growth() {
    std::vector<uint8_t> ram(kRamSize);
    auto* rdram = ram.data();
    calls = {}; calls.mode = Mode::Adjacent;
    constexpr uint32_t capacity = 0x30, request = 0x60;
    set_old_block(ram.data(), capacity, true);
    const uint32_t next = kOld + capacity;
    const uint32_t next_next = kOld - 16 + 16 + request + 0x20;
    put_word(ram.data(), next + 4, next_next);
    put_word(ram.data(), next + 16, kFreePrev);
    put_word(ram.data(), next + 20, kFreeNext);
    put_word(ram.data(), kFreeNext + 16, next);
    put_word(ram.data(), kFreePrev + 20, next);
    put_word(ram.data(), next_next, next);
    put_word(rdram, 0x8003F750, 0x20);
    auto ctx = context(request), before = ctx;
    heap_realloc(ram.data(), &ctx);
    require(u32(ctx.r2) == kOld, "adjacent growth moved allocation");
    require(calls.resize == 1 && calls.resize_header == kOld - 16 && calls.resize_size == request,
            "adjacent growth resize mismatch");
    require(calls.pins == std::vector<uint32_t>({kOld, 0}), "adjacent growth pin lifecycle mismatch");
    require(calls.alloc == 0 && calls.copies == 0 && calls.frees == 0,
            "adjacent growth entered move path");
    require(word(rdram,kFreeNext+16)==kFreePrev && word(rdram,kFreePrev+20)==kFreeNext,
            "adjacent free-list unlink differs");
    require(word(rdram,kOld-12)==next_next && word(rdram,next_next)==kOld-16,
            "adjacent block-list splice differs");
    require(word(rdram,0x8003F750)==0x20+next_next-next, "adjacent heap accounting differs");
    require(calls.order==std::vector<std::string>({"pin","resize","unpin"}), "adjacent call order changed");
    check_abi(before, ctx);
}
} // namespace

extern "C" void heap_resize_block(uint8_t*, recomp_context* ctx) {
    calls.order.push_back("resize");
    ++calls.resize; calls.resize_header = u32(ctx->r4); calls.resize_size = u32(ctx->r5);
}
extern "C" void func_8001B864(uint8_t*, recomp_context* ctx) {
    calls.order.push_back(ctx->r4 ? "pin" : "unpin");
    calls.pins.push_back(u32(ctx->r4));
}
extern "C" void heap_alloc(uint8_t*, recomp_context* ctx) {
    calls.order.push_back("alloc");
    ++calls.alloc; calls.alloc_size = u32(ctx->r4); ctx->r2 = guest(calls.allocation_result);
}
extern "C" void func_8001BD30(uint8_t*, recomp_context* ctx) {
    calls.order.push_back("align");
    ctx->r2 = (u32(ctx->r4) + 15u) & ~15u;
}
extern "C" void aligned8_memcpy(uint8_t* rdram, recomp_context* ctx) {
    calls.order.push_back("copy");
    ++calls.copies;
    calls.copy_dst = u32(ctx->r4); calls.copy_src = u32(ctx->r5); calls.copy_size = u32(ctx->r6);
    for (uint32_t i = 0; i < calls.copy_size; ++i)
        MEM_B(guest(calls.copy_dst + i), 0) = MEM_BU(guest(calls.copy_src + i), 0);
}
extern "C" void heap_free(uint8_t*, recomp_context* ctx) {
    calls.order.push_back("free");
    ++calls.frees; calls.free_ptr = u32(ctx->r4);
}

int main() {
    try {
        move_growth();
        allocation_failure();
        shrink_in_place();
        adjacent_growth();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
