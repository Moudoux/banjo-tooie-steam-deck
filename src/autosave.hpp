#pragma once

#include "recomp.h"

#include <cstdint>
#include <filesystem>

namespace tooie::autosave {

// Player: saves the game every few minutes of play (Controls menu "Autosave
// every"), through the routine the pause menu's Save & Exit and every map
// change use, so the game's own rules on when a file may be saved still apply.
// The game's own saves restart the timer. Before each autosave the save file
// is copied to <save>.autosave.bak; an autosave whose backup fails is skipped.
void set_interval_minutes(int minutes) noexcept; // 0 turns autosave off.
// Startup, before the runtime reads the save: a save file of the wrong size
// (cut short by a crash while it was written) is replaced by the backup.
void recover(const std::filesystem::path& save);
// ImGui frame owner only, inside a frame: "Saving..." in the bottom-right
// corner while an autosave is written, and briefly after.
void render();

} // namespace tooie::autosave

// Raw hook just before func_800991B0 calls bainput_update; a0 is the PlayerState being updated.
extern "C" void tooie_autosave_update(std::uint8_t* rdram, recomp_context* ctx);
