#pragma once
// Declared-hook argument transport only. Guest helper owns all input policy.
extern "C" void tooie_modern_fps_capture(uint8_t*,recomp_context*);
extern "C" void baseline_modern_fps_capture(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;
    camera_clone_context(clone,*ctx);
    clone.r4=ctx->r16; // Original self in s0, before jal delay slot sets a0.
    tooie_modern_fps_capture(rdram,&clone);
}
