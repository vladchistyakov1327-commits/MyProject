#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <format>
#include <source_location>

namespace utils {

enum class LogLevel { Debug, Info, Warning, Error };

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void set_level(LogLevel level) { min_level_ = level; }
    void set_file(const std::wstring& path);

    void log(LogLevel level, std::string_view msg,
             const std::source_location& loc = std::source_location::current());

    void debug(std::string_view msg,
               const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Debug, msg, loc);
    }
    void info(std::string_view msg,
              const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Info, msg, loc);
    }
    void warn(std::string_view msg,
              const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Warning, msg, loc);
    }
    void error(std::string_view msg,
               const std::source_location& loc = std::source_location::current()) {
        log(LogLevel::Error, msg, loc);
    }

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::mutex        mutex_;
    std::ofstream     file_;
    LogLevel          min_level_{ LogLevel::Debug };
    bool              console_enabled_{ true };
};

// Convenience macros
#define LOG_DEBUG(msg) utils::Logger::instance().debug(msg)
#define LOG_INFO(msg)  utils::Logger::instance().info(msg)
#define LOG_WARN(msg)  utils::Logger::instance().warn(msg)
#define LOG_ERROR(msg) utils::Logger::instance().error(msg)

} // namespace utils
