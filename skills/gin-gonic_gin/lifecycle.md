### Construction and Initialization
```go
package main

import (
    "context"
    "log"
    "net/http"
    "os"
    "os/signal"
    "syscall"
    "time"
    
    "github.com/gin-gonic/gin"
)

type App struct {
    router *gin.Engine
    db     *sql.DB
    cache  *redis.Client
}

func NewApp() *App {
    // Initialize database connection
    db, err := sql.Open("postgres", os.Getenv("DATABASE_URL"))
    if err != nil {
        log.Fatalf("Failed to connect to database: %v", err)
    }
    
    // Initialize cache
    cache := redis.NewClient(&redis.Options{
        Addr: os.Getenv("REDIS_URL"),
    })
    
    // Create router with middleware
    r := gin.New()
    r.Use(gin.LoggerWithWriter(log.Writer()))
    r.Use(gin.Recovery())
    
    return &App{
        router: r,
        db:     db,
        cache:  cache,
    }
}

func (a *App) SetupRoutes() {
    a.router.GET("/health", func(c *gin.Context) {
        c.JSON(200, gin.H{"status": "ok"})
    })
    
    api := a.router.Group("/api")
    {
        api.GET("/users", a.GetUsers)
        api.POST("/users", a.CreateUser)
    }
}

func (a *App) Run() {
    srv := &http.Server{
        Addr:         ":8080",
        Handler:      a.router,
        ReadTimeout:  15 * time.Second,
        WriteTimeout: 15 * time.Second,
        IdleTimeout:  60 * time.Second,
    }
    
    // Start server in goroutine
    go func() {
        log.Printf("Server starting on :8080")
        if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
            log.Fatalf("Server failed: %v", err)
        }
    }()
    
    // Wait for shutdown signal
    quit := make(chan os.Signal, 1)
    signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
    <-quit
    
    log.Println("Shutting down server...")
    
    // Graceful shutdown with timeout
    ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
    defer cancel()
    
    if err := srv.Shutdown(ctx); err != nil {
        log.Fatalf("Server forced to shutdown: %v", err)
    }
    
    // Cleanup resources
    a.db.Close()
    a.cache.Close()
    
    log.Println("Server exited")
}
```

### Resource Management
```go
// Proper cleanup of middleware resources
func RateLimiterMiddleware() gin.HandlerFunc {
    limiter := rate.NewLimiter(100, 200) // 100 requests per second, burst 200
    
    return func(c *gin.Context) {
        if !limiter.Allow() {
            c.AbortWithStatusJSON(429, gin.H{"error": "too many requests"})
            return
        }
        c.Next()
    }
}

// Database connection pooling
func DatabaseMiddleware(db *sql.DB) gin.HandlerFunc {
    db.SetMaxOpenConns(25)
    db.SetMaxIdleConns(5)
    db.SetConnMaxLifetime(5 * time.Minute)
    
    return func(c *gin.Context) {
        c.Set("db", db)
        c.Next()
    }
}
```
