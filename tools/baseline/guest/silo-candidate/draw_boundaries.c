/* Original draw boundaries with scoped public-label envelopes for admitted draws. */
#include "silo_bridge.h"
#include "actor_palette.h"
#if !defined(TOOIE_ACTOR_PALETTE) || (TOOIE_ACTOR_PALETTE != 0 && TOOIE_ACTOR_PALETTE != 1)
#error Explicit boolean TOOIE_ACTOR_PALETTE selection required
#endif
#if !defined(TOOIE_PLAYER_SECONDARY) || (TOOIE_PLAYER_SECONDARY != 0 && TOOIE_PLAYER_SECONDARY != 1)
#error Explicit boolean TOOIE_PLAYER_SECONDARY selection required
#endif
#if !defined(TOOIE_BK_PIPELINE) || (TOOIE_BK_PIPELINE != 0 && TOOIE_BK_PIPELINE != 1)
#error Explicit boolean TOOIE_BK_PIPELINE selection required
#endif
#if TOOIE_BK_PIPELINE
#include "bk_pipeline.h"
#endif
#define PATCH __attribute__((section(".recomp_patch"), noinline))
typedef struct GraphicsBuffers GraphicsBuffers;
void func_800DF47C(s32 (*callback)(Actor*), Actor* actor);
f32* func_801027F4(Actor* actor);
f32* func_801018D8(Actor* actor);
s32 func_8010347C(void* descriptor);
GraphicsBuffers* func_800A7180(void);
void* func_800DE498(GraphicsBuffers*, f32*, f32*, f32, f32*, s32);
void* func_800DE448(f32*, f32*, f32, f32*, s32);
u32 tooie_player_begin(SiloBuffers*, u32);
void tooie_player_end(SiloBuffers*);
#if TOOIE_PLAYER_SECONDARY
u32 tooie_player_secondary_begin(SiloBuffers*, u32);
#endif
_Static_assert(sizeof(void*) == 4 && sizeof(f32) == 4 &&
    __builtin_offsetof(Actor, unk0) == 0 &&
    __builtin_offsetof(Actor, position) == 4 &&
    __builtin_offsetof(Actor, scale) == 0x38, "original draw boundary ABI");

#if TOOIE_BK_PIPELINE
/* B4 (BK selection): BK __marker_draw owner scope. The actor capture stays only for the retained
 * actor-palette path (DEFB0 projection policy, shadow silhouette); the NPC665 pending capture
 * is retired. The BK-OFF body below is the original row boundary, unchanged. */
