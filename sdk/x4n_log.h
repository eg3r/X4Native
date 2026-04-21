// ---------------------------------------------------------------------------
// x4n_log.h — Logging API
// ---------------------------------------------------------------------------
// Part of the X4Native SDK. Included by x4native.h.
//
// Every level verb at the top level writes to THIS extension's own log file
// (`<profile>\x4native\<extension_id>.log`). Alternate destinations are
// reached through explicitly-named sink objects — sink-first, level-second,
// matching the convention used by spdlog, Python logging, Go slog, etc.
//
// Format strings use std::format's `{}` placeholders (NOT printf `%s`).
// The old variadic-printf API was removed because a `const char*` second
// argument was ambiguous with the "filename" overload and silently created
// files named after argument values.
//
// Usage:
//
//   x4n::log::info ("hello");                          // default (ext log)
//   x4n::log::info ("version: {}",  x4n::game_version());
//   x4n::log::warn ("odd value: {:.3f}", v);
//   x4n::log::error("failed: {}", err_code);
//
//   x4n::log::global.info("framework-visible note");        // x4native.log
//   x4n::log::to_file("events.log").info("event: {}", id);
//       → <profile>\x4native\<extension_id>\events.log
//
//   x4n::log::set_log_file("mymod_v2.log");  // redirect this ext's default
// ---------------------------------------------------------------------------
#pragma once

#include "x4n_core.h"

#include <cstdio>
#include <format>
#include <string_view>
#include <utility>

namespace x4n {
namespace log {

namespace detail {

    // Levels — must match X4NATIVE_LOG_* in x4native_extension.h.
    constexpr int LV_DEBUG = X4NATIVE_LOG_DEBUG;
    constexpr int LV_INFO  = X4NATIVE_LOG_INFO;
    constexpr int LV_WARN  = X4NATIVE_LOG_WARN;
    constexpr int LV_ERROR = X4NATIVE_LOG_ERROR;

    // Format into a fixed-size stack buffer (same cost as the old printf path).
    // Falls back to std::format if the untruncated message would exceed the
    // buffer — format_to_n's `size` field reports the *required* length, not
    // the truncated one, so that's our overflow signal.
    template<class... Args>
    inline std::string format_msg(std::format_string<Args...> fmt, Args&&... args) {
        char buf[1024];
        auto result = std::format_to_n(buf, sizeof(buf) - 1, fmt,
                                       std::forward<Args>(args)...);
        if (result.size <= static_cast<std::ptrdiff_t>(sizeof(buf) - 1))
            return std::string(buf, result.out);
        // Overflow: format into an allocating string.
        return std::format(fmt, std::forward<Args>(args)...);
    }

    // Write to the calling extension's log file (routes via _ext_log_fn).
    inline void write_ext(int level, std::string_view msg) {
        auto* api = ::x4n::detail::g_api;
        auto fn = reinterpret_cast<void(*)(int, const char*, void*)>(api->_ext_log_fn);
        if (fn) fn(level, std::string(msg).c_str(), api);
        else    api->log(level, std::string(msg).c_str());
    }

    // Write to the framework log (x4native.log).
    inline void write_global(int level, std::string_view msg) {
        ::x4n::detail::g_api->log(level, std::string(msg).c_str());
    }

    // Write to a named file in the extension's folder.
    inline void write_named(int level, const char* filename, std::string_view msg) {
        auto* api = ::x4n::detail::g_api;
        auto fn = reinterpret_cast<void(*)(int, const char*, const char*, void*)>(
                       api->_ext_log_named_fn);
        if (fn) fn(level, std::string(msg).c_str(), filename, api);
        else    write_ext(level, msg);
    }

} // namespace detail

// ---------------------------------------------------------------------------
// Default sink: this extension's own log file.
//
// Two forms per level:
//   level(fmt, args...)    — std::format-style formatting (compile-time fmt).
//   level(msg)             — pre-formatted runtime string (no formatting).
//
// The single-arg form accepts `std::string_view`, `const char*`, or `std::string`
// and is chosen automatically by overload resolution for non-literal arguments.
// ---------------------------------------------------------------------------

inline void debug(std::string_view msg) { detail::write_ext(detail::LV_DEBUG, msg); }
inline void info (std::string_view msg) { detail::write_ext(detail::LV_INFO,  msg); }
inline void warn (std::string_view msg) { detail::write_ext(detail::LV_WARN,  msg); }
inline void error(std::string_view msg) { detail::write_ext(detail::LV_ERROR, msg); }

template<class... Args>
    requires (sizeof...(Args) > 0)
inline void debug(std::format_string<Args...> fmt, Args&&... args) {
    detail::write_ext(detail::LV_DEBUG,
                      detail::format_msg(fmt, std::forward<Args>(args)...));
}
template<class... Args>
    requires (sizeof...(Args) > 0)
inline void info(std::format_string<Args...> fmt, Args&&... args) {
    detail::write_ext(detail::LV_INFO,
                      detail::format_msg(fmt, std::forward<Args>(args)...));
}
template<class... Args>
    requires (sizeof...(Args) > 0)
inline void warn(std::format_string<Args...> fmt, Args&&... args) {
    detail::write_ext(detail::LV_WARN,
                      detail::format_msg(fmt, std::forward<Args>(args)...));
}
template<class... Args>
    requires (sizeof...(Args) > 0)
inline void error(std::format_string<Args...> fmt, Args&&... args) {
    detail::write_ext(detail::LV_ERROR,
                      detail::format_msg(fmt, std::forward<Args>(args)...));
}

// ---------------------------------------------------------------------------
// Redirect the default log file for this extension.
// filename is relative to the per-extension log folder (or absolute).
// ---------------------------------------------------------------------------
inline void set_log_file(const char* filename) {
    auto* api = ::x4n::detail::g_api;
    auto fn = reinterpret_cast<void(*)(const char*, void*)>(api->_ext_init_log_fn);
    if (fn) fn(filename, api);
}

// ---------------------------------------------------------------------------
// Sink: framework log (x4native.log). Use sparingly — most extension logs
// should be scoped to the extension's own file.
// ---------------------------------------------------------------------------
struct global_sink {
    void debug(std::string_view msg) const { detail::write_global(detail::LV_DEBUG, msg); }
    void info (std::string_view msg) const { detail::write_global(detail::LV_INFO,  msg); }
    void warn (std::string_view msg) const { detail::write_global(detail::LV_WARN,  msg); }
    void error(std::string_view msg) const { detail::write_global(detail::LV_ERROR, msg); }

