#pragma once
// Pinned guest-memory identity: hash guest-order bytes (MEM_BU), never host-swizzled RDRAM words.
#include <cstdint>
#include <string>
#include <vector>
#include "recomp.h"
#include "platform_support.hpp"

namespace baseline {
inline std::string guest_range_sha256(uint8_t* rdram, uint32_t address, uint32_t bytes) {
    std::vector<uint8_t> data(bytes);
    const gpr start = static_cast<gpr>(static_cast<int32_t>(address));
    for (uint32_t i = 0; i < bytes; ++i) data[i] = uint8_t(MEM_BU(i, start));
    return tooie::platform::digest(data, true);
}
}
