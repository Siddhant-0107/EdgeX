#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>

#include "edgex/core/logger.hpp"
#include "edgex/proxy/round_robin_load_balancer.hpp"

namespace edgex::proxy {

/// Configuration for periodic HTTP health checks against backend instances.
struct HealthCheckerConfig {
    std::chrono::milliseconds interval{5000};
    std::chrono::milliseconds timeout{1000};
    std::string endpoint{"/health"};
};

/// Periodically probes load-balancer backends and updates their health state.
class HealthChecker {
public:
    HealthChecker(RoundRobinLoadBalancer& load_balancer,
                  HealthCheckerConfig config,
                  core::Logger& logger);
    ~HealthChecker();

    HealthChecker(const HealthChecker&) = delete;
    HealthChecker& operator=(const HealthChecker&) = delete;
    HealthChecker(HealthChecker&&) = delete;
    HealthChecker& operator=(HealthChecker&&) = delete;

    /// Starts periodic checks. Calling start() while already running is a no-op.
    void start();

    /// Stops periodic checks and waits for the worker to finish.
    void stop() noexcept;

    /// Runs one complete health-check pass synchronously.
    void check_once();

    [[nodiscard]] bool is_running() const noexcept;
    [[nodiscard]] const HealthCheckerConfig& config() const noexcept;

private:
    void run();
    [[nodiscard]] bool check_backend(const Backend& backend) const noexcept;

    RoundRobinLoadBalancer& load_balancer_;
    HealthCheckerConfig config_{};
    core::Logger& logger_;

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::thread worker_;
    bool running_{false};
    bool stop_requested_{false};
};

}  // namespace edgex::proxy
