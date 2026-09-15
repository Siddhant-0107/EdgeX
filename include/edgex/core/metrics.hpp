#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

#include "edgex/http/http_response.hpp"
#include "edgex/http/router.hpp"
#include "edgex/proxy/round_robin_load_balancer.hpp"

namespace edgex::core {

struct MetricsConfig {
    std::string endpoint{"/metrics"};
};

class Metrics {
public:
    explicit Metrics(MetricsConfig config = {});
    Metrics(const Metrics&) = delete;
    Metrics& operator=(const Metrics&) = delete;

    void record_request(bool success, std::chrono::microseconds duration) noexcept;
    void increment_requests() noexcept;
    void increment_errors() noexcept;
    void set_active_connections(std::int64_t count) noexcept;
    void connection_opened() noexcept;
    void connection_closed() noexcept;

    void register_endpoint(
        http::Router& router,
        const proxy::RoundRobinLoadBalancer* load_balancer = nullptr);
    [[nodiscard]] http::HttpResponse render(
        const proxy::RoundRobinLoadBalancer* load_balancer = nullptr) const;

    [[nodiscard]] const MetricsConfig& config() const noexcept;
    [[nodiscard]] std::uint64_t request_count() const noexcept;
    [[nodiscard]] std::uint64_t error_count() const noexcept;
    [[nodiscard]] std::uint64_t total_latency_microseconds() const noexcept;
    [[nodiscard]] std::uint64_t max_latency_microseconds() const noexcept;
    [[nodiscard]] std::int64_t active_connections() const noexcept;

private:
    MetricsConfig config_;
    std::atomic<std::uint64_t> request_count_{0};
    std::atomic<std::uint64_t> error_count_{0};
    std::atomic<std::uint64_t> total_latency_microseconds_{0};
    std::atomic<std::uint64_t> max_latency_microseconds_{0};
    std::atomic<std::int64_t> active_connections_{0};
};

} // namespace edgex::core
