/* Authored ordinary guest wrapper. Original model, pose, dispatcher, matrix
 * builders and display lists are called unchanged. No native admission policy. */
#define F3DEX_GBI_2
#include "common.h"
#include "memory.h"
#include "core2/anctrl.h"
#include "core2/1EB5E70.h"
#include "overlays/ba/anim.h"
#include "overlays/ba/backpack.h"
#include "overlays/ba/playerstate.h"
#include "rt64_extended_gbi.h"
#include "render_abi.h"
#include "wrapper_policy.h"
#include "scene_abi.h"
#define TOOIE_RECOMP_PATCH __attribute__((section(".recomp_patch")))

typedef struct { Gfx* gfx; Mtx* mtx; } TooieGraphicsBuffers;
_Static_assert(sizeof(void*) == 4 && sizeof(TooieGraphicsBuffers) == 8 &&
    sizeof(GfxCommand) == 8 && sizeof(Mtx) == 64, "original guest graphics ABI");
_Static_assert(sizeof(BaBackpack) == 24 &&
    __builtin_offsetof(PlayerState, backpack) == 0x14 &&
    __builtin_offsetof(PlayerState, unk184) == 0x184, "original owner ABI");

extern volatile GfxCommand tooie_gfx_pools[2][TOOIE_GFX_POOL_COMMANDS];
extern PlayerState* D_80135490[8];
extern u32 D_8007BCD0[2], D_8007BCD8[2], D_8007BCE8[2], D_8007BCF0[2];
extern s16 D_8007BEDA;
extern volatile s16 D_80127634;
extern s32 D_8007BF00;
extern u32 D_8012C838, D_8012C83C, D_8012C844, D_8012C848;
extern u32 D_8012C948[15], D_8012C9A8, D_8012C9B0, D_8012CEC4;
extern u32 D_8012CF8C, D_80123198[3], D_8012CA28;
extern u32 D_8012D5E0;
extern s32 D_8012C824;
extern u32 D_8012C840;
extern volatile u8 D_8012762C, D_8012B3F2, D_80127760;
extern volatile s8 D_8012B3F1;
extern Gfx D_80123418[], D_80123428[];
TooieGraphicsBuffers* func_800A7180(void);
void* func_800DE498(TooieGraphicsBuffers*, f32*, f32*, f32, f32*, s32);
s32 func_800A89F8(void), func_800EB854(void), func_8008CA74(PlayerState*);
s32 func_8080021C_babackpack(PlayerState*), func_80100AC4(s32);
void func_800DF738(s32), func_800DF818(s32), func_800DF720(s32);
void func_80098E64(PlayerState*);
void func_800A0714(PlayerState*, s32);
void func_8008B304(AnimCtrl*);
/* Mechanical capture only: none returns an admission decision. */
void tooie_wrapper_attachment_begin(PlayerState*);
void tooie_wrapper_attachment_end(PlayerState*);
void tooie_wrapper_source_span_begin(u32, void*, u32, u32);
void tooie_wrapper_source_span_end(void*);

/* Local occurrence proof only; no new saved provider state. */
struct TooieSceneSample {
    struct TooieSceneTuple32 tuple;
    u8 level, gameType, frontendMode;
    s8 saveSlot;
};

static inline __attribute__((always_inline)) int tooie_wrapper_scene_read(struct TooieSceneSample* s) {
    struct TooieSceneTuple32* t = &s->tuple;
    u32 low, high;
    s->level = D_8012762C;
    s->saveSlot = D_8012B3F1;
    s->gameType = D_8012B3F2;
    s->frontendMode = D_80127760;
    if (!tooie_get_scene_tuple32(t) || (t->rawFlags & ~TOOIE_SCENE_FLAGS) ||
        t->revisionHigh > 0x00FFFFFFu ||
        (t->revisionHigh == 0x00FFFFFFu && t->revisionLow == 0xFFFFFFFFu) ||
        (t->taskLow == 0xFFFFFFFFu && t->taskHigh == 0xFFFFFFFFu)) return 0;
    low = t->taskLow + 1u;
    high = t->taskHigh + (low == 0u);
    return low == tooie_wrapper_state.taskLow && high == tooie_wrapper_state.taskHigh &&
        t->epochLow == tooie_wrapper_state.epochLow &&
        t->epochHigh == tooie_wrapper_state.epochHigh;
}

