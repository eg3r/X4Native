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
#include <utility>
#include <vector>

namespace x4n {

enum class LogLevel { Debug, Info, Warn, Error };

class Logger {
public:
    // Two-phase init: start in memory-buffering mode so early messages
    // (before GameAPI is up and we can resolve the profile path) are not lost.
    // `mod_root` is the extension folder — kept as a fallback path in case
    // the profile can't be resolved later.
    static void init(const std::string& mod_root);

    // Resolve <profile>\x4native\ via GameAPI's GetSaveFolderPath, open
    // x4native.log there, and flush any messages buffered during init().
    // Must be called after GameAPI::init(). Falls back to `mod_root` if the
    // profile path can't be resolved.
    static void open_files();

    // Return "<profile>\x4native\" (with trailing separator), or empty if
    // the profile path isn't resolvable yet. Creates the directory on first
    // successful call. Cached.
    static std::string profile_log_dir();

    // Return "<profile>\x4native\<extension_id>\" (with trailing separator),
    // or empty on failure. Creates the subdirectory if needed.
    static std::string profile_ext_dir(const std::string& extension_id);

    // Validate a relative filename intended to live under a per-extension
    // log directory. Returns true iff `name` is non-empty, not absolute, and
    // contains no `..` segments. On rejection, logs a warning via `ctx` for
    // diagnostics and returns false.
    static bool is_safe_relative_name(const std::string& name, const char* ctx);

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

    // Buffer active between init() and open_files(): early lines land here
    // and get flushed to the file once opened. Discarded on open failure.
    static std::vector<std::pair<LogLevel, std::string>> s_buffer;
    static bool        s_buffering;
    static std::string s_mod_root;          // extension folder — fallback path
    static std::string s_profile_dir;       // "<profile>\x4native\" once resolved
};

} // namespace x4n
