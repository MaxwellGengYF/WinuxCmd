**Goroutine safety: Logger is safe for concurrent use**

```go
package main

import (
    "sync"
    "go.uber.org/zap"
)

func main() {
    logger, _ := zap.NewProduction()
    defer logger.Sync()

    var wg sync.WaitGroup
    for i := 0; i < 10; i++ {
        wg.Add(1)
        go func(id int) {
            defer wg.Done()
            // Logger is goroutine-safe
            logger.Info("goroutine running",
                zap.Int("goroutine_id", id),
            )
        }(i)
    }
    wg.Wait()
}
```

**Atomic level for dynamic configuration changes**

```go
package main

import (
    "sync"
    "go.uber.org/zap"
    "go.uber.org/zap/zapcore"
)

func main() {
    atomicLevel := zap.NewAtomicLevel()
    atomicLevel.SetLevel(zapcore.InfoLevel)

    logger := zap.New(zapcore.NewCore(
        zapcore.NewJSONEncoder(zap.NewProductionEncoderConfig()),
        zapcore.AddSync(os.Stdout),
        atomicLevel,
    ))
    defer logger.Sync()

    var wg sync.WaitGroup
    wg.Add(2)

    // Goroutine 1: Log messages
    go func() {
        defer wg.Done()
        for i := 0; i < 100; i++ {
            logger.Info("message", zap.Int("i", i))
        }
    }()

    // Goroutine 2: Change log level dynamically
    go func() {
        defer wg.Done()
        atomicLevel.SetLevel(zapcore.DebugLevel)
    }()

    wg.Wait()
}
```

**Channel-based logging with buffered writes**

```go
package main

import (
    "go.uber.org/zap"
)

type LogWorker struct {
    logCh chan *zap.Logger
    logger *zap.Logger
}

func NewLogWorker(logger *zap.Logger) *LogWorker {
    lw := &LogWorker{
        logCh:  make(chan *zap.Logger, 100),
        logger: logger,
    }
    go lw.processLogs()
    return lw
}

func (lw *LogWorker) processLogs() {
    for l := range lw.logCh {
        l.Sync()
    }
}

func (lw *LogWorker) LogAsync(msg string, fields ...zap.Field) {
    // Create a copy of logger for async logging
    l := lw.logger.With(fields...)
    l.Info(msg)
    select {
    case lw.logCh <- l:
    default:
        // Drop if channel is full
    }
}
```

**Sync.Pool for reusing loggers in high-concurrency scenarios**

```go
package main

import (
    "sync"
    "go.uber.org/zap"
)

type RequestContext struct {
    logger *zap.Logger
    requestID string
}

var contextPool = sync.Pool{
    New: func() interface{} {
        return &RequestContext{}
    },
}

func main() {
    logger, _ := zap.NewProduction()
    defer logger.Sync()

    var wg sync.WaitGroup
    for i := 0; i < 100; i++ {
        wg.Add(1)
        go func(id int) {
            defer wg.Done()
            ctx := contextPool.Get().(*RequestContext)
            ctx.logger = logger.With(zap.Int("request_id", id))
            ctx.requestID = string(rune(id))

            ctx.logger.Info("processing request")
            contextPool.Put(ctx)
        }(i)
    }
    wg.Wait()
}
```
