/* Guest-owned Talon Trot toggle; original hold remains the default. */
#include "common.h"
#include "overlays/bs/state.h"
#include "overlays/ba/input.h"
#define PATCH __attribute__((section(".recomp_patch"), noinline))
#define HELPER __attribute__((section(".tooie_trot_helpers"), noinline))
#define INLINE static __attribute__((always_inline)) inline
extern s32 tooie_baseline_trot_toggle(void); /* Live atomic host preference. */
extern void (*func_8009CAAC(PlayerState*, BanjoStateId))(PlayerState*);
extern void (*func_8009CAF8(PlayerState*, BanjoStateId))(PlayerState*);
extern s32 func_8009CA70(PlayerState*, BanjoStateId, s32);
extern s32 func_8008E39C(PlayerState*);
extern s32 bakey_released(PlayerState*, ButtonId);
extern s32 bakey_pressed(PlayerState*, ButtonId);
extern f32 func_800D8FF8(void);
extern f32 func_800F0E00(f32, f32);
extern PlayerState* func_800F46D8(PlayerState*);
extern PlayerState* D_80135490[8];
enum { NONE, AWAIT_RELEASE, ARMED, EXIT_PENDING };
struct TrotToggleSlot { u32 owner; BanjoStateId last_current; u8 phase; };
__attribute__((section(".tooie_trot_state"), aligned(4)))
struct TrotToggleSlot tooie_trot_toggle_slot = {0, BS_STATE_0_INVALID, NONE};
#define SLOT tooie_trot_toggle_slot
_Static_assert(sizeof(void*) == 4 && sizeof(BanjoStateId) == 4, "guest o32 ABI");
_Static_assert(__builtin_offsetof(PlayerState, input) == 0xA4, "input layout");
_Static_assert(__builtin_offsetof(PlayerState, state) == 0x120, "state layout");
_Static_assert(__builtin_offsetof(PlayerState, unk164) == 0x164, "forced exit width");
_Static_assert(__builtin_offsetof(BanjoState, current) == 4, "state current layout");
_Static_assert(__builtin_offsetof(BaInput, dive_cooldown) == 8, "cooldown layout");
_Static_assert(sizeof(struct TrotToggleSlot) == 12 && BUTTON_Z == 1, "fixture ABI");
HELPER void tooie_trot_toggle_retire(u32 self) {
    if (SLOT.owner != 0 && SLOT.owner == self) {
        SLOT.owner = 0;
        SLOT.last_current = BS_STATE_0_INVALID;
        SLOT.phase = NONE;
    }
}
HELPER void tooie_trot_toggle_move(u32 old, u32 next) {
    if (old != next && SLOT.owner == old) SLOT.owner = next;
}
/* OFF restores original hold at the next guest seam. ON cannot adopt an
 * existing occurrence: only the outside-to-family commit below acquires it. */
INLINE s32 trot_enabled(void) {
    s32 enabled = tooie_baseline_trot_toggle();
    if (!enabled && SLOT.owner != 0) tooie_trot_toggle_retire(SLOT.owner);
    return enabled;
}
INLINE void trot_commit(PlayerState* self, BanjoStateId previous, BanjoStateId current) {
    s32 inside = func_8009CA70(self, current, 0x40);
    if (SLOT.owner == (u32)self) {
        if (previous != SLOT.last_current || !inside) tooie_trot_toggle_retire((u32)self);
        else SLOT.last_current = current;
    }
    if (!SLOT.owner && inside && !func_8009CA70(self, previous, 0x40)) {
        SLOT.owner = (u32)self;
        SLOT.last_current = current;
        SLOT.phase = AWAIT_RELEASE;
    }
}
/* Owner only while the committed state chain is unbroken; otherwise original hold. */
INLINE s32 trot_owned(PlayerState* self) {
    if (SLOT.owner != (u32)self) return 0;
    if (self->state->current == SLOT.last_current) return 1;
    tooie_trot_toggle_retire((u32)self);
    return 0;
}
INLINE void trot_sample(PlayerState* self) {
    if (!trot_owned(self)) return;
    if (SLOT.phase == AWAIT_RELEASE) {
        if (bakey_released(self, BUTTON_Z)) SLOT.phase = ARMED;
    } else if (SLOT.phase == ARMED) {
        if (bakey_pressed(self, BUTTON_Z)) SLOT.phase = EXIT_PENDING;
    }
}
PATCH void bs_setState(PlayerState *self, BanjoStateId nextState) {
    if (nextState != BS_STATE_0_INVALID) {
        self->state->next = nextState;
        if (func_8009CAAC(self, self->state->current) != NULL) {
            func_8009CAAC(self, self->state->current)(self);
        }
        self->state->previous = self->state->current;
        self->state->current = self->state->next;
        self->state->next = NULL;
        /* After commit, before init: every nested commit is observed in execution order. */
        if (trot_enabled())
            trot_commit(self, self->state->previous, self->state->current);
        if (func_8009CAF8(self, self->state->current) != NULL) {
            func_8009CAF8(self, self->state->current)(self);
        }
    }
}
PATCH void bainput_update(PlayerState *self) {
    if (self->input->dive_cooldown != 0.0f) {
        self->input->dive_cooldown = func_800F0E00(self->input->dive_cooldown - func_800D8FF8(), 0.0f);
    }
    bainput_enable(self, 6, 0);
    if (trot_enabled()) trot_sample(self);
}
PATCH s32 func_80800710_bsbtrot(PlayerState* self) {
    if (self->unk164.word) return 1;
    if (func_8008E39C(self)) return 0;
    if (!trot_enabled() || !trot_owned(self))
        return bakey_released(self, BUTTON_Z);
    return SLOT.phase == EXIT_PENDING;
}
PATCH void func_800F86D8(void) {
    PlayerState** entry;
    for (entry = D_80135490; entry != D_80135490 + 8; ++entry) {
        PlayerState* old = *entry;
        if (old != NULL) {
            PlayerState* next = func_800F46D8(old);
            tooie_trot_toggle_move((u32)old, (u32)next);
            *entry = next;
        }
    }
}
