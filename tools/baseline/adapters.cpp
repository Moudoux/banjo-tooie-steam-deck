// A-GEN-UNSUPPORTED providers: rejected bodies fail explicitly if ever reached.
// Default S0 does not compile this file. Each symbol is individually selected.
#include "adapters.h"
#ifdef BASELINE_TERMINAL_FENCE
#include "runtime_lifecycle.hpp"
#endif
#include <cstdio>
#include <cstdlib>
#include <string_view>
#ifdef BASELINE_VI_CONTROLLER_CYCLE
#include <atomic>
extern std::atomic_bool exited;
#include <exception>
#include "si_adapter.hpp"
#include "devices.hpp"
#include "platform_support.hpp"
namespace { std::atomic<bool> vi_prefix_checked{false}; std::atomic<unsigned> vi_worker_returns{0}, thread_status_seen{0}; bool core2_pre_admitted = false; unsigned scheduler_initial_returns = 0, joy_stop_count = 0, joy_receive_arrivals = 0; bool scheduler_receive_observed = false; unsigned scheduler_completed_handlers = 0, pfs_absence_calls = 0; }
namespace {
std::atomic<bool> first_sync_entered{false}, tidy_promoted{false}, active_sync{false};
bool world_seen = false;
uint32_t first_sync_sp = 0;
unsigned sync_episode = 0;
bool first_graphics_seen = false;
unsigned graphics_dp_arrivals = 0;
bool sync_wait_observed = false;
uint32_t observed_dp_status(uint8_t* rdram) {
    recomp_context local{};
    osDpGetStatus_recomp(rdram, &local);
    return uint32_t(local.r2);
}
}
#endif
#ifdef BASELINE_CORE1_INIT
#include "ultramodern/ultramodern.hpp"
#endif
#ifdef BASELINE_BOOT_OBSERVER
#include <array>
#include <cfenv>
#endif
#if defined(BASELINE_BOOT_PI) || defined(BASELINE_BOOT_PATCH)
#include "librecomp/game.hpp"
#include "librecomp/addresses.hpp"
#include "librecomp/overlays.hpp"
#endif
#ifdef BASELINE_BOOT_PATCH
#include <algorithm>
#include <vector>
#include <utility>
#include "baseline_boot_patch_bin.h"
namespace baseline_boot_patch_registry {
#include BASELINE_BOOT_PATCH_REGISTRY
}
#endif
#ifdef BASELINE_CORE1_PUBLICATION
#include "guest_digest.hpp"
namespace {
const SectionTableEntry* core1_section = nullptr; const SectionTableEntry* following_section = nullptr;
// Guest-order SHA-256 of the original decompressed core1 and inflated core2 images; no ROM bytes are embedded.
constexpr char core1_reference_sha256[] = "965ccdb935542a113c1d58e12fccdb3ff326f3c02abebdda95cc7657403d7ad0";
constexpr char core2_reference_sha256[] = "6522eefee489b670fb24e594642c5bfb693e639e2c6d82c40dc9fe2eaa12adc7";
}
#endif
#ifdef BASELINE_CIC_OBSERVER
#include "ipl3_boot_state.hpp"
#endif

namespace {
[[noreturn]] void rejected_body(const char* name) {
    std::fprintf(stderr, "baseline rejected-generation provider reached: %s; no service/completion supplied\n", name);
    std::fflush(stderr);
    std::quick_exit(EXIT_FAILURE);
}
[[noreturn]] void missing_import(const char* name) {
    std::fprintf(stderr, "baseline missing-link import reached: %s; no service/completion supplied\n", name);
    std::fflush(stderr);
    std::quick_exit(EXIT_FAILURE);
}
}


#if BASELINE_CAMERA_IDENTITY
#include "cpu625_selection_layout.h"
#include "backpack621_selection_layout.h"
#include "npc665_selection_layout.h"
#endif
#include "camera_thunks.hpp"
#if BASELINE_CAMERA_IDENTITY
#include "cpu625_thunks.hpp"
#if TOOIE_BASELINE_HOST
#include "minimum_render.hpp"
#include "minimum_cutscene_thunks.hpp"
#endif
#include "npc665_thunks.hpp"
#endif
#include "camera_registry.hpp"

#if BASELINE_RENDER_BRIDGE
#if BASELINE_RENDER_SILO
#include "guest/silo-candidate/render_bridge.h"
#else
#include "guest/render_bridge.h"
#endif
#include "render_bridge_layout.h"
#if BASELINE_CAMERA_IDENTITY
#include "guest/camera-candidate/camera_identity.h"
constexpr uint32_t baseline_render_bridge_state_vram = TOOIE_BRIDGE_STATE_VRAM;
#include "camera_identity_layout.h"
static_assert(baseline_render_bridge_state_vram == TOOIE_BRIDGE_STATE_VRAM, "Camera/render bridge layout mismatch");
static_assert(uint64_t(TOOIE_CAMERA_MAIN_VRAM) % 16ull == 0, "Camera MAIN alignment");
static_assert(uint64_t(TOOIE_CAMERA_M_BYTES) % 8ull == 0, "Camera M extent alignment");
static_assert(uint64_t(TOOIE_CAMERA_MAIN_VRAM) >= 0x80801000ull, "Camera MAIN image lower bound");
static_assert(uint64_t(TOOIE_CAMERA_MAIN_VRAM) + 2ull * uint64_t(TOOIE_CAMERA_M_BYTES) <= 0x80801000ull + uint64_t(TOOIE_BRIDGE_IMAGE_BYTES), "Camera MAIN image upper bound");
static_assert(0x80801000ull + uint64_t(TOOIE_BRIDGE_IMAGE_BYTES) <= 0x81000000ull, "Camera image address window");
#endif
namespace { uint32_t bridge_birth_patch_address = 0, bridge_bone_patch_address = 0,
                     bridge_owner_patch_address = 0, bridge_draw_patch_address = 0; }
#if BASELINE_RENDER_SILO
namespace { uint32_t bridge_silo_reset_patch_address = 0, bridge_silo_transition_patch_address = 0; }
#endif
#if BASELINE_PLAYER_BOUNDARY && BASELINE_SILO_ACQUISITION_OBSERVER
namespace { std::atomic<uint32_t> player_observed_idbase{0}; }
extern "C" uint32_t baseline_player_observed_idbase() {
    return player_observed_idbase.load(std::memory_order_acquire);
}
#endif
// BEGIN BASELINE_RENDER_PROVIDER -- extracted unchanged by the focused fixture.
namespace {
struct BridgeProducerReceipt {
    bool checked = false;
#if BASELINE_RENDER_OBSERVER || BASELINE_SILO_ACQUISITION_OBSERVER
    uint64_t begin_calls = 0, last_print_call = 0;
    uint32_t lines = 0, reasons_seen = 0, last_hash = 0;
    bool admission_seen = false;
#endif
} bridge_producer_receipt;
void validate_bridge_image(const char* bytes, size_t size) {
#if BASELINE_CAMERA_IDENTITY && TOOIE_BACKPACK621_SELECTED
    constexpr uint32_t b621_offset=TOOIE_BACKPACK621_STATE-0x80801000u;
    if(!TOOIE_CPU625_SELECTED || TOOIE_BACKPACK621_STATE_BYTES!=612u ||
       b621_offset>size || size-b621_offset<612u ||
       uint64_t(TOOIE_BACKPACK621_STATE)+612u>0x81000000ull ||
       TOOIE_BACKPACK621_RAM!=TOOIE_BACKPACK621_PRIOR_END ||
       TOOIE_BACKPACK621_STATE!=TOOIE_BACKPACK621_RAM+TOOIE_BACKPACK621_SIZE)
        rejected_body("Backpack621 image state extent/overlap");
    for(uint32_t i=0;i<612u;++i) {
        constexpr uint8_t header[8]={0x36,0x32,0x31,0x31,0,0,0,1};
        if(uint8_t(bytes[b621_offset+i])!=(i<8?header[i]:0))
            rejected_body("Backpack621 initialized magic/layout/state");
    }
#endif
#if BASELINE_CAMERA_IDENTITY && TOOIE_MINIMUM_HOST_SELECTED
    constexpr uint32_t offset=TOOIE_MINIMUM_HOST_STATE-0x80801000u;
    if(TOOIE_MINIMUM_HOST_STATE_BYTES!=8u || offset>size || size-offset<8u || TOOIE_MINIMUM_HOST_STATE<TOOIE_PRE_STATE+124u)rejected_body("HOST state extent/overlap");
#if TOOIE_CPU625_SELECTED
    if(TOOIE_MINIMUM_HOST_STATE<TOOIE_CPU625_STATE+464u)rejected_body("HOST/CPU state overlap");
#endif
    for(uint32_t i=0;i<8u;++i)if(uint8_t(bytes[offset+i]))rejected_body("HOST zero initial state");
#endif
#if BASELINE_CAMERA_IDENTITY && TOOIE_NPC665_SELECTED
    constexpr uint32_t npc_offset=TOOIE_NPC665_STATE-0x80801000u;
    constexpr uint32_t prior_end=TOOIE_NPC665_PRIOR_END;
    if(TOOIE_NPC665_STATE_BYTES!=16u || npc_offset>size || size-npc_offset<16u ||
       uint64_t(TOOIE_NPC665_STATE)+16u>0x81000000ull ||
       TOOIE_NPC665_RAM!=prior_end || TOOIE_NPC665_STATE!=TOOIE_NPC665_RAM+TOOIE_NPC665_SIZE)
        rejected_body("NPC665 image sidecar extent/overlap");
    for(uint32_t i=0;i<16u;++i) {
        constexpr uint8_t initial[16]={0,0,0,0,0,0,0,0,0,0,0,0,0x66,0x50,0,0};
        if(uint8_t(bytes[npc_offset+i])!=initial[i])rejected_body("NPC665 initialized sidecar");
    }
#endif
#if BASELINE_CAMERA_IDENTITY && TOOIE_CPU625_SELECTED
    constexpr uint32_t cpu_offset=TOOIE_CPU625_STATE-0x80801000u;
    if(TOOIE_CPU625_STATE_BYTES!=464u || cpu_offset>size || size-cpu_offset<464u ||
       uint64_t(TOOIE_CPU625_STATE)+464u>0x81000000ull ||
       TOOIE_CPU625_STATE<TOOIE_PRE_STATE+124u)
        rejected_body("CPU625 image state extent/overlap");
    for(uint32_t i=0;i<464u;++i) {
        constexpr uint8_t header[8]={0x36,0x32,0x35,0x31,0,0,0,1};
        if(uint8_t(bytes[cpu_offset+i])!=(i<8?header[i]:0))
            rejected_body("CPU625 initialized magic/layout/state");
    }
#endif
    if (size != TOOIE_BRIDGE_IMAGE_BYTES ||
        TOOIE_BRIDGE_STATE_IMAGE_OFFSET > size ||
        size - TOOIE_BRIDGE_STATE_IMAGE_OFFSET < TOOIE_BRIDGE_STATE_BYTES ||
        uint64_t(TOOIE_BRIDGE_STATE_VRAM) + TOOIE_BRIDGE_STATE_BYTES > 0x81000000ull ||
        TOOIE_BRIDGE_STATE_VRAM != 0x80801000u + TOOIE_BRIDGE_STATE_IMAGE_OFFSET)
        rejected_body("bridge image layout/extent");
    for (uint32_t i = 0; i < TOOIE_BRIDGE_STATE_BYTES; ++i) {
        uint8_t expected = 0;
        if (i < 4) expected = uint8_t(TOOIE_BRIDGE_MAGIC >> ((3 - i) * 8));
        if (i >= 4 && i < 8) expected = uint8_t(TOOIE_BRIDGE_LAYOUT >> ((7 - i) * 8));
        if (i == 35) expected = 1; // independent birth cursor, big-endian image
        if (uint8_t(bytes[TOOIE_BRIDGE_STATE_IMAGE_OFFSET + i]) != expected)
            rejected_body("bridge image initialized magic/layout/state");
    }
}
uint32_t bridge_word(uint8_t* rdram, uint32_t offset) {
    return uint32_t(MEM_W(offset, gpr(int32_t(TOOIE_BRIDGE_STATE_VRAM))));
}
#if BASELINE_RENDER_SILO && BASELINE_SILO_ACQUISITION_OBSERVER
// BEGIN BASELINE_PALETTE_SOURCE -- exact helper reused by the focused fixture.
uint64_t palette_first_birth = 0; // Guest-thread-only sampling key; never a lifetime owner.
std::atomic<uint64_t> palette_selected_blocks{0}; // Frozen A low32, B high32.
static_assert(sizeof(palette_first_birth)+sizeof(palette_selected_blocks)==16);
bool palette_observation_mode() {
    static const bool enabled=[] {const char* mode=std::getenv("TOOIE_BASELINE_OBSERVER_MODE");return mode && std::string_view(mode)=="palette";}();
    return enabled;
}
void observe_palette_sources(uint8_t* rdram,uint32_t& rows,uint32_t& refusal_rows) {
    const uint32_t provider_epoch=bridge_word(rdram,8);
#if BASELINE_CAMERA_IDENTITY && TOOIE_BASELINE_CAMERA_RUNTIME_OBSERVER
    if(shadow_witness::forward_mode() && !shadow_witness::forward_ready.load(std::memory_order_acquire)) return;
#endif
    if(provider_epoch<2400u || provider_epoch==UINT32_MAX) return;
    uint64_t selected=palette_selected_blocks.load(std::memory_order_relaxed);
    if(selected>>32) return;
    bool authenticated=false;
    for(uint32_t slot=0;slot<200 && rows<16;++slot) {
        const uint32_t record=4180+slot*72,birth=bridge_word(rdram,record),block=bridge_word(rdram,record+8);
        if(!birth || birth!=bridge_word(rdram,180+slot*4) || !(block&TOOIE_BRIDGE_BLOCK_VALID) ||
           bridge_word(rdram,record+28)!=TOOIE_BRIDGE_PALETTE_ROUTE) continue;
        authenticated=true;
        const uint64_t birth_key=(uint64_t(birth)<<32)|slot;
        const uint32_t id=TOOIE_BRIDGE_ID_BASE+(block&0x7FFFFu)*512u;
        if(selected && (birth_key==palette_first_birth || id==uint32_t(selected))) continue;
        const int written=std::fprintf(stderr,"ACTOR_PALETTE_SOURCE seq=%u slot=%u birth=%u block=%08x model=%08x record_kind=PALETTE calls=%u admissions=%u refusals=%u counters=COARSE\n",
            rows+1,slot,birth,id,bridge_word(rdram,record+20),
            bridge_word(rdram,18652),bridge_word(rdram,18656),bridge_word(rdram,18660));
        if(written<0 || std::fflush(stderr)!=0 || std::ferror(stderr)) rejected_body("palette source observation IO failure");
        ++rows;
        if(!selected) {palette_first_birth=birth_key;selected=id;}
        else selected|=uint64_t(id)<<32;
        palette_selected_blocks.store(selected,std::memory_order_release);
        if(selected>>32) break;
    }
    if(!authenticated && rows<16 && refusal_rows<4 &&
       (bridge_word(rdram,18652) || bridge_word(rdram,18656) || bridge_word(rdram,18660))) {
        const int written=std::fprintf(stderr,"ACTOR_PALETTE_COUNTER seq=%u calls=%u admissions=%u refusals=%u reason=%u counters=COARSE sourceLimit=16\n",
            rows+1,bridge_word(rdram,18652),bridge_word(rdram,18656),bridge_word(rdram,18660),bridge_word(rdram,18664));
        if(written<0 || std::fflush(stderr)!=0 || std::ferror(stderr)) rejected_body("palette source counter IO failure");
        ++rows;++refusal_rows;
    }
}
// END BASELINE_PALETTE_SOURCE
#endif
}
#if BASELINE_PLAYER_BOUNDARY && BASELINE_SILO_ACQUISITION_OBSERVER
extern "C" bool baseline_palette_observation_mode(){return palette_observation_mode();}
extern "C" uint64_t baseline_palette_observed_blocks(){return palette_selected_blocks.load(std::memory_order_acquire);}
#endif
extern "C" void baseline_render_construction_begin(uint8_t* rdram, recomp_context* ctx) {
    (void)ctx; // This before-PC provider never changes a guest register.
    auto& receipt = bridge_producer_receipt;
    if (!receipt.checked) {
        if (bridge_word(rdram, 0) != TOOIE_BRIDGE_MAGIC ||
            bridge_word(rdram, 4) != TOOIE_BRIDGE_LAYOUT)
            rejected_body("bridge resident magic/layout");
#if BASELINE_PLAYER_BOUNDARY
        static_assert(__builtin_offsetof(TooieRenderBridgeState, player) == 18740 &&
            sizeof(TooiePlayerBoundary) == 104 && TOOIE_BRIDGE_STATE_BYTES == 18844);
        MEM_W(18740, gpr(int32_t(TOOIE_BRIDGE_STATE_VRAM))) = 1;
#endif
        receipt.checked = true;
    }
#if BASELINE_RENDER_SILO && BASELINE_SILO_ACQUISITION_OBSERVER
    // Acquisition only: fixed guest-thread scalars, before the unchanged epoch write.
    static_assert(__builtin_offsetof(TooieRenderBridgeState, silo_calls) == 18652 &&
        __builtin_offsetof(TooieRenderBridgeState, silo_admissions) == 18656 &&
        __builtin_offsetof(TooieRenderBridgeState, silo_refusals) == 18660 &&
        __builtin_offsetof(TooieRenderBridgeState, silo_last_reason) == 18664 &&
        __builtin_offsetof(TooieRenderBridgeState, silo_occurrence) == 18668 &&
        __builtin_offsetof(TooieSiloOccurrence, epoch) == 0 &&
        __builtin_offsetof(TooieSiloOccurrence, slot) == 4 &&
        __builtin_offsetof(TooieSiloOccurrence, birth) == 8 &&
        __builtin_offsetof(TooieSiloOccurrence, pair) == 12 &&
        __builtin_offsetof(TooieSiloOccurrence, model) == 16 &&
        __builtin_offsetof(TooieSiloOccurrence, role_mask) == 32 &&
        __builtin_offsetof(TooieRenderBridgeState, epoch) == 8 &&
        TOOIE_BRIDGE_LAYOUT == 3 && TOOIE_BRIDGE_STATE_BYTES == 18844);
    struct SiloAcquisitionReceipt {
        uint64_t begins = 0, summary_begin = 0;
        uint32_t rows = 0, shared_rows = 0, reasons = 0, refusal_rows = 0;
        bool armed = false, admission = false, completion = false;
        bool saturated = false, failed = false;
    };
    static SiloAcquisitionReceipt silo_receipt;
    auto& s = silo_receipt;
    if(palette_observation_mode()) observe_palette_sources(rdram,s.rows,s.refusal_rows);
    else if (!s.failed) {
        const uint32_t calls = bridge_word(rdram, 18652);
        const uint32_t admissions = bridge_word(rdram, 18656);
        const uint32_t refusals = bridge_word(rdram, 18660);
        const uint32_t reason = bridge_word(rdram, 18664);
        const uint32_t record_epoch = bridge_word(rdram, 18668);
        const uint32_t slot = bridge_word(rdram, 18672);
        const uint32_t birth = bridge_word(rdram, 18676);
        const uint32_t pair = bridge_word(rdram, 18680);
        const uint32_t model = bridge_word(rdram, 18684);
        const uint32_t role_mask = bridge_word(rdram, 18700);
        const uint32_t provider_epoch = bridge_word(rdram, 8);
        ++s.begins;
        const bool qualified = slot < 200 && birth >= 1 && birth <= 0xFFFFFFFEu &&
            (pair >> 16) == 0x917u && model != 0;
        const char* check = admissions ? (qualified ? "pass" : "fail") : "na";
        const bool complete = role_mask == 0x800001FFu;
        auto emit = [&](const char* event, bool fatal) {
            const int written = std::fprintf(stderr,
                "SILO_ACQ event=%s seq=%u begin=%llu calls=%u admissions=%u refusals=%u reason=%u record_epoch=%u slot=%u birth=%u pair=%08x model=%08x role_mask=%08x provider_epoch=%u crosscheck=%s completion=%u fatal=%u shared_limit=256 honey_limit=240 silo_limit=16\n",
                event, s.rows + 1, static_cast<unsigned long long>(s.begins), calls,
                admissions, refusals, reason, record_epoch, slot, birth, pair, model,
                role_mask, provider_epoch, check, unsigned(complete), unsigned(fatal));
            const int flushed = std::fflush(stderr);
            if (written < 0 || flushed != 0 || std::ferror(stderr)) {
                s.failed = true;
                std::fprintf(stdout, "SILO_ACQ event=io_failure fatal=1\n");
                std::fflush(stdout);
                return false;
            }
            ++s.rows;
            return true;
        };
        auto shared = [&](const char* event) {
            if (s.shared_rows < 13) {
                if (emit(event, admissions && !qualified)) ++s.shared_rows;
            } else if (!s.saturated && !s.failed) {
                if (emit("saturated", admissions && !qualified)) s.saturated = true;
            }
        };
        if (!s.armed && emit("armed", admissions && !qualified)) s.armed = true;
        if (!s.failed && admissions && !s.admission) {
            if (emit("admission", !qualified)) s.admission = true;
        }
        if (!s.failed && complete && !s.completion) {
            shared("completion"); s.completion = true;
        }
        if (!s.failed && refusals && reason < 32 && !(s.reasons & (1u << reason))) {
            s.reasons |= 1u << reason;
            if (s.refusal_rows < 6) { shared("refusal"); ++s.refusal_rows; }
        }
        if (!s.failed && s.begins - s.summary_begin >= 300) {
            shared("summary"); s.summary_begin = s.begins;
        }
    }
#endif
#if BASELINE_PLAYER_BOUNDARY
#if BASELINE_SILO_ACQUISITION_OBSERVER
    // Fixed source-owned diagnostic scalars, never render-thread guest reads.
    uint32_t player[23];
    for (uint32_t i = 0; i < 23; ++i) player[i] = bridge_word(rdram, 18740 + 4 * i);
    const uint32_t full_id = player[8] + 129u;
    const bool player_selected_62b = player[0] && player[15] && player[22] == 0x62Bu;
    const uint32_t provider_epoch = bridge_word(rdram, 8);
    const bool player_selected = provider_epoch >= 2400u && provider_epoch != UINT32_MAX &&
        player[0] && player[15] && player[8] != 0u && player[7] == provider_epoch &&
        player[22] == 0x62Au && player[21] == 0u;
    player_observed_idbase.store(player_selected ? player[8] : 0u, std::memory_order_release);
    static uint32_t player_rows = 0, previous_admissions = 0, previous_reason = UINT32_MAX, previous_reissues = 0;
    static bool player_seen_62b = false;
    const bool fatal = player[21] != 0;
    const bool player_first_admission = player[15] && !previous_admissions;
    // Reserve two of the existing 32 rows for first body and first observed admitted 62B.
    if (fatal || (player_first_admission && player_rows < 32) ||
        (player_selected_62b && !player_seen_62b && player_rows < 32) ||
        (player_rows < 30 && (!player_rows ||
         (player[16] && player[17] != previous_reason) || player[19] != previous_reissues || receipt.begin_calls % 300 == 0))) {
        const int written = std::fprintf(stderr,
            "PLAYER_ACQ seq=%u producerBegin=%llu providerEpoch=%u enabled=%u owner=%08x component=%08x model=%08x selectorHandle=%08x slot=%u viewPhase=%u stamp=%u idbase=%08x fullId=%08x buffers=%08x base=%08x poolIndex=%u begin=%08x end=%08x calls=%u admissions=%u refusals=%u lastReason=%u changes=%u reissues=%u pendingRefusals=%u tripwires=%u fatal=%u effectiveModelAsset=%08x\n",
            ++player_rows, static_cast<unsigned long long>(receipt.begin_calls), bridge_word(rdram, 8), player[0],
            player[1], player[2], player[3], player[4], player[5], player[6], player[7],
            player[8], full_id, player[9], player[10], player[11], player[12], player[13],
            player[14], player[15], player[16], player[17], player[18], player[19], player[20], player[21], unsigned(fatal), player[22]);
        if (written < 0 || std::fflush(stderr) != 0 || std::ferror(stderr))
            rejected_body("player source observation IO failure");
        else if (player_selected_62b) player_seen_62b = true;
    }
    previous_admissions = player[15]; previous_reason = player[17]; previous_reissues = player[19];
    if (fatal) rejected_body("player boundary tripwire");
#else
    if (bridge_word(rdram, 18824)) rejected_body("player boundary tripwire");
#endif
#endif
    const uint32_t epoch = bridge_word(rdram, 8);
    if (epoch != UINT32_MAX) MEM_W(8, gpr(int32_t(TOOIE_BRIDGE_STATE_VRAM))) = epoch + 1u;
#if BASELINE_RENDER_OBSERVER || BASELINE_SILO_ACQUISITION_OBSERVER
    ++receipt.begin_calls;
    uint32_t reasons = 0, hash = 2166136261u;
    for (uint32_t i = 40; i < 180; i += 4) hash = (hash ^ bridge_word(rdram, i)) * 16777619u;
    for (uint32_t i = 0; i < TOOIE_BRIDGE_REASON_COUNT; ++i)
        if (bridge_word(rdram, 100 + i * 4)) reasons |= 1u << i;
    const bool first_admission = !receipt.admission_seen && bridge_word(rdram, 44) != 0;
    const bool new_reason = (reasons & ~receipt.reasons_seen) != 0;
    const bool periodic_change = hash != receipt.last_hash && receipt.begin_calls - receipt.last_print_call >= 300;
#if BASELINE_RENDER_SILO && BASELINE_SILO_ACQUISITION_OBSERVER
    constexpr uint32_t honey_receipt_limit = 240;
#else
    constexpr uint32_t honey_receipt_limit = 256;
#endif
    if (receipt.lines < honey_receipt_limit && (!receipt.lines || first_admission || new_reason || periodic_change)) {
        std::fprintf(stderr, "baseline bridge-producer epoch=%u begin_calls=%llu calls=%u admissions=%u blocks=%u births=%u birth_saturated=%u E=%u E_high=%u tripwire=%u poison=%u last_id=%08x last_slot=%u last_birth=%u raw8c=%08x raw94=%08x raw7b=%08x raw78=%08x raw70=%08x refusals=",
            bridge_word(rdram, 8), static_cast<unsigned long long>(receipt.begin_calls),
            bridge_word(rdram, 40), bridge_word(rdram, 44), bridge_word(rdram, 36),
            bridge_word(rdram, 48), bridge_word(rdram, 52), bridge_word(rdram, 16),
            bridge_word(rdram, 56), bridge_word(rdram, 60), bridge_word(rdram, 64),
            bridge_word(rdram, 68), bridge_word(rdram, 72), bridge_word(rdram, 76),
            bridge_word(rdram, 80), bridge_word(rdram, 84), bridge_word(rdram, 88),
            bridge_word(rdram, 92), bridge_word(rdram, 96));
        for (uint32_t i = 0; i < TOOIE_BRIDGE_REASON_COUNT; ++i)
            std::fprintf(stderr, "%s%u:%u", i ? "," : "", i, bridge_word(rdram, 100 + i * 4));
        std::fprintf(stderr, " receipt_line=%u\n", ++receipt.lines);
        std::fflush(stderr);
        receipt.admission_seen |= first_admission;
        receipt.reasons_seen |= reasons;
        receipt.last_hash = hash;
        receipt.last_print_call = receipt.begin_calls;
    }
#endif
}
// END BASELINE_RENDER_PROVIDER
#endif

