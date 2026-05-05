Gorilla WebSocket is a Go implementation of the WebSocket protocol (RFC 6455). It provides full-duplex communication channels over a single TCP connection, enabling real-time data exchange between clients and servers. The library handles the WebSocket handshake, frame parsing, and connection lifecycle management.

Use Gorilla WebSocket when you need bidirectional, low-latency communication in web applications, such as chat systems, live notifications, collaborative editing, or real-time data streaming. Do not use it for simple HTTP request-response patterns, one-way server-sent events (use Server-Sent Events instead), or when you need message queuing or pub/sub semantics (consider NATS or RabbitMQ).

Key design features include:
- Automatic ping/pong handling for connection health monitoring
- Configurable read and write buffer sizes
- Support for custom dialers and upgrader configurations
- Built-in compression support (deflate)
- Concurrent-safe read and write operations (but not simultaneous)

Installation:
```bash
go get github.com/gorilla/websocket@v1.5.3
```
