// Pause-menu warp and jiggy list.
// - A "WARP" row in gcnewpause's own option list opens the list a warp pad
//   shows (chwarppad state 3: the world's pad text block, its title and
//   portrait, the pads' names and icons, rows greyed by the pad table's
//   transformation masks) with the pads activated in the current world.
//   Picking one warps the way a pad does (FLAG3_9FE and gcgoto to the pad's
//   map and exit). B returns to the pause list.
// - A on a world's View Totals page opens a list of that world's ten jiggies
//   in the same style, the ones not collected yet tinted red, with the count
//   in the header; A on a row shows there what to do for it, or
//   "COLLECTED!". B returns to the same Totals page.
// Raw hooks in minimum_host_hooks.py (PAUSE_WARP_HOOKS) reach the functions
// below; the guest image is unchanged.
//
// The WARP row comes after the variant's own rows, so its index and the
// option table (D_80802070) stay as they are; the hooks answer the index
// past the table. The jiggy list takes over the same option list: leaving
// Totals for it goes to the option page as B does, and the list is built
// in place of the pause rows. What the hooks share lives in k_extra_size bytes past the
// end of the pause allocation, never in host pointers: the game's heap
// defragmentation (func_800C08C0) moves the pause while it is open. The
// texts the hooks hand out live in a separate allocation made with the pause
// (k_strings), which nothing moves: the option list's zoomboxes keep the
// string pointers gldbstring returns.

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string_view>

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
constexpr std::uint32_t k_page = 0x0, k_subpage = 0x1, k_exit_type = 0x2, k_hud = 0x3, k_last_option = 0x5,
    k_controller = 0xC, k_variant = 0xD, k_confirm = 0xE, k_confirm_first = 0x11, k_can_move_left = 0x1E,
    k_can_move_right = 0x1F, k_option_state = 0x30;
constexpr std::uint8_t k_option_page = 2, k_totals_page = 3;
// Totals subpages 0..2 are the items, the Jinjos and the game's totals; the
// worlds follow (D_808020A8_gcnewpause gives each one's world).
constexpr std::uint8_t k_first_world_subpage = 3;
// Visible rows an OptionState can draw: one zoombox pointer each at +0x3C.
constexpr std::uint8_t k_max_visible = 5;
// Ours: a marker set when the WARP row or the jiggy list is offered, the
// open list, the variant's own row count, the warp pads' world and activated
// pads, the greyed-row colours (a pad the form may not use, a jiggy not
// collected), the strings allocation, the WARP row's index (k_no_row when
// not offered), whether Start hid a list, the Totals subpage the jiggy list
// is for (0 when none: A there asks for it, the list returns to it), and the
// listed world, its first jiggy and which of its jiggies are collected.
constexpr std::uint32_t k_marker = k_pause_size, k_listing = k_pause_size + 4, k_size = k_pause_size + 5,
    k_world_index = k_pause_size + 6, k_count = k_pause_size + 7, k_pads = k_pause_size + 8,
    k_colour = k_pause_size + 0x10, k_strings = k_pause_size + 0x18, k_warp_row = k_pause_size + 0x1C,
    k_jiggy_subpage = k_pause_size + 0x1D, k_jiggy_world = k_pause_size + 0x1E, k_start = k_pause_size + 0x20,
    k_first_jiggy = k_pause_size + 0x21, k_collected = k_pause_size + 0x22, k_missing_colour = k_pause_size + 0x24;
static_assert(k_missing_colour + 6 <= k_pause_size + k_extra_size);
constexpr std::int32_t k_marker_value = 0x57415250; // "WARP"
constexpr std::uint8_t k_no_row = 0xFF;
enum Listing : std::uint8_t { k_list_none = 0, k_list_pads = 1, k_list_jiggies = 2 };
// The strings allocation: the WARP label, the jiggy list's header, one name
// and one hint per jiggy row, and the answer to A on a collected one.
constexpr std::uint32_t k_warp_label = 0x0, k_title = 0x10, k_title_size = 0x20,
    k_names = k_title + k_title_size, k_name_size = 0x24;
constexpr std::uint8_t k_jiggies_per_world = 10;
constexpr std::uint32_t k_hints = k_names + k_jiggies_per_world * k_name_size, k_hint_size = 0x40,
    k_collected_text = k_hints + k_jiggies_per_world * k_hint_size, k_strings_size = k_collected_text + 0x10;
