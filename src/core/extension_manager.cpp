#include "extension_manager.h"
#include "logger.h"
#include "event_system.h"
#include "game_api.h"
#include "hook_manager.h"
#include "settings_manager.h"
#include "x4native_defs.h"

#include <x4_game_types.h>
#include <x4_game_func_table.h>
#include <x4_game_offsets.h>

// Runtime-resolved offsets populated by core.cpp::populate_offsets()
extern X4GameOffsets s_offsets;

#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <regex>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace x4n {

std::vector<ExtensionInfo> ExtensionManager::s_extensions;
std::string ExtensionManager::s_ext_root;
std::string ExtensionManager::s_game_version;
int  ExtensionManager::s_tick_frame    = 0;
bool ExtensionManager::s_any_autoreload = false;
static int (*s_raise_lua_event)(const char*, const char*) = nullptr;
static int (*s_register_lua_bridge)(const char*, const char*) = nullptr;
static stash_set_fn    s_stash_set    = nullptr;
static stash_get_fn    s_stash_get    = nullptr;
static stash_remove_fn s_stash_remove = nullptr;
static stash_clear_fn  s_stash_clear  = nullptr;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ExtensionManager::init(const std::string& ext_root, const std::string& game_version,
                            int (*raise_lua_event)(const char*, const char*),
                            int (*register_lua_bridge)(const char*, const char*),
                            stash_set_fn stash_set,
                            stash_get_fn stash_get,
                            stash_remove_fn stash_remove,
                            stash_clear_fn stash_clear) {
    s_ext_root = ext_root;
    s_game_version = game_version;
    s_raise_lua_event = raise_lua_event;
    s_register_lua_bridge = register_lua_bridge;
    s_stash_set    = stash_set;
    s_stash_get    = stash_get;
    s_stash_remove = stash_remove;
    s_stash_clear  = stash_clear;
    s_extensions.clear();
}

void ExtensionManager::shutdown() {
    // Unload in reverse priority order (reverse of load order)
    for (auto it = s_extensions.rbegin(); it != s_extensions.rend(); ++it) {
        unload_extension(*it);
    }
    s_extensions.clear();
    s_any_autoreload = false;
    s_tick_frame     = 0;
}

// ---------------------------------------------------------------------------
// Discovery — scan sibling extension folders for x4native.json
// ---------------------------------------------------------------------------

