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
./build-ucrt64/benchmarks/edgex_benchmark
```

On Windows PowerShell:

```powershell
.\build-ucrt64\benchmarks\edgex_benchmark.exe
```

The default workload is 10,000 operations with concurrency 4. Change the workload with:

```text
--component all|parser|router|threadpool|server
--requests N
--concurrency N
--port P
```

For example:

```powershell
.\build-ucrt64\benchmarks\edgex_benchmark.exe --component server --requests 10000 --concurrency 8 --port 19090
```

## Coverage

- **HTTP parser**: parses a representative HTTP/1.1 request repeatedly.
- **Router**: measures exact route matching and handler dispatch.
- **Thread pool**: measures task submission-to-execution latency and completion throughput.
- **TCP server**: runs local HTTP request/response traffic through `TCPServer` and its worker pool.

Each benchmark reports request/operation count, success count, failure count, elapsed time, throughput, and p50/p95/p99 latency where applicable. Request count and concurrency are configurable.

## Baseline recording

Run the suite on the target development machine and preserve the complete output as the v1.0 baseline. Hardware, compiler, build type, request count, concurrency, and operating system materially affect the numbers, so benchmark results are not hard-coded into the source.

Recommended baseline command:

```powershell
.\build-ucrt64\benchmarks\edgex_benchmark.exe --requests 10000 --concurrency 4 --port 19090
```

Record the resulting output in the release documentation under the v1.0 benchmark baseline section.
