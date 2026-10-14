#pragma once
#include "minimum_host.hpp"
#include <cstdint>
namespace RT64 { struct Application; struct SharedQueueResources; }
namespace baseline::host::render {
enum class Status { Pending, Applied, Unsupported };
void install();
void configure_startup(RT64::Application&);
void publish_renderer(RT64::Application&,SDL_Window*);
void latch_startup(bool pillarbox_capable);
Status reconcile_aspect(RT64::SharedQueueResources&,uint32_t settings,uint32_t publication);
Status vsync_status() noexcept;
Status aspect_status() noexcept;
bool effective_vsync() noexcept;
uint32_t settings_word() noexcept;
uint32_t publication_word() noexcept;
void publish_guest(uint32_t generation,uint32_t pillarbox) noexcept;
}
