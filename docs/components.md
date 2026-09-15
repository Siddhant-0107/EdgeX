# EdgeX Component Guide

## 1. Socket Layer

The socket layer owns platform-specific TCP socket details behind a C++ RAII interface. It provides creation, bind/listen, accept, close, state inspection, and native-handle access where a lower layer needs it.

The Windows implementation uses `SOCKET`; the abstraction avoids exposing platform-specific handle ownership to higher-level modules.

## 2. TCP Server

`TCPServer` owns the listening socket and server lifecycle. Its configuration includes bind address, port, backlog, and worker count. `start()` establishes the listener; `accept_client()` accepts a connection; `submit_client_task()` dispatches client work to the server's thread pool; `stop()` closes the listener and drains already-submitted worker tasks.

This separates connection acceptance from request work and gives the server a bounded worker model.

## 3. HTTP Parser

The parser incrementally consumes HTTP/1.1 bytes and produces an `HttpRequest` containing method, target, version, headers, and body information. It distinguishes incomplete input from a complete request and malformed input.

Parser correctness is covered by unit tests for complete, incomplete, headerless, and malformed requests.

## 4. HTTP Response Builder

The response model and builder construct status lines, headers, and bodies and serialize them into HTTP/1.1 bytes. `Content-Length` is generated during serialization when appropriate rather than requiring callers to duplicate it manually.

## 5. Router

The router registers handlers by HTTP method and exact request path. Matching is intentionally simple and deterministic: the first matching route is selected and an unmatched request receives a controlled 404 response.

The current v1.0 router favors clarity and testability over trie-based or pattern-heavy routing.

## 6. Static File Server

The static file server maps GET request targets into a configured document root. It strips query strings, canonicalizes paths, verifies that the resolved path remains inside the document root, requires a regular file, reads binary content, and selects a MIME type from the extension.

Path traversal outside the document root is rejected rather than normalized into an accessible location.

## 7. Thread Pool

`ThreadPool` owns a configurable number of worker threads and a synchronized task queue. `submit()` adds work; workers wait on a condition variable; `shutdown()` prevents new submissions and drains tasks already accepted by the pool.

The pool rejects zero workers and empty tasks. Tasks submitted after shutdown are rejected. Exceptions thrown by individual tasks are isolated so they do not terminate worker threads or the application.

## 8. Logger

The logger supports trace, debug, info, warning, error, and critical levels. A configurable minimum level filters lower-priority events. Output can go to the console, a file, or both. Writes are protected by a mutex and each emitted record is flushed.

The logger uses a pimpl to keep implementation details out of the public header.

## 9. TCP Client

`TCPClient` provides outbound IPv4 TCP connections with configurable connect and I/O timeouts. It supports connecting, sending complete strings/byte sequences, receiving chunks, checking connection state, and closing the socket.

It is used by the reverse proxy, health checker, and local benchmark infrastructure.

## 10. Reverse Proxy

The reverse proxy forwards an HTTP request to a selected upstream. It preserves the relevant method, target, headers, and body while filtering hop-by-hop headers. A `Host` header is supplied when the request does not already contain one.

Upstream communication failures are converted into a controlled 502 response. Proxy operations can be associated with the EdgeX logger for request, success, failure, and duration events.

## 11. Round-Robin Load Balancer

The load balancer owns a configured vector of backend instances and maintains the next selection position under a mutex. Each selection searches from that position for the next healthy backend, advances the position after selection, and returns no backend when every instance is unhealthy.

Health updates and snapshots are also synchronized, allowing selection from concurrent request-processing threads.

## 12. Health Checker

The health checker periodically snapshots the configured backend list and probes each backend's configured endpoint with a TCP/HTTP request. A successful 2xx HTTP response marks the backend healthy; connection, timeout, or invalid-response failures mark it unhealthy.

Checks run without holding the load-balancer mutex during network I/O. Health-state transitions are logged. `start()` performs an immediate check and then repeats at the configured interval; `stop()` wakes and joins the worker thread.

## 13. Metrics

`Metrics` maintains atomic request and error counters, total and maximum request latency, and active connection count. It can register a configurable HTTP endpoint, defaulting to `/metrics`, on a router.

When a load balancer is supplied, the rendered metrics also expose backend health state. Atomic counters make updates safe across concurrent request-processing threads.

## 14. Benchmark Suite

The benchmark executable measures four areas:

- HTTP parser operations.
- Router matching and handler execution.
- Thread-pool task scheduling/execution latency.
- End-to-end local TCP server request throughput.

Request count, concurrency, benchmark component, and server port are configurable. Results report operations, success/failure counts, elapsed time, throughput, and p50/p95/p99 latency where applicable.

Benchmarks are deliberately separate from CTest because performance measurements are not binary correctness tests.

## Request Lifecycle

For a normal local request:

1. The TCP server accepts a client socket.
2. Client work is submitted to the thread pool.
3. Raw bytes are parsed into an HTTP request.
4. The router selects a handler.
5. A static-file handler or reverse proxy produces a response.
6. The response is serialized and sent to the client.
7. Metrics and logging can record operational information.

For a proxy request, the reverse proxy asks the load balancer for a healthy backend before creating the outbound TCP connection. If there is no healthy backend, the request receives 503 Service Unavailable.

## Testing Strategy

Each major module has focused unit tests, while TCP server lifecycle and client/server interactions use integration-style tests. The v1.0 suite currently passes all 12 registered tests in the development environment.
