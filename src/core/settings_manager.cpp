#include "settings_manager.h"
#include "event_system.h"
#include "logger.h"

#include <filesystem>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace x4n {

SettingsManager::Map SettingsManager::s_map;

// ---------------------------------------------------------------------------
// Registration / lifecycle
// ---------------------------------------------------------------------------

void SettingsManager::register_extension(const std::string& extension_id,
                                         std::vector<SettingSchema> schema) {
    if (schema.empty()) return;
    auto& ext = s_map[extension_id];
    ext.schema = std::move(schema);
    ext.values.clear();
    ext.values_loaded = false;
    ext.abi_cache_built = false;
    Logger::info("Settings: registered {} setting(s) for extension '{}'",
                 ext.schema.size(), extension_id);
}

void SettingsManager::unregister_extension(const std::string& extension_id) {
    s_map.erase(extension_id);
}

void SettingsManager::shutdown() {
    s_map.clear();
}

// ---------------------------------------------------------------------------
// Lookups
// ---------------------------------------------------------------------------

ExtensionSettings* SettingsManager::find(const std::string& extension_id) {
    auto it = s_map.find(extension_id);
    return it == s_map.end() ? nullptr : &it->second;
}

const SettingSchema* SettingsManager::schema_for(const ExtensionSettings& ext,
                                                 const std::string& key) {
    for (const auto& s : ext.schema)
        if (s.id == key) return &s;
    return nullptr;
}

bool SettingsManager::has_key(const std::string& extension_id, const std::string& key) {
    auto* ext = find(extension_id);
    return ext && schema_for(*ext, key) != nullptr;
}

