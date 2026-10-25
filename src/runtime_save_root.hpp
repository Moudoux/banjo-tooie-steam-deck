#pragma once

#include <cstdint>
#include <filesystem>

namespace tooie {
void register_save_root(std::filesystem::path root);
std::filesystem::path runtime_save_folder(const std::filesystem::path& config_root);
// Promotes the saver's closed <save>.temp, which must hold exactly expected_size bytes and
// is OS-flushed first. Only a current main of exactly expected_size refreshes <save>.bak,
// through <save>.bak.temp; otherwise the existing backup is left as it is. Then <save>.temp
// is renamed over <save>. Main is never written in place and there is no copy fallback.
// Size is not game-level validation; power-loss durability is not claimed.
bool promote_save_file(const std::filesystem::path& save, std::uintmax_t expected_size) noexcept;
}
