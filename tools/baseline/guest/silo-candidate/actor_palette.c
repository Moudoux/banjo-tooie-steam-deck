/* One original flag40 palette leaf, enclosed by the original root/camera pair. */
#define F3DEX_GBI_2
#include "silo_bridge.h"
#include "actor_palette.h"
#include "rt64_extended_gbi.h"
#define PALETTE __attribute__((noinline, section(".text.actor_palette")))
#if !defined(TOOIE_ACTOR_PALETTE) || (TOOIE_ACTOR_PALETTE != 0 && TOOIE_ACTOR_PALETTE != 1)
#error Explicit boolean TOOIE_ACTOR_PALETTE selection required
#endif
#define AC SS.palette_context
#define REFUSE(reason) do { SS.silo_last_reason=(reason); silo_count(&SS.silo_refusals); return 0; } while (0)
extern u32 D_8012C820, D_8012C850[], D_8012CA18;
extern u8 D_80124740[];
extern void* D_80136E70;
void* func_80104148(s32);
void* func_80104200(s16);
void* func_800AE080(s16);

/* Original immutable assets are a premise; this recognizes a finite producer,
 * not arbitrary display lists. No triangle/cache simulation or recursive calls. */
SI int palette_offset(u32 model, u32 offset, u32 bytes, u32 mask) {
    return offset<=0x81000000u-model && silo_range(model+offset,bytes,mask);
}
SI u32 palette_leaf(u32 model, u32 poses, u32 leaves[2]) {
    u32 node=*(volatile u32*)(model+4u), selected=0, list=0, last_selector=0;
    u32 ordinal, next, selector, child, i, word, argument, op, depth=0, saw_matrix=0;
    s32 index;
    leaves[0]=leaves[1]=0;
    for (ordinal=0; ordinal<2u; ++ordinal) {
        if (!palette_offset(model,node,16u,3u)) return 0;
        u32 p=model+node;
        next=*(volatile u32*)(p+4u); selector=*(volatile u16*)(p+10u);
        child=*(volatile u32*)(p+12u);
        if (*(volatile u32*)p!=12u || *(volatile u16*)(p+8u)!=1u ||
            !selector || selector>=60u || selector==last_selector ||
            D_8012C850[selector]>1u || child<16u || child>0x1000000u ||
            !palette_offset(model,node+child,12u,3u)) return 0;
        last_selector=selector; leaves[ordinal]=node+child; p=model+leaves[ordinal];
        if (*(volatile u32*)p!=3u || *(volatile u32*)(p+4u)) return 0;
        index=*(volatile s16*)(p+8u);
        if (index<0) return 0;
        if (D_8012C850[selector]) {
            if (selected) return 0;
            selected=ordinal+1u;
            list=*(volatile u32*)(model+12u)+8u+(u32)index*8u;
        }
        if (!next) break;
        if (next<child+12u || next>0x1000000u) return 0;
        node+=next;
    }
    if (next || !selected) return 0;
    for (i=0; i<256u; ++i) {
        if (!palette_offset(model,list+i*8u,8u,7u)) return 0;
        word=*(volatile u32*)(model+list+i*8u);
        argument=*(volatile u32*)(model+list+i*8u+4u); op=word>>24;
        if (op==0xDFu) return word==0xDF000000u && !argument && !depth && saw_matrix ? selected : 0;
        if (op==0xDAu) {
            if (word!=0xDA380002u || (argument>>24)!=5u ||
                (argument&63u) || (argument&0xFFFFFFu)>=poses*64u || depth>=31u) return 0;
            ++depth; saw_matrix=1;
        } else if (op==0xD8u) {
            if (word!=0xD8380002u || !argument || (argument&63u) || argument/64u>depth) return 0;
            depth-=argument/64u;
        } else if (op==0x01u) {
            if (!saw_matrix || !depth || (argument>>24)!=1u) return 0;
        } else if (op==0xDEu) {
            if (word!=0xDE000000u || !(argument==0x03000040u || argument==0x07000000u)) return 0;
        } else if (!(op==0x05u || op==0x06u || op==0xD7u || op==0xD9u ||
            op==0xE2u || op==0xE3u || op==0xE6u || op==0xE7u || op==0xF0u ||
            op==0xF2u || op==0xF3u || op==0xF5u || op==0xFCu || op==0xFDu)) return 0;
    }
    return 0;
}