#ifdef BASELINE_CIC_OBSERVER
namespace {
bool scatter_applied = false, main_entry_observed = false;
tooie::boot::ScatterReport scatter{};
// Read actual original-ROM bytes; never use generated fingerprint constants.
uint32_t original_rom_word(size_t offset) {
    const auto rom = recomp::get_rom();
    if (rom.size() != 0x2000000 || offset > rom.size() - 4) rejected_body("scatter original-ROM word bounds");
    return (uint32_t(rom[offset]) << 24) | (uint32_t(rom[offset + 1]) << 16) |
        (uint32_t(rom[offset + 2]) << 8) | uint32_t(rom[offset + 3]);
}
}
namespace baseline {
// Named pre-entry guest-memory adaptation, followed by the existing observer.
void scatter_then_observe_boot(uint8_t* rdram, recomp_context* ctx) {
    if (scatter_applied) rejected_body("IPL3 scatter callback invoked twice");
    const auto rom = recomp::get_rom();
    const bool first_before = uint32_t(MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x802FB1F4)))) == original_rom_word(0x73C);
    const bool second_before = uint32_t(MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x802FE1C0)))) == original_rom_word(0x750);
    scatter = tooie::boot::apply_ipl3_scatter(rom, {rdram, 0x800000});
    bool equal = true;
    for (uint32_t block = 0; block < scatter.blocks; ++block)
        for (uint32_t i = 0; i < scatter.block_bytes; ++i)
            equal &= rdram[(scatter.rdram_destination + block * scatter.rdram_stride + i) ^ 3] == rom[scatter.rom_source + block * scatter.block_bytes + i];
    std::fprintf(stderr, "baseline IPL3 pre_two_words_only=1 first_equal=%d second_equal=%d post_full_equal=%d rom_source=%08x physical_destination=%08x blocks=%u block_bytes=%u stride=%u bytes=%u IPL2_key_assumption=1\n",
        first_before, second_before, equal, scatter.rom_source, scatter.rdram_destination,
        scatter.blocks, scatter.block_bytes, scatter.rdram_stride, scatter.blocks * scatter.block_bytes);
    if (!equal || scatter.rom_source != 0x738 || scatter.rdram_destination != 0x2FB1F0 ||
        scatter.blocks != 24 || scatter.block_bytes != 8 || scatter.rdram_stride != 0xFF0)
        rejected_body("IPL3 actual scatter descriptor/postcondition differs");
    scatter_applied = true;
    observe_boot_before(rdram, ctx);
}
}
#endif

// Raw generator emits the uniform guest-function ABI and two-argument calls;
// authored ignored entries are not declared in its funcs.h.
#ifdef BASELINE_FAIL_boot___osException
extern "C" void boot___osException(uint8_t*, recomp_context*) { rejected_body("boot___osException"); }
#endif
#ifdef BASELINE_FAIL_boot_handle_CpU
extern "C" void boot_handle_CpU(uint8_t*, recomp_context*) { rejected_body("boot_handle_CpU"); }
#endif
#ifdef BASELINE_FAIL_boot___osDispatchThread
extern "C" void boot___osDispatchThread(uint8_t*, recomp_context*) { rejected_body("boot___osDispatchThread"); }
#endif
#ifdef BASELINE_FAIL_boot_osWritebackDCache
extern "C" void boot_osWritebackDCache(uint8_t*, recomp_context*) { rejected_body("boot_osWritebackDCache"); }
#endif
#ifdef BASELINE_FAIL_boot_osInvalICache
extern "C" void boot_osInvalICache(uint8_t*, recomp_context*) { rejected_body("boot_osInvalICache"); }
#endif
#ifdef BASELINE_FAIL_boot_osUnmapTLBAll
extern "C" void boot_osUnmapTLBAll(uint8_t*, recomp_context*) { rejected_body("boot_osUnmapTLBAll"); }
#endif
#ifdef BASELINE_FAIL_boot_osMapTLBRdb
extern "C" void boot_osMapTLBRdb(uint8_t*, recomp_context*) { rejected_body("boot_osMapTLBRdb"); }
#endif
#ifdef BASELINE_FAIL_boot___osGetCause
extern "C" void boot___osGetCause(uint8_t*, recomp_context*) { rejected_body("boot___osGetCause"); }
#endif
#ifdef BASELINE_FAIL_boot___osProbeTLB
extern "C" void boot___osProbeTLB(uint8_t*, recomp_context*) { rejected_body("boot___osProbeTLB"); }
#endif
#ifdef BASELINE_FAIL_func_8001DDC0
extern "C" void func_8001DDC0(uint8_t*, recomp_context*) { rejected_body("func_8001DDC0"); }
#endif
#ifdef BASELINE_FAIL_ovl_entrypoint_alternate
extern "C" void ovl_entrypoint_alternate(uint8_t*, recomp_context*) { rejected_body("ovl_entrypoint_alternate"); }
#endif
#ifdef BASELINE_FAIL_boot_osInitialize
// Safe first-failure child: stop before original initialization touches PIF/MMIO.
extern "C" void boot_osInitialize(uint8_t*, recomp_context*) { rejected_body("boot_osInitialize (pre-PIF admission boundary)"); }
#endif
#ifdef BASELINE_FAIL_tooie_core1_osInitialize
extern "C" void tooie_core1_osInitialize(uint8_t*, recomp_context*) { rejected_body("tooie_core1_osInitialize (preworker boundary)"); }
#endif
#ifdef BASELINE_BOOT_PI
namespace { bool boot_pi_busy = false; }
#ifdef BASELINE_BOOT_OBSERVER
namespace {
struct BootBefore {
    uint64_t clock;
    uint32_t status, reset, tv;
    std::array<uint8_t, 16> preamble;
    bool captured = false;
    bool observed = false;
} boot_before;
#ifdef BASELINE_CORE1_INIT
BootBefore core1_before;
#endif
uint64_t boot_clock(uint8_t* rdram) {
    return (uint64_t(uint32_t(MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x80004250))))) << 32) |
        uint32_t(MEM_W(4, static_cast<gpr>(static_cast<int32_t>(0x80004250))));
}
// Read-only postcondition at the first PI request, before any ROM transfer.
void observe_boot_after(uint8_t* rdram, recomp_context* ctx) {
    if (!boot_before.captured) rejected_body("boot observation missing pre-entry snapshot");
    bool vectors = true, nmi = true;
    for (uint32_t vector = 0x80000000; vector <= 0x80000180; vector += 0x80)
        for (uint32_t i = 0; i < 16; ++i)
            vectors &= uint8_t(MEM_B(i, static_cast<gpr>(static_cast<int32_t>(vector)))) == boot_before.preamble[i];
    if (boot_before.reset == 0)
        for (uint32_t i = 0; i < 64; ++i) nmi &= MEM_B(i, static_cast<gpr>(static_cast<int32_t>(0x8000031C))) == 0;
    const uint32_t expected_vi = boot_before.tv == 0 ? 0x02F5B2D2 : boot_before.tv == 2 ? 0x02E6025C : 0x02E6D354;
    const bool finalrom = MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x80006F50))) == 1;
    // A-BOOT-CU1-OMIT preserves SR; it does not claim a console CU1 enable.
    const bool status = ctx->status_reg == boot_before.status;
    const bool clock = boot_clock(rdram) == boot_before.clock * uint64_t(3) / uint64_t(4);
    const bool vi = uint32_t(MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x80004258)))) == expected_vi;
    const bool domains = MEM_B(0, static_cast<gpr>(static_cast<int32_t>(0x80006F64))) == 7 && MEM_B(0, static_cast<gpr>(static_cast<int32_t>(0x80006FDC))) == 7;
    const bool nearest = std::fegetround() == FE_TONEAREST;
    std::fprintf(stderr, "baseline O-BOOT post finalrom=%d status=%d vectors=%d clock=%d nmi=%d vi=%d domains=%d nearest=%d clock_before=%016llx clock_after=%016llx sr_before=%08x sr_after=%08x CU1_omitted=1 full_FCSR_unmodeled=1\n",
        finalrom, status, vectors, clock, nmi, vi, domains, nearest,
        static_cast<unsigned long long>(boot_before.clock), static_cast<unsigned long long>(boot_clock(rdram)), boot_before.status, ctx->status_reg);
    if (!(finalrom && status && vectors && clock && nmi && vi && domains && nearest))
        rejected_body("guest-C boot initializer observed postcondition failure");
    boot_before.observed = true;
}
}
namespace baseline {
void observe_boot_before(uint8_t* rdram, recomp_context* ctx) {
    boot_before.clock = boot_clock(rdram); boot_before.status = ctx->status_reg;
    boot_before.reset = MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x8000030C))); boot_before.tv = MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x80000300)));
    for (uint32_t i = 0; i < 16; ++i) boot_before.preamble[i] = MEM_B(i, static_cast<gpr>(static_cast<int32_t>(0x80001C90)));
    boot_before.captured = true;
    std::fprintf(stderr, "baseline O-BOOT pre clock=%016llx sr=%08x reset=%u tv=%u\n",
        static_cast<unsigned long long>(boot_before.clock), boot_before.status, boot_before.reset, boot_before.tv);
}
}
#endif
// Preworker synchronous original-ROM transport through the supported runtime API.
extern "C" void boot___osPiRawStartDma(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t direction = ctx->r4, offset = ctx->r5, destination = ctx->r6, length = ctx->r7;
    const auto rom = recomp::get_rom();
    const uint32_t segment = destination & 0xE0000000, physical = destination & 0x1FFFFFFF;
    if (direction != 0 || !length || (length & 1) || (offset & 1) || (destination & 7) ||
        (segment != 0x80000000 && segment != 0xA0000000) ||
        uint64_t(offset) + length > rom.size() || uint64_t(physical) + length > 0x800000 || boot_pi_busy)
        rejected_body("boot PI request validation");
#ifdef BASELINE_BOOT_OBSERVER
    if (!boot_before.observed) observe_boot_after(rdram, ctx);
#endif
    boot_pi_busy = true;
    const gpr canonical = gpr(int32_t(0x80000000 | physical));
    std::fprintf(stderr, "baseline boot PI request rom_offset=%08x destination=%08x bytes=%u\n", offset, destination, length);
    recomp::do_rom_read(rdram, canonical, recomp::rom_base + offset, length);
    for (uint32_t i = 0; i < length; ++i) {
        if (uint8_t(MEM_B(i, canonical)) != rom[offset + i]) rejected_body("boot PI copied bytes differ from original ROM");
    }
    boot_pi_busy = false;
    ctx->r2 = 0;
    std::fprintf(stderr, "baseline boot PI completed original_bytes_equal=1 bytes=%u\n", length);
}
extern "C" void boot_osPiGetStatus(uint8_t*, recomp_context* ctx) { ctx->r2 = boot_pi_busy ? 1 : 0; }
#endif

#ifdef BASELINE_BOOT_PATCH
namespace baseline {
// Restore one omitted original address entry and register the proved patch route.
void register_boot_patch(SectionTableEntry* base, size_t count) {
    if (!count || base[0].rom_addr != 0x1000 || base[0].ram_addr != 0x80000400)
        rejected_body("boot section identity changed");
#ifdef BASELINE_CORE1_PUBLICATION
    if (count < 3 || base[1].rom_addr != 0x01E29B60 || base[1].ram_addr != 0x80012030 ||
        base[1].size != 0x31350 || base[1].index != 1 || base[1].num_relocs || base[1].relocs ||
        !base[1].num_funcs || base[1].funcs[0].offset != 0 || base[2].rom_addr != 0x01E5AEB0 ||
        std::any_of(base[1].funcs, base[1].funcs + base[1].num_funcs,
            [](const auto& entry) { return entry.offset == 0x8002D7B0 - 0x80012030; }))
        rejected_body("core1 publication registry/omitted initializer identity changed");
    core1_section = &base[1]; following_section = &base[2];
#endif
    static std::vector<FuncEntry> entries(base[0].funcs, base[0].funcs + base[0].num_funcs);
    constexpr uint32_t offset = 0x800016F0 - 0x80000400;
    if (std::any_of(entries.begin(), entries.end(), [](const auto& entry) { return entry.offset == offset; }))
        rejected_body("original boot initializer was not excluded");
    entries.push_back({.func = boot_osInitialize, .offset = offset, .rom_size = 0x2A8});
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.offset < b.offset; });
    base[0].funcs = entries.data(); base[0].num_funcs = entries.size();
#ifdef BASELINE_CORE1_INIT
#if BASELINE_CAMERA_IDENTITY
    camera_validate_patch_table();
#elif BASELINE_RENDER_SILO
    // Exact measured three-section carrier; the default replacement-only registry stays strict.
    if (ARRLEN(baseline_boot_patch_registry::section_table) != 3)
        rejected_body("selected Silo helper registry section count changed");
    const auto& replacement_section = baseline_boot_patch_registry::section_table[0];
    const auto& helper_section = baseline_boot_patch_registry::section_table[1];
    const auto& player_section = baseline_boot_patch_registry::section_table[2];
    if (replacement_section.rom_addr != 0 || replacement_section.ram_addr != 0x80801000 ||
        replacement_section.size != 0x24A0 || replacement_section.index != 1 ||
        replacement_section.num_funcs != 13 || !replacement_section.funcs ||
        helper_section.rom_addr != 0x24A0 || helper_section.ram_addr != 0x808034A0 ||
        helper_section.size != 0x2DF4 || helper_section.index != 2 ||
        helper_section.num_funcs != 1 || !helper_section.funcs ||
        helper_section.funcs[0].func != baseline_boot_patch_registry::tooie_silo_preflight ||
        helper_section.funcs[0].offset != 0 || helper_section.funcs[0].rom_size != 0x2DF4 ||
        helper_section.num_relocs != 2 || !helper_section.relocs ||
        uint64_t(helper_section.rom_addr) + helper_section.size != player_section.rom_addr ||
        player_section.rom_addr != 0x5294 || player_section.ram_addr != 0x80806294 ||
        player_section.size != 0x15B0 || player_section.index != 3 || player_section.num_funcs != 6 ||
        !player_section.funcs || player_section.num_relocs != 3 || !player_section.relocs ||
        player_section.funcs[0].func != baseline_boot_patch_registry::tooie_player_begin ||
        player_section.funcs[0].offset != 0 || player_section.funcs[0].rom_size != 0xC90 ||
        player_section.funcs[1].func != baseline_boot_patch_registry::tooie_player_bone_push ||
        player_section.funcs[1].offset != 0xC90 || player_section.funcs[1].rom_size != 0x268 ||
        player_section.funcs[2].func != baseline_boot_patch_registry::tooie_player_bone_pop ||
        player_section.funcs[2].offset != 0xEF8 || player_section.funcs[2].rom_size != 0x15C ||
        player_section.funcs[3].func != baseline_boot_patch_registry::tooie_player_leaf_begin ||
        player_section.funcs[3].offset != 0x1054 || player_section.funcs[3].rom_size != 0x274 ||
        player_section.funcs[4].func != baseline_boot_patch_registry::tooie_player_leaf_end ||
        player_section.funcs[4].offset != 0x12C8 || player_section.funcs[4].rom_size != 0x154 ||
        player_section.funcs[5].func != baseline_boot_patch_registry::tooie_player_end ||
        player_section.funcs[5].offset != 0x141C || player_section.funcs[5].rom_size != 0x194 ||
        uint64_t(player_section.rom_addr) + player_section.size != TOOIE_BRIDGE_STATE_IMAGE_OFFSET ||
        uint64_t(player_section.ram_addr) + player_section.size != TOOIE_BRIDGE_STATE_VRAM ||
        std::any_of(replacement_section.funcs, replacement_section.funcs + replacement_section.num_funcs,
            [](const auto& entry) { return entry.func == baseline_boot_patch_registry::tooie_silo_preflight; }))
        rejected_body("selected Silo helper registry identity/extent changed");
