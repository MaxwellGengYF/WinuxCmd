Tokio is an asynchronous runtime for Rust that provides event-driven, non-blocking I/O. It's designed for building reliable, scalable network applications and services.

**When to use Tokio:**
- Building web servers, proxies, or network services
- Handling many concurrent connections efficiently
- Performing I/O-bound operations (file, network, database)
- Implementing asynchronous workflows with timeouts and cancellation

**When NOT to use Tokio:**
- CPU-bound computations (use threads instead)
- Simple synchronous scripts
- Embedded systems with limited resources
- When you need deterministic execution order

**Key Design:**
- Work-stealing task scheduler for multi-threaded execution
- Reactor backed by OS event queues (epoll, kqueue, IOCP)
- Zero-cost abstractions for async/await
- Cooperative scheduling with cancellation

**Installation:**
```bash
cargo add tokio --features full
```

For minimal setup:
```bash
cargo add tokio --features rt,net,io-util
```
