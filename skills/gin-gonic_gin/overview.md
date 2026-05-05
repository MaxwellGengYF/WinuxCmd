Gin is a high-performance HTTP web framework for Go that provides a Martini-like API with significantly better performance—up to 40 times faster—thanks to its zero-allocation router based on httprouter. It's designed for building REST APIs, web applications, and microservices where speed and developer productivity are essential.

**When to use Gin:**
- Building high-throughput REST APIs that need to handle many concurrent requests
- Developing microservices requiring fast response times and low memory overhead
- Creating web applications that need middleware support for authentication, logging, CORS
- Prototyping web services quickly with minimal boilerplate code

**When NOT to use Gin:**
- When you need full MVC framework features (consider Revel or Buffalo)
- For extremely simple static file servers (net/http is sufficient)
- When you need WebSocket support out of the box (use gorilla/websocket separately)
- For projects requiring GraphQL-first design (use gqlgen directly)

**Key design principles:**
- Zero-allocation router for maximum performance
- Middleware chain pattern for request processing
- Context-based request handling with built-in binding and validation
- Route grouping for organizing endpoints with shared middleware

**Installation:**
```bash
go get github.com/gin-gonic/gin@v1.12.0
```

**Basic setup:**
```go
package main

import "github.com/gin-gonic/gin"

func main() {
    r := gin.Default() // Creates router with Logger and Recovery middleware
    r.GET("/health", func(c *gin.Context) {
        c.JSON(200, gin.H{"status": "ok"})
    })
    r.Run() // Default listens on :8080
}
```