#else
    if (ARRLEN(baseline_boot_patch_registry::section_table) != 1)
        rejected_body("selected guest initializer registry section count changed");
#endif
    if (
        baseline_boot_patch_registry::section_table[0].ram_addr != 0x80801000 ||
#if BASELINE_CAMERA_IDENTITY && TOOIE_ACTOR_PALETTE_SELECTED
        baseline_boot_patch_registry::section_table[0].num_funcs != 39+TOOIE_MINIMUM_HOST_SELECTED ||
#else
        baseline_boot_patch_registry::section_table[0].num_funcs != (BASELINE_CAMERA_IDENTITY ? 38 : BASELINE_RENDER_BRIDGE ? (BASELINE_RENDER_SILO ? 13 : 8) : (BASELINE_HONEY_PATCH ? 4 : (BASELINE_HEAP_PATCH ? 3 : 2))) ||
#endif
        baseline_boot_patch_registry::section_table[0].funcs[0].func != boot_osInitialize ||
        baseline_boot_patch_registry::section_table[0].funcs[0].offset != 0 ||
        baseline_boot_patch_registry::section_table[0].funcs[1].func != tooie_core1_osInitialize ||
        baseline_boot_patch_registry::section_table[0].funcs[1].offset < 1180)
        rejected_body("selected guest initializer registry changed");
    static std::vector<FuncEntry> core1_entries(base[1].funcs, base[1].funcs + base[1].num_funcs);
#ifdef BASELINE_ROM_PIO_BRIDGE
    if (std::any_of(core1_entries.begin(), core1_entries.end(), [](const auto& entry) { return entry.offset == 0xC1E0; }))
        rejected_body("original ROM-PIO body/address was not excluded");
    core1_entries.push_back({.func = rom_read_word, .offset = 0xC1E0, .rom_size = 0x8C});
#endif
#if BASELINE_HEAP_PATCH
    if (baseline_boot_patch_registry::section_table[0].funcs[2].func != heap_realloc ||
        baseline_boot_patch_registry::section_table[0].funcs[2].offset < 2360 ||
        std::any_of(core1_entries.begin(), core1_entries.end(), [](const auto& entry) { return entry.offset == 0x91D8; }))
        rejected_body("selected heap provider/original address was not excluded");
    core1_entries.push_back({.func = heap_realloc, .offset = 0x91D8, .rom_size = 0x180});
#endif
#if BASELINE_HONEY_PATCH || BASELINE_RENDER_BRIDGE
    const auto& patch_section = baseline_boot_patch_registry::section_table[0];
    const auto* honey_entry = std::find_if(patch_section.funcs, patch_section.funcs + patch_section.num_funcs,
        [](const auto& entry) { return entry.func == func_80800380_chhoneycarrier; });
    if (honey_entry == patch_section.funcs + patch_section.num_funcs)
        rejected_body("selected Honey symbol missing");
    const auto& honey = *honey_entry;
    if (honey.func != func_80800380_chhoneycarrier || honey.offset < 2760 ||
        uint64_t(honey.offset) + honey.rom_size > baseline_boot_patch_bin_size)
        rejected_body("selected Honey patch provider/extent changed");
#endif
#if BASELINE_RENDER_BRIDGE
    validate_bridge_image(baseline_boot_patch_bin, baseline_boot_patch_bin_size);
    const auto* birth_entry = std::find_if(patch_section.funcs, patch_section.funcs + patch_section.num_funcs,
        [](const auto& entry) { return entry.func == func_800EBED4; });
    if (birth_entry == patch_section.funcs + patch_section.num_funcs || birth_entry->offset < 2760 ||
        uint64_t(birth_entry->offset) + birth_entry->rom_size > TOOIE_BRIDGE_STATE_IMAGE_OFFSET || count <= 2)
        rejected_body("selected EBED4 provider/extent");
    const auto* bone_entry = std::find_if(patch_section.funcs, patch_section.funcs + patch_section.num_funcs,
        [](const auto& entry) { return entry.func == func_800DCF48; });
    const auto* owner_entry = std::find_if(patch_section.funcs, patch_section.funcs + patch_section.num_funcs,
        [](const auto& entry) { return entry.func == func_80101808; });
    const auto* draw_entry = std::find_if(patch_section.funcs, patch_section.funcs + patch_section.num_funcs,
        [](const auto& entry) { return entry.func == func_800DE448; });
    if (bone_entry == patch_section.funcs + patch_section.num_funcs || bone_entry->offset < 2760 ||
        uint64_t(bone_entry->offset) + bone_entry->rom_size > TOOIE_BRIDGE_STATE_IMAGE_OFFSET ||
        owner_entry == patch_section.funcs + patch_section.num_funcs || owner_entry->offset < 2760 ||
        uint64_t(owner_entry->offset) + owner_entry->rom_size > TOOIE_BRIDGE_STATE_IMAGE_OFFSET ||
        draw_entry == patch_section.funcs + patch_section.num_funcs || draw_entry->offset < 2760 ||
        uint64_t(draw_entry->offset) + draw_entry->rom_size > TOOIE_BRIDGE_STATE_IMAGE_OFFSET ||
        std::count_if(patch_section.funcs, patch_section.funcs + patch_section.num_funcs,
            [](const auto& entry) { return entry.func == func_800DCF48 || entry.func == func_800EBED4 ||
                                          entry.func == func_80101808 || entry.func == func_800DE448; }) != 4)
        rejected_body("selected core2 patch providers/extent/uniqueness");
    bridge_birth_patch_address = patch_section.ram_addr + birth_entry->offset;
    bridge_bone_patch_address = patch_section.ram_addr + bone_entry->offset;
    bridge_owner_patch_address = patch_section.ram_addr + owner_entry->offset;
    bridge_draw_patch_address = patch_section.ram_addr + draw_entry->offset;
#if BASELINE_RENDER_SILO
    const auto* reset_entry = std::find_if(patch_section.funcs, patch_section.funcs + patch_section.num_funcs,
        [](const auto& entry) { return entry.func == func_8008B10C; });
    const auto* transition_entry = std::find_if(patch_section.funcs, patch_section.funcs + patch_section.num_funcs,
        [](const auto& entry) { return entry.func == anctrl_setSmoothTransition; });
    if (reset_entry == patch_section.funcs + patch_section.num_funcs || transition_entry == patch_section.funcs + patch_section.num_funcs ||
        reset_entry->offset < 2760 || transition_entry->offset < 2760 ||
        uint64_t(reset_entry->offset) + reset_entry->rom_size > TOOIE_BRIDGE_STATE_IMAGE_OFFSET ||
        uint64_t(transition_entry->offset) + transition_entry->rom_size > TOOIE_BRIDGE_STATE_IMAGE_OFFSET ||
        std::count_if(patch_section.funcs, patch_section.funcs + patch_section.num_funcs,
            [](const auto& entry) { return entry.func == func_8008B10C || entry.func == anctrl_setSmoothTransition; }) != 2)
        rejected_body("held Silo setter providers/extent/uniqueness");
    const auto* leaf_entry = std::find_if(patch_section.funcs, patch_section.funcs + patch_section.num_funcs,
        [](const auto& entry) { return entry.func == func_800DD998; });
    if (leaf_entry == patch_section.funcs + patch_section.num_funcs || leaf_entry->offset < 2760 ||
        uint64_t(leaf_entry->offset) + leaf_entry->rom_size > TOOIE_BRIDGE_STATE_IMAGE_OFFSET ||
        std::count_if(patch_section.funcs, patch_section.funcs + patch_section.num_funcs,
            [](const auto& entry) { return entry.func == func_800DD998; }) != 1)
        rejected_body("selected DD998 provider/extent/uniqueness");
    bridge_silo_reset_patch_address = patch_section.ram_addr + reset_entry->offset;
    bridge_silo_transition_patch_address = patch_section.ram_addr + transition_entry->offset;
#endif
    static std::vector<FuncEntry> core2_entries(base[2].funcs, base[2].funcs + base[2].num_funcs);
    if (std::any_of(core2_entries.begin(), core2_entries.end(), [](const auto& entry) {
            return entry.offset == 0x6A914 || entry.offset == 0x5B988 || entry.offset == 0x80248 || entry.offset == 0x5CE88; }))
        rejected_body("original core2 patch body/address was not excluded");
    core2_entries.push_back({.func = func_800EBED4, .offset = 0x6A914, .rom_size = 0xB8});
    core2_entries.push_back({.func = func_800DCF48, .offset = 0x5B988, .rom_size = 0xDC});
    core2_entries.push_back({.func = func_80101808, .offset = 0x80248, .rom_size = 0x68});
    core2_entries.push_back({.func = func_800DE448, .offset = 0x5CE88, .rom_size = 0x50});
#if BASELINE_RENDER_SILO
    if (std::any_of(core2_entries.begin(), core2_entries.end(), [](const auto& entry) {
            return entry.offset == 0x9B4C || entry.offset == 0x9BE0 || entry.offset == 0x5C3D8; }))
        rejected_body("original Silo setter addresses were not excluded");
    core2_entries.push_back({.func = func_800DD998, .offset = 0x5C3D8, .rom_size = 0x1A0});
#if BASELINE_CAMERA_IDENTITY && TOOIE_ACTOR_PALETTE_SELECTED
    if (std::any_of(core2_entries.begin(), core2_entries.end(), [](const auto& entry) {
            return entry.offset == 0x5BA64; }))
        rejected_body("original DD024 patch address was not excluded");
    core2_entries.push_back({.func = baseline_boot_patch_registry::func_800DD024, .offset = 0x5BA64, .rom_size = 0x58});
#endif
    core2_entries.push_back({.func = func_8008B10C, .offset = 0x9B4C, .rom_size = 0x28});
    core2_entries.push_back({.func = anctrl_setSmoothTransition, .offset = 0x9BE0, .rom_size = 0x1C});
#if !BASELINE_CAMERA_IDENTITY
    if (std::any_of(core2_entries.begin(), core2_entries.end(), [](const auto& entry) {
            return entry.offset == 0x112D8 || entry.offset == 0x722C0; }))
        rejected_body("original player lifecycle providers were not excluded");
    core2_entries.push_back({.func = func_80092898, .offset = 0x112D8, .rom_size = 0x184});
    core2_entries.push_back({.func = func_800F3880, .offset = 0x722C0, .rom_size = 0x28});
#endif
#endif
    std::sort(core2_entries.begin(), core2_entries.end(), [](const auto& a, const auto& b) { return a.offset < b.offset; });
    if (std::adjacent_find(core2_entries.begin(), core2_entries.end(),
            [](const auto& a, const auto& b) { return a.offset == b.offset; }) != core2_entries.end())
        rejected_body("duplicate core2 original address offsets");
    base[2].funcs = core2_entries.data(); base[2].num_funcs = core2_entries.size();
#endif
    core1_entries.push_back({.func = tooie_core1_osInitialize, .offset = 0x8002D7B0 - 0x80012030, .rom_size = 0x2A8});
    std::sort(core1_entries.begin(), core1_entries.end(), [](const auto& a, const auto& b) { return a.offset < b.offset; });
    base[1].funcs = core1_entries.data(); base[1].num_funcs = core1_entries.size();
#endif
#if BASELINE_CAMERA_IDENTITY
    camera_register_aliases(base,count);
#endif
    recomp::overlays::register_patches(baseline_boot_patch_bin, baseline_boot_patch_bin_size,
        baseline_boot_patch_registry::section_table, ARRLEN(baseline_boot_patch_registry::section_table));
    std::fprintf(stderr, "baseline guest-C boot initializer registered at original 800016F0 and patch 80801000\n");
}
}
#endif

#ifdef BASELINE_CORE1_PUBLICATION
// Observe original decompression before publishing only its existing code range.
extern "C" void baseline_observe_and_publish_core1(uint8_t* rdram, recomp_context* ctx) {
    static bool published = false;
    if (published || !core1_section || !following_section || core1_section->rom_addr != 0x01E29B60 ||
        core1_section->ram_addr != 0x80012030 || core1_section->size != 0x31350 ||
        core1_section->num_relocs || following_section->rom_addr != 0x01E5AEB0)
        rejected_body("core1 publication admission changed");
    const std::string actual = baseline::guest_range_sha256(rdram, 0x80012030, 0x31350);
    if (actual != core1_reference_sha256) {
        std::fprintf(stderr, "baseline O-CORE1 equal=0 bytes=201552 sha256=%s expected=%s\n", actual.c_str(), core1_reference_sha256);
        rejected_body("core1 original decompression digest mismatch; no publication");
    }
    std::fprintf(stderr, "baseline O-CORE1 equal=1 bytes=201552 reference_sha256=965ccdb935542a113c1d58e12fccdb3ff326f3c02abebdda95cc7657403d7ad0\n");
#ifdef BASELINE_CORE1_INIT
    core1_before.clock = (uint64_t(uint32_t(MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x80041390))))) << 32) |
        uint32_t(MEM_W(4, static_cast<gpr>(static_cast<int32_t>(0x80041390))));
    core1_before.status = ctx->status_reg;
    core1_before.reset = MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x8000030C)));
    core1_before.tv = MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x80000300)));
    for (uint32_t i = 0; i < 16; ++i) core1_before.preamble[i] = MEM_BU(i, static_cast<gpr>(static_cast<int32_t>(0x80031FE0)));
    core1_before.captured = true;
#endif
    load_overlays(0x01E29B60, static_cast<int32_t>(0x80012030), 0x31350);
#if BASELINE_CAMERA_IDENTITY
    camera_validate_published_section(*core1_section,0x80012030);
#endif
    published = true;
    std::fprintf(stderr, "baseline core1 published rom=01e29b60 ram=80012030 size=00031350; next admission is condition-selected\n");
}
#endif

#ifdef BASELINE_CORE1_INIT
// Prefix observers retain their inherited hook names; none is a terminal arm.
// Read-only postcondition before80012064 admits original idle creation.
extern "C" void baseline_observe_core1_init_and_stop(uint8_t* rdram, recomp_context* ctx) {
    if (!core1_before.captured || core1_before.observed) rejected_body("core1 initializer snapshot/stop admission");
    bool vectors = true, nmi = true;
    for (uint32_t vector = 0x80000000; vector <= 0x80000180; vector += 0x80)
        for (uint32_t i = 0; i < 16; ++i)
            vectors &= MEM_BU(i, static_cast<gpr>(static_cast<int32_t>(vector))) == core1_before.preamble[i];
    if (core1_before.reset == 0)
        for (uint32_t i = 0; i < 64; ++i) nmi &= MEM_B(i, static_cast<gpr>(static_cast<int32_t>(0x8000031C))) == 0;
    const uint64_t after = (uint64_t(uint32_t(MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x80041390))))) << 32) |
        uint32_t(MEM_W(4, static_cast<gpr>(static_cast<int32_t>(0x80041390))));
    const uint32_t expected_vi = core1_before.tv == 0 ? 0x02F5B2D2 : core1_before.tv == 2 ? 0x02E6025C : 0x02E6D354;
    const bool finalrom = MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x8007EAD0))) == 1;
    const bool status = ctx->status_reg == core1_before.status;
    const bool clock = after == core1_before.clock * uint64_t(3) / uint64_t(4);
    const bool vi = uint32_t(MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x80041398)))) == expected_vi;
    const bool domains = MEM_B(0, static_cast<gpr>(static_cast<int32_t>(0x8007EAE4))) == 7 && MEM_B(0, static_cast<gpr>(static_cast<int32_t>(0x8007EB5C))) == 7;
    const bool nearest = std::fegetround() == FE_TONEAREST;
    std::fprintf(stderr, "baseline O-CORE1-INIT post finalrom=%d status=%d vectors=%d clock=%d nmi=%d vi=%d domains=%d nearest=%d clock_before=%016llx clock_after=%016llx CU1_omitted=1 full_FCSR_unmodeled=1\n",
        finalrom, status, vectors, clock, nmi, vi, domains, nearest,
        static_cast<unsigned long long>(core1_before.clock), static_cast<unsigned long long>(after));
    if (!(finalrom && status && vectors && clock && nmi && vi && domains && nearest)) rejected_body("core1 guest initializer observed postcondition failure");
    core1_before.observed = true;
    std::fprintf(stderr, "baseline checked core1 initializer returned; admitting original workers and bounded ROM-PIO; post-core2-prefix arm selected\n");
    std::fflush(stderr);
}
#endif

#ifdef BASELINE_CIC_OBSERVER
// First original main instruction, before its stack stores or startup services.
extern "C" void baseline_stop_at_main_entry(uint8_t* rdram, recomp_context* ctx) {
    static bool entered = false;
    const auto thread = ultramodern::this_thread();
    if (!core1_before.observed || entered || uint32_t(thread) != 0x80045788 || osGetThreadId(rdram, thread) != 6)
        rejected_body("main-entry stop missing observer or unexpected current thread");
    entered = true;
    if (!scatter_applied) rejected_body("main entry without original-ROM scatter");
    main_entry_observed = true;
    std::fprintf(stderr, "baseline original main entry reached PC=800124ec thread=%08x id=6 SP=%08x arg=%08x; admitting checked CIC reads and bounded ROM-PIO; post-core2-prefix arm selected\n",
        uint32_t(thread), uint32_t(ctx->r29), uint32_t(ctx->r4));
    std::fflush(stderr);
}
#endif

#ifdef BASELINE_CIC_OBSERVER
// Both original loads ran; SP+18 delay-slot store/comparisons remain unexecuted.
extern "C" void baseline_stop_after_cic_reads(uint8_t* rdram, recomp_context* ctx) {
    const auto thread = ultramodern::this_thread();
    if (!scatter_applied || !main_entry_observed || uint32_t(thread) != 0x8007D8B0 ||
        osGetThreadId(rdram, thread) != 3 || uint32_t(ctx->r29) != 0x8007DB38)
        rejected_body("CIC read observation thread/stack admission");
    const uint32_t first = original_rom_word(0x73C), second = original_rom_word(0x750);
    const bool aliases = uint32_t(ctx->r14) == 0x80300000 && uint32_t(ctx->r24) == 0x80300000;
    const bool first_equal = uint32_t(ctx->r15) == first, second_equal = uint32_t(ctx->r25) == second;
    const bool stack_equal = uint32_t(MEM_W(0x1C, ctx->r29)) == first;
    constexpr const char* disposition = "continuation selected to original comparison/queue and bounded ROM-PIO; post-core2-prefix arm selected; workers/joins unaccepted";
    std::fprintf(stderr, "baseline CIC read boundary PC=8001de8c id=3 aliases=%d first_loaded_equal=%d second_loaded_equal=%d first_stack_equal=%d second_stack_store_not_executed=1 comparison_not_executed=1; %s\n",
        aliases, first_equal, second_equal, stack_equal, disposition);
    std::fflush(stderr);
    if (!(aliases && first_equal && second_equal && stack_equal)) rejected_body("CIC actual ROM-source read/stack comparison failed");
}
#endif

#ifdef BASELINE_ROM_PIO_BRIDGE
namespace { uint32_t pio_copies = 0, pio_offset = 0, pio_destination = 0; }
// Preserve original cart OR semantics; the public service synchronously copies four bytes.
extern "C" void rom_read_word(uint8_t* rdram, recomp_context* ctx) {
    const uint32_t cart = 0xB0000000u | uint32_t(ctx->r4), offset = cart - 0xB0000000u;
    const uint32_t destination = ctx->r5, segment = destination & 0xE0000000u, physical = destination & 0x1FFFFFFFu;
    const auto rom = recomp::get_rom();
    if ((cart & 3) || uint64_t(offset) + 4 > rom.size() || (destination & 3) ||
        (segment != 0x80000000 && segment != 0xA0000000) || uint64_t(physical) + 4 > 0x800000) {
        std::fprintf(stderr, "baseline ROM-PIO rejected cart=%08x destination=%08x bytes=4; no write\n", cart, destination);
        rejected_body("ROM-PIO request validation");
    }
    const gpr canonical = static_cast<gpr>(static_cast<int32_t>(0x80000000u | physical));
    recomp::do_rom_pio(rdram, canonical, recomp::rom_base + offset);
    for (uint32_t i = 0; i < 4; ++i)
        if (MEM_BU(i, canonical) != rom[offset + i]) rejected_body("ROM-PIO copied bytes differ from original ROM");
    ctx->r2 = 0;
    ++pio_copies; pio_offset = offset; pio_destination = uint32_t(canonical);
#if BASELINE_SUCCESS_TRACE
    std::fprintf(stderr, "baseline ROM-PIO completed rom_offset=%08x destination=%08x bytes=4 original_bytes_equal=1 return=0\n", offset, destination);
#endif
}
// Original CRC initializer returned; validate the prefix before VI creation.
extern "C" void baseline_stop_after_crc_init(uint8_t* rdram, recomp_context* ctx) {
    const auto thread = ultramodern::this_thread();
    if (!main_entry_observed || uint32_t(thread) != 0x80045788 || osGetThreadId(rdram, thread) != 6 ||
        uint32_t(ctx->r29) != 0x80045728 || pio_copies != 1 || pio_destination != 0x8007E9AC)
        rejected_body("post-CRC stop thread/stack/PIO admission");
    const gpr destination = static_cast<gpr>(static_cast<int32_t>(pio_destination));
    const auto rom = recomp::get_rom();
    for (uint32_t i = 0; i < 4; ++i)
        if (MEM_BU(i, destination) != rom[pio_offset + i]) rejected_body("CRC byte-count destination differs from recorded original ROM request");
    std::fprintf(stderr, "baseline post-CRC checked PC=80012238 id=6 rom_offset=%08x destination=%08x bytes=4 equal=1; Q-VI-LOCK status-seeded post-core2-prefix arm\n", pio_offset, pio_destination);
    vi_prefix_checked.store(true, std::memory_order_release);
    std::fflush(stderr);
}
#endif