static inline __attribute__((always_inline)) int tooie_wrapper_scene_admits(const struct TooieSceneSample* s) {
    u32 flags = s->tuple.rawFlags;
    return (s->level < 0x0Fu || (s->level < 0x1Cu && s->level != 0x11u)) &&
        (s->saveSlot >= 0 || (s->frontendMode == 2u && s->tuple.replayRate != 0)) &&
        s->gameType != 3u && (flags & TOOIE_SCENE_MAP) &&
        !(flags & (TOOIE_SCENE_ACTIVATION | TOOIE_SCENE_INTRO)) &&
        !((flags & TOOIE_SCENE_CUTSCENE) && (flags & TOOIE_SCENE_ORIGINAL_MOTION));
}

static inline __attribute__((always_inline)) int tooie_wrapper_scene_equal(const struct TooieSceneSample* a,
    const struct TooieSceneSample* b) {
    return a->level == b->level && a->saveSlot == b->saveSlot &&
        a->gameType == b->gameType && a->frontendMode == b->frontendMode &&
        a->tuple.taskLow == b->tuple.taskLow && a->tuple.taskHigh == b->tuple.taskHigh &&
        a->tuple.epochLow == b->tuple.epochLow && a->tuple.epochHigh == b->tuple.epochHigh &&
        a->tuple.rawFlags == b->tuple.rawFlags && a->tuple.replayRate == b->tuple.replayRate &&
        a->tuple.revisionLow == b->tuple.revisionLow &&
        a->tuple.revisionHigh == b->tuple.revisionHigh;
}

/* Validate actual allocator metadata before any new owner/model dereference.
 * Original entry3 still has its original caller contract when labels refuse. */
static inline __attribute__((always_inline)) u32 tooie_wrapper_allocation_end(u32 address) {
    u32 start = (u32)D_8007E990, end, header, next, previous;
    struct HeapHeader* h;
    if (D_8007BF00 <= 0 || start < 0x80000000u ||
        start >= 0x81000000u || (u32)D_8007BF00 > 0x81000000u - start) return 0;
    end = start + (u32)D_8007BF00;
    if ((address & 15u) || address < start + 48u || address > end - 32u) return 0;
    header = address - 16u;
    h = (struct HeapHeader*)header;
    next = (u32)h->next;
    previous = (u32)h->prev;
    if (h->state != HEAP_BLOCK_STATE_USED || next < address || next > end - 32u ||
        (next & 15u) || previous < start || previous >= header || (previous & 15u) ||
        (u32)((struct HeapHeader*)previous)->next != header ||
        (u32)((struct HeapHeader*)next)->prev != header ||
        h->allocation_overhead > next - address) return 0;
    return next - h->allocation_overhead;
}

/* Counting uses publication alone, before label-only allocation/release guards.
 * Entry3 retains its original owner/backpack caller contract. */
static inline __attribute__((always_inline)) s32 tooie_wrapper_published_slot(PlayerState* self) {
    u32 i, found = 8;
    for (i = 0; i < 8; ++i) if (D_80135490[i] == self) {
        if (found != 8) return -1;
        found = i;
    }
    return found == 8 ? -1 : (s32)found;
}

static inline __attribute__((always_inline)) s32 tooie_wrapper_owner_slot(PlayerState* self) {
    s32 found = tooie_wrapper_published_slot(self);
    u32 address = (u32)self, end;
    if (found < 0 || (tooie_wrapper_state.releaseMask & (1u << found))) return -1;
    end = tooie_wrapper_allocation_end(address);
    if (!end || end - address < 0x188u || self->unk184 != (s32)found) return -1;
    if ((u32)self->backpack < address || ((u32)self->backpack & 3u) ||
        (u32)self->backpack > end || end - (u32)self->backpack < sizeof(BaBackpack)) return -1;
    return (s32)found;
}