// OptionState (gcnewoption): +0x23C the loaded text block, +0x244 the callback.
constexpr std::uint32_t k_option_text = 0x23C, k_option_callback = 0x244;
// Text indices the gldbstring hook answers: the WARP label in the pause
// block (0x1863 has 0..7), and in the jiggy list (which loads that block
// too) its header (0, the header text in that layout), the names, the
// hints and "COLLECTED!".
constexpr std::int16_t k_warp_label_index = 0x7E, k_title_index = 0, k_name_index = 0x80, k_hint_index = 0x90,
    k_collected_index = 0x9F;
constexpr std::int16_t k_pause_block = 0x1863;
// Row icons: 0x97, the portrait the pad table gives most worlds' entry and
// exit pad; 0x01, the jiggy of the pause's View Totals row.
constexpr std::uint16_t k_warp_icon = 0x97, k_jiggy_icon = 0x01;
// The A button's press counter (func_80016B30's slot 0; B is slot 1).
constexpr gpr k_button_a = 0;
// Header portraits: the pad list's (0x6F) and the jiggy list's.
constexpr std::uint16_t k_pad_portrait = 0x6F, k_jiggy_portrait = 0x6F;
// The open pause (D_80128AF0); D_80127630 +0x02 (s16): 3 in play, 4 paused (also
// while the pause closes); D_8012762C: the world chwarppad and the jiggy table use.
constexpr std::uint32_t k_pause_pointer = 0x80128AF0U, k_game_state = 0x80127632U, k_world = 0x8012762CU;
constexpr std::int32_t k_flag_warping = 0x9FE; // FLAG3_9FE_WARPING_VIA_WARPPAD
constexpr std::int32_t k_item_jiggy = 1;       // Item type of the jiggy table (func_800D06D4 and kin).

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
// Jiggy names by JiggyId (include/jiggies.h) - 1: ten per world, in the order
// the game's jiggy table groups them (func_800D06D4 gives each id's world).
// The pause font has capitals only.
constexpr std::array<std::string_view, 90> k_jiggy_names{{
    // Mayahem Temple
    "TARGITZAN", "SLIGHTLY SACRED CHAMBER", "KICKBALL", "BOVINA", "TREASURE CHAMBER", "GOLDEN GOLIATH",
    "PRISON QUICKSAND", "PILLARS", "TOP OF THE TEMPLE", "SSSLUMBER",
    // Glitter Gulch Mine
    "OLD KING COAL", "CANARY MARY", "GENERATOR CAVERN", "WATERFALL CAVERN", "ORDNANCE STORAGE", "DILBERTA",
    "CRUSHING SHED", "WATERFALL", "POWER HUT BASEMENT", "FLOODED CAVES",
    // Witchyworld
    "HOOP HURRY", "DODGEMS", "MR. PATCH", "SAUCER OF PERIL", "BALLOON BURST", "DIVE OF DEATH", "MRS. BOGGY",
    "STAR SPINNER", "THE INFERNO", "CACTUS OF STRENGTH",
    // Jolly Roger's Lagoon
    "MINI-SUB CHALLENGE", "TIPTUP CHOIR", "CHRIS P. BACON", "PIGLETS' POOL", "SMUGGLERS' CAVERN", "MERRY MAGGIE",
    "LORD WOO FAK FAK", "SEEMEE", "PAWNO'S EMPORIUM", "UFO",
    // Terrydactyland
    "UNDER TERRY'S NEST", "DIPPY", "SCROTTY", "TERRY", "OOGLE BOOGLES", "CHOMPA'S BELLY", "TERRY'S KIDS",
    "STOMPING PLAINS", "ROCKNUT TRIBE", "CODE OF THE DINOSAURS",
    // Grunty Industries
    "WASTE DISPOSAL UNDERWATER", "WELDAR", "CLINKER'S CAVERN", "LAUNDRY", "FLOOR 5", "QUALITY CONTROL",
    "GUARDED JIGGY", "TRASH COMPACTOR", "PACKING ROOM", "WASTE DISPOSAL BOX",
    // Hailfire Peaks
    "DRAGON BROTHERS", "INSIDE THE VOLCANO", "SABREMAN", "BOGGY", "ICY SIDE STATION", "OIL DRILL",
    "STOMPING PLAINS", "KICKBALL", "ALIEN", "LAVA WATERFALL",
    // Cloud Cuckooland
    "MINGY JONGO", "MR. FIT", "POT O' GOLD", "CANARY MARY", "ZUBBAS' NEST", "JIGGIUM PLANTS", "CHEESE WEDGE",
    "TRASH CAN", "SUPERSTASH", "JELLY CASTLE",
    // Isle o' Hags (Jinjo Village)
    "WHITE JINJOS", "ORANGE JINJOS", "YELLOW JINJOS", "BROWN JINJOS", "GREEN JINJOS", "RED JINJOS",
    "BLUE JINJOS", "PURPLE JINJOS", "BLACK JINJOS", "KING JINGALING",
}};
// What to do for each jiggy, shown in the header for A on one not collected.
// The game has none; these are ours, two header lines at most (about 22
// characters each).
constexpr std::array<std::string_view, 90> k_jiggy_hints{{
    // Mayahem Temple
    "BEAT TARGITZAN IN HIS REALLY SACRED TEMPLE.", "SEARCH THE SLIGHTLY SACRED CHAMBER.",
    "AS STONY, WIN THE KICKBALL TOURNAMENT.", "SHOOT THE FLIES PESTERING BOVINA.",
    "SOLVE THE PUZZLE IN THE TREASURE CHAMBER.", "GOLDEN GOLIATH, IN JADE SNAKE GROVE.",
    "AS STONY, CROSS THE PRISON QUICKSAND.", "CLIMB THE PILLARS IN THE PRISON COMPOUND.",
    "CLIMB TO THE TOP OF THE MAYAN TEMPLE.", "VISIT SSSLUMBER IN JADE SNAKE GROVE.",
    // Glitter Gulch Mine
    "BEAT OLD KING COAL IN CHUFFY'S BOILER.", "BEAT CANARY MARY IN THE MINE CART RACE.",
    "SEARCH THE GENERATOR CAVERN.", "SEARCH THE WATERFALL CAVERN.",
    "SHOOT ALL THE TNT IN THE ORDNANCE STORAGE.", "FREE DILBERTA FROM THE BOULDER.",
    "GET THROUGH THE CRUSHING SHED.", "LOOK IN THE WATERFALL.", "SEARCH THE POWER HUT BASEMENT.",
    "SWIM THROUGH THE FLOODED CAVES.",
    // Witchyworld
    "WIN HOOP HURRY.", "WIN AT THE DODGEMS.", "BEAT MR. PATCH IN THE BIG TOP.", "WIN SAUCER OF PERIL.",
    "WIN BALLOON BURST.", "SURVIVE THE DIVE OF DEATH.", "HELP MRS. BOGGY AND HER KIDS.",
    "GET THE STAR SPINNER GOING.", "SEARCH THE INFERNO.", "RING THE BELL ON THE CACTUS OF STRENGTH.",
    // Jolly Roger's Lagoon
    "WIN THE MINI-SUB CHALLENGE.", "HELP TIPTUP'S CHOIR.", "HELP CHRIS P. BACON UNDERWATER.",
    "SEARCH THE PIGLETS' POOL.", "SEARCH THE SMUGGLERS' CAVERN.", "SEE MERRY MAGGIE MALPASS.",
    "BEAT LORD WOO FAK FAK.", "HELP SEEMEE, DOWN IN THE DEEP.", "BUY IT FROM PAWNO'S EMPORIUM.",
    "HELP THE ALIENS WITH THEIR UFO.",
    // Terrydactyland
    "LOOK UNDER TERRY'S NEST.", "GIVE DIPPY SOME WATER.", "HELP SCROTTY AND HER KIDS.",
    "BEAT TERRY, THE GIANT PTERODACTYL.", "HELP THE OOGLE BOOGLE TRIBE.", "SURVIVE CHOMPA'S BELLY.",
    "RETURN TERRY'S EGGS SO HIS KIDS HATCH.", "SEARCH STOMPING PLAINS.", "HELP THE ROCKNUT TRIBE.",
    "CRACK THE CODE OF THE DINOSAURS.",
    // Grunty Industries
    "DIVE IN THE WASTE DISPOSAL PLANT.", "BEAT WELDAR.", "WIN IN CLINKER'S CAVERN.",
    "SEARCH THE LAUNDRY.", "SEARCH FLOOR 5.", "GET PAST QUALITY CONTROL.",
    "GET THE GUARDED JIGGY ON FLOOR 1.", "SEARCH THE TRASH COMPACTOR.",
    "PACK THE TWINKLIES IN THE PACKING ROOM.", "OPEN THE BOX IN THE WASTE DISPOSAL PLANT.",
    // Hailfire Peaks
    "BEAT CHILLI BILLI AND CHILLY WILLY.", "SEARCH INSIDE THE VOLCANO.",
    "HELP SABREMAN.", "WARM UP BOGGY.", "SEARCH THE ICY SIDE TRAIN STATION.", "SEARCH THE OIL DRILL.",
    "WHERE HAILFIRE PEAKS MEETS STOMPING PLAINS.", "WIN COLOSSEUM KICKBALL.", "HELP THE ALIEN FAMILY.",
    "SEARCH THE LAVA WATERFALL.",
    // Cloud Cuckooland
    "BEAT MINGY JONGO.", "BEAT MR. FIT'S EVENTS.", "WIN POT O' GOLD.", "BEAT CANARY MARY IN THE RACE.",
    "WIN ZUBBAS' NEST.", "GROW THE JIGGIUM PLANTS.", "GET INSIDE THE CHEESE WEDGE.", "WIN THE TRASH CAN GAME.",
    "OPEN THE SUPERSTASH.", "SEARCH THE JELLY CASTLE.",
    // Isle o' Hags
    "RESCUE THE WHITE JINJO.", "RESCUE THE 2 ORANGE JINJOS.", "RESCUE THE 3 YELLOW JINJOS.",
    "RESCUE THE 4 BROWN JINJOS.", "RESCUE THE 5 GREEN JINJOS.", "RESCUE THE 6 RED JINJOS.",
    "RESCUE THE 7 BLUE JINJOS.", "RESCUE THE 8 PURPLE JINJOS.", "RESCUE THE 9 BLACK JINJOS.",
    "TALK TO KING JINGALING.",
}};
constexpr bool texts_fit() {
    for (const auto name : k_jiggy_names) if (name.size() >= k_name_size) return false;
    for (const auto hint : k_jiggy_hints) if (hint.size() >= k_hint_size) return false;
    return true;
}
static_assert(texts_fit());

