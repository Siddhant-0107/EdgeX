#include "edgex/core/metrics.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

#include "edgex/http/http_response_builder.hpp"

namespace edgex::core {

namespace {

std::string metric_line(std::string_view name, std::uint64_t value) {
    return std::string{name} + " " + std::to_string(value) + "\n";
}

} // namespace

Metrics::Metrics(MetricsConfig config) : config_(std::move(config)) {
    if (config_.endpoint.empty() || config_.endpoint.front() != '/') {
        throw std::invalid_argument("metrics endpoint must start with '/'");
    }
}

void Metrics::record_request(bool success, std::chrono::microseconds duration) noexcept {
    increment_requests();
    if (!success) {
        increment_errors();
    }

    const auto micros = static_cast<std::uint64_t>(std::max(duration.count(), 0LL));
    total_latency_microseconds_.fetch_add(micros, std::memory_order_relaxed);

    auto current = max_latency_microseconds_.load(std::memory_order_relaxed);
    while (current < micros &&
           !max_latency_microseconds_.compare_exchange_weak(
               current, micros, std::memory_order_relaxed, std::memory_order_relaxed)) {
    }
}

void Metrics::increment_requests() noexcept {
    request_count_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::increment_errors() noexcept {
    error_count_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::set_active_connections(std::int64_t count) noexcept {
    active_connections_.store(std::max<std::int64_t>(count, 0), std::memory_order_relaxed);
}

void Metrics::connection_opened() noexcept {
    active_connections_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::connection_closed() noexcept {
    auto current = active_connections_.load(std::memory_order_relaxed);
    while (current > 0 &&
           !active_connections_.compare_exchange_weak(
               current, current - 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
    }
}

void Metrics::register_endpoint(
    http::Router& router,
    const proxy::RoundRobinLoadBalancer* load_balancer) {
    router.get(config_.endpoint, [this, load_balancer](const http::HttpRequest&) {
        return render(load_balancer);
    });
}

http::HttpResponse Metrics::render(const proxy::RoundRobinLoadBalancer* load_balancer) const {
    std::string body;
    body.reserve(512);
    body += metric_line("edgex_requests_total", request_count());
    body += metric_line("edgex_errors_total", error_count());
    body += metric_line("edgex_request_duration_microseconds_total", total_latency_microseconds());
    body += metric_line("edgex_request_duration_microseconds_max", max_latency_microseconds());
    body += metric_line("edgex_active_connections", static_cast<std::uint64_t>(active_connections()));

    if (load_balancer != nullptr) {
        const auto backends = load_balancer->backends();
        for (std::size_t index = 0; index < backends.size(); ++index) {
            body += "edgex_backend_healthy{index=\"" + std::to_string(index) + "\"} " +
                    (backends[index].healthy ? "1\n" : "0\n");
        }
    }

    http::HttpResponseBuilder builder;
    builder.status(http::HttpStatus::Ok)
        .header("Content-Type", "text/plain; version=0.0.4")
        .body(body);
    return builder.build();
}

const MetricsConfig& Metrics::config() const noexcept {
    return config_;
}

std::uint64_t Metrics::request_count() const noexcept {
    return request_count_.load(std::memory_order_relaxed);
}

std::uint64_t Metrics::error_count() const noexcept {
    return error_count_.load(std::memory_order_relaxed);
}

std::uint64_t Metrics::total_latency_microseconds() const noexcept {
    return total_latency_microseconds_.load(std::memory_order_relaxed);
}

std::uint64_t Metrics::max_latency_microseconds() const noexcept {
    return max_latency_microseconds_.load(std::memory_order_relaxed);
}

std::int64_t Metrics::active_connections() const noexcept {
    return active_connections_.load(std::memory_order_relaxed);
}

} // namespace edgex::core
