### Goroutine Safety with Gin Context
```go
// NEVER use gin.Context in goroutines without proper handling
// BAD: Using context in goroutine directly
func badHandler(c *gin.Context) {
    go func() {
        // This will panic when accessing c after handler returns
        c.JSON(200, gin.H{"result": "async"})
    }()
}

// GOOD: Copy context values or use channels
func goodHandler(c *gin.Context) {
    result := make(chan string)
    
    go func() {
        // Do async work
        time.Sleep(100 * time.Millisecond)
        result <- "async result"
    }()
    
    select {
    case res := <-result:
        c.JSON(200, gin.H{"result": res})
    case <-c.Request.Context().Done():
        c.JSON(499, gin.H{"error": "client disconnected"})
    }
}
```

### Concurrent Request Processing
```go
// Use sync.WaitGroup for parallel processing
func parallelHandler(c *gin.Context) {
    var wg sync.WaitGroup
    results := make([]string, 3)
    
    for i := 0; i < 3; i++ {
        wg.Add(1)
        go func(index int) {
            defer wg.Done()
            results[index] = fmt.Sprintf("result-%d", index)
        }(i)
    }
    
    wg.Wait()
    c.JSON(200, gin.H{"results": results})
}

// Use errgroup for error handling
func parallelWithErrors(c *gin.Context) {
    g, ctx := errgroup.WithContext(c.Request.Context())
    
    results := make([]string, 3)
    
    for i := 0; i < 3; i++ {
        i := i // Capture loop variable
        g.Go(func() error {
            select {
            case <-ctx.Done():
                return ctx.Err()
            default:
                results[i] = fmt.Sprintf("result-%d", i)
                return nil
            }
        })
    }
    
    if err := g.Wait(); err != nil {
        c.JSON(500, gin.H{"error": err.Error()})
        return
    }
    
    c.JSON(200, gin.H{"results": results})
}
```

### Mutex Protection for Shared State
```go
type CounterHandler struct {
    mu      sync.Mutex
    counter int
}

func (h *CounterHandler) Increment(c *gin.Context) {
    h.mu.Lock()
    h.counter++
    h.mu.Unlock()
    
    c.JSON(200, gin.H{"counter": h.counter})
}

// Use RWMutex for read-heavy workloads
type CacheHandler struct {
    mu    sync.RWMutex
    cache map[string]interface{}
}

func (h *CacheHandler) Get(c *gin.Context) {
    key := c.Param("key")
    
    h.mu.RLock()
    value, exists := h.cache[key]
    h.mu.RUnlock()
    
    if !exists {
        c.JSON(404, gin.H{"error": "not found"})
        return
    }
    
    c.JSON(200, value)
}

func (h *CacheHandler) Set(c *gin.Context) {
    key := c.Param("key")
    var value interface{}
    c.ShouldBindJSON(&value)
    
    h.mu.Lock()
    h.cache[key] = value
    h.mu.Unlock()
    
    c.JSON(200, gin.H{"status": "set"})
}
```

### Channel Patterns for Async Processing
```go
// Fan-out pattern for parallel processing
func fanOutHandler(c *gin.Context) {
    jobs := make(chan int, 10)
    results := make(chan string, 10)
    
    // Start workers
    var wg sync.WaitGroup
    for w := 0; w < 3; w++ {
        wg.Add(1)
        go func() {
            defer wg.Done()
            for job := range jobs {
                results <- fmt.Sprintf("processed-%d", job)
            }
        }()
    }
    
    // Send jobs
    go func() {
        for i := 0; i < 10; i++ {
            jobs <- i
        }
        close(jobs)
    }()
    
    // Wait for completion
    go func() {
        wg.Wait()
        close(results)
    }()
    
    // Collect results
    var processed []string
    for result := range results {
        processed = append(processed, result)
    }
    
    c.JSON(200, gin.H{"results": processed})
}
```
