#include "minimum_render.hpp"
#include "minimum_presentation_abi.h"
#include "widescreen.hpp"
#include "hle/rt64_application.h"
#include "hle/rt64_shared_queue_resources.h"
#include "hle/rt64_state.h"
#include <algorithm>
#include <atomic>
#include <mutex>
#include <stdexcept>

namespace baseline::host::render {
namespace {
std::atomic<uint32_t> settings{0},publication{0};
int applied_dither_noise=100; // Graphics owner; RT64's own default strength.
bool rt64_rate(RT64::UserConfiguration& user,const ultramodern::renderer::GraphicsConfig& config) {
    if(config.rr_manual_value<20 || config.rr_manual_value>360)return false;
    using Rate=RT64::UserConfiguration::RefreshRate;
    Rate rate;
    switch(config.rr_option) {
    case ultramodern::renderer::RefreshRate::Original:rate=Rate::Original;break;
    case ultramodern::renderer::RefreshRate::Display:rate=Rate::Display;break;
    case ultramodern::renderer::RefreshRate::Manual:rate=Rate::Manual;break;
    default:return false;
    }
    user.refreshRate=rate;user.refreshRateTarget=config.rr_manual_value;
    return true;
}
}
void preference_rate(ultramodern::renderer::GraphicsConfig& config,int option,int manual_hz) {
    using Rate=ultramodern::renderer::RefreshRate;
    config.rr_option=option==2?Rate::Display:option==3?Rate::Manual:Rate::Original;
    config.rr_manual_value=manual_hz;
}
bool update_rate(RT64::UserConfiguration& app_user,RT64::SharedQueueResources& shared,
    const ultramodern::renderer::GraphicsConfig& old_config,const ultramodern::renderer::GraphicsConfig& new_config) {
    if(old_config==new_config)return false;
    auto other=new_config;
    other.rr_option=old_config.rr_option;other.rr_manual_value=old_config.rr_manual_value;
    if(other!=old_config)throw std::runtime_error("Graphics config changed outside output rate");
    RT64::UserConfiguration rate;
    if(!rt64_rate(rate,new_config))throw std::runtime_error("Invalid output-rate configuration");
    std::lock_guard lock(shared.configurationMutex);
    shared.userConfig.refreshRate=app_user.refreshRate=rate.refreshRate;
    shared.userConfig.refreshRateTarget=app_user.refreshRateTarget=rate.refreshRateTarget;
    shared.userConfigChanged=true;shared.newConfigValidated=false;
    return true;
}
int effective_supersampling(int scale,int supersampling) noexcept {
    // Supersampling renders at scale x factor and resolves to the chosen scale, so it
    // needs a fixed scale; the internal scale stays within the menu's 8x maximum.
    if(scale<=0)return 1;
    while(supersampling>1 && scale*supersampling>8)supersampling/=2;
    return std::max(supersampling,1);
}
bool update_resolution(RT64::UserConfiguration& app_user,RT64::SharedQueueResources& shared,int scale,int supersampling) {
    if(scale<0 || scale>8)throw std::runtime_error("Invalid resolution scale");
    if(supersampling!=1 && supersampling!=2 && supersampling!=4)throw std::runtime_error("Invalid supersampling factor");
    // Scale 0 lets RT64 pick the integer scale that covers the window height.
    using Mode=RT64::UserConfiguration::Resolution;
    const Mode mode=scale==0?Mode::WindowIntegerScale:Mode::Manual;
    const int factor=effective_supersampling(scale,supersampling);
    const int internal=scale*factor;
    std::lock_guard lock(shared.configurationMutex);
    if(shared.userConfig.resolution==mode && app_user.resolution==mode &&
       shared.userConfig.downsampleMultiplier==factor && app_user.downsampleMultiplier==factor &&
       (scale==0 || (shared.userConfig.resolutionMultiplier==internal && app_user.resolutionMultiplier==internal)))return false;
    // RT64's workload owner rebuilds targets after this configuration change.
    shared.userConfig.resolution=app_user.resolution=mode;
    if(scale)shared.userConfig.resolutionMultiplier=app_user.resolutionMultiplier=internal;
    shared.userConfig.downsampleMultiplier=app_user.downsampleMultiplier=factor;
    shared.userConfigChanged=true;shared.fbConfigChanged=true;shared.newConfigValidated=false;
    return true;
}
bool update_msaa(RT64::Application& app,int samples) {
    using AA=RT64::UserConfiguration::Antialiasing;
    samples=std::min(samples,max_msaa());
    const AA wanted=samples>=8?AA::MSAA8X:samples>=4?AA::MSAA4X:samples>=2?AA::MSAA2X:AA::None;
    auto& shared=*app.sharedQueueResources;
    {
        std::lock_guard lock(shared.configurationMutex);
        if(app.userConfig.antialiasing==wanted && shared.userConfig.antialiasing==wanted)return false;
        shared.userConfig.antialiasing=app.userConfig.antialiasing=wanted;
        shared.userConfigChanged=true;shared.fbConfigChanged=true;shared.newConfigValidated=false;
    }
    // Waits for the queues and rebuilds the multisampled shaders and targets (graphics
    // owner, outside the configuration lock), as the retired frontend did.
    app.updateMultisampling();
    return true;
}
bool update_picture(RT64::Application& app,Picture picture) {
    using Upscale=RT64::UserConfiguration::Upscale2D;
    if(picture.upscale_2d<0 || picture.upscale_2d>2 || picture.dither_noise<0 || picture.dither_noise>100)
        throw std::runtime_error("Invalid picture configuration");
    const Upscale upscale=Upscale(picture.upscale_2d);
    const bool three_point=picture.three_point_filtering!=0,lod=picture.texture_lod!=0;
    // RT64's state reads these from the application copies on this (graphics owner) thread
    // as it builds each workload, so they apply from the next display list.
    // RT64 clears its extended state after every workload, so the noise is set before
    // each display list; only a new value is reported.
    app.state->setDitherNoiseStrength(float(picture.dither_noise)/100.0f);
    const bool noise_changed=picture.dither_noise!=applied_dither_noise;
    applied_dither_noise=picture.dither_noise;
    if(app.userConfig.upscale2D==upscale && app.userConfig.threePointFiltering==three_point &&
       app.enhancementConfig.textureLOD.scale==lod)return noise_changed;
    {
        auto& shared=*app.sharedQueueResources;
        std::lock_guard lock(shared.configurationMutex);
        shared.userConfig.upscale2D=app.userConfig.upscale2D=upscale;
        shared.userConfig.threePointFiltering=app.userConfig.threePointFiltering=three_point;
        shared.userConfigChanged=true;shared.newConfigValidated=false;
    }
    app.enhancementConfig.textureLOD.scale=lod;
    app.updateEnhancementConfig();
    return true;
}
void latch_startup(bool pillarbox_capable) {
    if(settings.load(std::memory_order_acquire))throw std::runtime_error("Baseline supports one game per process");
    const auto& prefs=latched_preferences();
    uint32_t word=1u<<BASELINE_PRESENT_GENERATION_SHIFT;
    if(pillarbox_capable)word|=BASELINE_PRESENT_PILLARBOX_CAPABLE;
    if(prefs.cutscene_aspect==0)word|=BASELINE_PRESENT_ORIGINAL_CUTSCENE;
    word|=uint32_t(prefs.native_aspect)<<BASELINE_PRESENT_ASPECT_SHIFT;
    if(prefs.wide>=0) {
        tooie::widescreen::configure_profile(prefs.wide!=0);
        tooie::widescreen::configure_native_aspect(tooie::widescreen::NativeAspect(prefs.native_aspect));
        tooie::widescreen::latch_for_game_start();
        word|=BASELINE_PRESENT_ACTIVE;
        if(prefs.wide)word|=BASELINE_PRESENT_WIDE;
    }
    // Missing preference never activates the existing G07 override.
    publication.store(0,std::memory_order_release);
    settings.store(word,std::memory_order_release);
}
uint32_t settings_word() noexcept{return settings.load(std::memory_order_acquire);}
uint32_t publication_word() noexcept{return publication.load(std::memory_order_acquire);}
void publish_guest(uint32_t generation,uint32_t pillarbox) noexcept {
    publication.store((generation<<1)|(pillarbox&1u),std::memory_order_release);
}
Status reconcile_aspect(RT64::SharedQueueResources& shared,uint32_t word,uint32_t guest) {
    using Aspect=RT64::UserConfiguration::AspectRatio;
    const bool wide=(word&(BASELINE_PRESENT_ACTIVE|BASELINE_PRESENT_WIDE))==
        (BASELINE_PRESENT_ACTIVE|BASELINE_PRESENT_WIDE);
    const bool bars=wide && (word&BASELINE_PRESENT_PILLARBOX_CAPABLE) &&
        (word>>BASELINE_PRESENT_GENERATION_SHIFT)==(guest>>1) && (guest&1u);
    const Aspect wanted=wide?Aspect::Manual:Aspect::Original;
    constexpr double aspects[]={16.0/9.0,21.0/9.0,32.0/9.0,43.0/18.0,16.0/10.0,16.0/9.0,16.0/9.0,16.0/9.0};
    const double target=bars?4.0/3.0:aspects[(word>>BASELINE_PRESENT_ASPECT_SHIFT)&7u];
    std::unique_lock lock(shared.configurationMutex,std::try_to_lock);
    if(!lock.owns_lock())return Status::Pending;
    auto& config=shared.userConfig;
    const bool changed=config.aspectRatio!=wanted || config.extAspectRatio!=Aspect::Original ||
        (wanted==Aspect::Manual && config.aspectTarget!=target);
    if(changed) {
        config.aspectRatio=wanted;config.extAspectRatio=Aspect::Original;
        if(wanted==Aspect::Manual)config.aspectTarget=target;
        shared.userConfigChanged=true;shared.fbConfigChanged=true;shared.newConfigValidated=false;
    }
    return Status::Applied; // Shared configuration, not displayed pixels; consumers may delay/coalesce.
}
void configure_startup(RT64::Application& app) {
    const auto& prefs=latched_preferences();
    if(prefs.resolution)app.userConfig.resolutionMultiplier=prefs.resolution;
    else app.userConfig.resolution=RT64::UserConfiguration::Resolution::WindowIntegerScale;
#if BASELINE_PLAYER
    // Player presentation stays independent of the engineering RenderMode and output rate.
    app.enhancementConfig.presentation.mode=RT64::EnhancementConfiguration::Presentation::Mode::PresentEarly;
    app.enhancementConfig.textureLOD.scale=prefs.texture_lod!=0; // Mip choice follows rendered resolution.
    // RT64 picks its render target formats once, during setup.
    app.userConfig.internalColorFormat=RT64::UserConfiguration::InternalColorFormat(prefs.color_format);
    app.userConfig.upscale2D=RT64::UserConfiguration::Upscale2D(prefs.upscale_2d);
    app.userConfig.threePointFiltering=prefs.three_point_filtering!=0;
    ultramodern::renderer::GraphicsConfig config{};
    preference_rate(config,prefs.output_rate,prefs.manual_hz);
    if(!rt64_rate(app.userConfig,config))throw std::runtime_error("Invalid startup output rate");
#else
    if(prefs.output_rate==1)app.userConfig.refreshRate=RT64::UserConfiguration::RefreshRate::Original;
    if(prefs.output_rate==2)app.userConfig.refreshRate=RT64::UserConfiguration::RefreshRate::Display;
    if(prefs.output_rate==3) {
        app.userConfig.refreshRate=RT64::UserConfiguration::RefreshRate::Manual;
        app.userConfig.refreshRateTarget=prefs.manual_hz;
    }
    // Output-rate0 inherits the explicitly selected CLI render condition.
#endif
}
}

