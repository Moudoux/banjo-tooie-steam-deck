#ifndef TOOIE_BK_PIPELINE_H
#define TOOIE_BK_PIPELINE_H
/* BK-recomp model tagging adapted for Banjo-Tooie (B1 state, B2 owner/root/bone groups).
 * Adapted from BanjoRecomp dbd52154bc1e00e56867b1fac94a8f907052f41b:
 *   patches/transform_tagging.c         set_model_matrix_group, pop_model_matrix_group,
 *                                       modelRender_draw root group policy
 *   patches/marker_transform_tagging.c  __marker_draw, player_draw owner IDs
 *   patches/transform_ids.h             owner ID ranges
 *   patches/patches.h                   matrix-group convenience macros
 * Keep this attribution and the upstream licence notice with every adapted file. */
#ifdef TOOIE_BRIDGE_GUEST
#include "common.h"
typedef u32 TooieBkWord;
#else
#include <stdint.h>
typedef uint32_t TooieBkWord;
#endif

#define TOOIE_BK_MAGIC 0x424B5031u            /* 'BKP1' */
#define TOOIE_BK_LAYOUT 1u
#define TOOIE_BK_STATE_BYTES 3412u

/* Owner IDs (BK transform_ids.h layout, Tooie domains). BK: BANJO_TRANSFORM_ID_START and
 * MARKER_TRANSFORM_ID_START + spawn_index * 256. Tooie keeps 256 IDs per draw sample and has
 * 4 views x 7 phases of samples per owner. Existing live domains stay untouched:
 * product family 0x10000000-0x1FFFFFFF, product labels 0x40000000-0x407FFFFF, bridge
 * 0x90000000+, camera 0xA0000000-0xAFFFFFFF, CPU625 0xB1000000+, backpack621 0xC0000000+. */
#define TOOIE_BK_TRANSFORM_ID_COUNT 256u
#define TOOIE_BK_VIEW_COUNT 4u
#define TOOIE_BK_PHASE_COUNT 7u
#define TOOIE_BK_SAMPLE_COUNT (TOOIE_BK_VIEW_COUNT * TOOIE_BK_PHASE_COUNT)
#define TOOIE_BK_OWNER_STRIDE (TOOIE_BK_SAMPLE_COUNT * TOOIE_BK_TRANSFORM_ID_COUNT)
#define TOOIE_BK_PLAYER_ATTACHMENTS 16u       /* 0 own, 1..7 nested; 8 secondary, 9..15 nested */
#define TOOIE_BK_PLAYER_SECONDARY 8u
#define TOOIE_BK_ACTOR_ATTACHMENTS 4u         /* 0 own, 1..3 nested */
#define TOOIE_BK_ACTOR_OWNER_STRIDE (TOOIE_BK_ACTOR_ATTACHMENTS * TOOIE_BK_OWNER_STRIDE)
#define TOOIE_BK_PLAYER_ID_START 0x20000000u  /* BK BANJO_TRANSFORM_ID_START */
#define TOOIE_BK_PLAYER_ID_END (TOOIE_BK_PLAYER_ID_START + TOOIE_BK_PLAYER_ATTACHMENTS * TOOIE_BK_OWNER_STRIDE)
/* @tooie BK draws one player under BANJO_TRANSFORM_ID_START. Tooie can draw two at once (split Banjo and
 * Kazooie), each its own PlayerState in the D_80135490 roster, so each drawn player keys its whole range
 * (root, nested, secondary and backpack ordinals) by its roster slot, never by the controlled slot. Slot 0
 * keeps the range above; slots 1..7 follow the shadow domain (bk_shadow_attach.inc 0x2001C000-0x20E8C000). */