#ifdef BASELINE_VI_CONTROLLER_CYCLE
namespace baseline {
// thread_create_callback seeds first dispatch: FF03 saved, unchanged mask, ERET clears EXL.
void seed_thread_status(uint8_t* rdram, recomp_context* ctx) {
    const auto thread = ultramodern::this_thread();
    const int32_t id = osGetThreadId(rdram, thread), priority = osGetThreadPri(rdram, thread);
    constexpr uint32_t identities[][3] = {{0x800775F0, 1, 0}, {0x80045788, 6, 20}, {0x8007D8B0, 3, 30}, {0x80079140, 0, 80}, {0x80078C98, 5, 60}, {0x80079BA8, 7, 40}, {0x8007AF10, 8, 25}, {0x800459F8, 4, 50}, {0x80076A20, 2, 10}};
    unsigned bit = 0;
    for (unsigned i = 0; i < std::size(identities); ++i)
        if (uint32_t(thread) == identities[i][0] && uint32_t(id) == identities[i][1] && uint32_t(priority) == identities[i][2]) bit = 1u << i;
    if (uint32_t(thread) == 0x80076A20 && id == 2 && priority == 35 && active_sync.load(std::memory_order_acquire) && tidy_promoted.load(std::memory_order_acquire)) bit = 1u << 8;
    const uint32_t global = MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x800413A0)));
    if (!bit || ctx->status_reg != 0 || ctx->mips3_float_mode != 0 || ctx->f_odd != &ctx->f0.u32h || global != 0x003FFF01 ||
        (thread_status_seen.fetch_or(bit, std::memory_order_relaxed) & bit)) rejected_body("thread status fresh identity/FR0/global/repeat admission");
    const uint32_t before = ctx->status_reg;
    ctx->status_reg = 0xFF01;
    std::fprintf(stderr, "baseline thread-status seed thread=%08x id=%d priority=%d before=%08x after=%08x global=%08x\n",
        uint32_t(thread), id, priority, before, ctx->status_reg, global);
    std::fflush(stderr);
}
}
// Creator returned; verify fixed queue capacity/storage and known worker identity.
extern "C" void baseline_observe_vi_creation(uint8_t* rdram, recomp_context* ctx) {
    const auto thread = ultramodern::this_thread();
    if (!vi_prefix_checked.load(std::memory_order_acquire) || uint32_t(thread) != 0x80045788 ||
        osGetThreadId(rdram, thread) != 6 || uint32_t(ctx->r29) != 0x80045728)
        rejected_body("VI creation main thread/stack/prefix admission");
    constexpr int32_t vi_thread = static_cast<int32_t>(0x80079140u);
    if (osGetThreadId(rdram, vi_thread) != 0 || osGetThreadPri(rdram, vi_thread) != 80)
        rejected_body("VI creation original worker identity/priority");
    constexpr uint32_t queues[][3] = {{0x80078FD0, 0x80078FE8, 10}, {0x80079010, 0x80079028, 1}, {0x80079030, 0x80079048, 60}};
    for (const auto& queue : queues) {
        const gpr address = static_cast<gpr>(static_cast<int32_t>(queue[0]));
        if (uint32_t(MEM_W(16, address)) != queue[2] || uint32_t(MEM_W(20, address)) != queue[1])
            rejected_body("VI creation original queue capacity/storage");
    }
    std::fprintf(stderr, "baseline VI creation observed PC=80012240 id=6 VI_thread=80079140 id=0 priority=80 queues=3 capacity_storage_equal=1 creator_returned=1 main_continuation=1; core2/joins unaccepted\n");
    std::fflush(stderr);
}
// Historical stop hook at800154E0 now observes each bounded original tail and returns.
extern "C" void baseline_stop_after_vi_handler(uint8_t* rdram, recomp_context* ctx) {
    unsigned passes = vi_worker_returns.load(std::memory_order_relaxed);
    const auto thread = ultramodern::this_thread();
    const int32_t id = osGetThreadId(rdram, thread), priority = osGetThreadPri(rdram, thread);
    if (uint32_t(thread) != 0x80079140 || id != 0 || priority != 80 ||
        !vi_prefix_checked.load(std::memory_order_acquire) || uint32_t(ctx->r29) != 0x80079698 ||
        uint32_t(ctx->r20) != 0x80078FD0 || uint32_t(ctx->r21) != 0x800796E8 || ctx->status_reg != 0xFF01)
        rejected_body("VI worker-cycle original worker/stack/queue/status/prefix/count admission");
    const auto word = [&](uint32_t address, int offset = 0) {
        return uint32_t(MEM_W(offset, static_cast<gpr>(static_cast<int32_t>(address))));
    };
    const uint32_t message = MEM_W(0x50, ctx->r29), counter = word(0x80079138);
    const uint32_t outbound = word(0x80079030, 8), head = word(0x80079030, 12);
    const uint32_t input_first = word(0x80078FD0, 12);
    const bool frame = first_sync_entered.load(std::memory_order_acquire);
    if ((!frame && (counter != passes || outbound != std::min(passes, 60u) || head != 0 ||
        word(0x80079030) != 0 || word(0x80079030, 4) != 0)) || message != 0 || outbound > 60 || head >= 60 ||
        word(0x80079030, 16) != 60 || word(0x80079030, 20) != 0x80079048 ||
        word(0x80078FD0, 16) != 10 || word(0x80078FD0, 20) != 0x80078FE8 || input_first != (passes + 1) % 10)
        rejected_body("VI worker-cycle message/counter/input/outbound admission");
    if (word(0x80078F90) ? (word(0x80078F90) != 0x800783D0 || word(0x80078F94) != 5) : word(0x80078F94) != 0)
        rejected_body("VI worker-cycle registered scheduler admission");
    if (word(0x80078F9C) || (word(0x80078F98) && (word(0x80078F90) != 0x800783D0 || word(0x80078F98) != 0x8007AEF0)))
        rejected_body("VI worker-cycle registered motor admission");
    for (uint32_t address = 0x80078FA0; address < 0x80078FD0; address += 8)
        if (word(address) != 0) rejected_body("VI worker-cycle extra registration pointer admission");
    ++passes;
    vi_worker_returns.store(passes, std::memory_order_release);
}
// Observe before800122BC and its800122C0 delay slot; allow the original loader.
extern "C" void baseline_observe_before_core2_loader(uint8_t* rdram, recomp_context* ctx) {
    const auto thread = ultramodern::this_thread();
    if (core2_pre_admitted || !vi_prefix_checked.load(std::memory_order_acquire) || uint32_t(thread) != 0x80045788 ||
        osGetThreadId(rdram, thread) != 6 || osGetThreadPri(rdram, thread) != 20 ||
        uint32_t(ctx->r29) != 0x80045728 || ctx->status_reg != 0xFF01 ||
        ctx->mips3_float_mode != 0 || ctx->f_odd != &ctx->f0.u32h)
        rejected_body("main pre-loader context/status/FR0 admission");
    const auto word = [&](uint32_t address, int offset = 0) {
        return uint32_t(MEM_W(offset, static_cast<gpr>(static_cast<int32_t>(address))));
    };
    constexpr uint32_t witnesses[][2] = {{0x8007E998, 0x80400000}, {0x8007E990, 0x80137800},
        {0x8007E994, 0x80137800}, {0x8007C714, 0x0F070100}, {0x8007D0D8, 0x00050001}, {0x8007D154, 0x63056001}};
    for (const auto& witness : witnesses)
        if (word(witness[0]) != witness[1]) rejected_body("main pre-loader buffer/pool witness admission");
    const uint32_t registers[] = {uint32_t(ctx->r5), uint32_t(ctx->r6), uint32_t(ctx->r7)};
    constexpr uint32_t arguments[] = {0x800815C0, 0x80137390, 0x01E5AEB0, 0x01F00020, 0x800815C0,
        0x80117C60, 0x80117C60, 0x80126730, 0x80126730, 0x80137390};
    for (unsigned i = 0; i < 10; ++i) {
        const uint32_t actual = i < 3 ? registers[i] : word(uint32_t(ctx->r29), 0x10 + 4 * (i - 3));
        if (actual != arguments[i]) rejected_body("main pre-loader prepared argument admission");
    }
    const auto rom = recomp::get_rom();
    if (word(0x80012010) != 0x01E42550 || word(0x80012014) != 0x01E899B0 ||
        rom.size() < 0x01E899B0 || (word(0x8007E994) & 15) ||
        0x8017EC60u > word(0x8007E998) || word(0x8007E994) < 0x80137390u ||
        word(0x8007695C) || word(0x8007695C, 4) || word(0x8007695C, 8) || word(0x8007695C, 12) ||
        word(0x8007695C, 16) != 1 || word(0x8007695C, 20) != 0x80076958)
        rejected_body("core2 original descriptor/ROM/RDRAM/nonoverlap/DMA queue admission");
    core2_pre_admitted = true;
    const unsigned worker_returns = vi_worker_returns.load(std::memory_order_acquire);
    std::fprintf(stderr, "baseline main-preloader PC=800122bc id=6 priority=20 SR=%08x FR0=1 buffers_equal=1 tables_witnessed=1 arguments_equal=1 descriptors_equal=1 transfer_bounds_equal=1 dma_queue_equal=1 loader_call_unexecuted=1 loader_delay_slot_unexecuted=1 worker_returns=%u; phase=loader-window core2/joins unaccepted\n",
        ctx->status_reg, worker_returns);
    std::fflush(stderr);
}
// Observe the original loader return before800122C4 and its heap-call delay slot.
extern "C" void baseline_observe_and_publish_core2(uint8_t* rdram, recomp_context* ctx) {
    static bool published = false;
    const auto thread = ultramodern::this_thread();
    if (!core2_pre_admitted || published || uint32_t(thread) != 0x80045788 ||
        osGetThreadId(rdram, thread) != 6 || osGetThreadPri(rdram, thread) != 20 ||
        uint32_t(ctx->r29) != 0x80045728 || ctx->status_reg != 0xFF01 ||
        ctx->mips3_float_mode != 0 || ctx->f_odd != &ctx->f0.u32h)
        rejected_body("core2 post-loader context/status/FR0/prior admission");
    const auto word = [&](uint32_t address) { return uint32_t(MEM_W(0, static_cast<gpr>(static_cast<int32_t>(address)))); };
    const auto rom = recomp::get_rom();
    if (rom.size() < 0x01E899B0)
        rejected_body("core2 observation reference/ROM extent admission");
    // Compressed transport is compared byte-for-byte with the user's validated ROM.
    for (uint32_t i = 0; i < 0x47460; ++i) {
        const unsigned actual = MEM_BU(i, static_cast<gpr>(static_cast<int32_t>(0x80137800))), expected = rom[0x01E42550 + i];
        if (actual != expected) {
            std::fprintf(stderr, "baseline core2 transport_equal=0 first_offset=%08x actual=%02x expected=%02x\n", i, actual, expected);
            rejected_body("core2 original transport/image mismatch; no publication");
        }
    }
    // The inflated image has no stored ROM equivalent; its guest-order SHA-256 is pinned.
    const std::string image = baseline::guest_range_sha256(rdram, 0x800815C0, 676208);
    if (image != core2_reference_sha256) {
        std::fprintf(stderr, "baseline core2 image_equal=0 bytes=676208 sha256=%s expected=%s\n", image.c_str(), core2_reference_sha256);
        rejected_body("core2 original transport/image mismatch; no publication");
    }
    constexpr uint32_t checksums[] = {38961375, 1547267646, 1818534, 147349199};
    for (unsigned i = 0; i < 4; ++i)
        if (word(0x80126730 + 4 * i) != checksums[i]) rejected_body("core2 original checksum words differ");
    for (uint32_t address = 0x80126740; address < 0x80137390; ++address)
        if (MEM_BU(0, static_cast<gpr>(static_cast<int32_t>(address)))) rejected_body("core2 remaining BSS is not zero");
    if (word(0x8007D158) != 0x8017EC5A) rejected_body("core2 final compressed cursor differs");
    if (!following_section || following_section->rom_addr != 0x01E5AEB0 || following_section->ram_addr != 0x800815C0 ||
        following_section->size != 0xA5170 || following_section->index != 2 || following_section->num_relocs ||
        !following_section->num_funcs || !following_section->funcs || following_section->funcs[0].offset != 0 || !following_section->funcs[0].func)
        rejected_body("core2 section identity changed before publication");
    load_overlays(0x01E5AEB0, static_cast<int32_t>(0x800815C0), 0xA5170);
#if BASELINE_CAMERA_IDENTITY
    camera_validate_published_section(*following_section,0x800815C0);
#endif
    if (get_function(static_cast<int32_t>(0x800815C0)) != following_section->funcs[0].func)
        rejected_body("core2 published entry lookup differs");
#if BASELINE_RENDER_BRIDGE
    if (!bridge_birth_patch_address || !bridge_bone_patch_address || !bridge_owner_patch_address || !bridge_draw_patch_address ||
        get_function(static_cast<int32_t>(0x800EBED4)) != func_800EBED4 ||
        get_function(static_cast<int32_t>(bridge_birth_patch_address)) != func_800EBED4 ||
        get_function(static_cast<int32_t>(0x800DCF48)) != func_800DCF48 ||
        get_function(static_cast<int32_t>(bridge_bone_patch_address)) != func_800DCF48 ||
        get_function(static_cast<int32_t>(0x80101808)) != func_80101808 ||
        get_function(static_cast<int32_t>(bridge_owner_patch_address)) != func_80101808 ||
        get_function(static_cast<int32_t>(0x800DE448)) != func_800DE448 ||
        get_function(static_cast<int32_t>(bridge_draw_patch_address)) != func_800DE448)
        rejected_body("core2 original/patch/provider lookup differs");
#if BASELINE_RENDER_SILO
    if (!bridge_silo_reset_patch_address || !bridge_silo_transition_patch_address ||
        get_function(static_cast<int32_t>(0x8008B10C)) != func_8008B10C ||
        get_function(static_cast<int32_t>(bridge_silo_reset_patch_address)) != func_8008B10C ||
        get_function(static_cast<int32_t>(0x8008B1A0)) != anctrl_setSmoothTransition ||
        get_function(static_cast<int32_t>(bridge_silo_transition_patch_address)) != anctrl_setSmoothTransition)
        rejected_body("Silo setter original/patch/provider lookup differs");
#endif
#endif
    published = true;
    std::fprintf(stderr, "baseline core2-publication PC=800122c4 transport_equal=1 image_equal=1 bytes=676208 bss_equal=1 checksums_equal=1 cursor=8017ec5a publication_equal=1 heap_call_unexecuted=1 heap_delay_slot_unexecuted=1 worker_returns=%u data_bytes=%u global_crc0=%08x global_crc1=%08x v0=%08x r31=%08x prefix_continuation_admitted=1; phase=loader-window scheduler/input/audio/RSP/joins unaccepted\n",
        vi_worker_returns.load(std::memory_order_acquire), word(0x8007C700), word(0x8003F760), word(0x8003F764), uint32_t(ctx->r2), uint32_t(ctx->r31));
    std::fflush(stderr);
}
// Original heap/bookkeeping/core2 leaf returned; observe before80012314 and80012318.
extern "C" void baseline_observe_before_scheduler_init(uint8_t* rdram, recomp_context* ctx) {
    const auto thread = ultramodern::this_thread();
    if (uint32_t(thread) != 0x80045788 || osGetThreadId(rdram, thread) != 6 || osGetThreadPri(rdram, thread) != 20 ||
        uint32_t(ctx->r29) != 0x80045728 || ctx->status_reg != 0xFF01 ||
        ctx->mips3_float_mode != 0 || ctx->f_odd != &ctx->f0.u32h ||
        pio_copies != 2 || pio_offset != 0x50E0 || pio_destination != 0x80126730)
        rejected_body("post-core2 main context/status/FR0/PIO admission");
    constexpr uint32_t witnesses[][2] = {{0x8007BF00, 0x002C8800}, {0x8003F750, 0x00076880},
        {0x80078F70, 0x803DE220}, {0x80078F74, 0x803BC450}, {0x80078F78, 0x803AB540},
        {0x80078F80, 0x803DE240}, {0x80078F84, 0x803BC480}, {0x80078F88, 0x803AB540},
        {0x8007BCC0, 0x80137830}, {0x8007BCC4, 0x80137840}, {0x80137814, 0x801595F0}, {0x801595F4, 0x803AB530},
        {0x80043788, 0x01020304}, {0x800459D8, 0x01020304}, {0x8004378C, 0x05060708}, {0x800459DC, 0x05060708},
        {0x80124A8C, 0x80011000}, {0x80124A80, 0x8000E800}, {0x80126730, 0x01E899B0},
        {0x80117C60, 0}, {0x80117C64, 0}, {0x80126CB8, 0}, {0x800413A0, 0x003FFF01},
        {0x80078F90, 0}, {0x80078F98, 0}, {0x80078FA0, 0}, {0x80078FA8, 0},
        {0x80078FB0, 0}, {0x80078FB8, 0}, {0x80078FC0, 0}, {0x80078FC8, 0}};
    for (const auto& witness : witnesses) {
        const uint32_t actual = MEM_W(0, static_cast<gpr>(static_cast<int32_t>(witness[0])));
        if (actual != witness[1]) {
            std::fprintf(stderr, "baseline post-core2 witness address=%08x actual=%08x expected=%08x\n", witness[0], actual, witness[1]);
            rejected_body("post-core2 original fixed-word witness differs");
        }
    }
    std::fprintf(stderr, "baseline post-core2-prefix PC=80012314 heap_equal=1 vi_buffers_equal=1 scratch_equal=1 links_equal=1 markers_equal=1 core2_leaf_equal=1 pio_equal=1 bookkeeping_equal=1 registrations_zero=1 scheduler_call_unexecuted=1 scheduler_delay_slot_unexecuted=1 worker_returns=%u continuation_admitted=1; phase=loader-window scheduler/input/audio/RSP/joins unaccepted\n", vi_worker_returns.load(std::memory_order_acquire));
    std::fflush(stderr);
}
// Historical stop hook now observes once before80014AA4, then allows genuine receive.
extern "C" void baseline_stop_before_scheduler_receive(uint8_t* rdram, recomp_context* ctx) {
    const auto thread = ultramodern::this_thread();
    const unsigned phase = vi_worker_returns.load(std::memory_order_acquire), expected = phase;
    if (uint32_t(thread) != 0x80078C98 || osGetThreadId(rdram, thread) != 5 || osGetThreadPri(rdram, thread) != 60 ||
        uint32_t(ctx->r29) != 0x80078C68 || ctx->status_reg != 0xFF01 ||
        ctx->mips3_float_mode != 0 || ctx->f_odd != &ctx->f0.u32h || uint32_t(ctx->r4) != 0x800783D0)
        rejected_body("scheduler-start context/status/FR0/phase admission");
    if (scheduler_receive_observed) return; // Later receives retain context admission, not startup contents.
    const auto word = [&](uint32_t address) { return uint32_t(MEM_W(0, static_cast<gpr>(static_cast<int32_t>(address)))); };
    const uint32_t witnesses[][2] = {{0x8007BEE0, 0}, {0x8007BEE4, 0}, {0x8007BEE8, 1},
        {0x8007BEEC, 0}, {0x8007BEF0, 1}, {0x8007BEF4, 0x8007BEF8},
        {0x800783D0, 0}, {0x800783D4, 0}, {0x800783D8, 0}, {0x800783DC, 0}, {0x800783E0, 20}, {0x800783E4, 0x800783E8},
        {0x80078438, 0}, {0x8007843C, 0}, {0x80078440, 0}, {0x80078444, 0}, {0x80078448, 10}, {0x8007844C, 0x80078450},
        {0x80078F90, 0x800783D0}, {0x80078F94, 5}, {0x80078F98, 0}, {0x80078FA0, 0}, {0x80078FA8, 0},
        {0x80078FB0, 0}, {0x80078FB8, 0}, {0x80078FC0, 0}, {0x80078FC8, 0}, {0x8007D834, 0x80078C98},
        {0x80079138, expected}, {0x80079030, 0}, {0x80079034, 0}, {0x80079038, expected},
        {0x8007903C, 0}, {0x80079040, 60}, {0x80079044, 0x80079048}};
    for (const auto& witness : witnesses) {
        const uint32_t actual = word(witness[0]);
        if (actual != witness[1]) {
            std::fprintf(stderr, "baseline scheduler-start witness address=%08x actual=%08x expected=%08x\n", witness[0], actual, witness[1]);
            rejected_body("scheduler-start original queue/registration/worker witness differs");
        }
    }
    if (word(0x80078F0C) || word(0x80078F10) || word(0x80078F14))
        rejected_body("input-prefix initial watchdog admission");
    scheduler_initial_returns = phase; scheduler_receive_observed = true;
    std::fprintf(stderr, "baseline scheduler-pre-receive PC=80014aa4 id=5 priority=60 SR=0000ff01 FR0=1 queues_equal=1 registrations_equal=1 p0=%u counter=%u outbound=%u receive_continuation_admitted=1 receive_call_unexecuted=1 receive_delay_slot_unexecuted=1 main_start_continuation_unexecuted=1 task_submission_unexecuted=1; FAULT/PRENMI unmapped input/audio/RSP/joins unaccepted\n", phase, word(0x80079138), word(0x80079038));
    std::fflush(stderr);
}
// Historical rejecting fallback now observes natural main continuation before input.
extern "C" void baseline_reject_main_after_scheduler_start(uint8_t* rdram, recomp_context* ctx) {
    const auto thread = ultramodern::this_thread();
    if (uint32_t(thread) != 0x80045788 || osGetThreadId(rdram, thread) != 6 || osGetThreadPri(rdram, thread) != 20 ||
        uint32_t(ctx->r29) != 0x80045728 || ctx->status_reg != 0xFF01 || ctx->mips3_float_mode != 0 || ctx->f_odd != &ctx->f0.u32h)
        rejected_body("input-prefix main continuation context/status/FR0 admission");
    std::fprintf(stderr, "baseline input-prefix main-continuation PC=8001231c id=6 SR=0000ff01 input_call_unexecuted=1 input_delay_slot_unexecuted=1 continuation_admitted=1\n");
    std::fflush(stderr);
}
// Observe each original VI handler return; real one-shot timers remain admitted.
extern "C" void baseline_stop_after_scheduler_vi(uint8_t* rdram, recomp_context* ctx) {
    const auto thread = ultramodern::this_thread();
    if (uint32_t(thread) != 0x80078C98 || osGetThreadId(rdram, thread) != 5 || osGetThreadPri(rdram, thread) != 60 ||
        uint32_t(ctx->r29) != 0x80078C68 || ctx->status_reg != 0xFF01 || ctx->mips3_float_mode != 0 || ctx->f_odd != &ctx->f0.u32h)
        rejected_body("timer-owned VI-handler context/counter admission");
    ++scheduler_completed_handlers;
    const auto word = [&](uint32_t address) { return uint32_t(MEM_W(0, int32_t(address))); };
    if (!sync_wait_observed && active_sync.load(std::memory_order_acquire) && sync_episode == 3 && word(0x80079038) == 60) {
        sync_wait_observed = true;
        const auto* main = TO_PTR(OSThread, int32_t(0x80045788));
        std::fprintf(stderr, "baseline sync-wait-snapshot PC=80014aec episode=%u main_priority=%d main_state=%u main_queue=%08x completion_count=%u completion_receiver=%08x completion_sender=%08x vi_count=%u vi_head=%u vi_receiver=%08x vi_sender=%08x work_count=%u work_sender=%08x stop_count=%u stop_sender=%08x dp_status=%08x active=%08x pending=%08x sp=%08x ring_in=%u ring_out=%u disabled=%u\n",
            sync_episode, main->priority, unsigned(main->state), uint32_t(main->queue),
            word(0x80079018), word(0x80079010), word(0x80079014), word(0x80079038), word(0x8007903C), word(0x80079030), word(0x80079034),
            word(0x800769E8), word(0x800769E4), word(0x80076A08), word(0x80076A04), observed_dp_status(rdram),
            word(0x80078484), word(0x80078488), word(0x8007848C), word(0x80078EA8), word(0x80078EAC), word(0x80078480));
        std::fflush(stderr);
    }
}
extern "C" void baseline_observe_graphics_dp(uint8_t* rdram, recomp_context* ctx) {
    const auto thread = ultramodern::this_thread();
    if (uint32_t(thread) != 0x80078C98 || osGetThreadId(rdram, thread) != 5 || osGetThreadPri(rdram, thread) != 60 ||
        ctx->status_reg != 0xFF01 || ctx->mips3_float_mode || ctx->f_odd != &ctx->f0.u32h)
        rejected_body("graphics DP observer scheduler/context ownership");
    if (graphics_dp_arrivals >= 2) return;
    const unsigned ordinal = ++graphics_dp_arrivals;
    const auto word = [&](uint32_t address) { return uint32_t(MEM_W(0, int32_t(address))); };
    std::fprintf(stderr, "baseline graphics-dp-enter PC=800142f0 ordinal=%u dp_status=%08x active=%08x pending=%08x sp=%08x ring_in=%u ring_out=%u completion_count=%u\n",
        ordinal, observed_dp_status(rdram), word(0x80078484), word(0x80078488), word(0x8007848C),
        word(0x80078EA8), word(0x80078EAC), word(0x80079018));
    std::fflush(stderr);
}
extern "C" void baseline_observe_joy_stopped(uint8_t* rdram, recomp_context* ctx) {
    ++joy_stop_count;
    const auto thread = ultramodern::this_thread();
    const auto word = [&](uint32_t a) { return uint32_t(MEM_W(0, static_cast<gpr>(static_cast<int32_t>(a)))); };
    OSThread* joy = TO_PTR(OSThread, static_cast<int32_t>(0x80079BA8));
    if (uint32_t(thread) != 0x80078C98 || osGetThreadId(rdram, thread) != 5 || osGetThreadPri(rdram, thread) != 60 ||
        uint32_t(ctx->r29) != 0x80078C50 || ctx->status_reg != 0xFF01 || ctx->mips3_float_mode || ctx->f_odd != &ctx->f0.u32h ||
        word(0x80078C84) != 13 || uint32_t(ctx->r4) != 0x80080000 || word(0x80079F78) || word(0x80079BA0) != 1 ||
        word(0x80079B68) || joy->state != OSThreadState::STOPPED || !joy->context)
        rejected_body("controller-cycle post-stop context/token/joy admission");
    const auto absent = [&](int32_t queue) {
        constexpr uint32_t nodes[] = {0x800775F0, 0x80045788, 0x8007D8B0, 0x80079140, 0x80078C98, 0x80079BA8, 0x8007AF10, 0x800459F8, 0x80076A20};
        unsigned visited = 0;
        for (int32_t p = ultramodern::thread_queue_peek(rdram, queue); p != NULLPTR; p = TO_PTR(OSThread, p)->next) {
            unsigned i = 0; while (i < std::size(nodes) && nodes[i] != uint32_t(p)) ++i;
            if (i == std::size(nodes) || uint32_t(p) == 0x80079BA8 || (visited & (1u << i))) return false;
            visited |= 1u << i;
        }
        return true;
    };
    if (!absent(static_cast<int32_t>(0x80079B60)) || !absent(ultramodern::running_queue))
        rejected_body("controller-cycle stopped joy remains linked/invalid list");
#if BASELINE_SUCCESS_TRACE
    std::fprintf(stderr, "baseline controller-cycle post-stop PC=80016840 stop_count=%u joy_stopped=1 joy_unlinked=1 read_call_unexecuted=1 read_delay_slot_unexecuted=1\n", joy_stop_count);
    std::fflush(stderr);
#endif
}
extern "C" void baseline_observe_joy_receive(uint8_t* rdram, recomp_context* ctx) {
    const auto thread = ultramodern::this_thread();
    if (uint32_t(thread) != 0x80079BA8 || osGetThreadId(rdram, thread) != 7 || osGetThreadPri(rdram, thread) != 40 ||
        uint32_t(ctx->r29) != 0x80079F18 || ctx->status_reg != 0xFF01 || ctx->mips3_float_mode || ctx->f_odd != &ctx->f0.u32h ||
        uint32_t(ctx->r4) != 0x80079B60 || uint32_t(ctx->r5) != 0 || uint32_t(ctx->r17) != 1)
        rejected_body("controller-cycle original joy receive continuation");
    ++joy_receive_arrivals;
}
extern "C" void baseline_stop_before_audio(uint8_t* rdram, recomp_context* ctx) {
    const auto thread = ultramodern::this_thread();
    const unsigned n = vi_worker_returns.load(std::memory_order_acquire), m = scheduler_completed_handlers;
    if (uint32_t(thread) != 0x80045788 || osGetThreadId(rdram, thread) != 6 || osGetThreadPri(rdram, thread) != 20 ||
        uint32_t(ctx->r29) != 0x80045728 || ctx->status_reg != 0xFF01 || ctx->mips3_float_mode != 0 || ctx->f_odd != &ctx->f0.u32h ||
        thread_status_seen.load(std::memory_order_acquire) != 0x7F || pfs_absence_calls != 4 || !scheduler_receive_observed ||
        n > 8 || n < scheduler_initial_returns || n - scheduler_initial_returns != m)
        rejected_body("input-prefix main/context/seven seeds/four no-pak calls/N/p0/M admission");
    const uint32_t witnesses[][2] = {{0x80078F58, 1}, {0x80079B60, 0x80079BA8}, {0x80079B68, 0},
        {0x80079F78, 1}, {0x80079F7C, 1}, {0x80079BA0, 0}, {0x8007AEF0, 0x8007AF10}, {0x8007AEF8, 0},
        {0x80078F90, 0x800783D0}, {0x80078F94, 5}, {0x80078F98, 0x8007AEF0}, {0x80078F9C, 0},
        {0x80078FA0, 0}, {0x80078FA8, 0}, {0x80078FB0, 0}, {0x80078FB8, 0}, {0x80078FC0, 0}, {0x80078FC8, 0},
        {0x800783D0, 0x80078C98}, {0x800783D8, 0}, {0x80079138, n}, {0x80079038, n}};
    for (const auto& witness : witnesses) {
        const uint32_t actual = MEM_W(0, static_cast<gpr>(static_cast<int32_t>(witness[0])));
        if (actual != witness[1]) {
            std::fprintf(stderr, "baseline input-prefix witness address=%08x actual=%08x expected=%08x\n", witness[0], actual, witness[1]);
            rejected_body("input-prefix original wait/registration/count witness differs");
        }
    }
    std::fprintf(stderr, "baseline preaudio observation PC=80012334 id=6 priority=20 SR=0000ff01 FR0=1 seeds=7 pfs_calls=%u joy_waiting=1 motor_waiting=1 registrations_equal=1 N=%u p0=%u M=%u counter=%u outbound=%u bitpattern=%02x errno=%02x,%02x,%02x,%02x timer_actions_admitted=1 main_continuation_admitted=1 audio_call_unexecuted=1 audio_delay_slot_unexecuted=1\n", pfs_absence_calls, n, scheduler_initial_returns, m,
        uint32_t(MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x80079138)))), uint32_t(MEM_W(0, static_cast<gpr>(static_cast<int32_t>(0x80079038)))),
        unsigned(MEM_BU(0, static_cast<gpr>(static_cast<int32_t>(0x80079D68)))), unsigned(MEM_BU(0, static_cast<gpr>(static_cast<int32_t>(0x80079B93)))), unsigned(MEM_BU(0, static_cast<gpr>(static_cast<int32_t>(0x80079B97)))), unsigned(MEM_BU(0, static_cast<gpr>(static_cast<int32_t>(0x80079B9B)))), unsigned(MEM_BU(0, static_cast<gpr>(static_cast<int32_t>(0x80079B9F)))));
    std::fflush(stderr);
}
extern "C" void baseline_hold_after_audio_init(uint8_t* rdram, recomp_context* ctx) {
    const auto thread = ultramodern::this_thread();
    const auto word = [&](uint32_t a) { return uint32_t(MEM_W(0, static_cast<int32_t>(a))); };
    if (uint32_t(thread) != 0x80045788 || osGetThreadId(rdram, thread) != 6 || osGetThreadPri(rdram, thread) != 20 ||
        uint32_t(ctx->r29) != 0x80045728 || ctx->status_reg != 0xFF01 || ctx->mips3_float_mode || ctx->f_odd != &ctx->f0.u32h ||
        word(0x80046A78) != 0x80046A88 || word(0x80046A80) != 0x22A00 ||
        word(0x80046A7C) < 0x80046A88 || word(0x80046A7C) > 0x80069488 || word(0x8003CA40) != 1)
        rejected_body("audio initialized main/context/heap/start admission");
    constexpr uint32_t queues[][3] = {{0x80069488, 0x800694A0, 50}, {0x80045BF0, 0x80045C08, 8}, {0x80045BB8, 0x80045BD0, 8}};
    for (const auto& q : queues) if (word(q[0] + 16) != q[2] || word(q[0] + 20) != q[1]) rejected_body("audio initialized queue capacity/storage");

    std::fprintf(stderr, "baseline audio-main-continued PC=8001233c heap_current=%08x queues_equal=1 original125B0_returned=1\n", word(0x80046A7C)); std::fflush(stderr);
}
extern "C" void baseline_completed_pcm(uint8_t* rdram, recomp_context* ctx) {
    static bool observed = false;
    if (observed) return;
    const auto thread = ultramodern::this_thread();
    const auto word = [&](uint32_t a) { return uint32_t(MEM_W(0, static_cast<int32_t>(a))); };
    if (uint32_t(thread) != 0x800459F8 || osGetThreadId(rdram, thread) != 4 || osGetThreadPri(rdram, thread) != 50 ||
        uint32_t(ctx->r29) != 0x800469F8 || ctx->status_reg != 0xFF01 || ctx->mips3_float_mode || ctx->f_odd != &ctx->f0.u32h)
        rejected_body("completed PCM audio context/status/FR0");
    const uint32_t prev = word(uint32_t(ctx->r29) + 0x3C);
    if ((prev != word(0x800459E8) && prev != word(0x800459EC) && prev != word(0x800459F0)) ||
        prev < 0x80046A88 || prev > 0x80069478 || word(0x8003CC60) != prev || word(0x8003CC5C) != prev + 8 || word(prev + 12) != prev)
        rejected_body("completed PCM original descriptor/receive witnesses");
    const uint32_t pcm = word(prev), bytes = uint16_t(MEM_H(4, static_cast<int32_t>(prev))) * 4;
    if ((pcm != 0x80073E18 && pcm != 0x80074C78 && pcm != 0x80075AD8) ||
        (bytes != 0xE60 && bytes != 0x8A0 && bytes != 0xB80) || uint32_t(ctx->r2) != 0)
        rejected_body("completed PCM original buffer/count/AI return");
    std::vector<uint8_t> canonical(bytes); unsigned nonzero = 0;
    for (uint32_t i = 0; i < bytes; ++i) canonical[i] = MEM_BU(0, static_cast<int32_t>(pcm + i));
    for (uint32_t i = 0; i < bytes; i += 2) nonzero += canonical[i] != 0 || canonical[i + 1] != 0;
    std::fprintf(stderr, "baseline completed-PCM PC=80012a54 descriptor=%08x pcm=%08x bytes=%u sha256=%s nonzero_samples=%u main_held=0 AI_return=0 queued=1\n",
        prev, pcm, bytes, tooie::platform::digest(canonical, true).c_str(), nonzero);
    std::fflush(stderr); observed = true;
}

