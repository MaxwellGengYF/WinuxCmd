**Zero-allocation logging with Logger**

```go
package main

import (
    "go.uber.org/zap"
)

func main() {
    logger, _ := zap.NewProduction()
    defer logger.Sync()

    // Logger allocates 0 objects for static messages
    logger.Info("static message") // 0 allocs/op

    // Logger allocates 5 objects for message with 10 fields
    logger.Info("dynamic message",
        zap.String("key1", "value1"),
        zap.Int("key2", 42),
        zap.Bool("key3", true),
    ) // 5 allocs/op
}
```

**SugaredLogger performance trade-off**

```go
package main

import (
    "go.uber.org/zap"
)

func main() {
    logger, _ := zap.NewProduction()
    defer logger.Sync()
    sugar := logger.Sugar()

    // SugaredLogger allocates 1 object for static messages
    sugar.Info("static message") // 1 alloc/op

    // SugaredLogger allocates 10 objects for message with 10 fields
    sugar.Infow("dynamic message",
        "key1", "value1",
        "key2", 42,
        "key3", true,
    ) // 10 allocs/op
}
```

**GC considerations: Avoid allocations in hot paths**

```go
package main

import (
    "go.uber.org/zap"
)

// BAD: Allocates in hot path
func processItemBad(logger *zap.Logger, id int) {
    logger.Info("processing",
        zap.Int("id", id),
        zap.String("status", getStatus(id)), // Allocates string
    )
}

// GOOD: Pre-allocate and reuse
var statusCache = make(map[int]string)

func processItemGood(logger *zap.Logger, id int) {
    status := getCachedStatus(id)
    logger.Info("processing",
        zap.Int("id", id),
        zap.String("status", status), // No allocation
    )
}

func getStatus(id int) string {
    return "active"
}

func getCachedStatus(id int) string {
    if s, ok := statusCache[id]; ok {
        return s
    }
    s := getStatus(id)
    statusCache[id] = s
    return s
}
```

**Memory allocation comparison**

```go
package main

import (
    "go.uber.org/zap"
)

func main() {
    logger, _ := zap.NewProduction()
    defer logger.Sync()
    sugar := logger.Sugar()

    // Logger: 5 allocs/op for 10 fields
    for i := 0; i < 1000; i++ {
        logger.Info("message",
            zap.Int("field1", i),
            zap.Int("field2", i*2),
        )
    }

    // SugaredLogger: 10 allocs/op for 10 fields
    for i := 0; i < 1000; i++ {
        sugar.Infow("message",
            "field1", i,
            "field2", i*2,
        )
    }
}
```
