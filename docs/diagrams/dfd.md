# EdgeX Data Flow Diagram

## Level 0 — System Context

```mermaid
flowchart LR
    Client[HTTP Client] -->|HTTP/1.1 request| EdgeX[EdgeX]
    EdgeX -->|HTTP response| Client
    Config[Configuration] -->|runtime settings| EdgeX
    EdgeX -->|logs| LogSink[Console / Log File]
    Monitor[Monitoring Client] -->|GET /metrics| EdgeX
    EdgeX -->|metrics response| Monitor
    EdgeX <-->|proxied HTTP| Backend[Backend Services]
```

## Level 1 — Request Data Flow

```mermaid
flowchart TD
    Raw[Raw TCP bytes] --> TCP[TCP Server]
    TCP --> Tasks[Client task]
    Tasks --> Parser[HTTP Parser]
    Parser --> Request[Structured HttpRequest]
    Request --> Router[Router]
    Router --> Static[Static File Server]
    Router --> Proxy[Reverse Proxy]
    Proxy --> Select[Round-Robin selection]
    Health[Health Checker] --> Select
    Select --> Backend[Healthy Backend]
    Static --> Response[HttpResponse]
    Backend --> Proxy
    Proxy --> Response
    Router -->|no match| Response
    Response --> Serialize[HTTP serialization]
    Serialize --> RawOut[Response bytes]
    RawOut --> TCP
```

## Data-flow notes

- TCP bytes become a structured `HttpRequest` only after parser validation.
- The router decides whether data flows to a local/static handler or the proxy path.
- The load balancer contributes a backend endpoint; it does not perform the HTTP forwarding itself.
- Health-check results modify backend eligibility.
- Responses are represented as `HttpResponse` objects before serialization.
- Logging and metrics observe system activity without becoming part of the request-routing decision itself.
