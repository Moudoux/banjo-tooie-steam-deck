// Mechanical o32 ABI only. The typed guest helper owns all command dispatch.
extern "C" void tooie_baseline_cutscene_command(uint8_t*,recomp_context*);
extern "C" void baseline_cutscene_command(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;
    camera_clone_context(clone,*ctx); // Existing checked FR/pointer mapping and16-byte clone frame.
    tooie_baseline_cutscene_command(rdram,&clone);
}
extern "C" void tooie_baseline_presentation_settings(uint8_t*,recomp_context* ctx) {
    ctx->r2=(gpr)(int32_t)baseline::host::render::settings_word();
}
extern "C" void tooie_baseline_presentation_publish(uint8_t*,recomp_context* ctx) {
    baseline::host::render::publish_guest(uint32_t(ctx->r4),uint32_t(ctx->r5));
}
extern "C" void tooie_baseline_quiet_talon_trot(uint8_t*,recomp_context* ctx) {
    ctx->r2=baseline::host::requested_quiet_talon_trot();
}
