/* Guest data effects of the retained libultra initialize.c, with distinct
   boot and core1 linked storage. Native-runtime adaptations are @recomp. */
#include <ultra64.h>
#include <PR/R4300.h>
#define TOOIE_RECOMP_PATCH __attribute__((section(".recomp_patch")))

extern u32 boot___osFinalrom;
extern u64 boot_osClockRate;
extern s32 boot_osViClock;
extern OSPiHandle boot___Dom1SpeedParam, boot___Dom2SpeedParam;
extern u8 boot___osExceptionPreamble[16];
extern u32 __osFinalrom;
extern u64 osClockRate;
extern s32 osViClock;
extern OSPiHandle __Dom1SpeedParam, __Dom2SpeedParam;
extern u8 __osExceptionPreamble[16];
extern u32 boot___osSetFpcCsr(u32);

_Static_assert(sizeof(void*) == 4 && sizeof(long) == 4 && sizeof(u64) == 8,
               "initializer patch requires guest o32 types");
_Static_assert(sizeof(OSPiHandle) == 0x74 && __builtin_offsetof(OSPiHandle, type) == 4,
               "original PI handle layout changed");
_Static_assert(UT_VEC == 0x80000000 && XUT_VEC == 0x80000080 &&
               ECC_VEC == 0x80000100 && E_VEC == 0x80000180,
               "original exception destinations changed");

TOOIE_RECOMP_PATCH void boot_osInitialize(void) {
    u32 vector, offset;
    boot___osFinalrom = TRUE;
    // @recomp Omit console CU1/MMIO/PIF, PI timing, cache and TLB setup.
    // Native FP has no CU1 gate; guest data retains its original owners.
    // @recomp Restore the guest FP helper. Recompiled COP1 CS models rounding
    // only: FS/EV are not simulated, and this requests FE_TONEAREST.
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

TOOIE_RECOMP_PATCH void tooie_core1_osInitialize(void) {
    u32 vector, offset;
    __osFinalrom = TRUE;
    // @recomp Omit console CU1/MMIO/PIF, PI timing, cache and TLB setup.
    // @recomp Use the actual boot guest FP helper for supported rounding,
    // rather than the unprefixed native FCSR no-op or a synthetic FS/EV shadow.
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
    __Dom1SpeedParam.type = DEVICE_TYPE_INIT;
    __Dom2SpeedParam.type = DEVICE_TYPE_INIT;
}