#endif

// Link-only child, separate from rejected guest generation bodies.
#if (BASELINE_HONEY_PATCH || BASELINE_RENDER_BRIDGE) && !defined(BASELINE_IMPORT_recomp_syscall_handler)
#error Honey patch requires the original-overlay alias registration path
#endif
#ifdef BASELINE_IMPORT_recomp_syscall_handler
namespace {
std::span<const SectionTableEntry> overlay_sections;
std::span<const int> overlay_ids;
struct PendingOverlay {
    uint32_t pc, id;
    enum class Phase { awaiting, entry, wrapper, done } phase = Phase::awaiting;
    bool alternate;
    uint32_t h = 0, text = 0, count, caller, delay, reset = 0;
    PendingOverlay* previous;
    static thread_local PendingOverlay* top;
    PendingOverlay(uint32_t p, uint32_t i, bool alt, uint32_t n, uint32_t ra, uint32_t d)
        : pc(p), id(i), alternate(alt), count(n), caller(ra), delay(d), previous(top) { top = this; }
    ~PendingOverlay() { top = previous; }
};
thread_local PendingOverlay* PendingOverlay::top = nullptr;
unsigned overlay_publications = 0, overlay_repeated = 0, overlay_nested = 0, alternate_completed = 0;
unsigned resource_si_commits = 0;
std::array<uint8_t, 16> resource_si_response{};
bool guest_window(uint32_t a, uint64_t size) {
    return a >= 0x80000000 && a < 0x80800000 && size <= 0x80800000 - a && !(a & 3);
}
void resource_si_scope(uint8_t* rdram) {
    const auto* pending = PendingOverlay::top;
    if (uint32_t(ultramodern::this_thread()) != 0x80045788 || osGetThreadId(rdram, ultramodern::this_thread()) != 6 ||
        !pending || !pending->alternate || pending->id != 708 || pending->pc != 0x800888A8 ||
        pending->phase != PendingOverlay::Phase::entry) rejected_body("resource SI main/alternate entry scope");
}
void resource_si_call(uint8_t* rdram, recomp_context* ctx, void (*provider)(uint8_t*, recomp_context*)) {
    resource_si_scope(rdram);
    try { provider(rdram, ctx); }
    catch (const std::exception& error) {
        std::fprintf(stderr, "baseline resource-SI-provider threw: %s\n", error.what());
        std::fflush(stderr);
        rejected_body("resource SI provider failure");
    }
}
}
namespace {
struct BulkOverlay {
    uint32_t h, id, text, group, reset, stub;
    size_t ordinal;
    std::vector<uint32_t> delays;
    bool admitted = false;
};
std::vector<BulkOverlay> bulk_overlays;
std::vector<std::pair<size_t, uint32_t>> bulk_residents;
bool bulk_started = false, bulk_completed = false, resource_prepared = false;
bool resource_decoded = false;
int bulk_current = -1;
uint32_t bulk_old_count = 0, bulk_old_last = 0, bulk_load_index = 0;
unsigned bulk_admitted = 0;
uint32_t bulk_sp = 0, bulk_map = 0, bulk_entrance = 0;
void overlay_interval_clear(size_t ordinal, uint32_t text) {
    const auto& selected = overlay_sections[ordinal];
    const uint64_t end = uint64_t(text) + selected.size;
    for (const auto& section : overlay_sections) {
        const uint32_t live = uint32_t(section_addresses[section.index]);
        if (section.index != selected.index && live != 0x80800000 &&
            text < uint64_t(live) + section.size && end >= live)
            rejected_body("public unpublication overlaps/right-adjoins live section");
    }
}
void unpublish_overlay(size_t ordinal, uint32_t text) {
    const auto& selected = overlay_sections[ordinal];
    overlay_interval_clear(ordinal, text);
    if (uint32_t(section_addresses[selected.index]) != text) rejected_body("prefree live identity");
    unload_overlays(static_cast<int32_t>(text), selected.size);
}
struct OverlayMove { uint32_t h = 0, text = 0, index = 0, id = 0; size_t ordinal = 0; };
thread_local OverlayMove overlay_move;
unsigned overlay_moves = 0;
void overlay_move_phase(uint8_t* rdram) {
    const auto thread = ultramodern::this_thread();
    // Main's empty PendingOverlay chain and resident continuation were proved at151BC.
    // Only this checked sync episode admits tidy's resident defrag interval.
    if (!active_sync.load(std::memory_order_acquire) || !tidy_promoted.load(std::memory_order_acquire) ||
        uint32_t(thread) != 0x80076A20 || osGetThreadId(rdram, thread) != 2 || osGetThreadPri(rdram, thread) != 35 ||
        uint32_t(MEM_W(0, int32_t(0x80126CBC))) != 1 || !section_addresses)
        rejected_body("move first-sync/tidy/busy phase");
}
struct NormalMoveImage { uint32_t id, text, index, entries; size_t ordinal; };
NormalMoveImage normal_move_image(uint8_t* rdram, uint32_t h) {
    if (!guest_window(h, 0x38)) rejected_body("move header window");
    const auto word = [&](uint32_t a) { return uint32_t(MEM_W(0, int32_t(a))); };
    const auto half = [&](uint32_t a) { return int16_t(MEM_H(0, int32_t(a))); };
    const uint32_t id = uint16_t(MEM_H(0, int32_t(h + 0x2C))), count = word(0x80117C60);
    if (id >= overlay_ids.size() || overlay_ids[id] < 0 || size_t(overlay_ids[id]) >= overlay_sections.size())
        rejected_body("move compiled ID");
    const size_t ordinal = size_t(overlay_ids[id]);
    const auto& section = overlay_sections[ordinal];
    const auto& registry = recomp::overlays::get_vrom_to_section_map();
    const auto found = registry.find(section.rom_addr);
    if (section.ram_addr != 0x80800000 || found == registry.end() || found->second != ordinal)
        rejected_body("move compiled section/ROM identity");
    const int index = half(h + 0x30), n = half(h + 8), ra = half(h + 0xA), rb = half(h + 0xC), bss = half(h + 6);
    if (!count || count > 352 || index < 0 || uint32_t(index) >= count || word(0x80117C68) != uint32_t(index) ||
        word(0x80126738 + 4 * uint32_t(index)) != h || n <= 0 || ra < 0 || rb < 0 || bss < 0 ||
        half(h) < 0 || half(h + 2) < 0 || half(h + 4) < 0 ||
        uint32_t(half(h) + half(h + 2) + half(h + 4)) * 16 != section.size)
        rejected_body("move original list/header/compiled geometry");
    const uint64_t a = (uint64_t(h) + 0x38 + 4 * n + uint8_t(MEM_BU(0, int32_t(h + 0xE))) + 3) & ~uint64_t(3);
    const uint64_t b = (a + 2 * ra + 3) & ~uint64_t(3), text = (b + 4 * rb + 15) & ~uint64_t(15);
    if (!guest_window(h, text - h + section.size + uint32_t(bss) * 16) || word(h + 0x34) != text)
        rejected_body("move prefix/image/BSS window");
    const uint32_t table = h + 0x38;
    const uint32_t thunk[] = {0x3C090000 | (table >> 16), 0x35290000 | (table & 0xFFFF),
        0x01095020, 0x8D4B0000, 0x01600008, 0xA520FFFA};
    for (unsigned i = 0; i < std::size(thunk); ++i)
        if (word(h + 0x10 + 4 * i) != thunk[i]) rejected_body("move NORMAL thunk");
    // Check ALL entry targets before a caller may remove any public mapping.
    for (int i = 0; i < n; ++i) {
        const uint32_t target = word(table + 4 * uint32_t(i));
        FuncEntry entry{};
        if ((target & 3) || target < text || uint64_t(target) - text >= section.size ||
            !recomp::overlays::get_func_entry_by_section_index_function_offset(found->second, target - uint32_t(text), entry) || !entry.func)
            rejected_body("move all compiled entry targets");
    }
    return {id, uint32_t(text), uint32_t(index), uint32_t(n), ordinal};
}
void arm_overlay_move(uint8_t* rdram, recomp_context* ctx) {
    overlay_move_phase(rdram);
    if (overlay_move.h) rejected_body("nested move arm");
    const uint32_t h = uint32_t(ctx->r16);
    const auto image = normal_move_image(rdram, h);
    if (uint32_t(section_addresses[overlay_sections[image.ordinal].index]) != image.text)
        rejected_body("move arm live public identity");
    overlay_interval_clear(image.ordinal, image.text);
    overlay_move = {h, image.text, image.index, image.id, image.ordinal};
}
}
extern "C" void baseline_overlay_move_returned(uint8_t* rdram, recomp_context* ctx) {
    if (!overlay_move.h) return; // Unarmed age/no-free predecessor has stale r17.
    overlay_move_phase(rdram);
    const auto captured = overlay_move;
    const auto& section = overlay_sections[captured.ordinal];
    const auto word = [&](uint32_t a) { return uint32_t(MEM_W(0, int32_t(a))); };
    const uint32_t new_h = uint32_t(ctx->r17), count = word(0x80117C60);
    if (uint32_t(ctx->r16) != captured.h || uint32_t(section_addresses[section.index]) != captured.text ||
        !count || count > 352 || captured.index >= count || word(0x80117C68) != captured.index ||
        word(0x80126738 + 4 * captured.index) != new_h)
        rejected_body("move join captured/list/public identity");
    if (new_h == captured.h) {
        overlay_move = {};
        return;
    }
    const int64_t delta = int64_t(new_h) - int64_t(captured.h), expected_text = int64_t(captured.text) + delta;
    if (delta < INT32_MIN || delta > INT32_MAX || expected_text < 0 || expected_text > UINT32_MAX)
        rejected_body("move checked signed delta/text arithmetic");
    const auto image = normal_move_image(rdram, new_h); // Never read the moved/freed old header.
    if (image.id != captured.id || image.ordinal != captured.ordinal || image.index != captured.index || image.text != uint32_t(expected_text))
        rejected_body("move relocated ID/index/text identity");
    overlay_interval_clear(captured.ordinal, captured.text);
    overlay_interval_clear(captured.ordinal, image.text);
    unpublish_overlay(captured.ordinal, captured.text);
    load_overlays(section.rom_addr, static_cast<int32_t>(image.text), section.size);
#if BASELINE_CAMERA_IDENTITY
    camera_validate_published_section(section,image.text);
#endif
    if (uint32_t(section_addresses[section.index]) != image.text) rejected_body("move new public base");
    for (uint32_t i = 0; i < image.entries; ++i) {
        const uint32_t target = word(new_h + 0x38 + 4 * i);
        FuncEntry entry{};
        if (!recomp::overlays::get_func_entry_by_section_index_function_offset(uint16_t(image.ordinal), target - image.text, entry) ||
            !entry.func || get_function(static_cast<int32_t>(target)) != entry.func)
            rejected_body("move new public function identity");
    }
    ++overlay_moves;
#if BASELINE_SUCCESS_TRACE
    std::fprintf(stderr, "baseline overlay-move PC=80081cf0 id=%u oldH=%08x newH=%08x oldText=%08x newText=%08x index=%u delta=%lld public_identity=1 list_identity=1 entries=%u moves=%u\n",
        image.id, captured.h, new_h, captured.text, image.text, captured.index, static_cast<long long>(delta), image.entries, overlay_moves);
    std::fflush(stderr);
#endif
    overlay_move = {};
}
extern "C" void baseline_audio_callable(uint8_t*, recomp_context* ctx, uint32_t pc) {
    const uint32_t target = uint32_t(ctx->r25);
    bool allowed = false;
    switch (pc) {
    case 0x80021624: allowed = target == 0x8002241C || target == 0x8001ED68; break;
    case 0x800225B8: case 0x80022704: allowed = target == 0x80021E1C; break;
    case 0x80023444: case 0x8002348C: case 0x80023624: case 0x8002366C: allowed = target == 0x80021C8C; break;
    case 0x8002A724: allowed = target == 0x80022060; break;
    case 0x8002C2A8: allowed = target == 0x800272B0; break;
    case 0x8002CEA8: allowed = target == 0x80012BB4; break;
    }
    if (!allowed) {
        std::fprintf(stderr, "baseline audio-callable-refusal PC=%08x target=%08x\n", pc, target);
        std::fflush(stderr);
        rejected_body("audio finite resident callable admission");
    }
}
namespace baseline {
void register_overlay_metadata(std::span<SectionTableEntry> sections, std::span<const int> ids, size_t total) {
    uint64_t previous_end = 0;
    for (size_t i = 0; i < sections.size(); ++i) {
        const auto& section = sections[i];
        const uint64_t end = uint64_t(section.rom_addr) + section.size;
        if (!section.size || end > UINT32_MAX || section.index != i || i >= total ||
            (i && (section.rom_addr <= sections[i - 1].rom_addr || section.rom_addr < previous_end)))
            rejected_body("overlay metadata order/interval/index admission");
        previous_end = end;
    }
    for (int ordinal : ids) if (ordinal < -1 || (ordinal >= 0 && size_t(ordinal) >= sections.size()))
        rejected_body("overlay metadata ID admission");
    if (sections.size() <= 433 || ids.size() <= 438 || ids[438] != 433)
        rejected_body("Honey overlay module/section mapping changed");
    auto& honey = sections[433];
#if BASELINE_HONEY_PATCH || BASELINE_RENDER_BRIDGE
    if (honey.rom_addr != 0x02063630 || honey.ram_addr != 0x80800000 ||
        honey.size != 0x440 || honey.index != 433 || honey.num_funcs != 5 || !honey.funcs ||
        std::any_of(honey.funcs, honey.funcs + honey.num_funcs,
            [](const auto& entry) { return entry.offset == 0x380; }))
        rejected_body("Honey original overlay identity/omitted draw changed");
    static std::vector<FuncEntry> entries(honey.funcs, honey.funcs + honey.num_funcs);
    entries.push_back({.func = func_80800380_chhoneycarrier, .offset = 0x380, .rom_size = 0x3C});
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.offset < b.offset; });
    if (std::adjacent_find(entries.begin(), entries.end(),
            [](const auto& a, const auto& b) { return a.offset == b.offset; }) != entries.end())
        rejected_body("Honey restored overlay offsets are not unique");
    honey.funcs = entries.data(); honey.num_funcs = entries.size();
