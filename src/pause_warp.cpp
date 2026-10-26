// Pause-menu warp. A "WARP" row in gcnewpause's own option list opens the list
// a warp pad shows (chwarppad state 3: the world's pad text block, its title
// and portrait, the pads' names and icons, rows greyed by the pad table's
// transformation masks) with the pads activated in the current world. Picking
// one warps the way a pad does (FLAG3_9FE and gcgoto to the pad's map and
// exit); B returns to the pause list. Raw hooks in minimum_host_hooks.py
// (PAUSE_WARP_HOOKS) reach the functions below; the guest image is unchanged.
//
// The row comes after the variant's own rows, so their indices and the
// option table (D_80802070) stay as they are; the hooks answer the one index
// past the table. What the hooks share lives in k_extra_size bytes past the
// end of the pause allocation, never in host pointers: the game's heap
// defragmentation moves the pause while it is open.

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>

#include "funcs.h"

namespace {
gpr guest(std::uint32_t address) noexcept { return static_cast<gpr>(static_cast<std::int32_t>(address)); }
// As camera_analog.cpp: callee frames go below the hook's stack pointer and
// the whole context is restored, so the hooked function continues unchanged.
template <typename Call>
void guest_call(recomp_context* ctx, Call&& call) {
    const auto saved = *ctx;
    try {
        ctx->r29 -= 0x60;
        call();
        *ctx = saved;
    } catch (...) {
        *ctx = saved;
        throw;
    }
}
using GuestFunction = void (*)(std::uint8_t*, recomp_context*);
// o32: four register arguments, the rest from sp+0x10; returns v0.
gpr call(std::uint8_t* rdram, recomp_context* ctx, GuestFunction function, gpr a0 = 0, gpr a1 = 0, gpr a2 = 0,
    gpr a3 = 0, std::initializer_list<std::uint32_t> stack = {}) {
    gpr result = 0;
    guest_call(ctx, [&] {
        ctx->r4 = a0; ctx->r5 = a1; ctx->r6 = a2; ctx->r7 = a3;
        std::uint32_t offset = 0x10;
        for (const auto word : stack) { MEM_W(offset, ctx->r29) = static_cast<std::int32_t>(word); offset += 4; }
        function(rdram, ctx);
        result = ctx->r2;
    });
    return result;
}
std::uint32_t bits(float value) noexcept { return std::bit_cast<std::uint32_t>(value); }

// PauseState (include/gc/newpause.h), heap_alloc(0x260 + 0x30) plus ours
// (PAUSE_WARP_EXTRA in minimum_host_hooks.py).
constexpr std::uint32_t k_pause_size = 0x290, k_extra_size = 0x40;
constexpr std::uint32_t k_page = 0x0, k_exit_type = 0x2, k_hud = 0x3, k_last_option = 0x5, k_controller = 0xC,
    k_variant = 0xD, k_confirm = 0xE, k_confirm_first = 0x11, k_option_state = 0x30;
constexpr std::uint8_t k_option_page = 2;
// Visible rows an OptionState can draw: one zoombox pointer each at +0x3C.
constexpr std::uint8_t k_max_visible = 5;
// Ours: a marker set by func_80800A08 when the row is offered, the list flag,
// the variant's own row count, the world, the activated pads, the greyed-row
// colour, the row's label and whether Start hid the pad list.
constexpr std::uint32_t k_marker = k_pause_size, k_listing = k_pause_size + 4, k_size = k_pause_size + 5,
    k_world_index = k_pause_size + 6, k_count = k_pause_size + 7, k_pads = k_pause_size + 8,
    k_colour = k_pause_size + 0x10, k_label = k_pause_size + 0x18, k_start = k_pause_size + 0x20;
constexpr std::int32_t k_marker_value = 0x57415250; // "WARP"
constexpr char k_label_text[] = "WARP";
static_assert(k_label + sizeof(k_label_text) <= k_pause_size + k_extra_size);
// OptionState (gcnewoption): +0x23C the loaded text block, +0x244 the callback.
constexpr std::uint32_t k_option_text = 0x23C, k_option_callback = 0x244;
// Text index of the row in the pause block (0x1863 has 0..7); the gldbstring
// hook answers it with the label.
constexpr std::int16_t k_label_index = 0x7E;
// Icon of the row: 0x97, the portrait the pad table gives most worlds' entry and exit pad.
constexpr std::uint16_t k_warp_icon = 0x97;
// The open pause (D_80128AF0); D_80127630 +0x02 (s16): 3 in play, 4 paused (also
// while the pause closes); D_8012762C: the world chwarppad looks up.
constexpr std::uint32_t k_pause_pointer = 0x80128AF0U, k_game_state = 0x80127632U, k_world = 0x8012762CU;
constexpr std::int32_t k_flag_warping = 0x9FE; // FLAG3_9FE_WARPING_VIA_WARPPAD

// D_808010B0_chwarppad, one group per world in order (the group's position is
// the "world" of the pad flags 0x3AB + world * 5 + pad and of the text block
// 0x185A + world). The pad rows' packed ROM offsets are resolved to maps as
// func_80800E10_chwarppad does; exit = 0x1E + pad - 1, icon = row +5, mask =
// row +8 (the transformations that may not use the pad: func_80090178 greys
// the row; at a pad it also keeps the menu shut).
struct Pad { std::uint16_t map; std::uint16_t icon; std::uint32_t mask; };
struct World { std::uint8_t id; std::uint8_t count; std::array<Pad, 5> pads; };
constexpr std::array<World, 9> k_worlds{{
    {0x10, 5, {{{0x0B8, 0x97, 0x2000}, {0x0B8, 0x10, 0x2000}, {0x0B9, 0x32, 0x2000}, {0x0C4, 0x40, 0x2000}, {0x0BB, 0x35, 0x2000}}}}, // MT
    {0x0F, 5, {{{0x0C7, 0x97, 0}, {0x0C7, 0x10, 0}, {0x0E9, 0x40, 0x1000}, {0x0C7, 0x01, 0}, {0x0C7, 0x42, 0}}}},                  // GGM
    {0x12, 5, {{{0x0D6, 0x97, 0}, {0x0D6, 0x52, 0}, {0x0D6, 0x41, 0}, {0x0D6, 0x40, 0}, {0x0E7, 0x10, 0}}}},                       // WW
    {0x14, 5, {{{0x1A7, 0x53, 0x800}, {0x1A8, 0x63, 0x1400}, {0x1A8, 0x64, 0x1400}, {0x1A9, 0x56, 0x1400}, {0x1A9, 0xB4, 0x1400}}}}, // JRL
    {0x15, 5, {{{0x112, 0x97, 0x40000}, {0x11A, 0x58, 0x40000}, {0x112, 0x10, 0x40000}, {0x112, 0x40, 0x40000}, {0x112, 0x57, 0x40000}}}}, // TDL
    {0x16, 5, {{{0x101, 0x8F, 0x40}, {0x106, 0x40, 0x40}, {0x108, 0x10, 0x40}, {0x10B, 0x04, 0x40}, {0x100, 0x0D, 0x40}}}},        // GI
    {0x17, 5, {{{0x127, 0x10, 0}, {0x127, 0x76, 0}, {0x128, 0xA8, 0}, {0x128, 0x40, 0}, {0x132, 0xA7, 0}}}},                       // HFP
    {0x18, 2, {{{0x136, 0x97, 0}, {0x13A, 0x69, 0}}}},                                                                             // CCL
    {0x19, 2, {{{0x15D, 0xAB, 0}, {0x15D, 0x22, 0}}}},                                                                             // CK
}};
// D_80801080_chwarppad: transformation -> the pad block's refusal text
// (func_80800ADC_chwarppad), for a greyed row.
constexpr std::array<std::array<std::uint8_t, 2>, 15> k_refusals{{
    {0x01, 0x1E}, {0x13, 0x24}, {0x12, 0x23}, {0x11, 0x22}, {0x10, 0x23}, {0x0F, 0x23}, {0x0E, 0x24}, {0x0D, 0x21},
    {0x0C, 0x23}, {0x0B, 0x20}, {0x0A, 0x1F}, {0x08, 0x23}, {0x07, 0x23}, {0x02, 0x23}, {0x06, 0x23},
}};
// No warping from boss arenas, minigames (story and replay) or the train:
// maps from include/maps.h.
constexpr std::uint16_t k_blocked_maps[] = {
    // Bosses: Targitzan, Old King Coal, Mr. Patch, Lord Woo Fak Fak, Terry,
    // Weldar, Chilli Billi, Chilly Willy, Mingy Jongo, Hag 1.
    0x17A, 0x0D1, 0x0F9, 0x0FC, 0x113, 0x110, 0x12B, 0x12C, 0x13F, 0x19A,
    // Minigames: Mayan and Colosseum kickball, Ordnance Storage, Canary Mary
    // races, Dodgems, Balloon Burst, Hoop Hurry, Saucer of Peril, Chompa's
    // Belly, Clinker's Cavern, Twinkly packing, Pot o' Gold, Trash Can, Zubbas,
    // Tower of Tragedy quiz.
    0x0C6, 0x0C8, 0x0C9, 0x0CA, 0x0CB, 0x0CC, 0x0CD, 0x0CE, 0x12E, 0x12F, 0x130, 0x0DC, 0x16F, 0x170, 0x161,
    0x0DE, 0x0DF, 0x0E0, 0x0E4, 0x0E5, 0x124, 0x13B, 0x13C, 0x123, 0x162, 0x10A, 0x13D, 0x137, 0x139, 0x15F,
    // Replay and multiplayer copies (MP_*).
    0x164, 0x165, 0x166, 0x17B, 0x17C, 0x17D, 0x17F, 0x180, 0x182, 0x183, 0x185, 0x186, 0x188, 0x189,
    // Chuffy's cab and wagon.
    0x0D0, 0x121,
};

// Guest thread only: the warp chosen in the pause, made on the first player
// update in play after it closes.
struct Pending { bool set = false; std::uint16_t map = 0; std::uint8_t exit = 0; } pending;

bool offered(std::uint8_t* rdram, gpr pause) {
    return MEM_BU(k_page, pause) == k_option_page && MEM_W(k_marker, pause) == k_marker_value;
}
const Pad& pad_of(std::uint8_t* rdram, gpr pause, std::uint8_t row) {
    return k_worlds[MEM_BU(k_world_index, pause)].pads[MEM_BU(k_pads + row, pause) - 1];
}

// The game's rules for using a pad, as far as they apply away from one.
// Writes the world and the activated pads; true when one may be used.
bool collect(std::uint8_t* rdram, recomp_context* ctx, gpr pause) {
    if (MEM_B(k_variant, pause) > 3) return false; // Replay / multiplayer menus.
    const std::uint8_t id = MEM_BU(0, guest(k_world));
    std::size_t world = 0;
    while (world < k_worlds.size() && k_worlds[world].id != id) world++;
    if (world == k_worlds.size()) return false;
    const auto map = static_cast<std::uint16_t>(call(rdram, ctx, func_800EA05C));
    for (const auto blocked : k_blocked_maps) if (map == blocked) return false;
    // subaddiedialog's test for talking to an object, which a pad also needs
    // (func_800F3930 with any form, flag 4): steady footing, not in a
    // cutscene or transition, not Clockwork Kazooie or the Golden Goliath.
    const gpr character = call(rdram, ctx, func_800F54E4);
    if (!call(rdram, ctx, func_800F4B8C, character, guest(0xFFFFFFFFU), 4)) return false;
    std::uint8_t count = 0;
    bool usable = false;
    for (std::uint8_t pad = 1; pad <= k_worlds[world].count; pad++) {
        if (!call(rdram, ctx, func_800DA298, static_cast<gpr>(0x3AB + world * 5 + pad))) continue;
        MEM_BU(k_pads + count++, pause) = pad;
        if (!call(rdram, ctx, func_80090178, static_cast<gpr>(k_worlds[world].pads[pad - 1].mask))) usable = true;
    }
    MEM_BU(k_world_index, pause) = static_cast<std::uint8_t>(world);
    MEM_BU(k_count, pause) = count;
    return usable;
}

void open_list(std::uint8_t* rdram, recomp_context* ctx, gpr pause) {
    const gpr state = pause + k_option_state;
    const gpr callback = MEM_W(k_option_callback, state);
    const std::uint8_t count = MEM_BU(k_count, pause);
    // As func_80800534_gcnewpause leaving the option page, then the pad's
    // menu setup (func_80800418_chwarppad state 3, func_80800124_chwarppad).
    call(rdram, ctx, _gcnewoption_entrypoint_7, state);
    call(rdram, ctx, _bzero, state, 0x260);
    call(rdram, ctx, _gcnewoption_entrypoint_5, state, callback, 0,
        static_cast<gpr>(0x185A + MEM_BU(k_world_index, pause)), {0});
    call(rdram, ctx, _gcnewoption_entrypoint_8, state, count >= 5 ? 4 : count);
    call(rdram, ctx, _gcnewoption_entrypoint_9, state, count);
    call(rdram, ctx, _gcnewoption_entrypoint_10, state, 0);
    call(rdram, ctx, _gcnewoption_entrypoint_32, state, 0x6F);
    for (std::uint8_t row = 0; row < count; row++)
        call(rdram, ctx, _gcnewoption_entrypoint_31, state, row, pad_of(rdram, pause, row).icon);
    call(rdram, ctx, _gcnewoption_entrypoint_14, state, 0, 0, 0, {bits(0.0f)});
    call(rdram, ctx, _gcnewoption_entrypoint_40, state, 0x41, 0x22, 0x20, {bits(0.8f), bits(1.0f)});
    call(rdram, ctx, _gcnewoption_entrypoint_42, state, static_cast<gpr>(static_cast<std::int32_t>(bits(0.1f))));
    call(rdram, ctx, _gcnewoption_entrypoint_46, state, MEM_BU(k_controller, pause));
    call(rdram, ctx, _gcnewoption_entrypoint_11, state);
    // Hiding the pause list cleared the HUD counts flag; they stay away under
    // the list's header, as nothing else is drawn with a pad's list.
    MEM_BU(k_start, pause) = 0;
    MEM_BU(k_listing, pause) = 1;
}

// The pad list's callback events (func_80800B34_chwarppad), in the pause.
void list_event(std::uint8_t* rdram, recomp_context* ctx, gpr pause, std::int32_t index, std::int32_t event) {
    const gpr state = pause + k_option_state;
    const std::uint8_t count = MEM_BU(k_count, pause);
    switch (event) {
    case 0: // Row texts: the pad names, unusable rows greyed (func_808001F4_chwarppad).
        for (std::uint8_t row = 0; row < count; row++) {
            call(rdram, ctx, _gcnewoption_entrypoint_12, state, row, MEM_BU(k_pads + row, pause) + 9);
            if (call(rdram, ctx, func_80090178, static_cast<gpr>(pad_of(rdram, pause, row).mask))) {
                call(rdram, ctx, _gcnewoption_entrypoint_23, state, row);
                call(rdram, ctx, _gcnewoption_entrypoint_13, state, row, pause + k_colour);
            }
        }
        break;
    case 3: // A on a row.
        if (index < 0 || index >= count) break;
        if (call(rdram, ctx, _gcnewoption_entrypoint_19, state, index)) {
            // Greyed: the pad's refusal for this form (func_80800ADC_chwarppad).
            const auto form = static_cast<std::uint8_t>(call(rdram, ctx, func_8008FD48));
            for (const auto& refusal : k_refusals)
                if (refusal[0] == form) { call(rdram, ctx, _gcnewoption_entrypoint_34, state, refusal[1]); break; }
        } else {
            const std::uint8_t pad = MEM_BU(k_pads + index, pause);
            pending = Pending{true, pad_of(rdram, pause, index).map, static_cast<std::uint8_t>(0x1E + pad - 1)};
            std::fprintf(stderr, "player pause warp: world %u pad %u -> map %#x exit %#x\n",
                unsigned(MEM_BU(k_world_index, pause)), unsigned(pad), unsigned(pending.map), unsigned(pending.exit));
            // As Return to Game: close the pause.
            MEM_BU(k_exit_type, pause) = 1;
        }
        break;
    case 2: // The list closed.
        if (MEM_BU(k_start, pause)) {
            // Start: close the pause, which frees the list.
            MEM_BU(k_exit_type, pause) = 1;
        } else {
            // B: back to the pause list with its HUD counts (D_80802072), the
            // cursor on the warp row (func_80800C54 restores it).
            call(rdram, ctx, _gcnewoption_entrypoint_7, state);
            MEM_BU(k_listing, pause) = 0;
            call(rdram, ctx, func_80800A08_gcnewpause, pause);
            MEM_B(k_hud, pause) = 1;
        }
        break;
    }
}
}

