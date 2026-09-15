#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
namespace edgex::core {

/// Executes submitted tasks using a fixed set of worker threads.
class ThreadPool {
public:
    /// Creates a thread pool with the requested number of workers.
    ///
    /// @throws std::invalid_argument if worker_count is zero.
    explicit ThreadPool(std::size_t worker_count);

    /// Shuts down the pool and waits for all worker threads to finish.
    ///
    /// Tasks already submitted before shutdown begins are allowed to
    /// complete. Tasks submitted after shutdown begins are rejected.
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /// Adds a task to the worker queue.
    ///
    /// @throws std::invalid_argument if task is empty.
    /// @throws std::runtime_error if shutdown has begun.
    void submit(std::function<void()> task);

    /// Begins shutdown and waits for all pending tasks to finish.
    ///
    /// Calling shutdown more than once is safe.
    void shutdown();

private:
    class Impl;

    std::unique_ptr<Impl> impl_;
};

} // namespace edgex::core
