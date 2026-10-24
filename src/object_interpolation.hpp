#pragma once

#include <cstdint>

namespace RT64 { struct State; }

// Matrix groups for every actor the game draws and the player, without writing to
// its display lists: the draw hooks record each object's command range, and an RT64
// interpreter observer opens the object's group while it runs those commands.
// linux/INTERPOLATION.md describes the cases, the fixes and the trace.
namespace tooie::objects {
void set_enabled(bool enabled) noexcept;
bool enabled() noexcept;
// Graphics owner, before RT64 interprets the task's display list.
void begin_display_list(RT64::State* state, std::uint32_t physical_address);
// Graphics owner, after RT64 interpreted it: writes the armed trace frame's workload.
void end_display_list(RT64::State* state);
// With TOOIE_INTERPOLATION_TRACE=1, arms a trace of the next three frames (F4).
void request_trace() noexcept;
}
