#include "edgex/net/tcp_client.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#include <ws2tcpip.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace edgex::net {
namespace {

#if defined(_WIN32)

void set_nonblocking(native_socket_t socket, bool enabled) {
    u_long mode = enabled ? 1UL : 0UL;
    if (::ioctlsocket(socket, FIONBIO, &mode) != 0) {
        throw std::system_error(::WSAGetLastError(), std::system_category(),
                                "ioctlsocket");
    }
}

void set_io_timeout(native_socket_t socket, std::chrono::milliseconds timeout) {
    const auto milliseconds = std::max<std::int64_t>(1, timeout.count());
    const DWORD value = static_cast<DWORD>(std::min<std::int64_t>(
        milliseconds, std::numeric_limits<DWORD>::max()));

    if (::setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,
                     reinterpret_cast<const char*>(&value), sizeof(value)) != 0 ||
        ::setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO,
                     reinterpret_cast<const char*>(&value), sizeof(value)) != 0) {
        throw std::system_error(::WSAGetLastError(), std::system_category(),
                                "setsockopt timeout");
    }
}

bool wait_for_connect(native_socket_t socket, std::chrono::milliseconds timeout) {
    fd_set writable;
    FD_ZERO(&writable);
    FD_SET(socket, &writable);

    const auto seconds = timeout.count() / 1000;
    const auto micros = (timeout.count() % 1000) * 1000;
    timeval tv{static_cast<long>(seconds), static_cast<long>(micros)};

    const int result = ::select(0, nullptr, &writable, nullptr, &tv);
    if (result == 0) {
        throw std::system_error(WSAETIMEDOUT, std::system_category(),
                                "connect timeout");
    }
    if (result == SOCKET_ERROR) {
        throw std::system_error(::WSAGetLastError(), std::system_category(),
                                "select");
    }

    int error = 0;
    int length = sizeof(error);
    if (::getsockopt(socket, SOL_SOCKET, SO_ERROR,
                     reinterpret_cast<char*>(&error), &length) != 0) {
        throw std::system_error(::WSAGetLastError(), std::system_category(),
                                "getsockopt SO_ERROR");
    }

    if (error != 0) {
        throw std::system_error(error, std::system_category(), "connect");
    }

    return true;
}

#else

void set_nonblocking(native_socket_t socket, bool enabled) {
    const int flags = ::fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        throw std::system_error(errno, std::generic_category(), "fcntl F_GETFL");
    }

    const int updated = enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    if (::fcntl(socket, F_SETFL, updated) == -1) {
        throw std::system_error(errno, std::generic_category(), "fcntl F_SETFL");
    }
}

void set_io_timeout(native_socket_t socket, std::chrono::milliseconds timeout) {
    const auto milliseconds = std::max<std::int64_t>(1, timeout.count());
    timeval tv{
        static_cast<long>(milliseconds / 1000),
        static_cast<long>((milliseconds % 1000) * 1000),
    };

    if (::setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0 ||
        ::setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) {
        throw std::system_error(errno, std::generic_category(),
                                "setsockopt timeout");
    }
}

bool wait_for_connect(native_socket_t socket, std::chrono::milliseconds timeout) {
    fd_set writable;
    FD_ZERO(&writable);
    FD_SET(socket, &writable);

    const auto seconds = timeout.count() / 1000;
    const auto micros = (timeout.count() % 1000) * 1000;
    timeval tv{static_cast<long>(seconds), static_cast<long>(micros)};

    const int result = ::select(socket + 1, nullptr, &writable, nullptr, &tv);
    if (result == 0) {
        throw std::system_error(ETIMEDOUT, std::generic_category(),
                                "connect timeout");
    }
    if (result == -1) {
        if (errno == EINTR) {
            return wait_for_connect(socket, timeout);
        }
        throw std::system_error(errno, std::generic_category(), "select");
    }

    int error = 0;
    socklen_t length = sizeof(error);
    if (::getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &length) != 0) {
        throw std::system_error(errno, std::generic_category(),
                                "getsockopt SO_ERROR");
    }

    if (error != 0) {
        throw std::system_error(error, std::generic_category(), "connect");
    }

    return true;
}

#endif

void validate_config(const TCPClientConfig& config) {
    if (config.address.empty()) {
        throw std::invalid_argument("address must not be empty");
    }
    if (config.port == 0) {
        throw std::invalid_argument("port must be greater than 0");
    }
    if (config.connect_timeout.count() <= 0) {
        throw std::invalid_argument("connect_timeout must be greater than 0");
    }
    if (config.io_timeout.count() <= 0) {
        throw std::invalid_argument("io_timeout must be greater than 0");
    }
}

}  // namespace

