// ---------------------------------------------------------------------------
// X4Native Extension SDK
// ---------------------------------------------------------------------------
//
// Include this header in your native extension DLL to integrate with
// X4Native. See the project documentation for the full guide.
//
// Minimal extension:
//
//   #include "x4native_extension.h"
//
//   X4NATIVE_EXPORT int x4native_api_version(void) {
//       return X4NATIVE_API_VERSION;
//   }
//
//   X4NATIVE_EXPORT int x4native_init(X4NativeAPI* api) {
//       api->log(X4NATIVE_LOG_INFO, "Hello from my extension!");
//       return X4NATIVE_OK;
//   }
//
//   X4NATIVE_EXPORT void x4native_shutdown(void) {}
//
// ---------------------------------------------------------------------------
#ifndef X4NATIVE_EXTENSION_H
#define X4NATIVE_EXTENSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// ---- Version & constants -------------------------------------------------
#define X4NATIVE_API_VERSION  1

#define X4NATIVE_OK     0
#define X4NATIVE_ERROR  1

#define X4NATIVE_LOG_DEBUG  0
#define X4NATIVE_LOG_INFO   1
#define X4NATIVE_LOG_WARN   2
#define X4NATIVE_LOG_ERROR  3

// ---- Export macro ---------------------------------------------------------
#ifdef _MSC_VER
  #define X4NATIVE_EXPORT extern "C" __declspec(dllexport)
#elif defined(__GNUC__)
  #define X4NATIVE_EXPORT extern "C" __attribute__((visibility("default")))
#else
  #define X4NATIVE_EXPORT extern "C"
#endif

// ---- Event callback ------------------------------------------------------
//   event_name:  Name of the event being fired (e.g. "on_game_loaded")
//   data:        Event-specific payload (may be NULL)
//   userdata:    Opaque pointer passed during subscribe()
typedef void (*X4NativeEventCallback)(const char* event_name,
                                      void* data,
                                      void* userdata);

// ---- Hook types (internal — wrapped by x4n::hook in x4native.h) ---------

// Context passed to raw hook callbacks (internal dispatcher type).
// Extensions never construct this — the framework fills it in.
typedef struct X4HookContext {
    const char* function_name;
    void**      args;            // Array of pointers to each argument
    void*       result;          // Pointer to return value buffer (zero-initialized)
    int         skip_original;   // OR-gate: any setter causes skip
    void*       userdata;
} X4HookContext;

// Raw hook callback signature. Returns 0 on success.
typedef int (*X4HookCallback)(X4HookContext* ctx);

// ---- Forward declaration for game function table -------------------------
// Include x4_game_func_table.h for the full struct definition.
// The struct contains both exported and internal (RE'd) functions.
struct X4GameFunctions;

