#include "../tools/baseline/menu_scope.hpp"
#include "../tools/baseline/ui_scope.hpp"
#include <cassert>
using namespace tooie_menu;
int main() {
    State s;
    assert(initialize(s,0x100,1)); assert(initialize(s,0x200,1)); assert(initialize(s,0x300,1));
    assert(!initialize(s,0x400,1)); // Disproves unchecked singleton/replacement design.
    auto outer=enter(s,0x100,0x1000,1);
    assert(occurrence(s,2,primary)); const auto a=identity(s,0x1000,model);
    emitted(s); assert(a && !identity(s,0x2000,model));
    assert(identity(s,0x1000,sprite)!=a);
    assert(occurrence(s,2,upper_wrap)); assert(identity(s,0x1000,model)!=a);
    auto nested=enter(s,0x400,0x1000,1);
    assert(!identity(s,0x1000,model)); // Refused child must NOT inherit outer ID.
    leave(s,nested); assert(identity(s,0x1000,model));
    leave(s,outer);
    auto repeated=enter(s,0x100,0x1000,1); occurrence(s,2,primary);
    assert(identity(s,0x1000,model)!=a); leave(s,repeated);
    auto overflow=enter(s,0x100,0x1000,1); assert(!identity(s,0x1000,model)); leave(s,overflow);
    submitted(s,1);
    auto next=enter(s,0x100,0x1000,2); occurrence(s,2,primary);
    assert(identity(s,0x1000,model)==a && !s.active.cut); leave(s,next);
    retire(s,0x100); assert(!initialize(s,0x500,2)); assert(initialize(s,0x500,3));
    auto reused=enter(s,0x500,0x1000,3); assert(s.active.cut);
    assert(!occurrence(s,boxes,primary)); assert(!identity(s,0x1000,model)); leave(s,reused);
    // A construction that is never submitted cannot consume the lifetime cut.
    auto suppressed=enter(s,0x500,0x1000,4); assert(s.active.cut); leave(s,suppressed);
    auto future=enter(s,0x500,0x1000,5); assert(s.active.cut); leave(s,future);
    // A moved option draws at an address that never passed init. Stale registrations
    // must not permanently starve it, or associate it with another option's history.
    State moved;
    assert(initialize(moved,0x10,1) && initialize(moved,0x20,1) && initialize(moved,0x30,1));
    auto recent=enter(moved,0x40,0x1000,2); assert(!moved.active.valid); leave(moved,recent);
    auto admitted=enter(moved,0x40,0x1000,3);
    assert(moved.active.valid && moved.active.cut);
    leave(moved,admitted);
    // Draw-time admission (post-4.4 captures: init5 addresses that never draw again hold every slot).
    State t;
    assert(initialize(t,0x10,1) && initialize(t,0x20,1) && initialize(t,0x30,1) && !initialize(t,0x40,2));
    auto early=enter(t,0x40,0x1000,2); assert(!t.active.valid); leave(t,early);   // previous-construction owners protected
    auto drawn=enter(t,0x20,0x1000,3); leave(t,drawn);                            // 0x20 draws in construction 3
    auto late=enter(t,0x40,0x1000,3);
    assert(t.active.valid && t.active.slot==0 && t.active.cut && t.entries[0].owner==0x40 && t.entries[1].owner==0x20);
    leave(t,late);
    auto back=enter(t,0x10,0x1000,4); assert(t.active.valid && t.active.slot==2 && t.active.cut); leave(t,back); // displaced owner readmitted fresh
    auto held=enter(t,0x50,0x1000,4); assert(!t.active.valid); leave(t,held);     // every slot drew in 3 or 4
    retire(t,0x40); auto freed=enter(t,0x50,0x1000,6); assert(t.active.valid && t.active.slot==0); leave(t,freed); // free slot first
    // History continues only from the previous construction (BK); a skipped construction restarts it.
    State g; assert(initialize(g,0x60,1));
    auto first=enter(g,0x60,0x1000,1); assert(g.active.cut); emitted(g); leave(g,first); submitted(g,1);
    auto next1=enter(g,0x60,0x1000,2); assert(g.active.valid && !g.active.cut); emitted(g); leave(g,next1); submitted(g,2);
    auto resumed=enter(g,0x60,0x1000,4); assert(g.active.valid && g.active.cut); leave(g,resumed);
    State edge;
    assert(initialize(edge,1,0xFFFFFFFFu) && initialize(edge,2,0xFFFFFFFFu) && initialize(edge,3,0xFFFFFFFFu));
    auto full=enter(edge,4,0x1000,0xFFFFFFFFu); assert(!edge.active.valid); leave(edge,full);
    // Unowned DF944 UI models (ui_scope.hpp): the drawn model and its draw ordinal in the construction,
    // as BK keys zoombox/HUD elements by element and draw index. Disjoint from the menu domain above.
    static_assert(tooie_ui::first_id>=end_id && tooie_ui::stride%(2u*component_stride)==0u,
        "UI model components follow the menu domain, model/sprite aligned");
    {
        tooie_ui::State u;
        const auto a0=tooie_ui::draw(u,0x80400000u,10); const auto a1=tooie_ui::draw(u,0x80400000u,10);
        assert(a0.id==tooie_ui::first_id && a0.cut && a1.id==tooie_ui::first_id+tooie_ui::stride && a1.cut);
        const auto b0=tooie_ui::draw(u,0x80410000u,10);   // another model: its own slot, first sight cuts
        assert(b0.id==tooie_ui::first_id+tooie_ui::ordinals*tooie_ui::stride && b0.cut);
        const auto c0=tooie_ui::draw(u,0x80400000u,11); const auto c1=tooie_ui::draw(u,0x80400000u,11);
        assert(c0.id==a0.id && !c0.cut && c1.id==a1.id && !c1.cut);   // the next construction continues each ordinal
        const auto c2=tooie_ui::draw(u,0x80400000u,11);
        assert(c2.id==a1.id+tooie_ui::stride && c2.cut);              // a third draw is a new element
        const auto d0=tooie_ui::draw(u,0x80400000u,13); const auto d1=tooie_ui::draw(u,0x80400000u,13);
        assert(d0.id==a0.id && d0.cut && d1.id==a1.id && d1.cut);     // a skipped construction restarts history
        for(uint32_t k=2;k<tooie_ui::ordinals;++k) assert(tooie_ui::draw(u,0x80400000u,13).id);
        assert(!tooie_ui::draw(u,0x80400000u,13).id);                  // ordinal bound: that draw stays original
        assert(!tooie_ui::draw(u,0,13).id && !tooie_ui::draw(u,0x80420000u,0).id);
    }
    {
        // Slot admission: a free slot first, else a model that drew in neither this nor the previous construction.
        tooie_ui::State u;
        for(uint32_t m=0;m<tooie_ui::models;++m) assert(tooie_ui::draw(u,0x80500000u+m*0x1000u,1).id);
        assert(!tooie_ui::draw(u,0x80600000u,2).id);                   // every model drew in construction 1
        for(uint32_t m=1;m<tooie_ui::models;++m) assert(tooie_ui::draw(u,0x80500000u+m*0x1000u,3).id);
        const auto r=tooie_ui::draw(u,0x80600000u,3);
        assert(r.id==tooie_ui::first_id && r.cut && u.entries[0].model==0x80600000u); // the stale slot, fresh history
        assert(!tooie_ui::draw(u,0x80500000u,3).id);                   // the displaced model waits; no shared history
        const auto again=tooie_ui::draw(u,0x80600000u,4);
        assert(again.id==r.id && !again.cut);
    }
}
