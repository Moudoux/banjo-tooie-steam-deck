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
constexpr size_t camera_patch_count = (TOOIE_SHADOW_SELECTED ? 61 : TOOIE_DEMO_TIMING_SELECTED ? 53 : 50)+(TOOIE_PRE_SELECTED?3:0)+(TOOIE_ACTOR_PALETTE_SELECTED?4:0)+(TOOIE_CPU625_SELECTED?4:0)+(TOOIE_MINIMUM_HOST_SELECTED?2:0)+(TOOIE_NPC665_SELECTED?8:0)+(TOOIE_BACKPACK621_SELECTED?8:0);
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
void camera_validate_patch_table() {
    const auto& sections=baseline_boot_patch_registry::section_table;
#if TOOIE_PRE_SELECTED
    if(camera_actor_palette_selected>1 || camera_actor_palette_selected!=TOOIE_ACTOR_PALETTE_SELECTED || !TOOIE_DEMO_TIMING_SELECTED || !TOOIE_SHADOW_SELECTED || std::size(sections)!=6+TOOIE_DEMO_TIMING_SELECTED+TOOIE_SHADOW_SELECTED+TOOIE_CPU625_SELECTED+TOOIE_MINIMUM_HOST_SELECTED+TOOIE_NPC665_SELECTED+TOOIE_BACKPACK621_SELECTED || std::size(camera_aliases)!=27+TOOIE_MINIMUM_HOST_SELECTED || std::size(camera_expected_patches)!=camera_patch_count)
        rejected_body("PRE selection exact provider/alias/section counts");
    if(sections[0].rom_addr!=0 || sections[0].ram_addr!=0x80801000 || sections[0].index!=1 || sections[0].num_funcs!=((camera_actor_palette_selected?39u:38u)+TOOIE_MINIMUM_HOST_SELECTED) || sections[0].size!=TOOIE_PRE_TEXT_SIZE || sections[0].num_relocs!=TOOIE_PRE_TEXT_RELOCS)rejected_body("PRE fixed section0 identity");
#if TOOIE_BACKPACK621_SELECTED
    for(size_t i=0;i<5;i++) {const auto& a=sections[i];const auto& e=backpack621_shared_sections[i];
        if(a.rom_addr!=e[0] || a.ram_addr!=e[1] || a.size!=e[2] || a.index!=e[3] || a.num_funcs!=e[4] || a.num_relocs!=e[5])rejected_body("Backpack621 exact changed shared section");}
#elif TOOIE_NPC665_SELECTED
    for(size_t i=0;i<5;i++) {const auto& a=sections[i];const auto& e=npc665_shared_sections[i];
        if(a.rom_addr!=e[0] || a.ram_addr!=e[1] || a.size!=e[2] || a.index!=e[3] || a.num_funcs!=e[4] || a.num_relocs!=e[5])rejected_body("NPC665 exact changed shared section");}
#else
    constexpr uint32_t fixed[4][6]={{0x96C8,0x8080A6C8,0x2E38,2,1,2},{0xC500,0x8080D500,0x15FC,3,6,3},{0xDAFC,0x8080EAFC,camera_actor_palette_selected?0xD10u:0x68Cu,4,camera_actor_palette_selected?4u:3u,0},{camera_actor_palette_selected?0xE820u:0xE19Cu,camera_actor_palette_selected?0x8080F820u:0x8080F19Cu,camera_actor_palette_selected?0x3DB4u:0x20F0u,8,camera_actor_palette_selected?4u:2u,camera_actor_palette_selected?6u:4u}};
    for(size_t i=1;i<5;i++) {const auto& a=sections[i];const auto& e=fixed[i-1];
        if(a.rom_addr!=e[0]+TOOIE_PRE_SHIFT || a.ram_addr!=e[1]+TOOIE_PRE_SHIFT || a.size!=e[2] || a.index!=e[3] || a.num_funcs!=e[4] || a.num_relocs!=e[5])rejected_body("PRE fixed section identity/uniform shift");}
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
        rejected_body("camera/helper/player exact registry section/count identity");
#endif
#if TOOIE_DEMO_TIMING_SELECTED
    if(sections[5].rom_addr!=TOOIE_DEMO_ROM || sections[5].ram_addr!=TOOIE_DEMO_RAM ||
       sections[5].size!=TOOIE_DEMO_SIZE || sections[5].index!=TOOIE_DEMO_INDEX ||
       sections[5].num_funcs!=3 || sections[5].num_relocs!=TOOIE_DEMO_RELOCS)
        rejected_body("demo timing exact compiled helper section");
#endif
#if TOOIE_SHADOW_SELECTED
    if(!TOOIE_DEMO_TIMING_SELECTED || (camera_actor_palette_selected && !TOOIE_PRE_SELECTED) || sections[6].rom_addr!=TOOIE_SHADOW_ROM ||
       sections[6].ram_addr!=TOOIE_SHADOW_RAM || sections[6].size!=TOOIE_SHADOW_SIZE || sections[6].index!=TOOIE_SHADOW_INDEX ||
       sections[6].num_funcs!=8 || sections[6].num_relocs!=TOOIE_SHADOW_RELOCS)
        rejected_body("shadow exact compiled helper section");
#endif
#if TOOIE_PRE_SELECTED
    {const auto& p=sections[TOOIE_PRE_SECTION];
     if(p.rom_addr!=TOOIE_PRE_ROM || p.ram_addr!=TOOIE_PRE_RAM || p.size!=TOOIE_PRE_SIZE || p.index!=TOOIE_PRE_INDEX || p.num_funcs!=3 || p.num_relocs!=TOOIE_PRE_RELOCS)rejected_body("PRE exact compiled helper section");}
#endif
#if TOOIE_CPU625_SELECTED
    {const auto& p=sections[TOOIE_CPU625_SECTION];
     if(!TOOIE_PRE_SELECTED || !TOOIE_ACTOR_PALETTE_SELECTED || p.rom_addr!=TOOIE_CPU625_ROM ||
        p.ram_addr!=TOOIE_CPU625_RAM || p.size!=TOOIE_CPU625_SIZE || p.index!=TOOIE_CPU625_INDEX ||
        p.num_funcs!=4 || p.num_relocs!=TOOIE_CPU625_RELOCS)
         rejected_body("CPU625 exact compiled helper section");}
#endif
    #if TOOIE_MINIMUM_HOST_SELECTED
    {const auto& p=sections[TOOIE_MINIMUM_HOST_SECTION];
     if(!TOOIE_PRE_SELECTED || !TOOIE_ACTOR_PALETTE_SELECTED || p.rom_addr!=TOOIE_MINIMUM_HOST_ROM || p.ram_addr!=TOOIE_MINIMUM_HOST_RAM || p.size!=TOOIE_MINIMUM_HOST_SIZE || p.index!=TOOIE_MINIMUM_HOST_INDEX || p.num_funcs!=1 || p.num_relocs!=TOOIE_MINIMUM_HOST_RELOCS)rejected_body("HOST exact compiled helper section");}
    #endif
#if TOOIE_NPC665_SELECTED
    {const auto& p=sections[TOOIE_NPC665_SECTION];
     if(!TOOIE_PRE_SELECTED || !TOOIE_ACTOR_PALETTE_SELECTED || p.rom_addr!=TOOIE_NPC665_ROM ||
        p.ram_addr!=TOOIE_NPC665_RAM || p.size!=TOOIE_NPC665_SIZE || p.index!=TOOIE_NPC665_INDEX ||
        p.num_funcs!=8 || p.num_relocs!=TOOIE_NPC665_RELOCS)
         rejected_body("NPC665 exact eight compiled helpers");}
#endif
#if TOOIE_BACKPACK621_SELECTED
    {const auto& p=sections[TOOIE_BACKPACK621_SECTION];
     if(!TOOIE_CPU625_SELECTED || !TOOIE_PRE_SELECTED || !TOOIE_ACTOR_PALETTE_SELECTED ||
        p.rom_addr!=TOOIE_BACKPACK621_ROM || p.ram_addr!=TOOIE_BACKPACK621_RAM ||
        p.size!=TOOIE_BACKPACK621_SIZE || p.index!=TOOIE_BACKPACK621_INDEX ||
        p.num_funcs!=8 || p.num_relocs!=TOOIE_BACKPACK621_RELOCS)
         rejected_body("Backpack621 exact eight compiled helpers");}
#endif
    size_t resident=0;
    for(size_t s=0;s<std::size(sections);++s) {
        const auto& section=sections[s];
        if(!section.funcs || uint64_t(section.ram_addr)+section.size>0x81000000ull)
            rejected_body("camera patch section bounds/relocations");
        for(size_t i=0;i<section.num_funcs;++i) {
            const auto& f=section.funcs[i];
            if(!f.func || !f.rom_size || uint64_t(f.offset)+f.rom_size>section.size ||
                (i && uint64_t(section.funcs[i-1].offset)+section.funcs[i-1].rom_size>f.offset))
                rejected_body("camera patch function bounds/overlap");
            size_t matches=0;
            for(const auto& expected:camera_expected_patches) if(expected.func==f.func) {
                if(expected.section!=s) rejected_body("camera helper section ownership");
                ++matches;
            }
            if(matches!=1) rejected_body("camera unexpected/duplicate provider identity");
            for(size_t j=0;j<resident;++j) if(camera_resident[j].func==f.func)
                rejected_body("camera provider appears more than once");
            camera_resident[resident++]={f.func,section.ram_addr+f.offset};
        }
    }
#if TOOIE_NPC665_SELECTED
    size_t end_matches=0;
    for(size_t i=0;i<resident;i++) if(camera_resident[i].address==TOOIE_NPC665_END) {
        if(camera_resident[i].func!=baseline_npc665_end_checked)rejected_body("NPC665 checked end provider");
        ++end_matches;
    }
    if(end_matches!=1)rejected_body("NPC665 unique end address");
#endif
    if(resident!=camera_patch_count) rejected_body("camera complete provider membership");
}
void camera_register_aliases(SectionTableEntry* sections,size_t count) {
    if(camera_registry_installed) rejected_body("camera registry installed twice");
    camera_validate_patch_table();
    static std::array<std::vector<FuncEntry>,3> owned;
    size_t modified=0,added=0;
    for(size_t s=0;s<count;++s) {
        auto& section=sections[s];bool selected=false;
        for(const auto& alias:camera_aliases) if(alias.rom==section.rom_addr) selected=true;
        if(!selected) continue;
        if(modified==owned.size()) rejected_body("camera alias section count");
        auto& entries=owned[modified++];entries.assign(section.funcs,section.funcs+section.num_funcs);
        for(const auto& alias:camera_aliases) if(alias.rom==section.rom_addr) {
            if(section.ram_addr!=alias.ram || section.size!=alias.section_size ||
                uint64_t(alias.offset)+alias.extent>section.size ||
                std::any_of(entries.begin(),entries.end(),[&](const auto& e){return e.offset==alias.offset;}))
                rejected_body("camera original alias not omitted or section differs");
            entries.push_back({alias.func,alias.offset,alias.extent});++added;
        }
        std::sort(entries.begin(),entries.end(),[](const auto& a,const auto& b){return a.offset<b.offset;});
        if(std::adjacent_find(entries.begin(),entries.end(),[](const auto& a,const auto& b){return a.offset==b.offset;})!=entries.end())
            rejected_body("camera duplicate original section offset");
        section.funcs=entries.data();section.num_funcs=entries.size();
    }
    if(modified!=3 || added!=27+TOOIE_MINIMUM_HOST_SELECTED) rejected_body("camera alias installation completeness");
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
        std::fprintf(stderr,"baseline camera-registry original_aliases=%zu patch_providers=%zu helpers=%u rom=%08x live=%08x lookup_equal=1\n",checked,camera_patch_count,(TOOIE_SHADOW_SELECTED?23u:TOOIE_DEMO_TIMING_SELECTED?15u:12u)+(TOOIE_PRE_SELECTED?3u:0u)+(TOOIE_ACTOR_PALETTE_SELECTED?3u:0u)+(TOOIE_CPU625_SELECTED?4u:0u)+TOOIE_MINIMUM_HOST_SELECTED+(TOOIE_NPC665_SELECTED?8u:0u)+(TOOIE_BACKPACK621_SELECTED?8u:0u),section.rom_addr,live);
        std::fflush(stderr);
    }
}
}
#endif
