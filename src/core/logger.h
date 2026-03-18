#pragma once
// ---------------------------------------------------------------------------
// x4native_core.dll — Logger
//
// Lightweight logger with two sinks:
//   1. File sink  → <mod_root>/x4native.log   (rotated each init: keeps x4native.1-4.log)
//   2. MSVC sink  → OutputDebugString          (visible in debugger / DebugView)
//
// Uses C++23 std::format — no external dependencies.
// ---------------------------------------------------------------------------

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <format>
#include <string>
#include <string_view>
#include <mutex>

namespace x4n {

enum class LogLevel { Debug, Info, Warn, Error };

class Logger {
public:
    static void init(const std::string& mod_root);
    static void shutdown();

    // Open (and rotate) a log file at an absolute path.
    // Strips the .log suffix to form the backup base name, shifts .1-.4 backups,
    // then creates a fresh file. Returns INVALID_HANDLE_VALUE on failure.
    // Used by both the framework log and per-extension logs.
    static HANDLE open_log(const std::string& log_path);

    static void write(LogLevel level, std::string_view msg);

    // Write to an arbitrary HANDLE — used by per-extension log routing.
    static void write_to(HANDLE h, LogLevel level, std::string_view msg);

    template<typename... Args>
    static void debug([[maybe_unused]] std::format_string<Args...> fmt, [[maybe_unused]] Args&&... args) {
#ifndef NDEBUG
        if (s_handle) write(LogLevel::Debug, std::format(fmt, std::forward<Args>(args)...));
#endif
    }

    template<typename... Args>
    static void info(std::format_string<Args...> fmt, Args&&... args) {
        if (s_handle) write(LogLevel::Info, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void warn(std::format_string<Args...> fmt, Args&&... args) {
        if (s_handle) write(LogLevel::Warn, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void error(std::format_string<Args...> fmt, Args&&... args) {
        if (s_handle) write(LogLevel::Error, std::format(fmt, std::forward<Args>(args)...));
    }

private:
    static HANDLE     s_handle;
    static std::mutex s_mutex;
};

} // namespace x4n
