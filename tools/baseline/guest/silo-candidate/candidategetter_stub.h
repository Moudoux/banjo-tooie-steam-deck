#pragma once
#include "recomp.h"
#include <stdexcept>
extern "C" {
#ifndef TOOIE_ACTOR_PALETTE_FIXTURE
inline void func_80100368(uint8_t*,recomp_context*) { throw std::runtime_error("unexpected ActorData getter in unbound differential"); }
#endif
inline void func_80104248(uint8_t*,recomp_context*) { throw std::runtime_error("unexpected controller getter in unbound differential"); }
#ifndef TOOIE_PLAYER_STATE_ADDRESS
inline void func_800A89F8(uint8_t*,recomp_context*) { throw std::runtime_error("unexpected view getter in unbound differential"); }
#endif
#ifndef TOOIE_PLAYER_STATE_ADDRESS
inline void func_800EB854(uint8_t*,recomp_context*) { throw std::runtime_error("unexpected phase getter in unbound differential"); }
#endif
}
