#pragma once
// Guest-thread observation only; call after unchanged S1 provider, before camera begin.
#include <cstdint>
#include <cstdio>
#include "camera_identity.h"
#include "camera_identity_layout.h"
static_assert(TOOIE_CAMERA_LAYOUT == 2 && TOOIE_CAMERA_STATE_BYTES == 1380, "G6 camera ABI");
static_assert(offsetof(TooieCameraState, diagnostics) == 1280 && offsetof(TooieCameraState, token) == 1360 && offsetof(TooieCameraState, owner_frame) == 1376, "G6 summary offsets");
namespace tooie_g6_guest {
struct Receipt {
    uint64_t begins = 0, last_print = 0;
    uint32_t lines = 0, last_hash = 0, reason = 0, mismatch = 0, reentry = 0, overflow = 0;
    bool production_seen = false;
};
// Same existing single guest construction owner as the S1 receipt; one process-wide256 cap.
inline Receipt receipt;
inline uint32_t word(uint8_t* rdram, uint32_t offset) noexcept {
    return uint32_t(MEM_W(offset, gpr(int32_t(TOOIE_CAMERA_STATE_VRAM))));
}
inline void next_begin(uint8_t* rdram) {
    // No context pointer or register access; no guest writes or state reset.
    if (uint64_t(TOOIE_CAMERA_STATE_VRAM) < 0x80000000ull || uint64_t(TOOIE_CAMERA_STATE_VRAM) + 1380 > 0x81000000ull ||
        word(rdram, 1024) != TOOIE_CAMERA_MAGIC || word(rdram, 1028) != 2)
        rejected_body("G6 camera summary linked layout2/1380 initialized magic");
    auto& r = receipt; ++r.begins;
    uint32_t d[16]; for (uint32_t i = 0; i < 16; ++i) d[i] = word(rdram, 1280 + i * 4);
    uint32_t hash = 2166136261u;
    for (uint32_t i = 1024; i < 1380; i += 4) hash = (hash ^ word(rdram, i)) * 16777619u;
    const bool first_production = !r.production_seen && d[1] != 0;
    const bool new_failure_or_reason = d[3] != r.reason || d[9] > r.mismatch || d[10] > r.reentry || d[11] > r.overflow;
    const bool periodic_change = hash != r.last_hash && r.begins - r.last_print >= 300;
    if (r.lines < 256 && (!r.lines || first_production || new_failure_or_reason || periodic_change)) {
        std::fprintf(stderr, "baseline camera-g6-guest line=%u begin_calls=%llu prior_camera_epoch=%u active=%u latch=%u main_extra=%u main_reserve=%u backup_depth=%u owner_frame=%08x token_paid=%u token_epoch=%u fill0_extra=%u fill0_reserve=%u fill0_valid=%u fill0_latched=%u fill0_closed=%u fill1_extra=%u fill1_reserve=%u fill1_valid=%u fill1_latched=%u fill1_closed=%u calls=%u productions=%u refusals=%u first_reason_code=%u raw0=%08x raw1=%08x raw2=%08x raw3=%08x tokenless=%u mismatch=%u reentry=%u overflow=%u births=%u revisions=%u id_highwater=%08x reserved_pool_highwater=%u s1_epoch_join_claim=0 final_without_next_begin_unobserved=1\n",
            ++r.lines, (unsigned long long)r.begins, word(rdram, 1032), word(rdram, 1036), word(rdram, 1060),
            word(rdram, 1052), word(rdram, 1056), word(rdram, 1080), word(rdram, 1376), word(rdram, 1372), word(rdram, 1368),
            word(rdram, 1196), word(rdram, 1200), word(rdram, 1204), word(rdram, 1220), word(rdram, 1224),
            word(rdram, 1244), word(rdram, 1248), word(rdram, 1252), word(rdram, 1268), word(rdram, 1272),
            d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
        std::fflush(stderr); r.last_print = r.begins; r.last_hash = hash;
    }
    r.production_seen |= d[1] != 0; r.reason = d[3]; r.mismatch = d[9]; r.reentry = d[10]; r.overflow = d[11];
}
} // namespace tooie_g6_guest
