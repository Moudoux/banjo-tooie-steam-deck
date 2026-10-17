#pragma once
// baseline_input_host_test only: SDL's dummy driver never grants keyboard focus.
// Replaces OS focus and keyboard-state queries; mapper/host policy remains production code.
// Real native focus acquisition/event ordering and SDL keyboard state are untested.
#include <SDL.h>
SDL_Window* baseline_fixture_keyboard_focus() noexcept;
#define SDL_GetKeyboardFocus baseline_fixture_keyboard_focus
const Uint8* baseline_fixture_keyboard_state(int* count) noexcept;
#define SDL_GetKeyboardState baseline_fixture_keyboard_state
