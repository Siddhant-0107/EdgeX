#include "edgex/core/thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using edgex::core::ThreadPool;

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void test_zero_workers_rejected() {
    bool threw = false;

    try {
        ThreadPool pool(0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    expect(threw,
           "zero worker count should throw std::invalid_argument");

    std::cout << "PASS: zero workers rejected\n";
}

void test_single_task_execution() {
    ThreadPool pool(1);

    std::atomic<bool> executed{false};

    pool.submit([&executed] {
        executed.store(true);
    });

    pool.shutdown();

    expect(executed.load(),
           "submitted task should execute");

    std::cout << "PASS: single task execution\n";
}

void test_multiple_tasks_execute() {
    ThreadPool pool(2);

    constexpr int task_count = 100;
    std::atomic<int> completed{0};

    for (int i = 0; i < task_count; ++i) {
        pool.submit([&completed] {
            completed.fetch_add(1);
        });
    }

    pool.shutdown();

    expect(completed.load() == task_count,
           "all submitted tasks should execute");

    std::cout << "PASS: multiple task execution\n";
}

void test_concurrent_submission() {
    ThreadPool pool(4);

    constexpr int submitter_count = 4;
    constexpr int tasks_per_submitter = 50;
    constexpr int expected_tasks =
        submitter_count * tasks_per_submitter;

    std::atomic<int> completed{0};

    std::vector<std::thread> submitters;
    submitters.reserve(submitter_count);

    for (int i = 0; i < submitter_count; ++i) {
        submitters.emplace_back([&pool, &completed] {
            for (int j = 0; j < tasks_per_submitter; ++j) {
                pool.submit([&completed] {
                    completed.fetch_add(1);
                });
            }
        });
    }

    for (auto& submitter : submitters) {
        submitter.join();
    }

    pool.shutdown();

    expect(completed.load() == expected_tasks,
           "tasks submitted concurrently should all execute");

    std::cout << "PASS: concurrent submission\n";
}

void test_shutdown_drains_pending_tasks() {
    ThreadPool pool(2);

    constexpr int task_count = 100;
    std::atomic<int> completed{0};

    for (int i = 0; i < task_count; ++i) {
        pool.submit([&completed] {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1));

            completed.fetch_add(1);
        });
    }

    pool.shutdown();

    expect(completed.load() == task_count,
           "shutdown should allow pending tasks to finish");

    std::cout << "PASS: shutdown drains pending tasks\n";
}

void test_submit_after_shutdown_rejected() {
    ThreadPool pool(1);

    pool.shutdown();

    bool threw = false;

    try {
        pool.submit([] {});
    } catch (const std::runtime_error&) {
        threw = true;
    }

    expect(threw,
           "submit after shutdown should throw std::runtime_error");

    std::cout << "PASS: submit after shutdown rejected\n";
}

void test_task_exception_isolated() {
    ThreadPool pool(1);

    std::atomic<bool> second_task_executed{false};

    pool.submit([] {
        throw std::runtime_error("intentional task failure");
    });

    pool.submit([&second_task_executed] {
        second_task_executed.store(true);
    });

    pool.shutdown();

    expect(second_task_executed.load(),
           "exception in one task should not stop worker");

    std::cout << "PASS: task exception isolated\n";
}

} // namespace

int main() {
    test_zero_workers_rejected();
    test_single_task_execution();
    test_multiple_tasks_execute();
    test_concurrent_submission();
    test_shutdown_drains_pending_tasks();
    test_submit_after_shutdown_rejected();
    test_task_exception_isolated();

    std::cout << "All 7 thread pool tests passed\n";

    return EXIT_SUCCESS;
}
