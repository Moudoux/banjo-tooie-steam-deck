/* Accepted M2 code-only placement; guest storage stays in original RDRAM. */
MEMORY { patch : ORIGIN = 0x80801000, LENGTH = 0x7FF000 }
SECTIONS {
    .recomp_patch : { *(.recomp_patch) } >patch
    .forbidden : {
        *(.text*) *(.rodata*) *(.srodata*) *(.data*) *(.sdata*)
        *(.bss*) *(.sbss*) *(COMMON) *(.tdata*) *(.tbss*) *(.lit*)
        *(.ctors*) *(.dtors*) *(.init*) *(.fini*) *(.preinit_array*)
    } >patch
    ASSERT(SIZEOF(.recomp_patch) > 0, "missing boot initializer patch")
    ASSERT(SIZEOF(.forbidden) == 0, "boot patch cannot define helpers or data")
    ASSERT(. <= 0x81000000, "boot patch exceeds reserved range")
    .rel.recomp_patch 0 : { *(.rel.recomp_patch) }
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
