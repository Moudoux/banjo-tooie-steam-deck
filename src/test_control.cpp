#include "test_control.hpp"

#include "SDL.h"

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

// Commands, one per line (linux/tests/harness/player.py writes them):
//   pad attach | pad detach | pad state (logs the axes SDL reports)
//   pad button NAME 0|1      a b x y back guide start ls rs lb rb up down left right
//   pad axis NAME VALUE      lx ly rx ry lt rt, -32768..32767 (SDL's axis values)
//   key NAME down|up         SDL scancode name, e.g. F4
//   mouse DX DY              relative motion in counts
//   hint NAME                logs the SDL hint's value
//   camera X Y Z P Y R       holds the camera at a pose every frame (camera_analog.cpp's orbit
//                            hook, on the game thread); "camera off" releases it
//   echo TOKEN               logs TOKEN once every earlier command is applied
//   quit                     requests the normal shutdown
// Each command's effect is logged as "test-control ..." in the player log.
namespace {
const char* const path = std::getenv("TOOIE_TEST_CONTROL");
std::mutex queue_mutex;
std::deque<std::string> queue;
SDL_Window* window = nullptr;
SDL_Joystick* pad = nullptr;
SDL_JoystickID pad_instance = -1;
std::mutex pose_mutex;
bool pose_set = false;
float pose[6]{};

void read_commands() {
    // A plain descriptor, not stdio: a FILE blocked in a read holds its lock, and the
    // player's final fflush(nullptr) would wait on it forever.
    // Opening a FIFO for reading waits for the writer; reopen if the harness reconnects.
    for (;;) {
        const int fd = ::open(path, O_RDONLY | O_CLOEXEC);
        if (fd < 0) { std::fprintf(stderr, "test-control error=open path=%s\n", path); return; }
        std::string pending;
        char buffer[512];
        ssize_t count;
        while ((count = ::read(fd, buffer, sizeof buffer)) > 0) {
            pending.append(buffer, size_t(count));
            for (size_t end; (end = pending.find('\n')) != std::string::npos; pending.erase(0, end + 1)) {
                std::string command = pending.substr(0, end);
                if (!command.empty() && command.back() == '\r') command.pop_back();
                if (command.empty()) continue;
                std::lock_guard lock(queue_mutex);
                queue.push_back(std::move(command));
            }
        }
        ::close(fd);
    }
}

int button(const std::string& name) {
    static const char* const names[] = {"a", "b", "x", "y", "back", "guide", "start", "ls", "rs", "lb", "rb",
        "up", "down", "left", "right"};
    for (int i = 0; i < int(std::size(names)); ++i) if (name == names[i]) return i; // SDL_GameControllerButton order.
    return -1;
}
int axis(const std::string& name) {
    static const char* const names[] = {"lx", "ly", "rx", "ry", "lt", "rt"};
    for (int i = 0; i < int(std::size(names)); ++i) if (name == names[i]) return i; // SDL_GameControllerAxis order.
    return -1;
}

void push_key(SDL_Scancode code, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_KEYDOWN : SDL_KEYUP;
    event.key.windowID = SDL_GetWindowID(window);
    event.key.state = down ? SDL_PRESSED : SDL_RELEASED;
    event.key.keysym.scancode = code;
    event.key.keysym.sym = SDL_GetKeyFromScancode(code);
    SDL_PushEvent(&event);
}

void apply(const std::string& command) {
    std::istringstream words(command);
    std::string verb, a, b;
    words >> verb >> a >> b;
    bool ok = true;
    if (verb == "pad" && a == "attach" && !pad) {
        // An Xbox 360 layout: SDL maps a virtual controller's buttons and axes in its own order.
        SDL_VirtualJoystickDesc desc{};
        desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
        desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
        desc.naxes = SDL_CONTROLLER_AXIS_MAX;
        desc.nbuttons = SDL_CONTROLLER_BUTTON_DPAD_RIGHT + 1;
        desc.vendor_id = 0x045E;
        desc.product_id = 0x028E;
        desc.name = "Tooie test pad";
        const int index = SDL_JoystickAttachVirtualEx(&desc);
        pad = index < 0 ? nullptr : SDL_JoystickOpen(index);
        ok = pad != nullptr;
        if (pad) {
            pad_instance = SDL_JoystickInstanceID(pad);
            // Triggers rest at their minimum, as a real pad reports them.
            SDL_JoystickSetVirtualAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -32768);
            SDL_JoystickSetVirtualAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768);
        }
    } else if (verb == "pad" && a == "detach" && pad) {
        SDL_JoystickClose(pad);
        pad = nullptr;
        ok = false;
        for (int i = 0; i < SDL_NumJoysticks(); ++i)
            if (SDL_JoystickGetDeviceInstanceID(i) == pad_instance) ok = SDL_JoystickDetachVirtual(i) == 0;
    } else if (verb == "pad" && a == "button" && pad && button(b) >= 0) {
        std::string value; words >> value;
        ok = SDL_JoystickSetVirtualButton(pad, button(b), value == "1" ? SDL_PRESSED : SDL_RELEASED) == 0;
    } else if (verb == "pad" && a == "axis" && pad && axis(b) >= 0) {
        int value = 0; words >> value;
        ok = SDL_JoystickSetVirtualAxis(pad, axis(b), Sint16(SDL_clamp(value, -32768, 32767))) == 0;
    } else if (verb == "key" && window && SDL_GetScancodeFromName(a.c_str()) != SDL_SCANCODE_UNKNOWN &&
               (b == "down" || b == "up")) {
        push_key(SDL_GetScancodeFromName(a.c_str()), b == "down");
    } else if (verb == "mouse" && window) {
        SDL_Event event{};
        event.type = SDL_MOUSEMOTION;
        event.motion.windowID = SDL_GetWindowID(window);
        event.motion.xrel = std::atoi(a.c_str());
        event.motion.yrel = std::atoi(b.c_str());
        SDL_PushEvent(&event);
    } else if (verb == "pad" && a == "state" && pad) {
        // What the game's controller layer reads back from the virtual pad.
        SDL_GameController* controller = SDL_GameControllerFromInstanceID(pad_instance);
        std::fprintf(stderr, "test-control pad-state mapped=%d", controller != nullptr);
        for (int i = 0; controller && i < SDL_CONTROLLER_AXIS_MAX; ++i)
            std::fprintf(stderr, " a%d=%d", i, SDL_GameControllerGetAxis(controller, SDL_GameControllerAxis(i)));
        for (int i = 0; i < SDL_JoystickNumAxes(pad); ++i) std::fprintf(stderr, " j%d=%d", i, SDL_JoystickGetAxis(pad, i));
        std::fprintf(stderr, "\n");
        return;
    } else if (verb == "camera") {
        float values[6]{};
        std::istringstream all(command.substr(7));
        int count = 0;
        while (count < 6 && all >> values[count]) ++count;
        std::lock_guard lock(pose_mutex);
        pose_set = count == 6;
        std::copy(values, values + 6, pose);
        ok = pose_set || a == "off";
    } else if (verb == "hint") {
        const char* value = SDL_GetHint(a.c_str());
        std::fprintf(stderr, "test-control hint %s=%s\n", a.c_str(), value ? value : "(unset)");
        return;
    } else if (verb == "echo") {
        std::fprintf(stderr, "test-control echo %s\n", a.c_str());
        return;
    } else if (verb == "quit") {
        SDL_Event event{};
        event.type = SDL_QUIT;
        SDL_PushEvent(&event);
    } else {
        ok = false;
    }
    if (!ok) std::fprintf(stderr, "test-control error=command \"%s\" %s\n", command.c_str(), SDL_GetError());
}
} // namespace

namespace tooie::test_control {

bool active() noexcept { return path && *path; }

void set_window(SDL_Window* target) noexcept { window = target; }

bool camera_pose(float (&position)[3], float (&rotation)[3]) noexcept {
    if (!active()) return false;
    std::lock_guard lock(pose_mutex);
    if (!pose_set) return false;
    std::copy(pose, pose + 3, position);
    std::copy(pose + 3, pose + 6, rotation);
    return true;
}

void pump() noexcept {
    if (!active()) return;
    static bool started = false;
    if (!started) {
        started = true;
        std::thread(read_commands).detach();
        std::fprintf(stderr, "test-control active path=%s\n", path);
    }
    std::deque<std::string> commands;
    {
        std::lock_guard lock(queue_mutex);
        commands.swap(queue);
    }
    for (const auto& command : commands) apply(command);
}

} // namespace tooie::test_control
