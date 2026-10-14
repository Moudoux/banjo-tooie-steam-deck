#pragma once
#include "librecomp/rsp.hpp"
#include "json/json.hpp"
#include <functional>
#include <string>

namespace tooie::audio_rsp {
using Log = std::function<void(const char*, const nlohmann::json&)>;
using Failure = std::function<void(const std::string&)>;
struct Recognition {
    bool supported;
    std::string reason;
};
// Read-only verification against original-ROM-derived size/guest-order SHA-256 identities.
// rdram points to the runtime's word-swapped, original 8 MiB accessible memory.
Recognition recognize_task(const uint8_t* rdram, const OSTask* task);
// A pure description of the two translated IMEM permutations; -1 rejects an
// unsupported destination/source/DMA length. Does not execute a microcode task.
int overlay_index(uint32_t imem, uint32_t source_offset, uint32_t dma_length) noexcept;
// Configure while runtime workers are stopped. Failure reports never post guest
// events. Stock run_task rejects shape before DMA; wrapper failures report/throw.
void configure(Log log, Failure failure = {}, bool detailed_success_logging = true);
recomp::rsp::callbacks_t callbacks();
}
