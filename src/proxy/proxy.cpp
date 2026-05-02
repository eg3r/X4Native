// ---------------------------------------------------------------------------
// x4native_64.dll — Thin Proxy DLL
//
// This is the stable entry point loaded by Lua's package.loadlib().
// It rarely changes, so the Windows file lock is irrelevant.
//
// Responsibilities:
//   1. Resolve Lua API function pointers from host process
//   2. Copy-on-load x4native_core.dll → x4native_core_live.dll
//   3. LoadLibrary the copy, call core_init() to fill dispatch table
//   4. Return a Lua table whose functions dispatch through the table
//   5. On /reloadui: detect newer core on disk, hot-reload if needed
// ---------------------------------------------------------------------------

#include "lua_api.h"
#include "x4native_defs.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

// ---------------------------------------------------------------------------
// State (persists across /reloadui because the proxy stays mapped)
// ---------------------------------------------------------------------------
static lua_State*       g_lua            = nullptr;
static HMODULE          g_core_module    = nullptr;
static CoreDispatch     g_dispatch       = {};
static core_init_fn     g_core_init      = nullptr;
static core_shutdown_fn g_core_shutdown  = nullptr;
static std::string      g_ext_root;
static std::string      g_core_path;
static std::string      g_core_live_path;
static bool             g_initialized    = false;

// ---------------------------------------------------------------------------
// Core hot-reload state (compiled in when X4N_WITH_RELOAD=1)
// ---------------------------------------------------------------------------
// Guarded on a dedicated feature flag rather than NDEBUG because the project's
// actually-run build config is Release/RelWithDebInfo (std::string layout
// must match the game's Release STL). Runtime behavior is separately gated
// by the `autoreload` key in x4native_settings.json.
#ifdef X4N_WITH_RELOAD
#include <fstream>
#include <nlohmann/json.hpp>

static bool g_autoreload_enabled   = false;   // from x4native_settings.json
static bool g_autoreload_checked   = false;   // settings file read?
static FILETIME g_last_core_mtime  = {};       // last known timestamp

/// Read x4native_settings.json to check "autoreload" flag.
/// Called on each core load so the result is logged after hot-reloads too.
static void read_autoreload_setting() {
    std::string path = g_ext_root + "x4native_settings.json";
    std::ifstream file(path);
    if (!file.is_open()) {
        if (g_dispatch.log) g_dispatch.log(2, "Core autoreload: settings file not found");
        return;
    }

    try {
        auto cfg = nlohmann::json::parse(file);
        if (cfg.contains("autoreload") && cfg["autoreload"].is_boolean())
            g_autoreload_enabled = cfg["autoreload"].get<bool>();
    } catch (...) {}

    if (g_dispatch.log)
        g_dispatch.log(1, g_autoreload_enabled
            ? "Core autoreload: ENABLED"
            : "Core autoreload: disabled");
}

/// Check if core DLL on disk is newer than last known timestamp.
/// Updates g_last_core_mtime on change.
static bool core_modified_since_last_check() {
    WIN32_FILE_ATTRIBUTE_DATA attr = {};
    if (!GetFileAttributesExA(g_core_path.c_str(),
                              GetFileExInfoStandard, &attr))
        return false;

    if (g_last_core_mtime.dwHighDateTime == 0 &&
        g_last_core_mtime.dwLowDateTime == 0) {
        // First call — seed with current timestamp
        g_last_core_mtime = attr.ftLastWriteTime;
        return false;
    }

    if (CompareFileTime(&attr.ftLastWriteTime, &g_last_core_mtime) > 0) {
        g_last_core_mtime = attr.ftLastWriteTime;
        if (g_dispatch.log) g_dispatch.log(1, "Autoreload: core DLL modified on disk");
        return true;
    }
    return false;
}
#endif // X4N_WITH_RELOAD

// ---------------------------------------------------------------------------
// Stash — in-memory key-value that survives /reloadui and core hot-reload
// Keyed by namespace (typically extension name) + key.
// ---------------------------------------------------------------------------
static std::unordered_map<std::string,
           std::unordered_map<std::string, std::vector<uint8_t>>> g_stash;
static std::mutex g_stash_mutex;

