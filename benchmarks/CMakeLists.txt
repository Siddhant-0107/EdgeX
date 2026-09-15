add_executable(edgex_benchmark
    benchmark.cpp
)

target_link_libraries(edgex_benchmark
    PRIVATE
        EdgeX::core
)

edgex_enable_warnings(edgex_benchmark)

# Benchmarks are intentionally not registered as CTest tests because they
# measure elapsed time and throughput rather than pass/fail correctness.
