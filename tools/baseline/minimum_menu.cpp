#include "minimum_menu.hpp"
#include "minimum_render.hpp"
#include "imgui/imgui.h"
#include <algorithm>
#include <string>

namespace baseline::host {
namespace {
void preference(Preference key,int value) {
    Request request;request.kind=RequestKind::Preference;request.preference=key;request.value=value;enqueue(std::move(request));
}
const char* status(render::Status value) {
    switch(value) {
    case render::Status::Applied:return "Applied";
    case render::Status::Unsupported:return "Unavailable";
    default:return "Pending";
    }
}
void controls(const Snapshot& snapshot) {
    const char* selected="Automatic";
    for(const auto& pad:snapshot.controllers)if(pad.selected)selected=pad.friendly_name.empty()?pad.name.c_str():pad.friendly_name.c_str();
    if(ImGui::BeginCombo("Controller",selected)) {
        if(ImGui::Selectable("Automatic")){Request r;r.kind=RequestKind::Controller;r.value=-1;enqueue(std::move(r));}
        for(const auto& pad:snapshot.controllers) {
            ImGui::PushID(pad.instance);
            const auto& name=pad.friendly_name.empty()?pad.name:pad.friendly_name;
            if(ImGui::Selectable(name.c_str(),pad.selected)){Request r;r.kind=RequestKind::Controller;r.value=pad.instance;enqueue(std::move(r));}
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    if(ImGui::BeginCombo("Controller profile",snapshot.active_profile.c_str())) {
        for(const auto& profile:snapshot.profiles)if(ImGui::Selectable(profile.c_str(),profile==snapshot.active_profile)) {
            Request r;r.kind=RequestKind::Profile;r.name=profile;enqueue(std::move(r));
        }
        ImGui::EndCombo();
    }
    static int device=0;
    ImGui::Combo("Binding type",&device,"Keyboard\0Controller\0");
    const bool capturable=device==0 || std::any_of(snapshot.controllers.begin(),snapshot.controllers.end(),[](const auto& pad){return pad.selected;});
    if(!capturable)ImGui::TextUnformatted("Connect or select a controller to change its bindings.");
    if(snapshot.capture.active) {
        ImGui::TextUnformatted(snapshot.capture.arming?"Release the controls, then press a new binding. Escape cancels.":"Press the new binding. Escape cancels.");
        ImGui::Text("%d seconds remaining",snapshot.capture.remaining_seconds);
        if(ImGui::Button("Cancel binding")){Request r;r.kind=RequestKind::CancelCapture;enqueue(std::move(r));}
    }
    if(ImGui::BeginTable("bindings",3,ImGuiTableFlags_Borders|ImGuiTableFlags_RowBg|ImGuiTableFlags_SizingStretchProp)) {
        float action_width=ImGui::CalcTextSize("Action").x;
        for(const auto& row:snapshot.bindings)action_width=std::max(action_width,ImGui::CalcTextSize(row.action.c_str()).x);
        ImGui::TableSetupColumn("Action",ImGuiTableColumnFlags_WidthFixed,action_width);ImGui::TableSetupColumn("Primary",ImGuiTableColumnFlags_WidthStretch,1.f);ImGui::TableSetupColumn("Secondary",ImGuiTableColumnFlags_WidthStretch,1.f);ImGui::TableHeadersRow();
        for(unsigned action=0;action<snapshot.bindings.size();++action) {
            const auto& row=snapshot.bindings[action];ImGui::PushID(int(action));ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);ImGui::TextUnformatted(row.action.c_str());
            for(unsigned slot=0;slot<2;++slot) {
                ImGui::TableSetColumnIndex(int(slot+1));ImGui::PushID(int(slot));
                std::string label=device==0?row.keyboard[slot]:std::string{};
                if(device!=0)for(unsigned kind=0;kind<3;++kind) {
                    const auto& binding=row.controller[kind*2+slot];
                    if(binding.empty() || binding=="Unbound")continue;
                    if(!label.empty())label+=" / ";
                    label+=binding;
                }
                ImGui::BeginDisabled(!capturable);
                if(ImGui::Button(label.empty()?"Unbound":label.c_str(),ImVec2(-1,0)) && capturable) {
                    Request r;r.kind=RequestKind::Capture;r.action=tooie::input::Action(action);
                    r.device=device==0?tooie::input::Device::Keyboard:tooie::input::Device::Button;r.slot=slot;enqueue(std::move(r));
                }
                ImGui::EndDisabled();
                ImGui::PopID();
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if(ImGui::Button("Reset bindings")){Request r;r.kind=RequestKind::ResetBindings;enqueue(std::move(r));}
}
void video(const Snapshot& snapshot) {
    const auto& prefs=snapshot.preferences;
#if BASELINE_PLAYER
    ImGui::TextWrapped("Resolution, output rate, VSync and fullscreen apply now; aspect takes effect next launch.");
    bool fullscreen=prefs.fullscreen!=0;
    if(ImGui::Checkbox("Fullscreen",&fullscreen))preference(Preference::Fullscreen,int(fullscreen));
#else
    ImGui::TextWrapped("Display settings except VSync take effect next launch.");
#endif
    int resolution=prefs.resolution;
    if(ImGui::SliderInt("Resolution scale",&resolution,0,8,resolution?"%dx":"Auto (fit window)"))preference(Preference::Resolution,resolution);
#if BASELINE_PLAYER
    // MSAA levels the GPU supports (render::max_msaa); unsupported levels are not offered.
    const int msaa_max=render::max_msaa();
    if(msaa_max>=2) {
        int msaa_index=prefs.msaa>=8?3:prefs.msaa>=4?2:prefs.msaa>=2?1:0;
        const char* msaa_items=msaa_max>=8?"Off\0 2x\0 4x\0 8x\0":msaa_max>=4?"Off\0 2x\0 4x\0":"Off\0 2x\0";
        if(ImGui::Combo("Anti-aliasing (MSAA)",&msaa_index,msaa_items))
            preference(Preference::Msaa,msaa_index?1<<msaa_index:0);
    } else ImGui::TextDisabled("Anti-aliasing (MSAA): not supported by this GPU");
    int ss_index=prefs.supersampling>=4?2:prefs.supersampling>=2?1:0;
    if(ImGui::Combo("Supersampling",&ss_index,"Off\0 2x\0 4x\0"))preference(Preference::Supersampling,1<<ss_index);
    if(prefs.supersampling>1) {
        const int factor=render::effective_supersampling(prefs.resolution,prefs.supersampling);
        if(prefs.resolution==0)ImGui::TextDisabled("Supersampling needs a fixed resolution scale.");
        else if(factor<prefs.supersampling)ImGui::TextDisabled("Limited to %dx: the internal scale stops at 8x.",factor);
    }
    int distance=prefs.draw_distance;
    if(ImGui::SliderInt("Draw distance",&distance,1,6,distance>1?"%dx":"Original"))preference(Preference::DrawDistance,distance);
    bool lod=prefs.disable_lod!=0;
    if(ImGui::Checkbox("Disable LOD (always full-detail models)",&lod))preference(Preference::DisableLod,int(lod));
#endif
    int rate=prefs.output_rate;
    if(ImGui::Combo("Output rate",&rate,"Default (Original)\0Original\0Display\0Manual\0"))preference(Preference::OutputRate,rate);
    if(rate==3) {
        int hz=prefs.manual_hz;if(ImGui::SliderInt("Manual rate",&hz,20,360,"%d Hz"))preference(Preference::ManualHz,hz);
    }
    int wide=prefs.wide+1;
    if(ImGui::Combo("Widescreen",&wide,"Use game setting\0Off\0On\0"))preference(Preference::Wide,wide-1);
    int aspect=prefs.native_aspect;
    if(ImGui::Combo("Gameplay aspect",&aspect,"16:9\0 21:9\0 32:9\0 43:18\0 16:10\0"))preference(Preference::NativeAspect,aspect);
    int cutscene=prefs.cutscene_aspect;
    if(ImGui::Combo("Cutscene aspect",&cutscene,"Original (4:3)\0Match gameplay\0"))preference(Preference::CutsceneAspect,cutscene);
    bool analog=prefs.analog_camera!=0;
    if(ImGui::Checkbox("Analog camera (right stick)",&analog))preference(Preference::AnalogCamera,int(analog));
    bool invert=prefs.camera_invert_x!=0;
    if(ImGui::Checkbox("Invert camera X",&invert))preference(Preference::CameraInvertX,int(invert));
    bool invert_y=prefs.camera_invert_y!=0;
    if(ImGui::Checkbox("Invert camera Y",&invert_y))preference(Preference::CameraInvertY,int(invert_y));
#if BASELINE_PLAYER
    bool mouse=prefs.mouse_aim!=0;
    if(ImGui::Checkbox("Mouse / gyro aiming (first person)",&mouse))preference(Preference::MouseAim,int(mouse));
    if(mouse) {
        float mouse_speed=prefs.mouse_sensitivity/100.0f;
        if(ImGui::SliderFloat("Aim sensitivity",&mouse_speed,0.01f,1.0f,"%.2f deg"))
            preference(Preference::MouseSensitivity,int(mouse_speed*100.0f+0.5f));
        bool mouse_invert=prefs.mouse_invert_y!=0;
        if(ImGui::Checkbox("Invert aim Y",&mouse_invert))preference(Preference::MouseInvertY,int(mouse_invert));
        ImGui::TextWrapped("Steam Deck gyro: in the Steam controller layout, set the gyro to output as mouse.");
    }
#endif
    bool sync=prefs.vsync<0?render::effective_vsync():prefs.vsync!=0;
    if(ImGui::Checkbox("VSync",&sync))preference(Preference::VSync,int(sync));
    ImGui::Text("VSync: %s",status(render::vsync_status()));
    ImGui::Text("Aspect request: %s",status(render::aspect_status()));
}
#if BASELINE_PLAYER && BASELINE_QUIET_TROT
void audio(const Snapshot& snapshot) {
    bool quiet=snapshot.preferences.quiet_talon_trot!=0;
    if(ImGui::Checkbox("Mute Talon Trot movement sound",&quiet))
        preference(Preference::QuietTalonTrot,int(quiet));
    ImGui::TextWrapped("Applies on the next movement sound. Long-legged and stilt movement sounds are unchanged.");
}
#endif
}
void draw_menu(const Snapshot& snapshot) {
    if(!snapshot.toast.empty()) {
        const auto* viewport=ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x+12.f,viewport->WorkPos.y+viewport->WorkSize.y-12.f),ImGuiCond_Always,ImVec2(0,1));
        if(ImGui::Begin("##issue-marker-toast",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|
            ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoSavedSettings))
            ImGui::TextUnformatted(snapshot.toast.c_str());
        ImGui::End();
    }
    if(!snapshot.menu)return;
    const auto* viewport=ImGui::GetMainViewport();
    const ImVec2 margin(std::min(12.f,std::max(0.f,(viewport->WorkSize.x-1.f)/2.f)),
                        std::min(12.f,std::max(0.f,(viewport->WorkSize.y-1.f)/2.f)));
    const ImVec2 available(std::max(1.f,viewport->WorkSize.x-2*margin.x),
                           std::max(1.f,viewport->WorkSize.y-2*margin.y));
    ImGui::SetNextWindowSizeConstraints(ImVec2(1,1),available);
    ImGui::SetNextWindowSize(ImVec2(std::min(720.f,available.x),std::min(620.f,available.y)),ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x+margin.x,viewport->WorkPos.y+margin.y),ImGuiCond_FirstUseEver);
    bool open=true;
    if(ImGui::Begin("Banjo-Tooie settings",&open,ImGuiWindowFlags_NoCollapse)) {
        const auto position=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();
        const ImVec2 lo(viewport->WorkPos.x+margin.x,viewport->WorkPos.y+margin.y);
        const ImVec2 hi(std::max(lo.x,lo.x+available.x-size.x),std::max(lo.y,lo.y+available.y-size.y));
        ImGui::SetWindowPos(ImVec2(std::clamp(position.x,lo.x,hi.x),std::clamp(position.y,lo.y,hi.y)));
        if(!snapshot.notice.empty())ImGui::TextWrapped("%s",snapshot.notice.c_str());
        if(ImGui::BeginTabBar("settings")) {
            if(ImGui::BeginTabItem("Controls")){controls(snapshot);ImGui::EndTabItem();}
            if(ImGui::BeginTabItem("Video")){video(snapshot);ImGui::EndTabItem();}
#if BASELINE_PLAYER && BASELINE_QUIET_TROT
            if(ImGui::BeginTabItem("Audio")){audio(snapshot);ImGui::EndTabItem();}
#endif
            ImGui::EndTabBar();
        }
        ImGui::Separator();
        if(ImGui::Button("Resume"))request_close();
        ImGui::SameLine();if(ImGui::Button("Quit"))request_quit();
    }
    ImGui::End();
    if(!open)request_close();
}
}
