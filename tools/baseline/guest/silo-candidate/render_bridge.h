#ifndef TOOIE_BASELINE_RENDER_BRIDGE_H
#define TOOIE_BASELINE_RENDER_BRIDGE_H
#ifdef TOOIE_BRIDGE_GUEST
#include "common.h"
typedef u32 TooieBridgeWord;
#else
#include <stdint.h>
typedef uint32_t TooieBridgeWord;
#endif
#define TOOIE_BRIDGE_MAGIC 0x42524731u
#define TOOIE_BRIDGE_LAYOUT 3u
#define TOOIE_BRIDGE_STATE_BYTES 18844u
#define TOOIE_BRIDGE_MAX_BIRTH 0xFFFFFFFEu
#define TOOIE_BRIDGE_BLOCK_COUNT 0x80000u
#define TOOIE_BRIDGE_BLOCK_VALID 0x80000000u
#define TOOIE_BRIDGE_ID_BASE 0x90000000u
#define TOOIE_BRIDGE_SLOT_COUNT 200u
#define TOOIE_BRIDGE_REASON_COUNT 20u
#define TOOIE_BRIDGE_PALETTE_ROUTE 0x80000000u
#define TOOIE_BRIDGE_PALETTE_ADMITTED 3u
#define TOOIE_BRIDGE_CPU_ROOT_ROUTE 0x80000001u
#define TOOIE_BRIDGE_CPU_ROOT_ADMITTED 4u
#define TOOIE_BRIDGE_PALETTE_LEAF_POLICY 0x02010AA9u
#define TOOIE_BRIDGE_PALETTE_ROOT_POLICY 0x0201154Eu
enum TooieBridgeReason {
    TOOIE_BRIDGE_REENTRY = 0,
    TOOIE_BRIDGE_EPOCH = 1,
    TOOIE_BRIDGE_POISONED = 2,
    TOOIE_BRIDGE_POOL = 3,
    TOOIE_BRIDGE_OVERRIDE = 4,
    TOOIE_BRIDGE_RANGE = 5,
    TOOIE_BRIDGE_ALIGN = 6,
    TOOIE_BRIDGE_SLOT = 7,
    TOOIE_BRIDGE_BIRTH = 8,
    TOOIE_BRIDGE_SELECTOR = 9,
    TOOIE_BRIDGE_VIEW = 10,
    TOOIE_BRIDGE_PHASE = 11,
    TOOIE_BRIDGE_ANIMATION = 12,
    TOOIE_BRIDGE_EXPRESSION = 13,
    TOOIE_BRIDGE_DEFORM = 14,
    TOOIE_BRIDGE_MODEL_DATA = 15,
    TOOIE_BRIDGE_DUPLICATE = 16,
    TOOIE_BRIDGE_CAPACITY = 17,
    TOOIE_BRIDGE_BLOCKS = 18,
    TOOIE_BRIDGE_CURSOR_DROP = 19
};
struct TooieBridgeSlot {
    TooieBridgeWord birth, selector_pair, block_and_valid, stamp;
};
struct TooieSiloSlot {
    TooieBridgeWord birth, pair, block_and_valid, stamp, seen_mask, model, controller, flags;
    TooieBridgeWord root[10];
};
struct TooieSiloContext {
    TooieBridgeWord actor, descriptor, birth, pair, view, phase, epoch, buffers;
    TooieBridgeWord rootnode, model, controller, idbase, admitted, seen_roles, active_roles, slot;
};
struct TooieActorPaletteSlot {
    TooieBridgeWord birth, pair, block_and_valid, stamp, leaf0_seen, model, controller, flags;
    TooieBridgeWord camera_id[4], effective_selector, pose, leaf1_seen;
    TooieBridgeWord leaf_offset[2], camera_cut_views;
};
struct TooieActorPaletteContext {
    TooieBridgeWord actor, descriptor, birth, pair, view, phase, epoch, buffers;
    TooieBridgeWord gfx_begin, gfx_base, pool_index, idbase, admitted, leaf_role, open_group, slot;
};
struct TooieNpc665Slot {
    TooieBridgeWord birth, pair, block_and_valid, stamp, seen_mask, model, controller, flags;
    TooieBridgeWord camera_id[4], pose_handle, controller_handle, camera_cut_views, reserved[3];
};
struct TooieNpc665Sidecar {
    union {
        struct { TooieBridgeWord actor_data, capture_count, event; } pending;
        struct { TooieBridgeWord emitted_bones, current_bone_role, emitted_hybrid_calls; } admitted;
    } phase;
    TooieBridgeWord control;
};
struct TooieSiloOccurrence {
    TooieBridgeWord epoch, slot, birth, pair, model, view_phase, idbase, template_draw, role_mask;
    TooieBridgeWord mtx[9];
};
struct TooiePlayerBoundary {
    TooieBridgeWord enabled, owner, component, model, selector_handle, slot, view_phase;
    TooieBridgeWord stamp, idbase, buffers, base, pool_index, begin, end;
    TooieBridgeWord calls, admissions, refusals, last_reason, changes, reissues;
    TooieBridgeWord pending_refusals, tripwires, effective_model_asset;
    TooieBridgeWord emitted_bones;
    TooieBridgeWord current_bone_role, emitted_leaf_calls;
};
struct TooieRenderBridgeState {
    TooieBridgeWord magic;
    TooieBridgeWord layout;
    TooieBridgeWord epoch;
    TooieBridgeWord seen_epoch;
    TooieBridgeWord extra_bytes;
    TooieBridgeWord last_end;
    TooieBridgeWord poisoned;
    TooieBridgeWord active;
    TooieBridgeWord birth_cursor;
    TooieBridgeWord next_block;
    TooieBridgeWord calls;
    TooieBridgeWord admissions;
    TooieBridgeWord birth_count;
    TooieBridgeWord birth_saturated;
    TooieBridgeWord extra_highwater;
    TooieBridgeWord tripwire_count;
    TooieBridgeWord poison_count;
    TooieBridgeWord last_id;
    TooieBridgeWord last_slot;
    TooieBridgeWord last_birth;
    TooieBridgeWord raw_8c;
    TooieBridgeWord raw_94;
    TooieBridgeWord raw_7b;
    TooieBridgeWord raw_78;
    TooieBridgeWord raw_70;
    TooieBridgeWord refused[TOOIE_BRIDGE_REASON_COUNT];
    TooieBridgeWord births[TOOIE_BRIDGE_SLOT_COUNT];
    struct TooieBridgeSlot slots[TOOIE_BRIDGE_SLOT_COUNT];
    union {
        struct TooieSiloSlot silo[TOOIE_BRIDGE_SLOT_COUNT];
        struct TooieActorPaletteSlot palette[TOOIE_BRIDGE_SLOT_COUNT];
        struct TooieNpc665Slot npc665[TOOIE_BRIDGE_SLOT_COUNT];
    };
    TooieBridgeWord silo_valid_count, silo_depth;
    union {
        struct TooieSiloContext silo_context;
        struct TooieActorPaletteContext palette_context;
    };
    TooieBridgeWord silo_calls, silo_admissions, silo_refusals, silo_last_reason;
    struct TooieSiloOccurrence silo_occurrence;
    struct TooiePlayerBoundary player;
};
extern volatile struct TooieRenderBridgeState tooie_render_bridge_state;
#ifdef __cplusplus
#define TOOIE_BRIDGE_ASSERT static_assert
#else
#define TOOIE_BRIDGE_ASSERT _Static_assert
#endif
TOOIE_BRIDGE_ASSERT(sizeof(TooieBridgeWord) == 4 && sizeof(struct TooieBridgeSlot) == 16,
    "bridge word/slot ABI");
