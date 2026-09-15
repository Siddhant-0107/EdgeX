# Software Requirements Specification — EdgeX v1.0

## 1. Introduction

EdgeX is a modular backend infrastructure platform implemented in Modern C++17. Version 1.0 is an educational, production-inspired implementation of TCP networking, HTTP/1.1 processing, routing, concurrency, proxying, load balancing, health checking, logging, metrics, and benchmarking.

EdgeX is designed for learning, experimentation, and portfolio development. It is not intended to replace mature internet-facing server infrastructure.

## 2. v1.0 Scope

The implemented v1.0 system includes:

- RAII TCP socket abstraction and TCP server lifecycle.
- Outbound TCP client with connection and I/O timeouts.
- Incremental HTTP/1.1 request parsing.
- HTTP response construction and serialization.
- Exact method/path routing.
- Static file serving with document-root traversal protection.
- Configurable thread-pool execution and graceful shutdown.
- Thread-safe multi-level logging to console and/or file.
- Reverse proxying to upstream HTTP services.
- Thread-safe round-robin load balancing across healthy backends.
- Periodic backend health checking.
- Thread-safe runtime metrics and `/metrics` endpoint rendering.
- Repeatable component and TCP-server benchmarks.
- Unit and integration test coverage for the implemented modules.

### Explicit v1.0 boundaries

The following are outside the implemented v1.0 scope:

- HTTPS/TLS.
- HTTP/2 and HTTP/3.
- WebSockets.
- Authentication, authorization, and rate limiting.
- Database and message-queue integration.
- Dynamic service discovery.
- Distributed tracing and distributed-system coordination.
- Event-driven I/O redesign using `epoll`, `kqueue`, or IOCP.
- Production-grade proxy features such as advanced transfer-encoding handling and comprehensive connection pooling.

## 3. System Overview

The normal request flow is:

`Client -> TCP Server -> Thread Pool -> HTTP Parser -> Router -> Static File Server or Reverse Proxy -> HTTP Response -> Client`

For a proxy route, the reverse proxy obtains a backend endpoint from the round-robin load balancer. The load balancer excludes unhealthy backends; if no healthy backend exists, the proxy returns 503 Service Unavailable.

The health checker periodically probes configured backends and updates their health state. Logger and Metrics provide cross-cutting operational visibility.

## 4. Functional Requirements

### 4.1 Networking

- **FR-01:** The TCP server shall bind to a configured address and port.
- **FR-02:** The server shall listen for and accept TCP client connections.
- **FR-03:** Server client work shall be dispatchable through a configurable thread pool.
- **FR-04:** The outbound TCP client shall support connection and I/O timeouts.
- **FR-05:** Socket ownership shall use RAII semantics and prevent accidental copying of owned handles.

### 4.2 HTTP Request Parsing

- **FR-06:** The parser shall consume HTTP/1.1 request bytes incrementally.
- **FR-07:** The parser shall produce a structured request containing method, target, version, headers, and body information as supported by the implementation.
- **FR-08:** The parser shall distinguish complete, incomplete, and malformed input.

### 4.3 HTTP Responses

- **FR-09:** The response model shall represent status, headers, and body.
- **FR-10:** Response serialization shall produce an HTTP/1.1 response.
- **FR-11:** Content length shall be generated during serialization when appropriate.

### 4.4 Routing

- **FR-12:** Routes shall be registered by HTTP method and exact path.
- **FR-13:** A matching route shall invoke its associated handler.
- **FR-14:** An unmatched request shall produce a controlled 404 response.

### 4.5 Static Files

- **FR-15:** The static file server shall serve files below its configured document root.
- **FR-16:** Query strings shall not change the selected filesystem path.
- **FR-17:** Resolved paths shall remain within the document root.
- **FR-18:** Common file extensions shall receive appropriate MIME types.
- **FR-19:** Missing or invalid files shall return controlled HTTP errors.

### 4.6 Concurrency

