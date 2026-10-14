/* Original default handler plus held, finite Silo correspondence. */
#include "silo_bridge.h"
#if !defined(TOOIE_BACKPACK621) || (TOOIE_BACKPACK621 != 0 && TOOIE_BACKPACK621 != 1)
#error Explicit boolean TOOIE_BACKPACK621 selection required
#endif
#if TOOIE_BACKPACK621
#include "../backpack621-candidate/backpack621_owner.h"
#endif
#define PATCH __attribute__((section(".recomp_patch")))
typedef SiloBuffers BoneBuffers;
extern void* volatile D_8012C968;
extern u8 D_8012CE58[];
void* func_800ADE2C(void*, s32);
void func_80019AA0(void*, void*);
void mlMtxApply(Mtx*);
void func_800DE2A4(BoneBuffers*, u8*);
void mlMtxPop(void);
_Static_assert(sizeof(void*) == 4 && sizeof(BoneBuffers) == 8 &&
    __builtin_offsetof(BoneBuffers, mtx) == 4 && sizeof(Gfx) == 8 && sizeof(Mtx) == 64,
    "original bone handler guest ABI");
PATCH void func_800DCF48(BoneBuffers* buffers, u8* node) {
    void* animation = D_8012C968;
    u32 role=silo_begin((SiloBuffers*)buffers,node),didPush=0,playerBoneToken=0;
#if TOOIE_NPC665
    u32 npcBoneToken=0;
#endif
#if TOOIE_BACKPACK621
    u32 backpack621BoneToken=0;
#endif
    if (animation) {
        void* matrix = func_800ADE2C(animation, *(volatile s8*)(node + 9));
        func_80019AA0(D_8012CE58, matrix);
        Mtx* applied=buffers->mtx;
        mlMtxApply(applied);
        if(role) { silo_push((SiloBuffers*)buffers,role,(u32)applied); didPush=1; }
#if TOOIE_BACKPACK621
        if(backpack621_paid())
            backpack621BoneToken=tooie_backpack621_bone_push((SiloBuffers*)buffers,*(volatile s8*)(node+9));
        else
#endif
#if TOOIE_NPC665
        if(SC.admitted==TOOIE_NPC665_ADMITTED)
            npcBoneToken=tooie_npc665_bone_push((SiloBuffers*)buffers,*(volatile s8*)(node+9));
        else
#endif
        playerBoneToken=tooie_player_bone_push((SiloBuffers*)buffers,*(volatile s8*)(node+9));
        Gfx* cursor = buffers->gfx;
        buffers->gfx = cursor + 1;
        ((volatile u32*)cursor)[0] = 0xDA380002u;
        ((volatile u32*)cursor)[1] = (u32)buffers->mtx;
        buffers->mtx = (Mtx*)((u8*)buffers->mtx + 64);
    }
    u32 child = *(volatile u8*)(node + 8);
    if (child) func_800DE2A4(buffers, node + child);
    if (D_8012C968) {
        mlMtxPop();
        Gfx* cursor = buffers->gfx;
        buffers->gfx = cursor + 1;
        ((volatile u32*)cursor)[1] = 0x40u;
        ((volatile u32*)cursor)[0] = 0xD8380002u;
    }
    if(didPush) silo_pop((SiloBuffers*)buffers,role);
    if(playerBoneToken) tooie_player_bone_pop((SiloBuffers*)buffers,playerBoneToken);
#if TOOIE_NPC665
    if(npcBoneToken) tooie_npc665_bone_pop((SiloBuffers*)buffers,npcBoneToken);
#endif
#if TOOIE_BACKPACK621
    if(backpack621BoneToken) tooie_backpack621_bone_pop((SiloBuffers*)buffers,backpack621BoneToken);
#endif
}

