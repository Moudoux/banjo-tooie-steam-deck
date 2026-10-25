#include "autosave.hpp"

#include "imgui/imgui.h"
#include "save_persistence.hpp"
#include "ultramodern/ultramodern.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <system_error>
#include <vector>
#ifndef _WIN32
#include <unistd.h>
#endif

#include "funcs.h"

namespace fs = std::filesystem;

namespace {
using Clock = std::chrono::steady_clock;
// 16 Kbit EEPROM, the only size the runtime writes (player_startup.hpp save_bytes).
constexpr std::uintmax_t k_save_bytes = 2048;
constexpr char k_backup_suffix[] = ".autosave.bak";
// Controlled character: index and PlayerState table (as src/levitate.cpp).
constexpr std::uint32_t k_controlled = 0x801354DCU, k_players = 0x80135490U;
// D_8012B3F0, the loaded game file as func_800D389C reads it: +1 the slot
// (-1 for none), +9 a state that is 0 during play and set while a file is
// copied out on Save & Exit.
constexpr std::uint32_t k_file = 0x8012B3F0U;
// While the game refuses to save (some worlds, no file loaded), ask again after this.
constexpr auto k_retry = std::chrono::seconds(30);
// "Saving..." stays up at least this long so it can be read.
constexpr auto k_shown = std::chrono::milliseconds(1500);

std::atomic<int> interval{0};
// Clock ticks when the last autosave started (0: none yet), and the save
// generation it produced; the text stays up until that generation is on disk.
std::atomic<Clock::rep> shown_at{0};
std::atomic<std::uint64_t> shown_generation{0};

// Guest thread only.
Clock::time_point due{};
int armed = -1;
std::uint64_t seen = 0;

gpr guest(std::uint32_t address) noexcept { return static_cast<gpr>(static_cast<std::int32_t>(address)); }
// As camera_analog.cpp: callee frames go below the hook's stack pointer and
// the whole context is restored, so the hooked function continues unchanged.
template <typename Call>
void guest_call(recomp_context* ctx, Call&& call) {
    const auto saved = *ctx;
    try {
        ctx->r29 -= 0x60;
        call();
        *ctx = saved;
    } catch (...) {
        *ctx = saved;
        throw;
    }
}
bool settled(std::uint64_t generation) noexcept {
    using namespace tooie::save_persistence;
    return generation == 0 || complete(generation) || unsuccessful(generation);
}
fs::path backup_path(const fs::path& save) {
    auto path = save;
    path += k_backup_suffix;
    return path;
}
// Copies the save beside itself through a temporary file, so a crash leaves
// either the previous backup or the new one. No save yet is nothing to keep;
// a save of the wrong size never replaces the backup.
bool back_up(const fs::path& save) {
    std::error_code error;
    const auto size = fs::file_size(save, error);
    if (error) return !fs::exists(save, error) && !error;
    if (size != k_save_bytes) return true;
    std::vector<char> bytes(k_save_bytes);
    std::FILE* in = std::fopen(save.string().c_str(), "rb");
    if (!in) return false;
    const bool read = std::fread(bytes.data(), 1, bytes.size(), in) == bytes.size();
    std::fclose(in);
    if (!read) return false;
    const auto backup = backup_path(save);
    auto temp = backup;
    temp += ".temp";
    std::FILE* out = std::fopen(temp.string().c_str(), "wb");
    if (!out) return false;
    bool written = std::fwrite(bytes.data(), 1, bytes.size(), out) == bytes.size() && std::fflush(out) == 0;
#ifndef _WIN32
    written = written && ::fsync(::fileno(out)) == 0;
#endif
    written = std::fclose(out) == 0 && written;
    if (written) fs::rename(temp, backup, error);
    if (!written || error) {
        fs::remove(temp, error);
        return false;
    }
    return true;
}
}

namespace tooie::autosave {

void set_interval_minutes(int minutes) noexcept { interval.store(minutes, std::memory_order_release); }

void recover(const fs::path& save) {
    std::error_code error;
    const auto size = fs::file_size(save, error);
    if (error || size == k_save_bytes) return; // No save (a fresh start), or a whole one.
    const auto backup = backup_path(save);
    const auto backup_size = fs::file_size(backup, error);
    if (error || backup_size != k_save_bytes) {
        std::fprintf(stderr, "player autosave: %s is %ju bytes and there is no usable backup; left unchanged\n",
            save.string().c_str(), size);
        return;
    }
    fs::copy_file(backup, save, fs::copy_options::overwrite_existing, error);
    std::fprintf(stderr, "player autosave: %s was %ju bytes; %s\n", save.string().c_str(), size,
        error ? ("restoring the backup failed (" + error.message() + ")").c_str() : "restored the autosave backup");
}

void render() {
    const auto started = shown_at.load(std::memory_order_acquire);
    if (!started) return;
    const auto since = Clock::now() - Clock::time_point(Clock::duration(started));
    if (since >= k_shown && settled(shown_generation.load(std::memory_order_acquire))) return;
    const auto* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 12.f,
        viewport->WorkPos.y + viewport->WorkSize.y - 12.f), ImGuiCond_Always, ImVec2(1, 1));
    if (ImGui::Begin("##autosave", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings))
        ImGui::TextUnformatted("Saving...");
    ImGui::End();
}

} // namespace tooie::autosave

extern "C" void tooie_autosave_update(std::uint8_t* rdram, recomp_context* ctx) {
    // bainput_update can run once per character; count time once per frame.
    const auto controlled = static_cast<std::uint32_t>(MEM_W(0, guest(k_controlled)));
    if (controlled >= 2U) return;
    if (ctx->r4 != static_cast<gpr>(MEM_W(0, guest(k_players + controlled * 4U)))) return;
    const auto now = Clock::now();
    const int minutes = interval.load(std::memory_order_acquire);
    const auto generation = tooie::save_persistence::generation();
    if (minutes <= 0 || minutes != armed || generation != seen) {
        // Off, a new interval, or the game just saved (a map change or the
        // pause menu): a full interval from here.
        armed = minutes; seen = generation;
        due = now + std::chrono::minutes(minutes);
        return;
    }
    if (now < due || !settled(generation)) return; // Not yet, or a save is still going to disk.
    if (MEM_B(1, guest(k_file)) == -1 || MEM_BU(9, guest(k_file)) != 0) { due = now + k_retry; return; }
    int allowed = 0;
    guest_call(ctx, [&] { func_800D3770(rdram, ctx); allowed = static_cast<int>(ctx->r2); });
    if (!allowed) { due = now + k_retry; return; }
    const auto save = ultramodern::get_save_file_path();
    if (!back_up(save)) {
        std::fprintf(stderr, "player autosave skipped: %s could not be backed up\n", save.string().c_str());
        due = now + std::chrono::minutes(minutes);
        return;
    }
    shown_generation.store(generation, std::memory_order_release);
    shown_at.store(now.time_since_epoch().count(), std::memory_order_release);
    // The save Save & Exit and map changes make; its EEPROM writes reach the
    // runtime's save writer, which replaces the file through its own temp file.
    guest_call(ctx, [&] { func_800D389C(rdram, ctx); });
    seen = tooie::save_persistence::generation();
    shown_generation.store(seen, std::memory_order_release);
    due = now + std::chrono::minutes(minutes);
    std::fprintf(stderr, "player autosave slot=%d generation=%llu\n", static_cast<int>(MEM_B(1, guest(k_file))),
        static_cast<unsigned long long>(seen));
}
