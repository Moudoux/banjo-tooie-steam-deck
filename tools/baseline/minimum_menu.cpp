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
    ImGui::TextWrapped("Output rate, VSync and fullscreen apply now; resolution and aspect take effect next launch.");
    bool fullscreen=prefs.fullscreen!=0;
    if(ImGui::Checkbox("Fullscreen",&fullscreen))preference(Preference::Fullscreen,int(fullscreen));
#else
    ImGui::TextWrapped("Display settings except VSync take effect next launch.");
#endif
    int resolution=prefs.resolution;
    if(ImGui::SliderInt("Resolution scale",&resolution,1,8,"%dx"))preference(Preference::Resolution,resolution);
    int rate=prefs.output_rate;
    if(ImGui::Combo("Output rate",&rate,"Default (Original)\0Original\0Display\0Manual\0"))preference(Preference::OutputRate,rate);
    if(rate==3) {
        int hz=prefs.manual_hz;if(ImGui::SliderInt("Manual rate",&hz,20,360,"%d Hz"))preference(Preference::ManualHz,hz);
    }
    int wide=prefs.wide+1;
    if(ImGui::Combo("Widescreen",&wide,"Use game setting\0Off\0On\0"))preference(Preference::Wide,wide-1);
    int aspect=prefs.native_aspect;
    if(ImGui::Combo("Gameplay aspect",&aspect,"16:9\0 21:9\0 32:9\0 43:18\0"))preference(Preference::NativeAspect,aspect);
    int cutscene=prefs.cutscene_aspect;
    if(ImGui::Combo("Cutscene aspect",&cutscene,"Original (4:3)\0Match gameplay\0"))preference(Preference::CutsceneAspect,cutscene);
    bool sync=prefs.vsync<0?render::effective_vsync():prefs.vsync!=0;
    if(ImGui::Checkbox("VSync",&sync))preference(Preference::VSync,int(sync));
    ImGui::Text("VSync: %s",status(render::vsync_status()));
    ImGui::Text("Aspect request: %s",status(render::aspect_status()));
}
}
void draw_menu(const Snapshot& snapshot) {
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
