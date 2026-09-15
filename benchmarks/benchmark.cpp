#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "edgex/core/thread_pool.hpp"
#include "edgex/http/http_parser.hpp"
#include "edgex/http/http_response_builder.hpp"
#include "edgex/http/router.hpp"
#include "edgex/net/tcp_client.hpp"
#include "edgex/net/tcp_server.hpp"

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

namespace {
using Clock = std::chrono::steady_clock;

struct Options {
    std::string component{"all"};
    std::size_t requests{10000};
    std::size_t concurrency{4};
    std::uint16_t port{19090};
};

struct Result {
    std::string name;
    std::size_t operations{0};
    std::size_t success{0};
    std::size_t failure{0};
    double elapsed_ms{0.0};
    std::vector<double> latencies_us;
};

void usage() {
    std::cout << "EdgeX benchmark suite\n"
              << "Usage: edgex_benchmark [--component all|parser|router|threadpool|server] "
                 "[--requests N] [--concurrency N] [--port P]\n";
}

std::size_t parse_size(const char* value, const char* option) {
    try {
        const unsigned long long parsed = std::stoull(value);
        if (parsed == 0 || parsed > std::numeric_limits<std::size_t>::max()) {
            throw std::runtime_error("out of range");
        }
        return static_cast<std::size_t>(parsed);
    } catch (...) {
        throw std::invalid_argument(std::string("invalid value for ") + option);
    }
}

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            usage();
            std::exit(EXIT_SUCCESS);
        }
        if (i + 1 >= argc) throw std::invalid_argument("missing value for " + arg);
        if (arg == "--component") options.component = argv[++i];
        else if (arg == "--requests") options.requests = parse_size(argv[++i], "--requests");
        else if (arg == "--concurrency") options.concurrency = parse_size(argv[++i], "--concurrency");
        else if (arg == "--port") {
            const auto parsed = parse_size(argv[++i], "--port");
            if (parsed > 65535) throw std::invalid_argument("invalid value for --port");
            options.port = static_cast<std::uint16_t>(parsed);
        } else {
            throw std::invalid_argument("unknown option: " + arg);
        }
    }
    if (options.component != "all" && options.component != "parser" &&
        options.component != "router" && options.component != "threadpool" &&
        options.component != "server") {
        throw std::invalid_argument("unknown component: " + options.component);
    }
    return options;
}

