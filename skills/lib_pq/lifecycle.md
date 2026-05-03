# Lifecycle

### Construction
```go
// Method 1: Using DSN string
db, err := sql.Open("postgres", "host=localhost dbname=mydb user=postgres password=secret sslmode=disable")
if err != nil {
    log.Fatal(err)
}

// Method 2: Using pq.Config
cfg := &pq.Config{
    Host:     "localhost",
    Port:     5432,
    Database: "mydb",
    User:     "postgres",
    Password: "secret",
}

connector, err := pq.NewConnectorConfig(cfg)
if err != nil {
    log.Fatal(err)
}
db := sql.OpenDB(connector)

// Method 3: From DSN string with Config
cfg, err := pq.NewConfig("host=localhost dbname=mydb user=postgres")
if err != nil {
    log.Fatal(err)
}
cfg.Password = os.Getenv("DB_PASSWORD")
connector, err := pq.NewConnectorConfig(cfg)
if err != nil {
    log.Fatal(err)
}
db := sql.OpenDB(connector)
```

### Destruction
```go
// Always close the database when done
func processData() error {
    db, err := sql.Open("postgres", "host=localhost dbname=mydb")
    if err != nil {
        return err
    }
    defer db.Close() // Ensures all connections are returned and pool is closed

    // Use the database...
    return nil
}

// For long-running applications, use defer in main
func main() {
    db, err := createDB()
    if err != nil {
        log.Fatal(err)
    }
    defer db.Close()

    // Application logic...
}
```

### Move Semantics
```go
// sql.DB is designed to be passed by value (it's a handle)
func createAndUseDB() {
    db := createDB() // Returns *sql.DB
    defer db.Close()

    // Pass to other functions
    processUsers(db)
    processOrders(db)
}

func processUsers(db *sql.DB) {
    // db is safe to use here
    rows, err := db.Query("SELECT * FROM users")
    if err != nil {
        log.Fatal(err)
    }
    defer rows.Close()
    // ...
}

// For dependency injection
type Repository struct {
    db *sql.DB
}

func NewRepository(db *sql.DB) *Repository {
    return &Repository{db: db}
}
```

### Resource Management
```go
// Proper resource cleanup in complex operations
func complexOperation(db *sql.DB) error {
    // Transaction lifecycle
    tx, err := db.Begin()
    if err != nil {
        return err
    }
    defer tx.Rollback() // Clean up if we don't commit

    // Prepared statement lifecycle
    stmt, err := tx.Prepare("INSERT INTO users (name) VALUES ($1) RETURNING id")
    if err != nil {
        return err
    }
    defer stmt.Close() // Clean up prepared statement

    // Row lifecycle
    var id int
    err = stmt.QueryRow("Alice").Scan(&id)
    if err != nil {
        return err
    }

    // Commit transaction
    return tx.Commit()
}

// Connection lifecycle management
func getConnectionWithTimeout(db *sql.DB, timeout time.Duration) (*sql.Conn, error) {
    ctx, cancel := context.WithTimeout(context.Background(), timeout)
    defer cancel()

    conn, err := db.Conn(ctx)
    if err != nil {
        return nil, err
    }
    // Caller must close this connection
    return conn, nil
}
```