PALETTE u32 tooie_actor_palette_begin(struct TooieRenderBuffers* b,
    u32* local_gfx_cursor, u32 model, u32 camera_id) {
    u32 base, current, mtxbase, mtx, pose, poses, controller=0, class_slot, actor_data;
    u32 i, role, leaves[2], renew, seen, view_bit, mode, static_list, controller_pool;
    s32 pose_handle, controller_handle, stride, capacity;
    volatile struct TooieActorPaletteSlot* e;
    if (!AC.actor || AC.admitted || SS.silo_depth!=1u || AC.buffers!=(u32)b) return 0;
    silo_count(&SS.silo_calls);
    silo_epoch();
    if (!SS.epoch || SS.epoch==0xFFFFFFFFu || AC.epoch!=SS.epoch) REFUSE(TOOIE_BRIDGE_EPOCH);
    if (SS.poisoned || SS.active) REFUSE(TOOIE_BRIDGE_POISONED);
    if (!actor_binding(AC.actor,AC.descriptor,AC.slot,AC.birth,AC.pair)) REFUSE(TOOIE_BRIDGE_BIRTH);
    if (AC.view>3u || !camera_id) REFUSE(TOOIE_BRIDGE_VIEW);
    if (!(AC.phase==0u || AC.phase==1u || AC.phase==2u || AC.phase==5u || AC.phase==6u))
        REFUSE(TOOIE_BRIDGE_PHASE);
    if ((u32)b!=0x80127658u || D_8007BEDA<0 || D_8007BEDA>1 ||
        D_8007BCE8[0] || D_8007BCE8[1] || D_8007BCF0[0] || D_8007BCF0[1]) REFUSE(TOOIE_BRIDGE_POOL);
    if (!silo_range((u32)local_gfx_cursor,4u,3u)) REFUSE(TOOIE_BRIDGE_RANGE);
    base=D_8007BCD0[D_8007BEDA]; current=*local_gfx_cursor;
    if (!silo_range(base,0x8FC0u,7u) || current<base || current>base+0x8FC0u || (current&7u))
        REFUSE(TOOIE_BRIDGE_RANGE);
    if (current<SS.last_end) {
        SS.poisoned=1; silo_count(&SS.poison_count); REFUSE(TOOIE_BRIDGE_CURSOR_DROP);
    }
    if (!silo_range(model,0x40u,7u) || *(volatile u16*)(model+10u)!=0x40u ||
        *(volatile u32*)(model+0x28u) || D_8012C820 || D_8012C838 || D_8012CEC4 ||
        D_8012C948[14] || D_8012C948[26] || D_8012C948[28] ||
        D_8012C948[24]!=0x801017D0u || D_8012C948[25]!=AC.actor) REFUSE(TOOIE_BRIDGE_OVERRIDE);
    if (!palette_offset(model,*(volatile u32*)(model+12u),8u,7u) ||
        D_8012C948[6]!=model+*(volatile u32*)(model+12u) ||
        !palette_offset(model,*(volatile u32*)(model+16u),24u,3u) ||
        D_8012C948[7]!=model+*(volatile u32*)(model+16u)) REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    if (!silo_range(D_8012CF8C,13u*4u,3u) || D_8012CF8C!=*(volatile u32*)0x80123198u ||
        *(volatile u32*)(D_8012CF8C+3u*4u)!=0x800DD024u ||
        *(volatile u32*)(D_8012CF8C+12u*4u)!=0x800DD56Cu) REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    actor_data=(u32)func_80100368((Actor*)AC.actor);
    if (!silo_range(actor_data,0x3Cu,3u) || *(volatile u32*)(actor_data+0x38u)!=0x80107C2Cu)
        REFUSE(TOOIE_BRIDGE_BIRTH);
    pose=(u32)D_8012C968;
    if (!silo_range(pose,0x50u,3u)) REFUSE(TOOIE_BRIDGE_ANIMATION);
    poses=*(volatile u32*)pose;
    if (!poses || poses>32u || *(volatile u32*)(pose+4u)<poses ||
        !silo_finite(pose+0x10u,(poses+1u)*16u) || !silo_finite((u32)D_8012CE58,16u))
        REFUSE(TOOIE_BRIDGE_ANIMATION);
    controller_pool=(u32)D_80136E70;
    if (!silo_range(controller_pool,4u,3u)) REFUSE(TOOIE_BRIDGE_ANIMATION);
    stride=*(volatile s16*)controller_pool; capacity=*(volatile s16*)(controller_pool+2u);
    if (stride<0x30 || capacity<=1 || !silo_range(controller_pool,4u+(u32)stride*(u32)capacity,3u))
        REFUSE(TOOIE_BRIDGE_ANIMATION);
    for (i=0; i<11u; ++i) {
        class_slot=(u32)func_80104148((s32)i);
        if (!silo_range(class_slot,20u,3u)) REFUSE(TOOIE_BRIDGE_ANIMATION);
        if (!*(volatile u8*)(class_slot+4u) || *(volatile u8*)(class_slot+5u)!=1u ||
            (*(volatile u16*)(D_80124740+i*8u+4u)&3u)) continue;
        pose_handle=*(volatile s16*)class_slot;
        if (pose_handle<=0 || pose_handle>=250 || (u32)func_800AE080((s16)pose_handle)!=pose) continue;
        controller_handle=*(volatile s16*)(class_slot+2u);
        if (controller || controller_handle<=0 || controller_handle>=capacity) REFUSE(TOOIE_BRIDGE_ANIMATION);
        controller=(u32)func_80104200((s16)controller_handle);
        if (controller!=controller_pool+4u+(u32)controller_handle*(u32)stride ||
            !silo_range(controller,0x30u,3u)) REFUSE(TOOIE_BRIDGE_ANIMATION);
    }
    if (!controller) REFUSE(TOOIE_BRIDGE_ANIMATION);
    role=palette_leaf(model,poses,leaves);
    if (!role) REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    /* DE498 binds segment3 from this original table; E7FF8 owns segment7.
     * Both possible culling lists and the selected material list must terminate. */
    mode=D_8012C948[56];
    if (mode>4u) REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    static_list=*(volatile u32*)(0x80122DC8u+mode*8u+(D_8012C948[23]==255u ? 0u : 4u));
    if (!silo_range(static_list,0x50u,7u) || !silo_shared(static_list+0x40u,0xE2u) ||
        !silo_shared(0x80123418u,0xD9u) || !silo_shared(0x80123428u,0xD9u)) REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    mtxbase=D_8007BCD8[D_8007BEDA]; mtx=(u32)b->mtx;
    if (!silo_range(mtxbase,0x12C00u,7u) || mtx<mtxbase || mtx>mtxbase+0x12C00u ||
        ((mtx-mtxbase)&63u) || mtxbase+0x12C00u-mtx<(poses+2u)*64u ||
        SS.extra_bytes>6368u || base+0x8FC0u-current<96u) REFUSE(TOOIE_BRIDGE_CAPACITY);
    e=&SS.palette[AC.slot];
    renew=!(e->block_and_valid&TOOIE_BRIDGE_BLOCK_VALID) || e->birth!=AC.birth || e->pair!=AC.pair ||
        e->flags!=TOOIE_BRIDGE_PALETTE_ROUTE || e->model!=model || e->controller!=controller ||
        e->pose!=pose || e->effective_selector!=D_8012CA18 ||
        e->leaf_offset[0]!=leaves[0] || e->leaf_offset[1]!=leaves[1];
    seen=1u<<(AC.view*7u+AC.phase); view_bit=1u<<AC.view;
    if (!renew) {
        if (e->stamp!=SS.epoch) {
            e->stamp=SS.epoch; e->leaf0_seen=e->leaf1_seen=0; e->camera_cut_views=0;
        }
        if (e->camera_id[AC.view] && e->camera_id[AC.view]!=camera_id) e->camera_cut_views|=view_bit;
        e->camera_id[AC.view]=camera_id;
        if (e->camera_cut_views&view_bit) REFUSE(TOOIE_BRIDGE_VIEW);
        if ((role==1u ? e->leaf0_seen : e->leaf1_seen)&seen) REFUSE(TOOIE_BRIDGE_DUPLICATE);
    }
    if (renew) {
        if (SS.next_block>=TOOIE_BRIDGE_BLOCK_COUNT) REFUSE(TOOIE_BRIDGE_BLOCKS);
        silo_retire(&SS.silo[AC.slot]);
        e->birth=AC.birth; e->pair=AC.pair; e->model=model; e->controller=controller;
        e->flags=TOOIE_BRIDGE_PALETTE_ROUTE; e->pose=pose; e->effective_selector=D_8012CA18;
        e->leaf_offset[0]=leaves[0]; e->leaf_offset[1]=leaves[1];
        e->stamp=SS.epoch; e->leaf0_seen=e->leaf1_seen=e->camera_cut_views=0;
        for (i=0; i<4u; ++i) e->camera_id[i]=0;
        e->camera_id[AC.view]=camera_id;
        e->block_and_valid=SS.next_block|TOOIE_BRIDGE_BLOCK_VALID;
        ++SS.next_block; ++SS.silo_valid_count;
    }
    if (role==1u) e->leaf0_seen|=seen; else e->leaf1_seen|=seen;
    AC.idbase=TOOIE_BRIDGE_ID_BASE+(e->block_and_valid&0x7FFFFu)*512u;
    AC.gfx_base=base; AC.pool_index=(u32)D_8007BEDA; AC.gfx_begin=current; AC.leaf_role=role;
    AC.admitted=TOOIE_BRIDGE_PALETTE_ADMITTED; AC.open_group=1u;
    SS.extra_bytes+=32u;
    if (SS.extra_bytes>SS.extra_highwater) SS.extra_highwater=SS.extra_bytes;
    SS.active=1;
    GfxCommand* cursor=(GfxCommand*)current;
    gEXEnable(cursor);
    gEXMatrixGroup(cursor+1,AC.idbase+AC.view*128u+AC.phase*16u+role,G_EX_INTERPOLATE_SIMPLE,
        G_EX_PUSH,G_MTX_MODELVIEW,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,
        G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,G_EX_COMPONENT_INTERPOLATE,
        G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO,G_EX_ORDER_LINEAR,G_EX_EDIT_NONE,
        G_EX_ASPECT_AUTO,G_EX_COMPONENT_SKIP,G_EX_COMPONENT_AUTO);
    *local_gfx_cursor=(u32)(cursor+3);
    silo_count(&SS.silo_admissions);
    return AC.idbase+AC.view*128u+AC.phase*16u;
}

