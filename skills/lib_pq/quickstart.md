# Quickstart

```go
package main

import (
    "database/sql"
    "fmt"
    "log"
    "time"

    _ "github.com/lib/pq"
)

// Pattern 1: Basic connection with DSN
func basicConnection() {
    db, err := sql.Open("postgres", "host=localhost port=5432 user=postgres dbname=mydb sslmode=disable connect_timeout=5")
    if err != nil {
        log.Fatal(err)
    }
    defer db.Close()

    if err := db.Ping(); err != nil {
        log.Fatal(err)
    }
    fmt.Println("Connected!")
}

// Pattern 2: Using pq.Config for structured configuration
func configConnection() {
    cfg := &pq.Config{
        Host:           "localhost",
        Port:           5432,
        User:           "postgres",
        Database:       "mydb",
        ConnectTimeout: 5 * time.Second,
    }

    connector, err := pq.NewConnectorConfig(cfg)
    if err != nil {
        log.Fatal(err)
    }

    db := sql.OpenDB(connector)
    defer db.Close()

    if err := db.Ping(); err != nil {
        log.Fatal(err)
    }
}

// Pattern 3: Query with parameters
func queryWithParams(db *sql.DB) {
    rows, err := db.Query("SELECT id, name FROM users WHERE age > $1 AND active = $2", 18, true)
    if err != nil {
        log.Fatal(err)
    }
    defer rows.Close()

    for rows.Next() {
        var id int
        var name string
        if err := rows.Scan(&id, &name); err != nil {
            log.Fatal(err)
        }
        fmt.Printf("User: %d - %s\n", id, name)
    }
}

// Pattern 4: INSERT with RETURNING
func insertReturning(db *sql.DB) {
    var newID int
    err := db.QueryRow(
        "INSERT INTO users (name, email) VALUES ($1, $2) RETURNING id",
        "Alice", "alice@example.com",
    ).Scan(&newID)
    if err != nil {
        log.Fatal(err)
    }
    fmt.Printf("Inserted user with ID: %d\n", newID)
}

// Pattern 5: Bulk import with COPY
func bulkImport(db *sql.DB) {
    tx, err := db.Begin()
    if err != nil {
        log.Fatal(err)
    }
    defer tx.Rollback()

    stmt, err := tx.Prepare(pq.CopyIn("users", "name", "email"))
    if err != nil {
        log.Fatal(err)
    }

    for _, user := range []struct{ name, email string }{
        {"Bob", "bob@example.com"},
        {"Carol", "carol@example.com"},
    } {
        _, err = stmt.Exec(user.name, user.email)
        if err != nil {
            log.Fatal(err)
        }
    }

    _, err = stmt.Exec()
    if err != nil {
        log.Fatal(err)
    }

    err = stmt.Close()
    if err != nil {
        log.Fatal(err)
    }

    err = tx.Commit()
    if err != nil {
        log.Fatal(err)
    }
}

// Pattern 6: LISTEN/NOTIFY
func listenNotify() {
    l := pq.NewListener("dbname=mydb", time.Second, time.Minute, nil)
    defer l.Close()

    err := l.Listen("channel_name")
    if err != nil {
        log.Fatal(err)
    }

    go func() {
        for n := range l.Notify {
            if n == nil {
                return
            }
            fmt.Printf("Notification on %q: %s\n", n.Channel, n.Extra)
        }
    }()
}

// Pattern 7: Error handling with pq.As
func errorHandling(db *sql.DB) {
    _, err := db.Exec("INSERT INTO users (email) VALUES ($1)", "existing@example.com")
    if err != nil {
        if pqErr := pq.As(err, pqerror.UniqueViolation); pqErr != nil {
            fmt.Printf("Duplicate email: %s\n", pqErr.ErrorWithDetail())
        } else {
            log.Fatal(err)
        }
    }
}

// Pattern 8: Transaction with error handling
func transactionExample(db *sql.DB) {
    tx, err := db.Begin()
    if err != nil {
        log.Fatal(err)
    }
    defer tx.Rollback()

    _, err = tx.Exec("UPDATE accounts SET balance = balance - $1 WHERE id = $2", 100, 1)
    if err != nil {
        log.Fatal(err)
    }

    _, err = tx.Exec("UPDATE accounts SET balance = balance + $1 WHERE id = $2", 100, 2)
    if err != nil {
        log.Fatal(err)
    }

    if err := tx.Commit(); err != nil {
        log.Fatal(err)
    }
}
```