static int proxy_stash_set(const char* ns, const char* key,
                           const void* data, uint32_t size) {
    if (!ns || !key || (!data && size > 0)) return 0;
    std::lock_guard lock(g_stash_mutex);
    auto& entry = g_stash[ns][key];
    entry.assign(static_cast<const uint8_t*>(data),
                 static_cast<const uint8_t*>(data) + size);
    return 1;
}

static const void* proxy_stash_get(const char* ns, const char* key,
                                   uint32_t* out_size) {
    if (!ns || !key) return nullptr;
    std::lock_guard lock(g_stash_mutex);
    auto ns_it = g_stash.find(ns);
    if (ns_it == g_stash.end()) return nullptr;
    auto it = ns_it->second.find(key);
    if (it == ns_it->second.end()) return nullptr;
    if (out_size) *out_size = static_cast<uint32_t>(it->second.size());
    return it->second.data();
}

static int proxy_stash_remove(const char* ns, const char* key) {
    if (!ns || !key) return 0;
    std::lock_guard lock(g_stash_mutex);
    auto ns_it = g_stash.find(ns);
    if (ns_it == g_stash.end()) return 0;
    return ns_it->second.erase(key) > 0 ? 1 : 0;
}

static void proxy_stash_clear(const char* ns) {
    if (!ns) return;
    std::lock_guard lock(g_stash_mutex);
    g_stash.erase(ns);
}

// Forward declarations (defined below with Lua-facing functions)
static int proxy_raise_lua_event(const char* name, const char* param);
static int proxy_register_lua_bridge(const char* lua_event, const char* cpp_event);
static bool proxy_get_lua_property(const char* getter_fn, X4nLuaKey key,
                                   const char* field, X4nLuaValueType vt,
                                   void* out);
static bool proxy_get_lua_property_str(const char* getter_fn, X4nLuaKey key,
                                       const char* field,
                                       char* out_buf, size_t buf_size);

// ---------------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------------

/// Derive the extension root from the proxy DLL path.
/// Proxy lives at <ext_root>/native/x4native_64.dll
static std::string detect_ext_root() {
    char buf[MAX_PATH];
    HMODULE self = nullptr;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(&detect_ext_root), &self);
    GetModuleFileNameA(self, buf, MAX_PATH);

    std::string p(buf);
    auto pos = p.rfind("\\native\\");
    if (pos != std::string::npos)
        return p.substr(0, pos + 1);      // includes trailing backslash
    // Fallback: parent of DLL
    pos = p.rfind('\\');
    return (pos != std::string::npos) ? p.substr(0, pos + 1) : p;
}

// ---------------------------------------------------------------------------
// Core DLL loading
// ---------------------------------------------------------------------------

static bool load_core() {
    // Copy-on-load: the original stays unlocked so builds can overwrite it
    if (!CopyFileA(g_core_path.c_str(), g_core_live_path.c_str(), FALSE)) {
        OutputDebugStringA("X4Native proxy: CopyFile failed for core DLL\n");
        return false;
    }

    g_core_module = LoadLibraryA(g_core_live_path.c_str());
    if (!g_core_module) {
        OutputDebugStringA("X4Native proxy: LoadLibrary failed for core DLL\n");
        return false;
    }

    g_core_init     = reinterpret_cast<core_init_fn>(
                          GetProcAddress(g_core_module, "core_init"));
    g_core_shutdown = reinterpret_cast<core_shutdown_fn>(
                          GetProcAddress(g_core_module, "core_shutdown"));

    if (!g_core_init) {
        OutputDebugStringA("X4Native proxy: core_init export not found\n");
        FreeLibrary(g_core_module);
        g_core_module = nullptr;
        return false;
    }

    CoreInitContext ctx = {};
    ctx.lua_state = g_lua;
    ctx.ext_root  = g_ext_root.c_str();
    ctx.dispatch  = &g_dispatch;
    ctx.raise_lua_event = proxy_raise_lua_event;
    ctx.register_lua_bridge = proxy_register_lua_bridge;
    ctx.stash_set    = proxy_stash_set;
    ctx.stash_get    = proxy_stash_get;
    ctx.stash_remove = proxy_stash_remove;
    ctx.stash_clear  = proxy_stash_clear;
    ctx.get_lua_property     = proxy_get_lua_property;
    ctx.get_lua_property_str = proxy_get_lua_property_str;

    if (g_core_init(&ctx) != 0) {
        OutputDebugStringA("X4Native proxy: core_init returned error\n");
        FreeLibrary(g_core_module);
        g_core_module = nullptr;
        return false;
    }

#ifdef X4N_WITH_RELOAD
    read_autoreload_setting();
#endif

    return true;
}

