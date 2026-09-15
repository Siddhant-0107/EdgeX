# EdgeX High-Level Architecture

## Purpose

EdgeX is a modular HTTP/1.1 backend infrastructure platform implemented in Modern C++17. The architecture separates networking, request parsing, routing, local content serving, proxying, backend selection, health checking, concurrency, logging, and metrics.

## Request Path

`Client -> TCP Server -> Thread Pool -> HTTP Parser -> Router -> Handler -> HTTP Response`

Proxy route: `Router -> Reverse Proxy -> Round-Robin Load Balancer -> Healthy Backend -> Reverse Proxy -> HTTP Response`

Static route: `Router -> Static File Server -> HTTP Response`

The load balancer **selects** a backend. The reverse proxy **performs** the outbound HTTP exchange. The health checker changes backend eligibility.

## Module Boundaries

| Module | Responsibility |
|---|---|
| Socket Layer | RAII ownership and platform-specific TCP socket operations. |
| TCP Server | Listener lifecycle, client acceptance, and dispatch to the worker pool. |
| TCP Client | Outbound TCP connection, timeout, send, receive, and close operations. |
| Thread Pool | Bounded workers, task queue, synchronization, and graceful draining shutdown. |
| HTTP Parser | Incremental HTTP/1.1 request decoding and validation. |
| Response Builder | HTTP response representation and serialization. |
| Router | Exact method/path route registration and dispatch. |
| Static File Server | Safe document-root resolution, MIME mapping, and file responses. |
| Reverse Proxy | Upstream request serialization, connection, response handling, and proxy errors. |
| Load Balancer | Thread-safe round-robin selection of healthy backends. |
| Health Checker | Periodic backend probing and health-state updates. |
| Logger | Thread-safe multi-level console/file logging. |
| Metrics | Atomic counters/latency/connection data and `/metrics` rendering. |
| Benchmark Suite | Repeatable parser/router/thread-pool/server performance measurements. |

## Concurrency Model

The TCP server owns the listening socket. Accepted client work can be submitted to a fixed-size thread pool. The pool protects its task queue with a mutex and coordinates workers using a condition variable. Shutdown closes the listening path and drains work already accepted by the pool according to its shutdown contract.

The load balancer synchronizes selection state and health updates. Metrics use atomics for frequently updated counters and latency aggregates. Logging serializes output writes.

## Proxy and Health-Check Interaction

1. The router dispatches to the reverse proxy.
2. The proxy asks the load balancer for a backend.
3. The load balancer skips unhealthy instances and returns the next eligible backend.
4. The proxy connects using the TCP client and forwards the HTTP request.
5. The upstream response is converted into an EdgeX response.
6. No healthy backend produces 503; communication failure produces controlled 502 handling.
7. Separately, the health checker probes each backend periodically and updates health state.

## Static File Safety

The static file server resolves the requested path relative to its configured document root and verifies that the resolved path remains within that root. This containment check prevents traversal from escaping the configured document tree.

## Observability

Logger records operational events according to its configured minimum level. Metrics records request/error counts, latency aggregates, active connections, and backend health information when a load balancer is supplied.

## Performance Evaluation

The benchmark suite is separate from CTest: CTest establishes correctness, while the benchmark executable measures elapsed time and throughput for parser operations, router matching, thread-pool scheduling/execution, and local TCP-server request handling.

## Design Principles

- **Single responsibility:** each subsystem owns one infrastructure concern.
- **RAII:** operating-system resources have explicit ownership and deterministic cleanup.
- **Thread safety:** mutable shared state is synchronized at the owning module boundary.
- **Testability:** networking-dependent behavior uses local test servers and focused tests.
- **Portability:** platform-specific socket handling is isolated from HTTP and proxy logic.
- **Controlled failure:** malformed requests, unavailable backends, and upstream failures become explicit results rather than process termination.

See [component.md](diagrams/component.md), [sequence.md](diagrams/sequence.md), and [dfd.md](diagrams/dfd.md) for the supporting diagrams.
