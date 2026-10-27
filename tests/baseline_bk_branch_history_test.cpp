// Visible-branch continuity (bk_pipeline.h): the original 800DD56C choice, the certified appearance rows,
// the primary-body predicate and owner latch, per-construction staging and the A78E8 direct commit; and the
// projected shadow's own DEFB0 decision, its attachment latch and its separate table.
// Synthetic signatures only; no game data, guest code or renderer.
#include "guest/silo-candidate/bk_pipeline.h"

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <stdexcept>

namespace {
void require(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }

TooieBkBranchSig sig(TooieBkWord model, std::initializer_list<TooieBkWord> masks) {
    TooieBkBranchSig s;
    std::memset(&s, 0, sizeof(s));
    s.model = model;
    for (TooieBkWord m : masks) s.masks[s.count++] = m;
    return s;
}

TooieBkWord choice(int index, int value, int count, TooieBkWord expect_mask) {
    TooieBkWord mask = 0xDEADBEEFu;
    if (!tooie_bk_branch_choice(index, value, count, &mask)) return 0;
    return mask == expect_mask;
}

// One construction stream: A7888 begin (diagnostics.frames, latches cleared), roots, then A78E8 or not.
struct World {
    TooieBkBranch table[TOOIE_BK_BRANCH_ENTRIES];
    TooieBkBranchLatch latches[TOOIE_BK_BRANCH_LATCHES];
    TooieBkWord frame = 0, overflow = 0;
    World() { std::memset(table, 0, sizeof(table)); std::memset(latches, 0, sizeof(latches)); }
    void begin() { ++frame; std::memset(latches, 0, sizeof(latches)); }
    void submit() { tooie_bk_branch_commit(table, frame); }
    // Host mirror of bk_branch_root's decision order (bk_select.inc); `s` 0 is an unsupported body.
    TooieBkWord root(TooieBkWord id, TooieBkWord owner, TooieBkWord attachments, TooieBkWord index,
                     TooieBkWord depth, const TooieBkBranchSig *s) {
        if (!id || !owner) return 0;
        if (!tooie_bk_branch_is_body(TOOIE_BK_OWNER_PLAYER, id, owner, attachments, index, depth))
            return tooie_bk_branch_latched(latches, owner);
        const TooieBkWord snap = tooie_bk_branch_body(table, id, s, frame) | tooie_bk_branch_latched(latches, owner);
        tooie_bk_branch_latch(latches, owner, snap, &overflow);
        return snap;
    }
    TooieBkWord body(TooieBkWord owner, const TooieBkBranchSig *s) { return root(owner, owner, 0, 0, 1, s); }
};
constexpr TooieBkWord player = 0x20000500u, second = 0x20E8C500u, model = 0x802ABC30u;

// Projected shadow on one real resident state, in the guest's order (bk_shadow_attach.inc): the A6664 seam
// computes the birth/light/world-cut projection, DEFB0 (tooie_bk_shadow_project) stages and decides with the
// shared rule and zeroes the projection on a snap, the A66BC attachment of the same activation follows,
// and A78E8 commits both tables (bk_pipeline.c tooie_bk_branch_submitted).
struct Draw { TooieBkWord skip, projection; };
struct Shadows {
    TooieBkState s;
    TooieBkWord body_frame = 0;
    Shadows() { std::memset(&s, 0, sizeof(s)); }
    TooieBkWord frame() const { return s.diagnostics.frames; }
    void begin() {
        ++s.diagnostics.frames;
        std::memset(s.branch_latches, 0, sizeof(s.branch_latches));
        s.shadow_body_id = s.shadow_body_snap = s.shadow_projected = 0;
        body_frame = 0;
    }
    void submit() {
        tooie_bk_branch_commit(s.branches, frame());
        tooie_bk_branch_commit(s.shadow_branches, frame());
    }
    // A6664 then DEFB0. `reason`: the seam's birth/light skip; `cut`: the world cut; `v` 0: unsupported.
    Draw body(TooieBkWord id, TooieBkWord activation, TooieBkWord reason, TooieBkWord cut, const TooieBkBranchSig *v) {
        body_frame = 0; s.shadow_body_id = 0; s.shadow_body_snap = 0;
        Draw d{reason || cut, id + 2u * TOOIE_BK_OWNER_STRIDE};
        s.shadow_body_id = id; body_frame = activation;
        const TooieBkWord snap = tooie_bk_shadow_branch(s.shadow_branches, id, v, frame());
        s.shadow_body_snap |= snap;
        if (snap) d.skip = 1;
        return d;
    }
    Draw attachment(TooieBkWord id, TooieBkWord body_id, TooieBkWord activation, TooieBkWord reason, TooieBkWord cut) {
        Draw d{reason || cut, 0};
        if (body_frame == activation && s.shadow_body_id && s.shadow_body_id == body_id) d.skip |= s.shadow_body_snap;
        d.projection = id + 2u * TOOIE_BK_OWNER_STRIDE;
        return d;
    }
    // The player body at its DF0B0 root (bk_branch_root), on the same state.
    TooieBkWord player_body(TooieBkWord id, const TooieBkBranchSig *v) {
        const TooieBkWord snap = tooie_bk_branch_body(s.branches, id, v, frame()) | tooie_bk_branch_latched(s.branch_latches, id);
        tooie_bk_branch_latch(s.branch_latches, id, snap, &s.branch_latch_overflow);
        return snap;
    }
};
// Shadow body samples: owner slot 0 fallback, its attachment, slot 0 light route 0, slot 1 fallback.
constexpr TooieBkWord shadow = 0x2001C000u, shadow_attach = shadow + TOOIE_BK_OWNER_STRIDE,
    shadow_light = shadow + 2u * 4u * TOOIE_BK_OWNER_STRIDE, shadow_slot1 = shadow + 66u * 4u * TOOIE_BK_OWNER_STRIDE,
    silhouette = 0x802A3BA0u, sp = 0x801FF5A0u;
} // namespace

