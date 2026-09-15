#include "edgex/proxy/health_checker.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "edgex/net/tcp_client.hpp"

namespace edgex::proxy {

namespace {

bool is_successful_http_response(const std::vector<std::uint8_t>& data) noexcept {
    const std::string_view response(reinterpret_cast<const char*>(data.data()), data.size());
    const std::size_t line_end = response.find("\r\n");
    if (line_end == std::string_view::npos) return false;

    const std::string_view status_line = response.substr(0, line_end);
    if (status_line.size() < 12 || status_line.substr(0, 9) != "HTTP/1.1 ") return false;

    const char hundreds = status_line[9];
    const char tens = status_line[10];
    const char ones = status_line[11];
    return hundreds == '2' && tens >= '0' && tens <= '9' && ones >= '0' && ones <= '9';
}

}  // namespace

HealthChecker::HealthChecker(RoundRobinLoadBalancer& load_balancer,
                             HealthCheckerConfig config,
                             core::Logger& logger)
    : load_balancer_(load_balancer), config_(std::move(config)), logger_(logger) {
    if (config_.interval.count() <= 0) {
        throw std::invalid_argument("health-check interval must be greater than 0");
    }
    if (config_.timeout.count() <= 0) {
        throw std::invalid_argument("health-check timeout must be greater than 0");
    }
    if (config_.endpoint.empty() || config_.endpoint.front() != '/') {
        throw std::invalid_argument("health-check endpoint must start with '/'");
    }
}

HealthChecker::~HealthChecker() { stop(); }

void HealthChecker::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) return;

    stop_requested_ = false;
    worker_ = std::thread(&HealthChecker::run, this);
    running_ = true;
}

void HealthChecker::stop() noexcept {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) return;
        stop_requested_ = true;
    }
    condition_.notify_all();

    if (worker_.joinable()) worker_.join();

    std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
}

void HealthChecker::check_once() {
    const std::vector<Backend> backends = load_balancer_.backends();

    for (std::size_t index = 0; index < backends.size(); ++index) {
        const bool healthy = check_backend(backends[index]);
        const bool previous = backends[index].healthy;

        load_balancer_.set_healthy(index, healthy);

        if (healthy != previous) {
            logger_.info("health_checker transition backend=" + backends[index].address +
                         ":" + std::to_string(backends[index].port) +
                         " state=" + (healthy ? "healthy" : "unhealthy"));
        }
    }
}

bool HealthChecker::is_running() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

const HealthCheckerConfig& HealthChecker::config() const noexcept { return config_; }

void HealthChecker::run() {
    for (;;) {
        try {
            check_once();
        } catch (const std::exception& error) {
            logger_.error(std::string("health_checker pass failed error=") + error.what());
        } catch (...) {
            logger_.error("health_checker pass failed error=unknown");
        }

        std::unique_lock<std::mutex> lock(mutex_);
        const bool stop = condition_.wait_for(lock, config_.interval, [this] {
            return stop_requested_;
        });
        if (stop) break;
    }
}

bool HealthChecker::check_backend(const Backend& backend) const noexcept {
    try {
        net::TCPClient client(net::TCPClientConfig{
            backend.address, backend.port, config_.timeout, config_.timeout});
        client.connect();

        const std::string request = "GET " + config_.endpoint +
                                    " HTTP/1.1\r\nHost: " + backend.address +
                                    ":" + std::to_string(backend.port) +
                                    "\r\nConnection: close\r\n\r\n";
        client.send_all(request);

        std::vector<std::uint8_t> response;
        for (;;) {
            const auto chunk = client.receive_some();
            if (chunk.empty()) break;
            response.insert(response.end(), chunk.begin(), chunk.end());
            if (response.size() >= 16 * 1024) break;
        }
        client.close();
        return is_successful_http_response(response);
    } catch (...) {
        return false;
    }
}

}  // namespace edgex::proxy