static bool reload_core() {
    // Tell current core to prepare (unhook, notify extensions)
    if (g_dispatch.prepare_reload)
        g_dispatch.prepare_reload();

    if (g_core_module) {
        if (g_core_shutdown)
            g_core_shutdown();
        FreeLibrary(g_core_module);
        g_core_module = nullptr;
    }

    // Reset dispatch table + function pointers
    g_dispatch      = {};
    g_core_init     = nullptr;
    g_core_shutdown = nullptr;

    return load_core();
}

/// Returns true if the on-disk core is newer than the live copy.
static bool core_needs_reload() {
    WIN32_FILE_ATTRIBUTE_DATA disk_attr = {}, live_attr = {};
    if (!GetFileAttributesExA(g_core_path.c_str(),
                              GetFileExInfoStandard, &disk_attr))
        return false;
    if (!GetFileAttributesExA(g_core_live_path.c_str(),
                              GetFileExInfoStandard, &live_attr))
        return true;   // live copy missing — need to load
    return CompareFileTime(&disk_attr.ftLastWriteTime,
                           &live_attr.ftLastWriteTime) > 0;
}

// ---------------------------------------------------------------------------
// Lua-facing API functions  (thin forwarders into g_dispatch)
// ---------------------------------------------------------------------------

static int l_discover_extensions(lua_State* L) {
    if (g_dispatch.discover_extensions)
        g_dispatch.discover_extensions();
    return 0;
}

static int l_raise_event(lua_State* L) {
    const char* ev = x4n::lua::L_checkstring(L, 1);
    const char* param = nullptr;
    if (x4n::lua::gettop(L) >= 2 && x4n::lua::type(L, 2) == LUA_TSTRING)
        param = x4n::lua::tostring(L, 2);
    if (g_dispatch.raise_event)
        g_dispatch.raise_event(ev, param);
    return 0;
}

// ---------------------------------------------------------------------------
// Lua bridge: proxy_raise_lua_event
//
// Called by extensions (via core dispatch) to fire a Lua event.
// Uses X4's global CallEventScripts() — same path as MD <raise_lua_event>.
// ---------------------------------------------------------------------------
static int proxy_raise_lua_event(const char* name, const char* param) {
    if (!g_lua) return -1;
    x4n::lua::getfield(g_lua, LUA_GLOBALSINDEX, "CallEventScripts");
    x4n::lua::pushstring(g_lua, name);
    if (param) x4n::lua::pushstring(g_lua, param);
    else       x4n::lua::pushnil(g_lua);
    return x4n::lua::pcall(g_lua, 2, 0, 0);
}