void ExtensionManager::discover() {
    // If extensions are already loaded, shut them down first.
    // This handles save-load and /reloadui where Lua re-executes
    // and calls discover_extensions() again.
    if (!s_extensions.empty()) {
        Logger::info("Re-discovery: shutting down {} existing extension(s)", s_extensions.size());
        shutdown();
    }

    // ext_root is e.g. "G:\...\extensions\x4native\"
    // We scan the parent "extensions\" dir for sibling extensions
    // Note: trailing separator means parent_path() just strips it,
    // so we need parent_path() twice (or strip trailing sep first).
    fs::path ext_path = fs::path(s_ext_root);
    fs::path extensions_dir = ext_path.has_filename()
        ? ext_path.parent_path()
        : ext_path.parent_path().parent_path();

    if (!fs::is_directory(extensions_dir)) {
        Logger::warn("Extension discovery: cannot find extensions dir: {}", extensions_dir.string());
        return;
    }

    Logger::info("Scanning for native extensions in: {}", extensions_dir.string());

    for (const auto& entry : fs::directory_iterator(extensions_dir)) {
        if (!entry.is_directory()) continue;

        // Skip ourselves
        auto dir_name = entry.path().filename().string();
        if (dir_name == "x4native") continue;

        auto config_path = entry.path() / "x4native.json";
        if (!fs::exists(config_path)) continue;

        ExtensionInfo info;
        info.path = entry.path().string() + "\\";

        if (parse_config(config_path.string(), info)) {
            // Identity comes from content.xml — X4 enforces id uniqueness.
            // The regex anchors on `<content ...>` so a stray `id=` attribute
            // elsewhere in the file can't hijack the match.
            auto content_xml = entry.path() / "content.xml";
            if (fs::exists(content_xml)) {
                std::ifstream xml_file(content_xml);
                std::string xml_content((std::istreambuf_iterator<char>(xml_file)),
                                         std::istreambuf_iterator<char>());
                static const std::regex id_re  (R"xml(<content\b[^>]*\bid="([^"]+)")xml");
                static const std::regex name_re(R"xml(<content\b[^>]*\bname="([^"]+)")xml");
                std::smatch m;
                if (std::regex_search(xml_content, m, id_re))
                    info.extension_id = m[1].str();
                if (std::regex_search(xml_content, m, name_re))
                    info.display_name = m[1].str();
            }
            // Fallbacks: content.xml missing or lacking these attributes is a
            // degenerate case (X4 wouldn't load the extension either). Still,
            // keep loading — framework-side we use extension_id everywhere.
            if (info.extension_id.empty())
                info.extension_id = dir_name;
            if (info.display_name.empty())
                info.display_name = info.extension_id;

            // Deprecated: legacy "name" in x4native.json. Warn (and track so
            // the warning also reaches the extension's own log once open).
            if (!info.json_name.empty()) {
                auto msg = std::format(
                    "x4native.json: 'name' field is deprecated "
                    "(ignored in favour of content.xml id/name) — remove from {}",
                    config_path.string());
                Logger::warn("{}", msg);
                info.pending_warnings.push_back({LogLevel::Warn, std::move(msg)});
            }

            Logger::info("Discovered extension: {} (id={}, priority={}, api={})",
                         info.display_name, info.extension_id,
                         info.priority, info.api_version);

            // Register settings now that we know the extension_id.
            if (!info.settings_schema.empty()) {
                SettingsManager::register_extension(info.extension_id,
                                                    std::move(info.settings_schema));
                info.settings_schema.clear();  // moved out; keep the vector empty
            }

            s_extensions.push_back(std::move(info));
        }
    }

    // Sort by priority (lower = loads first)
    std::sort(s_extensions.begin(), s_extensions.end(),
              [](const ExtensionInfo& a, const ExtensionInfo& b) {
                  return a.priority < b.priority;
              });

    Logger::info("Discovered {} native extension(s)", s_extensions.size());
}

// ---------------------------------------------------------------------------
// Config parsing
// ---------------------------------------------------------------------------

bool ExtensionManager::parse_config(const std::string& json_path, ExtensionInfo& info) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
        Logger::warn("Cannot open config: {}", json_path);
        return false;
    }

    json cfg;
    try {
        file >> cfg;
    } catch (const json::parse_error& e) {
        Logger::error("JSON parse error in {}: {}", json_path, e.what());
        return false;
    }

    // Required: library
    if (!cfg.contains("library") || !cfg["library"].is_string()) {
        Logger::warn("Config missing 'library': {}", json_path);
        return false;
    }

    std::string dll_rel = cfg["library"].get<std::string>();
    info.dll_path = info.path + dll_rel;

    // Optional: legacy "name" field. Identity now comes from content.xml.
    // Stored so we can log a deprecation warning once the logger is open.
    if (cfg.contains("name") && cfg["name"].is_string())
        info.json_name = cfg["name"].get<std::string>();

    if (cfg.contains("logfile") && cfg["logfile"].is_string())
        info.log_name = cfg["logfile"].get<std::string>();

    if (cfg.contains("priority") && cfg["priority"].is_number_integer())
        info.priority = cfg["priority"].get<int>();

    if (cfg.contains("min_api_version") && cfg["min_api_version"].is_number_integer())
        info.api_version = cfg["min_api_version"].get<int>();

    if (cfg.contains("autoreload") && cfg["autoreload"].is_boolean())
        info.autoreload = cfg["autoreload"].get<bool>();

    // Settings schema — optional. Actual registration happens in discover()
    // once we know the extension_id (pulled from content.xml). Context string
    // uses the folder path so warnings reference a concrete file.
    if (cfg.contains("settings"))
        SettingsManager::parse_schema_array(cfg["settings"], info.settings_schema,
                                            json_path, &info.pending_warnings);

    return true;
}

// ---------------------------------------------------------------------------
// Per-extension log helpers
// ---------------------------------------------------------------------------

// api_log_ext — routes x4n::log::info/warn/etc. to the extension's own log file.
// Falls back to the global framework log if the handle is not set.
static void api_log_ext(int level, const char* message, void* api_ptr) {
    auto lv = static_cast<x4n::LogLevel>(level);
    if (api_ptr) {
        auto* api = static_cast<X4NativeAPI*>(api_ptr);
        HANDLE h = static_cast<HANDLE>(api->_ext_log_handle);
        if (h && h != INVALID_HANDLE_VALUE) {
            x4n::Logger::write_to(h, lv, message);
            return;
        }
    }
    x4n::Logger::write(lv, message);
}

// api_init_log — called by x4n::log::set_log_file("filename") to redirect
// the extension's default log. Filename is relative to the extension's
// subfolder (<profile>\x4native\<ext_id>\) — absolute paths and traversal
// are rejected.
static void api_init_log(const char* filename, void* api_ptr) {
    if (!api_ptr) return;
    auto* api  = static_cast<X4NativeAPI*>(api_ptr);
    auto* ext  = static_cast<ExtensionInfo*>(api->_ext_info);
    if (!ext) return;

    if (!filename || !filename[0]) return;

    if (!Logger::is_safe_relative_name(
            filename,
            ("Extension '" + ext->display_name + "': set_log_file()").c_str()))
        return;

    auto ext_dir = Logger::profile_ext_dir(ext->extension_id);
    std::string base = ext_dir.empty() ? ext->path : ext_dir;
    std::string new_path = base + filename;

    // Ensure any author-requested intermediate directories exist.
    std::error_code ec;
    fs::create_directories(fs::path(new_path).parent_path(), ec);
    if (ec) {
        Logger::warn("Extension '{}': set_log_file() failed to create '{}': {}",
                     ext->display_name, new_path, ec.message());
        return;
    }

    HANDLE new_h = Logger::open_log(new_path);
    if (new_h == INVALID_HANDLE_VALUE) {
        Logger::warn("Extension '{}': set_log_file() could not open '{}'",
                     ext->display_name, new_path);
        return;
    }

    // Close the old handle only after the new one opened successfully.
    HANDLE old_h = static_cast<HANDLE>(api->_ext_log_handle);
    if (old_h && old_h != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(old_h);
        CloseHandle(old_h);
    }

    ext->log_handle      = new_h;
    ext->log_path        = new_path;
    api->_ext_log_handle = new_h;
    x4n::Logger::write_to(new_h, x4n::LogLevel::Info,
                           "Extension log initialized: " + new_path);
}

// api_log_named — backs x4n::log::to_file("name").info(...).
// Opens `<profile>\x4native\<ext_id>\<filename>` for append, writes one
// record, closes. Author-provided nested paths are allowed (intermediate
// subfolders are created on first write) but absolute paths and `..`
// segments are rejected — all writes stay inside the per-extension subfolder.
static void api_log_named(int level, const char* message,
                           const char* filename, void* api_ptr) {
    if (!filename || !filename[0]) {
        api_log_ext(level, message, api_ptr);
        return;
    }
    if (!api_ptr) return;

    auto* api = static_cast<X4NativeAPI*>(api_ptr);
    auto* ext = static_cast<ExtensionInfo*>(api->_ext_info);
    if (!ext) return;

    if (!Logger::is_safe_relative_name(
            filename,
            ("Extension '" + ext->display_name + "': to_file()").c_str()))
        return;

    auto ext_dir = Logger::profile_ext_dir(ext->extension_id);
    std::string base = ext_dir;
    if (base.empty()) {
        // Profile unreachable — fall back to the extension folder, same as
        // earlier releases. The relative-name guard above still prevents
        // traversal even in this fallback.
        base = api->extension_path ? api->extension_path : "";
        if (base.empty()) return;
    }

    fs::path full = fs::path(base) / filename;
    // Ensure any author-requested intermediate directories exist.
    std::error_code ec;
    fs::create_directories(full.parent_path(), ec);
    if (ec) return;

    HANDLE h = CreateFileA(full.string().c_str(),
                           GENERIC_WRITE | FILE_APPEND_DATA,
                           FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        SetFilePointer(h, 0, nullptr, FILE_END);
        x4n::Logger::write_to(h, static_cast<x4n::LogLevel>(level), message);
        CloseHandle(h);
    }
}

// ---------------------------------------------------------------------------
// Loading — LoadLibrary + resolve exports + SEH-wrapped init
// ---------------------------------------------------------------------------

void ExtensionManager::load_all() {
    for (auto& ext : s_extensions) {
        auto result = load_extension(ext);
        if (result == LoadResult::failed)
            Logger::error("Failed to load extension: {}", ext.display_name);
    }
    s_any_autoreload = std::any_of(s_extensions.begin(), s_extensions.end(),
                                   [](const ExtensionInfo& e) { return e.autoreload && e.initialized; });
    if (s_any_autoreload)
        Logger::info("Autoreload: watching {} extension(s) for DLL changes",
                     std::count_if(s_extensions.begin(), s_extensions.end(),
                                   [](const ExtensionInfo& e) { return e.autoreload && e.initialized; }));
}

// SEH wrappers — must be in separate functions (no C++ objects requiring unwinding)
static int seh_call_api_version(ExtensionInfo::api_version_fn fn) {
    __try {
        return fn();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

static int seh_call_init(ExtensionInfo::init_fn fn, X4NativeAPI* api) {
    __try {
        return fn(api);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

static void seh_call_shutdown(ExtensionInfo::shutdown_fn fn) {
    __try {
        fn();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // logged by caller
    }
}

ExtensionManager::LoadResult ExtensionManager::load_extension(ExtensionInfo& ext) {
    // Check if the X4 extension is enabled in-game
    auto* game = GameAPI::table();
    if (game && game->IsExtensionEnabled) {
        if (!game->IsExtensionEnabled(ext.extension_id.c_str(), false)) {
            Logger::info("Extension '{}' (id={}) is disabled in-game — skipping",
                         ext.display_name, ext.extension_id);
            return LoadResult::skipped;
        }
    }

    // Check API version compatibility
    if (ext.api_version > X4NATIVE_API_VERSION) {
        Logger::error("Extension '{}' requires API v{}, we have v{}",
                      ext.display_name, ext.api_version, X4NATIVE_API_VERSION);
        return LoadResult::failed;
    }

    // Check DLL exists
    if (!fs::exists(ext.dll_path)) {
        Logger::error("Extension '{}' DLL not found: {}", ext.display_name, ext.dll_path);
        return LoadResult::failed;
    }

    // Copy-on-load: keep the original unlocked so the extension can be rebuilt
    // while the game is running. The live copy is what gets LoadLibrary'd.
    fs::path src(ext.dll_path);
    ext.dll_live_path = (src.parent_path() / (src.stem().string() + "_live" + src.extension().string())).string();
    if (!CopyFileA(ext.dll_path.c_str(), ext.dll_live_path.c_str(), FALSE)) {
        Logger::error("Extension '{}': CopyFile failed (error={})", ext.display_name, GetLastError());
        return LoadResult::failed;
    }

    // Snapshot the original DLL's mtime so tick() can detect future changes
    WIN32_FILE_ATTRIBUTE_DATA mtime_attr = {};
    if (GetFileAttributesExA(ext.dll_path.c_str(), GetFileExInfoStandard, &mtime_attr))
        ext.dll_mtime = mtime_attr.ftLastWriteTime;

    ext.module = LoadLibraryA(ext.dll_live_path.c_str());
    if (!ext.module) {
        Logger::error("Extension '{}': LoadLibrary failed (error={})",
                      ext.display_name, GetLastError());
        DeleteFileA(ext.dll_live_path.c_str());
        return LoadResult::failed;
    }

    // Resolve required exports
    ext.fn_api_version = reinterpret_cast<ExtensionInfo::api_version_fn>(
        GetProcAddress(ext.module, "x4native_api_version"));
    ext.fn_init = reinterpret_cast<ExtensionInfo::init_fn>(
        GetProcAddress(ext.module, "x4native_init"));
    ext.fn_shutdown = reinterpret_cast<ExtensionInfo::shutdown_fn>(
        GetProcAddress(ext.module, "x4native_shutdown"));

    auto unload_live = [&] {
        FreeLibrary(ext.module);
        ext.module = nullptr;
        DeleteFileA(ext.dll_live_path.c_str());
        ext.dll_live_path.clear();
        // Close per-extension log if it was already opened (log open happens after this
        // lambda is defined, but ext.log_handle starts as INVALID_HANDLE_VALUE so early
        // failure paths are safe — they simply skip the close)
        if (ext.log_handle != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(ext.log_handle);
            CloseHandle(ext.log_handle);
            ext.log_handle = INVALID_HANDLE_VALUE;
        }
        ext.log_path.clear();
    };

    if (!ext.fn_api_version || !ext.fn_init || !ext.fn_shutdown) {
        Logger::error("Extension '{}': missing required exports (x4native_api_version, x4native_init, x4native_shutdown)",
                      ext.display_name);
        unload_live();
        return LoadResult::failed;
    }

    // Check runtime API version
    int ext_api = seh_call_api_version(ext.fn_api_version);
    if (ext_api == -1) {
        Logger::error("Extension '{}': x4native_api_version() crashed", ext.display_name);
        unload_live();
        return LoadResult::failed;
    }

    if (ext_api > X4NATIVE_API_VERSION) {
        Logger::error("Extension '{}': runtime API v{} > framework v{}",
                      ext.display_name, ext_api, X4NATIVE_API_VERSION);
        unload_live();
        return LoadResult::failed;
    }

    // Per-extension log file lives in the user profile, under a subfolder
    // named after the extension:
    //   <profile>\x4native\<extension_id>\<extension_id>.log
    // Authors that set "logfile" in x4native.json can pick the filename
    // (still relative to the subfolder, no traversal).
    // If the profile can't be resolved, fall back to the extension folder.
    {
        auto ext_dir = Logger::profile_ext_dir(ext.extension_id);
        std::string log_dir = ext_dir.empty() ? ext.path : ext_dir;

        if (ext.log_name.empty()) {
            ext.log_path = log_dir + ext.extension_id + ".log";
        } else if (Logger::is_safe_relative_name(ext.log_name,
                        ("Extension '" + ext.display_name + "': 'logfile'").c_str())) {
            ext.log_path = log_dir + ext.log_name;
        } else {
            ext.log_path = log_dir + ext.extension_id + ".log";
        }
        ext.log_handle = Logger::open_log(ext.log_path);
        if (ext.log_handle == INVALID_HANDLE_VALUE) {
            Logger::warn("Extension '{}': could not open log at '{}'",
                         ext.display_name, ext.log_path);
        } else {
            Logger::write_to(ext.log_handle, LogLevel::Info,
                             "X4Native extension log — " + ext.display_name
                             + " (" + ext.extension_id + ")");
            // Flush warnings buffered during discovery (pre-log) so the
            // extension author sees them at the top of their own log.
            for (const auto& [lv, msg] : ext.pending_warnings)
                Logger::write_to(ext.log_handle, lv, msg);
            ext.pending_warnings.clear();
        }
    }

    // Build the API struct for this extension (stored in ExtensionInfo,
    // persists until shutdown — extensions keep a pointer to it)
    ext.api = {};
    fill_api(ext.api, ext);

    // SEH-wrapped init call
    int result = seh_call_init(ext.fn_init, &ext.api);
    if (result == -1) {
        Logger::error("Extension '{}': x4native_init() crashed (SEH exception)", ext.display_name);
        unload_live();
        return LoadResult::failed;
    }

    if (result != X4NATIVE_OK) {
        Logger::error("Extension '{}': x4native_init() returned error ({})", ext.display_name, result);
        unload_live();
        return LoadResult::failed;
    }

    ext.initialized = true;
    Logger::info("Extension '{}' loaded successfully", ext.display_name);
    return LoadResult::ok;
}

void ExtensionManager::unload_extension(ExtensionInfo& ext) {
    if (ext.initialized && ext.fn_shutdown) {
        Logger::info("Shutting down extension: {}", ext.display_name);
        seh_call_shutdown(ext.fn_shutdown);
        ext.initialized = false;
    }
    // Remove event subscriptions owned by this extension
    for (int id : ext.subscription_ids)
        EventSystem::unsubscribe(id);
    ext.subscription_ids.clear();
    // Remove any hooks this extension registered (must happen before FreeLibrary
    // since hook callbacks point into the extension's DLL code). Keyed on
    // extension_id — the canonical unique identifier.
    HookManager::remove_all_for_extension(ext.extension_id.c_str());
    if (ext.module) {
        FreeLibrary(ext.module);
        ext.module = nullptr;
    }
    if (!ext.dll_live_path.empty()) {
        DeleteFileA(ext.dll_live_path.c_str());
        ext.dll_live_path.clear();
    }
    // Close per-extension log (after shutdown so the extension can log until the end)
    if (ext.log_handle != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(ext.log_handle);
        CloseHandle(ext.log_handle);
        ext.log_handle = INVALID_HANDLE_VALUE;
    }
    ext.log_path.clear();  // reset so load_extension recomputes from log_name on hot-reload

    // Settings stay registered across hot-reload so values survive; they are
    // only dropped on full shutdown (SettingsManager::shutdown()).
}

// ---------------------------------------------------------------------------
// Autoreload — per-extension mtime polling and deferred reload
// ---------------------------------------------------------------------------

static constexpr int AUTORELOAD_TICK_INTERVAL = 120;  // frames between mtime checks (~2s @60fps)

void ExtensionManager::tick() {
    if (!s_any_autoreload) return;
    if (++s_tick_frame < AUTORELOAD_TICK_INTERVAL) return;
    s_tick_frame = 0;

    for (auto& ext : s_extensions) {
        if (!ext.autoreload || !ext.initialized || ext.reload_pending) continue;

        WIN32_FILE_ATTRIBUTE_DATA attr = {};
        if (!GetFileAttributesExA(ext.dll_path.c_str(), GetFileExInfoStandard, &attr)) continue;

        if (CompareFileTime(&attr.ftLastWriteTime, &ext.dll_mtime) > 0) {
            Logger::info("Extension '{}': DLL changed on disk, queuing hot-reload",
                         ext.display_name);
            ext.reload_pending = true;
        }
    }
}

void ExtensionManager::flush_pending_reloads() {
    if (!s_any_autoreload) return;

    bool any_reloaded = false;
    for (auto& ext : s_extensions) {
        if (!ext.reload_pending) continue;
        ext.reload_pending = false;
        any_reloaded = true;

        Logger::info("Hot-reloading extension: {}", ext.display_name);
        unload_extension(ext);

        if (load_extension(ext) == LoadResult::ok)
            Logger::info("Extension '{}': hot-reload complete", ext.display_name);
        else
            Logger::error("Extension '{}': hot-reload failed", ext.display_name);
    }

    if (any_reloaded)
        s_any_autoreload = std::any_of(s_extensions.begin(), s_extensions.end(),
                                       [](const ExtensionInfo& e) { return e.autoreload && e.initialized; });
}

// ---------------------------------------------------------------------------
// API struct — what extensions receive in x4native_init()
// ---------------------------------------------------------------------------

// Static wrappers that forward to EventSystem / Logger
static int api_subscribe(const char* event_name, X4NativeEventCallback cb, void* ud, void* api_ptr) {
    int id = EventSystem::subscribe(event_name, cb, ud);
    // Track subscription for auto-cleanup on extension unload
    if (id > 0 && api_ptr) {
        auto* api = static_cast<X4NativeAPI*>(api_ptr);
        auto* ids = static_cast<std::vector<int>*>(api->_ext_subscription_ids);
        if (ids) ids->push_back(id);
    }
    return id;
}

static void api_unsubscribe(int id) {
    EventSystem::unsubscribe(id);
}

static void api_raise_event(const char* event_name, void* data) {
    EventSystem::fire(event_name, data);
}

static int api_raise_lua_event(const char* event_name, const char* param) {
    if (s_raise_lua_event) return s_raise_lua_event(event_name, param);
    return -1;
}

static void api_log(int level, const char* message) {
    auto lv = static_cast<LogLevel>(level);
    Logger::write(lv, message);
}

static const char* s_game_ver_cache;
static const char* s_x4n_ver_cache = X4_GAME_VERSION_LABEL;

static const char* api_get_game_version() {
    return s_game_ver_cache;
}

static const char* api_get_x4native_version() {
    return s_x4n_ver_cache;
}

// Hook wrappers — extract extension context from the API pointer.
// HookManager keys on extension_id (canonical unique), not display name.
static int api_hook_before(const char* fn, X4HookCallback cb, void* ud, void* api_ptr) {
    auto* api = static_cast<X4NativeAPI*>(api_ptr);
    int ext_priority = static_cast<int>(api->_ext_priority);
    return HookManager::hook_before(fn, cb, ud, ext_priority, api->_ext_id);
}

static int api_hook_after(const char* fn, X4HookCallback cb, void* ud, void* api_ptr) {
    auto* api = static_cast<X4NativeAPI*>(api_ptr);
    int ext_priority = static_cast<int>(api->_ext_priority);
    return HookManager::hook_after(fn, cb, ud, ext_priority, api->_ext_id);
}

static void api_unhook(int hook_id) {
    HookManager::unhook(hook_id);
}

static void* api_ensure_detour(const char* fn, void* detour_fn) {
    return HookManager::ensure_detour(fn, detour_fn);
}

static void api_run_before_hooks(X4HookContext* ctx) {
    HookManager::run_before_hooks(ctx);
}

static void api_run_after_hooks(X4HookContext* ctx) {
    HookManager::run_after_hooks(ctx);
}

static void* api_resolve_internal(const char* name) {
    return GameAPI::get_internal(name);
}

// MD event subscription wrappers
static int api_md_subscribe_before(uint32_t type_id, X4NativeEventCallback cb, void* ud, void* /*api*/) {
    return EventSystem::md_subscribe_before(type_id, cb, ud);
}

static int api_md_subscribe_after(uint32_t type_id, X4NativeEventCallback cb, void* ud, void* /*api*/) {
    return EventSystem::md_subscribe_after(type_id, cb, ud);
}

static int api_register_lua_bridge(const char* lua_event, const char* cpp_event) {
    if (s_register_lua_bridge)
        return s_register_lua_bridge(lua_event, cpp_event);
    return -1;
}

// Settings wrappers — resolve the calling extension via _ext_info.
static const char* ext_id_from_api(void* api_ptr) {
    if (!api_ptr) return nullptr;
    auto* api = static_cast<X4NativeAPI*>(api_ptr);
    auto* ext = static_cast<ExtensionInfo*>(api->_ext_info);
    return ext ? ext->extension_id.c_str() : nullptr;
}

static int api_get_setting_bool(const char* key, int fallback, void* api_ptr) {
    const char* id = ext_id_from_api(api_ptr);
    if (!id || !key) return fallback;
    return SettingsManager::get_bool(id, key, fallback != 0) ? 1 : 0;
}

static double api_get_setting_number(const char* key, double fallback, void* api_ptr) {
    const char* id = ext_id_from_api(api_ptr);
    if (!id || !key) return fallback;
    return SettingsManager::get_number(id, key, fallback);
}

static const char* api_get_setting_string(const char* key, const char* fallback, void* api_ptr) {
    const char* id = ext_id_from_api(api_ptr);
    if (!id || !key) return fallback;
    return SettingsManager::get_string(id, key, fallback);
}

static void api_set_setting_bool(const char* key, int value, void* api_ptr) {
    const char* id = ext_id_from_api(api_ptr);
    if (!id || !key) return;
    SettingsManager::set_bool(id, key, value != 0);
}

static void api_set_setting_number(const char* key, double value, void* api_ptr) {
    const char* id = ext_id_from_api(api_ptr);
    if (!id || !key) return;
    SettingsManager::set_number(id, key, value);
}

static void api_set_setting_string(const char* key, const char* value, void* api_ptr) {
    const char* id = ext_id_from_api(api_ptr);
    if (!id || !key) return;
    SettingsManager::set_string(id, key, value ? value : "");
}

void ExtensionManager::fill_api(X4NativeAPI& api, ExtensionInfo& ext) {
    s_game_ver_cache = s_game_version.c_str();

    api.api_version          = X4NATIVE_API_VERSION;
    api.subscribe            = api_subscribe;
    api.unsubscribe          = api_unsubscribe;
    api.raise_event          = api_raise_event;
    api.raise_lua_event      = api_raise_lua_event;
    api.log                  = api_log;
    api.get_game_version     = api_get_game_version;
    api.get_x4native_version = api_get_x4native_version;
    api.extension_path       = ext.path.c_str();
    api.game                 = GameAPI::table();
    api.get_game_function    = &GameAPI::get_function;
    api.game_func_count      = GameAPI::total_count();
    api.game_types_build     = X4_GAME_TYPES_BUILD;
    api.hook_before          = api_hook_before;
    api.hook_after           = api_hook_after;
    api.unhook               = api_unhook;
    api._ensure_detour       = api_ensure_detour;
    api._run_before_hooks    = api_run_before_hooks;
    api._run_after_hooks     = api_run_after_hooks;
    api.resolve_internal     = api_resolve_internal;
    api.exe_base             = GameAPI::exe_base();
    api.register_lua_bridge  = api_register_lua_bridge;
    api.md_subscribe_before  = api_md_subscribe_before;
    api.md_subscribe_after   = api_md_subscribe_after;
    api.stash_set            = s_stash_set;
    api.stash_get            = s_stash_get;
    api.stash_remove         = s_stash_remove;
    api.stash_clear          = s_stash_clear;
    api._ext_id               = ext.extension_id.c_str();
    api._ext_display_name     = ext.display_name.c_str();
    api._ext_priority         = static_cast<intptr_t>(ext.priority);
    api._ext_subscription_ids = &ext.subscription_ids;
    api._ext_log_handle       = ext.log_handle;
    api._ext_log_fn           = reinterpret_cast<void*>(api_log_ext);
    api._ext_init_log_fn      = reinterpret_cast<void*>(api_init_log);
    api._ext_log_named_fn     = reinterpret_cast<void*>(api_log_named);
    api._ext_info             = &ext;
    api.offsets               = &s_offsets;
    api.get_setting_bool      = api_get_setting_bool;
    api.get_setting_number    = api_get_setting_number;
    api.get_setting_string    = api_get_setting_string;
    api.set_setting_bool      = api_set_setting_bool;
    api.set_setting_number    = api_set_setting_number;
    api.set_setting_string    = api_set_setting_string;
}

// ---------------------------------------------------------------------------
// JSON serialization of loaded extensions
// ---------------------------------------------------------------------------

std::string ExtensionManager::loaded_extensions_json() {
    json arr = json::array();
    for (const auto& ext : s_extensions) {
        if (ext.initialized) {
            arr.push_back({
                {"id", ext.extension_id},
                {"display_name", ext.display_name},
                {"path", ext.path},
                {"priority", ext.priority}
            });
        }
    }
    // Store in static string so c_str() survives the call
    static std::string cached;
    cached = arr.dump();
    return cached;
}

} // namespace x4n