// Guest thread only: the warp chosen in the pause, made on the first player
// update in play after it closes.
struct Pending { bool set = false; std::uint16_t map = 0; std::uint8_t exit = 0; } pending;

bool offered(std::uint8_t* rdram, gpr pause) {
    return MEM_BU(k_page, pause) == k_option_page && MEM_W(k_marker, pause) == k_marker_value;
}
const Pad& pad_of(std::uint8_t* rdram, gpr pause, std::uint8_t row) {
    return k_worlds[MEM_BU(k_world_index, pause)].pads[MEM_BU(k_pads + row, pause) - 1];
}
void write_string(std::uint8_t* rdram, gpr address, std::string_view text) {
    for (std::uint32_t i = 0; i < text.size(); i++) MEM_BU(i, address) = static_cast<std::uint8_t>(text[i]);
    MEM_BU(text.size(), address) = 0;
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

// A world's first jiggy (world ids as D_8012762C), found as gccollectDll's
// func_80800000_gccollectDll does (each world has ten in a row); 0 when the
// world has none (Spiral Mountain, Cauldron Keep).
std::uint8_t first_jiggy(std::uint8_t* rdram, recomp_context* ctx, std::uint8_t world) {
    const auto last = static_cast<std::int32_t>(call(rdram, ctx, func_800D0894, k_item_jiggy));
    for (auto id = static_cast<std::int32_t>(call(rdram, ctx, func_800D0820, k_item_jiggy)); id <= last; id++) {
        if (static_cast<std::uint8_t>(call(rdram, ctx, func_800D06D4, id, k_item_jiggy)) != world) continue;
        if (id + k_jiggies_per_world - 1 > static_cast<std::int32_t>(k_jiggy_names.size())) return 0;
        return static_cast<std::uint8_t>(id);
    }
    return 0;
}

// Rebuilds the pause's own OptionState as a list with a header (mode 0), as
// func_80800534_gcnewpause leaving the option page, then the pad's menu setup
// (func_80800418_chwarppad state 3, func_80800124_chwarppad).
void open_menu(std::uint8_t* rdram, recomp_context* ctx, gpr pause, Listing listing, std::int16_t block,
    std::uint8_t count, std::uint16_t portrait, std::initializer_list<std::uint16_t> icons) {
    const gpr state = pause + k_option_state;
    const gpr callback = MEM_W(k_option_callback, state);
    call(rdram, ctx, _gcnewoption_entrypoint_7, state);
    call(rdram, ctx, _bzero, state, 0x260);
    call(rdram, ctx, _gcnewoption_entrypoint_5, state, callback, 0, static_cast<gpr>(block), {0});
    call(rdram, ctx, _gcnewoption_entrypoint_8, state, count >= 5 ? 4 : count);
    call(rdram, ctx, _gcnewoption_entrypoint_9, state, count);
    call(rdram, ctx, _gcnewoption_entrypoint_10, state, 0);
    call(rdram, ctx, _gcnewoption_entrypoint_32, state, portrait);
    std::uint8_t row = 0;
    for (const auto icon : icons) {
        if (row == count) break;
        call(rdram, ctx, _gcnewoption_entrypoint_31, state, row++, icon);
    }
    call(rdram, ctx, _gcnewoption_entrypoint_14, state, 0, 0, 0, {bits(0.0f)});
    call(rdram, ctx, _gcnewoption_entrypoint_40, state, 0x41, 0x22, 0x20, {bits(0.8f), bits(1.0f)});
    call(rdram, ctx, _gcnewoption_entrypoint_42, state, static_cast<gpr>(static_cast<std::int32_t>(bits(0.1f))));
    call(rdram, ctx, _gcnewoption_entrypoint_46, state, MEM_BU(k_controller, pause));
    call(rdram, ctx, _gcnewoption_entrypoint_11, state);
    // Hiding the pause list cleared the HUD counts flag; they stay away under
    // the list's header, as nothing else is drawn with a pad's list.
    MEM_BU(k_start, pause) = 0;
    MEM_BU(k_listing, pause) = listing;
}

void open_pads(std::uint8_t* rdram, recomp_context* ctx, gpr pause) {
    const std::uint8_t count = MEM_BU(k_count, pause);
    std::array<std::uint16_t, 5> icons{};
    for (std::uint8_t row = 0; row < count; row++) icons[row] = pad_of(rdram, pause, row).icon;
    // The pad's list: the world's pad text block, its title (string 0) and portrait.
    open_menu(rdram, ctx, pause, k_list_pads, static_cast<std::int16_t>(0x185A + MEM_BU(k_world_index, pause)),
        count, k_pad_portrait, {icons[0], icons[1], icons[2], icons[3], icons[4]});
}

void open_jiggies(std::uint8_t* rdram, recomp_context* ctx, gpr pause) {
    const gpr strings = MEM_W(k_strings, pause);
    const std::uint8_t first = MEM_BU(k_first_jiggy, pause);
    std::uint16_t collected = 0;
    unsigned got = 0;
    for (std::uint8_t row = 0; row < k_jiggies_per_world; row++) {
        const std::uint8_t id = first + row;
        write_string(rdram, strings + k_names + row * k_name_size, k_jiggy_names[id - 1]);
        write_string(rdram, strings + k_hints + row * k_hint_size, k_jiggy_hints[id - 1]);
        // The save flag itself (func_800D0A80 gives it): func_800D0B68 also
        // answers "collected" during some cutscenes and with a cheat.
        const gpr flag = call(rdram, ctx, func_800D0A80, id, k_item_jiggy);
        if (call(rdram, ctx, func_800DA298, flag)) { collected |= 1U << row; got++; }
    }
    MEM_HU(k_collected, pause) = collected;
    char title[k_title_size];
    std::snprintf(title, sizeof(title), "%u OF %u JIGGIES FOUND", got, unsigned(k_jiggies_per_world));
    write_string(rdram, strings + k_title, title);
    // The game's own count, as the Totals page shows it for this world.
    const std::uint8_t world = MEM_BU(k_jiggy_world, pause);
    const auto totals = static_cast<unsigned>(call(rdram, ctx, func_800D0018, k_item_jiggy, world));
    std::fprintf(stderr, "player pause jiggies: world %#x ids %u..%u collected 0x%03x (%u of %u, totals %u)\n",
        unsigned(world), unsigned(first), unsigned(first + k_jiggies_per_world - 1), unsigned(collected), got,
        unsigned(k_jiggies_per_world), totals);
    // The pause block is loaded only so that the gldbstring hook knows the
    // list; it answers the header and the names.
    const auto icon = k_jiggy_icon;
    open_menu(rdram, ctx, pause, k_list_jiggies, k_pause_block, k_jiggies_per_world, k_jiggy_portrait,
        {icon, icon, icon, icon, icon, icon, icon, icon, icon, icon});
}

// The list's B and Start (its close event, the option update's last act):
// Start closes the pause, which frees the list. B from the jiggy list
// returns to its Totals page as View Totals opens one (func_80800CE4: the
// subpage, then the page change, which frees the list); B from the pad list
// returns to the pause list with its HUD counts (D_80802072), the cursor on
// WARP (func_80800C54 restores it).
void close_list(std::uint8_t* rdram, recomp_context* ctx, gpr pause) {
    const gpr state = pause + k_option_state;
    if (MEM_BU(k_start, pause)) {
        MEM_BU(k_exit_type, pause) = 1;
        return;
    }
    if (MEM_BU(k_listing, pause) == k_list_jiggies) {
        MEM_BU(k_listing, pause) = k_list_none;
        const std::uint8_t subpage = MEM_BU(k_jiggy_subpage, pause);
        MEM_BU(k_jiggy_subpage, pause) = 0;
        call(rdram, ctx, func_808017C8_gcnewpause, pause, subpage);
        call(rdram, ctx, func_80800534_gcnewpause, pause, k_totals_page);
        return;
    }
    call(rdram, ctx, _gcnewoption_entrypoint_7, state);
    MEM_BU(k_listing, pause) = k_list_none;
    call(rdram, ctx, func_80800A08_gcnewpause, pause);
    MEM_B(k_hud, pause) = 1;
}

// The pad list's callback events (func_80800B34_chwarppad), in the pause.
void pad_event(std::uint8_t* rdram, recomp_context* ctx, gpr pause, std::int32_t index, std::int32_t event) {
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
        close_list(rdram, ctx, pause);
        break;
    }
}

