/* Guest-owned sequenced-music requests; original queue delivery may refuse.
 * Reconstructs 80025ED0 and 800123BC; adds gain at their guest-owned request boundary.
 */
#include "common.h"

#define MUSIC_PATCH __attribute__((section(".recomp_patch"), noinline))
#define MUSIC_INLINE static inline __attribute__((always_inline))

enum {
    MUSIC_PLAYER_COUNT = 6,
    MUSIC_RECORD_BYTES = 0x274,
    MUSIC_PLAYER_OFFSET = 0x1E0,
    MUSIC_QUEUE_OFFSET = 0x48
};

extern u8 D_80079FA0[];
extern s32 tooie_baseline_music_volume(void);
extern void func_80026500(void* queue, void* event, s32 delay, s32 flags);
extern s32 func_800A7D84(void);
extern void func_800A7840(s32 mode);
extern void func_800A5D1C(void);

struct TooieMusicState {
    s16 last_req[MUSIC_PLAYER_COUNT];
    u32 valid;
    u32 last_percent;
};

__attribute__((section(".tooie_music_state"), aligned(4)))
struct TooieMusicState tooie_music_state = {{0}, 0, 100};

/* Original event: type at0, signed payload at4, sixteen copied bytes.
 * Original stack slot is eight-byte aligned; padding has no defined value.
 */
struct __attribute__((aligned(8))) MusicVolumeEvent {
    s16 type;
    u16 padding0;
    s16 volume;
    u16 padding1[5];
};

_Static_assert(sizeof(void*) == 4 && sizeof(s16) == 2 && sizeof(s32) == 4,
               "original guest o32 scalar ABI");
_Static_assert(sizeof(struct TooieMusicState) == 20 &&
               __builtin_offsetof(struct TooieMusicState, valid) == 12 &&
               __builtin_offsetof(struct TooieMusicState, last_percent) == 16,
               "guest-owned music state extent and offsets");
_Static_assert(sizeof(struct MusicVolumeEvent) == 16 &&
               _Alignof(struct MusicVolumeEvent) == 8 &&
               __builtin_offsetof(struct MusicVolumeEvent, volume) == 4,
               "original volume event extent, alignment and payload");

MUSIC_INLINE s32 music_percent(void) {
    const s32 value = tooie_baseline_music_volume();
    return value < 0 ? 0 : value > 100 ? 100 : value;
}

/* Compare guest addresses without claiming an ELF-sized array object. */
MUSIC_INLINE s32 music_slot(void* player) {
    const u32 first = (u32)D_80079FA0 + MUSIC_PLAYER_OFFSET;
    for (s32 i = 0; i < MUSIC_PLAYER_COUNT; ++i) {
        if ((u32)player == first + (u32)i * MUSIC_RECORD_BYTES) return i;
    }
    return -1;
}

/* percent is already clamped. The product is within signed32 range. */
MUSIC_INLINE s16 music_scale(s16 volume, s32 percent) {
    return (s16)(((s32)volume * percent) / 100);
}

MUSIC_INLINE void music_post(void* player, s16 volume) {
    struct MusicVolumeEvent event = {0};
    event.type = 10;
    event.volume = volume;
    func_80026500((void*)((u32)player + MUSIC_QUEUE_OFFSET), &event, 0, 0);
}

/* Original library operation, with an explicit low16 truncation of its s32 input.
 * Valid records a request even if the original queue refuses that request.
 */
MUSIC_PATCH void func_80025ED0(void* player, s32 volume) {
    const s16 request = (s16)volume;
    /* @recomp Capture each exact music-player request and post it scaled;
     * any other player keeps the original unscaled post. */
    const s32 slot = music_slot(player);
    if (slot < 0) {
        music_post(player, request);
        return;
    }

    const s32 percent = music_percent();
    tooie_music_state.last_req[slot] = request;
    tooie_music_state.valid |= 1u << (u32)slot;
    music_post(player, music_scale(request, percent));
}

/* Original caller owns the mode1 guard. No original pending-volume cache is read.
 * Republish uses the private post, never the replaced operation: no double scale.
 * Original selected caller consumes no return value from this boundary.
 */
MUSIC_PATCH void func_800123BC(void) {
    /* @recomp Republish captured requests when the percent changes; the original
     * update calls below are unchanged and still follow. */
    const s32 percent = music_percent();
    if ((u32)percent != tooie_music_state.last_percent) {
        const u32 valid = tooie_music_state.valid;
        const u32 first = (u32)D_80079FA0 + MUSIC_PLAYER_OFFSET;
        for (u32 i = 0; i < MUSIC_PLAYER_COUNT; ++i) {
            if (valid & (1u << i)) {
                music_post((void*)(first + i * MUSIC_RECORD_BYTES),
                           music_scale(tooie_music_state.last_req[i], percent));
            }
        }
        tooie_music_state.last_percent = (u32)percent;
    }

    if (func_800A7D84()) {
        func_800A7840(0);
    }
    func_800A5D1C();
}
