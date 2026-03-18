// ---------------------------------------------------------------------------
// X4Native Extension SDK — Modern C++ API
// ---------------------------------------------------------------------------
//
// Single-include header for X4Native extension development.
// Wraps the internal C ABI (X4NativeAPI) into a clean x4n:: namespace.
//
// Minimal extension:
//
//   #include <x4native.h>
//
//   X4N_EXTENSION {
//       x4n::log::info("Hello from my extension!");
//       x4n::on("on_game_loaded", [] {
//           x4n::log::info("Game loaded!");
//       });
//   }
//
// See docs/SDK_CONTRACT.md for the full API reference.
// ---------------------------------------------------------------------------
#ifndef X4NATIVE_H
#define X4NATIVE_H

// Internal C ABI layer (X4NativeAPI struct, constants, export macro)
#include "x4native_extension.h"
// Typed game function table (X4GameFunctions struct — exported + internal entries)
#include "x4_game_func_table.h"
// Hand-authored RE'd types (X4NativeFrameUpdate, etc.)
#include "x4_manual_types.h"

#include <cstdio>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>

// ===========================================================================
// x4n namespace — primary API surface for extension developers
// ===========================================================================
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

} // namespace detail

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

/// Subscribe with a void() callback (ignores event name and data).
inline int on(const char* name, void(*callback)()) {
    return detail::g_api->subscribe(
        name, detail::trampoline_void, reinterpret_cast<void*>(callback),
        detail::g_api);
}

/// Subscribe with a void(void*) callback (receives event data pointer).
inline int on(const char* name, void(*callback)(void*)) {
    return detail::g_api->subscribe(
        name, detail::trampoline_data, reinterpret_cast<void*>(callback),
        detail::g_api);
}

/// Subscribe with a typed X4NativeFrameUpdate callback (for on_native_frame_update).
inline int on(const char* name, void(*callback)(const X4NativeFrameUpdate*)) {
    return detail::g_api->subscribe(
        name, detail::trampoline_frame_update, reinterpret_cast<void*>(callback),
        detail::g_api);
}

/// Subscribe with a void(const char*) callback (receives string param from Lua bridges).
inline int on(const char* name, void(*callback)(const char*)) {
    return detail::g_api->subscribe(
        name, detail::trampoline_str, reinterpret_cast<void*>(callback),
        detail::g_api);
}

/// Subscribe with the raw 3-argument callback and explicit userdata.
inline int on(const char* name, X4NativeEventCallback callback,
              void* userdata = nullptr) {
    return detail::g_api->subscribe(name, callback, userdata, detail::g_api);
}

/// Unsubscribe by subscription ID (returned by on()).
inline void off(int subscription_id) {
    detail::g_api->unsubscribe(subscription_id);
}

/// Raise a C++ event (dispatched to all subscribers).
inline void raise(const char* name, void* data = nullptr) {
    detail::g_api->raise_event(name, data);
}

/// Raise a Lua event (C++ -> Lua bridge via CallEventScripts).
/// Must be called from UI thread. Returns 0 on success.
inline int raise_lua(const char* name, const char* param = nullptr) {
    return detail::g_api->raise_lua_event(name, param);
}

/// Register a dynamic Lua→C++ event bridge.
/// When the Lua event fires, the framework will raise the C++ event.
/// Returns 0 on success. Must be called from UI thread (during init is fine).
inline int bridge_lua_event(const char* lua_event, const char* cpp_event) {
    return detail::g_api->register_lua_bridge(lua_event, cpp_event);
}

// ---------------------------------------------------------------------------
// Game API
// ---------------------------------------------------------------------------

/// Typed game function table (exported + internal entries).
/// Cache this pointer during init for performance.
inline X4GameFunctions* game() {
    return detail::g_api->game;
}

/// Named lookup for any exported function (returns NULL if not found).
inline void* game_fn(const char* name) {
    return detail::g_api->get_game_function(name);
}

/// Resolve a non-exported (internal) game function by name.
/// Uses the RVA database (native/version_db/internal_functions.json).
/// Returns the resolved address, or nullptr if not found for this game version.
/// Cast the result to a typed function pointer to call it.
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

// ---------------------------------------------------------------------------
// Logging — x4n::log::info("format %s", arg), etc.
//
// Default target:  per-extension log file (<ext_folder>/<name>.log).
//                  Opened by the framework at load time — no setup needed.
//
// Overloads:
//   info("text", false)        — route this message to the global x4native.log
//   info("text", true)         — same as info("text") — explicit default
//   info("text", "extra.log")  — one-shot write to a named file in the ext folder
//                                NOTE: second arg is a filename, not a format arg.
//                                Use info("fmt %s", str) for formatted string args.
//
// set_log_file("custom.log")   — reopen the extension's log under a new filename
//                                (relative paths resolve inside the extension folder)
// ---------------------------------------------------------------------------
namespace log {
namespace detail {