    template<class... Args> requires (sizeof...(Args) > 0)
    void debug(std::format_string<Args...> fmt, Args&&... args) const {
        detail::write_global(detail::LV_DEBUG,
                             detail::format_msg(fmt, std::forward<Args>(args)...));
    }
    template<class... Args> requires (sizeof...(Args) > 0)
    void info(std::format_string<Args...> fmt, Args&&... args) const {
        detail::write_global(detail::LV_INFO,
                             detail::format_msg(fmt, std::forward<Args>(args)...));
    }
    template<class... Args> requires (sizeof...(Args) > 0)
    void warn(std::format_string<Args...> fmt, Args&&... args) const {
        detail::write_global(detail::LV_WARN,
                             detail::format_msg(fmt, std::forward<Args>(args)...));
    }
    template<class... Args> requires (sizeof...(Args) > 0)
    void error(std::format_string<Args...> fmt, Args&&... args) const {
        detail::write_global(detail::LV_ERROR,
                             detail::format_msg(fmt, std::forward<Args>(args)...));
    }
};
inline constexpr global_sink global;

// ---------------------------------------------------------------------------
// Sink: one-shot write to a named file scoped to this extension.
// File path is `<profile>\x4native\<extension_id>\<filename>` — the per-ext
// subfolder is created on first write. Append-open per call; no rotation.
// Created per call via x4n::log::to_file("name.log"); zero-cost temporary.
// ---------------------------------------------------------------------------
struct file_sink {
    const char* filename;

    void debug(std::string_view msg) const { detail::write_named(detail::LV_DEBUG, filename, msg); }
    void info (std::string_view msg) const { detail::write_named(detail::LV_INFO,  filename, msg); }
    void warn (std::string_view msg) const { detail::write_named(detail::LV_WARN,  filename, msg); }
    void error(std::string_view msg) const { detail::write_named(detail::LV_ERROR, filename, msg); }

    template<class... Args> requires (sizeof...(Args) > 0)
    void debug(std::format_string<Args...> fmt, Args&&... args) const {
        detail::write_named(detail::LV_DEBUG, filename,
                            detail::format_msg(fmt, std::forward<Args>(args)...));
    }
    template<class... Args> requires (sizeof...(Args) > 0)
    void info(std::format_string<Args...> fmt, Args&&... args) const {
        detail::write_named(detail::LV_INFO, filename,
                            detail::format_msg(fmt, std::forward<Args>(args)...));
    }
    template<class... Args> requires (sizeof...(Args) > 0)
    void warn(std::format_string<Args...> fmt, Args&&... args) const {
        detail::write_named(detail::LV_WARN, filename,
                            detail::format_msg(fmt, std::forward<Args>(args)...));
    }
    template<class... Args> requires (sizeof...(Args) > 0)
    void error(std::format_string<Args...> fmt, Args&&... args) const {
        detail::write_named(detail::LV_ERROR, filename,
                            detail::format_msg(fmt, std::forward<Args>(args)...));
    }
};
/// Build a temporary sink that writes to a named file under this extension's
/// subfolder in the user profile (`<profile>\x4native\<extension_id>\<name>`).
/// Usage:
///     x4n::log::to_file("events.log").info("event: {}", id);
inline file_sink to_file(const char* name) { return {name}; }

} // namespace log
} // namespace x4n