#if TOOIE_ACTOR_PALETTE
/* Immutable 065E's fifteen unconditional type17 lists inherit one root WORLD.
 * Original C964 input stability until parsing remains the baseline premise. */
PALETTE u32 tooie_actor_cpu_root_begin(struct TooieRenderBuffers* b,
    u32* local_gfx_cursor, u32 model, u32 camera_id) {
    u32 base, current, actor_data, renew, seen, view_bit, i;
    volatile struct TooieActorPaletteSlot* e;
    GfxCommand* cursor;

    /* Nonclass draws preserve both the original stream and existing counters. */
    if (!AC.actor || AC.admitted || SS.silo_depth != 1u || AC.buffers != (u32)b ||
        D_8012C948[24] != 0x801015D0u || D_8012CA18 != 0x65Eu ||
        !silo_range(model, 14952u, 7u) || *(volatile u16*)(model + 10u) != 0u)
        return 0;
    silo_count(&SS.silo_calls);
    silo_epoch();
    if (!SS.epoch || SS.epoch == 0xFFFFFFFFu || AC.epoch != SS.epoch)
        REFUSE(TOOIE_BRIDGE_EPOCH);
    if (SS.poisoned || SS.active) REFUSE(TOOIE_BRIDGE_POISONED);
    if (!actor_binding(AC.actor, AC.descriptor, AC.slot, AC.birth, AC.pair))
        REFUSE(TOOIE_BRIDGE_BIRTH);
    if (AC.view > 3u || !camera_id) REFUSE(TOOIE_BRIDGE_VIEW);
    if (!(AC.phase == 0u || AC.phase == 1u || AC.phase == 2u ||
        AC.phase == 5u || AC.phase == 6u)) REFUSE(TOOIE_BRIDGE_PHASE);
    if ((u32)b != 0x80127658u || D_8007BEDA < 0 || D_8007BEDA > 1 ||
        D_8007BCE8[0] || D_8007BCE8[1] || D_8007BCF0[0] || D_8007BCF0[1])
        REFUSE(TOOIE_BRIDGE_POOL);
    if (!silo_range((u32)local_gfx_cursor, 4u, 3u)) REFUSE(TOOIE_BRIDGE_RANGE);
    base = D_8007BCD0[D_8007BEDA];
    current = *local_gfx_cursor;
    if (!silo_range(base, 0x8FC0u, 7u) || current < base ||
        current > base + 0x8FC0u || (current & 7u)) REFUSE(TOOIE_BRIDGE_RANGE);
    if (current < SS.last_end) {
        SS.poisoned = 1;
        silo_count(&SS.poison_count);
        REFUSE(TOOIE_BRIDGE_CURSOR_DROP);
    }
    /* 3024 decimal is 0xBD0. Do not use the review's hexadecimal typo. */
    if (*(volatile u32*)(model + 4u) != 14712u ||
        *(volatile u32*)(model + 12u) != 424u ||
        *(volatile u32*)(model + 0x18u) != 6936u ||
        *(volatile u32*)(model + 0x28u) != 0xBD0u ||
        *(volatile s16*)(model + 6940u) != 19 || D_8012C948[56] > 3u ||
        D_8012C820 || D_8012C838 || D_8012CEC4 || D_8012C948[14] ||
        D_8012C948[26] || D_8012C948[28] || D_8012C948[25] != AC.actor ||
        D_8012C948[6] != model + 424u) REFUSE(TOOIE_BRIDGE_OVERRIDE);
    if (!silo_range(D_8012CF8C, 18u * 4u, 3u) ||
        D_8012CF8C != *(volatile u32*)0x80123198u ||
        *(volatile u32*)(D_8012CF8C + 17u * 4u) != 0x800DD65Cu)
        REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    actor_data = (u32)func_80100368((Actor*)AC.actor);
    if (!silo_range(actor_data, 0x3Cu, 3u) ||
        *(volatile u32*)(actor_data + 0x38u) != 0x80107C2Cu)
        REFUSE(TOOIE_BRIDGE_BIRTH);
    if (SS.extra_bytes > 6368u || base + 0x8FC0u - current < 96u)
        REFUSE(TOOIE_BRIDGE_CAPACITY);
    e = &SS.palette[AC.slot];
    renew = !(e->block_and_valid & TOOIE_BRIDGE_BLOCK_VALID) ||
        e->birth != AC.birth || e->pair != AC.pair ||
        e->flags != TOOIE_BRIDGE_CPU_ROOT_ROUTE || e->model != model;
    seen = 1u << (AC.view * 7u + AC.phase);
    view_bit = 1u << AC.view;
    if (!renew) {
        if (e->stamp != SS.epoch) {
            e->stamp = SS.epoch;
            e->leaf0_seen = e->leaf1_seen = e->camera_cut_views = 0;
        }
        if (e->camera_id[AC.view] && e->camera_id[AC.view] != camera_id)
            e->camera_cut_views |= view_bit;
        e->camera_id[AC.view] = camera_id;
        if (e->camera_cut_views & view_bit) REFUSE(TOOIE_BRIDGE_VIEW);
        if (e->leaf0_seen & seen) REFUSE(TOOIE_BRIDGE_DUPLICATE);
    }
    if (renew) {
        if (SS.next_block >= TOOIE_BRIDGE_BLOCK_COUNT) REFUSE(TOOIE_BRIDGE_BLOCKS);
        silo_retire(&SS.silo[AC.slot]);
        e->birth = AC.birth;
        e->pair = AC.pair;
        e->model = model;
        e->flags = TOOIE_BRIDGE_CPU_ROOT_ROUTE;
        e->stamp = SS.epoch;
        e->leaf0_seen = e->leaf1_seen = e->camera_cut_views = 0;
        for (i = 0; i < 4u; ++i) e->camera_id[i] = 0;
        e->camera_id[AC.view] = camera_id;
        e->block_and_valid = SS.next_block | TOOIE_BRIDGE_BLOCK_VALID;
        ++SS.next_block;
        ++SS.silo_valid_count;
    }
    e->leaf0_seen |= seen;
    AC.idbase = TOOIE_BRIDGE_ID_BASE + (e->block_and_valid & 0x7FFFFu) * 512u;
    AC.gfx_base = base;
    AC.pool_index = (u32)D_8007BEDA;
    AC.gfx_begin = current;
    AC.leaf_role = 1u;
    AC.admitted = TOOIE_BRIDGE_CPU_ROOT_ADMITTED;
    AC.open_group = 1u;
    SS.extra_bytes += 32u;
    if (SS.extra_bytes > SS.extra_highwater) SS.extra_highwater = SS.extra_bytes;
    SS.active = 1;
    cursor = (GfxCommand*)current;
    gEXEnable(cursor);
    gEXMatrixGroup(cursor + 1, AC.idbase + AC.view * 128u + AC.phase * 16u + 1u,
        G_EX_INTERPOLATE_SIMPLE, G_EX_PUSH, G_MTX_MODELVIEW,
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE,
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_AUTO, G_EX_COMPONENT_AUTO,
        G_EX_ORDER_LINEAR, G_EX_EDIT_NONE, G_EX_ASPECT_AUTO,
        G_EX_COMPONENT_SKIP, G_EX_COMPONENT_AUTO);
    *local_gfx_cursor = (u32)(cursor + 3);
    silo_count(&SS.silo_admissions);
    return AC.idbase + AC.view * 128u + AC.phase * 16u;
}

