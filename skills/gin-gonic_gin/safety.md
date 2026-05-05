### Red Line 1: NEVER use `gin.Default()` in production without customizing recovery
```go
// BAD: Default recovery logs to stdout and may expose stack traces
func badSetup() {
    r := gin.Default() // Stack traces printed to stdout
    r.Run()
}

// GOOD: Custom recovery with proper logging
func goodSetup() {
    r := gin.New()
    r.Use(gin.LoggerWithWriter(log.Writer()))
    r.Use(gin.CustomRecovery(func(c *gin.Context, recovered interface{}) {
        log.Printf("Panic recovered: %v", recovered)
        c.AbortWithStatusJSON(500, gin.H{"error": "internal server error"})
    }))
    r.Run()
}
```

### Red Line 2: NEVER expose internal error details to clients
```go
// BAD: Returning raw database errors
func badHandler(c *gin.Context) {
    var user User
    if err := db.First(&user, c.Param("id")).Error; err != nil {
        c.JSON(500, gin.H{"error": err.Error()}) // Exposes SQL details
        return
    }
    c.JSON(200, user)
}

// GOOD: Log internally, return generic messages
func goodHandler(c *gin.Context) {
    var user User
    if err := db.First(&user, c.Param("id")).Error; err != nil {
        log.Printf("Database error for user %s: %v", c.Param("id"), err)
        c.JSON(500, gin.H{"error": "internal server error"})
        return
    }
    c.JSON(200, user)
}
```

### Red Line 3: NEVER use `gin.H` for sensitive data
```go
// BAD: Including sensitive data in responses
func badHandler(c *gin.Context) {
    user := User{Password: "secret123", Email: "user@example.com"}
    c.JSON(200, gin.H{"user": user}) // Exposes password
}

// GOOD: Use structs with JSON tags to control output
type UserResponse struct {
    Email string `json:"email"`
    Name  string `json:"name"`
}

func goodHandler(c *gin.Context) {
    user := UserResponse{Email: "user@example.com", Name: "John"}
    c.JSON(200, user)
}
```

### Red Line 4: NEVER ignore request context cancellation
```go
// BAD: Not checking context cancellation
func badHandler(c *gin.Context) {
    result := make(chan string)
    go func() {
        result <- slowOperation()
    }()
    select {
    case res := <-result:
        c.JSON(200, gin.H{"result": res})
    }
}

// GOOD: Respect context cancellation
func goodHandler(c *gin.Context) {
    result := make(chan string)
    go func() {
        result <- slowOperation()
    }()
    select {
    case res := <-result:
        c.JSON(200, gin.H{"result": res})
    case <-c.Request.Context().Done():
        c.JSON(499, gin.H{"error": "client disconnected"})
    }
}
```

### Red Line 5: NEVER use `r.Run()` without error handling
```go
// BAD: Ignoring server startup errors
func badSetup() {
    r := gin.Default()
    r.Run(":8080") // Error ignored
}

// GOOD: Always handle server errors
func goodSetup() {
    r := gin.Default()
    if err := r.Run(":8080"); err != nil {
        log.Fatalf("Server failed to start: %v", err)
    }
}
```
