# EdgeX Interview Preparation Guide

This document is a practical interview guide for explaining, defending, and discussing EdgeX v1.0.0. It is based on the implemented project scope and architecture documented in the repository.

## 1. 30-Second Project Introduction

> EdgeX is a modular backend infrastructure platform built from scratch in Modern C++17. I implemented the networking stack around TCP sockets and HTTP/1.1, including an incremental HTTP parser, response builder, router, static file server, thread pool, logging, TCP client, reverse proxy, round-robin load balancer, health checker, metrics, and a benchmark suite. The project is cross-platform, uses CMake, has 12 unit/integration tests, and was released as v1.0.0.

Do not call EdgeX a full production web server. The README explicitly describes it as a production-inspired educational infrastructure project and states that it is not intended for internet-facing production deployment.

## 2. 2-Minute Deep-Dive Answer

A request enters EdgeX through the TCP server. The server owns the listening socket and accepts client connections. Client work is handed to the thread pool rather than creating an unbounded thread per connection.

The HTTP parser incrementally validates the incoming HTTP/1.1 request. Once parsed, the router performs method-and-path matching. A request can be handled locally, for example by the static file server, or forwarded through the reverse proxy.

For proxied requests, the round-robin load balancer selects a healthy configured backend. The health checker periodically probes backends and updates their health state. The reverse proxy then uses the selected backend to perform outbound TCP communication and HTTP forwarding.

Logging and metrics provide operational visibility. The benchmark suite separately measures parser, router, thread-pool, and TCP-server performance. Correctness is validated with 12 unit/integration tests.

## 3. Architecture You Must Be Able to Draw

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

Health Checker ---> backend health state
Logger -----------> console/file
Metrics ----------> request/error/latency/connection visibility
```

Core responsibility split:
- TCP server: listening, accepting, lifecycle.
- Thread pool: executing client work concurrently.
- HTTP parser: request parsing and validation.
- Router: method + path dispatch.
- Static file server: safe document-root file serving.
- TCP client: outbound TCP connection and I/O.
- Reverse proxy: HTTP forwarding.
- Load balancer: healthy backend selection.
- Health checker: backend health transitions.
- Logger: structured level-based logging.
- Metrics: thread-safe runtime measurements.

## 4. Networking Fundamentals

### Q1. What is a socket?

A socket is an OS-managed communication endpoint. EdgeX wraps platform TCP sockets in a RAII `Socket` abstraction so ownership and cleanup are tied to object lifetime.

### Q2. What is the TCP socket lifecycle?

For a server:

```text
socket -> bind -> listen -> accept -> recv/send -> close
```

For a client:

```text
socket -> connect -> send/recv -> close
```

### Q3. Why does EdgeX need a TCP client in addition to a TCP server?

The server accepts inbound client connections. The reverse proxy needs outbound connections to upstream backends, so EdgeX needs a separate client-side abstraction.

### Q4. TCP vs UDP?

TCP is connection-oriented and provides ordered, reliable byte-stream delivery. UDP is connectionless and does not provide TCP-style reliability or ordering. HTTP/1.1 in EdgeX uses TCP.

### Q5. What is the TCP three-way handshake?

```text
Client -> SYN
Server -> SYN-ACK
Client -> ACK
```

It establishes the connection and synchronizes sequence numbers.

### Q6. What is a file descriptor / socket handle?

On POSIX, sockets are represented by file descriptors. Windows uses `SOCKET`, which is not safely interchangeable with a 32-bit `int`. EdgeX accounts for this platform difference in its socket abstraction.

## 5. HTTP Fundamentals

### Q7. What does an HTTP/1.1 request look like?

```text
METHOD /target HTTP/1.1\r\n
Header: value\r\n
Another: value\r\n
\r\n
optional body
```

### Q8. Why is HTTP called a protocol over TCP?

HTTP defines application-level semantics such as methods, headers, status codes, and bodies. TCP provides the reliable ordered byte stream underneath it.

### Q9. What does incremental parsing mean?

The parser does not assume the complete request arrives in one `recv`. It can consume input progressively and determine whether the request is incomplete, complete, or invalid.

### Q10. Why can't one `recv()` be assumed to contain one HTTP request?

TCP is a byte stream, not a message protocol. A request can be split across multiple receives, or multiple application-level messages can be present in one receive.

## 6. Thread Pool

### Q11. Why use a thread pool?

Creating a new thread for every request can create excessive creation/destruction overhead and unbounded concurrency. A thread pool keeps a fixed number of workers and queues tasks.

### Q12. How does the EdgeX thread pool work?

Conceptually:

```text
submit(task)
     |
     v
 synchronized queue
     |
     v
