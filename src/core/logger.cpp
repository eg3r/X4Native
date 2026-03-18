#include "logger.h"

#include <chrono>
#include <format>

namespace x4n {

HANDLE     Logger::s_handle = INVALID_HANDLE_VALUE;
std::mutex Logger::s_mutex;

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
    s_handle = open_log(mod_root + "x4native.log");
    if (s_handle == INVALID_HANDLE_VALUE)
        OutputDebugStringA("X4Native: Failed to open log file\n");
}

void Logger::shutdown() {
    if (s_handle != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(s_handle);
        CloseHandle(s_handle);
        s_handle = INVALID_HANDLE_VALUE;
    }
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
    write_handle(s_mutex, s_handle, level, msg);
}

void Logger::write_to(HANDLE h, LogLevel level, std::string_view msg) {
    write_handle(s_mutex, h, level, msg);
}

} // namespace x4n