SettingType SettingsManager::type_of(const std::string& extension_id,
                                     const std::string& key) {
    auto* ext = find(extension_id);
    if (!ext) return SettingType::Toggle;
    auto* s = schema_for(*ext, key);
    return s ? s->type : SettingType::Toggle;
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

std::string SettingsManager::user_file_path(const std::string& extension_id) {
    // Reuse the same path logic the logger uses so framework + settings land
    // together under <profile>\x4native\<extension_id>\.
    auto ext_dir = Logger::profile_ext_dir(extension_id);
    if (ext_dir.empty()) {
        Logger::warn("Settings: profile path unresolved; cannot persist '{}'",
                     extension_id);
        return {};
    }
    return ext_dir + "settings.user.json";
}

void SettingsManager::ensure_loaded(const std::string& extension_id,
                                    ExtensionSettings& ext) {
    if (ext.values_loaded) return;
    ext.values_loaded = true;

    // Seed every key with its default first.
    for (const auto& s : ext.schema) {
        switch (s.type) {
            case SettingType::Toggle:   ext.values[s.id] = s.default_bool;   break;
            case SettingType::Slider:   ext.values[s.id] = s.default_number; break;
            case SettingType::Dropdown: ext.values[s.id] = s.default_string; break;
        }
    }

    auto path = user_file_path(extension_id);
    if (path.empty()) return;

    std::ifstream f(path);
    if (!f.is_open()) return;

    json doc;
    try {
        f >> doc;
    } catch (const json::parse_error& e) {
        Logger::warn("Settings: parse error in '{}': {}", path, e.what());
        return;
    }

    if (!doc.is_object()) return;

    int loaded = 0;
    for (const auto& s : ext.schema) {
        auto it = doc.find(s.id);
        if (it == doc.end()) continue;
        try {
            switch (s.type) {
                case SettingType::Toggle:
                    if (it->is_boolean()) {
                        ext.values[s.id] = it->get<bool>();
                        ++loaded;
                    }
                    break;
                case SettingType::Slider:
                    if (it->is_number()) {
                        ext.values[s.id] = it->get<double>();
                        ++loaded;
                    }
                    break;
                case SettingType::Dropdown:
                    if (it->is_string()) {
                        ext.values[s.id] = it->get<std::string>();
                        ++loaded;
                    }
                    break;
            }
        } catch (...) {}
    }
    Logger::info("Settings: loaded {} value(s) for '{}' from {}",
                 loaded, extension_id, path);
}

void SettingsManager::write_user_file(const std::string& extension_id,
                                      const ExtensionSettings& ext) {
    auto path = user_file_path(extension_id);
    if (path.empty()) return;

    json doc = json::object();
    for (const auto& s : ext.schema) {
        auto it = ext.values.find(s.id);
        if (it == ext.values.end()) continue;
        std::visit([&](const auto& v) { doc[s.id] = v; }, it->second);
    }

    // Atomic write: temp file + rename.
    auto tmp = path + ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f.is_open()) {
            Logger::warn("Settings: cannot open '{}' for write", tmp);
            return;
        }
        f << doc.dump(2);
    }
    std::error_code ec;
    fs::rename(tmp, path, ec);
    if (ec) {
        // rename fails if the destination exists on some platforms; remove+rename.
        fs::remove(path, ec);
        fs::rename(tmp, path, ec);
        if (ec) {
            Logger::warn("Settings: rename '{}' -> '{}' failed: {}",
                         tmp, path, ec.message());
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Typed getters / setters
// ---------------------------------------------------------------------------

bool SettingsManager::get_bool(const std::string& extension_id,
                               const std::string& key, bool fallback) {
    auto* ext = find(extension_id);
    if (!ext) return fallback;
    ensure_loaded(extension_id, *ext);
    auto it = ext->values.find(key);
    if (it == ext->values.end()) return fallback;
    if (auto* p = std::get_if<bool>(&it->second)) return *p;
    return fallback;
}

double SettingsManager::get_number(const std::string& extension_id,
                                   const std::string& key, double fallback) {
    auto* ext = find(extension_id);
    if (!ext) return fallback;
    ensure_loaded(extension_id, *ext);
    auto it = ext->values.find(key);
    if (it == ext->values.end()) return fallback;
    if (auto* p = std::get_if<double>(&it->second)) return *p;
    return fallback;
}

const char* SettingsManager::get_string(const std::string& extension_id,
                                        const std::string& key, const char* fallback) {
    auto* ext = find(extension_id);
    if (!ext) return fallback;
    ensure_loaded(extension_id, *ext);
    auto it = ext->values.find(key);
    if (it == ext->values.end()) return fallback;
    if (auto* p = std::get_if<std::string>(&it->second)) return p->c_str();
    return fallback;
}

void SettingsManager::fire_change(const std::string& extension_id,
                                  const std::string& key,
                                  const SettingValue& new_value) {
    ::X4NativeSettingChanged payload = {};
    payload.extension_id = extension_id.c_str();
    payload.key          = key.c_str();
    std::visit([&](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, bool>) {
            payload.type = X4N_SETTING_TOGGLE;
            payload.b    = v ? 1 : 0;
        } else if constexpr (std::is_same_v<T, double>) {
            payload.type = X4N_SETTING_SLIDER;
            payload.d    = v;
        } else if constexpr (std::is_same_v<T, std::string>) {
            payload.type = X4N_SETTING_DROPDOWN;
            payload.s    = v.c_str();
        }
    }, new_value);

    // Fire a scoped event for the owning extension so subscribers only
    // receive their own keys. Also fire the unscoped "on_setting_changed"
    // so cross-extension observers (debug tools, settings inspectors) can
    // tap into the firehose if needed.
    std::string scoped = "on_setting_changed:" + extension_id;
    EventSystem::fire(scoped.c_str(), &payload);
    EventSystem::fire("on_setting_changed", &payload);
}

void SettingsManager::set_bool(const std::string& extension_id,
                               const std::string& key, bool value) {
    auto* ext = find(extension_id);
    if (!ext) return;
    auto* s = schema_for(*ext, key);
    if (!s || s->type != SettingType::Toggle) {
        Logger::warn("Settings: set_bool on '{}/{}' rejected (schema mismatch)",
                     extension_id, key);
        return;
    }
    ensure_loaded(extension_id, *ext);
    auto it = ext->values.find(key);
    if (it != ext->values.end()) {
        if (auto* p = std::get_if<bool>(&it->second); p && *p == value) return;
    }
    ext->values[key] = value;
    write_user_file(extension_id, *ext);
    refresh_abi_current_values(*ext);
    fire_change(extension_id, key, ext->values[key]);
}

void SettingsManager::set_number(const std::string& extension_id,
                                 const std::string& key, double value) {
    auto* ext = find(extension_id);
    if (!ext) return;
    auto* s = schema_for(*ext, key);
    if (!s || s->type != SettingType::Slider) {
        Logger::warn("Settings: set_number on '{}/{}' rejected (schema mismatch)",
                     extension_id, key);
        return;
    }
    // Clamp to schema bounds.
    if (value < s->min) value = s->min;
    if (value > s->max) value = s->max;
    ensure_loaded(extension_id, *ext);
    auto it = ext->values.find(key);
    if (it != ext->values.end()) {
        if (auto* p = std::get_if<double>(&it->second); p && *p == value) return;
    }
    ext->values[key] = value;
    write_user_file(extension_id, *ext);
    refresh_abi_current_values(*ext);
    fire_change(extension_id, key, ext->values[key]);
}

void SettingsManager::set_string(const std::string& extension_id,
                                 const std::string& key, const char* value) {
    if (!value) return;
    auto* ext = find(extension_id);
    if (!ext) return;
    auto* s = schema_for(*ext, key);
    if (!s || s->type != SettingType::Dropdown) {
        Logger::warn("Settings: set_string on '{}/{}' rejected (schema mismatch)",
                     extension_id, key);
        return;
    }
    // Validate against option set if one is declared (non-empty).
    if (!s->options.empty()) {
        bool ok = false;
        for (const auto& o : s->options) {
            if (o.id == value) { ok = true; break; }
        }
        if (!ok) {
            Logger::warn("Settings: '{}/{}' value '{}' is not a valid option — ignored",
                         extension_id, key, value);
            return;
        }
    }
    ensure_loaded(extension_id, *ext);
    std::string new_val(value);
    auto it = ext->values.find(key);
    if (it != ext->values.end()) {
        if (auto* p = std::get_if<std::string>(&it->second); p && *p == new_val) return;
    }
    ext->values[key] = new_val;
    write_user_file(extension_id, *ext);
    refresh_abi_current_values(*ext);
    fire_change(extension_id, key, ext->values[key]);
}

// ---------------------------------------------------------------------------
// ABI cache (plain-C array seen by the proxy)
// ---------------------------------------------------------------------------

void SettingsManager::rebuild_abi_cache(ExtensionSettings& ext) {
    ext.abi_cache.clear();
    ext.options_cache.clear();
    ext.abi_cache.reserve(ext.schema.size());
    ext.options_cache.reserve(ext.schema.size());

    for (const auto& s : ext.schema) {
        SettingInfo info = {};
        info.id   = s.id.c_str();
        info.name = s.name.c_str();
        switch (s.type) {
            case SettingType::Toggle:   info.type = X4N_SETTING_TOGGLE;   break;
            case SettingType::Dropdown: info.type = X4N_SETTING_DROPDOWN; break;
            case SettingType::Slider:   info.type = X4N_SETTING_SLIDER;   break;
        }
        info.default_bool   = s.default_bool ? 1 : 0;
        info.default_number = s.default_number;
        info.default_string = s.default_string.c_str();
        info.min  = s.min;
        info.max  = s.max;
        info.step = s.step;

        // Options array for dropdowns — stored in parallel per-schema vector.
        std::vector<SettingOptionC> opts;
        opts.reserve(s.options.size());
        for (const auto& o : s.options)
            opts.push_back({ o.id.c_str(), o.text.c_str() });
        ext.options_cache.push_back(std::move(opts));
        const auto& stored = ext.options_cache.back();
        info.options      = stored.empty() ? nullptr : stored.data();
        info.option_count = static_cast<int>(stored.size());

        ext.abi_cache.push_back(info);
    }
    ext.abi_cache_built = true;
}

void SettingsManager::refresh_abi_current_values(ExtensionSettings& ext) {
    if (!ext.abi_cache_built) rebuild_abi_cache(ext);
    for (size_t i = 0; i < ext.schema.size(); ++i) {
        const auto& s    = ext.schema[i];
        auto& info       = ext.abi_cache[i];
        auto it          = ext.values.find(s.id);
        if (it == ext.values.end()) {
            // Missing → fall back to defaults.
            info.current_bool   = info.default_bool;
            info.current_number = info.default_number;
            info.current_string = info.default_string;
            continue;
        }
        std::visit([&](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>)        info.current_bool   = v ? 1 : 0;
            else if constexpr (std::is_same_v<T, double>) info.current_number = v;
            else if constexpr (std::is_same_v<T, std::string>) {
                // Map string value back to the stable option id pointer when
                // possible so the proxy sees a pointer that outlives this call.
                const char* resolved = nullptr;
                for (const auto& o : s.options) {
                    if (o.id == v) { resolved = o.id.c_str(); break; }
                }
                if (!resolved) {
                    // Not in options — keep the raw stored std::string's c_str.
                    if (auto* p = std::get_if<std::string>(&it->second))
                        resolved = p->c_str();
                }
                info.current_string = resolved;
            }
        }, it->second);
    }
}