#define TOOIE_BK_PLAYER_SLOTS 8u
#define TOOIE_BK_PLAYER_SLOT_STRIDE (TOOIE_BK_PLAYER_ATTACHMENTS * TOOIE_BK_OWNER_STRIDE)
#define TOOIE_BK_PLAYER_SLOT_ID_START 0x20E8C000u
#define TOOIE_BK_PLAYER_SLOT_ID_END (TOOIE_BK_PLAYER_SLOT_ID_START + (TOOIE_BK_PLAYER_SLOTS - 1u) * TOOIE_BK_PLAYER_SLOT_STRIDE)
#define TOOIE_BK_ACTOR_ID_START 0x50000000u   /* BK MARKER_TRANSFORM_ID_START */
#define TOOIE_BK_ACTOR_ID_END 0x90000000u
#define TOOIE_BK_ACTOR_GENERATIONS ((TOOIE_BK_ACTOR_ID_END - TOOIE_BK_ACTOR_ID_START) / TOOIE_BK_ACTOR_OWNER_STRIDE)

#define TOOIE_BK_ROOT_DEPTH 8u
#define TOOIE_BK_SCOPE_DEPTH 8u
#define TOOIE_BK_DRAW_DEPTH 8u
#define TOOIE_BK_HANDLER_DEPTH 16u

/* Owner kinds. */
#define TOOIE_BK_OWNER_NONE 0u
#define TOOIE_BK_OWNER_ACTOR 1u
#define TOOIE_BK_OWNER_PLAYER 2u
#define TOOIE_BK_OWNER_SHADOW 3u              /* bk_shadow_attach.inc: shadow producer draws */
#define TOOIE_BK_OWNER_MENU 4u                /* tooie_bk_menu_model_begin: menu models and transition masks */
/* menu_scope.hpp model/sprite blocks [START, END); the five transition mask windows follow at END. The DEFB0
 * projection of a menu bit40 model is its model ID + OFFSET (shadow_projection.c), past the transition IDs. */
#define TOOIE_BK_MENU_ID_START 0x30000000u
#define TOOIE_BK_MENU_ID_END 0x30060000u
#define TOOIE_BK_MENU_PROJECTION_OFFSET 0x00080000u

struct TooieBkOwner {
    /* birth: an actor's creation generation; a player's drawing roster slot + 1 (0: no player identity). */
    TooieBkWord kind, birth, view, phase, base_attachment, attachments, own_draw_pending, id;
};
struct TooieBkRootFrame {
    TooieBkWord pushed_group, outer_offset, outer_uses_bones, reserved;
};
struct TooieBkScope {
    TooieBkWord prev_transform_id, nested;
    struct TooieBkOwner prev;
};
struct TooieBkDiagnostics {
    TooieBkWord frames, roots, root_groups, groups, late_closes, offset_overflow;
    TooieBkWord scope_overflow, root_overflow, parsed_arms, parsed_waits, suppressed_waits, player_untagged;
};
/* @tooie Visible-branch continuity (bk_select.inc): per PLAYER body sample, the executed selector choices
 * staged by its body draws in the current construction and committed at the last submission. */
