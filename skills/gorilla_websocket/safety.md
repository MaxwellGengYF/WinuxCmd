### 1. NEVER Write to a Closed Connection
```go
// BAD: Writing after connection is closed
conn.Close()
conn.WriteMessage(websocket.TextMessage, []byte("data")) // Panic!

// GOOD: Check before writing
func safeWrite(conn *websocket.Conn, msg []byte) error {
    if conn == nil {
        return errors.New("connection is nil")
    }
    conn.SetWriteDeadline(time.Now().Add(5 * time.Second))
    return conn.WriteMessage(websocket.TextMessage, msg)
}
```

### 2. NEVER Read and Write Concurrently Without Synchronization
```go
// BAD: Concurrent read/write causes data races
go conn.ReadMessage()
go conn.WriteMessage(websocket.TextMessage, []byte("data"))

// GOOD: Use separate goroutines with channel synchronization
type Connection struct {
    conn    *websocket.Conn
    send    chan []byte
    receive chan []byte
    done    chan struct{}
}

func (c *Connection) readPump() {
    defer close(c.done)
    for {
        _, msg, err := c.conn.ReadMessage()
        if err != nil {
            return
        }
        select {
        case c.receive <- msg:
        case <-c.done:
            return
        }
    }
}

func (c *Connection) writePump() {
    ticker := time.NewTicker(30 * time.Second)
    defer ticker.Stop()
    
    for {
        select {
        case msg, ok := <-c.send:
            if !ok {
                c.conn.WriteMessage(websocket.CloseMessage, []byte{})
                return
            }
            c.conn.SetWriteDeadline(time.Now().Add(10 * time.Second))
            if err := c.conn.WriteMessage(websocket.TextMessage, msg); err != nil {
                return
            }
        case <-ticker.C:
            c.conn.SetWriteDeadline(time.Now().Add(10 * time.Second))
            if err := c.conn.WriteMessage(websocket.PingMessage, nil); err != nil {
                return
            }
        case <-c.done:
            return
        }
    }
}
```

### 3. NEVER Ignore Errors from ReadMessage/WriteMessage
```go
// BAD: Silently ignoring errors
conn.ReadMessage()
conn.WriteMessage(websocket.TextMessage, []byte("data"))

// GOOD: Always handle errors
func safeRead(conn *websocket.Conn) ([]byte, error) {
    _, msg, err := conn.ReadMessage()
    if err != nil {
        if websocket.IsUnexpectedCloseError(err, websocket.CloseGoingAway) {
            return nil, fmt.Errorf("unexpected close: %w", err)
        }
        return nil, err
    }
    return msg, nil
}
```

### 4. NEVER Use Default Upgrader in Production
```go
// BAD: Using default upgrader with no restrictions
var upgrader = websocket.Upgrader{}

// GOOD: Configure upgrader with proper limits
var upgrader = websocket.Upgrader{
    ReadBufferSize:  4096,
    WriteBufferSize: 4096,
    CheckOrigin: func(r *http.Request) bool {
        return r.Header.Get("Origin") == "https://example.com"
    },
    EnableCompression: true,
}
```

### 5. NEVER Forget to Set Read/Write Deadlines
```go
// BAD: No deadlines set
conn.ReadMessage()

// GOOD: Always set deadlines
func readWithTimeout(conn *websocket.Conn, timeout time.Duration) ([]byte, error) {
    conn.SetReadDeadline(time.Now().Add(timeout))
    _, msg, err := conn.ReadMessage()
    return msg, err
}
```