// ---------------------------------------------------------------------------
// Lua-property accessors: proxy_get_lua_property / _str
//
// Generic over Get*Data-style host C-funcs (GetWareData, GetComponentData,
// future GetFactionData, …). SDK exposes typed inlines per domain
// (x4n::ware::*, x4n::entity::*) — adding a new accessor never touches
// proxy or core, only the SDK header. UI thread only.
// ---------------------------------------------------------------------------
// Resolve `getter_fn` and push (key, field). Returns false (and restores
// stack) if any global is missing or pcall fails. On success leaves the
// function and 2 args on the stack ready for pcall(2,1,0).
//
// UNIT64 keys go through ConvertStringToLuaID(tostring(uid)) — the engine's
// Get*Data functions expect this opaque "Lua ID" form, NOT a bare
// lua_Integer (verified empirically: lua_pushinteger produces a Lua number
// that GetComponentData silently fails on, returning nil for every entity).
// Vanilla pattern: menu_diplomacy.lua:1648, menu_interactmenu.lua:531/565/etc.
// Resolve `getter_fn` and push (key, field). Returns false (and restores
// stack) if any global is missing or pcall fails. On success leaves the
// function and 2 args on the stack ready for pcall(2,1,0).
//
// UNIT64 keys go through ConvertStringToLuaID(tostring(uid)) — the engine's
// Lua C-funcs expect this opaque "Lua ID" userdata form, NOT a bare
// lua_Integer. Vanilla pattern: menu_diplomacy.lua:1648, menu_interactmenu.lua.
//
// CAVEAT: not every MD-script-accessible field is reachable through
// GetComponentData. `buildresourcevalue` for example is exposed ONLY in MD
// namespace (`component.{X}.buildresourcevalue`); GetComponentData returns
// nil for it on valid live stations. Stick to fields that have at least one
// vanilla Lua caller before adding an SDK domain helper for them.
static bool prepare_call(int top, const char* getter_fn,
                         const X4nLuaKey& key, const char* field) {
    if (key.type == X4N_KEY_UINT64) {
        x4n::lua::getfield(g_lua, LUA_GLOBALSINDEX, "ConvertStringToLuaID");
        if (x4n::lua::type(g_lua, -1) != LUA_TFUNCTION) {
            x4n::lua::settop(g_lua, top);
            return false;
        }
        char id_str[24];
        snprintf(id_str, sizeof(id_str), "%llu",
                 (unsigned long long)key.v.u);
        x4n::lua::pushstring(g_lua, id_str);
        if (x4n::lua::pcall(g_lua, 1, 1, 0) != 0) {
            x4n::lua::settop(g_lua, top);
            return false;
        }
        // Stack now has the converted Lua ID userdata on top.
    }

    x4n::lua::getfield(g_lua, LUA_GLOBALSINDEX, getter_fn);
    if (x4n::lua::type(g_lua, -1) != LUA_TFUNCTION) {
        x4n::lua::settop(g_lua, top);
        return false;
    }

    if (key.type == X4N_KEY_UINT64) {
        // Stack: [..., converted_id, getter_fn]. Insert getter_fn below
        // converted_id so the call shape is getter_fn(converted_id, field).
        x4n::lua::insert(g_lua, -2);
    } else {
        x4n::lua::pushstring(g_lua, key.v.s ? key.v.s : "");
    }
    x4n::lua::pushstring(g_lua, field);
    return true;
}

static bool proxy_get_lua_property(const char* getter_fn, X4nLuaKey key,
                                   const char* field, X4nLuaValueType vt,
                                   void* out) {
    if (!g_lua || !getter_fn || !field || !out) return false;
    int top = x4n::lua::gettop(g_lua);
    if (!prepare_call(top, getter_fn, key, field)) return false;

    bool ok = false;
    if (x4n::lua::pcall(g_lua, 2, 1, 0) == 0) {
        int t = x4n::lua::type(g_lua, -1);
        switch (vt) {
            case X4N_VAL_INT64:
                if (t == LUA_TNUMBER) {
                    *static_cast<int64_t*>(out) =
                        static_cast<int64_t>(x4n::lua::tointeger(g_lua, -1));
                    ok = true;
                }
                break;
            case X4N_VAL_DOUBLE:
                if (t == LUA_TNUMBER) {
                    *static_cast<double*>(out) =
                        static_cast<double>(x4n::lua::tonumber(g_lua, -1));
                    ok = true;
                }
                break;
            case X4N_VAL_BOOL:
                // Strict: 1/0 numeric flags must be fetched as INT64.
                if (t == LUA_TBOOLEAN) {
                    *static_cast<bool*>(out) =
                        x4n::lua::toboolean(g_lua, -1) != 0;
                    ok = true;
                }
                break;
        }
    }
    x4n::lua::settop(g_lua, top);
    return ok;
}

