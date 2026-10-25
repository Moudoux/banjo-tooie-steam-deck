/* Accepted M2 code-only placement; guest storage stays in original RDRAM. */
MEMORY { patch : ORIGIN = 0x80801000, LENGTH = 0x7FF000 }
tooie_get_latched_widescreen_settings = 0x8F000020;
tooie_baseline_presentation_settings = 0x8F000024;
tooie_baseline_presentation_publish = 0x8F000028;
tooie_baseline_quiet_talon_trot = 0x8F00002C;
tooie_baseline_trot_toggle = 0x8F000030;
baseline_npc665_end_checked = 0x8F000034;
tooie_baseline_first_person_aim = 0x8F000038;
tooie_bridge_required_stop = 0x8F00003C;
tooie_baseline_music_volume = 0x8F000040;
SECTIONS {
    .recomp_patch : { *(.recomp_patch) } >patch
    .silo_helper ALIGN(4) : { *(.text.silo_preflight) } >patch
    .player_helper ALIGN(4) : { *(.text.player_boundary) } >patch
    .tooie_camera_helpers ALIGN(4) : { *camera_hook_leaves.o(.tooie_camera_helpers) } >patch
    ASSERT(SIZEOF(.tooie_camera_helpers)>0, "missing exact camera helpers")
    .tooie_camera_constants ALIGN(4) : { *camera_lifecycle.o(.rodata.cst4) } >patch
    ASSERT(SIZEOF(.tooie_camera_constants)==8, "camera literal constant extent")
    .tooie_camera_ncpod_constants ALIGN(4) : { *camera_ncpod.o(.rodata.cst4) } >patch
    ASSERT(SIZEOF(.tooie_camera_ncpod_constants)==8, "ncpod literal constant extent")
    /* Keep section identities stable: pool and float reciprocal share the mergeable 1.0f literal. */
    .tooie_camera_pool_constants ALIGN(4) : { *camera_pools.o(.rodata.cst4) *camera_emitters.o(.rodata.cst4) } >patch
    ASSERT(SIZEOF(.tooie_camera_pool_constants)==4, "pool and float camera literal constant extent")
    .actor_helper ALIGN(4) : { *(.text.actor_palette) } >patch
    .tooie_render_bridge_state ALIGN(4) : { *(.tooie_render_bridge_state) } >patch
    ASSERT(SIZEOF(.tooie_render_bridge_state)==23036, "helper/player bridge state extent")
    .tooie_bk_state ALIGN(4) : { *bk.o(.tooie_bk_state) } >patch
    ASSERT(SIZEOF(.tooie_bk_state)==0 || SIZEOF(.tooie_bk_state)==3412, "BK pipeline state layout1 extent")
    .tooie_bk_float ALIGN(8) : { *bk.o(.tooie_bk_float) } >patch
    ASSERT(SIZEOF(.tooie_bk_float)==0 || SIZEOF(.tooie_bk_float)==803072, "BK float producer layout1 extent")
    .tooie_bk_cut ALIGN(4) : { *bk.o(.tooie_bk_cut) } >patch
    ASSERT(SIZEOF(.tooie_bk_cut)==0 || SIZEOF(.tooie_bk_cut)==64, "BK cut adapter layout1 extent")
    .tooie_bk_shadow ALIGN(4) : { *bk.o(.tooie_bk_shadow) } >patch
    ASSERT(SIZEOF(.tooie_bk_shadow)==0 || SIZEOF(.tooie_bk_shadow)==1144, "BK shadow state layout2 extent")
    .tooie_camera_state ALIGN(4) : { *(.tooie_camera_state) } >patch
    .tooie_camera_pools ALIGN(16) : { *(.tooie_camera_pools) } >patch
    ASSERT(SIZEOF(.tooie_camera_state)==1380, "camera state extent")
    ASSERT(SIZEOF(.tooie_camera_pools)==813824, "camera pool extent")
    ASSERT(ADDR(.tooie_camera_pools)%16==0, "camera pool alignment")
    .tooie_demo_timing_helpers ALIGN(4) : { *demo_timing.o(.tooie_demo_timing_helpers) } >patch
    .tooie_demo_timing_state ALIGN(4) : { *demo_timing.o(.tooie_demo_timing_state) } >patch
    ASSERT(SIZEOF(.tooie_demo_timing_state)==0 || SIZEOF(.tooie_demo_timing_state)==8, "demo timing state extent")
    .tooie_shadow_helpers ALIGN(4) : { *shadow_projection.o(.tooie_shadow_helpers) } >patch
    .tooie_shadow_state ALIGN(4) : { *shadow_projection.o(.tooie_shadow_state) } >patch
    ASSERT(SIZEOF(.tooie_shadow_state)==0 || SIZEOF(.tooie_shadow_state)==56 || SIZEOF(.tooie_shadow_state)==100, "shadow state extent (100: coefficient modelview tail)")
    .tooie_pre_background_helpers ALIGN(4) : { *camera_pre_background_slots.o(.tooie_pre_background_helpers) } >patch
    .tooie_pre_background_state ALIGN(4) : { *camera_pre_background_state.o(.tooie_pre_background_state) } >patch
    ASSERT(SIZEOF(.tooie_pre_background_state)==0 || SIZEOF(.tooie_pre_background_state)==124, "PRE sidecar extent")
    .tooie_cpu625_helpers ALIGN(4) : { *cpu625.o(.tooie_cpu625_helpers) } >patch
    .tooie_cpu625_state ALIGN(4) : { *cpu625.o(.tooie_cpu625_state) } >patch
    ASSERT(SIZEOF(.tooie_cpu625_state)==0 || SIZEOF(.tooie_cpu625_state)==464, "CPU625 state extent")
    .tooie_music_state ALIGN(4) : { *music_gain.o(.tooie_music_state) } >patch
    ASSERT(SIZEOF(.tooie_music_state)==0 || SIZEOF(.tooie_music_state)==20, "music request cache extent")
    .baseline_host_helpers ALIGN(4) : { *minimum_cutscene.o(.baseline_host_helpers) } >patch
    .baseline_host_state ALIGN(4) : { *minimum_cutscene.o(.baseline_host_state) } >patch
    .baseline_g07_literals ALIGN(8) : { *native_aspect_projection.o(.rodata.cst8) } >patch
    ASSERT(SIZEOF(.baseline_host_state)==0 || SIZEOF(.baseline_host_state)==12, "HOST state extent")
    ASSERT(SIZEOF(.baseline_g07_literals)==0 || SIZEOF(.baseline_g07_literals)==24, "G07 literal extent")
    .tooie_npc665_helpers ALIGN(4) : { *npc665.o(.tooie_npc665_helpers) } >patch
    .tooie_npc665_state ALIGN(4) : { *npc665.o(.tooie_npc665_state) } >patch
    ASSERT(SIZEOF(.tooie_npc665_state)==0 || SIZEOF(.tooie_npc665_state)==16, "NPC665 sidecar extent")
    .tooie_backpack621_helpers : { *(.tooie_backpack621_helpers) } >patch
    .tooie_backpack621_state ALIGN(4) : { *(.tooie_backpack621_state) } >patch
    ASSERT(SIZEOF(.tooie_backpack621_state)==0 || SIZEOF(.tooie_backpack621_state)==612, "621 state extent")
    .tooie_graphics_rate_state ALIGN(8) : { *graphics_rate.o(.tooie_graphics_rate_state) } >patch
    ASSERT(SIZEOF(.tooie_graphics_rate_state)==0 || SIZEOF(.tooie_graphics_rate_state)==48, "two graphics-rate tails extent")
    .tooie_trot_helpers ALIGN(4) : { *talon_trot_toggle.o(.tooie_trot_helpers) } >patch
    .tooie_trot_state ALIGN(4) : { *talon_trot_toggle.o(.tooie_trot_state) } >patch
    ASSERT(SIZEOF(.tooie_trot_state)==0 || SIZEOF(.tooie_trot_state)==12, "Trot slot extent")
    .tooie_modern_fps_helpers ALIGN(4) : { *modern_fps.o(.tooie_modern_fps_helpers) } >patch
    .tooie_modern_fps_state ALIGN(4) : { *modern_fps.o(.tooie_modern_fps_state) } >patch
    .tooie_modern_fps_literals ALIGN(4) : { *modern_fps.o(.rodata.cst4) } >patch
    ASSERT(SIZEOF(.tooie_modern_fps_state)==0 || SIZEOF(.tooie_modern_fps_state)==28, "FPS sample extent")
    .tooie_ckcopy_helpers ALIGN(4) : { *ckcopy.o(.tooie_ckcopy_helpers) } >patch
    .tooie_ckcopy_state ALIGN(8) : { *ckcopy.o(.tooie_ckcopy_state) } >patch
    ASSERT(SIZEOF(.tooie_ckcopy_state)==0 || SIZEOF(.tooie_ckcopy_state)==122960, "ckpack copy storage extent (layout 2)")
    .forbidden : {
        *(.text*) *(.rodata*) *(.srodata*) *(.data*) *(.sdata*)
        *(.bss*) *(.sbss*) *(COMMON) *(.tdata*) *(.tbss*) *(.lit*)
        *(.ctors*) *(.dtors*) *(.init*) *(.fini*) *(.preinit_array*)
    } >patch
    ASSERT(SIZEOF(.recomp_patch) > 0, "missing boot initializer patch")
    ASSERT(SIZEOF(.forbidden) == 0, "boot patch cannot define helpers or data")
    ASSERT(. <= 0x81000000, "boot patch exceeds reserved range")
    .rel.recomp_patch 0 : { *(.rel.recomp_patch) }
    .rel.silo_helper 0 : { *(.rel.silo_helper) }
    .rel.player_helper 0 : { *(.rel.player_helper) }
    .rel.actor_helper 0 : { *(.rel.actor_helper) }
    .symtab 0 : { *(.symtab) }
    .strtab 0 : { *(.strtab) }
    .shstrtab 0 : { *(.shstrtab) }
    /* Preparation proves no input GOT/GP dependency before discarding the
       fixed unused LLD MIPS GOT header, matching the accepted M2 recipe. */
    /DISCARD/ : {
        *(.got) *(.reginfo) *(.MIPS.abiflags) *(.MIPS.options) *(.pdr)
        *(.comment) *(.note.GNU-stack) *(.gnu.attributes) *(.mdebug*)
    }
}
