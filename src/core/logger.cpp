#include "logger.h"
#include "game_api.h"

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

HANDLE Logger::open_log(const std::string& log_path) {
    static constexpr int MAX_BACKUPS = 4;
    std::string base = log_path;
    if (base.size() >= 4 && base.compare(base.size() - 4, 4, ".log") == 0)
        base.resize(base.size() - 4);

    DeleteFileA((base + ".4.log").c_str());
    for (int i = MAX_BACKUPS - 1; i >= 1; --i) {
        MoveFileA((base + "." + std::to_string(i) + ".log").c_str(),
                  (base + "." + std::to_string(i + 1) + ".log").c_str());
    }
    MoveFileA(log_path.c_str(), (base + ".1.log").c_str());

    return CreateFileA(log_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                       nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
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
    if (name.empty()) {
        Logger::warn("{}: empty filename rejected", ctx ? ctx : "path");
        return false;
    }
    fs::path p(name);
    if (p.is_absolute()) {
        Logger::warn("{}: absolute path rejected: '{}'",
                     ctx ? ctx : "path", name);
        return false;
    }
    for (const auto& part : p) {
        if (part == "..") {
            Logger::warn("{}: '..' in path rejected: '{}'",
                         ctx ? ctx : "path", name);
            return false;
        }
    }
    return true;
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

static void write_handle(std::mutex& mtx, HANDLE h, LogLevel level, std::string_view msg) {
    auto now = std::chrono::system_clock::now();
    auto line = std::format("[{:%Y-%m-%d %H:%M:%S}] [{}] {}\n", now, level_tag(level), msg);

    {
        std::lock_guard lock(mtx);
        if (h != INVALID_HANDLE_VALUE) {
            DWORD written;
            WriteFile(h, line.data(), static_cast<DWORD>(line.size()), &written, nullptr);
            if (level >= LogLevel::Info)
                FlushFileBuffers(h);
        }
    }

    OutputDebugStringA(line.c_str());
}

void Logger::write(LogLevel level, std::string_view msg) {
    // While buffering, hold lines in memory so they can be replayed once a
    // file handle exists. Still dump to OutputDebugString immediately so
    // developers see them live in a debugger.
    if (s_buffering) {
        {
            std::lock_guard lock(s_mutex);
            if (s_buffering) {
                s_buffer.emplace_back(level, std::string(msg));
                auto now = std::chrono::system_clock::now();
                auto line = std::format("[{:%Y-%m-%d %H:%M:%S}] [{}] {}\n",
                                        now, level_tag(level), msg);
                OutputDebugStringA(line.c_str());
                return;
            }
        }
        // Raced with open_files(); fall through to normal write path.
    }
    write_handle(s_mutex, s_handle, level, msg);
}

void Logger::write_to(HANDLE h, LogLevel level, std::string_view msg) {
    write_handle(s_mutex, h, level, msg);
}

} // namespace x4n
