### 1. Not Setting Read/Write Deadlines
```go
// BAD: No deadlines - connection can hang forever
func handleConn(conn *websocket.Conn) {
    for {
        _, msg, err := conn.ReadMessage()
        if err != nil {
            break
        }
        // process msg
    }
}

// GOOD: Set deadlines to prevent hanging connections
func handleConn(conn *websocket.Conn) {
    conn.SetReadDeadline(time.Now().Add(60 * time.Second))
    conn.SetPongHandler(func(string) error {
        conn.SetReadDeadline(time.Now().Add(60 * time.Second))
        return nil
    })
    
    for {
        _, msg, err := conn.ReadMessage()
        if err != nil {
            break
        }
        // process msg
    }
}
```

### 2. Concurrent Read/Write Without Mutex
```go
// BAD: Concurrent read and write from different goroutines
func handleConn(conn *websocket.Conn) {
    go func() {
        for {
            conn.WriteMessage(websocket.TextMessage, []byte("ping"))
        }
    }()
    go func() {
        for {
            _, msg, _ := conn.ReadMessage()
            // process msg
        }
    }()
}

// GOOD: Use a single goroutine or mutex for writes
type SafeConn struct {
    mu   sync.Mutex
    conn *websocket.Conn
}

func (s *SafeConn) WriteMessage(msgType int, data []byte) error {
    s.mu.Lock()
    defer s.mu.Unlock()
    return s.conn.WriteMessage(msgType, data)
}
```

### 3. Ignoring Close Codes
```go
// BAD: Ignoring close messages
func handleConn(conn *websocket.Conn) {
    for {
        _, msg, err := conn.ReadMessage()
        if err != nil {
            log.Println("error:", err)
            break
        }
    }
}

// GOOD: Handle close frames properly
func handleConn(conn *websocket.Conn) {
    defer conn.Close()
    conn.SetCloseHandler(func(code int, text string) error {
        log.Printf("peer closed connection: %d %s", code, text)
        message := websocket.FormatCloseMessage(code, "")
        conn.WriteControl(websocket.CloseMessage, message, time.Now().Add(time.Second))
        return nil
    })
    
    for {
        _, msg, err := conn.ReadMessage()
        if err != nil {
            if websocket.IsCloseError(err, websocket.CloseNormalClosure) {
                log.Println("normal closure")
            } else {
                log.Println("error:", err)
            }
            break
        }
    }
}
```

### 4. Not Handling Ping/Pong
```go
// BAD: No ping/pong handling - connection may timeout
func handleConn(conn *websocket.Conn) {
    for {
        _, msg, err := conn.ReadMessage()
        if err != nil {
            break
        }
    }
}

// GOOD: Handle ping/pong for keepalive
func handleConn(conn *websocket.Conn) {
    conn.SetPongHandler(func(appData string) error {
        log.Println("received pong")
        return nil
    })
    
    ticker := time.NewTicker(30 * time.Second)
    defer ticker.Stop()
    
    go func() {
        for range ticker.C {
            if err := conn.WriteMessage(websocket.PingMessage, nil); err != nil {
                return
            }
        }
    }()
    
    for {
        _, msg, err := conn.ReadMessage()
        if err != nil {
            break
        }
    }
}
```

### 5. Not Checking Origin in Production
```go
// BAD: Accepting all origins
var upgrader = websocket.Upgrader{
    CheckOrigin: func(r *http.Request) bool {
        return true // Security risk!
    },
}

// GOOD: Validate origin
var upgrader = websocket.Upgrader{
    CheckOrigin: func(r *http.Request) bool {
        origin := r.Header.Get("Origin")
        allowedOrigins := []string{"https://example.com", "https://app.example.com"}
        for _, allowed := range allowedOrigins {
            if origin == allowed {
                return true
            }
        }
        return false
    },
}
```

### 6. Not Handling Write Buffer Full
```go
// BAD: Blocking write without timeout
func broadcast(conn *websocket.Conn, msg []byte) {
    conn.WriteMessage(websocket.TextMessage, msg)
}

// GOOD: Use write deadline to prevent blocking
func broadcast(conn *websocket.Conn, msg []byte) error {
    conn.SetWriteDeadline(time.Now().Add(10 * time.Second))
    return conn.WriteMessage(websocket.TextMessage, msg)
}
```
