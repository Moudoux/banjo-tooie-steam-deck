/* Reconstructed original initialization and inherited destruction flow.
 * Port-owned retirement additions are marked below. */
#include "silo_bridge.h"
#if !defined(TOOIE_BACKPACK621) || (TOOIE_BACKPACK621 != 0 && TOOIE_BACKPACK621 != 1)
#error Explicit boolean TOOIE_BACKPACK621 selection required
#endif
#if TOOIE_BACKPACK621
#include "../backpack621-candidate/backpack621_owner.h"
#endif
#if !defined(TOOIE_CPU625) || (TOOIE_CPU625 != 0 && TOOIE_CPU625 != 1)
#error Explicit boolean TOOIE_CPU625 selection required
#endif
#if TOOIE_CPU625
#include "../cpu625-candidate/cpu625_layout.h"
#endif
#if !defined(TOOIE_TROT_TOGGLE) || (TOOIE_TROT_TOGGLE != 0 && TOOIE_TROT_TOGGLE != 1)
#error Explicit boolean TOOIE_TROT_TOGGLE selection required
#endif
#if TOOIE_TROT_TOGGLE
void tooie_trot_toggle_retire(u32);
#endif
#define PATCH __attribute__((section(".recomp_patch"), noinline))
#define PC SS.player

void _babackpack_entrypoint_9(void*);
void func_800936E8(void*, s32);
void func_80093700(void*, s32);
s32 func_800AE020(void);
s32 func_800E09B8(s32);
void func_800AE6FC(void*, s16);
u32 func_800DBFF8(void);
void func_800EFD24(void*);
void func_80093370(void*, s32);
void func_8009337C(void*, s32);
/* Original o32 passes these float payloads in a1 (the callees mtc1 a1,f12). */
void func_80093230(void*, f32);
void func_800931AC(void*, s32);
void func_80092778(void*);
void func_80092864(void*, f32);
void func_80098E64(void*);
void heap_free(void*);
_Static_assert(sizeof(void*)==4 && sizeof(s16)==2 && sizeof(u32)==4,
               "original player lifecycle ABI");

/* Repeated volatile reads preserve the original body-reload order across calls. */
SI u8* player_body(u8* self) {
    return *(u8* volatile*)(self+0x50);
}

/* @recomp Retire or complete the port-owned retained renderer identity. */
SI void player_lifecycle(u8* self, u32 complete) {
    u32 i,paid=SS.primary_paid,spaid=SS.secondary_ledger.paid;
    /* Preserve stamp/ID/cursor/role state for prepaid pops on unsupported reentry. PC[8]: the
     * open paid scope is the admitted entry named by primary_paid, never another family's
     * SS.active or a merely retained entry; a secondary scope likewise by secondary_ledger.paid. */
    if(SS.active && ((paid && paid<=32u && SS.primary[(paid-1u)>>2][(paid-1u)&3u].owner==(u32)self) ||
       (spaid && spaid<=64u &&
        SS.secondary[(spaid-1u)>>3][((spaid-1u)>>1)&3u][(spaid-1u)&1u].owner==(u32)self))) {
        silo_count(&PC.tripwires); silo_count(&SS.tripwire_count); SS.poisoned=1;
        return;
    }
    /* Every retained entry of this owner retires, not only the last admitted one. */
    for(i=0;i<32u;++i)
        if(SS.primary[i>>2][i&3u].owner==(u32)self) {
            SS.primary[i>>2][i&3u].idbase=0;
            SS.primary[i>>2][i&3u].stamp=complete ? 0u : 0xFFFFFFFFu;
        }
    for(i=0;i<64u;++i)
        if(SS.secondary[i>>3][(i>>1)&3u][i&1u].owner==(u32)self) {
            SS.secondary[i>>3][(i>>1)&3u][i&1u].idbase=0;
            SS.secondary[i>>3][(i>>1)&3u][i&1u].stamp=complete ? 0u : 0xFFFFFFFFu;
        }
    if(PC.owner!=(u32)self) return;
    PC.idbase=0;
    PC.stamp=complete ? 0u : 0xFFFFFFFFu;
}

/* Reconstructed original func_80092898 initialization from core2/1E6B900.
 * @recomp CPU625/Backpack/Trot retirements and player_lifecycle begin/end
 * are port additions; original call/store and body-reload order stays intact. */
PATCH void func_80092898(u8* self) {
#if TOOIE_CPU625
    tooie_cpu625_retire((u32)self,0u,3u);
#endif
#if TOOIE_BACKPACK621
    tooie_backpack621_retire((u32)self,0u,3u);
#endif
#if TOOIE_TROT_TOGGLE
    tooie_trot_toggle_retire((u32)self);
#endif
    void* pose;
    u8* body;
    s32 result;
    u32 zero_bits;
    player_lifecycle(self,0);
    _babackpack_entrypoint_9(self);
    *(volatile u8*)(player_body(self)+0x11)=0;
    *(volatile u8*)(player_body(self)+0x14)=0xFF;
    *(volatile u8*)(player_body(self)+0x10)=0;
    *(volatile u8*)(player_body(self)+0x16)=0;
    *(volatile u8*)(player_body(self)+0x12)=0;
    *(volatile u8*)(player_body(self)+0x13)=0;
    *(volatile u16*)(player_body(self)+0x0C)=0;
    *(volatile u8*)(player_body(self)+0xA2)=0;
    *(volatile u16*)(player_body(self)+0x9C)=0;
    func_800936E8(self,0);
    func_80093700(self,1);
    result=func_800AE020();
    *(volatile s16*)player_body(self)=(s16)result;
    pose=func_800AE080(*(volatile s16*)player_body(self));
    result=func_800E09B8(10);
    func_800AE6FC(pose,(s16)result);
    *(volatile u8*)(player_body(self)+0x88)=0;
    result=func_800DBFF8();
    *(volatile u32*)(player_body(self)+0x04)=(u32)result;
    *(volatile u32*)(player_body(self)+0x84)=0;
    *(volatile u8*)(player_body(self)+0x7C)=0;
    *(volatile u8*)(player_body(self)+0x81)=0;
    func_800EFD24(player_body(self)+0x2C);
    func_800EFD24(player_body(self)+0x38);
    func_800EFD24(player_body(self)+0x44);
    *(volatile u32*)(player_body(self)+0x20)=0;
    body=player_body(self);
    zero_bits=*(volatile u32*)(body+0x20);
    *(volatile u32*)(body+0x24)=zero_bits;
    *(volatile u32*)(player_body(self)+0x28)=zero_bits;
    func_80093370(self,1);
    func_8009337C(self,1);
    func_80093230(self,1.0f);
    *(volatile u8*)(player_body(self)+0x15)=0;
    func_800931AC(self,1);
    func_80092778(self);
    func_80092864(self,1.0f);
    player_lifecycle(self,1);
}

/* Original core2/1ECD170 C destruction flow, re-expressed in this guest ABI.
 * @recomp The retirement prelude and player_lifecycle call are port additions. */
PATCH void func_800F3880(u8* self) {
#if TOOIE_CPU625
    tooie_cpu625_retire((u32)self,0u,4u);
#endif
#if TOOIE_BACKPACK621
    tooie_backpack621_retire((u32)self,0u,4u);
#endif
#if TOOIE_TROT_TOGGLE
    tooie_trot_toggle_retire((u32)self);
#endif
    player_lifecycle(self,0);
    func_80098E64(self);
    heap_free(self);
}