// The jiggy list's events: the names, the ones not collected tinted red (the
// list already draws the rows away from the cursor grey). Every row is
// marked unusable (gcnewoption_entrypoint_23 only sets a flag), so A shows
// the hint or "COLLECTED!" in the header as a pad's refusal does and the list stays; on a
// usable row the list would hide and wait for its owner to act (it froze
// the pause on the Deck).
void jiggy_event(std::uint8_t* rdram, recomp_context* ctx, gpr pause, std::int32_t index, std::int32_t event) {
    const gpr state = pause + k_option_state;
    const std::uint16_t collected = MEM_HU(k_collected, pause);
    switch (event) {
    case 0:
        for (std::uint8_t row = 0; row < k_jiggies_per_world; row++) {
            call(rdram, ctx, _gcnewoption_entrypoint_12, state, row, k_name_index + row);
            call(rdram, ctx, _gcnewoption_entrypoint_23, state, row);
            if (!(collected & (1U << row)))
                call(rdram, ctx, _gcnewoption_entrypoint_13, state, row, pause + k_missing_colour);
        }
        break;
    case 3: // A on a row.
        if (index < 0 || index >= k_jiggies_per_world) break;
        call(rdram, ctx, _gcnewoption_entrypoint_34, state,
            (collected >> index) & 1 ? k_collected_index : k_hint_index + index);
        break;
    case 2:
        close_list(rdram, ctx, pause);
        break;
    }
}

