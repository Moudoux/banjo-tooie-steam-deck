#pragma once
#include "minimum_host.hpp"
#include "ultramodern/config.hpp"
#include <cstdint>
namespace RT64 { struct Application; struct SharedQueueResources; struct UserConfiguration; }
namespace baseline::host::render {
enum class Status { Pending, Applied, Unsupported };
void install();
void configure_startup(RT64::Application&);
void preference_rate(ultramodern::renderer::GraphicsConfig&,int option,int manual_hz);
bool update_rate(RT64::UserConfiguration&,RT64::SharedQueueResources&,
    const ultramodern::renderer::GraphicsConfig& old_config,const ultramodern::renderer::GraphicsConfig& new_config);
void publish_renderer(RT64::Application&,SDL_Window*);
bool update_resolution(RT64::UserConfiguration&,RT64::SharedQueueResources&,int scale,int supersampling);
int effective_supersampling(int scale,int supersampling) noexcept;
bool update_msaa(RT64::Application&,int samples); // Graphics owner.
int max_msaa() noexcept; // Highest MSAA sample count the device supports; 0 if none.
void latch_startup(bool pillarbox_capable);
Status reconcile_aspect(RT64::SharedQueueResources&,uint32_t settings,uint32_t publication);
Status vsync_status() noexcept;
Status aspect_status() noexcept;
bool effective_vsync() noexcept;
uint32_t settings_word() noexcept;
uint32_t publication_word() noexcept;
void publish_guest(uint32_t generation,uint32_t pillarbox) noexcept;
}