// func_80800A08_gcnewpause before gcnewoption_entrypoint_36 (0x80800ADC): s4
// the pause, s3 its OptionState, s2 the variant's row count, already set as
// the visible and total count with their icons. Adds the warp row after them.
extern "C" void tooie_pause_warp_setup(std::uint8_t* rdram, recomp_context* ctx) {
    const gpr pause = ctx->r20, state = ctx->r19;
    const auto size = static_cast<std::uint8_t>(ctx->r18);
    MEM_W(k_marker, pause) = 0;
    MEM_BU(k_listing, pause) = 0;
    if (size == 0 || size > 5 || !collect(rdram, ctx, pause)) return;
    MEM_BU(k_size, pause) = size;
    // D_80801344_chwarppad: the greyed rows' colour.
    MEM_H(0, pause + k_colour) = 0xFF;
    MEM_H(2, pause + k_colour) = 0xFF;
    MEM_H(4, pause + k_colour) = 0;
    for (std::uint32_t i = 0; i < sizeof(k_label_text); i++) MEM_BU(i, pause + k_label) = k_label_text[i];
    MEM_W(k_marker, pause) = k_marker_value;
    // An OptionState has row boxes for k_max_visible rows (+0x3C..+0x4F); the
    // longest pause list (5 rows) plus ours scrolls, as a pad's list does.
    call(rdram, ctx, _gcnewoption_entrypoint_8, state, size + 1 > k_max_visible ? k_max_visible : size + 1);
    call(rdram, ctx, _gcnewoption_entrypoint_9, state, size + 1);
    call(rdram, ctx, _gcnewoption_entrypoint_31, state, size, k_warp_icon);
}