static bool proxy_get_lua_property_str(const char* getter_fn, X4nLuaKey key,
                                       const char* field, char* out_buf,
                                       size_t buf_size) {
    if (!g_lua || !getter_fn || !field || !out_buf || buf_size == 0)
        return false;
    int top = x4n::lua::gettop(g_lua);
    if (!prepare_call(top, getter_fn, key, field)) return false;

    bool ok = false;
    if (x4n::lua::pcall(g_lua, 2, 1, 0) == 0) {
        if (x4n::lua::type(g_lua, -1) == LUA_TSTRING) {
            size_t len = 0;
            const char* s = x4n::lua::tolstring(g_lua, -1, &len);
            // Truncation is failure — caller can't otherwise tell whether
            // the returned value is the full string. Retry with larger buf.
            if (s && len + 1 <= buf_size) {
                std::memcpy(out_buf, s, len);
                out_buf[len] = '\0';
                ok = true;
            }
        }
    }
    x4n::lua::settop(g_lua, top);
    return ok;
}

// ---------------------------------------------------------------------------
// Dynamic Lua→C++ event bridge
//
// Extensions call register_lua_bridge(lua_event, cpp_event) to wire a
// Lua RegisterEvent handler that forwards into the C++ event bus.
// A single C function (bridge_handler) is registered as an upvalue-based
// Lua closure for each mapping.
// ---------------------------------------------------------------------------
static std::unordered_map<std::string, std::string> g_lua_bridges;

// Lua closure: upvalue 1 = C++ event name string
// Lua calls handler(eventName, argument1) — argument1 is at stack index 2
static int bridge_handler(lua_State* L) {
    const char* cpp_event = x4n::lua::tostring(L, lua_upvalueindex(1));
    if (cpp_event && g_dispatch.raise_event) {
        const char* param = nullptr;
        if (x4n::lua::gettop(L) >= 2 && x4n::lua::type(L, 2) == LUA_TSTRING)
            param = x4n::lua::tostring(L, 2);
        g_dispatch.raise_event(cpp_event, param);
    }
    return 0;
}

static int proxy_register_lua_bridge(const char* lua_event, const char* cpp_event) {
    if (!g_lua || !lua_event || !cpp_event) return -1;

    // Already registered?
    if (g_lua_bridges.count(lua_event)) return 0;

    // Create a Lua closure with cpp_event as upvalue, then call RegisterEvent
    // Stack: RegisterEvent, lua_event, closure
    x4n::lua::getfield(g_lua, LUA_GLOBALSINDEX, "RegisterEvent");
    x4n::lua::pushstring(g_lua, lua_event);
    x4n::lua::pushstring(g_lua, cpp_event);
    x4n::lua::pushcclosure(g_lua, bridge_handler, 1);
    int err = x4n::lua::pcall(g_lua, 2, 0, 0);

    if (err == 0) {
        g_lua_bridges[lua_event] = cpp_event;
        if (g_dispatch.log)
            g_dispatch.log(1, (std::string("Lua bridge: registered '") +
                               lua_event + "' -> '" + cpp_event + "'").c_str());
    }
    return err;
}

static int l_raise_lua_event(lua_State* L) {
    const char* name  = x4n::lua::L_checkstring(L, 1);
    const char* param = nullptr;
    if (x4n::lua::gettop(L) >= 2 && x4n::lua::type(L, 2) == LUA_TSTRING)
        param = x4n::lua::tostring(L, 2);
    x4n::lua::pushinteger(L, proxy_raise_lua_event(name, param));
    return 1;
}

static int l_log(lua_State* L) {
    int level = static_cast<int>(x4n::lua::tointeger(L, 1));
    const char* msg = x4n::lua::L_checkstring(L, 2);
    if (g_dispatch.log)
        g_dispatch.log(level, msg);
    return 0;
}

static int l_get_version(lua_State* L) {
    const char* v = g_dispatch.get_version
                        ? g_dispatch.get_version()
                        : "unknown";
    x4n::lua::pushstring(L, v);
    return 1;
}

static int l_get_loaded_extensions(lua_State* L) {
    const char* j = g_dispatch.get_loaded_extensions
                        ? g_dispatch.get_loaded_extensions()
                        : "[]";
    x4n::lua::pushstring(L, j);
    return 1;
}

static int l_reload(lua_State* L) {
    x4n::lua::pushboolean(L, reload_core() ? 1 : 0);
    return 1;
}

static int l_prepare_reload(lua_State* L) {
    if (g_dispatch.prepare_reload)
        g_dispatch.prepare_reload();
    return 0;
}