#define TOOIE_BK_BRANCH_ENTRIES 8u
#define TOOIE_BK_BRANCH_SELECTORS 16u         /* executed selector occurrences per body (handler slots: 16) */
#define TOOIE_BK_BRANCH_LATCHES 4u            /* player owner samples decided in one construction */
#define TOOIE_BK_SELECT_CONTROLS 60u          /* D_8012C850..D_8012C93F */
struct TooieBkBranchSig {
    TooieBkWord model, count, masks[TOOIE_BK_BRANCH_SELECTORS];   /* executed child mask per selector, walk order */
};
struct TooieBkBranch {
    TooieBkWord id, staged_frame, staged_valid, committed_valid;
    struct TooieBkBranchSig staged, committed;
};
struct TooieBkBranchLatch { TooieBkWord owner, snap; };
/* Guest-visible resident state. The native B1 thunks own `parsed_*`; guest helpers own the rest. */
struct TooieBkState {
    TooieBkWord magic, layout;
    /* B1: one-message OSMesgQueue (24 bytes), its message slot and the single outstanding arm. */
    TooieBkWord parsed_queue[6], parsed_message, parsed_armed, parsed_root, draw_depth;
    /* B2: BK transform_tagging.c globals. */
    TooieBkWord skip_all_interpolation, cur_drawn_model_transform_id, cur_drawn_model_skip_interpolation;
    TooieBkWord cur_model_transform_id_offset, cur_model_uses_bones, root_depth, scope_depth, reserved0;
    struct TooieBkOwner owner;
    struct TooieBkRootFrame roots[TOOIE_BK_ROOT_DEPTH];
    struct TooieBkScope scopes[TOOIE_BK_SCOPE_DEPTH];
    TooieBkWord draw_roots[TOOIE_BK_DRAW_DEPTH];   /* root_depth at each DE448 entry */
    /* Packet2 original-handler hooks (billboards): opened group per invocation, keyed by the
     * handler's own SP so a path that skipped the begin seam never pops another's group. */
    TooieBkWord handler_depth, handler_reserved;
    struct { TooieBkWord sp, opened; } handlers[TOOIE_BK_HANDLER_DEPTH];
    struct TooieBkDiagnostics diagnostics;
    /* Type-12 selector numbering (bk_select.inc): bit d is set while root frame d is a reserved PLAYER or
     * owned projected-shadow root; select_declined counts such roots kept on the original numbering.
     * Selector frames live in handlers[] (sp = the selector activation's SP, opened = its base ordinal). */
    TooieBkWord select_roots, select_declined;
    /* @tooie Visible-branch continuity. branch_commits and branches[].committed* are written natively at
     * the A78E8 submission seam only (tooie_bk_branch_commit); guest helpers own the rest. */
    TooieBkWord branch_cuts, branch_declined, branch_latch_overflow, branch_commits;
    struct TooieBkBranchLatch branch_latches[TOOIE_BK_BRANCH_LATCHES];
    struct TooieBkBranch branches[TOOIE_BK_BRANCH_ENTRIES];
    /* @tooie Projected-shadow branch continuity (bk_shadow_attach.inc tooie_bk_shadow_project, DEFB0): its own
     * table keyed by shadow body sample IDs, never the player's, committed natively at A78E8 with branches[].
     * shadow_body_id: the current A6250 activation's tagged body sample; shadow_body_snap: its DEFB0
     * decision, followed by that activation's attachment. shadow_projected: the open shadow scope is an
     * A6250 body or attachment (not the blob), whose root takes the type-12 reservation above. */
    TooieBkWord shadow_cuts, shadow_declined, shadow_body_id, shadow_body_snap, shadow_projected;
    struct TooieBkBranch shadow_branches[TOOIE_BK_BRANCH_ENTRIES];
};
/* Shadow producer route and discontinuity state (bk_shadow_attach.inc), layout2. The camera
 * DEFB0/compositor helpers (shadow_projection.c, BK branch) read projection*, body_frame. */
#define TOOIE_BK_SHADOW_MAGIC 0x424B5331u     /* 'BKS1' */
#define TOOIE_BK_SHADOW_LAYOUT 2u
#define TOOIE_BK_SHADOW_SLOTS 8u              /* D_80135490 owner slots */
#define TOOIE_BK_SHADOW_LIGHT_SLOTS 64u       /* light route slots for actual handles (any positive value) */
#define TOOIE_BK_SHADOW_BYTES 1144u
struct TooieBkLightSlot {
    TooieBkWord handle;              /* actual one-based light handle held by this route slot; 0 free */
    TooieBkWord birth;               /* that handle's allocation serial (C8B78 return); 0 not seen */
    TooieBkWord drawn_frame;         /* BK float frame of this slot's last draw */
    TooieBkWord skip_until;          /* last BK float frame whose samples of this slot snap */
};
struct TooieBkShadow {
    TooieBkWord magic, layout;
    TooieBkWord route;               /* pending projected route from A6CF0/A6D60: 1 fallback, 2 light; 0 none */
    TooieBkWord light;               /* pending local-light handle (A6B24 s0) */
    TooieBkWord serial;              /* light allocation serial: C8A98 returns */
    TooieBkWord resets;              /* light vector init/free/loader resets: C90AC/C90D0/C90F4 */
    TooieBkWord body_frame;          /* A6250 frame whose body draw was tagged: full-frame composite */
    TooieBkWord projection;          /* open scope's projection-group ID; 0 snaps (IGNORE) */
    TooieBkWord projection_open;     /* a shadow scope is open for its own draw */
    TooieBkWord scopes, untagged, discontinuities, light_events, evictions;
    TooieBkWord birth[TOOIE_BK_SHADOW_SLOTS];        /* last receiver birth per owner slot */
    TooieBkWord birth_frame[TOOIE_BK_SHADOW_SLOTS];  /* BK float frame of that slot's last birth change */
    struct TooieBkLightSlot lights[TOOIE_BK_SHADOW_LIGHT_SLOTS];
};
#ifdef __cplusplus
#define TOOIE_BK_ASSERT static_assert
#else
#define TOOIE_BK_ASSERT _Static_assert
#endif
TOOIE_BK_ASSERT(sizeof(struct TooieBkOwner) == 32 && sizeof(struct TooieBkRootFrame) == 16 &&
    sizeof(struct TooieBkScope) == 40 && sizeof(struct TooieBkDiagnostics) == 48, "BK record ABI");
