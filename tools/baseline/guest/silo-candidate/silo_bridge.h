/* Isolated experiment: full preflight has one external noinline guest helper. */
#ifndef TOOIE_SILO_BRIDGE_H
#define TOOIE_SILO_BRIDGE_H
#define TOOIE_BRIDGE_GUEST 1
#include "render_bridge.h"
/* compile_objects(shared=True) is used only by camera-pool compositions.
 * Standalone bridge storage retains its original physical extent. This bound
 * permits paid in-draw operations; admission spans use TOOIE_BRIDGE_LOWER_LIST_BYTES. */
#ifndef TOOIE_BRIDGE_PHYSICAL_LIST_BYTES
#define TOOIE_BRIDGE_PHYSICAL_LIST_BYTES 0x8FC0u
#endif
#if TOOIE_BRIDGE_PHYSICAL_LIST_BYTES != 0x8FC0u && TOOIE_BRIDGE_PHYSICAL_LIST_BYTES != 0x5BEC0u
#error Invalid bridge physical command-list extent
#endif
/* Lower admission span: standalone storage keeps 0x8FC0. In the camera-pool composition it is the
 * physical extent minus the 6400-byte camera tail (camera_identity.h TOOIE_BRIDGE_LOWER_BYTES). */
#if TOOIE_BRIDGE_PHYSICAL_LIST_BYTES == 0x8FC0u
#define TOOIE_BRIDGE_LOWER_LIST_BYTES 0x8FC0u
#else
#define TOOIE_BRIDGE_LOWER_LIST_BYTES (TOOIE_BRIDGE_PHYSICAL_LIST_BYTES-6400u)
#endif
#define SI static inline __attribute__((always_inline))
#define SS tooie_render_bridge_state
#define SC SS.silo_context
#include "npc665_owner.h"
extern Unk80132ED0 D_80132ED0[200];
extern u8 D_80135450[25];
extern u32* D_80136EE0;
extern u32 D_8007BCD0[2], D_8007BCD8[2], D_8007BCE8[2], D_8007BCF0[2];
extern s16 D_8007BEDA;
extern u32 D_8012C948[], D_8012C838, D_8012CF8C, D_8012CEC4;
extern u8 D_8012CA11;
extern s32 D_8012C824;
extern u32 D_8012C840;
extern void* volatile D_8012C968;
extern u8 D_8012CE58[];
void* func_80104248(Actor*);
void* func_80100368(Actor*);
void* func_800AE080(s16);
s32 func_800A89F8(void), func_800EB854(void);
typedef struct TooieRenderBuffers { Gfx* volatile gfx; Mtx* volatile mtx; } SiloBuffers;
SI void silo_count(volatile u32* v) { if (*v != 0xFFFFFFFFu) *v += 1u; }
SI int silo_range(u32 p, u32 n, u32 mask) {
    return p >= 0x80000000u && p <= 0x81000000u-n && !(p & mask);
}
SI u32 silo_pair(const Unk80132ED0* d) { return ((u32)d->unk14<<16)|d->unk16; }
#if TOOIE_NPC665 && TOOIE_ACTOR_PALETTE
/* Original title53 event1F reaches 015D0; metadata prefix is two halfwords. */
SI int title53_owner(u32 actor,u32 pair,u32 data) {
    return pair==0x065E0000u && (*(volatile u16*)(actor+0x76u)>>7)==0x53u &&
        (*(volatile u32*)(actor+0x74u)&0x10000000u) && silo_range(data,0x44u,3u) &&
        *(volatile u16*)(data+2u)==0x317u && *(volatile u32*)(data+0x38u)==0x80107C2Cu &&
        *(volatile u32*)(data+0x40u)==0x80803D0Cu;
}
void* func_80100094(Actor*,u32);
#endif
SI void silo_epoch(void) {
    if (SS.epoch != SS.seen_epoch) {
        SS.seen_epoch=SS.epoch; SS.extra_bytes=0; SS.last_end=0; SS.poisoned=0;
        SS.primary_bytes=0;
        SS.secondary_ledger.bytes=0;
    }
}
SI void silo_retire(volatile struct TooieSiloSlot* e) {
    if (e->block_and_valid & TOOIE_BRIDGE_BLOCK_VALID) {
#if TOOIE_NPC665 && !(defined(TOOIE_BK_PIPELINE) && TOOIE_BK_PIPELINE)
        /* @tooie L06c-1: BK builds no NPC665 admission (L06b-3), so SC.admitted is never
           TOOIE_NPC665_ADMITTED there and this hop could only read the sidecar. */
        if(e->flags==TOOIE_NPC665_ROUTE || e->flags==TOOIE_SKELETAL_ROUTE)
            npc665_invalidated(((u32)e-(u32)SS.silo)/72u);
#endif
        if (SS.silo_valid_count) SS.silo_valid_count -= 1u;
        e->block_and_valid=0;
    }
}
SI int silo_finite(u32 p, u32 count) {
    u32 i;
    if (!silo_range(p,count*4u,3u)) return 0;
    for (i=0;i<count;++i) if ((*(volatile u32*)(p+i*4u)&0x7F800000u)==0x7F800000u) return 0;
    return 1;
}
/* DE498 consumes C968 as AE080(descriptor+0x10) or shared scratch C840. This renewal-only
 * key joins the direct pointer, or C824 input class 1/2 for scratch; never C840's address
 * and no proof of producer. AE080 is unchecked; source allocates handles 1..249 (8001B668). */
