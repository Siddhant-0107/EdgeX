#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "edgex/http/http_request.hpp"
#include "edgex/http/http_response.hpp"

namespace edgex::proxy {

struct ReverseProxyConfig {
    std::string upstream_address{"127.0.0.1"};
    std::uint16_t upstream_port{0};
    std::chrono::milliseconds connect_timeout{5000};
    std::chrono::milliseconds io_timeout{5000};
};

class ReverseProxy {
public:
    explicit ReverseProxy(ReverseProxyConfig config);

    [[nodiscard]] http::HttpResponse forward(const http::HttpRequest& request) const;

    [[nodiscard]] const ReverseProxyConfig& config() const noexcept;

private:
    ReverseProxyConfig config_{};
};

}  // namespace edgex::proxy