// ---- API context ---------------------------------------------------------
// Passed to x4native_init(). Pointers remain valid until x4native_shutdown().
typedef struct X4NativeAPI {
    int api_version;

    // --- Event system ---
    int  (*subscribe)(const char* event_name,
                      X4NativeEventCallback callback,
                      void* userdata,
                      void* _api_ptr);
    void (*unsubscribe)(int subscription_id);
    void (*raise_event)(const char* event_name, void* data);

    // --- Lua bridge (outbound: C++ → Lua) ---
    // Dispatches to all Lua scripts that called RegisterEvent(name, fn).
    // Equivalent to MD's <raise_lua_event> but callable from C++.
    // param is a string argument passed to listeners (or NULL).
    // Must be called from UI thread (i.e. from within an event callback).
    // Returns 0 on success, non-zero on error.
    int  (*raise_lua_event)(const char* event_name, const char* param);

    // --- Lua bridge (inbound: Lua → C++) ---
    // Register a dynamic Lua→C++ event bridge. When lua_event fires in Lua,
    // the framework calls RegisterEvent and forwards it as cpp_event to the
    // C++ event bus. Returns 0 on success.
    int  (*register_lua_bridge)(const char* lua_event, const char* cpp_event);

    // --- Logging ---
    void (*log)(int level, const char* message);

    // --- Info ---
    const char* (*get_game_version)(void);
    const char* (*get_x4native_version)(void);
    const char* extension_path;                // Abs path to calling ext's folder

    // --- Game API (resolved at startup) ---
    // Type-safe table of 2000+ game function pointers.
    // Include x4_game_func_table.h and/or cache this pointer during init:
    //   X4GameFunctions* game = api->game;
    //   if (game->GetPlayerID) player = game->GetPlayerID();
    struct X4GameFunctions* game;

    // Named lookup for any exported function (typed or untyped).
    // Returns NULL if the function name is not found in X4.exe.
    void* (*get_game_function)(const char* name);

    // Number of function pointer slots in the game-> struct.
    // Extensions compiled against a newer SDK should check this before
    // accessing high-index fields that may not exist at runtime.
    int game_func_count;

    // X4.exe image base address. Use for resolving global RVAs:
    //   void* ptr = *(void**)(api->exe_base + MY_RVA);
    // Populated once at framework startup. Always non-zero after init.
    uintptr_t exe_base;

    // Game build number the types/functions were extracted from (e.g. 900).
    int game_types_build;

    // --- Hook system (internal — wrapped by x4n::hook in x4native.h) ---
    // Install a before-hook on a game function. Returns hook ID (>0) or -1.
    // _api_ptr: pointer to this X4NativeAPI (passed by SDK wrapper for context).
    int  (*hook_before)(const char* function_name,
                        X4HookCallback callback, void* userdata,
                        void* _api_ptr);
    // Install an after-hook on a game function. Returns hook ID (>0) or -1.
    int  (*hook_after)(const char* function_name,
                       X4HookCallback callback, void* userdata,
                       void* _api_ptr);
    // Remove a hook by ID.
    void (*unhook)(int hook_id);

    // Internal: typed-dispatch plumbing (called by x4native.h templates).
    // Do not call directly — use x4n::hook::before<>() / after<>() instead.
    void* (*_ensure_detour)(const char* function_name, void* detour_fn);
    void  (*_run_before_hooks)(X4HookContext* ctx);
    void  (*_run_after_hooks)(X4HookContext* ctx);

    // --- Internal (non-exported) function resolution ---
    // Resolve a non-exported game function by name via the RVA database
    // (native/version_db/internal_functions.json). Returns NULL if not found
    // for the current game version. Use this for reverse-engineered functions
    // that aren't in X4.exe's export table.
    void* (*resolve_internal)(const char* name);

    // --- MD event subscription (O(1) dispatch by type_id) ---
    // Subscribe to MD events before/after dispatch. Type IDs from x4_md_events.h.
    // Returns subscription ID (>0), or -1 on error. Unsubscribe via unsubscribe().
    int  (*md_subscribe_before)(uint32_t type_id,
                                X4NativeEventCallback callback, void* userdata,
                                void* _api_ptr);
    int  (*md_subscribe_after)(uint32_t type_id,
                               X4NativeEventCallback callback, void* userdata,
                               void* _api_ptr);

    // --- Stash (in-memory key-value, survives /reloadui and hot-reload) ---
    // Data lives in the proxy DLL (pinned in memory for the process lifetime).
    // Lost on game exit. Namespace isolates keys; by convention use your
    // extension name. The C++ SDK (x4n::stash) auto-namespaces for you.
    //
    // stash_set: copy data into the stash. Returns 1 on success, 0 on error.
    int         (*stash_set)(const char* ns, const char* key,
                             const void* data, uint32_t size);
    // stash_get: returns pointer to internal buffer (valid until next set/remove
    //            on the same key). Returns NULL if not found.
    const void* (*stash_get)(const char* ns, const char* key,
                             uint32_t* out_size);
    // stash_remove: remove a single key. Returns 1 if found, 0 otherwise.
    int         (*stash_remove)(const char* ns, const char* key);
    // stash_clear: remove all keys in a namespace.
    void        (*stash_clear)(const char* ns);

    // --- Per-extension context (set by framework during init) ---
    const char* _ext_name;              // Extension name (auto-used by x4n::stash as namespace)
    intptr_t    _ext_priority;          // Load priority
    void*       _ext_subscription_ids;  // vector<int>* — event subscription IDs for auto-cleanup
    void*       _ext_log_handle;        // HANDLE — per-extension log file
    void*       _ext_log_fn;            // fn(int,cstr,ptr) — write to extension's own log
    void*       _ext_init_log_fn;       // fn(cstr,ptr) — reinitialize log with a new filename
    void*       _ext_log_named_fn;      // fn(int,cstr,cstr,ptr) — one-shot write to named file
    void*       _ext_info;              // ExtensionInfo* — internal framework pointer

    // --- Runtime-resolved game offsets (populated by framework at startup) ---
    // Pre-computed struct of all version-dependent values.
    // Extensions read from this via x4n::offsets() — never need recompilation.
    const void* offsets;            // X4GameOffsets* — internal, used by SDK inline functions

    void* _reserved[11];
} X4NativeAPI;

// ---- Required exports from extension DLLs --------------------------------

// Return X4NATIVE_API_VERSION so the loader can check compatibility.
X4NATIVE_EXPORT int x4native_api_version(void);

// Called once when the extension is loaded. Register event subscriptions here.
// Return X4NATIVE_OK (0) on success.
X4NATIVE_EXPORT int x4native_init(X4NativeAPI* api);

// Called when the extension should release all resources.
X4NATIVE_EXPORT void x4native_shutdown(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // X4NATIVE_EXTENSION_H
