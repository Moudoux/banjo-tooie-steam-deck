#pragma once

#include <cstdint>

namespace tooie::map_names {

// Display name of a gameplay map (the u16 at 0x80132DC2), from the decomp's
// include/maps.h; nullptr for cutscenes, menus, minigame replays and unknown ids.
const char* name(std::uint16_t map) noexcept;

} // namespace tooie::map_names
