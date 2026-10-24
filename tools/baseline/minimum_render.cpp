#include "minimum_render.hpp"
#include "minimum_menu.hpp"
#include "gui/rt64_inspector.h"
#include "hle/rt64_application.h"
#include "render/rt64_render_target.h"
#include "rhi/rt64_render_hooks.h"
#include "plume_vulkan.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#ifdef _WIN32
#include "imgui/backends/imgui_impl_dx12.h"
#endif
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace baseline::host::render {
namespace {
struct Owners {
    RT64::Application* application;
    plume::RenderDevice* device;
    plume::RenderSwapChain* swap_chain;
    RT64::RenderWorker* worker;
    RT64::UserConfiguration::GraphicsAPI api;
    SDL_Window* window; // Opaque backend handle; external mode performs no SDL-owner calls.
};
std::shared_ptr<const Owners> published;
std::atomic<int> msaa_limit{0};
std::unique_ptr<RT64::Inspector> inspector;
bool failed=false,was_suppressed=false,actual_vsync=true;
int last_vsync=-2;
Status sync_status=Status::Pending,config_status=Status::Pending;
void init(plume::RenderInterface*,plume::RenderDevice*) {
    // Early RT64 Init precedes swapchain/workers; complete publication follows setup.
}
void fail_once(const char* reason) noexcept {
    if(!failed)std::fprintf(stderr,"baseline menu unavailable: %s\n",reason);
    failed=true;publish_ready(false);
}
void apply_gamepad(const Snapshot& snapshot) {
    auto& io=ImGui::GetIO();
    const auto& pad=snapshot.gamepad;
    const bool connected=pad.connected && !snapshot.suppressed;
    if(connected)io.BackendFlags|=ImGuiBackendFlags_HasGamepad;
    else io.BackendFlags&=~ImGuiBackendFlags_HasGamepad;
    const uint32_t buttons=connected?pad.buttons:0;
    const auto button=[&](SDL_GameControllerButton key,ImGuiKey target){io.AddKeyEvent(target,(buttons&(1u<<key))!=0);};
    button(SDL_CONTROLLER_BUTTON_START,ImGuiKey_GamepadStart);button(SDL_CONTROLLER_BUTTON_BACK,ImGuiKey_GamepadBack);
    button(SDL_CONTROLLER_BUTTON_X,ImGuiKey_GamepadFaceLeft);button(SDL_CONTROLLER_BUTTON_B,ImGuiKey_GamepadFaceRight);
    button(SDL_CONTROLLER_BUTTON_Y,ImGuiKey_GamepadFaceUp);button(SDL_CONTROLLER_BUTTON_A,ImGuiKey_GamepadFaceDown);
    button(SDL_CONTROLLER_BUTTON_DPAD_LEFT,ImGuiKey_GamepadDpadLeft);button(SDL_CONTROLLER_BUTTON_DPAD_RIGHT,ImGuiKey_GamepadDpadRight);
    button(SDL_CONTROLLER_BUTTON_DPAD_UP,ImGuiKey_GamepadDpadUp);button(SDL_CONTROLLER_BUTTON_DPAD_DOWN,ImGuiKey_GamepadDpadDown);
    button(SDL_CONTROLLER_BUTTON_LEFTSHOULDER,ImGuiKey_GamepadL1);button(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,ImGuiKey_GamepadR1);
    button(SDL_CONTROLLER_BUTTON_LEFTSTICK,ImGuiKey_GamepadL3);button(SDL_CONTROLLER_BUTTON_RIGHTSTICK,ImGuiKey_GamepadR3);
    const auto analog=[&](ImGuiKey key,float value) {
        const float strength=value<=.18f?0.f:std::clamp((value-.18f)/.82f,0.f,1.f);
        io.AddKeyAnalogEvent(key,strength>0,strength);
    };
    const float x=connected?pad.left_x:0.f,y=connected?pad.left_y:0.f;
    analog(ImGuiKey_GamepadLStickLeft,-x);analog(ImGuiKey_GamepadLStickRight,x);
    analog(ImGuiKey_GamepadLStickUp,-y);analog(ImGuiKey_GamepadLStickDown,y);
}
bool initialize_once(const Owners& owners) {
    if(failed)return false;
    if(inspector)return true;
    try {
        inspector=std::make_unique<RT64::Inspector>(owners.device,owners.swap_chain,owners.api,owners.window,true);
        auto& io=ImGui::GetIO();io.IniFilename=nullptr;
        io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard|ImGuiConfigFlags_NavEnableGamepad|ImGuiConfigFlags_NoMouseCursorChange;
        if(!io.Fonts->AddFontDefault()){fail_once("font creation failed");return false;}
        if(owners.api==RT64::UserConfiguration::GraphicsAPI::Vulkan) {
            const auto* swap=static_cast<const plume::VulkanSwapChain*>(owners.swap_chain);
            if(!swap->commandQueue || !swap->commandQueue->queue || !swap->commandQueue->queue->mutex) {
                fail_once("font queue unavailable");return false;
            }
            // Lock the shared physical VkQueue, not only the logical worker queue.
            std::lock_guard lock(*swap->commandQueue->queue->mutex);
            if(!ImGui_ImplVulkan_CreateFontsTexture() || !io.Fonts->TexID) {
                fail_once("font texture unavailable");return false;
            }
        }
        else {
#ifdef _WIN32
            // Keep the selected D3D12 backend's own device/font creation path.
            ImGui_ImplDX12_NewFrame();
            if(!io.Fonts->TexID){fail_once("font texture unavailable");return false;}
#else
            fail_once("graphics API unavailable");return false;
#endif
        }
        std::fputs("baseline menu initialized on public Draw; font available\n",stderr);
        return true;
    } catch(const std::exception& error) {fail_once(error.what());return false;}
    // Existing backend Vulkan error reporting remains authoritative; TexID is
    // not a proof of arbitrary allocation/device-loss recovery or GPU completion.
}
void draw(plume::RenderCommandList* list,plume::RenderFramebuffer* framebuffer) {
    const auto owners=std::atomic_load_explicit(&published,std::memory_order_acquire);
    if(!owners || !list || !framebuffer || list!=owners->worker->commandList.get()) {
        publish_ready(false);return;
    }
    config_status=reconcile_aspect(*owners->application->sharedQueueResources,settings_word(),publication_word());
    const int requested=requested_vsync();
    if(requested>=0) {
        if(requested!=last_vsync) {owners->swap_chain->setVsyncEnabled(requested!=0);last_vsync=requested;}
        sync_status=owners->swap_chain->needsResize()?Status::Pending:
            (owners->swap_chain->isVsyncEnabled()==(requested!=0)?Status::Applied:Status::Unsupported);
    }
    else sync_status=Status::Applied; // Inherited backend state, no setter call.
    actual_vsync=owners->swap_chain->isVsyncEnabled();
    auto input=take_render_input();
    if(!input.snapshot || input.snapshot->minimized || input.snapshot->width<=0 || input.snapshot->height<=0) {
        publish_ready(false);return;
    }
    if(!initialize_once(*owners))return;
    auto& io=ImGui::GetIO();const auto& snapshot=*input.snapshot;
    io.DisplaySize=ImVec2(float(snapshot.width),float(snapshot.height));
    io.DisplayFramebufferScale=ImVec2(float(snapshot.pixel_width)/snapshot.width,float(snapshot.pixel_height)/snapshot.height);
    if(input.reset_input || (snapshot.suppressed && !was_suppressed))io.ClearInputKeys();
    was_suppressed=snapshot.suppressed;
    io.AddFocusEvent(snapshot.focused);
    for(auto& event:input.events) {
        if(snapshot.suppressed && (event.type==SDL_KEYDOWN || event.type==SDL_KEYUP || event.type==SDL_TEXTINPUT))continue;
        inspector->handleSdlEvent(&event);
    }
    apply_gamepad(snapshot);
    inspector->newFrame(owners->worker);
    draw_menu(snapshot);
    inspector->endFrame();
    inspector->draw(list);
    publish_ready(true);
}
void deinit() {
    // Application::end has joined PresentQueue before this callback and still
    // owns device/worker/swapchain; destruction never races public Draw.
    publish_ready(false);
    std::atomic_store_explicit(&published,std::shared_ptr<const Owners>{},std::memory_order_release);
    inspector.reset();failed=false;was_suppressed=false;last_vsync=-2;
    sync_status=config_status=Status::Pending;
}
}
void install(){RT64::SetRenderHooks(init,draw,deinit);}
void publish_renderer(RT64::Application& app,SDL_Window* window) {
    if(!window || !app.device || !app.swapChain || !app.presentGraphicsWorker || !app.sharedQueueResources ||
       (app.chosenGraphicsAPI!=RT64::UserConfiguration::GraphicsAPI::Vulkan &&
        app.chosenGraphicsAPI!=RT64::UserConfiguration::GraphicsAPI::D3D12))
        throw std::runtime_error("Incomplete baseline render publication");
    const auto prior=std::atomic_load_explicit(&published,std::memory_order_acquire);
    if(prior)throw std::runtime_error("Repeated baseline renderer publication without Deinit");
    std::shared_ptr<const Owners> owners=std::make_shared<Owners>(Owners{&app,app.device.get(),app.swapChain.get(),
        app.presentGraphicsWorker.get(),app.chosenGraphicsAPI,window});
    std::atomic_store_explicit(&published,std::move(owners),std::memory_order_release);
    // RT64 multisampling needs programmable sample locations and a sample count the
    // color and depth formats share (RT64::Application::setup's own check).
    int highest=0;
    if(app.device->getCapabilities().sampleLocations) {
        const auto common=app.device->getSampleCountsSupported(RT64::RenderTarget::colorBufferFormat(app.shaderLibrary->usesHDR))&
            app.device->getSampleCountsSupported(RT64::RenderTarget::depthBufferFormat());
        if(common&plume::RenderSampleCount::Bits::COUNT_2)highest=2;
        if(common&plume::RenderSampleCount::Bits::COUNT_4)highest=4;
        if(common&plume::RenderSampleCount::Bits::COUNT_8)highest=8;
    }
    msaa_limit.store(highest,std::memory_order_release);
}
int max_msaa() noexcept{return msaa_limit.load(std::memory_order_acquire);}
Status vsync_status() noexcept{return sync_status;}
Status aspect_status() noexcept{return config_status;}
bool effective_vsync() noexcept{return actual_vsync;}
}
