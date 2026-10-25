// Mechanical o32 ABI only. The typed guest helper owns all command dispatch.
extern "C" void tooie_baseline_cutscene_command(uint8_t*,recomp_context*);
extern "C" void baseline_cutscene_command(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;
    camera_clone_context(clone,*ctx); // Existing checked FR/pointer mapping and16-byte clone frame.
    tooie_baseline_cutscene_command(rdram,&clone);
}
extern "C" void tooie_baseline_cutscene_ticker_claim(uint8_t*,recomp_context*);
extern "C" void tooie_baseline_cutscene_ticker_retire(uint8_t*,recomp_context*);
// chintroticker update before 80614 `lbu v0,0x79(s3)`: s3 is the ticker actor.
extern "C" void baseline_cutscene_ticker_update(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;
    camera_clone_context(clone,*ctx);
    clone.r4=ctx->r19;
    tooie_baseline_cutscene_ticker_claim(rdram,&clone);
}
// chintroticker +0x28 destruction entry: a0 is the actor FF3EC is retiring.
extern "C" void baseline_cutscene_ticker_destroy(uint8_t* rdram,recomp_context* ctx) {
    recomp_context clone;
    camera_clone_context(clone,*ctx);
    tooie_baseline_cutscene_ticker_retire(rdram,&clone);
}
extern "C" void tooie_baseline_presentation_settings(uint8_t*,recomp_context* ctx) {
    ctx->r2=(gpr)(int32_t)baseline::host::render::settings_word();
}
extern "C" void tooie_baseline_presentation_publish(uint8_t*,recomp_context* ctx) {
    baseline::host::render::publish_guest(uint32_t(ctx->r4),uint32_t(ctx->r5));
}
extern "C" void tooie_baseline_quiet_talon_trot(uint8_t*,recomp_context* ctx) {
    ctx->r2=(gpr)(int32_t)baseline::host::requested_quiet_talon_trot();
}
extern "C" void tooie_baseline_trot_toggle(uint8_t*,recomp_context* ctx) {
    ctx->r2=(gpr)(int32_t)baseline::host::requested_talon_trot_toggle();
}
extern "C" void tooie_baseline_music_volume(uint8_t*,recomp_context* ctx) {
    ctx->r2=(gpr)(int32_t)baseline::host::requested_music_volume();
}
