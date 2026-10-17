// Production guest leaf versus retained baseline/raw originals; no voice/listening verdict.
#include "recomp.h"
#ifndef FB05_CANDIDATE_FRAME_BYTES
#error Supply the qualified production receipt frame facts
#endif
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

extern "C" {
void func_8009DE38(uint8_t*, recomp_context*);
void func_8009DCCC(uint8_t*, recomp_context*);
void tooie_baseline_quiet_talon_trot(uint8_t*, recomp_context*);
}
#include "candidate_fixture.inc"
#define func_808000DC_bsbtrot func_808000DC_bsbtrot_original
#include "original_bsbtrot_fixture.inc"
#undef func_808000DC_bsbtrot
#define func_8009DE38 func_8009DE38_original
#include "original_de38_fixture.inc"
#undef func_8009DE38

namespace {
constexpr uint32_t player = 0x80220000, state104 = 0x80221000;
constexpr uint32_t stack = 0x80818000, stack_window = 512, home_bytes = 16;
static_assert(FB05_CANDIDATE_FRAME_BYTES > 0 && FB05_CANDIDATE_FRAME_BYTES < stack_window);
static_assert(FB05_ORIGINAL_LEAF_FRAME_BYTES == 24 && FB05_ORIGINAL_WRAPPER_FRAME_BYTES == 32);
constexpr std::array<uint32_t, 4> selectors{0, 1, 0xFFFFFFFFu, 0x80000000u};
constexpr std::array<uint8_t, 3> slots{0, 0x80, 0xFF};
constexpr std::array<gpr recomp_context::*, 12> saved_gpr{
    &recomp_context::r16, &recomp_context::r17, &recomp_context::r18, &recomp_context::r19,
    &recomp_context::r20, &recomp_context::r21, &recomp_context::r22, &recomp_context::r23,
    &recomp_context::r28, &recomp_context::r29, &recomp_context::r30, &recomp_context::r31};
constexpr std::array<fpr recomp_context::*, 12> saved_fpr{
    &recomp_context::f20, &recomp_context::f21, &recomp_context::f22, &recomp_context::f23,
    &recomp_context::f24, &recomp_context::f25, &recomp_context::f26, &recomp_context::f27,
    &recomp_context::f28, &recomp_context::f29, &recomp_context::f30, &recomp_context::f31};
struct De38Call { uint32_t owner, sound, pitch; };
struct DcccCall { uint32_t owner, slot, sound, pitch, volume; };
struct Observation {
    unsigned getter = 0;
    std::vector<De38Call> de38;
    std::vector<DcccCall> dccc;
} observed;
uint32_t setting = 0;
bool candidate_call = false, negative_pitch = false;
unsigned off_cases = 0, on_cases = 0, recovery_cases = 0;
unsigned invocations = 0, candidate_getter_total = 0;

gpr guest(uint32_t p) { return S32(p); }
void require(bool value, const char* why) {
    if (!value) throw std::runtime_error(why);
}
uint32_t expected_pitch(uint32_t selector) {
    return selector != 0 ? 0x3F75C28Fu : 0x3F851EB8u;
}
void clobber_volatile(recomp_context* c) {
    c->r1 = 0x101; c->r2 = 0x102; c->r3 = 0x103;
    c->r4 = 0x104; c->r5 = 0x105; c->r6 = 0x106; c->r7 = 0x107;
    c->r8 = 0x108; c->r9 = 0x109; c->r10 = 0x10A; c->r11 = 0x10B;
    c->r12 = 0x10C; c->r13 = 0x10D; c->r14 = 0x10E; c->r15 = 0x10F;
    c->r24 = 0x118; c->r25 = 0x119; c->hi = 0x1234; c->lo = 0x5678;
    c->f0.u64 = c->f2.u64 = c->f4.u64 = c->f6.u64 = c->f8.u64 = 0xABCDEF;
    c->f10.u64 = c->f12.u64 = c->f14.u64 = c->f16.u64 = c->f18.u64 = 0xFEDCBA;
}
void seed(recomp_context& c, uint32_t selector) {
    c = {};
    unsigned index = 0;
    for (auto reg : saved_gpr) c.*reg = guest(0x81230000u + 4u * index++);
    index = 0;
    for (auto reg : saved_fpr) (c.*reg).u64 = 0xCAFEBABE12340000ull + index++;
    c.r4 = guest(player); c.r5 = guest(selector); c.r29 = guest(stack); c.r31 = guest(0x80045678);
    c.r8 = 0xDEADBEEF; c.status_reg = 0xFF01;
    c.mips3_float_mode = 0; c.f_odd = &c.f0.u32h;
}
Observation invoke(recomp_func_t* body, uint32_t selector, uint8_t slot,
                   uint32_t quiet, bool is_candidate, bool wrapper = false) {
    std::vector<uint8_t> memory(16 * 1024 * 1024, 0xCD);
    auto* rdram = memory.data();
    MEM_W(0x104, guest(player)) = state104;
    MEM_BU(2, guest(state104)) = slot;
    // Adjacent bytes differ so an incorrect pointer offset cannot pass by luck.
    MEM_BU(1, guest(state104)) = 0x5A; MEM_BU(3, guest(state104)) = 0xA5;
    const auto before_memory = memory;
    recomp_context c{}; seed(c, selector);
    if (wrapper) { c.r5 = 0x4433; c.r6 = expected_pitch(selector); }
    const auto before = c;
    observed = {}; setting = quiet; candidate_call = is_candidate;
    body(rdram, &c);
    ++invocations;
    require(observed.getter == unsigned(is_candidate), "exact getter call count");
    if (is_candidate) candidate_getter_total += observed.getter;
    for (auto reg : saved_gpr) require(c.*reg == before.*reg, "callee-saved GPR/SP/RA restored");
    for (auto reg : saved_fpr) require((c.*reg).u64 == (before.*reg).u64, "callee-saved FPR bits restored");
    require(c.f_odd == before.f_odd && c.mips3_float_mode == before.mips3_float_mode &&
            c.status_reg == before.status_reg, "float mapping/mode and status preserved");
    const size_t low = (stack & 0x1FFFFFFFu) - stack_window;
    const size_t high = (stack & 0x1FFFFFFFu) + home_bytes;
    require(std::equal(memory.begin(), memory.begin() + low, before_memory.begin()), "memory below stack window unchanged");
    require(std::equal(memory.begin() + high, memory.end(), before_memory.begin() + high), "memory above stack/home window unchanged");
    const size_t frame = wrapper ? FB05_ORIGINAL_WRAPPER_FRAME_BYTES :
        is_candidate ? FB05_CANDIDATE_FRAME_BYTES : FB05_ORIGINAL_LEAF_FRAME_BYTES;
    require(std::equal(memory.begin() + low, memory.begin() + (stack & 0x1FFFFFFFu) - frame,
        before_memory.begin() + low), "no spills below measured guest frame");
    return observed;
}
bool same(const De38Call& a, const De38Call& b) {
    return a.owner == b.owner && a.sound == b.sound && a.pitch == b.pitch;
}
}