TOOIE_BK_ASSERT(__builtin_offsetof(struct TooieBkState, parsed_queue) == 8 &&
    __builtin_offsetof(struct TooieBkState, parsed_message) == 32 &&
    __builtin_offsetof(struct TooieBkState, parsed_armed) == 36 &&
    __builtin_offsetof(struct TooieBkState, parsed_root) == 40 &&
    __builtin_offsetof(struct TooieBkState, owner) == 80 &&
    __builtin_offsetof(struct TooieBkState, roots) == 112 &&
    __builtin_offsetof(struct TooieBkState, scopes) == 240 &&
    __builtin_offsetof(struct TooieBkState, draw_roots) == 560 &&
    __builtin_offsetof(struct TooieBkState, handler_depth) == 592 &&
    __builtin_offsetof(struct TooieBkState, handlers) == 600 &&
    __builtin_offsetof(struct TooieBkState, diagnostics) == 728 &&
    __builtin_offsetof(struct TooieBkState, select_roots) == 776 && TOOIE_BK_ROOT_DEPTH <= 32u &&
    __builtin_offsetof(struct TooieBkState, branch_cuts) == 784 &&
    __builtin_offsetof(struct TooieBkState, branch_commits) == 796 &&
    __builtin_offsetof(struct TooieBkState, branch_latches) == 800 &&
    __builtin_offsetof(struct TooieBkState, branches) == 832 &&
    __builtin_offsetof(struct TooieBkState, shadow_cuts) == 2112 &&
    __builtin_offsetof(struct TooieBkState, shadow_body_id) == 2120 &&
    __builtin_offsetof(struct TooieBkState, shadow_projected) == 2128 &&
    __builtin_offsetof(struct TooieBkState, shadow_branches) == 2132 &&
    sizeof(struct TooieBkBranchSig) == 72 && sizeof(struct TooieBkBranch) == 160 &&
    sizeof(struct TooieBkBranchLatch) == 8 &&
    sizeof(struct TooieBkState) == TOOIE_BK_STATE_BYTES, "BK resident state layout1");
TOOIE_BK_ASSERT(TOOIE_BK_PLAYER_ID_END <= 0x40000000u && TOOIE_BK_PLAYER_SLOT_ID_END <= 0x40000000u &&
    TOOIE_BK_PLAYER_SLOT_STRIDE == 0x1C000u && TOOIE_BK_ACTOR_ID_START >= 0x40800000u &&
    TOOIE_BK_ACTOR_GENERATIONS == 37449u &&
    (unsigned long long)TOOIE_BK_ACTOR_ID_START +
    (unsigned long long)TOOIE_BK_ACTOR_GENERATIONS * TOOIE_BK_ACTOR_OWNER_STRIDE <= TOOIE_BK_ACTOR_ID_END,
    "BK owner domains disjoint from live Tooie domains");
TOOIE_BK_ASSERT(__builtin_offsetof(struct TooieBkShadow, body_frame) == 24 &&
    __builtin_offsetof(struct TooieBkShadow, projection) == 28 &&
    __builtin_offsetof(struct TooieBkShadow, projection_open) == 32 &&
    __builtin_offsetof(struct TooieBkShadow, birth) == 56 &&
    __builtin_offsetof(struct TooieBkShadow, lights) == 120 && sizeof(struct TooieBkLightSlot) == 16 &&
    sizeof(struct TooieBkShadow) == TOOIE_BK_SHADOW_BYTES, "BK shadow state layout2");
