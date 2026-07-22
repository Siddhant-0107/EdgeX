# EdgeX Git Workflow

## Purpose

This workflow keeps EdgeX development simple for a solo developer while preserving professional practices: stable mainline code, focused reviews, traceable releases, and scalable repository organization.

## 1. Branch strategy

Use a lightweight trunk-based workflow. The `main` branch is the stable integration branch; all changes are developed on short-lived branches and merged through pull requests.

```text
main
 |- feature/http-parser
 |- feature/thread-pool
 |- fix/request-timeout
 |- docs/update-srs
 `- build/add-clang-format
```

- `main` must remain buildable and should pass relevant tests before each merge.
- Do not use long-lived `develop` or release branches at the current project scale.
- Keep every feature branch focused on one logical outcome.

## 2. Feature branches

Create a branch from the latest `main` using one of these prefixes:

| Prefix | Purpose | Example |
| --- | --- | --- |
| `feature/` | New capability | `feature/http-request-parser` |
| `fix/` | Bug correction | `fix/static-file-path-traversal` |
| `refactor/` | Internal design improvement | `refactor/socket-raii-wrapper` |
| `test/` | Test-only work | `test/router-coverage` |
| `docs/` | Documentation change | `docs/coding-standards` |
| `build/` | CMake, CI, or tooling | `build/add-clang-format` |
| `chore/` | Maintenance work | `chore/update-gitignore` |

Typical workflow:

```powershell
git checkout main
git pull
git checkout -b feature/http-request-parser

# Implement and validate the change.

git add include/edgex/http src/http tests/unit
git commit -m "feat(http): add request-line parser"
git push -u origin feature/http-request-parser
```

After the pull request is merged, delete the feature branch locally and remotely.

## 3. Commit message format

Use Conventional Commit-style messages:

```text
<type>(<scope>): <short imperative summary>
```

Use the imperative mood: `add`, `fix`, `update`, and `remove`, rather than `added`, `fixed`, or `updates`.

| Type | Use |
| --- | --- |
| `feat` | New user-visible capability |
| `fix` | Bug fix |
| `refactor` | Internal change without intended behavior change |
| `test` | Test additions or modifications |
| `docs` | Documentation-only change |
| `build` | CMake, compiler, dependency, or CI changes |
| `perf` | Performance improvement |
| `chore` | Maintenance task |

Examples:

```text
feat(net): add TCP socket wrapper
feat(proxy): add round-robin backend selection
fix(http): reject malformed content-length header
test(router): add unmatched-route coverage
docs(architecture): add component diagram
build(cmake): enable compiler warnings
```

Keep subjects below 72 characters where practical. Add a commit body when a design trade-off, compatibility impact, or non-obvious decision needs explanation.

## 4. Pull request style

Open a pull request even when working alone. Pull requests provide a reviewable record of design decisions and make it easier to inspect changes before they reach `main`.

Each pull request should:

- Have one clear purpose.
- Be small enough to review comfortably.
- Include relevant tests.
- Avoid unrelated formatting or refactoring.
- Update documentation when the behavior, API, configuration, or architecture changes.

Use this template:

```markdown
## Summary

Briefly describe what changed and why.

## Changes

- Added:
- Updated:
- Removed:

## Validation

- [ ] Project builds successfully
- [ ] Relevant unit tests pass
- [ ] Integration tests pass, if applicable
- [ ] Documentation updated
- [ ] No credentials or generated files included

## Notes

Known limitations, follow-up work, or design decisions.
```

Before merging, review the diff as if it were submitted by another engineer. Pay particular attention to API design, resource ownership, thread safety, error handling, test coverage, and documentation.

## 5. Release tags

Create annotated tags from `main` for meaningful releases:

```powershell
git checkout main
git pull
git tag -a v1.0.0 -m "Release EdgeX v1.0.0"
git push origin v1.0.0
```

Publish release notes with these sections:

```markdown
## Added
## Changed
## Fixed
## Known Limitations
```

Release only from a validated `main` revision. Never move or reuse a published version tag.

## 6. Semantic versioning

EdgeX uses Semantic Versioning:

```text
MAJOR.MINOR.PATCH
```

- **MAJOR:** Breaking API, behavior, or configuration changes.
- **MINOR:** New backward-compatible functionality.
- **PATCH:** Backward-compatible bug fixes.

Suggested progression:

```text
v0.1.0  Initial scaffold
v0.2.0  HTTP parser added
v0.3.0  Static file server added
v0.4.0  Reverse proxy added
v1.0.0  First planned complete EdgeX release
v1.0.1  Fix malformed-request handling
v1.1.0  Add configurable load-balancing policy
v2.0.0  Breaking configuration format redesign
```

Before the first stable release, use `0.x.y`. When useful, publish milestone builds such as `v1.0.0-alpha.1`, `v1.0.0-beta.1`, and `v1.0.0-rc.1`.

## 7. Folder organization for future scalability

Maintain a module-oriented layout:

```text
EdgeX/
|- include/edgex/       # Public headers by module
|- src/                 # Implementations mirroring include/edgex
|- tests/
|  |- unit/
|  |- integration/
|  |- fixtures/
|  `- support/          # Shared test helpers and mock services
|- benchmarks/
|- examples/
|- docs/
|  |- diagrams/
|  `- adr/              # Architecture Decision Records
|- config/
|- scripts/
|- cmake/
`- third_party/
```

- Keep public APIs in `include/edgex/` and private implementation details in `src/`.
- Mirror module boundaries in source and test directories.
- Add `docs/adr/` as design decisions become significant.
- Add `tests/support/` for reusable fixtures, mocks, and local test services.
- Add modules only when they have a clear ownership boundary; avoid generic catch-all utility folders.