    // Write a formatted message to the extension's own log (via _reserved[4]).
    // Falls back to the global framework log if the slot is not populated.
    template<typename... Args>
    inline void write(int level, const char* fmt, Args... args) {
        char buf[1024];
        const char* msg = fmt;
        if constexpr (sizeof...(args) > 0) {
            snprintf(buf, sizeof(buf), fmt, args...);
            msg = buf;
        }
        auto* api = ::x4n::detail::g_api;
        auto  fn  = reinterpret_cast<void(*)(int, const char*, void*)>(api->_reserved[4]);
        if (fn) fn(level, msg, api);
        else    api->log(level, msg);
    }

    // Write directly to the global x4native.log (bypasses per-extension file).
    inline void write_global(int level, const char* msg) {
        ::x4n::detail::g_api->log(level, msg);
    }

    // Write to a named file in the extension's folder (one-shot, via _reserved[6]).
    inline void write_named(int level, const char* msg, const char* filename) {
        auto* api = ::x4n::detail::g_api;
        auto  fn  = reinterpret_cast<void(*)(int, const char*, const char*, void*)>(
                        api->_reserved[6]);
        if (fn) fn(level, msg, filename, api);
        else    write(level, msg);
    }

} // namespace detail

// Redirect all subsequent log calls to a different file inside the extension folder.
// Closes the current log, rotates it, and opens the new one.
// Relative paths are resolved against the extension folder; absolute paths are used as-is.
inline void set_log_file(const char* filename) {
    auto* api = ::x4n::detail::g_api;
    auto  fn  = reinterpret_cast<void(*)(const char*, void*)>(api->_reserved[5]);
    if (fn) fn(filename, api);
}

// ---------------------------------------------------------------------------
// debug / info / warn / error
//
// Three call forms per level:
//   level("fmt", args...)          — format + args → extension's own log
//   level("text", bool)            — bool false → global log; true → extension log
//   level("text", "filename")      — one-shot write to named file in ext folder
// ---------------------------------------------------------------------------

template<typename... Args>
inline void debug(const char* fmt, Args... args) {
    detail::write(X4NATIVE_LOG_DEBUG, fmt, args...);
}
inline void debug(const char* msg, bool to_ext_log) {
    if (to_ext_log) detail::write(X4NATIVE_LOG_DEBUG, msg);
    else            detail::write_global(X4NATIVE_LOG_DEBUG, msg);
}
inline void debug(const char* msg, const char* filename) {
    detail::write_named(X4NATIVE_LOG_DEBUG, msg, filename);
}

template<typename... Args>
inline void info(const char* fmt, Args... args) {
    detail::write(X4NATIVE_LOG_INFO, fmt, args...);
}
inline void info(const char* msg, bool to_ext_log) {
    if (to_ext_log) detail::write(X4NATIVE_LOG_INFO, msg);
    else            detail::write_global(X4NATIVE_LOG_INFO, msg);
}
inline void info(const char* msg, const char* filename) {
    detail::write_named(X4NATIVE_LOG_INFO, msg, filename);
}

template<typename... Args>
inline void warn(const char* fmt, Args... args) {
    detail::write(X4NATIVE_LOG_WARN, fmt, args...);
}
inline void warn(const char* msg, bool to_ext_log) {
    if (to_ext_log) detail::write(X4NATIVE_LOG_WARN, msg);
    else            detail::write_global(X4NATIVE_LOG_WARN, msg);
}
inline void warn(const char* msg, const char* filename) {
    detail::write_named(X4NATIVE_LOG_WARN, msg, filename);
}

template<typename... Args>
inline void error(const char* fmt, Args... args) {
    detail::write(X4NATIVE_LOG_ERROR, fmt, args...);
}
inline void error(const char* msg, bool to_ext_log) {
    if (to_ext_log) detail::write(X4NATIVE_LOG_ERROR, msg);
    else            detail::write_global(X4NATIVE_LOG_ERROR, msg);
}
inline void error(const char* msg, const char* filename) {
    detail::write_named(X4NATIVE_LOG_ERROR, msg, filename);
}

} // namespace log

