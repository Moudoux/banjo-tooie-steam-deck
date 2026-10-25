#ifndef TOOIE_MENU_SCOPE_HPP
#define TOOIE_MENU_SCOPE_HPP
#include <stdint.h>
/* Pure native menu owner state. No guest memory writes or rendering calls.
 * BK dbd52154 projection_transform_tagging.c supplies the separate box/portrait
 * identity design. Tooie physical slot and wrapping-role distinctions are local.
 * Limits below are admission limits, NOT claimed original-game maxima. */
namespace tooie_menu {
constexpr uint32_t slots = 3, invocations = 2, boxes = 32, roles = 4, components = 2;
constexpr uint32_t first_id = 0x30000000u;
constexpr uint32_t component_stride = 256;
constexpr uint32_t slot_stride = invocations * boxes * roles * components * component_stride;
constexpr uint32_t end_id = first_id + slots * slot_stride;
static_assert(end_id <= 0x40000000u, "menu must stay below product-label domain");
enum Role : uint32_t { primary, upper_wrap, lower_wrap, extra };
enum Component : uint32_t { model, sprite };
struct Slot { uint32_t owner, frame, draws, fresh, emitted; };
struct Context { uint32_t valid, slot, owner, gfx, invocation, box, role, cut; };
struct State { Slot entries[slots]{}; Context active{}; };
/* Original option init5/free7 are the only lifecycle authority. An unavailable
 * slot stays unregistered; never evict a still-live option to tag a later one. */
inline bool initialize(State& s, uint32_t owner, uint32_t frame) {
    if (!owner || !frame) return false;
    for (auto& e : s.entries) if (e.owner == owner) { e.fresh=1; e.emitted=0; return true; }
    for (auto& e : s.entries) if (!e.owner && e.frame!=frame) { e={owner,frame,0,1,0}; return true; }
    return false;
}
inline void retire(State& s, uint32_t owner) {
    for (auto& e : s.entries) if (e.owner == owner) { e.owner=0; e.emitted=0; }
}
/* Every caller saves/restores Context, INCLUDING a refused invocation. Otherwise
 * an unsupported nested menu could accidentally inherit the outer owner's IDs. */
inline Context enter(State& s, uint32_t owner, uint32_t gfx, uint32_t frame) {
    Context previous = s.active;
    s.active = {};
    if (!owner || !gfx || !frame) return previous;
    for (uint32_t i=0; i<slots; ++i) {
        auto& e=s.entries[i];
        if (e.owner != owner) continue;
        if (e.frame != frame) { e.frame=frame; e.draws=0; e.emitted=0; }
        if (e.draws >= invocations) return previous;
        s.active={1,i,owner,gfx,e.draws++,0,0,e.fresh};
        return previous;
    }
    return previous;
}
inline void leave(State& s, const Context& previous) { s.active=previous; }
/* Called only after actual submitted construction, never construction-end.
 * Init/retire later in that construction must not accidentally clear freshness:
 * keep fresh unless this exact live slot actually had an admitted draw. */
inline void emitted(State& s) { if(s.active.valid) s.entries[s.active.slot].emitted=1; }
inline void submitted(State& s, uint32_t frame) {
    for (auto& e:s.entries) if (e.owner && e.frame==frame && e.emitted) e.fresh=0;
}
inline bool occurrence(State& s, uint32_t box, uint32_t role) {
    if (!s.active.valid || box>=boxes || role>=roles) { s.active.valid=0; return false; }
    s.active.box=box; s.active.role=role; return true;
}
inline uint32_t identity(const State& s, uint32_t gfx, uint32_t component) {
    const auto& c=s.active;
    if (!c.valid || c.gfx!=gfx || component>=components) return 0;
    return first_id + c.slot*slot_stride +
        (((c.invocation*boxes+c.box)*roles+c.role)*components+component)*component_stride;
}
}
#endif
