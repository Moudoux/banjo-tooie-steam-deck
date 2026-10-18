#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace tooie::frontend::settings {
void initialize(const std::filesystem::path& config_root);
// Explicit callers attempt immediately; periodic callers back off after failed I/O.
bool flush(bool periodic = false);
// Static notice for an unreadable primary (writes blocked) or transient write failure.
std::string_view recovery_notice();
#ifdef TOOIE_PERSISTENT_RUNTIME_EXPERIMENT
// Conservative compatibility input, read-only and never flushed to disk here.
std::string persistent_identity_payload();
#endif
bool get_bool(std::string_view id, bool fallback = false);
int get_int(std::string_view id, int fallback = 0);
double get_number(std::string_view id, double fallback = 0.0);
std::string get_string(std::string_view id, std::string_view fallback = {});
void set_bool(std::string_view id, bool value);
void set_int(std::string_view id, int value);
void set_number(std::string_view id, double value);
void set_string(std::string_view id, std::string value);
}