#define FEATHER_ROUTE 0x80000002u
extern u8* volatile D_8012C960;

/* Selected 6EA leaves own final billboard WORLDs, not a pose palette. */
PALETTE u32 tooie_actor_feather_leaf_id(struct TooieRenderBuffers* b,
    u8* node, u32 camera_id) {
    u32 model, role, base, current, actor_data, renew, seen, view_bit, i;
    volatile struct TooieActorPaletteSlot* e;
    volatile u32* role_seen;

    if (!AC.actor || AC.admitted || SS.active || SS.silo_depth != 1u ||
        AC.buffers != (u32)b || D_8012CA18 != 0x6EAu ||
        D_8012C948[24] != 0x801017D0u) return 0;
    model = D_8012C948[6] - 12976u;
    if (!silo_range(model, 17676u, 7u) ||
        *(volatile u32*)(model + 12u) != 12976u) return 0;
    role = (u32)node == model + 17552u ? 1u :
        (u32)node == model + 17608u ? 2u :
        (u32)node == model + 17664u ? 3u : 0u;
    if (!role || *(volatile u32*)node != 3u ||
        *(volatile u32*)(node + 4u) ||
        *(volatile s16*)(node + 8u) != (s32)(63u + (role - 1u) * 18u)) return 0;

    silo_count(&SS.silo_calls);
    silo_epoch();
    if (!SS.epoch || SS.epoch == 0xFFFFFFFFu || AC.epoch != SS.epoch)
        REFUSE(TOOIE_BRIDGE_EPOCH);
    if (SS.poisoned) REFUSE(TOOIE_BRIDGE_POISONED);
    if (!actor_binding(AC.actor, AC.descriptor, AC.slot, AC.birth, AC.pair))
        REFUSE(TOOIE_BRIDGE_BIRTH);
    if (AC.view > 3u || !camera_id) REFUSE(TOOIE_BRIDGE_VIEW);
    if (!(AC.phase == 0u || AC.phase == 1u || AC.phase == 2u ||
        AC.phase == 5u || AC.phase == 6u)) REFUSE(TOOIE_BRIDGE_PHASE);
    if ((u32)b != 0x80127658u || D_8007BEDA < 0 || D_8007BEDA > 1 ||
        D_8007BCE8[0] || D_8007BCE8[1] || D_8007BCF0[0] || D_8007BCF0[1])
        REFUSE(TOOIE_BRIDGE_POOL);
    base = D_8007BCD0[D_8007BEDA];
    current = (u32)b->gfx;
    if (!silo_range(base, 0x8FC0u, 7u) || current < base ||
        current > base + 0x8FC0u || (current & 7u)) REFUSE(TOOIE_BRIDGE_RANGE);
    if (current < SS.last_end) {
        SS.poisoned = 1;
        silo_count(&SS.poison_count);
        REFUSE(TOOIE_BRIDGE_CURSOR_DROP);
    }
    if (*(volatile u32*)(model + 4u) != 17120u ||
        *(volatile u16*)(model + 10u) || *(volatile u32*)(model + 0x18u) != 15792u ||
        *(volatile u32*)(model + 0x28u) || (u32)D_8012C960 != model + 12976u ||
        D_8012C820 || D_8012C838 || D_8012CEC4 || D_8012C948[14] ||
        D_8012C948[26] || D_8012C948[28] || D_8012C948[25] != AC.actor ||
        D_8012C948[56] > 3u) REFUSE(TOOIE_BRIDGE_OVERRIDE);
    if (!silo_range(D_8012CF8C, 13u * 4u, 3u) ||
        D_8012CF8C != *(volatile u32*)0x80123198u ||
        *(volatile u32*)(D_8012CF8C + 0u * 4u) != 0x800DC634u ||
        *(volatile u32*)(D_8012CF8C + 1u * 4u) != 0x800DC958u ||
        *(volatile u32*)(D_8012CF8C + 2u * 4u) != 0x800DCF48u ||
        *(volatile u32*)(D_8012CF8C + 3u * 4u) != 0x800DD024u ||
        *(volatile u32*)(D_8012CF8C + 12u * 4u) != 0x800DD56Cu)
        REFUSE(TOOIE_BRIDGE_MODEL_DATA);
    actor_data = (u32)func_80100368((Actor*)AC.actor);
    if (!silo_range(actor_data, 0x3Cu, 3u) ||
        *(volatile u32*)(actor_data + 0x38u) != 0x80107C2Cu)
        REFUSE(TOOIE_BRIDGE_BIRTH);
    if (*(volatile u32*)(AC.actor + 0x1Cu) || *(volatile u32*)(AC.actor + 0x30u) ||
        D_8012C850[2] != 1u) REFUSE(TOOIE_BRIDGE_SELECTOR);
    for (i = 1u; i <= 8u; ++i)
        if (i != 2u && D_8012C850[i]) REFUSE(TOOIE_BRIDGE_SELECTOR);
    if (SS.extra_bytes > 6368u || base + 0x8FC0u - current < 40u)
        REFUSE(TOOIE_BRIDGE_CAPACITY);

    e = &SS.palette[AC.slot];
    renew = !(e->block_and_valid & TOOIE_BRIDGE_BLOCK_VALID) ||
        e->birth != AC.birth || e->pair != AC.pair || e->model != model ||
        e->flags != FEATHER_ROUTE;
    seen = 1u << (AC.view * 7u + AC.phase);
    view_bit = 1u << AC.view;
    if (renew) {
        if (SS.next_block >= TOOIE_BRIDGE_BLOCK_COUNT) REFUSE(TOOIE_BRIDGE_BLOCKS);
        silo_retire(&SS.silo[AC.slot]);
        e->birth = AC.birth;
        e->pair = AC.pair;
        e->model = model;
        e->flags = FEATHER_ROUTE;
        e->controller = 0;
        e->stamp = SS.epoch;
        e->leaf0_seen = e->leaf1_seen = e->leaf_offset[0] = e->camera_cut_views = 0;
        for (i = 0u; i < 4u; ++i) e->camera_id[i] = 0;
        e->block_and_valid = SS.next_block | TOOIE_BRIDGE_BLOCK_VALID;
        ++SS.next_block;
        ++SS.silo_valid_count;
    } else if (e->stamp != SS.epoch) {
        e->stamp = SS.epoch;
        e->leaf0_seen = e->leaf1_seen = e->leaf_offset[0] = e->camera_cut_views = 0;
    }
    if (e->camera_id[AC.view] && e->camera_id[AC.view] != camera_id)
        e->camera_cut_views |= view_bit;
    e->camera_id[AC.view] = camera_id;
    if (e->camera_cut_views & view_bit) REFUSE(TOOIE_BRIDGE_VIEW);
    role_seen = role == 1u ? &e->leaf0_seen :
        role == 2u ? &e->leaf1_seen : &e->leaf_offset[0];
    if (*role_seen & seen) REFUSE(TOOIE_BRIDGE_DUPLICATE);
    *role_seen |= seen;
    SS.extra_bytes += 32u;
    if (SS.extra_bytes > SS.extra_highwater) SS.extra_highwater = SS.extra_bytes;
    silo_count(&SS.silo_admissions);
    return TOOIE_BRIDGE_ID_BASE + (e->block_and_valid & 0x7FFFFu) * 512u +
        AC.view * 128u + AC.phase * 16u + role;
}
#endif