// The world of a Totals subpage with jiggies (func_808016A4_gcnewpause), or 0.
std::uint8_t jiggy_world_of(std::uint8_t* rdram, recomp_context* ctx, std::uint8_t subpage) {
    if (subpage < k_first_world_subpage) return 0;
    const auto world = static_cast<std::uint8_t>(call(rdram, ctx, func_808016A4_gcnewpause, subpage));
    return world && first_jiggy(rdram, ctx, world) ? world : 0;
}
}

// gcnewpause_entrypoint_0 after bzero (0x80800028): the new pause at sp+0x1C.
// Our bytes past the pause, the colours and the strings allocation.
extern "C" void tooie_pause_warp_open(std::uint8_t* rdram, recomp_context* ctx) {
    const gpr pause = MEM_W(0x1C, ctx->r29);
    for (std::uint32_t i = 0; i < k_extra_size; i++) MEM_BU(k_pause_size + i, pause) = 0;
    MEM_BU(k_warp_row, pause) = k_no_row;
    // D_80801344_chwarppad: the greyed rows' colour; red for missing jiggies.
    MEM_H(0, pause + k_colour) = 0xFF;
    MEM_H(2, pause + k_colour) = 0xFF;
    MEM_H(4, pause + k_colour) = 0;
    MEM_H(0, pause + k_missing_colour) = 0xFF;
    MEM_H(2, pause + k_missing_colour) = 0x50;
    MEM_H(4, pause + k_missing_colour) = 0x50;
    const gpr strings = call(rdram, ctx, heap_alloc, k_strings_size);
    MEM_W(k_strings, pause) = strings;
    if (!strings) return;
    write_string(rdram, strings + k_warp_label, "WARP");
    write_string(rdram, strings + k_collected_text, "COLLECTED!");
}

