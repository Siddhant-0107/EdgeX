#include "edgex/net/tcp_client.hpp"
#include "edgex/net/socket.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

constexpr std::uint16_t kTestPort = 18081;

void send_bytes(edgex::net::native_socket_t socket, const char* data, std::size_t size) {
    std::size_t sent = 0;
    while (sent < size) {
#if defined(_WIN32)
        const int result = ::send(socket, data + sent, static_cast<int>(size - sent), 0);
        assert(result != SOCKET_ERROR && result > 0);
#else
        const ssize_t result = ::send(socket, data + sent, size - sent, 0);
        assert(result > 0);
#endif
        sent += static_cast<std::size_t>(result);
    }
}

}  // namespace

int main() {
    using edgex::net::Socket;
    using edgex::net::TCPClient;
    using edgex::net::TCPClientConfig;

    Socket listener = Socket::create_tcp_ipv4();
    listener.bind(kTestPort, "127.0.0.1");
    listener.listen(8);

    std::thread server([&listener] {
        Socket connection = listener.accept();

        std::vector<char> buffer(64);
#if defined(_WIN32)
        const int received = ::recv(connection.native_handle(), buffer.data(),
                                     static_cast<int>(buffer.size()), 0);
#else
        const ssize_t received = ::recv(connection.native_handle(), buffer.data(),
                                        buffer.size(), 0);
#endif
        assert(received > 0);
        const std::string message(buffer.data(), static_cast<std::size_t>(received));
        assert(message == "hello from EdgeX");

        send_bytes(connection.native_handle(), "hello from upstream", 19);
    });

    TCPClient client(TCPClientConfig{
        "127.0.0.1",
        kTestPort,
        std::chrono::milliseconds(1000),
        std::chrono::milliseconds(1000),
    });

    assert(!client.is_connected());
    client.connect();
    assert(client.is_connected());

    client.send_all("hello from EdgeX");
    const auto response = client.receive_some();
    assert(std::string(response.begin(), response.end()) == "hello from upstream");

    client.close();
    assert(!client.is_connected());

    server.join();
    listener.close();

    return 0;
}