TOOIE_BRIDGE_ASSERT(sizeof(struct TooieRenderBridgeState) == TOOIE_BRIDGE_STATE_BYTES,
    "resident bridge state extent");
TOOIE_BRIDGE_ASSERT(__builtin_offsetof(struct TooieRenderBridgeState, refused) == 100 &&
    __builtin_offsetof(struct TooieRenderBridgeState, births) == 180 &&
    __builtin_offsetof(struct TooieRenderBridgeState, slots) == 980,
    "resident bridge state offsets");
TOOIE_BRIDGE_ASSERT(sizeof(struct TooieSiloSlot) == 72 &&
    sizeof(struct TooieSiloContext) == 64 && sizeof(struct TooieSiloOccurrence) == 72,
    "Silo record/context ABI");
TOOIE_BRIDGE_ASSERT(sizeof(struct TooieActorPaletteSlot) == 72 &&
    sizeof(struct TooieActorPaletteContext) == 64 &&
    __builtin_offsetof(struct TooieActorPaletteSlot, controller) == 24 &&
    __builtin_offsetof(struct TooieActorPaletteSlot, flags) == 28 &&
    __builtin_offsetof(struct TooieActorPaletteSlot, camera_id) == 32 &&
    __builtin_offsetof(struct TooieActorPaletteContext, admitted) == 48 &&
    __builtin_offsetof(struct TooieRenderBridgeState, palette) == 4180 &&
    __builtin_offsetof(struct TooieRenderBridgeState, palette_context) == 18588,
    "palette overlays existing record/context without resident growth");
TOOIE_BRIDGE_ASSERT(__builtin_offsetof(struct TooieRenderBridgeState, silo) == 4180 &&
    __builtin_offsetof(struct TooieRenderBridgeState, silo_valid_count) == 18580 &&
    __builtin_offsetof(struct TooieRenderBridgeState, silo_depth) == 18584 &&
    __builtin_offsetof(struct TooieRenderBridgeState, silo_context) == 18588 &&
    __builtin_offsetof(struct TooieRenderBridgeState, silo_calls) == 18652 &&
    __builtin_offsetof(struct TooieRenderBridgeState, silo_occurrence) == 18668,
    "Silo append-only layout2 offsets");
TOOIE_BRIDGE_ASSERT(sizeof(struct TooiePlayerBoundary) == 104 &&
    __builtin_offsetof(struct TooieRenderBridgeState, player) == 18740 &&
    __builtin_offsetof(struct TooiePlayerBoundary, effective_model_asset) == 88 &&
    __builtin_offsetof(struct TooiePlayerBoundary, emitted_bones) == 92 &&
    __builtin_offsetof(struct TooiePlayerBoundary, current_bone_role) == 96 &&
    __builtin_offsetof(struct TooiePlayerBoundary, emitted_leaf_calls) == 100,
    "player append-only layout3 extent");
TOOIE_BRIDGE_ASSERT(sizeof(struct TooieNpc665Slot)==72 &&
    __builtin_offsetof(struct TooieNpc665Slot, block_and_valid)==8 &&
    __builtin_offsetof(struct TooieNpc665Slot, controller)==24 &&
    __builtin_offsetof(struct TooieNpc665Slot, flags)==28 &&
    __builtin_offsetof(struct TooieRenderBridgeState, npc665)==4180 &&
    sizeof(struct TooieNpc665Sidecar)==16 &&
    __builtin_offsetof(struct TooieNpc665Sidecar, control)==12,
    "NPC retained union and separately registered sidecar ABI");
#undef TOOIE_BRIDGE_ASSERT
#endif
