#include <cstdint>

#include "recomp.h"
#define F3DEX_GBI_2
#include "rt64_extended_gbi.h"

// Matrix groups for the game select map's models (0x158). Its picture frames
// and their contents are ordinary actors with no group, so RT64's automatic
// matching paired them across frames by draw-call hash and order; several share
// a model, which mismatched them and made them jitter above the original rate.
// Raw hooks around the template draw call in func_8010A79C (the actor draw
// dispatcher) give each actor there its own group, keyed by descriptor selector
// and slot. Other maps are untouched; gameplay actors keep their existing owners.
namespace {
constexpr std::uint32_t k_current_map = 0x80132DC2U;      // u16, as func_800EA05C returns.
constexpr std::uint32_t k_game_select = 0x158U;
constexpr std::uint32_t k_descriptors = 0x80132ED0U;      // D_80132ED0[200], 0x30 bytes each.
constexpr std::uint32_t k_descriptor_count = 200U;
constexpr std::uint32_t k_hand_selector = 0x862U;         // Game select hand, grouped by its own owner.
constexpr std::uint32_t k_gfx_cursor = 0x80127658U;       // GraphicsBuffers.gfx (func_800A7180).
constexpr std::uint32_t k_gfx_pools = 0x8007BCD0U;        // D_8007BCD0[2] pool bases.
constexpr std::uint32_t k_gfx_override = 0x8007BCE8U;     // D_8007BCE8[0], nonzero redirects output.
constexpr std::uint32_t k_gfx_pool_index = 0x8007BEDAU;   // D_8007BEDA, s16.
constexpr std::uint32_t k_gfx_pool_bytes = 0x8FC0U;
// Leave the game most of its pool: the groups here use at most 1 KiB of it.
constexpr std::uint32_t k_budget_bytes = 1024U;
constexpr std::uint32_t k_open_bytes = 3U * sizeof(GfxCommand);
constexpr std::uint32_t k_close_bytes = sizeof(GfxCommand);
// Group ids 0xD1000000..0xD10FFFFF; the guest owners use 0x665xxxxx and 0x90000000..0xD0000000.
constexpr std::uint32_t k_id_base = 0xD1000000U;
constexpr int k_max_depth = 8;

struct Frame { std::uint32_t pool = 0, cursor = 0, used = 0; };
Frame frame;
bool admitted[k_max_depth]{};
int depth = 0;

gpr address(std::uint32_t value) { return static_cast<gpr>(static_cast<std::int32_t>(value)); }
std::uint32_t word(std::uint8_t* rdram, std::uint32_t at) { return static_cast<std::uint32_t>(MEM_W(0, address(at))); }

std::uint32_t group_id(std::uint8_t* rdram, std::uint32_t actor) {
    if (static_cast<std::uint16_t>(MEM_H(0, address(k_current_map))) != k_game_select) return 0;
    const std::uint32_t descriptor = word(rdram, actor);
    const std::uint32_t offset = descriptor - k_descriptors;
    if (descriptor < k_descriptors || offset >= k_descriptor_count * 0x30U || offset % 0x30U) return 0;
    const std::uint32_t selector = static_cast<std::uint16_t>(MEM_H(0x14, address(descriptor)));
    if (selector == 0 || selector > 0xFFFU || selector == k_hand_selector) return 0;
    return k_id_base | (selector << 8) | (offset / 0x30U);
}

// The pool in use and the cursor inside it, or 0 when the game is drawing elsewhere.
// The pools sit in the heap's expanded RDRAM, above the original 8 MiB.
std::uint32_t pool_cursor(std::uint8_t* rdram, std::uint32_t bytes) {
    if (word(rdram, k_gfx_override) != 0) return 0;
    const auto index = static_cast<std::int16_t>(MEM_H(0, address(k_gfx_pool_index)));
    if (index < 0 || index > 1) return 0;
    const std::uint32_t base = word(rdram, k_gfx_pools + 4U * std::uint32_t(index));
    const std::uint32_t cursor = word(rdram, k_gfx_cursor);
    if ((base & 7U) || (cursor & 7U) || base < 0x80000000U || base > 0xFFFFFFFFU - k_gfx_pool_bytes ||
        cursor < base || cursor - base > k_gfx_pool_bytes - bytes) return 0;
    // A new frame starts over at its pool's base, whichever pool it uses.
    if (frame.pool != base || cursor < frame.cursor) frame = {base, cursor, 0};
    frame.cursor = cursor;
    return frame.used + bytes <= k_budget_bytes ? cursor : 0;
}

void append(std::uint8_t* rdram, std::uint32_t cursor, const GfxCommand* commands, std::uint32_t count) {
    for (std::uint32_t i = 0; i < count; ++i) {
        MEM_W(0, address(cursor + 8U * i)) = static_cast<std::int32_t>(commands[i].values.word0);
        MEM_W(4, address(cursor + 8U * i)) = static_cast<std::int32_t>(commands[i].values.word1);
    }
    MEM_W(0, address(k_gfx_cursor)) = static_cast<std::int32_t>(cursor + 8U * count);
    frame.used += 8U * count;
}
}

// Before the template draw call (0x8010A7E8): a0 is the actor.
extern "C" void tooie_menu_group_begin(std::uint8_t* rdram, recomp_context* ctx) {
    const int level = depth++;
    if (level >= k_max_depth) return;
    admitted[level] = false;
    const std::uint32_t id = group_id(rdram, static_cast<std::uint32_t>(ctx->r4));
    if (!id) return;
    // Room for this group and its pop, reserved together.
    const std::uint32_t cursor = pool_cursor(rdram, k_open_bytes + k_close_bytes);
    if (!cursor) return;
    GfxCommand commands[3];
    gEXEnable(&commands[0]);
    gEXMatrixGroupDecomposed(&commands[1], id, G_EX_PUSH, 0,
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP,
        G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP,
        G_EX_ORDER_LINEAR, G_EX_EDIT_NONE,
        G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP);
    append(rdram, cursor, commands, 3U);
    frame.used += k_close_bytes; // The pop below is already paid for.
    admitted[level] = true;
}

// After the template draw returns (0x8010A7F0).
extern "C" void tooie_menu_group_end(std::uint8_t* rdram, recomp_context*) {
    if (depth == 0) return;
    const int level = --depth;
    if (level >= k_max_depth || !admitted[level]) return;
    admitted[level] = false;
    frame.used -= k_close_bytes;
    const std::uint32_t cursor = pool_cursor(rdram, k_close_bytes);
    GfxCommand pop;
    gEXPopMatrixGroup(&pop, 0);
    // Without room the group stays open until RT64 resets its group stack at the
    // next display list; later draws in this one would share its id.
    if (cursor) append(rdram, cursor, &pop, 1U);
}
