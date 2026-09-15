#pragma once

#include <cstddef>
#include <cstdint>
#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include "edgex/net/socket.hpp"

namespace edgex::net {

/// Configuration for an outbound IPv4 TCP connection.
struct TCPClientConfig {
    std::string address{"127.0.0.1"};
    std::uint16_t port{0};
    std::chrono::milliseconds connect_timeout{5000};
    std::chrono::milliseconds io_timeout{5000};
};

/// Owns one outbound TCP connection and provides reliable byte I/O.
class TCPClient {
public:
    explicit TCPClient(TCPClientConfig config);

    ~TCPClient() = default;

    TCPClient(const TCPClient&) = delete;
    TCPClient& operator=(const TCPClient&) = delete;

    TCPClient(TCPClient&&) noexcept = default;
    TCPClient& operator=(TCPClient&&) noexcept = default;

    /// Establishes the configured connection.
    /// @throws std::invalid_argument for an invalid address or timeout.
    /// @throws std::system_error when connection setup fails or times out.
    void connect();

    /// Sends all bytes in the supplied buffer.
    /// @throws std::logic_error when the client is not connected.
    /// @throws std::system_error when the send fails or times out.
    void send_all(std::string_view data);
    void send_all(const std::vector<std::uint8_t>& data);

    /// Receives up to max_bytes. An empty result means the peer closed cleanly.
    /// @throws std::logic_error when the client is not connected.
    /// @throws std::system_error when the receive fails or times out.
    [[nodiscard]] std::vector<std::uint8_t> receive_some(std::size_t max_bytes = 16 * 1024);

    void close() noexcept;

    [[nodiscard]] bool is_connected() const noexcept;
    [[nodiscard]] const TCPClientConfig& config() const noexcept;

private:
    TCPClientConfig config_{};
    Socket socket_{};
};

}  // namespace edgex::net
