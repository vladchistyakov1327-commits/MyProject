#include "logger.h"
#include <windows.h>
#include <iostream>

namespace utils {

void Logger::set_file(const std::wstring& path) {
    std::lock_guard lock(mutex_);
    file_.open(path, std::ios::app);
}

void Logger::log(LogLevel level, std::string_view msg, const std::source_location& loc) {
    if (level < min_level_) return;

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &time_t);

    const char* level_str = [level]() -> const char* {
        switch (level) {
            case LogLevel::Debug:   return "DBG";
            case LogLevel::Info:    return "INF";
            case LogLevel::Warning: return "WRN";
            case LogLevel::Error:   return "ERR";
            default:                return "???";
        }
    }();

    char buf[64];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             tm.tm_hour, tm.tm_min, tm.tm_sec);

    // Extract just the filename from the path
    std::string_view file_path = loc.file_name();
    auto slash = file_path.rfind('\\');
    if (slash == std::string_view::npos) slash = file_path.rfind('/');
    std::string_view filename = (slash != std::string_view::npos)
                                ? file_path.substr(slash + 1)
                                : file_path;

    std::string line = std::format("[{}][{}] {}:{} - {}\n",
        buf, level_str, filename, loc.line(), msg);

    std::lock_guard lock(mutex_);
    if (console_enabled_) {
        OutputDebugStringA(line.c_str());
        if (level >= LogLevel::Error) {
            std::cerr << line;
        }
    }
    if (file_.is_open()) {
        file_ << line;
        file_.flush();
    }
}

} // namespace utils
