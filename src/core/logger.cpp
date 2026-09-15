#include "edgex/core/logger.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace edgex::core {

namespace {

const char* level_name(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::trace:
        return "TRACE";
    case LogLevel::debug:
        return "DEBUG";
    case LogLevel::info:
        return "INFO";
    case LogLevel::warning:
        return "WARNING";
    case LogLevel::error:
        return "ERROR";
    case LogLevel::critical:
        return "CRITICAL";
    }

    return "UNKNOWN";
}

} // namespace

class Logger::Impl {
public:
    explicit Impl(Config config)
        : config_(std::move(config)) {
        if (!config_.console && config_.file_path.empty()) {
            throw std::invalid_argument("logger requires a destination");
        }

        if (!config_.file_path.empty()) {
            file_.open(config_.file_path, std::ios::app);
            if (!file_) {
                throw std::runtime_error("failed to open log file: " + config_.file_path);
            }
        }
    }

    void set_level(LogLevel level) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.level = level;
    }

    [[nodiscard]] LogLevel level() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_.level;
    }

    void log(LogLevel level, std::string_view message) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (static_cast<int>(level) < static_cast<int>(config_.level)) {
            return;
        }

        const auto now = std::chrono::system_clock::now();
        const std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::tm local_time{};
#if defined(_WIN32)
        localtime_s(&local_time, &time);
#else
        localtime_r(&time, &local_time);
#endif

        std::ostringstream line;
        line << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S")
             << " [" << level_name(level) << "] " << message << '\n';

        const std::string text = line.str();
        if (config_.console) {
            std::clog << text;
            std::clog.flush();
        }
        if (file_) {
            file_ << text;
            file_.flush();
        }
    }

private:
    Config config_;
    std::ofstream file_;
    mutable std::mutex mutex_;
};

Logger::Logger()
    : Logger(Config{}) {}

Logger::Logger(Config config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

Logger::~Logger() = default;
Logger::Logger(Logger&&) noexcept = default;
Logger& Logger::operator=(Logger&&) noexcept = default;

void Logger::set_level(LogLevel level) noexcept {
    impl_->set_level(level);
}

LogLevel Logger::level() const noexcept {
    return impl_->level();
}

void Logger::log(LogLevel level, std::string_view message) {
    impl_->log(level, message);
}

void Logger::trace(std::string_view message) { log(LogLevel::trace, message); }
void Logger::debug(std::string_view message) { log(LogLevel::debug, message); }
void Logger::info(std::string_view message) { log(LogLevel::info, message); }
void Logger::warning(std::string_view message) { log(LogLevel::warning, message); }
void Logger::error(std::string_view message) { log(LogLevel::error, message); }
void Logger::critical(std::string_view message) { log(LogLevel::critical, message); }

const char* to_string(LogLevel level) noexcept {
    return level_name(level);
}

} // namespace edgex::core