// gcnewpause_entrypoint_1 at entry (0x808000D4), a0 the pause being freed.
extern "C" void tooie_pause_warp_close(std::uint8_t* rdram, recomp_context* ctx) {
    const gpr pause = ctx->r4;
    const gpr strings = MEM_W(k_strings, pause);
    MEM_W(k_strings, pause) = 0;
    MEM_W(k_marker, pause) = 0;
    if (strings) call(rdram, ctx, heap_free, strings);
}

// func_80800A08_gcnewpause before gcnewoption_entrypoint_36 (0x80800ADC): s4
// the pause, s3 its OptionState, s2 the variant's row count, already set as
// the visible and total count with their icons. Adds the WARP row after them.
extern "C" void tooie_pause_warp_setup(std::uint8_t* rdram, recomp_context* ctx) {
    const gpr pause = ctx->r20, state = ctx->r19;
    const auto size = static_cast<std::uint8_t>(ctx->r18);
    MEM_W(k_marker, pause) = 0;
    MEM_BU(k_listing, pause) = k_list_none;
    MEM_BU(k_warp_row, pause) = k_no_row;
    if (size == 0 || size > 5 || !MEM_W(k_strings, pause) || MEM_B(k_variant, pause) > 3) return;
    if (!collect(rdram, ctx, pause)) return;
    const std::uint8_t rows = size + 1;
    MEM_BU(k_warp_row, pause) = size;
    MEM_BU(k_size, pause) = size;
    MEM_W(k_marker, pause) = k_marker_value;
    // An OptionState has row boxes for k_max_visible rows (+0x3C..+0x4F);
    // longer lists scroll, as a pad's list does.
    call(rdram, ctx, _gcnewoption_entrypoint_8, state, rows > k_max_visible ? k_max_visible : rows);
    call(rdram, ctx, _gcnewoption_entrypoint_9, state, rows);
    call(rdram, ctx, _gcnewoption_entrypoint_31, state, size, k_warp_icon);
}

