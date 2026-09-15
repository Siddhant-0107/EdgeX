#include "edgex/core/logger.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

int main() {
    using edgex::core::LogLevel;
    using edgex::core::Logger;

    assert(edgex::core::to_string(LogLevel::trace) == std::string("TRACE"));
    assert(edgex::core::to_string(LogLevel::debug) == std::string("DEBUG"));
    assert(edgex::core::to_string(LogLevel::info) == std::string("INFO"));
    assert(edgex::core::to_string(LogLevel::warning) == std::string("WARNING"));
    assert(edgex::core::to_string(LogLevel::error) == std::string("ERROR"));
    assert(edgex::core::to_string(LogLevel::critical) == std::string("CRITICAL"));

    Logger logger(Logger::Config{LogLevel::warning, false, "logger_test.log"});
    assert(logger.level() == LogLevel::warning);

    logger.info("filtered");
    logger.warning("visible warning");
    logger.error("visible error");
    logger.set_level(LogLevel::error);
    logger.warning("filtered warning");
    logger.critical("visible critical");

    std::ifstream file("logger_test.log");
    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string output = buffer.str();

    assert(output.find("[WARNING] visible warning") != std::string::npos);
    assert(output.find("[ERROR] visible error") != std::string::npos);
    assert(output.find("[CRITICAL] visible critical") != std::string::npos);
    assert(output.find("filtered") == std::string::npos);
    assert(output.find("filtered warning") == std::string::npos);

    std::error_code error;
    std::filesystem::remove("logger_test.log", error);
    assert(!error);

    Logger thread_safe(Logger::Config{LogLevel::trace, false, "logger_thread_test.log"});
    constexpr int thread_count = 4;
    constexpr int messages_per_thread = 100;
    std::vector<std::thread> threads;
    threads.reserve(thread_count);

    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&thread_safe] {
            for (int j = 0; j < messages_per_thread; ++j) {
                thread_safe.debug("concurrent message");
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::ifstream thread_file("logger_thread_test.log");
    int line_count = 0;
    std::string line;
    while (std::getline(thread_file, line)) {
        ++line_count;
    }
    assert(line_count == thread_count * messages_per_thread);

    std::filesystem::remove("logger_thread_test.log", error);
    assert(!error);

    return 0;
}
