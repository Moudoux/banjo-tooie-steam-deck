#pragma once

#include "recomp.h"

#include <cstdint>

namespace tooie::level_minimap {

// Player: a top-down map of the loaded level's collision floors around the
// player, drawn in the bottom-left corner (Video menu "Minimap"). The guest
// thread copies the geometry when the level's collision sources change; the
// ImGui frame owner draws the copy, rotated so the camera looks up the map.
void set_enabled(bool enabled) noexcept;
bool enabled() noexcept;
// ImGui frame owner only, inside a frame. Draws nothing unless the game
// updated the player recently (not in menus, cutscenes, loads or the pause menu).
void render();

} // namespace tooie::level_minimap

// Raw hook just before func_800991B0 calls bainput_update; a0 is the PlayerState being updated.
extern "C" void tooie_level_minimap_observe(std::uint8_t* rdram, recomp_context* ctx);