int SettingsManager::enumerate(const std::string& extension_id,
                               const SettingInfo** out) {
    if (out) *out = nullptr;
    auto* ext = find(extension_id);
    if (!ext || ext->schema.empty()) return 0;
    ensure_loaded(extension_id, *ext);
    if (!ext->abi_cache_built) rebuild_abi_cache(*ext);
    refresh_abi_current_values(*ext);
    if (out) *out = ext->abi_cache.data();
    return static_cast<int>(ext->abi_cache.size());
}

void SettingsManager::set_from_abi(const std::string& extension_id,
                                   const std::string& key,
                                   const SettingValueC& value) {
    switch (value.type) {
        case X4N_SETTING_TOGGLE:   set_bool  (extension_id, key, value.b != 0);     break;
        case X4N_SETTING_SLIDER:   set_number(extension_id, key, value.d);          break;
        case X4N_SETTING_DROPDOWN: set_string(extension_id, key, value.s ? value.s : ""); break;
        default:
            Logger::warn("Settings: set_from_abi with unknown type={}", value.type);
            break;
    }
}

// ---------------------------------------------------------------------------
// Schema parsing (called from ExtensionManager::parse_config)
// ---------------------------------------------------------------------------

// Emit a parse-time warning to both the framework log and (if provided) a
// per-extension buffer that the caller will flush into the extension's own
// log once it's open.
template<typename... Args>
static void schema_warn(std::vector<std::pair<LogLevel, std::string>>* buf,
                        std::format_string<Args...> fmt, Args&&... args) {
    auto msg = std::format(fmt, std::forward<Args>(args)...);
    Logger::warn("{}", msg);
    if (buf) buf->push_back({LogLevel::Warn, std::move(msg)});
}