#else
    if (!honey.funcs || !std::any_of(honey.funcs, honey.funcs + honey.num_funcs,
            [](const auto& entry) { return entry.offset == 0x380; }))
        rejected_body("Honey original draw missing while its patch is disabled");
#endif
    overlay_sections = sections; overlay_ids = ids;
}
}
extern "C" int32_t baseline_overlay_hardware_word(uint32_t address) {
    if (address == 0xA4600010) return 0;
    const auto rom = recomp::get_rom();
    const uint32_t offset = address - 0xB0000000;
    if (address < 0xB0000040 || (address & 3) || offset > rom.size() || rom.size() - offset < 4)
        rejected_body("overlay hardware address/ROM extent");
    return static_cast<int32_t>((uint32_t(rom[offset]) << 24) | (uint32_t(rom[offset + 1]) << 16) |
        (uint32_t(rom[offset + 2]) << 8) | rom[offset + 3]);
}
extern "C" void recomp_syscall_handler(uint8_t* rdram, recomp_context* ctx, int32_t instruction_vram) {
    const uint32_t pc = uint32_t(instruction_vram);
    const auto word = [&](uint32_t a) { return uint32_t(MEM_W(0, static_cast<int32_t>(a))); };
    if (!guest_window(pc, 8) || uint32_t(ultramodern::this_thread()) != 0x80045788 ||
        osGetThreadId(rdram, ultramodern::this_thread()) != 6 || !section_addresses)
        rejected_body("normal overlay stub/main/live-address admission");
    const uint32_t instruction = word(pc), delay = word(pc + 4), slot = delay & 0xFFFF;
    auto* pending = PendingOverlay::top;
    const bool waiting = pending && pending->phase == PendingOverlay::Phase::awaiting;
    const bool jump = (instruction & 0xFC000000) == 0x08000000;
    if (waiting && (!jump || pending->pc != pc || pending->delay != delay)) rejected_body("overlay exclusive waiting reentry");
    const bool alternate_delay = (delay & 0xFFFF0000) == 0x38080000;
    if (slot >= 0x8000 || (slot & 3) ||
        ((delay & 0xFFFF0000) != 0x20080000 && !alternate_delay) ||
        (waiting && alternate_delay != pending->alternate)) rejected_body("overlay slot/opcode admission");
    uint32_t id = (instruction >> 6) & 0x7FFFF, h = 0;
    if (jump) {
        const uint32_t destination = ((instruction & 0x03FFFFFF) << 2) | ((pc + 4) & 0xF0000000);
        h = destination - 0x10;
        if (!guest_window(h, 0x38)) rejected_body("normal overlay header window");
        id = uint16_t(MEM_H(0, static_cast<int32_t>(h + 0x2C)));
        if (waiting && id != pending->id) rejected_body("overlay reentry ID mismatch");
    } else if ((instruction & 0xFC00003F) != 0xC) rejected_body("normal overlay instruction admission");
    if (id >= overlay_ids.size() || overlay_ids[id] < 0) rejected_body("normal overlay uncompiled ID");
    const auto ordinal = size_t(overlay_ids[id]);
    const auto& section = overlay_sections[ordinal];
    const auto& registry = recomp::overlays::get_vrom_to_section_map();
    const auto found = registry.find(section.rom_addr);
    if (section.ram_addr != 0x80800000 || found == registry.end() || found->second != ordinal)
        rejected_body("normal overlay section identity");
    const uint32_t live = uint32_t(section_addresses[section.index]);
    if (!jump) {
        if (live != section.ram_addr) rejected_body("fresh overlay already has live image");
        if (pending && pending->phase != PendingOverlay::Phase::entry)
            rejected_body("fresh overlay child outside original entry phase");
        if (id == 708 && pc == 0x800888A8) {
            if (!alternate_delay || slot || resource_si_commits != (resource_decoded ? 2u : 0u))
                rejected_body("resource fresh ALT/prior transaction admission");
            if (!resource_decoded && (!bulk_completed || !resource_prepared || !pending ||
                pending->alternate || pending->id != 2 || pending->pc != 0x80082558))
                rejected_body("first encrypted resource original context");
            std::fprintf(stderr, "baseline resource-entry pc=%08x descriptor=%08x repeat=%u SP=%08x prior_commits=%u\n",
                pc, uint32_t(ctx->r4), unsigned(resource_decoded), uint32_t(ctx->r29), resource_si_commits);
            std::fflush(stderr);
            resource_si_commits = 0;
        }
        if (pending) ++overlay_nested;
        const uint32_t count = word(0x80117C60);
        if (count >= 352) rejected_body("fresh normal/alternate original list capacity");
        PendingOverlay frame(pc, id, alternate_delay, count, uint32_t(ctx->r31), delay);
        ctx->r8 = static_cast<int32_t>(pc);
        LOOKUP_FUNC(static_cast<int32_t>(0x80081E74))(rdram, ctx);
        if (frame.phase != (frame.alternate ? PendingOverlay::Phase::done : PendingOverlay::Phase::entry))
            rejected_body("overlay handler returned without completed reentry");
        if (frame.alternate && frame.id == 708 && frame.pc == 0x800888A8) {
            const uint32_t capacity = word(0x8012B7E8), produced = word(0x8007C700), output = uint32_t(ctx->r2);
            if (!capacity || !produced || produced > capacity || !guest_window(output, capacity))
                rejected_body("resource original decode count/output window");
            if (word(0x80043788) != 0x01020304 || word(0x8004378C) != 0x05060708)
                rejected_body("resource decode original stack guards");
            std::fprintf(stderr, "baseline resource-decode-return O=%u D=%u output=%08x alternate_done=1 handler_returned=1 stack_guards=1\n",
                capacity, produced, output);
            std::fflush(stderr);
            resource_decoded = true;
        }
        return;
    }
    const auto half = [&](uint32_t a) { return int16_t(MEM_H(0, static_cast<int32_t>(a))); };
    const int n = half(h + 8), ra = half(h + 0xA), rb = half(h + 0xC);
    if (waiting && pending->alternate) {
        std::fprintf(stderr, "baseline alternate-decoded id=%u H=%08x text=%08x size=%u flags=%02x BSS=%d group=%u index=%d count=%u caller=%08x counts=%d,%d,%d sizes16=%d,%d,%d\n",
            id, h, word(h + 0x34), section.size, uint8_t(MEM_BU(0, int32_t(h + 0xF))), half(h + 6),
            uint16_t(half(h + 0x2E)), half(h + 0x30), pending->count, pending->caller, n, ra, rb, half(h), half(h + 2), half(h + 4));
        std::fflush(stderr);
    }
    if (n <= 0 || ra < 0 || rb < 0 || slot >= uint32_t(n) * 4) rejected_body("normal overlay counts/slot");
    const uint64_t a = (uint64_t(h) + 0x38 + 4 * n + uint8_t(MEM_BU(0, static_cast<int32_t>(h + 0xE))) + 3) & ~uint64_t(3);
    const uint64_t b = (a + 2 * ra + 3) & ~uint64_t(3), calculated = (b + 4 * rb + 15) & ~uint64_t(15);
    if (!guest_window(h, calculated - h + section.size) || word(h + 0x34) != calculated)
        rejected_body("normal overlay prefix/image window");
    const uint32_t text = uint32_t(calculated), table = h + 0x38;
    const uint32_t target = word(table + slot);
    FuncEntry entry{};
    if ((target & 3) || target < text || target - text >= section.size ||
        !recomp::overlays::get_func_entry_by_section_index_function_offset(found->second, target - text, entry) || !entry.func)
        rejected_body("normal overlay compiled target");
    if (waiting && pending->alternate) {
        const uint32_t flags = uint8_t(MEM_BU(0, static_cast<int32_t>(h + 0xF)));
        const int bss = half(h + 6), index = half(h + 0x30);
        std::fprintf(stderr, "baseline alternate-template id=%u target=%08x offset=%x words=%08x,%08x,%08x,%08x,%08x,%08x\n",
            id, target, target - text, word(h + 0x10), word(h + 0x14), word(h + 0x18), word(h + 0x1C), word(h + 0x20), word(h + 0x24));
        std::fflush(stderr);
        pending->h = h; pending->text = text; pending->reset = ((flags & 1) && bss != 0) ? 1 : 0;
        const uint32_t alternate_thunk[] = {0x0C000000 | ((target & 0x0FFFFFFF) >> 2), 0,
            0x0C02078C, 0, pending->caller, 0};
        for (unsigned i = 0; i < std::size(alternate_thunk); ++i)
            if (word(h + 0x10 + 4 * i) != alternate_thunk[i]) rejected_body("alternate six-word thunk");
        if (!alternate_delay || bss < 0 || (flags & 4) || index != int(pending->count) ||
            word(0x80117C60) != pending->count + 1 || word(0x80126738 + 4 * pending->count) != h ||
            half(h) < 0 || half(h + 2) < 0 || half(h + 4) < 0 ||
            uint32_t(half(h) + half(h + 2) + half(h + 4)) * 16 != section.size ||
            !guest_window(text, uint64_t(section.size) + uint32_t(bss) * 16) ||
            ctx->mips3_float_mode || ctx->f_odd != &ctx->f0.u32h)
            rejected_body("alternate header/list/image/FR0 admission");
    } else {
        const uint32_t thunk[] = {0x3C090000 | (table >> 16), 0x35290000 | (table & 0xFFFF),
            0x01095020, 0x8D4B0000, 0x01600008, 0xA520FFFA};
        for (unsigned i = 0; i < std::size(thunk); ++i)
            if (word(h + 0x10 + 4 * i) != thunk[i]) rejected_body("normal overlay six-word thunk");
    }
    if (waiting) {
        if (live != section.ram_addr) rejected_body("normal overlay first publication live admission");
        load_overlays(section.rom_addr, static_cast<int32_t>(text), section.size);
#if BASELINE_CAMERA_IDENTITY
    camera_validate_published_section(section,text);
#endif
        pending->phase = PendingOverlay::Phase::entry;
        std::fprintf(stderr, "baseline %s-overlay-publication id=%u pc=%08x H=%08x text=%08x slot=%u target=%08x publications=%u\n",
            pending->alternate ? "alternate" : "normal", id, pc, h, text, slot, target, ++overlay_publications);
        std::fflush(stderr);
    } else ++overlay_repeated;
    if (uint32_t(section_addresses[section.index]) != text || get_function(static_cast<int32_t>(target)) != entry.func)
        rejected_body("normal overlay live/public function identity");
    if (waiting && pending->alternate) {
        const uint32_t sp = uint32_t(ctx->r29);
        if (pc == 0x800888A8 && ((!resource_decoded && uint32_t(ctx->r4) != 0xAE0) || uint32_t(ctx->r5) != 0x8012B7E8))
            rejected_body("resource original entry descriptor/output pointer");
        ctx->r8 = slot; ctx->r31 = static_cast<int32_t>(h + 0x18);
        LOOKUP_FUNC(static_cast<int32_t>(target))(rdram, ctx);
        const uint32_t body_count = word(0x80117C60);
        if (ctx->r31 != gpr(int32_t(h + 0x18)) || ctx->r29 != gpr(int32_t(sp)) ||
            body_count < pending->count + 1 || body_count > 352 ||
            word(0x80126738 + 4 * pending->count) != h || half(h + 0x30) != int(pending->count))
            rejected_body("alternate genuine entry return/list");
        const uint32_t body_tail = word(0x80126738 + 4 * (body_count - 1));
        if (!guest_window(body_tail, 0x38) || ((body_count == pending->count + 1) != (body_tail == h)))
            rejected_body("alternate original body tail ownership");
        const uint32_t v0 = uint32_t(ctx->r2), v1 = uint32_t(ctx->r3);
        const uint64_t f2 = ctx->f2.u64;
        pending->phase = PendingOverlay::Phase::wrapper;
        ctx->r31 = static_cast<int32_t>(h + 0x20);
        LOOKUP_FUNC(static_cast<int32_t>(0x80081E30))(rdram, ctx);
        // ovl_unload spills H and a1 over the wrapper's f0 save in the caller argument area.
        if (ctx->r2 != gpr(int32_t(v0)) || ctx->r3 != gpr(int32_t(v1)) ||
            ctx->r29 != gpr(int32_t(sp)) || ctx->r31 != gpr(int32_t(pending->caller)) ||
            ctx->f0.u64 != ((uint64_t(h) << 32) | 1) || ctx->f2.u64 != f2 || word(0x80117C60) != body_count - 1 ||
            word(0x80126738 + 4 * pending->count) != (body_tail == h ? 0 : body_tail) ||
            word(0x80126738 + 4 * (body_count - 1)) || uint32_t(section_addresses[section.index]) != section.ram_addr ||
            word(pc) != ((id << 6) | (pending->reset << 25) | 0xC) || word(pc + 4) != pending->delay)
            rejected_body("alternate original wrapper/results/reset/unpublication");
        if (body_tail != h && uint16_t(half(body_tail + 0x30)) != pending->count)
            rejected_body("alternate surviving swap-tail index");
        pending->phase = PendingOverlay::Phase::done;
        std::fprintf(stderr, "baseline alternate-complete id=%u count=%u completed=%u integer_f2_preserved=1 original_f0_slots=1 body_count=%u after_count=%u\n",
            id, pending->count, ++alternate_completed, body_count, body_count - 1);
        std::fflush(stderr);
        return;
    }
    ctx->r8 = slot;
    ctx->r9 = static_cast<int32_t>(table);
    ctx->r10 = static_cast<int32_t>(table + slot);
    ctx->r11 = static_cast<int32_t>(target);
    MEM_H(0, static_cast<int32_t>(h + 0x32)) = 0;
    if (waiting) LOOKUP_FUNC(ctx->r11)(rdram, ctx);
    else {
        PendingOverlay frame(pc, id, false, word(0x80117C60), uint32_t(ctx->r31), delay);
        frame.phase = PendingOverlay::Phase::entry; frame.h = h; frame.text = text;
        LOOKUP_FUNC(ctx->r11)(rdram, ctx);
    }
}
extern "C" void baseline_first_overlay_returned(uint8_t* rdram, recomp_context* ctx) {
    const auto byte = [&](uint32_t a) { return uint32_t(MEM_BU(0, static_cast<int32_t>(a))); };
    const auto word = [&](uint32_t a) { return uint32_t(MEM_W(0, static_cast<int32_t>(a))); };
    if (uint32_t(ultramodern::this_thread()) != 0x80045788 ||
        osGetThreadId(rdram, ultramodern::this_thread()) != 6 || osGetThreadPri(rdram, ultramodern::this_thread()) != 20 ||
        uint32_t(ctx->r29) != 0x80045728 || ctx->status_reg != 0xFF01 || ctx->mips3_float_mode || ctx->f_odd != &ctx->f0.u32h)
        rejected_body("first overlay returned main/context");
    const uint32_t overlay_header = word(0x80126738);
    if (!guest_window(overlay_header, 0x38) || uint16_t(MEM_H(0, static_cast<int32_t>(overlay_header + 0x2C))) != 679)
        rejected_body("startup first679 original header");
    const uint32_t overlay_text = word(overlay_header + 0x34);
    if (!guest_window(overlay_text, 0x240)) rejected_body("startup first679 image window");
    if (word(0x8012B400) != 0x173 || byte(0x8012B404) != 2 || byte(0x8012B409) != 1 ||
        byte(0x8012B40F) || byte(0x8012B410) || byte(0x8012B411) != 2) rejected_body("first overlay entry0 effects");
    for (unsigned i = 0; i < 4; ++i) if (byte(0x8012B405 + i) != byte(overlay_text + 0x230 + 2 * i) ||
        byte(0x8012B40A + i) != byte(overlay_text + 0x231 + 2 * i)) rejected_body("first overlay relocated data pairs");
    const uint32_t n = word(0x80117C60);
    if (byte(0x80127114) || byte(0x80127115) != 1 || uint32_t(ctx->r2) != 0x18B || word(0x800459CC) != 0x18B)
        rejected_body("startup choice original effects/result");
    std::fprintf(stderr, "baseline startup-choice PC=80012398 H=%08x text=%08x count=%u second=%08x third=%08x saved=0000018b halfwords=", overlay_header, overlay_text, n, word(0x8012673C), word(0x80126740));
    for (unsigned i = 0; i < 6; ++i) std::fprintf(stderr, "%04x,", uint16_t(MEM_H(0, static_cast<int32_t>(overlay_header + 2 * i))));
    std::fprintf(stderr, " pairs=");
    for (unsigned i = 0; i < 8; ++i) std::fprintf(stderr, "%02x", byte(overlay_text + 0x230 + i));
    std::fprintf(stderr, " effects_equal=1 returned=1\n"); std::fflush(stderr);
    // Original startup choices observed; original scene prefix continues.
}
extern "C" void baseline_sfx_sweep_service(uint8_t* rdram, recomp_context* ctx) {
#ifdef BASELINE_TERMINAL_FENCE
    tooie::lifecycle::poll();
#else
    if (::exited.load()) rejected_body("SFX sweep observed stop before service");
#endif
    const auto admitted = [&] {
        const auto thread = ultramodern::this_thread();
        return uint32_t(thread) == 0x80045788 && osGetThreadId(rdram, thread) == 6 &&
            osGetThreadPri(rdram, thread) == 20 && uint32_t(ctx->r18) == 60 &&
            uint32_t(ctx->r19) > 0 && uint32_t(ctx->r19) < 60 &&
            uint32_t(ctx->r17) == 0x8012A990 && uint32_t(ctx->r20) == 3 &&
            uint32_t(ctx->r21) == 60 && uint32_t(ctx->r22) == 0x80128C10 &&
            !PendingOverlay::top && !overlay_move.h && !active_sync.load(std::memory_order_acquire);
    };
    if (!admitted()) rejected_body("SFX pre-delay sweep/main/lifetime admission");
    static unsigned services = 0; // Main-owned observation only; never gates service.
    const unsigned count = ++services;
    const bool log = count <= 4 || (count & (count - 1)) == 0;
    const auto observe = [&](const char* phase) {
        if (!log) return;
        std::fprintf(stderr, "baseline sfx-service-%s count=%u s3=%u s1=%08x s2=%u s4=%u s5=%u s6=%08x priority=20\n",
            phase, count, uint32_t(ctx->r19), uint32_t(ctx->r17), uint32_t(ctx->r18),
            uint32_t(ctx->r20), uint32_t(ctx->r21), uint32_t(ctx->r22));
        std::fflush(stderr);
    };
    observe("enter");
    ultramodern::wait_for_external_message_timed(rdram, 1);
    ultramodern::check_running_queue(rdram);
    // Observed-stop refusal is not a RDRAM lifetime fence.
#ifdef BASELINE_TERMINAL_FENCE
    tooie::lifecycle::poll();
#else
    if (::exited.load()) rejected_body("SFX sweep observed stop after service");
#endif
    if (!admitted()) rejected_body("SFX service return sweep/main/lifetime admission");
    observe("return"); // s3 is the previous sweep count; original code recomputes it.
}
extern "C" void baseline_overlay_boundary_refusal(uint8_t* rdram, recomp_context* ctx, uint32_t pc) {
    if (pc == 0x80081CC0) {
        arm_overlay_move(rdram, ctx);
        return;
    }
#if !BASELINE_HEAP_PATCH
    if (pc == 0x8001B2E4) {
        std::fprintf(stderr, "baseline original-moving-realloc thread=%08x old_payload=%08x requested=%08x SP=%08x\n",
            uint32_t(ultramodern::this_thread()), uint32_t(ctx->r17), uint32_t(ctx->r4), uint32_t(ctx->r29));
        std::fflush(stderr);
        return;
    }
#endif
    auto* pending = PendingOverlay::top;
    if (pc == 0x800819E4 && pending && pending->alternate && pending->phase == PendingOverlay::Phase::wrapper &&
        uint32_t(ultramodern::this_thread()) == 0x80045788 && osGetThreadId(rdram, ultramodern::this_thread()) == 6 &&
        uint32_t(ctx->r4) == pending->h && uint32_t(ctx->r5) == 1) return;
    if (pc == 0x800819E4 && bulk_started && !pending && bulk_current == -1 &&
        uint32_t(ultramodern::this_thread()) == 0x80045788 && osGetThreadId(rdram, ultramodern::this_thread()) == 6 &&
        uint32_t(ctx->r5) == 1) {
        const auto word = [&](uint32_t a) { return uint32_t(MEM_W(0, int32_t(a))); };
        const uint32_t h = uint32_t(ctx->r4), count = word(0x80117C60);
        for (size_t i = 0; i < bulk_overlays.size(); ++i) {
            if (bulk_overlays[i].h != h || bulk_overlays[i].admitted) continue;
            if (!count || count != bulk_overlays.size() - bulk_admitted || word(0x80126738) != h)
                rejected_body("bulk current-list0/count admission");
            bulk_current = int(i); bulk_old_count = count;
            bulk_old_last = word(0x80126738 + 4 * (count - 1));
            bulk_load_index = uint16_t(MEM_H(0, int32_t(h + 0x30)));
            if (bulk_load_index != 0) rejected_body("bulk current header load index");
            return;
        }
    }
    if (pc == 0x800819E4 && bulk_completed &&
        uint32_t(ultramodern::this_thread()) == 0x80045788 && osGetThreadId(rdram, ultramodern::this_thread()) == 6)
        return; // Original protected return leaves the public mapping untouched.
    if (pc == 0x800819E4 && uint32_t(ultramodern::this_thread()) == 0x80076A20) {
        overlay_move_phase(rdram);
        const uint32_t h = uint32_t(ctx->r4);
        if (!bulk_completed || bulk_current != -1 || pending || overlay_move.h ||
            uint32_t(ctx->r5) != 0 || h != uint32_t(ctx->r16))
            rejected_body("tidy unload completed-bulk/pending/move/original arguments");
        const auto image = normal_move_image(rdram, h);
        if (uint32_t(section_addresses[overlay_sections[image.ordinal].index]) != image.text)
            rejected_body("tidy unload live public identity");
        return;
    }
    if (pc == 0x800153CC) {
        const auto* tidy = TO_PTR(OSThread, int32_t(0x80076A20));
        std::fprintf(stderr, "baseline first-sync-yield PC=%08x thread=%08x id=%d priority=%d stop_count=%u stop_storage=%08x stop_receiver=%08x busy=%u tidy_priority=%d tidy_state=%u tidy_queue=%08x\n",
            pc, uint32_t(ultramodern::this_thread()), osGetThreadId(rdram, ultramodern::this_thread()), osGetThreadPri(rdram, ultramodern::this_thread()),
            uint32_t(MEM_W(8, int32_t(0x80076A00))), uint32_t(MEM_W(20, int32_t(0x80076A00))), uint32_t(MEM_W(0, int32_t(0x80076A00))),
            uint32_t(MEM_W(0, int32_t(0x80126CBC))), tidy->priority, tidy->state, uint32_t(tidy->queue));
    }
    if (pc == 0x800819E4) {
        const uint32_t h = uint32_t(ctx->r4);
        const bool valid = guest_window(h, 0x38);
        std::fprintf(stderr, "baseline defrag-frontier PC=%08x thread=%08x id=%d priority=%d H=%08x valid=%u candidate_id=%u flags=%02x age=%u\n",
            pc, uint32_t(ultramodern::this_thread()), osGetThreadId(rdram, ultramodern::this_thread()), osGetThreadPri(rdram, ultramodern::this_thread()), h, valid,
            valid ? uint16_t(MEM_H(0x2C, int32_t(h))) : 0, valid ? uint8_t(MEM_BU(0xF, int32_t(h))) : 0, valid ? uint16_t(MEM_H(0x32, int32_t(h))) : 0);
    }
    const char* boundary = pc == 0x8001A6C0 ? "heap-exhaustion" :
        pc == 0x800A9DA8 ? "heap-pressure-cleanup" : "overlay-frame-service";
    std::fprintf(stderr, "baseline normal-overlay-refusal PC=%08x publications=%u repeated=%u nested=%u tidy_seed=%u pending=%d boundary=%s a0=%08x a1=%08x s1=%08x SP=%08x saved_return=%08x\n",
        pc, overlay_publications, overlay_repeated, overlay_nested, (thread_status_seen.load() >> 8) & 1, PendingOverlay::top != nullptr, boundary,
        uint32_t(ctx->r4), uint32_t(ctx->r5), uint32_t(ctx->r17), uint32_t(ctx->r29), uint32_t(ctx->r31));
    std::fflush(stderr);
    rejected_body("original overlay lifetime/frame/SFX boundary");
}
extern "C" void baseline_alternate_before_free(uint8_t* rdram, recomp_context* ctx) {
    auto* pending = PendingOverlay::top;
    const auto word = [&](uint32_t a) { return uint32_t(MEM_W(0, int32_t(a))); };
    const uint32_t thread = uint32_t(ultramodern::this_thread());
    if (thread == 0x80076A20) {
        overlay_move_phase(rdram);
        if (!bulk_completed || bulk_current != -1 || pending || overlay_move.h)
            rejected_body("tidy prefree completed-bulk/pending/move phase");
    } else if (thread != 0x80045788 || osGetThreadId(rdram, ultramodern::this_thread()) != 6)
        baseline_overlay_boundary_refusal(rdram, ctx, 0x80081A74);
    if (pending && pending->alternate && pending->phase == PendingOverlay::Phase::wrapper) {
        const uint32_t count = word(0x80117C60);
        if (uint32_t(ctx->r4) != pending->h || count < pending->count || count >= 352)
            rejected_body("alternate original prefree parent/count");
        const uint32_t tail = word(0x80126738 + 4 * count);
        if (!guest_window(tail, 0x38) || word(0x80126738 + 4 * pending->count) != tail ||
            uint16_t(MEM_H(0, int32_t(tail + 0x30))) != pending->count ||
            ((count == pending->count) != (tail == pending->h)))
            rejected_body("alternate original prefree swap-tail shape");
        unpublish_overlay(size_t(overlay_ids[pending->id]), pending->text);
        return;
    }
    if (bulk_completed && bulk_current == -1) {
        const uint32_t h = uint32_t(ctx->r4), count = word(0x80117C60);
        if (!guest_window(h, 0x38) || count >= 352 || (MEM_BU(0, int32_t(h + 0xF)) & 4))
            rejected_body("external prefree header/count/protected state");
        const uint32_t id = uint16_t(MEM_H(0, int32_t(h + 0x2C)));
        const uint32_t index = uint16_t(MEM_H(0, int32_t(h + 0x30))), text = word(h + 0x34);
        if (id >= overlay_ids.size() || overlay_ids[id] < 0 || index > count)
            rejected_body("external prefree compiled ID/index");
        const size_t ordinal = size_t(overlay_ids[id]);
        const auto& section = overlay_sections[ordinal];
        if (section.ram_addr != 0x80800000 || !guest_window(text, section.size))
            rejected_body("external prefree compiled image window");
        const uint32_t tail = word(0x80126738 + 4 * count);
        if (!guest_window(tail, 0x38) || word(0x80126738 + 4 * index) != tail ||
            uint16_t(MEM_H(0, int32_t(tail + 0x30))) != index)
            rejected_body("external prefree original swap-tail shape");
        for (auto* frame = pending; frame; frame = frame->previous)
            if (frame->alternate || size_t(overlay_ids[frame->id]) == ordinal)
                rejected_body("external unload within active carrier section/ALT");
        unpublish_overlay(ordinal, text);
#if BASELINE_SUCCESS_TRACE
        std::fprintf(stderr, "baseline external-prefree thread=%08x id=%u H=%08x text=%08x count=%u index=%u tail=%08x public_removed=1 free_pending=1\n",
            thread, id, h, text, count, index, tail);
        std::fflush(stderr);
#endif
        return;
    }
    if (pending || !bulk_started || bulk_current < 0) baseline_overlay_boundary_refusal(rdram, ctx, 0x80081A74);
    auto& selected = bulk_overlays[size_t(bulk_current)];
    const uint32_t count = word(0x80117C60);
    if (uint32_t(ctx->r4) != selected.h ||
        count != bulk_old_count - 1 || word(0x80126738 + 4 * bulk_load_index) != bulk_old_last ||
        word(0x80126738 + 4 * count) != bulk_old_last || !guest_window(bulk_old_last, 0x38) ||
        uint16_t(MEM_H(0, int32_t(bulk_old_last + 0x30))) != bulk_load_index)
        rejected_body("bulk original saved-H/count/swap-tail/index writes");
    for (uint32_t i = 0; i < count; ++i) {
        const uint32_t h = word(0x80126738 + 4 * i);
        bool captured = false;
        for (const auto& candidate : bulk_overlays) captured |= candidate.h == h && !candidate.admitted && h != selected.h;
        if (!captured) rejected_body("bulk active list added/moved/removed header");
        for (uint32_t j = 0; j < i; ++j) if (word(0x80126738 + 4 * j) == h) rejected_body("bulk duplicate active header");
    }
    unpublish_overlay(selected.ordinal, selected.text);
    selected.admitted = true; bulk_current = -1;
    std::fprintf(stderr, "baseline bulk-prefree H=%08x id=%u old_count=%u count=%u old_last=%08x admitted=%u\n",
        selected.h, selected.id, bulk_old_count, count, bulk_old_last, ++bulk_admitted);
    std::fflush(stderr);
}
extern "C" void baseline_alternate_returned(uint8_t* rdram, recomp_context* ctx) {
    auto* pending = PendingOverlay::top;
    if (!pending || pending->id != 651 || pending->alternate ||
        pending->phase != PendingOverlay::Phase::entry || uint32_t(ultramodern::this_thread()) != 0x80045788 ||
        osGetThreadId(rdram, ultramodern::this_thread()) != 6) baseline_overlay_boundary_refusal(rdram, ctx, 0x800DA0F4);
    std::fprintf(stderr, "baseline alternate-continuation PC=800da0f4 completed=%u publications=%u repeated=%u nested=%u pending_id=651 enclosing_calls_active=1\n",
        alternate_completed, overlay_publications, overlay_repeated, overlay_nested);
    std::fflush(stderr);
}
extern "C" void baseline_stop_before_preworld_unload(uint8_t* rdram, recomp_context* ctx) {
    if (uint32_t(ultramodern::this_thread()) != 0x80045788 || osGetThreadId(rdram, ultramodern::this_thread()) != 6 ||
        uint32_t(MEM_W(0, static_cast<int32_t>(0x800459CC))) != 0x18B) rejected_body("preworld terminal main/saved choice");
    if (PendingOverlay::top)
        baseline_overlay_boundary_refusal(rdram, ctx, 0x800A7350);
    std::fprintf(stderr, "baseline normal-overlay-terminal PC=800a7350 count=%u publications=%u repeated=%u nested=%u tidy_seed=%u completed=%u SP=%08x pending=0 enclosing_calls_active=1\n",
        uint32_t(MEM_W(0, static_cast<int32_t>(0x80117C60))), overlay_publications, overlay_repeated,
        overlay_nested, (thread_status_seen.load() >> 8) & 1, alternate_completed, uint32_t(ctx->r29));
    std::fflush(stderr);
    const auto word = [&](uint32_t a) { return uint32_t(MEM_W(0, int32_t(a))); };
    const auto half = [&](uint32_t a) { return int16_t(MEM_H(0, int32_t(a))); };
    const uint32_t sp = uint32_t(ctx->r29);
    const bool repeat = bulk_started;
    if (!guest_window(sp, 0x30) || active_sync.load(std::memory_order_acquire) || overlay_move.h ||
        word(0x80126CBC) || osGetThreadId(rdram, int32_t(0x80076A20)) != 2 ||
        osGetThreadPri(rdram, int32_t(0x80076A20)) != 10 || word(0x800773E0))
        rejected_body("bulk main-local/sync/tidy idle admission");
    if (repeat) {
        const auto* tidy = TO_PTR(OSThread, int32_t(0x80076A20));
        if (!bulk_completed || bulk_current != -1 || bulk_admitted != bulk_overlays.size() ||
            uint32_t(tidy->queue) != 0x800769E0 || word(0x800769E0) != 0x80076A20 || tidy->next)
            rejected_body("bulk prior completion/tidy receive-list admission");
    } else if (sp != 0x800456D0) rejected_body("bulk initial main frame witness");
    for (uint32_t q : {0x800769E0u, 0x80076A00u})
        if (word(q + 8) || word(q + 16) != 1 || word(q + 20) != q + 0x18) rejected_body("bulk tidy queues empty/capacity/storage");
    const uint32_t count = word(0x80117C60);
    if (count > 352) rejected_body("bulk original list capacity");
    bulk_sp = sp;
    bulk_map = word(sp + 0x20);
    bulk_entrance = word(sp + 0x24);
    bulk_overlays.clear();
    bulk_residents.clear();
    bulk_admitted = 0;
    bulk_current = -1;
    bulk_old_count = bulk_old_last = bulk_load_index = 0;
    bulk_completed = false;
    resource_prepared = false;
    for (uint32_t i = 0; i < count; ++i) {
        const uint32_t h = word(0x80126738 + 4 * i);
        if (!guest_window(h, 0x38)) rejected_body("bulk header window");
        const uint32_t id = uint16_t(half(h + 0x2C)), group = uint16_t(half(h + 0x2E));
        if (id >= overlay_ids.size() || overlay_ids[id] < 0 || half(h + 0x30) != int(i)) rejected_body("bulk header ID/index");
        const size_t ordinal = size_t(overlay_ids[id]);
        const auto& section = overlay_sections[ordinal];
        const int n = half(h + 8), ra = half(h + 0xA), rb = half(h + 0xC), bss = half(h + 6);
        const uint32_t flags = uint8_t(MEM_BU(0, int32_t(h + 0xF)));
        if (n <= 0 || ra < 0 || rb < 0 || bss < 0 || (flags & 4) || half(h) < 0 || half(h + 2) < 0 || half(h + 4) < 0 ||
            uint32_t(half(h) + half(h + 2) + half(h + 4)) * 16 != section.size) rejected_body("bulk sizes/counts/protected flags");
        const uint64_t a = (uint64_t(h) + 0x38 + 4 * n + uint8_t(MEM_BU(0, int32_t(h + 0xE))) + 3) & ~uint64_t(3);
        const uint64_t b = (a + 2 * ra + 3) & ~uint64_t(3), text = (b + 4 * rb + 15) & ~uint64_t(15);
        if (!guest_window(h, text - h + section.size + uint32_t(bss) * 16) || word(h + 0x34) != text ||
            section.ram_addr != 0x80800000 || uint32_t(section_addresses[section.index]) != text) rejected_body("bulk prefix/image/live section");
        const uint32_t table = h + 0x38;
        const uint32_t thunk[] = {0x3C090000 | (table >> 16), 0x35290000 | (table & 0xFFFF),
            0x01095020, 0x8D4B0000, 0x01600008, 0xA520FFFA};
        for (unsigned j = 0; j < std::size(thunk); ++j) if (word(h + 0x10 + 4 * j) != thunk[j]) rejected_body("bulk NORMAL thunk");
        for (const auto& candidate : bulk_overlays) if (candidate.h == h || candidate.id == id || candidate.group == group)
            rejected_body("bulk duplicate header/ID/reset group");
        BulkOverlay captured{h, id, uint32_t(text), group, uint32_t((flags & 1) && bss != 0), 0x80082540 + 8 * group, ordinal, {}};
        const uint32_t installed = 0x08000000 | (((h + 0x10) & 0x0FFFFFFF) >> 2);
        uint32_t stub = captured.stub;
        if (stub >= 0x8008A988 || word(stub) != installed) rejected_body("bulk reset group installed J");
        while (stub < 0x8008A988 && word(stub) == installed) {
            captured.delays.push_back(word(stub + 4)); stub += 8;
        }
        if (word(stub) == installed) rejected_body("bulk reset run lacks distinct sentinel");
        bulk_overlays.push_back(std::move(captured));
    }
    unsigned live_heap = 0;
    for (size_t i = 0; i < overlay_sections.size(); ++i) {
        const auto& section = overlay_sections[i];
        const uint32_t live = uint32_t(section_addresses[section.index]);
        if (section.ram_addr != 0x80800000) bulk_residents.emplace_back(i, live);
        else if (live != section.ram_addr) {
            ++live_heap;
            bool captured = false;
            for (const auto& candidate : bulk_overlays) captured |= candidate.ordinal == i;
            if (!captured) rejected_body("bulk live section missing from original list");
        }
    }
    if (live_heap != count) rejected_body("bulk list/live section bijection");
    bulk_started = true;
    std::fprintf(stderr, "baseline bulk-snapshot count=%u residents=%zu tidy_initialized=1 tidy_seed=%u repeat=%u queues_empty=2 SP=%08x map=%08x entrance=%08x\n",
        count, bulk_residents.size(), (thread_status_seen.load() >> 8) & 1, unsigned(repeat), bulk_sp, bulk_map, bulk_entrance);
    std::fflush(stderr);
}
extern "C" void baseline_bulk_returned(uint8_t* rdram, recomp_context* ctx) {
    const auto word = [&](uint32_t a) { return uint32_t(MEM_W(0, int32_t(a))); };
    if (!bulk_started || bulk_completed || PendingOverlay::top || bulk_current != -1 || bulk_admitted != bulk_overlays.size() ||
        uint32_t(ultramodern::this_thread()) != 0x80045788 || osGetThreadId(rdram, ultramodern::this_thread()) != 6 ||
        word(0x800459CC) != 0x18B || uint32_t(ctx->r29) != bulk_sp || word(0x80117C60) || word(0x80126738))
        rejected_body("bulk genuine return/count/list/main admission");
    for (const auto& captured : bulk_overlays) {
        if (!captured.admitted || uint32_t(section_addresses[overlay_sections[captured.ordinal].index]) != 0x80800000)
            rejected_body("bulk returned heap nominal identity");
        for (size_t i = 0; i < captured.delays.size(); ++i)
            if (word(captured.stub + 8 * uint32_t(i)) != ((captured.id << 6) | (captured.reset << 25) | 0xC) ||
                word(captured.stub + 8 * uint32_t(i) + 4) != captured.delays[i]) rejected_body("bulk original reset words/delays");
    }
    for (const auto& resident : bulk_residents) if (uint32_t(section_addresses[overlay_sections[resident.first].index]) != resident.second)
        rejected_body("bulk resident base changed");
    std::fprintf(stderr, "baseline bulk-terminal PC=800a7358 captured=%zu prefree_admitted=%u genuine_batch_return=1 count=0 list0=0 heap_nominal=1 reset_words_delays=1 resident_bases=1 unload_exercised=%u\n",
        bulk_overlays.size(), bulk_admitted, bulk_admitted != 0);
    std::fflush(stderr);
    bulk_completed = true;
}
extern "C" void baseline_resource_preparation(uint8_t* rdram, recomp_context* ctx) {
    const auto word = [&](uint32_t a) { return uint32_t(MEM_W(0, int32_t(a))); };
    const uint32_t sp = uint32_t(ctx->r29);
    if (!bulk_completed || PendingOverlay::top || uint32_t(ultramodern::this_thread()) != 0x80045788 ||
        osGetThreadId(rdram, ultramodern::this_thread()) != 6 || !guest_window(sp, 0x24) ||
        uint64_t(sp) + 0x18 != uint64_t(bulk_sp) ||
        uint32_t(ctx->r4) != 0x80132DC0 || uint32_t(ctx->r5) != word(sp + 0x18) || word(sp + 0x18) != bulk_map ||
        uint32_t(ctx->r6) != word(sp + 0x1C) || word(sp + 0x1C) != bulk_entrance ||
        word(sp + 0x20) || word(0x800459CC) != 0x18B)
        rejected_body("resource preparation original main/bulk/arguments");
    const uint32_t count = word(0x80117C60);
    if (count > 352) rejected_body("resource preparation original list capacity");
    unsigned published = 0;
    for (const auto& section : overlay_sections)
        published += section.ram_addr == 0x80800000 && uint32_t(section_addresses[section.index]) != section.ram_addr;
    for (uint32_t i = 0; i < count; ++i) {
        const uint32_t h = word(0x80126738 + 4 * i);
        if (!guest_window(h, 0x38)) rejected_body("resource preparation current header window");
        const uint32_t id = uint16_t(MEM_H(0, int32_t(h + 0x2C)));
        if (id >= overlay_ids.size() || overlay_ids[id] < 0 || int16_t(MEM_H(0, int32_t(h + 0x30))) != int(i))
            rejected_body("resource preparation current header ID/index");
        const auto& section = overlay_sections[size_t(overlay_ids[id])];
        if (section.ram_addr != 0x80800000 || !guest_window(word(h + 0x34), section.size) ||
            uint32_t(section_addresses[section.index]) != word(h + 0x34)) rejected_body("resource preparation current live mapping");
        for (uint32_t j = 0; j < i; ++j) {
            const uint32_t previous = uint16_t(MEM_H(0, int32_t(word(0x80126738 + 4 * j) + 0x2C)));
            if (overlay_ids[previous] == overlay_ids[id]) rejected_body("resource preparation duplicate section");
        }
    }
    if (published != count) rejected_body("resource preparation list/public mapping bijection");
    std::fprintf(stderr, "baseline resource-preparation-terminal PC=800ea10c count=%u bulk_returned=1 mappings_equal=1 object=%08x map=%08x second=%08x third=%08x map_outputs=%08x,%08x sky_outputs=%08x,%08x SP=%08x preparation_calls_returned=1 world_entered=0\n",
        count, uint32_t(ctx->r4), uint32_t(ctx->r5), uint32_t(ctx->r6), word(sp + 0x20),
        word(0x801285D4), word(0x80128600), word(0x80128788), word(0x8012878C), sp);
    std::fflush(stderr);
    resource_prepared = true;
}
extern "C" void baseline_world_returned(uint8_t* rdram, recomp_context* ctx) {
    if (!bulk_completed || !resource_prepared || PendingOverlay::top || uint32_t(ctx->r29) != bulk_sp ||
        uint32_t(ultramodern::this_thread()) != 0x80045788 || osGetThreadId(rdram, ultramodern::this_thread()) != 6 ||
        uint32_t(MEM_W(0, int32_t(0x800459CC))) != 0x18B)
        rejected_body("world prefix genuine return/main/preparation admission");
    if (!resource_decoded || uint32_t(MEM_W(0, int32_t(0x80043788))) != 0x01020304 ||
        uint32_t(MEM_W(0, int32_t(0x8004378C))) != 0x05060708)
        rejected_body("world prefix decode/original stack guards");
    const uint32_t count = uint32_t(MEM_W(0, int32_t(0x80117C60)));
    if (count > 352) rejected_body("world prefix original list capacity");
    std::fprintf(stderr, "baseline world-prefix-terminal PC=800a7368 count=%u completed=%u bulk_returned=1 prepared=1 world_returned=1 saved=0000018b pending=0 resource_decoded=1 stack_guards=1\n",
        count, alternate_completed);
    std::fflush(stderr);
    world_seen = true;
}

