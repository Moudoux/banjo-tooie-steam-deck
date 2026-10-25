#include "minimum_menu.hpp"
#include "guest/minimum-host/first_person_aim_abi.h"
#include "minimum_render.hpp"
#include "imgui/imgui.h"
#if BASELINE_PLAYER
#include "level_minimap.hpp"
#include "autosave.hpp"
#endif
#include <algorithm>
#include <string>

namespace baseline::host {
namespace {
void preference(Preference key, int value) {
    Request request;
    request.kind = RequestKind::Preference;
    request.preference = key;
    request.value = value;
    enqueue(std::move(request));
}
// Narrow windows shrink this tab's controls so its widest label stays visible
// beside them; wider windows keep ImGui's default width. Pair with PopItemWidth.
void push_label_room(std::initializer_list<const char*> labels) {
    float label = 0;
    for (const char* text : labels) label = std::max(label, ImGui::CalcTextSize(text).x);
    const float room = ImGui::GetContentRegionAvail().x - label - ImGui::GetStyle().ItemInnerSpacing.x;
    ImGui::PushItemWidth(std::max(1.f, std::min(ImGui::CalcItemWidth(), room)));
}
void note(const char* text) {
    ImGui::PushTextWrapPos(0.f);
    ImGui::TextDisabled("%s", text);
    ImGui::PopTextWrapPos();
}
const char* status(render::Status value) {
    switch(value) {
    case render::Status::Applied:return "Applied";
    case render::Status::Unsupported:return "Unavailable";
    default:return "Pending";
    }
}
void controls(const Snapshot& snapshot) {
    push_label_room({"Aim sensitivity", "Controller", "Rumble strength", "Controller profile",
        "Controller button labels", "Binding type", "Profile modifier"});
#if BASELINE_PLAYER && BASELINE_FIRST_PERSON_AIM
    bool aim = snapshot.preferences.first_person_aim != 0;
    if (ImGui::Checkbox("First-person aiming with right stick", &aim))
        preference(Preference::FirstPersonAim, int(aim));
    ImGui::TextWrapped("Uses the selected controller's right stick where the game supports it.");
    if (aim) {
        int sensitivity = snapshot.preferences.first_person_aim_sensitivity;
        if (ImGui::SliderInt("Aim sensitivity", &sensitivity, TOOIE_FP_AIM_SENSITIVITY_MIN,
                TOOIE_FP_AIM_SENSITIVITY_MAX, "%d%%"))
            preference(Preference::FirstPersonAimSensitivity, sensitivity);
        bool invert_x = (snapshot.preferences.first_person_aim_invert & 1) != 0;
        bool invert_y = (snapshot.preferences.first_person_aim_invert & 2) != 0;
        const bool changed_x = ImGui::Checkbox("Invert aim horizontal", &invert_x);
        const bool changed_y = ImGui::Checkbox("Invert aim vertical", &invert_y);
        if (changed_x || changed_y)
            preference(Preference::FirstPersonAimInvert, int(invert_x) | int(invert_y) << 1);
    }
#endif
#if BASELINE_PLAYER && BASELINE_TROT_TOGGLE
    bool toggle = snapshot.preferences.talon_trot_toggle != 0;
    if (ImGui::Checkbox("Talon Trot: press Z again to stop", &toggle))
        preference(Preference::TalonTrotToggle, int(toggle));
    ImGui::TextWrapped("Turning off restores hold-Z now. Turning on applies when you next enter Talon Trot.");
#endif
    SDL_JoystickID shown_controller = -1;
    std::string selected = snapshot.controller_auto ? "Automatic" : "No controller selected";
    for (const auto& pad : snapshot.controllers)
        if (pad.selected) {
            shown_controller = pad.instance;
            const auto& name = pad.friendly_name.empty() ? pad.name : pad.friendly_name;
            selected = snapshot.controller_auto ? "Automatic: " + name : name;
        }
    if (ImGui::BeginCombo("Controller", selected.c_str())) {
        if (ImGui::Selectable("Automatic", snapshot.controller_auto)) {
            Request r;
            r.kind = RequestKind::Controller;
            r.value = -1;
            enqueue(std::move(r));
        }
        for (const auto& pad : snapshot.controllers) {
            ImGui::PushID(pad.instance);
            const auto& name = pad.friendly_name.empty() ? pad.name : pad.friendly_name;
            if (ImGui::Selectable(name.c_str(), pad.selected && !snapshot.controller_auto)) {
                Request r;
                r.kind = RequestKind::Controller;
                r.value = pad.instance;
                enqueue(std::move(r));
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
#if BASELINE_PLAYER
    if (std::any_of(snapshot.controllers.begin(), snapshot.controllers.end(),
            [](const auto& pad) { return pad.selected && pad.rumble; })) {
        int strength = snapshot.preferences.rumble_strength;
        if (ImGui::SliderInt("Rumble strength", &strength, 0, 100, "%d%%"))
            preference(Preference::RumbleStrength, strength);
        ImGui::TextWrapped("Rumble strength is shared by all controllers and applies to the selected one. 0 turns rumble off.");
    }
#endif
    const bool has_controller = std::any_of(snapshot.controllers.begin(), snapshot.controllers.end(),
        [](const auto& pad) {
            return pad.selected;
        });
    ImGui::BeginDisabled(snapshot.capture.active); // The host also refuses this switch during capture.
    if (ImGui::BeginCombo("Controller profile", snapshot.active_profile.c_str())) {
        for (const auto& profile : snapshot.profiles)
            if (ImGui::Selectable(profile.c_str(), profile == snapshot.active_profile)) {
                Request r;
                r.kind = RequestKind::Profile;
                r.controller = shown_controller;
                r.name = profile;
                enqueue(std::move(r));
            }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    ImGui::BeginDisabled(!has_controller || snapshot.capture.active);
    if (ImGui::Button("Duplicate profile")) {
        Request r;
        r.kind = RequestKind::DuplicateProfile;
        r.controller = shown_controller;
        enqueue(std::move(r));
    }
    const auto& style = ImGui::GetStyle();
    if (ImGui::GetContentRegionAvail().x >= ImGui::GetItemRectSize().x + style.ItemSpacing.x +
            ImGui::CalcTextSize("Create direct-C profile").x + 2 * style.FramePadding.x)
        ImGui::SameLine();
    if (ImGui::Button("Create direct-C profile")) {
        Request r;
        r.kind = RequestKind::CreateDirectCProfile;
        r.controller = shown_controller;
        enqueue(std::move(r));
    }
    ImGui::EndDisabled();
    note("Duplicate copies this controller's current bindings into a new profile and switches to it.");
    ImGui::TextWrapped("Creates and selects a new profile with all four C directions on buttons and triggers. Existing profiles and keyboard bindings are unchanged. The right stick does not drive C buttons here; first-person aiming remains its own option.");
    using Type = tooie::input::ControllerType;
    const Type shown_type = offered_controller_labels(snapshot.controller_type) ? snapshot.controller_type : Type::Other;
    const auto type_label = [](Type type) {
        return type == Type::Other ? "Default (SDL)" : tooie::input::controller_type_name(type);
    };
    ImGui::BeginDisabled(!has_controller || snapshot.capture.active);
    if (ImGui::BeginCombo("Controller button labels", type_label(shown_type))) {
        for (auto type : {Type::Other, Type::Xbox, Type::DualSense, Type::DualShock, Type::Steam,
                Type::SteamDeck, Type::N64, Type::NintendoPro, Type::GameCube}) {
            if (!offered_controller_labels(type))
                continue;
            if (ImGui::Selectable(type_label(type), type == shown_type)) {
                Request r;
                r.kind = RequestKind::ControllerLabels;
                r.controller = shown_controller;
                r.value = int(type);
                enqueue(std::move(r));
            }
        }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    note("Changes labels only, for this controller. SDL mappings and bindings stay the same.");
#if BASELINE_PLAYER
    bool skip = snapshot.preferences.start_skips_cutscenes != 0;
    if (ImGui::Checkbox("Start skips story cutscenes", &skip))
        preference(Preference::StartSkipsCutscenes, int(skip));
    bool levitate = snapshot.preferences.levitate != 0;
    if (ImGui::Checkbox("Hold L to levitate", &levitate))
        preference(Preference::Levitate, int(levitate));
    int autosave = snapshot.preferences.autosave_minutes;
    if (ImGui::SliderInt("Autosave every", &autosave, 0, 30, autosave ? "%d min" : "Off"))
        preference(Preference::AutosaveMinutes, autosave);
#endif
    static int device = 0;
    ImGui::Combo("Binding type", &device, "Keyboard\0Controller\0");
    const bool capturable = device == 0 || has_controller;
    if (!capturable)
        ImGui::TextWrapped("Connect or select a controller to change its bindings.");
    if (snapshot.capture.active) {
        ImGui::TextWrapped("%s", snapshot.capture.arming
            ? "Release the controls, then press a new binding. Escape cancels."
            : "Press the new binding. Escape cancels.");
        ImGui::Text("%d seconds remaining", snapshot.capture.remaining_seconds);
        if (ImGui::Button("Cancel binding")) {
            Request r;
            r.kind = RequestKind::CancelCapture;
            enqueue(std::move(r));
        }
    }
    const auto binding_cell = [&](const BindingRow& row, tooie::input::Action action,
            unsigned slot, tooie::input::BindingLayer layer) {
        ImGui::TableSetColumnIndex(int(slot + 1));
        ImGui::PushID(int(slot));
        ImGui::PushID(int(layer));
        std::string label = device == 0 ? row.keyboard[slot] : std::string{};
        if (device != 0)
            for (unsigned kind = 0; kind < 3; ++kind) {
                const auto& binding = row.controller[kind * 2 + slot];
                if (binding.empty())
                    continue;
                if (!label.empty())
                    label += " / ";
                label += binding;
            }
        ImGui::BeginDisabled(!capturable);
        const float clear_width = ImGui::CalcTextSize("Clear").x + 2 * ImGui::GetStyle().FramePadding.x;
        const float binding_width = std::max(1.f,
            ImGui::GetContentRegionAvail().x - clear_width - ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::Button(label.empty() ? "Unbound" : label.c_str(), ImVec2(binding_width, 0)) && capturable) {
            Request r;
            r.kind = RequestKind::Capture;
            if (device != 0)
                r.controller = shown_controller;
            r.action = action;
            r.layer = layer;
            r.device = device == 0 ? tooie::input::Device::Keyboard : tooie::input::Device::Button;
            r.slot = slot;
            enqueue(std::move(r));
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(snapshot.capture.active);
        if (ImGui::Button("Clear", ImVec2(clear_width, 0)) && capturable) {
            Request r;
            r.kind = RequestKind::ClearBinding;
            if (device != 0)
                r.controller = shown_controller;
            r.action = action;
            r.layer = layer;
            r.device = device == 0 ? tooie::input::Device::Keyboard : tooie::input::Device::Button;
            r.slot = slot;
            enqueue(std::move(r));
        }
        ImGui::EndDisabled();
        ImGui::EndDisabled();
        ImGui::PopID();
        ImGui::PopID();
    };
    if (ImGui::BeginTable("bindings", 3,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        float action_width = ImGui::CalcTextSize("Action").x;
        for (const auto& row : snapshot.bindings)
            action_width = std::max(action_width, ImGui::CalcTextSize(row.action.c_str()).x);
#if BASELINE_PLAYER
        action_width = std::max(action_width, ImGui::CalcTextSize(snapshot.fast_forward.action.c_str()).x);
#endif
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, action_width);
        ImGui::TableSetupColumn("Primary", ImGuiTableColumnFlags_WidthStretch, 1.f);
        ImGui::TableSetupColumn("Secondary", ImGuiTableColumnFlags_WidthStretch, 1.f);
        ImGui::TableHeadersRow();
        const unsigned row_count = unsigned(snapshot.bindings.size())
#if BASELINE_PLAYER
            +1u
#endif
            ;
        for (unsigned row_index = 0; row_index < row_count; ++row_index) {
            const bool utility = row_index == snapshot.bindings.size();
            const auto action = utility ? tooie::input::Action::FastForward : tooie::input::Action(row_index);
            const auto& row = utility ? snapshot.fast_forward : snapshot.bindings[row_index];
            ImGui::PushID(int(action));
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(row.action.c_str());
            for (unsigned slot = 0; slot < 2; ++slot)
                binding_cell(row, action, slot, tooie::input::BindingLayer::Base);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    if (device != 0 && capturable) {
        const auto& modifier = snapshot.modifier;
        const auto modifier_label = modifier.code < 0 ? std::string("None")
            : controller_binding_label(modifier, snapshot.controller_type);
        ImGui::BeginDisabled(snapshot.capture.active);
        if (ImGui::BeginCombo("Profile modifier", modifier_label.c_str())) {
            const auto option = [&](tooie::input::Binding binding) {
                const auto label = binding.code < 0 ? std::string("None")
                    : controller_binding_label(binding, snapshot.controller_type);
                const bool selected = binding.device == modifier.device && binding.code == modifier.code;
                if (ImGui::Selectable(label.c_str(), selected)) {
                    Request r;
                    r.kind = RequestKind::Modifier;
                    r.controller = shown_controller;
                    r.device = binding.device;
                    r.value = binding.code;
                    enqueue(std::move(r));
                }
            };
            option({tooie::input::Device::Button, -1});
            for (int button = 0; button < SDL_CONTROLLER_BUTTON_MAX; ++button)
                option({tooie::input::Device::Button, button});
            option({tooie::input::Device::AxisPositive, SDL_CONTROLLER_AXIS_TRIGGERLEFT});
            option({tooie::input::Device::AxisPositive, SDL_CONTROLLER_AXIS_TRIGGERRIGHT});
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();
        ImGui::TextWrapped("The modifier applies to the whole controller profile. None disables modified bindings without clearing them.");
        if (modifier.code >= 0) {
            ImGui::TextWrapped("Hold the modifier first; pressing the second control first also sends it to the game. When recording, press only the second control.");
            if (snapshot.modifier_solo == tooie::input::ModifierSoloBehavior::ModifierOnly)
                ImGui::TextWrapped("This saved profile suppresses the modifier's normal action while it is held.");
            else
                ImGui::TextWrapped("The modifier's own action still reaches the game, including during the combination.");
            if (ImGui::BeginTable("modified-menu", 3,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
                const char* held_label = "Open Menu (hold modifier)";
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed,
                    std::max(ImGui::CalcTextSize("Action").x, ImGui::CalcTextSize(held_label).x));
                ImGui::TableSetupColumn("Primary", ImGuiTableColumnFlags_WidthStretch, 1.f);
                ImGui::TableSetupColumn("Secondary", ImGuiTableColumnFlags_WidthStretch, 1.f);
                ImGui::TableHeadersRow();
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(held_label);
                for (unsigned slot = 0; slot < 2; ++slot)
                    binding_cell(snapshot.held_menu, tooie::input::Action::Menu, slot, tooie::input::BindingLayer::Modified);
                ImGui::EndTable();
            }
        }
    }
#if BASELINE_PLAYER
    ImGui::TextWrapped("Hold Fast-forward for 2x speed with audio muted.");
#endif
    ImGui::TextWrapped("Escape opens and closes this menu. During binding capture it cancels.");
    if (ImGui::Button("Reset bindings")) {
        Request r;
        r.kind = RequestKind::ResetBindings;
        r.controller = shown_controller;
        enqueue(std::move(r));
    }
    ImGui::PopItemWidth();
}
void video(const Snapshot& snapshot) {
    const auto& prefs=snapshot.preferences;
    push_label_room({"Resolution scale","Output rate","Manual rate","Texture filtering","Output scaling",
        "Widescreen","Gameplay aspect","Cutscene aspect"});
#if BASELINE_PLAYER
    ImGui::TextWrapped("Resolution, output rate, texture filtering, output scaling, Widescreen, gameplay aspect, Cutscene aspect, VSync and fullscreen apply now. On/Off overrides the in-game widescreen option; Use game setting follows it.");
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
    if(ImGui::SliderInt("Draw distance",&distance,1,12,distance>1?"%dx":"Original"))preference(Preference::DrawDistance,distance);
    bool lod=prefs.disable_lod!=0;
    if(ImGui::Checkbox("Disable LOD (always full-detail models)",&lod))preference(Preference::DisableLod,int(lod));
    bool texture_lod=prefs.texture_lod!=0;
    if(ImGui::Checkbox("Sharp distant textures (scale texture LOD)",&texture_lod))preference(Preference::TextureLod,int(texture_lod));
    int upscale=prefs.upscale_2d;
    if(ImGui::Combo("2D upscaling",&upscale,"Off (original)\0Scaled 2D only\0All 2D\0"))preference(Preference::Upscale2D,upscale);
    int noise=prefs.dither_noise;
    if(ImGui::SliderInt("Dither noise",&noise,0,100,noise?"%d%%":"Off"))preference(Preference::DitherNoise,noise);
    int color=prefs.color_format;
    if(ImGui::Combo("Color precision",&color,"Standard\0High (less banding)\0Automatic\0"))preference(Preference::ColorFormat,color);
    if(color!=snapshot.latched_color_format)ImGui::TextDisabled("Color precision changes on the next launch.");
    bool minimap=prefs.minimap!=0;
    if(ImGui::Checkbox("Minimap (bottom left, F5)",&minimap))preference(Preference::Minimap,int(minimap));
#endif
    int rate=prefs.output_rate;
    if(ImGui::Combo("Output rate",&rate,"Default (Original)\0Original\0Display\0Manual\0"))preference(Preference::OutputRate,rate);
    if(rate==3) {
        int hz=prefs.manual_hz;if(ImGui::SliderInt("Manual rate",&hz,20,360,"%d Hz"))preference(Preference::ManualHz,hz);
    }
#if BASELINE_PLAYER
    int texture=prefs.texture_filter;
    if(ImGui::Combo("Texture filtering",&texture,"N64 three-point (original)\0Standard bilinear\0"))preference(Preference::TextureFilter,texture);
    int output=prefs.output_filter;
    if(ImGui::Combo("Output scaling",&output,"Pixel scaling (default)\0Nearest\0Linear\0"))preference(Preference::OutputFilter,output);
    note("Texture filtering smooths game textures; it does not add mipmaps. Output scaling changes only how the final image fits the window.");
#endif
    int wide=prefs.wide+1;
    if(ImGui::Combo("Widescreen",&wide,"Use game setting\0Off\0On\0"))preference(Preference::Wide,wide-1);
    int aspect=prefs.native_aspect;
#ifdef _WIN32
    if(ImGui::Combo("Gameplay aspect",&aspect,"16:9\0" "21:9\0" "32:9\0" "43:18\0" "Fit window (16:9\xe2\x80\x93" "32:9)\0"))preference(Preference::NativeAspect,aspect);
#else
    if(ImGui::Combo("Gameplay aspect",&aspect,"16:9\0" "21:9\0" "32:9\0" "43:18\0" "Fit window (16:10\xe2\x80\x93" "32:9)\0"))preference(Preference::NativeAspect,aspect);
#endif
    note("Gameplay aspect is used when the game is widescreen.");
    int cutscene=prefs.cutscene_aspect;
    if(ImGui::Combo("Cutscene aspect",&cutscene,"Original (4:3)\0Match gameplay\0"))preference(Preference::CutsceneAspect,cutscene);
    bool analog=prefs.analog_camera!=0;
    if(ImGui::Checkbox("Analog camera (right stick)",&analog))preference(Preference::AnalogCamera,int(analog));
    bool invert=prefs.camera_invert_x!=0;
    if(ImGui::Checkbox("Invert camera X",&invert))preference(Preference::CameraInvertX,int(invert));
    bool invert_y=prefs.camera_invert_y!=0;
    if(ImGui::Checkbox("Invert camera Y",&invert_y))preference(Preference::CameraInvertY,int(invert_y));
    if(analog) {
        int camera_distance=prefs.camera_distance;
        if(ImGui::SliderInt("Camera distance",&camera_distance,50,150,"%d%%"))
            preference(Preference::CameraDistance,camera_distance);
    }
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
    ImGui::PopItemWidth();
}
#if BASELINE_PLAYER
void audio(const Snapshot& snapshot) {
    int master_volume = snapshot.preferences.master_volume;
    if (ImGui::SliderInt("Master volume", &master_volume, 0, 100, "%d%%"))
        preference(Preference::MasterVolume, master_volume);
    ImGui::TextWrapped("Scales all game audio, including music and sound effects. "
        "Music volume below applies on top of this setting. Applies to the next audio the game sends.");
    int music_volume = snapshot.preferences.music_volume;
    if (ImGui::SliderInt("Music volume", &music_volume, 0, 100, "%d%%"))
        preference(Preference::MusicVolume, music_volume);
    ImGui::TextWrapped("Scales sequenced music, including jingles and cues. "
        "Positional sound effects are unaffected. The game requests the new volume on its next update. "
        "If its audio queue is full, later music-volume requests still use this setting.");
#if BASELINE_QUIET_TROT
    bool quiet=snapshot.preferences.quiet_talon_trot!=0;
    if(ImGui::Checkbox("Mute Talon Trot movement sound",&quiet))
        preference(Preference::QuietTalonTrot,int(quiet));
    ImGui::TextWrapped("Applies on the next movement sound. Long-legged and stilt movement sounds are unchanged.");
#endif
}
#endif
}
void draw_menu(const Snapshot& snapshot) {
#if BASELINE_PLAYER
    tooie::level_minimap::render();
    tooie::autosave::render();
#endif
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
    // A permanent scrollbar keeps tab content width fixed: an automatic one is
    // chosen from the previous frame and resizes stretch columns one frame late.
    if(ImGui::Begin("Banjo-Tooie settings",&open,ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
        const auto position=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();
        const ImVec2 lo(viewport->WorkPos.x+margin.x,viewport->WorkPos.y+margin.y);
        const ImVec2 hi(std::max(lo.x,lo.x+available.x-size.x),std::max(lo.y,lo.y+available.y-size.y));
        ImGui::SetWindowPos(ImVec2(std::clamp(position.x,lo.x,hi.x),std::clamp(position.y,lo.y,hi.y)));
        if(!snapshot.notice.empty())ImGui::TextWrapped("%s",snapshot.notice.c_str());
        if(ImGui::BeginTabBar("settings")) {
            if(ImGui::BeginTabItem("Controls")){controls(snapshot);ImGui::EndTabItem();}
            if(ImGui::BeginTabItem("Video")){video(snapshot);ImGui::EndTabItem();}
#if BASELINE_PLAYER
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
