#ifndef TOOIE_WRAPPER_POLICY_H
#define TOOIE_WRAPPER_POLICY_H

#include "../wrapper_state.h"

#define TOOIE_WRAPPER_UNPUBLISHED 0x80000000u
#define TOOIE_WRAPPER_LABEL_DEPTH 0x7FFFFFFFu

static inline __attribute__((always_inline)) void tooie_wrapper_clear_eligibility(unsigned int slots) {
    tooie_wrapper_state.previousMask &= ~slots;
    tooie_wrapper_state.currentMask &= ~slots;
}

static inline __attribute__((always_inline)) void tooie_wrapper_clear_slots(unsigned int slots) {
    tooie_wrapper_clear_eligibility(slots | (slots << 8));
}

static inline __attribute__((always_inline)) void tooie_wrapper_clear_history(void) {
    unsigned int i;
    tooie_wrapper_state.previousMask = 0;
    tooie_wrapper_state.currentMask = 0;
    tooie_wrapper_state.seenMask = 0;
    for (i = 0; i < 8; ++i) tooie_wrapper_state.history[i].ownGroupAddress = 0;
}

/* The high bit outlives POP: a completed root still awaits exact publication.
 * Neither history invalidation nor a fresh pool selection abandons that root. */
static inline __attribute__((always_inline)) int tooie_wrapper_label_can_begin(void) {
    return (tooie_wrapper_state.pendingLabels & TOOIE_WRAPPER_LABEL_DEPTH) !=
        TOOIE_WRAPPER_LABEL_DEPTH;
}

static inline __attribute__((always_inline)) int tooie_wrapper_label_begin(void) {
    unsigned int depth = tooie_wrapper_state.pendingLabels & TOOIE_WRAPPER_LABEL_DEPTH;
    if (depth == TOOIE_WRAPPER_LABEL_DEPTH) return 0;
    tooie_wrapper_state.pendingLabels = TOOIE_WRAPPER_UNPUBLISHED | (depth + 1u);
    return 1;
}

static inline __attribute__((always_inline)) int tooie_wrapper_label_end(void) {
    unsigned int depth = tooie_wrapper_state.pendingLabels & TOOIE_WRAPPER_LABEL_DEPTH;
    if (!depth) {
        tooie_wrapper_state.pendingLabels |= TOOIE_WRAPPER_UNPUBLISHED;
        tooie_wrapper_clear_history();
        tooie_wrapper_state.constructionWorker = 0;
        return 0;
    }
    tooie_wrapper_state.pendingLabels = TOOIE_WRAPPER_UNPUBLISHED | (depth - 1u);
    return 1;
}

/* Only A7840's saved original 1A270 return may discharge this construction.
 * Source sequence is independent of asynchronous native task binding. */
static inline __attribute__((always_inline)) void tooie_wrapper_root_published(
    unsigned int worker, unsigned int sequence) {
    if (worker && sequence && tooie_wrapper_state.constructionWorker == worker &&
        tooie_wrapper_state.constructionSequence == sequence &&
        tooie_wrapper_state.boundSequence == sequence &&
        !tooie_wrapper_state.contextDepth &&
        !(tooie_wrapper_state.pendingLabels & TOOIE_WRAPPER_LABEL_DEPTH))
        tooie_wrapper_state.pendingLabels &= ~TOOIE_WRAPPER_UNPUBLISHED;
}

/* Source A7840 calls this AFTER final pool selection, before constructing
 * the root. The query reads N; this construction binds N+1, no reservation. */
static inline __attribute__((always_inline)) void tooie_wrapper_construction_begin(unsigned int worker) {
    struct TooieSubmittedRenderTuple t;
    unsigned int nextLow, nextHigh, i;
    int adjacent;
    if (!worker || tooie_wrapper_state.constructionWorker ||
        tooie_wrapper_state.contextDepth || tooie_wrapper_state.pendingLabels ||
        tooie_wrapper_state.constructionSequence == 0xFFFFFFFFu ||
        !tooie_get_submitted_render_tuple(&t) ||
        (t.taskLow == 0xFFFFFFFFu && t.taskHigh == 0xFFFFFFFFu)) {
        tooie_wrapper_clear_history();
        tooie_wrapper_state.constructionWorker = 0;
        return;
    }
    nextLow = t.taskLow + 1u;
    nextHigh = t.taskHigh + (nextLow == 0u);
    adjacent = tooie_wrapper_state.boundSequence != 0 &&
        tooie_wrapper_state.taskLow == t.taskLow &&
        tooie_wrapper_state.taskHigh == t.taskHigh &&
        tooie_wrapper_state.epochLow == t.epochLow &&
        tooie_wrapper_state.epochHigh == t.epochHigh;
    tooie_wrapper_state.previousMask = adjacent ? tooie_wrapper_state.currentMask : 0;
    tooie_wrapper_state.currentMask = 0;
    tooie_wrapper_state.seenMask = 0;
    for (i = 0; i < 8; ++i) tooie_wrapper_state.history[i].ownGroupAddress = 0;
    tooie_wrapper_state.taskLow = nextLow;
    tooie_wrapper_state.taskHigh = nextHigh;
    tooie_wrapper_state.epochLow = t.epochLow;
    tooie_wrapper_state.epochHigh = t.epochHigh;
    ++tooie_wrapper_state.constructionSequence;
    tooie_wrapper_state.boundSequence = tooie_wrapper_state.constructionSequence;
    tooie_wrapper_state.constructionWorker = worker;
}

