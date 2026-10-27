#pragma once
// Load-time parity with RecompFrontend's renderer::enable_texture_pack and is_texture_pack_enable_config_option
// (recompui renderer.h special_option_texture_pack_enabled). A pack whose config schema declares that option as a
// two-value enum or a bool is applied only while its value is nonzero; any other schema leaves the pack applied.
// There is no pack menu or live toggle, so the value is the mod's stored config or the schema default.
#include "librecomp/config.hpp"
#include <cstdint>
#include <string>
#include <variant>

namespace baseline::texture_pack_option {
inline const std::string id = "_recomp_texture_pack_enabled";

// The reference check without its error dialogs. A type/variant mismatch, which Runtime's manifest parser does
// not produce, is treated as no option instead of throwing inside Runtime's enable callback.
inline bool valid(const recomp::config::ConfigOption& option) {
    using Type = recomp::config::ConfigOptionType;
    if (option.id != id) return false;
    if (option.type == Type::Bool) return true;
    const auto* values = std::get_if<recomp::config::ConfigOptionEnum>(&option.variant);
    return option.type == Type::Enum && values && values->options.size() == 2;
}

// False only when a valid declared option's value is zero or unset; value() is read only for a valid option.
template <class Value>
bool allows(const recomp::config::ConfigSchema& schema, Value&& value) {
    const auto found = schema.options_by_id.find(id);
    if (found == schema.options_by_id.end() || found->second >= schema.options.size()) return true;
    const auto& option = schema.options[found->second];
    if (!valid(option)) return true;
    const recomp::config::ConfigValueVariant current = value(option.id);
    if (option.type == recomp::config::ConfigOptionType::Enum) {
        const auto* number = std::get_if<uint32_t>(&current);
        return number && *number != 0;
    }
    const auto* flag = std::get_if<bool>(&current);
    return flag && *flag;
}
}
