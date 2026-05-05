```go
package main

import (
    "log"
    "net/http"
    "github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{
    ReadBufferSize:  1024,
    WriteBufferSize: 1024,
}

func main() {
    http.HandleFunc("/ws", handleWebSocket)
    log.Fatal(http.ListenAndServe(":8080", nil))
}

func handleWebSocket(w http.ResponseWriter, r *http.Request) {
    conn, err := upgrader.Upgrade(w, r, nil)
    if err != nil {
        log.Print("Upgrade failed:", err)
        return
    }
    defer conn.Close()

    for {
        messageType, p, err := conn.ReadMessage()
        if err != nil {
            log.Println("Read error:", err)
            break
        }
        if err := conn.WriteMessage(messageType, p); err != nil {
            log.Println("Write error:", err)
            break
        }
    }
}
```

```go
package main

import (
    "log"
    "net/url"
    "github.com/gorilla/websocket"
)

func main() {
    u := url.URL{Scheme: "ws", Host: "localhost:8080", Path: "/ws"}
    c, _, err := websocket.DefaultDialer.Dial(u.String(), nil)
    if err != nil {
        log.Fatal("dial:", err)
    }
    defer c.Close()

    err = c.WriteMessage(websocket.TextMessage, []byte("hello"))
    if err != nil {
        log.Fatal("write:", err)
    }

    _, message, err := c.ReadMessage()
    if err != nil {
        log.Fatal("read:", err)
    }
    log.Printf("Received: %s", message)
}
```

```go
// Concurrent read/write with goroutines
func handleConn(conn *websocket.Conn) {
    defer conn.Close()
    done := make(chan struct{})

    go func() {
        defer close(done)
        for {
            _, message, err := conn.ReadMessage()
            if err != nil {
                return
            }
            log.Printf("recv: %s", message)
        }
    }()

    ticker := time.NewTicker(30 * time.Second)
    defer ticker.Stop()

    for {
        select {
        case <-done:
            return
        case t := <-ticker.C:
            err := conn.WriteMessage(websocket.TextMessage, []byte(t.String()))
            if err != nil {
                log.Println("write:", err)
                return
            }
        }
    }
}
```
