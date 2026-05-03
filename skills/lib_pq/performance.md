# Performance

### Connection Pool Optimization
```go
// Optimize pool settings based on workload
func configurePool(db *sql.DB) {
    // For read-heavy workloads
    db.SetMaxOpenConns(50)      // Maximum concurrent connections
    db.SetMaxIdleConns(25)      // Keep connections warm
    db.SetConnMaxLifetime(30 * time.Minute) // Refresh connections periodically
    db.SetConnMaxIdleTime(5 * time.Minute)  // Close idle connections

    // For write-heavy workloads
    db.SetMaxOpenConns(100)
    db.SetMaxIdleConns(10)      // Fewer idle connections
    db.SetConnMaxLifetime(15 * time.Minute) // More frequent refresh
}
```

### Batch Operations with COPY
```go
// BAD: Individual inserts (slow)
func insertUsersSlow(db *sql.DB, users []User) {
    for _, u := range users {
        _, err := db.Exec("INSERT INTO users (name, email) VALUES ($1, $2)", u.Name, u.Email)
        if err != nil {
            log.Fatal(err)
        }
    }
}

// GOOD: Bulk insert with COPY (fast)
func insertUsersFast(db *sql.DB, users []User) error {
    tx, err := db.Begin()
    if err != nil {
        return err
    }
    defer tx.Rollback()

    stmt, err := tx.Prepare(pq.CopyIn("users", "name", "email"))
    if err != nil {
        return err
    }

    for _, u := range users {
        _, err = stmt.Exec(u.Name, u.Email)
        if err != nil {
            return err
        }
    }

    _, err = stmt.Exec() // Flush remaining data
    if err != nil {
        return err
    }

    err = stmt.Close()
    if err != nil {
        return err
    }

    return tx.Commit()
}
```

### Prepared Statement Caching
```go
// BAD: Preparing statements repeatedly
func getUserBad(db *sql.DB, id int) (User, error) {
    stmt, err := db.Prepare("SELECT name, email FROM users WHERE id = $1")
    if err != nil {
        return User{}, err
    }
    defer stmt.Close()

    var u User
    err = stmt.QueryRow(id).Scan(&u.Name, &u.Email)
    return u, err
}

// GOOD: Prepare once, reuse many times
type UserRepository struct {
    db    *sql.DB
    stmt  *sql.Stmt
    once  sync.Once
    err   error
}

func (r *UserRepository) getStmt() (*sql.Stmt, error) {
    r.once.Do(func() {
        r.stmt, r.err = r.db.Prepare("SELECT name, email FROM users WHERE id = $1")
    })
    return r.stmt, r.err
}

func (r *UserRepository) GetUser(id int) (User, error) {
    stmt, err := r.getStmt()
    if err != nil {
        return User{}, err
    }

    var u User
    err = stmt.QueryRow(id).Scan(&u.Name, &u.Email)
    return u, err
}
```

### Query Optimization Tips
```go
// Use specific columns instead of SELECT *
// BAD: SELECT * fetches all columns
rows, _ := db.Query("SELECT * FROM users WHERE id = $1", id)

// GOOD: Select only needed columns
rows, _ := db.Query("SELECT id, name, email FROM users WHERE id = $1", id)

// Use LIMIT for pagination
// BAD: Fetching all rows
rows, _ := db.Query("SELECT id, name FROM users")

// GOOD: Paginated queries
rows, _ := db.Query("SELECT id, name FROM users ORDER BY id LIMIT $1 OFFSET $2", limit, offset)

// Use EXISTS instead of COUNT for existence checks
// BAD: Counting all matching rows
var count int
db.QueryRow("SELECT COUNT(*) FROM users WHERE email = $1", email).Scan(&count)
exists := count > 0

// GOOD: Using EXISTS (stops at first match)
var exists bool
db.QueryRow("SELECT EXISTS(SELECT 1 FROM users WHERE email = $1)", email).Scan(&exists)
```