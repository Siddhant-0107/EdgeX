# EdgeX Coding Standards

## Language and Build

- Use C++17 and keep code portable across supported platforms.
- Prefer standard-library facilities over custom utilities when they are clear and sufficient.
- Keep compiler warnings enabled and treat warnings as design feedback.

## Ownership and Lifetime

- Prefer RAII for sockets, files, locks, and other resources.
- Delete copy operations for unique resource owners.
- Use move semantics when ownership transfer is meaningful.
- Avoid raw owning pointers.

## Interfaces

- Keep public headers focused on stable APIs.
- Use `const` and `noexcept` where they accurately express contracts.
- Prefer value types and references for non-owning relationships.
- Validate invalid configuration and arguments at the owning boundary.

## Concurrency

- Protect shared mutable state with the narrowest appropriate synchronization boundary.
- Do not hold locks while performing blocking network or filesystem I/O unless the lock is explicitly part of the operation's contract.
- Use atomics for simple independent counters/state where appropriate.
- Define shutdown behavior explicitly for worker threads and queues.

## Networking

- Keep platform-specific socket types and system calls isolated in the networking layer.
- Always define ownership of accepted and outbound sockets.
- Handle partial writes and expected transient errors where the API requires it.
- Configure network timeouts rather than relying on indefinite blocking for outbound infrastructure operations.

## HTTP

- Keep parsing, routing, response construction, and proxying as separate concerns.
- Treat incomplete input differently from malformed input.
- Validate request syntax before normal application dispatch.
- Do not blindly forward hop-by-hop HTTP headers through a proxy.

## Error Handling

- Use exceptions for invalid construction/configuration and exceptional infrastructure failures where the API contract calls for them.
- Convert expected network/backend failures into controlled module results or HTTP errors at the appropriate boundary.
- Do not let one user task terminate the thread pool.
- Error messages should identify the failing operation without exposing sensitive configuration values.

## Testing

- Add focused tests for new behavior and failure paths.
- Prefer deterministic local fixtures and loopback servers for networking tests.
- Run the complete CTest suite before merging a behavioral change.
- Keep benchmarks separate from correctness tests.

## Documentation

- Update public API comments and technical documentation when behavior changes.
- Keep architecture diagrams consistent with actual ownership and data flow.
- Record deliberate limitations instead of implying unsupported features exist.

## Naming and Formatting

- Use clear descriptive names.
- Use `PascalCase` for types and `snake_case` for functions/variables, matching the existing EdgeX codebase.
- Prefer small functions with one clear responsibility.
- Avoid unnecessary abbreviations and clever code.
- Preserve consistent formatting through the repository's existing style.
