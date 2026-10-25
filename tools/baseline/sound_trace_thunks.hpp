#pragma once
#include "sound_trace.hpp"
// Included only by adapters.cpp, after adapters.h supplies the guest ABI types.
// Entry hooks copy argument bits only. They never inspect or write guest memory.
// aux is raw r31, NOT a reliable caller: static recomp calls need not set r31.
extern "C" void baseline_trace_flip_init(uint8_t*, recomp_context* ctx) {
    baseline::sound_trace::record(baseline::sound_trace::Kind::FlipInit, uint32_t(ctx->r4), 0, uint32_t(ctx->r31));
}
extern "C" void baseline_trace_flip_substate(uint8_t*, recomp_context* ctx) {
    baseline::sound_trace::record(baseline::sound_trace::Kind::FlipSubstate, uint32_t(ctx->r4), uint32_t(ctx->r5), uint32_t(ctx->r31));
}
extern "C" void baseline_trace_landing(uint8_t*, recomp_context* ctx) {
    baseline::sound_trace::record(baseline::sound_trace::Kind::Landing, uint32_t(ctx->r4), 0, uint32_t(ctx->r31));
}
extern "C" void baseline_trace_sound_id(uint8_t*, recomp_context* ctx) {
    baseline::sound_trace::record(baseline::sound_trace::Kind::SoundId, uint32_t(ctx->r4) & 0xff, uint32_t(ctx->r5) & 0xffff, uint32_t(ctx->r31));
}
extern "C" void baseline_trace_sound_start(uint8_t*, recomp_context* ctx) {
    baseline::sound_trace::record(baseline::sound_trace::Kind::StartRequest, uint32_t(ctx->r4) & 0xff, 0, uint32_t(ctx->r31));
}
extern "C" void baseline_trace_sound_submit(uint8_t*, recomp_context* ctx) {
    baseline::sound_trace::record(baseline::sound_trace::Kind::Submit, uint32_t(ctx->r4) & 0xff, 0, uint32_t(ctx->r31));
}
extern "C" void baseline_trace_sound_stop(uint8_t*, recomp_context* ctx) {
    baseline::sound_trace::record(baseline::sound_trace::Kind::Stop, uint32_t(ctx->r4) & 0xff, 0, uint32_t(ctx->r31));
}
