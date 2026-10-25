#include "map_names.hpp"

#include <algorithm>
#include <iterator>

namespace {
struct Entry { std::uint16_t map; const char* name; };

// Ids and areas from deps/banjo-tooie/include/maps.h, written as the game names
// them. Sorted by id. A sub-area names its world first (ASCII: the overlay font has no dash).
// maps.h calls 0x171 GGM_MUMBO; leaving it lands in Terrydactyland (0xD9 is Glitter Gulch Mine's).
#define MT "Mayahem Temple - "
#define GGM "Glitter Gulch Mine - "
#define WW "Witchyworld - "
#define JRL "Jolly Roger's Lagoon - "
#define GI "Grunty Industries - "
#define TDL "Terrydactyland - "
#define HFP "Hailfire Peaks - "
#define CCL "Cloud Cuckooland - "
#define SM "Spiral Mountain - "
#define JV "Jinjo Village - "
#define IOH "Isle o' Hags - "
#define CK "Cauldron Keep - "
constexpr Entry k_names[] = {
    {0x0AD, SM "Grunty's Lair"},
    {0x0AE, SM "Behind the Waterfall"},
    {0x0AF, "Spiral Mountain"},
    {0x0B6, MT "Wumba's Wigwam"},
    {0x0B7, MT "Mumbo's Skull"},
    {0x0B8, "Mayahem Temple"},
    {0x0B9, MT "Prison Compound"},
    {0x0BA, MT "Columns Vault"},
    {0x0BB, MT "Mayan Kickball Stadium Lobby"},
    {0x0BC, MT "Code Chamber"},
    {0x0C4, MT "Jade Snake Grove"},
    {0x0C5, MT "Treasure Chamber"},
    {0x0C6, MT "Mayan Kickball Stadium"},
    {0x0C7, "Glitter Gulch Mine"},
    {0x0C8, MT "Mayan Kickball Stadium"},
    {0x0C9, MT "Mayan Kickball Stadium"},
    {0x0CA, MT "Mayan Kickball Stadium"},
    {0x0CB, MT "Mayan Kickball Stadium"},
    {0x0CC, MT "Mayan Kickball Stadium"},
    {0x0CD, MT "Mayan Kickball Stadium"},
    {0x0CE, MT "Mayan Kickball Stadium"},
    {0x0CF, GGM "Power Hut Basement"},
    {0x0D0, "Chuffy's Cab"},
    {0x0D1, "Inside Chuffy's Boiler"},
    {0x0D2, GGM "Gloomy Caverns"},
    {0x0D3, GGM "Generator Cavern"},
    {0x0D4, GGM "Power Hut"},
    {0x0D5, WW "Wumba's Wigwam"},
    {0x0D6, "Witchyworld"},
    {0x0D7, GGM "Train Station"},
    {0x0D8, GGM "Prospector's Hut"},
    {0x0D9, GGM "Mumbo's Skull"},
    {0x0DA, GGM "Toxic Gas Cave"},
    {0x0DB, GGM "Canary Cave"},
    {0x0DC, GGM "Ordnance Storage"},
    {0x0DD, WW "Dodgem Dome Lobby"},
    {0x0DE, WW "Dodgem Dome"},
    {0x0DF, WW "Dodgem Dome"},
    {0x0E0, WW "Dodgem Dome"},
    {0x0E1, WW "Crazy Castle Stockade"},
    {0x0E2, WW "Crazy Castle Lobby"},
    {0x0E3, WW "Crazy Castle Pump Room"},
    {0x0E4, WW "Balloon Burst"},
    {0x0E5, WW "Hoop Hurry"},
    {0x0E6, WW "Star Spinner"},
    {0x0E7, WW "The Inferno"},
    {0x0E9, GGM "Wumba's Wigwam"},
    {0x0EA, WW "Cave of Horrors"},
    {0x0EB, WW "Haunted Cavern"},
    {0x0EC, WW "Train Station"},
    {0x0ED, JRL "Jolly's"},
    {0x0EE, JRL "Pawno's Emporium"},
    {0x0EF, JRL "Mumbo's Skull"},
    {0x0F1, JRL "UFO"},
    {0x0F5, JRL "Ancient Swimming Baths"},
    {0x0F6, JRL "Electric Eels' Lair"},
    {0x0F7, JRL "Seaweed Sanctum"},
    {0x0F8, JRL "Inside the Big Fish"},
    {0x0F9, WW "Big Top"},
    {0x0FA, JRL "Temple of the Fishes"},
    {0x0FC, JRL "Lord Woo Fak Fak"},
    {0x0FF, JRL "Blubber's Wave Race Hire"},
    {0x100, "Grunty Industries"},
    {0x101, GI "Floor 1"},
    {0x102, GI "Train Station"},
    {0x103, GI "Workers' Quarters"},
    {0x104, GI "Trash Compactor"},
    {0x105, GI "Elevator Shaft"},
    {0x106, GI "Floor 2"},
    {0x107, GI "Electromagnet Chamber"},
    {0x108, GI "Floor 3"},
    {0x109, GI "Boiler Plant"},
    {0x10A, GI "Packing Room"},
    {0x10B, GI "Floor 4"},
    {0x10C, GI "Cable Room"},
    {0x10D, GI "Quality Control"},
    {0x10E, GI "Floor 5"},
    {0x10F, GI "Basement"},
    {0x110, GI "Repair Depot"},
    {0x111, GI "Waste Disposal Plant"},
    {0x112, "Terrydactyland"},
    {0x113, TDL "Terry's Nest"},
    {0x114, TDL "Train Station"},
    {0x115, TDL "Oogle Boogle Cave"},
    {0x116, TDL "Inside the Mountain"},
    {0x117, TDL "River Passage"},
    {0x118, TDL "Styracosaurus Family Cave"},
    {0x119, TDL "Unga Bunga's Cave"},
    {0x11A, TDL "Stomping Plains"},
    {0x11B, TDL "Bonfire Cavern"},
    {0x11E, TDL "Wumba's Wigwam"},
    {0x11F, GI "Wumba's Wigwam"},
    {0x120, JRL "Wumba's Wigwam"},
    {0x121, "Chuffy's Wagon"},
    {0x122, TDL "Wumba's Wigwam"},
    {0x123, TDL "Inside Chompa's Belly"},
    {0x124, WW "Saucer of Peril"},
    {0x125, GI "Water Supply Pipe"},
    {0x126, GGM "Water Supply Pipe"},
    {0x127, HFP "Lava Side"},
    {0x128, HFP "Icy Side"},
    {0x129, HFP "Lava Train Station"},
    {0x12A, HFP "Icy Train Station"},
    {0x12B, HFP "Chilli Billi Crater"},
    {0x12C, HFP "Chilly Willy Crater"},
    {0x12D, HFP "Kickball Stadium Lobby"},
    {0x12E, HFP "Kickball Stadium"},
    {0x12F, HFP "Kickball Stadium"},
    {0x130, HFP "Kickball Stadium"},
    {0x131, HFP "Boggy's Igloo"},
    {0x132, HFP "Icicle Grotto"},
    {0x133, HFP "Inside the Volcano"},
    {0x134, HFP "Mumbo's Skull"},
    {0x135, HFP "Wumba's Wigwam"},
    {0x136, "Cloud Cuckooland"},
    {0x137, CCL "Inside the Trash Can"},
    {0x138, CCL "Inside the Cheese Wedge"},
    {0x139, CCL "Zubbas' Nest"},
    {0x13A, CCL "Central Cavern"},
    {0x13B, WW "Crazy Castle Stockade"},
    {0x13C, WW "Star Spinner"},
    {0x13D, CCL "Pot o' Gold"},
    {0x13E, CCL "Mumbo's Skull"},
    {0x13F, CCL "Mingy Jongo's Skull"},
    {0x140, CCL "Wumba's Wigwam"},
    {0x141, IOH "Digger Tunnel"},
    {0x142, "Jinjo Village"},
    {0x143, JV "Bottles' House"},
    {0x144, JV "King Jingaling's Throne Room"},
    {0x145, JV "Green Jinjo Family House"},
    {0x146, JV "Black Jinjo Family House"},
    {0x147, JV "Yellow Jinjo Family House"},
    {0x148, JV "Blue Jinjo Family House"},
    {0x14A, JV "Brown Jinjo Family House"},
    {0x14B, JV "Orange Jinjo Family House"},
    {0x14C, JV "Purple Jinjo Family House"},
    {0x14D, JV "Red Jinjo Family House"},
    {0x14E, JV "White Jinjo Family House"},
    {0x14F, IOH "Wooded Hollow"},
    {0x150, IOH "Heggy's Egg Shed"},
    {0x151, IOH "Jiggywiggy's Temple"},
    {0x152, IOH "Plateau"},
    {0x153, IOH "Honey B's Hive"},
    {0x154, IOH "Pine Grove"},
    {0x155, IOH "Cliff Top"},
    {0x156, IOH "Mumbo's Skull"},
    {0x157, IOH "Wumba's Wigwam"},
    {0x15A, IOH "Wasteland"},
    {0x15B, IOH "Digger Tunnel"},
    {0x15C, IOH "Quagmire"},
    {0x15D, "Cauldron Keep"},
    {0x15E, CK "The Gatehouse"},
    {0x15F, CK "Tower of Tragedy Quiz"},
    {0x160, CK "Gun Chamber"},
    {0x161, CCL "Canary Mary Race"},
    {0x162, GI "Clinker's Cavern"},
    {0x163, GGM "Ordnance Storage Entrance"},
    {0x16F, GGM "Canary Mary Race"},
    {0x170, GGM "Canary Mary Race"},
    {0x171, TDL "Mumbo's Skull"},
    {0x172, GI "Mumbo's Skull"},
    {0x173, SM "Banjo's House"},
    {0x176, WW "Mumbo's Skull"},
    {0x177, MT "Slightly Sacred Chamber"},
    {0x178, MT "Inside Targitzan's Temple"},
    {0x179, MT "Targitzan's Temple Lobby"},
    {0x17A, MT "Targitzan's Really Sacred Chamber"},
    {0x181, JRL "Sea Bottom Cavern"},
    {0x187, GI "Sewer Entrance"},
    {0x18A, CK "Inside the Digger"},
    {0x19A, "Cauldron Keep"},
    {0x19B, JV "King Jingaling's Throne Room"},
    {0x1A6, JRL "Smuggler's Cavern"},
    {0x1A7, "Jolly Roger's Lagoon"},
    {0x1A8, JRL "Atlantis"},
    {0x1A9, JRL "Sea Bottom"},
};
#undef MT
#undef GGM
#undef WW
#undef JRL
#undef GI
#undef TDL
#undef HFP
#undef CCL
#undef SM
#undef JV
#undef IOH
#undef CK
static_assert(std::is_sorted(std::begin(k_names), std::end(k_names),
    [](const Entry& a, const Entry& b) { return a.map < b.map; }));
}

namespace tooie::map_names {

const char* name(std::uint16_t map) noexcept {
    const auto* found = std::lower_bound(std::begin(k_names), std::end(k_names), map,
        [](const Entry& entry, std::uint16_t id) { return entry.map < id; });
    return found != std::end(k_names) && found->map == map ? found->name : nullptr;
}

} // namespace tooie::map_names
