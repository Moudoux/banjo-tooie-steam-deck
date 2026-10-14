#pragma once
#include "recomp.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
void tooie_boot_hook(uint8_t* rdram, recomp_context* ctx, unsigned stage);
bool tooie_thread_hooks_active(void);
void tooie_thread_hook(uint8_t* rdram, recomp_context* ctx, unsigned stage);
bool tooie_main_hooks_active(void);
void tooie_main_hook(uint8_t* rdram, recomp_context* ctx, unsigned stage);
void tooie_continuous_poll(uint8_t* rdram, recomp_context* ctx, uint32_t pc);
void tooie_sfx_sweep_service(uint8_t* rdram, recomp_context* ctx);
void tooie_pause_snapshot_capture(void);
void tooie_guest_update_observed(int wait_bypassed);
void tooie_game_delta_observed(uint32_t bits);
void tooie_replay_timing_begin_record(void);
void tooie_replay_timing_record_divisor(uint32_t divisor, int record_available);
uint32_t tooie_replay_timing_scheduler_divisor(uint32_t original, int first_query);
uint32_t tooie_widescreen_latched_settings_word(void);
void tooie_get_latched_widescreen_settings(uint8_t*, recomp_context*);
void tooie_observe_graphics(const uint8_t* rdram, const recomp_context* ctx, uint32_t site);
void tooie_observe_title(const uint8_t* rdram, const recomp_context* ctx, uint32_t site);
void tooie_observe_map_actor_list(const uint8_t* rdram, const recomp_context* ctx, uint32_t site);
void tooie_observe_menu(const uint8_t* rdram, const recomp_context* ctx, uint32_t site);
void tooie_core2_hook(uint8_t* rdram, recomp_context* ctx, unsigned stage);
void heap_realloc(uint8_t* rdram, recomp_context* ctx);
void boot_osInitialize(uint8_t* rdram, recomp_context* ctx);
void tooie_core1_osInitialize(uint8_t* rdram, recomp_context* ctx);
void func_8001A324(uint8_t* rdram, recomp_context* ctx);
void func_8001A3A0(uint8_t* rdram, recomp_context* ctx);
void func_800A7840(uint8_t* rdram, recomp_context* ctx);
void func_800EBED4(uint8_t* rdram, recomp_context* ctx);
void func_8010A79C(uint8_t* rdram, recomp_context* ctx);
void func_800E42F0(uint8_t* rdram, recomp_context* ctx);
void tooie_get_hud_half_width_bits(uint8_t* rdram, recomp_context* ctx);
void tooie_get_construction_model_policy(uint8_t* rdram, recomp_context* ctx);
void tooie_rom_read_word(uint8_t* rdram, recomp_context* ctx);
int32_t tooie_overlay_hardware_word(uint32_t address);
void tooie_overlay_after_relocate(uint8_t*,uint32_t header,int32_t delta);
void tooie_overlay_before_free(uint8_t*,uint32_t header);
void tooie_validate_overlay(uint8_t*,uint32_t header,int32_t delta);
void tooie_overlay_callsite_push(uint32_t original_jal_pc);
void tooie_overlay_callsite_pop(void);
void tooie_get_scene_tuple32(uint8_t*, recomp_context*);
void tooie_get_submitted_render_tuple(uint8_t*, recomp_context*);
void tooie_wrapper_attachment_begin(uint8_t*, recomp_context*);
void tooie_wrapper_attachment_end(uint8_t*, recomp_context*);
void tooie_wrapper_source_span_begin(uint8_t*, recomp_context*);
void tooie_wrapper_source_span_end(uint8_t*, recomp_context*);
#ifdef __cplusplus
}
namespace tooie {
void prepare_core1_diagnostic(uint8_t* rdram, bool idle=false, bool main=false);
void prepare_core1_continuous(uint8_t* rdram);
void restore_core1_boundary();
bool core1_boundary_reached();
void boot_procedure_entry(uint8_t* rdram, recomp_context* ctx);
}
#endif
