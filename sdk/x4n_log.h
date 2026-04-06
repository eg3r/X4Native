// ---------------------------------------------------------------------------
// x4n_log.h — Logging API
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Usage:
//   x4n::log::info("format %s", arg);
//   x4n::log::warn("text", false);       // route to global x4native.log
//   x4n::log::error("text", "extra.log"); // one-shot write to named file
//   x4n::log::set_log_file("custom.log"); // redirect extension log
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"

namespace x4n {
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
        auto  fn  = reinterpret_cast<void(*)(int, const char*, void*)>(api->_ext_log_fn);
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
                        api->_ext_log_named_fn);
        if (fn) fn(level, msg, filename, api);
        else    write(level, msg);
    }

} // namespace detail

// Redirect all subsequent log calls to a different file inside the extension folder.
inline void set_log_file(const char* filename) {
    auto* api = ::x4n::detail::g_api;
    auto  fn  = reinterpret_cast<void(*)(const char*, void*)>(api->_ext_init_log_fn);
    if (fn) fn(filename, api);
}

// ---------------------------------------------------------------------------
// debug / info / warn / error
//
// Three call forms per level:
//   level("fmt", args...)          — format + args -> extension's own log
//   level("text", bool)            — bool false -> global log; true -> extension log
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
} // namespace x4n

