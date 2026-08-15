#pragma once

#include <cstdint>
#include <string>

#if defined(_WIN32)
#include <winsock2.h>
#endif

namespace edgex::net {

/// The operating system's native socket handle type.
#if defined(_WIN32)
using native_socket_t = SOCKET;
#else
using native_socket_t = int;
#endif

/// Owns an IPv4 TCP socket handle.
class Socket {
public:
    /// Creates an empty socket that owns no file descriptor.
    Socket() noexcept = default;

    /// Takes exclusive ownership of an existing native socket handle.
    explicit Socket(native_socket_t fd) noexcept;

    /// Closes the owned file descriptor, if any.
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    /// Transfers file descriptor ownership from another socket.
    Socket(Socket&& other) noexcept;

    /// Releases this descriptor and transfers ownership from another socket.
    Socket& operator=(Socket&& other) noexcept;

    /// Creates an IPv4 TCP socket.
    /// @throws std::system_error when the socket cannot be created.
    [[nodiscard]] static Socket create_tcp_ipv4();

    /// Binds the socket to an IPv4 address and port.
    /// @throws std::invalid_argument when address is not a valid IPv4 address.
    /// @throws std::system_error when bind fails.
    void bind(std::uint16_t port, const std::string& address = "0.0.0.0");

    /// Marks the socket as a listener.
    /// @throws std::system_error when listen fails.
    void listen(int backlog = 128);

    /// Accepts one pending TCP connection.
    /// @return A socket that exclusively owns the accepted connection.
    /// @throws std::system_error when accept fails.
    [[nodiscard]] Socket accept();

    /// Releases the descriptor now, if this socket owns one.
    void close() noexcept;

    /// Returns whether this socket owns an open descriptor.
    [[nodiscard]] bool is_open() const noexcept;

    /// Returns the owned native handle without transferring ownership.
    [[nodiscard]] native_socket_t native_handle() const noexcept;

private:
    native_socket_t fd_{
#if defined(_WIN32)
        INVALID_SOCKET
#else
        -1
#endif
    };
};

}  // namespace edgex::net
