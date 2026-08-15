#include "edgex/net/socket.hpp"

#include <cstddef>
#include <stdexcept>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#include <ws2tcpip.h>
#else
#include <cerrno>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace edgex::net {
namespace {

#if defined(_WIN32)

class WinsockRuntime {
public:
    WinsockRuntime() {
        WSADATA data{};
        const int result = ::WSAStartup(MAKEWORD(2, 2), &data);
        if (result != 0) {
            throw std::system_error(result, std::system_category(), "WSAStartup");
        }
    }

    ~WinsockRuntime() {
        (void)::WSACleanup();
    }
};

void ensure_winsock_initialized() {
    static const WinsockRuntime runtime;
    (void)runtime;
}

constexpr native_socket_t kInvalidSocket = INVALID_SOCKET;

int parse_ipv4_address(const std::string& address, in_addr* destination) {
#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
    // Legacy MinGW lacks InetPtonA despite providing Winsock2.
    auto* bytes = reinterpret_cast<unsigned char*>(destination);
    std::size_t position = 0;

    for (std::size_t octet_index = 0; octet_index < 4; ++octet_index) {
        if (position == address.size() || address[position] < '0' ||
            address[position] > '9') {
            return 0;
        }

        unsigned int octet = 0;
        while (position < address.size() && address[position] >= '0' &&
               address[position] <= '9') {
            octet = (octet * 10) + static_cast<unsigned int>(address[position] - '0');
            if (octet > 255) {
                return 0;
            }

            ++position;
        }

        bytes[octet_index] = static_cast<unsigned char>(octet);
        if (octet_index < 3) {
            if (position == address.size() || address[position] != '.') {
                return 0;
            }

            ++position;
        }
    }

    return position == address.size() ? 1 : 0;
#else
    return ::InetPtonA(AF_INET, address.c_str(), destination);
#endif
}

[[noreturn]] void throw_last_socket_error(const char* operation) {
    throw std::system_error(::WSAGetLastError(), std::system_category(), operation);
}

#else

constexpr native_socket_t kInvalidSocket = -1;

int parse_ipv4_address(const std::string& address, in_addr* destination) {
    return ::inet_pton(AF_INET, address.c_str(), destination);
}

[[noreturn]] void throw_last_socket_error(const char* operation) {
    throw std::system_error(errno, std::generic_category(), operation);
}

#endif

}  // namespace

Socket::Socket(native_socket_t fd) noexcept : fd_(fd) {}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept : fd_(std::exchange(other.fd_, kInvalidSocket)) {}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        fd_ = std::exchange(other.fd_, kInvalidSocket);
    }

    return *this;
}

Socket Socket::create_tcp_ipv4() {
#if defined(_WIN32)
    ensure_winsock_initialized();
#endif

    const native_socket_t fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd == kInvalidSocket) {
        throw_last_socket_error("socket");
    }

    return Socket(fd);
}

void Socket::bind(std::uint16_t port, const std::string& address) {
    sockaddr_in socket_address{};
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(port);

    const int conversion_result = parse_ipv4_address(address, &socket_address.sin_addr);
    if (conversion_result != 1) {
        if (conversion_result == 0) {
            throw std::invalid_argument("address is not a valid IPv4 address");
        }

        throw_last_socket_error("inet_pton");
    }

    if (::bind(fd_, reinterpret_cast<const sockaddr*>(&socket_address),
               static_cast<int>(sizeof(socket_address))) == -1) {
        throw_last_socket_error("bind");
    }
}

void Socket::listen(int backlog) {
    if (::listen(fd_, backlog) == -1) {
        throw_last_socket_error("listen");
    }
}

Socket Socket::accept() {
    native_socket_t client_fd = kInvalidSocket;
#if defined(_WIN32)
    client_fd = ::accept(fd_, nullptr, nullptr);
#else
    do {
        client_fd = ::accept(fd_, nullptr, nullptr);
    } while (client_fd == kInvalidSocket && errno == EINTR);
#endif

    if (client_fd == kInvalidSocket) {
        throw_last_socket_error("accept");
    }

    return Socket(client_fd);
}

void Socket::close() noexcept {
    if (fd_ == kInvalidSocket) {
        return;
    }

    const native_socket_t fd = std::exchange(fd_, kInvalidSocket);
#if defined(_WIN32)
    (void)::closesocket(fd);
#else
    (void)::close(fd);
#endif
}

bool Socket::is_open() const noexcept {
    return fd_ != kInvalidSocket;
}

native_socket_t Socket::native_handle() const noexcept {
    return fd_;
}

}  // namespace edgex::net