// ---------------------------------------------------------------------------
// Hooks — x4n::hook::before<&X4GameFunctions::Fn>(callback), etc.
// ---------------------------------------------------------------------------
namespace hook {

// HookControl — passed to before-hook callbacks
struct HookControl {
    int& skip_original;

    template<typename T>
    void set_result(const T& val) {
        if (_ctx->result)
            *static_cast<T*>(_ctx->result) = val;
        skip_original = 1;
    }

    explicit HookControl(X4HookContext* ctx)
        : skip_original(ctx->skip_original), _ctx(ctx) {}

private:
    X4HookContext* _ctx;
};

namespace detail {

// --- Type trait helpers ---
template<typename T> struct member_type;
template<typename T, typename C> struct member_type<T C::*> { using type = T; };

template<typename T> struct fn_traits;
template<typename R, typename... A> struct fn_traits<R(*)(A...)> {
    using ret = R;
    using args = std::tuple<A...>;
    static constexpr bool is_void_ret = std::is_void_v<R>;
    static constexpr size_t arity = sizeof...(A);
};

// --- Per-member-pointer static state (one per hooked field) ---
template<auto MPtr>
struct state {
    static inline void* trampoline = nullptr;
    static inline const char* name = nullptr;
    static inline bool detour_installed = false;
};

// --- Function name lookup from X4GameFunctions member offset ---
struct FuncNameEntry { size_t offset; const char* name; };

#define X4_FUNC(ret, name, params) { offsetof(X4GameFunctions, name), #name },
inline const FuncNameEntry g_func_names[] = {
#include "x4_game_func_list.inc"
#include "x4_internal_func_list.inc"
};
#undef X4_FUNC

inline const char* name_from_offset(size_t off) {
    for (const auto& e : g_func_names)
        if (e.offset == off) return e.name;
    return nullptr;
}

template<auto MPtr>
size_t get_offset() {
    static const size_t off = [] {
        X4GameFunctions dummy;
        std::memset(&dummy, 0, sizeof(dummy));
        auto* member = &(dummy.*MPtr);
        return static_cast<size_t>(
            reinterpret_cast<char*>(member) - reinterpret_cast<char*>(&dummy));
    }();
    return off;
}

template<auto MPtr>
const char* get_name() {
    if (!state<MPtr>::name)
        state<MPtr>::name = name_from_offset(get_offset<MPtr>());
    return state<MPtr>::name;
}

template<auto MPtr>
bool install_detour(void* detour_fn) {
    if (state<MPtr>::detour_installed) return true;
    void* t = ::x4n::detail::g_api->_ensure_detour(get_name<MPtr>(), detour_fn);
    if (!t) return false;
    state<MPtr>::trampoline = t;
    state<MPtr>::detour_installed = true;
    return true;
}

// --- Typed detour functions (generated per unique member pointer) ---

// Non-void return
template<auto MPtr, typename Ret, typename... Args>
Ret typed_detour(Args... args) {
    void* arg_ptrs[] = { static_cast<void*>(&args)..., nullptr };
    Ret result{};

    X4HookContext ctx{};
    ctx.function_name = state<MPtr>::name;
    ctx.args = arg_ptrs;
    ctx.result = &result;
    ctx.skip_original = 0;

    ::x4n::detail::g_api->_run_before_hooks(&ctx);

    if (!ctx.skip_original) {
        auto orig = reinterpret_cast<Ret(*)(Args...)>(state<MPtr>::trampoline);
        result = orig(args...);
    }

    ctx.result = &result;
    ::x4n::detail::g_api->_run_after_hooks(&ctx);

    return result;
}

// Void return
template<auto MPtr, typename... Args>
void typed_detour_void(Args... args) {
    void* arg_ptrs[] = { static_cast<void*>(&args)..., nullptr };

    X4HookContext ctx{};
    ctx.function_name = state<MPtr>::name;
    ctx.args = arg_ptrs;
    ctx.result = nullptr;
    ctx.skip_original = 0;

    ::x4n::detail::g_api->_run_before_hooks(&ctx);

    if (!ctx.skip_original) {
        auto orig = reinterpret_cast<void(*)(Args...)>(state<MPtr>::trampoline);
        orig(args...);
    }

    ::x4n::detail::g_api->_run_after_hooks(&ctx);
}

// Select the right detour based on return type
template<auto MPtr, typename FnPtr> struct detour_selector;
template<auto MPtr, typename Ret, typename... Args>
struct detour_selector<MPtr, Ret(*)(Args...)> {
    static void* get() {
        if constexpr (std::is_void_v<Ret>)
            return reinterpret_cast<void*>(&typed_detour_void<MPtr, Args...>);
        else
            return reinterpret_cast<void*>(&typed_detour<MPtr, Ret, Args...>);
    }
};

// --- Before-hook adapter: user void(HookControl&, Args&...) → raw int(X4HookContext*) ---
template<typename UserFn, typename... Args>
struct before_adapter_impl {
    static int raw(X4HookContext* ctx) {
        auto fn = reinterpret_cast<UserFn>(ctx->userdata);
        HookControl ctl(ctx);
        [&]<size_t... I>(std::index_sequence<I...>) {
            fn(ctl, *static_cast<Args*>(ctx->args[I])...);
        }(std::make_index_sequence<sizeof...(Args)>{});
        return 0;
    }
};

template<typename UserFn, typename FnPtr> struct before_adapter;
template<typename UserFn, typename Ret, typename... Args>
struct before_adapter<UserFn, Ret(*)(Args...)> {
    static constexpr X4HookCallback get() {
        return &before_adapter_impl<UserFn, Args...>::raw;
    }
};

// --- After-hook adapter (non-void): user void(Ret&, Args...) → raw int(X4HookContext*) ---
template<typename UserFn, typename Ret, typename... Args>
struct after_adapter_nonvoid {
    static int raw(X4HookContext* ctx) {
        auto fn = reinterpret_cast<UserFn>(ctx->userdata);
        Ret& result = *static_cast<Ret*>(ctx->result);
        [&]<size_t... I>(std::index_sequence<I...>) {
            fn(result, *static_cast<Args*>(ctx->args[I])...);
        }(std::make_index_sequence<sizeof...(Args)>{});
        return 0;
    }
};

// --- After-hook adapter (void): user void(Args...) → raw int(X4HookContext*) ---
template<typename UserFn, typename... Args>
struct after_adapter_void {
    static int raw(X4HookContext* ctx) {
        auto fn = reinterpret_cast<UserFn>(ctx->userdata);
        [&]<size_t... I>(std::index_sequence<I...>) {
            fn(*static_cast<Args*>(ctx->args[I])...);
        }(std::make_index_sequence<sizeof...(Args)>{});
        return 0;
    }
};

template<typename UserFn, typename FnPtr> struct after_adapter;
template<typename UserFn, typename Ret, typename... Args>
struct after_adapter<UserFn, Ret(*)(Args...)> {
    static constexpr X4HookCallback get() {
        if constexpr (std::is_void_v<Ret>)
            return &after_adapter_void<UserFn, Args...>::raw;
        else
            return &after_adapter_nonvoid<UserFn, Ret, Args...>::raw;
    }
};

} // namespace detail

// ---------------------------------------------------------------------------
// Public hook API
// ---------------------------------------------------------------------------

/// Install a before-hook on a game function. Returns hook ID (>0) or -1.
/// Callback: void(HookControl&, ArgTypes&...)
template<auto MemberPtr, typename BeforeFn>
int before(BeforeFn fn) {
    using MemberType = typename detail::member_type<decltype(MemberPtr)>::type;
    auto fp = +fn;  // convert stateless lambda to function pointer
    using FpType = decltype(fp);

    void* detour = detail::detour_selector<MemberPtr, MemberType>::get();
    if (!detail::install_detour<MemberPtr>(detour)) return -1;

    X4HookCallback raw_cb = detail::before_adapter<FpType, MemberType>::get();

    return ::x4n::detail::g_api->hook_before(
        detail::get_name<MemberPtr>(),
        raw_cb,
        reinterpret_cast<void*>(fp),
        ::x4n::detail::g_api);
}

/// Install an after-hook on a game function. Returns hook ID (>0) or -1.
/// Callback: void(RetType&, ArgTypes...) for non-void, void(ArgTypes...) for void.
template<auto MemberPtr, typename AfterFn>
int after(AfterFn fn) {
    using MemberType = typename detail::member_type<decltype(MemberPtr)>::type;
    auto fp = +fn;
    using FpType = decltype(fp);

    void* detour = detail::detour_selector<MemberPtr, MemberType>::get();
    if (!detail::install_detour<MemberPtr>(detour)) return -1;

    X4HookCallback raw_cb = detail::after_adapter<FpType, MemberType>::get();

    return ::x4n::detail::g_api->hook_after(
        detail::get_name<MemberPtr>(),
        raw_cb,
        reinterpret_cast<void*>(fp),
        ::x4n::detail::g_api);
}

/// Remove a hook by ID.
inline void remove(int hook_id) {
    ::x4n::detail::g_api->unhook(hook_id);
}

} // namespace hook

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

#endif // X4NATIVE_H
