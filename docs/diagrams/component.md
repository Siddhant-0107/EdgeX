# EdgeX Component Diagram

The diagram shows the major v1.0 modules and their primary relationships.

```mermaid
flowchart LR
    Client[Client]
    Socket[Socket Layer]
    TCP[TCP Server]
    Pool[Thread Pool]
    Parser[HTTP Parser]
    Router[Router]
    Static[Static File Server]
    Proxy[Reverse Proxy]
    LB[Round-Robin Load Balancer]
    Health[Health Checker]
    ClientUtil[TCP Client]
    Logger[Logger]
    Metrics[Metrics]

    Client --> Socket --> TCP --> Pool --> Parser --> Router
    Router --> Static
    Router --> Proxy
    Proxy --> LB
    LB --> ClientUtil
    Health --> ClientUtil
    Health --> LB
    TCP --> Logger
    Proxy --> Logger
    Health --> Logger
    TCP --> Metrics
    Proxy --> Metrics
    Health --> Metrics
```

The important separation is between **selection** and **forwarding**: the load balancer selects a healthy backend while the reverse proxy owns the upstream HTTP exchange.