#undef TOOIE_BK_ASSERT
#define TOOIE_BK_PARSED_QUEUE_OFFSET 8u
#define TOOIE_BK_PARSED_MESSAGE_OFFSET 32u
#define TOOIE_BK_PARSED_ARMED_OFFSET 36u
#define TOOIE_BK_PARSED_ROOT_OFFSET 40u
#define TOOIE_BK_DIAGNOSTICS_OFFSET 728u
#define TOOIE_BK_OFFSET_WORD_OFFSET 60u       /* cur_model_transform_id_offset */
#define TOOIE_BK_BRANCH_COMMITS_OFFSET 796u
#define TOOIE_BK_BRANCH_OFFSET 832u           /* branches[] */
#define TOOIE_BK_SHADOW_BRANCH_OFFSET 2132u   /* shadow_branches[], to the end of the state */
/* Word offsets inside diagnostics, written natively by the B1 thunks. */
#define TOOIE_BK_DIAG_PARSED_ARMS 32u
#define TOOIE_BK_DIAG_PARSED_WAITS 36u
#define TOOIE_BK_DIAG_SUPPRESSED_WAITS 40u

/* ---- Visible-branch continuity: pure rules shared by the guest, the native seam and host tests -------
 * Appearance rows of D_80123030, ordered initializer, leaf, teardown (12 bytes each). */
#define TOOIE_BK_ROW0_INIT 0x8010EF70u
#define TOOIE_BK_ROW0_LEAF 0x8010F070u
#define TOOIE_BK_ROW0_TEARDOWN 0x8010F270u
#define TOOIE_BK_ROW1_INIT 0x8010E2C4u
#define TOOIE_BK_ROW1_LEAF 0x8010E4C4u
#define TOOIE_BK_ROW1_TEARDOWN 0x8010E7F4u
/* Controls final at DF0B0 (APPEARANCE_FINALIZATION, ROW1_ADDENDUM). Mode D_8012C838 == 1 runs the selected
 * row after DF0B0: row 0 exactly (EF70/F070/F270), or row 1 exactly (E2C4/E4C4/E7F4) with argument
 * D_8012CA0C nonzero, the E2C4 route that bypasses E204's indirect light iteration. Any other mode runs
 * no initializer and keeps the installed leaf D_8012CEC4, which must be none, F070 or E4C4. `rows` is
 * the six words of rows 0 and 1. Anything else: 0, the guard declines and the draw stays original. */
#if defined(TOOIE_BRIDGE_GUEST)
__attribute__((always_inline))
#endif
static inline TooieBkWord tooie_bk_branch_appearance(TooieBkWord mode, TooieBkWord row, TooieBkWord argument,
    TooieBkWord leaf, const volatile TooieBkWord* rows) {
    if (mode != 1u) return !leaf || leaf == TOOIE_BK_ROW0_LEAF || leaf == TOOIE_BK_ROW1_LEAF;
    if (row == 0u)
        return rows[0] == TOOIE_BK_ROW0_INIT && rows[1] == TOOIE_BK_ROW0_LEAF && rows[2] == TOOIE_BK_ROW0_TEARDOWN;
    if (row == 1u)
        return argument != 0u && rows[3] == TOOIE_BK_ROW1_INIT && rows[4] == TOOIE_BK_ROW1_LEAF &&
            rows[5] == TOOIE_BK_ROW1_TEARDOWN;
    return 0;
}
/* Original 800DD56C choice: control index 0 selects no child (it exits at DD5A8; its delay slot only
 * reads control 0). Otherwise the index's control word: positive selects that one-based child when
 * within the count, zero none, negative the bitmask of its negation over the count. 0: unsupported
 * (count outside 1..32 or index outside the 60 controls). */
