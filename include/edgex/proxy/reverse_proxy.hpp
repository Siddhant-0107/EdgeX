#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "edgex/core/logger.hpp"
#include "edgex/http/http_request.hpp"
#include "edgex/http/http_response.hpp"
#include "edgex/proxy/round_robin_load_balancer.hpp"

namespace edgex::proxy {

struct ReverseProxyConfig {
    // Legacy single-upstream configuration. Kept for API compatibility.
    std::string upstream_address{"127.0.0.1"};
    std::uint16_t upstream_port{0};

    // When non-empty, these backends are used by the round-robin load balancer.
    // The legacy upstream fields are used when this list is empty.
    std::vector<Backend> upstreams{};

    std::chrono::milliseconds connect_timeout{5000};
    std::chrono::milliseconds io_timeout{5000};
};

class ReverseProxy {
public:
    explicit ReverseProxy(ReverseProxyConfig config);
    ReverseProxy(ReverseProxyConfig config, core::Logger& logger);

    [[nodiscard]] http::HttpResponse forward(const http::HttpRequest& request) const;

    [[nodiscard]] const ReverseProxyConfig& config() const noexcept;

private:
    ReverseProxyConfig config_{};
    std::unique_ptr<RoundRobinLoadBalancer> load_balancer_;
    core::Logger* logger_{nullptr};
};

}  // namespace edgex::proxy