worker threads -> execute tasks
```

A mutex protects shared state and a condition variable wakes workers when work arrives or shutdown begins.

### Q13. What happens during shutdown?

EdgeX drains tasks that were already submitted before shutdown, then workers exit. Repeated shutdown is safe, and submitting after shutdown is rejected.

### Q14. How are task exceptions handled?

Worker execution isolates task exceptions so an exception from one task does not terminate the worker thread or the entire process.

### Q15. What are the trade-offs of a thread pool?

Too few workers can limit throughput; too many can increase contention, context switching, memory usage, and scheduling overhead. The correct worker count depends on workload and hardware.

## 7. HTTP Router

### Q16. How does EdgeX routing work?

Routes match the HTTP method and path. The implementation uses a linear route collection and controlled 404 behavior for unmatched routes.

### Q17. Why is a linear router acceptable here?

The project intentionally keeps v1.0 focused and simple. A linear structure is easy to reason about and test. For a much larger routing table, a trie, radix tree, or other indexing strategy could reduce lookup cost.

## 8. Static File Server and Security

### Q18. What security problem does a static file server have to prevent?

Path traversal. A client must not be able to escape the configured document root using paths such as `../`.

### Q19. How does EdgeX approach traversal protection?

The implementation resolves/canonicalizes the document root and requested path and checks that the resulting requested path remains within the root before serving the file.

### Q20. Why is canonicalization important?

Checking raw strings alone can miss equivalent filesystem paths. Canonicalized filesystem paths provide a stronger basis for containment checks.

## 9. Reverse Proxy

### Q21. What does a reverse proxy do?

A reverse proxy accepts a client request and forwards it to an upstream backend, then returns the upstream response to the client.

### Q22. Why does EdgeX have both a TCP server and TCP client?

The TCP server handles inbound connections; the TCP client lets the proxy create outbound connections to upstream services.

### Q23. What is a 502 response used for?

In EdgeX, a failed upstream forwarding operation is converted into a controlled HTTP 502 Bad Gateway response.

### Q24. What are hop-by-hop headers?

They describe a connection rather than an end-to-end message path and should not blindly be forwarded through a proxy. EdgeX removes relevant hop-by-hop headers while forwarding requests/responses.

### Q25. Why does the proxy use `Connection: close`?

The v1 implementation intentionally uses close-delimited connection behavior to keep connection management simple. This is a deliberate limitation rather than a claim of full production proxy behavior.

### Q26. What are known reverse-proxy limitations?

The v1.0 implementation has intentionally limited response handling and connection management compared with mature proxy stacks. Advanced transfer encodings are outside the v1.0 scope.

## 10. Load Balancing

### Q27. What is round-robin load balancing?

Given healthy backends:

```text
A -> B -> C -> A -> B -> C -> ...
```

The next selection advances through the backend list while skipping unhealthy instances.

### Q28. Why does the load balancer need synchronization?

Multiple request-handling threads can select backends concurrently. The selection index and health state are shared mutable state, so access is synchronized.

### Q29. What happens when every backend is unhealthy?

The load balancer returns no backend, and the reverse proxy returns HTTP 503 Service Unavailable.

### Q30. Why round-robin instead of least-connections or weighted routing?

Round-robin is deterministic, simple, and sufficient for the v1.0 scope. Additional strategies are future scope.

## 11. Health Checking

### Q31. Why separate health checking from load balancing?

The load balancer should answer “which healthy backend should I select?” while the health checker answers “which backends are currently healthy?” Separating these responsibilities keeps each component focused.

### Q32. How does the health checker work?

It periodically probes configured backends through the TCP client, evaluates the HTTP response, and updates the load balancer's health state.

### Q33. Why should the health checker avoid holding the load-balancer lock during network I/O?

Network operations can block. Holding a shared lock during a potentially slow network operation would unnecessarily block request threads that need to select or update backend state.

## 12. Logger

### Q34. What log levels are implemented?

`trace`, `debug`, `info`, `warning`, `error`, and `critical`.

### Q35. How is logging made thread-safe?

Logger output is synchronized so concurrent worker threads do not corrupt shared output streams.

### Q36. Why flush log output?

Flushing makes messages visible promptly and reduces the chance that important diagnostic information remains buffered during failures.

## 13. Metrics

### Q37. Why do metrics need synchronization?

Requests are handled concurrently. Counters and latency-related state can be updated from multiple threads, so updates need thread-safe coordination.

### Q38. What does EdgeX expose through metrics?

The v1.0 metrics component tracks request/error/latency/connection information and renders metrics through the `/metrics` endpoint, along with backend health visibility.

## 14. CMake and Project Engineering

### Q39. Why CMake?

CMake provides a portable build configuration across supported platforms and compilers and lets the project keep library, test, and benchmark targets organized.

### Q40. Why C++17?

C++17 provides useful modern language and library features while remaining widely supported by current GCC/Clang/MSVC toolchains.

### Q41. Why use RAII?

RAII ties resource lifetime to object lifetime. This is particularly useful for sockets, files, threads, and other resources that must be reliably released.

## 15. Testing

### Q42. What does the test suite contain?

The v1.0 suite contains 12 unit/integration tests covering HTTP parsing, response construction, routing, static-file security, thread-pool lifecycle/concurrency, logging, TCP client behavior, reverse proxying, load balancing, health checking, metrics, and TCP server lifecycle/integration behavior.

### Q43. Why separate benchmarks from CTest?

Tests answer whether behavior is correct. Benchmarks measure performance and are environment-dependent. EdgeX intentionally keeps performance measurement separate from pass/fail correctness tests.

### Q44. How did you validate the release?

The final main-branch validation completed with:

```text
100% tests passed out of 12
Total Test time (real) = 2.10 sec
```

## 16. Benchmark Questions

### Q45. What did you benchmark?

The suite measures parser, router, thread-pool, and TCP-server performance.

### Q46. What makes benchmark results hard to generalize?

CPU, OS, compiler, optimization settings, background load, network stack behavior, and machine configuration all affect results. EdgeX therefore treats the recorded numbers as a reproducible reference rather than universal performance claims.

### Q47. What did the benchmark teach you?

The in-process parser/router paths are much cheaper than a complete TCP request path. Network operations, scheduling, synchronization, and connection setup introduce substantially more latency than simple parsing or route matching.

## 17. Cross-Platform Questions

### Q48. What Windows networking issue did you have to account for?

Windows uses the `SOCKET` type for socket handles, whereas POSIX uses integer file descriptors. Treating a Windows socket as an `int` can truncate the handle. EdgeX keeps the platform-specific handle representation safe behind its socket abstraction.

### Q49. What is the value of cross-platform abstraction here?

Higher-level components such as the HTTP layer and proxy should not need to know whether the underlying platform represents a socket as a POSIX descriptor or a Windows `SOCKET`.

## 18. Design Trade-offs

Be prepared to say these explicitly:

| Decision | Reason | Trade-off |
|---|---|---|
| Thread pool | Bounded worker concurrency | Queueing and contention |
| Linear router | Simple and testable v1 design | Lookup scales linearly |
| Round-robin | Deterministic/simple | Does not account for backend capacity |
| Periodic active health checks | Simple backend health model | Health can lag between checks |
| TCP-based proxy | Direct control of networking | More protocol work than using a mature proxy library |
| Pimpl in some components | Hide implementation details | Extra indirection/heap allocation |
| Plain HTTP/1.1 | Focused v1 scope | No TLS/HTTP2/HTTP3 |
| Separate benchmark executable | Keeps correctness tests deterministic | Benchmark must be run explicitly |

## 19. Questions About Limitations

### Q50. Why isn't EdgeX production-ready?

Because v1.0 deliberately excludes TLS, HTTP/2, HTTP/3, WebSockets, authentication, authorization, rate limiting, advanced proxy transfer encodings, dynamic service discovery, event-driven I/O, and distributed-system features. The README explicitly defines these as scope boundaries.

### Q51. If you had another month, what would you build?

A strong answer:
1. TLS/HTTPS.
2. Better HTTP connection management and transfer-encoding support.
3. Event-driven I/O using platform-appropriate mechanisms.
4. More load-balancing strategies.
5. Richer metrics/tracing.
6. Authentication and rate limiting.
7. CI and containerization.
8. More extensive integration and stress testing.

## 20. Hard Interview Questions

### “Why didn't you use Boost.Asio?”

Because the educational goal was to understand the networking primitives rather than hide them behind a mature networking abstraction. The project was intentionally built around the socket layer and progressively composed higher-level infrastructure on top.

### “Why not use epoll?”

The v1.0 scope intentionally uses a thread-pool model rather than event-driven I/O. `epoll`/`kqueue` are explicitly future scope. The important engineering point is to recognize the scalability trade-off rather than claim the current model is universally superior.

### “Is your server really production-ready?”

No. It is production-inspired infrastructure with a completed v1.0 implementation, tests, benchmarks, and documentation. The project explicitly says it is not intended for internet-facing production deployment.

### “What was the hardest part?”

A good answer should be specific rather than generic. Focus on the interaction between platform-specific socket ownership, concurrency, proxying, health state, and clean shutdown. Explain the bug, root cause, fix, and validation rather than simply saying “C++ was difficult.”

### “How would you improve scalability?”

Move from blocking/thread-pool-centric I/O toward event-driven I/O, introduce better connection management and keep-alive handling, optimize routing structures where needed, and add load testing and profiling before changing architecture.

## 21. Debugging Story You Should Know

One important engineering lesson from EdgeX was that a networking project can fail because of resource ownership or build integration even when the high-level code looks correct.

Be ready to explain:
- platform-specific socket handle representation;
- RAII ownership and cleanup;
- CMake target/source integration;
- stale build directories and the need to reconfigure after target changes;
- why a full rebuild and complete CTest run matter after integration changes;
- why concurrency bugs require deterministic tests and careful synchronization.

## 22. Interview Red Flags — Do Not Say These

Avoid:
- “It is a production-ready replacement for Nginx.”
- “It can handle unlimited concurrent users.”
- “The benchmark proves EdgeX is faster than other servers.”
- “TCP guarantees that one `recv()` equals one request.”
- “The thread pool makes the application automatically scalable.”
- “Round-robin guarantees equal real-world load.”

Instead, explain the actual scope and trade-offs.

## 23. Final Revision Checklist

Before an interview, you should be able to explain without notes:

- [ ] TCP socket lifecycle.
- [ ] Windows `SOCKET` vs POSIX file descriptor.
- [ ] TCP three-way handshake.
- [ ] Why TCP is a byte stream.
- [ ] HTTP/1.1 request structure.
- [ ] Incremental parsing.
- [ ] Router lookup and 404 behavior.
- [ ] Thread-pool queue/worker/condition-variable design.
- [ ] Graceful shutdown.
- [ ] Task exception isolation.
- [ ] Static-file path traversal protection.
- [ ] Reverse-proxy request/response flow.
- [ ] Hop-by-hop headers.
- [ ] 502 vs 503.
- [ ] Round-robin selection and synchronization.
- [ ] Active health checking.
- [ ] Metrics synchronization.
- [ ] Logger synchronization and levels.
- [ ] CMake target organization.
- [ ] Unit vs integration tests.
- [ ] Benchmark methodology and limitations.
- [ ] v1.0 scope and known limitations.
- [ ] At least two concrete engineering/debugging stories.

## 24. One-Line Summary to Remember

> **EdgeX is a C++17 networking infrastructure project where I built the path from raw TCP sockets to an HTTP server and then composed concurrency, routing, proxying, load balancing, health checking, observability, testing, and benchmarking into a documented v1.0 system.**