// func_80800C54_gcnewpause after the row texts (0x80800CB8): s3 the OptionState.
extern "C" void tooie_pause_warp_texts(std::uint8_t* rdram, recomp_context* ctx) {
    const gpr state = ctx->r19, pause = state - k_option_state;
    if (MEM_W(k_marker, pause) != k_marker_value || MEM_BU(k_warp_row, pause) == k_no_row) return;
    call(rdram, ctx, _gcnewoption_entrypoint_12, state, MEM_BU(k_warp_row, pause), k_warp_label_index);
}

// gcnewpause_entrypoint_2 on a Totals page waiting for input, before it reads
// Start (0x80800224, s0 the pause): A on a world with jiggies leaves the page
// as B does (func_808015F8_gcnewpause(pause, 0): its sound, the page sliding
// away, then the option page) and asks for that world's jiggy list. The
// arrows slide away with it, and no stick flip can start in the same frame.
// Otherwise a request left over from a frame where the stick won is dropped.
extern "C" void tooie_pause_jiggies_totals(std::uint8_t* rdram, recomp_context* ctx) {
    const gpr pause = ctx->r16;
    MEM_BU(k_jiggy_subpage, pause) = 0;
    if (!MEM_W(k_strings, pause) || MEM_B(k_variant, pause) > 3) return;
    if (call(rdram, ctx, func_80016B30, MEM_BU(k_controller, pause), k_button_a) != 1) return;
    const std::uint8_t subpage = MEM_BU(k_subpage, pause);
    if (!jiggy_world_of(rdram, ctx, subpage)) return;
    MEM_BU(k_jiggy_subpage, pause) = subpage;
    MEM_BU(k_can_move_left, pause) = 0;
    MEM_BU(k_can_move_right, pause) = 0;
    call(rdram, ctx, func_808015F8_gcnewpause, pause, 0);
}