// ---------------------------------------------------------------------------
// Per-extension settings — Lua marshalling
// ---------------------------------------------------------------------------
//
// l_get_extension_settings(ext_id) returns an ordered array of row tables:
//   { {id, name, type, current, default, options?, min?, max?, step?}, ... }
// l_set_extension_setting(ext_id, key, value) writes the value (type inferred
// from the Lua value).

static void push_setting_row(lua_State* L, const SettingInfo& info) {
    x4n::lua::createtable(L, 0, 8);

    x4n::lua::pushstring(L, info.id ? info.id : "");
    x4n::lua::setfield(L, -2, "id");
    x4n::lua::pushstring(L, info.name ? info.name : "");
    x4n::lua::setfield(L, -2, "name");

    const char* type_str = "toggle";
    if      (info.type == X4N_SETTING_DROPDOWN) type_str = "dropdown";
    else if (info.type == X4N_SETTING_SLIDER)   type_str = "slider";
    x4n::lua::pushstring(L, type_str);
    x4n::lua::setfield(L, -2, "type");

    switch (info.type) {
        case X4N_SETTING_TOGGLE:
            x4n::lua::pushboolean(L, info.current_bool);
            x4n::lua::setfield(L, -2, "current");
            x4n::lua::pushboolean(L, info.default_bool);
            x4n::lua::setfield(L, -2, "default");
            break;

        case X4N_SETTING_SLIDER:
            x4n::lua::pushnumber(L, info.current_number);
            x4n::lua::setfield(L, -2, "current");
            x4n::lua::pushnumber(L, info.default_number);
            x4n::lua::setfield(L, -2, "default");
            x4n::lua::pushnumber(L, info.min);
            x4n::lua::setfield(L, -2, "min");
            x4n::lua::pushnumber(L, info.max);
            x4n::lua::setfield(L, -2, "max");
            x4n::lua::pushnumber(L, info.step);
            x4n::lua::setfield(L, -2, "step");
            break;

        case X4N_SETTING_DROPDOWN:
            x4n::lua::pushstring(L, info.current_string ? info.current_string : "");
            x4n::lua::setfield(L, -2, "current");
            x4n::lua::pushstring(L, info.default_string ? info.default_string : "");
            x4n::lua::setfield(L, -2, "default");
            x4n::lua::createtable(L, info.option_count, 0);
            for (int i = 0; i < info.option_count; ++i) {
                x4n::lua::createtable(L, 0, 2);
                x4n::lua::pushstring(L, info.options[i].id ? info.options[i].id : "");
                x4n::lua::setfield(L, -2, "id");
                x4n::lua::pushstring(L, info.options[i].text ? info.options[i].text : "");
                x4n::lua::setfield(L, -2, "text");
                x4n::lua::rawseti(L, -2, i + 1);
            }
            x4n::lua::setfield(L, -2, "options");
            break;
    }
}

static int l_get_extension_settings(lua_State* L) {
    const char* ext_id = x4n::lua::L_checkstring(L, 1);
    const SettingInfo* entries = nullptr;
    int n = 0;
    if (g_dispatch.enumerate_settings)
        n = g_dispatch.enumerate_settings(ext_id, &entries);
    x4n::lua::createtable(L, n, 0);
    for (int i = 0; i < n; ++i) {
        push_setting_row(L, entries[i]);
        x4n::lua::rawseti(L, -2, i + 1);
    }
    return 1;
}

static int l_set_extension_setting(lua_State* L) {
    const char* ext_id = x4n::lua::L_checkstring(L, 1);
    const char* key    = x4n::lua::L_checkstring(L, 2);

    SettingValueC v = {};
    int t = x4n::lua::type(L, 3);
    if (t == LUA_TBOOLEAN) {
        v.type = X4N_SETTING_TOGGLE;
        v.b    = x4n::lua::toboolean(L, 3);
    } else if (t == LUA_TNUMBER) {
        v.type = X4N_SETTING_SLIDER;
        v.d    = static_cast<double>(x4n::lua::tonumber(L, 3));
    } else if (t == LUA_TSTRING) {
        v.type = X4N_SETTING_DROPDOWN;
        v.s    = x4n::lua::tostring(L, 3);
    } else {
        return x4n::lua::L_error(L,
            "set_extension_setting: value must be boolean, number, or string");
    }

    if (g_dispatch.set_extension_setting)
        g_dispatch.set_extension_setting(ext_id, key, &v);
    return 0;
}

