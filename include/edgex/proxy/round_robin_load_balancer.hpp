#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace edgex::proxy {

struct Backend {
    std::string address;
    std::uint16_t port{0};
    bool healthy{true};
};

class RoundRobinLoadBalancer {
public:
    explicit RoundRobinLoadBalancer(std::vector<Backend> backends);

    RoundRobinLoadBalancer(const RoundRobinLoadBalancer&) = delete;
    RoundRobinLoadBalancer& operator=(const RoundRobinLoadBalancer&) = delete;
    RoundRobinLoadBalancer(RoundRobinLoadBalancer&&) = delete;
    RoundRobinLoadBalancer& operator=(RoundRobinLoadBalancer&&) = delete;

    [[nodiscard]] std::optional<Backend> select();
    void set_healthy(std::size_t index, bool healthy);
    [[nodiscard]] std::vector<Backend> backends() const;

private:
    mutable std::mutex mutex_;
    std::vector<Backend> backends_;
    std::size_t next_index_{0};
};

}  // namespace edgex::proxy