TCPClient::TCPClient(TCPClientConfig config)
    : config_(std::move(config)) {
    validate_config(config_);
}

void TCPClient::connect() {
    if (socket_.is_open()) {
        throw std::logic_error("TCPClient is already connected");
    }

    Socket socket = Socket::create_tcp_ipv4();

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(config_.port);

    const int conversion = ::inet_pton(
        AF_INET, config_.address.c_str(), &address.sin_addr);
    if (conversion != 1) {
        if (conversion == 0) {
            throw std::invalid_argument("address is not a valid IPv4 address");
        }
#if defined(_WIN32)
        throw std::system_error(::WSAGetLastError(), std::system_category(),
                                "inet_pton");
#else
        throw std::system_error(errno, std::generic_category(), "inet_pton");
#endif
    }

    set_nonblocking(socket.native_handle(), true);

    const int result = ::connect(
        socket.native_handle(),
        reinterpret_cast<const sockaddr*>(&address),
        static_cast<int>(sizeof(address)));

#if defined(_WIN32)
    if (result == SOCKET_ERROR) {
        const int error = ::WSAGetLastError();
        if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS &&
            error != WSAEALREADY) {
            throw std::system_error(error, std::system_category(), "connect");
        }
        (void)wait_for_connect(socket.native_handle(), config_.connect_timeout);
    }
#else
    if (result == -1) {
        if (errno != EINPROGRESS) {
            throw std::system_error(errno, std::generic_category(), "connect");
        }
        (void)wait_for_connect(socket.native_handle(), config_.connect_timeout);
    }
#endif

    set_nonblocking(socket.native_handle(), false);
    set_io_timeout(socket.native_handle(), config_.io_timeout);
    socket_ = std::move(socket);
}

void TCPClient::send_all(std::string_view data) {
    if (!socket_.is_open()) {
        throw std::logic_error("TCPClient is not connected");
    }

    std::size_t sent = 0;
    while (sent < data.size()) {
        const std::size_t remaining = data.size() - sent;
        const std::size_t chunk_size = std::min<std::size_t>(
            remaining, static_cast<std::size_t>(std::numeric_limits<int>::max()));

#if defined(_WIN32)
        const int result = ::send(socket_.native_handle(), data.data() + sent,
                                  static_cast<int>(chunk_size), 0);
        if (result == SOCKET_ERROR) {
            throw std::system_error(::WSAGetLastError(), std::system_category(),
                                    "send");
        }
#else
        const ssize_t result = ::send(socket_.native_handle(), data.data() + sent,
                                      chunk_size, 0);
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw std::system_error(errno, std::generic_category(), "send");
        }
#endif

        if (result == 0) {
            throw std::system_error(
#if defined(_WIN32)
                WSAECONNRESET, std::system_category(), "send"
#else
                EPIPE, std::generic_category(), "send"
#endif
            );
        }

        sent += static_cast<std::size_t>(result);
    }
}

void TCPClient::send_all(const std::vector<std::uint8_t>& data) {
    send_all(std::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
}

std::vector<std::uint8_t> TCPClient::receive_some(std::size_t max_bytes) {
    if (!socket_.is_open()) {
        throw std::logic_error("TCPClient is not connected");
    }
    if (max_bytes == 0) {
        throw std::invalid_argument("max_bytes must be greater than 0");
    }

    const std::size_t buffer_size = std::min<std::size_t>(
        max_bytes, static_cast<std::size_t>(std::numeric_limits<int>::max()));
    std::vector<std::uint8_t> buffer(buffer_size);

#if defined(_WIN32)
    const int result = ::recv(socket_.native_handle(),
                              reinterpret_cast<char*>(buffer.data()),
                              static_cast<int>(buffer.size()), 0);
    if (result == SOCKET_ERROR) {
        throw std::system_error(::WSAGetLastError(), std::system_category(),
                                "recv");
    }
#else
    const ssize_t result = ::recv(socket_.native_handle(), buffer.data(),
                                  buffer.size(), 0);
    if (result < 0) {
        if (errno == EINTR) {
            return receive_some(max_bytes);
        }
        throw std::system_error(errno, std::generic_category(), "recv");
    }
#endif

    buffer.resize(static_cast<std::size_t>(result));
    return buffer;
}

void TCPClient::close() noexcept {
    socket_.close();
}

bool TCPClient::is_connected() const noexcept {
    return socket_.is_open();
}

const TCPClientConfig& TCPClient::config() const noexcept {
    return config_;
}

}  // namespace edgex::net
