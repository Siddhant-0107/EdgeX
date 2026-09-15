# EdgeX Benchmark Suite

The benchmark suite is disabled by default so normal builds and CTest remain focused on correctness. Enable it with `-DEDGEX_BUILD_BENCHMARKS=ON`.

## Build

```bash
cmake -S . -B build-ucrt64 -DEDGEX_BUILD_BENCHMARKS=ON
cmake --build build-ucrt64 --target edgex_benchmark
```

## Run

Run the full suite:

```bash
./build-ucrt64/benchmarks/edgex_benchmark.exe --requests 10000 --concurrency 4 --port 19090
```

On Windows, the benchmark should be launched from the **MSYS2 UCRT64 shell** when the executable depends on the UCRT64 runtime environment. The same shell is used for the documented UCRT64 build workflow.

The default workload is 10,000 operations with concurrency 4. Change the workload with:

```text
--component all|parser|router|threadpool|server
--requests N
--concurrency N
--port P
```

For example:

```bash
./build-ucrt64/benchmarks/edgex_benchmark.exe --component server --requests 10000 --concurrency 8 --port 19090
```

## Coverage

- **HTTP parser**: parses a representative HTTP/1.1 request repeatedly.
- **Router**: measures exact route matching and handler dispatch.
- **Thread pool**: measures task submission-to-execution latency and completion throughput.
- **TCP server**: runs local HTTP request/response traffic through `TCPServer` and its worker pool.

Each benchmark reports operation/request count, success count, failure count, elapsed time, throughput, and p50/p95/p99 latency where applicable. Request count and concurrency are configurable.

## v1.0 Baseline

Recorded in the project development environment with **10,000 requests and concurrency 4**:

| Benchmark | Throughput | p50 | p95 | p99 |
|---|---:|---:|---:|---:|
| HTTP parser | 201,242 ops/s | 3.90 us | 5.80 us | 6.00 us |
| Router matching | 1,876,736 ops/s | 0.40 us | 0.50 us | 0.50 us |
| Thread-pool task execution | 241,944 ops/s | 9.70 us | 855.20 us | 967.50 us |
| TCP server request throughput | 2,573 req/s | 508.90 us | 10,706.60 us | 21,762.85 us |

All 10,000 operations/requests succeeded in the recorded run. These figures are environment-dependent and are intended as a repeatable reference for future performance comparisons.

## Methodology Notes

- Timing uses a monotonic `steady_clock`.
- Component benchmarks measure the operation under test directly; the TCP server benchmark measures local client/server request handling.
- Thread-pool latency represents submission-to-worker-execution delay rather than application work duration.
- The TCP server benchmark uses a local loopback connection, so it measures EdgeX processing plus local socket overhead rather than network performance.
- Run repeated trials when making performance claims. Avoid comparing numbers across different machines or build configurations without recording the environment.