double elapsed_ms(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

double percentile(std::vector<double> values, double p) {
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    const double index = (p / 100.0) * static_cast<double>(values.size() - 1);
    const auto lower = static_cast<std::size_t>(index);
    const auto upper = std::min(lower + 1, values.size() - 1);
    const double fraction = index - static_cast<double>(lower);
    return values[lower] + (values[upper] - values[lower]) * fraction;
}

void print_result(const Result& result) {
    const double throughput = result.elapsed_ms > 0.0
        ? (static_cast<double>(result.success) / result.elapsed_ms) * 1000.0
        : 0.0;
    std::cout << std::fixed << std::setprecision(2)
              << "\n" << result.name << "\n"
              << "  operations : " << result.operations << "\n"
              << "  success    : " << result.success << "\n"
              << "  failure    : " << result.failure << "\n"
              << "  elapsed_ms : " << result.elapsed_ms << "\n"
              << "  throughput : " << throughput << " ops/s\n";
    if (!result.latencies_us.empty()) {
        std::cout << "  latency_us : p50=" << percentile(result.latencies_us, 50)
                  << " p95=" << percentile(result.latencies_us, 95)
                  << " p99=" << percentile(result.latencies_us, 99) << "\n";
    }
}

Result benchmark_parser(std::size_t requests) {
    const std::string request = "GET /benchmark HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\n\r\n";
    Result result{"HTTP parser benchmark", requests, 0, 0, 0.0, {}};
    result.latencies_us.reserve(requests);
    const auto start = Clock::now();
    for (std::size_t i = 0; i < requests; ++i) {
        const auto op_start = Clock::now();
        edgex::http::HttpParser parser;
        const auto parsed = parser.feed(request);
        if (parsed.status == edgex::http::ParseStatus::Complete && parsed.request) ++result.success;
        else ++result.failure;
        result.latencies_us.push_back(std::chrono::duration<double, std::micro>(Clock::now() - op_start).count());
    }
    result.elapsed_ms = elapsed_ms(start, Clock::now());
    return result;
}

Result benchmark_router(std::size_t requests) {
    edgex::http::Router router;
    router.get("/benchmark", [](const edgex::http::HttpRequest&) {
        return edgex::http::HttpResponseBuilder{}
            .status(edgex::http::HttpStatus::Ok)
            .body("ok")
            .build();
    });
    edgex::http::HttpRequest request;
    request.method = edgex::http::HttpMethod::Get;
    request.target = "/benchmark";
    request.version = edgex::http::HttpVersion::Http11;

    Result result{"Router matching benchmark", requests, 0, 0, 0.0, {}};
    result.latencies_us.reserve(requests);
    const auto start = Clock::now();
    for (std::size_t i = 0; i < requests; ++i) {
        const auto op_start = Clock::now();
        const auto response = router.handle(request);
        if (response.status == edgex::http::HttpStatus::Ok) ++result.success;
        else ++result.failure;
        result.latencies_us.push_back(std::chrono::duration<double, std::micro>(Clock::now() - op_start).count());
    }
    result.elapsed_ms = elapsed_ms(start, Clock::now());
    return result;
}

Result benchmark_thread_pool(std::size_t requests, std::size_t concurrency) {
    edgex::core::ThreadPool pool(concurrency);
    std::atomic<std::size_t> completed{0};
    std::mutex latency_mutex;
    std::vector<double> latencies;
    latencies.reserve(requests);
    const auto start = Clock::now();
    for (std::size_t i = 0; i < requests; ++i) {
        const auto submitted = Clock::now();
        pool.submit([&completed, &latencies, &latency_mutex, submitted] {
            const double us = std::chrono::duration<double, std::micro>(Clock::now() - submitted).count();
            {
                std::lock_guard<std::mutex> lock(latency_mutex);
                latencies.push_back(us);
            }
            completed.fetch_add(1, std::memory_order_relaxed);
        });
    }
    pool.shutdown();
    Result result{"Thread-pool task execution benchmark", requests,
                  completed.load(std::memory_order_relaxed), 0,
                  elapsed_ms(start, Clock::now()), std::move(latencies)};
    result.failure = result.operations - result.success;
    return result;
}

#if defined(_WIN32)
int native_recv(edgex::net::native_socket_t socket, char* buffer, int size) {
    return ::recv(socket, buffer, size, 0);
}
int native_send(edgex::net::native_socket_t socket, const char* buffer, int size) {
    return ::send(socket, buffer, size, 0);
}
#else
ssize_t native_recv(edgex::net::native_socket_t socket, char* buffer, int size) {
    return ::recv(socket, buffer, static_cast<std::size_t>(size), 0);
}
ssize_t native_send(edgex::net::native_socket_t socket, const char* buffer, int size) {
    return ::send(socket, buffer, static_cast<std::size_t>(size), 0);
}
#endif

void send_response(edgex::net::Socket& socket) {
    const std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nok";
    std::size_t sent = 0;
    while (sent < response.size()) {
        const auto n = native_send(socket.native_handle(), response.data() + sent,
                                   static_cast<int>(response.size() - sent));
        if (n <= 0) return;
        sent += static_cast<std::size_t>(n);
    }
}

Result benchmark_server(std::size_t requests, std::size_t concurrency, std::uint16_t port) {
    edgex::net::TCPServer server({"127.0.0.1", port, 128, concurrency});
    server.start();

    std::atomic<std::size_t> accepted{0};
    std::atomic<std::size_t> failed_tasks{0};
    std::thread acceptor([&] {
        try {
            while (accepted.load(std::memory_order_relaxed) < requests) {
                auto client = server.accept_client();
                accepted.fetch_add(1, std::memory_order_relaxed);
                server.submit_client_task([client = std::move(client), &failed_tasks]() mutable {
                    char buffer[4096];
                    std::string request;
                    for (;;) {
                        const auto n = native_recv(client.native_handle(), buffer, sizeof(buffer));
                        if (n <= 0) {
                            ++failed_tasks;
                            return;
                        }
                        request.append(buffer, static_cast<std::size_t>(n));
                        if (request.find("\r\n\r\n") != std::string::npos) break;
                        if (request.size() > 64 * 1024) {
                            ++failed_tasks;
                            return;
                        }
                    }
                    send_response(client);
                });
            }
        } catch (...) {
            ++failed_tasks;
        }
    });

    std::atomic<std::size_t> next_request{0};
    std::atomic<std::size_t> successes{0};
    std::atomic<std::size_t> failures{0};
    std::mutex latency_mutex;
    std::vector<double> latencies;
    latencies.reserve(requests);
    const auto start = Clock::now();
    std::vector<std::thread> clients;
    clients.reserve(concurrency);
    for (std::size_t worker = 0; worker < concurrency; ++worker) {
        clients.emplace_back([&] {
            for (;;) {
                const std::size_t index = next_request.fetch_add(1, std::memory_order_relaxed);
                if (index >= requests) break;
                try {
                    edgex::net::TCPClient client({"127.0.0.1", port,
                                                  std::chrono::milliseconds(2000),
                                                  std::chrono::milliseconds(2000)});
                    const auto op_start = Clock::now();
                    client.connect();
                    client.send_all("GET /benchmark HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
                    std::string response;
                    for (;;) {
                        const auto chunk = client.receive_some();
                        if (chunk.empty()) break;
                        response.append(reinterpret_cast<const char*>(chunk.data()), chunk.size());
                    }
                    const bool ok = response.rfind("HTTP/1.1 200", 0) == 0;
                    if (ok) successes.fetch_add(1, std::memory_order_relaxed);
                    else failures.fetch_add(1, std::memory_order_relaxed);
                    const double us = std::chrono::duration<double, std::micro>(Clock::now() - op_start).count();
                    std::lock_guard<std::mutex> lock(latency_mutex);
                    latencies.push_back(us);
                } catch (...) {
                    failures.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& client : clients) client.join();

    server.stop();
    acceptor.join();

    Result result{"TCP server request-throughput benchmark", requests,
                  successes.load(std::memory_order_relaxed),
                  failures.load(std::memory_order_relaxed) + failed_tasks.load(std::memory_order_relaxed),
                  elapsed_ms(start, Clock::now()), std::move(latencies)};
    return result;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        std::cout << "EdgeX v1.0 benchmark baseline\n"
                  << "requests=" << options.requests
                  << " concurrency=" << options.concurrency << "\n";
        if (options.component == "all" || options.component == "parser")
            print_result(benchmark_parser(options.requests));
        if (options.component == "all" || options.component == "router")
            print_result(benchmark_router(options.requests));
        if (options.component == "all" || options.component == "threadpool")
            print_result(benchmark_thread_pool(options.requests, options.concurrency));
        if (options.component == "all" || options.component == "server")
            print_result(benchmark_server(options.requests, options.concurrency, options.port));
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        std::cerr << "Benchmark error: " << ex.what() << '\n';
        usage();
        return EXIT_FAILURE;
    }
}
