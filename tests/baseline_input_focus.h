#pragma once
// baseline_input_host_test only: SDL's dummy driver never grants keyboard focus.
// Replaces the OS focus query; mapper/host focus policy remains production code.
// Real native focus acquisition/event ordering and SDL keyboard state are untested.
#include <SDL.h>
SDL_Window* baseline_fixture_keyboard_focus() noexcept;
#define SDL_GetKeyboardFocus baseline_fixture_keyboard_focus
