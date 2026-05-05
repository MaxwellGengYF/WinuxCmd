**Condition 1: NEVER ignore the error from NewProduction/NewDevelopment**

```go
// BAD: Silently ignoring initialization error
logger, _ := zap.NewProduction()

// GOOD: Always check and handle initialization error
logger, err := zap.NewProduction()
if err != nil {
    log.Fatalf("failed to initialize zap: %v", err)
}
defer logger.Sync()
```

**Condition 2: NEVER call Sync() on a nil logger**

```go
// BAD: Nil logger causes panic on Sync()
var logger *zap.Logger
defer logger.Sync() // PANIC: nil pointer dereference

// GOOD: Check for nil before Sync()
if logger != nil {
    defer logger.Sync()
}
```

**Condition 3: NEVER use SugaredLogger after Logger is closed**

```go
// BAD: Using sugar after logger is invalidated
logger, _ := zap.NewProduction()
sugar := logger.Sugar()
logger.Sync()
sugar.Info("after sync") // Undefined behavior

// GOOD: Keep logger alive while sugar is in use
logger, _ := zap.NewProduction()
sugar := logger.Sugar()
sugar.Info("before sync")
logger.Sync()
```

**Condition 4: NEVER pass nil to zap.Error()**

```go
// BAD: nil error creates empty field
var err error = nil
logger.Error("message", zap.Error(err)) // Creates empty error field

// GOOD: Only log non-nil errors
if err != nil {
    logger.Error("message", zap.Error(err))
}
```

**Condition 5: NEVER modify Fields after passing to logger**

```go
// BAD: Modifying field after passing to logger
field := zap.String("key", "original")
logger.Info("message", field)
field.String = "modified" // Undefined behavior

// GOOD: Create new fields for each log call
logger.Info("message", zap.String("key", "original"))
logger.Info("message", zap.String("key", "modified"))
```
