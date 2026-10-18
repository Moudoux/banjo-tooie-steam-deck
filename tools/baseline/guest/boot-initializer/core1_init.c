/* Adapted from deps/banjo-tooie/lib/ultralib/src/os/initialize.c (J, FINALROM)
   using original core1 guest storage. Finalrom, exception vectors, clock scaling,
   NMI buffer clear, VI clock and PI type sentinels keep their original data effects.
   Omitted natively: CU1 enable, PIF/AI MMIO, cache/TLB maintenance, the Cause-IP5
   hang and PI timing-register reads. No full console or FCSR equivalence is claimed. */
#include <ultra64.h>
#include <PR/R4300.h>

extern u32 __osFinalrom;
extern u64 osClockRate;
extern s32 osViClock;
extern OSPiHandle __Dom1SpeedParam, __Dom2SpeedParam;
extern u8 __osExceptionPreamble[16];
extern u32 boot___osSetFpcCsr(u32);

_Static_assert(sizeof(void*) == 4 && sizeof(long) == 4 && sizeof(u64) == 8,
               "core1 patch requires guest o32 types");
_Static_assert(sizeof(OSPiHandle) == 0x74 && __builtin_offsetof(OSPiHandle, type) == 4,
               "original PI handle layout changed");
_Static_assert(UT_VEC == 0x80000000 && XUT_VEC == 0x80000080 &&
               ECC_VEC == 0x80000100 && E_VEC == 0x80000180,
               "original exception destinations changed");

__attribute__((section(".recomp_patch"))) void tooie_core1_osInitialize(void) {
    u32 vector, offset;
    __osFinalrom = TRUE;
    /* @recomp Native FP needs no CU1 enable. Keep the original FCSR value via the boot
       helper; native FCSR modelling covers its rounding mode only. The unprefixed native
       helper is a no-op. */
    boot___osSetFpcCsr(FPCSR_FS | FPCSR_EV);
    for (vector = UT_VEC; vector <= E_VEC; vector += 0x80) {
        for (offset = 0; offset < 16; ++offset)
            ((u8*)vector)[offset] = __osExceptionPreamble[offset];
    }
    osClockRate = osClockRate * (u64)3 / (u64)4;
    if (osResetType == 0) {
        for (offset = 0; offset < OS_APP_NMI_BUFSIZE; ++offset)
            ((u8*)osAppNMIBuffer)[offset] = 0;
    }
    osViClock = osTvType == OS_TV_PAL ? VI_PAL_CLOCK :
                osTvType == OS_TV_MPAL ? VI_MPAL_CLOCK : VI_NTSC_CLOCK;
    /* @recomp Keep the original type sentinels. The four timing fields per domain are
       not sampled; native ROM transport does not read them, and retained guest EPI
       readers are not proven unreachable. */
    __Dom1SpeedParam.type = DEVICE_TYPE_INIT;
    __Dom2SpeedParam.type = DEVICE_TYPE_INIT;
}
