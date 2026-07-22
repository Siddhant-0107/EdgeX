# EdgeX Coding Standards

## Purpose

This document defines the coding conventions for EdgeX. It targets Modern C++17 and prioritizes readability, safety, testability, and maintainability. When a rule conflicts with a clear, well-justified design decision, document the exception in the relevant code review or architecture decision record.

## 1. Naming conventions

Use descriptive English names. Avoid abbreviations unless they are widely understood in the domain, such as `http`, `tcp`, `url`, or `id`.

| Element | Convention | Example |
| --- | --- | --- |
| Namespace | lowercase | `edgex::http` |
| Class, struct, enum | `PascalCase` | `HttpRequestParser` |
| Function and method | `snake_case` | `parse_request()` |
| Variable and parameter | `snake_case` | `request_timeout` |
| Private data member | `snake_case_` | `worker_count_` |
| Constant / `constexpr` | `kPascalCase` | `kDefaultPort` |
| Enum value | `PascalCase` | `LogLevel::Warning` |
| Template parameter | `PascalCase` | `typename Handler` |
| Macro | `UPPER_SNAKE_CASE` | `EDGEX_ENABLE_TRACE` |

Do not encode types in names, such as `str_name`, `p_socket`, or `vec_routes`.

## 2. Class naming and design

- Name classes as singular nouns that describe their responsibility: `Router`, `ThreadPool`, `HealthChecker`.
- Name interfaces by behavior where appropriate: `RequestHandler`, `LoadBalancingStrategy`.
- Keep classes focused on one responsibility. Prefer composition over inheritance.
- Make constructors `explicit` when a single argument must not allow implicit conversion.
- Mark overridden virtual methods with `override`; use `final` only when further derivation is intentionally prohibited.
- Prefer `enum class` over unscoped enums.

## 3. Function naming

- Use verbs or verb phrases: `start()`, `stop()`, `load_config()`, `write_response()`.
- Use predicates for boolean-returning functions: `is_healthy()`, `has_route()`, `can_accept()`.
- Use `get_` only when it improves clarity; prefer meaningful accessors such as `status()` or `headers()`.
- Keep functions small and give each a single, observable responsibility.
- Mark functions `[[nodiscard]]` when ignoring their return value is likely an error, especially parsing and I/O results.

## 4. Variable naming

- Use names that communicate purpose, not implementation: `request_buffer`, not `data`; `backend_index`, not `i` outside short loops.
- Use short loop variables only in tightly scoped loops.
- Prefer a scoped `const` value over mutable state when possible.
- Do not use global mutable variables. Use an object owned by the application, dependency injection, or function-local static data when appropriate.

## 5. File naming and layout

- Use lowercase `snake_case` filenames: `http_request_parser.hpp`, `reverse_proxy.cpp`.
- Place public headers in `include/edgex/<module>/`.
- Mirror public-header modules in `src/<module>/`.
- A source file should normally implement the correspondingly named header.
- Keep private implementation headers in `src/` or a clearly named `detail/` directory; do not expose them under `include/`.
- Avoid catch-all files such as `utils.hpp`, `helpers.cpp`, or `common.cpp` unless their scope is genuinely cohesive.

## 6. Namespace rules

- All EdgeX code belongs beneath `namespace edgex`.
- Use nested namespaces: `namespace edgex::http { ... }`.
- Match namespaces to module ownership, for example `edgex::net`, `edgex::proxy`, and `edgex::observability`.
- Never write `using namespace` in a header.
- Avoid `using namespace` in source files; use a scoped `using` declaration only when it improves local clarity.

## 7. Include order

Every `.cpp` file includes its own corresponding header first. Then group includes in this order, with one blank line between groups:

1. Corresponding project header.
2. Other EdgeX project headers.
3. C++ standard-library headers.
4. Third-party library headers.
5. Platform or system headers.

Sort headers alphabetically within each group.

```cpp
#include "edgex/http/http_request_parser.hpp"

#include "edgex/common/result.hpp"
#include "edgex/observability/logger.hpp"

#include <string_view>
#include <utility>

#include <fmt/format.h>

#include <sys/socket.h>
```

Headers must be self-contained: including a public header by itself should compile.

## 8. Header guards

- Use `#pragma once` in all EdgeX headers.
- Do not place `using namespace`, non-inline function definitions, or non-`inline` variables in headers.
- Prefer forward declarations when they reduce dependencies without obscuring ownership or requiring incomplete-type misuse.

## 9. Const correctness

