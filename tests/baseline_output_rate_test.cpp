#include "minimum_render.hpp"
#include "minimum_presentation_abi.h"
#include "common/rt64_user_configuration.h"
#include "hle/rt64_shared_queue_resources.h"
#include <cassert>
#include <stdexcept>

namespace baseline::host {
const Preferences& latched_preferences() noexcept { static const Preferences prefs; return prefs; }
}

int main() {
    using namespace baseline::host;
    using namespace baseline::host::render;
    using namespace ultramodern::renderer;
    using Rate=RT64::UserConfiguration::RefreshRate;
    GraphicsConfig config{};
    config.developer_mode=true;config.res_option=Resolution::Original2x;
    config.wm_option=WindowMode::Windowed;config.ar_option=AspectRatio::Expand;
    config.ds_option=3;
    for(int option=0;option<=3;++option) {
        auto mapped=config;
        preference_rate(mapped,option,120);
        assert(mapped.rr_option==(option==2?RefreshRate::Display:option==3?RefreshRate::Manual:RefreshRate::Original));
        assert(mapped.rr_manual_value==120);
        mapped.rr_option=config.rr_option;mapped.rr_manual_value=config.rr_manual_value;
        assert(mapped==config);
    }
    for(const auto requested:{OutputRate{3,360},OutputRate{0,20}}) {
        std::atomic<uint32_t> handoff{pack_output_rate({})};
        handoff.store(pack_output_rate(requested),std::memory_order_release);
        const auto received=unpack_output_rate(handoff.load(std::memory_order_acquire));
        assert(received.option==requested.option && received.manual_hz==requested.manual_hz);
    }

    RT64::UserConfiguration app;
    RT64::SharedQueueResources shared;
    preference_rate(config,0,60);
    app.refreshRate=shared.userConfig.refreshRate=Rate::Original;
    app.refreshRateTarget=shared.userConfig.refreshRateTarget=60;
    const uint32_t wide=(1u<<BASELINE_PRESENT_GENERATION_SHIFT)|BASELINE_PRESENT_ACTIVE|BASELINE_PRESENT_WIDE;
    assert(reconcile_aspect(shared,wide,0)==Status::Applied);
    assert(shared.userConfig.aspectRatio==RT64::UserConfiguration::AspectRatio::Manual);
    assert(shared.userConfig.aspectTarget==16.0/9.0 && shared.fbConfigChanged);
    for(const auto requested:{OutputRate{2,60},OutputRate{3,120},OutputRate{3,75},OutputRate{1,60}}) {
        auto next=config;
        preference_rate(next,requested.option,requested.manual_hz);
        auto expected_shared=shared.userConfig,expected_app=app;
        const auto rate=requested.option==2?Rate::Display:requested.option==3?Rate::Manual:Rate::Original;
        expected_shared.refreshRate=expected_app.refreshRate=rate;
        expected_shared.refreshRateTarget=expected_app.refreshRateTarget=requested.manual_hz;
        const bool pending=shared.fbConfigChanged;
        const bool rt_changed=shared.rtConfigChanged,swap_changed=shared.swapChainSizeChanged;
        shared.userConfigChanged=false;shared.newConfigValidated=true;
        assert(update_rate(app,shared,config,next));
        assert(json(shared.userConfig)==json(expected_shared) && json(app)==json(expected_app));
        assert(shared.userConfigChanged && !shared.newConfigValidated);
        assert(shared.fbConfigChanged==pending && shared.rtConfigChanged==rt_changed && shared.swapChainSizeChanged==swap_changed);
        shared.userConfigChanged=false;shared.newConfigValidated=true;
        assert(!update_rate(app,shared,next,next));
        assert(json(shared.userConfig)==json(expected_shared) && json(app)==json(expected_app));
        assert(!shared.userConfigChanged && shared.newConfigValidated && shared.fbConfigChanged==pending);
        config=next;
        shared.fbConfigChanged=false; // Subsequent rate changes must not request a discard.
    }
    for(int rejection=0;rejection<5;++rejection) {
        auto next=config;
        preference_rate(next,2,60);
        if(rejection==0)next.ar_option=AspectRatio::Manual;
        if(rejection==1)next.wm_option=WindowMode::Fullscreen;
        if(rejection==2)next.rr_option=RefreshRate(7);
        if(rejection==3)next.rr_manual_value=19;
        if(rejection==4)next.rr_manual_value=361;
        const json before_shared=shared.userConfig,before_app=app;
        const bool changed=shared.userConfigChanged,validated=shared.newConfigValidated,pending=shared.fbConfigChanged;
        bool threw=false;
        try { update_rate(app,shared,config,next); } catch(const std::runtime_error&) { threw=true; }
        assert(threw);
        assert(json(shared.userConfig)==before_shared && json(app)==before_app);
        assert(shared.userConfigChanged==changed && shared.newConfigValidated==validated && shared.fbConfigChanged==pending);
    }

    // Resolution shares the configuration lock but must preserve live aspect/rate.
    app.resolution=shared.userConfig.resolution=RT64::UserConfiguration::Resolution::Manual;
    app.resolutionMultiplier=shared.userConfig.resolutionMultiplier=1;
    for(int scale:{4,8,1}) {
        auto expected_shared=shared.userConfig,expected_app=app;
        expected_shared.resolutionMultiplier=expected_app.resolutionMultiplier=scale;
        const bool rt_changed=shared.rtConfigChanged,swap_changed=shared.swapChainSizeChanged;
        shared.userConfigChanged=false;shared.fbConfigChanged=false;shared.newConfigValidated=true;
        assert(update_resolution(app,shared,scale));
        assert(json(shared.userConfig)==json(expected_shared) && json(app)==json(expected_app));
        assert(shared.userConfigChanged && shared.fbConfigChanged && !shared.newConfigValidated);
        assert(shared.rtConfigChanged==rt_changed && shared.swapChainSizeChanged==swap_changed);
        shared.userConfigChanged=false;shared.fbConfigChanged=false;shared.newConfigValidated=true;
        assert(!update_resolution(app,shared,scale));
        assert(!shared.userConfigChanged && !shared.fbConfigChanged && shared.newConfigValidated);
    }
    for(int scale:{0,9}) {
        const json before_shared=shared.userConfig,before_app=app;
        bool threw=false;
        try { update_resolution(app,shared,scale); } catch(const std::runtime_error&) { threw=true; }
        assert(threw && json(shared.userConfig)==before_shared && json(app)==before_app);
        assert(!shared.userConfigChanged && !shared.fbConfigChanged && shared.newConfigValidated);
    }
}
