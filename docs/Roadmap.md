# EdgeX Roadmap

## Sprint 1 - v1.0

**Objective:** Deliver a modular HTTP/1.1 backend infrastructure platform in Modern C++17. Version 1.0 focuses on core networking, request handling, proxying, observability, performance evaluation, and project documentation.

**Out of scope:** HTTPS, HTTP/2, WebSockets, authentication, databases, `epoll`, and distributed-system features.

## Milestone 1 - Socket Layer

**Goal:** Build a safe, reusable RAII abstraction over operating-system TCP sockets.

**Expected outcome:** EdgeX can create, configure, bind, listen on, accept, read from, write to, and close sockets without manual resource leaks.

**Deliverables:**

- [ ] `Socket` RAII wrapper with move semantics and deleted copy operations.
- [ ] Socket creation, bind, listen, accept, send, receive, and close operations.
- [ ] Error representation for expected socket failures.
- [ ] Unit tests for ownership and failure paths where practical.
- [ ] Module documentation for the socket lifecycle.

## Milestone 2 - TCP Server

**Goal:** Create the server lifecycle and connection-acceptance loop.

**Expected outcome:** EdgeX can listen on a configured address and port, accept client connections, and manage graceful startup and shutdown.

**Deliverables:**

- [ ] `TcpServer` component using the Socket Layer.
- [ ] Configurable host, port, backlog, and connection timeout settings.
- [ ] Graceful shutdown mechanism.
- [ ] Initial connection lifecycle logging.
- [ ] Local smoke test using a TCP client.

## Milestone 3 - HTTP Parser

**Goal:** Parse and validate incoming HTTP/1.1 requests.

**Expected outcome:** Raw client bytes are converted into structured request objects or clear parse errors.

**Deliverables:**

- [ ] `HttpRequest` model for method, target, version, headers, and body.
- [ ] HTTP request-line parser.
- [ ] Header and optional request-body parser.
- [ ] Validation for malformed request syntax and configured request-size limits.
- [ ] Unit tests for valid, incomplete, and malformed requests.

## Milestone 4 - Response Builder

**Goal:** Generate valid HTTP/1.1 responses.

**Expected outcome:** EdgeX can return status lines, headers, bodies, and standard error responses to clients.

**Deliverables:**

- [ ] `HttpResponse` model and response serialization utility.
- [ ] Content-Length generation where applicable.
- [ ] Standard responses for 200, 400, 404, 500, 502, and 503.
- [ ] Configurable response headers and content types.
- [ ] Unit tests for serialized response correctness.

## Milestone 5 - Router

**Goal:** Route requests by method and path to the correct handler.

**Expected outcome:** EdgeX can match configured routes and return a controlled 404 response when no route matches.

**Deliverables:**

- [ ] Route registration by HTTP method and path.
- [ ] Route-match result and handler interface.
- [ ] Default 404 handling.
- [ ] Unit tests for matching, conflicting, and missing routes.
- [ ] Router API documentation.

## Milestone 6 - Static File Server

**Goal:** Serve static files safely from a configured document root.

**Expected outcome:** Static routes return requested files with suitable content types while preventing access outside the document root.

**Deliverables:**

- [ ] Static-file route handler.
- [ ] Configurable document root.
- [ ] Common MIME type mapping.
- [ ] Path normalization and path-traversal protection.
- [ ] 404 and unreadable-file handling.
- [ ] Integration tests using fixture files.

## Milestone 7 - Thread Pool

**Goal:** Process request work concurrently with a bounded worker pool.

**Expected outcome:** Multiple client requests can be handled without a single request blocking server-wide progress.

**Deliverables:**

- [ ] Configurable `ThreadPool` with a task queue.
- [ ] Safe task submission and worker lifecycle management.
- [ ] Graceful shutdown and pending-task policy.
- [ ] Thread-safety tests and basic concurrency validation.
- [ ] TCP Server integration.

## Milestone 8 - Logger

**Goal:** Provide useful, configurable operational logging.

