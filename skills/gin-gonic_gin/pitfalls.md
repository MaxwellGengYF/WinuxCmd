### Pitfall 1: Not handling binding errors
```go
// BAD: Ignoring binding errors
func badHandler(c *gin.Context) {
    var user User
    c.ShouldBindJSON(&user) // Error ignored
    c.JSON(200, user) // May return partial/invalid data
}

// GOOD: Always check binding errors
func goodHandler(c *gin.Context) {
    var user User
    if err := c.ShouldBindJSON(&user); err != nil {
        c.JSON(400, gin.H{"error": err.Error()})
        return
    }
    c.JSON(200, user)
}
```

### Pitfall 2: Using gin.Default() in production without customization
```go
// BAD: Using default logger in production
func badSetup() {
    r := gin.Default() // Logs to stdout, no customization
    r.Run()
}

// GOOD: Customize logging and recovery
func goodSetup() {
    r := gin.New()
    r.Use(gin.LoggerWithWriter(log.Writer()))
    r.Use(gin.Recovery())
    r.Run()
}
```

### Pitfall 3: Not setting request timeouts
```go
// BAD: No timeout on long-running requests
func badHandler(c *gin.Context) {
    result := slowDatabaseQuery() // Could hang forever
    c.JSON(200, result)
}

// GOOD: Use context with timeout
func goodHandler(c *gin.Context) {
    ctx, cancel := context.WithTimeout(c.Request.Context(), 5*time.Second)
    defer cancel()
    
    result, err := slowDatabaseQueryWithContext(ctx)
    if err != nil {
        c.JSON(500, gin.H{"error": "request timed out"})
        return
    }
    c.JSON(200, result)
}
```

### Pitfall 4: Exposing internal errors to clients
```go
// BAD: Returning raw error messages
func badHandler(c *gin.Context) {
    result, err := db.Query("SELECT ...")
    if err != nil {
        c.JSON(500, gin.H{"error": err.Error()}) // Exposes SQL details
        return
    }
    c.JSON(200, result)
}

// GOOD: Log internal errors, return generic messages
func goodHandler(c *gin.Context) {
    result, err := db.Query("SELECT ...")
    if err != nil {
        log.Printf("Database error: %v", err)
        c.JSON(500, gin.H{"error": "internal server error"})
        return
    }
    c.JSON(200, result)
}
```

### Pitfall 5: Not validating request parameters
```go
// BAD: Using parameters without validation
func badHandler(c *gin.Context) {
    id := c.Param("id")
    // id could be empty or malicious
    c.JSON(200, gin.H{"id": id})
}

// GOOD: Validate parameters
func goodHandler(c *gin.Context) {
    id := c.Param("id")
    if id == "" {
        c.JSON(400, gin.H{"error": "id is required"})
        return
    }
    // Additional validation as needed
    c.JSON(200, gin.H{"id": id})
}
```

### Pitfall 6: Not using proper HTTP status codes
```go
// BAD: Always returning 200
func badHandler(c *gin.Context) {
    if err := processRequest(); err != nil {
        c.JSON(200, gin.H{"error": err.Error()}) // Wrong status code
        return
    }
    c.JSON(200, gin.H{"success": true})
}

// GOOD: Use appropriate status codes
func goodHandler(c *gin.Context) {
    if err := processRequest(); err != nil {
        c.JSON(400, gin.H{"error": err.Error()})
        return
    }
    c.JSON(201, gin.H{"success": true}) // Created
}
```

### Pitfall 7: Not handling CORS properly
```go
// BAD: Allowing all origins without restrictions
func badSetup() {
    r := gin.Default()
    r.Use(func(c *gin.Context) {
        c.Header("Access-Control-Allow-Origin", "*")
        c.Next()
    })
    r.Run()
}

// GOOD: Restrict CORS to specific origins
func goodSetup() {
    r := gin.Default()
    r.Use(func(c *gin.Context) {
        origin := c.Request.Header.Get("Origin")
        allowedOrigins := map[string]bool{
            "https://myapp.com": true,
            "https://admin.myapp.com": true,
        }
        if allowedOrigins[origin] {
            c.Header("Access-Control-Allow-Origin", origin)
        }
        c.Next()
    })
    r.Run()
}
```

### Pitfall 8: Not closing response body in custom middleware
```go
// BAD: Not closing response body
func badMiddleware() gin.HandlerFunc {
    return func(c *gin.Context) {
        resp, _ := http.Get("https://api.example.com")
        // resp.Body never closed - memory leak
        c.Next()
    }
}

// GOOD: Always close response bodies
func goodMiddleware() gin.HandlerFunc {
    return func(c *gin.Context) {
        resp, err := http.Get("https://api.example.com")
        if err != nil {
            c.AbortWithStatusJSON(500, gin.H{"error": "upstream failed"})
            return
        }
        defer resp.Body.Close()
        c.Next()
    }
}
```