#if defined(TOOIE_BRIDGE_GUEST)
__attribute__((always_inline))
#endif
static inline TooieBkWord tooie_bk_branch_choice(int index, int value, int count, TooieBkWord* mask) {
    TooieBkWord limit;
    if (count <= 0 || count > 32 || index < 0 || index >= (int)TOOIE_BK_SELECT_CONTROLS) return 0;
    limit = count == 32 ? 0xFFFFFFFFu : (1u << count) - 1u;
    if (!index || !value) *mask = 0u;
    else if (value > 0) *mask = value <= count ? 1u << (value - 1) : 0u;
    else *mask = (0u - (TooieBkWord)value) & limit;
    return 1;
}
/* The player's primary body root: its own sample ID, the top-level root of its draw, before any
 * attachment. Only this root decides; nested roots and attachments follow its decision. */
static inline TooieBkWord tooie_bk_branch_is_body(TooieBkWord kind, TooieBkWord id, TooieBkWord owner,
    TooieBkWord attachments, TooieBkWord index, TooieBkWord draw_depth) {
    return kind == TOOIE_BK_OWNER_PLAYER && id && id == owner && !attachments && !index && draw_depth == 1u;
}
static inline TooieBkWord tooie_bk_branch_latched(const volatile struct TooieBkBranchLatch* latches, TooieBkWord owner) {
    TooieBkWord i;
    for (i = 0; i < TOOIE_BK_BRANCH_LATCHES; i++) if (owner && latches[i].owner == owner) return latches[i].snap;
    /* A full table cannot retain this owner: snap its body and followers consistently. */
    for (i = 0; i < TOOIE_BK_BRANCH_LATCHES; i++) if (!latches[i].owner) return 0;
    return owner != 0;
}
static inline void tooie_bk_branch_latch(volatile struct TooieBkBranchLatch* latches, TooieBkWord owner,
    TooieBkWord snap, volatile TooieBkWord* overflow) {
    TooieBkWord i;
    for (i = 0; i < TOOIE_BK_BRANCH_LATCHES; i++)
        if (latches[i].owner == owner) { latches[i].snap |= snap; return; }
    for (i = 0; i < TOOIE_BK_BRANCH_LATCHES; i++)
        if (!latches[i].owner) { latches[i].owner = owner; latches[i].snap = snap; return; }
    if (snap && *overflow != 0xFFFFFFFFu) *overflow += 1u;
}
#if defined(TOOIE_BRIDGE_GUEST)
__attribute__((always_inline))
#endif
static inline void tooie_bk_branch_copy(volatile struct TooieBkBranchSig* dst, const volatile struct TooieBkBranchSig* src) {
    TooieBkWord i;
    dst->model = src->model;
    dst->count = src->count;
    for (i = 0; i < TOOIE_BK_BRANCH_SELECTORS; i++) dst->masks[i] = i < src->count ? src->masks[i] : 0u;
}
#if defined(TOOIE_BRIDGE_GUEST)
__attribute__((always_inline))
#endif
static inline TooieBkWord tooie_bk_branch_equal(const volatile struct TooieBkBranchSig* a, const volatile struct TooieBkBranchSig* b) {
    TooieBkWord i;
    if (a->model != b->model || a->count != b->count || a->count > TOOIE_BK_BRANCH_SELECTORS) return 0;
    for (i = 0; i < a->count; i++) if (a->masks[i] != b->masks[i]) return 0;
    return 1;
}
/* One primary body draw of sample `id` in construction `frame`; `sig` is 0 when the draw is unsupported
 * (unreserved, uncertified appearance or grammar). Stages it: a construction commits only when every
 * body draw of the sample in it was supported and identical. Returns 1 when this draw differs from the
 * committed baseline (the sample's body in the last submitted workload). Unknown baselines conservatively snap until a supported, consistent workload commits. */
