#include "edgex/core/thread_pool.hpp"

#include <condition_variable>
#include <exception>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace edgex::core {

class ThreadPool::Impl {
public:
    explicit Impl(std::size_t worker_count)
        : stopping(false) {

        if (worker_count == 0) {
            throw std::invalid_argument(
                "ThreadPool requires at least one worker");
        }

        workers.reserve(worker_count);

        for (std::size_t i = 0; i < worker_count; ++i) {
            workers.emplace_back([this] {
                worker_loop();
            });
        }
    }

    ~Impl() {
        shutdown();
    }

    void submit(std::function<void()> task) {
        if (!task) {
            throw std::invalid_argument("cannot submit an empty task");
        }

        {
            std::lock_guard<std::mutex> lock(mutex);

            if (stopping) {
                throw std::runtime_error(
                    "cannot submit task after shutdown");
            }

            tasks.push(std::move(task));
        }

        condition.notify_one();
    }

    void shutdown() noexcept {
        {
            std::lock_guard<std::mutex> lock(mutex);

            if (stopping) {
                return;
            }

            stopping = true;
        }

        condition.notify_all();

        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    void worker_loop() noexcept {
        while (true) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(mutex);

                condition.wait(lock, [this] {
                    return stopping || !tasks.empty();
                });

                if (stopping && tasks.empty()) {
                    return;
                }

                task = std::move(tasks.front());
                tasks.pop();
            }

            try {
                task();
            } catch (...) {
                // A failed task must not terminate the worker thread
                // or the entire process.
            }
        }
    }

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex mutex;
    std::condition_variable condition;

    bool stopping;
};

ThreadPool::ThreadPool(std::size_t worker_count)
    : impl_(std::make_unique<Impl>(worker_count)) {}

ThreadPool::~ThreadPool() = default;

void ThreadPool::submit(std::function<void()> task) {
    impl_->submit(std::move(task));
}

void ThreadPool::shutdown() {
    impl_->shutdown();
}

} // namespace edgex::core
