#pragma once

#include <cstdint>
#include <string>

#include "edgex/net/socket.hpp"

namespace edgex::net {

struct TCPServerConfig {
    std::string bind_address{"0.0.0.0"};
    std::uint16_t port{0};
    int backlog{128};
};

class TCPServer {
public:
    explicit TCPServer(TCPServerConfig config);

    ~TCPServer() = default;

    TCPServer(const TCPServer&) = delete;
    TCPServer& operator=(const TCPServer&) = delete;

    TCPServer(TCPServer&&) noexcept = default;
    TCPServer& operator=(TCPServer&&) noexcept = default;

    void start();
    void stop() noexcept;

    [[nodiscard]] Socket accept_client();

    [[nodiscard]] bool is_running() const noexcept;

    [[nodiscard]] const std::string& bind_address() const noexcept;
    [[nodiscard]] std::uint16_t port() const noexcept;
    [[nodiscard]] int backlog() const noexcept;

private:
    TCPServerConfig config_{};
    Socket listener_{};
    bool running_{false};
};

}  // namespace edgex::net