#if defined(TOOIE_BRIDGE_GUEST)
__attribute__((always_inline))
#endif
static inline TooieBkWord tooie_bk_branch_body(volatile struct TooieBkBranch* table, TooieBkWord id,
    const volatile struct TooieBkBranchSig* sig, TooieBkWord frame) {
    volatile struct TooieBkBranch* e = 0;
    TooieBkWord i;
    for (i = 0; i < TOOIE_BK_BRANCH_ENTRIES; i++) if (table[i].id == id) { e = &table[i]; break; }
    if (!e) {
        /* An empty entry, else the least recently staged one; a new entry has no baseline. */
        e = &table[0];
        for (i = 0; i < TOOIE_BK_BRANCH_ENTRIES; i++) {
            if (!table[i].id) { e = &table[i]; break; }
            if (table[i].staged_frame < e->staged_frame) e = &table[i];
        }
        e->id = id; e->staged_frame = 0; e->staged_valid = 0; e->committed_valid = 0;
    }
    if (e->staged_frame != frame) {
        e->staged_frame = frame;
        e->staged_valid = sig != 0;
        if (sig) tooie_bk_branch_copy(&e->staged, sig);
    }
    else if (e->staged_valid && (!sig || !tooie_bk_branch_equal(&e->staged, sig))) e->staged_valid = 0;
    return sig && (!e->committed_valid || !tooie_bk_branch_equal(&e->committed, sig));
}
/* The owned projected-shadow body at DEFB0 (tooie_bk_shadow_project): stages this draw like a player body
 * (0: unsupported, staged unknown), but returns 1 ONLY for a proven discontinuity: supported choices that
 * differ from a valid committed baseline. First sight, no baseline and unsupported draws prove nothing and
 * keep the shadow's continuous policy, as BK func_802CA7BC skips its shadow only on a detected change. */
#if defined(TOOIE_BRIDGE_GUEST)
__attribute__((always_inline))
#endif
static inline TooieBkWord tooie_bk_shadow_branch(volatile struct TooieBkBranch* table, TooieBkWord id,
    const volatile struct TooieBkBranchSig* sig, TooieBkWord frame) {
    TooieBkWord i;
    if (!tooie_bk_branch_body(table, id, sig, frame)) return 0;
    for (i = 0; i < TOOIE_BK_BRANCH_ENTRIES; i++) if (table[i].id == id) return table[i].committed_valid;
    return 0;
}
/* A78E8, construction `frame` submitted: each sample's committed baseline becomes exactly what this
 * workload drew, or unknown when it drew the sample unsupported, inconsistently, or not at all. A
 * suppressed construction never reaches here, so it never commits. */
static inline void tooie_bk_branch_commit(volatile struct TooieBkBranch* table, TooieBkWord frame) {
    TooieBkWord i;
    for (i = 0; i < TOOIE_BK_BRANCH_ENTRIES; i++) {
        volatile struct TooieBkBranch* e = &table[i];
        if (!e->id) continue;
        if (e->staged_frame == frame && e->staged_valid) {
            tooie_bk_branch_copy(&e->committed, &e->staged);
            e->committed_valid = 1;
        }
        else e->committed_valid = 0;
    }
}

#ifdef TOOIE_BRIDGE_GUEST
extern volatile struct TooieBkState tooie_bk_state;
extern volatile struct TooieBkShadow tooie_bk_shadow;
typedef struct TooieRenderBuffers TooieBkBuffers;
/* BK transform_tagging.c helpers. */
u32 tooie_bk_set_model_matrix_group(Gfx** gfx, u32 node, u32 skip_rotation);
void tooie_bk_pop_model_matrix_group(Gfx** gfx);
/* Hook leaves: A7888 frame begin, DF0B0 root begin, DF234 root end. */
void tooie_bk_frame_begin(void);
void tooie_bk_root_begin(u32 buffers, u32 cursor_slot, u32 model);
void tooie_bk_root_end(u32 buffers);
/* Draw boundaries: 80101808 and DE448. */
u32 tooie_bk_actor_draw_begin(u32 actor, u32 model);
u32 tooie_bk_de448_begin(u32 position, u32 model);
void tooie_bk_de448_end(u32 token, u32 buffers);
void tooie_bk_scope_end(u32 token);
/* DEFB0, from tooie_shadow_de498 (shadow_projection.c): owned projected-shadow decision; projection ID. */
u32 tooie_bk_shadow_project(u32 model);
#endif
#endif
