#pragma once

struct SDL_Window;

namespace tooie::test_control {

// Linux test suite (linux/tests): when TOOIE_TEST_CONTROL names a FIFO at startup,
// the player reads commands from it and applies them on the SDL owner thread: a
// virtual gamepad, key and mouse events, hint queries and quit. It also stands in
// for window focus, so tests run in the background. Set but empty, it only keeps
// errors out of dialogs (main.cpp). Without the variable nothing here does anything.
bool active() noexcept;

// SDL owner thread: the game window, from baseline::host::initialize.
void set_window(SDL_Window* window) noexcept;

// SDL owner thread, before each event pump: applies the commands received so far.
void pump() noexcept;

// Guest thread (camera_analog.cpp's orbit hook): the camera pose the suite asked for with
// "camera X Y Z PITCH YAW ROLL" (position, rotation in degrees), until "camera off".
bool camera_pose(float (&position)[3], float (&rotation)[3]) noexcept;

} // namespace tooie::test_control