static inline __attribute__((always_inline)) int tooie_wrapper_pool_interval(TooieGraphicsBuffers* b, u32* baseOut, u32* endOut) {
    s32 index = D_8007BEDA;
    u32 base, end, current;
    if (index < 0 || index > 1 || D_8007BCE8[0] || D_8007BCE8[1] ||
        D_8007BCF0[0] || D_8007BCF0[1]) return 0;
    base = (u32)tooie_gfx_pools[index];
    end = base + TOOIE_GFX_POOL_BYTES;
    current = (u32)b->gfx;
    if (end < base || D_8007BCD0[index] != base || current < base + 8u ||
        current > end || ((current - base) & 7u) ||
        tooie_gfx_pools[index][0].values.word0 != TOOIE_POLICY_WORD0 ||
        tooie_gfx_pools[index][0].values.word1 != (TOOIE_POLICY_MAGIC | TOOIE_POLICY_MODEL_LABELS)) return 0;
    *baseOut = base;
    *endOut = end;
    return 1;
}

static inline __attribute__((always_inline)) int tooie_wrapper_pool(TooieGraphicsBuffers* b, u32* baseOut, u32* endOut) {
    s32 index = D_8007BEDA;
    u32 base, end, current = (u32)b->gfx, mtxBase, mtxEnd, mtx, offset, remaining;
    if (!tooie_wrapper_pool_interval(b, &base, &end)) return 0;
    offset = current - base;
    remaining = end - current;
    if (remaining < 1024u || remaining < TOOIE_GFX_ORIGINAL_BYTES + 32u + 8u*(offset/24u)) return 0;
    mtxBase = D_8007BCD8[index];
    mtxEnd = tooie_wrapper_allocation_end(mtxBase);
    mtx = (u32)b->mtx;
    if (!mtxBase || !mtxEnd || mtxBase > mtxEnd || mtxEnd-mtxBase < 0x12C00u ||
        mtx < mtxBase || mtx-mtxBase > 0x12C00u || ((mtx-mtxBase)&63u) ||
        0x12C00u - (mtx-mtxBase) < 0x580u) return 0;
    *baseOut = base;
    *endOut = end;
    return 1;
}

static inline __attribute__((always_inline)) void tooie_wrapper_group(GfxCommand* group, u32 id) {
    u32 component = id == G_EX_ID_IGNORE ? G_EX_COMPONENT_SKIP : G_EX_COMPONENT_INTERPOLATE;
    gEXMatrixGroup(group, id, G_EX_INTERPOLATE_SIMPLE, G_EX_PUSH, 0,
        component, component, component, component, component,
        G_EX_COMPONENT_SKIP, id == G_EX_ID_IGNORE ? G_EX_COMPONENT_SKIP : G_EX_COMPONENT_AUTO,
        G_EX_ORDER_LINEAR, G_EX_EDIT_NONE, G_EX_ASPECT_AUTO,
        G_EX_COMPONENT_SKIP, id == G_EX_ID_IGNORE ? G_EX_COMPONENT_SKIP : G_EX_COMPONENT_AUTO);
}

/* The selected original closure and synchronous imports cannot submit or yield.
 * This authority is local to its current occurrence, including tracker loss.
 * An available tuple adds task-drift refusal; it does not confer ownership. */
static inline __attribute__((always_inline)) int tooie_wrapper_unsubmitted(u32 worker,
    u32 sequence, u32 owner, u32 model) {
    struct TooieSubmittedRenderTuple t;
    u32 low, high;
    if (!worker || worker != (u32)osGetThreadId(NULL) || !sequence ||
        tooie_wrapper_state.constructionWorker != worker ||
        tooie_wrapper_state.constructionSequence != sequence ||
        tooie_wrapper_state.boundSequence != sequence ||
        tooie_wrapper_state.contextWorker != worker ||
        tooie_wrapper_state.contextOwner != owner ||
        tooie_wrapper_state.contextModel != model ||
        tooie_wrapper_state.contextDepth != 1) return 0;
    if (!tooie_get_submitted_render_tuple(&t)) return 1;
    if (t.taskLow == 0xFFFFFFFFu && t.taskHigh == 0xFFFFFFFFu) return 0;
    low = t.taskLow+1u;
    high = t.taskHigh+(low == 0u);
    return low == tooie_wrapper_state.taskLow && high == tooie_wrapper_state.taskHigh;
}