static bool parse_one(const json& j, SettingSchema& out,
                      const std::string& context,
                      std::vector<std::pair<LogLevel, std::string>>* warnings) {
    if (!j.is_object()) {
        schema_warn(warnings, "Settings: {} entry is not an object", context);
        return false;
    }
    if (!j.contains("id") || !j["id"].is_string()) {
        schema_warn(warnings, "Settings: {} entry missing 'id'", context);
        return false;
    }
    out.id = j["id"].get<std::string>();
    out.name = j.value("name", out.id);

    auto type_str = j.value("type", std::string{});
    if      (type_str == "toggle")   out.type = SettingType::Toggle;
    else if (type_str == "dropdown") out.type = SettingType::Dropdown;
    else if (type_str == "slider")   out.type = SettingType::Slider;
    else {
        schema_warn(warnings, "Settings: {}/{} unknown type '{}' — skipping",
                    context, out.id, type_str);
        return false;
    }

    switch (out.type) {
        case SettingType::Toggle:
            if (j.contains("default") && j["default"].is_boolean())
                out.default_bool = j["default"].get<bool>();
            break;

        case SettingType::Slider:
            if (j.contains("min")  && j["min"].is_number())  out.min  = j["min"].get<double>();
            if (j.contains("max")  && j["max"].is_number())  out.max  = j["max"].get<double>();
            if (j.contains("step") && j["step"].is_number()) out.step = j["step"].get<double>();
            if (out.max < out.min) std::swap(out.min, out.max);
            if (out.step <= 0) out.step = 1.0;
            if (j.contains("default") && j["default"].is_number()) {
                double v = j["default"].get<double>();
                if (v < out.min) v = out.min;
                if (v > out.max) v = out.max;
                out.default_number = v;
            } else {
                out.default_number = out.min;
            }
            break;

        case SettingType::Dropdown:
            if (j.contains("options") && j["options"].is_array()) {
                for (const auto& o : j["options"]) {
                    if (!o.is_object()) continue;
                    SettingOption opt;
                    opt.id   = o.value("id",   std::string{});
                    opt.text = o.value("text", opt.id);
                    if (!opt.id.empty()) out.options.push_back(std::move(opt));
                }
            }
            if (j.contains("default") && j["default"].is_string())
                out.default_string = j["default"].get<std::string>();
            else if (!out.options.empty())
                out.default_string = out.options.front().id;
            if (out.options.empty()) {
                schema_warn(warnings,
                            "Settings: {}/{} dropdown has no options — skipping",
                            context, out.id);
                return false;
            }
            break;
    }
    return true;
}

bool SettingsManager::parse_schema_array(
        const json& node,
        std::vector<SettingSchema>& out,
        const std::string& context,
        std::vector<std::pair<LogLevel, std::string>>* warnings) {
    out.clear();
    if (node.is_null()) return true;
    if (!node.is_array()) {
        schema_warn(warnings, "Settings: {} 'settings' is not an array — ignoring",
                    context);
        return false;
    }
    for (size_t i = 0; i < node.size(); ++i) {
        SettingSchema s;
        if (parse_one(node[i], s,
                      context + "[" + std::to_string(i) + "]",
                      warnings))
            out.push_back(std::move(s));
    }
    return true;
}

} // namespace x4n
