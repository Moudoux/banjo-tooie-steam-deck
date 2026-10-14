/* Guest initialization subset from retained
   deps/banjo-tooie/lib/ultralib/src/os/initialize.c semantics.
   This disposable native-runtime condition omits console MMIO/cache/TLB.
   Original bootproc, decompression, helper bodies and guest data remain owners. */
#include <ultra64.h>
#include <PR/R4300.h>

extern u32 boot___osFinalrom;
extern u64 boot_osClockRate;
extern s32 boot_osViClock;
extern OSPiHandle boot___Dom1SpeedParam, boot___Dom2SpeedParam;
extern u8 boot___osExceptionPreamble[16];
extern u32 boot___osSetFpcCsr(u32);

_Static_assert(sizeof(void*) == 4 && sizeof(long) == 4 && sizeof(u64) == 8,
               "boot patch requires guest o32 types");
_Static_assert(sizeof(OSPiHandle) == 0x74 && __builtin_offsetof(OSPiHandle, type) == 4,
               "original PI handle layout changed");
_Static_assert(UT_VEC == 0x80000000 && XUT_VEC == 0x80000080 &&
               ECC_VEC == 0x80000100 && E_VEC == 0x80000180,
               "original exception destinations changed");

__attribute__((section(".recomp_patch"))) void boot_osInitialize(void) {
    u32 vector, offset;
    boot___osFinalrom = TRUE;
    /* Native FP executes without CU1 gating; stock runtime rejects that bit. */
    boot___osSetFpcCsr(FPCSR_FS | FPCSR_EV);
    for (vector = UT_VEC; vector <= E_VEC; vector += 0x80) {
        for (offset = 0; offset < 16; ++offset)
            ((u8*)vector)[offset] = boot___osExceptionPreamble[offset];
    }
    boot_osClockRate = boot_osClockRate * (u64)3 / (u64)4;
    if (osResetType == 0) {
        for (offset = 0; offset < OS_APP_NMI_BUFSIZE; ++offset)
            ((u8*)osAppNMIBuffer)[offset] = 0;
    }
    boot_osViClock = osTvType == OS_TV_PAL ? VI_PAL_CLOCK :
                    osTvType == OS_TV_MPAL ? VI_MPAL_CLOCK : VI_NTSC_CLOCK;
    boot___Dom1SpeedParam.type = DEVICE_TYPE_INIT;
    boot___Dom2SpeedParam.type = DEVICE_TYPE_INIT;
}