PALETTE void tooie_actor_palette_end(struct TooieRenderBuffers* b) {
    u32 current, base, pool;
#if TOOIE_ACTOR_PALETTE
    u32 route;
    if ((AC.admitted != TOOIE_BRIDGE_PALETTE_ADMITTED &&
        AC.admitted != TOOIE_BRIDGE_CPU_ROOT_ADMITTED) || !AC.open_group || SS.silo_depth != 1u) return;
    route = AC.admitted == TOOIE_BRIDGE_CPU_ROOT_ADMITTED ?
        TOOIE_BRIDGE_CPU_ROOT_ROUTE : TOOIE_BRIDGE_PALETTE_ROUTE;
#else
    if (AC.admitted!=TOOIE_BRIDGE_PALETTE_ADMITTED || !AC.open_group || SS.silo_depth!=1u) return;
#endif
    pool=AC.pool_index;
    current=AC.buffers==(u32)b && (u32)b==0x80127658u ? (u32)b->gfx : 0;
    base=pool<2u ? D_8007BCD0[pool] : 0;
    if (!SS.active || !actor_binding(AC.actor,AC.descriptor,AC.slot,AC.birth,AC.pair) ||
        !(SS.palette[AC.slot].block_and_valid&TOOIE_BRIDGE_BLOCK_VALID) ||
#if TOOIE_ACTOR_PALETTE
        SS.palette[AC.slot].flags != route) {
#else
        SS.palette[AC.slot].flags!=TOOIE_BRIDGE_PALETTE_ROUTE) {
#endif
        silo_count(&SS.tripwire_count); SS.poisoned=1;
    }
    /* The saved pool and occurrence own the pop after original scalar cleanup. */
    if (AC.buffers!=(u32)b || pool>1u || D_8007BEDA!=(s32)pool || base!=AC.gfx_base || AC.epoch!=SS.epoch ||
        !silo_range(base,0x8FC0u,7u) || AC.gfx_begin<base || AC.gfx_begin>base+0x8FC0u-96u ||
        current<AC.gfx_begin+24u ||
        current>base+0x8FC0u-8u || (current&7u)) {
        silo_count(&SS.tripwire_count); SS.poisoned=1;
    } else {
        /* A successful occurrence owns its prepaid pop, even after poisoning. */
        GfxCommand* cursor=(GfxCommand*)current;
        gEXPopMatrixGroup(cursor,G_MTX_MODELVIEW);
        b->gfx=(Gfx*)(cursor+1);
        if ((u32)b->gfx>SS.last_end) SS.last_end=(u32)b->gfx;
    }
    AC.open_group=0; AC.admitted=2u; SS.active=0;
}