SI u32 actor_pose_key(u32 descriptor,u32 pose) {
    s32 handle=*(volatile s16*)(descriptor+0x10u);
    u32 scratch=*(volatile u32*)&D_8012C840;
    if (!handle) return pose==scratch ? (*(volatile u32*)&D_8012C824 ? 2u : 1u) : 0u;
    if (handle<0 || handle>=250 || pose==scratch) return 0;
    return (u32)func_800AE080((s16)handle)==pose ? pose : 0u;
}
SI u32 silo_root_word(u32 i) {
    if (!i) return D_8012C948[0];
    if (i<=3) return *(volatile u32*)(D_8012C948[1]+(i-1u)*4u);
    if (i<=6) return *(volatile u32*)(D_8012C948[2]+(i-4u)*4u);
    return D_8012C948[3] ? *(volatile u32*)(D_8012C948[3]+(i-7u)*4u) : 0;
}
SI u32 silo_role(u32 node, u32 root) {
    if (node==root) return 1;
    if (node==root+0x68u) return 2;
    if (node==root+0x88u) return 3;
    if (node==root+0xA8u) return 4;
    if (node==root+0xC8u) return 5;
    if (node==root+0xE8u) return 6;
    if (node==root+0x108u) return 7;
    if (node==root+0x128u) return 8;
    if (node==root+0x148u) return 9;
    return 0;
}
SI int silo_leaf(u32 p,u32 n,u32 ordinal,u32 target,int second) {
    u32 i,w,op;
    if (!silo_range(p,n*8u,7u)) return 0;
    for(i=0;i<n;++i) {
        w=*(volatile u32*)(p+i*8u); op=w>>24;
        if (i==n-1u) { if (w!=0xDF000000u || *(volatile u32*)(p+i*8u+4u)!=0) return 0; }
        else if (i==ordinal || (second && i==20u)) {
            if(w!=0xDE000000u || *(volatile u32*)(p+i*8u+4u)!=(i==ordinal?target:0x07000000u)) return 0;
        } else if (!(op==0x01u||op==0x05u||op==0x06u||op==0xD7u||op==0xD9u||op==0xE3u||op==0xE6u||op==0xE7u||op==0xF0u||op==0xF2u||op==0xF3u||op==0xF5u||op==0xFCu||op==0xFDu)) return 0;
    }
    return 1;
}
SI int silo_shared(u32 p,u32 op) {
    return (*(volatile u32*)p>>24)==op && *(volatile u32*)(p+8)==0xDF000000u && *(volatile u32*)(p+12)==0;
}
SI int silo_graph(u32 model) {
    if(!silo_range(model,77056u,7u)) return 0;
    if(*(volatile u32*)(model+4)!=0x12B90u || *(volatile u16*)(model+0xAu)!=0 ||
       *(volatile u32*)(model+0xCu)!=0x10F58u || *(volatile u32*)(model+0x18u)!=0x11B58u ||
       *(volatile u32*)(model+0x28u)!=0) return 0;
    if(*(volatile u32*)(model+0x12B90u)!=2u || *(volatile u32*)(model+0x12B94u)!=0u || *(volatile u8*)(model+0x12B98u)!=16u || *(volatile s8*)(model+0x12B99u)!=0) return 0;
    if(*(volatile u32*)(model+0x12BA0u)!=17u || *(volatile u32*)(model+0x12BA4u)!=16u || *(volatile u16*)(model+0x12BA8u)!=0u || *(volatile u16*)(model+0x12BAAu)!=0u || *(volatile u16*)(model+0x12BACu)!=64u || *(volatile u16*)(model+0x12BAEu)!=0u) return 0;
    if(*(volatile u32*)(model+0x12BB0u)!=17u || *(volatile u32*)(model+0x12BB4u)!=16u || *(volatile u16*)(model+0x12BB8u)!=51u || *(volatile u16*)(model+0x12BBAu)!=64u || *(volatile u16*)(model+0x12BBCu)!=32u || *(volatile u16*)(model+0x12BBEu)!=0u) return 0;
    if(*(volatile u32*)(model+0x12BC0u)!=12u || *(volatile u32*)(model+0x12BC4u)!=56u || *(volatile u16*)(model+0x12BC8u)!=2u || *(volatile u16*)(model+0x12BCAu)!=3u || *(volatile u32*)(model+0x12BCCu)!=24u || *(volatile u32*)(model+0x12BD0u)!=40u) return 0;
    if(*(volatile u32*)(model+0x12BD8u)!=17u || *(volatile u32*)(model+0x12BDCu)!=0u || *(volatile u16*)(model+0x12BE0u)!=86u || *(volatile u16*)(model+0x12BE2u)!=96u || *(volatile u16*)(model+0x12BE4u)!=16u || *(volatile u16*)(model+0x12BE6u)!=0u) return 0;
    if(*(volatile u32*)(model+0x12BE8u)!=17u || *(volatile u32*)(model+0x12BECu)!=0u || *(volatile u16*)(model+0x12BF0u)!=104u || *(volatile u16*)(model+0x12BF2u)!=112u || *(volatile u16*)(model+0x12BF4u)!=16u || *(volatile u16*)(model+0x12BF6u)!=0u) return 0;
    if(*(volatile u32*)(model+0x12BF8u)!=2u || *(volatile u32*)(model+0x12BFCu)!=32u || *(volatile u8*)(model+0x12C00u)!=16u || *(volatile s8*)(model+0x12C01u)!=1) return 0;
    if(*(volatile u32*)(model+0x12C08u)!=17u || *(volatile u32*)(model+0x12C0Cu)!=0u || *(volatile u16*)(model+0x12C10u)!=122u || *(volatile u16*)(model+0x12C12u)!=128u || *(volatile u16*)(model+0x12C14u)!=8u || *(volatile u16*)(model+0x12C16u)!=0u) return 0;
    if(*(volatile u32*)(model+0x12C18u)!=2u || *(volatile u32*)(model+0x12C1Cu)!=32u || *(volatile u8*)(model+0x12C20u)!=16u || *(volatile s8*)(model+0x12C21u)!=2) return 0;
    if(*(volatile u32*)(model+0x12C28u)!=17u || *(volatile u32*)(model+0x12C2Cu)!=0u || *(volatile u16*)(model+0x12C30u)!=144u || *(volatile u16*)(model+0x12C32u)!=136u || *(volatile u16*)(model+0x12C34u)!=9u || *(volatile u16*)(model+0x12C36u)!=0u) return 0;
    if(*(volatile u32*)(model+0x12C38u)!=2u || *(volatile u32*)(model+0x12C3Cu)!=32u || *(volatile u8*)(model+0x12C40u)!=16u || *(volatile s8*)(model+0x12C41u)!=3) return 0;
    if(*(volatile u32*)(model+0x12C48u)!=17u || *(volatile u32*)(model+0x12C4Cu)!=0u || *(volatile u16*)(model+0x12C50u)!=166u || *(volatile u16*)(model+0x12C52u)!=145u || *(volatile u16*)(model+0x12C54u)!=8u || *(volatile u16*)(model+0x12C56u)!=0u) return 0;
    if(*(volatile u32*)(model+0x12C58u)!=2u || *(volatile u32*)(model+0x12C5Cu)!=32u || *(volatile u8*)(model+0x12C60u)!=16u || *(volatile s8*)(model+0x12C61u)!=4) return 0;
    if(*(volatile u32*)(model+0x12C68u)!=17u || *(volatile u32*)(model+0x12C6Cu)!=0u || *(volatile u16*)(model+0x12C70u)!=188u || *(volatile u16*)(model+0x12C72u)!=153u || *(volatile u16*)(model+0x12C74u)!=8u || *(volatile u16*)(model+0x12C76u)!=0u) return 0;
    if(*(volatile u32*)(model+0x12C78u)!=2u || *(volatile u32*)(model+0x12C7Cu)!=32u || *(volatile u8*)(model+0x12C80u)!=16u || *(volatile s8*)(model+0x12C81u)!=5) return 0;
    if(*(volatile u32*)(model+0x12C88u)!=17u || *(volatile u32*)(model+0x12C8Cu)!=0u || *(volatile u16*)(model+0x12C90u)!=210u || *(volatile u16*)(model+0x12C92u)!=161u || *(volatile u16*)(model+0x12C94u)!=9u || *(volatile u16*)(model+0x12C96u)!=0u) return 0;
    if(*(volatile u32*)(model+0x12C98u)!=2u || *(volatile u32*)(model+0x12C9Cu)!=32u || *(volatile u8*)(model+0x12CA0u)!=16u || *(volatile s8*)(model+0x12CA1u)!=6) return 0;
    if(*(volatile u32*)(model+0x12CA8u)!=17u || *(volatile u32*)(model+0x12CACu)!=0u || *(volatile u16*)(model+0x12CB0u)!=232u || *(volatile u16*)(model+0x12CB2u)!=170u || *(volatile u16*)(model+0x12CB4u)!=10u || *(volatile u16*)(model+0x12CB6u)!=0u) return 0;
    if(*(volatile u32*)(model+0x12CB8u)!=2u || *(volatile u32*)(model+0x12CBCu)!=32u || *(volatile u8*)(model+0x12CC0u)!=16u || *(volatile s8*)(model+0x12CC1u)!=7) return 0;
    if(*(volatile u32*)(model+0x12CC8u)!=17u || *(volatile u32*)(model+0x12CCCu)!=0u || *(volatile u16*)(model+0x12CD0u)!=254u || *(volatile u16*)(model+0x12CD2u)!=180u || *(volatile u16*)(model+0x12CD4u)!=8u || *(volatile u16*)(model+0x12CD6u)!=0u) return 0;
    if(*(volatile u32*)(model+0x12CD8u)!=2u || *(volatile u32*)(model+0x12CDCu)!=0u || *(volatile u8*)(model+0x12CE0u)!=16u || *(volatile s8*)(model+0x12CE1u)!=8) return 0;
    if(*(volatile u32*)(model+0x12CE8u)!=17u || *(volatile u32*)(model+0x12CECu)!=0u || *(volatile u16*)(model+0x12CF0u)!=276u || *(volatile u16*)(model+0x12CF2u)!=188u || *(volatile u16*)(model+0x12CF4u)!=8u || *(volatile u16*)(model+0x12CF6u)!=0u) return 0;
    if(!silo_leaf(model+0x10F60u,51u,14u,0x3000010u,0)) return 0;
    if(!silo_leaf(model+0x110F8u,35u,14u,0x3000010u,0)) return 0;
    if(!silo_leaf(model+0x11210u,18u,11u,0x30001a0u,0)) return 0;
    if(!silo_leaf(model+0x112A0u,18u,11u,0x30001a0u,0)) return 0;
    if(!silo_leaf(model+0x11330u,22u,14u,0x3000010u,1)) return 0;
    if(!silo_leaf(model+0x113E0u,22u,14u,0x3000010u,1)) return 0;
    if(!silo_leaf(model+0x11490u,22u,14u,0x3000010u,1)) return 0;
    if(!silo_leaf(model+0x11540u,22u,14u,0x3000010u,1)) return 0;
    if(!silo_leaf(model+0x115F0u,22u,14u,0x3000010u,1)) return 0;
    if(!silo_leaf(model+0x116A0u,22u,14u,0x3000010u,1)) return 0;
    if(!silo_leaf(model+0x11750u,22u,14u,0x3000010u,1)) return 0;
    if(!silo_leaf(model+0x11800u,22u,14u,0x3000010u,1)) return 0;
    if(!silo_shared(0x8011B9C0u,0xE2u)) return 0;
    if(!silo_shared(0x8011BB50u,0xE2u)) return 0;
    if(!silo_shared(0x8011E540u,0xE2u)) return 0;
    if(!silo_shared(0x8011E6D0u,0xE2u)) return 0;
    if(!silo_shared(0x8011C840u,0xE2u)) return 0;
    if(!silo_shared(0x8011C9D0u,0xE2u)) return 0;
    if(!silo_shared(0x8011F3C0u,0xE2u)) return 0;
    if(!silo_shared(0x8011F550u,0xE2u)) return 0;
    if(!silo_shared(0x8011D6C0u,0xE2u)) return 0;
    if(!silo_shared(0x8011D850u,0xE2u)) return 0;
    if(!silo_shared(0x80120240u,0xE2u)) return 0;
    if(!silo_shared(0x801203D0u,0xE2u)) return 0;
    if(!silo_shared(0x801210C0u,0xE2u)) return 0;
    if(!silo_shared(0x80121250u,0xE2u)) return 0;
    if(!silo_shared(0x80121F40u,0xE2u)) return 0;
    if(!silo_shared(0x801220D0u,0xE2u)) return 0;
    if(!silo_shared(0x80123418u,0xD9u)) return 0;
    if(!silo_shared(0x80123428u,0xD9u)) return 0;
    return 1;
}
SI int silo_binding(u32 actor,u32 descriptor,u32 slot,u32 birth,u32 pair);
SI void silo_invalidate(u32 controller);
u32 tooie_silo_preflight(SiloBuffers* b,u8* node);
SI u32 silo_begin(SiloBuffers* b,u8* node) {
    u32 role,reason;
    if(!SC.actor || (SC.pair>>16)!=0x917u || SS.silo_depth!=1u || SC.buffers!=(u32)b) return 0;
    if(!SC.admitted) {
        if(D_8012C948[6]<0x80010F58u || (u32)node!=D_8012C948[6]-0x10F58u+0x12B90u) return 0;
        reason=tooie_silo_preflight(b,node);
        if(reason) { silo_count(&SS.silo_refusals); SS.silo_last_reason=reason; SC.admitted=2; return 0; }
    }
    if(SC.admitted!=1u) return 0;
    role=silo_role((u32)node,SC.rootnode);
    if(!role || *(volatile s8*)(node+9)!=(s32)(role-1u) || (SC.seen_roles&(1u<<(role-1u)))) return 0;
    SC.seen_roles|=1u<<(role-1u); SC.active_roles|=1u<<(role-1u);
    return role;
}
/* @recomp Root-role Enable, then a public MODELVIEW group with role-derived ID.
 * Word2 is the named packed leaf policy; word3 is reserved zero. */
