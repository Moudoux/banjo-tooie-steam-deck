#pragma once
#if BASELINE_CAMERA_IDENTITY
#include <array>
#include "demo_timing_layout.h"
#include "shadow_projection_layout.h"
#include "pre_background_layout.h"
#include "cpu625_selection_layout.h"
#include "minimum_host_selection_layout.h"
static_assert(TOOIE_MINIMUM_HOST_SELECTED==TOOIE_BASELINE_HOST,"Native/guest HOST selection mismatch");
#include "npc665_selection_layout.h"
#include "backpack621_selection_layout.h"
#include "trot_toggle_selection_layout.h"
#include "modern_fps_selection_layout.h"
#include "flap_flip_selection_layout.h"
#include "ckcopy_selection_layout.h"
static_assert(TOOIE_FLAP_FLIP_SELECTED==BASELINE_FLAP_FLIP,"Native/guest landing selection mismatch");
static_assert(TOOIE_MODERN_FPS_SELECTED==BASELINE_FIRST_PERSON_AIM,"Native/guest FPS selection mismatch");
#include "bk_pipeline_selection_layout.h"
// The BK pipeline retires the P04b redirect in the current player composition.
static_assert(TOOIE_CKCOPY_SELECTED==(TOOIE_CPU625_SELECTED && BASELINE_FLAP_FLIP && !TOOIE_BK_PIPELINE_SELECTED),"Native/guest ckpack copy-storage selection mismatch");
#include "secondary_selection_layout.h"
static_assert(TOOIE_PLAYER_SECONDARY_SELECTED==(TOOIE_CPU625_SELECTED && BASELINE_FLAP_FLIP),"Native/guest secondary route selection mismatch");
static_assert(TOOIE_PLAYER_SECONDARY_ADMIT==0u || TOOIE_PLAYER_SECONDARY_SELECTED==1u,"Secondary admission requires the secondary route selection");
#include "bk_pipeline_selection_layout.h"
// BK pipeline packet1 joins the current player composition; its 9 helpers join section2 like the secondary helpers.
static_assert((!TOOIE_BK_PIPELINE_SELECTED || TOOIE_PLAYER_SECONDARY_SELECTED) && TOOIE_BK_HELPERS==29u,"Native/guest BK pipeline selection mismatch");
// The fixed-section fallbacks below hard-code section2's six primary helpers; the five secondary helpers join section2 only with measured shared-section tuples.
static_assert(!TOOIE_PLAYER_SECONDARY_SELECTED || TOOIE_NPC665_SELECTED || TOOIE_BACKPACK621_SELECTED,"Secondary section2 growth needs measured shared sections");
constexpr size_t camera_alias_section_count=3+BASELINE_FIRST_PERSON_AIM+BASELINE_FLAP_FLIP;
static_assert(TOOIE_TROT_SELECTED==BASELINE_TROT_TOGGLE,"Native/guest Trot selection mismatch");
#if TOOIE_SHADOW_SELECTED
// Eight shadow helpers plus the coefficient route's MODELVIEW close (generated TOOIE_SHADOW_MV).
static_assert(TOOIE_SHADOW_MV==0u || TOOIE_SHADOW_MV==1u,"Explicit generated shadow modelview selection");
#ifdef TOOIE_SHADOW_HELPERS
// BK selection: six helpers (source/prepare retired; no MODELVIEW close).
static_assert(TOOIE_SHADOW_HELPERS==6u && TOOIE_SHADOW_MV==0u,"BK shadow helper set");
constexpr size_t camera_shadow_helper_count=TOOIE_SHADOW_HELPERS;
#else
constexpr size_t camera_shadow_helper_count=8u+TOOIE_SHADOW_MV;
#endif
#else
constexpr size_t camera_shadow_helper_count=0u;
#endif
// CKCOPY adds its 4 helpers and, in the CPU625 section, the 2 N3 attachment helpers; the secondary route adds its 5 helpers to section2.
constexpr size_t camera_patch_count = (TOOIE_SHADOW_SELECTED ? 53+camera_shadow_helper_count : TOOIE_DEMO_TIMING_SELECTED ? 53 : 50)+(TOOIE_PRE_SELECTED?3:0)+(TOOIE_ACTOR_PALETTE_SELECTED?4:0)+(TOOIE_CPU625_SELECTED?4:0)+(TOOIE_MINIMUM_HOST_SELECTED?7:0)+(TOOIE_NPC665_SELECTED?8:0)+(TOOIE_BACKPACK621_SELECTED?9:0)+BASELINE_QUIET_TROT+(TOOIE_TROT_BODIES+TOOIE_TROT_HELPERS)*BASELINE_TROT_TOGGLE+(TOOIE_MODERN_FPS_BODIES+TOOIE_MODERN_FPS_HELPERS)*BASELINE_FIRST_PERSON_AIM+BASELINE_FLAP_FLIP+(TOOIE_CKCOPY_SELECTED?6:0)+(TOOIE_PLAYER_SECONDARY_SELECTED?5:0)+(TOOIE_BK_PIPELINE_SELECTED?TOOIE_BK_HELPERS:0);
#if TOOIE_PRE_SELECTED
// Independent selected-body expectation, never derived from the table being checked.
constexpr size_t camera_pre_replacement_count =
    (TOOIE_ACTOR_PALETTE_SELECTED ? 39u : 38u) +
    3u * TOOIE_MINIMUM_HOST_SELECTED + TOOIE_BACKPACK621_SELECTED + BASELINE_QUIET_TROT +
    TOOIE_TROT_BODIES * BASELINE_TROT_TOGGLE +
    TOOIE_MODERN_FPS_BODIES * BASELINE_FIRST_PERSON_AIM + BASELINE_FLAP_FLIP;
