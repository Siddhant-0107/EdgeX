#include <atomic>
#include <cassert>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>

#include "edgex/proxy/round_robin_load_balancer.hpp"

namespace {

using edgex::proxy::Backend;
using edgex::proxy::RoundRobinLoadBalancer;

std::vector<Backend> make_backends() {
    return {
        {"127.0.0.1", 18090, true},
        {"127.0.0.1", 18091, true},
        {"127.0.0.1", 18092, true},
    };
}

void test_round_robin_order() {
    RoundRobinLoadBalancer balancer(make_backends());

    const auto first = balancer.select();
    const auto second = balancer.select();
    const auto third = balancer.select();
    const auto fourth = balancer.select();

    assert(first.has_value() && first->port == 18090);
    assert(second.has_value() && second->port == 18091);
    assert(third.has_value() && third->port == 18092);
    assert(fourth.has_value() && fourth->port == 18090);
}

void test_unhealthy_backend_is_skipped() {
    RoundRobinLoadBalancer balancer(make_backends());

    assert(balancer.select()->port == 18090);
    balancer.set_healthy(1, false);

    assert(balancer.select()->port == 18092);
    assert(balancer.select()->port == 18090);
    assert(balancer.select()->port == 18092);
}

void test_all_unhealthy_returns_no_backend() {
    RoundRobinLoadBalancer balancer(make_backends());
    balancer.set_healthy(0, false);
    balancer.set_healthy(1, false);
    balancer.set_healthy(2, false);

    assert(!balancer.select().has_value());
}

void test_health_recovery() {
    RoundRobinLoadBalancer balancer(make_backends());
    balancer.set_healthy(0, false);
    balancer.set_healthy(1, false);

    assert(balancer.select()->port == 18092);
    assert(!balancer.select().has_value() == false);

    balancer.set_healthy(0, true);
    assert(balancer.select()->port == 18090);
}

void test_empty_backends() {
    RoundRobinLoadBalancer balancer({});
    assert(!balancer.select().has_value());
}

void test_invalid_backend_rejected() {
    bool threw = false;
    try {
        RoundRobinLoadBalancer balancer({{"", 18090, true}});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);

    threw = false;
    try {
        RoundRobinLoadBalancer balancer({{"127.0.0.1", 0, true}});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);
}

void test_selection_is_thread_safe() {
    constexpr std::size_t kThreadCount = 8;
    constexpr std::size_t kSelectionsPerThread = 1000;
    RoundRobinLoadBalancer balancer(make_backends());
    std::atomic<std::size_t> completed{0};

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);
    for (std::size_t i = 0; i < kThreadCount; ++i) {
        threads.emplace_back([&]() {
            for (std::size_t j = 0; j < kSelectionsPerThread; ++j) {
                const auto backend = balancer.select();
                assert(backend.has_value());
                assert(backend->port >= 18090 && backend->port <= 18092);
                completed.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    assert(completed == kThreadCount * kSelectionsPerThread);
}

}  // namespace

int main() {
    test_round_robin_order();
    test_unhealthy_backend_is_skipped();
    test_all_unhealthy_returns_no_backend();
    test_health_recovery();
    test_empty_backends();
    test_invalid_backend_rejected();
    test_selection_is_thread_safe();
    return 0;
}
