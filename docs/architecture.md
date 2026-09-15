# EdgeX High-Level Architecture

## 1. Purpose

EdgeX is a modular HTTP/1.1 backend infrastructure platform implemented in Modern C++17. The architecture separates networking, request parsing, routing, local content serving, proxying, backend selection, health checking, concurrency, logging, and metrics.

## 2. Request Path

The normal logical request path is:

`Client -> TCP Server -> Thread Pool -> HTTP Parser -> Router -> Handler -> HTTP Response`

A proxy route changes the handler path to:

`Router -> Reverse Proxy -> Round-Robin Load Balancer -> Healthy Backend -> Reverse Proxy -> HTTP Response`

A static route uses:

`Router -> Static File Server -> HTTP Response`

The load balancer **selects** a backend. The reverse proxy **performs** the outbound HTTP exchange. The health checker changes backend eligibility; it does not perform request routing.

## 3. Module Boundaries

| Module | Responsibility |
|---|---|
| Socket Layer | RAII ownership and platform-specific TCP socket operations. |
| TCP Server | Listener lifecycle, client acceptance, and dispatch to the worker pool. |
| TCP Client | Outbound TCP connection, timeout, send, receive, and close operations. |
| Thread Pool | Bounded worker threads, task queue, synchronization, and graceful draining shutdown. |
| HTTP Parser | Incremental HTTP/1.1 request decoding and validation. |
| Response Builder | HTTP response representation and serialization. |
| Router | Exact method/path route registration and dispatch. |
| Static File Server | Safe document-root file resolution, MIME mapping, and file responses. |
| Reverse Proxy | Upstream request serialization, connection, response handling, and proxy errors. |
| Load Balancer | Thread-safe round-robin selection of healthy backends. |
| Health Checker | Periodic backend probing and health-state updates. |
| Logger | Thread-safe multi-level console/file logging. |
| Metrics | Atomic counters/latency/connection data and `/metrics` rendering. |
| Benchmark Suite | Repeatable parser/router/thread-pool/server performance measurements. |

## 4. Component Diagram

See [component.md](diagrams/component.md).

## 5. Concurrency Model

The TCP server owns the listening socket. Accepted client work can be submitted to a fixed-size thread pool. The thread pool protects its task queue with a mutex and coordinates workers using a condition variable.

Shutdown closes the listening path, prevents new work from being accepted/submitted, and drains work already accepted by the pool according to the thread-pool shutdown contract.

The load balancer protects selection state and health updates with synchronization. Metrics use atomics for frequently updated counters and latency aggregates. Logging serializes output writes.

## 6. Proxy and Health-Check Interaction

For each proxy request:

1. The router dispatches to the reverse proxy.
2. The reverse proxy asks the load balancer for a backend.
3. The load balancer skips unhealthy instances.
4. The selected backend is returned to the proxy.
5. The proxy connects using the TCP client and forwards the HTTP request.
6. The upstream response is converted into an EdgeX response.
7. If no healthy backend exists, the proxy returns 503.
8. If communication with the selected backend fails, the proxy returns a controlled 502.

Separately, the health checker probes each backend periodically and updates the load balancer's health state.

## 7. Static File Safety

The static file server resolves the requested path relative to its configured document root and verifies that the resulting path remains within that root. This containment check prevents traversal from escaping the configured document tree.

## 8. Observability

Logger records operational events according to its configured minimum level. Metrics records request/error counts, latency aggregates, active connections, and backend health information when a load balancer is supplied.

## 9. Performance Evaluation

The benchmark suite is intentionally separate from CTest. CTest establishes correctness; the benchmark executable measures elapsed time and throughput. The v1.0 suite covers parser operations, router matching, thread-pool scheduling/execution, and local TCP-server request handling.

## 10. Design Principles

- **Single responsibility:** each subsystem owns one infrastructure concern.
- **RAII:** operating-system resources have explicit ownership and deterministic cleanup.
- **Thread safety:** mutable shared state is synchronized at the owning module boundary.
- **Testability:** networking-dependent behavior can be exercised through local test servers and focused tests.
- **Portability:** platform-specific socket handling is isolated from HTTP and proxy logic.
- **Controlled failure:** malformed requests, unavailable backends, and upstream failures become explicit results rather than process termination.
