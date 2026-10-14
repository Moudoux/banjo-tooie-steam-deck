// Guest-order identity: hashes MEM_BU bytes, not host-swizzled RDRAM words.
#include <array>
#include <cstdio>
#include <cstdlib>
#include <span>
#include "guest_digest.hpp"

static void require(bool ok, const char* what) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", what); std::exit(1); }
}

int main() {
    std::array<uint8_t, 8> rdram{};
    // Guest bytes "abc" at 80000000; recompiled RDRAM stores each byte at address ^ 3.
    rdram[3] = 'a'; rdram[2] = 'b'; rdram[1] = 'c';
    constexpr char abc[] = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";  // FIPS 180-2 "abc".
    require(baseline::guest_range_sha256(rdram.data(), 0x80000000, 3) == abc, "guest-order known vector");
    require(tooie::platform::digest(std::span(rdram).first(3), true) != abc, "host-swizzled bytes must not match");
    rdram[1] = 'd';
    require(baseline::guest_range_sha256(rdram.data(), 0x80000000, 3) != abc, "changed guest byte must not match");
    std::puts("PASS baseline guest digest");
    return 0;
}