- Mark member functions `const` when they do not modify observable object state.
- Pass read-only objects by `const T&` when copying is non-trivial.
- Pass small, cheap value types by value when it is simpler, for example `int`, enums, and often `std::string_view`.
- Return `const` values only when returning references or pointers where it expresses meaningful immutability; do not return `const T` by value.
- Prefer immutable local values and `constexpr` values where possible.

## 10. RAII guidelines

- Every resource must have a clear owner and deterministic release behavior.
- Use RAII wrappers for sockets, file descriptors, mutex locking, files, threads, and other operating-system resources.
- Acquire resources in constructors or factory functions and release them in destructors.
- Do not manually pair `new`/`delete`, `malloc`/`free`, `open`/`close`, or `lock`/`unlock` in ordinary control flow.
- Use `std::lock_guard` or `std::unique_lock` for mutexes; never manually unlock a mutex solely to handle an early return.

## 11. Smart pointer guidelines

- Prefer automatic storage duration and value members whenever ownership is clear.
- Use `std::unique_ptr<T>` for exclusive heap ownership.
- Use `std::shared_ptr<T>` only when ownership is genuinely shared and its lifetime model is documented.
- Use `std::weak_ptr<T>` to break ownership cycles or observe a shared object without extending its lifetime.
- Create smart pointers with `std::make_unique` and `std::make_shared`.
- Do not use owning raw pointers. Use raw pointers or references only as non-owning views with a clear lifetime contract.

## 12. Exception handling philosophy

- Use exceptions for exceptional, non-local failures where recovery is not naturally expressed through a return value.
- Do not use exceptions for ordinary control flow, expected parse failures, unavailable routes, or normal network errors.
- For expected failures, use an explicit result type such as `Result<T, Error>` or `std::optional<T>` where the absence of a value is sufficient.
- Do not allow exceptions to escape C-style callbacks, thread entry points, or destructors.
- Destructors must not throw.
- Catch exceptions at application and thread boundaries, log useful context, and translate them into a safe failure response or shutdown action.

## 13. Logging guidelines

- Use structured, level-appropriate logs: `trace`, `debug`, `info`, `warning`, `error`, and `critical`.
- Log one event per message with meaningful context: request ID, method, path, status code, backend target, and duration where relevant.
- Never log credentials, tokens, authorization headers, cookies, private keys, or full sensitive request bodies.
- Do not use logging as a substitute for error handling.
- Avoid high-volume `info` logs on hot paths in production; use `debug` or metrics where appropriate.
- Log errors at the layer that has enough context to explain the failure, while avoiding duplicate logs at every propagation layer.

## 14. Formatting conventions

- Use four spaces for indentation; do not use tabs.
- Keep lines at or below 100 characters where practical.
- Place opening braces on the same line as the control statement or declaration.
- Use one statement per line and one declaration per line when it improves readability.
- Include a space after control-flow keywords: `if (condition)`, `for (...)`, `while (...)`.
- Use braces for all control-flow bodies, including single-statement bodies.
- Prefer early returns to avoid deeply nested control flow.
- Run the project formatter before committing. EdgeX should standardize on a checked-in `.clang-format` file once formatting is enabled.

```cpp
if (!request.is_valid()) {
    return ErrorResponse::bad_request();
}
```

## 15. Documentation comments

- Write comments to explain *why*, invariants, ownership, concurrency expectations, or non-obvious protocol behavior—not to restate code.
- Document all public classes, functions, enums, and configuration fields.
- Use Doxygen-style comments for public APIs when generated API documentation is desired.
- Document preconditions, postconditions, errors, thread-safety, and ownership for interfaces where these are not obvious.

```cpp
/// Selects the next healthy backend using round-robin scheduling.
/// @return A non-owning backend view, or an error when no backend is healthy.
[[nodiscard]] Result<BackendView, LoadBalancerError> select_backend();
```

## 16. Git commit conventions

- Make each commit small, buildable, and focused on one logical change.
- Use imperative, concise commit subjects with Conventional Commit-style prefixes.
- Keep the subject line below 72 characters when possible.
- Add a body when the intent, trade-off, or compatibility impact needs explanation.

```text
feat(http): add request-line parser
fix(proxy): reject unavailable backend targets
test(router): cover unmatched route handling
docs(srs): clarify proxy error responses
build(cmake): add debug and release presets
refactor(net): isolate socket ownership wrapper
```

Before committing, run the relevant formatter, build, and tests. Do not commit generated binaries, build directories, logs, credentials, or unrelated formatting changes.
