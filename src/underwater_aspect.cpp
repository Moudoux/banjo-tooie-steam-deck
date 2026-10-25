// Underwater sway at host width. Upstream's guest cam_unwiden_raster
// (camera_emitters.c) narrows the world camera's raster projection back to 4:3
// so RT64's AUTO aspect does not widen it a second time, but only when the
// camera's aspect (+0xE8) is the view's own aspect, or func_800C54F0's
// derivation of it with zero offsets. chcamerawibble (the underwater sway)
// feeds func_800C54F0 nonzero fov/aspect offsets every frame through
// func_800C4B90 (D_8012A9A0 +0x7C/+0x78), so underwater the view stayed
// widened twice, except for frames where both offsets were exactly zero, which
// snapped to the right width for one game frame.
//
// func_800C4B90 scales the aspect offset by func_80015D14's (widened) aspect,
// so func_800C54F0's result is the view's aspect times a factor and the guest's
// widening factor narrows it exactly. The first hook records what
// func_800C54F0 writes when an offset is nonzero; the second, at the end of
// func_800CB124 (called only by the guest's func_800E44FC), narrows the raster
// copy when the camera still holds that value and every other condition of
// cam_unwiden_raster holds. Frames the guest narrows itself are left alone.

#include <bit>
#include <cmath>
#include <cstdint>

#include "recomp.h"
#include "widescreen.hpp"

namespace {
constexpr int32_t k_widescreen_enabled = int32_t(0x800799B8); // s16
constexpr int32_t k_wide_scale = int32_t(0x80041710);         // f32, func_80015D14
constexpr int32_t k_current_view = int32_t(0x80127674);       // D_80127674
constexpr int32_t k_views = int32_t(0x80127688);              // {viewport, camera, fov}
constexpr int32_t k_aspect_offset = int32_t(0x8012AA18);
constexpr int32_t k_fov_offset = int32_t(0x8012AA1C);
constexpr int k_view_count = 5;

struct Derived {
    bool valid;
    uint32_t camera, base_aspect, base_fov, aspect, fov;
};
Derived derived[k_view_count]{};

uint32_t settings_word() noexcept {
    return tooie::widescreen::latched_settings_word();
}

bool positive_finite(float value) noexcept {
    const uint32_t word = std::bit_cast<uint32_t>(value);
    return !(word & 0x80000000u) && word && word < 0x7F800000u;
}

// func_80015D14 (patches/game/native_aspect_projection.c) for a viewport of
// width x height, with widescreen on and the host settings active.
float host_aspect(uint8_t* rdram, int width, int height, uint32_t settings) noexcept {
    const float wide = std::bit_cast<float>(uint32_t(MEM_W(0, k_wide_scale)));
    volatile float original = float(width) / float(height);
    original = original * wide;
    if (settings & TOOIE_WIDESCREEN_SETTINGS_FIT) {
        const float fit = std::bit_cast<float>(settings & TOOIE_WIDESCREEN_SETTINGS_FIT_SCALE_MASK);
        return float(double(original) * double(fit));
    }
    switch ((settings >> TOOIE_WIDESCREEN_SETTINGS_ASPECT_SHIFT) & TOOIE_WIDESCREEN_SETTINGS_ASPECT_MASK) {
        case 0: return original;
        case 1: return float(double(original) * 0x1.5000000000001p+0);
        case 2: return float(double(original) * 0x1.0000000000000p+1);
        default: return float(double(original) * 0x1.5800000000000p+0);
    }
}
}

// Before func_800C54F0's jal func_800CA510 (0x800C5630): a0 is the camera, a1
// the derived aspect; the frame holds the view (0x40), the view's fov (0x2C)
// and the derived fov (0x24, already in +0xE4); +0xE8 still holds the aspect
// func_80015CE8 set from func_80015D14.
extern "C" void tooie_underwater_aspect_record(uint8_t* rdram, recomp_context* ctx) {
    const int32_t view = int32_t(MEM_W(0x40, ctx->r29));
    if (view < 0 || view >= k_view_count) return;
    const bool offset = (uint32_t(MEM_W(0, k_aspect_offset)) & 0x7FFFFFFFu) ||
                        (uint32_t(MEM_W(0, k_fov_offset)) & 0x7FFFFFFFu);
    const uint32_t camera = uint32_t(ctx->r4);
    derived[view] = Derived{offset, camera, uint32_t(MEM_W(0xE8, int32_t(camera))),
        uint32_t(MEM_W(0x2C, ctx->r29)), uint32_t(ctx->r5), uint32_t(MEM_W(0x24, ctx->r29))};
}

// After func_800CB124 stores the projection (0x800CB310): a2 is the camera and
// the destination matrix is in the frame at 0x3C.
extern "C" void tooie_underwater_aspect_unwiden(uint8_t* rdram, recomp_context* ctx) {
    const int32_t view = int32_t(MEM_W(0, k_current_view));
    if (view < 0 || view >= k_view_count) return;
    const Derived& record = derived[view];
    const uint32_t camera = uint32_t(ctx->r6);
    if (!record.valid || record.camera != camera || MEM_H(0, k_widescreen_enabled) == 0) return;
    const uint32_t settings = settings_word();
    constexpr uint32_t host_width = TOOIE_WIDESCREEN_SETTINGS_ACTIVE | TOOIE_WIDESCREEN_SETTINGS_ENABLED;
    if ((settings & host_width) != host_width) return;
    const int32_t entry = k_views + view * 12;
    if (uint32_t(MEM_W(4, entry)) != camera) return;
    const uint32_t stored = uint32_t(MEM_W(0xE8, int32_t(camera)));
    const uint32_t stored_fov = uint32_t(MEM_W(0xE4, int32_t(camera)));
    if (stored != record.aspect || stored_fov != record.fov) return;
    // cam_unwiden_raster's viewport test: full original width, inside 228 lines.
    const int32_t viewport = int32_t(MEM_W(0, entry));
    const int x = MEM_H(4, viewport), y = MEM_H(6, viewport);
    const int width = MEM_H(8, viewport), height = MEM_H(0xA, viewport);
    if (x != 0 || width != 304 || y < 0 || height <= 0 || height > 228 - y) return;
    const float aspect = host_aspect(rdram, width, height, settings);
    const uint32_t fov_word = uint32_t(MEM_W(8, entry));
    const float fov = std::bit_cast<float>(fov_word);
    if (!positive_finite(aspect) || !positive_finite(fov)) return;
    // Derived from this view's own aspect and fov, not some other value.
    if (record.base_aspect != std::bit_cast<uint32_t>(aspect) || record.base_fov != fov_word) return;
    // The guest narrows these itself.
    if (stored == std::bit_cast<uint32_t>(aspect)) return;
    if (stored_fov == fov_word) {
        volatile float numerator = aspect * fov;
        numerator = numerator + 0.0f;
        volatile float denominator = fov + 0.0f;
        if (stored == std::bit_cast<uint32_t>(float(numerator / denominator))) return;
    }
    const float widening = host_aspect(rdram, 1, 1, settings);
    if (!positive_finite(widening)) return;
    const int32_t matrix = int32_t(MEM_W(0x3C, ctx->r29));
    for (int row = 0; row < 4; row++) {
        const float value = std::bit_cast<float>(uint32_t(MEM_W(row * 16, matrix)));
        MEM_W(row * 16, matrix) = int32_t(std::bit_cast<uint32_t>(value * widening));
    }
}