SI void silo_push(SiloBuffers* b,u32 role,u32 applied) {
    volatile u32* p=(volatile u32*)b->gfx;
    if(role==1u) { p[0]=0xE0525464u; p[1]=0x10000064u; p+=2; }
    p[0]=0x6400000Cu; p[1]=SC.idbase+SC.view*128u+SC.phase*16u+role;
    p[2]=TOOIE_BRIDGE_PALETTE_LEAF_POLICY; p[3]=0;
    b->gfx=(Gfx*)(p+4);
    SS.silo_occurrence.mtx[role-1u]=applied;
    SS.silo_occurrence.role_mask|=1u<<(role-1u);
}
/* @recomp Public MODELVIEW group pop: word1 has count1 and projection-selector0. */
SI void silo_pop(SiloBuffers* b,u32 role) {
    volatile u32* p=(volatile u32*)b->gfx;
    p[0]=0x6400000Du; p[1]=1u; b->gfx=(Gfx*)(p+2);
    SC.active_roles &= ~(1u<<(role-1u));
    if((u32)b->gfx>SS.last_end) SS.last_end=(u32)b->gfx;
    if(role==1u) {
        if(SC.seen_roles==0x1FFu && !SC.active_roles && SC.epoch==SS.epoch && SS.silo_occurrence.role_mask==0x1FFu)
            SS.silo_occurrence.role_mask|=0x80000000u;
        SC.admitted=2;
    }
}
SI int actor_binding(u32 actor,u32 descriptor,u32 slot,u32 birth,u32 pair) {
    volatile u32* v;
    u32 stride,begin,end,mem_end,index;
    if(slot>=200u || descriptor!=(u32)&D_80132ED0[slot] || !birth || birth>TOOIE_BRIDGE_MAX_BIRTH ||
       SS.births[slot]!=birth || !(*(volatile u8*)&D_80135450[slot>>3]&(0x80u>>(slot&7u)))) return 0;
    if(actor<0x80000000u || actor>0x807FFF64u || (actor&3u) ||
       *(volatile u32*)actor!=descriptor || silo_pair((Unk80132ED0*)descriptor)!=pair) return 0;
    v=(volatile u32*)D_80136EE0;
    if((u32)v<0x80000000u || (u32)v>0x807FFFF0u || ((u32)v&3u)) return 0;
    stride=v[0];begin=v[1];end=v[2];mem_end=v[3];
    if(stride<0x9Cu || stride>0xFFFFu || begin<0x80000000u || begin>=0x80800000u ||
       end<begin || end>=0x80800000u || mem_end<end || mem_end>=0x80800000u) return 0;
    index=*(volatile u16*)(descriptor+0x1Au)>>5;
    if((index+1u)*stride>end-begin || begin+index*stride!=actor) return 0;
    return 1;
}
SI int silo_binding(u32 actor,u32 descriptor,u32 slot,u32 birth,u32 pair) {
    return (pair>>16)==0x917u && actor_binding(actor,descriptor,slot,birth,pair);
}
SI void silo_invalidate(u32 controller) {
    u32 i;
    volatile struct TooieSiloSlot* e;
    if(!SS.silo_valid_count) return;
    for(i=0;i<200u;++i) {
        e=&SS.silo[i];
        if(!(e->block_and_valid&TOOIE_BRIDGE_BLOCK_VALID)) continue;
        if(!(*(volatile u8*)&D_80135450[i>>3]&(0x80u>>(i&7u))) || !e->birth ||
           e->birth!=SS.births[i] || e->controller==controller) silo_retire(e);
    }
}
u32 tooie_player_begin(SiloBuffers* buffers, u32 model);
void tooie_player_end(SiloBuffers* buffers);
u32 tooie_player_bone_push(SiloBuffers* buffers, s32 index);
void tooie_player_bone_pop(SiloBuffers* buffers, u32 token);
u32 tooie_player_leaf_begin(SiloBuffers* buffers);
void tooie_player_leaf_end(SiloBuffers* buffers);
#endif
