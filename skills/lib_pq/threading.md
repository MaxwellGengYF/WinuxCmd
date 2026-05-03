# Threading

### Thread Safety Guarantees
```go
// sql.DB is safe for concurrent use by multiple goroutines
func concurrentAccess(db *sql.DB) {
    var wg sync.WaitGroup
    for i := 0; i < 10; i++ {
        wg.Add(1)
        go func(id int) {
            defer wg.Done()
            // Each goroutine can safely use the same db instance
            var name string
            err := db.QueryRow("SELECT name FROM users WHERE id = $1", id).Scan(&name)
            if err != nil {
                log.Printf("Error for id %d: %v", id, err)
                return
            }
            log.Printf("User %d: %s", id, name)
        }(i)
    }
    wg.Wait()
}
```

### Connection-Level Operations
```go
// sql.Conn is NOT safe for concurrent use
func unsafeConnUsage(db *sql.DB) {
    ctx := context.Background()
    conn, err := db.Conn(ctx)
    if err != nil {
        log.Fatal(err)
    }
    defer conn.Close()

    // BAD: Concurrent use of same connection
    go func() {
        conn.QueryRowContext(ctx, "SELECT 1") // Race condition!
    }()
    go func() {
        conn.QueryRowContext(ctx, "SELECT 2") // Race condition!
    }()
}

// GOOD: Use separate connections or synchronize
func safeConnUsage(db *sql.DB) {
    var wg sync.WaitGroup
    for i := 0; i < 2; i++ {
        wg.Add(1)
        go func() {
            defer wg.Done()
            conn, err := db.Conn(context.Background())
            if err != nil {
                log.Fatal(err)
            }
            defer conn.Close()
            conn.QueryRowContext(context.Background(), "SELECT 1")
        }()
    }
    wg.Wait()
}
```

### Transaction Thread Safety
```go
// Transactions are NOT safe for concurrent use
func unsafeTransaction(tx *sql.Tx) {
    // BAD: Concurrent operations on same transaction
    go func() {
        tx.Exec("UPDATE accounts SET balance = 100 WHERE id = 1")
    }()
    go func() {
        tx.Exec("UPDATE accounts SET balance = 200 WHERE id = 1") // Race!
    }()
}

// GOOD: Sequential operations within transaction
func safeTransaction(db *sql.DB) error {
    tx, err := db.Begin()
    if err != nil {
        return err
    }
    defer tx.Rollback()

    // Sequential operations
    _, err = tx.Exec("UPDATE accounts SET balance = 100 WHERE id = 1")
    if err != nil {
        return err
    }

    _, err = tx.Exec("UPDATE accounts SET balance = 200 WHERE id = 2")
    if err != nil {
        return err
    }

    return tx.Commit()
}
```

### LISTEN/NOTIFY Thread Safety
```go
// pq.Listener is safe for concurrent use
func listenerThreadSafety() {
    l := pq.NewListener("dbname=mydb", time.Second, time.Minute, nil)
    defer l.Close()

    // Multiple goroutines can listen to notifications
    go func() {
        for n := range l.Notify {
            if n == nil {
                return
            }
            fmt.Printf("Goroutine 1: %s\n", n.Extra)
        }
    }()

    go func() {
        for n := range l.Notify {
            if n == nil {
                return
            }
            fmt.Printf("Goroutine 2: %s\n", n.Extra)
        }
    }()

    // Listen is also thread-safe
    go func() {
        l.Listen("channel1")
    }()
    go func() {
        l.Listen("channel2")
    }()
}
```

### Context-Based Cancellation
```go
// Proper context usage for cancellation
func cancellableOperation(db *sql.DB) {
    ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
    defer cancel()

    // This query will be cancelled after 5 seconds
    rows, err := db.QueryContext(ctx, "SELECT pg_sleep(10)")
    if err != nil {
        log.Printf("Query cancelled: %v", err) // Expected after timeout
        return
    }
    defer rows.Close()
}

// Multiple concurrent queries with different contexts
func concurrentQueries(db *sql.DB) {
    var wg sync.WaitGroup
    queries := []struct {
        query string
        timeout time.Duration
    }{
        {"SELECT pg_sleep(1)", 2 * time.Second},
        {"SELECT pg_sleep(5)", 3 * time.Second}, // Will timeout
    }

    for _, q := range queries {
        wg.Add(1)
        go func(q struct{ query string; timeout time.Duration }) {
            defer wg.Done()
            ctx, cancel := context.WithTimeout(context.Background(), q.timeout)
            defer cancel()

            var result int
            err := db.QueryRowContext(ctx, q.query).Scan(&result)
            if err != nil {
                log.Printf("Query failed: %v", err)
                return
            }
            log.Printf("Result: %d", result)
        }(q)
    }
    wg.Wait()
}
```