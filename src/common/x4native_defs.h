#pragma once
#include <cstdint>

#define X4NATIVE_API_VERSION 2
#define X4NATIVE_VERSION_MAJOR 0
#define X4NATIVE_VERSION_MINOR 9
#define X4NATIVE_VERSION_PATCH 0
#define X4NATIVE_VERSION_STR "0.9.0"

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
};

// Callback the proxy provides so the core can fire Lua events.
typedef int (*raise_lua_event_fn)(const char* name, const char* param);

// Callback the proxy provides so the core can register Lua→C++ bridges.
typedef int (*register_lua_bridge_fn)(const char* lua_event, const char* cpp_event);

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
};

// Core DLL exported function signatures
typedef int  (*core_init_fn)(CoreInitContext* ctx);
typedef void (*core_shutdown_fn)();
