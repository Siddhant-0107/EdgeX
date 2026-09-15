# EdgeX Roadmap

## Sprint 1 — v1.0

**Objective:** Deliver a modular HTTP/1.1 backend infrastructure platform in Modern C++17 covering core networking, request handling, proxying, load balancing, health checks, observability, performance evaluation, and engineering documentation.

**v1.0 scope boundary:** HTTPS/TLS, HTTP/2/3, WebSockets, authentication/authorization, databases, event-driven I/O redesigns, dynamic service discovery, and distributed-system features are deferred to future releases.

## Milestone Status

| # | Milestone | Status |
|---:|---|:---:|
| 1 | Socket Layer | ✅ Complete |
| 2 | TCP Server | ✅ Complete |
| 3 | HTTP Parser | ✅ Complete |
| 4 | Response Builder | ✅ Complete |
| 5 | Router | ✅ Complete |
| 6 | Static File Server | ✅ Complete |
| 7 | Thread Pool | ✅ Complete |
| 8 | Logger | ✅ Complete |
| 9 | Reverse Proxy | ✅ Complete |
| 10 | Load Balancer | ✅ Complete |
| 11 | Health Checker | ✅ Complete |
| 12 | Metrics | ✅ Complete |
| 13 | Benchmarking | ✅ Complete |
| 14 | Documentation and Release | 🔄 In progress |

## Completed v1.0 Deliverables

### Networking

- RAII TCP socket abstraction with platform-aware native handle handling.
- TCP server lifecycle with configurable bind address, port, backlog, and worker count.
- Outbound TCP client with connection and I/O timeouts.

### HTTP

- Incremental HTTP/1.1 request parser.
- HTTP response model and serialization/builder support.
- Exact method/path router with controlled 404 handling.
- Static file serving with document-root containment and traversal protection.

### Concurrency and operations

- Configurable thread pool with synchronized task queue and graceful draining shutdown.
- Thread-safe multi-level logger with console/file output.
- Runtime metrics for requests, errors, latency, active connections, and backend health.

### Proxying and resilience

- Reverse proxy with upstream timeouts and controlled 502 handling.
- Thread-safe round-robin backend selection.
- Health checker that probes backends and updates load-balancer eligibility.
- 503 behavior when no healthy backend is available.

### Validation and performance

- 12 registered unit/integration tests.
- Benchmark suite for parser, router, thread pool, and TCP server.
- v1.0 reference run: 10,000 requests, concurrency 4, with 100% successful benchmark operations.

## Documentation and Release

The final release milestone is responsible for:

- Keeping README and technical documents synchronized with implemented behavior.
- Providing architecture, component, sequence, and data-flow diagrams.
- Recording benchmark methodology and baseline results.
- Maintaining coding and development workflow guidance.
- Publishing release notes and known limitations.
- Performing final validation and creating the annotated `v1.0.0` tag.

## Sprint Completion Criteria

- [x] Core v1.0 implementation milestones completed.
- [x] EdgeX builds using the documented CMake workflow.
- [x] Unit and integration tests pass.
- [x] Benchmark baseline recorded.
- [x] Documentation updated for implemented behavior and v1.0 boundaries.
- [ ] Final release notes and annotated `v1.0.0` tag.