__attribute__((noinline, section(".text.silo_preflight"))) u32 tooie_silo_preflight(SiloBuffers* b,u8* node) {
    u32 i,base,current,end,mtxbase,mtx,flags,renew,actor_data;
    volatile struct TooieSiloSlot* e;
    if (!SC.actor || SC.admitted || SS.silo_depth!=1u || SC.buffers!=(u32)b) return 1;
    silo_count(&SS.silo_calls);
    silo_epoch();
    if(SC.epoch!=SS.epoch || !SS.epoch || SS.epoch==0xFFFFFFFFu) return 2;
    if(SS.poisoned || SS.active) return 3;
    if(!silo_binding(SC.actor,SC.descriptor,SC.slot,SC.birth,SC.pair)) return 4;
    if(SC.view>3 || !(SC.phase==0||SC.phase==1||SC.phase==2||SC.phase==5||SC.phase==6)) return 5;
    if(D_8007BEDA<0 || D_8007BEDA>1 || D_8007BCE8[0] || D_8007BCE8[1] || D_8007BCF0[0] || D_8007BCF0[1]) return 6;
    base=D_8007BCD0[D_8007BEDA]; current=(u32)b->gfx;
    if(!silo_range(base,0x8FC0u,7u) || current<base || current>base+0x8FC0u || (current&7u)) return 7;
    if(current<SS.last_end) { SS.poisoned=1; silo_count(&SS.poison_count); return 8; }
    end=base+0x8FC0u;
    mtxbase=D_8007BCD8[D_8007BEDA]; mtx=(u32)b->mtx;
    if(!silo_range(mtxbase,0x12C00u,7u) || mtx<mtxbase || mtx>mtxbase+0x12C00u || ((mtx-mtxbase)&63u) || mtxbase+0x12C00u-mtx<576u) return 9;
    if(SS.extra_bytes>6176u || end-current<224u) return 10;
    if(D_8012C948[6]<0x80010F58u) return 11;
    SC.model=D_8012C948[6]-0x10F58u;
    if((u32)node!=SC.model+0x12B90u || !silo_graph(SC.model)) return 11;
    if(!silo_range(D_8012CF8C,18u*4u,3u) ||
       *(volatile u32*)(D_8012CF8C+2u*4u)!=0x800DCF48u ||
       *(volatile u32*)(D_8012CF8C+12u*4u)!=0x800DD56Cu ||
       *(volatile u32*)(D_8012CF8C+17u*4u)!=0x800DD65Cu) return 12;
    if(D_8012C948[28] || D_8012C948[24]!=0x801017D0u || D_8012C948[25]!=SC.actor || D_8012C948[26]) return 13;
    if((!D_8012C838 && D_8012CEC4) || (D_8012C838 &&
       !((D_8012CA11==1 && D_8012CEC4==0x8010E4C4u) ||
         ((D_8012CA11==2 || D_8012CA11==3) && !D_8012CEC4)))) return 13;
    flags=(*(volatile u16*)(SC.actor+0x72u))>>10;
    if(flags<1u || flags>11u || (*(volatile u32*)(SC.actor+0x78u)&0x80000u)) return 14;
    actor_data=(u32)func_80100368((Actor*)SC.actor);
    if(actor_data<0x80000000u || actor_data>0x807FFFC4u || (actor_data&3u) ||
       *(volatile u16*)(actor_data+4u)!=0x917u ||
       *(volatile u32*)(actor_data+0x38u)!=0x80107C2Cu) return 18;
    if((D_8012C948[0]&0x7F800000u)==0x7F800000u ||
       !silo_finite(D_8012C948[1],3) || !silo_finite(D_8012C948[2],3) ||
       (D_8012C948[3] && !silo_finite(D_8012C948[3],3)) ||
       !silo_finite((u32)D_8012CE58,16)) return 15;
    i=(u32)D_8012C968;
    if(!silo_range(i,0x290u,3u) || *(volatile u32*)i<9u || *(volatile u32*)(i+4u)<9u ||
       !silo_finite(i+0x50u,9u*16u)) return 15;
    e=&SS.silo[SC.slot];
    if(!(e->flags&TOOIE_BRIDGE_PALETTE_ROUTE) && e->birth==SC.birth && e->stamp==SC.epoch &&
       (e->seen_mask&(1u<<(SC.view*7u+SC.phase)))) return 16;
    /* The getter cannot cause a refusal: conservatively require one spare block even for stable binding. */
    if(SS.next_block>=TOOIE_BRIDGE_BLOCK_COUNT) return 17;
    SC.controller=(u32)func_80104248((Actor*)SC.actor);
    flags |= D_8012C948[3] ? 64u : 0u;
    renew=!(e->block_and_valid&TOOIE_BRIDGE_BLOCK_VALID) || e->birth!=SC.birth || e->pair!=SC.pair ||
        e->model!=SC.model || e->controller!=SC.controller || e->flags!=flags;
    if(!(e->flags&TOOIE_BRIDGE_PALETTE_ROUTE))
        for(i=0;i<10;++i) if(e->root[i]!=silo_root_word(i)) renew=1;
    if(renew) {
        silo_retire(e);
        if(e->birth!=SC.birth || (e->flags&TOOIE_BRIDGE_PALETTE_ROUTE)) { e->stamp=0; e->seen_mask=0; }
        e->birth=SC.birth; e->pair=SC.pair; e->model=SC.model; e->controller=SC.controller; e->flags=flags;
        for(i=0;i<10;++i) e->root[i]=silo_root_word(i);
        e->block_and_valid=SS.next_block|TOOIE_BRIDGE_BLOCK_VALID;
        SS.next_block+=1u; SS.silo_valid_count+=1u;
    }
    if(e->stamp!=SC.epoch) { e->stamp=SC.epoch; e->seen_mask=0; }
    e->seen_mask |= 1u<<(SC.view*7u+SC.phase);
    SC.idbase=TOOIE_BRIDGE_ID_BASE+(e->block_and_valid&0x7FFFFu)*512u;
    SC.rootnode=(u32)node; SC.admitted=1; SC.seen_roles=0; SC.active_roles=0;
    SS.extra_bytes+=224u;
    if(SS.extra_bytes>SS.extra_highwater) SS.extra_highwater=SS.extra_bytes;
    silo_count(&SS.silo_admissions);
    SS.silo_occurrence.epoch=SC.epoch; SS.silo_occurrence.slot=SC.slot; SS.silo_occurrence.birth=SC.birth;
    SS.silo_occurrence.pair=SC.pair; SS.silo_occurrence.model=SC.model;
    SS.silo_occurrence.view_phase=SC.view|(SC.phase<<2); SS.silo_occurrence.idbase=SC.idbase;
    SS.silo_occurrence.template_draw=*(volatile u32*)(actor_data+0x14u); SS.silo_occurrence.role_mask=0;
    for(i=0;i<9;++i) SS.silo_occurrence.mtx[i]=0;
    return 0;
}