namespace {
void first_sync_main(uint8_t* rdram, recomp_context* ctx, int priority) {
    const auto thread = ultramodern::this_thread();
    if (uint32_t(thread) != 0x80045788 || osGetThreadId(rdram, thread) != 6 || osGetThreadPri(rdram, thread) != priority ||
        !world_seen || PendingOverlay::top || uint32_t(MEM_W(0, int32_t(0x800459CC))) != 0x18B ||
        ctx->status_reg != 0xFF01 || ctx->mips3_float_mode || ctx->f_odd != &ctx->f0.u32h)
        rejected_body("first sync original main/world/saved/pending admission");
}
}
extern "C" void baseline_first_sync_enter(uint8_t* rdram, recomp_context* ctx) {
    first_sync_main(rdram, ctx, 20);
    if (active_sync.load(std::memory_order_acquire) || uint32_t(ctx->r4) != 1 || uint32_t(MEM_W(8, int32_t(0x80079010))) > 1 ||
        uint32_t(MEM_W(12, int32_t(0x80079010))) != 0 || uint32_t(MEM_W(16, int32_t(0x80079010))) != 1 ||
        uint32_t(MEM_W(20, int32_t(0x80079010))) != 0x80079028) rejected_body("first sync entry/stop queue shape");
    first_sync_sp = uint32_t(ctx->r29);
    ++sync_episode;
    tidy_promoted.store(false, std::memory_order_release);
    active_sync.store(true, std::memory_order_release);
    first_sync_entered.store(true, std::memory_order_release);
#if BASELINE_SUCCESS_TRACE
    std::fprintf(stderr, "baseline first-sync-enter PC=800151bc episode=%u SP=%08x tidy_priority=%d tidy_seed=%u stop_count=%u stop_message=%08x\n",
        sync_episode, first_sync_sp, osGetThreadPri(rdram, int32_t(0x80076A20)), (thread_status_seen.load() >> 8) & 1,
        uint32_t(MEM_W(8, int32_t(0x80079010))), uint32_t(MEM_W(0, int32_t(0x80079028))));
    std::fflush(stderr);
#endif
}
extern "C" void baseline_first_sync_promotion(uint8_t* rdram, recomp_context* ctx) {
    first_sync_main(rdram, ctx, 127);
    const uint32_t gate = uint32_t(MEM_W(0, int32_t(0x80043380)));
    const int priority = osGetThreadPri(rdram, int32_t(0x80076A20));
    if (!active_sync.load(std::memory_order_acquire) || uint64_t(uint32_t(ctx->r29)) + 0x20 != uint64_t(first_sync_sp) ||
        (gate == 1 && (osGetThreadId(rdram, int32_t(0x80076A20)) != 2 || priority != 35)))
        rejected_body("first sync original tidy promotion");
    if (gate == 1) tidy_promoted.store(true, std::memory_order_release);
#if BASELINE_SUCCESS_TRACE
    std::fprintf(stderr, "baseline first-sync-promotion PC=800151e4 episode=%u gate=%u tidy_priority=%d promoted=%u\n", sync_episode, gate, priority, gate == 1);
    std::fflush(stderr);
#endif
}
extern "C" void baseline_first_sync_returned(uint8_t* rdram, recomp_context* ctx) {
    first_sync_main(rdram, ctx, 20);
    if (!active_sync.load(std::memory_order_acquire) || uint64_t(uint32_t(ctx->r29)) != uint64_t(first_sync_sp) + 0x18 ||
        uint32_t(MEM_W(0, int32_t(0x80126CBC))) != 0 ||
        uint32_t(MEM_W(0, int32_t(0x80043788))) != 0x01020304 || uint32_t(MEM_W(0, int32_t(0x8004378C))) != 0x05060708)
        rejected_body("first sync genuine return/stack guards");
#if BASELINE_SUCCESS_TRACE
    const auto* tidy = TO_PTR(OSThread, int32_t(0x80076A20));
    std::fprintf(stderr, "baseline sync-return PC=800a78b0 episode=%u task_processed=%u SP=%08x counter=%u VI_returns=%u outbound=%u head=%u framebuffer=%08x next_field=%08x tidy_priority=%d tidy_state=%u tidy_queue=%08x stop_count=%u busy=%u promoted=%u tidy_ran=%u handler_returned=1 stack_guards=1\n",
        sync_episode, unsigned(baseline::first_graphics_processed()), uint32_t(ctx->r29), uint32_t(MEM_W(0, int32_t(0x80079138))), vi_worker_returns.load(),
        uint32_t(MEM_W(8, int32_t(0x80079030))), uint32_t(MEM_W(12, int32_t(0x80079030))),
        uint32_t(MEM_W(0, int32_t(0x80079704))), uint32_t(MEM_W(0, int32_t(0x80078F64))), tidy->priority, tidy->state, uint32_t(tidy->queue),
        uint32_t(MEM_W(8, int32_t(0x80076A00))), uint32_t(MEM_W(0, int32_t(0x80126CBC))), tidy_promoted.load(), uint32_t(MEM_W(0, int32_t(0x80127650))));
    std::fflush(stderr);
#endif
    active_sync.store(false, std::memory_order_release);
    tidy_promoted.store(false, std::memory_order_release);
}