PATCH void* func_80101808(Actor* actor, s32 (*callback)(Actor*)) {
    func_800DF47C(callback, actor);
    f32* rotation = func_801027F4(actor);
    f32* extra = func_801018D8(actor);
    s32 model = func_8010347C(actor->unk0);
    f32 scale = actor->scale;
    u32 owns=0, slot, offset;
    Unk80132ED0* descriptor=actor->unk0;
    if(!SC.actor && !SS.silo_depth) {
        offset=(u32)descriptor-(u32)D_80132ED0;
        if((u32)descriptor>=(u32)D_80132ED0 && offset<200u*0x30u && !(offset%0x30u)) {
            slot=offset/0x30u;
            if(actor_binding((u32)actor,(u32)descriptor,slot,SS.births[slot],silo_pair(descriptor))) {
                SC.actor=(u32)actor; SC.descriptor=(u32)descriptor; SC.slot=slot;
                SC.birth=SS.births[slot]; SC.pair=silo_pair(descriptor); SC.epoch=SS.epoch;
                SC.view=(u32)func_800A89F8(); SC.phase=(u32)func_800EB854();
                SC.admitted=0; SC.buffers=0; owns=1;
            }
        }
    }
    /* @recomp BK __marker_draw: set the actor's creation-generation transform ID around the draw. */
    u32 bkToken=tooie_bk_actor_draw_begin((u32)actor,(u32)model);
    void* result=func_800DE448(actor->position, rotation, scale, extra, model);
    tooie_bk_scope_end(bkToken);
    if(owns) { SC.actor=0; SC.admitted=0; }
    return result;
}
#else
PATCH void* func_80101808(Actor* actor, s32 (*callback)(Actor*)) {
    func_800DF47C(callback, actor);
    f32* rotation = func_801027F4(actor);
    f32* extra = func_801018D8(actor);
    s32 model = func_8010347C(actor->unk0);
    f32 scale = actor->scale;
    /* @recomp Scoped actor/descriptor/birth/view/phase capture after the original callback
       and resolved draw arguments. */
    u32 owns=0, slot, offset;
    Unk80132ED0* descriptor=actor->unk0;
    if(!SC.actor && !SS.silo_depth) {
        offset=(u32)descriptor-(u32)D_80132ED0;
        if((u32)descriptor>=(u32)D_80132ED0 && offset<200u*0x30u && !(offset%0x30u)) {
            slot=offset/0x30u;
            if(actor_binding((u32)actor,(u32)descriptor,slot,SS.births[slot],silo_pair(descriptor))) {
                SC.actor=(u32)actor; SC.descriptor=(u32)descriptor; SC.slot=slot;
                SC.birth=SS.births[slot]; SC.pair=silo_pair(descriptor); SC.epoch=SS.epoch;
                SC.view=(u32)func_800A89F8(); SC.phase=(u32)func_800EB854();
                SC.admitted=0; SC.buffers=0; owns=1;
#if TOOIE_NPC665
                npc665_pending((u32)callback);
#endif
            }
        }
    }
    void* result=func_800DE448(actor->position, rotation, scale, extra, model);
    /* @recomp Matching NPC/context cleanup after the original DE448 call. */
    if(owns) {
#if TOOIE_NPC665
        npc665_reset();
#endif
        SC.actor=0; SC.admitted=0;
    }
    return result;
}
#endif

#if TOOIE_NPC665
/* DF47C stages C9A8/C9AC together; DE318 clears C9A8 on every DE498 exit.
 * position==actor+4 is the CD4 world branch: C20/C48 pass temporaries and DF944
 * never reaches DE448. The ordinary 80101808 owner stays unchanged. */
SI u32 direct_world_capture(u32 actor) {
    u32 descriptor,offset,slot,pair;
    if(!silo_range(actor,4u,3u)) return 0;
    descriptor=*(volatile u32*)actor; offset=descriptor-(u32)D_80132ED0;
    if(descriptor<(u32)D_80132ED0 || offset>=200u*0x30u || offset%0x30u) return 0;
    slot=offset/0x30u; pair=silo_pair((Unk80132ED0*)descriptor);
    /* 80103140 publishes the resource at descriptor+0x14. The model is resolved later. */
    if(!actor_binding(actor,descriptor,slot,SS.births[slot],pair)) return 0;
    if(!skeletal_resource(pair>>16)) {
#if TOOIE_ACTOR_PALETTE
        if(pair!=0x065E0000u || !title53_owner(actor,pair,(u32)func_80100368((Actor*)actor))) return 0;
#else
        return 0;
#endif
    }
    SC.actor=actor; SC.descriptor=descriptor; SC.slot=slot;
    SC.birth=SS.births[slot]; SC.pair=pair; SC.epoch=SS.epoch;
    SC.view=(u32)func_800A89F8(); SC.phase=(u32)func_800EB854();
    SC.admitted=0; SC.buffers=0;
    return 1;
}
#endif

#if TOOIE_BK_PIPELINE
/* B4 (BK selection): NPC665 direct-world capture/checked end and the PC[8] secondary begin are
 * retired. Depth, buffer publication, the player begin/end (primary identity read by the
 * retained shadow route) and the actor-palette close (retained DEFB0/shadow path) stay. The
 * BK-OFF body below is the original row boundary, unchanged. */
