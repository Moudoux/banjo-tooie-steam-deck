#ifndef TOOIE_WRAPPER_STATE_H
#define TOOIE_WRAPPER_STATE_H

/* Fixed guest ABI. This is the NEW 364-byte suffix; the existing family
 * provider's first 804 bytes retain their original layout and contents. */
#define TOOIE_WRAPPER_SLOTS 8u
#define TOOIE_WRAPPER_ROLE 1u
#define TOOIE_WRAPPER_POSE_BASE 0x80000000u
#define TOOIE_WRAPPER_MAX_POSE 0x03FFFFFFu
/* Low8 eligibility, upper8 duplicate fallback; seenMask is independent. */
_Static_assert(TOOIE_WRAPPER_SLOTS <= 8u, "one duplicate bit per owner slot");

struct TooieSubmittedRenderTuple {
    unsigned int taskLow, taskHigh, epochLow, epochHigh;
};
struct TooieWrapperOwner {
    unsigned int owner, birth, model, role;
};
struct TooieWrapperHistory {
    float xyz[3];
    unsigned int poseID, ownGroupAddress;
};
struct TooieWrapperState {
    struct TooieWrapperOwner owners[8];
    struct TooieWrapperHistory history[8];
    unsigned int birthCursor, poseCursor;
    unsigned int taskLow, taskHigh, epochLow, epochHigh;
    unsigned int previousMask, currentMask, seenMask;
    unsigned int constructionSequence, constructionWorker, pendingLabels, boundSequence;
    unsigned int contextOwner, contextModel, contextView, contextDepth, contextWorker;
    unsigned int releaseMask;
};
_Static_assert(sizeof(unsigned int) == 4 && sizeof(float) == 4, "guest scalar ABI");
_Static_assert(sizeof(struct TooieSubmittedRenderTuple) == 16, "tuple transport ABI");
_Static_assert(sizeof(struct TooieWrapperOwner) == 16, "owner ABI");
_Static_assert(sizeof(struct TooieWrapperHistory) == 20, "history ABI");
_Static_assert(sizeof(struct TooieWrapperState) == 364, "new render extent");
_Static_assert(__builtin_offsetof(struct TooieWrapperState, history) == 128 &&
    __builtin_offsetof(struct TooieWrapperState, birthCursor) == 288 &&
    __builtin_offsetof(struct TooieWrapperState, contextOwner) == 340 &&
    __builtin_offsetof(struct TooieWrapperState, releaseMask) == 360, "render offsets");

/* The integrator binds this symbol to family object +804, yielding 1168 bytes. */
extern volatile struct TooieWrapperState tooie_wrapper_state;
extern int tooie_get_submitted_render_tuple(struct TooieSubmittedRenderTuple*);

#endif
