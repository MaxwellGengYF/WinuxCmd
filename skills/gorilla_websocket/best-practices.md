### Use Separate Read/Write Goroutines
```go
type Client struct {
    conn *websocket.Conn
    send chan []byte
    done chan struct{}
}

func NewClient(conn *websocket.Conn) *Client {
    return &Client{
        conn: conn,
        send: make(chan []byte, 256),
        done: make(chan struct{}),
    }
}

func (c *Client) Start() {
    go c.readPump()
    go c.writePump()
}

func (c *Client) readPump() {
    defer func() {
        close(c.done)
        c.conn.Close()
    }()
    
    c.conn.SetReadLimit(4096)
    c.conn.SetReadDeadline(time.Now().Add(60 * time.Second))
    c.conn.SetPongHandler(func(string) error {
        c.conn.SetReadDeadline(time.Now().Add(60 * time.Second))
        return nil
    })
    
    for {
        _, msg, err := c.conn.ReadMessage()
        if err != nil {
            break
        }
        // Process message
    }
}

func (c *Client) writePump() {
    ticker := time.NewTicker(30 * time.Second)
    defer func() {
        ticker.Stop()
        c.conn.Close()
    }()
    
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

### Implement Graceful Shutdown
```go
type Server struct {
    clients map[*Client]bool
    mu      sync.RWMutex
    done    chan struct{}
}

func (s *Server) Shutdown() {
    close(s.done)
    s.mu.Lock()
    defer s.mu.Unlock()
    
    for client := range s.clients {
        client.conn.WriteMessage(websocket.CloseMessage, 
            websocket.FormatCloseMessage(websocket.CloseNormalClosure, "server shutdown"))
        client.conn.Close()
    }
}
```

### Use Connection Pooling
```go
type ConnectionPool struct {
    mu      sync.Mutex
    conns   map[string]*websocket.Conn
    maxSize int
}

func (p *ConnectionPool) Add(id string, conn *websocket.Conn) error {
    p.mu.Lock()
    defer p.mu.Unlock()
    
    if len(p.conns) >= p.maxSize {
        return errors.New("connection pool full")
    }
    p.conns[id] = conn
    return nil
}

func (p *ConnectionPool) Remove(id string) {
    p.mu.Lock()
    defer p.mu.Unlock()
    
    if conn, ok := p.conns[id]; ok {
        conn.Close()
        delete(p.conns, id)
    }
}
```
