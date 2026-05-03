# Safety

### Red Line 1: Never ignore sql.Open() errors
```go
// BAD: Ignoring error from sql.Open
db, _ := sql.Open("postgres", "invalid://")
// db is nil, will panic on any operation
db.Ping() // PANIC: runtime error: invalid memory address
```

```go
// GOOD: Always check errors
db, err := sql.Open("postgres", "host=localhost dbname=mydb")
if err != nil {
    log.Fatalf("Failed to create connection pool: %v", err)
}
defer db.Close()
```

### Red Line 2: Never use LastInsertId with PostgreSQL
```go
// BAD: Relying on unsupported LastInsertId
result, _ := db.Exec("INSERT INTO users (name) VALUES ('Alice')")
id, _ := result.LastInsertId()
// id is always 0, no error returned
// This silently breaks your application logic
```

```go
// GOOD: Use RETURNING clause
var id int64
err := db.QueryRow(
    "INSERT INTO users (name) VALUES ($1) RETURNING id",
    "Alice",
).Scan(&id)
if err != nil {
    log.Fatal(err)
}
// id now contains the actual database-generated ID
```

### Red Line 3: Never leave transactions uncommitted or unrolled
```go
// BAD: Transaction leak
func updateBalance(db *sql.DB, id int, amount float64) error {
    tx, _ := db.Begin()
    _, err := tx.Exec("UPDATE accounts SET balance = $1 WHERE id = $2", amount, id)
    if err != nil {
        return err // Transaction remains open!
    }
    return tx.Commit()
}
// Multiple calls will exhaust connection pool
```

```go
// GOOD: Always defer rollback
func updateBalance(db *sql.DB, id int, amount float64) error {
    tx, err := db.Begin()
    if err != nil {
        return err
    }
    defer tx.Rollback() // Safe: no-op if committed

    _, err = tx.Exec("UPDATE accounts SET balance = $1 WHERE id = $2", amount, id)
    if err != nil {
        return err
    }
    return tx.Commit()
}
```

### Red Line 4: Never use context.Background() for database operations in production
```go
// BAD: No timeout context
rows, _ := db.QueryContext(context.Background(), "SELECT pg_sleep(100)")
// This query will block for 100 seconds with no way to cancel
```

```go
// GOOD: Always use context with timeout
ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
defer cancel()

rows, err := db.QueryContext(ctx, "SELECT pg_sleep(100)")
if err != nil {
    log.Printf("Query failed (expected after timeout): %v", err)
    return
}
defer rows.Close()
```

### Red Line 5: Never use string interpolation for query parameters
```go
// BAD: SQL injection vulnerability
username := "'; DROP TABLE users; --"
query := fmt.Sprintf("SELECT * FROM users WHERE name = '%s'", username)
rows, _ := db.Query(query) // Drops the users table!
```

```go
// GOOD: Always use parameterized queries
username := "'; DROP TABLE users; --"
rows, err := db.Query("SELECT * FROM users WHERE name = $1", username)
if err != nil {
    log.Fatal(err)
}
defer rows.Close()
// Safe: username is treated as a literal string
```