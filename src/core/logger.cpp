#include "logger.h"
#include "game_api.h"
#include "safe_name.h"

#include <x4_game_func_table.h>

#include <chrono>
#include <filesystem>
#include <format>
#include <system_error>

namespace fs = std::filesystem;

namespace x4n {

HANDLE     Logger::s_handle = INVALID_HANDLE_VALUE;
std::mutex Logger::s_mutex;

std::vector<std::pair<LogLevel, std::string>> Logger::s_buffer;
bool        Logger::s_buffering = true;
std::string Logger::s_mod_root;
std::string Logger::s_profile_dir;

static constexpr const char* level_tag(LogLevel lv) {
    switch (lv) {
        case LogLevel::Debug: return "debug";
        case LogLevel::Info:  return "info";
        case LogLevel::Warn:  return "warn";
        case LogLevel::Error: return "error";
    }
    return "?";
}

// Rotation failures that aren't "the file doesn't exist" are surfaced via
// OutputDebugStringA. We can't use the Logger itself here: open_log runs
// during Logger bring-up, so recursion would be unsafe.
static void log_rotate_error(const char* op, const std::string& path) {
    DWORD err = GetLastError();
    if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) return;
    std::string msg = std::string("X4Native: log ") + op + " '" + path +
                      "' failed (GLE=" + std::to_string(err) + ")\n";
    OutputDebugStringA(msg.c_str());
}

HANDLE Logger::open_log(const std::string& log_path) {
    static constexpr int MAX_BACKUPS = 4;
    std::string base = log_path;
    if (base.size() >= 4 && base.compare(base.size() - 4, 4, ".log") == 0)
        base.resize(base.size() - 4);

    std::string oldest = base + ".4.log";
    if (!DeleteFileA(oldest.c_str())) log_rotate_error("delete", oldest);

    for (int i = MAX_BACKUPS - 1; i >= 1; --i) {
        std::string src = base + "." + std::to_string(i) + ".log";
        std::string dst = base + "." + std::to_string(i + 1) + ".log";
        if (!MoveFileA(src.c_str(), dst.c_str())) log_rotate_error("rotate", src);
    }
    std::string first_backup = base + ".1.log";
    if (!MoveFileA(log_path.c_str(), first_backup.c_str()))
        log_rotate_error("rotate", log_path);

    HANDLE h = CreateFileA(log_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                           nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) log_rotate_error("open", log_path);
    return h;
}

void Logger::init(const std::string& mod_root) {
    std::lock_guard lock(s_mutex);
    s_mod_root = mod_root;
    s_buffering = true;
    s_buffer.clear();
    s_handle = INVALID_HANDLE_VALUE;
}

std::string Logger::profile_log_dir() {
    if (!s_profile_dir.empty()) return s_profile_dir;

    auto* game = GameAPI::table();
    if (!game || !game->GetSaveFolderPath) return {};

    const char* raw = game->GetSaveFolderPath();
    if (!raw || !raw[0]) return {};

    // GetSaveFolderPath returns "<profile>\save\" — strip the save/ suffix
    // to get the profile root, then append our own subfolder.
    fs::path save_folder(raw);
    fs::path profile = save_folder.has_filename()
        ? save_folder.parent_path()
        : save_folder.parent_path().parent_path();

    fs::path dir = profile / "x4native";
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) return {};

    // Store with trailing separator so callers can concat a filename directly.
    s_profile_dir = dir.string();
    if (!s_profile_dir.empty() && s_profile_dir.back() != '\\' && s_profile_dir.back() != '/')
        s_profile_dir += '\\';
    return s_profile_dir;
}

std::string Logger::profile_ext_dir(const std::string& extension_id) {
    if (extension_id.empty()) return {};
    auto base = profile_log_dir();
    if (base.empty()) return {};

    fs::path dir = fs::path(base) / extension_id;
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) return {};

    std::string out = dir.string();
    if (!out.empty() && out.back() != '\\' && out.back() != '/')
        out += '\\';
    return out;
}

bool Logger::is_safe_relative_name(const std::string& name, const char* ctx) {
    const char* reason = unsafe_relative_name_reason(name);
    if (!reason) return true;
    Logger::warn("{}: {} rejected: '{}'", ctx ? ctx : "path", reason, name);
    return false;
}

void Logger::open_files() {
    std::string dir = profile_log_dir();
    if (dir.empty()) dir = s_mod_root;  // fallback: write next to the extension

    HANDLE h = open_log(dir + "x4native.log");

    std::vector<std::pair<LogLevel, std::string>> pending;
    {
        std::lock_guard lock(s_mutex);
        s_handle = h;
        s_buffering = false;
        pending.swap(s_buffer);
    }

    if (h == INVALID_HANDLE_VALUE) {
        OutputDebugStringA("X4Native: Failed to open framework log file\n");
        return;
    }

    // Flush buffered entries to the freshly-opened file.
    for (auto& [lv, msg] : pending)
        write(lv, msg);
}

void Logger::shutdown() {
    std::lock_guard lock(s_mutex);
    if (s_handle != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(s_handle);
        CloseHandle(s_handle);
        s_handle = INVALID_HANDLE_VALUE;
    }
    s_buffer.clear();
    s_buffering = false;
    s_profile_dir.clear();
    s_mod_root.clear();
}

// Write `line` to `h` under `mtx`, then (outside the lock) optionally flush.
// Flushing outside the lock keeps high-volume log callers from serialising
// on disk sync — FlushFileBuffers is thread-safe per MSDN.
static void write_handle(std::mutex& mtx, HANDLE h, LogLevel level, std::string_view line) {
    bool should_flush = false;
    if (h != INVALID_HANDLE_VALUE) {
        std::lock_guard lock(mtx);
        DWORD written;
        WriteFile(h, line.data(), static_cast<DWORD>(line.size()), &written, nullptr);
        should_flush = (level >= LogLevel::Info);
    }
    if (should_flush) FlushFileBuffers(h);
}

void Logger::write(LogLevel level, std::string_view msg) {
    auto now = std::chrono::system_clock::now();
    auto line = std::format("[{:%Y-%m-%d %H:%M:%S}] [{}] {}\n", now, level_tag(level), msg);

    // Decide buffer vs file in a single critical section so we never race
    // with open_files() swapping s_buffering/s_handle.
    HANDLE h = INVALID_HANDLE_VALUE;
    {
        std::lock_guard lock(s_mutex);
        if (s_buffering) {
            s_buffer.emplace_back(level, std::string(msg));
            OutputDebugStringA(line.c_str());
            return;
        }
        h = s_handle;
    }

    write_handle(s_mutex, h, level, line);
    OutputDebugStringA(line.c_str());
}

void Logger::write_to(HANDLE h, LogLevel level, std::string_view msg) {
    auto now = std::chrono::system_clock::now();
    auto line = std::format("[{:%Y-%m-%d %H:%M:%S}] [{}] {}\n", now, level_tag(level), msg);
    write_handle(s_mutex, h, level, line);
    OutputDebugStringA(line.c_str());
}

} // namespace x4n
