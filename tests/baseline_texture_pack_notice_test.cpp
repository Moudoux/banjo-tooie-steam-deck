// Texture-pack start refusal and mod-load recovery text (texture_pack_notice.hpp): names the mod, its file and the
// folder to move it out of, says the game did not start, mentions dependents only for a required mod, and extends
// only Runtime's own load-failure report. Synthetic strings only; no runtime, renderer, packs or profile.
#include "../tools/baseline/texture_pack_notice.hpp"
#include <cassert>
#include <cstdio>
#include <string>
using namespace baseline::texture_pack_notice;
namespace {
bool has(const std::string& text, std::string_view part) { return text.find(part) != std::string::npos; }
}
int main() {
    const std::string folder = "D:\\Profile\\mods";
    // A directly enabled code mod or ROM patch: the refusal names it and its file, then the folder.
    const auto direct = refusal("Faster Swim", "faster_swim.nrm", folder, false);
    assert(direct.starts_with("Only texture packs are supported, so the game did not start.\n\n"));
    assert(has(direct, "\"Faster Swim\" (faster_swim.nrm) contains a code mod or ROM patch.\n\n"));
    assert(direct.ends_with("Move it out of this folder, then start the game again:\n" + folder));
    assert(!has(direct, "require"));
    // A required dependency: moving it alone would leave its dependents failing Runtime's load checks.
    const auto required = refusal("Shared Code", "shared-code", folder, true);
    assert(has(required, "contains a code mod or ROM patch, and another installed mod requires it.\n\n"));
    assert(has(required, "Move it and the mods that require it out of this folder"));
    // Runtime's report (one line per failed mod) is kept and gains the steps after it.
    const std::string report = "Error loading mods:\n\nPack A: synthetic reason (9.0.0)\n";
    const auto recovered = with_mod_load_recovery(report, folder);
    assert(recovered.starts_with(report));
    assert(recovered.ends_with("\nThe game did not start. Move the mods named above out of this folder, then start "
        "the game again:\n" + folder));
    // Any other retained report, a near miss, or none is unchanged.
    assert(with_mod_load_recovery("Terminal guest fence timed out", folder) == "Terminal guest fence timed out");
    assert(with_mod_load_recovery("Error loading", folder) == "Error loading");
    assert(with_mod_load_recovery("", folder).empty());
    // UTF-8 path text, including a non-ASCII profile folder.
    assert(path_text(std::filesystem::path(u8"D:/Fixtures/Zo\u00EB/mods")) == "D:/Fixtures/Zo\xC3\xAB/mods");
    std::printf("baseline_texture_pack_notice: ok\n");
    return 0;
}
