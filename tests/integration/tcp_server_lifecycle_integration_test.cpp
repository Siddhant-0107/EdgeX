#include "edgex/net/tcp_server.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

void expect(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

#if defined(_WIN32)

using native_socket_t = SOCKET;
constexpr native_socket_t kInvalidSocket = INVALID_SOCKET;

class WinsockRuntime {
public:
    WinsockRuntime() {
        WSADATA data{};
        const int result = ::WSAStartup(MAKEWORD(2, 2), &data);
        if (result != 0) {
            fail("WSAStartup failed");
        }
    }

    ~WinsockRuntime() {
        (void)::WSACleanup();
    }
};

void ensure_network_runtime() {
    static const WinsockRuntime runtime;
    (void)runtime;
}

int parse_ipv4_address(const std::string& address, in_addr* destination) {
#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
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

void close_native_socket(native_socket_t fd) noexcept {
    if (fd != kInvalidSocket) {
        (void)::closesocket(fd);
    }
}

#else

using native_socket_t = int;
constexpr native_socket_t kInvalidSocket = -1;

void ensure_network_runtime() {}

int parse_ipv4_address(const std::string& address, in_addr* destination) {
    return ::inet_pton(AF_INET, address.c_str(), destination);
}

void close_native_socket(native_socket_t fd) noexcept {
    if (fd != kInvalidSocket) {
        (void)::close(fd);
    }
}

#endif

class NativeTcpClient {
public:
    NativeTcpClient(const std::string& address, std::uint16_t port) {
        ensure_network_runtime();

        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ == kInvalidSocket) {
            fail("native client socket() failed");
        }

        sockaddr_in socket_address{};
        socket_address.sin_family = AF_INET;
        socket_address.sin_port = htons(port);

#if defined(_WIN32)
        const int parse_result = parse_ipv4_address(address, &socket_address.sin_addr);
        if (parse_result != 1) {
            fail("native client address parse failed");
        }

        const int connect_result =
            ::connect(fd_, reinterpret_cast<const sockaddr*>(&socket_address),
                      static_cast<int>(sizeof(socket_address)));
        if (connect_result == SOCKET_ERROR) {
            fail("native client connect() failed");
        }
#else
        const int parse_result = parse_ipv4_address(address, &socket_address.sin_addr);
        if (parse_result != 1) {
            fail("native client address parse failed");
        }

        if (::connect(fd_, reinterpret_cast<const sockaddr*>(&socket_address),
                      static_cast<socklen_t>(sizeof(socket_address))) == -1) {
            fail("native client connect() failed");
        }
#endif
    }

    ~NativeTcpClient() {
        close();
    }

    NativeTcpClient(const NativeTcpClient&) = delete;
    NativeTcpClient& operator=(const NativeTcpClient&) = delete;

    NativeTcpClient(NativeTcpClient&& other) noexcept
        : fd_(std::exchange(other.fd_, kInvalidSocket)) {}

    NativeTcpClient& operator=(NativeTcpClient&& other) noexcept {
        if (this != &other) {
            close();
            fd_ = std::exchange(other.fd_, kInvalidSocket);
        }

        return *this;
    }

    void close() noexcept {
        if (fd_ == kInvalidSocket) {
            return;
        }

        const native_socket_t fd = std::exchange(fd_, kInvalidSocket);
        close_native_socket(fd);
    }

private:
    native_socket_t fd_{kInvalidSocket};
};

class ServerStopGuard {
public:
    explicit ServerStopGuard(edgex::net::TCPServer& server) : server_(server) {}

    ~ServerStopGuard() {
        if (server_.is_running()) {
            server_.stop();
        }
    }

    ServerStopGuard(const ServerStopGuard&) = delete;
    ServerStopGuard& operator=(const ServerStopGuard&) = delete;

private:
    edgex::net::TCPServer& server_;
};

}  // namespace

int main() {
    try {
        constexpr std::uint16_t kTestPort = 18080;

        edgex::net::TCPServer server(edgex::net::TCPServerConfig{
            "127.0.0.1",
            kTestPort,
            8,
        });
        ServerStopGuard stop_guard(server);

        expect(!server.is_running(), "server should start in stopped state");

        server.start();
        expect(server.is_running(), "server should be running after start()");

        NativeTcpClient client_one("127.0.0.1", kTestPort);
        edgex::net::Socket accepted_one = server.accept_client();
        expect(accepted_one.is_open(), "first accepted socket should be open");
        expect(server.is_running(), "server should stay running after first accept");

        NativeTcpClient client_two("127.0.0.1", kTestPort);
        edgex::net::Socket accepted_two = server.accept_client();
        expect(accepted_two.is_open(), "second accepted socket should be open");

        expect(accepted_one.is_open(), "first accepted socket should still be open");
        expect(accepted_two.is_open(), "second accepted socket should still be open");

        accepted_one.close();
        expect(!accepted_one.is_open(), "first accepted socket should close");
        expect(accepted_two.is_open(), "second accepted socket should remain open");

        accepted_two.close();
        expect(!accepted_two.is_open(), "second accepted socket should close");

        server.stop();
        expect(!server.is_running(), "server should not be running after stop()");

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "tcp_server_lifecycle_integration_test failed: " << ex.what()
                  << '\n';
        return 1;
    }
}
