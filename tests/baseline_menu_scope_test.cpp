#include "../tools/baseline/menu_scope.hpp"
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
}