#endif
// Registry identity only. Guest leaves retain all camera semantic policy.
namespace {
struct CameraAlias {
    recomp_func_t* func;
    uint32_t rom,ram,section_size,offset,extent;
};
struct CameraPatchExpected { recomp_func_t* func; size_t section; };
#include "camera_registry_data.hpp"
struct CameraResident { recomp_func_t* func; uint32_t address; };
std::array<CameraResident,camera_patch_count> camera_resident{};
bool camera_registry_installed = false;
// Table validation and alias installation run only inside register_boot_patch.
void camera_validate_patch_table() {
    const auto& sections=baseline_boot_patch_registry::section_table;
#if TOOIE_PRE_SELECTED
    if(camera_actor_palette_selected>1 || camera_actor_palette_selected!=TOOIE_ACTOR_PALETTE_SELECTED || !TOOIE_DEMO_TIMING_SELECTED || !TOOIE_SHADOW_SELECTED || std::size(sections)!=6+TOOIE_DEMO_TIMING_SELECTED+TOOIE_SHADOW_SELECTED+TOOIE_CPU625_SELECTED+TOOIE_MINIMUM_HOST_SELECTED+TOOIE_NPC665_SELECTED+TOOIE_BACKPACK621_SELECTED+BASELINE_TROT_TOGGLE+BASELINE_FIRST_PERSON_AIM+TOOIE_CKCOPY_SELECTED || std::size(camera_aliases)!=27+3*TOOIE_MINIMUM_HOST_SELECTED+TOOIE_BACKPACK621_SELECTED+TOOIE_MODERN_FPS_BODIES*BASELINE_FIRST_PERSON_AIM+BASELINE_FLAP_FLIP || std::size(camera_expected_patches)!=camera_patch_count)
        rejected_registration("PRE selection exact provider/alias/section counts");
    if (sections[0].rom_addr != 0 || sections[0].ram_addr != 0x80801000 || sections[0].index != 1 ||
        sections[0].num_funcs != camera_pre_replacement_count ||
        sections[0].size != TOOIE_PRE_TEXT_SIZE || sections[0].num_relocs != TOOIE_PRE_TEXT_RELOCS)
        rejected_registration("PRE fixed section0 identity");
#if TOOIE_BACKPACK621_SELECTED
    for(size_t i=0;i<5;i++) {const auto& a=sections[i];const auto& e=backpack621_shared_sections[i];
        if(a.rom_addr!=e[0] || a.ram_addr!=e[1] || a.size!=e[2] || a.index!=e[3] || a.num_funcs!=e[4] || a.num_relocs!=e[5])rejected_registration("Backpack621 exact changed shared section");}
#elif TOOIE_NPC665_SELECTED
    for(size_t i=0;i<5;i++) {const auto& a=sections[i];const auto& e=npc665_shared_sections[i];
        if(a.rom_addr!=e[0] || a.ram_addr!=e[1] || a.size!=e[2] || a.index!=e[3] || a.num_funcs!=e[4] || a.num_relocs!=e[5])rejected_registration("NPC665 exact changed shared section");}
#else
    constexpr uint32_t fixed[4][6]={{0x96C8,0x8080A6C8,0x2E38,2,1,2},{0xC500,0x8080D500,0x15FC,3,6,3},{0xDAFC,0x8080EAFC,camera_actor_palette_selected?0xD10u:0x68Cu,4,camera_actor_palette_selected?4u:3u,0},{camera_actor_palette_selected?0xE820u:0xE19Cu,camera_actor_palette_selected?0x8080F820u:0x8080F19Cu,camera_actor_palette_selected?0x3DB4u:0x20F0u,8,camera_actor_palette_selected?4u:2u,camera_actor_palette_selected?6u:4u}};
    for(size_t i=1;i<5;i++) {const auto& a=sections[i];const auto& e=fixed[i-1];
        if(a.rom_addr!=e[0]+TOOIE_PRE_SHIFT || a.ram_addr!=e[1]+TOOIE_PRE_SHIFT || a.size!=e[2] || a.index!=e[3] || a.num_funcs!=e[4] || a.num_relocs!=e[5])rejected_registration("PRE fixed section identity/uniform shift");}
#endif
#else
    if (camera_actor_palette_selected>1 || camera_actor_palette_selected!=TOOIE_ACTOR_PALETTE_SELECTED || std::size(sections)!=(TOOIE_SHADOW_SELECTED?7:TOOIE_DEMO_TIMING_SELECTED?6:5) || sections[0].rom_addr!=0 || sections[0].ram_addr!=0x80801000 ||
        sections[0].size!=0x96C8 || sections[0].index!=1 || sections[0].num_funcs!=38 || sections[0].num_relocs!=187 ||
        sections[1].rom_addr!=0x96C8 || sections[1].ram_addr!=0x8080A6C8 || sections[1].size!=0x2E38 ||
        sections[1].index!=2 || sections[1].num_funcs!=1 || sections[1].num_relocs!=2 ||
        sections[2].rom_addr!=0xC500 || sections[2].ram_addr!=0x8080D500 || sections[2].size!=0x15FC ||
        sections[2].index!=3 || sections[2].num_funcs!=6 || sections[2].num_relocs!=3 ||
        sections[3].rom_addr!=0xDAFC || sections[3].ram_addr!=0x8080EAFC || sections[3].size!=(camera_actor_palette_selected?0x9E8:0x68C) ||
        sections[3].index!=4 || sections[3].num_funcs!=3 || sections[3].num_relocs!=0 ||
        sections[4].rom_addr!=(camera_actor_palette_selected?0xE4F8:0xE19C) || sections[4].ram_addr!=(camera_actor_palette_selected?0x8080F4F8:0x8080F19C) || sections[4].size!=0x20F0 ||
        sections[4].index!=8 || sections[4].num_funcs!=2 || sections[4].num_relocs!=4 ||
        std::size(camera_aliases)!=27 || std::size(camera_expected_patches)!=camera_patch_count)
        rejected_registration("camera/helper/player exact registry section/count identity");
#endif
#if TOOIE_DEMO_TIMING_SELECTED
    if(sections[5].rom_addr!=TOOIE_DEMO_ROM || sections[5].ram_addr!=TOOIE_DEMO_RAM ||
       sections[5].size!=TOOIE_DEMO_SIZE || sections[5].index!=TOOIE_DEMO_INDEX ||
       sections[5].num_funcs!=3 || sections[5].num_relocs!=TOOIE_DEMO_RELOCS)
        rejected_registration("demo timing exact compiled helper section");
#endif
#if TOOIE_SHADOW_SELECTED
    if(!TOOIE_DEMO_TIMING_SELECTED || (camera_actor_palette_selected && !TOOIE_PRE_SELECTED) || sections[6].rom_addr!=TOOIE_SHADOW_ROM ||
       sections[6].ram_addr!=TOOIE_SHADOW_RAM || sections[6].size!=TOOIE_SHADOW_SIZE || sections[6].index!=TOOIE_SHADOW_INDEX ||
       sections[6].num_funcs!=camera_shadow_helper_count || sections[6].num_relocs!=TOOIE_SHADOW_RELOCS)
        rejected_registration("shadow exact compiled helper section");
#endif
#if TOOIE_PRE_SELECTED
    {const auto& p=sections[TOOIE_PRE_SECTION];
     if(p.rom_addr!=TOOIE_PRE_ROM || p.ram_addr!=TOOIE_PRE_RAM || p.size!=TOOIE_PRE_SIZE || p.index!=TOOIE_PRE_INDEX || p.num_funcs!=3 || p.num_relocs!=TOOIE_PRE_RELOCS)rejected_registration("PRE exact compiled helper section");}
#endif
#if TOOIE_CPU625_SELECTED
    {const auto& p=sections[TOOIE_CPU625_SECTION];
     if(!TOOIE_PRE_SELECTED || !TOOIE_ACTOR_PALETTE_SELECTED || p.rom_addr!=TOOIE_CPU625_ROM ||
        p.ram_addr!=TOOIE_CPU625_RAM || p.size!=TOOIE_CPU625_SIZE || p.index!=TOOIE_CPU625_INDEX ||
        p.num_funcs!=4u+2u*TOOIE_CKCOPY_SELECTED || p.num_relocs!=TOOIE_CPU625_RELOCS)
         rejected_registration("CPU625 exact compiled helper section");}
#endif
    #if TOOIE_MINIMUM_HOST_SELECTED
    {const auto& p=sections[TOOIE_MINIMUM_HOST_SECTION];
     if(TOOIE_MINIMUM_HOST_MUSIC_BYTES!=20 || !TOOIE_PRE_SELECTED || !TOOIE_ACTOR_PALETTE_SELECTED || p.rom_addr!=TOOIE_MINIMUM_HOST_ROM || p.ram_addr!=TOOIE_MINIMUM_HOST_RAM || p.size!=TOOIE_MINIMUM_HOST_SIZE || p.index!=TOOIE_MINIMUM_HOST_INDEX || p.num_funcs!=4 || p.num_relocs!=TOOIE_MINIMUM_HOST_RELOCS)rejected_registration("HOST exact compiled helper section");}
    #endif
#if TOOIE_NPC665_SELECTED
    {const auto& p=sections[TOOIE_NPC665_SECTION];
     if(!TOOIE_PRE_SELECTED || !TOOIE_ACTOR_PALETTE_SELECTED || p.rom_addr!=TOOIE_NPC665_ROM ||
        p.ram_addr!=TOOIE_NPC665_RAM || p.size!=TOOIE_NPC665_SIZE || p.index!=TOOIE_NPC665_INDEX ||
        p.num_funcs!=8 || p.num_relocs!=TOOIE_NPC665_RELOCS)
         rejected_registration("NPC665 exact eight compiled helpers");}
#endif
#if TOOIE_BACKPACK621_SELECTED
    {const auto& p=sections[TOOIE_BACKPACK621_SECTION];
     if(!TOOIE_CPU625_SELECTED || !TOOIE_PRE_SELECTED || !TOOIE_ACTOR_PALETTE_SELECTED ||
        p.rom_addr!=TOOIE_BACKPACK621_ROM || p.ram_addr!=TOOIE_BACKPACK621_RAM ||
        p.size!=TOOIE_BACKPACK621_SIZE || p.index!=TOOIE_BACKPACK621_INDEX ||
        p.num_funcs!=8 || p.num_relocs!=TOOIE_BACKPACK621_RELOCS)
         rejected_registration("Backpack621 exact eight compiled helpers");}
#endif
#if BASELINE_TROT_TOGGLE
    {const auto& p=sections[TOOIE_TROT_SECTION];
     if(!TOOIE_BACKPACK621_SELECTED || !TOOIE_MINIMUM_HOST_SELECTED || !BASELINE_QUIET_TROT ||
        p.rom_addr!=TOOIE_TROT_ROM || p.ram_addr!=TOOIE_TROT_RAM || p.size!=TOOIE_TROT_SIZE ||
        p.index!=TOOIE_TROT_INDEX || p.num_funcs!=TOOIE_TROT_HELPERS || p.num_relocs!=TOOIE_TROT_RELOCS ||
        TOOIE_TROT_SLOT_SIZE!=12 || TOOIE_TROT_SLOT_ADDRESS!=p.ram_addr+p.size ||
        uint64_t(TOOIE_TROT_SLOT_ADDRESS)+12>0x80801000ull+baseline_boot_patch_bin_size)
         rejected_registration("Trot exact two helpers/slot/selection");}
#endif
#if BASELINE_FIRST_PERSON_AIM
    {const auto& section=sections[TOOIE_MODERN_FPS_SECTION];
     if(!BASELINE_TROT_TOGGLE || !TOOIE_MINIMUM_HOST_SELECTED ||
        section.rom_addr!=TOOIE_MODERN_FPS_ROM || section.ram_addr!=TOOIE_MODERN_FPS_RAM ||
        section.size!=TOOIE_MODERN_FPS_SIZE || section.index!=TOOIE_MODERN_FPS_INDEX ||
        section.num_funcs!=TOOIE_MODERN_FPS_HELPERS || section.num_relocs!=TOOIE_MODERN_FPS_RELOCS ||
        TOOIE_MODERN_FPS_STATE_BYTES!=28 || TOOIE_MODERN_FPS_STATE!=section.ram_addr+section.size ||
#if TOOIE_CKCOPY_SELECTED
        ((TOOIE_MODERN_FPS_IMAGE_END+3u)&~3u)!=TOOIE_CKCOPY_RAM)
#else
        TOOIE_MODERN_FPS_IMAGE_END!=0x80801000ull+baseline_boot_patch_bin_size)
#endif
         rejected_registration("FPS exact helper/state/image extent");}
#endif
#if TOOIE_CKCOPY_SELECTED
    {const auto& p=sections[TOOIE_CKCOPY_SECTION];
     if(!TOOIE_CPU625_SELECTED || !BASELINE_FIRST_PERSON_AIM || p.rom_addr!=TOOIE_CKCOPY_ROM ||
        p.ram_addr!=TOOIE_CKCOPY_RAM || p.size!=TOOIE_CKCOPY_SIZE || p.index!=TOOIE_CKCOPY_INDEX ||
        p.num_funcs!=4 || p.num_relocs!=TOOIE_CKCOPY_RELOCS || TOOIE_CKCOPY_STATE_BYTES!=122960u ||
        TOOIE_CKCOPY_STATE<p.ram_addr+p.size ||
        TOOIE_CKCOPY_IMAGE_END!=0x80801000ull+baseline_boot_patch_bin_size)
         rejected_registration("ckpack copy storage exact helpers/state/image extent");}
#endif
    size_t resident=0;
    for(size_t s=0;s<std::size(sections);++s) {
        const auto& section=sections[s];
        if(!section.funcs || uint64_t(section.ram_addr)+section.size>0x81000000ull)
            rejected_registration("camera patch section bounds/relocations");
        for(size_t i=0;i<section.num_funcs;++i) {
            const auto& f=section.funcs[i];
            if(!f.func || !f.rom_size || uint64_t(f.offset)+f.rom_size>section.size ||
                (i && uint64_t(section.funcs[i-1].offset)+section.funcs[i-1].rom_size>f.offset))
                rejected_registration("camera patch function bounds/overlap");
            size_t matches=0;
            for(const auto& expected:camera_expected_patches) if(expected.func==f.func) {
                if(expected.section!=s) rejected_registration("camera helper section ownership");
                ++matches;
            }
            if(matches!=1) rejected_registration("camera unexpected/duplicate provider identity");
            for(size_t j=0;j<resident;++j) if(camera_resident[j].func==f.func)
                rejected_registration("camera provider appears more than once");
            camera_resident[resident++]={f.func,section.ram_addr+f.offset};
        }
    }
#if TOOIE_NPC665_SELECTED
    size_t end_matches=0;
    for(size_t i=0;i<resident;i++) if(camera_resident[i].address==TOOIE_NPC665_END) {
        // The checked path is the absolute import; the table keeps the guest end itself.
        if(camera_resident[i].func!=baseline_boot_patch_registry::tooie_npc665_end)rejected_registration("NPC665 guest end provider");
        ++end_matches;
    }
    if(end_matches!=1)rejected_registration("NPC665 unique end address");
#endif
    if(resident!=camera_patch_count) rejected_registration("camera complete provider membership");
}
void camera_register_aliases(SectionTableEntry* sections,size_t count) {
    if(camera_registry_installed) rejected_registration("camera registry installed twice");
    camera_validate_patch_table();
    static std::array<std::vector<FuncEntry>,camera_alias_section_count> owned;
    size_t modified=0,added=0;
    for(size_t s=0;s<count;++s) {
        auto& section=sections[s];bool selected=false;
        for(const auto& alias:camera_aliases) if(alias.rom==section.rom_addr) selected=true;
        if(!selected) continue;
        if(modified==owned.size()) rejected_registration("camera alias section count");
        auto& entries=owned[modified++];entries.assign(section.funcs,section.funcs+section.num_funcs);
        for(const auto& alias:camera_aliases) if(alias.rom==section.rom_addr) {
            if(section.ram_addr!=alias.ram || section.size!=alias.section_size ||
                uint64_t(alias.offset)+alias.extent>section.size ||
                std::any_of(entries.begin(),entries.end(),[&](const auto& e){return e.offset==alias.offset;}))
                rejected_registration("camera original alias not omitted or section differs");
            entries.push_back({alias.func,alias.offset,alias.extent});++added;
        }
        std::sort(entries.begin(),entries.end(),[](const auto& a,const auto& b){return a.offset<b.offset;});
        if(std::adjacent_find(entries.begin(),entries.end(),[](const auto& a,const auto& b){return a.offset==b.offset;})!=entries.end())
            rejected_registration("camera duplicate original section offset");
        section.funcs=entries.data();section.num_funcs=entries.size();
    }
    if(modified!=camera_alias_section_count || added!=std::size(camera_aliases)) rejected_registration("camera alias installation completeness");
    camera_registry_installed=true;
}
void camera_validate_resident_patches() {
    if(!camera_registry_installed) rejected_body("camera registry was not installed");
    for(const auto& p:camera_resident) if(get_function(int32_t(p.address))!=p.func)
        rejected_body("camera own patch address lookup mismatch");
}
void camera_validate_published_section(const SectionTableEntry& section,uint32_t live) {
    size_t checked=0;
    for(const auto& alias:camera_aliases) if(alias.rom==section.rom_addr) {
        if(section.size!=alias.section_size || uint64_t(live)+alias.offset>UINT32_MAX ||
            get_function(int32_t(live+alias.offset))!=alias.func)
            rejected_body("camera original published alias lookup mismatch");
        ++checked;
    }
    if(checked) {
        camera_validate_resident_patches();
        std::fprintf(stderr,"baseline camera-registry original_aliases=%zu patch_providers=%zu helpers=%zu rom=%08x live=%08x lookup_equal=1\n",
            checked,camera_patch_count,camera_patch_count-size_t(baseline_boot_patch_registry::section_table[0].num_funcs),section.rom_addr,live);
        std::fflush(stderr);
    }
}
}
#endif
