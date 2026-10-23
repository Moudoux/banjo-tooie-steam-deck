/* One patch image; only the two named rendering objects add guest storage. */
MEMORY { patch : ORIGIN = 0x80801000, LENGTH = 0x7FF000 }
tooie_get_construction_model_policy = 0x8F000000;
tooie_get_hud_half_width_bits = 0x8F000004;
tooie_get_submitted_render_tuple = 0x8F000008;
tooie_wrapper_attachment_begin = 0x8F00000C;
tooie_wrapper_attachment_end = 0x8F000010;
tooie_wrapper_source_span_begin = 0x8F000014;
tooie_wrapper_source_span_end = 0x8F000018;
tooie_get_scene_tuple32 = 0x8F00001C;
tooie_get_latched_widescreen_settings = 0x8F000020;
SECTIONS {
    .recomp_patch : { *(.recomp_patch) } >patch
    .tooie_hud_literals ALIGN(4) : { *hud_projection.o(.rodata.cst4) } >patch
    .tooie_wrapper_literals ALIGN(8) : { *model_nodes.o(.rodata.cst8) } >patch
    .tooie_aspect_literals ALIGN(8) : { *native_aspect_projection.o(.rodata.cst8) } >patch
    .tooie_gfx_buffers ALIGN(8) : { *(.tooie_gfx_buffers) } >patch
    .tooie_family_birth_state ALIGN(4) : { *(.tooie_family_birth_state) } >patch
    tooie_wrapper_state = ABSOLUTE(tooie_family_birth_state + 804);
    .forbidden : {
        *(.text*) *(.rodata*) *(.srodata*) *(.data*) *(.sdata*)
        *(.bss*) *(.sbss*) *(COMMON) *(.tdata*) *(.tbss*) *(.lit*)
        *(.ctors*) *(.dtors*) *(.init*) *(.fini*) *(.preinit_array*)
    } >patch
    ASSERT(SIZEOF(.recomp_patch) > 0, "missing patch code")
    ASSERT(SIZEOF(.tooie_hud_literals) == 16, "HUD literal extent changed")
    ASSERT(SIZEOF(.tooie_wrapper_literals) == 8, "wrapper literal extent changed")
    ASSERT(SIZEOF(.tooie_aspect_literals) == 32, "aspect literal extent changed")
    ASSERT(SIZEOF(.tooie_gfx_buffers) == 112016, "Gfx pool extent changed")
    ASSERT(SIZEOF(.tooie_family_birth_state) == 1168, "birth object extent changed")
    ASSERT(SIZEOF(.forbidden) == 0, "patch image cannot define helpers or runtime data")
    ASSERT(. <= 0x81000000, "patch image exceeds reserved range")
    .rel.recomp_patch 0 : { *(.rel.recomp_patch) }
    .symtab 0 : { *(.symtab) }
    .strtab 0 : { *(.strtab) }
    .shstrtab 0 : { *(.shstrtab) }
    /* prepare_heap_patch proves no input GOT/GP dependency before linking.
       LLD's fixed two unused MIPS GOT header entries are not guest storage. */
    /DISCARD/ : {
        *(.got)
        *(.reginfo) *(.MIPS.abiflags) *(.MIPS.options) *(.pdr)
        *(.comment) *(.note.GNU-stack) *(.gnu.attributes) *(.mdebug*)
    }
}