static inline __attribute__((always_inline)) int tooie_wrapper_construction_matches(unsigned int worker) {
    struct TooieSubmittedRenderTuple t;
    unsigned int low, high;
    if (!worker || tooie_wrapper_state.constructionWorker != worker) return 0;
    if (tooie_wrapper_state.boundSequence != tooie_wrapper_state.constructionSequence ||
        !tooie_get_submitted_render_tuple(&t) ||
        (t.taskLow == 0xFFFFFFFFu && t.taskHigh == 0xFFFFFFFFu)) {
        tooie_wrapper_clear_history();
        tooie_wrapper_state.constructionWorker = 0;
        return 0;
    }
    low = t.taskLow + 1u;
    high = t.taskHigh + (low == 0u);
    if (low != tooie_wrapper_state.taskLow || high != tooie_wrapper_state.taskHigh ||
        t.epochLow != tooie_wrapper_state.epochLow || t.epochHigh != tooie_wrapper_state.epochHigh) {
        tooie_wrapper_clear_history();
        tooie_wrapper_state.constructionWorker = 0;
        return 0;
    }
    return 1;
}

/* Called only after A7840 original submission and tail have completed. */
static inline __attribute__((always_inline)) void tooie_wrapper_construction_end(unsigned int worker) {
    if (tooie_wrapper_state.constructionWorker != worker ||
        tooie_wrapper_state.contextDepth || tooie_wrapper_state.pendingLabels)
        tooie_wrapper_clear_history();
    tooie_wrapper_state.constructionWorker = 0;
}

static inline __attribute__((always_inline)) int tooie_wrapper_finite(float value) {
    union { float f; unsigned int word; } v;
    v.f = value;
    return (v.word & 0x7F800000u) != 0x7F800000u;
}

static inline __attribute__((always_inline)) int tooie_wrapper_bind_owner(unsigned int slot, unsigned int owner,
    unsigned int model) {
    volatile struct TooieWrapperOwner* r = &tooie_wrapper_state.owners[slot];
    unsigned int birth;
    if (r->owner != owner || !r->birth) {
        birth = tooie_wrapper_state.birthCursor;
        if (!birth || birth == 0xFFFFFFFFu) return 0;
        tooie_wrapper_state.birthCursor = birth + 1u;
        r->owner = owner;
        r->birth = birth;
        tooie_wrapper_clear_eligibility(1u << slot);
    }
    if (r->model != model || r->role != TOOIE_WRAPPER_ROLE) {
        tooie_wrapper_clear_eligibility(1u << slot);
    }
    r->model = model;
    r->role = TOOIE_WRAPPER_ROLE;
    return 1;
}

/* Inputs and capacity have already passed. Decide the label before committing
 * first birth, so nonfinite/exhausted refusals do not create a labelled owner. */
static inline __attribute__((always_inline)) unsigned int tooie_wrapper_pose(unsigned int slot,
    unsigned int owner, unsigned int model, const float* xyz) {
    volatile struct TooieWrapperOwner* r = &tooie_wrapper_state.owners[slot];
    volatile struct TooieWrapperHistory* h = &tooie_wrapper_state.history[slot];
    double dx, dy, dz;
    unsigned int id = 0, serial = 0;
    if (!tooie_wrapper_finite(xyz[0]) || !tooie_wrapper_finite(xyz[1]) ||
        !tooie_wrapper_finite(xyz[2])) return 0;
    if (r->owner == owner && r->birth && r->model == model &&
        r->role == TOOIE_WRAPPER_ROLE && (tooie_wrapper_state.previousMask & (1u << slot))) {
        dx = (double)xyz[0] - (double)h->xyz[0];
        dy = (double)xyz[1] - (double)h->xyz[1];
        dz = (double)xyz[2] - (double)h->xyz[2];
        if (tooie_wrapper_finite(h->xyz[0]) && tooie_wrapper_finite(h->xyz[1]) &&
            tooie_wrapper_finite(h->xyz[2]) && dx*dx + dy*dy + dz*dz <= 65536.0)
            id = h->poseID;
    }
    if (!id) {
        serial = tooie_wrapper_state.poseCursor;
        if (!serial || serial > TOOIE_WRAPPER_MAX_POSE) return 0;
        id = TOOIE_WRAPPER_POSE_BASE | (serial << 2);
    }
    if (!tooie_wrapper_bind_owner(slot, owner, model)) return 0;
    if (serial) tooie_wrapper_state.poseCursor = serial + 1u;
    return id;
}

static inline __attribute__((always_inline)) void tooie_wrapper_retire(unsigned int owner) {
    unsigned int i;
    for (i = 0; i < 8; ++i) if (tooie_wrapper_state.owners[i].owner == owner) {
        tooie_wrapper_state.owners[i].owner = 0;
        tooie_wrapper_state.owners[i].birth = 0;
        tooie_wrapper_state.owners[i].model = 0;
        tooie_wrapper_state.owners[i].role = 0;
        tooie_wrapper_state.history[i].ownGroupAddress = 0;
        tooie_wrapper_clear_slots(1u << i);
    }
}

#endif