int main() {
    try {
        static_assert(sizeof(TooieBkState) == TOOIE_BK_STATE_BYTES && TOOIE_BK_STATE_BYTES == 3412u, "layout");
        static_assert(offsetof(TooieBkState, branch_cuts) == 784u && offsetof(TooieBkState, branch_declined) == 788u &&
            offsetof(TooieBkState, branch_latch_overflow) == 792u &&
            offsetof(TooieBkState, branch_commits) == TOOIE_BK_BRANCH_COMMITS_OFFSET &&
            offsetof(TooieBkState, branches) == TOOIE_BK_BRANCH_OFFSET &&
            TOOIE_BK_BRANCH_OFFSET + sizeof(TooieBkBranch) * TOOIE_BK_BRANCH_ENTRIES == offsetof(TooieBkState, shadow_cuts) &&
            offsetof(TooieBkState, shadow_cuts) == 2112u && offsetof(TooieBkState, shadow_declined) == 2116u &&
            offsetof(TooieBkState, shadow_body_id) == 2120u && offsetof(TooieBkState, shadow_body_snap) == 2124u &&
            offsetof(TooieBkState, shadow_projected) == 2128u &&
            offsetof(TooieBkState, shadow_branches) == TOOIE_BK_SHADOW_BRANCH_OFFSET &&
            TOOIE_BK_SHADOW_BRANCH_OFFSET + sizeof(TooieBkBranch) * TOOIE_BK_BRANCH_ENTRIES == TOOIE_BK_STATE_BYTES,
            "branch block offsets");
        static_assert(shadow >= TOOIE_BK_PLAYER_ID_END && shadow_slot1 < TOOIE_BK_PLAYER_SLOT_ID_START,
            "shadow samples are outside both player ranges");
        const TooieBkBranchSig closed = sig(model, {1u, 0u}), open = sig(model, {1u, 2u});

        // Zero index: 800DD56C exits with no child whatever control 0 holds.
        require(choice(0, 5, 4, 0u) && choice(0, -3, 4, 0u) && choice(0, 0, 4, 0u), "index 0 selects no child");
        // Positive one-based child within the count, out of range none, zero none, negative bitmask.
        require(choice(3, 2, 4, 2u) && choice(3, 4, 4, 8u) && choice(3, 5, 4, 0u) && choice(3, 0, 4, 0u), "positive");
        require(choice(3, -5, 4, 5u) && choice(3, -0x1F, 3, 7u) && choice(59, -1, 32, 1u) && choice(59, -0x7FFFFFFF, 32, 0x7FFFFFFFu) &&
            choice(59, 32, 32, 0x80000000u), "negative mask over the count");
        TooieBkWord unused = 0;
        require(!tooie_bk_branch_choice(60, 1, 4, &unused) && !tooie_bk_branch_choice(-1, 1, 4, &unused) &&
            !tooie_bk_branch_choice(3, 1, 0, &unused) && !tooie_bk_branch_choice(3, 1, 33, &unused) &&
            !tooie_bk_branch_choice(0, 1, 33, &unused), "unsupported index/count declines");

        // Exact ordered appearance rows; row1 only with D_8012CA0C nonzero; other modes by installed leaf.
        {
            const TooieBkWord rows[6] = {TOOIE_BK_ROW0_INIT, TOOIE_BK_ROW0_LEAF, TOOIE_BK_ROW0_TEARDOWN,
                                         TOOIE_BK_ROW1_INIT, TOOIE_BK_ROW1_LEAF, TOOIE_BK_ROW1_TEARDOWN};
            require(tooie_bk_branch_appearance(1, 0, 0, 0, rows) && tooie_bk_branch_appearance(1, 0, 7, 0, rows), "row0 exact");
            require(tooie_bk_branch_appearance(1, 1, 1, 0, rows), "row1 exact, argument nonzero");
            require(!tooie_bk_branch_appearance(1, 1, 0, 0, rows), "row1 with argument 0 declines");
            require(!tooie_bk_branch_appearance(1, 2, 1, 0, rows), "row2 declines");
            TooieBkWord permuted[6];
            std::memcpy(permuted, rows, sizeof(rows));
            permuted[3] = TOOIE_BK_ROW1_LEAF; permuted[4] = TOOIE_BK_ROW1_INIT;
            require(!tooie_bk_branch_appearance(1, 1, 1, 0, permuted), "row1 permuted targets decline");
            require(tooie_bk_branch_appearance(1, 0, 0, 0, permuted), "row0 unaffected by row1 words");
            std::memcpy(permuted, rows, sizeof(rows));
            permuted[5] = TOOIE_BK_ROW0_TEARDOWN;
            require(!tooie_bk_branch_appearance(1, 1, 1, 0, permuted), "row1 changed teardown declines");
            std::memcpy(permuted, rows, sizeof(rows));
            permuted[0] = TOOIE_BK_ROW0_LEAF; permuted[1] = TOOIE_BK_ROW0_INIT;
            require(!tooie_bk_branch_appearance(1, 0, 0, 0, permuted), "row0 permuted targets decline");
            require(tooie_bk_branch_appearance(0, 9, 0, 0, rows) && tooie_bk_branch_appearance(2, 0, 0, TOOIE_BK_ROW0_LEAF, rows) &&
                tooie_bk_branch_appearance(0, 0, 0, TOOIE_BK_ROW1_LEAF, rows), "other mode, certified installed leaf");
            require(!tooie_bk_branch_appearance(0, 0, 0, TOOIE_BK_ROW1_INIT, rows) &&
                !tooie_bk_branch_appearance(0, 0, 0, 0x8010F074u, rows), "other mode, other leaf declines");
        }

        // Primary body predicate: own sample ID, top-level root 0, no attachment.
        require(tooie_bk_branch_is_body(TOOIE_BK_OWNER_PLAYER, player, player, 0, 0, 1), "body");
        require(!tooie_bk_branch_is_body(TOOIE_BK_OWNER_PLAYER, player, player, 1, 0, 1) &&
            !tooie_bk_branch_is_body(TOOIE_BK_OWNER_PLAYER, player, player, 0, 1, 1) &&
            !tooie_bk_branch_is_body(TOOIE_BK_OWNER_PLAYER, player, player, 0, 0, 2) &&
            !tooie_bk_branch_is_body(TOOIE_BK_OWNER_PLAYER, player + 1u, player, 0, 0, 1) &&
            !tooie_bk_branch_is_body(TOOIE_BK_OWNER_ACTOR, player, player, 0, 0, 1) &&
            !tooie_bk_branch_is_body(TOOIE_BK_OWNER_PLAYER, 0, 0, 0, 0, 1), "not the body");

        // Latch: OR on repeat, distinct owners, overflow counted only for a lost snap.
        {
            TooieBkBranchLatch l[TOOIE_BK_BRANCH_LATCHES];
            TooieBkWord overflow = 0;
            std::memset(l, 0, sizeof(l));
            tooie_bk_branch_latch(l, player, 1, &overflow);
            tooie_bk_branch_latch(l, player, 0, &overflow);
            require(tooie_bk_branch_latched(l, player) == 1u && !tooie_bk_branch_latched(l, second) &&
                !tooie_bk_branch_latched(l, 0), "latch ORs");
            for (TooieBkWord i = 1; i < TOOIE_BK_BRANCH_LATCHES; ++i) tooie_bk_branch_latch(l, second + i, 0, &overflow);
            tooie_bk_branch_latch(l, 0x20000900u, 0, &overflow);
            require(overflow == 0, "full latches, no snap lost");
            tooie_bk_branch_latch(l, 0x20000900u, 1, &overflow);
            require(overflow == 1 && tooie_bk_branch_latched(l, 0x20000900u), "overflow owner conservatively snaps");
        }

        // Stable choices never cut; a change cuts once; the next submitted construction interpolates.
        {
            World w;
            w.begin(); require(w.body(player, &closed), "first construction conservatively snaps without baseline"); w.submit();
            w.begin(); require(!w.body(player, &closed), "stable"); w.submit();
            w.begin(); require(w.body(player, &open), "newly visible branch cuts"); w.submit();
            w.begin(); require(!w.body(player, &open), "committed open interpolates"); w.submit();
        }

        // Repeated changed frame: every repeat compares against the same committed baseline.
        {
            World w;
            w.begin(); w.body(player, &closed); w.submit();
            w.begin(); require(w.body(player, &open) && w.body(player, &open) && w.body(player, &open), "every repeat cuts");
            w.submit();
            w.begin(); require(!w.body(player, &open), "consistent repeats committed"); w.submit();
            // An inconsistent repeat compares each draw to the baseline and commits unknown.
            w.begin(); require(w.body(player, &closed) && w.body(player, &open) && w.body(player, &closed), "latched cut survives return to baseline");
            w.submit();
            w.begin(); require(w.body(player, &closed) && w.body(player, &open), "unknown inconsistent prior conservatively snaps");
        }

        // Suppressed construction: never commits, so the next submitted one cuts against the RT64 prior.
        {
            World w;
            w.begin(); w.body(player, &open); w.submit();              // RT64 prior: open
            w.begin(); require(w.body(player, &closed), "suppressed construction decides"); // no A78E8
            w.begin(); require(w.body(player, &closed), "still cuts against open"); w.submit();
            w.begin(); require(!w.body(player, &closed), "then interpolates"); w.submit();
            // A suppressed unsupported draw does not invalidate the submitted baseline either.
            w.begin(); w.body(player, nullptr);
            w.begin(); require(w.body(player, &open), "baseline survives a suppressed unsupported draw"); w.submit();
        }

        // Unsupported between valid: the intervening RT64 draw makes history unknown; no stale comparison.
        {
            World w;
            w.begin(); w.body(player, &closed); w.submit();
            w.begin(); require(!w.body(player, nullptr), "unsupported never cuts"); w.submit();
            w.begin(); require(w.body(player, &open), "unknown unsupported prior conservatively snaps"); w.submit();
            w.begin(); require(!w.body(player, &open), "valid again"); w.submit();
            // Supported then unsupported in one construction: the cut stands, the commit is unknown.
            w.begin(); require(w.body(player, &closed) && w.body(player, nullptr), "mixed construction retains owner latch"); w.submit();
            w.begin(); require(w.body(player, &open), "mixed construction committed unknown so next snaps"); w.submit();
            // Unsupported first, supported repeat: still compared to the baseline, still committed unknown.
            w.begin(); require(!w.body(player, nullptr) && w.body(player, &closed), "late supported repeat"); w.submit();
            w.begin(); require(w.body(player, &open), "committed unknown conservatively snaps"); w.submit();
        }

        // Submitted without the body: the workload had none of its groups, so history is unknown.
        {
            World w;
            w.begin(); w.body(player, &closed); w.submit();
            w.begin(); w.submit();
            w.begin(); require(w.body(player, &open), "conservative snap across absent workload"); w.submit();
        }

        // Nested roots and attachments follow the body decided at its root; they never decide.
        {
            World w;
            const TooieBkWord nested = player + 0x40u, backpack = player + TOOIE_BK_OWNER_STRIDE;
            w.begin(); w.body(player, &closed); w.body(second, &closed); w.submit();
            w.begin();
            require(w.body(player, &open), "body decides");
            require(w.root(nested, player, 0, 1, 1, &closed), "nested root (index 1) follows");
            require(w.root(player, player, 0, 0, 2, &closed), "nested draw depth follows");
            require(w.root(backpack, player, 1, 0, 1, &closed), "attachment (92710 backpack) follows");
            require(!w.body(second, &closed) && !w.root(second + 0x40u, second, 1, 0, 1, &open), "other player independent");
            w.submit();
            // An attachment before any body decides nothing and stages nothing.
            w.begin();
            require(!w.root(backpack, player, 1, 0, 1, &closed), "attachment before the body: no latch");
            w.submit();
            w.begin(); require(w.body(player, &closed), "attachment-only unknown baseline snaps"); w.submit();
            w.begin(); require(!w.root(nested, player, 0, 1, 1, &closed), "latches cleared per construction");
        }

        // Model identity and selector sequence are part of the signature.
        {
            World w;
            w.begin(); w.body(player, &closed); w.submit();
            const TooieBkBranchSig other_model = sig(0x80214470u, {1u, 0u}), shorter = sig(0x80214470u, {1u});
            w.begin(); require(w.body(player, &other_model), "model change cuts"); w.submit();
            w.begin(); require(w.body(player, &shorter), "selector sequence change cuts"); w.submit();
        }

        // Samples are independent; an evicted sample restarts without a baseline.
        {
            World w;
            w.begin(); w.body(player, &closed); w.submit();
            for (TooieBkWord i = 0; i < TOOIE_BK_BRANCH_ENTRIES; ++i) {
                w.begin(); w.body(0x20001000u + i * 0x100u, &closed); w.submit();
            }
            w.begin(); require(w.body(player, &open), "evicted sample conservatively snaps");
        }

        // ---- Projected shadow (tooie_bk_shadow_project) ----------------------------------------------------
        const TooieBkBranchSig lit = sig(silhouette, {1u, 2u, 1u}), dim = sig(silhouette, {1u, 0u, 1u});

        // Regression (every ordinary shadow snapped): unsupported, first-sight and no-baseline draws prove no
        // discontinuity and keep the continuous policy; only a supported change against a valid baseline snaps.
        {
            Shadows w;
            for (int i = 0; i < 300; ++i) {
                w.begin(); const Draw d = w.body(shadow, sp, 0, 0, nullptr);
                require(!d.skip && d.projection == shadow + 2u * TOOIE_BK_OWNER_STRIDE, "300 unsupported constructions never snap");
                w.submit();
            }
            w.begin(); require(!w.body(shadow, sp, 0, 0, &lit).skip, "first supported draw after unknown: nothing proven"); w.submit();
            for (int i = 0; i < 300; ++i) {
                w.begin(); require(!w.body(shadow, sp, 0, 0, &lit).skip, "300 stable constructions interpolate"); w.submit();
            }
            w.begin(); require(w.body(shadow, sp, 0, 0, &dim).skip, "proven change snaps"); w.submit();
            w.begin(); require(!w.body(shadow, sp, 0, 0, nullptr).skip, "unsupported after a change: no snap"); w.submit();
            w.begin(); require(!w.body(shadow, sp, 0, 0, &lit).skip, "no comparison across the unsupported workload"); w.submit();
            Shadows f;
            f.begin(); require(!f.body(shadow, sp, 0, 0, &lit).skip, "first sight: no baseline, no snap");
        }

        // Ordering: the shadow decides at DEFB0 from its own choices before the later player body; neither
        // decision nor table reaches the other, and a snap keeps the projection ID with SKIP before any root.
        {
            Shadows w;
            w.begin(); w.body(shadow, sp, 0, 0, &lit); require(w.player_body(player, &closed), "player first sight"); w.submit();
            w.begin();
            Draw d = w.body(shadow, sp, 0, 0, &lit);
            require(!d.skip && d.projection == shadow + 2u * TOOIE_BK_OWNER_STRIDE, "stable shadow keeps its projection");
            require(w.player_body(player, &open), "later player change cuts the player only");
            require(!d.skip && d.projection, "the earlier shadow decision is unchanged by it");
            w.submit();
            w.begin();
            TooieBkBranch players[TOOIE_BK_BRANCH_ENTRIES];
            std::memcpy(players, w.s.branches, sizeof(players));
            d = w.body(shadow, sp, 0, 0, &dim);
            require(d.skip && d.projection == shadow + 2u * TOOIE_BK_OWNER_STRIDE, "shadow change: identified SkipAll projection before its root");
            require(!std::memcmp(players, w.s.branches, sizeof(players)), "shadow decision never touches the player table");
            require(!w.player_body(player, &open), "player stable despite the shadow change");
            w.submit();
            for (const TooieBkBranch& e : w.s.branches) require(e.id != shadow, "no shadow entry in the player table");
            for (const TooieBkBranch& e : w.s.shadow_branches) require(e.id != player, "no player entry in the shadow table");
        }

        // Repeated construction: one snap for a change, then interpolation; repeats compare to one baseline.
        {
            Shadows w;
            w.begin(); w.body(shadow, sp, 0, 0, &lit); w.submit();
            w.begin(); require(!w.body(shadow, sp, 0, 0, &lit).skip, "stable"); w.submit();
            w.begin(); require(w.body(shadow, sp, 0, 0, &dim).skip && w.body(shadow, sp, 0, 0, &dim).skip,
                "both draws of the changed construction snap"); w.submit();
            w.begin(); require(!w.body(shadow, sp, 0, 0, &dim).skip, "next submitted construction interpolates"); w.submit();
            w.begin(); require(!w.body(shadow, sp, 0, 0, &dim).skip, "and stays interpolated"); w.submit();
        }

        // Suppressed submission: its choices never become the baseline.
        {
            Shadows w;
            w.begin(); w.body(shadow, sp, 0, 0, &lit); w.submit();
            w.begin(); require(w.body(shadow, sp, 0, 0, &dim).skip, "suppressed construction decides");     // no A78E8
            w.begin(); require(w.body(shadow, sp, 0, 0, &dim).skip, "still a change against the submitted lit"); w.submit();
            w.begin(); require(!w.body(shadow, sp, 0, 0, &dim).skip, "then interpolates"); w.submit();
            w.begin(); require(w.body(shadow, sp, 0, 0, nullptr).projection, "suppressed unsupported draw keeps its projection"); // no A78E8
            w.begin(); require(!w.body(shadow, sp, 0, 0, &dim).skip, "baseline survives a suppressed unsupported draw"); w.submit();
        }

        // Attachment: follows its own activation's body decision only.
        {
            Shadows w;
            w.begin(); w.body(shadow, sp, 0, 0, &lit); w.body(shadow_light, sp, 0, 0, &lit); w.submit();
            w.begin();
            w.body(shadow, sp, 0, 0, &dim);
            Draw a = w.attachment(shadow_attach, shadow, sp, 0, 0);
            require(a.skip && a.projection == shadow_attach + 2u * TOOIE_BK_OWNER_STRIDE, "attachment of the snapped body snaps");
            // The next activation (light route, same A6250 sp) decides afresh; its attachment follows it alone.
            require(!w.body(shadow_light, sp, 0, 0, &lit).skip, "other route stable");
            a = w.attachment(shadow_light + TOOIE_BK_OWNER_STRIDE, shadow_light, sp, 0, 0);
            require(!a.skip, "no leak of the earlier activation's snap");
            require(!w.attachment(shadow_attach, shadow, sp, 0, 0).skip, "stale body sample does not match");
            require(!w.attachment(shadow_light + TOOIE_BK_OWNER_STRIDE, shadow_light, sp + 8u, 0, 0).skip, "other activation frame");
            w.submit();
        }

        // Unrelated owners: another owner slot and another route keep separate baselines.
        {
            Shadows w;
            w.begin(); w.body(shadow, sp, 0, 0, &lit); w.body(shadow_slot1, sp, 0, 0, &lit); w.body(shadow_light, sp, 0, 0, &lit); w.submit();
            w.begin();
            require(w.body(shadow, sp, 0, 0, &dim).skip, "slot 0 fallback changes");
            require(!w.body(shadow_slot1, sp, 0, 0, &lit).skip && !w.body(shadow_light, sp, 0, 0, &lit).skip, "others stay");
            w.submit();
            w.begin(); require(!w.body(shadow_slot1, sp, 0, 0, &lit).skip, "slot 1 still on its own baseline"); w.submit();
        }

        // Existing reasons: birth/light skip and the world cut are only added to, never cleared.
        {
            Shadows w;
            w.begin(); w.body(shadow, sp, 0, 0, &lit); w.submit();
            Draw d;
            w.begin(); d = w.body(shadow, sp, 1, 0, &lit); require(d.skip && d.projection == shadow + 2u * TOOIE_BK_OWNER_STRIDE, "birth/light skip kept"); w.submit();
            w.begin(); d = w.body(shadow, sp, 0, 1, &lit); require(d.skip && d.projection == shadow + 2u * TOOIE_BK_OWNER_STRIDE, "world cut skips the draw with its identified projection"); w.submit();
            w.begin(); d = w.body(shadow, sp, 1, 1, &dim); require(d.skip && d.projection == shadow + 2u * TOOIE_BK_OWNER_STRIDE, "all reasons together"); w.submit();
            w.begin(); d = w.attachment(shadow_attach, shadow, sp, 1, 0); require(d.skip, "attachment's own reason kept");
        }
        std::printf("baseline_bk_branch_history: ok\n");
        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "baseline_bk_branch_history: %s\n", e.what());
        return 1;
    }
}
