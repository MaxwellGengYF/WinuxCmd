**Pitfall 1: Forgetting to Sync the Logger**

```go
// BAD: Missing Sync() causes lost log entries on exit
logger, _ := zap.NewProduction()
logger.Info("important message")
// Logger is garbage collected without flushing buffer

// GOOD: Always defer Sync()
logger, _ := zap.NewProduction()
defer logger.Sync() // Ensures buffer is flushed
logger.Info("important message")
```

**Pitfall 2: Ignoring Error from NewProduction**

```go
// BAD: Ignoring error can mask configuration issues
logger, _ := zap.NewProduction()

// GOOD: Handle error properly
logger, err := zap.NewProduction()
if err != nil {
    panic("failed to initialize zap: " + err.Error())
}
defer logger.Sync()
```

**Pitfall 3: Using SugaredLogger in Hot Paths**

```go
// BAD: SugaredLogger allocates more in hot paths
for i := 0; i < 1000000; i++ {
    sugar.Infof("iteration %d", i) // 10 allocs/op
}

// GOOD: Use Logger with typed fields in hot paths
for i := 0; i < 1000000; i++ {
    logger.Info("iteration", zap.Int("i", i)) // 5 allocs/op
}
```

**Pitfall 4: Mixing Logger and SugaredLogger APIs**

```go
// BAD: Inconsistent API usage
sugar := logger.Sugar()
sugar.Info("message", zap.String("key", "value")) // Wrong: SugaredLogger doesn't take Fields

// GOOD: Use appropriate API
sugar.Infow("message", "key", "value") // SugaredLogger uses key-value pairs
logger.Info("message", zap.String("key", "value")) // Logger uses Fields
```

**Pitfall 5: Not Using With() for Repeated Fields**

```go
// BAD: Repeating fields in every log call
logger.Info("request", zap.String("service", "api"), zap.String("user", "alice"))
logger.Error("error", zap.String("service", "api"), zap.String("user", "alice"))

// GOOD: Create logger with common fields
requestLogger := logger.With(
    zap.String("service", "api"),
    zap.String("user", "alice"),
)
requestLogger.Info("request")
requestLogger.Error("error")
```

**Pitfall 6: Using Fatal or Panic Without Cleanup**

```go
// BAD: Fatal exits immediately, skipping deferred Sync
logger, _ := zap.NewProduction()
defer logger.Sync()
logger.Fatal("critical error") // Exits before Sync()

// GOOD: Use Error and handle exit manually
logger, _ := zap.NewProduction()
defer logger.Sync()
logger.Error("critical error")
os.Exit(1) // Sync() runs before exit
```
