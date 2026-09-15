#include "edgex/proxy/round_robin_load_balancer.hpp"

#include <stdexcept>
#include <utility>

namespace edgex::proxy {

RoundRobinLoadBalancer::RoundRobinLoadBalancer(std::vector<Backend> backends)
    : backends_(std::move(backends)) {
    for (const auto& backend : backends_) {
        if (backend.address.empty()) {
            throw std::invalid_argument("backend address must not be empty");
        }
        if (backend.port == 0) {
            throw std::invalid_argument("backend port must be greater than 0");
        }
    }
}

std::optional<Backend> RoundRobinLoadBalancer::select() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (backends_.empty()) {
        return std::nullopt;
    }

    for (std::size_t offset = 0; offset < backends_.size(); ++offset) {
        const std::size_t index = (next_index_ + offset) % backends_.size();
        if (!backends_[index].healthy) {
            continue;
        }

        next_index_ = (index + 1) % backends_.size();
        return backends_[index];
    }

    return std::nullopt;
}

void RoundRobinLoadBalancer::set_healthy(std::size_t index, bool healthy) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (index >= backends_.size()) {
        throw std::out_of_range("backend index out of range");
    }
    backends_[index].healthy = healthy;
}

std::vector<Backend> RoundRobinLoadBalancer::backends() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return backends_;
}

}  // namespace edgex::proxy
