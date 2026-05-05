### Construction and Initialization
```go
type WebSocketHandler struct {
    upgrader websocket.Upgrader
    clients  map[*websocket.Conn]bool
    mu       sync.RWMutex
    done     chan struct{}
}

func NewWebSocketHandler() *WebSocketHandler {
    return &WebSocketHandler{
        upgrader: websocket.Upgrader{
            ReadBufferSize:  4096,
            WriteBufferSize: 4096,
            CheckOrigin: func(r *http.Request) bool {
                return true // Configure for production
            },
        },
        clients: make(map[*websocket.Conn]bool),
        done:    make(chan struct{}),
    }
}
```

### Connection Lifecycle Management
```go
func (h *WebSocketHandler) HandleConnection(w http.ResponseWriter, r *http.Request) {
    conn, err := h.upgrader.Upgrade(w, r, nil)
    if err != nil {
        log.Printf("upgrade failed: %v", err)
        return
    }
    
    h.mu.Lock()
    h.clients[conn] = true
    h.mu.Unlock()
    
    defer func() {
        h.mu.Lock()
        delete(h.clients, conn)
        h.mu.Unlock()
        conn.Close()
    }()
    
    // Configure connection
    conn.SetReadLimit(4096)
    conn.SetReadDeadline(time.Now().Add(60 * time.Second))
    conn.SetPongHandler(func(string) error {
        conn.SetReadDeadline(time.Now().Add(60 * time.Second))
        return nil
    })
    
    // Main read loop
    for {
        select {
        case <-h.done:
            conn.WriteMessage(websocket.CloseMessage, 
                websocket.FormatCloseMessage(websocket.CloseNormalClosure, "server shutdown"))
            return
        default:
            _, msg, err := conn.ReadMessage()
            if err != nil {
                return
            }
            h.broadcast(msg)
        }
    }
}
```

### Cleanup and Resource Management
```go
func (h *WebSocketHandler) Shutdown() {
    log.Println("shutting down WebSocket handler")
    close(h.done)
    
    h.mu.Lock()
    defer h.mu.Unlock()
    
    for conn := range h.clients {
        conn.WriteMessage(websocket.CloseMessage, 
            websocket.FormatCloseMessage(websocket.CloseNormalClosure, "server shutdown"))
        conn.Close()
    }
    
    h.clients = make(map[*websocket.Conn]bool)
}
```

### Proper Connection Teardown
```go
func cleanupConnection(conn *websocket.Conn) {
    // Send close frame
    message := websocket.FormatCloseMessage(websocket.CloseNormalClosure, "bye")
    conn.WriteControl(websocket.CloseMessage, message, time.Now().Add(time.Second))
    
    // Wait for close frame from peer
    conn.SetReadDeadline(time.Now().Add(2 * time.Second))
    _, _, err := conn.ReadMessage()
    if err != nil {
        log.Printf("error reading close frame: %v", err)
    }
    
    // Close the underlying connection
    conn.Close()
}
```
