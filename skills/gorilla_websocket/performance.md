### Buffer Size Configuration
```go
// Optimize buffer sizes based on message patterns
var upgrader = websocket.Upgrader{
    ReadBufferSize:  4096,  // 4KB for typical messages
    WriteBufferSize: 4096,  // 4KB for typical messages
}

// For large messages, increase buffer sizes
var upgraderLarge = websocket.Upgrader{
    ReadBufferSize:  65536, // 64KB for large payloads
    WriteBufferSize: 65536, // 64KB for large payloads
}
```

### Memory Allocation Patterns
```go
// BAD: Allocating new buffer for each message
func handleMessages(conn *websocket.Conn) {
    for {
        _, msg, err := conn.ReadMessage()
        if err != nil {
            break
        }
        process(msg) // msg is a new allocation each time
    }
}

// GOOD: Reuse buffers with sync.Pool
var bufferPool = sync.Pool{
    New: func() interface{} {
        return make([]byte, 4096)
    },
}

func handleMessages(conn *websocket.Conn) {
    for {
        buf := bufferPool.Get().([]byte)
        n, err := conn.Read(buf)
        if err != nil {
            bufferPool.Put(buf)
            break
        }
        process(buf[:n])
        bufferPool.Put(buf)
    }
}
```

### Compression Considerations
```go
// Enable compression for bandwidth-sensitive applications
var upgrader = websocket.Upgrader{
    EnableCompression: true,
}

// Monitor compression ratio
func monitorCompression(conn *websocket.Conn) {
    if conn.EnableWriteCompression {
        log.Println("compression enabled")
    }
}
```

### GC Impact Mitigation
```go
// Reduce GC pressure by reusing message structures
type Message struct {
    Type int
    Data []byte
}

var msgPool = sync.Pool{
    New: func() interface{} {
        return &Message{Data: make([]byte, 0, 4096)}
    },
}

func processMessages(conn *websocket.Conn) {
    for {
        msg := msgPool.Get().(*Message)
        msg.Type, msg.Data, _ = conn.ReadMessage()
        
        // Process message
        go func(m *Message) {
            defer msgPool.Put(m)
            // Handle message
        }(msg)
    }
}
```
