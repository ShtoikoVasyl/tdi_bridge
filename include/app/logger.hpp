#pragma once

#include <mutex>
#include <string>

namespace app {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error
};

class Logger {
  public:
    static Logger& instance();

    void set_level(LogLevel level);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

  private:
    void log(LogLevel level, const std::string& message);
    const char* level_name(LogLevel level) const;

    LogLevel level_ = LogLevel::Info;
    std::mutex mutex_;
};

LogLevel parse_log_level(const std::string& value);

}  // namespace app