extern "C" void tooie_baseline_quiet_talon_trot(uint8_t*, recomp_context* c) {
    ++observed.getter;
    clobber_volatile(c); c->r2 = setting;
}
extern "C" void func_8009DE38(uint8_t*, recomp_context* c) {
    De38Call call{uint32_t(c->r4), uint32_t(c->r5), uint32_t(c->r6)};
    // Deliberate oracle sensitivity control, never a production candidate mutation.
    if (negative_pitch && candidate_call) call.pitch ^= 1u;
    observed.de38.push_back(call); clobber_volatile(c);
}
extern "C" void func_8009DCCC(uint8_t* rdram, recomp_context* c) {
    observed.dccc.push_back({uint32_t(c->r4), uint32_t(c->r5), uint32_t(c->r6),
        uint32_t(c->r7), uint32_t(MEM_W(0x10, c->r29))});
    clobber_volatile(c);
}

int main(int argc, char** argv) try {
    if (argc == 3 && std::string_view(argv[1]) == "--negative" && std::string_view(argv[2]) == "pitch")
        negative_pitch = true;
    else require(argc == 1, "usage: test.exe [--negative pitch]");
    for (uint32_t selector : selectors) {
        const auto original = invoke(func_808000DC_bsbtrot_original, selector, 0, 0, false);
        require(original.de38.size() == 1 && original.dccc.empty(), "original leaf reaches DE38 only");
        const De38Call exact{player, 0x4433, expected_pitch(selector)};
        require(same(original.de38[0], exact), "original selector and exact pitch bits");
        const auto off = invoke(func_808000DC_bsbtrot, selector, 0, 0, true);
        require(off.de38.size() == 1 && off.dccc.empty(), "OFF reaches DE38 exactly once only");
        require(same(off.de38[0], original.de38[0]), "OFF tuple equals extracted original");
        ++off_cases;
        for (uint8_t slot : slots) {
            const auto on = invoke(func_808000DC_bsbtrot, selector, slot, 1, true);
            require(on.de38.empty() && on.dccc.size() == 1, "ON reaches DCCC exactly once only");
            const auto& call = on.dccc[0];
            require(call.owner == player && call.slot == uint32_t(slot) && call.sound == 0x4433 &&
                    call.pitch == original.de38[0].pitch && call.volume == 0, "ON exact player/unsigned slot/sound/pitch/stack volume");
            ++on_cases;
            const auto recovery = invoke(func_8009DE38_original, selector, slot, 0, false, true);
            require(recovery.de38.empty() && recovery.dccc.size() == 1, "original wrapper reaches DCCC only");
            const auto& recovered = recovery.dccc[0];
            require(recovered.owner == call.owner && recovered.slot == call.slot &&
                    recovered.sound == call.sound && recovered.pitch == call.pitch &&
                    recovered.volume == 0x55F0, "original wrapper recovery differs only in volume");
            ++recovery_cases;
        }
    }
    require(off_cases == 4 && on_cases == 12 && recovery_cases == 12 &&
            invocations == 32 && candidate_getter_total == 16, "exact normalized case totals");
    std::cout << "{\"status\":\"PASS\",\"off_cases\":4,\"on_cases\":12,\"recovery_cases\":12,"
        "\"original_leaf_cases\":4,\"invocations\":32,\"candidate_getter_calls\":16,"
        "\"rdram_bytes\":16777216,\"stack_window_below\":512,\"o32_home_bytes\":16,"
        "\"memory_outside_stack_unchanged\":true,\"measured_frame_lower_guard\":true,\"callee_saved_gpr_fpr_restored\":true,"
        "\"scope\":\"compiled leaf ABI only; no sound voice, splice, menu, integration or listening acceptance\"}\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "FAIL: " << error.what() << '\n'; return 1;
}
