# EdgeX Design Decisions

## 1. C++17 and CMake

EdgeX uses Modern C++17 and CMake to keep the implementation portable while retaining explicit control over compilation and dependencies.

## 2. RAII for Network Resources

Sockets use explicit ownership and deterministic cleanup. Copying an owning socket is disabled; moving transfers ownership. This avoids manual close bookkeeping and double-close errors.

## 3. Thread Pool Instead of Unbounded Per-Request Threads

Client work is dispatched through a configurable worker pool. This provides bounded concurrency and makes shutdown behavior explicit.

## 4. Pimpl for Stateful Infrastructure Components

The thread pool and logger hide implementation state behind pimpls. Public headers expose stable interfaces without exposing synchronization primitives or implementation containers.

## 5. Router Simplicity for v1.0

Routing uses exact method/path matching rather than a complex pattern engine. The choice keeps behavior deterministic and the implementation easy to test. More advanced matching can be introduced later without changing the overall request pipeline.

## 6. Separate Backend Selection from Proxying

The load balancer selects a healthy backend; the reverse proxy performs the upstream HTTP exchange. This separation allows health and scheduling policy to evolve independently from forwarding logic.

## 7. Health Checks Outside the Load-Balancer Lock

The health checker snapshots backend state before performing network I/O. It does not hold the load-balancer mutex while connecting to or waiting on an upstream service, preventing slow backends from blocking unrelated selections.

## 8. Atomic Metrics

Frequently updated counters and latency aggregates use atomics so request-processing threads can record observations without a single global metrics lock.

## 9. Benchmarks Separate from CTest

Correctness tests and performance measurements have different purposes. Benchmarks are therefore opt-in and are not registered as pass/fail CTest cases.

## 10. Intentional v1.0 Boundaries

EdgeX v1.0 favors a focused, understandable implementation over production-scale feature breadth. TLS, HTTP/2/3, WebSockets, dynamic service discovery, event-driven I/O redesigns, and distributed features are deferred rather than hidden behind incomplete abstractions.
