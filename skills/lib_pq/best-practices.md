# Best Practices

### Connection Pool Configuration
```go
db, err := sql.Open("postgres", "host=localhost dbname=mydb pool_max_conns=25 pool_min_conns=5")
if err != nil {
    log.Fatal(err)
}
defer db.Close()

// Configure pool settings
db.SetMaxOpenConns(25)
db.SetMaxIdleConns(5)
db.SetConnMaxLifetime(30 * time.Minute)
db.SetConnMaxIdleTime(5 * time.Minute)
```

### Structured Error Handling
```go
func handleDBError(err error) error {
    if err == nil {
        return nil
    }

    // Check for specific PostgreSQL error codes
    if pqErr := pq.As(err, pqerror.UniqueViolation); pqErr != nil {
        return fmt.Errorf("duplicate entry: %s", pqErr.ErrorWithDetail())
    }
    if pqErr := pq.As(err, pqerror.ForeignKeyViolation); pqErr != nil {
        return fmt.Errorf("referenced record not found: %s", pqErr.ErrorWithDetail())
    }
    if pqErr := pq.As(err, pqerror.SerializationFailure); pqErr != nil {
        return fmt.Errorf("transaction conflict, retry: %s", pqErr.Message)
    }

    return fmt.Errorf("database error: %w", err)
}
```

### Connection String Best Practices
```go
// Use environment variables for configuration
func createDB() (*sql.DB, error) {
    dsn := fmt.Sprintf(
        "host=%s port=%s user=%s dbname=%s sslmode=%s connect_timeout=%s",
        getEnvOrDefault("DB_HOST", "localhost"),
        getEnvOrDefault("DB_PORT", "5432"),
        getEnvOrDefault("DB_USER", "postgres"),
        getEnvOrDefault("DB_NAME", "mydb"),
        getEnvOrDefault("DB_SSLMODE", "require"),
        getEnvOrDefault("DB_TIMEOUT", "10"),
    )

    db, err := sql.Open("postgres", dsn)
    if err != nil {
        return nil, fmt.Errorf("failed to open database: %w", err)
    }

    // Verify connection
    ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
    defer cancel()
    if err := db.PingContext(ctx); err != nil {
        db.Close()
        return nil, fmt.Errorf("failed to ping database: %w", err)
    }

    return db, nil
}

func getEnvOrDefault(key, defaultVal string) string {
    if val := os.Getenv(key); val != "" {
        return val
    }
    return defaultVal
}
```

### Health Check Pattern
```go
type DBHealth struct {
    db *sql.DB
}

func (h *DBHealth) Check(ctx context.Context) error {
    conn, err := h.db.Conn(ctx)
    if err != nil {
        return fmt.Errorf("failed to get connection: %w", err)
    }
    defer conn.Close()

    var result int
    if err := conn.QueryRowContext(ctx, "SELECT 1").Scan(&result); err != nil {
        return fmt.Errorf("health check query failed: %w", err)
    }
    if result != 1 {
        return fmt.Errorf("unexpected health check result: %d", result)
    }
    return nil
}
```

### Retry Logic for Transient Errors
```go
func executeWithRetry(ctx context.Context, db *sql.DB, query string, args ...interface{}) error {
    maxRetries := 3
    baseDelay := 100 * time.Millisecond

    for attempt := 0; attempt < maxRetries; attempt++ {
        err := db.QueryRowContext(ctx, query, args...).Scan()
        if err == nil {
            return nil
        }

        // Only retry on serialization failures or connection errors
        if pqErr := pq.As(err, pqerror.SerializationFailure); pqErr == nil {
            if !isConnectionError(err) {
                return err
            }
        }

        if attempt < maxRetries-1 {
            delay := baseDelay * time.Duration(1<<uint(attempt))
            select {
            case <-ctx.Done():
                return ctx.Err()
            case <-time.After(delay):
            }
        }
    }
    return fmt.Errorf("max retries exceeded: %w", err)
}

func isConnectionError(err error) bool {
    return strings.Contains(err.Error(), "connection") ||
        strings.Contains(err.Error(), "closed network")
}
```