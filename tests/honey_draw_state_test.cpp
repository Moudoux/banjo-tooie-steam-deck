// Executes the actual linked-image body extracted into honey_draw_fixture.inc.
#include "recomp.h"
#define F3DEX_GBI_2
#include "rt64_extended_gbi.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
void func_800A7180(uint8_t*, recomp_context*);
void func_800A89F8(uint8_t*, recomp_context*);
void func_800EB854(uint8_t*, recomp_context*);
void func_80101870(uint8_t*, recomp_context*);
void func_80103110(uint8_t*, recomp_context*);
}
#include "honey_draw_fixture.inc"

namespace {
static_assert(sizeof(GfxCommand) == 8 && sizeof(unsigned) == 4);
// Guest offsets from common.h: Actor.unk0=0, unk7C_12=bit12;
// Unk80132ED0 stride30, unk10=10, unk14=14. No host pointer layout assumed.
constexpr uint32_t actor = 0x80010000, argument = 0x80011000;
constexpr uint32_t descriptor = 0x80132ED0 + 3 * 0x30, store = 0x80127658;
constexpr uint32_t pool0 = 0x80040000, pool1 = 0x80050000, expected_id = 0x40018027;
uint32_t view, phase, draw_bytes, draw_entry, show_cursor;
std::vector<unsigned> calls;
void require(bool yes, const char* detail) { if (!yes) throw std::runtime_error(detail); }
gpr guest(uint32_t address) { return S32(address); }
uint32_t word(uint8_t* rdram, uint32_t address) { return uint32_t(MEM_W(0, guest(address))); }
void put(uint8_t* rdram, uint32_t address, uint32_t value) { MEM_W(0, guest(address)) = value; }
void run(bool emit, unsigned scenario) {
    std::vector<uint8_t> ram(8 * 1024 * 1024);
    uint8_t* rdram = ram.data();
    calls.clear(); view = scenario == 2 ? 1 : 0; draw_bytes = scenario == 1 ? 0 : 16;
    phase = scenario == 10 ? 1 : 0;
    const uint32_t override_pool = scenario == 8 ? 0x80060000 : 0;
    const int index = scenario == 3 ? -1 : scenario == 4 ? 2 : scenario == 7 ? 1 : 0;
    const uint32_t base = scenario == 7 ? pool1 : pool0;
    const uint32_t start = scenario == 5 ? base - 8 : scenario == 6 ? base + 0x8FC0 - 24 : base + 64;
    const bool admitted = emit && (scenario == 0 || scenario == 1 || scenario == 7);
    show_cursor = start + draw_bytes + (admitted ? 32 : 0);
    put(rdram, actor, descriptor); put(rdram, actor + 0x7C, scenario == 1 ? 0 : 0x1000);
    put(rdram, descriptor + 0x10, 0xA53C0001 | (39 << 1));
    MEM_H(0, guest(descriptor + 0x14)) = scenario == 9 ? 0x613 : 0x612;
    put(rdram, 0x8007BCE8, override_pool); put(rdram, 0x8007BCD0, pool0); put(rdram, 0x8007BCD4, pool1);
    MEM_H(0, guest(0x8007BEDA)) = index; put(rdram, store, start);
    for (int offset = -8; offset < 72; offset += 4) put(rdram, start + offset, 0xA5A5A5A5);
    const auto actor_before = std::vector<uint8_t>(ram.begin() + (actor & 0x7FFFFFFF), ram.begin() + (actor & 0x7FFFFFFF) + 0x90);
    const auto descriptor_before = std::vector<uint8_t>(ram.begin() + (descriptor & 0x7FFFFFFF), ram.begin() + (descriptor & 0x7FFFFFFF) + 0x30);
    recomp_context ctx{};
    ctx.r4 = guest(actor); ctx.r5 = guest(argument); ctx.r29 = guest(0x8007F000); ctx.r31 = guest(0x80045678);
    ctx.r16 = 16; ctx.r17 = 17; ctx.r18 = 18; ctx.r19 = 19;
    ctx.r20 = 20; ctx.r21 = 21; ctx.r22 = 22; ctx.r23 = 23; ctx.r28 = 28; ctx.r30 = 30;
    func_80800380_chhoneycarrier(rdram, &ctx);
    require(ctx.r29 == guest(0x8007F000) && ctx.r31 == guest(0x80045678), "guest sp/ra not restored");
    require(ctx.r16 == 16 && ctx.r17 == 17 && ctx.r18 == 18 && ctx.r19 == 19 &&
            ctx.r20 == 20 && ctx.r21 == 21 && ctx.r22 == 22 && ctx.r23 == 23 &&
            ctx.r28 == 28 && ctx.r30 == 30, "callee-saved registers changed");
    std::vector<unsigned> order;
    if (emit) { order.push_back(0); order.push_back(1); if (!view) order.push_back(2); }
    order.push_back(3); if (scenario != 1) order.push_back(4);
    require(calls == order, "getter/draw/show call order or count");
    require(draw_entry == start + (admitted ? 24 : 0), "draw did not receive post-push cursor");
    require(word(rdram, store) == start + draw_bytes + (admitted ? 32 : 0), "post-draw cursor reload/pop extent");
    require(word(rdram, 0x8007BCE8) == override_pool && word(rdram, 0x8007BCD0) == pool0 && word(rdram, 0x8007BCD4) == pool1 &&
            MEM_H(0, guest(0x8007BEDA)) == index, "original pool/index/override globals changed");
    require(std::equal(actor_before.begin(), actor_before.end(), ram.begin() + (actor & 0x7FFFFFFF)), "Actor changed");
    require(std::equal(descriptor_before.begin(), descriptor_before.end(), ram.begin() + (descriptor & 0x7FFFFFFF)), "descriptor changed");
    std::array<GfxCommand, 4> commands{};
    gEXEnable(commands.data());
    gEXMatrixGroupDecomposed(commands.data() + 1, expected_id, G_EX_PUSH, 0,
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
        G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP,
        G_EX_ORDER_LINEAR, G_EX_EDIT_NONE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP);
    gEXPopMatrixGroup(commands.data() + 3, 0);
    std::vector<uint32_t> expected;
    if (admitted) for (unsigned i = 0; i < 3; ++i) {
        expected.push_back(commands[i].values.word0); expected.push_back(commands[i].values.word1);
    }
    for (unsigned i = 0; i < draw_bytes / 4; ++i) expected.push_back(0xD0000000 + i);
    if (admitted) { expected.push_back(commands[3].values.word0); expected.push_back(commands[3].values.word1); }
    for (int offset = -8; offset < 72; offset += 4) {
        const uint32_t wanted = offset >= 0 && unsigned(offset / 4) < expected.size() ? expected[offset / 4] : 0xA5A5A5A5;
        require(word(rdram, start + offset) == wanted, "command encoding/ID/pop or surrounding sentinel");
    }
}
}
extern "C" void func_800A7180(uint8_t*, recomp_context* ctx) { calls.push_back(0); ctx->r2 = guest(store); }
extern "C" void func_800A89F8(uint8_t*, recomp_context* ctx) { calls.push_back(1); ctx->r2 = view; }
extern "C" void func_800EB854(uint8_t*, recomp_context* ctx) { calls.push_back(2); ctx->r2 = phase; }
extern "C" void func_80101870(uint8_t* rdram, recomp_context* ctx) {
    require((uint32_t(ctx->r29) & 7) == 0, "original draw o32 stack alignment");
    require(uint32_t(ctx->r4) == actor && uint32_t(ctx->r5) == argument, "original draw arguments");
    calls.push_back(3); draw_entry = word(rdram, store);
    for (unsigned i = 0; i < draw_bytes / 4; ++i) put(rdram, draw_entry + 4 * i, 0xD0000000 + i);
    put(rdram, store, draw_entry + draw_bytes);
    ctx->r2 = draw_bytes ? guest(0x80060000) : 0; ctx->r4 = 0xBAD; ctx->r5 = 0xBAD;
}
extern "C" void func_80103110(uint8_t* rdram, recomp_context* ctx) {
    require(uint32_t(ctx->r4) == actor && ctx->r5 == 1, "original optional-show arguments");
    require(word(rdram, store) == show_cursor, "pop/cursor restoration must precede optional show");
    calls.push_back(4); ctx->r2 = 0xBAD;
}
int main(int argc, char** argv) {
    try {
        require(argc == 2 && (std::string(argv[1]) == "on" || std::string(argv[1]) == "off"), "usage: HoneyDrawStateTest on|off");
        for (unsigned scenario = 0; scenario < 11; ++scenario) run(std::string(argv[1]) == "on", scenario);
        std::cout << "Honey linked-body fixture passed\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
