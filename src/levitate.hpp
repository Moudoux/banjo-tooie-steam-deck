#pragma once

#include "recomp.h"

#include <cstdint>

namespace tooie::levitate {

// Player: Lighthouse's "Hold L to Levitate" (Cheats.cpp). While N64 L is held
// the controlled character rises with gravity off; releasing L restores the
// default gravity. Off by default.
void set_enabled(bool enabled) noexcept;
bool enabled() noexcept;

} // namespace tooie::levitate

// Raw hook just before func_800991B0 calls bainput_update; a0 is the PlayerState being updated.
extern "C" void tooie_levitate_update(std::uint8_t* rdram, recomp_context* ctx);