static inline __attribute__((always_inline)) int tooie_wrapper_quarantine(u32 address, u32 id,
    TooieGraphicsBuffers* b, u32 worker, int endpoint, u32 sequence, u32 owner, u32 model) {
    u32 base, end;
    GfxCommand expected[2];
    GfxCommand* own = (GfxCommand*)address;
    if (!address || !(endpoint ? tooie_wrapper_unsubmitted(worker, sequence, owner, model) :
            tooie_wrapper_construction_matches(worker)) ||
        !tooie_wrapper_pool_interval(b, &base, &end) || address < base+16u || address > end-16u ||
        (address & 7u) || address+16u > (u32)b->gfx) return 0;
    tooie_wrapper_group(expected, id);
    if (own[0].values.word0 != expected[0].values.word0 ||
        own[0].values.word1 != expected[0].values.word1 ||
        own[1].values.word0 != expected[1].values.word0 ||
        own[1].values.word1 != expected[1].values.word1) return 0;
    tooie_wrapper_group(own, G_EX_ID_IGNORE);
    return 1;
}

static inline __attribute__((always_inline)) void tooie_wrapper_duplicate(u32 slot, TooieGraphicsBuffers* b, u32 worker) {
    tooie_wrapper_state.currentMask =
        (tooie_wrapper_state.currentMask & ~(1u << slot)) | (1u << (slot+8u));
    if (tooie_wrapper_quarantine(tooie_wrapper_state.history[slot].ownGroupAddress,
        tooie_wrapper_state.history[slot].poseID, b, worker, 0, 0, 0, 0))
        tooie_wrapper_state.history[slot].ownGroupAddress = 0;
}

static inline __attribute__((always_inline)) int tooie_wrapper_inputs(PlayerState* self, u32 model, TooieGraphicsBuffers* b) {
    const u8* p = (const u8*)model;
    u32 end = tooie_wrapper_allocation_end(model), cursor, base, poolEnd, expectedSegment;
    u32 pose, poseEnd, required;
    s32 count;
    BaBackpack* pack = self->backpack;
    u32 cache = (u32)pack->unk4, cacheEnd = tooie_wrapper_allocation_end(cache);
    if (!end || end-model != 8096u || *(const u32*)(p+4) != 7384u ||
        *(const u32*)(p+7384u) != 2u ||
        *(const s16*)(p+0xA) != 0 || *(const u32*)(p+0xC) != 1400u ||
        *(const u32*)(p+0x18) != 3608u || *(const u32*)(p+0x20) != 0 ||
        *(const u32*)(p+0x28) != 0 || *(const s16*)(p+3608u+4) != 14 ||
        pack->state != 2 || pack->unk0 != 0x621 || pack->unkA != 0 ||
        !pack->unk8 || !pack->unk4 || D_8012C844 != (u32)pack->unk4 ||
        !cacheEnd || cache > cacheEnd || cacheEnd-cache < 0x118u ||
        D_8012CF8C != D_80123198[0] || D_8012C838 || D_8012C83C ||
        D_8012CEC4 || D_8012C9A8 || D_8012C9B0 || D_8012C848 || D_8012CA28 != 1 ||
        D_8012C948[5] || D_8012C948[6] || D_8012C948[8] ||
        D_8012C948[10] || D_8012C948[12] || D_8012C948[13] || D_8012C948[14] ||
        func_800EB854() != 0 || !tooie_wrapper_pool(b, &base, &poolEnd)) return 0;
    /* Late read after entry3's final setters. The same validated live skeleton
     * count governs the original builders; no allocation or pose precomputation. */
    count = (s32)*(const s16*)(p + *(const u32*)(p+0x18) + 4u);
    if (count <= 0) return 0;
    required = 0x50u + (u32)count * 64u;
    pose = D_8012C840;
    poseEnd = tooie_wrapper_allocation_end(pose);
    if (!poseEnd || pose > poseEnd || poseEnd-pose < required ||
        *(const s32*)(pose+4u) < count ||
        (D_8012C824 != 0 && *(const u32*)(pose+8u) != 0)) return 0;
    /* Observe the most recent source segment7 setter, preserving returning DLs. */
    expectedSegment = ((u32)(D_8012D5E0 == G_CULL_BACK ? D_80123418 : D_80123428)) & 0x1FFFFFFFu;
    cursor = (u32)b->gfx;
    while (cursor >= base+16u) {
        const GfxCommand* cmd = (const GfxCommand*)(cursor-8u);
        if (cmd->values.word0 == 0xDB06001Cu) return cmd->values.word1 == expectedSegment;
        cursor -= 8u;
    }
    return 0;
}

