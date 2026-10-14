#include "minimum_render.hpp"
#include "minimum_presentation_abi.h"
#include "widescreen.hpp"
#include "hle/rt64_application.h"
#include "hle/rt64_shared_queue_resources.h"
#include <atomic>
#include <mutex>
#include <stdexcept>

namespace baseline::host::render {
namespace {
std::atomic<uint32_t> settings{0},publication{0};
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
    constexpr double aspects[]={16.0/9.0,21.0/9.0,32.0/9.0,43.0/18.0};
    const double target=bars?4.0/3.0:aspects[(word>>BASELINE_PRESENT_ASPECT_SHIFT)&3u];
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
    app.userConfig.resolutionMultiplier=prefs.resolution;
    if(prefs.output_rate==1)app.userConfig.refreshRate=RT64::UserConfiguration::RefreshRate::Original;
    if(prefs.output_rate==2)app.userConfig.refreshRate=RT64::UserConfiguration::RefreshRate::Display;
    if(prefs.output_rate==3) {
        app.userConfig.refreshRate=RT64::UserConfiguration::RefreshRate::Manual;
        app.userConfig.refreshRateTarget=prefs.manual_hz;
    }
    // Output-rate0 inherits the explicitly selected CLI render condition.
}
}

