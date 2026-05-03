# Pitfalls

### Pitfall 1: Not checking Ping() after sql.Open()
```go
// BAD: sql.Open() doesn't actually connect
db, _ := sql.Open("postgres", "invalid_connection_string")
// No error here! Connection attempt happens lazily
db.Query("SELECT 1") // Panics or hangs here
```

```go
// GOOD: Always verify connection
db, err := sql.Open("postgres", "host=localhost dbname=mydb connect_timeout=5")
if err != nil {
    log.Fatal(err)
}
defer db.Close()

if err := db.Ping(); err != nil {
    log.Fatal(err) // Catches connection issues early
}
```

### Pitfall 2: Forgetting connect_timeout
```go
// BAD: No timeout - can hang indefinitely
db, _ := sql.Open("postgres", "host=unreachable-host dbname=mydb")
db.Ping() // Blocks forever
```

```go
// GOOD: Always set connect_timeout
db, err := sql.Open("postgres", "host=unreachable-host dbname=mydb connect_timeout=10")
if err != nil {
    log.Fatal(err)
}
defer db.Close()

ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
defer cancel()
if err := db.PingContext(ctx); err != nil {
    log.Fatal(err) // Fails after 5 seconds
}
```

### Pitfall 3: Using LastInsertId
```go
// BAD: LastInsertId not supported
result, _ := db.Exec("INSERT INTO users (name) VALUES ('Alice')")
id, _ := result.LastInsertId() // Returns 0, no error
fmt.Printf("ID: %d\n", id) // Always prints 0
```

```go
// GOOD: Use RETURNING clause
var id int
err := db.QueryRow(
    "INSERT INTO users (name) VALUES ($1) RETURNING id",
    "Alice",
).Scan(&id)
if err != nil {
    log.Fatal(err)
}
fmt.Printf("ID: %d\n", id) // Actual database-generated ID
```

### Pitfall 4: Ignoring transaction rollback
```go
// BAD: No rollback on error
tx, _ := db.Begin()
_, err := tx.Exec("UPDATE accounts SET balance = balance - 100 WHERE id = 1")
if err != nil {
    return err // Transaction left open!
}
tx.Commit()
```

```go
// GOOD: Always defer rollback
tx, err := db.Begin()
if err != nil {
    return err
}
defer tx.Rollback() // No-op if committed, rolls back on error

_, err = tx.Exec("UPDATE accounts SET balance = balance - 100 WHERE id = 1")
if err != nil {
    return err
}
return tx.Commit()
```

### Pitfall 5: Not closing rows
```go
// BAD: Rows not closed
rows, _ := db.Query("SELECT id FROM users")
for rows.Next() {
    var id int
    rows.Scan(&id)
    // Process
}
// Connection not returned to pool until rows.Close() is called
```

```go
// GOOD: Always close rows
rows, err := db.Query("SELECT id FROM users")
if err != nil {
    log.Fatal(err)
}
defer rows.Close()

for rows.Next() {
    var id int
    if err := rows.Scan(&id); err != nil {
        log.Fatal(err)
    }
}
if err := rows.Err(); err != nil {
    log.Fatal(err)
}
```

### Pitfall 6: Incorrect timestamp handling
```go
// BAD: Assuming timestamp without timezone is UTC
var t time.Time
db.QueryRow("SELECT timestamp_col FROM events WHERE id = 1").Scan(&t)
fmt.Println(t.Location()) // Prints +0000, but it's NOT UTC
```

```go
// GOOD: Handle timestamps explicitly
var t time.Time
db.QueryRow("SELECT timestamp_col AT TIME ZONE 'UTC' FROM events WHERE id = 1").Scan(&t)
// Or use timestamptz column type
db.QueryRow("SELECT timestamptz_col FROM events WHERE id = 1").Scan(&t)
```

### Pitfall 7: Not handling COPY flush
```go
// BAD: Missing final Exec() to flush COPY data
stmt, _ := tx.Prepare(pq.CopyIn("users", "name"))
stmt.Exec("Alice")
stmt.Exec("Bob")
stmt.Close() // Data not flushed!
```

```go
// GOOD: Always call Exec() with no args to flush
stmt, err := tx.Prepare(pq.CopyIn("users", "name"))
if err != nil {
    log.Fatal(err)
}

stmt.Exec("Alice")
stmt.Exec("Bob")

_, err = stmt.Exec() // Flush remaining data
if err != nil {
    log.Fatal(err)
}
stmt.Close()
```

### Pitfall 8: Using []byte for jsonb in COPY
```go
// BAD: []byte becomes bytea in COPY
stmt, _ := tx.Prepare(pq.CopyIn("events", "data"))
jsonData := []byte(`{"key": "value"}`)
stmt.Exec(jsonData) // Error: jsonb column expects JSON, not bytea
```

```go
// GOOD: Use string for jsonb columns in COPY
stmt, err := tx.Prepare(pq.CopyIn("events", "data"))
if err != nil {
    log.Fatal(err)
}

jsonData := `{"key": "value"}`
stmt.Exec(jsonData) // Works correctly with jsonb
```