TOOIE_RECOMP_PATCH void* func_800DE448(f32* position, f32* rotation, f32 scale,
    f32* transform, s32 model) {
    TooieGraphicsBuffers* b = func_800A7180();
    PlayerState* self = (PlayerState*)tooie_wrapper_state.contextOwner;
    u32 worker = (u32)osGetThreadId(NULL), base, end, id = 0, slot = 8, groupAddress = 0;
    u32 sequence = 0;
    struct TooieSceneSample before, after;
    int sceneStable = 0;
    void* result;
    s32 selectedSlot;
    if (tooie_wrapper_state.contextDepth == 1 && tooie_wrapper_state.contextWorker == worker &&
        tooie_wrapper_state.contextModel == (u32)model && tooie_wrapper_construction_matches(worker)) {
        selectedSlot = tooie_wrapper_published_slot(self);
        /* Native capture keys do not include pack state or selector. Count all
         * qualified published entry3 occurrences; inputs alone narrow labels. */
        if (selectedSlot >= 0) {
            volatile struct TooieWrapperOwner* owner;
            slot = (u32)selectedSlot;
            owner = &tooie_wrapper_state.owners[slot];
            /* Raw live-owner provenance is not a public birth. Stamp once even
             * when later label guards refuse; the same unborn owner keeps poison. */
            if (owner->owner != (u32)self && tooie_wrapper_owner_slot(self) == selectedSlot) {
                tooie_wrapper_clear_slots(1u << slot);
                tooie_wrapper_state.seenMask &= ~(1u << slot);
                owner->owner = (u32)self;
                owner->birth = owner->model = owner->role = 0;
                tooie_wrapper_state.history[slot].ownGroupAddress = 0;
            }
            /* Duplicate counting is coarser than the labelled model/role key. */
            if (owner->owner != (u32)self || owner->model != (u32)model ||
                owner->role != TOOIE_WRAPPER_ROLE)
                tooie_wrapper_clear_eligibility(1u << slot);
            if (tooie_wrapper_state.seenMask & (1u << slot)) {
                tooie_wrapper_duplicate(slot, b, worker);
            } else {
                /* This boundary is marked even when a later guard refuses. */
                tooie_wrapper_state.seenMask |= 1u << slot;
                tooie_wrapper_state.currentMask &= ~(1u << slot);
                if (!(tooie_wrapper_state.previousMask & (1u << (slot+8u))) &&
                    tooie_wrapper_owner_slot(self) == selectedSlot &&
                    D_80127634 == 0 && position && tooie_wrapper_state.contextView < 4 &&
                    func_800A89F8() == (s32)tooie_wrapper_state.contextView &&
                    tooie_wrapper_inputs(self, (u32)model, b) &&
                    tooie_wrapper_scene_read(&before) && tooie_wrapper_scene_admits(&before) &&
                    tooie_wrapper_state.constructionSequence != 0 &&
                    tooie_wrapper_label_can_begin())
                    id = tooie_wrapper_pose(slot, (u32)self, (u32)model, position);
            }
        }
    }
    if (id && tooie_wrapper_pool(b, &base, &end) &&
        tooie_wrapper_state.constructionSequence != 0 &&
        tooie_wrapper_state.boundSequence == tooie_wrapper_state.constructionSequence &&
        tooie_wrapper_label_begin()) {
        GfxCommand* cursor = (GfxCommand*)b->gfx;
        /* FFFFFFFF is the last legal construction; the next begin refuses. */
        sequence = tooie_wrapper_state.constructionSequence;
        groupAddress = (u32)(cursor+1);
        tooie_wrapper_source_span_begin((u32)self, b,
            tooie_wrapper_state.contextView, groupAddress);
        gEXEnable(cursor);
        tooie_wrapper_group(cursor+1, id);
        b->gfx = (Gfx*)(cursor+3);
    }
    result = func_800DE498(b, position, rotation, scale, transform, model);
    if (groupAddress) {
        GfxCommand* cursor = (GfxCommand*)b->gfx;
        /* The original selected closure has no submission. Endpoint drift is
         * handled before native assignment/filtering, using this occurrence's
         * exact owned address rather than an uncommitted history address. */
        sceneStable = tooie_wrapper_scene_read(&after) && tooie_wrapper_scene_equal(&before, &after);
        if (!sceneStable) {
            tooie_wrapper_quarantine(groupAddress, id, b, worker, 1,
                sequence, (u32)self, (u32)model);
            tooie_wrapper_clear_eligibility(1u << slot);
            /* After local quarantine, apply ordinary global epoch/task/query-
             * loss invalidation. Successful task drift refuses the rewrite;
             * unavailable tracking uses only this selected no-submit closure. */
            tooie_wrapper_construction_matches(worker);
        }
        tooie_wrapper_source_span_end(b);
        gEXPopMatrixGroup(cursor, 0);
        b->gfx = (Gfx*)(cursor+1);
        if (tooie_wrapper_label_end() && result && sceneStable && tooie_wrapper_construction_matches(worker)) {
            volatile struct TooieWrapperHistory* h = &tooie_wrapper_state.history[slot];
            h->xyz[0] = position[0]; h->xyz[1] = position[1]; h->xyz[2] = position[2];
            h->poseID = id;
            h->ownGroupAddress = groupAddress;
            tooie_wrapper_state.currentMask |= 1u << slot;
        }
    }
    return result;
}

