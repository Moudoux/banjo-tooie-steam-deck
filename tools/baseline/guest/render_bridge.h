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
#define TOOIE_BRIDGE_LAYOUT 1u
#define TOOIE_BRIDGE_STATE_BYTES 4180u
#define TOOIE_BRIDGE_MAX_BIRTH 0xFFFFFFFEu
#define TOOIE_BRIDGE_BLOCK_COUNT 0x80000u
#define TOOIE_BRIDGE_BLOCK_VALID 0x80000000u
#define TOOIE_BRIDGE_ID_BASE 0x90000000u
#define TOOIE_BRIDGE_SLOT_COUNT 200u
#define TOOIE_BRIDGE_REASON_COUNT 20u
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
#undef TOOIE_BRIDGE_ASSERT
#endif
