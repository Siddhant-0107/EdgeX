#include "edgex/core/logger.hpp"
#include "edgex/net/socket.hpp"
#include "edgex/proxy/health_checker.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

namespace {

constexpr std::uint16_t kHealthyPort = 18084;
constexpr std::uint16_t kRecoveryPort = 18085;
constexpr std::uint16_t kPeriodicPort = 18086;

void send_all(edgex::net::native_socket_t socket, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
#if defined(_WIN32)
        const int result = ::send(socket, data.data() + sent,
                                  static_cast<int>(data.size() - sent), 0);
        assert(result != SOCKET_ERROR && result > 0);
#else
        const ssize_t result = ::send(socket, data.data() + sent, data.size() - sent, 0);
        assert(result > 0);
#endif
        sent += static_cast<std::size_t>(result);
    }
}

void serve_health_requests(edgex::net::Socket& listener, int request_count) {
    for (int i = 0; i < request_count; ++i) {
        edgex::net::Socket connection = listener.accept();
        char buffer[512]{};
#if defined(_WIN32)
        const int received = ::recv(connection.native_handle(), buffer, sizeof(buffer), 0);
#else
        const ssize_t received = ::recv(connection.native_handle(), buffer, sizeof(buffer), 0);
#endif
        assert(received > 0);
        const std::string request(buffer, static_cast<std::size_t>(received));
        assert(request.find("GET /health HTTP/1.1") != std::string::npos);

        send_all(connection.native_handle(),
                 "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nok");
    }
}

edgex::core::Logger make_test_logger(const char* path) {
    return edgex::core::Logger(edgex::core::Logger::Config{
        edgex::core::LogLevel::info, false, path});
}

}  // namespace

int main() {
    using edgex::proxy::Backend;
    using edgex::proxy::HealthChecker;
    using edgex::proxy::HealthCheckerConfig;
    using edgex::proxy::RoundRobinLoadBalancer;

    {
        edgex::net::Socket listener = edgex::net::Socket::create_tcp_ipv4();
        listener.bind(kHealthyPort, "127.0.0.1");
        listener.listen(8);
        std::thread server([&listener] { serve_health_requests(listener, 1); });

        RoundRobinLoadBalancer balancer({Backend{"127.0.0.1", kHealthyPort, false}});
        auto logger = make_test_logger("health_checker_healthy.log");
        HealthChecker checker(balancer, HealthCheckerConfig{
            std::chrono::milliseconds(100),
            std::chrono::milliseconds(500),
            "/health"}, logger);

        checker.check_once();
        assert(balancer.backends().at(0).healthy);
        assert(balancer.select().has_value());

        server.join();
        listener.close();
    }

    {
        RoundRobinLoadBalancer balancer({Backend{"127.0.0.1", kRecoveryPort, true}});
        auto logger = make_test_logger("health_checker_recovery.log");
        HealthChecker checker(balancer, HealthCheckerConfig{
            std::chrono::milliseconds(100),
            std::chrono::milliseconds(100),
            "/health"}, logger);

        checker.check_once();
        assert(!balancer.backends().at(0).healthy);
        assert(!balancer.select().has_value());

        edgex::net::Socket listener = edgex::net::Socket::create_tcp_ipv4();
        listener.bind(kRecoveryPort, "127.0.0.1");
        listener.listen(8);
        std::thread server([&listener] { serve_health_requests(listener, 1); });

        checker.check_once();
        assert(balancer.backends().at(0).healthy);
        assert(balancer.select().has_value());

        server.join();
        listener.close();
    }

    {
        edgex::net::Socket listener = edgex::net::Socket::create_tcp_ipv4();
        listener.bind(kPeriodicPort, "127.0.0.1");
        listener.listen(8);
        std::thread server([&listener] { serve_health_requests(listener, 2); });

        RoundRobinLoadBalancer balancer({Backend{"127.0.0.1", kPeriodicPort, false}});
        auto logger = make_test_logger("health_checker_periodic.log");
        HealthChecker checker(balancer, HealthCheckerConfig{
            std::chrono::milliseconds(25),
            std::chrono::milliseconds(500),
            "/health"}, logger);

        checker.start();
        assert(checker.is_running());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        checker.stop();
        assert(!checker.is_running());
        assert(balancer.backends().at(0).healthy);

        server.join();
        listener.close();
    }

    {
        RoundRobinLoadBalancer balancer({
            Backend{"127.0.0.1", kHealthyPort, false},
            Backend{"127.0.0.1", kRecoveryPort, false},
        });
        auto logger = make_test_logger("health_checker_filter.log");
        HealthChecker checker(balancer, HealthCheckerConfig{
            std::chrono::milliseconds(100),
            std::chrono::milliseconds(100),
            "/health"}, logger);

        edgex::net::Socket listener = edgex::net::Socket::create_tcp_ipv4();
        listener.bind(kHealthyPort, "127.0.0.1");
        listener.listen(8);
        std::thread server([&listener] { serve_health_requests(listener, 1); });

        checker.check_once();
        const auto selected = balancer.select();
        assert(selected.has_value());
        assert(selected->port == kHealthyPort);

        server.join();
        listener.close();
    }

    {
        std::ifstream file("health_checker_recovery.log");
        const std::string content((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
        assert(content.find("state=unhealthy") != std::string::npos);
        assert(content.find("state=healthy") != std::string::npos);
    }

    std::error_code error;
    for (const char* path : {
             "health_checker_healthy.log",
             "health_checker_recovery.log",
             "health_checker_periodic.log",
             "health_checker_filter.log"}) {
        std::filesystem::remove(path, error);
        assert(!error);
    }

    return 0;
}
