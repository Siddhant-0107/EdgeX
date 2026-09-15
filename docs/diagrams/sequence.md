# EdgeX Request Sequence Diagram

```mermaid
sequenceDiagram
    participant C as Client
    participant S as TCP Server
    participant P as Thread Pool
    participant H as HTTP Parser
    participant R as Router
    participant X as Reverse Proxy
    participant L as Load Balancer
    participant B as Backend
    participant F as Static File Server

    C->>S: TCP connect + HTTP request
    S->>P: submit client task
    P->>H: parse request bytes
    H-->>P: HttpRequest
    P->>R: route request

    alt Static route
        R->>F: serve(request)
        F-->>R: HttpResponse
    else Proxy route
        R->>X: forward(request)
        X->>L: select healthy backend
        L-->>X: backend endpoint
        X->>B: HTTP request
        B-->>X: HTTP response
        X-->>R: HttpResponse
    else No matching route
        R-->>P: 404 response
    end

    P-->>S: serialized response
    S-->>C: HTTP response
```

The thread pool owns execution concurrency; the parser owns protocol decoding; the router owns local dispatch; and proxy/load-balancer responsibilities remain separate.
