### Goroutine-Safe Connection Management
```go
type SafeConnection struct {
    mu     sync.Mutex
    conn   *websocket.Conn
    closed bool
}

func (s *SafeConnection) WriteMessage(msgType int, data []byte) error {
    s.mu.Lock()
    defer s.mu.Unlock()
    
    if s.closed {
        return errors.New("connection closed")
    }
    
    s.conn.SetWriteDeadline(time.Now().Add(10 * time.Second))
    return s.conn.WriteMessage(msgType, data)
}

func (s *SafeConnection) Close() error {
    s.mu.Lock()
    defer s.mu.Unlock()
    
    if s.closed {
        return nil
    }
    
    s.closed = true
    return s.conn.Close()
}
```

### Channel-Based Communication Pattern
```go
type WebSocketClient struct {
    conn     *websocket.Conn
    send     chan []byte
    receive  chan []byte
    errors   chan error
    done     chan struct{}
    closeOnce sync.Once
}

func NewWebSocketClient(conn *websocket.Conn) *WebSocketClient {
    return &WebSocketClient{
        conn:    conn,
        send:    make(chan []byte, 256),
        receive: make(chan []byte, 256),
        errors:  make(chan error, 1),
        done:    make(chan struct{}),
    }
}

func (c *WebSocketClient) Start() {
    go c.readLoop()
    go c.writeLoop()
}

func (c *WebSocketClient) readLoop() {
    defer close(c.done)
    
    for {
        _, msg, err := c.conn.ReadMessage()
        if err != nil {
            select {
            case c.errors <- err:
            default:
            }
            return
        }
        
        select {
        case c.receive <- msg:
        case <-c.done:
            return
        }
    }
}

func (c *WebSocketClient) writeLoop() {
    ticker := time.NewTicker(30 * time.Second)
    defer ticker.Stop()
    
    for {
        select {
        case msg, ok := <-c.send:
            if !ok {
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

func (c *WebSocketClient) Close() {
    c.closeOnce.Do(func() {
        close(c.send)
        <-c.done
        c.conn.Close()
    })
}
```

### Fan-Out Pattern for Broadcasting
```go
type Hub struct {
    clients    map[*SafeConnection]bool
    broadcast  chan []byte
    register   chan *SafeConnection
    unregister chan *SafeConnection
    mu         sync.RWMutex
}

func NewHub() *Hub {
    return &Hub{
        clients:    make(map[*SafeConnection]bool),
        broadcast:  make(chan []byte, 256),
        register:   make(chan *SafeConnection),
        unregister: make(chan *SafeConnection),
    }
}

func (h *Hub) Run() {
    for {
        select {
        case client := <-h.register:
            h.mu.Lock()
            h.clients[client] = true
            h.mu.Unlock()
            
        case client := <-h.unregister:
            h.mu.Lock()
            if _, ok := h.clients[client]; ok {
                delete(h.clients, client)
                close(client.send)
            }
            h.mu.Unlock()
            
        case message := <-h.broadcast:
            h.mu.RLock()
            for client := range h.clients {
                select {
                case client.send <- message:
                default:
                    close(client.send)
                    delete(h.clients, client)
                }
            }
            h.mu.RUnlock()
        }
    }
}
```

### Atomic Operations for Connection State
```go
type ConnectionState struct {
    mu       sync.RWMutex
    isAlive  bool
    lastPing time.Time
}

func (s *ConnectionState) SetAlive(alive bool) {
    s.mu.Lock()
    defer s.mu.Unlock()
    s.isAlive = alive
    if alive {
        s.lastPing = time.Now()
    }
}

func (s *ConnectionState) IsAlive() bool {
    s.mu.RLock()
    defer s.mu.RUnlock()
    return s.isAlive && time.Since(s.lastPing) < 60*time.Second
}
```
