#include "edgex/net/tcp_server.hpp"

#include <stdexcept>
#include <utility>

namespace edgex::net {

TCPServer::TCPServer(TCPServerConfig config) : config_(std::move(config)) {}

void TCPServer::start() {
    if (running_) {
        throw std::logic_error("TCPServer is already running");
    }

    if (config_.backlog <= 0) {
        throw std::invalid_argument("backlog must be greater than 0");
    }

    Socket listener = Socket::create_tcp_ipv4();
    listener.bind(config_.port, config_.bind_address);
    listener.listen(config_.backlog);

    listener_ = std::move(listener);
    running_ = true;
}

void TCPServer::stop() noexcept {
    listener_.close();
    running_ = false;
}

Socket TCPServer::accept_client() {
    if (!running_ || !listener_.is_open()) {
        throw std::logic_error("TCPServer is not running");
    }

    return listener_.accept();
}

bool TCPServer::is_running() const noexcept {
    return running_;
}

const std::string& TCPServer::bind_address() const noexcept {
    return config_.bind_address;
}

std::uint16_t TCPServer::port() const noexcept {
    return config_.port;
}

int TCPServer::backlog() const noexcept {
    return config_.backlog;
}

}  // namespace edgex::net