// func_80800C54_gcnewpause after the row texts (0x80800CB8): s3 the OptionState.
extern "C" void tooie_pause_warp_texts(std::uint8_t* rdram, recomp_context* ctx) {
    const gpr state = ctx->r19, pause = state - k_option_state;
    if (MEM_W(k_marker, pause) != k_marker_value) return;
    call(rdram, ctx, _gcnewoption_entrypoint_12, state, MEM_BU(k_size, pause), k_label_index);
}

// gcnewpause_entrypoint_4 (0x808007D8), the pause list's option callback:
// a1 the OptionState, a2 the row, a3 the event. Nonzero when handled here;
// the original reads D_80802070 for the row, which has no entry for ours.
extern "C" int tooie_pause_warp_event(std::uint8_t* rdram, recomp_context* ctx) {
    const gpr state = ctx->r5, pause = state - k_option_state;
    if (!offered(rdram, pause)) return 0;
    const auto index = static_cast<std::int32_t>(ctx->r6), event = static_cast<std::int32_t>(ctx->r7);
    if (MEM_BU(k_listing, pause)) {
        list_event(rdram, ctx, pause, index, event);
        return 1;
    }
    if (index != MEM_BU(k_size, pause)) return 0;
    const std::int8_t confirm = MEM_B(k_confirm, pause);
    switch (event) {
    case 3: // Chosen, once the list hid.
        MEM_BU(k_last_option, pause) = static_cast<std::uint8_t>(index);
        open_list(rdram, ctx, pause);
        return 1;
    case 4: // A: entrypoint_4 case 4 for a row without a confirmation.
        if (call(rdram, ctx, _gcnewoption_entrypoint_37, state, 0)) {
            if (confirm == MEM_B(k_confirm_first, pause)) call(rdram, ctx, _gcaudiolist_entrypoint_0, 6);
            MEM_B(k_hud, pause) = 0;
            MEM_B(k_confirm, pause) = 0;
        }
        return 1;
    case 5: // B: entrypoint_4 case 5, with our label.
        if (confirm == 0) {
            if (call(rdram, ctx, _gcnewoption_entrypoint_37, state, 1)) MEM_B(k_hud, pause) = 0;
        } else if (call(rdram, ctx, _gcnewoption_entrypoint_39, state)) {
            if (confirm == MEM_B(k_confirm_first, pause)) call(rdram, ctx, _gcaudiolist_entrypoint_0, 7);
            MEM_B(k_confirm, pause) = 0;
            call(rdram, ctx, _gcnewoption_entrypoint_12, state, index, k_label_index);
        }
        return 1;
    }
    return 0;
}