PATCH void* func_800DE448(f32* position, f32* rotation, f32 scale,
                         f32* extra, s32 model) {
    /* @recomp BK owner selection for player, direct-world and nested draws. */
    u32 bkToken=tooie_bk_de448_begin((u32)position,(u32)model);
    u32 increment=SS.silo_depth!=0xFFFFFFFFu;
    if(increment) SS.silo_depth+=1u;
    GraphicsBuffers* buffers = func_800A7180();
    if(increment && SS.silo_depth==1u && SC.actor) SC.buffers=(u32)buffers;
    u32 player=0;
    if(increment && SS.silo_depth==1u && D_8012C948[24]==0x80092234u)
        player=tooie_player_begin((SiloBuffers*)buffers,(u32)model);
    void* result=func_800DE498(buffers, position, rotation, scale, extra, model);
    /* @recomp BK closes before the retained closes below (LIFO group order). */
    tooie_bk_de448_end(bkToken,(u32)buffers);
    /* Close terms kept verbatim: retired admissions are never set, so their terms stay inert. */
#if TOOIE_ACTOR_PALETTE
    if(increment && SS.silo_depth==1u && (SC.admitted==TOOIE_BRIDGE_PALETTE_ADMITTED ||
        SC.admitted==TOOIE_BRIDGE_CPU_ROOT_ADMITTED
#if TOOIE_NPC665
        || SC.admitted==TOOIE_BRIDGE_TITLE53_ADMITTED
#endif
        ))
#else
    if(increment && SS.silo_depth==1u && SC.admitted==TOOIE_BRIDGE_PALETTE_ADMITTED)
#endif
        tooie_actor_palette_end((SiloBuffers*)buffers);
    if(player) tooie_player_end((SiloBuffers*)buffers);
    if(increment) SS.silo_depth-=1u;
    return result;
}
#else
PATCH void* func_800DE448(f32* position, f32* rotation, f32 scale,
                         f32* extra, s32 model) {
    /* @recomp Direct-world owner capture, depth protection, buffer publication and player
       begin around the original buffer getter. */
    u32 owns=0;
#if TOOIE_NPC665
    owns=!SC.actor && !SS.silo_depth && D_8012C948[24]==0x801017D0u &&
        (u32)position==D_8012C948[25]+4u && direct_world_capture(D_8012C948[25]);
#endif
    u32 increment=SS.silo_depth!=0xFFFFFFFFu;
    if(increment) SS.silo_depth+=1u;
    GraphicsBuffers* buffers = func_800A7180();
    if(increment && SS.silo_depth==1u && SC.actor) SC.buffers=(u32)buffers;
    u32 player=0;
    if(increment && SS.silo_depth==1u && D_8012C948[24]==0x80092234u)
        player=tooie_player_begin((SiloBuffers*)buffers,(u32)model);
#if TOOIE_PLAYER_SECONDARY
    /* @recomp PC[8] secondary: only the 93864(mode1) occurrence its route token armed. */
    else if(increment && SS.silo_depth==1u && D_8012C948[24]==0x80092224u)
        player=tooie_player_secondary_begin((SiloBuffers*)buffers,(u32)model);
#endif
    void* result=func_800DE498(buffers, position, rotation, scale, extra, model);
    /* @recomp NPC/actor/player close, depth release and capture cleanup after the original DE498. */
#if TOOIE_NPC665
    if(increment && SS.silo_depth==1u && SC.admitted==TOOIE_NPC665_ADMITTED)
        baseline_npc665_end_checked((SiloBuffers*)buffers);
#endif
#if TOOIE_ACTOR_PALETTE
    if(increment && SS.silo_depth==1u && (SC.admitted==TOOIE_BRIDGE_PALETTE_ADMITTED ||
        SC.admitted==TOOIE_BRIDGE_CPU_ROOT_ADMITTED
#if TOOIE_NPC665
        || SC.admitted==TOOIE_BRIDGE_TITLE53_ADMITTED
#endif
        ))
#else
    if(increment && SS.silo_depth==1u && SC.admitted==TOOIE_BRIDGE_PALETTE_ADMITTED)
#endif
        tooie_actor_palette_end((SiloBuffers*)buffers);
    if(player) tooie_player_end((SiloBuffers*)buffers);
    if(increment) SS.silo_depth-=1u;
    if(owns) { SC.actor=0; SC.admitted=0; }
    return result;
}
#endif
