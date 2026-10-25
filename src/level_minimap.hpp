#pragma once

#include "recomp.h"

#include <cstdint>

namespace tooie::level_minimap {

// Player: a top-down map of the loaded level's collision floors around the
// player, drawn in the bottom-left corner (Video menu "Minimap"), with the
// area's name and markers for the collectibles and pads still in the level.
// The guest thread copies the geometry when the level's collision sources
// change; the ImGui frame owner draws the copy, rotated so the camera looks up
// the map. While the game is paused the same map can fill the screen.
void set_enabled(bool enabled) noexcept;
bool enabled() noexcept;
// Zoom level, 0 (widest) to zoom_levels - 1; default_zoom is 3000 units to the edge.
constexpr int zoom_levels = 6, default_zoom = 2;
const char* zoom_label(int level) noexcept;
void set_zoom(int level) noexcept;
int zoom() noexcept;
// Any thread. The minimap is on screen: enabled, in gameplay, not paused.
bool visible() noexcept;
// Any thread. The game's pause menu is open over a map the minimap has.
bool paused() noexcept;
// Full-screen map over the pause menu, driven by the SDL owner.
void set_full_map(bool open) noexcept;
bool full_map() noexcept;
void zoom_full_map(int steps) noexcept;
// Stick values (-1..1, up positive) held for `seconds`; full tilt pans half a view per second.
void pan_full_map(float right, float up, float seconds) noexcept;
// ImGui frame owner only, inside a frame. Draws nothing unless the game
// updated the player recently (not in menus, cutscenes or loads), or the full
// map or its hint while paused.
void render();
// Graphics owner, once per screen: samples the game state (pause) from RDRAM.
void observe_screen(const std::uint8_t* rdram) noexcept;

} // namespace tooie::level_minimap

// Raw hook just before func_800991B0 calls bainput_update; a0 is the PlayerState being updated.
extern "C" void tooie_level_minimap_observe(std::uint8_t* rdram, recomp_context* ctx);