**Expected outcome:** Developers can trace startup, shutdown, request processing, failures, and health changes without exposing sensitive data.

**Deliverables:**

- [ ] Log levels: trace, debug, info, warning, error, and critical.
- [ ] Console output and optional file output.
- [ ] Configurable log level and destination.
- [ ] Request-context logging for method, path, status, and duration.
- [ ] Logging guidelines and tests for configuration behavior.

## Milestone 9 - Reverse Proxy

**Goal:** Forward configured requests to upstream HTTP backend services.

**Expected outcome:** Proxy routes preserve relevant request data, return upstream responses, and produce controlled proxy errors when upstream communication fails.

**Deliverables:**

- [ ] Upstream connection and request-forwarding component.
- [ ] Forwarding of method, path, relevant headers, and body.
- [ ] Configurable upstream connection and response timeouts.
- [ ] 502 Bad Gateway error handling.
- [ ] Integration tests with a local mock upstream server.

## Milestone 10 - Load Balancer

**Goal:** Select upstream backend instances using round-robin scheduling.

**Expected outcome:** Proxy traffic is distributed among healthy configured backend instances.

**Deliverables:**

- [ ] Backend pool configuration model.
- [ ] Thread-safe round-robin selection strategy.
- [ ] No-healthy-backend result handling.
- [ ] 503 Service Unavailable response path.
- [ ] Unit tests for selection order and unavailable backends.

## Milestone 11 - Health Checker

**Goal:** Track the availability of upstream backend services.

**Expected outcome:** Unhealthy backends are excluded from load-balancer selection and are restored when they recover.

**Deliverables:**

- [ ] Periodic health-check scheduler.
- [ ] Configurable health-check path, interval, and timeout.
- [ ] Thread-safe backend health-state store.
- [ ] Health-state transition logging.
- [ ] Integration tests for healthy, failing, and recovered backends.

## Milestone 12 - Metrics

**Goal:** Expose essential runtime operational metrics.

**Expected outcome:** A monitoring client can retrieve current EdgeX request, latency, error, connection, and backend-health data from a metrics endpoint.

**Deliverables:**

- [ ] Thread-safe counters and latency measurements.
- [ ] Metrics for total requests, errors, active connections, and backend health.
- [ ] Configurable `GET /metrics` endpoint.
- [ ] Metrics response format and endpoint documentation.
- [ ] Unit and integration tests for metric updates and endpoint output.

## Milestone 13 - Benchmarking

**Goal:** Measure throughput and latency of core EdgeX operations.

**Expected outcome:** The project can produce repeatable baseline measurements for server and component performance.

**Deliverables:**

- [ ] Benchmarks for HTTP parsing, routing, thread-pool task handling, and server throughput.
- [ ] Configurable request count and client concurrency.
- [ ] Benchmark report containing RPS, success/failure counts, elapsed time, and latency values.
- [ ] Benchmark execution instructions.
- [ ] Baseline benchmark results for the v1.0 release environment.

## Milestone 14 - Documentation and Release

**Goal:** Prepare EdgeX v1.0 for a reproducible, documented release.

**Expected outcome:** A new contributor can build, run, configure, test, benchmark, and understand EdgeX from repository documentation.

**Deliverables:**

- [ ] Updated README with build, run, test, and configuration instructions.
- [ ] Completed SRS, architecture, component, sequence, and DFD documentation.
- [ ] Updated coding standards and Git workflow documents.
- [ ] Release notes and known-limitations list.
- [ ] Final build and test validation.
- [ ] Annotated Git tag: `v1.0.0`.

## Sprint Completion Criteria

- [ ] All fourteen milestones are complete or explicitly documented as deferred.
- [ ] EdgeX builds using the documented CMake workflow.
- [ ] Relevant unit and integration tests pass.
- [ ] The example configuration runs locally.
- [ ] Benchmark baseline results are recorded.
- [ ] Documentation reflects the implemented behavior and v1.0 limitations.