// gcnewpause_entrypoint_2 after a Totals page went back to the option page
// (0x8080042C, s0 the pause): for an A there, the pause list just built
// becomes the jiggy list of the page's world.
extern "C" void tooie_pause_jiggies_open(std::uint8_t* rdram, recomp_context* ctx) {
    const gpr pause = ctx->r16;
    const std::uint8_t subpage = MEM_BU(k_jiggy_subpage, pause);
    if (!subpage || MEM_BU(k_page, pause) != k_option_page) return;
    const std::uint8_t world = jiggy_world_of(rdram, ctx, subpage);
    if (!world) {
        MEM_BU(k_jiggy_subpage, pause) = 0;
        return;
    }
    MEM_BU(k_jiggy_world, pause) = world;
    MEM_BU(k_first_jiggy, pause) = first_jiggy(rdram, ctx, world);
    // The list is ours now (offered() and the event and text hooks); no HUD
    // counts under its header (func_80800534 just asked for them).
    MEM_W(k_marker, pause) = k_marker_value;
    MEM_BU(k_warp_row, pause) = k_no_row;
    MEM_B(k_hud, pause) = 0;
    open_jiggies(rdram, ctx, pause);
}

// gcnewpause_entrypoint_4 (0x808007D8), the pause list's option callback:
// a1 the OptionState, a2 the row, a3 the event. Nonzero when handled here;
// the original reads D_80802070 for the row, which has no entry for ours.
extern "C" int tooie_pause_warp_event(std::uint8_t* rdram, recomp_context* ctx) {
    const gpr state = ctx->r5, pause = state - k_option_state;
    if (!offered(rdram, pause)) return 0;
    const auto index = static_cast<std::int32_t>(ctx->r6), event = static_cast<std::int32_t>(ctx->r7);
    switch (MEM_BU(k_listing, pause)) {
    case k_list_pads: pad_event(rdram, ctx, pause, index, event); return 1;
    case k_list_jiggies: jiggy_event(rdram, ctx, pause, index, event); return 1;
    }
    if (index != MEM_BU(k_warp_row, pause)) return 0;
    const std::int16_t label = k_warp_label_index;
    const std::int8_t confirm = MEM_B(k_confirm, pause);
    switch (event) {
    case 3: // Chosen, once the list hid.
        MEM_BU(k_last_option, pause) = static_cast<std::uint8_t>(index);
        open_pads(rdram, ctx, pause);
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
            call(rdram, ctx, _gcnewoption_entrypoint_12, state, index, label);
        }
        return 1;
    }
    return 0;
}

// gcnewpause_entrypoint_2 when Start hid the option list (0x80800184, s0 the
// pause): the open list's close event then closes the pause, as on the pause list.
extern "C" void tooie_pause_warp_start(std::uint8_t* rdram, recomp_context* ctx) {
    const gpr pause = ctx->r16;
    if (offered(rdram, pause) && MEM_BU(k_listing, pause) != k_list_none) MEM_BU(k_start, pause) = 1;
}

// gldbstring_entrypoint_1 (0x8080003C): a0 the text block, a1 the index. The
// texts the pause block does not have: the WARP label, and the jiggy list's
// header, names and answers.
extern "C" int tooie_pause_warp_text(std::uint8_t* rdram, recomp_context* ctx) {
    const gpr pause = MEM_W(0, guest(k_pause_pointer));
    if (!pause || !offered(rdram, pause)) return 0;
    if (ctx->r4 != MEM_W(k_option_text, pause + k_option_state)) return 0;
    const gpr strings = MEM_W(k_strings, pause);
    const auto index = static_cast<std::int16_t>(ctx->r5);
    gpr text = 0;
    switch (MEM_BU(k_listing, pause)) {
    case k_list_none:
        if (index == k_warp_label_index) text = strings + k_warp_label;
        break;
    case k_list_jiggies:
        if (index == k_title_index) text = strings + k_title;
        else if (index >= k_name_index && index < k_name_index + k_jiggies_per_world)
            text = strings + k_names + (index - k_name_index) * k_name_size;
        else if (index >= k_hint_index && index < k_hint_index + k_jiggies_per_world)
            text = strings + k_hints + (index - k_hint_index) * k_hint_size;
        else if (index == k_collected_index) text = strings + k_collected_text;
        break;
    }
    if (!text) return 0;
    ctx->r2 = text;
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