#ifdef X4N_WITH_RELOAD
static int l_should_autoreload(lua_State* L) {
    if (!g_autoreload_checked) {
        read_autoreload_setting();
        g_autoreload_checked = true;
    }
    if (!g_autoreload_enabled) {
        x4n::lua::pushboolean(L, 0);
        return 1;
    }
    x4n::lua::pushboolean(L, core_modified_since_last_check() ? 1 : 0);
    return 1;
}
#endif // X4N_WITH_RELOAD

// ---------------------------------------------------------------------------
// Entry point — called by Lua: package.loadlib("...dll", "luaopen_x4native")
// ---------------------------------------------------------------------------
extern "C" __declspec(dllexport)
int luaopen_x4native(lua_State* L) {
    g_lua = L;

    // Resolve Lua C API function pointers (idempotent)
    if (!x4n::lua::resolve()) {
        OutputDebugStringA("X4Native: FATAL — failed to resolve Lua API\n");
        return 0;
    }

    if (!g_initialized) {
        // --- First load (game start) ---
        g_ext_root       = detect_ext_root();
        g_core_path      = g_ext_root + "native\\x4native_core.dll";
        g_core_live_path = g_ext_root + "native\\x4native_core_live.dll";

        if (!load_core())
            return x4n::lua::L_error(L, "X4Native: failed to load core DLL");

        g_initialized = true;
    } else {
        // --- /reloadui or save load: proxy already loaded, update lua_State ---
        if (g_dispatch.set_lua_state)
            g_dispatch.set_lua_state(L);

        // Clear bridge cache — the old Lua state (and its RegisterEvent handlers)
        // is gone; bridges must re-register on the new state.
        g_lua_bridges.clear();

        // Hot-reload core if a newer build is on disk
        if (core_needs_reload())
            reload_core();
    }

    // Build the Lua API table returned to x4native.lua
    x4n::lua::newtable(L);

    struct { const char* name; lua_CFunction fn; } funcs[] = {
        { "discover_extensions",     l_discover_extensions     },
        { "raise_event",             l_raise_event             },
        { "raise_lua_event",         l_raise_lua_event         },
        { "log",                     l_log                     },
        { "get_version",             l_get_version             },
        { "get_loaded_extensions",   l_get_loaded_extensions   },
        { "reload",                  l_reload                  },
        { "prepare_reload",          l_prepare_reload          },
        { "get_extension_settings",  l_get_extension_settings  },
        { "set_extension_setting",   l_set_extension_setting   },
#ifdef X4N_WITH_RELOAD
        { "should_autoreload",       l_should_autoreload       },
#endif
    };

    for (auto& f : funcs) {
        x4n::lua::pushcfunction(L, f.fn);
        x4n::lua::setfield(L, -2, f.name);
    }

    return 1;  // one return value: the API table
}

// ---------------------------------------------------------------------------
// DllMain
// ---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        // Pin this DLL so LuaJIT's FreeLibrary (on lua_close during save
        // load) cannot unload us. This preserves all static state —
        // g_initialized, g_core_module, g_dispatch — across Lua state
        // destruction and recreation. Without this, save loads cause the
        // proxy to unload while core_live.dll remains file-locked, making
        // the next load_core() CopyFile fail.
        HMODULE pinned = nullptr;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCSTR>(hModule),
            &pinned);
    } else if (reason == DLL_PROCESS_DETACH) {
        if (reserved != nullptr) {
            // Process is terminating. Other DLLs' statics may already be
            // destroyed, all threads killed, heap potentially corrupted.
            // The OS reclaims all memory, handles, pipes, and file locks.
            // Companion detects broken pipe and exits on its own.
            return TRUE;
        }
        // Dynamic unload (FreeLibrary): safe to clean up.
        // Unreachable while pinned, but correct if pinning is ever removed.
        if (g_core_shutdown)
            g_core_shutdown();
        if (g_core_module) {
            FreeLibrary(g_core_module);
            g_core_module = nullptr;
        }
    }
    return TRUE;
}
