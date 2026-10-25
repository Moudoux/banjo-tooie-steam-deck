/* PC[8] secondary player bodies: shared route-token states, the open secondary scope's finite row
 * bounds for the shared player emitters, and the retained-identity checks both player admissions
 * share. Include after silo_bridge.h. */
#pragma once
#define TOOIE_SECONDARY_EMPTY 0u
#define TOOIE_SECONDARY_OPEN 1u    /* parent hook before the 93864(mode1) jal */
#define TOOIE_SECONDARY_ARMED 2u   /* 93A9C hook of that same 93864 invocation */
#define TOOIE_SECONDARY_SPENT 3u   /* one DE448 consumed it, or its armed window returned */
#define TOOIE_SECONDARY_MAIN 1u    /* 93AB8 -> 93864(mode1) at 80093D40 */
#define TOOIE_SECONDARY_LATE 2u    /* 93E18 -> 93864(mode1) at 80093E9C */
/* 93864's fixed frame: its sp at 93A9C/93AA4 is the parent hook's sp minus 0x68. */
#define TOOIE_SECONDARY_FRAME_DELTA 0x68u
/* secondary_ledger.paid = 1 + slot*8 + view*2 + (route-1); 0 when no secondary scope is open. */
static inline __attribute__((always_inline)) volatile struct TooieSecondaryEntry* secondary_paid_entry(void) {
    u32 paid=SS.secondary_ledger.paid;
    if(!paid || paid>64u) return 0;
    paid-=1u;
    return &SS.secondary[paid>>3][(paid>>1)&3u][paid&1u];
}
/* poses|bones<<8|leaf calls<<16 of the open secondary scope's admitted row, or 0. While a
 * secondary scope is paid, the player emitters bound roles and counts by these only. */
static inline __attribute__((always_inline)) u32 player_secondary_limits(void) {
    volatile struct TooieSecondaryEntry* e=secondary_paid_entry();
    return e ? e->limits : 0u;
}
/* @recomp PC[8]: a retained identity of either family, or the global paid snapshot, tombstoned for
 * this owner address. Both admissions check it before any owner dereference (bounded scans of the
 * 32 primary and 64 secondary entries). */
static inline __attribute__((always_inline)) u32 player_retained_tombstoned(u32 owner) {
    u32 i;
    if(SS.player.owner==owner && SS.player.stamp==0xFFFFFFFFu) return 1u;
    for(i=0;i<32u;++i)
        if(SS.primary[i>>2][i&3u].owner==owner && SS.primary[i>>2][i&3u].stamp==0xFFFFFFFFu) return 1u;
    for(i=0;i<64u;++i)
        if(SS.secondary[i>>3][(i>>1)&3u][i&1u].owner==owner &&
           SS.secondary[i>>3][(i>>1)&3u][i&1u].stamp==0xFFFFFFFFu) return 1u;
    return 0u;
}
/* @recomp PC[8]: F73C4 can cancel a pending slot without init. Whichever admission observes it
 * forces freshness of every retained identity of that slot in both families (4 primary views, 4
 * views x 2 secondary routes) and of the global snapshot when it is this owner. Not a tombstone. */
static inline __attribute__((always_inline)) void player_slot_fresh(u32 slot,u32 owner) {
    u32 i;
    for(i=0;i<4u;++i) SS.primary[slot][i].idbase=0;
    for(i=0;i<8u;++i) SS.secondary[slot][i>>1][i&1u].idbase=0;
    if(SS.player.owner==owner) SS.player.idbase=0;
}
