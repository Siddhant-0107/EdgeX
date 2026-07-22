# Design Decisions

Record significant architectural decisions here using short Architecture Decision Records (ADRs).

## ADR-001: Module-oriented source layout

**Decision:** Public headers and implementation files are grouped by subsystem.

**Rationale:** The layout keeps networking, HTTP, routing, proxy, and observability responsibilities isolated as EdgeX grows.
