#pragma once
#include <cstdint>

// Public constants (X4N_SETTING_*, X4NativeSettingChanged) live in the SDK.
#include <x4native_extension.h>

// ---------------------------------------------------------------------------
// Per-extension settings — ABI shared between proxy and core
// ---------------------------------------------------------------------------
// Core owns the backing data; proxy reads through `SettingInfo*` when
// building Lua tables. Pointers inside SettingInfo (id/name/strings/options)
// remain valid until the extension is unloaded or the value is re-set.

struct SettingOptionC {
    const char* id;
    const char* text;
};

struct SettingInfo {
    const char* id;
    const char* name;
    int         type;              // X4N_SETTING_*

    // Current value — only the field matching `type` is meaningful.
    int         current_bool;      // X4N_SETTING_TOGGLE
    double      current_number;    // X4N_SETTING_SLIDER
    const char* current_string;    // X4N_SETTING_DROPDOWN (option id)

    // Default value (populated for all types).
    int         default_bool;
    double      default_number;
    const char* default_string;

    // Dropdown options (type == X4N_SETTING_DROPDOWN).
    const SettingOptionC* options;
    int         option_count;

    // Slider bounds (type == X4N_SETTING_SLIDER).
    double      min;
    double      max;
    double      step;
};

// Tagged value passed to set_extension_setting. type must match the schema.
struct SettingValueC {
    int         type;   // X4N_SETTING_*
    int         b;      // bool (X4N_SETTING_TOGGLE)
    double      d;      // number (X4N_SETTING_SLIDER)
    const char* s;      // string (X4N_SETTING_DROPDOWN)
};

// ---------------------------------------------------------------------------
// Interface between proxy and core DLL.
// The proxy owns the CoreDispatch struct; the core fills it with function
// pointers during core_init(). All Lua → DLL calls go through this table,
// so reloading the core DLL just re-fills the pointers.
// ---------------------------------------------------------------------------
struct CoreDispatch {
    void        (*discover_extensions)();
    void        (*raise_event)(const char* event_name, const char* param);
    const char* (*get_version)();
    const char* (*get_loaded_extensions)();  // Returns JSON array string
    void        (*set_lua_state)(void* L);
    void        (*prepare_reload)();
    void        (*shutdown)();
    void        (*log)(int level, const char* message);

    // Per-extension settings
    // enumerate_settings returns the number of settings declared by the given
    // extension and fills *out with a pointer to core's internal array.
    // Pointer stays valid until the extension is unloaded.
    int         (*enumerate_settings)(const char* ext_id, const SettingInfo** out);
    // Write a value. type mismatch or unknown key is logged and ignored.
    void        (*set_extension_setting)(const char* ext_id, const char* key,
                                         const SettingValueC* value);
};

// Callback the proxy provides so the core can fire Lua events.
typedef int (*raise_lua_event_fn)(const char* name, const char* param);

// Callback the proxy provides so the core can register Lua→C++ bridges.
typedef int (*register_lua_bridge_fn)(const char* lua_event, const char* cpp_event);

// Callbacks the proxy provides so extensions can read engine values exposed
// only as Lua properties (Get*Data-style host C-funcs). Generic across all
// future accessors — signature matches X4NativeAPI.get_lua_property[_str].
typedef bool (*get_lua_property_fn)(const char* getter_fn, X4nLuaKey key,
                                    const char* field, X4nLuaValueType vt,
                                    void* out);
typedef bool (*get_lua_property_str_fn)(const char* getter_fn, X4nLuaKey key,
                                        const char* field,
                                        char* out_buf, size_t buf_size);

// Stash (proxy-owned in-memory key-value, survives /reloadui and hot-reload)
// Namespace isolates keys per extension; extensions may read other namespaces.
typedef int         (*stash_set_fn)(const char* ns, const char* key, const void* data, uint32_t size);
typedef const void* (*stash_get_fn)(const char* ns, const char* key, uint32_t* out_size);
typedef int         (*stash_remove_fn)(const char* ns, const char* key);
typedef void        (*stash_clear_fn)(const char* ns);

// Passed from proxy to core during core_init()
struct CoreInitContext {
    void*         lua_state;    // lua_State* (void* to avoid lua header dep in core)
    const char*   ext_root;     // Absolute path to extensions/x4native/
    CoreDispatch* dispatch;     // Proxy-owned table for core to fill
    raise_lua_event_fn raise_lua_event;  // Proxy-implemented Lua bridge
    register_lua_bridge_fn register_lua_bridge;  // Proxy-implemented Lua→C++ bridge registration

    // Stash (proxy-owned in-memory key-value, survives reloads)
    stash_set_fn    stash_set;
    stash_get_fn    stash_get;
    stash_remove_fn stash_remove;
    stash_clear_fn  stash_clear;

    // Lua-property accessors (proxy-implemented, generic over all Get*Data-
    // style host C-funcs). Core forwards them through to extensions verbatim.
    get_lua_property_fn     get_lua_property;
    get_lua_property_str_fn get_lua_property_str;
};

// Core DLL exported function signatures
typedef int  (*core_init_fn)(CoreInitContext* ctx);
typedef void (*core_shutdown_fn)();
