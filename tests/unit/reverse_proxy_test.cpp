#include "edgex/proxy/reverse_proxy.hpp"
#include "edgex/net/socket.hpp"

#include <cassert>
#include <chrono>
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

constexpr std::uint16_t kTestPort = 18082;

std::string receive_request(edgex::net::Socket& connection) {
    std::string request;
    char buffer[4096];

    for (;;) {
#if defined(_WIN32)
        const int received = ::recv(connection.native_handle(), buffer, sizeof(buffer), 0);
#else
        const ssize_t received = ::recv(connection.native_handle(), buffer, sizeof(buffer), 0);
#endif
        assert(received > 0);
        request.append(buffer, static_cast<std::size_t>(received));

        const std::size_t header_end = request.find("\r\n\r\n");
        if (header_end == std::string::npos) continue;

        const std::size_t length_pos = request.find("Content-Length:");
        if (length_pos == std::string::npos) break;
        const std::size_t value_start = length_pos + 15;
        const std::size_t line_end = request.find("\r\n", value_start);
        const std::size_t body_length = static_cast<std::size_t>(
            std::stoul(request.substr(value_start, line_end - value_start)));
        const std::size_t body_start = header_end + 4;
        if (request.size() >= body_start + body_length) break;
    }

    return request;
}

void send_all(edgex::net::native_socket_t socket, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
#if defined(_WIN32)
        const int result = ::send(socket, data.data() + sent,
                                  static_cast<int>(data.size() - sent), 0);
        assert(result > 0 && result != SOCKET_ERROR);
#else
        const ssize_t result = ::send(socket, data.data() + sent, data.size() - sent, 0);
        assert(result > 0);
#endif
        sent += static_cast<std::size_t>(result);
    }
}

}  // namespace

int main() {
    using edgex::http::HttpMethod;
    using edgex::http::HttpStatus;
    using edgex::net::Socket;
    using edgex::proxy::ReverseProxy;
    using edgex::proxy::ReverseProxyConfig;

    Socket listener = Socket::create_tcp_ipv4();
    listener.bind(kTestPort, "127.0.0.1");
    listener.listen(8);

    std::thread backend([&listener] {
        Socket connection = listener.accept();
        const std::string request = receive_request(connection);

        assert(request.find("POST /api/items?id=7 HTTP/1.1\r\n") == 0);
        assert(request.find("Host: example.test\r\n") != std::string::npos);
        assert(request.find("X-Request-ID: edge-x-1\r\n") != std::string::npos);
        assert(request.find("Connection: close\r\n") != std::string::npos);
        assert(request.find("Connection: keep-alive") == std::string::npos);
        assert(request.find("hello upstream") != std::string::npos);

        const std::string response =
            "HTTP/1.1 201 Created\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: 15\r\n"
            "Connection: close\r\n"
            "X-Upstream: mock\r\n"
            "\r\n"
            "{\"ok\":true}\n123";
        send_all(connection.native_handle(), response);
        connection.close();
    });

    ReverseProxy proxy(ReverseProxyConfig{
        "127.0.0.1",
        kTestPort,
        std::chrono::milliseconds(1000),
        std::chrono::milliseconds(1000),
    });

    edgex::http::HttpRequest request;
    request.method = HttpMethod::Post;
    request.target = "/api/items?id=7";
    request.headers = {
        {"Host", "example.test"},
        {"X-Request-ID", "edge-x-1"},
        {"Connection", "keep-alive"},
    };
    request.body = std::vector<std::uint8_t>{'h','e','l','l','o',' ','u','p','s','t','r','e','a','m'};

    const auto response = proxy.forward(request);
    assert(response.status == HttpStatus::Created);
    const std::vector<std::uint8_t> expected_body{
        '{','"','o','k','"',':','t','r','u','e','}','\n','1','2','3'
    };
    assert(response.body == expected_body);
    assert(response.has_header("Content-Type"));
    assert(response.has_header("X-Upstream"));
    assert(!response.has_header("Connection"));

    backend.join();
    listener.close();

    ReverseProxy unavailable(ReverseProxyConfig{
        "127.0.0.1",
        18083,
        std::chrono::milliseconds(100),
        std::chrono::milliseconds(100),
    });
    const auto bad_gateway = unavailable.forward(request);
    assert(bad_gateway.status == HttpStatus::BadGateway);

    return 0;
}
