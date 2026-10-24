#include "minimum_render.hpp"
#include "minimum_presentation_abi.h"
#include "widescreen.hpp"
#if BASELINE_PLAYER
#include "issue_evidence.hpp"
#endif
#include "hle/rt64_application.h"
#include "hle/rt64_shared_queue_resources.h"
#include <algorithm>
#include <atomic>
#include <mutex>
#include <stdexcept>

namespace baseline::host::render {
namespace {
std::atomic<uint32_t> settings{0},publication{0};
RT64::UserConfiguration::Filtering output_filtering(int option) {
    using Filtering=RT64::UserConfiguration::Filtering;
    switch(option) {
    case 1:return Filtering::Nearest;
    case 2:return Filtering::Linear;
    default:return Filtering::AntiAliasedPixelScaling;
    }
}
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
    app_user.refreshRate=rate.refreshRate;
    shared.userConfig.refreshRate=rate.refreshRate;
    app_user.refreshRateTarget=rate.refreshRateTarget;
    shared.userConfig.refreshRateTarget=rate.refreshRateTarget;
    shared.userConfigChanged=true;
    shared.newConfigValidated=false;
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
    app_user.resolution=mode;
    shared.userConfig.resolution=mode;
    if(scale) {
        app_user.resolutionMultiplier=internal;
        shared.userConfig.resolutionMultiplier=internal;
    }
    app_user.downsampleMultiplier=factor;
    shared.userConfig.downsampleMultiplier=factor;
    shared.userConfigChanged=true;
    shared.fbConfigChanged=true;
    shared.newConfigValidated=false;
    return true;
}
bool update_filters(RT64::UserConfiguration& app_user,RT64::SharedQueueResources& shared,int texture_filter,int output_filter) {
    if(texture_filter<0 || texture_filter>1 || output_filter<0 || output_filter>2)
        throw std::runtime_error("Invalid filtering preference");
    const bool three_point=texture_filter==0;
    const auto filtering=output_filtering(output_filter);
    std::lock_guard lock(shared.configurationMutex);
    if(app_user.threePointFiltering==three_point && shared.userConfig.threePointFiltering==three_point &&
       app_user.filtering==filtering && shared.userConfig.filtering==filtering)return false;
    // Targeted fields only: a full userConfig copy would overwrite shared aspect state.
    app_user.threePointFiltering=three_point;
    shared.userConfig.threePointFiltering=three_point;
    app_user.filtering=filtering;
    shared.userConfig.filtering=filtering;
    shared.userConfigChanged=true;
    shared.newConfigValidated=false;
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
void latch_startup(bool pillarbox_capable) {
    if(settings.load(std::memory_order_acquire))throw std::runtime_error("Baseline supports one game per process");
    const auto& prefs=latched_preferences();
    uint32_t word=1u<<BASELINE_PRESENT_GENERATION_SHIFT;
    if(pillarbox_capable)word|=BASELINE_PRESENT_PILLARBOX_CAPABLE;
    word|=uint32_t(prefs.native_aspect<fit_aspect?prefs.native_aspect:0)<<BASELINE_PRESENT_ASPECT_SHIFT;
    // G07 substitution is retired in this composition. Original game_wide
    // remains the projection gate; the guest alone applies the request.
    tooie::widescreen::configure_profile(true);
    // Fit is published live by host::tick; use the existing 16:9 startup fallback explicitly.
    const auto startup_aspect=prefs.native_aspect==fit_aspect?
        tooie::widescreen::NativeAspect::Ratio16x9:tooie::widescreen::NativeAspect(prefs.native_aspect);
    tooie::widescreen::configure_native_aspect(startup_aspect);
    tooie::widescreen::latch_for_game_start();
    word|=BASELINE_PRESENT_ACTIVE;
    if(prefs.wide==1)word|=BASELINE_PRESENT_WIDE; // Retained startup compatibility layout field, not applied state.
    publication.store(0,std::memory_order_release);
    settings.store(word,std::memory_order_release);
}
// Startup policy is immutable after latch_startup; the cutscene-aspect and
// wide requests below are sampled live. publication_word is a separate guest
// acknowledgement, not a frame-coherent snapshot of displayed output.
uint32_t settings_word() noexcept {
    uint32_t word=settings.load(std::memory_order_acquire);
    // Before latch the word stays zero apart from the wide request, as before.
    if(word && requested_cutscene_aspect()==0)word|=BASELINE_PRESENT_ORIGINAL_CUTSCENE;
    return word |
        ((uint32_t(tooie::widescreen::live_widescreen_request()+1)&BASELINE_PRESENT_WIDE_REQUEST_MASK)<<BASELINE_PRESENT_WIDE_REQUEST_SHIFT);
}
uint32_t publication_word() noexcept{return publication.load(std::memory_order_acquire);}
void publish_guest(uint32_t generation,uint32_t flags) noexcept {
    publication.store((generation<<BASELINE_PUBLISH_GENERATION_SHIFT)|(flags&BASELINE_PUBLISH_FLAGS_MASK),std::memory_order_release);
#if BASELINE_PLAYER
    issue_evidence::PublicationRecord record{};
    record.generation = generation; record.flags = flags;
    issue_evidence::record(record);
#endif
}
Status reconcile_aspect(RT64::SharedQueueResources& shared,uint32_t word,uint32_t guest) {
    using Aspect=RT64::UserConfiguration::AspectRatio;
    const uint32_t generation=word>>BASELINE_PRESENT_GENERATION_SHIFT;
    const bool applied=generation && generation==(guest>>BASELINE_PUBLISH_GENERATION_SHIFT);
    const bool bars=applied && (guest&BASELINE_PUBLISH_PILLARBOX);
    // Both pillarbox and widescreen publication select an explicit aspect target.
    const bool manual_aspect=applied && (guest&BASELINE_PUBLISH_FLAGS_MASK);
    const Aspect wanted=manual_aspect?Aspect::Manual:Aspect::Original;
    // One atomic ratio load, shared with guest projection. The two owners may
    // apply/defer/coalesce independently; no frame-lag bound is promised.
    const double target=bars?4.0/3.0:tooie::widescreen::latched_aspect_ratio();
    std::unique_lock lock(shared.configurationMutex,std::try_to_lock);
    if(!lock.owns_lock())return Status::Pending;
    auto& config=shared.userConfig;
    const bool changed=config.aspectRatio!=wanted || config.extAspectRatio!=Aspect::Original ||
        (wanted==Aspect::Manual && config.aspectTarget!=target);
    if(changed) {
        config.aspectRatio=wanted;
        config.extAspectRatio=Aspect::Original;
        if(wanted==Aspect::Manual)config.aspectTarget=target;
        shared.userConfigChanged=true;
        shared.fbConfigChanged=true;
        shared.newConfigValidated=false;
    }
    return Status::Applied; // Shared configuration, not displayed pixels; consumers may delay/coalesce.
}
void configure_startup(RT64::Application& app) {
    const auto& prefs=latched_preferences();
    if(prefs.resolution)app.userConfig.resolutionMultiplier=prefs.resolution;
    else app.userConfig.resolution=RT64::UserConfiguration::Resolution::WindowIntegerScale;
#if BASELINE_PLAYER
    // F4 player correspondence rows (rt64_matrix_trace.hpp) feed the issue-evidence rings; inert
    // unless the selected build includes the matrix-trace hook.
    RT64::tooiePlayerSink.store(&issue_evidence::record_player,std::memory_order_release);
    app.userConfig.threePointFiltering=prefs.texture_filter==0;
    app.userConfig.filtering=output_filtering(prefs.output_filter);
    // Player presentation stays independent of the engineering RenderMode and output rate.
    app.enhancementConfig.presentation.mode=RT64::EnhancementConfiguration::Presentation::Mode::PresentEarly;
    app.enhancementConfig.textureLOD.scale=true; // Mip choice follows rendered resolution.
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