TOOIE_RECOMP_PATCH void babackpack_entrypoint_3(PlayerState* self, f32* position,
    f32* rotation, s32 arg3, f32* transform) {
    s32 model;
    u32 worker, entered = 0, nested = 0;
    func_800DF738(self->backpack->unk4);
    func_800DF818(func_8080021C_babackpack(self));
    func_800DF720(func_80100AC4(self->backpack->unk8));
    if (self->backpack->state == 7) {
        func_8008B10C(self->backpack->anctrl, baanim_getTimer(self));
        func_8008B304(self->backpack->anctrl);
        func_800DF41C(func_8008B4D4(self->backpack->anctrl));
        func_800A0714(self, self->backpack->unk0);
    } else func_800DF41C(func_8008CA74(self));
    model = babackpack_entrypoint_1(self);
    worker = (u32)osGetThreadId(NULL);
    if (tooie_wrapper_state.contextDepth) {
        if (tooie_wrapper_state.contextDepth != 0xFFFFFFFFu) {
            ++tooie_wrapper_state.contextDepth; nested = 1;
        }
    } else if (tooie_wrapper_construction_matches(worker)) {
        tooie_wrapper_state.contextOwner = (u32)self;
        tooie_wrapper_state.contextModel = (u32)model;
        tooie_wrapper_state.contextView = (u32)func_800A89F8();
        tooie_wrapper_state.contextWorker = worker;
        tooie_wrapper_state.contextDepth = 1;
        entered = 1;
    }
    tooie_wrapper_attachment_begin(self);
    func_800DE448(position, rotation, self->backpack->scale, transform, model);
    if (entered) {
        tooie_wrapper_state.contextDepth = 0;
        tooie_wrapper_state.contextOwner = 0;
        tooie_wrapper_state.contextModel = 0;
        tooie_wrapper_state.contextView = 0;
        tooie_wrapper_state.contextWorker = 0;
    } else if (nested) --tooie_wrapper_state.contextDepth;
    tooie_wrapper_attachment_end(self);
    if (self->backpack->unkA != 0) self->backpack->unk9 ^= 1;
}

/* The guard only covers F3880 itself. The post-free A91F4/slot-clear interval
 * requires qualified ordinary closure or exact caller completion seams. */
TOOIE_RECOMP_PATCH void func_800F3880(s32 argument) {
    PlayerState* self = (PlayerState*)(u32)argument;
    u32 i, mask = 0, previousOwnBits;
    for (i = 0; i < 8; ++i) if (D_80135490[i] == self) mask |= 1u << i;
    previousOwnBits = tooie_wrapper_state.releaseMask & mask;
    tooie_wrapper_state.releaseMask |= mask;
    func_80098E64(self);
    tooie_wrapper_retire((u32)self);
    /* Publication also retires unlabelled conservative duplicate markers. */
    tooie_wrapper_clear_slots(mask);
    heap_free(self);
    tooie_wrapper_state.releaseMask &= ~(mask & ~previousOwnBits);
}