// gcnewpause_entrypoint_2 when Start hid the option list (0x80800184, s0 the
// pause): the pad list's close event then closes the pause, as on the pause list.
extern "C" void tooie_pause_warp_start(std::uint8_t* rdram, recomp_context* ctx) {
    const gpr pause = ctx->r16;
    if (offered(rdram, pause) && MEM_BU(k_listing, pause)) MEM_BU(k_start, pause) = 1;
}

// gldbstring_entrypoint_1 (0x8080003C): a0 the text block, a1 the index. The
// warp row's label, which the pause block does not have.
extern "C" int tooie_pause_warp_text(std::uint8_t* rdram, recomp_context* ctx) {
    if (static_cast<std::int16_t>(ctx->r5) != k_label_index) return 0;
    const gpr pause = MEM_W(0, guest(k_pause_pointer));
    if (!pause || !offered(rdram, pause) || MEM_BU(k_listing, pause)) return 0;
    if (ctx->r4 != MEM_W(k_option_text, pause + k_option_state)) return 0;
    ctx->r2 = pause + k_label;
    return 1;
}

// From the bainput_update hook (func_800991B0), a0 the PlayerState: the warp
// chosen in the pause, as chwarppad state 4 makes it, once play resumes.
extern "C" void tooie_pause_warp_update(std::uint8_t* rdram, recomp_context* ctx) {
    if (!pending.set) return;
    const std::int16_t game_state = MEM_H(0, guest(k_game_state));
    if (game_state == 4) return; // The pause is still closing.
    if (game_state != 3) {
        pending = Pending{};
        std::fprintf(stderr, "player pause warp dropped: game state %d after the pause\n", int(game_state));
        return;
    }
    const Pending warp = pending;
    pending = Pending{};
    call(rdram, ctx, func_800DA544, k_flag_warping);
    call(rdram, ctx, _gcgoto_entrypoint_1, warp.map, warp.exit);
}
