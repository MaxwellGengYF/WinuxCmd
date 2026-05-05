```go
// Install Gin
// go get github.com/gin-gonic/gin@v1.12.0
```

```go
package main

import (
    "net/http"
    "github.com/gin-gonic/gin"
)

func main() {
    r := gin.Default()
    
    // Pattern 1: Basic GET endpoint
    r.GET("/ping", func(c *gin.Context) {
        c.JSON(http.StatusOK, gin.H{"message": "pong"})
    })
    
    // Pattern 2: Path parameters
    r.GET("/users/:id", func(c *gin.Context) {
        id := c.Param("id")
        c.JSON(http.StatusOK, gin.H{"user_id": id})
    })
    
    // Pattern 3: Query parameters
    r.GET("/search", func(c *gin.Context) {
        query := c.Query("q")
        page := c.DefaultQuery("page", "1")
        c.JSON(http.StatusOK, gin.H{"query": query, "page": page})
    })
    
    // Pattern 4: POST with JSON binding
    r.POST("/users", func(c *gin.Context) {
        var user struct {
            Name  string `json:"name" binding:"required"`
            Email string `json:"email" binding:"required,email"`
        }
        if err := c.ShouldBindJSON(&user); err != nil {
            c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
            return
        }
        c.JSON(http.StatusCreated, gin.H{"user": user})
    })
    
    // Pattern 5: Route groups with middleware
    api := r.Group("/api")
    api.Use(gin.Logger())
    {
        api.GET("/items", func(c *gin.Context) {
            c.JSON(http.StatusOK, gin.H{"items": []string{"item1", "item2"}})
        })
        api.POST("/items", func(c *gin.Context) {
            c.JSON(http.StatusCreated, gin.H{"status": "created"})
        })
    }
    
    // Pattern 6: Static file serving
    r.Static("/static", "./public")
    
    // Pattern 7: HTML template rendering
    r.LoadHTMLGlob("templates/*")
    r.GET("/index", func(c *gin.Context) {
        c.HTML(http.StatusOK, "index.tmpl", gin.H{"title": "Gin Demo"})
    })
    
    // Pattern 8: Custom middleware
    r.Use(func(c *gin.Context) {
        c.Header("X-Custom-Header", "value")
        c.Next()
    })
    
    r.Run(":8080")
}
```