- **FR-20:** The thread pool shall support a configurable worker count and synchronized task queue.
- **FR-21:** Submission and worker execution shall be safe under concurrent use.
- **FR-22:** Shutdown shall reject new tasks and drain tasks already accepted by the pool.
- **FR-23:** An exception in an individual task shall not terminate the worker pool.

### 4.7 Logging

- **FR-24:** The logger shall support trace, debug, info, warning, error, and critical levels.
- **FR-25:** A configurable minimum level shall filter lower-priority messages.
- **FR-26:** Logging shall support console and optional file output.
- **FR-27:** Concurrent log writes shall be synchronized.

### 4.8 Reverse Proxy

- **FR-28:** The reverse proxy shall forward an HTTP request to a selected upstream backend.
- **FR-29:** Relevant method, target, headers, and body data shall be forwarded.
- **FR-30:** Hop-by-hop headers shall not be blindly propagated.
- **FR-31:** Upstream connection/I/O timeouts shall be configurable.
- **FR-32:** Upstream communication failures shall produce a controlled 502 response.

### 4.9 Load Balancing

- **FR-33:** Configured backends shall be selected using round-robin scheduling.
- **FR-34:** Unhealthy backends shall be excluded from selection.
- **FR-35:** Backend selection and health updates shall be thread-safe.
- **FR-36:** No healthy backend shall result in a 503 response from the proxy path.

### 4.10 Health Checking

- **FR-37:** The health checker shall periodically probe configured backends.
- **FR-38:** Health-check endpoint, interval, and timeout shall be configurable.
- **FR-39:** Successful 2xx health responses shall mark a backend healthy; failed/unusable checks shall mark it unhealthy.
- **FR-40:** Health-state transitions shall be logged.

### 4.11 Metrics

- **FR-41:** Metrics shall expose a configurable endpoint, defaulting to `/metrics`.
- **FR-42:** Metrics shall include request count, error count, latency totals/maxima, and active connections.
- **FR-43:** When a load balancer is supplied, backend health state shall be renderable with the metrics response.
- **FR-44:** Metric updates shall be safe during concurrent request processing.

### 4.12 Benchmarking

- **FR-45:** The benchmark suite shall measure HTTP parsing, router matching, thread-pool execution, and TCP-server throughput.
- **FR-46:** Request count and concurrency shall be configurable.
- **FR-47:** Benchmark output shall report operations, success/failure counts, elapsed time, throughput, and p50/p95/p99 latency where applicable.

## 5. Non-Functional Requirements

### Performance

- The system shall support concurrent task execution through the worker pool.
- Core operations shall be measurable using the benchmark suite.
- Latency and throughput measurements shall use a monotonic clock.

### Reliability

- Malformed input shall be handled without process termination.
- Backend failures shall be represented as controlled proxy errors and health-state changes.
- Shared mutable state shall use appropriate synchronization.

### Maintainability

- Networking, HTTP, routing, static serving, concurrency, proxying, load balancing, health checking, logging, and metrics shall remain separate modules.
- Resource ownership shall use RAII.
- Public interfaces shall be documented through headers and project documentation.
- Behavioral changes shall be backed by tests where practical.

### Portability

- The project shall use C++17.
- Platform-specific socket details shall remain isolated in the networking layer.
- The documented CMake workflow shall support the project development environments.

### Security

- Static-file path resolution shall prevent traversal outside the configured document root.
- Malformed requests shall be validated before normal request handling.
- EdgeX v1.0 shall document that HTTP is unencrypted and therefore not suitable for untrusted internet-facing deployment.

## 6. Validation

The v1.0 development environment currently passes all **12 registered CTest tests**. The benchmark suite also completed a 10,000-request run at concurrency 4 with 100% successful operations.

## 7. Assumptions

- EdgeX is operated in a controlled development or testing environment.
- Upstream services are configured and reachable where proxy/health-check functionality is exercised.
- Benchmark results are interpreted relative to the machine, compiler, OS, and build configuration used to produce them.

## 8. Traceability

The implementation is organized around the v1.0 roadmap milestones. Each major subsystem has a public interface, implementation, and focused tests; the documentation set provides the corresponding architecture, component, sequence, data-flow, coding, and development workflow views.
