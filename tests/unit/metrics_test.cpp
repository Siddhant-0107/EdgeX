#include "edgex/core/metrics.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace edgex;

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

http::HttpRequest make_request(http::HttpMethod method, std::string target) {
    http::HttpRequest request;
    request.method = method;
    request.target = std::move(target);
    request.version = http::HttpVersion::Http11;
    return request;
}

std::string body_as_string(const http::HttpResponse& response) {
    return std::string(response.body.begin(), response.body.end());
}

void test_counters_and_latency() {
    core::Metrics metrics;
    metrics.record_request(true, std::chrono::microseconds(120));
    metrics.record_request(false, std::chrono::microseconds(250));

    expect(metrics.request_count() == 2, "request count should be recorded");
    expect(metrics.error_count() == 1, "error count should be recorded");
    expect(metrics.total_latency_microseconds() == 370,
           "total latency should be accumulated");
    expect(metrics.max_latency_microseconds() == 250,
           "maximum latency should be recorded");

    std::cout << "PASS: counters and latency\n";
}

void test_active_connections() {
    core::Metrics metrics;
    metrics.connection_opened();
    metrics.connection_opened();
    expect(metrics.active_connections() == 2,
           "connection_opened should increment active connections");

    metrics.connection_closed();
    metrics.connection_closed();
    metrics.connection_closed();
    expect(metrics.active_connections() == 0,
           "connection_closed should never make count negative");

    std::cout << "PASS: active connections\n";
}

void test_thread_safe_updates() {
    core::Metrics metrics;
    constexpr int workers = 8;
    constexpr int requests_per_worker = 1000;
    std::vector<std::thread> threads;
    threads.reserve(workers);

    for (int i = 0; i < workers; ++i) {
        threads.emplace_back([&metrics] {
            for (int j = 0; j < requests_per_worker; ++j) {
                metrics.record_request(j % 10 != 0, std::chrono::microseconds(10));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    expect(metrics.request_count() == workers * requests_per_worker,
           "concurrent request updates should not be lost");
    expect(metrics.error_count() == workers * (requests_per_worker / 10),
           "concurrent error updates should not be lost");
    expect(metrics.total_latency_microseconds() == workers * requests_per_worker * 10,
           "concurrent latency updates should not be lost");

    std::cout << "PASS: thread-safe updates\n";
}

void test_custom_endpoint_and_route() {
    core::Metrics metrics(core::MetricsConfig{"/internal/metrics"});
    http::Router router;
    metrics.register_endpoint(router);
    metrics.record_request(true, std::chrono::microseconds(42));

    const auto response = router.handle(make_request(http::HttpMethod::Get, "/internal/metrics"));
    const auto body = body_as_string(response);

    expect(response.status == http::HttpStatus::Ok,
           "metrics endpoint should return 200");
    expect(response.has_header("Content-Type"),
           "metrics endpoint should set content type");
    expect(body.find("edgex_requests_total 1") != std::string::npos,
           "metrics endpoint should expose request count");
    expect(body.find("edgex_request_duration_microseconds_total 42") != std::string::npos,
           "metrics endpoint should expose latency");

    const auto missing = router.handle(make_request(http::HttpMethod::Get, "/metrics"));
    expect(missing.status == http::HttpStatus::NotFound,
           "custom endpoint should replace the default path");

    std::cout << "PASS: configurable metrics endpoint\n";
}

void test_backend_health_output() {
    std::vector<proxy::Backend> backends{
        {"127.0.0.1", 18080, true},
        {"127.0.0.1", 18081, false},
    };
    proxy::RoundRobinLoadBalancer load_balancer(std::move(backends));
    core::Metrics metrics;

    const auto response = metrics.render(&load_balancer);
    const auto body = body_as_string(response);

    expect(body.find("edgex_backend_healthy{index=\"0\"} 1") != std::string::npos,
           "healthy backend should be exposed as 1");
    expect(body.find("edgex_backend_healthy{index=\"1\"} 0") != std::string::npos,
           "unhealthy backend should be exposed as 0");

    std::cout << "PASS: backend health output\n";
}

void test_invalid_endpoint() {
    bool threw = false;
    try {
        core::Metrics metrics(core::MetricsConfig{"metrics"});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    expect(threw, "metrics endpoint without leading slash should be rejected");

    std::cout << "PASS: endpoint validation\n";
}

} // namespace

int main() {
    test_counters_and_latency();
    test_active_connections();
    test_thread_safe_updates();
    test_custom_endpoint_and_route();
    test_backend_health_output();
    test_invalid_endpoint();

    std::cout << "All 6 metrics tests passed\n";
    return EXIT_SUCCESS;
}
