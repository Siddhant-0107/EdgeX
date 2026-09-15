# EdgeX

> A modular backend infrastructure platform built from scratch in Modern C++17.

EdgeX is an educational, production-inspired backend infrastructure project that implements core backend building blocks without relying on a full web framework. The v1.0 scope covers TCP networking, HTTP/1.1 request handling, routing, static files, concurrency, reverse proxying, load balancing, health checks, logging, metrics, and benchmarking.

> **Project status:** v1.0 implementation complete. The repository contains the implemented core modules, unit/integration tests, benchmark suite, architecture documentation, and engineering workflow documentation. Final release packaging is tracked separately.

## v1.0 Features

| Component | Purpose |
|---|---|
| Socket layer | RAII abstraction over platform TCP sockets. |
| TCP server | Configurable bind/listen/accept lifecycle and worker-pool integration. |
| HTTP parser | Incremental HTTP/1.1 request parsing and validation. |
| Response builder | HTTP status, headers, body, and serialization support. |
| Router | Method + path route matching with controlled 404 handling. |
| Static file server | Document-root serving with traversal protection and MIME mapping. |
| Thread pool | Configurable workers, queued tasks, exception isolation, and graceful shutdown. |
| Logger | Thread-safe trace/debug/info/warning/error/critical logging to console and/or file. |
| TCP client | Outbound TCP connection, timeout, send, receive, and close support. |
| Reverse proxy | HTTP forwarding to an upstream backend with controlled 502 failures. |
| Round-robin load balancer | Thread-safe selection of healthy configured backends. |
| Health checker | Periodic backend probes with health-state transitions. |
| Metrics | Thread-safe request/error/latency/connection metrics and `/metrics` rendering. |
| Benchmark suite | Parser, router, thread-pool, and TCP-server performance measurements. |

## Architecture

The primary request path is:

```text
Client
  |
  | HTTP/1.1 over TCP
  v
TCP Server
  |
  v
Thread Pool
  |
  v
HTTP Parser
  |
  v
Router
  |----------------------|
  |                      |
  v                      v
Static File Server    Reverse Proxy
                         |
                         v
                 Round-Robin Load Balancer
                         |
                         v
                  Healthy Backend
```

The **load balancer selects** a healthy upstream; the **reverse proxy performs** the HTTP forwarding. The health checker updates backend health state, while logging and metrics provide operational visibility.

See [architecture.md](docs/architecture.md) and [component guide](docs/components.md) for detailed design.

## Repository Layout

```text
EdgeX/
|- include/edgex/       # Public C++ interfaces
|- src/                 # Module implementations
|- tests/               # Unit/integration tests and fixtures
|- benchmarks/          # Performance benchmark executable and instructions
|- config/              # Example configuration material
|- docs/                # Requirements, architecture, design, workflow, diagrams
|- scripts/              # Developer automation
|- cmake/               # Reusable CMake modules
|- examples/            # Small usage examples
`- third_party/         # Explicitly managed dependencies, if needed
```

## Build and Test

### Prerequisites

- CMake 3.21 or later.
- C++17 compiler.
- Windows: MSYS2 UCRT64 + GCC is a supported development environment; Visual Studio can also be used with an appropriate generator.
- Linux/macOS: C++17-capable GCC or Clang.

### Configure and build

```bash
cmake -S . -B build
cmake --build build
```

### Run tests

```bash
ctest --test-dir build --output-on-failure
```

The v1.0 validation suite contains **12 unit/integration tests**.

### Build benchmarks

Benchmarks are disabled by default:

```bash
cmake -S . -B build-ucrt64 -DEDGEX_BUILD_BENCHMARKS=ON
cmake --build build-ucrt64
```

Run the complete suite from the MSYS2 UCRT64 shell on Windows:

```bash
./build-ucrt64/benchmarks/edgex_benchmark.exe --requests 10000 --concurrency 4 --port 19090
```

Available options:

```text
--component all|parser|router|threadpool|server
--requests N
--concurrency N
--port P
```

See [benchmark documentation](benchmarks/README.md) for methodology.

## v1.0 Benchmark Baseline

Recorded using **10,000 requests and concurrency 4** on the project development environment:

| Benchmark | Throughput | p50 | p95 | p99 |
|---|---:|---:|---:|---:|
| HTTP parser | 201,242 ops/s | 3.90 us | 5.80 us | 6.00 us |
| Router matching | 1,876,736 ops/s | 0.40 us | 0.50 us | 0.50 us |
| Thread-pool task execution | 241,944 ops/s | 9.70 us | 855.20 us | 967.50 us |
| TCP server request throughput | 2,573 req/s | 508.90 us | 10,706.60 us | 21,762.85 us |

All 10,000 operations/requests completed successfully in the recorded run. Results are environment-dependent and should be treated as a reproducible reference, not a universal performance claim.

## Testing

The v1.0 suite covers HTTP parsing, response construction, routing, static-file security, thread-pool lifecycle/concurrency, logging, TCP client behavior, reverse proxying, load balancing, health checking, metrics, and TCP server lifecycle/integration behavior.

Correctness is kept separate from performance measurement: benchmarks are built and executed explicitly and are not registered as pass/fail CTest cases.

## Documentation

- [Software Requirements Specification](docs/SRS.md)
- [Architecture](docs/architecture.md)
- [Component Guide](docs/components.md)
- [Sequence Diagram](docs/diagrams/sequence.md)
- [Data Flow Diagram](docs/diagrams/dfd.md)
- [Component Diagram](docs/diagrams/component.md)
- [Design Decisions](docs/design-decisions.md)
- [Coding Standards](docs/coding-standards.md)
- [Development Workflow](docs/Development_Workflow.md)
- [Roadmap](docs/Roadmap.md)
- [Benchmarking](benchmarks/README.md)

## Known Limitations / Out of Scope

EdgeX v1.0 is a focused learning implementation and is **not intended for internet-facing production deployment**.

- HTTP/1.1 is the supported application protocol; HTTP/2 and HTTP/3 are out of scope.
- HTTPS/TLS and WebSockets are not implemented.
- Authentication, authorization, and rate limiting are not implemented.
- Reverse-proxy response handling and connection management are intentionally limited compared with mature proxy stacks; advanced transfer encodings are outside v1.0.
- Round-robin is the supported load-balancing strategy; dynamic service discovery is out of scope.
- Metrics are lightweight in-process metrics rather than a complete monitoring platform.
- The benchmark suite provides local reference measurements, not cross-machine performance certification.
- Event-driven I/O such as `epoll`/`kqueue` and distributed-system features are outside v1.0.

These are deliberate scope boundaries.

## Contributing

1. Read the [Coding Standards](docs/coding-standards.md) and [Development Workflow](docs/Development_Workflow.md).
2. Create a focused feature or fix branch.
3. Add or update tests for behavioral changes.
4. Build the project and run `ctest --test-dir build --output-on-failure`.
5. Keep commits focused and use Conventional Commit-style messages.
6. Document meaningful API or architectural changes.

## License

EdgeX is licensed under the [MIT License](LICENSE).

## Future Scope

Potential post-v1.0 work includes HTTPS/TLS, HTTP/2/3, WebSockets, authentication, richer metrics/tracing, additional load-balancing strategies, dynamic service discovery, event-driven I/O, containerization/CI, and distributed-system integrations.