// ---------------------------------------------------------------------------
// x4n_core.h — Core API, detail plumbing, game access, info
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. All other x4n_*.h headers depend on this.
// Included automatically by x4native.h, or include directly for minimal builds.
//
// Provides:
//   x4n::detail::g_api     — API pointer (set by X4N_EXTENSION macro)
//   x4n::game()            — typed game function table
//   x4n::game_fn()         — named function lookup
//   x4n::game_internal()   — internal function lookup
//   x4n::exe_base()        — X4.exe image base
//   x4n::game_version()    — game version string
//   x4n::version()         — X4Native version string
//   x4n::path()            — extension folder path
// ---------------------------------------------------------------------------
#pragma once

#include "x4native_extension.h"
#include "x4_game_func_table.h"
#include "x4_manual_types.h"

#include <cstdio>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>

namespace x4n {

// ---------------------------------------------------------------------------
// detail — internal plumbing, not for direct use
// ---------------------------------------------------------------------------
namespace detail {

    // API pointer set by X4N_EXTENSION macro during x4native_init()
    inline X4NativeAPI* g_api = nullptr;

    // Trampoline: adapts void() callback to X4NativeEventCallback signature
    inline void trampoline_void(const char*, void*, void* ud) {
        reinterpret_cast<void(*)()>(ud)();
    }

    // Trampoline: adapts void(void*) callback to X4NativeEventCallback signature
    inline void trampoline_data(const char*, void* data, void* ud) {
        reinterpret_cast<void(*)(void*)>(ud)(data);
    }

    // Trampoline: adapts void(const X4NativeFrameUpdate*) for native frame update events
    inline void trampoline_frame_update(const char*, void* data, void* ud) {
        reinterpret_cast<void(*)(const X4NativeFrameUpdate*)>(ud)(
            static_cast<const X4NativeFrameUpdate*>(data));
    }

    // Trampoline: adapts void(const char*) callback for string event params
    inline void trampoline_str(const char*, void* data, void* ud) {
        reinterpret_cast<void(*)(const char*)>(ud)(
            static_cast<const char*>(data));
    }

    // [OBSOLETE] Use x4n::md::on_radar_visibility_changed_before() instead.
    inline void trampoline_radar_changed(const char*, void* data, void* ud) {
        reinterpret_cast<void(*)(const X4RadarChangedEvent*)>(ud)(
            static_cast<const X4RadarChangedEvent*>(data));
    }

} // namespace detail

// ---------------------------------------------------------------------------
// Game API
// ---------------------------------------------------------------------------

/// Typed game function table (exported + internal entries).
inline X4GameFunctions* game() {
    return detail::g_api->game;
}

/// Named lookup for any exported function (returns NULL if not found).
inline void* game_fn(const char* name) {
    return detail::g_api->get_game_function(name);
}

/// Resolve a non-exported (internal) game function by name.
inline void* game_internal(const char* name) {
    return detail::g_api->resolve_internal(name);
}

// ---------------------------------------------------------------------------
// Info
// ---------------------------------------------------------------------------

/// Game version string (e.g. "9.00")
inline const char* game_version() { return detail::g_api->get_game_version(); }

/// X4Native framework version string
inline const char* version() { return detail::g_api->get_x4native_version(); }

/// Absolute path to the calling extension's folder
inline const char* path() { return detail::g_api->extension_path; }

/// X4.exe image base address. Use for resolving global RVAs:
///   auto ptr = *reinterpret_cast<void**>(x4n::exe_base() + MY_RVA);
inline uintptr_t exe_base() { return detail::g_api->exe_base; }

} // namespace x4n

// ===========================================================================
// Extension lifecycle macros
// ===========================================================================

// ---------------------------------------------------------------------------
// X4N_EXTENSION { ... }
//
// Generates x4native_init() and x4native_api_version() DLL exports.
// The body runs once when the extension is loaded by X4Native.
// Use x4n::* functions inside — the API pointer is already set.
//
//   X4N_EXTENSION {
//       x4n::on("on_game_loaded", my_handler);
//   }
// ---------------------------------------------------------------------------
#define X4N_EXTENSION                                                       \
    static void _x4n_user_init();                                           \
    X4NATIVE_EXPORT int x4native_api_version(void) {                        \
        return X4NATIVE_API_VERSION;                                        \
    }                                                                       \
    X4NATIVE_EXPORT int x4native_init(X4NativeAPI* _x4n_api) {              \
        ::x4n::detail::g_api = _x4n_api;                                    \
        _x4n_user_init();                                                   \
        return X4NATIVE_OK;                                                 \
    }                                                                       \
    void _x4n_user_init()

// ---------------------------------------------------------------------------
// X4N_SHUTDOWN { ... }
//
// Generates x4native_shutdown() DLL export.
// Optional — if omitted, no shutdown export is generated (the loader
// handles this gracefully). The API pointer is cleared automatically.
//
//   X4N_SHUTDOWN {
//       x4n::off(my_sub_id);
//   }
// ---------------------------------------------------------------------------
#define X4N_SHUTDOWN                                                        \
    static void _x4n_user_shutdown();                                       \
    X4NATIVE_EXPORT void x4native_shutdown(void) {                          \
        _x4n_user_shutdown();                                               \
        ::x4n::detail::g_api = nullptr;                                     \
    }                                                                       \
    void _x4n_user_shutdown()

