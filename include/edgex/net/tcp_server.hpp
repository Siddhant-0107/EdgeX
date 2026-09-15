#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include "edgex/core/thread_pool.hpp"
#include "edgex/net/socket.hpp"

namespace edgex::net {

struct TCPServerConfig {
    std::string bind_address{"0.0.0.0"};
    std::uint16_t port{0};
    int backlog{128};
    std::size_t worker_count{1};
};

class TCPServer {
public:
    explicit TCPServer(TCPServerConfig config);

    ~TCPServer() = default;

    TCPServer(const TCPServer&) = delete;
    TCPServer& operator=(const TCPServer&) = delete;

    TCPServer(TCPServer&&) noexcept = delete;
    TCPServer& operator=(TCPServer&&) noexcept = delete;

    void start();
    void stop() noexcept;

    [[nodiscard]] Socket accept_client();

    /// Submits client work to the server's worker pool.
    ///
    /// The task must not outlive resources it references.
    /// Tasks already submitted before stop() begins are allowed
    /// to finish before the server is destroyed.
    void submit_client_task(std::function<void()> task);

    [[nodiscard]] bool is_running() const noexcept;

    [[nodiscard]] const std::string& bind_address() const noexcept;
    [[nodiscard]] std::uint16_t port() const noexcept;
    [[nodiscard]] int backlog() const noexcept;
    [[nodiscard]] std::size_t worker_count() const noexcept;

private:
    TCPServerConfig config_{};
    Socket listener_{};
    core::ThreadPool thread_pool_;
    bool running_{false};
};

}  // namespace edgex::net
