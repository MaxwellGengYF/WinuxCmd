### Project Structure
```go
// Recommended project layout
// handlers/
//   user_handler.go
// middleware/
//   auth.go
// models/
//   user.go
// routes/
//   router.go
// main.go

package main

import (
    "github.com/gin-gonic/gin"
    "myapp/handlers"
    "myapp/middleware"
    "myapp/routes"
)

func main() {
    r := gin.New()
    r.Use(gin.LoggerWithWriter(log.Writer()))
    r.Use(gin.Recovery())
    r.Use(middleware.CORS())
    
    routes.SetupRoutes(r)
    
    srv := &http.Server{
        Addr:         ":8080",
        Handler:      r,
        ReadTimeout:  10 * time.Second,
        WriteTimeout: 10 * time.Second,
    }
    
    if err := srv.ListenAndServe(); err != nil {
        log.Fatalf("Server failed: %v", err)
    }
}
```

### Dependency Injection
```go
// Use dependency injection for testability
type UserHandler struct {
    db     *sql.DB
    logger *log.Logger
}

func NewUserHandler(db *sql.DB, logger *log.Logger) *UserHandler {
    return &UserHandler{db: db, logger: logger}
}

func (h *UserHandler) GetUser(c *gin.Context) {
    id := c.Param("id")
    user, err := h.getUserFromDB(id)
    if err != nil {
        h.logger.Printf("Error fetching user %s: %v", id, err)
        c.JSON(500, gin.H{"error": "internal server error"})
        return
    }
    c.JSON(200, user)
}
```

### Request Validation
```go
// Use struct tags for validation
type CreateUserRequest struct {
    Name     string `json:"name" binding:"required,min=2,max=100"`
    Email    string `json:"email" binding:"required,email"`
    Age      int    `json:"age" binding:"required,gte=0,lte=150"`
    Password string `json:"password" binding:"required,min=8"`
}

func CreateUser(c *gin.Context) {
    var req CreateUserRequest
    if err := c.ShouldBindJSON(&req); err != nil {
        c.JSON(400, gin.H{"error": err.Error()})
        return
    }
    // Process valid request
    c.JSON(201, gin.H{"status": "created"})
}
```

### Graceful Shutdown
```go
func main() {
    r := gin.Default()
    
    srv := &http.Server{
        Addr:    ":8080",
        Handler: r,
    }
    
    go func() {
        if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
            log.Fatalf("listen: %s\n", err)
        }
    }()
    
    // Wait for interrupt signal
    quit := make(chan os.Signal, 1)
    signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
    <-quit
    
    log.Println("Shutting down server...")
    
    ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
    defer cancel()
    
    if err := srv.Shutdown(ctx); err != nil {
        log.Fatal("Server forced to shutdown:", err)
    }
    
    log.Println("Server exiting")
}
```
