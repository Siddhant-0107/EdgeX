# EdgeX v1.0.0

## Added

- RAII TCP socket abstraction with cross-platform Windows/POSIX handling.
- Configurable TCP server lifecycle with thread-pool integration.
- Incremental HTTP/1.1 request parsing and validation.
- HTTP response construction and serialization.
- Method/path router with controlled 404 handling.
- Secure static file serving with path-traversal protection and MIME mapping.
- Configurable thread pool with graceful shutdown and task exception isolation.
- Thread-safe configurable logger with console and optional file output.
- Outbound TCP client with connection and I/O timeouts.
- Reverse proxy with controlled upstream failure handling.
- Thread-safe round-robin load balancing across healthy backends.
- Periodic backend health checking and health-state transition logging.
- Thread-safe runtime metrics including requests, errors, latency, connections, and backend health.
- Configurable benchmark suite for HTTP parsing, routing, thread-pool execution, and TCP server throughput.

## Changed

- Completed the planned EdgeX v1.0 backend infrastructure scope.
- Added benchmark build support through `EDGEX_BUILD_BENCHMARKS`.
- Updated project documentation to reflect implemented architecture, APIs, workflows, testing, benchmarking, and limitations.
- Updated project version to `1.0.0`.

## Fixed

- Corrected Windows socket-handle ownership and portability issues.
- Hardened static-file path containment and fixture handling.
- Improved thread-pool shutdown and task-exception behavior.
- Corrected logger lifecycle/file-output behavior on Windows.
- Added controlled proxy, load-balancer, and health-check failure paths.
- Fixed HTTP parser edge cases and test-helper lifetime/ownership issues.

## Known Limitations

- EdgeX v1.0 supports HTTP/1.1 over plain TCP; HTTPS/TLS, HTTP/2, HTTP/3, and WebSockets are out of scope.
- Authentication, authorization, rate limiting, and database integration are not implemented.
- Reverse-proxy transfer-encoding and connection-management behavior is intentionally limited compared with mature production proxy stacks.
- Load balancing is round-robin; dynamic service discovery is out of scope.
- Metrics are lightweight in-process metrics rather than a complete monitoring platform.
- Benchmarks are local reference measurements and are dependent on hardware, operating system, compiler, and build configuration.
- Event-driven I/O and distributed-system features are outside the v1.0 scope.

## Validation

The v1.0 development validation suite contains 12 unit/integration tests and was recorded at 100% passing. The benchmark suite includes configurable request count and concurrency and records throughput, success/failure counts, elapsed time, and latency percentiles.

See `README.md` and `benchmarks/README.md` for build, test, benchmark, and baseline instructions.
