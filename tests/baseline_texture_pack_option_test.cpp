// HD-option parity for texture packs (texture_pack_option.hpp) against RecompFrontend's enable_texture_pack:
// only a valid declared "_recomp_texture_pack_enabled" option with a zero or unset value holds a pack back, and the
// value is read only for a valid option. Synthetic schemas only; no Runtime context, renderer or packs.
#include "../tools/baseline/texture_pack_option.hpp"
#include <cassert>
#include <cstdio>
#include <string>
#include <utility>
using namespace recomp::config;
using baseline::texture_pack_option::allows;
using baseline::texture_pack_option::id;
namespace {
ConfigSchema with(ConfigOption option) {
    ConfigSchema schema;
    schema.options_by_id.emplace(option.id, schema.options.size());
    schema.options.push_back(std::move(option));
    return schema;
}
ConfigOption option(ConfigOptionType type, ConfigOptionVariant variant, std::string name = id) {
    ConfigOption result;
    result.id = std::move(name); result.type = type; result.variant = std::move(variant);
    return result;
}
ConfigOptionEnum choices(size_t count) {
    ConfigOptionEnum result;
    for (uint32_t i = 0; i < count; ++i) result.options.emplace_back(i, "choice" + std::to_string(i));
    return result;
}
// Returns the decision and how many times the value was read.
std::pair<bool, int> decide(const ConfigSchema& schema, ConfigValueVariant value) {
    int reads = 0;
    const bool result = allows(schema, [&](const std::string& option_id) {
        assert(option_id == id); ++reads; return value; });
    return {result, reads};
}
}
int main() {
    assert(id == "_recomp_texture_pack_enabled"); // recompui renderer.h at the locked RecompFrontend commit
    // No option, or another id: applied, value never read.
    assert(decide(ConfigSchema{}, false) == std::make_pair(true, 0));
    assert(decide(with(option(ConfigOptionType::Bool, ConfigOptionBool{false}, "other")), false) == std::make_pair(true, 0));
    // Bool option: its value decides; unset counts as off, like the reference's zero default.
    const auto flag = with(option(ConfigOptionType::Bool, ConfigOptionBool{true}));
    assert(decide(flag, true) == std::make_pair(true, 1));
    assert(decide(flag, false) == std::make_pair(false, 1));
    assert(decide(flag, std::monostate{}) == std::make_pair(false, 1));
    // Two-value enum: a nonzero stored value applies the pack, zero or another value type holds it back.
    const auto two = with(option(ConfigOptionType::Enum, choices(2)));
    assert(decide(two, uint32_t(1)) == std::make_pair(true, 1));
    assert(decide(two, uint32_t(7)) == std::make_pair(true, 1));
    assert(decide(two, uint32_t(0)) == std::make_pair(false, 1));
    assert(decide(two, true) == std::make_pair(false, 1));
    // Invalid declarations are not the option: applied, value never read.
    assert(decide(with(option(ConfigOptionType::Enum, choices(3))), uint32_t(0)) == std::make_pair(true, 0));
    assert(decide(with(option(ConfigOptionType::Enum, choices(1))), uint32_t(0)) == std::make_pair(true, 0));
    assert(decide(with(option(ConfigOptionType::Number, ConfigOptionNumber{})), 0.0) == std::make_pair(true, 0));
    assert(decide(with(option(ConfigOptionType::String, ConfigOptionString{})), std::string()) == std::make_pair(true, 0));
    assert(decide(with(option(ConfigOptionType::Enum, ConfigOptionBool{false})), uint32_t(0)) == std::make_pair(true, 0));
    // An index past the option list is ignored rather than read.
    ConfigSchema stale; stale.options_by_id.emplace(id, 3);
    assert(decide(stale, false) == std::make_pair(true, 0));
    std::printf("baseline_texture_pack_option: ok\n");
    return 0;
}
