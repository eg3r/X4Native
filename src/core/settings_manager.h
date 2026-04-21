#pragma once
// ---------------------------------------------------------------------------
// x4native_core.dll — Settings Manager
//
// Per-extension user-configurable settings.
//
// Each native extension may declare a "settings" array in its x4native.json.
// Values picked up by the UI (via proxy) and by the extension itself (via
// X4NativeAPI::get_setting_*). Current values persist in
//   <profile>\x4native\<extension_id>.user.json
// where <profile> is `GetSaveFolderPath()`'s parent.
// ---------------------------------------------------------------------------

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "logger.h"

#include "x4native_defs.h"

namespace x4n {

enum class SettingType { Toggle, Dropdown, Slider };

struct SettingOption {
    std::string id;
    std::string text;
};

struct SettingSchema {
    std::string id;
    std::string name;
    SettingType type = SettingType::Toggle;

    // Defaults (only the matching field is meaningful, but all are zeroed).
    bool        default_bool   = false;
    double      default_number = 0.0;
    std::string default_string;

    // Dropdown
    std::vector<SettingOption> options;

    // Slider
    double min  = 0.0;
    double max  = 100.0;
    double step = 1.0;
};

using SettingValue = std::variant<bool, double, std::string>;

// Per-extension container: schema + current values + ABI-shaped cache.
struct ExtensionSettings {
    std::vector<SettingSchema> schema;
    std::unordered_map<std::string, SettingValue> values;
    bool values_loaded = false;

    // Persistent per-option string storage for the ABI cache.
    std::vector<std::vector<SettingOptionC>> options_cache;
    // Mirrors `schema` in the ABI form. Refreshed on every enumerate().
    std::vector<SettingInfo> abi_cache;
    bool abi_cache_built = false;
};

class SettingsManager {
public:
    // Called from ExtensionManager. No-op if the extension has no settings.
    static void register_extension(const std::string& extension_id,
                                   std::vector<SettingSchema> schema);
    static void unregister_extension(const std::string& extension_id);

    // Clears everything (on core shutdown / reload).
    static void shutdown();

    // Read-side.
    static bool        has_key(const std::string& extension_id, const std::string& key);
    static SettingType type_of(const std::string& extension_id, const std::string& key);
    static bool        get_bool  (const std::string& extension_id, const std::string& key, bool fallback);
    static double      get_number(const std::string& extension_id, const std::string& key, double fallback);
    static const char* get_string(const std::string& extension_id, const std::string& key, const char* fallback);

    // Write-side. Fires `on_setting_changed` via EventSystem on change.
    // Payload struct: X4NativeSettingChanged (see below).
    static void set_bool  (const std::string& extension_id, const std::string& key, bool value);
    static void set_number(const std::string& extension_id, const std::string& key, double value);
    static void set_string(const std::string& extension_id, const std::string& key, const char* value);

    // ABI-facing (used by proxy via CoreDispatch).
    static int  enumerate(const std::string& extension_id, const SettingInfo** out);
    static void set_from_abi(const std::string& extension_id, const std::string& key,
                             const SettingValueC& value);

    // JSON helpers for parse_config / loaded_extensions_json.
    // Returns true if `node` held a valid array; empty array → ok, returns true.
    //
    // `warnings` (optional): per-extension buffer that collects parse-time
    // diagnostics. When provided, issues are pushed here instead of (or in
    // addition to) the framework logger — the caller flushes these into the
    // extension's own log file once it's open. Pass nullptr for the legacy
    // "log straight to framework log" behaviour.
    static bool parse_schema_array(
        const nlohmann::json& node,
        std::vector<SettingSchema>& out,
        const std::string& context,
        std::vector<std::pair<LogLevel, std::string>>* warnings = nullptr);

private:
    using Map = std::unordered_map<std::string, ExtensionSettings>;
    static Map s_map;

    static ExtensionSettings* find(const std::string& extension_id);
    static const SettingSchema* schema_for(const ExtensionSettings& ext, const std::string& key);

    // <profile>\x4native\<extension_id>.user.json — returns empty path on failure.
    static std::string user_file_path(const std::string& extension_id);

    static void ensure_loaded(const std::string& extension_id, ExtensionSettings& ext);
    static void write_user_file(const std::string& extension_id, const ExtensionSettings& ext);

    static void rebuild_abi_cache(ExtensionSettings& ext);
    static void refresh_abi_current_values(ExtensionSettings& ext);

    // Fires `on_setting_changed` with a pointer to X4NativeSettingChanged.
    static void fire_change(const std::string& extension_id,
                            const std::string& key,
                            const SettingValue& new_value);
};

// X4NativeSettingChanged is declared in <x4native_extension.h> (SDK header).

} // namespace x4n
