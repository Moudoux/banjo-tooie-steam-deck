#ifndef TOOIE_TRANSITION_SCOPE_HPP
#define TOOIE_TRANSITION_SCOPE_HPP
#include <stdint.h>
/* Pure native transition-mask model owner (TRANSITION_TAG_CONTRACT). BK dbd52154 transition_patches.c
 * gctransition_draw keeps one stable TRANSITION_MODEL_TRANSFORM_ID around every mask modelRender_draw.
 * Tooie's original C9150/C91C8 keep at most one transition object per view (0..4, object +0) in the
 * D_8012AB40 pointer list (byte count D_8012AB54); C9484 defrag rewrites pointers and C9214 compacts the
 * list, so neither pointer nor index is an identity: the view is. Lifetime is source-driven only:
 *   - 808000D8 (config/state; may free the object) entry: that view's generation advances;
 *   - C9214 (free + compact) entry: the matching registered view; an unlisted pointer is a no-op;
 *   - C929C (registry reset) entry: every view.
 * A generation stays fresh (its draws SKIP) until one of its owned roots, bit40 included, is part of an
 * actually submitted construction. One owned draw per view per construction; a repeat stays original.
 * IDs: the spare UI window 0x30060000.. inside the existing guest owner window [0x30000000,0x30060500),
 * after the menu domain; one 256-node window per view. No guest writes. */
namespace tooie_transition {
constexpr uint32_t views = 5, slots = 5, first_id = 0x30060000u, stride = 256;
constexpr uint32_t end_id = first_id + views * stride;
static_assert(end_id <= 0x30060500u, "inside the existing BK UI owner window (bk_pipeline.c)");
struct View { uint32_t generation, fresh, drawn_frame, emitted_frame, emitted_generation; };
struct State { View v[views]{{0,1,0,0,0},{0,1,0,0,0},{0,1,0,0,0},{0,1,0,0,0},{0,1,0,0,0}}; };
struct Decision { uint32_t id, cut; };
inline void invalidate(State& s, uint32_t view) {
    if (view < views) { s.v[view].generation += 1u; s.v[view].fresh = 1u; }
}
inline void invalidate_all(State& s) { for (uint32_t i = 0; i < views; ++i) invalidate(s, i); }
/* The original registry holds `object` as the only object of `view`: count <= 5, `object` listed, and no
 * other listed object of the same view. `listed_views[i]` is listed object i's +0 word. */
inline bool registered(uint32_t count, const uint32_t listed[slots], const uint32_t listed_views[slots],
    uint32_t object, uint32_t view) {
    uint32_t i, matches = 0, found = 0;
    if (count > slots || view >= views || !object) return false;
    for (i = 0; i < count; ++i) {
        if (listed_views[i] == view) ++matches;
        if (listed[i] == object) found = listed_views[i] == view;
    }
    return found && matches == 1u;
}
/* One owned mask draw of `view` in BK construction `frame` (nonzero). id 0: refused (original draw). */
inline Decision draw(State& s, uint32_t view, uint32_t frame) {
    if (view >= views || !frame || s.v[view].drawn_frame == frame) return {0u, 0u};
    s.v[view].drawn_frame = frame;
    return {first_id + view * stride, s.v[view].fresh};
}
/* The owned scope closed after its DE498 root began (normal or bit40). */
inline void emitted(State& s, uint32_t view, uint32_t frame) {
    if (view < views) { s.v[view].emitted_frame = frame; s.v[view].emitted_generation = s.v[view].generation; }
}
/* A78E8: construction `frame` was submitted. Only a generation that emitted in it becomes continuous. */
inline void submitted(State& s, uint32_t frame) {
    for (uint32_t i = 0; i < views; ++i)
        if (frame && s.v[i].emitted_frame == frame && s.v[i].emitted_generation == s.v[i].generation) s.v[i].fresh = 0u;
}
}
#endif
