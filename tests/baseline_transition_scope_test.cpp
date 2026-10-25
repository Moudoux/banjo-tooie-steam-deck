// Transition mask ownership (transition_scope.hpp): view identity, source lifetime, submitted freshness,
// same-view refusal and registry authentication. Pure native rule only; no guest code, renderer or game data.
#include "../tools/baseline/transition_scope.hpp"
#include "../tools/baseline/menu_scope.hpp"
#include <cstdio>
#include <stdexcept>
using namespace tooie_transition;
namespace {
void require(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
// One construction: a draw of `view`, its owned root observed, then submitted or not.
Decision construct(State& s, uint32_t view, uint32_t frame, bool submit = true, bool root = true) {
    const Decision d = draw(s, view, frame);
    if (d.id && root) emitted(s, view, frame);
    if (submit) submitted(s, frame);
    return d;
}
}
int main() {
    try {
        static_assert(first_id == tooie_menu::end_id && end_id == 0x30060500u, "spare UI window right after the menu domain");
        {   // Stable per-view IDs; fresh until a submitted construction carried an owned root.
            State s;
            require(construct(s, 2, 1).id == first_id + 2u * stride && draw(s, 2, 1).id == 0, "view ID, then same-view repeat refused");
            for (uint32_t f = 2; f < 302; ++f) {
                const Decision d = construct(s, 2, f);
                require(d.id == first_id + 2u * stride && !d.cut, "300 continuous constructions interpolate");
            }
            require(construct(s, 4, 302).cut && !construct(s, 4, 303).cut, "each view has its own freshness");
        }
        {   // Suppressed construction and a draw without a root never commit.
            State s;
            require(construct(s, 0, 1, false).cut, "first draw cuts");
            require(construct(s, 0, 2, true, false).cut, "suppressed prior: still fresh");
            require(construct(s, 0, 3).cut, "no owned root in the submitted prior: still fresh");
            require(!construct(s, 0, 4).cut, "committed by the submitted owned root");
        }
        {   // Source lifetime: 808000D8, C9214 (view or all) and C929C advance generations; defrag does not exist here.
            State s;
            construct(s, 1, 1); construct(s, 3, 1);
            require(!construct(s, 1, 2).cut && !construct(s, 3, 2).cut, "both continuous");
            invalidate(s, 1);
            require(construct(s, 1, 3).cut && !construct(s, 3, 3).cut, "808000D8 restarts only its view");
            invalidate_all(s);
            require(construct(s, 1, 4).cut && construct(s, 3, 4).cut, "C929C reset (or unmatched C9214) restarts every view");
            invalidate(s, 9);
            require(!construct(s, 1, 5).cut, "an out-of-range view changes nothing");
        }
        {   // A new generation that starts after the emission but before submission stays fresh.
            State s;
            construct(s, 0, 1);
            const Decision d = draw(s, 0, 2); require(d.id && !d.cut, "continuous draw");
            emitted(s, 0, 2); invalidate(s, 0); submitted(s, 2);
            require(construct(s, 0, 3).cut, "old-generation emission cannot commit the new generation");
        }
        {   // Registry authentication: the only listed object of its view, at most five entries.
            const uint32_t ptr[slots] = {0x80200000u, 0x80200040u, 0x80200080u, 0, 0}, vw[slots] = {0u, 4u, 1u, 0, 0};
            require(registered(3, ptr, vw, 0x80200040u, 4), "listed object of its view");
            require(!registered(3, ptr, vw, 0x80200040u, 1), "object listed under another view");
            require(!registered(3, ptr, vw, 0x80300000u, 0), "unlisted (freed or defragmented-away) pointer");
            const uint32_t dup[slots] = {0u, 4u, 0u, 0, 0};
            require(!registered(3, ptr, dup, 0x80200000u, 0), "two objects of one view: refused");
            require(!registered(6, ptr, vw, 0x80200000u, 0) && !registered(3, ptr, vw, 0x80200000u, 5), "count and view bounds");
            State b;
            require(!draw(b, 5, 1).id && !draw(b, 0, 0).id, "view and frame bounds");
        }
        std::printf("baseline_transition_scope: ok\n");
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "baseline_transition_scope: %s\n", e.what());
        return 1;
    }
}
