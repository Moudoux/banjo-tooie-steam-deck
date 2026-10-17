#pragma once
#include "recomp.h"

// Authored ignored functions are omitted from generated funcs.h. These exact
// guest ABI declarations type-check their emitted calls and selected providers.
#ifdef __cplusplus
extern "C" {
#endif
#if BASELINE_RENDER_BRIDGE
void baseline_render_construction_begin(uint8_t*, recomp_context*);
#if BASELINE_CAMERA_IDENTITY
#include "pre_background_layout.h"
#include "cpu625_selection_layout.h"
#if TOOIE_BASELINE_HOST
uint32_t tooie_widescreen_latched_settings_word(void);
void baseline_cutscene_command(uint8_t*,recomp_context*);
#endif
#include "npc665_selection_layout.h"
#if TOOIE_NPC665_SELECTED
void baseline_npc665_capture(uint8_t*, recomp_context*);
void baseline_npc665_retire(uint8_t*, recomp_context*);
void baseline_npc665_admit(uint8_t*, recomp_context*);
#endif
#if TOOIE_CPU625_SELECTED
void baseline_cpu625_begin(uint8_t*, recomp_context*);
void baseline_cpu625_end(uint8_t*, recomp_context*);
void baseline_cpu625_set_state(uint8_t*, recomp_context*);
void baseline_cpu625_init(uint8_t*, recomp_context*);
void baseline_cpu625_admit(uint8_t*, recomp_context*);
#endif
#if TOOIE_ACTOR_PALETTE_SELECTED
void baseline_actor_cpu_root_begin(uint8_t*, recomp_context*);
#endif
void baseline_camera_construction_begin(uint8_t*, recomp_context*);
void baseline_camera_construction_end(uint8_t*, recomp_context*);
void baseline_camera_de498_ignore(uint8_t*, recomp_context*);
void baseline_demo_begin(uint8_t*, recomp_context*);
void baseline_demo_record(uint8_t*, recomp_context*);
void baseline_demo_first(uint8_t*, recomp_context*);
void baseline_demo_loop(uint8_t*, recomp_context*);
void baseline_shadow_branch(uint8_t*, recomp_context*);
void baseline_shadow_end(uint8_t*, recomp_context*);
void baseline_shadow_projection(uint8_t*, recomp_context*);
void baseline_shadow_rectangle(uint8_t*, recomp_context*);
void baseline_shadow_local(uint8_t*, recomp_context*);
void baseline_shadow_fallback(uint8_t*, recomp_context*);
void baseline_pre_slot(uint8_t*, recomp_context*);
#if TOOIE_BASELINE_CAMERA_RUNTIME_OBSERVER
void baseline_feather_stable_begin(uint8_t*, recomp_context*);
void baseline_feather_stable_end(uint8_t*, recomp_context*);
void baseline_actor_model_observer(uint8_t*, recomp_context*);
#endif
void baseline_pre_background_begin(uint8_t*, recomp_context*);
void baseline_pre_background_end(uint8_t*, recomp_context*);
void baseline_pre_background_reset(uint8_t*, recomp_context*);
void baseline_shadow_source_local(uint8_t*, recomp_context*);
void baseline_shadow_source_fallback(uint8_t*, recomp_context*);
void baseline_shadow_prepare(uint8_t*, recomp_context*);
void baseline_shadow_fullframe(uint8_t*, recomp_context*);
void baseline_shadow_mode_begin(uint8_t*, recomp_context*);
void baseline_shadow_mode_end(uint8_t*, recomp_context*);
void baseline_shadow_complete(uint8_t*, recomp_context*);

#endif
void func_800EBED4(uint8_t*, recomp_context*);
void func_800DCF48(uint8_t*, recomp_context*);
void func_80101808(uint8_t*, recomp_context*);
void func_800DE448(uint8_t*, recomp_context*);
#if BASELINE_RENDER_SILO
void func_800DD998(uint8_t*, recomp_context*);
void func_8008B10C(uint8_t*, recomp_context*);
void anctrl_setSmoothTransition(uint8_t*, recomp_context*);
#endif
#endif
void boot___osException(uint8_t*, recomp_context*);
void boot_handle_CpU(uint8_t*, recomp_context*);
void boot___osDispatchThread(uint8_t*, recomp_context*);
void boot_osWritebackDCache(uint8_t*, recomp_context*);
void boot_osInvalICache(uint8_t*, recomp_context*);
void boot_osUnmapTLBAll(uint8_t*, recomp_context*);
void boot_osMapTLBRdb(uint8_t*, recomp_context*);
void boot___osGetCause(uint8_t*, recomp_context*);
void boot___osProbeTLB(uint8_t*, recomp_context*);
void func_8001DDC0(uint8_t*, recomp_context*);
void ovl_entrypoint_alternate(uint8_t*, recomp_context*);
void boot_osInitialize(uint8_t*, recomp_context*);
void tooie_core1_osInitialize(uint8_t*, recomp_context*);
void heap_realloc(uint8_t*, recomp_context*);
void func_80800380_chhoneycarrier(uint8_t*, recomp_context*);
void func_808000DC_bsbtrot(uint8_t*, recomp_context*);
#if BASELINE_TROT_TOGGLE
void bs_setState(uint8_t*, recomp_context*);
void bainput_update(uint8_t*, recomp_context*);
void func_80800710_bsbtrot(uint8_t*, recomp_context*);
void func_800F86D8(uint8_t*, recomp_context*);
void tooie_trot_toggle_retire(uint8_t*, recomp_context*);
void tooie_trot_toggle_move(uint8_t*, recomp_context*);
#endif
void boot___osPiRawStartDma(uint8_t*, recomp_context*);
void boot_osPiGetStatus(uint8_t*, recomp_context*);
void baseline_observe_and_publish_core1(uint8_t*, recomp_context*);
void baseline_observe_core1_init_and_stop(uint8_t*, recomp_context*);
void baseline_stop_at_main_entry(uint8_t*, recomp_context*);
void baseline_stop_after_cic_reads(uint8_t*, recomp_context*);
void rom_read_word(uint8_t*, recomp_context*);
void baseline_stop_after_crc_init(uint8_t*, recomp_context*);
void baseline_observe_vi_creation(uint8_t*, recomp_context*);
void baseline_stop_after_vi_handler(uint8_t*, recomp_context*);
void baseline_observe_before_core2_loader(uint8_t*, recomp_context*);
void baseline_observe_and_publish_core2(uint8_t*, recomp_context*);
void baseline_observe_before_scheduler_init(uint8_t*, recomp_context*);
void baseline_stop_before_scheduler_receive(uint8_t*, recomp_context*);
void baseline_reject_main_after_scheduler_start(uint8_t*, recomp_context*);
void baseline_stop_after_scheduler_vi(uint8_t*, recomp_context*);
void baseline_observe_graphics_dp(uint8_t*, recomp_context*);
void osDpGetStatus_recomp(uint8_t*, recomp_context*);
void baseline_observe_joy_stopped(uint8_t*, recomp_context*);
void baseline_observe_joy_receive(uint8_t*, recomp_context*);
void baseline_stop_before_audio(uint8_t*, recomp_context*);
void baseline_hold_after_audio_init(uint8_t*, recomp_context*);
void baseline_completed_pcm(uint8_t*, recomp_context*);
int32_t baseline_overlay_hardware_word(uint32_t);
void baseline_first_overlay_returned(uint8_t*, recomp_context*);
void baseline_stop_before_preworld_unload(uint8_t*, recomp_context*);
void baseline_overlay_boundary_refusal(uint8_t*, recomp_context*, uint32_t);
void baseline_sfx_sweep_service(uint8_t*, recomp_context*);
void baseline_overlay_move_returned(uint8_t*, recomp_context*);
void baseline_alternate_before_free(uint8_t*, recomp_context*);
void baseline_alternate_returned(uint8_t*, recomp_context*);
void baseline_audio_callable(uint8_t*, recomp_context*, uint32_t);
void baseline_bulk_returned(uint8_t*, recomp_context*);
void baseline_resource_preparation(uint8_t*, recomp_context*);
void baseline_world_returned(uint8_t*, recomp_context*);
void baseline_first_sync_enter(uint8_t*, recomp_context*);
void baseline_first_sync_promotion(uint8_t*, recomp_context*);
void baseline_first_sync_returned(uint8_t*, recomp_context*);
void baseline_first_graphics_submit(uint8_t*, recomp_context*);
void baseline_resource_cic_returned(uint8_t*, recomp_context*);
// Missing stock imports established by the actual raw link report.
void __osGetSR_recomp(uint8_t*, recomp_context*);
void __osSetSR_recomp(uint8_t*, recomp_context*);
void __osSiRawReadIo_recomp(uint8_t*, recomp_context*);
void __osSiRawWriteIo_recomp(uint8_t*, recomp_context*);
void osMapTLBRdb_recomp(uint8_t*, recomp_context*);
void __osGetCause_recomp(uint8_t*, recomp_context*);
void __osSiGetAccess_recomp(uint8_t*, recomp_context*);
void __osSiRawStartDma_recomp(uint8_t*, recomp_context*);
void __osSiRelAccess_recomp(uint8_t*, recomp_context*);
void osViGetCurrentLine_recomp(uint8_t*, recomp_context*);
void osPfsInit_recomp(uint8_t*, recomp_context*);
void osPfsInitPak_recomp(uint8_t*, recomp_context*);
// recomp.h itself declares the syscall ABI with its third instruction-VRAM argument.
#ifdef __cplusplus
}
#ifdef BASELINE_VI_CONTROLLER_CYCLE
namespace baseline { void seed_thread_status(uint8_t*, recomp_context*); }
#endif
#ifdef BASELINE_BOOT_PATCH
#include "librecomp/sections.h"
namespace baseline { void register_boot_patch(SectionTableEntry*, size_t); }
#endif
#ifdef BASELINE_IMPORT_recomp_syscall_handler
#include <span>
namespace baseline { void register_overlay_metadata(std::span<SectionTableEntry>, std::span<const int>, size_t); }
#endif
#ifdef BASELINE_BOOT_OBSERVER
namespace baseline { void observe_boot_before(uint8_t*, recomp_context*); }
#endif
#ifdef BASELINE_CIC_OBSERVER
namespace baseline { void scatter_then_observe_boot(uint8_t*, recomp_context*); }
#endif
#endif
