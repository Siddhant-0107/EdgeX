#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace edgex::core {

enum class LogLevel {
    trace,
    debug,
    info,
    warning,
    error,
    critical,
};

class Logger {
public:
    struct Config {
        LogLevel level{LogLevel::info};
        bool console{true};
        std::string file_path{};
    };

    explicit Logger(Config config = {});
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) noexcept;
    Logger& operator=(Logger&&) noexcept;

    void set_level(LogLevel level) noexcept;
    [[nodiscard]] LogLevel level() const noexcept;

    void log(LogLevel level, std::string_view message);

    void trace(std::string_view message);
    void debug(std::string_view message);
    void info(std::string_view message);
    void warning(std::string_view message);
    void error(std::string_view message);
    void critical(std::string_view message);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] const char* to_string(LogLevel level) noexcept;

} // namespace edgex::core
