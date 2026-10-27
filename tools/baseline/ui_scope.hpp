#ifndef TOOIE_UI_SCOPE_HPP
#define TOOIE_UI_SCOPE_HPP
#include <stdint.h>
/* Pure native identity for UI models the core DF944 wrapper draws without any BK owner (its direct DE498
 * call at DFAA0). No guest memory writes or rendering calls.
 * BK dbd52154 keys every UI element by its drawer's element and draw index: zoombox models by portrait_id
 * (projection_transform_tagging.c func_803163A8), HUD parts by item/part index (hud_transform_tagging.c).
 * DF944 exposes no such element field at its call, so the element here is the drawn model (DE498's
 * sixth argument) and the index is that model's draw ordinal within one construction. A model keeps
 * its slot while it draws; history continues only from the previous construction at the same ordinal,
 * anything else is a cut. Limits are admission limits, NOT claimed original-game maxima. */
namespace tooie_ui {
constexpr uint32_t models = 8, ordinals = 8;
constexpr uint32_t stride = 512;               // one menu model/sprite component pair: model component only
constexpr uint32_t first_id = 0x30100000u;
constexpr uint32_t end_id = first_id + models * ordinals * stride;
static_assert(end_id == 0x30108000u, "UI model domain (bk_pipeline.h TOOIE_BK_UI_ID_START/END)");
struct Entry { uint32_t model, frame, count; uint32_t drawn[ordinals]; };
struct State { Entry entries[models]{}; };
struct Decision { uint32_t id, cut; };
/* One unowned DF944 draw of `model` in construction `frame` (nonzero, nondecreasing until reset). Returns
 * {0,0} when the draw stays original: no model, every slot drawn in this or the previous construction,
 * or more than `ordinals` draws of this model in this construction. */
inline Decision draw(State& s, uint32_t model, uint32_t frame) {
    if (!model || !frame) return {};
    uint32_t i = 0;
    while (i < models && s.entries[i].model != model) ++i;
    if (i == models) {
        // A free slot first, else one whose model drew in neither this nor the previous construction.
        for (i = 0; i < models && s.entries[i].model; ++i) {}
        if (i == models)
            for (i = 0; i < models && !(frame > s.entries[i].frame && frame - s.entries[i].frame > 1u); ++i) {}
        if (i == models) return {};
        s.entries[i] = Entry{model, frame, 0, {}};
    }
    Entry& e = s.entries[i];
    if (e.frame != frame) { e.frame = frame; e.count = 0; }
    if (e.count >= ordinals) return {};
    const uint32_t k = e.count++;
    const uint32_t cut = !e.drawn[k] || frame <= e.drawn[k] || frame - e.drawn[k] != 1u;
    e.drawn[k] = frame;
    return {first_id + (i * ordinals + k) * stride, cut};
}
}
#endif
