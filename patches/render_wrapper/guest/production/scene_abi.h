#ifndef TOOIE_WRAPPER_SCENE_ABI_H
#define TOOIE_WRAPPER_SCENE_ABI_H

/* Distinct mechanical transport; construction tuple16 remains unchanged.
 * Eight guest words, four-byte alignment. Failure leaves output untouched. */
struct TooieSceneTuple32 {
    unsigned int taskLow, taskHigh, epochLow, epochHigh;
    unsigned int rawFlags, replayRate, revisionLow, revisionHigh;
};
_Static_assert(sizeof(struct TooieSceneTuple32) == 32 &&
    _Alignof(struct TooieSceneTuple32) == 4, "scene32 guest transport ABI");
_Static_assert(__builtin_offsetof(struct TooieSceneTuple32, rawFlags) == 16 &&
    __builtin_offsetof(struct TooieSceneTuple32, revisionLow) == 24,
    "scene32 guest transport offsets");
extern int tooie_get_scene_tuple32(struct TooieSceneTuple32*);

#define TOOIE_SCENE_MAP 1u
#define TOOIE_SCENE_ACTIVATION 2u
#define TOOIE_SCENE_CUTSCENE 4u
#define TOOIE_SCENE_ORIGINAL_MOTION 8u
#define TOOIE_SCENE_INTRO 16u
#define TOOIE_SCENE_FLAGS 31u

#endif