extern "C" void baseline_first_graphics_submit(uint8_t* rdram, recomp_context* ctx) {
    if (first_graphics_seen) return;
    first_sync_main(rdram, ctx, 20);
    const uint32_t sp = uint32_t(ctx->r29), start = uint32_t(ctx->r4);
    if (active_sync.load(std::memory_order_acquire) || !sync_episode || uint64_t(sp) != uint64_t(first_sync_sp) + 0x18 || !guest_window(sp, 0x20))
        rejected_body("first graphics original main/sync/stack admission");
    const uint32_t end = uint32_t(MEM_W(0x18, int32_t(sp)));
    const int index = MEM_H(0, int32_t(0x8007BEDA));
    const bool override_pool = MEM_W(0, int32_t(0x8007BCE8)) != 0;
    if (index < 0 || index > 1) rejected_body("first graphics pool index");
    const uint32_t pool = uint32_t(MEM_W(index * 4, int32_t(override_pool ? 0x8007BCE8 : 0x8007BCD0)));
#if BASELINE_CAMERA_IDENTITY
    const uint64_t camera_slot = uint64_t(TOOIE_CAMERA_MAIN_VRAM) + uint64_t(index) * TOOIE_CAMERA_M_BYTES;
    const bool camera_main = !override_pool && start == camera_slot && uint64_t(end) <= camera_slot + TOOIE_CAMERA_M_BYTES;
#else
    constexpr bool camera_main = false;
#endif
    const uint64_t size = uint64_t(end) - uint64_t(start);
    // This is the selected allocation bound, not evidence of an original overflow.
    constexpr uint32_t allocation = (BASELINE_HONEY_PATCH || BASELINE_RENDER_BRIDGE) ? 0x8FC0 : 0x76C0;
    if (start != pool || uint32_t(MEM_W(0x1C, int32_t(sp))) != start || end < start || ((start | end) & 7) ||
        (!camera_main && (!guest_window(start, size) || (!override_pool && size > allocation))))
        rejected_body("first graphics selected allocation/pool extent");
    first_graphics_seen = true;
    std::fprintf(stderr, "baseline graphics-submit PC=800a78e8 episode=%u start=%08x end=%08x size=%u pool_index=%d override=%u\n",
        sync_episode, start, end, unsigned(size), index, unsigned(override_pool));
    std::fflush(stderr);
    baseline::expect_first_graphics(start & 0x3FFFFFF, uint32_t(size));
}

#endif
#ifdef BASELINE_IMPORT___osGetSR_recomp
extern "C" void __osGetSR_recomp(uint8_t*, recomp_context*) { missing_import("__osGetSR_recomp"); }
#endif
#ifdef BASELINE_IMPORT___osSetSR_recomp
extern "C" void __osSetSR_recomp(uint8_t*, recomp_context*) { missing_import("__osSetSR_recomp"); }
#endif
#ifdef BASELINE_IMPORT___osSiRawReadIo_recomp
extern "C" void __osSiRawReadIo_recomp(uint8_t*, recomp_context*) { missing_import("__osSiRawReadIo_recomp"); }
#endif
#ifdef BASELINE_IMPORT___osSiRawWriteIo_recomp
extern "C" void __osSiRawWriteIo_recomp(uint8_t*, recomp_context*) { missing_import("__osSiRawWriteIo_recomp"); }
#endif
#ifdef BASELINE_IMPORT_osMapTLBRdb_recomp
extern "C" void osMapTLBRdb_recomp(uint8_t*, recomp_context*) { missing_import("osMapTLBRdb_recomp"); }
#endif
#ifdef BASELINE_IMPORT___osGetCause_recomp
extern "C" void __osGetCause_recomp(uint8_t*, recomp_context*) { missing_import("__osGetCause_recomp"); }
#endif
#ifdef BASELINE_VI_CONTROLLER_CYCLE
extern "C" void __osSiGetAccess_recomp(uint8_t* rdram, recomp_context* ctx) {
    resource_si_scope(rdram);
    if (!resource_decoded && !resource_si_commits && MEM_W(0, int32_t(0x800414A0)) != 0)
        rejected_body("resource SI initial access enabled");
    resource_si_call(rdram, ctx, tooie_si_get_access);
}
extern "C" void __osSiRawStartDma_recomp(uint8_t* rdram, recomp_context* ctx) {
    resource_si_scope(rdram);
    const uint32_t direction = uint32_t(ctx->r4);
    if (resource_si_commits >= 2 || direction != (resource_si_commits ? 0u : 1u) || uint32_t(ctx->r5) != 0x8007D850 ||
        MEM_W(0, int32_t(0x80079BA0)) != 5 || MEM_W(0, int32_t(0x80079F78)) || MEM_W(0, int32_t(0x80079B80)))
        rejected_body("resource SI ordered DMA/buffer/queue admission");
    resource_si_call(rdram, ctx, tooie_si_raw_start_dma);
    ++resource_si_commits;
    if (!direction) for (size_t i = 0; i < resource_si_response.size(); ++i)
        resource_si_response[i] = MEM_BU(0, int32_t(0x8007D880 + i));
    std::fprintf(stderr, "baseline resource-SI-commit direction=%u commits=%u\n", direction, resource_si_commits);
    std::fflush(stderr);
}
extern "C" void __osSiRelAccess_recomp(uint8_t* rdram, recomp_context* ctx) { resource_si_call(rdram, ctx, tooie_si_rel_access); }
extern "C" void baseline_resource_cic_returned(uint8_t* rdram, recomp_context* ctx) {
    resource_si_scope(rdram);
    if (uint64_t(uint32_t(ctx->r29)) < uint64_t(0x80043790) + 0xA70)
        rejected_body("resource forthcoming fixed decoder stack interval");
    const uint64_t response = uint64_t(uint32_t(ctx->r29)) + 0x28;
    if (resource_si_commits != 2 || MEM_W(0, int32_t(0x80079B80)) || response > UINT32_MAX || !guest_window(uint32_t(response), 16))
        rejected_body("resource CIC helper return/queue/stack window");
    for (size_t i = 0; i < resource_si_response.size(); ++i)
        if (MEM_BU(0, int32_t(response + i)) != resource_si_response[i]) rejected_body("resource CIC original response copy");
    char response_hex[33]{};
    constexpr char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < resource_si_response.size(); ++i) {
        response_hex[2 * i] = digits[resource_si_response[i] >> 4];
        response_hex[2 * i + 1] = digits[resource_si_response[i] & 15];
    }
    std::fprintf(stderr, "baseline resource-CIC-helper-return pc=80800100 commits=2 response_equal=1 sp=%08x F70_count=%u BA0=%u response=%s\n",
        uint32_t(ctx->r29), uint32_t(MEM_W(0, int32_t(0x80079F78))), uint32_t(MEM_W(0, int32_t(0x80079BA0))), response_hex);
    std::fflush(stderr);
}
#else
#ifdef BASELINE_IMPORT___osSiGetAccess_recomp
extern "C" void __osSiGetAccess_recomp(uint8_t*, recomp_context*) { missing_import("__osSiGetAccess_recomp"); }
#endif
#ifdef BASELINE_IMPORT___osSiRawStartDma_recomp
extern "C" void __osSiRawStartDma_recomp(uint8_t*, recomp_context*) { missing_import("__osSiRawStartDma_recomp"); }
#endif
#ifdef BASELINE_IMPORT___osSiRelAccess_recomp
extern "C" void __osSiRelAccess_recomp(uint8_t*, recomp_context*) { missing_import("__osSiRelAccess_recomp"); }
#endif
#endif
#ifdef BASELINE_IMPORT_osViGetCurrentLine_recomp
extern "C" void osViGetCurrentLine_recomp(uint8_t*, recomp_context*) { missing_import("osViGetCurrentLine_recomp"); }
#endif
#ifdef BASELINE_VI_CONTROLLER_CYCLE
// Scoped native no-pak policy; no raw PIF/PFS equivalence or accessory success.
extern "C" void osPfsInit_recomp(uint8_t* rdram, recomp_context* ctx) {
    const auto thread = ultramodern::this_thread();
    if (uint32_t(thread) != 0x80045788 || osGetThreadId(rdram, thread) != 6 || pfs_absence_calls >= 4 ||
        uint32_t(ctx->r6) != pfs_absence_calls || uint32_t(ctx->r4) != 0x80079B78 || uint32_t(ctx->r5) != 0x8007B2EC + 0x84 * pfs_absence_calls)
        rejected_body("input-prefix ordered main no-pak request admission");
    osPfsInitPak_recomp(rdram, ctx);
    std::fprintf(stderr, "baseline input-prefix no-pak channel=%u native_no_pak_policy=1\n", pfs_absence_calls++);
    std::fflush(stderr);
}
#elif defined(BASELINE_IMPORT_osPfsInit_recomp)
extern "C" void osPfsInit_recomp(uint8_t*, recomp_context*) { missing_import("osPfsInit_recomp"); }
#endif
