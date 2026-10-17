// Actual translated guest owners; imports are explicit fixture boundaries.
#include "recomp.h"
#include "widescreen_owner_facts.h"
#include "widescreen_owner_functions.h"
#include "../tools/baseline/guest/minimum-host/minimum_presentation_abi.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef WIDE_OWNER_NEGATIVE_ACTIVE
#include "widescreen_owner_negative.inc"
#else
#include "widescreen_owner_candidate.inc"
#endif

namespace {
constexpr size_t ram_bytes = 16 * 1024 * 1024;
constexpr uint32_t stack = 0x80818000;
constexpr const char* negative_oracle = "mid-cutscene frame must not restore";
std::vector<uint8_t> memory(ram_bytes);
uint8_t* rdram = memory.data();
uint32_t settings = 0, remembered = 0;
unsigned getter_calls = 0, settings_calls = 0, abi_checks = 0, cases = 0;
std::vector<uint32_t> setters;
std::vector<std::array<uint32_t, 2>> publications;
constexpr std::array<gpr recomp_context::*, 12> saved{
    &recomp_context::r16, &recomp_context::r17, &recomp_context::r18, &recomp_context::r19,
    &recomp_context::r20, &recomp_context::r21, &recomp_context::r22, &recomp_context::r23,
    &recomp_context::r28, &recomp_context::r29, &recomp_context::r30, &recomp_context::r31};
constexpr std::array<fpr recomp_context::*, 12> fsaved{
    &recomp_context::f20, &recomp_context::f21, &recomp_context::f22, &recomp_context::f23,
    &recomp_context::f24, &recomp_context::f25, &recomp_context::f26, &recomp_context::f27,
    &recomp_context::f28, &recomp_context::f29, &recomp_context::f30, &recomp_context::f31};
static_assert(BASELINE_PRESENT_WIDE_REQUEST_SHIFT == 6 && BASELINE_PRESENT_WIDE_REQUEST_MASK == 3);
static_assert(BASELINE_PRESENT_USE_GAME == 0 && BASELINE_PRESENT_FORCE_OFF == 1 && BASELINE_PRESENT_FORCE_ON == 2);
static_assert(BASELINE_PUBLISH_PILLARBOX == 1 && BASELINE_PUBLISH_WIDE == 2 &&
    BASELINE_PUBLISH_FLAGS_MASK == 3 && BASELINE_PUBLISH_GENERATION_SHIFT == 2);

void require(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
gpr guest(uint32_t value) { return S32(value); }
uint32_t word(uint32_t address) { return uint32_t(MEM_W(0, guest(address))); }
void word(uint32_t address, uint32_t value) { MEM_W(0, guest(address)) = value; }
uint32_t wide() { return uint16_t(MEM_H(0, guest(WIDE_OWNER_HALFWORD))); }
void wide(uint32_t value) { MEM_H(0, guest(WIDE_OWNER_HALFWORD)) = value; }
uint32_t active() { return word(WIDE_OWNER_STATE + 4); }
uint32_t request(uint32_t mode, bool original = true, bool capable = true, uint32_t generation = 7) {
    return (generation << BASELINE_PRESENT_GENERATION_SHIFT) |
        (mode << BASELINE_PRESENT_WIDE_REQUEST_SHIFT) |
        (original ? BASELINE_PRESENT_ORIGINAL_CUTSCENE : 0u) |
        (capable ? BASELINE_PRESENT_PILLARBOX_CAPABLE : 0u) |
        BASELINE_PRESENT_ACTIVE | BASELINE_PRESENT_WIDE;
}
void clobber(recomp_context* c) {
    c->r1 = 0x101; c->r2 = 0x102; c->r3 = 0x103; c->r4 = 0x104;
    c->r5 = 0x105; c->r6 = 0x106; c->r7 = 0x107; c->r8 = 0x108;
    c->r9 = 0x109; c->r10 = 0x10a; c->r11 = 0x10b; c->r12 = 0x10c;
    c->r13 = 0x10d; c->r14 = 0x10e; c->r15 = 0x10f; c->r24 = 0x118; c->r25 = 0x119;
    c->f0.u64 = c->f2.u64 = c->f4.u64 = c->f6.u64 = c->f8.u64 = 0xabcdef;
    c->f10.u64 = c->f12.u64 = c->f14.u64 = c->f16.u64 = c->f18.u64 = 0xfedcba;
}
void invoke(recomp_func_t* fn, uint32_t command = 0) {
    recomp_context c{};
    unsigned index = 0;
    for (auto r : saved) c.*r = guest(0x81230000 + 4 * index++);
    index = 0;
    for (auto r : fsaved) (c.*r).u64 = 0xcafebabe12340000ull + index++;
    c.r4 = guest(command); c.r29 = guest(stack); c.r31 = guest(0x80045678);
    c.status_reg = 0xff01; c.mips3_float_mode = 0; c.f_odd = &c.f0.u32h;
    const auto before = c;
    const auto bytes = memory;
    const auto preference = remembered;
    const auto settings_before = settings_calls;
    fn(rdram, &c);
    for (auto r : saved) require(c.*r == before.*r, "callee-saved GPR/SP/RA changed");
    for (auto r : fsaved) require((c.*r).u64 == (before.*r).u64, "callee-saved FPR changed");
    require(c.f_odd == before.f_odd && c.status_reg == before.status_reg &&
        c.mips3_float_mode == before.mips3_float_mode, "FR mapping/status changed");
    require(remembered == preference, "guest owner overwrote remembered game preference");
    require(settings_calls == settings_before + 1, "owner must sample settings once");
    // Guest halfwords are XOR2-swizzled in host RDRAM; words are not.
    std::array<std::array<size_t, 2>, 3> allowed{{
        {stack - 4096u - 0x80000000u, stack + 16u - 0x80000000u},
        {WIDE_OWNER_STATE - 0x80000000u, WIDE_OWNER_STATE + 8u - 0x80000000u},
        {(WIDE_OWNER_HALFWORD - 0x80000000u) ^ 2u, ((WIDE_OWNER_HALFWORD - 0x80000000u) ^ 2u) + 2u}}};
    std::sort(allowed.begin(), allowed.end());
    size_t cursor = 0;
    for (auto [lo, hi] : allowed) {
        require(lo >= cursor && hi <= ram_bytes, "fixture allowed spans overlap/outside RDRAM");
        require(std::memcmp(memory.data() + cursor, bytes.data() + cursor, lo - cursor) == 0,
            "write outside stack/state/halfword");
        cursor = hi;
    }
    require(std::memcmp(memory.data() + cursor, bytes.data() + cursor, ram_bytes - cursor) == 0,
        "write above allowed regions");
    ++abi_checks;
}
void frame() { invoke(tooie_baseline_widescreen_frame); }
void command(uint32_t value) { invoke(tooie_baseline_cutscene_command, value); }
void reset(uint32_t mode = BASELINE_PRESENT_FORCE_ON, bool original = true, bool capable = true) {
    std::fill(memory.begin(), memory.end(), 0xcd);
    word(WIDE_OWNER_STATE, 0); word(WIDE_OWNER_STATE + 4, 0); wide(0);
    settings = request(mode, original, capable); remembered = 0;
    getter_calls = settings_calls = 0; setters.clear(); publications.clear();
}
void published(uint32_t flags) {
    require(!publications.empty() && publications.back() == std::array<uint32_t, 2>{7, flags},
        "wrong generation/publication flags");
}
}

extern "C" void tooie_baseline_presentation_settings(uint8_t*, recomp_context* c) {
    ++settings_calls; clobber(c); c->r2 = settings;
}
extern "C" void func_800DA298(uint8_t*, recomp_context* c) {
    require(uint32_t(c->r4) == 0xBB9, "getter must query exact BB9 flag");
    ++getter_calls; clobber(c); c->r2 = remembered;
}
extern "C" void set_widescreen(uint8_t*, recomp_context* c) {
    const auto value = uint32_t(c->r4);
    require(value <= 1, "setter requires boolean"); setters.push_back(value); wide(value); clobber(c);
}
extern "C" void tooie_baseline_presentation_publish(uint8_t*, recomp_context* c) {
    const auto generation = uint32_t(c->r4), flags = uint32_t(c->r5);
    require((flags & ~BASELINE_PUBLISH_FLAGS_MASK) == 0 && flags != 3, "publication flags not exclusive");
    publications.push_back({generation, flags}); clobber(c);
}

int main() try {
    reset(); settings = request(BASELINE_PRESENT_USE_GAME, true, true, 0);
    frame(); command(2);
    require(getter_calls == 0 && setters.empty() && publications.empty() &&
        word(WIDE_OWNER_STATE) == 0 && active() == 0, "generation zero must be inert"); ++cases;

    reset(BASELINE_PRESENT_USE_GAME); frame(); published(0);
    require(getter_calls == 1 && setters.empty(), "Use game OFF must avoid setter");
    remembered = 1; frame(); published(2);
    require(setters == std::vector<uint32_t>{1}, "Use game ON must set live halfword");
    frame(); require(setters.size() == 1 && getter_calls == 3, "unchanged frame repeated setter"); ++cases;

    reset(); frame(); published(2);
    require(getter_calls == 0 && remembered == 0 && wide() == 1, "forced ON must not query or persist BB9");
    settings = request(BASELINE_PRESENT_FORCE_OFF); frame(); published(0);
    require(getter_calls == 0 && setters == std::vector<uint32_t>({1, 0}), "forced OFF did not restore halfword"); ++cases;

    reset(); frame(); wide(0); frame();
    require(setters == std::vector<uint32_t>({1, 1}), "forced ON must repair external halfword change");
    settings = request(BASELINE_PRESENT_USE_GAME); remembered = 0; wide(0); frame();
    require(setters.size() == 2 && getter_calls == 1, "Use game options change must avoid unnecessary setter"); ++cases;

    // No preceding frame: initialization MUST precede command2's active write.
    reset(); wide(1); command(2); published(1);
    require(word(WIDE_OWNER_STATE) == 7 && active() == 1 && wide() == 0 &&
        setters == std::vector<uint32_t>{0}, "first command2 must initialize then suppress");
    frame();
    require(active() == 1 && wide() == 0 && setters.size() == 1 &&
        publications.back() == std::array<uint32_t, 2>{7, 1}, negative_oracle); ++cases;
    for (auto value : {3u, 6u}) {
        command(value); published(1);
        require(active() == 1 && wide() == 0 && setters.size() == 1, "commands3/6 changed active suppression");
    }
    for (auto value : {1u, 4u, 5u}) {
        command(value); published(2); require(active() == 0 && wide() == 1, "end command did not restore wide");
        command(2); require(active() == 1 && wide() == 0, "next cutscene did not suppress");
    }
    require(word(WIDE_OWNER_STATE) == 7, "ordinary commands reset generation"); ++cases;

    reset(BASELINE_PRESENT_FORCE_ON, false); wide(1); command(2); published(2);
    require(active() == 1 && setters.empty(), "match-gameplay cutscene must retain wide");
    reset(BASELINE_PRESENT_FORCE_ON, true, false); wide(1); command(2); published(2);
    require(setters.empty(), "incapable cutscene must not suppress"); ++cases;

    reset(); command(2); settings = request(BASELINE_PRESENT_FORCE_OFF); frame(); command(1); published(0);
    require(wide() == 0 && setters.empty(), "OFF during cutscene must prevent end restoration"); ++cases;
    reset(BASELINE_PRESENT_FORCE_OFF); command(2); settings = request(BASELINE_PRESENT_FORCE_ON); frame(); published(1);
    require(wide() == 0 && setters.empty() && active() == 1, "ON during active cutscene restored early");
    command(4); published(2); require(wide() == 1 && setters == std::vector<uint32_t>{1}, "ON must restore at end"); ++cases;
    reset(BASELINE_PRESENT_USE_GAME); remembered = 1; command(2); frame(); published(1);
    command(5); published(2); require(wide() == 1 && remembered == 1, "Use game did not restore remembered ON"); ++cases;
    reset(BASELINE_PRESENT_USE_GAME); command(2); frame(); published(0);
    require(active() == 1 && wide() == 0 && setters.empty(), "remembered OFF should not pillarbox"); ++cases;

    std::cout << "widescreen owner PASS: " << cases << " cases, " << abi_checks
        << " ABI/memory checks; translated guest helpers, no gameplay acceptance\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "widescreen owner FAIL: " << error.what() << '\n';
#ifdef WIDE_OWNER_NEGATIVE_ACTIVE
    // WILL_FAIL accepts only the exact suppression oracle, never another error.
    return std::string(error.what()) == negative_oracle ? 1 : 0;
#else
    return 1;
#endif
}
