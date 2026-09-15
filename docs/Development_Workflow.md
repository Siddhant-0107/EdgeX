# EdgeX Development Workflow

## Branching

Use focused branches for each change, for example:

- `feature/http-parser`
- `feature/reverse-proxy`
- `feature/benchmarking`
- `fix/socket-lifecycle`

Do not mix unrelated features or formatting-only changes into a feature branch.

## Development Cycle

1. Define or review the GitHub issue and acceptance criteria.
2. Create a focused branch from the current development baseline.
3. Implement the smallest coherent change.
4. Add or update focused unit/integration tests.
5. Build with the documented CMake workflow.
6. Run the full CTest suite before considering the change complete.
7. For performance-sensitive changes, run the benchmark suite separately.
8. Update relevant technical documentation when behavior or architecture changes.
9. Commit with a clear Conventional Commit-style message.
10. Review the diff for accidental files, stale documentation, or generated artifacts.

## Commit Style

Use prefixes such as:

- `feat:` — new functionality
- `fix:` — behavioral correction
- `test:` — test-only change
- `docs:` — documentation
- `build:` — build/tooling changes
- `refactor:` — behavior-preserving restructuring

Keep commit messages specific, for example:

```text
feat(proxy): add round-robin healthy backend selection
fix(net): preserve Windows SOCKET handle width
docs: align architecture with v1.0 modules
```

## Validation Commands

Normal build:

```bash
cmake -S . -B build
cmake --build build
```

Full test suite:

```bash
ctest --test-dir build --output-on-failure
```

Benchmark build and run:

```bash
cmake -S . -B build-ucrt64 -DEDGEX_BUILD_BENCHMARKS=ON
cmake --build build-ucrt64
./build-ucrt64/benchmarks/edgex_benchmark.exe --requests 10000 --concurrency 4 --port 19090
```

On Windows/MSYS2 UCRT64, run the benchmark from the UCRT64 shell so the matching runtime environment is available.

## Definition of Done

A change is complete when:

- Acceptance criteria are satisfied.
- Relevant tests are present and passing.
- The complete CTest suite passes.
- Public APIs and documentation reflect the final behavior.
- No unrelated changes remain in the diff.
- The commit history clearly explains the change.

For the final v1.0 release, the repository must also have validated documentation, recorded benchmark results, release notes, and the annotated `v1.0.0` tag.
