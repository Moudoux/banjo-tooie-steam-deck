#pragma once
// Player text for a refused or failed texture-pack start. Runtime's mod manifest/order service and RT64's
// public texture cache stay the only pack owners; this words the existing fail-closed refusals.
#include <filesystem>
#include <string>
#include <string_view>

namespace baseline::texture_pack_notice {
// First line of Runtime's own load-failure report (librecomp recomp.cpp wait_for_game_started at the
// pinned commit, whose bytes Baseline.cmake hash-checks).
inline constexpr std::string_view runtime_mod_load_report = "Error loading mods:";

inline std::string path_text(const std::filesystem::path& path) {
    const auto text = path.u8string();
    return {reinterpret_cast<const char*>(text.data()), text.size()};
}

// A mod that Runtime cannot toggle at run time holds code or a ROM patch. Required means Runtime
// enabled it because another enabled mod depends on it.
inline std::string refusal(const std::string& name, const std::string& file, const std::string& folder,
    bool required) {
    return "Only texture packs are supported, so the game did not start.\n\n\"" + name + "\" (" + file +
        ") contains a code mod or ROM patch" + (required ? ", and another installed mod requires it" : "") +
        ".\n\nMove " + (required ? "it and the mods that require it" : "it") +
        " out of this folder, then start the game again:\n" + folder;
}

// Runtime's load-failure report gains the steps after it; any other report is returned unchanged.
inline std::string with_mod_load_recovery(const std::string& report, const std::string& folder) {
    if (!report.starts_with(runtime_mod_load_report)) return report;
    return report + "\nThe game did not start. Move the mods named above out of this folder, then start the game "
        "again:\n" + folder;
}
}
